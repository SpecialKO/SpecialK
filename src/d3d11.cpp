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
#include <d3dcompiler.h>

#include <algorithm>


// For texture caching to work correctly ...
//   DarkSouls3 seems to underflow references on occasion!!!
#define DS3_REF_TWEAK

CRITICAL_SECTION cs_shader      = { };
CRITICAL_SECTION cs_render_view = { };
CRITICAL_SECTION cs_mmio        = { };

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
                 last_results = { };
    } pipeline_stats_d3d11    = { };
  };
};

extern void WaitForInitDXGI (void);

HMODULE SK::DXGI::hModD3D11 = 0;

volatile LONG SK_D3D11_tex_init = FALSE;
volatile LONG  __d3d11_ready    = FALSE;

void WaitForInitD3D11 (void)
{
  while (! InterlockedCompareExchange (&__d3d11_ready, FALSE, FALSE))
    MsgWaitForMultipleObjectsEx (0, nullptr, config.system.init_delay, QS_ALLINPUT, MWMO_ALERTABLE);
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

interface ID3DX11ThreadPump;

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

bool  __stdcall SK_D3D11_TextureIsCached       ( ID3D11Texture2D*     pTex );
void  __stdcall SK_D3D11_RemoveTexFromCache    ( ID3D11Texture2D*     pTex );

//#define FULL_RESOLUTION

D3DX11CreateTextureFromFileW_pfn D3DX11CreateTextureFromFileW = nullptr;
D3DX11GetImageInfoFromFileW_pfn  D3DX11GetImageInfoFromFileW  = nullptr;
HMODULE                          hModD3DX11_43                = nullptr;

#include <SpecialK/tls.h>

bool SK_D3D11_IsTexInjectThread (DWORD dwThreadId = GetCurrentThreadId ())
{
  UNREFERENCED_PARAMETER (dwThreadId);

  SK_TLS* pTLS = SK_TLS_Top ();

  if (pTLS != nullptr)
    return pTLS->d3d11.texinject_thread;
  else {
    dll_log.Log (L"[ SpecialK ] >> Thread-Local Storage is BORKED! <<");
    return false;
  }
}

void
SK_D3D11_ClearTexInjectThread ( DWORD dwThreadId = GetCurrentThreadId () )
{
  UNREFERENCED_PARAMETER (dwThreadId);

  SK_TLS* pTLS = SK_TLS_Top ();

  if (pTLS != nullptr)
    pTLS->d3d11.texinject_thread = false;
  else
    dll_log.Log (L"[ SpecialK ] >> Thread-Local Storage is BORKED! <<");
}

void
SK_D3D11_SetTexInjectThread ( DWORD dwThreadId = GetCurrentThreadId () )
{
  UNREFERENCED_PARAMETER (dwThreadId);

  SK_TLS* pTLS = SK_TLS_Top ();

  if (pTLS != nullptr)
    pTLS->d3d11.texinject_thread = true;
  else
    dll_log.Log (L"[ SpecialK ] >> Thread-Local Storage is BORKED! <<");
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
    //if (SUCCEEDED (This->QueryInterface (IID_PPV_ARGS (&pTex)))){
      if (pTex != nullptr && SK_D3D11_TextureIsCached (pTex))
        SK_D3D11_UseTexture (pTex);
    //}
  }

  return IUnknown_AddRef_Original (This);
}

// NEVER, under any circumstances, call any functions using this!
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

      // We ARE technically holding a reference, but we never make calls to this
      //   interface - it's just for tracking purposes.
      g_pD3D11Dev = *ppDevice;

      SK_GetCurrentRenderBackend ().api                  = SK_RenderAPI::D3D11;
    }

    if (config.render.dxgi.exception_mode != -1)
      (*ppDevice)->SetExceptionMode (config.render.dxgi.exception_mode);

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
  DXGI_SWAP_CHAIN_DESC  swap_chain_override = {   };

  DXGI_LOG_CALL_1 (L"D3D11CreateDeviceAndSwapChain", L"Flags=%x", Flags );

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
    if (SK_GetCallingDLL () != SK_GetDLL ())
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

  if (SUCCEEDED (res))
  {
    if (swap_chain_desc != nullptr)
    {
      if ( dwRenderThread == 0x00 ||
           dwRenderThread == GetCurrentThreadId () )
      {
        if ( hWndRender                    != 0 &&
             swap_chain_desc->OutputWindow != 0 &&
             swap_chain_desc->OutputWindow != hWndRender )
          dll_log.Log (L"[  D3D 11  ] Game created a new window?!");
      }
    }

    extern void SK_DXGI_HookSwapChain (IDXGISwapChain* pSwapChain);

    if (ppSwapChain != nullptr)
      SK_DXGI_HookSwapChain (*ppSwapChain);

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
  DXGI_LOG_CALL_1 (L"D3D11CreateDevice", L"Flags=%x", Flags);

  return D3D11CreateDeviceAndSwapChain_Detour (pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, nullptr, nullptr, ppDevice, pFeatureLevel, ppImmediateContext);
}


__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
_In_            ID3D11Device           *This,
_In_  /*const*/ D3D11_TEXTURE2D_DESC   *pDesc,
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

D3D11_CopyResource_pfn                              D3D11_CopyResource_Original                              = nullptr;
D3D11_CopySubresourceRegion_pfn                     D3D11_CopySubresourceRegion_Original                     = nullptr;
D3D11_UpdateSubresource1_pfn                        D3D11_UpdateSubresource1_Original                        = nullptr;

bool
SK_D3D11_IsDataSizeClassOf (DXGI_FORMAT typeless, DXGI_FORMAT typed, int recursion = 0)
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
  if (pDesc != nullptr)
  {
    D3D11_RENDER_TARGET_VIEW_DESC desc;
    D3D11_RESOURCE_DIMENSION      dim;

    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      desc = *pDesc;

      DXGI_FORMAT newFormat = pDesc->Format;

      CComPtr <ID3D11Texture2D> pTex = nullptr;

      pResource->QueryInterface <ID3D11Texture2D> (&pTex);

      if (pTex)
      {
        D3D11_TEXTURE2D_DESC tex_desc;
        pTex->GetDesc (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          desc.Format = newFormat;

        HRESULT hr = D3D11Dev_CreateRenderTargetView_Original ( This, pResource,
                                                                  &desc, ppRTView );

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  HRESULT hr = D3D11Dev_CreateRenderTargetView_Original (
                 This, pResource,
                   pDesc, ppRTView );

  return hr;
}


bool SK_D3D11_EnableTracking = false;

struct SK_D3D11_RenderHacks_s
{
  bool hq_volumetric = false;
} SK_D3D11_Hacks;

SK_D3D11_KnownShaders SK_D3D11_Shaders;

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
    for (auto it : rt_views)
      it->Release ();

    for (auto it : ds_views)
      it->Release ();

    rt_views.clear ();
    ds_views.clear ();
  }

} SK_D3D11_RenderTargets;


struct SK_D3D11_KnownThreads
{
  SK_D3D11_KnownThreads (void)
  {
    InitializeCriticalSectionAndSpinCount (&cs, 0x6666);
  }

  void   clear_all    (void);
  size_t count_all    (void);

  void   clear_active (void)
  {
    SK_AutoCriticalSection auto_cs (&cs);

    active.clear ();
  }

  size_t count_active (void)
  {
    SK_AutoCriticalSection auto_cs (&cs);

    return active.size ();
  }

  float  active_ratio (void)
  {
    SK_AutoCriticalSection auto_cs (&cs);

    return (float)active.size () / (float)ids.size ();
  }

  void   mark         (void);

private:
  std::set <DWORD> ids;
  std::set <DWORD> active;
  CRITICAL_SECTION cs;
} SK_D3D11_MemoryThreads,
  SK_D3D11_DrawThreads,
  SK_D3D11_ShaderThreads;


void
SK_D3D11_KnownThreads::clear_all (void)
{
  SK_AutoCriticalSection auto_cs (&cs);

  ids.clear ();
}

size_t
SK_D3D11_KnownThreads::count_all (void)
{
  SK_AutoCriticalSection auto_cs (&cs);

  return ids.size ();
}

void
SK_D3D11_KnownThreads::mark (void)
{
  if (! SK_D3D11_EnableTracking)
    return;

  SK_AutoCriticalSection auto_cs (&cs);

  ids.emplace    (GetCurrentThreadId ());
  active.emplace (GetCurrentThreadId ());
}

#include <array>

struct memory_tracking_s
{
  memory_tracking_s (void)
  {
    cs = &cs_mmio;
  }

  struct history_s
  {
    history_s (void)
    {
      vertex_buffers.reserve   ( 32);
      index_buffers.reserve    ( 64);
      constant_buffers.reserve (512);
    }

    void clear (CRITICAL_SECTION* cs)
    {
      SK_AutoCriticalSection auto_cs (cs);

      InterlockedExchange (&map_types [0], 0); InterlockedExchange (&map_types [1], 0);
      InterlockedExchange (&map_types [2], 0); InterlockedExchange (&map_types [3], 0);
      InterlockedExchange (&map_types [4], 0);

      InterlockedExchange (&resource_types [0], 0); InterlockedExchange (&resource_types [1], 0);
      InterlockedExchange (&resource_types [2], 0); InterlockedExchange (&resource_types [3], 0);
      InterlockedExchange (&resource_types [4], 0);

      InterlockedExchange64 (&bytes_written, 0ULL);
      InterlockedExchange64 (&bytes_read,    0ULL);
      InterlockedExchange64 (&bytes_copied,  0ULL);

      mapped_resources.clear ();
      index_buffers.clear    ();
      vertex_buffers.clear   ();
      constant_buffers.clear ();
    }

    int pinned_frames = 0;

    std::unordered_set <ID3D11Buffer *>   index_buffers;
    std::unordered_set <ID3D11Buffer *>   vertex_buffers;
    std::unordered_set <ID3D11Buffer *>   constant_buffers;

    std::unordered_set <ID3D11Resource *> mapped_resources;
    volatile  LONG                        map_types      [ 5 ] = { };
    volatile  LONG                        resource_types [ 5 ] = { };

    volatile  LONG64                      bytes_read           = 0ULL;
    volatile  LONG64                      bytes_written        = 0ULL;
    volatile  LONG64                      bytes_copied         = 0ULL;
  } lifetime, last_frame;


  volatile ULONG                 num_maps      = 0UL;
  volatile ULONG                 num_unmaps    = 0UL; // If does not match, something is pinned.


  void clear (void)
  {
    if (num_maps != num_unmaps)
      ++lifetime.pinned_frames;

    InterlockedExchange (&num_maps,   0);
    InterlockedExchange (&num_unmaps, 0);

    InterlockedAdd (&lifetime.map_types [0], last_frame.map_types [0]);
    InterlockedAdd (&lifetime.map_types [1], last_frame.map_types [1]);
    InterlockedAdd (&lifetime.map_types [2], last_frame.map_types [2]);
    InterlockedAdd (&lifetime.map_types [3], last_frame.map_types [3]);
    InterlockedAdd (&lifetime.map_types [4], last_frame.map_types [4]);

    InterlockedAdd (&lifetime.resource_types [0], last_frame.resource_types [0]);
    InterlockedAdd (&lifetime.resource_types [1], last_frame.resource_types [1]);
    InterlockedAdd (&lifetime.resource_types [2], last_frame.resource_types [2]);
    InterlockedAdd (&lifetime.resource_types [3], last_frame.resource_types [3]);
    InterlockedAdd (&lifetime.resource_types [4], last_frame.resource_types [4]);

    InterlockedAdd64 (&lifetime.bytes_read,    last_frame.bytes_read);
    InterlockedAdd64 (&lifetime.bytes_written, last_frame.bytes_written);
    InterlockedAdd64 (&lifetime.bytes_copied,  last_frame.bytes_copied);

    last_frame.clear (cs);
  }

  CRITICAL_SECTION* cs;
} mem_map_stats;

struct target_tracking_s
{
  target_tracking_s (void)
  {
    ZeroMemory (active, sizeof (bool) * D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

    ref_vs.reserve (32); ref_ps.reserve (32);
    ref_gs.reserve (32);
    ref_hs.reserve (32); ref_ds.reserve (32);
    ref_cs.reserve (32);
  };

  void clear (void)
  {
    memset (active, 0, sizeof (bool) * D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

    num_draws = 0;

    ref_vs.clear ();
    ref_ps.clear ();
    ref_gs.clear ();
    ref_hs.clear ();
    ref_ds.clear ();
    ref_cs.clear ();
  }

  ID3D11RenderTargetView*       resource     =  (ID3D11RenderTargetView *)INTPTR_MAX;
  bool                          active [D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]
                                             = { };

  int                           num_draws    =     0;

  std::unordered_set <uint32_t> ref_vs;
  std::unordered_set <uint32_t> ref_ps;
  std::unordered_set <uint32_t> ref_gs;
  std::unordered_set <uint32_t> ref_hs;
  std::unordered_set <uint32_t> ref_ds;
  std::unordered_set <uint32_t> ref_cs;
} tracked_rtv;

ID3D11Texture2D* tracked_texture = nullptr;

DWORD            tracked_tex_blink_duration = 666UL;

struct SK_DisjointTimerQueryD3D11 shader_tracking_s::disjoint_query;



void
shader_tracking_s::activate ( ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances )
{
  for ( UINT i = 0 ; i < NumClassInstances ; i++ )
    if (ppClassInstances && ppClassInstances [i])
      addClassInstance (ppClassInstances [i]);


  if (! active)
  {
    active = true;
  }

  else
    return;


  CComPtr <ID3D11Device> dev = nullptr;

  if (SUCCEEDED (SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&dev)))
  {
    CComPtr <ID3D11DeviceContext> dev_ctx = nullptr;

    dev->GetImmediateContext (&dev_ctx);

    if (dev_ctx == nullptr)
      return;

    if (disjoint_query.async == nullptr && timers.size () == 0)
    {
      D3D11_QUERY_DESC query_desc {
        D3D11_QUERY_TIMESTAMP_DISJOINT, 0x00
      };

      if (SUCCEEDED (dev->CreateQuery (&query_desc, &disjoint_query.async)))
      {
        dev_ctx->Begin (disjoint_query.async);
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

      if (SUCCEEDED (dev->CreateQuery (&query_desc, &duration.start.async)))
      {
        dev_ctx->End (duration.start.async);
        timers.push_back (duration);
      }
    }
  }
}

void
shader_tracking_s::deactivate (void)
{
  if (active)
  {
    active = false;
  }

  else
    return;

  CComPtr <ID3D11Device> dev = nullptr;

  if (SUCCEEDED (SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&dev)) && disjoint_query.active)
  {
    CComPtr <ID3D11DeviceContext> dev_ctx = nullptr;

    dev->GetImmediateContext (&dev_ctx);

    if (dev_ctx == nullptr)
      return;

    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP, 0x00
    };

    duration_s& duration = timers.back ();

    if (SUCCEEDED (dev->CreateQuery (&query_desc, &duration.end.async)))
    {
      dev_ctx->End (duration.end.async);
    }
  }
}

