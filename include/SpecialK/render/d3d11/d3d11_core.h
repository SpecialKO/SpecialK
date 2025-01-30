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

#include <cinttypes>
#include <algorithm>
#include <memory>
#include <atomic>
#include <mutex>
#include <stack>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <atlbase.h>

#include <SpecialK/render/d3d11/d3d11_interfaces.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <d3dcompiler.h>
//#include <../depends/include/DXSDK/D3DX11.h>
//#include <../depends/include/DXSDK/D3DX11tex.h>

extern volatile LONG SK_D3D11_initialized;

enum class SK_D3D11_ShaderType {
  Vertex   =  1,
  Pixel    =  2,
  Geometry =  4,
  Domain   =  8,
  Hull     = 16,
  Compute  = 32,

  Invalid  = MAXINT
};

#if 1
bool SK_D3D11_IsDevCtxDeferred(ID3D11DeviceContext* ctx);
#else
#define SK_D3D11_IsDevCtxDeferred(ctx) false
#endif

// Thanks to some particularly awful JRPGs, Special K now hooks and wraps some
//   APIs simultaneously and needs to know whether the wrapped calls will
//     reliably pass-through the hook or if processing needs to be done
//       separately, so that's what this tally whacker does.
class SK_D3D11_HookTallyWhacker
{
public:
  bool is_whack (void) noexcept
  {
    if (whack != 0)
 return whack >  0 ? true : false;
    if   (                  dwHookCalls < 500 ) return true;
    else
    {
      if ( dwHookCalls < dwWrappedCalls - 500 ) whack =  1;
      else                                      whack = -1;

      return true;
    }

  };
  int hooked  (int whacks = 1) noexcept { return ( dwHookCalls    += whacks ); };
  int wrapped (int whacks = 1) noexcept { return ( dwWrappedCalls += whacks ); };

// Call tallys
protected:
  int   whack          = 0;
  DWORD dwHookCalls    = 0,
        dwWrappedCalls = 0;
}; // This looks stupid as hell, but it's much quicker than querying the
   //   device context's private data or getting TLS on every draw call.


#define SK_WRAP_AND_HOOK                                  \
  static SK_D3D11_HookTallyWhacker                        \
                 call_tally;                              \
  /* Ugly caching mechanism, but effective. */            \
  static std::pair<ID3D11DeviceContext*,bool>             \
                                      _lastDeferredCheck0;\
  static std::pair<ID3D11DeviceContext*,bool>             \
                                      _lastDeferredCheck1;\
  static std::pair<ID3D11DeviceContext*,bool>             \
                                      _lastDeferredCheck2;\
  static std::pair<ID3D11DeviceContext*,bool>             \
                                      _lastDeferredCheck3;\
  static std::pair<ID3D11DeviceContext*,bool>             \
                                      _lastDeferredCheck4;\
  static std::pair<ID3D11DeviceContext*,bool>             \
                                      _lastDeferredCheck5;\
  static std::pair<ID3D11DeviceContext*,bool>             \
                                      _lastDeferredCheck6;\
  static std::pair<ID3D11DeviceContext*,bool>             \
                                      _lastDeferredCheck7;\
  static std::pair<ID3D11DeviceContext*,bool>*            \
    _pLastDeferredCheck        = &_lastDeferredCheck0;    \
                                                          \
  const bool bIsLastDevCtx     =                          \
    _pLastDeferredCheck->first == pDevCtx;                \
  const bool bIsDevCtxDeferred = bIsLastDevCtx ?          \
    _pLastDeferredCheck->second                :          \
    SK_D3D11_IsDevCtxDeferred (pDevCtx);                  \
                                                          \
    _pLastDeferredCheck->second = bIsDevCtxDeferred;      \
    _pLastDeferredCheck->first  = pDevCtx;                \
                                                          \
    if (      _pLastDeferredCheck == &_lastDeferredCheck0)\
              _pLastDeferredCheck  = &_lastDeferredCheck1;\
    else if ( _pLastDeferredCheck == &_lastDeferredCheck1)\
              _pLastDeferredCheck  = &_lastDeferredCheck2;\
    else if ( _pLastDeferredCheck == &_lastDeferredCheck2)\
              _pLastDeferredCheck  = &_lastDeferredCheck3;\
    else if ( _pLastDeferredCheck == &_lastDeferredCheck3)\
              _pLastDeferredCheck  = &_lastDeferredCheck4;\
    else if ( _pLastDeferredCheck == &_lastDeferredCheck4)\
              _pLastDeferredCheck  = &_lastDeferredCheck5;\
    else if ( _pLastDeferredCheck == &_lastDeferredCheck5)\
              _pLastDeferredCheck  = &_lastDeferredCheck6;\
    else if ( _pLastDeferredCheck == &_lastDeferredCheck6)\
              _pLastDeferredCheck  = &_lastDeferredCheck7;\
    else      _pLastDeferredCheck  = &_lastDeferredCheck0;\
                                                          \
  bool bMustNotIgnore =                                   \
    true;                                                 \
                                                          \
  if (config.render.d3d11.wrap_d3d11_dev_ctx) {           \
    std::ignore = bIsDevCtxDeferred;                      \
    if (true)/*! bIsDevCtxDeferred)*/                     \
    {                                                     \
      call_tally.hooked  ( bWrapped ? 0 : 1 );            \
      call_tally.wrapped ( bWrapped ? 1 : 0 );            \
    }                                                     \
    bMustNotIgnore =                                      \
      ( call_tally.is_whack () ||                         \
              (! bWrapped) );                             \
  }


__forceinline
bool
SK_D3D11_IsDevCtxDeferred (ID3D11DeviceContext* ctx)
{
  // Ugly caching mechanism, but effective.
  static std::pair<ID3D11DeviceContext*,bool>
                                      _lastDeferredCheck0;
  static std::pair<ID3D11DeviceContext*,bool>
                                      _lastDeferredCheck1;
  static std::pair<ID3D11DeviceContext*,bool>
                                      _lastDeferredCheck2;
  static std::pair<ID3D11DeviceContext*,bool>
                                      _lastDeferredCheck3;
  static std::pair<ID3D11DeviceContext*,bool>
                                      _lastDeferredCheck4;
  static std::pair<ID3D11DeviceContext*,bool>
                                      _lastDeferredCheck5;
  static std::pair<ID3D11DeviceContext*,bool>
                                      _lastDeferredCheck6;
  static std::pair<ID3D11DeviceContext*,bool>
                                      _lastDeferredCheck7;
  static std::pair<ID3D11DeviceContext*,bool>*
    _pLastDeferredCheck        = &_lastDeferredCheck0;

  const bool bIsLastDevCtx     =
    _pLastDeferredCheck->first == ctx;
  const bool bIsDevCtxDeferred = bIsLastDevCtx ?
    _pLastDeferredCheck->second               :
    ctx->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED;

  _pLastDeferredCheck->second = bIsDevCtxDeferred;
  _pLastDeferredCheck->first  = ctx;

    if (      _pLastDeferredCheck == &_lastDeferredCheck0)
              _pLastDeferredCheck  = &_lastDeferredCheck1;
    else if ( _pLastDeferredCheck == &_lastDeferredCheck1)
              _pLastDeferredCheck  = &_lastDeferredCheck2;
    else if ( _pLastDeferredCheck == &_lastDeferredCheck2)
              _pLastDeferredCheck  = &_lastDeferredCheck3;
    else if ( _pLastDeferredCheck == &_lastDeferredCheck3)
              _pLastDeferredCheck  = &_lastDeferredCheck4;
    else if ( _pLastDeferredCheck == &_lastDeferredCheck4)
              _pLastDeferredCheck  = &_lastDeferredCheck5;
    else if ( _pLastDeferredCheck == &_lastDeferredCheck5)
              _pLastDeferredCheck  = &_lastDeferredCheck6;
    else if ( _pLastDeferredCheck == &_lastDeferredCheck6)
              _pLastDeferredCheck  = &_lastDeferredCheck7;
    else      _pLastDeferredCheck  = &_lastDeferredCheck0;

  return bIsDevCtxDeferred;
}

bool
SK_D3D11_EnsureMatchingDevices (ID3D11Device *pDevice0, ID3D11Device *pDevice1);

inline
bool
SK_D3D11_IgnoreWrappedOrDeferred ( bool                 bWrapped,
                                   bool                 bDeferred,
                                   ID3D11DeviceContext* pDevCtx )
{
  if (!config.render.dxgi.deferred_isolation) [[likely]]
  {
    if (bWrapped) [[likely]]
      return true;

    if (bDeferred) [[unlikely]]
      return true;
  }

  // Ugly caching mechanism, but effective.
  static std::pair<ID3D11DeviceContext*,bool>  _lastVerdict0;
  static std::pair<ID3D11DeviceContext*,bool>  _lastVerdict1;
  static std::pair<ID3D11DeviceContext*,bool>  _lastVerdict2;
  static std::pair<ID3D11DeviceContext*,bool>  _lastVerdict3;
  static std::pair<ID3D11DeviceContext*,bool>  _lastVerdict4;
  static std::pair<ID3D11DeviceContext*,bool>  _lastVerdict5;
  static std::pair<ID3D11DeviceContext*,bool>  _lastVerdict6;
  static std::pair<ID3D11DeviceContext*,bool>  _lastVerdict7;
  static std::pair<ID3D11DeviceContext*,bool>* _pLastVerdict = &_lastVerdict0;

  std::pair<ID3D11DeviceContext*,bool>* pLastVerdict = _pLastVerdict;

  if (     pLastVerdict->first == pDevCtx)
    return pLastVerdict->second;
  else
  {
    if (      _pLastVerdict == &_lastVerdict0)
              _pLastVerdict  = &_lastVerdict1;
    else if ( _pLastVerdict == &_lastVerdict1)
              _pLastVerdict  = &_lastVerdict2;
    else if ( _pLastVerdict == &_lastVerdict2)
              _pLastVerdict  = &_lastVerdict3;
    else if ( _pLastVerdict == &_lastVerdict3)
              _pLastVerdict  = &_lastVerdict4;
    else if ( _pLastVerdict == &_lastVerdict4)
              _pLastVerdict  = &_lastVerdict5;
    else if ( _pLastVerdict == &_lastVerdict5)
              _pLastVerdict  = &_lastVerdict6;
    else if ( _pLastVerdict == &_lastVerdict6)
              _pLastVerdict  = &_lastVerdict7;
    else      _pLastVerdict  = &_lastVerdict0;

    pLastVerdict = _pLastVerdict;
  }

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ((! bWrapped) && ( rb.d3d11.immediate_ctx == nullptr || (pDevCtx != rb.d3d11.immediate_ctx && !config.reshade.is_addon))) [[unlikely]]
  {
    if ( rb.d3d11.immediate_ctx == nullptr ||
         rb.device.p            == nullptr )
    {
      if (rb.swapchain.p != nullptr)
      {
        if (config.system.log_level > 2)
        {
          SK_ReleaseAssert (!"Hooked command ignored while render backend is uninitialized");
        }

        pLastVerdict->first = nullptr;

        return true;
      }
    }

#if 1
    // Ugly, but effective, optimization
           ID3D11Device*        pDev        = nullptr;
    static ID3D11Device*        pLastDev    = nullptr;
    static ID3D11DeviceContext* pLastDevCtx = nullptr;

    if (pLastDevCtx == pDevCtx)
            pDev = pLastDev;
    else
    {
      SK_ComPtr <ID3D11Device>
                           pDevice;
      pDevCtx->GetDevice (&pDevice.p);
                pLastDev = pDevice.p;
                pDev     = pDevice.p;
    }

    pLastDevCtx = pDevCtx;
#else
    SK_ComPtr <ID3D11Device>
                         pDev;
    pDevCtx->GetDevice (&pDev.p);
#endif

    // TOO HIGH OVERHEAD: Use direct compare and expect a few misses
    //if (! rb.getDevice <ID3D11Device> ().IsEqualObject (pDevice))
    if (rb.device.p != pDev && (! config.reshade.is_addon))
    {
      if (rb.device.p == nullptr)
      {
        rb.device = pDev;
      }

      else if (! SK_D3D11_EnsureMatchingDevices ((ID3D11Device *)rb.device.p, pDev))
      {
        if (config.system.log_level > 2)
        {
          SK_ReleaseAssert (!"Hooked command ignored because it happened on the wrong device");
        }

        pLastVerdict->first  = pDevCtx;
        pLastVerdict->second = true;

        return true;
      }
    }
  }


  //
  // Handle D3D11On12, but only if the active API is not already D3D11 :)
  //
  if (rb.api != SK_RenderAPI::D3D11)
  {
#if 1
           ID3D11Device*        pDev        = nullptr;
    static ID3D11Device*        pLastDev    = nullptr;
    static ID3D11DeviceContext* pLastDevCtx = nullptr;

    if (pLastDevCtx == pDevCtx)
            pDev = pLastDev;
    else
    {
      SK_ComPtr <ID3D11Device>
                           pDevice;
      pDevCtx->GetDevice (&pDevice.p);
                pLastDev = pDevice.p;
                pDev     = pDevice.p;
    }

    pLastDevCtx = pDevCtx;
#else
    SK_ComPtr <ID3D11Device>
                         pDev;
    pDevCtx->GetDevice (&pDev.p);
#endif

    SK_ComPtr <IUnknown> pD3D11On12Device;
    if (pDev)
        pDev->QueryInterface (
          IID_ID3D11On12Device,
    (void **)&pD3D11On12Device.p);

    if (pD3D11On12Device.p != nullptr)
    {
    //SK_ReleaseAssert (!"D3D11On12");
      pLastVerdict->first  = pDevCtx;
      pLastVerdict->second = true;
      return true;
    }
  }

  pLastVerdict->first  = pDevCtx;
  pLastVerdict->second = false;

  return
    false;
}

