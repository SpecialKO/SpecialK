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

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <Windows.h>

#include <SpecialK/diagnostics/compatibility.h>

#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/command.h>
#include <SpecialK/config.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/render_backend.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>

extern LARGE_INTEGER SK_QueryPerf (void);
#include <SpecialK/framerate.h>

#include <atlbase.h>

#include <d3d11.h>
#include <d3d11_1.h>

#include <algorithm>


// For texture caching to work correctly ...
//   DarkSouls3 seems to underflow references on occasion!!!
#define DS3_REF_TWEAK

namespace SK
{
  namespace DXGI
  {
    struct PipelineStatsD3D11
    {
      struct StatQueryD3D11  
      {
        ID3D11Query* async  = nullptr;
        bool         active = false;
      } query;

      D3D11_QUERY_DATA_PIPELINE_STATISTICS
                 last_results = { 0 };
    } pipeline_stats_d3d11;
  };
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


extern void WaitForInitDXGI (void);

HMODULE SK::DXGI::hModD3D11 = 0;

volatile LONG SK_D3D11_tex_init = FALSE;
volatile LONG  __d3d11_ready    = FALSE;

void WaitForInitD3D11 (void)
{
  while (! InterlockedCompareExchange (&__d3d11_ready, FALSE, FALSE))
    Sleep (config.system.init_delay);
}


typedef enum D3DX11_IMAGE_FILE_FORMAT {
  D3DX11_IFF_BMP          = 0,
  D3DX11_IFF_JPG          = 1,
  D3DX11_IFF_PNG          = 3,
  D3DX11_IFF_DDS          = 4,
  D3DX11_IFF_TIFF         = 10,
  D3DX11_IFF_GIF          = 11,
  D3DX11_IFF_WMP          = 12,
  D3DX11_IFF_FORCE_DWORD  = 0x7fffffff
} D3DX11_IMAGE_FILE_FORMAT, *LPD3DX11_IMAGE_FILE_FORMAT;

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
} D3DX11_IMAGE_INFO, *LPD3DX11_IMAGE_INFO;


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
} D3DX11_IMAGE_LOAD_INFO, *LPD3DX11_IMAGE_LOAD_INFO;

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


void  __stdcall SK_D3D11_TexCacheCheckpoint    ( void);
bool  __stdcall SK_D3D11_TextureIsCached       ( ID3D11Texture2D*     pTex );
void  __stdcall SK_D3D11_UseTexture            ( ID3D11Texture2D*     pTex );
void  __stdcall SK_D3D11_RemoveTexFromCache    ( ID3D11Texture2D*     pTex );

void  __stdcall SK_D3D11_UpdateRenderStats     ( IDXGISwapChain*      pSwapChain );

bool __stdcall SK_D3D11_TextureIsCached    (ID3D11Texture2D* pTex);
void __stdcall SK_D3D11_RemoveTexFromCache (ID3D11Texture2D* pTex);

//#define FULL_RESOLUTION

D3DX11CreateTextureFromFileW_pfn D3DX11CreateTextureFromFileW = nullptr;
D3DX11GetImageInfoFromFileW_pfn  D3DX11GetImageInfoFromFileW  = nullptr;
HMODULE                          hModD3DX11_43                = nullptr;

#ifdef NO_TLS
std::set <DWORD> texinject_tids;
CRITICAL_SECTION cs_texinject;
#else
#include <SpecialK/tls.h>
#endif

bool SK_D3D11_IsTexInjectThread (DWORD dwThreadId = GetCurrentThreadId ())
{
#ifdef NO_TLS
  bool bRet = false;

  EnterCriticalSection (&cs_texinject);
  bRet = (texinject_tids.count (dwThreadId) > 0);
  LeaveCriticalSection (&cs_texinject);

  return bRet;
#else
  UNREFERENCED_PARAMETER (dwThreadId);

  SK_TLS* pTLS = SK_GetTLS ();

  if (pTLS != nullptr)
    return pTLS->d3d11.texinject_thread;
  else {
    dll_log.Log (L"[ SpecialK ] >> Thread-Local Storage is BORKED! <<");
    return false;
  }
#endif
}

void
SK_D3D11_ClearTexInjectThread ( DWORD dwThreadId = GetCurrentThreadId () )
{
#ifdef NO_TLS
  EnterCriticalSection (&cs_texinject);
  texinject_tids.erase (dwThreadId);
  LeaveCriticalSection (&cs_texinject);
#else
  UNREFERENCED_PARAMETER (dwThreadId);

  SK_TLS* pTLS = SK_GetTLS ();

  if (pTLS != nullptr)
    pTLS->d3d11.texinject_thread = false;
  else
    dll_log.Log (L"[ SpecialK ] >> Thread-Local Storage is BORKED! <<");
#endif
}

void
SK_D3D11_SetTexInjectThread ( DWORD dwThreadId = GetCurrentThreadId () )
{
#ifdef NO_TLS
  EnterCriticalSection (&cs_texinject);
  texinject_tids.insert (dwThreadId);
  LeaveCriticalSection (&cs_texinject);
#else
  UNREFERENCED_PARAMETER (dwThreadId);

  SK_TLS* pTLS = SK_GetTLS ();

  if (pTLS != nullptr)
    pTLS->d3d11.texinject_thread = true;
  else
    dll_log.Log (L"[ SpecialK ] >> Thread-Local Storage is BORKED! <<");
#endif
}

typedef ULONG (WINAPI *IUnknown_Release_pfn) (IUnknown* This);
typedef ULONG (WINAPI *IUnknown_AddRef_pfn)  (IUnknown* This);

IUnknown_Release_pfn IUnknown_Release_Original = nullptr;
IUnknown_AddRef_pfn  IUnknown_AddRef_Original  = nullptr;

__declspec (noinline)
ULONG
WINAPI
IUnknown_Release (IUnknown* This)
{
  if (! SK_D3D11_IsTexInjectThread ())
  {
    ID3D11Texture2D* pTex = nullptr;
    if (SUCCEEDED (This->QueryInterface (IID_PPV_ARGS (&pTex))))
    {
      ULONG count = IUnknown_Release_Original (pTex);

      // If count is == 0, something's screwy
      if (pTex != nullptr && count <= 1)
        SK_D3D11_RemoveTexFromCache (pTex);
    }
  }

  return IUnknown_Release_Original (This);
}

__declspec (noinline)
ULONG
WINAPI
IUnknown_AddRef (IUnknown* This)
{
  if (! SK_D3D11_IsTexInjectThread ())
  {
    ID3D11Texture2D* pTex = (ID3D11Texture2D *)This;//nullptr;

    // This would cause the damn thing to recurse infinitely...
    //if (SUCCEEDED (This->QueryInterface (IID_PPV_ARGS (&pTex)))) {
      if (pTex != nullptr && SK_D3D11_TextureIsCached (pTex))
        SK_D3D11_UseTexture (pTex);
    //}
  }

  return IUnknown_AddRef_Original (This);
}

ID3D11Device* g_pD3D11Dev = nullptr;

unsigned int __stdcall HookD3D11 (LPVOID user);

struct d3d11_caps_t {
  struct {
    bool d3d11_1         = false;
  } feature_level;
} d3d11_caps;

volatile D3D11CreateDeviceAndSwapChain_pfn D3D11CreateDeviceAndSwapChain_Import = nullptr;
volatile D3D11CreateDevice_pfn             D3D11CreateDevice_Import             = nullptr;

void
SK_D3D11_SetDevice ( ID3D11Device           **ppDevice,
                     D3D_FEATURE_LEVEL        FeatureLevel )
{
  if ( ppDevice != nullptr )
  {
    if ( *ppDevice != g_pD3D11Dev )
    {
      dll_log.Log ( L"[  D3D 11  ] >> Device = %ph (Feature Level:%s)",
                      *ppDevice,
                        SK_DXGI_FeatureLevelsToStr ( 1,
                                                      (DWORD *)&FeatureLevel
                                                   ).c_str ()
                  );
      g_pD3D11Dev = *ppDevice;
    }

    if (config.render.dxgi.exception_mode != -1)
      g_pD3D11Dev->SetExceptionMode (config.render.dxgi.exception_mode);

    CComPtr <IDXGIDevice>  pDXGIDev = nullptr;
    CComPtr <IDXGIAdapter> pAdapter = nullptr;

    HRESULT hr =
      (*ppDevice)->QueryInterface ( IID_PPV_ARGS (&pDXGIDev) );

    if ( SUCCEEDED ( hr ) )
    {
      hr =
        pDXGIDev->GetParent ( IID_PPV_ARGS (&pAdapter) );

      if ( SUCCEEDED ( hr ) )
      {
        if ( pAdapter == nullptr )
          return;

        const int iver =
          SK_GetDXGIAdapterInterfaceVer ( pAdapter );

        // IDXGIAdapter3 = DXGI 1.4 (Windows 10+)
        if ( iver >= 3 )
        {
          SK::DXGI::StartBudgetThread ( &pAdapter );
        }
      }
    }
  }
}