void
shader_tracking_s::use (IUnknown* pShader)
{
  ++num_draws;
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


  NvAPI_Status ret =
    NvAPI_D3D11_CreateVertexShaderEx_Original ( pDevice,
                                                  pShaderBytecode, BytecodeLength,
                                                    pClassLinkage, pCreateVertexShaderExArgs,
                                                      ppVertexShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum = crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    EnterCriticalSection (&cs_shader);

    if (! SK_D3D11_Shaders.vertex.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;

      desc.type   = SK_D3D11_ShaderType::Vertex;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.vertex.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.vertex.rev.count (*ppVertexShader) &&
               SK_D3D11_Shaders.vertex.rev [*ppVertexShader] != checksum )
      SK_D3D11_Shaders.vertex.rev.erase (*ppVertexShader);

    SK_D3D11_Shaders.vertex.rev.emplace (std::make_pair (*ppVertexShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.vertex.descs [checksum];

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
    _time64 (&desc.usage.last_time);

    LeaveCriticalSection (&cs_shader);
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


  NvAPI_Status ret =
    NvAPI_D3D11_CreateHullShaderEx_Original ( pDevice,
                                                pShaderBytecode, BytecodeLength,
                                                  pClassLinkage, pCreateHullShaderExArgs,
                                                    ppHullShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum = crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    EnterCriticalSection (&cs_shader);

    if (! SK_D3D11_Shaders.hull.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Hull;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.hull.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.hull.rev.count (*ppHullShader) &&
               SK_D3D11_Shaders.hull.rev [*ppHullShader] != checksum )
      SK_D3D11_Shaders.hull.rev.erase (*ppHullShader);

    SK_D3D11_Shaders.hull.rev.emplace (std::make_pair (*ppHullShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.hull.descs [checksum];

    LeaveCriticalSection (&cs_shader);

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


  NvAPI_Status ret =
    NvAPI_D3D11_CreateDomainShaderEx_Original ( pDevice,
                                                  pShaderBytecode, BytecodeLength,
                                                    pClassLinkage, pCreateDomainShaderExArgs,
                                                      ppDomainShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum = crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    EnterCriticalSection (&cs_shader);

    if (! SK_D3D11_Shaders.domain.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Domain;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.domain.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.domain.rev.count (*ppDomainShader) &&
               SK_D3D11_Shaders.domain.rev [*ppDomainShader] != checksum )
      SK_D3D11_Shaders.domain.rev.erase (*ppDomainShader);

    SK_D3D11_Shaders.domain.rev.emplace (std::make_pair (*ppDomainShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.domain.descs [checksum];

    LeaveCriticalSection (&cs_shader);

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
    uint32_t checksum = crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    EnterCriticalSection (&cs_shader);

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.geometry.descs [checksum];

    LeaveCriticalSection (&cs_shader);

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


  NvAPI_Status ret =
    NvAPI_D3D11_CreateFastGeometryShaderExplicit_Original ( pDevice,
                                                              pShaderBytecode, BytecodeLength,
                                                                pClassLinkage, pCreateFastGSArgs,
                                                                  ppGeometryShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum = crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    EnterCriticalSection (&cs_shader);

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.geometry.descs [checksum];

    LeaveCriticalSection (&cs_shader);

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


  NvAPI_Status ret =
    NvAPI_D3D11_CreateFastGeometryShader_Original ( pDevice,
                                                      pShaderBytecode, BytecodeLength,
                                                        pClassLinkage, ppGeometryShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum = crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    EnterCriticalSection (&cs_shader);

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.geometry.descs [checksum];

    LeaveCriticalSection (&cs_shader);

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
    return crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ? 
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
  {
    return 0x00;
  }
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
  SK_D3D11_ShaderThreads.mark ();


  HRESULT hr =
    D3D11Dev_CreateVertexShader_Original ( This,
                                             pShaderBytecode, BytecodeLength,
                                               pClassLinkage, ppVertexShader );

  if (SUCCEEDED (hr) && ppVertexShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      return hr;

    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (! SK_D3D11_Shaders.vertex.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;

      desc.type   = SK_D3D11_ShaderType::Vertex;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.vertex.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.vertex.rev.count (*ppVertexShader) &&
               SK_D3D11_Shaders.vertex.rev [*ppVertexShader] != checksum )
      SK_D3D11_Shaders.vertex.rev.erase (*ppVertexShader);

    SK_D3D11_Shaders.vertex.rev.emplace (std::make_pair (*ppVertexShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.vertex.descs [checksum];

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
    _time64 (&desc.usage.last_time);
  }

  return hr;
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
  SK_D3D11_ShaderThreads.mark ();


  HRESULT hr =
    D3D11Dev_CreatePixelShader_Original ( This, pShaderBytecode,
                                            BytecodeLength, pClassLinkage,
                                              ppPixelShader );

  if (SUCCEEDED (hr) && ppPixelShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      return hr;

    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (! SK_D3D11_Shaders.pixel.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Pixel;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.pixel.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.pixel.rev.count (*ppPixelShader) &&
               SK_D3D11_Shaders.pixel.rev [*ppPixelShader] != checksum )
      SK_D3D11_Shaders.pixel.rev.erase (*ppPixelShader);

    SK_D3D11_Shaders.pixel.rev.emplace (std::make_pair (*ppPixelShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.pixel.descs [checksum];

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
    _time64 (&desc.usage.last_time);
  }

  return hr;
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
  SK_D3D11_ShaderThreads.mark ();


  HRESULT hr =
    D3D11Dev_CreateGeometryShader_Original ( This, pShaderBytecode,
                                               BytecodeLength, pClassLinkage,
                                                 ppGeometryShader );

  if (SUCCEEDED (hr) && ppGeometryShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      return hr;

    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.geometry.descs [checksum];

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
    _time64 (&desc.usage.last_time);
  }

  return hr;
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
  _Out_opt_       ID3D11GeometryShader       **ppGeometryShader )
{
  SK_D3D11_ShaderThreads.mark ();


  HRESULT hr =
    D3D11Dev_CreateGeometryShaderWithStreamOutput_Original ( This, pShaderBytecode,
                                                               BytecodeLength,
                                                                 pSODeclaration, NumEntries,
                                                                   pBufferStrides, NumStrides,
                                                                     RasterizedStream, pClassLinkage,
                                                                       ppGeometryShader );

  if (SUCCEEDED (hr) && ppGeometryShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      return hr;

    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.geometry.descs [checksum];

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
  SK_D3D11_ShaderThreads.mark ();


  HRESULT hr =
    D3D11Dev_CreateHullShader_Original ( This, pShaderBytecode,
                                           BytecodeLength, pClassLinkage,
                                             ppHullShader );

  if (SUCCEEDED (hr) && ppHullShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      return hr;

    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (! SK_D3D11_Shaders.hull.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Hull;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.hull.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.hull.rev.count (*ppHullShader) &&
               SK_D3D11_Shaders.hull.rev [*ppHullShader] != checksum )
      SK_D3D11_Shaders.hull.rev.erase (*ppHullShader);

    SK_D3D11_Shaders.hull.rev.emplace (std::make_pair (*ppHullShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.hull.descs [checksum];

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
    _time64 (&desc.usage.last_time);
  }

  return hr;
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
  SK_D3D11_ShaderThreads.mark ();


  HRESULT hr =
    D3D11Dev_CreateDomainShader_Original ( This, pShaderBytecode,
                                             BytecodeLength, pClassLinkage,
                                               ppDomainShader );

  if (SUCCEEDED (hr) && ppDomainShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      return hr;

    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (! SK_D3D11_Shaders.domain.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Domain;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.domain.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.domain.rev.count (*ppDomainShader) &&
               SK_D3D11_Shaders.domain.rev [*ppDomainShader] != checksum )
      SK_D3D11_Shaders.domain.rev.erase (*ppDomainShader);

    SK_D3D11_Shaders.domain.rev.emplace (std::make_pair (*ppDomainShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.domain.descs [checksum];

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
    _time64 (&desc.usage.last_time);
  }

  return hr;
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
  SK_D3D11_ShaderThreads.mark ();


  HRESULT hr =
    D3D11Dev_CreateComputeShader_Original ( This, pShaderBytecode,
                                              BytecodeLength, pClassLinkage,
                                                ppComputeShader );

  if (SUCCEEDED (hr) && ppComputeShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      return hr;

    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (! SK_D3D11_Shaders.compute.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Compute;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);

      SK_D3D11_Shaders.compute.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.compute.rev.count (*ppComputeShader) &&
               SK_D3D11_Shaders.compute.rev [*ppComputeShader] != checksum )
      SK_D3D11_Shaders.compute.rev.erase (*ppComputeShader);

    SK_D3D11_Shaders.compute.rev.emplace (std::make_pair (*ppComputeShader, checksum));

    SK_D3D11_ShaderDesc& desc = SK_D3D11_Shaders.compute.descs [checksum];

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
    _time64 (&desc.usage.last_time);
  }

  return hr;
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
  {
    SK_D3D11_Shaders.vertex.tracked.deactivate ();
    SK_D3D11_Shaders.vertex.current = 0x0;

    return D3D11_VSSetShader_Original (This, pVertexShader, ppClassInstances, NumClassInstances);
  }


  if (pVertexShader)
  {
    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (SK_D3D11_Shaders.vertex.rev.count (pVertexShader))
    {
      ++SK_D3D11_Shaders.vertex.changes_last_frame;

      uint32_t checksum = SK_D3D11_Shaders.vertex.descs [SK_D3D11_Shaders.vertex.rev [pVertexShader]].crc32c;

      SK_D3D11_Shaders.vertex.current = checksum;

      InterlockedExchange (&SK_D3D11_Shaders.vertex.descs [checksum].usage.last_frame, SK_GetFramesDrawn ());
      _time64 (&SK_D3D11_Shaders.vertex.descs [checksum].usage.last_time);

      if (checksum == SK_D3D11_Shaders.vertex.tracked.crc32c)
        SK_D3D11_Shaders.vertex.tracked.activate (ppClassInstances, NumClassInstances);

      else
        SK_D3D11_Shaders.vertex.tracked.deactivate ();
    }

    else
    {
      SK_D3D11_Shaders.vertex.tracked.deactivate ();
      SK_D3D11_Shaders.vertex.current = 0x0;
    }
  }

  else
  {
    SK_D3D11_Shaders.vertex.tracked.deactivate ();
    SK_D3D11_Shaders.vertex.current = 0x0;
  }

  D3D11_VSSetShader_Original ( This, pVertexShader,
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
  return D3D11_VSGetShader_Original ( This, ppVertexShader,
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
  {
    SK_D3D11_Shaders.pixel.tracked.deactivate ();
    SK_D3D11_Shaders.pixel.current = 0x0;

    return D3D11_PSSetShader_Original (This, pPixelShader, ppClassInstances, NumClassInstances);
  }

  if (pPixelShader)
  {
    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (SK_D3D11_Shaders.pixel.rev.count (pPixelShader))
    {
      ++SK_D3D11_Shaders.pixel.changes_last_frame;

      uint32_t checksum = SK_D3D11_Shaders.pixel.descs [SK_D3D11_Shaders.pixel.rev [pPixelShader]].crc32c;

      SK_D3D11_Shaders.pixel.current = checksum;

      InterlockedExchange (&SK_D3D11_Shaders.pixel.descs [checksum].usage.last_frame, SK_GetFramesDrawn ());
      _time64 (&SK_D3D11_Shaders.pixel.descs [checksum].usage.last_time);

      if (checksum == SK_D3D11_Shaders.pixel.tracked.crc32c)
        SK_D3D11_Shaders.pixel.tracked.activate   (ppClassInstances, NumClassInstances);
      else
        SK_D3D11_Shaders.pixel.tracked.deactivate ();
    }

    else
    {
      SK_D3D11_Shaders.pixel.tracked.deactivate ();
      SK_D3D11_Shaders.pixel.current = 0x0;
    }
  }

  else
  {
    SK_D3D11_Shaders.pixel.tracked.deactivate ();
    SK_D3D11_Shaders.pixel.current = 0x0;
  }

  D3D11_PSSetShader_Original ( This, pPixelShader,
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
  {
    SK_D3D11_Shaders.geometry.tracked.deactivate ();
    SK_D3D11_Shaders.geometry.current = 0x0;

    return D3D11_GSSetShader_Original (This, pGeometryShader, ppClassInstances, NumClassInstances);
  }


  if (pGeometryShader)
  {
    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (SK_D3D11_Shaders.geometry.rev.count (pGeometryShader))
    {
      ++SK_D3D11_Shaders.geometry.changes_last_frame;

      uint32_t checksum = SK_D3D11_Shaders.geometry.descs [SK_D3D11_Shaders.geometry.rev [pGeometryShader]].crc32c;

      SK_D3D11_Shaders.geometry.current = checksum;

      InterlockedExchange (&SK_D3D11_Shaders.geometry.descs [checksum].usage.last_frame, SK_GetFramesDrawn ());
      _time64 (&SK_D3D11_Shaders.geometry.descs [checksum].usage.last_time);

      if (checksum == SK_D3D11_Shaders.geometry.tracked.crc32c)
        SK_D3D11_Shaders.geometry.tracked.activate (ppClassInstances, NumClassInstances);

      else
        SK_D3D11_Shaders.geometry.tracked.deactivate ();
    }

    else
    {
      SK_D3D11_Shaders.geometry.tracked.deactivate ();
      SK_D3D11_Shaders.geometry.current = 0x0;
    }
  }

  else
  {
    SK_D3D11_Shaders.geometry.tracked.deactivate ();
    SK_D3D11_Shaders.geometry.current = 0x0;
  }

  D3D11_GSSetShader_Original (This, pGeometryShader, ppClassInstances, NumClassInstances);
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
  {
    SK_D3D11_Shaders.hull.tracked.deactivate ();
    SK_D3D11_Shaders.hull.current = 0x0;

    return D3D11_HSSetShader_Original (This, pHullShader, ppClassInstances, NumClassInstances);
  }

  if (pHullShader)
  {
    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (SK_D3D11_Shaders.hull.rev.count (pHullShader))
    {
      ++SK_D3D11_Shaders.hull.changes_last_frame;

      uint32_t checksum = SK_D3D11_Shaders.hull.descs [SK_D3D11_Shaders.hull.rev [pHullShader]].crc32c;

      SK_D3D11_Shaders.hull.current = checksum;

      InterlockedExchange (&SK_D3D11_Shaders.hull.descs [checksum].usage.last_frame, SK_GetFramesDrawn ());
      _time64 (&SK_D3D11_Shaders.hull.descs [checksum].usage.last_time);

      if (checksum == SK_D3D11_Shaders.hull.tracked.crc32c)
        SK_D3D11_Shaders.hull.tracked.activate (ppClassInstances, NumClassInstances);

      else
        SK_D3D11_Shaders.hull.tracked.deactivate ();
    }

    else
    {
      SK_D3D11_Shaders.hull.tracked.deactivate ();
      SK_D3D11_Shaders.hull.current = 0x0;
    }
  }

  else
  {
    SK_D3D11_Shaders.hull.tracked.deactivate ();
    SK_D3D11_Shaders.hull.current = 0x0;
  }

  D3D11_HSSetShader_Original ( This, pHullShader,
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
  return D3D11_HSGetShader_Original ( This, ppHullShader,
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
  {
    SK_D3D11_Shaders.domain.tracked.deactivate ();
    SK_D3D11_Shaders.domain.current = 0x0;

    return D3D11_DSSetShader_Original (This, pDomainShader, ppClassInstances, NumClassInstances);
  }


  if (pDomainShader)
  {
    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (SK_D3D11_Shaders.domain.rev.count (pDomainShader))
    {
      ++SK_D3D11_Shaders.domain.changes_last_frame;

      uint32_t checksum = SK_D3D11_Shaders.domain.descs [SK_D3D11_Shaders.domain.rev [pDomainShader]].crc32c;

      SK_D3D11_Shaders.domain.current = checksum;

      InterlockedExchange (&SK_D3D11_Shaders.domain.descs [checksum].usage.last_frame, SK_GetFramesDrawn ());
      _time64 (&SK_D3D11_Shaders.domain.descs [checksum].usage.last_time);

      if (checksum == SK_D3D11_Shaders.domain.tracked.crc32c)
        SK_D3D11_Shaders.domain.tracked.activate (ppClassInstances, NumClassInstances);

      else
        SK_D3D11_Shaders.domain.tracked.deactivate ();
    }

    else
    {
      SK_D3D11_Shaders.domain.tracked.deactivate ();
      SK_D3D11_Shaders.domain.current = 0x0;
    }
  }

  else
  {
    SK_D3D11_Shaders.domain.tracked.deactivate ();
    SK_D3D11_Shaders.domain.current = 0x0;
  }

  D3D11_DSSetShader_Original ( This, pDomainShader,
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
  {
    SK_D3D11_Shaders.compute.tracked.deactivate ();
    SK_D3D11_Shaders.compute.current = 0x0;

    return D3D11_CSSetShader_Original (This, pComputeShader, ppClassInstances, NumClassInstances);
  }


  if (pComputeShader)
  {
    SK_AutoCriticalSection auto_cs (&cs_shader);

    if (SK_D3D11_Shaders.compute.rev.count (pComputeShader))
    {
      ++SK_D3D11_Shaders.compute.changes_last_frame;

      uint32_t checksum = SK_D3D11_Shaders.compute.descs [SK_D3D11_Shaders.compute.rev [pComputeShader]].crc32c;

      SK_D3D11_Shaders.compute.current = checksum;

      InterlockedExchange (&SK_D3D11_Shaders.compute.descs [checksum].usage.last_frame, SK_GetFramesDrawn ());
      _time64 (&SK_D3D11_Shaders.compute.descs [checksum].usage.last_time);

      if (checksum == SK_D3D11_Shaders.compute.tracked.crc32c)
        SK_D3D11_Shaders.compute.tracked.activate (ppClassInstances, NumClassInstances);

      else
        SK_D3D11_Shaders.compute.tracked.deactivate ();
    }

    else
    {
      SK_D3D11_Shaders.compute.tracked.deactivate ();
      SK_D3D11_Shaders.compute.current = 0x0;
    }
  }

  else
  {
    SK_D3D11_Shaders.compute.tracked.deactivate ();
    SK_D3D11_Shaders.compute.current = 0x0;
  }

  D3D11_CSSetShader_Original ( This, pComputeShader,
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
  return D3D11_CSGetShader_Original (This, ppComputeShader, ppClassInstances, pNumClassInstances);
}

typedef void (STDMETHODCALLTYPE *D3D11_PSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11PixelShader          *pPixelShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_GSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11GeometryShader       *pGeometryShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_HSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11HullShader           *pHullShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_DSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11DomainShader         *pDomainShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);

typedef void (STDMETHODCALLTYPE *D3D11_CSSetShader_pfn)(ID3D11DeviceContext        *This,
                                               _In_opt_ ID3D11ComputeShader        *pComputeShader,
                                               _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                                                        UINT                        NumClassInstances);


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetScissorRects_Override (
        ID3D11DeviceContext *This,
        UINT                 NumRects,
  const D3D11_RECT          *pRects )
{
  return D3D11_RSSetScissorRects_Original (This, NumRects, pRects);
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
  return D3D11_VSSetConstantBuffers_Original (This, StartSlot, NumBuffers, ppConstantBuffers );
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
  return D3D11_PSSetConstantBuffers_Original (This, StartSlot, NumBuffers, ppConstantBuffers );
}

std::unordered_set <ID3D11Texture2D *> used_textures;

bool
SK_D3D11_ActivateSRV (ID3D11ShaderResourceView* pSRV)
{
  if (! pSRV)
    return true;

  D3D11_SHADER_RESOURCE_VIEW_DESC rsv_desc;

  pSRV->GetDesc (&rsv_desc);

  CComPtr <ID3D11Resource>  pRes = nullptr;
  CComPtr <ID3D11Texture2D> pTex = nullptr;

  if (rsv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
  {
    pSRV->GetResource (&pRes);

    if (SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)))
    {
      SK_AutoCriticalSection auto_cs (&cs_render_view);

      used_textures.emplace (pTex);

      if (tracked_texture == pTex && config.textures.d3d11.highlight_debug)
      {
        if (timeGetTime () % tracked_tex_blink_duration > tracked_tex_blink_duration / 2)
          return false;
      }
    }
  }

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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
    return D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView** newResourceViews = nullptr;

  if (ppShaderResourceViews && NumViews > 0)
  {
    newResourceViews = new ID3D11ShaderResourceView* [NumViews];

    SK_AutoCriticalSection auto_cs (&cs_shader);

    shader_tracking_s& tracked = SK_D3D11_Shaders.vertex.tracked;

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRV (ppShaderResourceViews [i]))
        newResourceViews [i] =  ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (   tracked.crc32c != 0 && tracked.active && ppShaderResourceViews [i] &&
          (! tracked.used_views.count (ppShaderResourceViews [i])))
      {
        ppShaderResourceViews [i]->AddRef ();

        tracked.used_views.insert (ppShaderResourceViews [i]);
      }
    }
  }

  D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);

  if (newResourceViews)
    delete [] newResourceViews;
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
    return D3D11_PSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView** newResourceViews = nullptr;

  if (ppShaderResourceViews && NumViews > 0)
  {
    newResourceViews = new ID3D11ShaderResourceView* [NumViews];

    SK_AutoCriticalSection auto_cs (&cs_shader);

    shader_tracking_s& tracked = SK_D3D11_Shaders.pixel.tracked;

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRV (ppShaderResourceViews [i]))
        newResourceViews [i] =  ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (   tracked.crc32c != 0 && tracked.active && ppShaderResourceViews [i] &&
          (! tracked.used_views.count (ppShaderResourceViews [i])))
      {
        ppShaderResourceViews [i]->AddRef ();

        tracked.used_views.insert (ppShaderResourceViews [i]);
      }
    }
  }

  D3D11_PSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);

  if (newResourceViews)
    delete [] newResourceViews;
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
    return D3D11_GSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView** newResourceViews = nullptr;

  if (ppShaderResourceViews && NumViews > 0)
  {
    newResourceViews = new ID3D11ShaderResourceView* [NumViews];

    SK_AutoCriticalSection auto_cs (&cs_shader);

    shader_tracking_s& tracked = SK_D3D11_Shaders.geometry.tracked;

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRV (ppShaderResourceViews [i]))
        newResourceViews [i] =  ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (   tracked.crc32c != 0 && tracked.active && ppShaderResourceViews [i] &&
          (! tracked.used_views.count (ppShaderResourceViews [i])))
      {
        ppShaderResourceViews [i]->AddRef ();

        tracked.used_views.insert (ppShaderResourceViews [i]);
      }
    }
  }

  D3D11_GSSetShaderResources_Original ( This, StartSlot,
                                          NumViews, newResourceViews );

  if (newResourceViews)
    delete [] newResourceViews;
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
    return D3D11_HSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView** newResourceViews = nullptr;

  if (ppShaderResourceViews && NumViews > 0)
  {
    newResourceViews = new ID3D11ShaderResourceView* [NumViews];

    SK_AutoCriticalSection auto_cs (&cs_shader);

    shader_tracking_s& tracked = SK_D3D11_Shaders.hull.tracked;

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRV (ppShaderResourceViews [i]))
        newResourceViews [i] =  ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (   tracked.crc32c != 0 && tracked.active && ppShaderResourceViews [i] &&
          (! tracked.used_views.count (ppShaderResourceViews [i])))
      {
        ppShaderResourceViews [i]->AddRef ();

        tracked.used_views.insert (ppShaderResourceViews [i]);
      }
    }
  }

  D3D11_HSSetShaderResources_Original ( This, StartSlot,
                                          NumViews, newResourceViews );

  if (newResourceViews != nullptr)
    delete [] newResourceViews;
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
    return D3D11_DSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView** newResourceViews = nullptr;

  if (ppShaderResourceViews && NumViews > 0)
  {
    newResourceViews = new ID3D11ShaderResourceView* [NumViews];

    SK_AutoCriticalSection auto_cs (&cs_shader);

    shader_tracking_s& tracked = SK_D3D11_Shaders.domain.tracked;

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRV (ppShaderResourceViews [i]))
        newResourceViews [i] =  ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (   tracked.crc32c != 0 && tracked.active && ppShaderResourceViews [i] &&
          (! tracked.used_views.count (ppShaderResourceViews [i])))
      {
        ppShaderResourceViews [i]->AddRef ();

        tracked.used_views.insert (ppShaderResourceViews [i]);
      }
    }
  }

  D3D11_DSSetShaderResources_Original ( This, StartSlot,
                                          NumViews, newResourceViews );

  if (newResourceViews != nullptr)
    delete [] newResourceViews;
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking))
    return D3D11_CSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView** newResourceViews = nullptr;

  if (ppShaderResourceViews && NumViews > 0)
  {
    newResourceViews = new ID3D11ShaderResourceView* [NumViews];

    SK_AutoCriticalSection auto_cs (&cs_shader);

    shader_tracking_s& tracked = SK_D3D11_Shaders.compute.tracked;

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRV (ppShaderResourceViews [i]))
        newResourceViews [i] =  ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (   tracked.crc32c != 0 && tracked.active && ppShaderResourceViews [i] &&
          (! tracked.used_views.count (ppShaderResourceViews [i])))
      {
        ppShaderResourceViews [i]->AddRef ();

        tracked.used_views.insert (ppShaderResourceViews [i]);
      }
    }
  }

  D3D11_CSSetShaderResources_Original ( This, StartSlot,
                                          NumViews, newResourceViews );

  if (newResourceViews != nullptr)
    delete [] newResourceViews;
}

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
  //dll_log.Log (L"[   DXGI   ] [!]D3D11_UpdateSubresource (%ph, %lu, %ph, %ph, %lu, %lu)",
  //          pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);

  if (config.textures.cache.allow_staging)
  {
    CComPtr <ID3D11Texture2D> pTex = nullptr;

    if (DstSubresource == 0 && SUCCEEDED (pDstResource->QueryInterface <ID3D11Texture2D> (&pTex)) )
    {
      if (SK_D3D11_TextureIsCached (pTex))
      {
        dll_log.Log (L"[DX11TexMgr] Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
                       SK_GetCallerName ().c_str ());
        SK_D3D11_RemoveTexFromCache (pTex);
      }
    }
  }

  return D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                              pDstBox, pSrcData, SrcRowPitch,
                                                SrcDepthPitch );
}

std::map <ID3D11Resource*,  D3D11_MAPPED_SUBRESOURCE> mapped_textures;
std::map <ID3D11Resource*,  uint32_t>                 dynamic_textures;
std::map <ID3D11Texture2D*, uint32_t>                 dynamic_textures2;

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11_Map_Override (
   _In_ ID3D11DeviceContext      *This,
   _In_ ID3D11Resource           *pResource,
   _In_ UINT                      Subresource,
   _In_ D3D11_MAP                 MapType,
   _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource )
{
  //D3D11_RESOURCE_DIMENSION dim;
  //pResource->GetType (&dim);
  //
  //if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
  //{
  //  if (! (MapFlags & D3D11_MAP_FLAG_DO_NOT_WAIT))
  //  {
  //    if (MapType != D3D11_MAP_READ && MapType != D3D11_MAP_READ_WRITE)
  //      MapType = D3D11_MAP_WRITE_DISCARD;
  //  }
  //}

  D3D11_MAPPED_SUBRESOURCE local_map = { };

  if (pMappedResource == nullptr)
    pMappedResource = &local_map;

  HRESULT hr = D3D11_Map_Original ( This, pResource, Subresource,
                                      MapType, MapFlags, pMappedResource );

  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! (SK_D3D11_EnableTracking || config.textures.cache.allow_staging)))
  {
    return hr;
  }

  if (SUCCEEDED (hr) && pMappedResource)
  {
    SK_D3D11_MemoryThreads.mark ();

    if (config.textures.cache.allow_staging && MapType != D3D11_MAP_WRITE_DISCARD && MapType != D3D11_MAP_READ)
    {
      if (pMappedResource && Subresource == 0)
      {
        CComPtr <ID3D11Texture2D> pTex = nullptr;
        
        if (            pResource != nullptr &&
             SUCCEEDED (pResource->QueryInterface <ID3D11Texture2D> (&pTex)) )
        {
          if (SK_D3D11_TextureIsCached (pTex))
          {
            dll_log.Log (L"[DX11TexMgr] Cached texture was updated... removing from cache! - <%s>",
                           SK_GetCallerName ().c_str ());
            SK_D3D11_RemoveTexFromCache (pTex);
          }

          else
          {
            mapped_textures.emplace (std::make_pair (pResource, *pMappedResource));
            //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture...");
          }
        }
      }
    }

    bool read = ( MapType == D3D11_MAP_READ || 
                  MapType == D3D11_MAP_READ_WRITE );

    bool write = ( MapType == D3D11_MAP_WRITE             ||
                   MapType == D3D11_MAP_WRITE_DISCARD     ||
                   MapType == D3D11_MAP_READ_WRITE        ||
                   MapType == D3D11_MAP_WRITE_NO_OVERWRITE );

    InterlockedIncrement (&mem_map_stats.last_frame.map_types [MapType-1]);

    D3D11_RESOURCE_DIMENSION res_dim;

    pResource->GetType (&res_dim);

    switch (res_dim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]);
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]);

        CComPtr <ID3D11Buffer> pBuffer = nullptr;

        pResource->QueryInterface <ID3D11Buffer> (&pBuffer);

        D3D11_BUFFER_DESC buf_desc;
        pBuffer->GetDesc (&buf_desc);
        {
          SK_AutoCriticalSection auto_cs (&cs_mmio);

          if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
            mem_map_stats.last_frame.index_buffers.emplace (pBuffer);

          if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
            mem_map_stats.last_frame.vertex_buffers.emplace (pBuffer);

          if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
            mem_map_stats.last_frame.constant_buffers.emplace (pBuffer);
        }

        if (read)
          InterlockedAdd64 (&mem_map_stats.last_frame.bytes_read, buf_desc.ByteWidth);

        if (write)
          InterlockedAdd64 (&mem_map_stats.last_frame.bytes_written, buf_desc.ByteWidth);
      } break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]);
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]);
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]);
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
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! (SK_D3D11_EnableTracking || config.textures.cache.allow_staging)))
  {
    D3D11_Unmap_Original (This, pResource, Subresource);
    return;
  }

  SK_D3D11_MemoryThreads.mark ();


  if (pResource == nullptr)
    return;


  if (config.textures.cache.allow_staging && Subresource == 0)
  {
    D3D11_RESOURCE_DIMENSION rdim;
    pResource->GetType (&rdim);

    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      SK_AutoCriticalSection auto_cs (&cs_mmio);

      if (mapped_textures.count (pResource))
      {
        CComPtr <ID3D11Texture2D> pTex = nullptr;
        
        if (            pResource != nullptr &&
             SUCCEEDED (pResource->QueryInterface <ID3D11Texture2D> (&pTex)) )
        {
          uint32_t      checksum  = 0;
          size_t        size      = 0;

          uint32_t top_crc32 = 0x00;

          D3D11_TEXTURE2D_DESC desc;
          pTex->GetDesc (&desc);

          __declspec (noinline)
          uint32_t
          __cdecl
          crc32_tex (  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
                       _In_      const D3D11_SUBRESOURCE_DATA *pInitialData,
                       _Out_opt_       size_t                 *pSize,
                       _Out_opt_       uint32_t               *pLOD0_CRC32 );

          checksum = crc32_tex (&desc, (D3D11_SUBRESOURCE_DATA *)&mapped_textures [pResource], &size, &top_crc32);
          //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture... (%x -- %lu bytes)", checksum, size);

          dynamic_textures.emplace (std::make_pair (pResource, checksum));
        }

        mapped_textures.erase (pResource);
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
  ID3D11Texture2D* pDstTex = nullptr;

  if (            pDstResource != nullptr && 
       SUCCEEDED (pDstResource->QueryInterface <ID3D11Texture2D> (&pDstTex)) )
  {
    if (! SK_D3D11_IsTexInjectThread ())
    {
      if (SK_D3D11_TextureIsCached (pDstTex))
      {
        dll_log.Log (L"[DX11TexMgr] Cached texture was modified (CopyResource)... removing from cache! - <%s>",
                       SK_GetCallerName ().c_str ());
        SK_D3D11_RemoveTexFromCache (pDstTex);
      }
    }
  }


  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! (SK_D3D11_EnableTracking || config.textures.cache.allow_staging)))
  {
    if (pDstTex) pDstTex->Release ();

    D3D11_CopyResource_Original (This, pDstResource, pSrcResource);

    return;
  }


  SK_D3D11_MemoryThreads.mark ();

  D3D11_RESOURCE_DIMENSION res_dim;

  pSrcResource->GetType (&res_dim);

  switch (res_dim)
  {
    case D3D11_RESOURCE_DIMENSION_UNKNOWN:
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]);
      break;
    case D3D11_RESOURCE_DIMENSION_BUFFER:
    {
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]);

      CComPtr <ID3D11Buffer> pBuffer = nullptr;

      pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer);

      D3D11_BUFFER_DESC buf_desc;
      pBuffer->GetDesc (&buf_desc);
        {
          SK_AutoCriticalSection auto_cs (&cs_mmio);

          if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
            mem_map_stats.last_frame.index_buffers.emplace (pBuffer);

          if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
            mem_map_stats.last_frame.vertex_buffers.emplace (pBuffer);

          if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
            mem_map_stats.last_frame.constant_buffers.emplace (pBuffer);
        }
      InterlockedAdd64 (&mem_map_stats.last_frame.bytes_copied, buf_desc.ByteWidth);
    } break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]);
      break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]);
      break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]);
      break;
  }

  D3D11_CopyResource_Original (This, pDstResource, pSrcResource);

  if (config.textures.cache.allow_staging)
  {
    if (pDstTex != nullptr && dynamic_textures.count (pSrcResource))
    {
      dynamic_textures2.emplace (std::make_pair (pDstTex, dynamic_textures [pSrcResource]));
      dynamic_textures.erase    (pSrcResource);
    }
    else if (pDstTex)
      pDstTex->Release ();

    return;
  }

  if (pDstTex)
    pDstTex->Release ();
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
  ID3D11Texture2D* pDstTex = nullptr;

  if (            pDstResource != nullptr && 
       SUCCEEDED (pDstResource->QueryInterface <ID3D11Texture2D> (&pDstTex)) )
  {
    if (! SK_D3D11_IsTexInjectThread ())
    {
      if (DstSubresource == 0 && SK_D3D11_TextureIsCached (pDstTex))
      {
        dll_log.Log (L"[DX11TexMgr] Cached texture was modified (CopySubresourceRegion)... removing from cache! - <%s>",
                       SK_GetCallerName ().c_str ());
        SK_D3D11_RemoveTexFromCache (pDstTex);
      }
    }
  }


  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing || (! (SK_D3D11_EnableTracking || config.textures.cache.allow_staging)))
  {
    if (pDstTex) pDstTex->Release ();

    D3D11_CopySubresourceRegion_Original (This, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);

    return;
  }

  SK_D3D11_MemoryThreads.mark ();


  D3D11_RESOURCE_DIMENSION res_dim;

  pSrcResource->GetType (&res_dim);

  switch (res_dim)
  {
    case D3D11_RESOURCE_DIMENSION_UNKNOWN:
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]);
      break;
    case D3D11_RESOURCE_DIMENSION_BUFFER:
    {
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]);

      CComPtr <ID3D11Buffer> pBuffer = nullptr;

      pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer);

      D3D11_BUFFER_DESC buf_desc;
      pBuffer->GetDesc (&buf_desc);
      {
        SK_AutoCriticalSection auto_cs (&cs_mmio);

        if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
          mem_map_stats.last_frame.index_buffers.emplace (pBuffer);

        if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
          mem_map_stats.last_frame.vertex_buffers.emplace (pBuffer);

        if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
          mem_map_stats.last_frame.constant_buffers.emplace (pBuffer);
      }

      InterlockedAdd64 (&mem_map_stats.last_frame.bytes_copied, buf_desc.ByteWidth);
    } break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]);
      break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]);
      break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
      InterlockedIncrement (&mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]);
      break;
  }

  D3D11_CopySubresourceRegion_Original (This, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);

  if (config.textures.cache.allow_staging)
  {
    if (pDstTex != nullptr && dynamic_textures.count (pSrcResource))
    {
      dynamic_textures2.emplace (std::make_pair (pDstTex, dynamic_textures [pSrcResource]));
      dynamic_textures.erase    (pSrcResource);
    }

    else if (pDstTex)
      pDstTex->Release ();

    return;
  }

 if (pDstTex)
   pDstTex->Release ();
}