extern const GUID IID_ID3D11Device2;
extern const GUID IID_ID3D11Device3;
extern const GUID IID_ID3D11Device4;
extern const GUID IID_ID3D11Device5;

// {9A222196-4D44-45C3-AAA4-2FD47915CC70}
extern const GUID IID_IUnwrappedD3D11DeviceContext;

// {BAC138B1-7D7F-4CB9-A2D3-BED2DAEFE24C}
extern const GUID IID_IUnwrappedD3D11RenderTargetView;

struct __declspec (uuid ("9A222196-4D44-45C3-AAA4-2FD47915CC70"))
                      IUnwrappedD3D11DeviceContext;

// {4F5D4B49-730F-4BFA-A6A4-0C82BF114001}
static constexpr GUID IID_ITrackThisD3D11Device =
{ 0x4f5d4b49, 0x730f, 0x4bfa, { 0xa6, 0xa4, 0xc, 0x82, 0xbf, 0x11, 0x40, 0x1 } };





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

__declspec (noinline)
HRESULT
WINAPI
D3D11CoreCreateDevice_Detour (
          IDXGIFactory       *pFactory,
          IDXGIAdapter       *pAdapter,
          D3D_DRIVER_TYPE     DriverType,
          HINSTANCE           Software,
          UINT                Flags,
    const D3D_FEATURE_LEVEL  *pFeatureLevels,
          UINT                FeatureLevels,
          INT                 SDKVersion,
          ID3D11Device      **ppDevice,
          D3D_FEATURE_LEVEL  *pFeatureLevel );

extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_vs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_ps;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_gs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_hs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_ds;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_cs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_mmio;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_render_view;

void
SK_D3D11_MergeCommandLists ( ID3D11DeviceContext *pSurrogate,
                             ID3D11DeviceContext *pMerge );

void
SK_D3D11_ResetContextState ( ID3D11DeviceContext *pDevCtx,
                             UINT                  dev_idx = UINT_MAX );

extern std::pair <BOOL*, BOOL>
SK_ImGui_FlagDrawing_OnD3D11Ctx (UINT dev_idx);
extern bool
SK_ImGui_IsDrawing_OnD3D11Ctx   (UINT& dev_idx, ID3D11DeviceContext* pDevCtx);

extern thread_local bool SK_D3D11_ApplyingStateBlock;


struct shader_stage_s
{
  using bind_fn =
    void (shader_stage_s::*)(int, int, ID3D11ShaderResourceView*);

  void nulBind (int slot, ID3D11ShaderResourceView* pView)
  {
    IUnknown_Set ((IUnknown **)&skipped_bindings [slot],
                  (IUnknown  *)pView);
  };

  void Bind (int slot, ID3D11ShaderResourceView* pView)
  {
    IUnknown_AtomicRelease ((void **)&skipped_bindings [slot]);

    // The D3D11 Runtime is holding a reference if this is non-null.
    real_bindings [slot] = pView;
  };

  // We have to hold references to these things, because the D3D11
  //   runtime would have if they were bound to the pipeline, and the
  //     solitary pipeline reference may be keeping the resource alive!
  std::array < ID3D11ShaderResourceView*,
               D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT >
  skipped_bindings = { };

  std::array < ID3D11ShaderResourceView*,
               D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT >
  real_bindings    = { };
};

uint32_t
__cdecl
SK_D3D11_ChecksumShaderBytecode (
  _In_ const void   *pShaderBytecode,
  _In_       SIZE_T  BytecodeLength  );

std::string
SK_D3D11_DescribeResource (ID3D11Resource* pRes);


HRESULT
STDMETHODCALLTYPE
SK_D3D11Dev_CreateRenderTargetView_Impl (
  _In_            ID3D11Device                   *pDev,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView,
                  BOOL                            bWrapped );

HRESULT
STDMETHODCALLTYPE
SK_D3D11Dev_CreateRenderTargetView1_Impl (
  _In_            ID3D11Device3                  *pDev,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc,
  _Out_opt_       ID3D11RenderTargetView1       **ppRTView,
                  BOOL                            bWrapped );

HRESULT
WINAPI
D3D11Dev_CreateTexture2DCore_Impl (
  _In_              ID3D11Device            *This,
  _Inout_opt_       D3D11_TEXTURE2D_DESC    *pDesc0,
  _Inout_opt_       D3D11_TEXTURE2D_DESC1   *pDesc1,
  _In_opt_    const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_         ID3D11Texture2D        **ppTexture2D0,
  _Out_opt_         ID3D11Texture2D1       **ppTexture2D1,
                    LPVOID                   lpCallerAddr,
                    SK_TLS                  *pTLS = nullptr );

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
   UINT                 dev_idx = UINT_MAX );

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
                 BOOL                 bWrapped,
                 LPCVOID              pCallerAddr );

void
STDMETHODCALLTYPE
SK_D3D11_ResolveSubresource_Impl (
    _In_ ID3D11DeviceContext *pDevCtx,
    _In_ ID3D11Resource      *pDstResource,
    _In_ UINT                 DstSubresource,
    _In_ ID3D11Resource      *pSrcResource,
    _In_ UINT                 SrcSubresource,
    _In_ DXGI_FORMAT          Format,
         BOOL                 bWrapped );

HRESULT
STDMETHODCALLTYPE
SK_D3D11_Map_Impl (
  _In_      ID3D11DeviceContext      *pDevCtx,
  _In_      ID3D11Resource           *pResource,
  _In_      UINT                      Subresource,
  _In_      D3D11_MAP                 MapType,
  _In_      UINT                      MapFlags,
  _Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource,
            BOOL                      bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_Unmap_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource,
       BOOL                 bWrapped );

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
                 BOOL            bWrapped );
void
STDMETHODCALLTYPE
SK_D3D11_CopyResource_Impl (
       ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource,
       BOOL                 bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_DrawAuto_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
       BOOL                 bWrapped,
       UINT                 dev_idx = UINT_MAX );

void
STDMETHODCALLTYPE
SK_D3D11_Draw_Impl (
  ID3D11DeviceContext* pDevCtx,
  UINT                 VertexCount,
  UINT                 StartVertexLocation,
  bool                 Wrapped = false,
  UINT                 dev_idx = UINT_MAX );

void
STDMETHODCALLTYPE
SK_D3D11_DrawIndexed_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
       BOOL                 bWrapped,
       UINT                 dev_idx = UINT_MAX );

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
       UINT                 dev_idx = UINT_MAX );

void
STDMETHODCALLTYPE
SK_D3D11_DrawIndexedInstancedIndirect_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs,
       BOOL                 bWrapped,
       UINT                 dev_idx = UINT_MAX );

void
STDMETHODCALLTYPE
SK_D3D11_DrawInstanced_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation,
       BOOL                 bWrapped,
       UINT                 dev_idx = UINT_MAX );

void
STDMETHODCALLTYPE
SK_D3D11_DrawInstancedIndirect_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs,
       BOOL                 bWrapped,
       UINT                 dev_idx = UINT_MAX );

void
STDMETHODCALLTYPE
SK_D3D11_OMSetRenderTargets_Impl (
         ID3D11DeviceContext           *pDevCtx,
_In_     UINT                           NumViews,
_In_opt_ ID3D11RenderTargetView *const *ppRenderTargetViews,
_In_opt_ ID3D11DepthStencilView        *pDepthStencilView,
         BOOL                           bWrapped,
         UINT                           dev_idx = UINT_MAX );

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
                                              UINT                              dev_idx = UINT_MAX );


void
STDMETHODCALLTYPE
SK_D3D11_Dispatch_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 ThreadGroupCountX,
  _In_ UINT                 ThreadGroupCountY,
  _In_ UINT                 ThreadGroupCountZ,
       BOOL                 bWrapped,
       UINT                 dev_idx );

void
STDMETHODCALLTYPE
SK_D3D11_DispatchIndirect_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs,
       BOOL                 bWrapped,
       UINT                 dev_idx );

bool
SK_D3D11_DispatchHandler (
  ID3D11DeviceContext* pDevCtx,
  UINT&                dev_idx,
  SK_TLS**             ppTLS = nullptr );

void
SK_D3D11_PostDispatch (
  ID3D11DeviceContext* pDevCtx,
  UINT&                dev_idx,
  SK_TLS*              pTLS = SK_TLS_Bottom () );




#if 1
HRESULT
WINAPI
D3D11Dev_CreateRasterizerState_Override (
  ID3D11Device            *This,
  const D3D11_RASTERIZER_DESC   *pRasterizerDesc,
  ID3D11RasterizerState  **ppRasterizerState );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateSamplerState_Override
(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateBuffer_Override (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateTexture2D1_Override (
  _In_            ID3D11Device3          *This,
  _In_      const D3D11_TEXTURE2D_DESC1  *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D1       **ppTexture2D );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateRenderTargetView_Override (
  _In_            ID3D11Device                   *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateRenderTargetView1_Override (
  _In_            ID3D11Device3                  *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC1 *pDesc,
  _Out_opt_       ID3D11RenderTargetView1       **ppRTView );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateShaderResourceView_Override (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateShaderResourceView1_Override (
  _In_           ID3D11Device3                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC1  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView1        **ppSRView );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDepthStencilView_Override (
  _In_            ID3D11Device                  *This,
  _In_            ID3D11Resource                *pResource,
  _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11DepthStencilView        **ppDepthStencilView );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateUnorderedAccessView_Override (
  _In_            ID3D11Device                     *This,
  _In_            ID3D11Resource                   *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView       **ppUAView );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateUnorderedAccessView1_Override (
  _In_            ID3D11Device3                    *This,
  _In_            ID3D11Resource                   *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC1 *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView1       **ppUAView );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateVertexShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11VertexShader **ppVertexShader );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreatePixelShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11PixelShader  **ppPixelShader );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateGeometryShader_Override (
  _In_            ID3D11Device          *This,
  _In_      const void                  *pShaderBytecode,
  _In_            SIZE_T                 BytecodeLength,
  _In_opt_        ID3D11ClassLinkage    *pClassLinkage,
  _Out_opt_       ID3D11GeometryShader **ppGeometryShader );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
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
  _Out_opt_       ID3D11GeometryShader      **ppGeometryShader );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateHullShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11HullShader   **ppHullShader );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDomainShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11DomainShader **ppDomainShader );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateComputeShader_Override (
  _In_            ID3D11Device         *This,
  _In_      const void                 *pShaderBytecode,
  _In_            SIZE_T                BytecodeLength,
  _In_opt_        ID3D11ClassLinkage   *pClassLinkage,
  _Out_opt_       ID3D11ComputeShader **ppComputeShader );