extern volatile DWORD SK_D3D11_init_tid;

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
  // Even if the game doesn't care about the feature level, we do.
  D3D_FEATURE_LEVEL ret_level  = D3D_FEATURE_LEVEL_11_1;
  ID3D11Device*     ret_device = nullptr;

  // Allow override of swapchain parameters
  DXGI_SWAP_CHAIN_DESC* swap_chain_desc     = (DXGI_SWAP_CHAIN_DESC *)pSwapChainDesc;
  DXGI_SWAP_CHAIN_DESC  swap_chain_override = { 0 };

  DXGI_LOG_CALL_0 (L"D3D11CreateDeviceAndSwapChain");

  dll_log.LogEx ( true,
                    L"[  D3D 11  ]  <~> Preferred Feature Level(s): <%u> - %s\n",
                      FeatureLevels,
                        SK_DXGI_FeatureLevelsToStr (
                          FeatureLevels,
                            (DWORD *)pFeatureLevels
                        ).c_str ()
                );

  // Optionally Enable Debug Layer
  if (InterlockedAdd (&__d3d11_ready, 0))
  {
    if (config.render.dxgi.debug_layer && (! (Flags & D3D11_CREATE_DEVICE_DEBUG)))
    {
      SK_LOG0 ( ( L" ==> Enabling D3D11 Debug layer" ),
                  L"  D3D 11  " );
      Flags |= D3D11_CREATE_DEVICE_DEBUG;
    }
  }

  else
  {
    if (InterlockedExchangeAdd (&SK_D3D11_init_tid, 0) != GetCurrentThreadId () && SK_GetCallingDLL () != SK_GetDLL ())
      WaitForInitDXGI ();
  }

  //
  // DXGI Adapter Override (for performance)
  //

  SK_DXGI_AdapterOverride ( &pAdapter, &DriverType );

  if (swap_chain_desc != nullptr)
  {
    dll_log.LogEx ( true,
                      L"[   DXGI   ]  SwapChain: (%lux%lu@%4.1f Hz - Scaling: %s) - "
                      L"[%lu Buffers] :: Flags=0x%04X, SwapEffect: %s\n",
                        swap_chain_desc->BufferDesc.Width,
                        swap_chain_desc->BufferDesc.Height,
                        swap_chain_desc->BufferDesc.RefreshRate.Denominator > 0 ?
                 (float)swap_chain_desc->BufferDesc.RefreshRate.Numerator /
                 (float)swap_chain_desc->BufferDesc.RefreshRate.Denominator :
                 (float)swap_chain_desc->BufferDesc.RefreshRate.Numerator,
                        swap_chain_desc->BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED ?
                          L"Unspecified" :
                          swap_chain_desc->BufferDesc.Scaling == DXGI_MODE_SCALING_CENTERED  ?
                            L"Centered" :
                            L"Stretched",
                        swap_chain_desc->BufferCount,
                        swap_chain_desc->Flags,
                        swap_chain_desc->SwapEffect == 0         ?
                          L"Discard" :
                          swap_chain_desc->SwapEffect == 1       ?
                            L"Sequential" :
                            swap_chain_desc->SwapEffect == 2     ?
                              L"<Unknown>" :
                              swap_chain_desc->SwapEffect == 3   ?
                                L"Flip Sequential" :
                                swap_chain_desc->SwapEffect == 4 ?
                                  L"Flip Discard" :
                                  L"<Unknown>");

    swap_chain_override = *swap_chain_desc;
    swap_chain_desc     = &swap_chain_override;

    if ( config.render.dxgi.scaling_mode      != -1 &&
          swap_chain_desc->BufferDesc.Scaling !=
            (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
    {
      dll_log.Log ( L"[  D3D 11  ]  >> Scaling Override "
                    L"(Requested: %s, Using: %s)",
                      SK_DXGI_DescribeScalingMode (
                        swap_chain_desc->BufferDesc.Scaling
                      ),
                        SK_DXGI_DescribeScalingMode (
                          (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode
                        )
                  );

      swap_chain_desc->BufferDesc.Scaling =
        (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode;
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

  HRESULT res;

  DXGI_CALL(res, 
    D3D11CreateDeviceAndSwapChain_Import (pAdapter,
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
                                          ppImmediateContext));

  if (SUCCEEDED (res))
  {
    if (swap_chain_desc != nullptr)
    {
      if ( dwRenderThread == 0x00 ||
           dwRenderThread == GetCurrentThreadId () )
      {
        if ( hWndRender                   != 0 &&
             swap_chain_desc->OutputWindow != 0 &&
             swap_chain_desc->OutputWindow != hWndRender )
          dll_log.Log (L"[  D3D 11  ] Game created a new window?!");

        //if (hWndRender == nullptr || (! IsWindow (hWndRender))) {
          //hWndRender       = swap_chain_desc->OutputWindow;
          //game_window.hWnd = hWndRender;
        //}
      }
    }

    // Assume the first thing to create a D3D11 render device is
    //   the game and that devices never migrate threads; for most games
    //     this assumption holds.
    if ( dwRenderThread == 0x00 ||
         dwRenderThread == GetCurrentThreadId () ) {
      dwRenderThread = GetCurrentThreadId ();
    }

    SK_D3D11_SetDevice ( &ret_device, ret_level );
  }

  if (ppDevice != nullptr)
    *ppDevice = ret_device;

  if (pFeatureLevel != nullptr)
    *pFeatureLevel = ret_level;

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
  DXGI_LOG_CALL_0 (L"D3D11CreateDevice");

  return D3D11CreateDeviceAndSwapChain_Detour (pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, nullptr, nullptr, ppDevice, pFeatureLevel, ppImmediateContext);
}


__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
_In_            ID3D11Device           *This,
_In_      const D3D11_TEXTURE2D_DESC   *pDesc,
_In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
_Out_opt_       ID3D11Texture2D        **ppTexture2D );

D3D11Dev_CreateBuffer_pfn             D3D11Dev_CreateBuffer_Original             = nullptr;
D3D11Dev_CreateTexture2D_pfn          D3D11Dev_CreateTexture2D_Original          = nullptr;
D3D11Dev_CreateRenderTargetView_pfn   D3D11Dev_CreateRenderTargetView_Original   = nullptr;
D3D11Dev_CreateShaderResourceView_pfn D3D11Dev_CreateShaderResourceView_Original = nullptr;

D3D11_RSSetScissorRects_pfn            D3D11_RSSetScissorRects_Original            = nullptr;
D3D11_RSSetViewports_pfn               D3D11_RSSetViewports_Original               = nullptr;
D3D11_VSSetConstantBuffers_pfn         D3D11_VSSetConstantBuffers_Original         = nullptr;
D3D11_PSSetShaderResources_pfn         D3D11_PSSetShaderResources_Original         = nullptr;
D3D11_UpdateSubresource_pfn            D3D11_UpdateSubresource_Original            = nullptr;
D3D11_DrawIndexed_pfn                  D3D11_DrawIndexed_Original                  = nullptr;
D3D11_Draw_pfn                         D3D11_Draw_Original                         = nullptr;
D3D11_DrawIndexedInstanced_pfn         D3D11_DrawIndexedInstanced_Original         = nullptr;
D3D11_DrawIndexedInstancedIndirect_pfn D3D11_DrawIndexedInstancedIndirect_Original = nullptr;
D3D11_DrawInstanced_pfn                D3D11_DrawInstanced_Original                = nullptr;
D3D11_DrawInstancedIndirect_pfn        D3D11_DrawInstancedIndirect_Original        = nullptr;
D3D11_Map_pfn                          D3D11_Map_Original                          = nullptr;

D3D11_CopyResource_pfn          D3D11_CopyResource_Original       = nullptr;
D3D11_UpdateSubresource1_pfn    D3D11_UpdateSubresource1_Original = nullptr;

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateRenderTargetView_Override (
  _In_            ID3D11Device                   *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView )
{
  return D3D11Dev_CreateRenderTargetView_Original (
           This, pResource,
             pDesc, ppRTView );
}

__declspec (noinline)
void
WINAPI
D3D11_RSSetScissorRects_Override (
        ID3D11DeviceContext *This,
        UINT                 NumRects,
  const D3D11_RECT          *pRects )
{
  return D3D11_RSSetScissorRects_Original (This, NumRects, pRects);
}

__declspec (noinline)
void
WINAPI
D3D11_VSSetConstantBuffers_Override (
  ID3D11DeviceContext*  This,
  UINT                  StartSlot,
  UINT                  NumBuffers,
  ID3D11Buffer *const  *ppConstantBuffers )
{
  //dll_log.Log (L"[   DXGI   ] [!]D3D11_VSSetConstantBuffers (%lu, %lu, ...)", StartSlot, NumBuffers);
  return D3D11_VSSetConstantBuffers_Original (This, StartSlot, NumBuffers, ppConstantBuffers );
}

__declspec (noinline)
void
WINAPI
D3D11_PSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews
)
{
  return D3D11_PSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);
}

__declspec (noinline)
void
WINAPI
D3D11_UpdateSubresource_Override (
        ID3D11DeviceContext* This,
        ID3D11Resource      *pDstResource,
        UINT                 DstSubresource,
  const D3D11_BOX           *pDstBox,
  const void                *pSrcData,
        UINT                 SrcRowPitch,
        UINT                 SrcDepthPitch)
{
  //dll_log.Log (L"[   DXGI   ] [!]D3D11_UpdateSubresource (%ph, %lu, %ph, %ph, %lu, %lu)", pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);

  CComPtr <ID3D11Texture2D> pTex = nullptr;

  if (            pDstResource != nullptr &&
       SUCCEEDED (pDstResource->QueryInterface (IID_PPV_ARGS (&pTex))) )
  {
    if (SK_D3D11_TextureIsCached (pTex))
    {
      dll_log.Log (L"[DX11TexMgr] Cached texture was updated... removing from cache!");
      SK_D3D11_RemoveTexFromCache (pTex);
    }

    else
    {
      D3D11_UpdateSubresource_Original (This, pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
      //dll_log.Log (L"[DX11TexMgr] Updated 2D texture...");
      return;
    }
  }

  return D3D11_UpdateSubresource_Original (This, pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}

__declspec (noinline)
HRESULT
WINAPI
D3D11_Map_Override (
   _In_ ID3D11DeviceContext      *This,
   _In_ ID3D11Resource           *pResource,
   _In_ UINT                      Subresource,
   _In_ D3D11_MAP                 MapType,
   _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource )
{
  if (pResource == nullptr)
    return S_OK;

  CComPtr <ID3D11Texture2D> pTex = nullptr;

  if (            pResource != nullptr &&
       SUCCEEDED (pResource->QueryInterface (IID_PPV_ARGS (&pTex))) )
  {
    if (SK_D3D11_TextureIsCached (pTex))
    {
      dll_log.Log (L"[DX11TexMgr] Cached texture was updated... removing from cache!");
      SK_D3D11_RemoveTexFromCache (pTex);
    }

    else
    {
      //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture...");
    }
  }

  return D3D11_Map_Original (This, pResource, Subresource, MapType, MapFlags, pMappedResource);
}

__declspec (noinline)
void
WINAPI
D3D11_CopyResource_Override (
       ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource )
{
  //CComPtr <ID3D11Texture2D> pSrcTex;
  CComPtr <ID3D11Texture2D> pDstTex = nullptr;

  if (            pDstResource != nullptr && 
       SUCCEEDED (pDstResource->QueryInterface (IID_PPV_ARGS (&pDstTex))) )
  {
    if (SK_D3D11_TextureIsCached (pDstTex)) {
      dll_log.Log (L"[DX11TexMgr] Cached texture was modified... removing from cache!");
      SK_D3D11_RemoveTexFromCache (pDstTex);
    }
  }

  return D3D11_CopyResource_Original (This, pDstResource, pSrcResource);
}


__declspec (noinline)
void
WINAPI
D3D11_DrawIndexed_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation )
{
  return D3D11_DrawIndexed_Original ( This, IndexCount,
                                              StartIndexLocation,
                                                BaseVertexLocation );
}

__declspec (noinline)
void
WINAPI
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation )
{
  return D3D11_Draw_Original ( This, VertexCount, StartVertexLocation );
}

__declspec (noinline)
void
WINAPI
D3D11_DrawIndexedInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  return D3D11_DrawIndexedInstanced_Original (This, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

__declspec (noinline)
void
WINAPI
D3D11_DrawIndexedInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  return D3D11_DrawIndexedInstancedIndirect_Original (This, pBufferForArgs, AlignedByteOffsetForArgs);
}

__declspec (noinline)
void
WINAPI
D3D11_DrawInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  return D3D11_DrawInstanced_Original (This, VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

__declspec (noinline)
void
WINAPI
D3D11_DrawInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  return D3D11_DrawInstancedIndirect_Original (This, pBufferForArgs, AlignedByteOffsetForArgs);
}



__declspec (noinline)
void
WINAPI
D3D11_RSSetViewports_Override (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports )
{
  return D3D11_RSSetViewports_Original (This, NumViewports, pViewports);
}


LPVOID pfnD3D11CreateDevice             = nullptr;
LPVOID pfnD3D11CreateDeviceAndSwapChain = nullptr;



struct cache_params_t {
  uint32_t max_entries       = 4096;
  uint32_t min_entries       = 1024;
   int32_t max_size          = 2048L;
   int32_t min_size          = 512L;
  uint32_t min_evict         = 16;
  uint32_t max_evict         = 64;
      bool ignore_non_mipped = false;
} cache_opts;

CRITICAL_SECTION tex_cs;
CRITICAL_SECTION hash_cs;
CRITICAL_SECTION dump_cs;
CRITICAL_SECTION cache_cs;
CRITICAL_SECTION inject_cs;

void WINAPI SK_D3D11_SetResourceRoot      (const wchar_t* root);
void WINAPI SK_D3D11_EnableTexDump        (bool enable);
void WINAPI SK_D3D11_EnableTexInject      (bool enable);
void WINAPI SK_D3D11_EnableTexCache       (bool enable);
void WINAPI SK_D3D11_PopulateResourceList (void);

#include <unordered_set>
#include <unordered_map>
#include <map>

std::unordered_map <uint32_t, std::wstring> tex_hashes;
std::unordered_map <uint32_t, std::wstring> tex_hashes_ex;

std::unordered_map <uint32_t, std::wstring> tex_hashes_ffx;

std::unordered_set <uint32_t>               dumped_textures;
std::unordered_set <uint32_t>               dumped_collisions;
std::unordered_set <uint32_t>               injectable_textures;
std::unordered_set <uint32_t>               injected_collisions;

std::unordered_set <uint32_t>               injectable_ffx; // HACK FOR FFX


SK_D3D11_TexMgr SK_D3D11_Textures;

class SK_D3D11_TexCacheMgr {
public:
};


std::string
SK_D3D11_SummarizeTexCache (void)
{
  char szOut [512];

  snprintf ( szOut, 512, "  Tex Cache: %#5zu MiB   Entries:   %#7zu\n"
                         "  Hits:      %#5lu       Time Saved: %#7.01lf ms\n"
                         "  Evictions: %#5lu",
               SK_D3D11_Textures.AggregateSize_2D >> 20ULL,
               SK_D3D11_Textures.TexRefs_2D.size (),
               SK_D3D11_Textures.RedundantLoads_2D,
               SK_D3D11_Textures.RedundantTime_2D,
               SK_D3D11_Textures.Evicted_2D );

  return szOut;
}

#include <SpecialK/utility.h>

bool         SK_D3D11_need_tex_reset      = false;
int32_t      SK_D3D11_amount_to_purge     = 0L;

bool         SK_D3D11_dump_textures       = false;// true;
bool         SK_D3D11_inject_textures_ffx = false;
bool         SK_D3D11_inject_textures     = false;
bool         SK_D3D11_cache_textures      = false;
bool         SK_D3D11_mark_textures       = false;
std::wstring SK_D3D11_res_root            = L"";

bool
SK_D3D11_TexMgr::isTexture2D (uint32_t crc32, const D3D11_TEXTURE2D_DESC *pDesc)
{
  if (! SK_D3D11_cache_textures)
    return false;

  SK_AutoCriticalSection critical (&tex_cs);

  if (crc32 != 0x00 && HashMap_2D [pDesc->MipLevels].count (crc32))
    return true;

  return false;
}

void
__stdcall
SK_D3D11_ResetTexCache (void)
{
  SK_D3D11_Textures.reset ();
}

#define PSAPI_VERSION           1

#include <psapi.h>
#pragma comment (lib, "psapi.lib")

void
__stdcall
SK_D3D11_TexCacheCheckpoint (void)
{
  static int       iter               = 0;

  static bool      init               = false;
  static ULONGLONG ullMemoryTotal_KiB = 0;
  static HANDLE    hProc              = nullptr;

  if (! init) {
    hProc = GetCurrentProcess ();
    GetPhysicallyInstalledSystemMemory (&ullMemoryTotal_KiB);
    init = true;
  }

  ++iter;

  if ((iter % 3))
    return;

  PROCESS_MEMORY_COUNTERS pmc    = { 0 };
                          pmc.cb = sizeof pmc;

  GetProcessMemoryInfo (hProc, &pmc, sizeof pmc);

  if ( (SK_D3D11_Textures.AggregateSize_2D >> 20ULL) > (uint32_t)cache_opts.max_size    ||
        SK_D3D11_Textures.TexRefs_2D.size ()         >           cache_opts.max_entries ||
        SK_D3D11_need_tex_reset                                                         ||
       (config.mem.reserve / 100.0f) * ullMemoryTotal_KiB 
                                                     < 
                          (pmc.PagefileUsage >> 10UL) )
  {
    //dll_log.Log (L"[DX11TexMgr] DXGI 1.4 Budget Change: Triggered a texture manager purge...");

    SK_D3D11_amount_to_purge =
      static_cast <int32_t> (
        (pmc.PagefileUsage >> 10UL) - (float)(ullMemoryTotal_KiB >> 10ULL) *
                       (config.mem.reserve / 100.0f)
      );

    SK_D3D11_need_tex_reset = false;
    SK_D3D11_Textures.reset ();
  }
}

void
SK_D3D11_TexMgr::reset (void)
{
  std::vector <SK_D3D11_TexMgr::tex2D_descriptor_s> textures;
  textures.reserve (TexRefs_2D.size ());

  uint32_t count  = 0;
  int64_t  purged = 0;

  {
    SK_AutoCriticalSection critical (&tex_cs);

    for ( ID3D11Texture2D* tex : TexRefs_2D ) {
      if (Textures_2D.count (tex))
        textures.push_back (Textures_2D [tex]);
    }
  }

  std::sort ( textures.begin (),
                textures.end (),
      []( SK_D3D11_TexMgr::tex2D_descriptor_s a,
          SK_D3D11_TexMgr::tex2D_descriptor_s b )
    {
      return a.last_used < b.last_used;
    }
  );

  const uint32_t max_count = cache_opts.max_evict;

  for ( SK_D3D11_TexMgr::tex2D_descriptor_s desc : textures ) {
    int64_t mem_size = (int64_t)(desc.mem_size >> 20);

    if (desc.texture != nullptr) {
#ifdef DS3_REF_TWEAK
      int refs = IUnknown_AddRef_Original (desc.texture) - 1;

      if (refs <= 3 && desc.texture->Release () <= 3) {
#else
      int refs = IUnknown_AddRef_Original (desc.texture) - 1;

      if (refs == 1 && desc.texture->Release () <= 1) {
#endif
        for (int i = 0; i < refs; i++) {
          desc.texture->Release ();
        }

        count++;

        purged += mem_size;

        AggregateSize_2D = std::max ((size_t)0, AggregateSize_2D);

        if ( ( (AggregateSize_2D >> 20ULL) <= (uint32_t)cache_opts.min_size &&
                                     count >=           cache_opts.min_evict ) ||
             (SK_D3D11_amount_to_purge     <=           purged              &&
                                     count >=           cache_opts.min_evict ) ||
                                     count >=           max_count )
        {
          SK_D3D11_amount_to_purge = std::max (0, SK_D3D11_amount_to_purge);
          //dll_log.Log ( L"[DX11TexMgr] Purged %llu MiB of texture "
                          //L"data across %lu textures",
                          //purged >> 20ULL, count );

          break;
        }
      } else {
        desc.texture->Release ();
      }
    }
  }
}

ID3D11Texture2D*
SK_D3D11_TexMgr::getTexture2D ( uint32_t              crc32,
                          const D3D11_TEXTURE2D_DESC* pDesc,
                                size_t*               pMemSize,
                                float*                pTimeSaved )
{
  if (! SK_D3D11_cache_textures)
    return nullptr;

  ID3D11Texture2D* pTex2D = nullptr;

  if (isTexture2D (crc32, pDesc))
  {
#ifdef TEST_COLLISIONS
    std::unordered_map <uint32_t, ID3D11Texture2D *>::iterator it =
      HashMap_2D [pDesc->MipLevels].begin ();

    while (it != HashMap_2D [pDesc->MipLevels].end ())
    {
      if (! SK_D3D11_TextureIsCached (it->second))
      {
        ++it;
        continue;
      }
#else
    auto it = HashMap_2D [pDesc->MipLevels][crc32];
#endif
      tex2D_descriptor_s desc2d;

      {
        SK_AutoCriticalSection critical (&tex_cs);

#ifdef TEST_COLISIONS
        desc2d =
          Textures_2D [it->second];
#else
        desc2d =
          Textures_2D [it];
#endif
      }

      D3D11_TEXTURE2D_DESC desc = desc2d.desc;

      if ( desc2d.crc32        == crc32                 &&
           desc.Format         == pDesc->Format         &&
           desc.Width          == pDesc->Width          &&
           desc.Height         == pDesc->Height         &&
           desc.BindFlags      == pDesc->BindFlags      &&
           desc.CPUAccessFlags == pDesc->CPUAccessFlags &&
           desc.Usage          == pDesc->Usage )
      {
        pTex2D = desc2d.texture;

        size_t   size = desc2d.mem_size;
        uint64_t time = desc2d.load_time;

        float   fTime = (float)time * 1000.0f / (float)PerfFreq.QuadPart;

        if (pMemSize != nullptr) {
          *pMemSize = size;
        }

        if (pTimeSaved != nullptr) {
          *pTimeSaved = fTime;
        }

        desc2d.last_used = SK_QueryPerf ().QuadPart;
        desc2d.hits++;

        RedundantData_2D += size;
        RedundantTime_2D += fTime;
        RedundantLoads_2D++;

        return pTex2D;
      }

#ifdef TEST_COLLISIONS
      else if (desc2d.crc32 == crc32)
      {
        dll_log.Log ( L"[DX11TexMgr] ## Hash Collision for Texture: "
                          L"'%08X'!! ## ",
                            crc32 );
      }

      ++it;
    }
#endif
  }

  return pTex2D;
}

bool
__stdcall
SK_D3D11_TextureIsCached (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return false;

  SK_AutoCriticalSection critical (&cache_cs);

  return SK_D3D11_Textures.Textures_2D.count (pTex) != 0;
}

void
__stdcall
SK_D3D11_UseTexture (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return;

  SK_AutoCriticalSection critical (&cache_cs);

  if (SK_D3D11_TextureIsCached (pTex)) {
    SK_D3D11_Textures.Textures_2D [pTex].last_used =
      SK_QueryPerf ().QuadPart;
  }
}

void
__stdcall
SK_D3D11_RemoveTexFromCache (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_TextureIsCached (pTex))
    return;

  if (pTex != nullptr) {
    SK_AutoCriticalSection critical (&cache_cs);

    uint32_t crc32 = SK_D3D11_Textures.Textures_2D [pTex].crc32;

    SK_D3D11_Textures.AggregateSize_2D -=
      SK_D3D11_Textures.Textures_2D [pTex].mem_size;

    SK_D3D11_Textures.Evicted_2D++;

    D3D11_TEXTURE2D_DESC desc;
    pTex->GetDesc (&desc);

    SK_D3D11_Textures.Textures_2D.erase                 (pTex);
    SK_D3D11_Textures.HashMap_2D [desc.MipLevels].erase (crc32);
    SK_D3D11_Textures.TexRefs_2D.erase                  (pTex);
  }
}

void
SK_D3D11_TexMgr::refTexture2D ( ID3D11Texture2D*      pTex,
                          const D3D11_TEXTURE2D_DESC *pDesc,
                                uint32_t              crc32,
                                size_t                mem_size,
                                uint64_t              load_time )
{
  if (! SK_D3D11_cache_textures)
    return;

  if (pTex == nullptr || crc32 == 0x00)
    return;

  //if (! injectable_textures.count (crc32))
    //return;

  if (SK_D3D11_TextureIsCached (pTex)) {
    dll_log.Log (L"[DX11TexMgr] Texture is already cached?!");
  }

  if (pDesc->Usage != D3D11_USAGE_DEFAULT &&
      pDesc->Usage != D3D11_USAGE_IMMUTABLE) {
//    dll_log.Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
//                  L"Due To Usage: %lu",
//                  crc32, pDesc->Usage );
    return;
  }

  if (pDesc->CPUAccessFlags != 0x00) {
//    dll_log.Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
//                  L"Due To CPUAccessFlags: 0x%X",
//                  crc32, pDesc->CPUAccessFlags );
    return;
  }

  AggregateSize_2D += mem_size;

  tex2D_descriptor_s desc2d;

  desc2d.desc      = *pDesc;
  desc2d.texture   =  pTex;
  desc2d.load_time =  load_time;
  desc2d.mem_size  =  mem_size;
  desc2d.crc32     =  crc32;


  SK_AutoCriticalSection critical (&cache_cs);

  TexRefs_2D.insert                    (                       pTex);
  HashMap_2D [pDesc->MipLevels].insert (std::make_pair (crc32, pTex));
  Textures_2D.insert                   (std::make_pair (       pTex, desc2d));

  // Hold a reference ourselves so that the game cannot free it
  pTex->AddRef ();
#ifdef DS3_REF_TWEAK
  pTex->AddRef ();
  pTex->AddRef ();
#endif
}

#include <Shlwapi.h>

void
WINAPI
SK_D3D11_PopulateResourceList (void)
{
  static bool init = false;

  if (init || SK_D3D11_res_root.empty ())
    return;

  SK_AutoCriticalSection critical (&tex_cs);

  init = true;

  wchar_t wszTexDumpDir_RAW [ MAX_PATH     ] = { L'\0' };
  wchar_t wszTexDumpDir     [ MAX_PATH + 2 ] = { L'\0' };

  lstrcatW (wszTexDumpDir_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexDumpDir_RAW, L"\\dump\\textures\\");
  lstrcatW (wszTexDumpDir_RAW, SK_GetHostApp ());

  wcscpy ( wszTexDumpDir,
             SK_EvalEnvironmentVars (wszTexDumpDir_RAW).c_str () );

  //
  // Walk custom textures so we don't have to query the filesystem on every
  //   texture load to check if a custom one exists.
  //
  if ( GetFileAttributesW (wszTexDumpDir) !=
         INVALID_FILE_ATTRIBUTES ) {
    WIN32_FIND_DATA fd;
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    unsigned int    files  = 0UL;
    LARGE_INTEGER   liSize = { 0 };

    LARGE_INTEGER   liCompressed   = { 0 };
    LARGE_INTEGER   liUncompressed = { 0 };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating dumped...    " );

    lstrcatW (wszTexDumpDir, L"\\*");

    hFind = FindFirstFileW (wszTexDumpDir, &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES) {
          // Dumped Metadata has the extension .dds.txt, do not
          //   include these while scanning for textures.
          if (    StrStrIW (fd.cFileName, L".dds")    &&
               (! StrStrIW (fd.cFileName, L".dds.txt") ) ) {
            uint32_t top_crc32 = 0x00;
            uint32_t checksum  = 0x00;

            bool compressed = false;

            if (StrStrIW (fd.cFileName, L"Uncompressed_")) {
              if (StrStrIW (StrStrIW (fd.cFileName, L"_") + 1, L"_")) {
                swscanf ( fd.cFileName,
                            L"Uncompressed_%08X_%08X.dds",
                              &top_crc32,
                                &checksum );
              } else {
                swscanf ( fd.cFileName,
                            L"Uncompressed_%08X.dds",
                              &top_crc32 );
                checksum = 0x00;
              }
            } else {
              if (StrStrIW (StrStrIW (fd.cFileName, L"_") + 1, L"_")) {
                swscanf ( fd.cFileName,
                            L"Compressed_%08X_%08X.dds",
                              &top_crc32,
                                &checksum );
              } else {
                swscanf ( fd.cFileName,
                            L"Compressed_%08X.dds",
                              &top_crc32 );
                checksum = 0x00;
              }
              compressed = true;
            }

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            if (compressed)
              liCompressed.QuadPart += fsize.QuadPart;
            else
              liUncompressed.QuadPart += fsize.QuadPart;

            if (dumped_textures.count (top_crc32) >= 1)
              dll_log.Log ( L"[DX11TexDmp] >> WARNING: Multiple textures have "
                            L"the same top-level LOD hash (%08X) <<",
                              top_crc32 );

            if (checksum == 0x00)
              dumped_textures.insert (top_crc32);
            else
              dumped_collisions.insert (crc32c (top_crc32, (uint8_t *)&checksum, 4));
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    dll_log.LogEx ( false, L" %lu files (%3.1f MiB -- %3.1f:%3.1f MiB Un:Compressed)\n",
                      files, (double)liSize.QuadPart / (1024.0 * 1024.0),
                               (double)liUncompressed.QuadPart / (1024.0 * 1024.0),
                                 (double)liCompressed.QuadPart /  (1024.0 * 1024.0) );
  }

  wchar_t wszTexInjectDir_RAW [ MAX_PATH + 2 ] = { L'\0' };
  wchar_t wszTexInjectDir     [ MAX_PATH + 2 ] = { L'\0' };

  lstrcatW (wszTexInjectDir_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir_RAW, L"\\inject\\textures");

  wcscpy ( wszTexInjectDir,
             SK_EvalEnvironmentVars (wszTexInjectDir_RAW).c_str () );

  if ( GetFileAttributesW (wszTexInjectDir) !=
         INVALID_FILE_ATTRIBUTES ) {
    WIN32_FIND_DATA fd     = { 0 };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    unsigned int    files  =   0;
    LARGE_INTEGER   liSize = { 0 };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating injectable..." );

    lstrcatW (wszTexInjectDir, L"\\*");

    hFind = FindFirstFileW (wszTexInjectDir, &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES) {
          if (StrStrIW (fd.cFileName, L".dds")) {
            uint32_t top_crc32;
            uint32_t checksum  = 0x00;

            if (StrStrIW (fd.cFileName, L"_")) {
              swscanf (fd.cFileName, L"%08X_%08X.dds", &top_crc32, &checksum);
            } else {
              swscanf (fd.cFileName, L"%08X.dds",    &top_crc32);
            }

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            injectable_textures.insert (top_crc32);

            if (checksum != 0x00)
              injected_collisions.insert (crc32c (top_crc32, (const uint8_t *)&checksum, 4));
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    dll_log.LogEx ( false, L" %lu files (%3.1f MiB)\n",
                      files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }

  wchar_t wszTexInjectDir_FFX_RAW [ MAX_PATH     ] = { L'\0' };
  wchar_t wszTexInjectDir_FFX     [ MAX_PATH + 2 ] = { L'\0' };

  lstrcatW (wszTexInjectDir_FFX_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir_FFX_RAW, L"\\inject\\textures\\UnX_Old");

  wcscpy ( wszTexInjectDir_FFX,
             SK_EvalEnvironmentVars (wszTexInjectDir_FFX_RAW).c_str () );

  if ( GetFileAttributesW (wszTexInjectDir_FFX) !=
         INVALID_FILE_ATTRIBUTES ) {
    WIN32_FIND_DATA fd     = { 0 };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    int             files  =   0;
    LARGE_INTEGER   liSize = { 0 };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating FFX inject..." );

    lstrcatW (wszTexInjectDir_FFX, L"\\*");

    hFind = FindFirstFileW (wszTexInjectDir_FFX, &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES) {
          if (StrStrIW (fd.cFileName, L".dds")) {
            uint32_t ffx_crc32;

            swscanf (fd.cFileName, L"%08X.dds", &ffx_crc32);

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            injectable_ffx.insert (ffx_crc32);
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    dll_log.LogEx ( false, L" %lu files (%3.1f MiB)\n",
                      files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }
}

void
WINAPI
SK_D3D11_SetResourceRoot (const wchar_t* root)
{
  // Non-absolute path (e.g. NOT C:\...\...")
  if (! wcsstr (root, L":"))
  {
    SK_D3D11_res_root = SK_GetRootPath ();
    SK_D3D11_res_root += L"\\";
    SK_D3D11_res_root += root;
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

  return;
}


bool
WINAPI
SK_D3D11_IsTexHashed (uint32_t top_crc32, uint32_t hash)
{
  SK_D3D11_InitTextures ();

  SK_AutoCriticalSection critical (&hash_cs);

  if (tex_hashes_ex.count (crc32c (top_crc32, (const uint8_t *)&hash, 4)) != 0)
    return true;

  return tex_hashes.count (top_crc32) != 0;
}

void
WINAPI
SK_D3D11_AddInjectable (uint32_t top_crc32, uint32_t checksum);

void
WINAPI
SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash )
{
  // Allow UnX to call this before a device has been created.
  SK_D3D11_InitTextures ();

  if (hash != 0x00) {
    if (! SK_D3D11_IsTexHashed (top_crc32, hash)) {
      SK_AutoCriticalSection critical (&hash_cs);

      tex_hashes_ex.insert (std::make_pair (crc32c (top_crc32, (const uint8_t *)&hash, 4), name));
      SK_D3D11_AddInjectable (top_crc32, hash);
    }
  }

  if (! SK_D3D11_IsTexHashed (top_crc32, 0x00)) {
    SK_AutoCriticalSection critical (&hash_cs);

    tex_hashes.insert (std::make_pair (top_crc32, name));

    if (! SK_D3D11_inject_textures_ffx)
      SK_D3D11_AddInjectable (top_crc32, 0x00);
    else
      injectable_ffx.insert (top_crc32);
  }
}

void
WINAPI
SK_D3D11_RemoveTexHash (uint32_t top_crc32, uint32_t hash)
{
  // Allow UnX to call this before a device has been created.
  SK_D3D11_InitTextures ();

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash)) {
    SK_AutoCriticalSection critical (&hash_cs);

    tex_hashes_ex.erase (crc32c (top_crc32, (const uint8_t *)&hash, 4));
  }

  else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00)) {
    tex_hashes.erase (top_crc32);
 }
}

std::wstring
__stdcall
SK_D3D11_TexHashToName (uint32_t top_crc32, uint32_t hash)
{
  // Allow UnX to call this before a device has been created.
  SK_D3D11_InitTextures ();

  std::wstring ret = L"";

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash)) {
    SK_AutoCriticalSection critical (&hash_cs);

    ret = tex_hashes_ex [crc32c (top_crc32, (const uint8_t *)&hash, 4)];
  } else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00)) {
    SK_AutoCriticalSection critical (&hash_cs);

    ret = tex_hashes [top_crc32];
  }

  return ret;
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

__declspec (noinline)
uint32_t
__cdecl
crc32_tex (  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *pInitialData,
             _Out_opt_       size_t                 *pSize,
             _Out_opt_       uint32_t               *pLOD0_CRC32 )
{
  // Ignore Cubemaps for Now
  if (pDesc->MiscFlags == 0x04)
  {
//    dll_log.Log (L"[ Tex Hash ] >> Will not hash cubemap");
    if (pLOD0_CRC32)
      *pLOD0_CRC32 = 0x0000;

    if (pSize)
      *pSize       = 0;

    return 0;
  }

  if (pDesc->MiscFlags != 0x00)
  {
    dll_log.Log ( L"[ Tex Hash ] >> Hashing texture with unexpected MiscFlags: "
                   L"0x%04X",
                     pDesc->MiscFlags );
  }

  uint32_t checksum = 0;

  bool compressed = false;

  if ( (pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS  &&
        pDesc->Format <= DXGI_FORMAT_BC5_SNORM)    ||
       (pDesc->Format >= DXGI_FORMAT_BC6H_TYPELESS &&
        pDesc->Format <= DXGI_FORMAT_BC7_UNORM_SRGB) )
    compressed = true;

  const int bpp = ( (pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS &&
                     pDesc->Format <= DXGI_FORMAT_BC1_UNORM_SRGB) ||
                    (pDesc->Format >= DXGI_FORMAT_BC4_TYPELESS &&
                     pDesc->Format <= DXGI_FORMAT_BC4_SNORM) ) ? 0 : 1;

  unsigned int width  = pDesc->Width;
  unsigned int height = pDesc->Height;

        size_t size   = 0UL;

  if (compressed)
  {
    for (unsigned int i = 0; i < pDesc->MipLevels; i++)
    {
      char* pData    = (char *)pInitialData [i].pSysMem;
      UINT stride = bpp == 0 ?
             std::max (1UL, ((width + 3UL) / 4UL) ) * 8UL :
             std::max (1UL, ((width + 3UL) / 4UL) ) * 16UL;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (stride == pInitialData [i].SysMemPitch)
      {
        unsigned int lod_size = stride * (height / 4 +
                                          height % 4);

        checksum = crc32c (checksum, (const uint8_t *)pData, lod_size);
        size    += lod_size;
      }

      else
      {
        // We are running through the compressed image block-by-block,
        //  the lines we are "scanning" actually represent 4 rows of image data.
        for (unsigned int j = 0; j < height; j += 4)
        {
          checksum =
            crc32c (checksum, (const uint8_t *)pData, stride);

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
      char* pData      = (char *)pInitialData [i].pSysMem;
      UINT  scanlength = SK_D3D11_BytesPerPixel (pDesc->Format) * width;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (scanlength == pInitialData [i].SysMemPitch) 
      {
        unsigned int lod_size = (scanlength * height);

        checksum = crc32c (checksum, (const uint8_t *)pData, lod_size);
        size    += lod_size;
      }

      else
      {
        for (unsigned int j = 0; j < height; j++)
        {
          checksum =
            crc32c (checksum, (const uint8_t *)pData, scanlength);

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
    *pSize = size;

  return checksum;
}

//
// OLD, BUGGY Algorithm... must remain here for compatibility with UnX :(
//
__declspec (noinline)
uint32_t
__cdecl
crc32_ffx (  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *pInitialData,
             _Out_opt_       size_t                 *pSize )
{
  uint32_t checksum = 0;

  bool compressed = false;

  if (pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS && pDesc->Format <= DXGI_FORMAT_BC5_SNORM)
    compressed = true;

  if (pDesc->Format >= DXGI_FORMAT_BC6H_TYPELESS && pDesc->Format <= DXGI_FORMAT_BC7_UNORM_SRGB)
    compressed = true;

  int block_size = pDesc->Format == DXGI_FORMAT_BC1_UNORM ? 8 : 16;

//int width      = pDesc->Width;
  int height     = pDesc->Height;

  size_t size = 0;

  for (unsigned int i = 0; i < pDesc->MipLevels; i++) {
    if (compressed) {
      size += (pInitialData [i].SysMemPitch / block_size) * (height >> i);

      checksum =
        crc32 (checksum, (const char *)pInitialData [i].pSysMem, (pInitialData [i].SysMemPitch / block_size) * (height >> i));
    } else {
      size += (pInitialData [i].SysMemPitch) * (height >> i);

      checksum =
        crc32 (checksum, (const char *)pInitialData [i].pSysMem, (pInitialData [i].SysMemPitch) * (height >> i));
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
  SK_AutoCriticalSection critical (&dump_cs);

  if (config.textures.d3d11.precise_hash && dumped_collisions.count (crc32c (top_crc32, (uint8_t *)&checksum, 4)))
    return true;
  if (! config.textures.d3d11.precise_hash)
    return dumped_textures.count (top_crc32) != 0;

  return false;
}

void
__stdcall
SK_D3D11_AddDumped (uint32_t top_crc32, uint32_t checksum)
{
  SK_AutoCriticalSection critical (&dump_cs);

  if (! config.textures.d3d11.precise_hash)
    dumped_textures.insert (top_crc32);

  dumped_collisions.insert (crc32c (top_crc32, (uint8_t *)&checksum, 4));
}

bool
__stdcall
SK_D3D11_IsInjectable (uint32_t top_crc32, uint32_t checksum)
{
  SK_AutoCriticalSection critical (&inject_cs);

  if (checksum != 0x00) {
    if (injected_collisions.count (crc32c (top_crc32, (uint8_t *)&checksum, 4)))
      return true;
    return false;
  }

  return injectable_textures.count (top_crc32) != 0;
}

bool
__stdcall
SK_D3D11_IsInjectable_FFX (uint32_t top_crc32)
{
  SK_AutoCriticalSection critical (&inject_cs);

  return injectable_ffx.count (top_crc32) != 0;
}


void
__stdcall
SK_D3D11_AddInjectable (uint32_t top_crc32, uint32_t checksum)
{
  SK_AutoCriticalSection critical (&inject_cs);

  if (checksum != 0x00)
    injected_collisions.insert (crc32c (top_crc32, (uint8_t *)&checksum, 4));

  injectable_textures.insert (top_crc32);
}

#include <DirectXTex/DirectXTex.h>

HRESULT
__stdcall
SK_D3D11_DumpTexture2D (  _In_ const D3D11_TEXTURE2D_DESC   *pDesc,
                          _In_ const D3D11_SUBRESOURCE_DATA *pInitialData,
                          _In_       uint32_t                top_crc32,
                          _In_       uint32_t                checksum )
{
  dll_log.Log ( L"[DX11TexDmp] Dumping Texture: %08x::%08x... (fmt=%03lu, "
                    L"BindFlags=0x%04x, Usage=0x%04x, CPUAccessFlags"
                    L"=0x%02x, Misc=0x%02x, MipLODs=%02lu, ArraySize=%02lu)",
                  top_crc32,
                    checksum,
                      pDesc->Format,
                        pDesc->BindFlags,
                          pDesc->Usage,
                            pDesc->CPUAccessFlags,
                              pDesc->MiscFlags,
                                pDesc->MipLevels,
                                  pDesc->ArraySize );

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
  mdata.format     = pDesc->Format;
  mdata.dimension  = DirectX::TEX_DIMENSION_TEXTURE2D;

  DirectX::ScratchImage image;
  image.Initialize (mdata);

  bool error = false;

  for (size_t slice = 0; slice < mdata.arraySize; ++slice) {
    size_t height = mdata.height;

    for (size_t lod = 0; lod < mdata.mipLevels; ++lod) {
      const DirectX::Image* img =
        image.GetImage (lod, slice, 0);

      if (! (img && img->pixels)) {
        error = true;
        break;
      }

      const size_t lines =
        DirectX::ComputeScanlines (mdata.format, height);

      if  (! lines) {
        error = true;
        break;
      }

      auto sptr =
        static_cast <const uint8_t *>(
          pInitialData [lod].pSysMem
        );

      uint8_t* dptr =
        img->pixels;

      for (size_t h = 0; h < lines; ++h) {
        size_t msize =
          std::min <size_t> (img->rowPitch, pInitialData [lod].SysMemPitch);

        memcpy_s (dptr, img->rowPitch, sptr, msize);

        sptr += pInitialData [lod].SysMemPitch;
        dptr += img->rowPitch;
      }

      if (height > 1) height >>= 1;
    }

    if (error)
      break;
  }

  wchar_t wszPath [ MAX_PATH + 2 ] = { L'\0' };

  wcscpy ( wszPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszPath, L"/dump/textures/");
  lstrcatW (wszPath, SK_GetHostApp ());
  lstrcatW (wszPath, L"/");

  SK_CreateDirectories (wszPath);

  bool compressed = false;

  if ( ( pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS &&
         pDesc->Format <= DXGI_FORMAT_BC5_SNORM )  ||
       ( pDesc->Format >= DXGI_FORMAT_BC6H_TYPELESS &&
         pDesc->Format <= DXGI_FORMAT_BC7_UNORM_SRGB ) )
    compressed = true;

  wchar_t wszOutPath [MAX_PATH + 2] = { L'\0' };
  wchar_t wszOutName [MAX_PATH + 2] = { L'\0' };

  wcscpy ( wszOutPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszOutPath, L"\\dump\\textures\\");
  lstrcatW (wszOutPath, SK_GetHostApp ());

  if (compressed && config.textures.d3d11.precise_hash) {
    _swprintf ( wszOutName, L"%s\\Compressed_%08X_%08X.dds",
                  wszOutPath, top_crc32, checksum );
  } else if (compressed) {
    _swprintf ( wszOutName, L"%s\\Compressed_%08X.dds",
                  wszOutPath, top_crc32 );
  } else if (config.textures.d3d11.precise_hash) {
    _swprintf ( wszOutName, L"%s\\Uncompressed_%08X_%08X.dds",
                  wszOutPath, top_crc32, checksum );
  } else {
    _swprintf ( wszOutName, L"%s\\Uncompressed_%08X.dds",
                  wszOutPath, top_crc32 );
  }

  if ((! error) && wcslen (wszOutName)) {
    if (GetFileAttributes (wszOutName) == INVALID_FILE_ATTRIBUTES) {
      dll_log.Log ( L"[DX11TexDmp]  >> File: '%s' (top: %x, full: %x)",
                      wszOutName,
                        top_crc32,
                          checksum );

      wchar_t wszMetaFilename [ MAX_PATH + 2 ] = { L'\0' };

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

      return SaveToDDSFile ( image.GetImages (), image.GetImageCount (),
                               image.GetMetadata (), DirectX::DDS_FLAGS_NONE,
                                 wszOutName );
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
  return D3D11Dev_CreateBuffer_Original (This, pDesc, pInitialData, ppBuffer);
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateShaderResourceView_Override (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView )
{
  return D3D11Dev_CreateShaderResourceView_Original (This, pResource, pDesc, ppSRView);
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
  // Exclude stuff that hooks D3D11 device creation and wants to recurse (i.e. NVIDIA Ansel)
  if (InterlockedExchangeAdd (&SK_D3D11_init_tid, 0) != GetCurrentThreadId ())
    WaitForInitDXGI ();

  if (InterlockedExchangeAdd (&SK_D3D11_tex_init, 0) == FALSE)
    SK_D3D11_InitTextures ();

  bool early_out = false;

  if ((! (SK_D3D11_cache_textures || SK_D3D11_dump_textures || SK_D3D11_inject_textures)) ||
         SK_D3D11_IsTexInjectThread ())
    early_out = true;

  if (early_out)
    return D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);

  LARGE_INTEGER load_start = SK_QueryPerf ();

  uint32_t      checksum  = 0;
  uint32_t      cache_tag = 0;
  size_t        size      = 0;

  ID3D11Texture2D* pCachedTex = nullptr;

  bool cacheable = (pInitialData          != nullptr &&
                    pInitialData->pSysMem != nullptr &&
                    pDesc->Width > 0 && pDesc->Height > 0);

  cacheable &=
    (! ((pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) ||
        (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET)) ) &&
        (pDesc->CPUAccessFlags == 0x0);

  cacheable &= (ppTexture2D != nullptr);

  const bool dumpable = 
              cacheable && pDesc->Usage != D3D11_USAGE_DYNAMIC &&
                           pDesc->Usage != D3D11_USAGE_STAGING;

  uint32_t top_crc32 = 0x00;
  uint32_t ffx_crc32 = 0x00;

  if (cacheable) {
    checksum = crc32_tex (pDesc, pInitialData, &size, &top_crc32);

    if (SK_D3D11_inject_textures_ffx) {
      ffx_crc32 = crc32_ffx (pDesc, pInitialData, &size);
    }

    const bool injectable = (
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
        cacheable &= pDesc->MipLevels > 1;

      cache_tag  = crc32c (top_crc32, (uint8_t *)pDesc, sizeof D3D11_TEXTURE2D_DESC);
      pCachedTex = SK_D3D11_Textures.getTexture2D (cache_tag, pDesc);
    } else {
      cacheable = false;
    }
  }

  if (pCachedTex != nullptr) {
    //dll_log.Log ( L"[DX11TexMgr] >> Redundant 2D Texture Load "
                  //L" (Hash=0x%08X [%05.03f MiB]) <<",
                  //checksum, (float)size / (1024.0f * 1024.0f) );
    pCachedTex->AddRef ();
    *ppTexture2D = pCachedTex;
    return S_OK;
  }

  SK_D3D11_Textures.CacheMisses_2D++;

  if (cacheable) {
    if (D3DX11CreateTextureFromFileW != nullptr && SK_D3D11_res_root.length ()) {
      wchar_t wszTex [MAX_PATH + 2] = { L'\0' };

      
      wcscpy ( wszTex,
                SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

      if (SK_D3D11_IsTexHashed (ffx_crc32, 0x00)) {
        SK_LOG4 ( ( L"Caching texture with crc32: %x", ffx_crc32 ),
                    L" Tex Hash " );
        _swprintf ( wszTex, L"%s\\%s",
                      wszTex,
                        SK_D3D11_TexHashToName (ffx_crc32, 0x00).c_str ()
        );
      }

      else if (SK_D3D11_IsTexHashed (top_crc32, checksum)) {
        SK_LOG4 ( ( L"Caching texture with crc32c: %x", top_crc32 ),
                    L"Tex Hash " );
        _swprintf ( wszTex, L"%s\\%s",
                      wszTex,
                        SK_D3D11_TexHashToName (top_crc32,checksum).c_str ()
                  );
      }

      else if (SK_D3D11_IsTexHashed (top_crc32, 0x00)) {
        SK_LOG4 ( ( L"Caching texture with crc32c: %x", top_crc32 ),
                    L" Tex Hash " );
        _swprintf ( wszTex, L"%s\\%s",
                      wszTex,
                        SK_D3D11_TexHashToName (top_crc32, 0x00).c_str ()
                  );
      }

      else if ( /*config.textures.d3d11.precise_hash &&*/
                SK_D3D11_inject_textures           &&
                SK_D3D11_IsInjectable (top_crc32, checksum) ) {
        _swprintf ( wszTex,
                      L"%s\\inject\\textures\\%08X_%08X.dds",
                        wszTex,
                          top_crc32, checksum );
      }

      else if ( SK_D3D11_inject_textures &&
                SK_D3D11_IsInjectable (top_crc32, 0x00) ) {
        SK_LOG4 ( ( L"Caching texture with crc32c: %08X", top_crc32 ),
                    L" Tex Hash " );
        _swprintf ( wszTex,
                      L"%s\\inject\\textures\\%08X.dds",
                        wszTex,
                          top_crc32 );
      }

      else if ( SK_D3D11_inject_textures           &&
                SK_D3D11_IsInjectable_FFX (ffx_crc32) ) {
        SK_LOG4 ( ( L"Caching texture with crc32: %08X", ffx_crc32 ),
                    L" Tex Hash " );
        _swprintf ( wszTex,
                      L"%s\\inject\\textures\\Unx_Old\\%08X.dds",
                        wszTex,
                          ffx_crc32 );
      }

      // Not a hashed texture, not an injectable texture, skip it...
      else *wszTex = L'\0';

      if (                   *wszTex  != L'\0' &&
           GetFileAttributes (wszTex) != INVALID_FILE_ATTRIBUTES )
      {
        SK_AutoCriticalSection inject_critical (&inject_cs);

      //ID3D11Resource* pRes = nullptr;

#define D3DX11_DEFAULT -1

        D3DX11_IMAGE_INFO      img_info   = { 0 };
        D3DX11_IMAGE_LOAD_INFO load_info  = { 0 };

        D3DX11GetImageInfoFromFileW (wszTex, nullptr, &img_info, nullptr);

        load_info.BindFlags      = pDesc->BindFlags;
        load_info.CpuAccessFlags = pDesc->CPUAccessFlags;
        load_info.Depth          = img_info.Depth;//D3DX11_DEFAULT;
        load_info.Filter         = (UINT)D3DX11_DEFAULT;
        load_info.FirstMipLevel  = 0;
        load_info.Format         = pDesc->Format;
        load_info.Height         = img_info.Height;//D3DX11_DEFAULT;
        load_info.MipFilter      = (UINT)D3DX11_DEFAULT;
        load_info.MipLevels      = img_info.MipLevels;//D3DX11_DEFAULT;
        load_info.MiscFlags      = img_info.MiscFlags;//pDesc->MiscFlags;
        load_info.pSrcInfo       = &img_info;
        load_info.Usage          = pDesc->Usage;
        load_info.Width          = img_info.Width;//D3DX11_DEFAULT;

        SK_D3D11_SetTexInjectThread ();

        if ( SUCCEEDED ( D3DX11CreateTextureFromFileW (
                           This, wszTex,
                             &load_info, nullptr,
             (ID3D11Resource**)ppTexture2D, nullptr )
                       )
           )
        {
          LARGE_INTEGER load_end = SK_QueryPerf ();

          SK_D3D11_ClearTexInjectThread ();

          SK_D3D11_Textures.refTexture2D (
            *ppTexture2D,
              pDesc,
                cache_tag,
                  size,
                    load_end.QuadPart - load_start.QuadPart
          );

          return S_OK;
        }

        SK_D3D11_ClearTexInjectThread ();
      }
    }
  }

  HRESULT ret =
    D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);

  if (ppTexture2D != nullptr) {
    static volatile ULONG init = FALSE;

    if (! InterlockedCompareExchange (&init, TRUE, FALSE)) {
      DXGI_VIRTUAL_HOOK ( ppTexture2D, 2, "IUnknown::Release",
                          IUnknown_Release,
                          IUnknown_Release_Original,
                          IUnknown_Release_pfn );
      DXGI_VIRTUAL_HOOK ( ppTexture2D, 1, "IUnknown::AddRef",
                          IUnknown_AddRef,
                          IUnknown_AddRef_Original,
                          IUnknown_AddRef_pfn );

      MH_ApplyQueued ();
    }
  }

  LARGE_INTEGER load_end = SK_QueryPerf ();

  if ( SUCCEEDED (ret) &&
          dumpable     &&
      checksum != 0x00 &&
      SK_D3D11_dump_textures )
  {
    if (! SK_D3D11_IsDumped (top_crc32, checksum)) {
      SK_D3D11_DumpTexture2D (pDesc, pInitialData, top_crc32, checksum);
    }
  }

  cacheable &=
    (SK_D3D11_cache_textures || SK_D3D11_IsInjectable (top_crc32, checksum));

  if ( SUCCEEDED (ret) && cacheable ) {
    SK_D3D11_Textures.refTexture2D (
      *ppTexture2D,
        pDesc,
          cache_tag,
            size,
              load_end.QuadPart - load_start.QuadPart
    );
  }

  return ret;
}


void
__stdcall
SK_D3D11_UpdateRenderStats (IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain && config.render.show))
    return;

  CComPtr <ID3D11Device> dev = nullptr;

  if (SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&dev)))) {
    CComPtr <ID3D11DeviceContext> dev_ctx = nullptr;

    dev->GetImmediateContext (&dev_ctx);

    if (dev_ctx == nullptr)
      return;

    SK::DXGI::PipelineStatsD3D11& pipeline_stats =
      SK::DXGI::pipeline_stats_d3d11;

    if (pipeline_stats.query.async != nullptr) {
      if (pipeline_stats.query.active) {
        dev_ctx->End (pipeline_stats.query.async);
        pipeline_stats.query.active = false;
      } else {
        HRESULT hr =
          dev_ctx->GetData ( pipeline_stats.query.async,
                              &pipeline_stats.last_results,
                                sizeof D3D11_QUERY_DATA_PIPELINE_STATISTICS,
                                  0x0 );
        if (hr == S_OK) {
          pipeline_stats.query.async->Release ();
          pipeline_stats.query.async = nullptr;
        }
      }
    }

    else {
      D3D11_QUERY_DESC query_desc {
        D3D11_QUERY_PIPELINE_STATISTICS, 0x00
      };

      if (SUCCEEDED (dev->CreateQuery (&query_desc, &pipeline_stats.query.async))) {
        dev_ctx->Begin (pipeline_stats.query.async);
        pipeline_stats.query.active = true;
      }
    }
  }
}

std::wstring
SK_CountToString (uint64_t count)
{
  wchar_t str [64];

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
  memcpy ( (void *)&SK::DXGI::pipeline_stats_d3d11.last_results,
             pData,
               sizeof D3D11_QUERY_DATA_PIPELINE_STATISTICS );
}

std::wstring
SK::DXGI::getPipelineStatsDesc (void)
{
  wchar_t wszDesc [1024];

  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
    pipeline_stats_d3d11.last_results;

  //
  // VERTEX SHADING
  //
  if (stats.VSInvocations > 0) {
    _swprintf ( wszDesc,
                  L"  VERTEX : %s   (%s Verts ==> %s Triangles)\n",
                    SK_CountToString (stats.VSInvocations).c_str (),
                      SK_CountToString (stats.IAVertices).c_str (),
                        SK_CountToString (stats.IAPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"  VERTEX : <Unused>\n" );
  }

  //
  // GEOMETRY SHADING
  //
  if (stats.GSInvocations > 0)
  {
    _swprintf ( wszDesc,
                  L"%s  GEOM   : %s   (%s Prims)\n",
                    wszDesc,
                      SK_CountToString (stats.GSInvocations).c_str (),
                        SK_CountToString (stats.GSPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  GEOM   : <Unused>\n",
                    wszDesc );
  }

  //
  // TESSELLATION
  //
  if (stats.HSInvocations > 0 || stats.DSInvocations > 0)
  {
    _swprintf ( wszDesc,
                  L"%s  TESS   : %s Hull ==> %s Domain\n",
                    wszDesc,
                      SK_CountToString (stats.HSInvocations).c_str (),
                        SK_CountToString (stats.DSInvocations).c_str () ) ;
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  TESS   : <Unused>\n",
                    wszDesc );
  }

  //
  // RASTERIZATION
  //
  if (stats.CInvocations > 0)
  {
    _swprintf ( wszDesc,
                  L"%s  RASTER : %5.1f%% Filled     (%s Triangles IN )\n",
                    wszDesc, 100.0f *
                        ( (float)stats.CPrimitives /
                          (float)stats.CInvocations ),
                      SK_CountToString (stats.CInvocations).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  RASTER : <Unused>\n",
                    wszDesc );
  }

  //
  // PIXEL SHADING
  //
  if (stats.PSInvocations > 0)
  {
    _swprintf ( wszDesc,
                  L"%s  PIXEL  : %s   (%s Triangles OUT)\n",
                    wszDesc,
                      SK_CountToString (stats.PSInvocations).c_str (),
                        SK_CountToString (stats.CPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  PIXEL  : <Unused>\n",
                    wszDesc );
  }

  //
  // COMPUTE
  //
  if (stats.CSInvocations > 0) {
    _swprintf ( wszDesc,
                  L"%s  COMPUTE: %s\n",
                    wszDesc, SK_CountToString (stats.CSInvocations).c_str () );
  } else {
    _swprintf ( wszDesc,
                  L"%s  COMPUTE: <Unused>\n",
                    wszDesc );
  }

  return wszDesc;
}


void
SK_D3D11_InitTextures (void)
{
  if (! InterlockedCompareExchange (&SK_D3D11_tex_init, TRUE, FALSE))
  {
    if ( StrStrIW (SK_GetHostApp (), L"ffx.exe")   ||
         StrStrIW (SK_GetHostApp (), L"ffx-2.exe") ||
         StrStrIW (SK_GetHostApp (), L"FFX&X-2_Will.exe") )
      SK_D3D11_inject_textures_ffx = true;

#ifdef NO_TLS
    InitializeCriticalSectionAndSpinCount (&cs_texinject, 0x4000);
#endif

    InitializeCriticalSectionAndSpinCount (&tex_cs,    MAXDWORD);
    InitializeCriticalSectionAndSpinCount (&hash_cs,   0x4000);
    InitializeCriticalSectionAndSpinCount (&dump_cs,   0x0200);
    InitializeCriticalSectionAndSpinCount (&cache_cs,  MAXDWORD);
    InitializeCriticalSectionAndSpinCount (&inject_cs, 0x1000);

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
    if (! SK_D3D11_inject_textures_ffx)
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
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MinEntries",
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

    if (! SK_D3D11_inject_textures_ffx)
      SK_D3D11_PopulateResourceList ();

    if (hModD3DX11_43 == nullptr)
    {
      hModD3DX11_43 =
        LoadLibraryW_Original (L"d3dx11_43.dll");

      if (hModD3DX11_43 == nullptr)
        hModD3DX11_43 = (HMODULE)1;
    }

    if (D3DX11CreateTextureFromFileW == nullptr && (uintptr_t)hModD3DX11_43 > 1)
    {
      D3DX11CreateTextureFromFileW =
        (D3DX11CreateTextureFromFileW_pfn)
          GetProcAddress (hModD3DX11_43, "D3DX11CreateTextureFromFileW");
    }

    if (D3DX11GetImageInfoFromFileW == nullptr && (uintptr_t)hModD3DX11_43 > 1)
    {
      D3DX11GetImageInfoFromFileW =
        (D3DX11GetImageInfoFromFileW_pfn)
          GetProcAddress (hModD3DX11_43, "D3DX11GetImageInfoFromFileW");
    }
  }
}

volatile LONG SK_D3D11_initialized = FALSE;

bool
SK_D3D11_Init (void)
{
  BOOL success = FALSE;

  if (! InterlockedCompareExchange (&SK_D3D11_initialized, TRUE, FALSE))
  {
    SK::DXGI::hModD3D11 =
      LoadLibraryW_Original (L"d3d11.dll");

    if ( MH_OK ==
           SK_CreateDLLHook2 ( L"d3d11.dll",
                                "D3D11CreateDevice",
                                 D3D11CreateDevice_Detour,
                      (LPVOID *)&D3D11CreateDevice_Import,
                             &pfnD3D11CreateDevice )
       )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2 ( L"d3d11.dll",
                                  "D3D11CreateDeviceAndSwapChain",
                                   D3D11CreateDeviceAndSwapChain_Detour,
                        (LPVOID *)&D3D11CreateDeviceAndSwapChain_Import,
                               &pfnD3D11CreateDeviceAndSwapChain )
         )
      {
        if ( MH_OK == MH_QueueEnableHook (pfnD3D11CreateDevice) &&
             MH_OK == MH_QueueEnableHook (pfnD3D11CreateDeviceAndSwapChain) )
        {
          success = (MH_OK == SK_ApplyQueuedHooks ());
        }
      }
    }
  }

  return success;
}

void
SK_D3D11_Shutdown (void)
{
  if (! InterlockedCompareExchange (&SK_D3D11_initialized, FALSE, TRUE))
    return;

  if (SK_D3D11_Textures.RedundantLoads_2D > 0)
  {
    dll_log.Log ( L"[Perf Stats] At shutdown: %7.2f seconds and %7.2f MiB of"
                  L" CPU->GPU I/O avoided by %lu texture cache hits.",
                    SK_D3D11_Textures.RedundantTime_2D / 1000.0f,
                      (float)SK_D3D11_Textures.RedundantData_2D /
                                 (1024.0f * 1024.0f),
                        SK_D3D11_Textures.RedundantLoads_2D );
  }

#if 0
  SK_D3D11_Textures.reset ();

  // Stop caching while we shutdown
  SK_D3D11_cache_textures = false;

  if (FreeLibrary_Original (SK::DXGI::hModD3D11))
  {
    DeleteCriticalSection (&tex_cs);
    DeleteCriticalSection (&hash_cs);
    DeleteCriticalSection (&dump_cs);
    DeleteCriticalSection (&cache_cs);
    DeleteCriticalSection (&inject_cs);

#ifdef NO_TLS
    DeleteCriticalSection (&cs_texinject);
#endif
  }
#endif
}

void
SK_D3D11_EnableHooks (void)
{
  InterlockedExchange (&__d3d11_ready, TRUE);
}


extern
unsigned int __stdcall HookD3D12                   (LPVOID user);

volatile ULONG __d3d11_hooked = FALSE;

struct sk_hook_d3d11_t {
 ID3D11Device**        ppDevice;
 ID3D11DeviceContext** ppImmediateContext;
};

__declspec (nothrow)
unsigned int
__stdcall
HookD3D11 (LPVOID user)
{
  // Wait for DXGI to boot
  if (CreateDXGIFactory_Import == nullptr)
  {
    static volatile ULONG implicit_init = FALSE;

    // If something called a D3D11 function before DXGI was initialized,
    //   begin the process, but ... only do this once.
    if (! InterlockedCompareExchange (&implicit_init, TRUE, FALSE))
    {
      dll_log.Log (L"[  D3D 11  ]  >> Implicit Initialization Triggered <<");
      SK_BootDXGI ();
    }

    while (CreateDXGIFactory_Import == nullptr)
      Sleep (33);

    // TODO: Handle situation where CreateDXGIFactory is unloadable
  }

  // This only needs to be done once
  if (InterlockedCompareExchange (&__d3d11_hooked, TRUE, FALSE))
    return 0;

  if (! config.apis.dxgi.d3d11.hook)
    return 0;

  bool success = SUCCEEDED (CoInitializeEx (nullptr, COINIT_MULTITHREADED));

  dll_log.Log (L"[  D3D 11  ]   Hooking D3D11");

  sk_hook_d3d11_t* pHooks = 
    (sk_hook_d3d11_t *)user;

  if (pHooks->ppDevice && pHooks->ppImmediateContext)
  {
    if (pHooks->ppDevice != nullptr)
    {
      DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 3, "ID3D11Device::CreateBuffer",
                             D3D11Dev_CreateBuffer_Override, D3D11Dev_CreateBuffer_Original,
                             D3D11Dev_CreateBuffer_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 5, "ID3D11Device::CreateTexture2D",
                             D3D11Dev_CreateTexture2D_Override, D3D11Dev_CreateTexture2D_Original,
                             D3D11Dev_CreateTexture2D_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 7, "ID3D11Device::CreateShaderResourceView",
                             D3D11Dev_CreateShaderResourceView_Override, D3D11Dev_CreateShaderResourceView_Original,
                             D3D11Dev_CreateShaderResourceView_pfn);

// Don't need it, so don't hook it.
#if 0
      DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 9, "ID3D11Device::CreateRenderTargetView",
                             D3D11Dev_CreateRenderTargetView_Override, D3D11Dev_CreateRenderTargetView_Original,
                             D3D11Dev_CreateRenderTargetView_pfn);
#endif
    }

    if (pHooks->ppImmediateContext != nullptr)
    {
      //
      // Third-party software frequently causes these hooks to become corrupted, try installing a new
      //   vftable pointer instead of hooking the function.
      //
#if 0
      DXGI_VIRTUAL_OVERRIDE (pHooks->ppImmediateContext, 7, "ID3D11DeviceContext::VSSetConstantBuffers",
                             D3D11_VSSetConstantBuffers_Override, D3D11_VSSetConstantBuffers_Original,
                             D3D11_VSSetConstantBuffers_pfn);
#else
      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 7, "ID3D11DeviceContext::VSSetConstantBuffers",
                             D3D11_VSSetConstantBuffers_Override, D3D11_VSSetConstantBuffers_Original,
                             D3D11_VSSetConstantBuffers_pfn);
#endif

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 8, "ID3D11DeviceContext::PSSetShaderResources",
                             D3D11_PSSetShaderResources_Override, D3D11_PSSetShaderResources_Original,
                             D3D11_PSSetShaderResources_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 12, "ID3D11DeviceContext::DrawIndexed",
                           D3D11_DrawIndexed_Override, D3D11_DrawIndexed_Original,
                           D3D11_DrawIndexed_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 13, "ID3D11DeviceContext::Draw",
                           D3D11_Draw_Override, D3D11_Draw_Original,
                           D3D11_Draw_pfn);

      //
      // Third-party software frequently causes these hooks to become corrupted, try installing a new
      //   vftable pointer instead of hooking the function.
      //
#if 0
      DXGI_VIRTUAL_OVERRIDE (pHooks->ppImmediateContext, 14, "ID3D11DeviceContext::Map",
                           D3D11_Map_Override, D3D11_Map_Original,
                           D3D11_Map_pfn);
#else
      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 14, "ID3D11DeviceContext::Map",
                           D3D11_Map_Override, D3D11_Map_Original,
                           D3D11_Map_pfn);
#endif

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 20, "ID3D11DeviceContext::DrawIndexedInstanced",
                           D3D11_DrawIndexedInstanced_Override, D3D11_DrawIndexedInstanced_Original,
                           D3D11_DrawIndexedInstanced_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 21, "ID3D11DeviceContext::DrawInstanced",
                           D3D11_DrawInstanced_Override, D3D11_DrawInstanced_Original,
                           D3D11_DrawInstanced_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 39, "ID3D11DeviceContext::DrawIndexedInstancedIndirect",
                           D3D11_DrawIndexedInstancedIndirect_Override, D3D11_DrawIndexedInstancedIndirect_Original,
                           D3D11_DrawIndexedInstancedIndirect_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 40, "ID3D11DeviceContext::DrawInstancedIndirect",
                           D3D11_DrawInstancedIndirect_Override, D3D11_DrawInstancedIndirect_Original,
                           D3D11_DrawInstancedIndirect_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 44, "ID3D11DeviceContext::RSSetViewports",
                           D3D11_RSSetViewports_Override, D3D11_RSSetViewports_Original,
                           D3D11_RSSetViewports_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 45, "ID3D11DeviceContext::RSSetScissorRects",
                           D3D11_RSSetScissorRects_Override, D3D11_RSSetScissorRects_Original,
                           D3D11_RSSetScissorRects_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 47, "ID3D11DeviceContext::CopyResource",
                           D3D11_CopyResource_Override, D3D11_CopyResource_Original,
                           D3D11_CopyResource_pfn);

      DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 48, "ID3D11DeviceContext::UpdateSubresource",
                           D3D11_UpdateSubresource_Override, D3D11_UpdateSubresource_Original,
                           D3D11_UpdateSubresource_pfn);
    }

    MH_ApplyQueued ();
  }

  if (config.apis.dxgi.d3d12.hook)
    HookD3D12 (nullptr);

  if (success)
    CoUninitialize ();

  return 0;
}


HRESULT
WINAPI
ReShade_D3D11CreateDevice_Detour (
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
  return D3D11CreateDeviceAndSwapChain_Detour ( pAdapter, DriverType, Software, Flags, pFeatureLevels,
                                                FeatureLevels, SDKVersion, nullptr, nullptr, ppDevice, pFeatureLevel,
                                                ppImmediateContext );
}

void
SK_D3D11_FixReShade (HMODULE hModReShade)
{
//  SK_LOG0 ( (L"Fixing Potential ReShade Infinite Recursion (D3D11CreateDevice)"),
//             L"ReShadeFix" );
//
//  LPVOID dontcare = nullptr;
//
//  SK_CreateFuncHook (     L"ReShade D3D11CreateDevice",
//      GetProcAddress (hModReShade, "D3D11CreateDevice"),
//                     ReShade_D3D11CreateDevice_Detour,
//                          (LPVOID *)&dontcare );
}