bool SK_D3D11_SkipDraw = false;

bool
SK_D3D11_DrawHandler (void)
{
  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top ()->imgui.drawing)
    return false;


  if (SK_D3D11_EnableTracking)
  {
    SK_AutoCriticalSection auto_cs (&cs_render_view);

    SK_D3D11_DrawThreads.mark ();

    bool rtv_active = false;

    for (auto it : tracked_rtv.active)
    {
      if (it)
        rtv_active = true;
    }

    if (rtv_active)
    {
      if (SK_D3D11_Shaders.vertex.current   != 0x00) tracked_rtv.ref_vs.insert (SK_D3D11_Shaders.vertex.current);
      if (SK_D3D11_Shaders.pixel.current    != 0x00) tracked_rtv.ref_ps.insert (SK_D3D11_Shaders.pixel.current);
      if (SK_D3D11_Shaders.geometry.current != 0x00) tracked_rtv.ref_gs.insert (SK_D3D11_Shaders.geometry.current);
      if (SK_D3D11_Shaders.hull.current     != 0x00) tracked_rtv.ref_hs.insert (SK_D3D11_Shaders.hull.current);
      if (SK_D3D11_Shaders.domain.current   != 0x00) tracked_rtv.ref_ds.insert (SK_D3D11_Shaders.domain.current);
    }
  }

  if (SK_D3D11_Shaders.vertex.tracked.active)   { SK_D3D11_Shaders.vertex.tracked.use   (nullptr); }
  if (SK_D3D11_Shaders.pixel.tracked.active)    { SK_D3D11_Shaders.pixel.tracked.use    (nullptr); }
  if (SK_D3D11_Shaders.geometry.tracked.active) { SK_D3D11_Shaders.geometry.tracked.use (nullptr); }
  if (SK_D3D11_Shaders.hull.tracked.active)     { SK_D3D11_Shaders.hull.tracked.use     (nullptr); }
  if (SK_D3D11_Shaders.domain.tracked.active)   { SK_D3D11_Shaders.domain.tracked.use   (nullptr); }

  if (SK_D3D11_Shaders.vertex.tracked.active   && SK_D3D11_Shaders.vertex.tracked.cancel_draws)   return true;
  if (SK_D3D11_Shaders.pixel.tracked.active    && SK_D3D11_Shaders.pixel.tracked.cancel_draws)    return true;
  if (SK_D3D11_Shaders.geometry.tracked.active && SK_D3D11_Shaders.geometry.tracked.cancel_draws) return true;
  if (SK_D3D11_Shaders.hull.tracked.active     && SK_D3D11_Shaders.hull.tracked.cancel_draws)     return true;
  if (SK_D3D11_Shaders.domain.tracked.active   && SK_D3D11_Shaders.domain.tracked.cancel_draws)   return true;

  if (SK_D3D11_Shaders.vertex.blacklist.count   (SK_D3D11_Shaders.vertex.current))   return true;
  if (SK_D3D11_Shaders.pixel.blacklist.count    (SK_D3D11_Shaders.pixel.current))    return true;
  if (SK_D3D11_Shaders.geometry.blacklist.count (SK_D3D11_Shaders.geometry.current)) return true;
  if (SK_D3D11_Shaders.hull.blacklist.count     (SK_D3D11_Shaders.hull.current))     return true;
  if (SK_D3D11_Shaders.domain.blacklist.count   (SK_D3D11_Shaders.domain.current))   return true;

  if (SK_D3D11_SkipDraw)
    return true;

  return false;
}

bool
SK_D3D11_DispatchHandler (void)
{
  //SK_D3D11_DrawThreads.mark ();


  if (SK_D3D11_EnableTracking)
  {
    SK_AutoCriticalSection auto_cs (&cs_render_view);

    bool rtv_active = false;

    for (const auto it : tracked_rtv.active)
    {
      if (it)
        rtv_active = true;
    }

    if (rtv_active)
    {
      if (SK_D3D11_Shaders.compute.current != 0x00) tracked_rtv.ref_cs.insert (SK_D3D11_Shaders.compute.current);
    }


    if (SK_D3D11_Shaders.compute.tracked.active) { SK_D3D11_Shaders.compute.tracked.use (nullptr); }
  }

  if (SK_D3D11_Shaders.compute.tracked.active && SK_D3D11_Shaders.compute.tracked.cancel_draws) return true;
  if (SK_D3D11_Shaders.compute.blacklist.count (SK_D3D11_Shaders.compute.current))              return true;

  if (SK_D3D11_SkipDraw)
    return true;

  return false;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawAuto_Override (_In_ ID3D11DeviceContext *This)
{
  if (SK_D3D11_DrawHandler ())
    return;

  return D3D11_DrawAuto_Original (This);
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
  if (SK_D3D11_DrawHandler ())
    return;

  return D3D11_DrawIndexed_Original ( This, IndexCount,
                                              StartIndexLocation,
                                                BaseVertexLocation );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation )
{
  if (SK_D3D11_DrawHandler ())
    return;

  return D3D11_Draw_Original ( This, VertexCount, StartVertexLocation );
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
  if (SK_D3D11_DrawHandler ())
    return;

  return
    D3D11_DrawIndexedInstanced_Original ( This, IndexCountPerInstance,
                                            InstanceCount, StartIndexLocation,
                                              BaseVertexLocation, StartInstanceLocation );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  if (SK_D3D11_DrawHandler ())
    return;

  return
    D3D11_DrawIndexedInstancedIndirect_Original ( This, pBufferForArgs,
                                                    AlignedByteOffsetForArgs );
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
  if (SK_D3D11_DrawHandler ())
    return;

  return
    D3D11_DrawInstanced_Original ( This, VertexCountPerInstance,
                                     InstanceCount, StartVertexLocation,
                                       StartInstanceLocation );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  if (SK_D3D11_DrawHandler ())
    return;

  return
    D3D11_DrawInstancedIndirect_Original ( This, pBufferForArgs,
                                             AlignedByteOffsetForArgs );
}


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Dispatch_Override ( _In_ ID3D11DeviceContext *This,
                          _In_ UINT                 ThreadGroupCountX,
                          _In_ UINT                 ThreadGroupCountY,
                          _In_ UINT                 ThreadGroupCountZ )
{
  if (SK_D3D11_DispatchHandler ())
    return;

  return D3D11_Dispatch_Original (This, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DispatchIndirect_Override ( _In_ ID3D11DeviceContext *This,
                                  _In_ ID3D11Buffer        *pBufferForArgs,
                                  _In_ UINT                 AlignedByteOffsetForArgs )
{
  if (SK_D3D11_DispatchHandler ())
    return;

  return D3D11_DispatchIndirect_Original (This, pBufferForArgs, AlignedByteOffsetForArgs);
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
  D3D11_OMSetRenderTargets_Original ( This, NumViews,
                                        ppRenderTargetViews, pDepthStencilView );

  SK_D3D11_SkipDraw = false;

  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top () && SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking)) 
  {
    for (int i = 0 ; i < 8; i++) tracked_rtv.active [i] = false;
    return;
  }

  if (NumViews > 0)
  {
    SK_AutoCriticalSection auto_cs (&cs_render_view);

    if (ppRenderTargetViews)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppRenderTargetViews [i] && (! SK_D3D11_RenderTargets.rt_views.count (ppRenderTargetViews [i])))
        {
          ppRenderTargetViews [i]->AddRef ();
          SK_D3D11_RenderTargets.rt_views.insert (ppRenderTargetViews [i]);
        }

        if (tracked_rtv.resource == ppRenderTargetViews [i])
        {
          tracked_rtv.active [i] = true;
        }

        else
        {
          tracked_rtv.active [i] = false;
        }
      }
    }

    if (pDepthStencilView)
    {
      if (! SK_D3D11_RenderTargets.ds_views.count (pDepthStencilView))
      {
        pDepthStencilView->AddRef ();
        SK_D3D11_RenderTargets.ds_views.insert (pDepthStencilView);
      }
    }
  }
}

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
  D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original (
    This, NumRTVs, ppRenderTargetViews,
      pDepthStencilView, UAVStartSlot, NumUAVs,
        ppUnorderedAccessViews, pUAVInitialCounts
  );

  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Top () && SK_TLS_Top ()->imgui.drawing || (! SK_D3D11_EnableTracking)) 
  {
    for (int i = 0 ; i < 8; i++) tracked_rtv.active [i] = false;
    return;
  }

  if (NumRTVs > 0)
  {
    SK_AutoCriticalSection auto_cs (&cs_render_view);

    if (ppRenderTargetViews)
    {
      for (UINT i = 0; i < NumRTVs; i++)
      {
        if (ppRenderTargetViews [i] && (! SK_D3D11_RenderTargets.rt_views.count (ppRenderTargetViews [i])))
        {
          ppRenderTargetViews [i]->AddRef ();
          SK_D3D11_RenderTargets.rt_views.insert (ppRenderTargetViews [i]);

          if (tracked_rtv.resource == ppRenderTargetViews [i])
          {
            tracked_rtv.active [i] = true;
          }

          else
          {
            tracked_rtv.active [i] = false;
          }
        }
      }
    }

    if (pDepthStencilView)
    {
      if (! SK_D3D11_RenderTargets.ds_views.count (pDepthStencilView))
      {
        pDepthStencilView->AddRef ();
        SK_D3D11_RenderTargets.ds_views.insert (pDepthStencilView);
      }
    }
  }
}

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
  char szOut [512] { };

  snprintf ( szOut, 511, "  Tex Cache: %#5zu MiB   Entries:   %#7zu\n"
                         "  Hits:      %#5lu       Time Saved: %#7.01lf ms\n"
                         "  Evictions: %#5lu",
               SK_D3D11_Textures.AggregateSize_2D >> 20ULL,
               SK_D3D11_Textures.TexRefs_2D.size (),
               SK_D3D11_Textures.RedundantLoads_2D,
               SK_D3D11_Textures.RedundantTime_2D,
               SK_D3D11_Textures.Evicted_2D );

  return std::move (szOut);
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

  SK_AutoCriticalSection critical (&cache_cs);

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

static volatile ULONG live_textures_dirty = FALSE;