interface ID3D11Device2;
interface ID3D11Device3;

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext_Override (
  _In_            ID3D11Device         *This,
  _In_            UINT                  ContextFlags,
  _Out_opt_       ID3D11DeviceContext **ppDeferredContext);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext1_Override (
  _In_            ID3D11Device1         *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext1 **ppDeferredContext1);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext2_Override (
  _In_            ID3D11Device2         *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext2 **ppDeferredContext2);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext3_Override (
  _In_            ID3D11Device3         *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext3 **ppDeferredContext3);

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext_Override (
  _In_            ID3D11Device         *This,
  _Out_           ID3D11DeviceContext **ppImmediateContext);

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext1_Override (
  _In_            ID3D11Device1        *This,
  _Out_           ID3D11DeviceContext1 **ppImmediateContext1);

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext2_Override (
  _In_            ID3D11Device2         *This,
  _Out_           ID3D11DeviceContext2 **ppImmediateContext2);

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext3_Override (
  _In_            ID3D11Device3         *This,
  _Out_           ID3D11DeviceContext3 **ppImmediateContext3);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetScissorRects_Override (
        ID3D11DeviceContext *This,
        UINT                 NumRects,
  const D3D11_RECT          *pRects);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetViewports_Override (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetConstantBuffers_Override (
  ID3D11DeviceContext*  This,
  UINT                  StartSlot,
  UINT                  NumBuffers,
  ID3D11Buffer *const  *ppConstantBuffers );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetConstantBuffers_Override (
  ID3D11DeviceContext*  This,
  UINT                  StartSlot,
  UINT                  NumBuffers,
  ID3D11Buffer *const  *ppConstantBuffers );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetUnorderedAccessViews_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumUAVs,
  _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_opt_ const UINT                             *pUAVInitialCounts );

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
  _In_           UINT                 SrcDepthPitch);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexed_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawAuto_Override (_In_ ID3D11DeviceContext *This);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Dispatch_Override ( _In_ ID3D11DeviceContext *This,
                          _In_ UINT                 ThreadGroupCountX,
                          _In_ UINT                 ThreadGroupCountY,
                          _In_ UINT                 ThreadGroupCountZ );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DispatchIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11_Map_Override (
     _In_ ID3D11DeviceContext      *This,
     _In_ ID3D11Resource           *pResource,
     _In_ UINT                      Subresource,
     _In_ D3D11_MAP                 MapType,
     _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Unmap_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargets_Override (
  ID3D11DeviceContext           *This,
  _In_     UINT                           NumViews,
  _In_opt_ ID3D11RenderTargetView *const *ppRenderTargetViews,
  _In_opt_ ID3D11DepthStencilView        *pDepthStencilView );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Override (
                 ID3D11DeviceContext              *This,
  _In_           UINT                              NumRTVs,
  _In_opt_       ID3D11RenderTargetView    *const *ppRenderTargetViews,
  _In_opt_       ID3D11DepthStencilView           *pDepthStencilView,
  _In_           UINT                              UAVStartSlot,
  _In_           UINT                              NumUAVs,
  _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_opt_ const UINT                             *pUAVInitialCounts );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargets_Override (
        ID3D11DeviceContext     *This,
  _In_  UINT                     NumViews,
  _Out_ ID3D11RenderTargetView **ppRenderTargetViews,
  _Out_ ID3D11DepthStencilView **ppDepthStencilView );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Override (
        ID3D11DeviceContext        *This,
  _In_  UINT                        NumRTVs,
  _Out_ ID3D11RenderTargetView    **ppRenderTargetViews,
  _Out_ ID3D11DepthStencilView    **ppDepthStencilView,
  _In_  UINT                        UAVStartSlot,
  _In_  UINT                        NumUAVs,
  _Out_ ID3D11UnorderedAccessView **ppUnorderedAccessViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ClearDepthStencilView_Override (
  _In_ ID3D11DeviceContext    *This,
  _In_ ID3D11DepthStencilView *pDepthStencilView,
  _In_ UINT                    ClearFlags,
  _In_ FLOAT                   Depth,
  _In_ UINT8                   Stencil);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ExecuteCommandList_Override (
  _In_  ID3D11DeviceContext *This,
  _In_  ID3D11CommandList   *pCommandList,
        BOOL                 RestoreContextState );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11_FinishCommandList_Override (
            ID3D11DeviceContext  *This,
            BOOL                  RestoreDeferredContextState,
  _Out_opt_ ID3D11CommandList   **ppCommandList );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetSamplers_Override
(
  _In_     ID3D11DeviceContext        *This,
  _In_     UINT                        StartSlot,
  _In_     UINT                        NumSamplers,
  _In_opt_ ID3D11SamplerState*/*const*/*ppSamplers);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11VertexShader         *pVertexShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11PixelShader          *pPixelShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11GeometryShader       *pGeometryShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11HullShader           *pHullShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11DomainShader         *pDomainShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11ComputeShader        *pComputeShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11VertexShader  **ppVertexShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11PixelShader   **ppPixelShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSGetShader_Override (
  _In_        ID3D11DeviceContext   *This,
  _Out_       ID3D11GeometryShader **ppGeometryShader,
  _Out_opt_   ID3D11ClassInstance  **ppClassInstances,
  _Inout_opt_ UINT                  *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11HullShader    **ppHullShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11DomainShader  **ppDomainShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11ComputeShader **ppComputeShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ClearState_Override (
  _In_        ID3D11DeviceContext *This );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11_GetData_Override (
  _In_  ID3D11DeviceContext *This,
  _In_  ID3D11Asynchronous  *pAsync,
  _Out_writes_bytes_opt_   ( DataSize )
        void                *pData,
  _In_  UINT                 DataSize,
  _In_  UINT                 GetDataFlags );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CopyResource_Override (
       ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource );

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
  _In_opt_ const D3D11_BOX           *pSrcBox );

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
  _In_           UINT                  CopyFlags);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ResolveSubresource_Override (
       ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ UINT                 DstSubresource,
  _In_ ID3D11Resource      *pSrcResource,
  _In_ UINT                 SrcSubresource,
  _In_ DXGI_FORMAT          Format );
#endif



using namespace std::placeholders;

using D3D11DevCtx_GetConstantBuffers_pfn =
    void (STDMETHODCALLTYPE ID3D11DeviceContext::* )
              ( UINT, UINT, ID3D11Buffer ** );

using D3D11DevCtx1_GetConstantBuffers1_pfn =
    void (STDMETHODCALLTYPE ID3D11DeviceContext1::* )
              ( UINT,   UINT, ID3D11Buffer **,
                UINT *, UINT * );

static constexpr
  std::tuple < D3D11DevCtx_GetConstantBuffers_pfn,
               D3D11DevCtx1_GetConstantBuffers1_pfn >
  GetConstantBuffers_FnTbl [] = {
    { &ID3D11DeviceContext ::VSGetConstantBuffers,
      &ID3D11DeviceContext1::VSGetConstantBuffers1 },
    { &ID3D11DeviceContext ::PSGetConstantBuffers,
      &ID3D11DeviceContext1::PSGetConstantBuffers1 },
    { &ID3D11DeviceContext ::GSGetConstantBuffers,
      &ID3D11DeviceContext1::GSGetConstantBuffers1 },
    { &ID3D11DeviceContext ::HSGetConstantBuffers,
      &ID3D11DeviceContext1::HSGetConstantBuffers1 },
    { &ID3D11DeviceContext ::DSGetConstantBuffers,
      &ID3D11DeviceContext1::DSGetConstantBuffers1 }
  };

using D3D11DevCtx_SetConstantBuffers_pfn =
    void (STDMETHODCALLTYPE ID3D11DeviceContext::* )
              ( UINT, UINT, ID3D11Buffer *const * );

static constexpr
  D3D11DevCtx_SetConstantBuffers_pfn
              SetConstantBuffers_FnTbl [] =
  {
    &ID3D11DeviceContext::VSSetConstantBuffers,
    &ID3D11DeviceContext::PSSetConstantBuffers,
    &ID3D11DeviceContext::GSSetConstantBuffers,
    &ID3D11DeviceContext::HSSetConstantBuffers,
    &ID3D11DeviceContext::DSSetConstantBuffers
  };


bool
SK_D3D11_OverrideDepthStencil (DXGI_FORMAT& fmt);

class SK_IWrapD3D11DeviceContext;
class SK_IWrapD3D11Multithread;

class SK_D3D11_Wrapper_Factory
{
public:
  ID3D11DeviceContext4*     wrapDeviceContext (ID3D11DeviceContext*            dev_ctx);
  SK_IWrapD3D11Multithread* wrapMultithread   (SK_IWrapD3D11DeviceContext* wrapped_ctx);
};

// Keep a tally of shader states loaded that require tracking all draws
//   and ignore conditional tracking states (i.e. HUD)
//
//   This allows a shader config file that only defines HUD shaders
//     to bypass state tracking overhead until the user requests that
//       HUD shaders be skipped.
//
struct SK_D3D11_StateTrackingCounters
{
  volatile LONG Always       = 0;
  volatile LONG Conditional  = 0;
  volatile LONG TextureBased = 0;
};

extern SK_LazyGlobal <SK_D3D11_Wrapper_Factory>       SK_D3D11_WrapperFactory;
extern SK_LazyGlobal <SK_D3D11_StateTrackingCounters> SK_D3D11_TrackingCount;

typedef HRESULT (WINAPI *D3D11CreateDevice_pfn)(
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

typedef HRESULT (WINAPI *D3D11CreateDeviceAndSwapChain_pfn)(
  _In_opt_                             IDXGIAdapter*,
                                       D3D_DRIVER_TYPE,
                                       HMODULE,
                                       UINT,
  _In_reads_opt_ (FeatureLevels) CONST D3D_FEATURE_LEVEL*,
                                       UINT FeatureLevels,
                                       UINT,
  _In_opt_                       CONST DXGI_SWAP_CHAIN_DESC*,
  _Out_opt_                            IDXGISwapChain**,
  _Out_opt_                            ID3D11Device**,
  _Out_opt_                            D3D_FEATURE_LEVEL*,
  _Out_opt_                            ID3D11DeviceContext**);

typedef HRESULT (WINAPI *D3D11CoreCreateDevice_pfn)(
          IDXGIFactory       *pFactory,
          IDXGIAdapter       *pAdapter,
          D3D_DRIVER_TYPE     DriverType,
          HINSTANCE           Software,
          UINT                Flags,
    const D3D_FEATURE_LEVEL  *pFeatureLevels,
          UINT                FeatureLevels,
          INT                 SDKVersion,
          ID3D11Device      **ppDevice,
          D3D_FEATURE_LEVEL  *pFeatureLevel);

extern D3D11CreateDevice_pfn             D3D11CreateDevice_Import;
extern D3D11CoreCreateDevice_pfn         D3D11CoreCreateDevice_Import;
extern D3D11CreateDeviceAndSwapChain_pfn D3D11CreateDeviceAndSwapChain_Import;

extern          ID3D11Device*         g_pD3D11Dev;

extern bool SK_D3D11_Init         (void);
extern void SK_D3D11_InitTextures (void);
extern void SK_D3D11_Shutdown     (void);
extern void SK_D3D11_EnableHooks  (void);
extern void SK_D3D11_QuickHook    (void);


struct SK_D3D11_ShaderDesc
{
  SK_D3D11_ShaderType type    = SK_D3D11_ShaderType::Invalid;
  uint32_t            crc32c  = 0UL;
  IUnknown*           pShader = nullptr;
  std::string         name    = "";
  std::vector <BYTE>  bytecode;

  struct
  {
             __time64_t last_time  = 0ULL;
    volatile ULONG64    last_frame = 0UL;
             ULONG      refs       = 0;
  } usage;
};

struct SK_DisjointTimerQueryD3D11
{
  // Always issue this from the immediate context

  SK_ComPtr <ID3D11Query> async  = nullptr;
  bool                    active = false;

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT
                  last_results   = { };
};

struct SK_TimerQueryD3D11
{
  SK_ComPtr <ID3D11Query> async  = nullptr;
  bool                    active = false;

  // Required per-query to support timing the execution of commands batched
  //   using deferred render contexts.
  SK_ComPtr <ID3D11DeviceContext> dev_ctx      = nullptr;
             UINT64               last_results = {     };
};

#define _MAX_VIEWS 128

struct d3d11_shader_resource_views_s {
  uint32_t                  shader    [SK_D3D11_MAX_DEV_CONTEXTS+1]             =   { }  ;
  ID3D11ShaderResourceView* views     [SK_D3D11_MAX_DEV_CONTEXTS+1][_MAX_VIEWS] = { { } };
  ID3D11ShaderResourceView* tmp_views [SK_D3D11_MAX_DEV_CONTEXTS+1][_MAX_VIEWS] = { { } };
  // Avoid allocating memory on the heap/stack when we have to manipulate an array
  //   large enough to store all D3D11 Shader Resource Views.
};

struct d3d11_shader_tracking_s
{
  void clear (void)
  {
    const auto dev_contexts =
      std::min ( SK_D3D11_AllocatedDevContexts + 1, SK_D3D11_MAX_DEV_CONTEXTS );

    for ( int i = 0 ; i < dev_contexts ; ++i )
    {
      active.set (i, false);
    }

    RtlZeroMemory ( &current_->views [0],
                      dev_contexts * sizeof (ID3D11ShaderResourceView*) );

    num_draws          = 0;
    num_deferred_draws = 0;

    if ( set_of_res.empty   () &&
         set_of_views.empty () &&
         classes.empty      () )
    {
      return;
    }

    auto shader_class_crit_sec = [&](void) noexcept
    {
      extern
        std::unique_ptr <SK_Thread_HybridSpinlock>
                        cs_shader_vs, cs_shader_ps,
                        cs_shader_gs,
                        cs_shader_hs, cs_shader_ds,
                        cs_shader_cs;

      switch (type_)
      {
        default:
        //assert (false);
          return cs_shader_vs.get ();

        case SK_D3D11_ShaderType::Vertex:   return cs_shader_vs.get ();
        case SK_D3D11_ShaderType::Pixel:    return cs_shader_ps.get ();
        case SK_D3D11_ShaderType::Geometry: return cs_shader_gs.get ();
        case SK_D3D11_ShaderType::Hull:     return cs_shader_hs.get ();
        case SK_D3D11_ShaderType::Domain:   return cs_shader_ds.get ();
        case SK_D3D11_ShaderType::Compute:  return cs_shader_cs.get ();
      }
    };

    ///for ( auto & it : set_of_res   ) it->Release ();
    ///for ( auto & it : set_of_views ) it->Release ();

    set_of_res.clear   ();
    set_of_views.clear ();

    first_rtv = { };

    std::scoped_lock <SK_Thread_HybridSpinlock>
         auto_lock (*shader_class_crit_sec ());

    classes.clear      ();

    pre_hud_rtv = nullptr;

    //used_textures.clear ();
  }

  void use        ( ID3D11DeviceContext* pDevCtx );
  void use_cmdlist( ID3D11DeviceContext* pDevCtx ); // Deferred draw, cannot be timed

  // Used for timing queries and interface tracking
  void activate   ( ID3D11DeviceContext        *pDevContext,
                    ID3D11ClassInstance *const *ppClassInstances,
                    UINT                        NumClassInstances,
                    UINT                        dev_idx = UINT_MAX );
  void deactivate ( ID3D11DeviceContext        *pDevCtx,
                    UINT                        dev_idx = UINT_MAX );

  std::atomic_uint32_t crc32c          =  0x00;
  std::atomic_bool     cancel_draws    = false;
  std::atomic_bool     highlight_draws = false;
  std::atomic_bool     wireframe       = false;
  std::atomic_bool     on_top          =  true;
  std::atomic_bool     clear_output    = false;

  struct
  {
    std::array < bool, SK_D3D11_MAX_DEV_CONTEXTS+1 > contexts = { };

    // Only examine the hash map when at least one context is active,
    //   or we will kill performance!
    volatile LONG
      active_count = 0L;

    bool get (int dev_idx)
    {
      if (ReadAcquire (&active_count) > 0)
      {
        assert (dev_idx < SK_D3D11_MAX_DEV_CONTEXTS);

        if (dev_idx <= SK_D3D11_MAX_DEV_CONTEXTS)
        {
          return contexts [dev_idx];
        }
      }

      return
        false;
    }

    bool get (ID3D11DeviceContext* pDevCtx)
    {
      return
        get (SK_D3D11_GetDeviceContextHandle (pDevCtx));
    }

    void set (int dev_idx, bool active)
    {
      assert (dev_idx < SK_D3D11_MAX_DEV_CONTEXTS);

      if (ReadAcquire (&active_count) > 0 || active == true)
      {
        if (dev_idx <= SK_D3D11_MAX_DEV_CONTEXTS)
        {
          if (contexts [dev_idx] != active)
          {
                 if (active) InterlockedIncrement (&active_count);
            else if (ReadAcquire (&active_count) > 0)
                             InterlockedDecrement (&active_count);

            contexts [dev_idx] = active;
          }
        }
      }
    }

    void set (ID3D11DeviceContext* pDevCtx, bool active)
    {
      set (
        SK_D3D11_GetDeviceContextHandle (pDevCtx),
          active
      );
    }
  } active;

  std::atomic_ulong                  num_draws          =       0;
  std::atomic_ulong                  num_deferred_draws =       0;
  std::atomic_bool                   pre_hud_source     =   false;
  std::atomic_long                   pre_hud_rt_slot    =      -1;
  std::atomic_long                   pre_hud_srv_slot   =      -1;
  SK_ComPtr <ID3D11RenderTargetView> pre_hud_rtv        = nullptr;

  // The slot used has meaning, but I think we can ignore it for now...
  //std::unordered_map <UINT, ID3D11ShaderResourceView *> used_views;

  //concurrency::concurrent_unordered_set <
  //  ID3D11Resource*          > set_of_res;
  //concurrency::concurrent_unordered_set <
  //  ID3D11ShaderResourceView*> set_of_views;
  concurrency::concurrent_unordered_set <
    SK_ComPtr <ID3D11Resource>             > set_of_res;
  concurrency::concurrent_unordered_set <
    SK_ComPtr <ID3D11ShaderResourceView>   > set_of_views;

  D3D11_TEXTURE2D_DESC first_rtv = {};


  struct cbuffer_override_s {
    uint32_t parent;
    size_t   BufferSize; // Parent buffer's size
    bool     Enable;
    uint32_t Slot;
    uint32_t StartAddr;
    uint32_t Size;
    float    Values [128];
  };

  std::vector <cbuffer_override_s>  overrides;

  IUnknown*                         shader_obj = nullptr;

  static SK_DisjointTimerQueryD3D11 disjoint_query;
  struct duration_s
  {
    // Timestamp at beginning
    SK_TimerQueryD3D11 start;

    // Timestamp at end
    SK_TimerQueryD3D11 end;
  };
  std::vector <duration_s>          timers;

  // Cumulative runtime of all timers after the disjoint query
  //   is finished and reading these results would not stall
  //     the pipeline
  std::atomic_uint64_t              runtime_ticks   = 0ULL;
  double                            runtime_ms      = 0.0;
  double                            last_runtime_ms = 0.0;


  void addClassInstance (ID3D11ClassInstance* pInstance)
  {
    classes.insert (pInstance);
  }

  std::set <SK_ComPtr <ID3D11ClassInstance>> classes;

//  struct shader_constant_s
//  {
//    char                Name [128];
//    D3DXREGISTER_SET    RegisterSet;
//    UINT                RegisterIndex;
//    UINT                RegisterCount;
//    D3DXPARAMETER_CLASS Class;
//    D3DXPARAMETER_TYPE  Type;
//    UINT                Rows;
//    UINT                Columns;
//    UINT                Elements;
//    std::vector <shader_constant_s>
//                        struct_members;
//    bool                Override;
//    float               Data [4]; // TEMP HACK
//  };

//  std::vector <shader_constant_s> constants;

  SK_D3D11_ShaderType            type_    = SK_D3D11_ShaderType::Invalid;
  d3d11_shader_resource_views_s* current_ = nullptr;
};

// Cached each frame
extern bool SK_D3D11_HasReShadeTriggers;

struct SK_D3D11_KnownShaders
{
  typedef std::unordered_map <
    uint32_t, std::unordered_set <uint32_t>
  > conditional_blacklist_t;

  template <typename _T>
  class ShaderRegistry
  {
  public:
    ShaderRegistry (void)
    {
           if (std::type_index (typeid (_T)) ==
               std::type_index (typeid (ID3D11VertexShader)))
                 type_ = SK_D3D11_ShaderType::Vertex;

      else if (std::type_index (typeid (_T)) ==
               std::type_index (typeid (ID3D11PixelShader)))
                 type_ = SK_D3D11_ShaderType::Pixel;

      else if (std::type_index (typeid (_T)) ==
               std::type_index (typeid (ID3D11GeometryShader)))
                 type_ = SK_D3D11_ShaderType::Geometry;

      else if (std::type_index (typeid (_T)) ==
               std::type_index (typeid (ID3D11DomainShader)))
                 type_ = SK_D3D11_ShaderType::Domain;

      else if (std::type_index (typeid (_T)) ==
               std::type_index (typeid (ID3D11HullShader)))
                 type_ = SK_D3D11_ShaderType::Hull;

      else if (std::type_index (typeid (_T)) ==
               std::type_index (typeid (ID3D11ComputeShader)))
                 type_ = SK_D3D11_ShaderType::Compute;

      tracked.type_    =  type_;
      tracked.current_ = &current;
    }

    std::unordered_map <ID3D11Device*, std::unordered_map <_T*,      SK_D3D11_ShaderDesc*>>  rev;
    std::unordered_map <ID3D11Device*, std::unordered_map <uint32_t, SK_D3D11_ShaderDesc>>   descs;

    std::unordered_map <uint32_t, LONG>                  blacklist;
    std::unordered_map <uint32_t, std::string>           names;

    //struct drawable_s {
      std::unordered_map <uint32_t, LONG>                wireframe;

      std::unordered_map <uint32_t, LONG>                on_top;
      std::unordered_map <uint32_t, LONG>                rewind;
      std::unordered_map <uint32_t, LONG>                hud;

      struct {
        std::unordered_map <uint32_t, LONG> before;
        std::unordered_map <uint32_t, LONG> after;
      } trigger_reshade;
    //} draw_base;

    //struct compute_s {
      std::unordered_map <uint32_t, LONG>                clear_uav;
    //} compute_base;

    bool addTrackingRef ( std::unordered_map <uint32_t, LONG>& state,
                                              uint32_t         crc32c )
    {
      auto state_ =
        state.find (crc32c);

      if ( state_ != state.cend () )
      {
        state_->second++;

        return false;
      }

      state [crc32c]++;

      return true;
    }

    bool releaseTrackingRef (std::unordered_map <uint32_t, LONG>& state,
                                                 uint32_t         crc32c )
    {
      auto state_ =
        state.find (crc32c);

      if (state_ != state.cend ())
      {
        if (--state_->second <= 0)
        {
          state.erase (state_);

          return true;
        }

        return false;
      }

      return true;
    }

    conditional_blacklist_t       blacklist_if_texture = { };
    d3d11_shader_tracking_s       tracked;

    d3d11_shader_resource_views_s current              = { };

    volatile
    LONG                          changes_last_frame = 0;
    SK_D3D11_ShaderType           type_              = SK_D3D11_ShaderType::Invalid;

    operator ShaderRegistry <IUnknown>& (void) const noexcept
    {
      return
        *((ShaderRegistry <IUnknown> *)this);
    }

    operator ShaderRegistry <IUnknown>* (void) const noexcept
    {
      return
        (ShaderRegistry <IUnknown> *)this;
    }
  };

  bool hasReShadeTriggers (void)
  {
    return
      config.reshade.is_addon &&
        ( (! pixel.trigger_reshade.before.empty  ()) ||
          (! vertex.trigger_reshade.before.empty ()) );
  }
  //static std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1> reshade_triggered;
  static bool                           reshade_triggered;

  ShaderRegistry <ID3D11PixelShader>    pixel;
  ShaderRegistry <ID3D11VertexShader>   vertex;
  ShaderRegistry <ID3D11GeometryShader> geometry;
  ShaderRegistry <ID3D11HullShader>     hull;
  ShaderRegistry <ID3D11DomainShader>   domain;
  ShaderRegistry <ID3D11ComputeShader>  compute;
};

typedef HRESULT (WINAPI *D3D11CreateDevice_pfn)(
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

typedef HRESULT (WINAPI *D3D11CreateDeviceAndSwapChain_pfn)(
  _In_opt_                             IDXGIAdapter*,
                                       D3D_DRIVER_TYPE,
                                       HMODULE,
                                       UINT,
  _In_reads_opt_ (FeatureLevels) CONST D3D_FEATURE_LEVEL*,
                                       UINT FeatureLevels,
                                       UINT,
  _In_opt_                       CONST DXGI_SWAP_CHAIN_DESC*,
  _Out_opt_                            IDXGISwapChain**,
  _Out_opt_                            ID3D11Device**,
  _Out_opt_                            D3D_FEATURE_LEVEL*,
  _Out_opt_                            ID3D11DeviceContext**);

typedef void (WINAPI *D3D11_UpdateSubresource1_pfn)(
  _In_           ID3D11DeviceContext1 *This,
  _In_           ID3D11Resource       *pDstResource,
  _In_           UINT                  DstSubresource,
  _In_opt_ const D3D11_BOX            *pDstBox,
  _In_     const void                 *pSrcData,
  _In_           UINT                  SrcRowPitch,
  _In_           UINT                  SrcDepthPitch,
  _In_           UINT                  CopyFlags
);

#include <../depends/include/nvapi/nvapi_lite_common.h>
typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateVertexShaderEx_pfn)( __in        ID3D11Device *pDevice,        __in     const void                *pShaderBytecode,
                                                                      __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage  *pClassLinkage,
                                                                      __in  const LPVOID                                                            pCreateVertexShaderExArgs,
                                                                      __out       ID3D11VertexShader                                              **ppVertexShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateHullShaderEx_pfn)( __in        ID3D11Device *pDevice,        __in const void               *pShaderBytecode,
                                                                    __in        SIZE_T        BytecodeLength, __in_opt   ID3D11ClassLinkage *pClassLinkage,
                                                                    __in  const LPVOID                                                       pCreateHullShaderExArgs,
                                                                    __out       ID3D11HullShader                                           **ppHullShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateDomainShaderEx_pfn)( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode,
                                                                      __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage,
                                                                      __in  const LPVOID                                                           pCreateDomainShaderExArgs,
                                                                      __out       ID3D11DomainShader                                             **ppDomainShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateGeometryShaderEx_2_pfn)( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode,
                                                                          __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage,
                                                                          __in  const LPVOID                                                           pCreateGeometryShaderExArgs,
                                                                          __out       ID3D11GeometryShader                                           **ppGeometryShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateFastGeometryShaderExplicit_pfn)( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode,
                                                                                  __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage,
                                                                                  __in  const LPVOID                                                           pCreateFastGSArgs,
                                                                                  __out       ID3D11GeometryShader                                           **ppGeometryShader );

typedef NvAPI_Status (__cdecl *NvAPI_D3D11_CreateFastGeometryShader_pfn)( __in  ID3D11Device *pDevice,        __in     const void                *pShaderBytecode,
                                                                          __in  SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage  *pClassLinkage,
                                                                          __out ID3D11GeometryShader                                            **ppGeometryShader );


typedef HRESULT (WINAPI *D3D11CreateDevice_pfn)(
  _In_opt_        IDXGIAdapter         *pAdapter,
                  D3D_DRIVER_TYPE       DriverType,
                  HMODULE               Software,
                  UINT                  Flags,
  _In_opt_  const D3D_FEATURE_LEVEL    *pFeatureLevels,
                  UINT                  FeatureLevels,
                  UINT                  SDKVersion,
  _Out_opt_       ID3D11Device        **ppDevice,
  _Out_opt_       D3D_FEATURE_LEVEL    *pFeatureLevel,
  _Out_opt_       ID3D11DeviceContext **ppImmediateContext);

typedef enum D3DX11_IMAGE_FILE_FORMAT {
   D3DX11_IFF_BMP          = 0,
   D3DX11_IFF_JPG          = 1,
   D3DX11_IFF_PNG          = 3,
   D3DX11_IFF_DDS          = 4,
   D3DX11_IFF_TIFF         = 10,
   D3DX11_IFF_GIF          = 11,
   D3DX11_IFF_WMP          = 12,
   D3DX11_IFF_FORCE_DWORD  = 0x7fffffff
}  D3DX11_IMAGE_FILE_FORMAT,
*LPD3DX11_IMAGE_FILE_FORMAT;

typedef struct D3DX11_IMAGE_INFO {
   UINT                     Width;
   UINT                     Height;
   UINT                     Depth;
   UINT                     ArraySize;
   UINT                     MipLevels;
   UINT                     MiscFlags;
   DXGI_FORMAT              Format;
   D3D11_RESOURCE_DIMENSION ResourceDimension;
   D3DX11_IMAGE_FILE_FORMAT ImageFileFormat;
}  D3DX11_IMAGE_INFO,
*LPD3DX11_IMAGE_INFO;

typedef struct D3DX11_IMAGE_LOAD_INFO {
   UINT              Width;
   UINT              Height;
   UINT              Depth;
   UINT              FirstMipLevel;
   UINT              MipLevels;
   D3D11_USAGE       Usage;
   UINT              BindFlags;
   UINT              CpuAccessFlags;
   UINT              MiscFlags;
   DXGI_FORMAT       Format;
   UINT              Filter;
   UINT              MipFilter;
   D3DX11_IMAGE_INFO *pSrcInfo;
}  D3DX11_IMAGE_LOAD_INFO,
*LPD3DX11_IMAGE_LOAD_INFO;

interface ID3DX11ThreadPump;

typedef HRESULT (WINAPI *D3DX11FilterTexture_pfn)(
   ID3D11DeviceContext *pContext,
   ID3D11Resource      *pTexture,
   UINT                SrcLevel,
   UINT                MipFilter
);

typedef HRESULT (WINAPI *D3DX11CreateTextureFromMemory_pfn)(
  _In_  ID3D11Device           *pDevice,
  _In_  LPCVOID                pSrcData,
  _In_  SIZE_T                 SrcDataSize,
  _In_  D3DX11_IMAGE_LOAD_INFO *pLoadInfo,
  _In_  ID3DX11ThreadPump      *pPump,
  _Out_ ID3D11Resource         **ppTexture,
  _Out_ HRESULT                *pHResult
);

typedef HRESULT (WINAPI *D3DX11CreateTextureFromFileW_pfn)(
  _In_  ID3D11Device           *pDevice,
  _In_  LPCWSTR                pSrcFile,
  _In_  D3DX11_IMAGE_LOAD_INFO *pLoadInfo,
  _In_  IUnknown               *pPump,
  _Out_ ID3D11Resource         **ppTexture,
  _Out_ HRESULT                *pHResult
);

interface ID3DX11ThreadPump;

typedef HRESULT (WINAPI *D3DX11GetImageInfoFromFileW_pfn)(
  _In_  LPCWSTR           pSrcFile,
  _In_  ID3DX11ThreadPump *pPump,
  _In_  D3DX11_IMAGE_INFO *pSrcInfo,
  _Out_ HRESULT           *pHResult
);

typedef void (WINAPI *D3D11_UpdateSubresource1_pfn)(
  _In_           ID3D11DeviceContext1 *This,
  _In_           ID3D11Resource       *pDstResource,
  _In_           UINT                  DstSubresource,
  _In_opt_ const D3D11_BOX            *pDstBox,
  _In_     const void                 *pSrcData,
  _In_           UINT                  SrcRowPitch,
  _In_           UINT                  SrcDepthPitch,
  _In_           UINT                  CopyFlags
);


namespace SK
{
  namespace DXGI
  {
    struct PipelineStatsD3D11
    {
      struct StatQueryD3D11
      {
        volatile ID3D11Query* async  = nullptr;
        volatile LONG         active = FALSE;
      } query;

      D3D11_QUERY_DATA_PIPELINE_STATISTICS
                 last_results = { };
    } extern pipeline_stats_d3d11;
  };
};

bool            WaitForInitDXGI             (DWORD dwTimeout = INFINITE);

void  __stdcall SK_D3D11_PreLoadTextures    (void);

void  __stdcall SK_D3D11_TexCacheCheckpoint (void);
bool  __stdcall SK_D3D11_TextureIsCached    (ID3D11Texture2D* pTex);
void  __stdcall SK_D3D11_UseTexture         (ID3D11Texture2D* pTex);
bool  __stdcall SK_D3D11_RemoveTexFromCache (ID3D11Texture2D* pTex,
                                             bool blacklist = false);


void  __stdcall SK_D3D11_PresentFirstFrame  (IDXGISwapChain*  pSwapChain);
void  __stdcall SK_D3D11_UpdateRenderStats  (IDXGISwapChain*  pSwapChain);


BOOL SK_DXGI_SupportsTearing  (void);
void SK_ImGui_QueueResetD3D11 (void);


void SK_D3D11_AssociateVShaderWithHUD (uint32_t crc32, bool set = true);
void SK_D3D11_AssociatePShaderWithHUD (uint32_t crc32, bool set = true);


extern SK_LazyGlobal <SK_D3D11_KnownShaders> SK_D3D11_Shaders;

#define SK_D3D11_DeclHUDShader_Vtx(crc32c)  {                    \
    InterlockedIncrement (&SK_D3D11_TrackingCount->Conditional); \
    SK_D3D11_Shaders->vertex.addTrackingRef (                    \
      SK_D3D11_Shaders->vertex.hud, (crc32c)); }

#define SK_D3D11_DeclHUDShader_Pix(crc32c)  {                    \
    InterlockedIncrement (&SK_D3D11_TrackingCount->Conditional); \
    SK_D3D11_Shaders->pixel.addTrackingRef  (                    \
      SK_D3D11_Shaders->pixel.hud, (crc32c)); }


#define SK_D3D11_DeclHUDShader(crc32c,type) \
  SK_D3D11_RegisterHUDShader ((crc32c), std::type_index (typeid (type)));

bool
SK_D3D11_RegisterHUDShader (        uint32_t  bytecode_crc32c,
                             std::type_index _T =
                             std::type_index  (
                                       typeid ( ID3D11VertexShader )
                                              ),
                                        bool  remove = false       );
bool
SK_D3D11_UnRegisterHUDShader ( uint32_t         bytecode_crc32c,
                               std::type_index _T =
                               std::type_index  (
                                         typeid ( ID3D11VertexShader )
                                                )                    );

struct sk_hook_d3d11_t {
 ID3D11Device**        ppDevice;
 ID3D11DeviceContext** ppImmediateContext;
};

struct SK_D3D11_TexCacheResidency_s
{
  struct
  {
    volatile LONG InVRAM   = 0;
    volatile LONG Shared   = 0;
    volatile LONG PagedOut = 0;
  } count;

  struct
  {
    volatile LONG64 InVRAM   = 0;
    volatile LONG64 Shared   = 0;
    volatile LONG64 PagedOut = 0;
  } size;
};

extern SK_LazyGlobal <SK_D3D11_TexCacheResidency_s> SK_D3D11_TexCacheResidency;