void
__stdcall
SK_D3D11_TexCacheCheckpoint (void)
{
  static int       iter               = 0;

  static bool      init               = false;
  static ULONGLONG ullMemoryTotal_KiB = 0;
  static HANDLE    hProc              = nullptr;

  if (! init)
  {
    init  = true;
    hProc = GetCurrentProcess ();

    GetPhysicallyInstalledSystemMemory (&ullMemoryTotal_KiB);
  }

  ++iter;

  if ((iter % 3))
    return;

  PROCESS_MEMORY_COUNTERS pmc    = {   };
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

  SK_AutoCriticalSection critical (&cache_cs);
  {
    for ( ID3D11Texture2D* tex : TexRefs_2D )
    {
      if (Textures_2D.count (tex))
        textures.push_back (Textures_2D [tex]);
    }
  }

  {
    std::sort ( textures.begin (),
                  textures.end (),
        []( SK_D3D11_TexMgr::tex2D_descriptor_s a,
            SK_D3D11_TexMgr::tex2D_descriptor_s b )
      {
        return a.last_used < b.last_used;
      }
    );
  }

  const uint32_t max_count =
    cache_opts.max_evict;

  for ( SK_D3D11_TexMgr::tex2D_descriptor_s desc : textures )
  {
    int64_t mem_size = (int64_t)(desc.mem_size >> 20);

    if (desc.texture != nullptr)
    {
#ifdef DS3_REF_TWEAK
      const int refs =
        IUnknown_AddRef_Original (desc.texture) - 1;

      if (refs <= 3 && desc.texture->Release () <= 3)
      {
#else
      int refs = IUnknown_AddRef_Original (desc.texture) - 1;

      if (refs == 1 && desc.texture->Release () <= 1)
      {
#endif
        for (int i = 0; i < refs; i++)
        {
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

        HashMap_2D [desc.desc.MipLevels].erase (desc.crc32);
        Textures_2D.erase (desc.texture);
        TexRefs_2D.erase  (desc.texture);
      }

      else
      {
        desc.texture->Release ();
      }
    }
  }

  if (count > 0)
    InterlockedExchange (&live_textures_dirty, TRUE);
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
    
    SK_AutoCriticalSection critical0 (&cache_cs);

    auto it = HashMap_2D [pDesc->MipLevels][crc32];
#endif
      tex2D_descriptor_s desc2d;
      {
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

        const size_t    size = desc2d.mem_size;
        const uint64_t  time = desc2d.load_time;
        const float    fTime = (float)time * 1000.0f / (float)PerfFreq.QuadPart;

        if (pMemSize != nullptr)
        {
          *pMemSize = size;
        }

        if (pTimeSaved != nullptr)
        {
          *pTimeSaved = fTime;
        }

        desc2d.last_used = SK_QueryPerf ().QuadPart;

        // Don't record cache hits caused by the shader mod interface
        if (! SK_TLS_Top ()->imgui.drawing)
        {
          desc2d.hits++;

          RedundantData_2D += size;
          RedundantTime_2D += fTime;
          RedundantLoads_2D++;
        }

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

  if (SK_D3D11_TextureIsCached (pTex))
  {
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

  if (pTex != nullptr)
  {
    SK_AutoCriticalSection critical0 (&cache_cs);

    uint32_t crc32 =
      SK_D3D11_Textures.Textures_2D [pTex].crc32;

    SK_D3D11_Textures.AggregateSize_2D -=
      SK_D3D11_Textures.Textures_2D [pTex].mem_size;

    SK_D3D11_Textures.Evicted_2D++;

    D3D11_TEXTURE2D_DESC desc;
    pTex->GetDesc (&desc);

    SK_D3D11_Textures.Textures_2D.erase  (pTex);
    //SK_D3D11_Textures.HashMap_2D [desc.MipLevels].erase (crc32);
    SK_D3D11_Textures.TexRefs_2D.erase   (pTex);

    const int refs =
      IUnknown_AddRef_Original (pTex) - 1;

    if (refs <= 3 && pTex->Release () <= 3)
    {
      for (int i = 0; i < refs; i++)
      {
        IUnknown_Release_Original (pTex);
      }
    }

    InterlockedExchange (&live_textures_dirty, TRUE);
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

  if (pDesc->Usage >= D3D11_USAGE_DYNAMIC)
  {
    dll_log.Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
                  L"Due To Usage: %lu",
                  crc32, pDesc->Usage );
    return;
  }

  if (pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_WRITE)
  {
    dll_log.Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
                  L"Due To CPUAccessFlags: 0x%X",
                  crc32, pDesc->CPUAccessFlags );
    return;
  }

  if (SK_D3D11_TextureIsCached (pTex))
  {
    dll_log.Log (L"[DX11TexMgr] Texture is already cached?!  { Original: %x, New: %x }",
                   Textures_2D [pTex].crc32, crc32 );
    return;
  }

  SK_AutoCriticalSection critical (&cache_cs);

  AggregateSize_2D += mem_size;

  tex2D_descriptor_s desc2d;

  desc2d.desc      = *pDesc;
  desc2d.texture   =  pTex;
  desc2d.load_time =  load_time;
  desc2d.mem_size  =  mem_size;
  desc2d.crc32     =  crc32;

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

  wchar_t wszTexDumpDir_RAW [ MAX_PATH     ] = { };
  wchar_t wszTexDumpDir     [ MAX_PATH + 2 ] = { };

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
         INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd     = {  };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    unsigned int    files  =  0UL;
    LARGE_INTEGER   liSize = {  };

    LARGE_INTEGER   liCompressed   = {   };
    LARGE_INTEGER   liUncompressed = {   };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating dumped...    " );

    lstrcatW (wszTexDumpDir, L"\\*");

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
              if (StrStrIW (StrStrIW (fd.cFileName, L"_") + 1, L"_"))
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
            {
              if (StrStrIW (StrStrIW (fd.cFileName, L"_") + 1, L"_"))
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

  wchar_t wszTexInjectDir_RAW [ MAX_PATH + 2 ] = { };
  wchar_t wszTexInjectDir     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexInjectDir_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir_RAW, L"\\inject\\textures");

  wcscpy ( wszTexInjectDir,
             SK_EvalEnvironmentVars (wszTexInjectDir_RAW).c_str () );

  if ( GetFileAttributesW (wszTexInjectDir) !=
         INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd     = {   };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    unsigned int    files  =   0;
    LARGE_INTEGER   liSize = {   };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating injectable..." );

    lstrcatW (wszTexInjectDir, L"\\*");

    hFind = FindFirstFileW (wszTexInjectDir, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          if (StrStrIW (fd.cFileName, L".dds"))
          {
            uint32_t top_crc32 = 0x00;
            uint32_t checksum  = 0x00;

            if (StrStrIW (fd.cFileName, L"_"))
            {
              swscanf (fd.cFileName, L"%08X_%08X.dds", &top_crc32, &checksum);
            }

            else
            {
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

  wchar_t wszTexInjectDir_FFX_RAW [ MAX_PATH     ] = { };
  wchar_t wszTexInjectDir_FFX     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexInjectDir_FFX_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir_FFX_RAW, L"\\inject\\textures\\UnX_Old");

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

    lstrcatW (wszTexInjectDir_FFX, L"\\*");

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

            injectable_ffx.insert (ffx_crc32);
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    dll_log.LogEx ( false, L" %li files (%3.1f MiB)\n",
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

  if (hash != 0x00)
  {
    if (! SK_D3D11_IsTexHashed (top_crc32, hash))
    {
      SK_AutoCriticalSection critical (&hash_cs);

      tex_hashes_ex.insert (std::make_pair (crc32c (top_crc32, (const uint8_t *)&hash, 4), name));
      SK_D3D11_AddInjectable (top_crc32, hash);
    }
  }

  if (! SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
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

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
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

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
    SK_AutoCriticalSection critical (&hash_cs);

    ret = tex_hashes_ex [crc32c (top_crc32, (const uint8_t *)&hash, 4)];
  }

  else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
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

    default:                                     return L"UNKNONW";
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
  if (pLOD0_CRC32 != nullptr)
    *pLOD0_CRC32 = 0ui32;

  // Ignore Cubemaps for Now
  if (pDesc->MiscFlags == D3D11_RESOURCE_MISC_TEXTURECUBE)
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

        __try {
          checksum = crc32c (checksum, (const uint8_t *)pData, lod_size);
          size += lod_size;
        }

        __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                     EXCEPTION_EXECUTE_HANDLER :
                     EXCEPTION_CONTINUE_SEARCH )
        {
          size += stride * (height / 4) + (height % 4);
          SK_LOG0 ( ( L"Access Violation while Hashing Texture: %x", checksum ),
                      L" Tex Hash " );
        }
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

  for (unsigned int i = 0; i < pDesc->MipLevels; i++)
  {
    if (compressed)
    {
      size += (pInitialData [i].SysMemPitch / block_size) * (height >> i);

      checksum =
        crc32 (checksum, (const char *)pInitialData [i].pSysMem, (pInitialData [i].SysMemPitch / block_size) * (height >> i));
    }

    else
    {
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

void
__stdcall
SK_D3D11_RemoveDumped (uint32_t top_crc32, uint32_t checksum)
{
  SK_AutoCriticalSection critical (&dump_cs);

  if (! config.textures.d3d11.precise_hash)
    dumped_textures.erase (top_crc32);

  dumped_collisions.erase (crc32c (top_crc32, (uint8_t *)&checksum, 4));
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
SK_D3D11_DumpTexture2D ( _In_ ID3D11Texture2D* pTex, uint32_t crc32c )
{
  CComPtr <ID3D11Device>        pDev    = nullptr;
  CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

  if (SUCCEEDED (SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&pDev)))
  {
    pDev->GetImmediateContext (&pDevCtx);
  
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

      bool compressed = false;

      if ( (fmt >= DXGI_FORMAT_BC1_TYPELESS  &&
            fmt <= DXGI_FORMAT_BC5_SNORM)    ||
           (fmt >= DXGI_FORMAT_BC6H_TYPELESS &&
            fmt <= DXGI_FORMAT_BC7_UNORM_SRGB) )
        compressed = true;

      wchar_t wszOutName [MAX_PATH + 2] = { };

      if (compressed)
      {
        _swprintf ( wszOutName, L"%s\\Compressed_%08X.dds",
                      wszPath, crc32c );
      }

      else
      {
        _swprintf ( wszOutName, L"%s\\Uncompressed_%08X.dds",
                      wszPath, crc32c );
      }

      SK_CreateDirectories (wszPath);

      HRESULT hr =
        DirectX::SaveToDDSFile (img.GetImages (), img.GetImageCount (), img.GetMetadata (), 0x00, wszOutName);

      if (SUCCEEDED (hr))
      {
        SK_D3D11_AddDumped (crc32c, crc32c);

        return hr;
      }
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
  _swprintf ( wszOutName, L"%s\\Compressed_%08X.dds",
                wszPath, crc32c );

  if (DeleteFileW (wszOutName))
  {
    SK_D3D11_RemoveDumped (crc32c, crc32c);

    return TRUE;
  }

  *wszOutName = L'\0';

  _swprintf ( wszOutName, L"%s\\Uncompressed_%08X.dds",
                wszPath, crc32c );

  if (DeleteFileW (wszOutName))
  {
    SK_D3D11_RemoveDumped (crc32c, crc32c);

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
  SK_ScopedTLS tls_scope;

  SK_TLS_Top ()->d3d11.texinject_thread = true;

  dll_log.Log ( L"[DX11TexDmp] Dumping Texture: %08x::%08x... (fmt=%03lu, "
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

      for (size_t h = 0; h < lines; ++h)
      {
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

  wchar_t wszPath [ MAX_PATH + 2 ] = { };

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

  wchar_t wszOutPath [MAX_PATH + 2] = { };
  wchar_t wszOutName [MAX_PATH + 2] = { };

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
  return
    D3D11Dev_CreateBuffer_Original ( This, pDesc,
                                       pInitialData, ppBuffer );
}


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
  if (pDesc != nullptr)
  {
    DXGI_FORMAT newFormat = pDesc->Format;

    D3D11_SHADER_RESOURCE_VIEW_DESC desc = *pDesc;
    D3D11_RESOURCE_DIMENSION        dim;

    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      CComPtr <ID3D11Texture2D> pTex = nullptr;

      pResource->QueryInterface <ID3D11Texture2D> (&pTex);

      if (pTex)
      {
        D3D11_TEXTURE2D_DESC tex_desc;
        pTex->GetDesc (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          desc.Format = newFormat;

        HRESULT hr = D3D11Dev_CreateShaderResourceView_Original ( This, pResource,
                                                                    &desc, ppSRView );

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  HRESULT hr =
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
  if (pDesc != nullptr)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC desc = *pDesc;

    D3D11_RESOURCE_DIMENSION dim;
    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      DXGI_FORMAT newFormat = pDesc->Format;

      CComPtr <ID3D11Texture2D> pTex = nullptr;

      pResource->QueryInterface <ID3D11Texture2D> (&pTex);

      if (pTex)
      {
        D3D11_TEXTURE2D_DESC tex_desc;
        pTex->GetDesc (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          desc.Format = newFormat;

        HRESULT hr = D3D11Dev_CreateDepthStencilView_Original ( This, pResource,
                                                                  &desc, ppDepthStencilView );

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  HRESULT hr =
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
  if (pDesc != nullptr)
  {
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = *pDesc;
    D3D11_RESOURCE_DIMENSION         dim;

    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      DXGI_FORMAT newFormat = pDesc->Format;

      CComPtr <ID3D11Texture2D> pTex = nullptr;

      pResource->QueryInterface <ID3D11Texture2D> (&pTex);

      if (pTex)
      {
        D3D11_TEXTURE2D_DESC tex_desc;
        pTex->GetDesc (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          desc.Format = newFormat;
            

        HRESULT hr = D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                                     &desc, ppUAView );

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  HRESULT hr =
    D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                    pDesc, ppUAView );
  return hr;
}


std::set <ID3D11Texture2D *> render_tex;

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
  if (ppSamplers != nullptr)
  {
    if (SK_D3D11_Hacks.hq_volumetric)
    {
      if (StartSlot == 0 && NumSamplers == 2)
      {
        if (SK_D3D11_Shaders.pixel.current == 0xfcdb65ad)
        {
          static CComPtr <ID3D11SamplerState> pSampler = nullptr;

          if (pSampler == nullptr)
          {
            D3D11_SAMPLER_DESC new_desc;

            new_desc.AddressU        = D3D11_TEXTURE_ADDRESS_MIRROR;
            new_desc.AddressV        = D3D11_TEXTURE_ADDRESS_MIRROR;
            new_desc.AddressW        = D3D11_TEXTURE_ADDRESS_MIRROR;
            new_desc.BorderColor [0] = 0.0f; new_desc.BorderColor [1] = 0.0f;
            new_desc.BorderColor [2] = 0.0f; new_desc.BorderColor [3] = 0.0f;
            new_desc.ComparisonFunc  = D3D11_COMPARISON_NEVER;
            new_desc.Filter          = D3D11_FILTER_ANISOTROPIC;
            new_desc.MaxAnisotropy   = 16;
            new_desc.MaxLOD          =  3.402823466e+38F;
            new_desc.MinLOD          = -3.402823466e+38F;
            new_desc.MipLODBias      = 0;

            D3D11Dev_CreateSamplerState_Original ((ID3D11Device *)SK_GetCurrentRenderBackend ().device, &new_desc, &pSampler);
          }

          else
          {
            static ID3D11SamplerState* override_samplers [2] = { pSampler, pSampler };

            return D3D11_PSSetSamplers_Original (This, StartSlot, NumSamplers, override_samplers);
          }
        }
      }
    }
  }
#endif

  D3D11_PSSetSamplers_Original (This, StartSlot, NumSamplers, ppSamplers);
}

HRESULT
WINAPI
D3D11Dev_CreateSamplerState_Override
(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState )
{
#if 0
  D3D11_SAMPLER_DESC new_desc = *pSamplerDesc;

  if ( new_desc.Filter == D3D11_FILTER_MIN_MAG_MIP_LINEAR       ||
       new_desc.Filter == D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT ||
       new_desc.Filter == D3D11_FILTER_ANISOTROPIC )
  {
    new_desc.Filter = D3D11_FILTER_ANISOTROPIC;

    if (new_desc.MaxAnisotropy != 0)
      new_desc.MaxAnisotropy = 16;
  }

  HRESULT hr =
    D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

  if (SUCCEEDED (hr))
    return hr;
#endif

  return D3D11Dev_CreateSamplerState_Original (This, pSamplerDesc, ppSamplerState);
}


__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_  /*const*/ D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  // ^^^^ Const qualifier discarded from prototype because making a copy of this variable
  //        and modifying causes any other hooked function to see the original data.


  WaitForInitDXGI ();


  SK_D3D11_MemoryThreads.mark ();



  if (pDesc != nullptr)
  {
    DXGI_FORMAT newFormat = pDesc->Format;

    if (pInitialData == nullptr || pInitialData->pSysMem == nullptr)
    {
      if (SK_D3D11_OverrideDepthStencil (newFormat))
      {
        pDesc->Format = newFormat;
        pInitialData  = nullptr;
      }
    }

    //if (! (config.render.dxgi.res.max.isZero () || config.render.dxgi.res.min.isZero ()))
    //{
    //  if ( (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)    ||
    //       (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET) )
    //  {
    //    if (pDesc->Width == config.render.dxgi.res.min.x)
    //      pDesc->Width *= (float)config.render.dxgi.res.max.x / (float)config.render.dxgi.res.min.x;
    //
    //    if (pDesc->Height == config.render.dxgi.res.min.y)
    //      pDesc->Height *= (float)config.render.dxgi.res.max.y / (float)config.render.dxgi.res.min.y;
    //  }
    //}
  }


  if (ppTexture2D == nullptr)
    ppTexture2D = nullptr;

  if (InterlockedExchangeAdd (&SK_D3D11_tex_init, 0) == FALSE)
    SK_D3D11_InitTextures ();

  bool early_out = false;

  if ((! (SK_D3D11_cache_textures || SK_D3D11_dump_textures || SK_D3D11_inject_textures)) ||
         SK_D3D11_IsTexInjectThread ())
    early_out = true;

  if (early_out || (! ppTexture2D))
    return D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);

  uint32_t      checksum  = 0;
  uint32_t      cache_tag = 0;
  size_t        size      = 0;

  ID3D11Texture2D* pCachedTex = nullptr;

  bool cacheable = ( ppTexture2D           != nullptr &&
                     pInitialData          != nullptr &&
                     pInitialData->pSysMem != nullptr &&
                     pDesc->SampleDesc.Count == 1     &&
                     pDesc->MiscFlags        == 0x0   &&
                     pDesc->Width  > 0                && 
                     pDesc->Height > 0                &&
                     pDesc->ArraySize == 1 );

  cacheable = cacheable &&
    ((! (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)    ||
        (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET) )) &&
        (pDesc->BindFlags & D3D11_BIND_SHADER_RESOURCE)  &&
        (pDesc->Usage    <  D3D11_USAGE_DYNAMIC); // Cancel out Staging
                                                  //   They will be handled through a
                                                  //     different codepath.

  const bool dumpable = 
              cacheable;

  cacheable = cacheable && (! (pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_WRITE));

  //if (cacheable && (! (pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_READ)))
    //pDesc->Usage = D3D11_USAGE_IMMUTABLE;

  uint32_t top_crc32 = 0x00;
  uint32_t ffx_crc32 = 0x00;

  if (cacheable)
  {
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

      if (cacheable)
      {
        cache_tag  = crc32c (top_crc32, (uint8_t *)pDesc, sizeof D3D11_TEXTURE2D_DESC);
        pCachedTex = SK_D3D11_Textures.getTexture2D (cache_tag, pDesc);
      }
    } else {
      cacheable = false;
    }
  }

  if (pCachedTex != nullptr)
  {
    //dll_log.Log ( L"[DX11TexMgr] >> Redundant 2D Texture Load "
                  //L" (Hash=0x%08X [%05.03f MiB]) <<",
                  //checksum, (float)size / (1024.0f * 1024.0f) );
    pCachedTex->AddRef ();
    *ppTexture2D = pCachedTex;
    return S_OK;
  }

  // The concept of a cache-miss only applies if the texture had data at the time
  //   of creation...
  if ( pInitialData          != nullptr &&
       pInitialData->pSysMem != nullptr )
    SK_D3D11_Textures.CacheMisses_2D++;

  LARGE_INTEGER load_start = SK_QueryPerf ();

  if (cacheable)
  {
    if (D3DX11CreateTextureFromFileW != nullptr && SK_D3D11_res_root.length ())
    {
      wchar_t wszTex [MAX_PATH + 2] = { };

      
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

        D3DX11_IMAGE_INFO      img_info   = {   };
        D3DX11_IMAGE_LOAD_INFO load_info  = {   };

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

  if (ppTexture2D != nullptr)
  {
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
    if (! SK_D3D11_IsDumped (top_crc32, checksum))
    {
      SK_D3D11_DumpTexture2D (pDesc, pInitialData, top_crc32, checksum);
    }
  }

  cacheable &=
    (SK_D3D11_cache_textures || SK_D3D11_IsInjectable (top_crc32, checksum));

  if ( SUCCEEDED (ret) && cacheable )
  {
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
SK_D3D11_UpdateRenderStatsEx (IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain))
    return;

  CComPtr <ID3D11Device> dev = nullptr;

  if (SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&dev))))
  {
    CComPtr <ID3D11DeviceContext> dev_ctx = nullptr;

    dev->GetImmediateContext (&dev_ctx);

    if (dev_ctx == nullptr)
      return;

    SK::DXGI::PipelineStatsD3D11& pipeline_stats =
      SK::DXGI::pipeline_stats_d3d11;

    if (pipeline_stats.query.async != nullptr)
    {
      if (pipeline_stats.query.active)
      {
        dev_ctx->End (pipeline_stats.query.async);
        pipeline_stats.query.active = false;
      }

      else
      {
        const HRESULT hr =
          dev_ctx->GetData ( pipeline_stats.query.async,
                              &pipeline_stats.last_results,
                                sizeof D3D11_QUERY_DATA_PIPELINE_STATISTICS,
                                  0x0 );
        if (hr == S_OK)
        {
          pipeline_stats.query.async->Release ();
          pipeline_stats.query.async = nullptr;
        }
      }
    }

    else
    {
      D3D11_QUERY_DESC query_desc {
        D3D11_QUERY_PIPELINE_STATISTICS, 0x00
      };

      if (SUCCEEDED (dev->CreateQuery (&query_desc, &pipeline_stats.query.async)))
      {
        dev_ctx->Begin (pipeline_stats.query.async);
        pipeline_stats.query.active = true;
      }
    }
  }
}

void
__stdcall
SK_D3D11_UpdateRenderStats (IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain && config.render.show))
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
  memcpy ( (void *)&SK::DXGI::pipeline_stats_d3d11.last_results,
             pData,
               sizeof D3D11_QUERY_DATA_PIPELINE_STATISTICS );
}

void
SK_D3D11_GetVertexPipelineDesc (wchar_t* wszDesc)
{
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
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
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
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
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
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
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.CInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  RASTER : %5.1f%% Filled     (%s Triangles IN )",
                   wszDesc, 100.0f *
                       ( (float)stats.CPrimitives /
                         (float)stats.CInvocations ),
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
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
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
  D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.CSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  COMPUTE: %s",
                   wszDesc, SK_CountToString (stats.CSInvocations).c_str () );
  } else {
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


void
SK_D3D11_InitTextures (void)
{
  if (! InterlockedCompareExchange (&SK_D3D11_tex_init, TRUE, FALSE))
  {
    if ( StrStrIW (SK_GetHostApp (), L"ffx.exe")   ||
         StrStrIW (SK_GetHostApp (), L"ffx-2.exe") ||
         StrStrIW (SK_GetHostApp (), L"FFX&X-2_Will.exe") )
      SK_D3D11_inject_textures_ffx = true;

    InitializeCriticalSectionAndSpinCount (&tex_cs,    969184);
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
      SleepEx (33, TRUE);

    // TODO: Handle situation where CreateDXGIFactory is unloadable
  }

  // This only needs to be done once
  if (InterlockedCompareExchange (&__d3d11_hooked, TRUE, FALSE))
    return 0;

  if (! config.apis.dxgi.d3d11.hook)
    return 0;

  InitializeCriticalSectionAndSpinCount (&cs_shader,      0xcafe);
  InitializeCriticalSectionAndSpinCount (&cs_mmio,        0xbabe);
  InitializeCriticalSectionAndSpinCount (&cs_render_view, 0x0fd0);

  bool success =
    SUCCEEDED (CoInitializeEx (nullptr, COINIT_MULTITHREADED));

  dll_log.Log (L"[  D3D 11  ]   Hooking D3D11");

  sk_hook_d3d11_t* pHooks = 
    (sk_hook_d3d11_t *)user;

  if (pHooks->ppDevice && pHooks->ppImmediateContext)
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

    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 8, "ID3D11Device::CreateUnorderedAccessView",
                         D3D11Dev_CreateUnorderedAccessView_Override, D3D11Dev_CreateUnorderedAccessView_Original,
                         D3D11Dev_CreateUnorderedAccessView_pfn);
    
    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 9, "ID3D11Device::CreateRenderTargetView",
                           D3D11Dev_CreateRenderTargetView_Override, D3D11Dev_CreateRenderTargetView_Original,
                           D3D11Dev_CreateRenderTargetView_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 10, "ID3D11Device::CreateDepthStencilView",
                           D3D11Dev_CreateDepthStencilView_Override, D3D11Dev_CreateDepthStencilView_Original,
                           D3D11Dev_CreateDepthStencilView_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 12, "ID3D11Device::CreateVertexShader",
                         D3D11Dev_CreateVertexShader_Override, D3D11Dev_CreateVertexShader_Original,
                         D3D11Dev_CreateVertexShader_pfn);
    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 13, "ID3D11Device::CreateGeometryShader",
                         D3D11Dev_CreateGeometryShader_Override, D3D11Dev_CreateGeometryShader_Original,
                         D3D11Dev_CreateGeometryShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 14, "ID3D11Device::CreateGeometryShaderWithStreamOutput",
                           D3D11Dev_CreateGeometryShaderWithStreamOutput_Override, D3D11Dev_CreateGeometryShaderWithStreamOutput_Original,
                           D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 15, "ID3D11Device::CreatePixelShader",
                         D3D11Dev_CreatePixelShader_Override, D3D11Dev_CreatePixelShader_Original,
                         D3D11Dev_CreatePixelShader_pfn);
    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 16, "ID3D11Device::CreateHullShader",
                         D3D11Dev_CreateHullShader_Override, D3D11Dev_CreateHullShader_Original,
                         D3D11Dev_CreateHullShader_pfn);
    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 17, "ID3D11Device::CreateDomainShader",
                         D3D11Dev_CreateDomainShader_Override, D3D11Dev_CreateDomainShader_Original,
                         D3D11Dev_CreateDomainShader_pfn);
    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 18, "ID3D11Device::CreateComputeShader",
                         D3D11Dev_CreateComputeShader_Override, D3D11Dev_CreateComputeShader_Original,
                         D3D11Dev_CreateComputeShader_pfn);

    //DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 19, "ID3D11Device::CreateClassLinkage",
    //                       D3D11Dev_CreateClassLinkage_Override, D3D11Dev_CreateClassLinkage_Original,
    //                       D3D11Dev_CreateClassLinkage_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 23, "ID3D11Device::CreateSamplerState",
                         D3D11Dev_CreateSamplerState_Override, D3D11Dev_CreateSamplerState_Original,
                         D3D11Dev_CreateSamplerState_pfn);

#if 1
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

#if 0
    DXGI_VIRTUAL_OVERRIDE (pHooks->ppImmediateContext, 9, "ID3D11DeviceContext::PSSetShader",
                             D3D11_PSSetShader_Override, D3D11_PSSetShader_Original,
                             D3D11_PSSetShader_pfn);
#else
    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 9, "ID3D11DeviceContext::PSSetShader",
                         D3D11_PSSetShader_Override, D3D11_PSSetShader_Original,
                         D3D11_PSSetShader_pfn);
#endif

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 10, "ID3D11DeviceContext::PSSetSamplers",
                         D3D11_PSSetSamplers_Override, D3D11_PSSetSamplers_Original,
                         D3D11_PSSetSamplers_pfn);

#if 0
    DXGI_VIRTUAL_OVERRIDE (pHooks->ppImmediateContext, 11, "ID3D11DeviceContext::VSSetShader",
                             D3D11_VSSetShader_Override, D3D11_VSSetShader_Original,
                             D3D11_VSSetShader_pfn);
#else
    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 11, "ID3D11DeviceContext::VSSetShader",
                         D3D11_VSSetShader_Override, D3D11_VSSetShader_Original,
                         D3D11_VSSetShader_pfn);
#endif

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
    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 15, "ID3D11DeviceContext::Unmap",
                         D3D11_Unmap_Override, D3D11_Unmap_Original,
                         D3D11_Unmap_pfn);
#endif

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 16, "ID3D11DeviceContext::PSSetConstantBuffers",
                         D3D11_PSSetConstantBuffers_Override, D3D11_PSSetConstantBuffers_Original,
                         D3D11_PSSetConstantBuffers_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 20, "ID3D11DeviceContext::DrawIndexedInstanced",
                         D3D11_DrawIndexedInstanced_Override, D3D11_DrawIndexedInstanced_Original,
                         D3D11_DrawIndexedInstanced_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 21, "ID3D11DeviceContext::DrawInstanced",
                         D3D11_DrawInstanced_Override, D3D11_DrawInstanced_Original,
                         D3D11_DrawInstanced_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 23, "ID3D11DeviceContext::GSSetShader",
                         D3D11_GSSetShader_Override, D3D11_GSSetShader_Original,
                         D3D11_GSSetShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 25, "ID3D11DeviceContext::VSSetShaderResources",
                         D3D11_VSSetShaderResources_Override, D3D11_VSSetShaderResources_Original,
                         D3D11_VSSetShaderResources_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 31, "ID3D11DeviceContext::GSSetShaderResources",
                         D3D11_GSSetShaderResources_Override, D3D11_GSSetShaderResources_Original,
                         D3D11_GSSetShaderResources_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 33, "ID3D11DeviceContext::OMSetRenderTargets",
                         D3D11_OMSetRenderTargets_Override, D3D11_OMSetRenderTargets_Original,
                         D3D11_OMSetRenderTargets_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 34, "ID3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews",
                         D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Override, D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original,
                         D3D11_OMSetRenderTargetsAndUnorderedAccessViews_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 38, "ID3D11DeviceContext::DrawAuto",
                         D3D11_DrawAuto_Override, D3D11_DrawAuto_Original,
                         D3D11_DrawAuto_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 39, "ID3D11DeviceContext::DrawIndexedInstancedIndirect",
                         D3D11_DrawIndexedInstancedIndirect_Override, D3D11_DrawIndexedInstancedIndirect_Original,
                         D3D11_DrawIndexedInstancedIndirect_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 40, "ID3D11DeviceContext::DrawInstancedIndirect",
                         D3D11_DrawInstancedIndirect_Override, D3D11_DrawInstancedIndirect_Original,
                         D3D11_DrawInstancedIndirect_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 41, "ID3D11DeviceContext::Dispatch",
                         D3D11_Dispatch_Override, D3D11_Dispatch_Original,
                         D3D11_Dispatch_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 42, "ID3D11DeviceContext::DispatchIndirect",
                         D3D11_DispatchIndirect_Override, D3D11_DispatchIndirect_Original,
                         D3D11_DispatchIndirect_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 44, "ID3D11DeviceContext::RSSetViewports",
                         D3D11_RSSetViewports_Override, D3D11_RSSetViewports_Original,
                         D3D11_RSSetViewports_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 45, "ID3D11DeviceContext::RSSetScissorRects",
                         D3D11_RSSetScissorRects_Override, D3D11_RSSetScissorRects_Original,
                         D3D11_RSSetScissorRects_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 46, "ID3D11DeviceContext::CopySubresourceRegion",
                         D3D11_CopySubresourceRegion_Override, D3D11_CopySubresourceRegion_Original,
                         D3D11_CopySubresourceRegion_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 47, "ID3D11DeviceContext::CopyResource",
                         D3D11_CopyResource_Override, D3D11_CopyResource_Original,
                         D3D11_CopyResource_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 48, "ID3D11DeviceContext::UpdateSubresource",
                         D3D11_UpdateSubresource_Override, D3D11_UpdateSubresource_Original,
                         D3D11_UpdateSubresource_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 59, "ID3D11DeviceContext::HSSetShaderResources",
                         D3D11_HSSetShaderResources_Override, D3D11_HSSetShaderResources_Original,
                         D3D11_HSSetShaderResources_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 60, "ID3D11DeviceContext::HSSetShader",
                         D3D11_HSSetShader_Override, D3D11_HSSetShader_Original,
                         D3D11_HSSetShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 63, "ID3D11DeviceContext::DSSetShaderResources",
                         D3D11_DSSetShaderResources_Override, D3D11_DSSetShaderResources_Original,
                         D3D11_DSSetShaderResources_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 64, "ID3D11DeviceContext::DSSetShader",
                         D3D11_DSSetShader_Override, D3D11_DSSetShader_Original,
                         D3D11_DSSetShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 67, "ID3D11DeviceContext::CSSetShaderResources",
                         D3D11_CSSetShaderResources_Override, D3D11_CSSetShaderResources_Original,
                         D3D11_CSSetShaderResources_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 69, "ID3D11DeviceContext::CSSetShader",
                         D3D11_CSSetShader_Override, D3D11_CSSetShader_Original,
                         D3D11_CSSetShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 74, "ID3D11DeviceContext::PSGetShader",
                         D3D11_PSGetShader_Override, D3D11_PSGetShader_Original,
                         D3D11_PSGetShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 76, "ID3D11DeviceContext::VSGetShader",
                         D3D11_VSGetShader_Override, D3D11_VSGetShader_Original,
                         D3D11_VSGetShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 82, "ID3D11DeviceContext::GSGetShader",
                         D3D11_GSGetShader_Override, D3D11_GSGetShader_Original,
                         D3D11_GSGetShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 98, "ID3D11DeviceContext::HSGetShader",
                         D3D11_HSGetShader_Override, D3D11_HSGetShader_Original,
                         D3D11_HSGetShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 102, "ID3D11DeviceContext::DSGetShader",
                         D3D11_DSGetShader_Override, D3D11_DSGetShader_Original,
                         D3D11_DSGetShader_pfn);

    DXGI_VIRTUAL_HOOK (pHooks->ppImmediateContext, 107, "ID3D11DeviceContext::CSGetShader",
                         D3D11_CSGetShader_Override, D3D11_CSGetShader_Original,
                         D3D11_CSGetShader_pfn);