DWORD
__stdcall
HookD3D11 (LPVOID user);

int  SK_D3D11_PurgeHookAddressCache  (void);
void SK_D3D11_UpdateHookAddressCache (void);

const char* SK_D3D11_DescribeUsage     (D3D11_USAGE              usage)  noexcept;
const char* SK_D3D11_DescribeFilter    (D3D11_FILTER             filter) noexcept;
std::string SK_D3D11_DescribeMiscFlags (D3D11_RESOURCE_MISC_FLAG flags);
std::string SK_D3D11_DescribeBindFlags (/*D3D11_BIND_FLAG*/UINT  flags);


constexpr int VERTEX_SHADER_STAGE   = 0;
constexpr int PIXEL_SHADER_STAGE    = 1;
constexpr int GEOMETRY_SHADER_STAGE = 2;
constexpr int HULL_SHADER_STAGE     = 3;
constexpr int DOMAIN_SHADER_STAGE   = 4;
constexpr int COMPUTE_SHADER_STAGE  = 5;


// VFTABLE Hooks
extern D3D11Dev_CreateRasterizerState_pfn
       D3D11Dev_CreateRasterizerState_Original;
extern D3D11Dev_CreateSamplerState_pfn
       D3D11Dev_CreateSamplerState_Original;
extern D3D11Dev_CreateBuffer_pfn
       D3D11Dev_CreateBuffer_Original;
extern D3D11Dev_CreateTexture2D_pfn
       D3D11Dev_CreateTexture2D_Original;
extern D3D11Dev_CreateTexture2D1_pfn
       D3D11Dev_CreateTexture2D1_Original;
extern D3D11Dev_CreateRenderTargetView_pfn
       D3D11Dev_CreateRenderTargetView_Original;