#endif

    if (sk::NVAPI::nv_hardware)
    {
    }

    MH_ApplyQueued ();
  }

#ifdef _WIN64
  if (config.apis.dxgi.d3d12.hook)
    HookD3D12 (nullptr);
#endif

  if (success)
    CoUninitialize ();

  return 0;
}





struct shader_disasm_s {
  std::string       header = "";
  std::string       code   = "";
  std::string       footer = "";

  struct constant_buffer
  {
    std::string      name = "";

    struct variable
    {
      std::string    name = "";
    };

    std::vector <variable> variables;

    size_t           size = 0UL;
  };

  std::vector <constant_buffer> cbuffers;
};

typedef interface ID3DXBuffer ID3DXBuffer;
typedef interface ID3DXBuffer *LPD3DXBUFFER;

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

enum class sk_shader_class {
  Unknown  = 0x00,
  Vertex   = 0x01,
  Pixel    = 0x02,
  Geometry = 0x04,
  Hull     = 0x08,
  Domain   = 0x10,
  Compute  = 0x20
};

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

#define SK_ImGui_IsItemRightClicked() ImGui::IsItemClicked (1) || (ImGui::IsItemFocused () && io.NavInputsDownDuration [ImGuiNavInput_PadActivate] > 0.4f && ((io.NavInputsDownDuration [ImGuiNavInput_PadActivate] = 0.0f) == 0.0f))

auto ShaderMenu = [&](std::unordered_set <uint32_t>& blacklist, uint32_t shader) ->
  void
  {
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
  };

std::vector <IUnknown *> temp_resources;

static size_t debug_tex_id = 0x0;
static size_t tex_dbg_idx  = 0;