extern D3D11Dev_CreateRenderTargetView1_pfn
       D3D11Dev_CreateRenderTargetView1_Original;
extern D3D11Dev_CreateShaderResourceView_pfn
       D3D11Dev_CreateShaderResourceView_Original;
extern D3D11Dev_CreateShaderResourceView1_pfn
       D3D11Dev_CreateShaderResourceView1_Original;
extern D3D11Dev_CreateDepthStencilView_pfn
       D3D11Dev_CreateDepthStencilView_Original;
extern D3D11Dev_CreateUnorderedAccessView_pfn
       D3D11Dev_CreateUnorderedAccessView_Original;
extern D3D11Dev_CreateUnorderedAccessView1_pfn
       D3D11Dev_CreateUnorderedAccessView1_Original;

extern D3D11Dev_CreateVertexShader_pfn
       D3D11Dev_CreateVertexShader_Original;
extern D3D11Dev_CreatePixelShader_pfn
       D3D11Dev_CreatePixelShader_Original;
extern D3D11Dev_CreateGeometryShader_pfn
       D3D11Dev_CreateGeometryShader_Original;
extern D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn
       D3D11Dev_CreateGeometryShaderWithStreamOutput_Original;
extern D3D11Dev_CreateHullShader_pfn
       D3D11Dev_CreateHullShader_Original;
extern D3D11Dev_CreateDomainShader_pfn
       D3D11Dev_CreateDomainShader_Original;
extern D3D11Dev_CreateComputeShader_pfn
       D3D11Dev_CreateComputeShader_Original;

extern D3D11Dev_CreateDeferredContext_pfn
       D3D11Dev_CreateDeferredContext_Original;
extern D3D11Dev_CreateDeferredContext1_pfn
       D3D11Dev_CreateDeferredContext1_Original;
extern D3D11Dev_CreateDeferredContext2_pfn
       D3D11Dev_CreateDeferredContext2_Original;
extern D3D11Dev_CreateDeferredContext3_pfn
       D3D11Dev_CreateDeferredContext3_Original;
extern D3D11Dev_GetImmediateContext_pfn
       D3D11Dev_GetImmediateContext_Original;
extern D3D11Dev_GetImmediateContext1_pfn
       D3D11Dev_GetImmediateContext1_Original;
extern D3D11Dev_GetImmediateContext2_pfn
       D3D11Dev_GetImmediateContext2_Original;
extern D3D11Dev_GetImmediateContext3_pfn
       D3D11Dev_GetImmediateContext3_Original;

extern D3D11_RSSetScissorRects_pfn
       D3D11_RSSetScissorRects_Original;
extern D3D11_RSSetViewports_pfn
       D3D11_RSSetViewports_Original;
extern D3D11_VSSetConstantBuffers_pfn
       D3D11_VSSetConstantBuffers_Original;
extern D3D11_VSSetShaderResources_pfn
       D3D11_VSSetShaderResources_Original;
extern D3D11_PSSetShaderResources_pfn
       D3D11_PSSetShaderResources_Original;
extern D3D11_PSSetConstantBuffers_pfn
       D3D11_PSSetConstantBuffers_Original;
extern D3D11_GSSetShaderResources_pfn
       D3D11_GSSetShaderResources_Original;
extern D3D11_HSSetShaderResources_pfn
       D3D11_HSSetShaderResources_Original;
extern D3D11_DSSetShaderResources_pfn
       D3D11_DSSetShaderResources_Original;
extern D3D11_CSSetShaderResources_pfn
       D3D11_CSSetShaderResources_Original;
extern D3D11_CSSetUnorderedAccessViews_pfn
       D3D11_CSSetUnorderedAccessViews_Original;
extern D3D11_UpdateSubresource_pfn
       D3D11_UpdateSubresource_Original;
extern D3D11_DrawIndexed_pfn
       D3D11_DrawIndexed_Original;
extern D3D11_Draw_pfn
       D3D11_Draw_Original;
extern D3D11_DrawAuto_pfn
       D3D11_DrawAuto_Original;
extern D3D11_DrawIndexedInstanced_pfn
       D3D11_DrawIndexedInstanced_Original;
extern D3D11_DrawIndexedInstancedIndirect_pfn
       D3D11_DrawIndexedInstancedIndirect_Original;
extern D3D11_DrawInstanced_pfn
       D3D11_DrawInstanced_Original;
extern D3D11_DrawInstancedIndirect_pfn
       D3D11_DrawInstancedIndirect_Original;
extern D3D11_Dispatch_pfn
       D3D11_Dispatch_Original;
extern D3D11_DispatchIndirect_pfn
       D3D11_DispatchIndirect_Original;
extern D3D11_Map_pfn
       D3D11_Map_Original;
extern D3D11_Unmap_pfn
       D3D11_Unmap_Original;

extern D3D11_OMSetRenderTargets_pfn
       D3D11_OMSetRenderTargets_Original;
extern D3D11_OMSetRenderTargetsAndUnorderedAccessViews_pfn
       D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original;
extern D3D11_OMGetRenderTargets_pfn
       D3D11_OMGetRenderTargets_Original;
extern D3D11_OMGetRenderTargetsAndUnorderedAccessViews_pfn
       D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original;
extern D3D11_ClearRenderTargetView_pfn
       D3D11_ClearRenderTargetView_Original;
extern D3D11_ClearDepthStencilView_pfn
       D3D11_ClearDepthStencilView_Original;

extern D3D11_PSSetSamplers_pfn
       D3D11_PSSetSamplers_Original;

extern D3D11_VSSetShader_pfn
       D3D11_VSSetShader_Original;
extern D3D11_PSSetShader_pfn
       D3D11_PSSetShader_Original;
extern D3D11_GSSetShader_pfn
       D3D11_GSSetShader_Original;
extern D3D11_HSSetShader_pfn
       D3D11_HSSetShader_Original;
extern D3D11_DSSetShader_pfn
       D3D11_DSSetShader_Original;
extern D3D11_CSSetShader_pfn
       D3D11_CSSetShader_Original;

extern D3D11_VSGetShader_pfn
       D3D11_VSGetShader_Original;
extern D3D11_PSGetShader_pfn
       D3D11_PSGetShader_Original;
extern D3D11_GSGetShader_pfn
       D3D11_GSGetShader_Original;
extern D3D11_HSGetShader_pfn
       D3D11_HSGetShader_Original;
extern D3D11_DSGetShader_pfn
       D3D11_DSGetShader_Original;
extern D3D11_CSGetShader_pfn
       D3D11_CSGetShader_Original;

extern D3D11_GetData_pfn
       D3D11_GetData_Original;

extern D3D11_CopyResource_pfn
       D3D11_CopyResource_Original;
extern D3D11_CopySubresourceRegion_pfn
       D3D11_CopySubresourceRegion_Original;
extern D3D11_UpdateSubresource1_pfn
       D3D11_UpdateSubresource1_Original;
extern D3D11_ResolveSubresource_pfn
       D3D11_ResolveSubresource_Original;


using D3D11On12CreateDevice_pfn =
  HRESULT (WINAPI *)(              _In_ IUnknown*             pDevice,
                                        UINT                  Flags,
  _In_reads_opt_( FeatureLevels ) CONST D3D_FEATURE_LEVEL*    pFeatureLevels,
                                        UINT                  FeatureLevels,
            _In_reads_opt_( NumQueues ) IUnknown* CONST*      ppCommandQueues,
                                        UINT                  NumQueues,
                                        UINT                  NodeMask,
                       _COM_Outptr_opt_ ID3D11Device**        ppDevice,
                       _COM_Outptr_opt_ ID3D11DeviceContext** ppImmediateContext,
                       _Out_opt_        D3D_FEATURE_LEVEL*    pChosenFeatureLevel );

extern "C" SK_API D3D11On12CreateDevice_pfn extern D3D11On12CreateDevice;


#define SK_D3D11_Decl(x) extern "C" extern FARPROC (x)

SK_D3D11_Decl (D3D11CreateDeviceForD3D12);
SK_D3D11_Decl (CreateDirect3D11DeviceFromDXGIDevice);
SK_D3D11_Decl (CreateDirect3D11SurfaceFromDXGISurface);
//SK_D3D11_Decl (D3D11On12CreateDevice);
SK_D3D11_Decl (EnableFeatureLevelUpgrade);
SK_D3D11_Decl (OpenAdapter10);
SK_D3D11_Decl (OpenAdapter10_2);
SK_D3D11_Decl (D3D11CoreCreateLayeredDevice);
SK_D3D11_Decl (D3D11CoreGetLayeredDeviceSize);
SK_D3D11_Decl (D3D11CoreRegisterLayers);
SK_D3D11_Decl (D3DPerformance_BeginEvent);
SK_D3D11_Decl (D3DPerformance_EndEvent);
SK_D3D11_Decl (D3DPerformance_GetStatus);
SK_D3D11_Decl (D3DPerformance_SetMarker);





struct SK_ImGui_D3D11Ctx
{
  SK_ComPtr <ID3D11Device>             pDevice;
  SK_ComPtr <ID3D11DeviceContext>      pDevCtx;

  DXGI_FORMAT                          RTVFormat = DXGI_FORMAT_UNKNOWN;

  SK_ComPtr <ID3D11Texture2D>          pFontTexture;
  SK_ComPtr <ID3D11ShaderResourceView> pFontTextureView;

  SK_ComPtr <ID3D11VertexShader>       pVertexShader;
  SK_ComPtr <ID3D11PixelShader>        pPixelShader;

  struct {
    struct {
      SK_ComPtr <ID3D11VertexShader>   pPixelShader;
      SK_ComPtr < ID3D11PixelShader  > pVertexShader;
    } SteamHDR, uPlayHDR;
  } overlays;

  SK_ComPtr <ID3D11InputLayout>        pInputLayout;

  SK_ComPtr <ID3D11Buffer>             pVertexConstantBuffer;
  SK_ComPtr <ID3D11Buffer>             pPixelConstantBuffer;

  struct {
    SK_ComPtr <ID3D11SamplerState>     pFont_clamp;
    SK_ComPtr <ID3D11SamplerState>     pFont_wrap;
  } samplers;

  SK_ComPtr <ID3D11BlendState>         pBlendState;
  SK_ComPtr <ID3D11RasterizerState>    pRasterizerState;
  SK_ComPtr <ID3D11DepthStencilState>  pDepthStencilState;

  struct FrameHeap
  {
    struct buffer_s : SK_ComPtr <ID3D11Buffer>
    {
      INT size;
    } Vb, Ib;
  } frame_heaps [DXGI_MAX_SWAP_CHAIN_BUFFERS];
};

// {DEC73284-D747-44CD-8E90-F6FC58754567}
static const GUID IID_SKD3D11RenderCtx =
{ 0xdec73284, 0xd747, 0x44cd, { 0x8e, 0x90, 0xf6, 0xfc, 0x58, 0x75, 0x45, 0x67 } };

struct SK_D3D11_RenderCtx {
  SK_Thread_SharedRecursiveMutex            _ctx_lock;

  SK_ComPtr <ID3D11Device>                  _pDevice          = nullptr;
  SK_ComPtr <ID3D11DeviceContext>           _pDeviceCtx       = nullptr;
  SK_ComPtr <IDXGISwapChain>                _pSwapChain       = nullptr;

  reshade::api::effect_runtime*             _pReShadeRuntime  = nullptr;

  struct FrameCtx {
    SK_D3D11_RenderCtx*                     pRoot             = nullptr;  

    //struct FenceCtx : SK_ComPtr <ID3D12Fence> {
    //  HANDLE                              event             =       0;
    //  volatile UINT64                     value             =       0;
    //} fence;

    SK_ComPtr <ID3D11Texture2D>             pRenderOutput     = nullptr;
    UINT                                    iBufferIdx        =UINT_MAX;

    struct {
      SK_ComPtr <ID3D11Texture2D>           pSwapChainCopy    = nullptr;
      SK_ComPtr <ID3D11RenderTargetView>    pRTV              = nullptr;
      SK_ComPtr <ID3D11UnorderedAccessView> pUAV              = nullptr;
      D3D11_RECT                            scissor           = {     };
      D3D11_VIEWPORT                        vp                = {     };
    } hdr;

    struct {
      SK_ComPtr <ID3D11Texture2D>           pSwapChainCopy    = nullptr;
      // Copy of last scanned-out frame so that skipped frames never tear
    } latent_sync;

    ~FrameCtx (void);
  };