void
SK_LiveTextureView (bool& can_scroll)
{
  ImGuiIO& io (ImGui::GetIO ());

  const float font_size           = ImGui::GetFont ()->FontSize * io.FontGlobalScale;
  const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  static float last_ht    = 256.0f;
  static float last_width = 256.0f;

  struct list_entry_s {
    std::string          name   = "";
    uint32_t             crc32c = 0UL;
    D3D11_TEXTURE2D_DESC desc     { };
    ID3D11Texture2D*     pTex   = nullptr;
    size_t               size   = 0;
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

  
  ImGui::Text      ("Current scene uses %5.2f MiB of texture memory", (double)total_texture_memory / (double)(1024 * 1024));
  ImGui::SameLine  ( );

  ImGui::PushItemWidth (ImGui::GetContentRegionAvailWidth () * 0.33f);
  ImGui::SliderInt     ("Frames Between Texture Refreshes", &refresh_interval, 0, 120);
  ImGui::PopItemWidth  ();

  ImGui::BeginChild (ImGui::GetID ("ToolHeadings##TexturesD3D11"), ImVec2 (font_size * 66.0f, font_size * 2.5f), false,
                     ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NavFlattened);

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

  ImGui::SameLine ();

  ImGui::PushItemWidth (font_size * strlen ("Used Textures   ") / 2);

  ImGui::Combo ("###TexturesD3D11_TextureSet", &tex_set, "All Textures\0Used Textures\0\0", 2);

  ImGui::PopItemWidth ();

  ImGui::SameLine ();

  if (ImGui::Button (" Clear Debug "))
  {
    sel                 = -1;
    debug_tex_id        =  0;
    list_contents.clear ();
    last_ht             =  0;
    last_width          =  0;
    lod                 =  0;
    tracked_texture     =  nullptr;
  }

  if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Exits texture debug mode.");

  ImGui::SameLine ();

  ImGui::Checkbox ("Highlight Selected Texture in Game##D3D11_HighlightSelectedTexture", &config.textures.d3d11.highlight_debug);

  ImGui::SameLine ();

  static bool hide_inactive = false;

  ImGui::Checkbox ("Hide Inactive Textures##D3D11_HideInactiveTextures",                 &hide_inactive);

  ImGui::Separator ();

  ImGui::EndChild  ();

  if (refresh_interval > 0 || list_dirty)
  {
    if ((LONG)SK_GetFramesDrawn () > last_frame + refresh_interval || list_dirty)
    {
      list_dirty           = true;
      last_frame           = SK_GetFramesDrawn ();
      total_texture_memory = 0ULL;
    }
  }

  if (list_dirty)
  {
    list_contents.clear ();

    if (debug_tex_id == 0)
      last_ht = 0;

    {
      SK_AutoCriticalSection critical1 (&cache_cs);

      for (auto&& it : SK_D3D11_Textures.HashMap_2D)
      {
        for (auto&& it2 : it)
        {
          if (SK_D3D11_Textures.TexRefs_2D.count (it2.second))
          {
            D3D11_TEXTURE2D_DESC desc;
            it2.second->GetDesc (&desc);

            list_entry_s entry;

            entry.crc32c = it2.first;
            entry.desc   = desc;
            entry.name   = "DontCare";
            entry.pTex   = it2.second;

            if (SK_D3D11_Textures.Textures_2D.count (it2.second))
            {
              entry.size = SK_D3D11_Textures.Textures_2D [it2.second].mem_size;
            }

            else
            {
              entry.size = 0;
            }

            if (! texture_map.count (entry.crc32c))
              texture_map.emplace (std::make_pair (entry.crc32c, entry));
          }
        }
      }

      for (auto& it : dynamic_textures2)
      {
        D3D11_TEXTURE2D_DESC desc;
        it.first->GetDesc (&desc);

        list_entry_s entry;

        entry.crc32c = it.second;
        entry.desc   = desc;
        entry.name   = "DontCare";
        entry.pTex   = it.first;

        if (! texture_map.count (entry.crc32c))
          texture_map.emplace (std::make_pair (entry.crc32c, entry));
      }

      for (auto&& it : texture_map)
      {
        bool active = used_textures.count (it.second.pTex) != 0;

        if (active || tex_set == 0)
        {
          char szDesc [48] = { };

          sprintf (szDesc, "%08x##Texture2D_D3D11", it.second.crc32c);

          list_entry_s entry;

          entry.crc32c = it.second.crc32c;
          entry.desc   = it.second.desc;
          entry.name   = szDesc;
          entry.pTex   = it.second.pTex;
          entry.size   = it.second.size;

          list_contents.emplace_back (entry);
          total_texture_memory += entry.size;
        }
      }
    }

    list_dirty = false;

    if (! SK_D3D11_Textures.TexRefs_2D.count (tracked_texture))
      tracked_texture = nullptr;
  }

  ImGui::BeginGroup ();

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  ImGui::BeginChild ( ImGui::GetID ("D3D11_TexHashes_CRC32C"),
                      ImVec2 ( font_size * 6.0f, std::max (font_size * 15.0f, last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize);

  if (ImGui::IsWindowHovered ())
    can_scroll = false;

  if (list_contents.size ())
  {
    static size_t last_sel     = std::numeric_limits <size_t>::max ();
    static bool   sel_changed  = false;
    
    if (sel != last_sel)
      sel_changed = true;
    
    last_sel = sel;
    
    for ( size_t line = 0; line < list_contents.size (); line++)
    {
      bool active = used_textures.count (list_contents [line].pTex) != 0;

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
            ImGui::SetScrollHere (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
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
      ImGui::BulletText   ("Press [ to select the previous texture from this list");
      ImGui::BulletText   ("Press ] to select the next texture from this list");
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

      while (sel >= 0 && sel < list_contents.size ())
      {
        sel += dir;

        if (hide_inactive)
        {
          bool active = used_textures.count (list_contents [sel].pTex) != 0;

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

    D3D11_TEXTURE2D_DESC tex_desc  = entry.desc;
    size_t               tex_size  = 0UL;
    float                load_time = 0.0f;


    if (lod_list_dirty)
    {
      int w = tex_desc.Width;
      int h = tex_desc.Height;

      char* pszLODList = lod_list;

      for ( UINT i = 0 ; i < tex_desc.MipLevels ; i++ )
      {
        int len = sprintf (pszLODList, "LOD%lu: (%lix%li)", i, std::max (1, w >> i), std::max (1, h >> i));
        pszLODList += (len + 1);
      }

      *pszLODList = '\0';

      lod_list_dirty = false;
    }


    SK_TLS_Push ();
    SK_TLS_Top  ()->imgui.drawing = true;

    ID3D11Texture2D* pTex = SK_D3D11_Textures.getTexture2D ((uint32_t)debug_tex_id, &tex_desc, &tex_size, &load_time);

    SK_TLS_Pop  ();

    bool staged = false;

    if (pTex == nullptr && dynamic_textures2.count (tracked_texture))
    {
      pTex      = tracked_texture;
      tex_size  = 0;
      load_time = 0.0f;
      pTex->GetDesc (&tex_desc);
      staged    = true;
    }


    if (pTex != nullptr)
    {
      ID3D11ShaderResourceView* pSRV = nullptr;

      D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

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
      };

      srv_desc.Texture2D.MipLevels       = -1;
      srv_desc.Texture2D.MostDetailedMip =  0;

      CComPtr <ID3D11Device> pDev = nullptr;

      if (SUCCEEDED (SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&pDev)))
      {
#if 0
        ImVec4 border_color = config.textures.highlight_debug_tex ? 
                                ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                  (__remap_textures && has_alternate) ? ImVec4 (0.5f,  0.5f,  0.5f, 1.0f) :
                                                                        ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#else
        ImVec4 border_color = ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#endif

        ImGui::PushStyleColor (ImGuiCol_Border, border_color);

        ImGui::BeginGroup     ();
        ImGui::BeginChild     ( ImGui::GetID ("Texture_Select_D3D11"),
                                ImVec2 ( std::max (font_size * 24.0f, (float)(tex_desc.Width >> lod) + 24.0f),
                              (float)(tex_desc.Height >> lod) + font_size * 10.0f),
                                  true,
                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NavFlattened);

        //if ((! config.textures.highlight_debug_tex) && has_alternate)
        //{
        //  if (ImGui::IsItemHovered ())
        //    ImGui::SetTooltip ("Click me to make this texture the visible version.");
        //  
        //  // Allow the user to toggle texture override by clicking the frame
        //  if (ImGui::IsItemClicked ())
        //    __remap_textures = false;
        //}

        last_width  = (float)(tex_desc.Width  >> lod);
        last_ht     = (float)(tex_desc.Height >> lod) + font_size * 10.0f;


        int num_lods = tex_desc.MipLevels;

        ImGui::BeginGroup ();
        ImGui::Text       ( "Dimensions:   " );
        ImGui::Text       ( "Format:       " );
        ImGui::Text       ( "Data Size:    " );
        ImGui::Text       ( "Load Time:    " );
        ImGui::EndGroup   ();

        ImGui::SameLine   ();

        ImGui::BeginGroup ();
        ImGui::PushItemWidth (-1);
        ImGui::Combo         ("###Texture_LOD_D3D11", &lod, lod_list, tex_desc.MipLevels);
        ImGui::PopItemWidth  (  );
        //ImGui::Text       ( "%lux%lu (%lu %s)",
                              //tex_desc.Width >> lod, tex_desc.Height >> lod,
                                //num_lods, num_lods > 1 ? "LODs" : "LOD" );
        ImGui::Text       ( "%ws",
                              SK_DXGI_FormatToStr (tex_desc.Format).c_str () );
        ImGui::Text       ( "%.2f MiB",
                              tex_size / (1024.0f * 1024.0f) );
        ImGui::Text       ( "%.6f Seconds",
                              load_time / 1000.0f );
        ImGui::EndGroup   ();

#if 0
        ImGui::SameLine   ();

        ImGui::BeginGroup ();
        {
          SK_AutoCriticalSection critical (&tex_cs);
          ImGui::Text       ( "Cache Hits:   %lu", SK_D3D11_Textures.Textures_2D [pTex].hits );
        }
        ImGui::EndGroup   ();
#endif

        ImGui::Separator  ();

        if (! SK_D3D11_IsDumped (entry.crc32c, entry.crc32c))
        {
          if ( ImGui::Button ("  Dump Texture to Disk  ###DumpTexture") )
          {
            SK_TLS_Push ();
            SK_TLS_Top  ()->imgui.drawing = true;
            {
              SK_D3D11_DumpTexture2D (pTex, entry.crc32c);
            }
            SK_TLS_Pop ();
          }
        }

        else
        {
          if ( ImGui::Button ("  Delete Dumped Texture from Disk  ###DumpTexture") )
          {
            SK_D3D11_DeleteDumpedTexture (entry.crc32c);
          }
        }

        if (staged)
        {
          ImGui::SameLine ();
          ImGui::TextColored (ImColor::HSV (0.25f, 1.0f, 1.0f), "Staged textures cannot be re-injected yet.");
        }

        ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));

        srv_desc.Texture2D.MipLevels       = 1;
        srv_desc.Texture2D.MostDetailedMip = lod;

        if (SUCCEEDED (pDev->CreateShaderResourceView (pTex, &srv_desc, &pSRV)))
        {
          ImGui::BeginChildFrame (ImGui::GetID ("TextureView_Frame"), ImVec2 ((float)(tex_desc.Width >> lod) + 8, (float)(tex_desc.Height >> lod) + 8),
                                  ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus );
          temp_resources.push_back (pSRV);
          ImGui::Image           ( pSRV,
                                     ImVec2 ((float)(tex_desc.Width >> lod), (float)(tex_desc.Height >> lod)),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (255,255,255,128)
                               );
          ImGui::EndChildFrame ();
        }
        ImGui::EndChild        ();
        ImGui::EndGroup        ();
        ImGui::PopStyleColor   (2);
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
  ImGui::EndGroup      ();
  ImGui::PopStyleColor (1);
  ImGui::PopStyleVar   (2);
}



void
SK_LiveShaderClassView (sk_shader_class shader_type, bool& can_scroll)
{
  ImGuiIO& io (ImGui::GetIO ());

  static float last_width = 256.0f;
  const  float font_size  = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  struct shader_class_imp_s
  {
    std::vector <std::string> contents;
    bool                      dirty      = true;
    uint32_t                  last_sel   =    0;
    int                            sel   =   -1;
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
    [](sk_shader_class& type) ->
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
    [](sk_shader_class& type) ->
      shader_tracking_s*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return &SK_D3D11_Shaders.vertex.tracked;
          case sk_shader_class::Pixel:    return &SK_D3D11_Shaders.pixel.tracked;
          case sk_shader_class::Geometry: return &SK_D3D11_Shaders.geometry.tracked;
          case sk_shader_class::Hull:     return &SK_D3D11_Shaders.hull.tracked;
          case sk_shader_class::Domain:   return &SK_D3D11_Shaders.domain.tracked;
          case sk_shader_class::Compute:  return &SK_D3D11_Shaders.compute.tracked;
        }

        assert (false);

        return nullptr;
      };

  shader_tracking_s*
    tracker = GetShaderTracker (shader_type);

  auto GetShaderSet =
    [](sk_shader_class& type) ->
      std::set <uint32_t>&
      {
        static std::set <uint32_t> set;
        set.clear ();

        SK_AutoCriticalSection auto_cs (&cs_shader);

        switch (type)
        {
          case sk_shader_class::Vertex:
          {
            for (auto const& vertex_shader : SK_D3D11_Shaders.vertex.descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( vertex_shader.first != 0xb42ede74 &&
                   vertex_shader.first != 0x1f8c62dc )
              {
                if (vertex_shader.first > 0x00000000) set.emplace (vertex_shader.first);
              }
            }
          } break;

          case sk_shader_class::Pixel:
          {
            for (auto const& pixel_shader : SK_D3D11_Shaders.pixel.descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( pixel_shader.first != 0xd3af3aa0 &&
                   pixel_shader.first != 0xb04a90ba )
              {
                if (pixel_shader.first > 0x00000000) set.emplace (pixel_shader.first);
              }
            }
          } break;

          case sk_shader_class::Geometry:
          {
            for (auto const& geometry_shader : SK_D3D11_Shaders.geometry.descs)
            {
              if (geometry_shader.first > 0x00000000) set.emplace (geometry_shader.first);
            }
          } break;

          case sk_shader_class::Hull:
          {
            for (auto const& hull_shader : SK_D3D11_Shaders.hull.descs)
            {
              if (hull_shader.first > 0x00000000) set.emplace (hull_shader.first);
            }
          } break;

          case sk_shader_class::Domain:
          {
            for (auto const& domain_shader : SK_D3D11_Shaders.domain.descs)
            {
              if (domain_shader.first > 0x00000000) set.emplace (domain_shader.first);
            }
          } break;

          case sk_shader_class::Compute:
          {
            for (auto const& compute_shader : SK_D3D11_Shaders.compute.descs)
            {
              if (compute_shader.first > 0x00000000) set.emplace (compute_shader.first);
            }
          } break;
        }

        return set;
      };

  std::set <uint32_t>& set =
    GetShaderSet (shader_type);

  std::vector <uint32_t>
    shaders ( set.begin (), set.end () );

  auto GetShaderDisasm =
    [](sk_shader_class& type) ->
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
    [](sk_shader_class& type) ->
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

  auto GetShaderChange =
    [&](sk_shader_class& type) ->
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
    [&](sk_shader_class& type)->
      std::unordered_set <uint32_t>&
      {
        static std::unordered_set <uint32_t> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return SK_D3D11_Shaders.vertex.blacklist;
          case sk_shader_class::Pixel:    return SK_D3D11_Shaders.pixel.blacklist;
          case sk_shader_class::Geometry: return SK_D3D11_Shaders.geometry.blacklist;
          case sk_shader_class::Hull:     return SK_D3D11_Shaders.hull.blacklist;
          case sk_shader_class::Domain:   return SK_D3D11_Shaders.domain.blacklist;
          case sk_shader_class::Compute:  return SK_D3D11_Shaders.compute.blacklist;
          default:                        return invalid;
        }
     };

  if (list->dirty)
  {
        list->sel = -1;
    int idx       =  0;
        list->contents.clear ();

    for ( auto it : shaders )
    {
      char szDesc [32] = { };

      const bool disabled = GetShaderBlacklist (shader_type).count (it) != 0;

      sprintf (szDesc, "%s%08lx##%s", disabled ? "*" : " ", it, GetShaderWord (shader_type));

      list->contents.emplace_back (szDesc);

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
  }

  bool scrolled = false;

  int dir = 0;

  if (ImGui::IsMouseHoveringRect (list->last_min, list->last_max))
  {
         if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { dir = -1;  io.WantCaptureKeyboard = true; scrolled = true; }
    else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { dir = +1;  io.WantCaptureKeyboard = true; scrolled = true; }
  }

  struct sk_shader_state_s {
    int  last_sel      = 0;
    bool sel_changed   = false;
    bool hide_inactive = false;
    int  active_frames = 2;
    bool hovering      = false;
    bool focused       = false;

    static int ClassToIdx (sk_shader_class& shader_class)
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

  int&  last_sel      =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].last_sel;
  bool& sel_changed   =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].sel_changed;
  bool* hide_inactive = &shader_state [sk_shader_state_s::ClassToIdx (shader_type)].hide_inactive;
  int&  active_frames =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].active_frames;

  ImGui::Checkbox (SK_FormatString ("Hide Inactive Shaders##%s", GetShaderWord (shader_type)).c_str (), hide_inactive);

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  bool& hovering = shader_state [sk_shader_state_s::ClassToIdx (shader_type)].hovering;
  bool& focused  = shader_state [sk_shader_state_s::ClassToIdx (shader_type)].focused;

  ImGui::BeginChild ( ImGui::GetID (szShaderWord),
                      ImVec2 ( font_size * 7.0f, std::max (font_size * 15.0f, list->last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize );

  if (hovering || focused)
  {
    can_scroll = false;

    if (! focused)
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected %s shader to disable an effect", szShaderWord);
      ImGui::Separator    ();
      ImGui::BulletText   ("Press [ while the mouse is hovering this list to select the previous shader");
      ImGui::BulletText   ("Press ] while the mouse is hovering this list to select the next shader");
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

  if (shaders.size ())
  {
    auto ShaderBase = [](sk_shader_class& shader_class) ->
      void*
      {
        switch (shader_class)
        {
          case sk_shader_class::Vertex:   return &SK_D3D11_Shaders.vertex;
          case sk_shader_class::Pixel:    return &SK_D3D11_Shaders.pixel;
          case sk_shader_class::Geometry: return &SK_D3D11_Shaders.geometry;
          case sk_shader_class::Hull:     return &SK_D3D11_Shaders.hull;
          case sk_shader_class::Domain:   return &SK_D3D11_Shaders.domain;
          case sk_shader_class::Compute:  return &SK_D3D11_Shaders.compute;
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

    // User wants to cycle list elements, we know only the direction, not how many indices in the list
    //   we need to increment to get an unfiltered list item.
    if (dir != 0)
    {
      do
      {
        list->sel += dir;

        if (*hide_inactive)
        {
          SK_D3D11_ShaderDesc& rDesc =
            GetShaderDesc (shader_type, (uint32_t)shaders [list->sel]);

          if (rDesc.usage.last_frame <= SK_GetFramesDrawn () - active_frames)
            continue;
        }

        break;
      } while (list->sel > 0 && (unsigned)list->sel < shaders.size () - 1);
    }


    if (list->sel != last_sel)
      sel_changed = true;

    last_sel = list->sel;

    auto ChangeSelectedShader = []( shader_class_imp_s*  list,
                                    shader_tracking_s*   tracker,
                                    SK_D3D11_ShaderDesc& rDesc ) ->
      void
        {
          list->last_sel           = rDesc.crc32c;
          tracker->crc32c          = rDesc.crc32c;
          tracker->runtime_ms      = 0.0;
          tracker->last_runtime_ms = 0.0;
          tracker->runtime_ticks   = 0ULL;
        };

    for ( UINT line = 0; line < shaders.size (); line++ )
    {
      SK_D3D11_ShaderDesc& rDesc =
        GetShaderDesc (shader_type, (uint32_t)shaders [line]);

      bool active = rDesc.usage.last_frame > SK_GetFramesDrawn () - active_frames;

      if (active)
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
      else
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.425f, 0.425f, 0.425f, 0.9f));

      if (line == list->sel)
      {
        bool selected = true;

        if (active || (! *hide_inactive))
          ImGui::Selectable (list->contents [line].c_str (), &selected);

        if (sel_changed)
        {
          ImGui::SetScrollHere (0.5f);

          sel_changed = false;

          ChangeSelectedShader (list, tracker, rDesc);
        }

        if (SK_ImGui_IsItemRightClicked ())
        {
          ImGui::SetNextWindowSize          (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
          ImGui::OpenPopup (SK_FormatString ("ShaderSubMenu_%s%08lx", GetShaderWord (shader_type), (uint32_t)shaders [line]).c_str ());
        }

        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_%s%08lx", GetShaderWord (shader_type), (uint32_t)shaders [line]).c_str ()))
        {
          ShaderMenu (GetShaderBlacklist (shader_type), (uint32_t)shaders [line]);
          ImGui::EndPopup ();
        }
      }

      else
      {
        bool selected    = false;

        if (active || (! *hide_inactive))
        {
          if (ImGui::Selectable (list->contents [line].c_str (), &selected))
          {
            sel_changed = true;
            list->sel   =  line;

            ChangeSelectedShader (list, tracker, rDesc);
          }
        }
      }

      ImGui::PopStyleColor ();
    }

    CComPtr <ID3DBlob>               pDisasm  = nullptr;
    CComPtr <ID3D11ShaderReflection> pReflect = nullptr;

    HRESULT hr = E_FAIL;

    if (tracker->crc32c != 0)
    {
      SK_AutoCriticalSection auto_cs (&cs_shader);

      switch (shader_type)
      {
        case sk_shader_class::Vertex:
            hr = D3DDisassemble ( SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.size (),
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
          break;

        case sk_shader_class::Pixel:
            hr = D3DDisassemble ( SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.size (),
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
          break;

        case sk_shader_class::Geometry:
            hr = D3DDisassemble ( SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
          break;

        case sk_shader_class::Hull:
            hr = D3DDisassemble ( SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
          break;

        case sk_shader_class::Domain:
            hr = D3DDisassemble ( SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
          break;

        case sk_shader_class::Compute:
            hr = D3DDisassemble ( SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
          break;
      }

      if (SUCCEEDED (hr) && strlen ((const char *)pDisasm->GetBufferPointer ()))
      {
        char* szDisasm      = _strdup ((const char *)pDisasm->GetBufferPointer ());
        char* comments_end  = nullptr;

        if (szDisasm && strlen (szDisasm))
        {
          comments_end = strstr (szDisasm,          "\nvs");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\nps");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\ngs");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\nhs");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\nds");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\ncs");
          char* footer_begins = comments_end ? strstr (comments_end + 1, "\n//") : nullptr;

          if (comments_end)  *comments_end  = '\0'; else (comments_end  = "  ");
          if (footer_begins) *footer_begins = '\0'; else (footer_begins = "  ");


          disassembly->emplace ( tracker->crc32c, shader_disasm_s {
                                                    szDisasm,
                                                      comments_end + 1,
                                                        footer_begins + 1
                                                  }
                              );


          D3D11_SHADER_DESC desc = { };

          if (pReflect)
          {
            pReflect->GetDesc (&desc);

            for (UINT i = 0; i < desc.ConstantBuffers; i++)
            {
              ID3D11ShaderReflectionConstantBuffer* pReflectedCBuffer =
                pReflect->GetConstantBufferByIndex (i);

              if (pReflectedCBuffer)
              {
                D3D11_SHADER_BUFFER_DESC buffer_desc;

                if (SUCCEEDED (pReflectedCBuffer->GetDesc (&buffer_desc)))
                {
                  if (buffer_desc.Type == D3D11_CT_CBUFFER)
                  {
                    shader_disasm_s::constant_buffer cbuffer;
                    cbuffer.name = buffer_desc.Name;
                    cbuffer.size = buffer_desc.Size;

                    for (UINT j = 0; j < buffer_desc.Variables; j++)
                    {
                      ID3D11ShaderReflectionVariable* pReflectedVariable =
                        pReflectedCBuffer->GetVariableByIndex (j);

                      if (pReflectedVariable)
                      {
                        D3D11_SHADER_VARIABLE_DESC var_desc;

                        if (SUCCEEDED (pReflectedVariable->GetDesc (&var_desc)))
                        {
                          shader_disasm_s::constant_buffer::variable var;
                          var.name = var_desc.Name;

                          cbuffer.variables.emplace_back (var);
                        }
                      }
                    }

                    (*disassembly) [tracker->crc32c].cbuffers.emplace_back (cbuffer);
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

  if (tracker->crc32c != 0x00 && list->sel >= 0 && list->sel < (int)list->contents.size ())
  {
    ImGui::BeginGroup ();
    switch (shader_type)
    {
      case sk_shader_class::Vertex:   ImGui::Checkbox ( "Cancel Draws Using Selected Vertex Shader",       &tracker->cancel_draws ); break;
      case sk_shader_class::Pixel:    ImGui::Checkbox ( "Cancel Draws Using Selected Pixel Shader",        &tracker->cancel_draws ); break;
      case sk_shader_class::Geometry: ImGui::Checkbox ( "Cancel Draws Using Selected Geometry Shader",     &tracker->cancel_draws ); break;
      case sk_shader_class::Hull:     ImGui::Checkbox ( "Cancel Draws Using Selected Hull Shader",         &tracker->cancel_draws ); break;
      case sk_shader_class::Domain:   ImGui::Checkbox ( "Cancel Draws Using Selected Domain Shader",       &tracker->cancel_draws ); break;
      case sk_shader_class::Compute:  ImGui::Checkbox ( "Cancel Dispatches Using Selected Compute Shader", &tracker->cancel_draws ); break;
    }
    ImGui::SameLine ( );


    int used_textures = 0;

    if (tracker->used_views.size ())
    {
      for ( auto it : tracker->used_views )
      {
        D3D11_SHADER_RESOURCE_VIEW_DESC rsv_desc;

        it->GetDesc (&rsv_desc);

        if (rsv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
        {
          ++used_textures;
        }
      }
    }


    ImGui::BeginGroup ();

    if (shader_type != sk_shader_class::Compute)
    {
      if (tracker->cancel_draws)
        ImGui::TextDisabled ("%3lu Skipped Draw%sLast Frame (%#lu textures)", tracker->num_draws, tracker->num_draws != 1 ? "s " : " ", used_textures );
      else
        ImGui::TextDisabled ("%3lu Draw%sLast Frame         (%#lu textures)", tracker->num_draws, tracker->num_draws != 1 ? "s " : " ", used_textures );
    }

    else
    {
      if (tracker->cancel_draws)
        ImGui::TextDisabled ("%3lu Skipped Dispatch%sLast Frame (%#lu textures)", tracker->num_draws, tracker->num_draws != 1 ? "es " : " ", used_textures );
      else
        ImGui::TextDisabled ("%3lu Dispatch%sLast Frame         (%#lu textures)", tracker->num_draws, tracker->num_draws != 1 ? "es " : " ", used_textures );
    }

    ImGui::TextDisabled ("GPU Runtime: %0.4f ms", tracker->runtime_ms);

    ImGui::EndGroup ();

    ImGui::Separator      ();
    ImGui::EndGroup       ();

    if (ImGui::IsItemHoveredRect () && tracker->used_views.size ())
    {
      ImGui::BeginTooltip ();
    
      DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
    
      for ( auto it : tracker->used_views )
      {
        D3D11_SHADER_RESOURCE_VIEW_DESC rsv_desc;

        it->GetDesc (&rsv_desc);

        if (rsv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
        {
          CComPtr <ID3D11Resource>  pRes = nullptr;
          CComPtr <ID3D11Texture2D> pTex = nullptr;

          it->GetResource (&pRes);

          if (pRes && SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)) && pTex)
          {
            D3D11_TEXTURE2D_DESC desc;

            pTex->GetDesc (&desc);

            fmt = desc.Format;

            if (desc.Height > 0 && desc.Width > 0)
            {
              ImGui::Image ( it,         ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
        ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                         ImVec2  (0,0),             ImVec2  (1,1),
                                         ImColor (255,255,255,255), ImColor (242,242,13,255) );
            }

            ImGui::SameLine ( );

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

    ImGui::PushFont (io.Fonts->Fonts [1]); // Fixed-width font

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.80f, 0.80f, 1.0f, 1.0f));
    ImGui::TextWrapped    ((*disassembly) [tracker->crc32c].header.c_str ());
    ImGui::PopStyleColor  ();
    
    ImGui::SameLine   ();
    ImGui::BeginGroup ();

    ImGui::TreePush       ("");
    ImGui::Spacing        (); ImGui::Spacing ();

    for (auto&& it : (*disassembly) [tracker->crc32c].cbuffers)
    {
      for (auto&& it2 : it.variables)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.1f, 0.7f, 1.0f));
        ImGui::Text           (it2.name.c_str ());
        ImGui::PopStyleColor  ();
      }
    }

    ImGui::TreePop    ();
    ImGui::EndGroup   ();
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
    ImGui::TextWrapped    ((*disassembly) [tracker->crc32c].code.c_str ());
    
    ImGui::Separator      ();
    
    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.5f, 0.95f, 0.5f, 1.0f));
    ImGui::TextWrapped    ((*disassembly) [tracker->crc32c].footer.c_str ());

    ImGui::PopFont        ();

    ImGui::PopStyleColor (2);
  }
  else
    tracker->cancel_draws = false;

  ImGui::EndGroup ();

  list->last_ht    = ImGui::GetItemRectSize ().y;

  list->last_min   = ImGui::GetItemRectMin ();
  list->last_max   = ImGui::GetItemRectMax ();
}


void
SK_D3D11_EndFrame (void)
{
  SK_D3D11_MemoryThreads.clear_active ();
  SK_D3D11_ShaderThreads.clear_active ();
  SK_D3D11_DrawThreads.clear_active   ();

  SK_D3D11_Shaders.vertex.tracked.deactivate   (); SK_D3D11_Shaders.vertex.current   = 0x0;
  SK_D3D11_Shaders.pixel.tracked.deactivate    (); SK_D3D11_Shaders.pixel.current    = 0x0;
  SK_D3D11_Shaders.geometry.tracked.deactivate (); SK_D3D11_Shaders.geometry.current = 0x0;
  SK_D3D11_Shaders.hull.tracked.deactivate     (); SK_D3D11_Shaders.hull.current     = 0x0;
  SK_D3D11_Shaders.domain.tracked.deactivate   (); SK_D3D11_Shaders.domain.current   = 0x0;
  SK_D3D11_Shaders.compute.tracked.deactivate  (); SK_D3D11_Shaders.compute.current  = 0x0;

  tracked_rtv.clear   ();
  used_textures.clear ();

  mem_map_stats.clear ();

  // True if the disjoint query is complete and we can get the results of
  //   each tracked shader's timing
  static bool disjoint_done = false;

  CComPtr <ID3D11Device>        dev     = nullptr;
  CComPtr <ID3D11DeviceContext> dev_ctx = nullptr;

  if (SK_GetCurrentRenderBackend ().device && SUCCEEDED (SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&dev)))
  {
    dev->GetImmediateContext (&dev_ctx);
  }

  if (! dev_ctx)
    disjoint_done = true;

  // End the Query and probe results (when the pipeline has drained)
  if (dev_ctx && (! disjoint_done) && shader_tracking_s::disjoint_query.async)
  {
    if (dev_ctx != nullptr)
    {
      if (shader_tracking_s::disjoint_query.active)
      {
        dev_ctx->End (shader_tracking_s::disjoint_query.async);
        shader_tracking_s::disjoint_query.active = false;
      }

      else
      {
        HRESULT const hr =
            dev_ctx->GetData ( shader_tracking_s::disjoint_query.async,
                                &shader_tracking_s::disjoint_query.last_results,
                                  sizeof D3D11_QUERY_DATA_TIMESTAMP_DISJOINT,
                                    0x0 );

        if (hr == S_OK)
        {
          shader_tracking_s::disjoint_query.async->Release ();
          shader_tracking_s::disjoint_query.async = nullptr;

          // Check for failure, if so, toss out the results.
          if (! shader_tracking_s::disjoint_query.last_results.Disjoint)
            disjoint_done = true;
          else
          {
            auto ClearTimers = [](shader_tracking_s* tracker) ->
              void
              {
                for (auto& it : tracker->timers)
                {
                  if (it.start.async) {
                    it.start.async->Release ();
                    it.start.async = nullptr;
                  }
                  if (it.end.async)
                  {
                    it.end.async->Release ();
                    it.end.async = nullptr;
                  }
                }

                tracker->timers.clear ();
              };

            ClearTimers (&SK_D3D11_Shaders.vertex.tracked);
            ClearTimers (&SK_D3D11_Shaders.pixel.tracked);
            ClearTimers (&SK_D3D11_Shaders.geometry.tracked);
            ClearTimers (&SK_D3D11_Shaders.hull.tracked);
            ClearTimers (&SK_D3D11_Shaders.domain.tracked);
            ClearTimers (&SK_D3D11_Shaders.compute.tracked);

            disjoint_done = true;
          }
        }
      }
    }
  }

  if (dev_ctx && disjoint_done)
  {
    auto GetTimerDataStart = [](ID3D11DeviceContext* dev_ctx, shader_tracking_s::duration_s* duration, bool& success) ->
      UINT64
      {
        if (! FAILED (dev_ctx->GetData ( duration->start.async, &duration->start.last_results, sizeof UINT64, 0x00 )))
        {
          duration->start.async->Release ();
          duration->start.async = nullptr;

          success = true;
          
          return duration->start.last_results;
        }

        success = false;

        return 0;
      };

    auto GetTimerDataEnd = [](ID3D11DeviceContext* dev_ctx, shader_tracking_s::duration_s* duration, bool& success) ->
      UINT64
      {
        if (duration->end.async == nullptr)
          return duration->start.last_results;

        if (! FAILED (dev_ctx->GetData ( duration->end.async, &duration->end.last_results, sizeof UINT64, 0x00 )))
        {
          duration->end.async->Release ();
          duration->end.async = nullptr;

          success = true;

          return duration->end.last_results;
        }

        success = false;

        return 0;
      };

    auto CalcRuntimeMS = [](shader_tracking_s* tracker) ->
      void
      {
        if (tracker->runtime_ticks != 0)
        {
          tracker->runtime_ms = 1000.0 * ((double)tracker->runtime_ticks / (double)tracker->disjoint_query.last_results.Frequency);

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

    auto AccumulateRuntimeTicks = [&](ID3D11DeviceContext* dev_ctx, shader_tracking_s* tracker, std::unordered_set <uint32_t>& blacklist) ->
      void
      {
        std::vector <shader_tracking_s::duration_s> rejects;

        tracker->runtime_ticks = 0;

        for (auto& it : tracker->timers)
        {
          bool   success0 = false, success1 = false;
          UINT64 time0    = 0ULL,  time1    = 0ULL;

          time0 = GetTimerDataEnd   (dev_ctx, &it, success0);
          time1 = GetTimerDataStart (dev_ctx, &it, success1);

          if (success0 && success1)
            tracker->runtime_ticks += time0 - time1;
          else
            rejects.push_back (it);
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

    AccumulateRuntimeTicks (dev_ctx, &SK_D3D11_Shaders.vertex.tracked,   SK_D3D11_Shaders.vertex.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.vertex.tracked);

    AccumulateRuntimeTicks (dev_ctx, &SK_D3D11_Shaders.pixel.tracked,    SK_D3D11_Shaders.pixel.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.pixel.tracked);

    AccumulateRuntimeTicks (dev_ctx, &SK_D3D11_Shaders.geometry.tracked, SK_D3D11_Shaders.geometry.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.geometry.tracked);

    AccumulateRuntimeTicks (dev_ctx, &SK_D3D11_Shaders.hull.tracked,     SK_D3D11_Shaders.hull.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.hull.tracked);

    AccumulateRuntimeTicks (dev_ctx, &SK_D3D11_Shaders.domain.tracked,   SK_D3D11_Shaders.domain.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.domain.tracked);

    AccumulateRuntimeTicks (dev_ctx, &SK_D3D11_Shaders.compute.tracked,  SK_D3D11_Shaders.compute.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.compute.tracked);

    disjoint_done = false;
  }

  SK_D3D11_Shaders.vertex.tracked.clear   ();
  SK_D3D11_Shaders.pixel.tracked.clear    ();
  SK_D3D11_Shaders.geometry.tracked.clear ();
  SK_D3D11_Shaders.hull.tracked.clear     ();
  SK_D3D11_Shaders.domain.tracked.clear   ();
  SK_D3D11_Shaders.compute.tracked.clear  ();

  SK_D3D11_Shaders.vertex.changes_last_frame   = 0;
  SK_D3D11_Shaders.pixel.changes_last_frame    = 0;
  SK_D3D11_Shaders.geometry.changes_last_frame = 0;
  SK_D3D11_Shaders.hull.changes_last_frame     = 0;
  SK_D3D11_Shaders.domain.changes_last_frame   = 0;
  SK_D3D11_Shaders.compute.changes_last_frame  = 0;

  for (auto it : temp_resources)
    it->Release ();

  temp_resources.clear ();

  SK_D3D11_RenderTargets.clear ();
}


bool
SK_D3D11_ShaderModDlg (void)
{
  ImGuiIO& io (ImGui::GetIO ());

  SK_AutoCriticalSection auto_cs (&cs_shader);

  const float font_size           = ImGui::GetFont ()->FontSize * io.FontGlobalScale;
  const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  bool show_dlg = true;

  ImGui::SetNextWindowSize ( ImVec2 ( io.DisplaySize.x * 0.66f, io.DisplaySize.y * 0.42f ), ImGuiSetCond_Appearing);

  ImGui::SetNextWindowSizeConstraints ( /*ImVec2 (768.0f, 384.0f),*/
                                        ImVec2 ( io.DisplaySize.x * 0.16f, io.DisplaySize.y * 0.16f ),
                                        ImVec2 ( io.DisplaySize.x * 0.96f, io.DisplaySize.y * 0.96f ) );

  if ( ImGui::Begin ( SK_FormatString ( "D3D11 Render Mod Toolkit     (%lu/%lu Memory Mgmt Threads,  %lu/%lu Shader Mgmt Threads,  %lu/%lu Draw Threads)###D3D11_RenderDebug",
                                          SK_D3D11_MemoryThreads.count_active   (), SK_D3D11_MemoryThreads.count_all (),
                                            SK_D3D11_ShaderThreads.count_active (), SK_D3D11_ShaderThreads.count_all (),
                                              SK_D3D11_DrawThreads.count_active (), SK_D3D11_DrawThreads.count_all   () ).c_str (),
                        &show_dlg,
                          ImGuiWindowFlags_ShowBorders ) )
  {
    SK_D3D11_EnableTracking = true;

    bool can_scroll = ImGui::IsWindowFocused () && ImGui::IsMouseHoveringRect ( ImVec2 (ImGui::GetWindowPos ().x,                             ImGui::GetWindowPos ().y),
                                                                                ImVec2 (ImGui::GetWindowPos ().x + ImGui::GetWindowSize ().x, ImGui::GetWindowPos ().y + ImGui::GetWindowSize ().y) );

    ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

    ImGui::Columns (2);

    ImGui::BeginChild ( ImGui::GetID ("Render_Left_Side"), ImVec2 (0,0), false, ImGuiWindowFlags_NavFlattened  );

    if (ImGui::CollapsingHeader ("Live Shader View", ImGuiTreeNodeFlags_DefaultOpen))
    {
      SK_AutoCriticalSection auto_cs2 (&cs_render_view);

      SK_D3D11_UpdateRenderStatsEx ((IDXGISwapChain *)SK_GetCurrentRenderBackend ().swapchain);

      ImGui::TreePush ("");

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
                                            used_last_frame   = SK_D3D11_Shaders.vertex.changes_last_frame > 0;
                                            //SK_D3D11_GetVertexPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Vertex Shaders\t\t%ws###LiveVertexShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Pixel:    ui_link_activated = change_sel_ps != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.pixel.changes_last_frame > 0;
                                            //SK_D3D11_GetRasterPipelineDesc (wszPipelineDesc);
                                            //lstrcatW                       (wszPipelineDesc, L"\t\t");
                                            //SK_D3D11_GetPixelPipelineDesc  (wszPipelineDesc);
                                            sprintf (label,     "Pixel Shaders\t\t%ws###LivePixelShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Geometry: ui_link_activated = change_sel_gs != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.geometry.changes_last_frame > 0;
                                            //SK_D3D11_GetGeometryPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Geometry Shaders\t\t%ws###LiveGeometryShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Hull:     ui_link_activated = change_sel_hs != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.hull.changes_last_frame > 0;
                                            //SK_D3D11_GetTessellationPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Hull Shaders\t\t%ws###LiveHullShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Domain:   ui_link_activated = change_sel_ds != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.domain.changes_last_frame > 0;
                                            //SK_D3D11_GetTessellationPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Domain Shaders\t\t%ws###LiveDomainShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Compute:  ui_link_activated = change_sel_cs != 0x00;
                                            used_last_frame   = SK_D3D11_Shaders.compute.changes_last_frame > 0;
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

            if (ImGui::CollapsingHeader (label))
              SK_LiveShaderClassView (shader_type, can_scroll);
          }
        };

        ImGui::TreePush    ("");
        ImGui::PushFont    (io.Fonts->Fonts [1]); // Fixed-width font
        ImGui::TextColored (ImColor (238, 250, 5), "%ws", SK::DXGI::getPipelineStatsDesc ().c_str ());
        ImGui::PopFont     ();
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

          const NUMBERFMTA fmt = { 0, 0, 3, ".", ",", 0 };

          snprintf (szNumber, 15, "%li", num);

          GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
                               0x00,
                                 szNumber, &fmt,
                                   szPrettyNumber, 32 );

          return szPrettyNumber;
        };

      if (ImGui::CollapsingHeader ("Live Memory View", ImGuiTreeNodeFlags_DefaultOpen))
      {
        //ImGui::Checkbox ("High Quality Volumetric Lighting", &SK_D3D11_Hacks.hq_volumetric);

        SK_AutoCriticalSection auto_cs3 (&cs_mmio);

        ImGui::BeginChild ( ImGui::GetID ("Render_MemStats_D3D11"), ImVec2 (0, 0), false, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNavInputs );

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

      ImGui::EndChild   ();

      ImGui::NextColumn ();

      ImGui::BeginChild ( ImGui::GetID ("Render_Right_Side"), ImVec2 (0, 0), false, ImGuiWindowFlags_NavFlattened );

      if (ImGui::CollapsingHeader ("Live Texture View", ImGuiTreeNodeFlags_DefaultOpen))
      {
        SK_AutoCriticalSection auto_cs2 (&cs_render_view);

        SK_LiveTextureView (can_scroll);
      }

      if (ImGui::CollapsingHeader ("Live RenderTarget View", ImGuiTreeNodeFlags_DefaultOpen))
      {
        SK_AutoCriticalSection auto_cs2 (&cs_render_view, true);

        if (auto_cs2.try_result ())
        {
        static float last_ht    = 256.0f;
        static float last_width = 256.0f;

        static std::vector <std::string> list_contents;
        static bool                      list_dirty     = true;
        static uintptr_t                 last_sel_ptr   =    0;
        static size_t                    sel            = std::numeric_limits <size_t>::max ();
        static bool                       first_frame   = true;

        std::set <ID3D11RenderTargetView *> live_textures;

        struct lifetime
        {
          ULONG last_frame;
          ULONG frames_active;
        };

        static std::unordered_map <ID3D11RenderTargetView *, lifetime> render_lifetime;
        static std::vector        <ID3D11RenderTargetView *>           render_textures;

        //render_textures.reserve (128);
        //render_textures.clear   ();

        for (auto it : SK_D3D11_RenderTargets.rt_views)
        {
          D3D11_RENDER_TARGET_VIEW_DESC desc;
          it->GetDesc (&desc);

          if (desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
          {
            CComPtr <ID3D11Texture2D> pTex = nullptr;
            CComPtr <ID3D11Resource>  pRes = nullptr;

            it->GetResource (&pRes);

            if (pRes && SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)) && pTex)
            {
              D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

              srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              srv_desc.Format                    = desc.Format;
              srv_desc.Texture2D.MipLevels       = desc.Texture2D.MipSlice + 1;
              srv_desc.Texture2D.MostDetailedMip = desc.Texture2D.MipSlice;

              CComPtr <ID3D11Device> pDev = nullptr;

              if (SUCCEEDED (SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&pDev)))
              {
                if (! render_lifetime.count (it))
                {
                  lifetime life;

                  life.frames_active = 1;
                  life.last_frame    = SK_GetFramesDrawn ();

                  render_textures.push_back (it);
                  render_lifetime.emplace   (std::make_pair (it, life));
                }

                else {
                  render_lifetime [it].frames_active++;
                  render_lifetime [it].last_frame = SK_GetFramesDrawn ();
                }

                live_textures.emplace (it);
              }
            }
          }
        }

        const ULONG      zombie_threshold = 120;
        static ULONG last_zombie_pass      = SK_GetFramesDrawn ();

        if (last_zombie_pass < SK_GetFramesDrawn () - zombie_threshold / 2)
        {
          bool newly_dead = false;

          for (auto it : render_textures)
          {
            if (render_lifetime [it].last_frame < SK_GetFramesDrawn () - zombie_threshold)
            {
              render_lifetime.erase (it);
              newly_dead = true;
            }
          }

          if (newly_dead)
          {
            render_textures.clear ();

            for (auto& it : render_lifetime)
              render_textures.push_back (it.first);
          }

          last_zombie_pass = SK_GetFramesDrawn ();
        }


        if (list_dirty)
        {
              sel = -1;
          int idx =  0;
              list_contents.clear ();

          // The underlying list is unsorted for speed, but that's not at all
          //   intuitive to humans, so sort the thing when we have the RT view open.
          std::sort ( render_textures.begin (),
                      render_textures.end   (),
            []( ID3D11RenderTargetView *a,
                ID3D11RenderTargetView *b )
            {
              return (uintptr_t)a < (uintptr_t)b;
            }
          );


          for ( auto it : render_textures )
          {
            static char szDesc [32] = { };

#ifdef _WIN64
            sprintf (szDesc, "%07llx###rtv_%p", (uintptr_t)it, it);
#else
            sprintf (szDesc, "%07lx###rtv_%p", (uintptr_t)it, it);
#endif

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
  
          if (render_textures.size ())
          {
            if (! focused)//hovered)
            {
              ImGui::BeginTooltip ();
              ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can view the output of individual render passes");
              ImGui::Separator    ();
              ImGui::BulletText   ("Press [ while the mouse is hovering this list to select the previous output");
              ImGui::BulletText   ("Press ] while the mouse is hovering this list to select the next output");
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
                              true, ImGuiWindowFlags_AlwaysAutoResize);
  
        if (render_textures.size ())
        {
          ImGui::BeginGroup ();
  
          if (first_frame)
          {
            sel         = 0;
            first_frame = false;
          }
  
          static      int last_sel = 0;
          static bool sel_changed  = false;
  
          if (sel >= 0 && sel < (int)render_textures.size ())
          {
            if (last_sel_ptr != (uintptr_t)render_textures [sel])
              sel_changed = true;
          }
  
          for ( int line = 0; line < (int)render_textures.size (); line++ )
          {      
            if (line == sel)
            {
              bool selected = true;
              ImGui::Selectable (list_contents [line].c_str (), &selected);
  
              if (sel_changed)
              {
                ImGui::SetScrollHere (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
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
  
          D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
          rt_view->GetDesc (&rtv_desc);
  
          if (rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
          {
            CComPtr <ID3D11Texture2D> pTex = nullptr;
            CComPtr <ID3D11Resource>  pRes = nullptr;
  
            rt_view->GetResource (&pRes);
  
            if (pRes && SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)) && pTex)
            {
              D3D11_TEXTURE2D_DESC desc;
              pTex->GetDesc (&desc);
  
              ID3D11ShaderResourceView* pSRV = nullptr;
  
              D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };
  
              srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              srv_desc.Format                    = rtv_desc.Format;
              srv_desc.Texture2D.MipLevels       = UINT_MAX;
              srv_desc.Texture2D.MostDetailedMip =  0;
  
              CComPtr <ID3D11Device> pDev = nullptr;
  
              if (SUCCEEDED (SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&pDev)))
              {
                // Some Render Targets are MASSIVE, let's try to keep the damn things on the screen ;)
                float effective_width  = std::min (0.75f * io.DisplaySize.x, (float)desc.Width  / 2.0f);
                float effective_height = std::min (0.75f * io.DisplaySize.y, (float)desc.Height / 2.0f);
  
                bool success =
                  SUCCEEDED (pDev->CreateShaderResourceView (pTex, &srv_desc, &pSRV));
  
                if (! success)
                {
                  effective_width  = 0;
                  effective_height = 0;
                }
  
                ImGui::SameLine ();
  
                size_t shaders = std::min ((size_t)12, std::max (tracked_rtv.ref_vs.size (), tracked_rtv.ref_ps.size ()) +
                                           tracked_rtv.ref_gs.size () +
                                 std::max (tracked_rtv.ref_hs.size (), tracked_rtv.ref_ds.size ()) +
                                           tracked_rtv.ref_cs.size ());
  
                ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::BeginChild      ( ImGui::GetID ("RenderTargetPreview"),
                                         ImVec2 ( std::max (font_size * 30.0f, effective_width  + 24.0f),
                                                  -1.0f ),
                                           true,
                                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs );
  
                last_width  = effective_width;
                last_ht     = effective_height + font_size * 4.0f + (float)shaders * font_size;
  
                ImGui::BeginGroup (                  );
                ImGui::Text       ( "Dimensions:   " );
                ImGui::Text       ( "Format:       " );
                ImGui::EndGroup   (                  );
  
                ImGui::SameLine   ( );
  
                ImGui::BeginGroup (                                              );
                ImGui::Text       ( "%lux%lu",
                                      desc.Width, desc.Height/*,
                                        pTex->d3d9_tex->GetLevelCount ()*/       );
                ImGui::Text       ( "%ws",
                                      SK_DXGI_FormatToStr (desc.Format).c_str () );
                ImGui::EndGroup   (                                              );
    
                if (success)
                {
                  ImGui::Separator  ( );

                  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
                  ImGui::BeginChildFrame   (ImGui::GetID ("ShaderResourceView_Frame"),
                                              ImVec2 (effective_width + 8.0f, effective_height + 8.0f), ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoInputs    |
                                                                                                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs |
                                                                                                        ImGuiWindowFlags_NoNavFocus);

                  temp_resources.push_back (pSRV);
                  ImGui::Image             ( pSRV,
                                               ImVec2 (effective_width, effective_height),
                                                 ImVec2  (0,0),             ImVec2  (1,1),
                                                 ImColor (255,255,255,255), ImColor (255,255,255,128)
                                           );

                  ImGui::EndChildFrame     (    );
                  ImGui::PopStyleColor     (    );
                }
  
  
                bool selected = false;
  
  
                size_t count = tracked_rtv.ref_vs.size () + tracked_rtv.ref_ps.size () + tracked_rtv.ref_gs.size () +
                               tracked_rtv.ref_hs.size () + tracked_rtv.ref_ds.size () + tracked_rtv.ref_cs.size ();
  
                if (shaders > 0 && count > 0)
                {
                  ImGui::Separator  ( );

                  ImGui::BeginChild ( ImGui::GetID ("RenderTargetContributors"),
                                    ImVec2 ( std::max (font_size * 30.0f, effective_width  + 24.0f),
                                             -1.0f ),
                                      true,
                                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NavFlattened );
  
                  if (tracked_rtv.ref_vs.size () > 0 || tracked_rtv.ref_ps.size () > 0)
                  {
                    ImGui::Columns (2);
  
                    if (ImGui::CollapsingHeader ("Vertex Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_vs )
                      {
                        bool disabled = SK_D3D11_Shaders.vertex.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##vs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_vs = it;
                        }
  
                        if (SK_ImGui_IsItemRightClicked ( ))
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_VS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_VS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.vertex.blacklist, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }
  
                    ImGui::NextColumn ();
  
                    if (ImGui::CollapsingHeader ("Pixel Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_ps )
                      {
                        bool disabled = SK_D3D11_Shaders.pixel.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##ps", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_ps = it;
                        }
  
                        if (SK_ImGui_IsItemRightClicked ( ))
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_PS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_PS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.pixel.blacklist, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }
  
                    ImGui::Columns   (1);
                  }
  
                  if (tracked_rtv.ref_gs.size () > 0)
                  {
                    ImGui::Separator ( );
  
                    if (ImGui::CollapsingHeader ("Geometry Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_gs )
                      {
                        bool disabled = SK_D3D11_Shaders.geometry.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##gs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_gs = it;
                        }
  
                        if (SK_ImGui_IsItemRightClicked ( ))
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_GS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_GS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.geometry.blacklist, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }
                  }
  
                  if (tracked_rtv.ref_hs.size () > 0 || tracked_rtv.ref_ds.size () > 0)
                  {
                    ImGui::Separator ( );
  
                    ImGui::Columns   (2);
  
                    if (ImGui::CollapsingHeader ("Hull Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_hs )
                      {
                        bool disabled = SK_D3D11_Shaders.hull.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##hs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_hs = it;
                        }
  
                        if (SK_ImGui_IsItemRightClicked ( ))
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_HS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_HS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.hull.blacklist, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }
  
                    ImGui::NextColumn ();
  
                    if (ImGui::CollapsingHeader ("Domain Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_ds )
                      {
                        bool disabled = SK_D3D11_Shaders.domain.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##ds", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_ds = it;
                        }
  
                        if (SK_ImGui_IsItemRightClicked ( ))
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_DS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_DS%08lx", it).c_str ()))
                        {
                          ShaderMenu (SK_D3D11_Shaders.domain.blacklist, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }
  
                    ImGui::Columns (1);
                  }
  
                  if (tracked_rtv.ref_cs.size () > 0)
                  {
                    ImGui::Separator ( );
  
                    if (ImGui::CollapsingHeader ("Compute Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_cs )
                      {
                        bool disabled =
                          (SK_D3D11_Shaders.compute.blacklist.count (it) != 0);
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##cs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_cs = it;
                        }
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_CS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_CS%08lx", it).c_str ()))
                        {
                          ShaderMenu          (SK_D3D11_Shaders.compute.blacklist, it);
                          ImGui::EndPopup     (                                      );
                        }
                      }
  
                      ImGui::TreePop ( );
                    }
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

      //else
      //{
      //  ImGui::Text ("Live RenderTarget View is UNSAFE in multi-threaded rendering engines, it has been disabled.");
      //}

      ImGui::EndChild     ( );
      ImGui::Columns      (1);

      ImGui::PopItemWidth ( );
    }

  ImGui::End            ( );

  SK_D3D11_EnableTracking = show_dlg;

  return show_dlg;
}













struct D3DX11_STATE_BLOCK
{
  ID3D11VertexShader*        VS;
  ID3D11SamplerState*        VSSamplers        [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  VSShaderResources [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              VSConstantBuffers [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       VSInterfaces      [D3D11_SHADER_MAX_INTERFACES];
  UINT                       VSInterfaceCount;

  ID3D11GeometryShader*      GS;
  ID3D11SamplerState*        GSSamplers        [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  GSShaderResources [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              GSConstantBuffers [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       GSInterfaces      [D3D11_SHADER_MAX_INTERFACES];
  UINT                       GSInterfaceCount;

  ID3D11HullShader*          HS;
  ID3D11SamplerState*        HSSamplers        [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  HSShaderResources [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              HSConstantBuffers [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       HSInterfaces      [D3D11_SHADER_MAX_INTERFACES];
  UINT                       HSInterfaceCount;

  ID3D11DomainShader*        DS;
  ID3D11SamplerState*        DSSamplers        [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  DSShaderResources [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              DSConstantBuffers [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       DSInterfaces      [D3D11_SHADER_MAX_INTERFACES];
  UINT                       DSInterfaceCount;

  ID3D11PixelShader*         PS;
  ID3D11SamplerState*        PSSamplers        [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  PSShaderResources [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              PSConstantBuffers [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       PSInterfaces      [D3D11_SHADER_MAX_INTERFACES];
  UINT                       PSInterfaceCount;

  ID3D11ComputeShader*       CS;
  ID3D11SamplerState*        CSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  CSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              CSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       CSInterfaces           [D3D11_SHADER_MAX_INTERFACES];
  UINT                       CSInterfaceCount;
  ID3D11UnorderedAccessView* CSUnorderedAccessViews [D3D11_PS_CS_UAV_REGISTER_COUNT];

  ID3D11Buffer*              IAVertexBuffers        [D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
  UINT                       IAVertexBuffersStrides [D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
  UINT                       IAVertexBuffersOffsets [D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              IAIndexBuffer;
  DXGI_FORMAT                IAIndexBufferFormat;
  UINT                       IAIndexBufferOffset;
  ID3D11InputLayout*         IAInputLayout;
  D3D11_PRIMITIVE_TOPOLOGY   IAPrimitiveTopology;

  ID3D11RenderTargetView*    OMRenderTargets        [D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
  ID3D11DepthStencilView*    OMRenderTargetStencilView;
  ID3D11UnorderedAccessView* OMUnorderedAccessViews [D3D11_PS_CS_UAV_REGISTER_COUNT];
  ID3D11DepthStencilState*   OMDepthStencilState;
  UINT                       OMDepthStencilRef;
  ID3D11BlendState*          OMBlendState;
  FLOAT                      OMBlendFactor          [4];
  UINT                       OMSampleMask;

  UINT                       RSViewportCount;
  D3D11_VIEWPORT             RSViewports    [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
  UINT                       RSScissorRectCount;
  D3D11_RECT                 RSScissorRects [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
  ID3D11RasterizerState*     RSRasterizerState;
  ID3D11Buffer*              SOBuffers      [4];
  ID3D11Predicate*           Predication;
  BOOL                       PredicationValue;
};

void CreateStateblock (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
            CComPtr <ID3D11Device> pDev = nullptr;
                   dc->GetDevice (&pDev);
  const D3D_FEATURE_LEVEL ft_lvl = pDev->GetFeatureLevel ();

  ZeroMemory (sb, sizeof D3DX11_STATE_BLOCK);

  dc->VSGetShader          (&sb->VS, sb->VSInterfaces, &sb->VSInterfaceCount);
  
  dc->VSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->VSSamplers);
  dc->VSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->VSShaderResources);
  dc->VSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->VSConstantBuffers);
  
  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    dc->GSGetShader          (&sb->GS, sb->GSInterfaces, &sb->GSInterfaceCount);
    
    dc->GSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->GSSamplers);
    dc->GSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->GSShaderResources);
    dc->GSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->GSConstantBuffers);
  }
  
  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->HSGetShader          (&sb->HS, sb->HSInterfaces, &sb->HSInterfaceCount);
    
    dc->HSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->HSSamplers);
    dc->HSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->HSShaderResources);
    dc->HSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->HSConstantBuffers);
    
    dc->DSGetShader          (&sb->DS, sb->DSInterfaces, &sb->DSInterfaceCount);
    dc->DSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->DSSamplers);
    dc->DSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->DSShaderResources);
    dc->DSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->DSConstantBuffers);
  }
  
  dc->PSGetShader          (&sb->PS, sb->PSInterfaces, &sb->PSInterfaceCount);
  
  dc->PSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->PSSamplers);
  dc->PSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->PSShaderResources);
  dc->PSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->PSConstantBuffers);
  
  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->CSGetShader          (&sb->CS, sb->CSInterfaces, &sb->CSInterfaceCount);
    
    dc->CSGetSamplers             (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->CSSamplers);
    dc->CSGetShaderResources      (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->CSShaderResources);
    dc->CSGetConstantBuffers      (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->CSConstantBuffers);
    dc->CSGetUnorderedAccessViews (0, D3D11_PS_CS_UAV_REGISTER_COUNT,                    sb->CSUnorderedAccessViews);
  }
  
  dc->IAGetVertexBuffers     (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, sb->IAVertexBuffers,
                                                                            sb->IAVertexBuffersStrides,
                                                                            sb->IAVertexBuffersOffsets);
  dc->IAGetIndexBuffer       (&sb->IAIndexBuffer, &sb->IAIndexBufferFormat, &sb->IAIndexBufferOffset);
  dc->IAGetInputLayout       (&sb->IAInputLayout);
  dc->IAGetPrimitiveTopology (&sb->IAPrimitiveTopology);


  dc->OMGetBlendState        (&sb->OMBlendState,         sb->OMBlendFactor, &sb->OMSampleMask);
  dc->OMGetDepthStencilState (&sb->OMDepthStencilState, &sb->OMDepthStencilRef);

  dc->OMGetRenderTargets ( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, sb->OMRenderTargets, &sb->OMRenderTargetStencilView );

  sb->RSViewportCount    = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  dc->RSGetViewports         (&sb->RSViewportCount, sb->RSViewports);

  sb->RSScissorRectCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  dc->RSGetScissorRects      (&sb->RSScissorRectCount, sb->RSScissorRects);
  dc->RSGetState             (&sb->RSRasterizerState);

  //if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  //{
  //  dc->SOGetTargets           (4, sb->SOBuffers);
  //}

  dc->GetPredication         (&sb->Predication, &sb->PredicationValue);
}

template <class _T>
static
__forceinline
UINT
calc_count (_T** arr, UINT max_count)
{
  return max_count;

#if 0
  for ( int i = (int)max_count - 1 ;
            i >= 0 ;
          --i )
  {
    if (arr [i] != 0)
      return i;
  }

  return max_count;
#endif
}

void ApplyStateblock (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
            CComPtr <ID3D11Device> pDev = nullptr;
                   dc->GetDevice (&pDev);
  const D3D_FEATURE_LEVEL ft_lvl = pDev->GetFeatureLevel ();

  UINT minus_one [D3D11_PS_CS_UAV_REGISTER_COUNT] = { std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
                                                      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
                                                      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
                                                      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max () };
  
  dc->VSSetShader            (sb->VS, sb->VSInterfaces, sb->VSInterfaceCount);

  if (sb->VS != nullptr) sb->VS->Release ();

  for (UINT i = 0; i < sb->VSInterfaceCount; i++)
  {
    if (sb->VSInterfaces [i] != nullptr)
      sb->VSInterfaces [i]->Release ();
  }
  
  UINT VSSamplerCount =
    calc_count               (sb->VSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
  
  if (VSSamplerCount)
  {
    dc->VSSetSamplers        (0, VSSamplerCount, sb->VSSamplers);

    for (UINT i = 0; i < VSSamplerCount; i++)
    {
      if (sb->VSSamplers [i] != nullptr)
        sb->VSSamplers [i]->Release ();
    }
  }
  
  UINT VSShaderResourceCount =
    calc_count               (sb->VSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
  
  if (VSShaderResourceCount)
  {
    dc->VSSetShaderResources (0, VSShaderResourceCount, sb->VSShaderResources);

    for (UINT i = 0; i < VSShaderResourceCount; i++)
    {
      if (sb->VSShaderResources [i] != nullptr)
        sb->VSShaderResources [i]->Release ();
    }
  }
  
  UINT VSConstantBufferCount =
    calc_count               (sb->VSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);
  
  if (VSConstantBufferCount)
  {
    dc->VSSetConstantBuffers (0, VSConstantBufferCount, sb->VSConstantBuffers);

    for (UINT i = 0; i < VSConstantBufferCount; i++)
    {
      if (sb->VSConstantBuffers [i] != nullptr)
        sb->VSConstantBuffers [i]->Release ();
    }
  }


  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    dc->GSSetShader            (sb->GS, sb->GSInterfaces, sb->GSInterfaceCount);

    if (sb->GS != nullptr) sb->GS->Release ();

    for (UINT i = 0; i < sb->GSInterfaceCount; i++)
    {
      if (sb->GSInterfaces [i] != nullptr)
        sb->GSInterfaces [i]->Release ();
    }

    UINT GSSamplerCount =
      calc_count               (sb->GSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    if (GSSamplerCount)
    {
      dc->GSSetSamplers        (0, GSSamplerCount, sb->GSSamplers);

      for (UINT i = 0; i < GSSamplerCount; i++)
      {
        if (sb->GSSamplers [i] != nullptr)
          sb->GSSamplers [i]->Release ();
      }
    }

    UINT GSShaderResourceCount =
      calc_count               (sb->GSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    if (GSShaderResourceCount)
    {
      dc->GSSetShaderResources (0, GSShaderResourceCount, sb->GSShaderResources);

      for (UINT i = 0; i < GSShaderResourceCount; i++)
      {
        if (sb->GSShaderResources [i] != nullptr)
          sb->GSShaderResources [i]->Release ();
      }
    }

    UINT GSConstantBufferCount =
      calc_count               (sb->GSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

    if (GSConstantBufferCount)
    {
      dc->GSSetConstantBuffers (0, GSConstantBufferCount, sb->GSConstantBuffers);

      for (UINT i = 0; i < GSConstantBufferCount; i++)
      {
        if (sb->GSConstantBuffers [i] != nullptr)
          sb->GSConstantBuffers [i]->Release ();
      }
    }
  }


  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->HSSetShader            (sb->HS, sb->HSInterfaces, sb->HSInterfaceCount);

    if (sb->HS != nullptr) sb->HS->Release ();

    for (UINT i = 0; i < sb->HSInterfaceCount; i++)
    {
      if (sb->HSInterfaces [i] != nullptr)
        sb->HSInterfaces [i]->Release ();
    }

    UINT HSSamplerCount =
      calc_count               (sb->HSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    if (HSSamplerCount)
    {
      dc->HSSetSamplers        (0, HSSamplerCount, sb->HSSamplers);

      for (UINT i = 0; i < HSSamplerCount; i++)
      {
        if (sb->HSSamplers [i] != nullptr)
          sb->HSSamplers [i]->Release ();
      }
    }

    UINT HSShaderResourceCount =
      calc_count               (sb->HSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    if (HSShaderResourceCount)
    {
      dc->HSSetShaderResources (0, HSShaderResourceCount, sb->HSShaderResources);

      for (UINT i = 0; i < HSShaderResourceCount; i++)
      {
        if (sb->HSShaderResources [i] != nullptr)
          sb->HSShaderResources [i]->Release ();
      }
    }

    UINT HSConstantBufferCount =
      calc_count               (sb->HSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

    if (HSConstantBufferCount)
    {
      dc->HSSetConstantBuffers (0, HSConstantBufferCount, sb->HSConstantBuffers);

      for (UINT i = 0; i < HSConstantBufferCount; i++)
      {
        if (sb->HSConstantBuffers [i] != nullptr)
          sb->HSConstantBuffers [i]->Release ();
      }
    }


    dc->DSSetShader            (sb->DS, sb->DSInterfaces, sb->DSInterfaceCount);

    if (sb->DS != nullptr) sb->DS->Release ();

    for (UINT i = 0; i < sb->DSInterfaceCount; i++)
    {
      if (sb->DSInterfaces [i] != nullptr)
        sb->DSInterfaces [i]->Release ();
    }

    UINT DSSamplerCount =
      calc_count               (sb->DSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    if (DSSamplerCount)
    {
      dc->DSSetSamplers        (0, DSSamplerCount, sb->DSSamplers);

      for (UINT i = 0; i < DSSamplerCount; i++)
      {
        if (sb->DSSamplers [i] != nullptr)
          sb->DSSamplers [i]->Release ();
      }
    }

    UINT DSShaderResourceCount =
      calc_count               (sb->DSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    if (DSShaderResourceCount)
    {
      dc->DSSetShaderResources (0, DSShaderResourceCount, sb->DSShaderResources);

      for (UINT i = 0; i < DSShaderResourceCount; i++)
      {
        if (sb->DSShaderResources [i] != nullptr)
          sb->DSShaderResources [i]->Release ();
      }
    }

    UINT DSConstantBufferCount =
      calc_count               (sb->DSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

    if (DSConstantBufferCount)
    {
      dc->DSSetConstantBuffers (0, DSConstantBufferCount, sb->DSConstantBuffers);

      for (UINT i = 0; i < DSConstantBufferCount; i++)
      {
        if (sb->DSConstantBuffers [i] != nullptr)
          sb->DSConstantBuffers [i]->Release ();
      }
    }
  }


  dc->PSSetShader            (sb->PS, sb->PSInterfaces, sb->PSInterfaceCount);

  if (sb->PS != nullptr) sb->PS->Release ();

  for (UINT i = 0; i < sb->PSInterfaceCount; i++)
  {
    if (sb->PSInterfaces [i] != nullptr)
      sb->PSInterfaces [i]->Release ();
  }

  UINT PSSamplerCount =
    calc_count               (sb->PSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

  if (PSSamplerCount)
  {
    dc->PSSetSamplers        (0, PSSamplerCount, sb->PSSamplers);

    for (UINT i = 0; i < PSSamplerCount; i++)
    {
      if (sb->PSSamplers [i] != nullptr)
        sb->PSSamplers [i]->Release ();
    }
  }

  UINT PSShaderResourceCount =
    calc_count               (sb->PSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

  if (PSShaderResourceCount)
  {
    dc->PSSetShaderResources (0, PSShaderResourceCount, sb->PSShaderResources);

    for (UINT i = 0; i < PSShaderResourceCount; i++)
    {
      if (sb->PSShaderResources [i] != nullptr)
        sb->PSShaderResources [i]->Release ();
    }
  }

  //D3D11_PSSetShaderResources_Original (dc, 0, PSShaderResourceCount, sb->PSShaderResources);
  UINT PSConstantBufferCount =
    calc_count               (sb->PSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

  if (PSConstantBufferCount)
  {
    dc->PSSetConstantBuffers (0, PSConstantBufferCount, sb->PSConstantBuffers);

    for (UINT i = 0; i < PSConstantBufferCount; i++)
    {
      if (sb->PSConstantBuffers [i] != nullptr)
        sb->PSConstantBuffers [i]->Release ();
    }
  }


  if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
  {
    dc->CSSetShader            (sb->CS, sb->CSInterfaces, sb->CSInterfaceCount);

    if (sb->CS != nullptr)
      sb->CS->Release ();

    for (UINT i = 0; i < sb->CSInterfaceCount; i++)
    {
      if (sb->CSInterfaces [i] != nullptr)
        sb->CSInterfaces [i]->Release ();
    }

    UINT CSSamplerCount =
      calc_count               (sb->CSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    if (CSSamplerCount)
    {
      dc->CSSetSamplers        (0, CSSamplerCount, sb->CSSamplers);

      for (UINT i = 0; i < CSSamplerCount; i++)
      {
        if (sb->CSSamplers [i] != nullptr)
          sb->CSSamplers [i]->Release ();
      }
    }

    UINT CSShaderResourceCount =
      calc_count               (sb->CSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    if (CSShaderResourceCount)
    {
      dc->CSSetShaderResources (0, CSShaderResourceCount, sb->CSShaderResources);

      for (UINT i = 0; i < CSShaderResourceCount; i++)
      {
        if (sb->CSShaderResources [i] != nullptr)
          sb->CSShaderResources [i]->Release ();
      }
    }

    UINT CSConstantBufferCount =
      calc_count               (sb->CSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

    if (CSConstantBufferCount)
    {
      dc->CSSetConstantBuffers (0, CSConstantBufferCount, sb->CSConstantBuffers);

      for (UINT i = 0; i < CSConstantBufferCount; i++)
      {
        if (sb->CSConstantBuffers [i] != nullptr)
          sb->CSConstantBuffers [i]->Release ();
      }
    }

    dc->CSSetUnorderedAccessViews (0, D3D11_PS_CS_UAV_REGISTER_COUNT, sb->CSUnorderedAccessViews, minus_one);

    for (UINT i = 0; i < D3D11_PS_CS_UAV_REGISTER_COUNT; i++)
    {
      if (sb->CSUnorderedAccessViews [i] != nullptr)
        sb->CSUnorderedAccessViews [i]->Release ();
    }
  }


  UINT IAVertexBufferCount =
    calc_count               (sb->IAVertexBuffers, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

  if (IAVertexBufferCount)
  {
    dc->IASetVertexBuffers   (0, IAVertexBufferCount, sb->IAVertexBuffers, sb->IAVertexBuffersStrides, sb->IAVertexBuffersOffsets);

    for (UINT i = 0; i < IAVertexBufferCount; i++)
    {
      if (sb->IAVertexBuffers [i] != nullptr)
        sb->IAVertexBuffers [i]->Release ();
    }
  }

  dc->IASetIndexBuffer       (sb->IAIndexBuffer, sb->IAIndexBufferFormat, sb->IAIndexBufferOffset);
  dc->IASetInputLayout       (sb->IAInputLayout);
  dc->IASetPrimitiveTopology (sb->IAPrimitiveTopology);

  if (sb->IAIndexBuffer != nullptr) sb->IAIndexBuffer->Release ();
  if (sb->IAInputLayout != nullptr) sb->IAInputLayout->Release ();


  dc->OMSetBlendState        (sb->OMBlendState,        sb->OMBlendFactor, sb->OMSampleMask);
  dc->OMSetDepthStencilState (sb->OMDepthStencilState, sb->OMDepthStencilRef);

  if (sb->OMBlendState)        sb->OMBlendState->Release        ();
  if (sb->OMDepthStencilState) sb->OMDepthStencilState->Release ();

  UINT OMRenderTargetCount =
    calc_count (sb->OMRenderTargets, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

  if (OMRenderTargetCount)
  {
    dc->OMSetRenderTargets   (OMRenderTargetCount, sb->OMRenderTargets, sb->OMRenderTargetStencilView);

    for (UINT i = 0; i < OMRenderTargetCount; i++)
    {
      if (sb->OMRenderTargets [i] != nullptr)
        sb->OMRenderTargets [i]->Release ();
    }
  }

  if (sb->OMRenderTargetStencilView != nullptr)
    sb->OMRenderTargetStencilView->Release ();

  dc->RSSetViewports         (sb->RSViewportCount,     sb->RSViewports);
  dc->RSSetScissorRects      (sb->RSScissorRectCount,  sb->RSScissorRects);

  dc->RSSetState             (sb->RSRasterizerState);

  if (sb->RSRasterizerState != nullptr)
    sb->RSRasterizerState->Release ();

  //if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  //{
  //  UINT SOBuffersOffsets [4] = {   }; /* (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 0,
  //                                        (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 1, 
  //                                        (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 2, 
  //                                        (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 3 };*/
  //
  //  UINT SOBufferCount =
  //    calc_count (sb->SOBuffers, 4);
  //
  //  if (SOBufferCount)
  //  {
  //    dc->SOSetTargets (SOBufferCount, sb->SOBuffers, SOBuffersOffsets);
  //
  //    for (UINT i = 0; i < SOBufferCount; i++)
  //    {
  //      if (sb->SOBuffers [i] != nullptr)
  //      sb->SOBuffers [i]->Release ();
  //    }
  //  }
  //}

  dc->SetPredication (sb->Predication, sb->PredicationValue);

  if (sb->Predication != nullptr)
    sb->Predication->Release ();
}