  SK_ComPtr <ID3D11BlendState>              pGenericBlend     = nullptr;

  std::vector <FrameCtx> frames_;

  void present (IDXGISwapChain*      pSwapChain);
  void release (IDXGISwapChain*      pSwapChain);
  bool init    (IDXGISwapChain*      pSwapChain,
                ID3D11Device*        pDevice,
                ID3D11DeviceContext* pDeviceCtx);
};

extern SK_LazyGlobal <SK_ImGui_D3D11Ctx>  _imgui_d3d11;
extern SK_LazyGlobal <SK_D3D11_RenderCtx> _d3d11_rbk;



template <class _T>
static
__forceinline
UINT
calc_count (_T** arr, UINT max_count) noexcept
{
  for ( INT i = sk::narrow_cast <INT> (max_count - 1) ;
            i >= 0 ;
          --i )
  {
    if (arr [i] != nullptr)
      return i + 1;
  }

  return max_count;
}


#define SK_D3D11_MAX_SCISSORS \
  D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE

#define D3D11_SHADER_MAX_INSTANCES_PER_CLASS 256

struct D3DX11_STATE_BLOCK
{
  ID3D11VertexShader*        VS                    = nullptr;
  ID3D11SamplerState*        VSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]             = { };
  ID3D11ShaderResourceView*  VSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]      = { };
  ID3D11Buffer*              VSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
  ID3D11ClassInstance*       VSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS]              = { };
  UINT                       VSInterfaceCount      = D3D11_SHADER_MAX_INSTANCES_PER_CLASS;

  ID3D11GeometryShader*      GS                    = nullptr;
  ID3D11SamplerState*        GSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]             = { };
  ID3D11ShaderResourceView*  GSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]      = { };
  ID3D11Buffer*              GSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
  ID3D11ClassInstance*       GSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS]              = { };
  UINT                       GSInterfaceCount      = D3D11_SHADER_MAX_INSTANCES_PER_CLASS;

  ID3D11HullShader*          HS                    = nullptr;
  ID3D11SamplerState*        HSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]             = { };
  ID3D11ShaderResourceView*  HSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]      = { };
  ID3D11Buffer*              HSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
  ID3D11ClassInstance*       HSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS]              = { };
  UINT                       HSInterfaceCount      = D3D11_SHADER_MAX_INSTANCES_PER_CLASS;

  ID3D11DomainShader*        DS                    = nullptr;
  ID3D11SamplerState*        DSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]             = { };
  ID3D11ShaderResourceView*  DSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]      = { };
  ID3D11Buffer*              DSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
  ID3D11ClassInstance*       DSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS]              = { };
  UINT                       DSInterfaceCount      = D3D11_SHADER_MAX_INSTANCES_PER_CLASS;

  ID3D11PixelShader*         PS                    = nullptr;
  ID3D11SamplerState*        PSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]             = { };
  ID3D11ShaderResourceView*  PSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]      = { };
  ID3D11Buffer*              PSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
  ID3D11ClassInstance*       PSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS]              = { };
  UINT                       PSInterfaceCount      = D3D11_SHADER_MAX_INSTANCES_PER_CLASS;

  ID3D11ComputeShader*       CS                    = nullptr;
  ID3D11SamplerState*        CSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]             = { };
  ID3D11ShaderResourceView*  CSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]      = { };
  ID3D11Buffer*              CSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
  ID3D11ClassInstance*       CSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS]              = { };
  UINT                       CSInterfaceCount      = D3D11_SHADER_MAX_INSTANCES_PER_CLASS;
  ID3D11UnorderedAccessView* CSUnorderedAccessViews [D3D11_PS_CS_UAV_REGISTER_COUNT]                    = { };

  ID3D11Buffer*              IAVertexBuffers        [D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = { };
  UINT                       IAVertexBuffersStrides [D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = { };
  UINT                       IAVertexBuffersOffsets [D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = { };
  ID3D11Buffer*              IAIndexBuffer              = nullptr;
  DXGI_FORMAT                IAIndexBufferFormat        = DXGI_FORMAT_UNKNOWN;
  UINT                       IAIndexBufferOffset        = 0;
  ID3D11InputLayout*         IAInputLayout              = nullptr;
  D3D11_PRIMITIVE_TOPOLOGY   IAPrimitiveTopology        = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

  ID3D11RenderTargetView*    OMRenderTargets        [D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { };
  UINT                       OMRenderTargetCount   = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
  ID3D11UnorderedAccessView* OMUnorderedAccessViews [D3D11_1_UAV_SLOT_COUNT] = { };
  UINT                       OMUnorderedAccessViewCount = D3D11_1_UAV_SLOT_COUNT;
  ID3D11DepthStencilView*    OMRenderTargetStencilView  = nullptr;
  ID3D11DepthStencilState*   OMDepthStencilState        = nullptr;
  UINT                       OMDepthStencilRef          = 0x0;
  ID3D11BlendState*          OMBlendState               = nullptr;
  FLOAT                      OMBlendFactor          [4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  UINT                       OMSampleMask               = 0x0;

  UINT                       RSViewportCount       = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
  D3D11_VIEWPORT             RSViewports            [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = { };
  UINT                       RSScissorRectCount    = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
  D3D11_RECT                 RSScissorRects         [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = { };
  ID3D11RasterizerState*     RSRasterizerState     = nullptr;

  ID3D11Buffer*              SOBuffers              [4] = { };
  ID3D11Predicate*           Predication                = nullptr;
  BOOL                       PredicationValue           = FALSE;

  ID3DDeviceContextState*    DeviceContext              = nullptr;
};


bool SK_D3D11_IsDirectCopyCompatible (DXGI_FORMAT src, DXGI_FORMAT dst);

void SK_D3D11_BeginFrame (void);
void SK_D3D11_EndFrame   (SK_TLS* pTLS = SK_TLS_Bottom ());

void CreateStateblock (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb);
void ApplyStateblock  (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            

std::wstring SK_D3D11_GetDebugNameW    (ID3D11DeviceChild* pD3D11Obj);
std::string  SK_D3D11_GetDebugNameA    (ID3D11DeviceChild* pD3D11Obj);

// Composite of GetDebugNameW (prioritized) and GetDebugNameA, as UTF8
std::string  SK_D3D11_GetDebugNameUTF8 (ID3D11DeviceChild* pD3D11Obj);




#define D3D11_SHADER_MAX_INSTANCES_PER_CLASS 256
#define D3D11_MAX_SCISSOR_AND_VIEWPORT_ARRAYS \
  D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE

#include <array>

template <typename _Tp, size_t n> using CComPtrArray = std::array <CComPtr <_Tp>, n>;

template <typename _Type>
struct D3D11ShaderState
{
  CComPtr <_Type>                     Shader;
  CComPtrArray <
    ID3D11Buffer,              D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT
  >                                   Constants;
  CComPtrArray <
    ID3D11ShaderResourceView,  D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT
  >                                   Resources;
  CComPtrArray <
    ID3D11UnorderedAccessView, D3D11_PS_CS_UAV_REGISTER_COUNT
  >                                   Unordered;
  CComPtrArray <
    ID3D11SamplerState,        D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT
  >                                   Samplers;
  struct {
    UINT                              Count = D3D11_SHADER_MAX_INSTANCES_PER_CLASS;
    CComPtrArray <
      ID3D11ClassInstance,     D3D11_SHADER_MAX_INSTANCES_PER_CLASS
    >                                 Array;
  } Instances;
};

using _VS = D3D11ShaderState <ID3D11VertexShader>;
using _PS = D3D11ShaderState <ID3D11PixelShader>;
using _GS = D3D11ShaderState <ID3D11GeometryShader>;
using _DS = D3D11ShaderState <ID3D11DomainShader>;
using _HS = D3D11ShaderState <ID3D11HullShader>;
using _CS = D3D11ShaderState <ID3D11ComputeShader>;

// Backup DX state that will be modified to restore it afterwards (unfortunately this is very ugly looking and verbose. Close your eyes!)
struct SK_IMGUI_D3D11StateBlock {
  struct {
    UINT                              RectCount;
    D3D11_RECT                        Rects [D3D11_MAX_SCISSOR_AND_VIEWPORT_ARRAYS];
  } Scissor;

  struct {
    UINT                              ArrayCount;
    D3D11_VIEWPORT                    Array [D3D11_MAX_SCISSOR_AND_VIEWPORT_ARRAYS];
  } Viewport;

  struct {
    CComPtr <ID3D11RasterizerState>   State;
  } Rasterizer;

  struct {
    CComPtr <ID3D11BlendState>        State;
    FLOAT                             Factor [4];
    UINT                              SampleMask;
  } Blend;

  struct {
    UINT                              StencilRef;
    CComPtr <ID3D11DepthStencilState> State;
  } DepthStencil;

  struct {
    _VS                               Vertex;
    _PS                               Pixel;
    _GS                               Geometry;
    _DS                               Domain;
    _HS                               Hull;
    _CS                               Compute;
  } Shaders;

  struct {
    struct {
      CComPtr <ID3D11Buffer>          Pointer;
      DXGI_FORMAT                     Format;
      UINT                            Offset;
    } Index;

    struct {
      CComPtrArray <
        ID3D11Buffer,                 D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT
      >                               Pointers;
      UINT                            Strides  [D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
      UINT                            Offsets  [D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    } Vertex;
  } Buffers;

  struct {
    CComPtrArray <
      ID3D11Buffer,                            D3D11_SO_BUFFER_SLOT_COUNT
    >                                 Buffers;
    const UINT                        Offsets [D3D11_SO_BUFFER_SLOT_COUNT] = { };
  } StreamOutput;

  struct {
    CComPtr <ID3D11Predicate>         Predication;
    BOOL                              PredicationValue;
  } Predication;

  struct {
    CComPtrArray <
      ID3D11RenderTargetView,         D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT
    >                                 RenderTargetViews;
    UINT                              RenderTargetViewCount    = 0;
    CComPtr <
      ID3D11DepthStencilView
    >                                 DepthStencilView;
    CComPtrArray <
      ID3D11UnorderedAccessView,      D3D11_1_UAV_SLOT_COUNT
    >                                 UnorderedAccessViews;
    UINT                              UnorderedAccessViewCount = 0;
  } RenderTargets;

  D3D11_PRIMITIVE_TOPOLOGY            PrimitiveTopology;
  CComPtr <ID3D11InputLayout>         InputLayout;

  CComPtr <ID3D11Device>              Device;
  D3D_FEATURE_LEVEL                   FeatureLevel;

  enum _StateMask : DWORD {
    VertexStage          = 0x00001,
    PixelStage           = 0x00002,
    GeometryStage        = 0x00004,
    HullStage            = 0x00008,
    DomainStage          = 0x00010,
    ComputeStage         = 0x00020,
    RasterizerState      = 0x00040,
    BlendState           = 0x00080,
    OutputMergeState     = 0x00100,
    DepthStencilState    = 0x00200,
    InputAssemblerState  = 0x00400,
    ViewportState        = 0x00800,
    ScissorState         = 0x01000,
    RenderTargetState    = 0x02000,
    UnorderedAccessState = 0x04000,
    StreamOutputState    = 0x08000,
    PredicationState     = 0x10000,
    _StateMask_All       = 0xffffffff
  };

#define        STAGE_INPUT_RESOURCE_SLOT_COUNT \
  D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT
#define        STAGE_CONSTANT_BUFFER_API_SLOT_COUNT \
  D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT
#define        STAGE_SAMPLER_SLOT_COUNT \
  D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT

#define _Stage(StageName)   Shaders.##StageName
#define Stage_Get(_Tp,Func) pDevCtx->##_Tp##Get##Func
#define Stage_Set(_Tp,Func) pDevCtx->##_Tp##Set##Func

  static constexpr UINT
    minus_one [D3D11_PS_CS_UAV_REGISTER_COUNT] =
    { std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max () };

  void Capture ( ID3D11DeviceContext* pDevCtx,
                 DWORD                iStateMask = _StateMask_All )
  {
    if (pDevCtx == nullptr)
      return;

    pDevCtx->GetDevice (&Device.p);

    FeatureLevel =
      Device->GetFeatureLevel ();

    if (iStateMask & ScissorState)        Scissor.RectCount   = D3D11_MAX_SCISSOR_AND_VIEWPORT_ARRAYS;
    if (iStateMask & ViewportState)       Viewport.ArrayCount = D3D11_MAX_SCISSOR_AND_VIEWPORT_ARRAYS;

    if (iStateMask & ScissorState)
       pDevCtx->RSGetScissorRects      ( &Scissor.RectCount,
                                          Scissor.Rects    );
    if (iStateMask & ViewportState)
       pDevCtx->RSGetViewports         ( &Viewport.ArrayCount,
                                          Viewport.Array   );
    if (iStateMask & RasterizerState)
      pDevCtx->RSGetState              ( &Rasterizer.State );

    if (iStateMask & BlendState)
       pDevCtx->OMGetBlendState        ( &Blend.State,
                                          Blend.Factor,
                                         &Blend.SampleMask );

    if (iStateMask & DepthStencilState)
       pDevCtx->OMGetDepthStencilState ( &DepthStencil.State,
                                         &DepthStencil.StencilRef );

#define _BackupStage(_Tp,StageName)                                                    \
    if (iStateMask & StageName##Stage) {                                               \
      Stage_Get(_Tp,ShaderResources)     ( 0, STAGE_INPUT_RESOURCE_SLOT_COUNT,         \
                                            &_Stage(StageName).Resources       [0].p );\
      Stage_Get(_Tp,ConstantBuffers)     ( 0, STAGE_CONSTANT_BUFFER_API_SLOT_COUNT,    \
                                            &_Stage(StageName).Constants       [0].p );\
      Stage_Get(_Tp,ConstantBuffers)     ( 0, STAGE_CONSTANT_BUFFER_API_SLOT_COUNT,    \
                                            &_Stage(StageName).Constants       [0].p );\
      Stage_Get(_Tp,Samplers)            ( 0, STAGE_SAMPLER_SLOT_COUNT,                \
                                            &_Stage(StageName).Samplers        [0].p );\
      Stage_Get(_Tp,Shader)              (  &_Stage(StageName).Shader,                 \
                                            &_Stage(StageName).Instances.Array [0].p,  \
                                            &_Stage(StageName).Instances.Count       );\
    }

#define _BackupStageUAV(_Tp,StageName)                                         \
      Stage_Get(_Tp,UnorderedAccessViews) ( 0, D3D11_PS_CS_UAV_REGISTER_COUNT, \
                                            &_Stage(StageName).Unordered [0].p );

    _BackupStage        ( VS, Vertex   );
    _BackupStage        ( PS, Pixel    );

    if (FeatureLevel >= D3D_FEATURE_LEVEL_10_0)
    {
      _BackupStage      ( GS, Geometry );

      if (FeatureLevel >= D3D_FEATURE_LEVEL_11_0)
      {
        _BackupStage    ( DS, Domain   );
        _BackupStage    ( HS, Hull     );
        _BackupStageUAV ( CS, Compute  );
      }
    }

    if (iStateMask & InputAssemblerState)
    {
      const int max_vtx_input_slots =
        FeatureLevel >= D3D_FEATURE_LEVEL_11_0 ? D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT
                                               : D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;

      pDevCtx->IAGetPrimitiveTopology ( &PrimitiveTopology );
      pDevCtx->IAGetIndexBuffer       ( &Buffers.Index.Pointer,
                                        &Buffers.Index.Format,
                                        &Buffers.Index.Offset );
      pDevCtx->IAGetVertexBuffers     ( 0, max_vtx_input_slots,
                                         &Buffers.Vertex.Pointers [0].p,
                                          Buffers.Vertex.Strides,
                                          Buffers.Vertex.Offsets );
      pDevCtx->IAGetInputLayout       ( &InputLayout );
    }

    if (iStateMask & StreamOutputState)
    {
      if (FeatureLevel >= D3D_FEATURE_LEVEL_10_0)
      {
        pDevCtx->SOGetTargets         ( D3D11_SO_BUFFER_SLOT_COUNT, &StreamOutput.Buffers [0].p );
      }
    }

    if (iStateMask & PredicationState)
    {
      pDevCtx->GetPredication         ( &Predication.Predication.p, &Predication.PredicationValue );
    }

    if ( (iStateMask & RenderTargetState) ||
         (iStateMask & UnorderedAccessState) )
    {
      auto ppRtv0 = &RenderTargets.RenderTargetViews   [0].p;
      auto ppDsv  = &RenderTargets.DepthStencilView       .p;
      auto ppUav0 = &RenderTargets.UnorderedAccessViews[0].p;

      pDevCtx->OMGetRenderTargets (D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, ppRtv0, ppDsv);

      RenderTargets.RenderTargetViewCount =
        calc_count (&RenderTargets.RenderTargetViews [0].p, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

      if (FeatureLevel >= D3D_FEATURE_LEVEL_11_0 && (iStateMask & UnorderedAccessState))
      {
        pDevCtx->OMGetRenderTargetsAndUnorderedAccessViews(
                                          RenderTargets.RenderTargetViewCount,
                           ppRtv0, ppDsv, RenderTargets.RenderTargetViewCount,
                                       FeatureLevel >= D3D_FEATURE_LEVEL_11_1 ?
                 D3D11_1_UAV_SLOT_COUNT - RenderTargets.RenderTargetViewCount :
         D3D11_PS_CS_UAV_REGISTER_COUNT - RenderTargets.RenderTargetViewCount,
                               ppUav0);

        RenderTargets.UnorderedAccessViewCount =
                   calc_count (ppUav0, FeatureLevel >= D3D_FEATURE_LEVEL_11_1 ?
                 D3D11_1_UAV_SLOT_COUNT - RenderTargets.RenderTargetViewCount :
         D3D11_PS_CS_UAV_REGISTER_COUNT - RenderTargets.RenderTargetViewCount);
      }
    }
  }

  void Apply ( ID3D11DeviceContext* pDevCtx,
               DWORD                iStateMask = _StateMask_All )
  {
    if (pDevCtx == nullptr)
      return;

    if ( (iStateMask & RenderTargetState) ||
         (iStateMask & UnorderedAccessState) )
    {
      if (FeatureLevel < D3D_FEATURE_LEVEL_11_0 || !(iStateMask & UnorderedAccessState))
      {
        pDevCtx->OMSetRenderTargets (  RenderTargets.RenderTargetViewCount,
                                      &RenderTargets.RenderTargetViews [0].p,
                                       RenderTargets.DepthStencilView.p );
      }

      else
      {
        if (RenderTargets.UnorderedAccessViewCount > 0)
        {
          pDevCtx->OMSetRenderTargetsAndUnorderedAccessViews (
                                        D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
                                        nullptr, nullptr,
                                         RenderTargets.RenderTargetViewCount,
                                         RenderTargets.UnorderedAccessViewCount,
                                        &RenderTargets.UnorderedAccessViews [0].p, minus_one );

          std::fill ( RenderTargets.UnorderedAccessViews.begin (),
                      RenderTargets.UnorderedAccessViews.end   (),
                        nullptr );
        }
      }

      std::fill ( RenderTargets.RenderTargetViews.begin (),
                  RenderTargets.RenderTargetViews.end   (),
                    nullptr );
    }

    if (iStateMask & PredicationState)
    {
      pDevCtx->SetPredication         ( Predication.Predication, Predication.PredicationValue );
    }

    if (iStateMask & StreamOutputState)
    {
      if (FeatureLevel >= D3D_FEATURE_LEVEL_10_0)
      {
        pDevCtx->SOSetTargets         ( D3D11_SO_BUFFER_SLOT_COUNT, &StreamOutput.Buffers [0].p,
                                                                     StreamOutput.Offsets );

        std::fill ( StreamOutput.Buffers.begin (),
                    StreamOutput.Buffers.end   (), nullptr );
      }
    }

    if (iStateMask & InputAssemblerState)
    {
      pDevCtx->IASetPrimitiveTopology ( PrimitiveTopology );
      pDevCtx->IASetIndexBuffer       ( Buffers.Index.Pointer,
                                        Buffers.Index.Format,
                                        Buffers.Index.Offset );
                                        Buffers.Index.Pointer = nullptr;
      pDevCtx->IASetVertexBuffers     ( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
                                       &Buffers.Vertex.Pointers [0].p,
                                        Buffers.Vertex.Strides,
                                        Buffers.Vertex.Offsets );
                            std::fill ( Buffers.Vertex.Pointers.begin (),
                                        Buffers.Vertex.Pointers.end   (),
                                          nullptr );

      pDevCtx->IASetInputLayout       ( InputLayout );
                                        InputLayout = nullptr;
    }

  #define _RestoreStage(_Tp,StageName)                                              \
    if (iStateMask & StageName##Stage) {                                            \
      Stage_Set(_Tp,Shader)         (   _Stage(StageName).Shader,                   \
                                       &_Stage(StageName).Instances.Array  [0].p,   \
                                        _Stage(StageName).Instances.Count );        \
                            std::fill ( _Stage(StageName).Instances.Array.begin ( ),\
                                        _Stage(StageName).Instances.Array.end   ( ),\
                                         nullptr );                                 \
      Stage_Set(_Tp,Samplers)       ( 0, STAGE_SAMPLER_SLOT_COUNT,                  \
                                       &_Stage(StageName).Samplers        [0].p );  \
                            std::fill ( _Stage(StageName).Samplers.begin  ( ),      \
                                        _Stage(StageName).Samplers.end    ( ),      \
                                          nullptr );                                \
      Stage_Set(_Tp,ConstantBuffers)( 0, STAGE_CONSTANT_BUFFER_API_SLOT_COUNT,      \
                                       &_Stage(StageName).Constants       [0].p );  \
                            std::fill ( _Stage(StageName).Constants.begin ( ),      \
                                        _Stage(StageName).Constants.end   ( ),      \
                                           nullptr );                               \
      Stage_Set(_Tp,ShaderResources)( 0, STAGE_INPUT_RESOURCE_SLOT_COUNT,           \
                                       &_Stage(StageName).Resources       [0].p );  \
                            std::fill ( _Stage(StageName).Resources.begin ( ),      \
                                        _Stage(StageName).Resources.end   ( ),      \
                                          nullptr );                                \
    }

  #define _RestoreStageUAV(_Tp,StageName)                                           \
      Stage_Set(_Tp,UnorderedAccessViews)                                           \
                                    ( 0, D3D11_PS_CS_UAV_REGISTER_COUNT,            \
                                       &_Stage(StageName).Unordered       [0].p,    \
                                        minus_one );                                \
                            std::fill ( _Stage(StageName).Unordered.begin ( ),      \
                                        _Stage(StageName).Unordered.end   ( ),      \
                                          nullptr );

    if (FeatureLevel >= D3D_FEATURE_LEVEL_10_0)
    {
      if (FeatureLevel >= D3D_FEATURE_LEVEL_11_0)
      {
        _RestoreStageUAV ( CS, Compute  );
        _RestoreStage    ( HS, Hull     );
        _RestoreStage    ( DS, Domain   );
      }
      _RestoreStage      ( GS, Geometry );
    }
    _RestoreStage        ( PS, Pixel    );
    _RestoreStage        ( VS, Vertex   );

    if (iStateMask & DepthStencilState)
    {  pDevCtx->OMSetDepthStencilState ( DepthStencil.State,
                                         DepthStencil.StencilRef );
                                         DepthStencil.State = nullptr;
    }

    if (iStateMask & BlendState)
    {  pDevCtx->OMSetBlendState        ( Blend.State,
                                         Blend.Factor,
                                         Blend.SampleMask);
                                         Blend.State        = nullptr;
    }

    if (iStateMask & RasterizerState)
    {
      pDevCtx->RSSetState              ( Rasterizer.State );
                                         Rasterizer.State   = nullptr;
    }

    if (iStateMask & ViewportState)
       pDevCtx->RSSetViewports         ( Viewport.ArrayCount, Viewport.Array );

    if (iStateMask & ScissorState)
       pDevCtx->RSSetScissorRects      ( Scissor.RectCount,   Scissor.Rects  );
    }
};

extern std::pair <BOOL*, BOOL> SK_ImGui_FlagDrawing_OnD3D11Ctx (UINT dev_idx);
extern bool                    SK_ImGui_IsDrawing_OnD3D11Ctx   (UINT& dev_idx, ID3D11DeviceContext* pDevCtx);

void SK_D3D11_InitMutexes (void);
BOOL SK_D3D11_SetWrappedImmediateContext ( ID3D11Device        *pDev,
                                           ID3D11DeviceContext *pDevCtx );

extern bool SK_GL_OnD3D11;