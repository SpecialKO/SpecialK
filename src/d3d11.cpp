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

#include <SpecialK/widgets/widget.h>

extern LARGE_INTEGER SK_QueryPerf (void);
static DWORD         dwFrameTime = timeGetTime (); // For effects that blink, updated once per-frame

#include <SpecialK/framerate.h>
#include <SpecialK/tls.h>

#include <algorithm>
#include <memory>
#include <atomic>
#include <mutex>
#include <concurrent_unordered_set.h>
#include <atlbase.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <../depends/include/nvapi/nvapi.h>

std::vector <IUnknown *> temp_resources;

SK_Thread_HybridSpinlock cs_shader      (0xcafe);
SK_Thread_HybridSpinlock cs_shader_vs   (0x6000);
SK_Thread_HybridSpinlock cs_shader_ps   (0x8000);
SK_Thread_HybridSpinlock cs_shader_gs   (0x4000);
SK_Thread_HybridSpinlock cs_shader_hs   (0x3000);
SK_Thread_HybridSpinlock cs_shader_ds   (0x3000);
SK_Thread_HybridSpinlock cs_shader_cs   (0x9000);
SK_Thread_HybridSpinlock cs_mmio        (0x7f7f);
SK_Thread_HybridSpinlock cs_render_view (0xdddd);

HMODULE SK::DXGI::hModD3D11 = nullptr;

SK::DXGI::PipelineStatsD3D11 SK::DXGI::pipeline_stats_d3d11 = { };

volatile LONG SK_D3D11_tex_init = FALSE;
volatile LONG  __d3d11_ready    = FALSE;

void WaitForInitD3D11 (void)
{
  while (! ReadAcquire (&__d3d11_ready))
  {
    MsgWaitForMultipleObjectsEx (0, nullptr, config.system.init_delay, QS_ALLINPUT, MWMO_ALERTABLE);
  }
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
crc32_tex (  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *pInitialData,
             _Out_opt_       size_t                 *pSize,
             _Out_opt_       uint32_t               *pLOD0_CRC32 );


using SK_ReShade_PresentCallback_pfn              = bool (__stdcall *)(void *user);
using SK_ReShade_OnCopyResourceD3D11_pfn          = void (__stdcall *)(void *user, ID3D11Resource         *&dest,    ID3D11Resource *&source);
using SK_ReShade_OnClearDepthStencilViewD3D11_pfn = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnGetDepthStencilViewD3D11_pfn   = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnSetDepthStencilViewD3D11_pfn   = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnDrawD3D11_pfn                  = void (__stdcall *)(void *user, ID3D11DeviceContext     *context, unsigned int   vertices);

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
  SK_ReShade_OnDrawD3D11_pfn fn   = nullptr;
  void*                      data = nullptr;
  void call (ID3D11DeviceContext *context, unsigned int vertices) { if (data != nullptr && fn != nullptr && (! SK_TLS_Bottom ()->imgui.drawing)) fn (data, context, vertices); }
} SK_ReShade_DrawCallback;

struct
{
  SK_ReShade_OnSetDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                     data = nullptr;
  void call (ID3D11DepthStencilView *&depthstencil) { if (data != nullptr && fn != nullptr && (! SK_TLS_Bottom ()->imgui.drawing)) fn (data, depthstencil); }
} SK_ReShade_SetDepthStencilViewCallback;

struct
{
  SK_ReShade_OnGetDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                     data = nullptr;
  void call (ID3D11DepthStencilView *&depthstencil) { if (data != nullptr && fn != nullptr && (! SK_TLS_Bottom ()->imgui.drawing)) fn (data, depthstencil); }
} SK_ReShade_GetDepthStencilViewCallback;

struct
{
  SK_ReShade_OnClearDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                       data = nullptr;
  void call (ID3D11DepthStencilView *&depthstencil) { if (data != nullptr && fn != nullptr && (! SK_TLS_Bottom ()->imgui.drawing)) fn (data, depthstencil); }
} SK_ReShade_ClearDepthStencilViewCallback;

struct
{
  SK_ReShade_OnCopyResourceD3D11_pfn fn   = nullptr;
  void*                              data = nullptr;
  void call (ID3D11Resource *&dest, ID3D11Resource *&source) { if (data != nullptr && fn != nullptr && (! SK_TLS_Bottom ()->imgui.drawing)) fn (data, dest, source); }
} SK_ReShade_CopyResourceCallback;



D3DX11CreateTextureFromFileW_pfn D3DX11CreateTextureFromFileW = nullptr;
D3DX11GetImageInfoFromFileW_pfn  D3DX11GetImageInfoFromFileW  = nullptr;
HMODULE                          hModD3DX11_43                = nullptr;

bool SK_D3D11_IsTexInjectThread (DWORD dwThreadId = GetCurrentThreadId ())
{
  UNREFERENCED_PARAMETER (dwThreadId);

  SK_TLS* pTLS = SK_TLS_Bottom ();

  return pTLS->texture_management.injection_thread;
}

void
SK_D3D11_ClearTexInjectThread ( DWORD dwThreadId = GetCurrentThreadId () )
{
  UNREFERENCED_PARAMETER (dwThreadId);

  SK_TLS* pTLS = SK_TLS_Bottom ();

  pTLS->texture_management.injection_thread = false;
}

void
SK_D3D11_SetTexInjectThread ( DWORD dwThreadId = GetCurrentThreadId () )
{
  UNREFERENCED_PARAMETER (dwThreadId);

  SK_TLS* pTLS = SK_TLS_Bottom ();

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

__declspec (noinline)
ULONG
WINAPI
IUnknown_Release (IUnknown* This)
{
  if (! SK_D3D11_IsTexInjectThread ())
  {
    ID3D11Texture2D* pTex = nullptr;

    if (SUCCEEDED (This->QueryInterface <ID3D11Texture2D> (&pTex)))
    {
      ULONG count =
        IUnknown_Release_Original (pTex);

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
    // Flag this thread so we don't infinitely recurse when querying the interface
    SK_D3D11_SetTexInjectThread ();

    ID3D11Texture2D* pTex = nullptr;

    if (SUCCEEDED (This->QueryInterface <ID3D11Texture2D> (&pTex)))
    {
      SK_D3D11_ClearTexInjectThread ();

      if (SK_D3D11_TextureIsCached (pTex))
        SK_D3D11_UseTexture (pTex);

      IUnknown_Release_Original (This);
    }

    else
      SK_D3D11_ClearTexInjectThread ();
  }

  return IUnknown_AddRef_Original (This);
}

// NEVER, under any circumstances, call any functions using this!
ID3D11Device* g_pD3D11Dev = nullptr;

struct d3d11_caps_t {
  struct {
    bool d3d11_1         = false;
  } feature_level;
} d3d11_caps;

D3D11CreateDeviceAndSwapChain_pfn D3D11CreateDeviceAndSwapChain_Import = nullptr;
D3D11CreateDevice_pfn             D3D11CreateDevice_Import             = nullptr;

void
SK_D3D11_SetDevice ( ID3D11Device           **ppDevice,
                     D3D_FEATURE_LEVEL        FeatureLevel )
{
  if ( ppDevice != nullptr )
  {
    if ( *ppDevice != g_pD3D11Dev )
    {
      dll_log.Log ( L"[  D3D 11  ]  >> Device = %ph (Feature Level:%s)",
                      *ppDevice,
                        SK_DXGI_FeatureLevelsToStr ( 1,
                                                      (DWORD *)&FeatureLevel
                                                   ).c_str ()
                  );

      // We ARE technically holding a reference, but we never make calls to this
      //   interface - it's just for tracking purposes.
      g_pD3D11Dev = *ppDevice;

      SK_GetCurrentRenderBackend ().api = SK_RenderAPI::D3D11;
    }

    if (config.render.dxgi.exception_mode != -1)
      (*ppDevice)->SetExceptionMode (config.render.dxgi.exception_mode);

    CComPtr <IDXGIDevice>  pDXGIDev = nullptr;
    CComPtr <IDXGIAdapter> pAdapter = nullptr;

    HRESULT hr =
      (*ppDevice)->QueryInterface <IDXGIDevice> (&pDXGIDev);

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

UINT
SK_D3D11_RemoveUndesirableFlags (UINT& Flags)
{
  UINT original = Flags;

  // The Steam overlay behaves strangely when this is present
  Flags &= ~D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;

  return original;
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
  DXGI_SWAP_CHAIN_DESC  swap_chain_override = {   };
  auto*                 swap_chain_desc     =
    const_cast <DXGI_SWAP_CHAIN_DESC *> (pSwapChainDesc);

  DXGI_LOG_CALL_1 (L"D3D11CreateDeviceAndSwapChain", L"Flags=0x%x", Flags );

  dll_log.LogEx ( true,
                    L"[  D3D 11  ]  <~> Preferred Feature Level(s): <%u> - %s\n",
                      FeatureLevels,
                        SK_DXGI_FeatureLevelsToStr (
                          FeatureLevels,
                            (DWORD *)pFeatureLevels
                        ).c_str ()
                );

  // Optionally Enable Debug Layer
  if (ReadAcquire (&__d3d11_ready))
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
    if (SK_GetCallingDLL () != SK_GetDLL () && ReadAcquire (&SK_D3D11_init_tid)  != static_cast <LONG> (GetCurrentThreadId ()) &&
                                               ReadAcquire (&SK_D3D11_ansel_tid) != static_cast <LONG> (GetCurrentThreadId ()) )
      WaitForInitDXGI ();
  }


  SK_D3D11_RemoveUndesirableFlags (Flags);


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
   static_cast <float> (swap_chain_desc->BufferDesc.RefreshRate.Numerator) /
   static_cast <float> (swap_chain_desc->BufferDesc.RefreshRate.Denominator) :
   static_cast <float> (swap_chain_desc->BufferDesc.RefreshRate.Numerator),
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
      void
      SK_InstallWindowHook (HWND hWnd);
      SK_InstallWindowHook (swap_chain_desc->OutputWindow);

      if ( dwRenderThread == 0x00 ||
           dwRenderThread == GetCurrentThreadId () )
      {
        if ( hWndRender                    != nullptr &&
             swap_chain_desc->OutputWindow != nullptr &&
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
    *ppDevice   = ret_device;

  if (pFeatureLevel != nullptr)
    *pFeatureLevel   = ret_level;

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
  DXGI_LOG_CALL_1 (L"D3D11CreateDevice            ", L"Flags=0x%x", Flags);

  // Ansel is the purest form of evil known to man
  if (! ReadAcquire (&__d3d11_ready))
  {
    if (SK_GetCallerName () == L"NvCamera64.dll")
      InterlockedExchange (&SK_D3D11_ansel_tid, GetCurrentThreadId ());

    return
      D3D11CreateDevice_Import ( pAdapter, DriverType, Software, Flags,
                                   pFeatureLevels, FeatureLevels, SDKVersion,
                                     ppDevice, pFeatureLevel, ppImmediateContext );
  }

  else
  {
    return
      D3D11CreateDeviceAndSwapChain_Detour ( pAdapter, DriverType, Software, Flags,
                                               pFeatureLevels, FeatureLevels, SDKVersion,
                                                 nullptr, nullptr, ppDevice, pFeatureLevel,
                                                   ppImmediateContext );
  }
}




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
  if (pDesc != nullptr)
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

        HRESULT hr =
          D3D11Dev_CreateRenderTargetView_Original ( This, pResource,
                                                       &desc, ppRTView );

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  return D3D11Dev_CreateRenderTargetView_Original ( This,  pResource,
                                                    pDesc, ppRTView   );
}

struct d3d11_state_machine_s
{
  enum class cmd_type {
    ImGui,
    Clear,
    Copy,
    Deferred,
    Flush,
    MemoryMap,
    Draw,
    Dispatch,
    VertexShader,
    PixelShader,
    GeometryShader,
    DomainShader,
    HullShader,
    Raster,
    StreamOutput,
    OutputMerger,
    ComputeShader,
    UpdateResource
  };

  bool enable  = false;

  struct {
    bool track = false;
  } mmio;

  struct {
    bool track = true;
  } shaders;

  struct {
    bool track = true;
  } render_targets;

  struct {
    bool track = false;
  } buffers;

  struct {
    bool isolate = false;

    bool shouldProcessCommand (ID3D11DeviceContext* pDevCtx, cmd_type type)
    {
      UNREFERENCED_PARAMETER (pDevCtx);

      auto skipDeferred = [](ID3D11DeviceContext* pCtx)
      {
        if (! config.render.dxgi.deferred_isolation)
          return (pCtx->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED);

        return false;
      };

      if (! SK_D3D11_StateMachine.enable)
        return false;

      switch (type)
      {
        case cmd_type::Deferred:
        {
          return (! skipDeferred (pDevCtx));
        } break;

        case cmd_type::ImGui:
        {
          if (SK_TLS_Bottom ()->imgui.drawing)
            return false;

          return true;
        } break;

        case cmd_type::MemoryMap:
        {
          if (skipDeferred (pDevCtx))
            return false;

          if (SK_D3D11_StateMachine.mmio.track)
            return true;

          return false;
        } break;

        case cmd_type::Clear:
        case cmd_type::Copy:
        case cmd_type::Flush:
        case cmd_type::Draw:
        case cmd_type::Dispatch:
        case cmd_type::VertexShader:
        case cmd_type::PixelShader:
        case cmd_type::GeometryShader:
        case cmd_type::DomainShader:
        case cmd_type::HullShader:
        case cmd_type::Raster:
        case cmd_type::StreamOutput:
        case cmd_type::OutputMerger:
        case cmd_type::ComputeShader:
        {
          if (SK_TLS_Bottom ()->imgui.drawing)
            return false;

          if (skipDeferred (pDevCtx))
            return false;

          return true;
        } break;

        default:
          return true;
      }
    }
  } dev_ctx;
} SK_D3D11_StateMachine;

void
SK_D3D11_EnableTracking (bool state)
{
  SK_D3D11_StateMachine.enable = state;
}


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
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

    for (auto it : rt_views)
      it->Release ();

    for (auto it : ds_views)
      it->Release ();

    rt_views.clear ();
    ds_views.clear ();
  }
};

std::unordered_map <ID3D11DeviceContext *, SK_D3D11_KnownTargets> SK_D3D11_RenderTargets;


struct SK_D3D11_KnownThreads
{
  SK_D3D11_KnownThreads (void)
  {
    InitializeCriticalSectionAndSpinCount (&cs, 0x444);

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
      return (float)active.size () / (float)ids.size ();
    }

    return (float)active.size () / (float)ids.size ();
  }

  void   mark         (void);

private:
  std::set <DWORD> ids;
  std::set <DWORD> active;
  CRITICAL_SECTION cs;
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
  //if (! SK_D3D11_StateMachine.enable)
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
    history_s (void) = default;
    //{
      //vertex_buffers.reserve   ( 128);
      //index_buffers.reserve    ( 256);
      //constant_buffers.reserve (2048);
    //}

    void clear (SK_Thread_CriticalSection* cs)
    {
      ///std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs);

      map_types [0] = 0; map_types [1] = 0;
      map_types [2] = 0; map_types [3] = 0;
      map_types [4] = 0;

      resource_types [0] = 0; resource_types [1] = 0;
      resource_types [2] = 0; resource_types [3] = 0;
      resource_types [4] = 0;

      bytes_written = 0ULL;
      bytes_read    = 0ULL;
      bytes_copied  = 0ULL;

      mapped_resources.clear ();
      index_buffers.clear    ();
      vertex_buffers.clear   ();
      constant_buffers.clear ();
    }

    int pinned_frames = 0;

    concurrency::concurrent_unordered_set <ID3D11Buffer *>   index_buffers;
    concurrency::concurrent_unordered_set <ID3D11Buffer *>   vertex_buffers;
    concurrency::concurrent_unordered_set <ID3D11Buffer *>   constant_buffers;

    concurrency::concurrent_unordered_set <ID3D11Resource *> mapped_resources;
    std::atomic <uint32_t>      map_types      [ 5 ] = { };
    std::atomic <uint32_t>      resource_types [ 5 ] = { };

    std::atomic <uint64_t>      bytes_read           = 0ULL;
    std::atomic <uint64_t>      bytes_written        = 0ULL;
    std::atomic <uint64_t>      bytes_copied         = 0ULL;
  } lifetime, last_frame;


  std::atomic <uint32_t>        num_maps             = 0UL;
  std::atomic <uint32_t>        num_unmaps           = 0UL; // If does not match, something is pinned.


  void clear (void)
  {
    if (num_maps != num_unmaps)
      ++lifetime.pinned_frames;

    num_maps   = 0UL;
    num_unmaps = 0UL;

    lifetime.map_types [0] += last_frame.map_types [0];
    lifetime.map_types [1] += last_frame.map_types [1];
    lifetime.map_types [2] += last_frame.map_types [2];
    lifetime.map_types [3] += last_frame.map_types [3];
    lifetime.map_types [4] += last_frame.map_types [4];

    lifetime.resource_types [0] += last_frame.resource_types [0];
    lifetime.resource_types [1] += last_frame.resource_types [1];
    lifetime.resource_types [2] += last_frame.resource_types [2];
    lifetime.resource_types [3] += last_frame.resource_types [3];
    lifetime.resource_types [4] += last_frame.resource_types [4];

    lifetime.bytes_read    += last_frame.bytes_read;
    lifetime.bytes_written += last_frame.bytes_written;
    lifetime.bytes_copied  += last_frame.bytes_copied;

    last_frame.clear (cs);
  }

  SK_Thread_CriticalSection* cs;
} mem_map_stats;

struct target_tracking_s
{
  target_tracking_s (void)
  {
    for (auto& i : active)
    {
      i = false;
    }

    ref_vs.reserve (16); ref_ps.reserve (16);
    ref_gs.reserve (8);
    ref_hs.reserve (4); ref_ds.reserve (4);
    ref_cs.reserve (2);
  };

  void clear (void)
  {
    for (auto& i : active)
    {
      i = false;
    }

    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

    num_draws = 0;

    ref_vs.clear ();
    ref_ps.clear ();
    ref_gs.clear ();
    ref_hs.clear ();
    ref_ds.clear ();
    ref_cs.clear ();
  }

  volatile ID3D11RenderTargetView*       resource     =  (ID3D11RenderTargetView *)INTPTR_MAX;
  std::atomic_bool                       active [D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]
                                                      = { };
  std::atomic <uint32_t>                 num_draws    =     0;

  std::unordered_set <uint32_t> ref_vs;
  std::unordered_set <uint32_t> ref_ps;
  std::unordered_set <uint32_t> ref_gs;
  std::unordered_set <uint32_t> ref_hs;
  std::unordered_set <uint32_t> ref_ds;
  std::unordered_set <uint32_t> ref_cs;
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


  bool is_active = active;

  if ((! is_active) && active.compare_exchange_weak (is_active, true))
  {
    switch (type_)
    {
      case SK_D3D11_ShaderType::Vertex:
        SK_D3D11_Shaders.vertex.current.shader   [pDevContext] = crc32c;
        break;
      case SK_D3D11_ShaderType::Pixel:
        SK_D3D11_Shaders.pixel.current.shader    [pDevContext] = crc32c;
        break;
      case SK_D3D11_ShaderType::Geometry:
        SK_D3D11_Shaders.geometry.current.shader [pDevContext] = crc32c;
        break;
      case SK_D3D11_ShaderType::Domain:
        SK_D3D11_Shaders.domain.current.shader   [pDevContext] = crc32c;
        break;
      case SK_D3D11_ShaderType::Hull:
        SK_D3D11_Shaders.hull.current.shader     [pDevContext] = crc32c;
        break;
      case SK_D3D11_ShaderType::Compute:
        SK_D3D11_Shaders.compute.current.shader  [pDevContext] = crc32c;
        break;
    }
  }

  else
    return;

  CComPtr <ID3D11Device>        dev     = nullptr;
  CComPtr <ID3D11DeviceContext> dev_ctx = nullptr;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( rb.device != nullptr                       && rb.d3d11.immediate_ctx != nullptr &&
       SUCCEEDED (rb.device->QueryInterface              <ID3D11Device>        (&dev)) &&
       SUCCEEDED (rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&dev_ctx)) )
  {
    if (ReadPointerAcquire ((void **)&disjoint_query.async) == nullptr && timers.empty ())
    {
      D3D11_QUERY_DESC query_desc {
        D3D11_QUERY_TIMESTAMP_DISJOINT, 0x00
      };

      ID3D11Query* pQuery = nullptr;
      if (SUCCEEDED (dev->CreateQuery (&query_desc, &pQuery)))
      {
        InterlockedExchangePointer ((void **)&disjoint_query.async, pQuery);
        dev_ctx->Begin      (pQuery);
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
        InterlockedExchangePointer ((void **)&duration.start.async, pQuery);
        dev_ctx->End        (pQuery);
        timers.emplace_back (duration);
      }
    }
  }
}

void
d3d11_shader_tracking_s::deactivate (void)
{
  bool is_active = active;

  if (is_active && active.compare_exchange_weak (is_active, false))
  {
    //switch (type_)
    //{
    //  case SK_D3D11_ShaderType::Vertex:
    //    SK_D3D11_Shaders.vertex.current   = 0x0;
    //    break;
    //  case SK_D3D11_ShaderType::Pixel:
    //    SK_D3D11_Shaders.pixel.current    = 0x0;
    //    break;
    //  case SK_D3D11_ShaderType::Geometry:
    //    SK_D3D11_Shaders.geometry.current = 0x0;
    //    break;
    //  case SK_D3D11_ShaderType::Domain:
    //    SK_D3D11_Shaders.domain.current   = 0x0;
    //    break;
    //  case SK_D3D11_ShaderType::Hull:
    //    SK_D3D11_Shaders.hull.current     = 0x0;
    //    break;
    //  case SK_D3D11_ShaderType::Compute:
    //    SK_D3D11_Shaders.compute.current  = 0x0;
    //    break;
    //}
  }

  else
    return;

  CComPtr <ID3D11Device>        dev     = nullptr;
  CComPtr <ID3D11DeviceContext> dev_ctx = nullptr;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( rb.device != nullptr                           && rb.d3d11.immediate_ctx != nullptr &&
       SUCCEEDED (rb.device->QueryInterface              <ID3D11Device>        (&dev))     &&
       SUCCEEDED (rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&dev_ctx)) && 

         ReadAcquire (&disjoint_query.active) )
  {
    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP, 0x00
    };

    duration_s& duration =
      timers.back ();

                                      ID3D11Query* pQuery = nullptr;
    if (SUCCEEDED (dev->CreateQuery (&query_desc, &pQuery)))
    {
      InterlockedExchangePointer ((void **)&duration.end.async, pQuery);
                                                  dev_ctx->End (pQuery);
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


  NvAPI_Status ret =
    NvAPI_D3D11_CreateVertexShaderEx_Original ( pDevice,
                                                  pShaderBytecode, BytecodeLength,
                                                    pClassLinkage, pCreateVertexShaderExArgs,
                                                      ppVertexShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_vs.lock ();

    if (! SK_D3D11_Shaders.vertex.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;

      desc.type   = SK_D3D11_ShaderType::Vertex;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.vertex.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.vertex.rev.count (*ppVertexShader) &&
               SK_D3D11_Shaders.vertex.rev [*ppVertexShader] != checksum )
      SK_D3D11_Shaders.vertex.rev.erase (*ppVertexShader);

    SK_D3D11_Shaders.vertex.rev.emplace (std::make_pair (*ppVertexShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.vertex.descs [checksum];

    cs_shader_vs.unlock ();

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


  NvAPI_Status ret =
    NvAPI_D3D11_CreateHullShaderEx_Original ( pDevice,
                                                pShaderBytecode, BytecodeLength,
                                                  pClassLinkage, pCreateHullShaderExArgs,
                                                    ppHullShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_hs.lock ();

    if (! SK_D3D11_Shaders.hull.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Hull;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.hull.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.hull.rev.count (*ppHullShader) &&
               SK_D3D11_Shaders.hull.rev [*ppHullShader] != checksum )
      SK_D3D11_Shaders.hull.rev.erase (*ppHullShader);

    SK_D3D11_Shaders.hull.rev.emplace (std::make_pair (*ppHullShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.hull.descs [checksum];

    cs_shader_hs.unlock ();

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
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_ds.lock ();

    if (! SK_D3D11_Shaders.domain.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Domain;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.domain.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.domain.rev.count (*ppDomainShader) &&
               SK_D3D11_Shaders.domain.rev [*ppDomainShader] != checksum )
      SK_D3D11_Shaders.domain.rev.erase (*ppDomainShader);

    SK_D3D11_Shaders.domain.rev.emplace (std::make_pair (*ppDomainShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.domain.descs [checksum];

    cs_shader_ds.unlock ();

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
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_gs.lock ();

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.geometry.descs [checksum];

    cs_shader_gs.unlock ();

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
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_gs.lock ();

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.geometry.descs [checksum];

    cs_shader_gs.unlock ();

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
    uint32_t checksum =
      crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_gs.lock ();

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.geometry.descs [checksum];

    cs_shader_gs.unlock ();

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
    dll_log.Log (L"[   DXGI   ]  >> Threw out disassembled shader due to access violation during hash.");
    return 0x00;
  }
}

enum class sk_shader_class {
  Unknown  = 0x00,
  Vertex   = 0x01,
  Pixel    = 0x02,
  Geometry = 0x04,
  Hull     = 0x08,
  Domain   = 0x10,
  Compute  = 0x20
};

__forceinline
HRESULT
SK_D3D11_CreateShader_Impl (
  _In_            ID3D11Device         *This,
  _In_      const void                 *pShaderBytecode,
  _In_            SIZE_T                BytecodeLength,
  _In_opt_        ID3D11ClassLinkage   *pClassLinkage,
  _Out_opt_       IUnknown            **ppShader,
                  sk_shader_class       type )
{
  SK_D3D11_ShaderThreads.mark ();

  HRESULT hr =
    S_OK;

  auto GetResources =
  [&]( SK_Thread_CriticalSection**                        ppCritical,
       SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain )
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppCritical     = &cs_shader_vs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.vertex
                          );
         break;
      case sk_shader_class::Pixel:
        *ppCritical     = &cs_shader_ps;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.pixel
                          );
         break;
      case sk_shader_class::Geometry:
        *ppCritical     = &cs_shader_gs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.geometry
                          );
         break;
      case sk_shader_class::Domain:
        *ppCritical     = &cs_shader_ds;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.domain
                          );
         break;
      case sk_shader_class::Hull:
        *ppCritical     = &cs_shader_hs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.hull
                          );
         break;
      case sk_shader_class::Compute:
        *ppCritical     = &cs_shader_cs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.compute
                          );
         break;
    }
  };

  auto InvokeCreateRoutine =
  [&]
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        return
          D3D11Dev_CreateVertexShader_Original ( This, pShaderBytecode,
                                                   BytecodeLength, pClassLinkage,
                                                     (ID3D11VertexShader **)ppShader );
      case sk_shader_class::Pixel:
        return
          D3D11Dev_CreatePixelShader_Original ( This, pShaderBytecode,
                                                  BytecodeLength, pClassLinkage,
                                                    (ID3D11PixelShader **)ppShader );
      case sk_shader_class::Geometry:
        return
          D3D11Dev_CreateGeometryShader_Original ( This, pShaderBytecode,
                                                     BytecodeLength, pClassLinkage,
                                                       (ID3D11GeometryShader **)ppShader );
      case sk_shader_class::Domain:
        return
          D3D11Dev_CreateDomainShader_Original ( This, pShaderBytecode,
                                                   BytecodeLength, pClassLinkage,
                                                     (ID3D11DomainShader **)ppShader );
      case sk_shader_class::Hull:
        return
          D3D11Dev_CreateHullShader_Original ( This, pShaderBytecode,
                                                 BytecodeLength, pClassLinkage,
                                                   (ID3D11HullShader **)ppShader );
      case sk_shader_class::Compute:
        return
          D3D11Dev_CreateComputeShader_Original ( This, pShaderBytecode,
                                                    BytecodeLength, pClassLinkage,
                                                      (ID3D11ComputeShader **)ppShader );
    }
  };


  SK_Thread_CriticalSection*                        pCritical   = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  GetResources (&pCritical, &pShaderRepo);


  if (SUCCEEDED (hr) && ppShader)
  {
    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
    {
      hr = InvokeCreateRoutine ();
    }

    else if (! pShaderRepo->descs.count (checksum))
    {
      hr = InvokeCreateRoutine ();

      if (SUCCEEDED (hr))
      {
        std::lock_guard <SK_Thread_CriticalSection> auto_lock (*pCritical);

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
        desc.pShader->AddRef ();

        for (UINT i = 0; i < BytecodeLength; i++)
        {
          desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
        }

        pShaderRepo->descs.emplace (std::make_pair (checksum, desc));
      }
    }

    else
    {
      std::lock_guard <SK_Thread_CriticalSection> auto_lock (*pCritical);

       *ppShader = (IUnknown *)pShaderRepo->descs [checksum].pShader;
      (*ppShader)->AddRef ();
    }

    if (SUCCEEDED (hr))
    {
      pCritical->lock ();

      if ( pShaderRepo->rev.count (*ppShader) &&
                 pShaderRepo->rev [*ppShader] != checksum )
        pShaderRepo->rev.erase (*ppShader);

      pShaderRepo->rev.emplace (std::make_pair (*ppShader, checksum));

      SK_D3D11_ShaderDesc& desc =
        pShaderRepo->descs [checksum];

      pCritical->unlock ();

      InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                  _time64 (&desc.usage.last_time);
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
  bool early_out = false;

#ifndef _WIN64
  if (SK_GetCallerName ().find (L"ReShade32") != std::wstring::npos)
    early_out = true;
#else
  if (SK_GetCallerName ().find (L"ReShade64") != std::wstring::npos)
    early_out = true;
#endif

  if (early_out)
    return D3D11Dev_CreateVertexShader_Original (This, pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);


  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                                       (IUnknown **)ppVertexShader,
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
  bool early_out = false;

#ifndef _WIN64
  if (SK_GetCallerName ().find (L"ReShade32") != std::wstring::npos)
    early_out = true;
#else
  if (SK_GetCallerName ().find (L"ReShade64") != std::wstring::npos)
    early_out = true;
#endif

  if (early_out)
    return D3D11Dev_CreatePixelShader_Original (This, pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);


  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                                       (IUnknown **)ppPixelShader,
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
                                       (IUnknown **)ppGeometryShader,
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
      checksum = (uint32_t)BytecodeLength;

    cs_shader_gs.lock ();

    if (! SK_D3D11_Shaders.geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      for (UINT i = 0; i < BytecodeLength; i++)
      {
        desc.bytecode.push_back (((uint8_t *)pShaderBytecode) [i]);
      }

      SK_D3D11_Shaders.geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( SK_D3D11_Shaders.geometry.rev.count (*ppGeometryShader) &&
               SK_D3D11_Shaders.geometry.rev [*ppGeometryShader] != checksum )
      SK_D3D11_Shaders.geometry.rev.erase (*ppGeometryShader);

    SK_D3D11_Shaders.geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      SK_D3D11_Shaders.geometry.descs [checksum];

    cs_shader_gs.unlock ();

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
                                       (IUnknown **)ppHullShader,
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
                                       (IUnknown **)ppDomainShader,
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
                                       (IUnknown **)ppComputeShader,
                                         sk_shader_class::Compute );
}


__forceinline
void
SK_D3D11_SetShader_Impl ( ID3D11DeviceContext* pDevCtx, IUnknown* pShader, sk_shader_class type, ID3D11ClassInstance *const *ppClassInstances,
                                                                                                 UINT                        NumClassInstances )
{
  auto Finish =
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


  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (pDevCtx, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return Finish ();
  }


  auto GetResources =
  [&]( SK_Thread_CriticalSection**                        ppCritical,
       SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain )
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppCritical     = &cs_shader_vs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.vertex
                          );
         break;
      case sk_shader_class::Pixel:
        *ppCritical     = &cs_shader_ps;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.pixel
                          );
         break;
      case sk_shader_class::Geometry:
        *ppCritical     = &cs_shader_gs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.geometry
                          );
         break;
      case sk_shader_class::Domain:
        *ppCritical     = &cs_shader_ds;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.domain
                          );
         break;
      case sk_shader_class::Hull:
        *ppCritical     = &cs_shader_hs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.hull
                          );
         break;
      case sk_shader_class::Compute:
        *ppCritical     = &cs_shader_cs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &SK_D3D11_Shaders.compute
                          );
         break;
    }
  };


  SK_Thread_CriticalSection*                        pCritical   = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  GetResources (&pCritical, &pShaderRepo);


  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (pDevCtx, d3d11_state_machine_s::cmd_type::ComputeShader))
  {
    SK_D3D11_ShaderDesc *pDesc = nullptr;
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

      pShaderRepo->current.shader [pDevCtx] = checksum;

      InterlockedExchange (&pDesc->usage.last_frame, SK_GetFramesDrawn ());
      _time64             (&pDesc->usage.last_time);

      if (checksum == pShaderRepo->tracked.crc32c)
      {
        pShaderRepo->tracked.activate (pDevCtx, ppClassInstances, NumClassInstances);

        return Finish ();
      }
    }
  }

  pShaderRepo->tracked.deactivate ();

  return Finish ();
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

std::set <ID3D11Texture2D *> used_textures;

struct shader_stage_s {
  void nulBind (int slot, ID3D11ShaderResourceView* pView)
  {
    if (skipped_bindings [slot] != nullptr)
    {
      skipped_bindings [slot]->Release ();
      skipped_bindings [slot] = nullptr;
    }

    skipped_bindings [slot] = pView;

    if (pView != nullptr)
    {
      pView->AddRef ();
    }
  };

  void Bind (int slot, ID3D11ShaderResourceView* pView)
  {
    if (skipped_bindings [slot] != nullptr)
    {
      skipped_bindings [slot]->Release ();
      skipped_bindings [slot] = nullptr;
    }

    skipped_bindings [slot] = nullptr;

    // The D3D11 Runtime is holding a reference if this is non-null.
    real_bindings    [slot] = pView;
  };

  // We have to hold references to these things, because the D3D11 runtime would have.
  std::array <ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> skipped_bindings = { };
  std::array <ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> real_bindings    = { };
};

std::unordered_map <ID3D11DeviceContext *, shader_stage_s> d3d11_shader_stages [6];

bool
SK_D3D11_ShowShaderModDlg (void)
{
  extern bool show_shader_mod_dlg;
  return show_shader_mod_dlg;
}

__forceinline
bool
SK_D3D11_ActivateSRVOnSlot (shader_stage_s& stage, ID3D11ShaderResourceView* pSRV, int SLOT)
{
  if (! pSRV)
  {
    stage.Bind (SLOT, pSRV);
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
      std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

      used_textures.insert        (pTex);
      temp_resources.emplace_back (pTex);

      if (tracked_texture == pTex && config.textures.d3d11.highlight_debug)
      {
        if (dwFrameTime % tracked_tex_blink_duration > tracked_tex_blink_duration / 2)
        {
          stage.nulBind (SLOT, pSRV);
          return false;
        }
      }
    }
  }

  stage.Bind (SLOT, pSRV);

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
  if (! (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::VertexShader) &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred)     &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui)) )
    return D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView* newResourceViews [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { };

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
  if (! (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::PixelShader) &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred)    &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui)) )
    return D3D11_PSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView* newResourceViews [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { };

  if (ppShaderResourceViews && NumViews > 0)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_ps);

    auto&&          views = SK_D3D11_Shaders.pixel.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [1][This];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.pixel.tracked;

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
    auto&&          views = SK_D3D11_Shaders.pixel.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [1][This];

    for (UINT i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
      SK_D3D11_ActivateSRVOnSlot (stage, nullptr, i);

      newResourceViews [i] = nullptr;
      views            [i] = nullptr;
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
  if (! (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::GeometryShader) &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred)       &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui)) )
    return D3D11_GSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView* newResourceViews [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { };

  if (ppShaderResourceViews && NumViews > 0)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_gs);

    auto&&          views = SK_D3D11_Shaders.geometry.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [2][This];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.geometry.tracked;

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

  D3D11_GSSetShaderResources_Original ( This, StartSlot,
                                          NumViews, newResourceViews );
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
  if (! (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::HullShader) &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred)   &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui)) )
    return D3D11_HSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView* newResourceViews [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { };

  if (ppShaderResourceViews && NumViews > 0)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_hs);

    auto&&          views = SK_D3D11_Shaders.hull.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [3][This];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.hull.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.load ();

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (stage, ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =               ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (shader_crc32c != 0 && active &&   ppShaderResourceViews [i])            {
        if (! tracked.set_of_views.count   (ppShaderResourceViews [i]))           {
              tracked.set_of_views.emplace (ppShaderResourceViews [i]);
                                            ppShaderResourceViews [i]->AddRef (); }
              tracked.used_views.push_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
    }
  }

  D3D11_HSSetShaderResources_Original ( This, StartSlot,
                                          NumViews, newResourceViews );
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
  if (! (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::DomainShader) &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred)     &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui)) )
    return D3D11_DSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView* newResourceViews [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { };

  if (ppShaderResourceViews && NumViews > 0)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_ds);

    auto&&          views = SK_D3D11_Shaders.domain.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [4][This];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.domain.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.load ();

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (stage, ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =               ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (shader_crc32c != 0 && active &&   ppShaderResourceViews [i])            {
        if (! tracked.set_of_views.count   (ppShaderResourceViews [i]))           {
              tracked.set_of_views.emplace (ppShaderResourceViews [i]);
                                            ppShaderResourceViews [i]->AddRef (); }
              tracked.used_views.push_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
    }
  }

  D3D11_DSSetShaderResources_Original ( This, StartSlot,
                                          NumViews, newResourceViews );
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
  if (! (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ComputeShader) &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred)      &&
         SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui)) )
    return D3D11_CSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  ID3D11ShaderResourceView* newResourceViews [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { };

  if (ppShaderResourceViews && NumViews > 0)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_cs);

    auto&&          views = SK_D3D11_Shaders.compute.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [5][This];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.compute.tracked;

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

  D3D11_CSSetShaderResources_Original ( This, StartSlot,
                                          NumViews, newResourceViews );
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
  return D3D11_CSSetUnorderedAccessViews_Original (This, StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
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
  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return D3D11_UpdateSubresource1_Original ( This, pDstResource, DstSubresource,
                                                 pDstBox, pSrcData, SrcRowPitch,
                                                   SrcDepthPitch, CopyFlags );
  }

  dll_log.Log (L"[   DXGI   ] [!]D3D11_UpdateSubresource1 (%ph, %lu, %ph, %ph, %lu, %lu, %x)",
            pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch, CopyFlags);

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  pDstResource->GetType  (&rdim);

  if (config.textures.cache.allow_staging && rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D && DstSubresource == 0)
  {
    CComQIPtr <ID3D11Texture2D> pTex (pDstResource);

    if (pTex != nullptr)
    {
      D3D11_TEXTURE2D_DESC desc = { };
           pTex->GetDesc (&desc);

      if (DstSubresource == 0)
      {
        D3D11_SUBRESOURCE_DATA srd = { };

        srd.pSysMem           = pSrcData;
        srd.SysMemPitch       = SrcRowPitch;
        srd.SysMemSlicePitch  = 0;

        size_t   size         = 0;
        uint32_t top_crc32c   = 0x0;

        uint32_t checksum     =
          crc32_tex   (&desc, &srd, &size, &top_crc32c);

        uint32_t cache_tag    =
          safe_crc32c (top_crc32c, (uint8_t *)&desc, sizeof D3D11_TEXTURE2D_DESC);

        auto start            = SK_QueryPerf ().QuadPart;

        ID3D11Texture2D* pCachedTex =
          SK_D3D11_Textures.getTexture2D (cache_tag, &desc);

        if (pCachedTex != nullptr)
        {
          CComQIPtr <ID3D11Resource> pCachedResource = pCachedTex;

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
            dll_log.Log (L"[DX11TexMgr] Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
                           SK_GetCallerName ().c_str ());
            SK_D3D11_RemoveTexFromCache (pTex, true);
          }

          D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                               pDstBox, pSrcData, SrcRowPitch,
                                                 SrcDepthPitch );
          auto end     = SK_QueryPerf ().QuadPart;
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
            bool cacheable = ( //desc.CPUAccessFlags == 0          &&
                               //desc.Usage != D3D11_USAGE_DYNAMIC &&
                               desc.MiscFlags <= 4               &&
                               desc.Width      > 0               && 
                               desc.Height     > 0               &&
                               desc.ArraySize == 1 );

            bool compressed = false;

            if ( (desc.Format >= DXGI_FORMAT_BC1_TYPELESS  &&
                  desc.Format <= DXGI_FORMAT_BC5_SNORM)    ||
                 (desc.Format >= DXGI_FORMAT_BC6H_TYPELESS &&
                  desc.Format <= DXGI_FORMAT_BC7_UNORM_SRGB) )
              compressed = true;

            // If this isn't an injectable texture, then filter out non-mipmapped
            //   textures.
            if (/*(! injectable) && */cache_opts.ignore_non_mipped)
              cacheable &= (desc.MipLevels > 1 || compressed);

            if (cacheable)
            {
              SK_LOG1 ( ( L"New Cacheable Texture: (%lux%lu) -- %x",
                            desc.Width, desc.Height, top_crc32c ),
                          L"DX11TexMgr" );

              SK_D3D11_Textures.CacheMisses_2D++;
              SK_D3D11_Textures.refTexture2D (pTex, &desc, cache_tag, size, elapsed, top_crc32c);

              return;
            }
          }
        }
      }
    }
  }

  return D3D11_UpdateSubresource1_Original ( This, pDstResource, DstSubresource,
                                               pDstBox, pSrcData, SrcRowPitch,
                                                 SrcDepthPitch, CopyFlags );
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
  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                                pDstBox, pSrcData, SrcRowPitch,
                                                  SrcDepthPitch );
  }

  //dll_log.Log (L"[   DXGI   ] [!]D3D11_UpdateSubresource (%ph, %lu, %ph, %ph, %lu, %lu)",
  //          pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  pDstResource->GetType  (&rdim);

  if (config.textures.cache.allow_staging && rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D && DstSubresource == 0)
  {
    CComQIPtr <ID3D11Texture2D> pTex (pDstResource);

    if (pTex != nullptr)
    {
      D3D11_TEXTURE2D_DESC desc = { };
           pTex->GetDesc (&desc);

      if (DstSubresource == 0)
      {
        D3D11_SUBRESOURCE_DATA srd = { };

        srd.pSysMem           = pSrcData;
        srd.SysMemPitch       = SrcRowPitch;
        srd.SysMemSlicePitch  = 0;

        size_t   size         = 0;
        uint32_t top_crc32c   = 0x0;

        uint32_t checksum     =
          crc32_tex   (&desc, &srd, &size, &top_crc32c);

        uint32_t cache_tag    =
          safe_crc32c (top_crc32c, (uint8_t *)&desc, sizeof D3D11_TEXTURE2D_DESC);

        auto start            = SK_QueryPerf ().QuadPart;

        ID3D11Texture2D* pCachedTex =
          SK_D3D11_Textures.getTexture2D (cache_tag, &desc);

        if (pCachedTex != nullptr)
        {
          CComQIPtr <ID3D11Resource> pCachedResource = pCachedTex;

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
            dll_log.Log (L"[DX11TexMgr] Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
                           SK_GetCallerName ().c_str ());
            SK_D3D11_RemoveTexFromCache (pTex, true);
          }

          D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                               pDstBox, pSrcData, SrcRowPitch,
                                                 SrcDepthPitch );
          auto end     = SK_QueryPerf ().QuadPart;
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
            bool cacheable = ( //desc.CPUAccessFlags == 0          &&
                               //desc.Usage != D3D11_USAGE_DYNAMIC &&
                               desc.MiscFlags <= 4               &&
                               desc.Width      > 0               && 
                               desc.Height     > 0               &&
                               desc.ArraySize == 1 );

            bool compressed = false;

            if ( (desc.Format >= DXGI_FORMAT_BC1_TYPELESS  &&
                  desc.Format <= DXGI_FORMAT_BC5_SNORM)    ||
                 (desc.Format >= DXGI_FORMAT_BC6H_TYPELESS &&
                  desc.Format <= DXGI_FORMAT_BC7_UNORM_SRGB) )
              compressed = true;

            // If this isn't an injectable texture, then filter out non-mipmapped
            //   textures.
            if (/*(! injectable) && */cache_opts.ignore_non_mipped)
              cacheable &= (desc.MipLevels > 1 || compressed);

            if (cacheable)
            {
              SK_LOG1 ( ( L"New Cacheable Texture: (%lux%lu) -- %x",
                            desc.Width, desc.Height, top_crc32c ),
                          L"DX11TexMgr" );

              SK_D3D11_Textures.CacheMisses_2D++;
              SK_D3D11_Textures.refTexture2D (pTex, &desc, cache_tag, size, elapsed, top_crc32c);

              return;
            }
          }
        }
      }
    }
  }

  return D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
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
  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
      pResource->GetType (&rdim);

  D3D11_MAPPED_SUBRESOURCE local_map = { };

  if (pMappedResource == nullptr)
    pMappedResource = &local_map;

  HRESULT hr =
    D3D11_Map_Original ( This, pResource, Subresource,
                           MapType, MapFlags, pMappedResource );

  // ImGui gets to pass-through without invoking the hook
  if ( (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui))    ||
       (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::MemoryMap) &&
         (! config.textures.cache.allow_staging) ) )
  {
    return hr;
  }


  if (SUCCEEDED (hr))
  {
    SK_D3D11_MemoryThreads.mark ();

    if (config.textures.cache.allow_staging && rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      // Reference will be released, but we expect the game's going to unmap this at some point anyway...
      CComQIPtr <ID3D11Texture2D> pTex = pResource;

      if (pTex != nullptr)
      {
        if (MapType == D3D11_MAP_WRITE_DISCARD)
        {
          auto&& it = SK_D3D11_Textures.Textures_2D.find (pTex);
          if (  it != SK_D3D11_Textures.Textures_2D.end  (    ) && it->second.crc32c != 0x00 )
          {
                                                                               it->second.discard = true;
            pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &it->second.discard);

            SK_D3D11_RemoveTexFromCache (pTex, true);
            SK_D3D11_Textures.HashMap_2D [it->second.orig_desc.MipLevels].erase (it->second.tag);

            SK_LOG4 ( ( L"Removing discarded texture from cache (it has been memory-mapped as discard)." ),
                        L"DX11TexMgr" );
          } 
        }

        D3D11_TEXTURE2D_DESC desc = { };
             pTex->GetDesc (&desc);

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

            else
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


    if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::MemoryMap))
      return hr;


    bool read =  ( MapType == D3D11_MAP_READ      ||
                   MapType == D3D11_MAP_READ_WRITE );

    bool write = ( MapType == D3D11_MAP_WRITE             ||
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

            if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
              mem_map_stats.last_frame.index_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
              mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
              mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
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
  if ((! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui)) || (! config.textures.cache.allow_staging))
  {
    D3D11_Unmap_Original (This, pResource, Subresource);
    return;
  }

  SK_D3D11_MemoryThreads.mark ();

  if (pResource == nullptr)
    return;

  if (config.textures.cache.allow_staging && Subresource == 0)
  {
    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        pResource->GetType (&rdim);

    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      auto&& map_ctx = mapped_resources [This];

      // More of an assertion, if this fails something's screwy!
      if (map_ctx.textures.count (pResource))
      {
        std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

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
              crc32_tex (&desc, (D3D11_SUBRESOURCE_DATA *)&map_ctx.textures [pResource], &size, &top_crc32);

            //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture... (%x -- %lu bytes)", checksum, size);

            if (checksum != 0x0)
            {
              if (desc.Usage != D3D11_USAGE_STAGING)
              {
                uint32_t cache_tag    =
                  safe_crc32c (top_crc32, (uint8_t *)&desc, sizeof D3D11_TEXTURE2D_DESC);

                SK_D3D11_Textures.CacheMisses_2D++;

                SK_D3D11_Textures.refTexture2D ( pTex,
                                                   &desc,
                                                     cache_tag,
                                                       size,
                                                         time_elapsed,
                                                           top_crc32 );
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
  // XXX: Should ReShade do this even when not tracking deferred commands?
  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
    return D3D11_CopyResource_Original (This, pDstResource, pSrcResource);

  SK_ReShade_CopyResourceCallback.call (pDstResource, pSrcResource);

  CComQIPtr <ID3D11Texture2D> pDstTex = pDstResource;

  if (pDstTex != nullptr)
  {
    if (! SK_D3D11_IsTexInjectThread ())
    {
      if (SK_D3D11_TextureIsCached (pDstTex))
      {
        dll_log.Log (L"[DX11TexMgr] Cached texture was modified (CopyResource)... removing from cache! - <%s>",
                       SK_GetCallerName ().c_str ());
        SK_D3D11_RemoveTexFromCache (pDstTex, true);
      }
    }
  }


  // ImGui gets to pass-through without invoking the hook
  if ( (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui)) ||
       (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::MemoryMap) &&
         (! config.textures.cache.allow_staging) ) )
  {
    D3D11_CopyResource_Original (This, pDstResource, pSrcResource);
  
    return;
  }


  D3D11_RESOURCE_DIMENSION res_dim = { };
   pSrcResource->GetType (&res_dim);


  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::MemoryMap))
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

  D3D11_CopyResource_Original (This, pDstResource, pSrcResource);


  if (config.textures.cache.allow_staging && res_dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
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
      uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
      uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

      D3D11_TEXTURE2D_DESC dst_desc = { };
        pDstTex->GetDesc (&dst_desc);

      uint32_t cache_tag =
        safe_crc32c (top_crc32, (uint8_t *)&dst_desc, sizeof D3D11_TEXTURE2D_DESC);

      if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
      {
        SK_D3D11_Textures.CacheMisses_2D++;

        SK_D3D11_Textures.refTexture2D ( pDstTex,
                                           &dst_desc,
                                             cache_tag,
                                               map_ctx.dynamic_sizes2   [checksum],
                                                 map_ctx.dynamic_times2 [checksum],
                                                   top_crc32 );
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
  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return D3D11_CopySubresourceRegion_Original (This, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
  }

  CComQIPtr <ID3D11Texture2D> pDstTex = pDstResource;

  if (pDstTex != nullptr)
  {
    if (! SK_D3D11_IsTexInjectThread ())
    {
      if (DstSubresource == 0 && /*SrcSubresource == 0 && pSrcBox == nullptr && DstX == 0 && DstY == 0 && (! dynamic_textures.count (pDstResource)) && dynamic_textures.count (pSrcResource) && */SK_D3D11_TextureIsCached (pDstTex))
      {
        dll_log.Log (L"[DX11TexMgr] Cached texture was modified (CopySubresourceRegion)... removing from cache! - <%s>",
                       SK_GetCallerName ().c_str ());
        SK_D3D11_RemoveTexFromCache (pDstTex, true);
      }
    }
  }


  // ImGui gets to pass-through without invoking the hook
  if ( (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui)) ||
       (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::MemoryMap) &&
         (! config.textures.cache.allow_staging) ) )
  {
    D3D11_CopySubresourceRegion_Original (This, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);

    return;
  }


  D3D11_RESOURCE_DIMENSION res_dim = { };
  pSrcResource->GetType  (&res_dim);



  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::MemoryMap))
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

  if (res_dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D && config.textures.cache.allow_staging && SrcSubresource == 0 && DstSubresource == 0)
  {
    auto&& map_ctx = mapped_resources [This];

    if (pDstTex != nullptr && map_ctx.dynamic_textures.count (pSrcResource))
    {
      uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
      uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

      D3D11_TEXTURE2D_DESC dst_desc = { };
        pDstTex->GetDesc (&dst_desc);

      uint32_t cache_tag =
        safe_crc32c (top_crc32, (uint8_t *)&dst_desc, sizeof D3D11_TEXTURE2D_DESC);

      if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
      {
        SK_D3D11_Textures.refTexture2D ( pDstTex,
                                           &dst_desc,
                                             cache_tag,
                                               map_ctx.dynamic_sizes2   [checksum],
                                                 map_ctx.dynamic_times2 [checksum],
                                                   top_crc32 );

        map_ctx.dynamic_textures.erase  (pSrcResource);
        map_ctx.dynamic_texturesx.erase (pSrcResource);

        map_ctx.dynamic_sizes2.erase    (checksum);
        map_ctx.dynamic_times2.erase    (checksum);
      }
    }
  }
}



void
SK_D3D11_ClearResidualDrawState (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();


  if (pTLS->d3d11.pDSVOrig != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      SK_GetCurrentRenderBackend ().d3d11.immediate_ctx
    );

    if (pDevCtx != nullptr)
    {
      ID3D11RenderTargetView* pRTV [8] = { };
      CComPtr <ID3D11DepthStencilView> pDSV;
      
      pDevCtx->OMGetRenderTargets (8, &pRTV [0], &pDSV);

      UINT OMRenderTargetCount =
        calc_count (&pRTV [0], D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

      pDevCtx->OMSetRenderTargets (OMRenderTargetCount, &pRTV [0], pTLS->d3d11.pDSVOrig);

      //for (int i = 0; i < OMRenderTargetCount; i++)
      //{
      //  if (pRTV [i] != 0) pRTV [i]->Release ();
      //}

      pTLS->d3d11.pDSVOrig = nullptr;
    }
  }

  if (pTLS->d3d11.pDepthStencilStateOrig != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      SK_GetCurrentRenderBackend ().d3d11.immediate_ctx
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
      SK_GetCurrentRenderBackend ().d3d11.immediate_ctx
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
}


void
SK_D3D11_PostDraw (void)
{
  if ( SK_GetCurrentRenderBackend ().d3d11.immediate_ctx == nullptr ||
       SK_GetCurrentRenderBackend ().device              == nullptr ||
       SK_GetCurrentRenderBackend ().swapchain           == nullptr ) return;

  SK_D3D11_ClearResidualDrawState ();
}

struct
{
  SK_ReShade_PresentCallback_pfn fn   = nullptr;
  void*                          data = nullptr;

  struct explict_draw_s
  {
    void*                   ptr   = nullptr;
    ID3D11RenderTargetView* pRTV  = nullptr;
    bool                    pass  = false;
    int                     calls = 0;
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



class SK_ExecuteReShadeOnReturn
{
public:
   SK_ExecuteReShadeOnReturn (ID3D11DeviceContext* pCtx) : _ctx (pCtx) { };
  ~SK_ExecuteReShadeOnReturn (void)
  {
    auto TriggerReShade_After = [&]
    {
      if (SK_ReShade_PresentCallback.fn && (! SK_D3D11_Shaders.reshade_triggered))
      {
        CComPtr <ID3D11DepthStencilView>  pDSV = nullptr;
        CComPtr <ID3D11DepthStencilState> pDSS = nullptr;
        CComPtr <ID3D11RenderTargetView>  pRTV = nullptr;

                                            UINT ref;
           _ctx->OMGetDepthStencilState (&pDSS, &ref);
           _ctx->OMGetRenderTargets     (1, &pRTV, &pDSV);

        if (pDSS)
        {
          D3D11_DEPTH_STENCIL_DESC ds_desc;
                   pDSS->GetDesc (&ds_desc);

          if ((! pDSV) || (! ds_desc.StencilEnable))
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

                SK_ScopedBool auto_bool (&SK_TLS_Bottom ()->imgui.drawing);
                SK_TLS_Bottom ()->imgui.drawing = true;

                SK_ReShade_PresentCallback.explicit_draw.calls++;
                SK_ReShade_PresentCallback.fn (&SK_ReShade_PresentCallback.explicit_draw);

                break;
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

bool
SK_D3D11_DrawHandler (ID3D11DeviceContext* pDevCtx)
{
  if ( SK_GetCurrentRenderBackend ().d3d11.immediate_ctx == nullptr ||
       SK_GetCurrentRenderBackend ().device              == nullptr ||
       SK_GetCurrentRenderBackend ().swapchain           == nullptr )
    return false;


  // ImGui gets to pass-through without invoking the hook
  if (SK_TLS_Bottom ()->imgui.drawing)
    return false;


  uint32_t current_vs = SK_D3D11_Shaders.vertex.current.shader   [pDevCtx];
  uint32_t current_ps = SK_D3D11_Shaders.pixel.current.shader    [pDevCtx];
  uint32_t current_gs = SK_D3D11_Shaders.geometry.current.shader [pDevCtx];
  uint32_t current_hs = SK_D3D11_Shaders.hull.current.shader     [pDevCtx];
  uint32_t current_ds = SK_D3D11_Shaders.domain.current.shader   [pDevCtx];


  auto TriggerReShade_Before = [&]
  {
    if (SK_ReShade_PresentCallback.fn && (! SK_D3D11_Shaders.reshade_triggered))
    {
      CComPtr <ID3D11DepthStencilState> pDSS = nullptr;
      CComPtr <ID3D11DepthStencilView>  pDSV = nullptr;
      CComPtr <ID3D11RenderTargetView>  pRTV = nullptr;
                                          UINT ref;

      pDevCtx->OMGetDepthStencilState (&pDSS, &ref);
      pDevCtx->OMGetRenderTargets     (1, &pRTV, &pDSV);

      if (pDSS)
      {
        D3D11_DEPTH_STENCIL_DESC ds_desc;
                 pDSS->GetDesc (&ds_desc);

        if ((! pDSV) || (! ds_desc.StencilEnable))
        {
          for (int i = 0 ; i < 5; i++)
          {
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *>* pShaderReg;
            uint32_t*                                           current_shader = nullptr;

            switch (i)
            {
              default:
              case 0:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.vertex;
                current_shader = &current_vs;
                break;
              case 1:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.pixel;
                current_shader = &current_ps;
                break;
              case 2:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.geometry;
                current_shader = &current_gs;
                break;
              case 3:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.hull;
                current_shader = &current_hs;
                break;
              case 4:
                pShaderReg     = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.domain;
                current_shader = &current_ds;
                break;
            };


            if ( *current_shader != 0x0                        &&
               (! pShaderReg->trigger_reshade.before.empty ()) &&
                  pShaderReg->trigger_reshade.before.count (*current_shader) )
            {
              SK_D3D11_Shaders.reshade_triggered = true;

              SK_ScopedBool auto_bool (&SK_TLS_Bottom ()->imgui.drawing);
              SK_TLS_Bottom ()->imgui.drawing = true;

              SK_ReShade_PresentCallback.explicit_draw.calls++;
              SK_ReShade_PresentCallback.fn (&SK_ReShade_PresentCallback.explicit_draw);

              break;
            }
          }
        }
      }
    }
  };


  TriggerReShade_Before ();


  if (SK_D3D11_StateMachine.enable)
  {
    SK_D3D11_DrawThreads.mark ();

    bool rtv_active = false;

    for (auto& it : tracked_rtv.active)
    {
      if (it)
        rtv_active = true;
    }

    if (rtv_active)
    {
      if (current_vs != 0x00) tracked_rtv.ref_vs.insert (current_vs);
      if (current_ps != 0x00) tracked_rtv.ref_ps.insert (current_ps);
      if (current_gs != 0x00) tracked_rtv.ref_gs.insert (current_gs);
      if (current_hs != 0x00) tracked_rtv.ref_hs.insert (current_hs);
      if (current_ds != 0x00) tracked_rtv.ref_ds.insert (current_ds);
    }
  }

  bool on_top           = false;
  bool wireframe        = false;
  bool highlight_shader = (dwFrameTime % tracked_shader_blink_duration > tracked_shader_blink_duration / 2);

  auto GetTracker = [&](int i)
  {
    switch (i)
    {
      default: return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex)->tracked;
      case 1:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel)->tracked;
      case 2:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry)->tracked;
      case 3:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull)->tracked;
      case 4:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain)->tracked;
    }
  };

  for (int i = 0; i < 5; i++) { if (GetTracker (i)->active) GetTracker (i)->use (nullptr); }
  for (int i = 0; i < 5; i++)
  {
    auto *pTracker = GetTracker (i);

    if (pTracker->active)
    {
      if (pTracker->cancel_draws) return true;

      if (pTracker->wireframe) wireframe = pTracker->highlight_draws ? highlight_shader : true;
      if (pTracker->on_top)    on_top    = pTracker->highlight_draws ? highlight_shader : true;

      if (! (wireframe || on_top))
      {
        if (pTracker->highlight_draws && highlight_shader) return highlight_shader;
      }
    }
  }


  if (SK_D3D11_Shaders.vertex.blacklist.count   (current_vs)) return true;
  if (SK_D3D11_Shaders.pixel.blacklist.count    (current_ps)) return true;
  if (SK_D3D11_Shaders.geometry.blacklist.count (current_gs)) return true;
  if (SK_D3D11_Shaders.hull.blacklist.count     (current_hs)) return true;
  if (SK_D3D11_Shaders.domain.blacklist.count   (current_ds)) return true;


  /*if ( SK_D3D11_Shaders.vertex.blacklist_if_texture.size   () ||
       SK_D3D11_Shaders.pixel.blacklist_if_texture.size    () ||
       SK_D3D11_Shaders.geometry.blacklist_if_texture.size () )*/

  if (SK_D3D11_Shaders.vertex.blacklist_if_texture.count (current_vs))
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_vs);

    auto&& views = SK_D3D11_Shaders.vertex.current.views [pDevCtx];

    for (auto&& it2 : views)
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

        if (SK_D3D11_Textures.Textures_2D.count (pTex) && SK_D3D11_Textures.Textures_2D [pTex].crc32c != 0x0)
        {
          if (SK_D3D11_Shaders.vertex.blacklist_if_texture [current_vs].count (SK_D3D11_Textures.Textures_2D [pTex].crc32c))
            return true;
        }
      }
    }
  }

  if (SK_D3D11_Shaders.pixel.blacklist_if_texture.count (current_ps))
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_ps);

    auto&& views = SK_D3D11_Shaders.pixel.current.views [pDevCtx];

    for (auto&& it2 : views)
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

        if (SK_D3D11_Textures.Textures_2D.count (pTex) && SK_D3D11_Textures.Textures_2D [pTex].crc32c != 0x0)
        {
          if (SK_D3D11_Shaders.pixel.blacklist_if_texture [current_ps].count (SK_D3D11_Textures.Textures_2D [pTex].crc32c))
            return true;
        }
      }
    }
  }

  if (SK_D3D11_Shaders.geometry.blacklist_if_texture.count (current_gs))
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_gs);

    auto&& views = SK_D3D11_Shaders.geometry.current.views [pDevCtx];

    for (auto&& it2 : views)
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

        if (SK_D3D11_Textures.Textures_2D.count (pTex) && SK_D3D11_Textures.Textures_2D [pTex].crc32c != 0x00)
        {
          if (SK_D3D11_Shaders.geometry.blacklist_if_texture [current_gs].count (SK_D3D11_Textures.Textures_2D [pTex].crc32c))
            return true;
        }
      }
    }
  }


  if ((! on_top) &&    SK_D3D11_Shaders.vertex.on_top.count(current_vs))  on_top = true;
  if ((! on_top) &&    SK_D3D11_Shaders.pixel.on_top.count (current_ps))  on_top = true;

  if (on_top)
  {
    if (SK_D3D11_Shaders.vertex.tracked.on_top)
    {
      if (SK_D3D11_Shaders.vertex.tracked.crc32c != current_vs) on_top = false;
    }

    if (SK_D3D11_Shaders.pixel.tracked.on_top)
    {
      if (SK_D3D11_Shaders.pixel.tracked.crc32c != current_ps) on_top = false;
    }
  }

  if ((! on_top) && SK_D3D11_Shaders.geometry.on_top.count (current_gs)) on_top = true;
  if ((! on_top) && SK_D3D11_Shaders.hull.on_top.count     (current_hs)) on_top = true;
  if ((! on_top) && SK_D3D11_Shaders.domain.on_top.count   (current_ds)) on_top = true;

  if (on_top)
  {
    CComPtr <ID3D11Device>        pDev    = nullptr;

    SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&pDev);

    D3D11_DEPTH_STENCIL_DESC desc = { };

    pDevCtx->OMGetDepthStencilState (&SK_TLS_Bottom ()->d3d11.pDepthStencilStateOrig, &SK_TLS_Bottom ()->d3d11.StencilRefOrig);

    SK_TLS_Bottom ()->d3d11.pDepthStencilStateOrig->GetDesc (&desc);

    desc.DepthEnable    = TRUE;
    desc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.StencilEnable  = FALSE;

    if (SK_TLS_Bottom ()->d3d11.pDepthStencilStateOrig != nullptr)
    {
      if (SUCCEEDED (pDev->CreateDepthStencilState (&desc, &SK_TLS_Bottom ()->d3d11.pDepthStencilStateNew)))
      {
        pDevCtx->OMSetDepthStencilState (SK_TLS_Bottom ()->d3d11.pDepthStencilStateNew, 0);
      }
    }
  }


  if ((! wireframe) && SK_D3D11_Shaders.vertex.wireframe.count   (current_vs)) wireframe = true;
  if ((! wireframe) && SK_D3D11_Shaders.pixel.wireframe.count    (current_ps)) wireframe = true;
  if ((! wireframe) && SK_D3D11_Shaders.geometry.wireframe.count (current_gs)) wireframe = true;
  if ((! wireframe) && SK_D3D11_Shaders.hull.wireframe.count     (current_hs)) wireframe = true;
  if ((! wireframe) && SK_D3D11_Shaders.domain.wireframe.count   (current_ds)) wireframe = true;

  if (wireframe)
  {
    CComPtr <ID3D11Device>        pDev    = nullptr;

    SK_GetCurrentRenderBackend ().device->QueryInterface <ID3D11Device> (&pDev);

    pDevCtx->RSGetState         (&SK_TLS_Bottom ()->d3d11.pRasterStateOrig);

    D3D11_RASTERIZER_DESC desc = { };

    if (SK_TLS_Bottom ()->d3d11.pRasterStateOrig != nullptr)
    {
      SK_TLS_Bottom ()->d3d11.pRasterStateOrig->GetDesc (&desc);

      desc.FillMode = D3D11_FILL_WIREFRAME;
      desc.CullMode = D3D11_CULL_NONE;
      //desc.FrontCounterClockwise = TRUE;
      //desc.DepthClipEnable       = FALSE;

      if (SUCCEEDED (pDev->CreateRasterizerState (&desc, &SK_TLS_Bottom ()->d3d11.pRasterStateNew)))
      {
        pDevCtx->RSSetState (SK_TLS_Bottom ()->d3d11.pRasterStateNew);
      }
    }
  }

  SK_ExecuteReShadeOnReturn easy_reshade (pDevCtx);

  return false;
}

bool
SK_D3D11_DispatchHandler (ID3D11DeviceContext* pDevCtx)
{
  SK_D3D11_DispatchThreads.mark ();

  if (SK_D3D11_StateMachine.enable)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

    bool rtv_active = false;

    for (const auto& it : tracked_rtv.active)
    {
      if (it)
        rtv_active = true;
    }

    if (rtv_active)
    {
      if (SK_D3D11_Shaders.compute.current.shader [pDevCtx] != 0x00)
        tracked_rtv.ref_cs.insert (SK_D3D11_Shaders.compute.current.shader [pDevCtx]);
    }

    if (SK_D3D11_Shaders.compute.tracked.active) { SK_D3D11_Shaders.compute.tracked.use (nullptr); }
  }

  if (SK_D3D11_Shaders.compute.tracked.active)
  {
    bool highlight_shader =
      (dwFrameTime % tracked_shader_blink_duration > tracked_shader_blink_duration / 2);

    if (SK_D3D11_Shaders.compute.tracked.highlight_draws && highlight_shader) return true;
    if (SK_D3D11_Shaders.compute.tracked.cancel_draws)                        return true;
  }

  if (SK_D3D11_Shaders.compute.blacklist.count (SK_D3D11_Shaders.compute.current.shader [pDevCtx])) return true;

  return false;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawAuto_Override (_In_ ID3D11DeviceContext *This)
{
  SK_LOG_FIRST_CALL

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return
      D3D11_DrawAuto_Original (This);
  }

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
  {
    if (SK_D3D11_DrawHandler (This))
      return;

    SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.auto_draw);
  }

  D3D11_DrawAuto_Original (This);

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
    SK_D3D11_PostDraw ();
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

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return
      D3D11_DrawIndexed_Original ( This, IndexCount,
                                     StartIndexLocation,
                                       BaseVertexLocation );
  }

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
  {
    if (SK_D3D11_DrawHandler (This))
      return;

    SK_ReShade_DrawCallback.call (This, IndexCount * SK_D3D11_ReshadeDrawFactors.indexed);
  }

  D3D11_DrawIndexed_Original ( This, IndexCount,
                                       StartIndexLocation,
                                         BaseVertexLocation );

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
    SK_D3D11_PostDraw ();
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

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return
      D3D11_Draw_Original ( This,
                              VertexCount,
                                StartVertexLocation );
  }

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
  {
    if (SK_D3D11_DrawHandler (This))
      return;

    SK_ReShade_DrawCallback.call (This, VertexCount * SK_D3D11_ReshadeDrawFactors.draw);
  }

  D3D11_Draw_Original ( This, VertexCount, StartVertexLocation );

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
    SK_D3D11_PostDraw ();
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

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return
      D3D11_DrawIndexedInstanced_Original ( This, IndexCountPerInstance,
                                              InstanceCount, StartIndexLocation,
                                                BaseVertexLocation, StartInstanceLocation );
  }

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
  {
    if (SK_D3D11_DrawHandler (This))
      return;

    SK_ReShade_DrawCallback.call (This, IndexCountPerInstance * InstanceCount * SK_D3D11_ReshadeDrawFactors.indexed_instanced);
  }

  D3D11_DrawIndexedInstanced_Original ( This, IndexCountPerInstance,
                                          InstanceCount, StartIndexLocation,
                                            BaseVertexLocation, StartInstanceLocation );

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
    SK_D3D11_PostDraw ();
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

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return
      D3D11_DrawIndexedInstancedIndirect_Original ( This, pBufferForArgs,
                                                      AlignedByteOffsetForArgs );
  }

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
  {
    if (SK_D3D11_DrawHandler (This))
      return;

    SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.indexed_instanced_indirect);
  }

  D3D11_DrawIndexedInstancedIndirect_Original ( This, pBufferForArgs,
                                                  AlignedByteOffsetForArgs );

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
    SK_D3D11_PostDraw ();
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

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return
      D3D11_DrawInstanced_Original ( This, VertexCountPerInstance,
                                       InstanceCount, StartVertexLocation,
                                         StartInstanceLocation );
  }

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
  {
    if (SK_D3D11_DrawHandler (This))
      return;

    SK_ReShade_DrawCallback.call (This, VertexCountPerInstance * InstanceCount * SK_D3D11_ReshadeDrawFactors.instanced);
  }

  D3D11_DrawInstanced_Original ( This, VertexCountPerInstance,
                                   InstanceCount, StartVertexLocation,
                                     StartInstanceLocation );

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
    SK_D3D11_PostDraw ();
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

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return
      D3D11_DrawInstancedIndirect_Original ( This, pBufferForArgs,
                                               AlignedByteOffsetForArgs );
  }

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
  {
    if (SK_D3D11_DrawHandler (This))
      return;

    SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.instanced_indirect);
  }

  D3D11_DrawInstancedIndirect_Original ( This, pBufferForArgs,
                                           AlignedByteOffsetForArgs );

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Draw))
    SK_D3D11_PostDraw ();
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

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return
      D3D11_Dispatch_Original ( This,
                                  ThreadGroupCountX,
                                    ThreadGroupCountY,
                                      ThreadGroupCountZ );
  }

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Dispatch))
  {
    if (SK_D3D11_DispatchHandler (This))
      return;

    SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.dispatch);//std::max (1ui32, ThreadGroupCountX) * std::max (1ui32, ThreadGroupCountY) * std::max (1ui32, ThreadGroupCountZ) );
  }

  D3D11_Dispatch_Original ( This,
                              ThreadGroupCountX,
                                ThreadGroupCountY,
                                  ThreadGroupCountZ );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DispatchIndirect_Override ( _In_ ID3D11DeviceContext *This,
                                  _In_ ID3D11Buffer        *pBufferForArgs,
                                  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    return
      D3D11_DispatchIndirect_Original ( This,
                                          pBufferForArgs,
                                            AlignedByteOffsetForArgs );
  }

  if (SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Dispatch))
  {
    if (SK_D3D11_DispatchHandler (This))
      return;

    SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.dispatch_indirect);
  }

  D3D11_DispatchIndirect_Original ( This,
                                      pBufferForArgs,
                                        AlignedByteOffsetForArgs );
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

  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    D3D11_OMSetRenderTargets_Original (
      This, NumViews, ppRenderTargetViews,
        pDSV );
    return;
  }

  if (pDepthStencilView != nullptr)
    SK_ReShade_SetDepthStencilViewCallback.call (pDSV);

  D3D11_OMSetRenderTargets_Original (
    This, NumViews, ppRenderTargetViews,
      pDSV );

  // ImGui gets to pass-through without invoking the hook
  if (! ( SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui) &&
          SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::OutputMerger )))
  {
    for ( auto&& i : tracked_rtv.active ) i = false;
    return;
  }

  if (NumViews > 0)
  {
    if (ppRenderTargetViews)
    {
      auto&&                          rt_views =
        SK_D3D11_RenderTargets [This].rt_views;

      auto* tracked_rtv_res  = 
        static_cast <ID3D11RenderTargetView *> (
          ReadPointerAcquire ((volatile PVOID *)&tracked_rtv.resource)
        );

      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppRenderTargetViews [i] && (! rt_views.count (ppRenderTargetViews [i])))
        {
                           ppRenderTargetViews [i]->AddRef ();
          rt_views.insert (ppRenderTargetViews [i]);
        }

        tracked_rtv.active [i] =
          ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
                       true :
                              false;
      }
    }

    if (pDepthStencilView)
    {
      auto&& ds_views =
        SK_D3D11_RenderTargets [This].ds_views;

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
D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Override ( ID3D11DeviceContext              *This,
                                            _In_           UINT                              NumRTVs,
                                            _In_opt_       ID3D11RenderTargetView    *const *ppRenderTargetViews,
                                            _In_opt_       ID3D11DepthStencilView           *pDepthStencilView,
                                            _In_           UINT                              UAVStartSlot,
                                            _In_           UINT                              NumUAVs,
                                            _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
                                            _In_opt_ const UINT                             *pUAVInitialCounts )
{
  if (! SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::Deferred))
  {
    D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original (
      This, NumRTVs, ppRenderTargetViews,
        pDepthStencilView, UAVStartSlot, NumUAVs,
          ppUnorderedAccessViews, pUAVInitialCounts
    );
    return;
  }

  ID3D11DepthStencilView *pDSV = pDepthStencilView;

  if (pDepthStencilView != nullptr)
    SK_ReShade_SetDepthStencilViewCallback.call (pDSV);

  D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original (
    This, NumRTVs, ppRenderTargetViews,
      pDSV, UAVStartSlot, NumUAVs,
        ppUnorderedAccessViews, pUAVInitialCounts
  );

  // ImGui gets to pass-through without invoking the hook
  if (! ( SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::ImGui) &&
          SK_D3D11_StateMachine.dev_ctx.shouldProcessCommand (This, d3d11_state_machine_s::cmd_type::OutputMerger )))
  {
    for (auto&& i : tracked_rtv.active) i = false;

    return;
  }

  if (NumRTVs > 0)
  {
    if (ppRenderTargetViews)
    {
      auto&&                          rt_views =
        SK_D3D11_RenderTargets [This].rt_views;

      auto* tracked_rtv_res = 
        static_cast <ID3D11RenderTargetView *> (
          ReadPointerAcquire ((volatile PVOID *)&tracked_rtv.resource)
        );

      for (UINT i = 0; i < NumRTVs; i++)
      {
        if (ppRenderTargetViews [i] && (! rt_views.count (ppRenderTargetViews [i])))
        {
                           ppRenderTargetViews [i]->AddRef ();
          rt_views.insert (ppRenderTargetViews [i]);

          tracked_rtv.active [i] =
                                 ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
                                              true :
                                                     false;
        }
      }
    }

    if (pDepthStencilView)
    {
      auto&& ds_views =
        SK_D3D11_RenderTargets [This].ds_views;

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

  if (ppDepthStencilView != nullptr)
    SK_ReShade_GetDepthStencilViewCallback.call (*ppDepthStencilView);
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

  if (ppDepthStencilView != nullptr)
    SK_ReShade_GetDepthStencilViewCallback.call (*ppDepthStencilView);
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
  SK_ReShade_ClearDepthStencilViewCallback.call (pDepthStencilView);

  D3D11_ClearDepthStencilView_Original (This, pDepthStencilView, ClearFlags, Depth, Stencil);
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


CRITICAL_SECTION tex_cs     = { };
CRITICAL_SECTION hash_cs    = { };
CRITICAL_SECTION dump_cs    = { };
CRITICAL_SECTION cache_cs   = { };
CRITICAL_SECTION inject_cs  = { };
CRITICAL_SECTION preload_cs = { };

void WINAPI SK_D3D11_SetResourceRoot      (const wchar_t* root);
void WINAPI SK_D3D11_EnableTexDump        (bool enable);
void WINAPI SK_D3D11_EnableTexInject      (bool enable);
void WINAPI SK_D3D11_EnableTexCache       (bool enable);
void WINAPI SK_D3D11_PopulateResourceList (void);

#include <unordered_set>
#include <unordered_map>
#include <map>

// TODO:
//
//  Why the hell is this stuff global? :)
//
std::unordered_map <uint32_t, std::wstring> tex_hashes;
std::unordered_map <uint32_t, std::wstring> tex_hashes_ex;

std::unordered_set <uint32_t>               dumped_textures;
std::unordered_set <uint32_t>               dumped_collisions;
std::unordered_set <uint32_t>               injectable_textures;
std::unordered_set <uint32_t>               injected_collisions;

std::unordered_set <uint32_t>               injectable_ffx; // HACK FOR FFX

struct {
  std::unordered_set <uint32_t>             wanted;
  std::unordered_set <uint32_t>             active;
} tex_preloads;


SK_D3D11_TexMgr SK_D3D11_Textures;

class SK_D3D11_TexCacheMgr {
public:
};


std::string
SK_D3D11_SummarizeTexCache (void)
{
  char szOut [512] { };

  snprintf ( szOut, 511, "  Tex Cache: %#5llu MiB   Entries:   %#7li\n"
                         "  Hits:      %#5li       Time Saved: %#7.01lf ms\n"
                         "  Evictions: %#5li",
               SK_D3D11_Textures.AggregateSize_2D >> 20ULL,
               SK_D3D11_Textures.Entries_2D.load        (),
               SK_D3D11_Textures.RedundantLoads_2D.load (),
               SK_D3D11_Textures.RedundantTime_2D,
               SK_D3D11_Textures.Evicted_2D.load        () );

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
  if (! (SK_D3D11_cache_textures && crc32))
    return false;

  return HashMap_2D [pDesc->MipLevels].contains (crc32);
}

void
__stdcall
SK_D3D11_ResetTexCache (void)
{
  SK_D3D11_need_tex_reset = true;
  SK_D3D11_Textures.reset ();
}

#include <psapi.h>

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

  bool reset =
    SK_D3D11_need_tex_reset;

  static PROCESS_MEMORY_COUNTERS pmc = {   };

  if (! (iter % 5))
  {
                                  pmc.cb = sizeof pmc;
    GetProcessMemoryInfo (hProc, &pmc,     sizeof pmc);

    reset |=
      ( (SK_D3D11_Textures.AggregateSize_2D >> 20ULL) > (uint64_t)cache_opts.max_size    ||
         SK_D3D11_Textures.Entries_2D                 >           cache_opts.max_entries ||
         (config.mem.reserve / 100.0f) * ullMemoryTotal_KiB 
                                                      <
                            (pmc.PagefileUsage >> 10UL) );
  }

  if (reset)
  {
    //dll_log.Log (L"[DX11TexMgr] DXGI 1.4 Budget Change: Triggered a texture manager purge...");

    SK_D3D11_amount_to_purge =
      static_cast <int32_t> (
        (pmc.PagefileUsage >> 10UL) - (float)(ullMemoryTotal_KiB) *
                       (config.mem.reserve / 100.0f)
      );

    if (SK_D3D11_need_tex_reset)
    {
      tex_preloads.active.clear ();
    }

    SK_D3D11_need_tex_reset = false;
    SK_D3D11_Textures.reset ();
  }

  else
  {
    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    if (rb.d3d11.immediate_ctx != nullptr)
    {
      SK_D3D11_PreLoadTextures ();
    }
  }
}

void
SK_D3D11_TexMgr::reset (void)
{
  uint32_t count  = 0;
  int64_t  purged = 0;


  bool must_free =
    SK_D3D11_need_tex_reset;


  LONGLONG time_now = SK_QueryPerf ().QuadPart;

  // Additional conditions that may trigger a purge
  if (! must_free)
  {
    // Throttle to at most one potentially unnecessary purge attempt per-ten seconds
    if ( LastModified_2D <=  LastPurge_2D &&
         time_now        < ( LastPurge_2D + ( PerfFreq.QuadPart * 10LL ) ) )
      return;


    const float overfill_factor = 1.05f;

    if ( (uint32_t)AggregateSize_2D >> 20ULL > ( (uint32_t)       cache_opts.max_size +
                                                ((uint32_t)(float)cache_opts.max_size * overfill_factor)) )
      must_free = true;

    if ( SK_D3D11_Textures.Entries_2D > (LONG)       cache_opts.max_entries +
                                        (LONG)(float)cache_opts.max_entries * overfill_factor )
      must_free = true;

    if ( SK_D3D11_amount_to_purge > 0 )
      must_free = true;
  }

  if (! must_free)
    return;


  LastPurge_2D.store (time_now);


  std::set    <ID3D11Texture2D *>                     cleared;
  std::vector <SK_D3D11_TexMgr::tex2D_descriptor_s *> textures;

  textures.reserve (Entries_2D.load ());

  SK_AutoCriticalSection critical (&cache_cs);
  {
    for ( auto& desc : Textures_2D )
    {
      if (desc.second.texture == nullptr || desc.second.crc32c == 0x00)
        continue;

      bool can_free = true;

      if (! SK_D3D11_need_tex_reset)
        can_free = (IUnknown_AddRef_Original (desc.second.texture) == 2);

      if (can_free)
        textures.emplace_back (&desc.second);

      if (! SK_D3D11_need_tex_reset)
        IUnknown_Release_Original (desc.second.texture);
    }

    std::sort ( textures.begin (),
                  textures.end (),
        []( SK_D3D11_TexMgr::tex2D_descriptor_s* a,
            SK_D3D11_TexMgr::tex2D_descriptor_s* b )
      {
        return b->last_used < a->last_used;
      }
    );
  }


  const uint32_t max_count =
    cache_opts.max_evict;


  for ( const auto& desc : textures )
  {
    auto mem_size =
      static_cast <int64_t> (desc->mem_size) >> 10ULL;

    if (desc->texture != nullptr && cleared.count (desc->texture) == 0)
    {
      int refs =
        IUnknown_AddRef_Original  (desc->texture) - 1;
        IUnknown_Release_Original (desc->texture);

      if (refs == 1)
      {
        // This will trigger RemoveTexFromCache (...)
        desc->texture->Release ();

        // Avoid double-freeing if the hash map somehow contains the same texture multiple times
        cleared.emplace (desc->texture);

        count++;
        purged += mem_size;

        if ( (! (SK_D3D11_need_tex_reset) ) )
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

  if (count > 0)
  {
    SK_LOG0 ( ( L"Texture Cache Purge:  Eliminated %.2f MiB of texture data across %lu textures (max. evict per-pass: %lu).",
                  static_cast <float> (purged) / 1024.0f,
                                       count,
                                       cache_opts.max_evict ),
                L"DX11TexMgr" );

    if ((AggregateSize_2D >> 20ULL) >= static_cast <uint64_t> (cache_opts.max_size))
    {
      SK_LOG0 ( ( L" >> Texture cache remains %.2f MiB over-budget; will schedule future passes until resolved.",
                    static_cast <float> ( AggregateSize_2D ) / (1024.0f * 1024.0f) -
                    static_cast <float> (cache_opts.max_size) ),
                  L"DX11TexMgr" );
    }

    if (Entries_2D >= cache_opts.max_entries)
    {
      SK_LOG0 ( ( L" >> Texture cache remains %lu entries over-filled; will schedule future passes until resolved.",
                    Entries_2D - cache_opts.max_entries ),
                  L"DX11TexMgr" );
    }

    InterlockedExchange (&live_textures_dirty, TRUE);
  }
}

ID3D11Texture2D*
SK_D3D11_TexMgr::getTexture2D ( uint32_t              tag,
                          const D3D11_TEXTURE2D_DESC* pDesc,
                                size_t*               pMemSize,
                                float*                pTimeSaved )
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
    bool zombie = desc2d.crc32c == 0x00;

    if ( desc2d.crc32c != 0x00 &&
         desc2d.tag    == tag  &&
      (! desc2d.discard) )
    {
      pTex2D = desc2d.texture;

      const size_t  size = desc2d.mem_size;
      const float  fTime = static_cast <float> (desc2d.load_time ) * 1000.0f /
                           static_cast <float> (PerfFreq.QuadPart);

      if (pMemSize)   *pMemSize   = size;
      if (pTimeSaved) *pTimeSaved = fTime;

      desc2d.last_used =
        SK_QueryPerf ().QuadPart;

      // Don't record cache hits caused by the shader mod interface
      if (! SK_TLS_Bottom ()->imgui.drawing)
      {
        InterlockedIncrement (&desc2d.hits);

        RedundantData_2D += static_cast <LONG64> (size);
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

  const auto& it =
    SK_D3D11_Textures.Textures_2D.find (pTex);

  return ( it                != SK_D3D11_Textures.Textures_2D.cend () &&
           it->second.crc32c != 0x00 );
}

uint32_t
__stdcall
SK_D3D11_TextureHashFromCache (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return 0x00;

  const auto& it =
    SK_D3D11_Textures.Textures_2D.find (pTex);

  if ( it != SK_D3D11_Textures.Textures_2D.cend () )
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

void
__stdcall
SK_D3D11_RemoveTexFromCache (ID3D11Texture2D* pTex, bool blacklist)
{
  if (! SK_D3D11_TextureIsCached (pTex))
    return;

  if (pTex != nullptr)
  {
    SK_D3D11_TexMgr::tex2D_descriptor_s& it =
      SK_D3D11_Textures.Textures_2D [pTex];

          uint32_t              tag  = it.tag;
    const D3D11_TEXTURE2D_DESC& desc = it.orig_desc;

    SK_AutoCriticalSection critical (&cache_cs);

    SK_D3D11_Textures.AggregateSize_2D -=
      SK_D3D11_Textures.Textures_2D [pTex].mem_size;

    //SK_D3D11_Textures.Textures_2D.erase (pTex);
    SK_D3D11_Textures.Textures_2D [pTex].crc32c = 0x00;

    SK_D3D11_Textures.Evicted_2D++;

    SK_D3D11_Textures.TexRefs_2D.erase   (pTex);

    SK_D3D11_Textures.Entries_2D--;

    if (blacklist)
    {
      SK_D3D11_Textures.Blacklist_2D [desc.MipLevels].emplace (tag);
      pTex->Release ();
    }
    else
      SK_D3D11_Textures.HashMap_2D   [desc.MipLevels].erase   (tag);

    live_textures_dirty = true;
  }

  // Lightweight signal that something changed and a purge may be needed
  SK_D3D11_Textures.LastModified_2D = SK_QueryPerf ().QuadPart;
}

void
SK_D3D11_TexMgr::refTexture2D ( ID3D11Texture2D*      pTex,
                          const D3D11_TEXTURE2D_DESC *pDesc,
                                uint32_t              tag,
                                size_t                mem_size,
                                uint64_t              load_time,
                                uint32_t              crc32c,
                                std::wstring          fileName,
                          const D3D11_TEXTURE2D_DESC *pOrigDesc )
{
  if (! SK_D3D11_cache_textures)
    return;

  if (pTex == nullptr || tag == 0x00)
    return;

  {
    static volatile ULONG init = FALSE;

    if (! InterlockedCompareExchange (&init, TRUE, FALSE))
    {
      DXGI_VIRTUAL_HOOK_IMM ( &pTex, 2, "IUnknown::Release",
                              IUnknown_Release,
                              IUnknown_Release_Original,
                              IUnknown_Release_pfn );
      DXGI_VIRTUAL_HOOK_IMM ( &pTex, 1, "IUnknown::AddRef",
                              IUnknown_AddRef,
                              IUnknown_AddRef_Original,
                              IUnknown_AddRef_pfn );
    }
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


  static tex2D_descriptor_s  null_desc = { };
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
    texDesc.last_used  = SK_QueryPerf ().QuadPart;
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

  tex2D_descriptor_s desc2d;

  desc2d.desc       = *pDesc;
  desc2d.orig_desc  =  pOrigDesc != nullptr ? *pOrigDesc : *pDesc;
  desc2d.texture    =  pTex;
  desc2d.load_time  =  load_time;
  desc2d.mem_size   =  mem_size;
  desc2d.tag        =  tag;
  desc2d.crc32c     =  crc32c;
  desc2d.last_used  =  SK_QueryPerf ().QuadPart;
  desc2d.last_frame =  SK_GetFramesDrawn ();
  desc2d.file_name  =  fileName;

  SK_AutoCriticalSection critical (&cache_cs);

  if (desc2d.orig_desc.MipLevels >= 18)
  {
    SK_LOG0 ( ( L"Too many mipmap LODs to cache (%lu), will not cache '%x'",
                  desc2d.orig_desc.MipLevels,
                    desc2d.crc32c ),
                L"DX11TexMgr" );
  }

  SK_LOG4 ( ( L"Referencing Texture '%x' with %lu mipmap levels :: (%ph)",
                desc2d.crc32c,
                  desc2d.orig_desc.MipLevels,
                    pTex ),
              L"DX11TexMgr" );

  HashMap_2D [desc2d.orig_desc.MipLevels][tag] = pTex;
  Textures_2D.insert            (std::make_pair (pTex, desc2d));
  TexRefs_2D.insert             (                pTex);

  Entries_2D = static_cast <LONG> (TexRefs_2D.size ());

  // Hold a reference ourselves so that the game cannot free it
  pTex->AddRef ();


  // Lightweight signal that something changed and a purge may be needed
  LastModified_2D = SK_QueryPerf ().QuadPart;
}

#include <Shlwapi.h>

void
SK_D3D11_RecursiveEnumAndAddTex ( std::wstring   directory, unsigned int& files,
                                  LARGE_INTEGER& liSize,    wchar_t*      wszPattern = L"*" );

void
WINAPI
SK_D3D11_PopulateResourceList (void)
{
  static bool init = false;

  if (init || SK_D3D11_res_root.empty ())
    return;

  SK_AutoCriticalSection critical0 (&tex_cs);
  SK_AutoCriticalSection critical1 (&inject_cs);

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
    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating injectable..." );

    unsigned int    files  =   0;
    LARGE_INTEGER   liSize = {   };

    SK_D3D11_RecursiveEnumAndAddTex (wszTexInjectDir, files, liSize);

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

            injectable_ffx.insert           (ffx_crc32);
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
}



bool
WINAPI
SK_D3D11_IsTexPreLoadable (uint32_t top_crc32c)
{
  SK_AutoCriticalSection critical (&preload_cs);

  return tex_preloads.wanted.count (top_crc32c) != 0;
}

void
WINAPI
SK_D3D11_AddTexPreLoad (uint32_t top_crc32c)
{
  if (top_crc32c != 0x00)
  {
    if (! SK_D3D11_IsTexPreLoadable (top_crc32c))
    {
      SK_AutoCriticalSection critical (&preload_cs);

      tex_preloads.wanted.emplace (top_crc32c);
    }
  }
}


bool
WINAPI
SK_D3D11_IsTexPreLoaded (uint32_t top_crc32c)
{
  SK_AutoCriticalSection critical (&preload_cs);

  return tex_preloads.active.count (top_crc32c) != 0;
}

void
WINAPI
SK_D3D11_PreLoadTexture (uint32_t top_crc32c)
{
  if (top_crc32c != 0x00)
  {
    SK_AutoCriticalSection critical (&preload_cs);

    if (SK_D3D11_IsTexPreLoadable (top_crc32c) && (! SK_D3D11_IsTexPreLoaded (top_crc32c)))
    {
      tex_preloads.active.emplace (top_crc32c);

      SK_RenderBackend& rb =
        SK_GetCurrentRenderBackend ();

      if (rb.d3d11.immediate_ctx != nullptr)
      {
        CComQIPtr <ID3D11Device> pDevice (rb.device);

        if (pDevice != nullptr)
        {
          //pDevice->CreateTexture2D ()
        }
      }
    }
  }
}


void
WINAPI
SK_D3D11_PreLoadTextures (void)
{
  if (tex_preloads.wanted.size () > tex_preloads.active.size ())
  {
    SK_AutoCriticalSection critical (&preload_cs);

    for (auto it : tex_preloads.wanted)
    {
      if (! tex_preloads.active.count (it))
      {
        SK_D3D11_PreLoadTexture (it);
      }
    }
  }
}

bool
WINAPI
SK_D3D11_IsTexHashed (uint32_t top_crc32, uint32_t hash)
{
  if (tex_hashes_ex.empty () && tex_hashes.empty ())
    return false;

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
  if (hash != 0x00)
  {
    if (! SK_D3D11_IsTexHashed (top_crc32, hash))
    {
      {
        SK_AutoCriticalSection critical (&hash_cs);
        tex_hashes_ex.emplace  (crc32c (top_crc32, (const uint8_t *)&hash, 4), name);
      }

      SK_D3D11_AddInjectable (top_crc32, hash);
    }
  }

  if (! SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
    {
      SK_AutoCriticalSection critical (&hash_cs);
      tex_hashes.emplace (top_crc32, name);
    }

    if (! SK_D3D11_inject_textures_ffx)
      SK_D3D11_AddInjectable (top_crc32, 0x00);
    else
    {
      SK_AutoCriticalSection critical (&inject_cs);
      injectable_ffx.emplace (top_crc32);
    }
  }
}

void
WINAPI
SK_D3D11_RemoveTexHash (uint32_t top_crc32, uint32_t hash)
{
  if (tex_hashes_ex.empty () && tex_hashes.empty ())
    return;

  SK_AutoCriticalSection critical (&hash_cs);

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
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
  if (tex_hashes_ex.empty () && tex_hashes.empty ())
    return std::move (L"");

  SK_AutoCriticalSection critical (&hash_cs);

  std::wstring ret = L"";

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
    ret = tex_hashes_ex [crc32c (top_crc32, (const uint8_t *)&hash, 4)];
  }

  else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
    ret = tex_hashes [top_crc32];
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

  bool preload =
    StrStrIW (directory.c_str (), L"\\Preload\\") != nullptr;

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
      {
        if (StrStrIW (fd.cFileName, L".dds"))
        {
          bool     preloaded = preload;
          uint32_t top_crc32 = 0x00;
          uint32_t checksum  = 0x00;

          wchar_t* wszFileName = fd.cFileName;

          if (StrStrIW (wszFileName, L"Preload") == fd.cFileName)
          {
            const size_t skip =
              wcslen (L"Preload");

            for ( size_t i = 0; i < skip; i++ )
              wszFileName = CharNextW (wszFileName);

            preloaded = true;
          }

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
          SK_D3D11_AddTexHash (wszPath, top_crc32, 0);

          if (checksum != 0x00)
            SK_D3D11_AddTexHash (wszPath, top_crc32, checksum);

          if (preloaded)
            SK_D3D11_AddTexPreLoad (top_crc32);
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
    return 0;
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
      auto* pData    =
        static_cast  <char *> (
          const_cast <void *> (pInitialData [i].pSysMem)
        );

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
          checksum  = crc32c (checksum, (const uint8_t *)pData, lod_size);
          size     += lod_size;
        }

        // Triggered by a certain JRPG that shall remain nameless (not so Marvelous!)
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
          uint32_t row_checksum =
            safe_crc32c (checksum, (const uint8_t *)pData, stride);

          if (row_checksum != 0x00)
            checksum = row_checksum;

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
      auto* pData      =
        static_const_cast <char *, void *> (pInitialData [i].pSysMem);

      UINT  scanlength =
        SK_D3D11_BytesPerPixel (pDesc->Format) * width;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (scanlength == pInitialData [i].SysMemPitch) 
      {
        unsigned int lod_size =
          (scanlength * height);

        checksum = safe_crc32c ( checksum,
                                   // Who gave the damn language lawyers so much power?!
                                   static_cast <const uint8_t *> (
                                     static_const_cast <const void *, const char *> (pData)
                                   ),
                                  lod_size );
        size    += lod_size;
      }

      else
      {
        for (unsigned int j = 0; j < height; j++)
        {
          uint32_t row_checksum =
            safe_crc32c (checksum, (const uint8_t *)pData, scanlength);

          if (row_checksum != 0x00)
            checksum = row_checksum;

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
  int height     = pDesc->Height;

  size_t size = 0;

  for (unsigned int i = 0; i < pDesc->MipLevels; i++)
  {
    if (compressed)
    {
      size += (pInitialData [i].SysMemPitch / block_size) * (height >> i);

      checksum =
        crc32 ( checksum, (const char *)pInitialData [i].pSysMem,
                  (pInitialData [i].SysMemPitch / block_size) * (height >> i) );
    }

    else
    {
      size += (pInitialData [i].SysMemPitch) * (height >> i);

      checksum =
        crc32 ( checksum, (const char *)pInitialData [i].pSysMem,
                  (pInitialData [i].SysMemPitch) * (height >> i) );
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
  if (dumped_textures.empty ())
    return false;

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
  if (dumped_textures.empty ())
    return;

  SK_AutoCriticalSection critical (&dump_cs);

  if (! config.textures.d3d11.precise_hash)
    dumped_textures.erase (top_crc32);

  dumped_collisions.erase (crc32c (top_crc32, (uint8_t *)&checksum, 4));
}

bool
__stdcall
SK_D3D11_IsInjectable (uint32_t top_crc32, uint32_t checksum)
{
  if (injectable_textures.empty ())
    return false;

  SK_AutoCriticalSection critical (&inject_cs);

  if (checksum != 0x00)
  {
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
  if (injectable_ffx.empty ())
    return false;

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

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( SUCCEEDED (rb.device->QueryInterface              <ID3D11Device>        (&pDev))  &&
       SUCCEEDED (rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pDevCtx)) )
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
        DirectX::SaveToDDSFile ( img.GetImages   (), img.GetImageCount (),
                                 img.GetMetadata (), 0x00, wszOutName );

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
        size_t msize =
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

  bool compressed = false;

  if ( ( pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS  &&
         pDesc->Format <= DXGI_FORMAT_BC5_SNORM )   ||
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

  if ((! error) && wcslen (wszOutName))
  {
    if (GetFileAttributes (wszOutName) == INVALID_FILE_ATTRIBUTES)
    {
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

  union
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
  const D3D11_SHADER_RESOURCE_VIEW_DESC* pDescOrig = pDesc;

  if (pDesc != nullptr && pResource != nullptr)
  {
    DXGI_FORMAT newFormat = pDesc->Format;

    D3D11_SHADER_RESOURCE_VIEW_DESC desc = *pDesc;
    D3D11_RESOURCE_DIMENSION        dim;

    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      CComQIPtr <ID3D11Texture2D> pTex (pResource);

      if (pTex)
      {
        bool override = false;

        D3D11_TEXTURE2D_DESC tex_desc = { };
        pTex->GetDesc      (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          override = true;

        if ( SK_D3D11_TextureIsCached (pTex) )
        {
          newFormat =
            SK_D3D11_Textures.Textures_2D [pTex].desc.Format;

          if (pDescOrig->Format != newFormat)
          {
            override = true;

            SK_LOG1 ( ( L"Overriding Resource View Format for Cached Texture '%08x'  { Was: '%s', Now: '%s' }",
                          SK_D3D11_Textures.Textures_2D [pTex].crc32c,
                            SK_DXGI_FormatToStr   (pDescOrig->Format).c_str (),
                              SK_DXGI_FormatToStr (newFormat).c_str         () ),
                        L"DX11TexMgr" );
          }
        }

        if (override)
        {
          auto pDescCopy =
            std::make_unique <D3D11_SHADER_RESOURCE_VIEW_DESC> (*pDescOrig);

          pDescCopy->Format = newFormat;
          pDesc             = pDescCopy.get ();

          HRESULT hr =
            D3D11Dev_CreateShaderResourceView_Original ( This, pResource,
                                                           pDesc, ppSRView );

          if (SUCCEEDED (hr))
          {
            return hr;
          }
        }
      }
    }

    pDesc = pDescOrig;
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
  const D3D11_DEPTH_STENCIL_VIEW_DESC* pDescOrig = pDesc;

  if (pDesc != nullptr)
  {
    D3D11_RESOURCE_DIMENSION dim;
    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      DXGI_FORMAT newFormat = pDesc->Format;

      CComQIPtr <ID3D11Texture2D> pTex (pResource);

      if (pTex != nullptr)
      {
        D3D11_TEXTURE2D_DESC tex_desc;
             pTex->GetDesc (&tex_desc);

        HRESULT hr = E_FAIL;

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
        {
          auto pDescCopy =
            std::make_unique <D3D11_DEPTH_STENCIL_VIEW_DESC> (*pDescOrig);

          pDescCopy->Format = newFormat;
          pDesc             = pDescCopy.get ();

          hr = 
            D3D11Dev_CreateDepthStencilView_Original ( This, pResource,
                                                         pDesc, ppDepthStencilView );
        }

        if (SUCCEEDED (hr))
          return hr;
      }
    }

    pDesc = pDescOrig;
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
  const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDescOrig = pDesc;

  if (pDesc != nullptr)
  {
    D3D11_RESOURCE_DIMENSION dim;

    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      DXGI_FORMAT newFormat = pDesc->Format;

      CComQIPtr <ID3D11Texture2D> pTex (pResource);

      if (pTex != nullptr)
      {
        bool                 override = false;

        D3D11_TEXTURE2D_DESC tex_desc = { };
             pTex->GetDesc (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          override = true;

        if (override)
        {
          auto pDescCopy =
            std::make_unique <D3D11_UNORDERED_ACCESS_VIEW_DESC> (*pDescOrig);

          pDescCopy->Format = newFormat;
          pDesc             = pDescCopy.get ();

          HRESULT hr = 
            D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                            pDesc, ppUAView );

          if (SUCCEEDED (hr))
            return hr;
        }
      }
    }

    pDesc = pDescOrig;
  }

  HRESULT hr =
    D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                    pDesc, ppUAView );
  return hr;
}


std::unordered_set <ID3D11Texture2D *> render_tex;

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

            D3D11Dev_CreateSamplerState_Original ( (ID3D11Device *)SK_GetCurrentRenderBackend ().device,
                                                     &new_desc,
                                                       &pSampler );
          }

          else
          {
            static ID3D11SamplerState* override_samplers [2] = { pSampler,
                                                                 pSampler };

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



HRESULT
SK_D3DX11_SAFE_GetImageInfoFromFileW (const wchar_t* wszTex, D3DX11_IMAGE_INFO* pInfo)
{
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
}

HRESULT
SK_D3DX11_SAFE_CreateTextureFromFileW ( ID3D11Device*           pDevice,   LPCWSTR           pSrcFile,
                                        D3DX11_IMAGE_LOAD_INFO* pLoadInfo, ID3D11Resource** ppTexture )
{
  __try {
    return D3DX11CreateTextureFromFileW (pDevice, pSrcFile, pLoadInfo, nullptr, ppTexture, nullptr);
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER :
               EXCEPTION_CONTINUE_SEARCH )
  {
    return E_FAIL;
  }
}



HRESULT
SK_D3D11_ReloadTexture (ID3D11Texture2D* pTex)
{
  SK_ScopedBool auto_bool (&SK_TLS_Bottom ()->texture_management.injection_thread);

  SK_TLS_Bottom ()->texture_management.injection_thread = true;
  {
    SK_D3D11_TexMgr::tex2D_descriptor_s texDesc2D =
      SK_D3D11_Textures.Textures_2D [pTex];

    std::wstring fname = texDesc2D.file_name;

    if (GetFileAttributes (fname.c_str ()) != INVALID_FILE_ATTRIBUTES )
    {
#define D3DX11_DEFAULT static_cast <UINT> (-1)

      D3DX11_IMAGE_INFO      img_info   = {   };
      D3DX11_IMAGE_LOAD_INFO load_info  = {   };

      LARGE_INTEGER load_start =
        SK_QueryPerf ();

      HRESULT hr =
        E_UNEXPECTED;

      if (SUCCEEDED ((hr = SK_D3DX11_SAFE_GetImageInfoFromFileW (fname.c_str (), &img_info))))
      {
        load_info.BindFlags      = texDesc2D.desc.BindFlags;
        load_info.CpuAccessFlags = texDesc2D.desc.CPUAccessFlags;
        load_info.Depth          = img_info.Depth;
        load_info.Filter         = D3DX11_DEFAULT;
        load_info.FirstMipLevel  = 0;
      
        if (config.textures.d3d11.injection_keeps_fmt)
          load_info.Format       = texDesc2D.desc.Format;
        else
          load_info.Format       = img_info.Format;

        load_info.Height         = img_info.Height;
        load_info.MipFilter      = D3DX11_DEFAULT;
        load_info.MipLevels      = img_info.MipLevels;
        load_info.MiscFlags      = img_info.MiscFlags;
        load_info.pSrcInfo       = &img_info;
        load_info.Usage          = texDesc2D.desc.Usage;
        load_info.Width          = img_info.Width;

        CComPtr <ID3D11Texture2D> pInjTex = nullptr;

        hr =
          SK_D3DX11_SAFE_CreateTextureFromFileW (
            (ID3D11Device *)SK_GetCurrentRenderBackend ().device, fname.c_str (),
              &load_info,
reinterpret_cast <ID3D11Resource **> (&pInjTex)
          );

        if (SUCCEEDED (hr))
        {
          D3D11_CopyResource_Original ((ID3D11DeviceContext *)SK_GetCurrentRenderBackend ().d3d11.immediate_ctx, pTex, pInjTex);

          LARGE_INTEGER load_end =
            SK_QueryPerf ();

          SK_D3D11_Textures.Textures_2D [pTex].load_time = (load_end.QuadPart - load_start.QuadPart);

          return S_OK;
        }
      }
    }
  }

  return E_FAIL;
}


int
SK_D3D11_ReloadAllTextures (void)
{
  int count = 0;

  for ( auto& it : SK_D3D11_Textures.Textures_2D )
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


__forceinline
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Impl (
  _In_              ID3D11Device            *This,
  _Inout_opt_       D3D11_TEXTURE2D_DESC    *pDesc,
  _In_opt_    const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_         ID3D11Texture2D        **ppTexture2D )
{
  bool early_out = false;

  if (SK_D3D11_IsTexInjectThread ())
    return D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);

  if ((! (SK_D3D11_cache_textures || SK_D3D11_dump_textures || SK_D3D11_inject_textures)))
    early_out = true;

  SK_D3D11_MemoryThreads.mark ();


  if (early_out || (! ppTexture2D))
    return D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);


  if (pDesc != nullptr)
  {
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
  }

  uint32_t checksum   = 0;
  uint32_t cache_tag  = 0;
  size_t   size       = 0;

  ID3D11Texture2D* pCachedTex = nullptr;

  bool cacheable = ( ppTexture2D           != nullptr &&
                     pInitialData          != nullptr &&
                     pInitialData->pSysMem != nullptr &&
                     pDesc->SampleDesc.Count == 1     &&
                     pDesc->MiscFlags <= 4            &&
                     pDesc->MiscFlags != 1            &&
                     pDesc->Width      > 0            &&
                     pDesc->Height     > 0            &&
                     pDesc->MipLevels  > 0            &&
                     pDesc->ArraySize == 1 );

  if (pDesc->MipLevels == 0 && pDesc->MiscFlags == D3D11_RESOURCE_MISC_GENERATE_MIPS)
  {
    SK_LOG0 ( ( L"Skipping Texture because of Mipmap Autogen" ),
                L" TexCache " );
  }

  bool injectable = false;

  cacheable = cacheable &&
    ((! (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)    ||
        (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET)    ||
        (pDesc->BindFlags & D3D11_BIND_UNORDERED_ACCESS) )) &&
        (pDesc->BindFlags & D3D11_BIND_SHADER_RESOURCE)     &&
        (pDesc->Usage    <  D3D11_USAGE_DYNAMIC); // Cancel out Staging
                                                  //   They will be handled through a
                                                  //     different codepath.

  bool dynamic = false;

  if (! cacheable)
  {
    SK_LOG1 ( ( L"Uncacheable texture -- Misc Flags: %x, MipLevels: %lu, ArraySize: %lu, CPUAccess: %x, BindFlags: %x, Usage: %x, pInitialData: %p (%p)",
                  pDesc->MiscFlags, pDesc->MipLevels, pDesc->ArraySize, pDesc->CPUAccessFlags, pDesc->BindFlags, pDesc->Usage, pInitialData, pInitialData ? pInitialData->pSysMem : nullptr ),
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
      bool compressed = false;

      if ( (pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS  &&
            pDesc->Format <= DXGI_FORMAT_BC5_SNORM)    ||
           (pDesc->Format >= DXGI_FORMAT_BC6H_TYPELESS &&
            pDesc->Format <= DXGI_FORMAT_BC7_UNORM_SRGB) )
        compressed = true;

      // If this isn't an injectable texture, then filter out non-mipmapped
      //   textures.
      if ((! injectable) && cache_opts.ignore_non_mipped)
        cacheable &= (pDesc->MipLevels > 1 || compressed);

      if (cacheable)
      {
        cache_tag  =
          safe_crc32c (top_crc32, (uint8_t *)pDesc, sizeof D3D11_TEXTURE2D_DESC);

        pCachedTex =
          SK_D3D11_Textures.getTexture2D (cache_tag, pDesc);
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
    pCachedTex->AddRef ();

    *ppTexture2D = pCachedTex;

    return S_OK;
  }

  // The concept of a cache-miss only applies if the texture had data at the time
  //   of creation...
  if ( cacheable )
    SK_D3D11_Textures.CacheMisses_2D++;


  LARGE_INTEGER load_start =
    SK_QueryPerf ();

  if (cacheable)
  {
    if (D3DX11CreateTextureFromFileW != nullptr && SK_D3D11_res_root.length ())
    {
      wchar_t wszTex [MAX_PATH + 2] = { };
      
      wcscpy ( wszTex,
                SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

      static std::wstring  null_ref;
             std::wstring& hash_name = null_ref;

      static bool ffx = GetModuleHandle (L"unx.dll") != nullptr;

      if (ffx && (! (hash_name = SK_D3D11_TexHashToName (ffx_crc32, 0x00)).empty ()))
      {
        SK_LOG4 ( ( L"Caching texture with crc32: %x", ffx_crc32 ),
                    L" Tex Hash " );

        PathCombineW (wszTex, wszTex, hash_name.c_str ());
      }

      else if (! ( (hash_name = SK_D3D11_TexHashToName (top_crc32, checksum)).empty () &&
                   (hash_name = SK_D3D11_TexHashToName (top_crc32, 0x00)    ).empty () ) )
      {
        SK_LOG4 ( ( L"Caching texture with crc32c: %x", top_crc32 ),
                    L" Tex Hash " );

        PathCombineW (wszTex, wszTex, hash_name.c_str ());
      }

      else if ( SK_D3D11_inject_textures )
      {
        if ( /*config.textures.d3d11.precise_hash &&*/
             SK_D3D11_IsInjectable (top_crc32, checksum) )
        {
          _swprintf ( wszTex,
                        L"%s\\inject\\textures\\%08X_%08X.dds",
                          wszTex,
                            top_crc32, checksum );
        }

        else if ( SK_D3D11_IsInjectable (top_crc32, 0x00) )
        {
          SK_LOG4 ( ( L"Caching texture with crc32c: %08X", top_crc32 ),
                      L" Tex Hash " );
          _swprintf ( wszTex,
                        L"%s\\inject\\textures\\%08X.dds",
                          wszTex,
                            top_crc32 );
        }

        else if ( ffx &&
                  SK_D3D11_IsInjectable_FFX (ffx_crc32) )
        {
          SK_LOG4 ( ( L"Caching texture with crc32: %08X", ffx_crc32 ),
                      L" Tex Hash " );
          _swprintf ( wszTex,
                        L"%s\\inject\\textures\\Unx_Old\\%08X.dds",
                          wszTex,
                            ffx_crc32 );
        }

        else *wszTex = L'\0';
      }

      // Not a hashed texture, not an injectable texture, skip it...
      else *wszTex = L'\0';

      SK_FixSlashesW (wszTex);

      if (                   *wszTex  != L'\0' &&
           GetFileAttributes (wszTex) != INVALID_FILE_ATTRIBUTES )
      {
#define D3DX11_DEFAULT static_cast <UINT> (-1)

        D3DX11_IMAGE_INFO      img_info   = {   };
        D3DX11_IMAGE_LOAD_INFO load_info  = {   };

        SK_D3D11_SetTexInjectThread ();

        HRESULT hr = E_UNEXPECTED;

        if (SUCCEEDED ((hr = SK_D3DX11_SAFE_GetImageInfoFromFileW (wszTex, &img_info))))
        {
          load_info.BindFlags      = pDesc->BindFlags;
          load_info.CpuAccessFlags = pDesc->CPUAccessFlags;
          load_info.Depth          = img_info.Depth;
          load_info.Filter         = D3DX11_DEFAULT;
          load_info.FirstMipLevel  = 0;

          if (config.textures.d3d11.injection_keeps_fmt)
            load_info.Format       = pDesc->Format;
          else
            load_info.Format       = img_info.Format;

          load_info.Height         = img_info.Height;
          load_info.MipFilter      = D3DX11_DEFAULT;
          load_info.MipLevels      = img_info.MipLevels;
          load_info.MiscFlags      = img_info.MiscFlags;
          load_info.pSrcInfo       = &img_info;
          load_info.Usage          = pDesc->Usage;
          load_info.Width          = img_info.Width;

          hr =
            SK_D3DX11_SAFE_CreateTextureFromFileW (
                             This, wszTex,
                               &load_info,
reinterpret_cast <ID3D11Resource **> (ppTexture2D)
            );

          if (SUCCEEDED (hr))
          {
            LARGE_INTEGER load_end =
              SK_QueryPerf ();

            SK_D3D11_ClearTexInjectThread ();

            D3D11_TEXTURE2D_DESC orig_desc = *pDesc;

            pDesc->BindFlags      = load_info.BindFlags;
            pDesc->CPUAccessFlags = load_info.CpuAccessFlags;
            pDesc->ArraySize      = load_info.Depth;
            pDesc->Format         = load_info.Format;
            pDesc->Height         = load_info.Height;
            pDesc->MipLevels      = load_info.MipLevels;
            pDesc->MiscFlags      = load_info.MiscFlags;
            pDesc->Usage          = load_info.Usage;
            pDesc->Width          = load_info.Width;

            SK_D3D11_Textures.refTexture2D (
              *ppTexture2D,
                pDesc,
                  cache_tag,
                    size,
                      load_end.QuadPart - load_start.QuadPart,
                        top_crc32,
                          wszTex,
                            &orig_desc );

            SK_D3D11_Textures.Textures_2D [*ppTexture2D].injected = true;

            return hr;
          }
        }

        dll_log.Log (L"[DX11TexMgr] *** Texture '%s' is corrupted (HRESULT=%x), skipping!", wszTex, hr);

        SK_D3D11_ClearTexInjectThread ();
      }
    }
  }

  HRESULT ret =
    D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);

  LARGE_INTEGER load_end =
    SK_QueryPerf ();

  if ( SUCCEEDED (ret) &&
          dumpable     &&
      checksum != 0x00 &&
      SK_D3D11_dump_textures )
  {
    if (! SK_D3D11_IsDumped (top_crc32, checksum))
    {
      SK_TLS_Bottom ()->texture_management.injection_thread = true;
      SK_D3D11_DumpTexture2D (pDesc, pInitialData, top_crc32, checksum);
      SK_TLS_Bottom ()->texture_management.injection_thread = false;
    }
  }

  cacheable &=
    (SK_D3D11_cache_textures || injectable);

  if ( SUCCEEDED (ret) && cacheable )
  {
    if (! SK_D3D11_Textures.Blacklist_2D [pDesc->MipLevels].count (checksum))
    {
      SK_D3D11_Textures.refTexture2D (
        *ppTexture2D,
          pDesc,
            cache_tag,
              size,
                load_end.QuadPart - load_start.QuadPart,
                  top_crc32
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
  static HMODULE hModSteamOverlay =
#ifndef _WIN64
        GetModuleHandle (L"gameoverlayrenderer.dll");
#else
        GetModuleHandle (L"gameoverlayrenderer64.dll");
#endif

  HMODULE hModCaller = SK_GetCallingDLL ();

  bool early_out = false;

#ifndef _WIN64
  if (SK_GetCallerName ().find (L"ReShade32") != std::wstring::npos)
    early_out = true;
#else
  if (SK_GetCallerName ().find (L"ReShade64") != std::wstring::npos)
    early_out = true;
#endif

  if ( hModCaller == hModSteamOverlay || early_out )
  {
    return
      D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);
  }

  const D3D11_TEXTURE2D_DESC* pDescOrig = pDesc;

  // Make a copy, then change the value on the stack to point to
  //   our copy in order to make changes propagate to other software
  //     that hooks this...
  auto pDescCopy =
    std::make_unique <D3D11_TEXTURE2D_DESC> (*pDescOrig);

  return
    D3D11Dev_CreateTexture2D_Impl (This, pDescCopy.get (), pInitialData, ppTexture2D);
}


void
__stdcall
SK_D3D11_UpdateRenderStatsEx (IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain))
    return;

  CComPtr <ID3D11Device>        pDev    = nullptr;
  CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.device == nullptr || rb.d3d11.immediate_ctx == nullptr || rb.swapchain == nullptr)
    return;

  if ( SUCCEEDED (rb.device->QueryInterface              <ID3D11Device>        (&pDev))  &&
       SUCCEEDED (rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pDevCtx)) )
  {
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

      ID3D11Query* pQuery;
      if (SUCCEEDED (pDev->CreateQuery (&query_desc, &pQuery)))
      {
        InterlockedExchangePointer ((void **)&pipeline_stats.query.async,  pQuery);
        pDevCtx->Begin             (                                       pQuery);
        InterlockedExchange        (         &pipeline_stats.query.active, TRUE);
      }
    }
  }
}

void
__stdcall
SK_D3D11_UpdateRenderStats (IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain && ( config.render.show ||
                           SK_ImGui_Widgets.d3d11_pipeline->isActive () ) ) )
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
                       ( static_cast <float> (stats.CPrimitives) /
                         static_cast <float> (stats.CInvocations) ),
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


void
SK_D3D11_InitTextures (void)
{
  if (! InterlockedCompareExchange (&SK_D3D11_tex_init, TRUE, FALSE))
  {
    if ( StrStrIW (SK_GetHostApp (), L"FFX.exe")   ||
         StrStrIW (SK_GetHostApp (), L"FFX-2.exe") ||
         StrStrIW (SK_GetHostApp (), L"FFX&X-2_Will.exe") )
      SK_D3D11_inject_textures_ffx = true;

    InitializeCriticalSectionAndSpinCount (&preload_cs,   0x0100);
    InitializeCriticalSectionAndSpinCount (&dump_cs,      0x0200);
    InitializeCriticalSectionAndSpinCount (&inject_cs,    0x1000);
    InitializeCriticalSectionAndSpinCount (&hash_cs,      0x4000);
    InitializeCriticalSectionAndSpinCount (&cache_cs,     0x8000);
    InitializeCriticalSectionAndSpinCount (&tex_cs,       0xFFFF);

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

    if ((! SK_D3D11_inject_textures_ffx) && config.textures.d3d11.inject)
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
           SK_CreateDLLHook2 (      L"d3d11.dll",
                                     "D3D11CreateDevice",
                                      D3D11CreateDevice_Detour,
             static_cast_p2p <void> (&D3D11CreateDevice_Import),
                                     &pfnD3D11CreateDevice )
       )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2 (      L"d3d11.dll",
                                       "D3D11CreateDeviceAndSwapChain",
                                        D3D11CreateDeviceAndSwapChain_Detour,
               static_cast_p2p <void> (&D3D11CreateDeviceAndSwapChain_Import),
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
                      (float)SK_D3D11_Textures.RedundantData_2D.load () /
                                 (1024.0f * 1024.0f),
                             SK_D3D11_Textures.RedundantLoads_2D.load () );
  }

  SK_D3D11_Textures.reset ();

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

  dll_log.Log (L"[  D3D 11  ]   Hooking D3D11");

  auto* pHooks = 
    static_cast <sk_hook_d3d11_t *> (user);

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

    //
    // Third-party software frequently causes these hooks to become corrupted, try installing a new
    //   vftable pointer instead of hooking the function.
    //
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


    CComQIPtr <ID3D11DeviceContext1> pDevCtx1 (*pHooks->ppImmediateContext);

    if (pDevCtx1 != nullptr)
    {
      DXGI_VIRTUAL_HOOK ( &pDevCtx1,  116,
                            "ID3D11DeviceContext1::UpdateSubresource1",
                                             D3D11_UpdateSubresource1_Override,
                                             D3D11_UpdateSubresource1_Original,
                                             D3D11_UpdateSubresource1_pfn );
    }
  }

#ifdef _WIN64
  if (config.apis.dxgi.d3d12.hook)
    HookD3D12 (nullptr);
#endif

  return 0;
}


auto IsWireframe = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  static std::unordered_set <uint32_t>  null_set  = {     };
         d3d11_shader_tracking_s*       tracker   = nullptr;
         std::unordered_set <uint32_t>& wireframe = null_set;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker   = &SK_D3D11_Shaders.vertex.tracked;
      wireframe =  SK_D3D11_Shaders.vertex.wireframe;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker   = &SK_D3D11_Shaders.pixel.tracked;
      wireframe =  SK_D3D11_Shaders.pixel.wireframe;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker   = &SK_D3D11_Shaders.geometry.tracked;
      wireframe =  SK_D3D11_Shaders.geometry.wireframe;
    } break;

    case sk_shader_class::Hull:
    {
      tracker   = &SK_D3D11_Shaders.hull.tracked;
      wireframe =  SK_D3D11_Shaders.hull.wireframe;
    } break;

    case sk_shader_class::Domain:
    {
      tracker   = &SK_D3D11_Shaders.domain.tracked;
      wireframe =  SK_D3D11_Shaders.domain.wireframe;
    } break;

    case sk_shader_class::Compute:
    {
      return false;
    } break;
  }

  if (tracker->crc32c == crc32c && tracker->wireframe)
    return true;

  return (wireframe.count (crc32c) != 0);
};

auto IsOnTop = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  static std::unordered_set <uint32_t>  null_set = {     };
         d3d11_shader_tracking_s*       tracker  = nullptr;
         std::unordered_set <uint32_t>& on_top   = null_set;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker = &SK_D3D11_Shaders.vertex.tracked;
      on_top  =  SK_D3D11_Shaders.vertex.on_top;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker = &SK_D3D11_Shaders.pixel.tracked;
      on_top  =  SK_D3D11_Shaders.pixel.on_top;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker = &SK_D3D11_Shaders.geometry.tracked;
      on_top  =  SK_D3D11_Shaders.geometry.on_top;
    } break;

    case sk_shader_class::Hull:
    {
      tracker = &SK_D3D11_Shaders.hull.tracked;
      on_top  =  SK_D3D11_Shaders.hull.on_top;
    } break;

    case sk_shader_class::Domain:
    {
      tracker = &SK_D3D11_Shaders.domain.tracked;
      on_top  =  SK_D3D11_Shaders.domain.on_top;
    } break;

    case sk_shader_class::Compute:
    {
      return false;
    } break;
  }

  if (tracker->crc32c == crc32c && tracker->on_top)
    return true;

  return (on_top.count (crc32c) != 0);
};

auto IsSkipped = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  static std::unordered_set <uint32_t>  null_set  = {     };
         d3d11_shader_tracking_s*       tracker   = nullptr;
         std::unordered_set <uint32_t>& blacklist = null_set;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker   = &SK_D3D11_Shaders.vertex.tracked;
      blacklist =  SK_D3D11_Shaders.vertex.blacklist;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker   = &SK_D3D11_Shaders.pixel.tracked;
      blacklist =  SK_D3D11_Shaders.pixel.blacklist;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker   = &SK_D3D11_Shaders.geometry.tracked;
      blacklist =  SK_D3D11_Shaders.geometry.blacklist;
    } break;

    case sk_shader_class::Hull:
    {
      tracker   = &SK_D3D11_Shaders.hull.tracked;
      blacklist =  SK_D3D11_Shaders.hull.blacklist;
    } break;

    case sk_shader_class::Domain:
    {
      tracker   = &SK_D3D11_Shaders.domain.tracked;
      blacklist =  SK_D3D11_Shaders.domain.blacklist;
    } break;

    case sk_shader_class::Compute:
    {
      tracker   = &SK_D3D11_Shaders.compute.tracked;
      blacklist =  SK_D3D11_Shaders.compute.blacklist;
    } break;
  }

  if (tracker->crc32c == crc32c && tracker->cancel_draws)
    return true;

  return (blacklist.count (crc32c) != 0);
};

struct shader_disasm_s {
  std::string       header = "";
  std::string       code   = "";
  std::string       footer = "";

  struct constant_buffer
  {
    std::string      name  = "";

    struct variable
    {
      std::string    name  = "";
    };

    std::vector <variable> variables;

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


extern bool
SK_ImGui_IsItemRightClicked (ImGuiIO& io = ImGui::GetIO ());

auto ShaderMenu =
[&] ( std::unordered_set <uint32_t>&                          blacklist,
      SK_D3D11_KnownShaders::conditional_blacklist_t&    cond_blacklist,
      std::vector       <ID3D11ShaderResourceView *>&   used_resources,
      std::set          <ID3D11ShaderResourceView *>& set_of_resources,
      uint32_t                                                   shader )
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

  auto DrawRSV = [&](ID3D11ShaderResourceView* pSRV)
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

          std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

          temp_resources.emplace_back (pSRV);
                                       pSRV->AddRef ();

          ImGui::Image ( pSRV,       ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
    ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                     ImVec2  (0,0),             ImVec2  (1,1),
                                     ImColor (255,255,255,255), ImColor (242,242,13,255) );
          ImGui::TreePop  ();
        }
      }
    }
  };

  std::set <uint32_t> listed;

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
        if (SK_D3D11_Textures.Textures_2D.count  (pTex2D) &&  SK_D3D11_Textures.Textures_2D [pTex2D].crc32c != 0x0)
        {
          crc32c = SK_D3D11_Textures.Textures_2D [pTex2D].crc32c;
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

      DrawRSV (it);

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
        if (SK_D3D11_Textures.Textures_2D.count  (pTex2D) && SK_D3D11_Textures.Textures_2D  [pTex2D].crc32c != 0x0)
        {
          crc32c = SK_D3D11_Textures.Textures_2D [pTex2D].crc32c;
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
          DrawRSV (it);
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

void
SK_LiveTextureView (bool& can_scroll)
{
  ImGuiIO& io (ImGui::GetIO ());

  std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

  const float font_size           = ImGui::GetFont ()->FontSize * io.FontGlobalScale;
  const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  static float last_ht    = 256.0f;
  static float last_width = 256.0f;

  struct list_entry_s {
    std::string          name      = "";
    uint32_t             tag       = 0UL;
    uint32_t             crc32c    = 0UL;
    bool                 injected  = false;
    D3D11_TEXTURE2D_DESC desc      = { };
    D3D11_TEXTURE2D_DESC orig_desc = { };
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

  ImGui::BeginChild ( ImGui::GetID ("ToolHeadings##TexturesD3D11"), ImVec2 (font_size * 66.0f, font_size * 2.5f), false,
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
      // Relatively immutable textures
      for (auto& it : SK_D3D11_Textures.HashMap_2D)
      {
        SK_AutoCriticalSection critical1 (&it.mutex);

        for (auto& it2 : it.entries)
        {
          size_t count = 0;

          {
            SK_AutoCriticalSection critical_refs (&cache_cs);
            count = SK_D3D11_Textures.TexRefs_2D.count (it2.second);
          }

          if (count > 0)
          {
            list_entry_s entry = { };

            entry.crc32c = 0;
            entry.tag    = it2.first;
            entry.name   = "DontCare";
            entry.pTex   = it2.second;

            if (SK_D3D11_TextureIsCached (it2.second))
            {
              const SK_D3D11_TexMgr::tex2D_descriptor_s& desc =
                SK_D3D11_Textures.Textures_2D [it2.second];

              entry.desc      = desc.desc;
              entry.orig_desc = desc.orig_desc;
              entry.size      = desc.mem_size;
              entry.crc32c    = desc.crc32c;
              entry.injected  = desc.injected;
            }

            else if (it2.second != nullptr)
            {
              it2.second->GetDesc (&entry.desc);
              entry.orig_desc = entry.desc;
              entry.size = 0;
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
        bool active =
          used_textures.count (it.second.pTex) != 0;

        if (active || tex_set == 0)
        {
          char szDesc [48] = { };

          sprintf (szDesc, "%08x##Texture2D_D3D11", it.second.crc32c);

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

    if (! SK_D3D11_Textures.TexRefs_2D.count (tracked_texture))
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

  if (! list_contents.empty ())
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

    D3D11_TEXTURE2D_DESC tex_desc  = entry.orig_desc;
    size_t               tex_size  = 0UL;
    float                load_time = 0.0f;

    ID3D11Texture2D* pTex =
      SK_D3D11_Textures.getTexture2D ((uint32_t)entry.tag, &tex_desc, &tex_size, &load_time);

    bool staged = false;

    if (pTex != nullptr)
    {
      // Get the REAL format, not the one the engine knows about through texture cache
      pTex->GetDesc (&tex_desc);

      if (lod_list_dirty)
      {
        UINT w = tex_desc.Width;
        UINT h = tex_desc.Height;

        char* pszLODList = lod_list;

        for ( UINT i = 0 ; i < tex_desc.MipLevels ; i++ )
        {
          int len =
            sprintf (pszLODList, "LOD%lu: (%ux%u)", i, std::max (1U, w >> i), std::max (1U, h >> i));

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
      };

      srv_desc.Texture2D.MipLevels       = 1;
      srv_desc.Texture2D.MostDetailedMip = 0;

      CComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

      if (pDev != nullptr)
      {
#if 0
        ImVec4 border_color = config.textures.highlight_debug_tex ? 
                                ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                  (__remap_textures && has_alternate) ? ImVec4 (0.5f,  0.5f,  0.5f, 1.0f) :
                                                                        ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#else
        ImVec4 border_color = entry.injected ? ImVec4 (0.3f,  0.3f,  1.0f, 1.0f) :
                                               ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#endif

        ImGui::PushStyleColor (ImGuiCol_Border, border_color);

        float scale_factor = 1.0f;

        float content_avail_y = (ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y) / scale_factor;
        float content_avail_x = (ImGui::GetWindowContentRegionMax ().x - ImGui::GetWindowContentRegionMin ().x) / scale_factor;
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
                                   ReadAcquire (&SK_D3D11_Textures.Textures_2D [pTex].hits) );
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
              SK_TLS_Bottom  ()->texture_management.injection_thread = true;
              {
                SK_D3D11_DumpTexture2D (pTex, entry.crc32c);
              }
              SK_TLS_Bottom  ()->texture_management.injection_thread = false;
            }
          }

          else
          {
            if ( ImGui::Button ("  Delete Dumped Texture from Disk  ###DumpTexture") )
            {
              SK_D3D11_DeleteDumpedTexture (entry.crc32c);
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
             effective_width  != (float)(tex_desc.Width >> lod) )
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
          ImVec2 uv0 (flip_horizontal ? 1.0f : 0.0f, flip_vertical ? 1.0f : 0.0f);
          ImVec2 uv1 (flip_horizontal ? 0.0f : 1.0f, flip_vertical ? 0.0f : 1.0f);

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
        }
        ImGui::EndChild        ();
        ImGui::EndGroup        ();
        last_ht = ImGui::GetItemRectSize ().y;
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

void
SK_D3D11_ClearShaderState (void)
{
  for (int i = 0; i < 6; i++)
  {
    auto shader_record =
    [&]
    {
      switch (i)
      {
        default:
        case 0:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex;
        case 1:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel;
        case 2:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry;
        case 3:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull;
        case 4:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain;
        case 5:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute;
      }
    };

    shader_record ()->blacklist.clear              ();
    shader_record ()->blacklist_if_texture.clear   ();
    shader_record ()->wireframe.clear              ();
    shader_record ()->on_top.clear                 ();
    shader_record ()->trigger_reshade.before.clear ();
    shader_record ()->trigger_reshade.after.clear  ();
  }
};

void
SK_D3D11_LoadShaderState (bool clear = true)
{
  auto wszShaderConfigFile =
    SK_FormatStringW ( LR"(%s\d3d11_shaders.ini)",
                         SK_GetConfigPath () );

  if (GetFileAttributesW (wszShaderConfigFile.c_str ()) == INVALID_FILE_ATTRIBUTES)
  {
    // No shader config file, nothing to do here...
    return;
  }

  std::unique_ptr <iSK_INI> d3d11_shaders_ini (
    SK_CreateINI (wszShaderConfigFile.c_str ())
  );

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
  } draw_states [7];

  auto shader_class_idx = [](std::wstring name)
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

      _snwscanf ((*sec).first.c_str (), 31, L"DrawState.%s", wszClass);

      shader_class =
        shader_class_idx (wszClass);

      for ( auto it : (*sec).second.keys )
      {
        int                                 shader = 0;
        swscanf (it.first.c_str (), L"%x", &shader);

        wchar_t* wszState =
          _wcsdup (it.second.c_str ());

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

            draw_states [shader_class].disable_if_texture [shader].insert (crc32c);
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
  }

  for (int i = 0; i < 6; i++)
  {
    auto shader_record =
      [&]
      {
        switch (i)
        {
          default:
          case 0:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute;
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
  }

  SK_D3D11_StateMachine.enable = true;
}


void
SK_D3D11_DumpShaderState (void)
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

  for (int i = 0; i < 6; i++)
  {
    auto shader_record =
      [&]
      {
        switch (i)
        {
          default:
          case 0:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute;
        }
      };

    iSK_INISection& sec =
      d3d11_shaders_ini->get_section_f (L"DrawState.%s", shader_class_name (i));

    std::set <uint32_t> shaders;

    shaders.insert ( shader_record ()->blacklist.begin              (), shader_record ()->blacklist.end              () );
    shaders.insert ( shader_record ()->wireframe.begin              (), shader_record ()->wireframe.end              () );
    shaders.insert ( shader_record ()->on_top.begin                 (), shader_record ()->on_top.end                 () );
    shaders.insert ( shader_record ()->trigger_reshade.before.begin (), shader_record ()->trigger_reshade.before.end () );
    shaders.insert ( shader_record ()->trigger_reshade.after.begin  (), shader_record ()->trigger_reshade.after.end  () );

    for (auto it : shader_record ()->blacklist_if_texture)
    {
      shaders.insert ( it.first );
    }

    for ( auto it : shaders )
    {
      std::queue <std::wstring> states;

      if (shader_record ()->blacklist.count (it))
        states.emplace (L"Disable");

      if (shader_record ()->wireframe.count (it))
        states.emplace (L"Wireframe");

      if (shader_record ()->on_top.count (it))
        states.emplace (L"OnTop");

      if (shader_record ()->blacklist_if_texture.count (it))
      {
        for ( auto it2 : shader_record ()->blacklist_if_texture [it] )
        {
          states.emplace (SK_FormatStringW (L"DisableIfTexture=%x", it2));
        }
      }

      if (shader_record ()->trigger_reshade.before.count (it))
      {
        states.emplace (L"TriggerReShade");
      }

      if (shader_record ()->trigger_reshade.after.count (it))
      {
        states.emplace (L"TriggerReShadeAfter");
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
  std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader);

  ImGuiIO& io (ImGui::GetIO ());

  ImGui::PushID (static_cast <int> (shader_type));

  static float last_width = 256.0f;
  const  float font_size  = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  struct shader_class_imp_s
  {
    std::vector <std::string> contents;
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
      d3d11_shader_tracking_s*
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

  d3d11_shader_tracking_s*
    tracker = GetShaderTracker (shader_type);

  auto GetShaderSet =
    [](sk_shader_class& type) ->
      std::set <uint32_t>&
      {
        static std::set <uint32_t> set  [6];
        static size_t              size [6];

        switch (type)
        {
          case sk_shader_class::Vertex:
          {
            if (size [0] == SK_D3D11_Shaders.vertex.descs.size ())
              return set [0];

            for (auto const& vertex_shader : SK_D3D11_Shaders.vertex.descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( vertex_shader.first != 0xb42ede74 &&
                   vertex_shader.first != 0x1f8c62dc )
              {
                if (vertex_shader.first > 0x00000000) set [0].emplace (vertex_shader.first);
              }
            }

                   size [0] = SK_D3D11_Shaders.vertex.descs.size ();
            return set  [0];
          } break;

          case sk_shader_class::Pixel:
          {
            if (size [1] == SK_D3D11_Shaders.pixel.descs.size ())
              return set [1];

            for (auto const& pixel_shader : SK_D3D11_Shaders.pixel.descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( pixel_shader.first != 0xd3af3aa0 &&
                   pixel_shader.first != 0xb04a90ba )
              {
                if (pixel_shader.first > 0x00000000) set [1].emplace (pixel_shader.first);
              }
            }

                   size [1] = SK_D3D11_Shaders.pixel.descs.size ();
            return set  [1];
          } break;

          case sk_shader_class::Geometry:
          {
            if (size [2] == SK_D3D11_Shaders.geometry.descs.size ())
              return set [2];

            for (auto const& geometry_shader : SK_D3D11_Shaders.geometry.descs)
            {
              if (geometry_shader.first > 0x00000000) set [2].emplace (geometry_shader.first);
            }

                   size [2] = SK_D3D11_Shaders.geometry.descs.size ();
            return set  [2];
          } break;

          case sk_shader_class::Hull:
          {
            if (size [3] == SK_D3D11_Shaders.hull.descs.size ())
              return set [3];

            for (auto const& hull_shader : SK_D3D11_Shaders.hull.descs)
            {
              if (hull_shader.first > 0x00000000) set [3].emplace (hull_shader.first);
            }

                   size [3] = SK_D3D11_Shaders.hull.descs.size ();
            return set  [3];
          } break;

          case sk_shader_class::Domain:
          {
            if (size [4] == SK_D3D11_Shaders.domain.descs.size ())
              return set [4];

            for (auto const& domain_shader : SK_D3D11_Shaders.domain.descs)
            {
              if (domain_shader.first > 0x00000000) set [4].emplace (domain_shader.first);
            }

                   size [4] = SK_D3D11_Shaders.domain.descs.size ();
            return set  [4];
          } break;

          case sk_shader_class::Compute:
          {
            if (size [5] == SK_D3D11_Shaders.compute.descs.size ())
              return set [5];

            for (auto const& compute_shader : SK_D3D11_Shaders.compute.descs)
            {
              if (compute_shader.first > 0x00000000) set [5].emplace (compute_shader.first);
            }

                   size [5] = SK_D3D11_Shaders.compute.descs.size ();
            return set  [5];
          } break;
        }

        return set [0];
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

  auto GetShaderBlacklistEx =
    [&](sk_shader_class& type)->
      SK_D3D11_KnownShaders::conditional_blacklist_t&
      {
        static SK_D3D11_KnownShaders::conditional_blacklist_t invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return SK_D3D11_Shaders.vertex.blacklist_if_texture;
          case sk_shader_class::Pixel:    return SK_D3D11_Shaders.pixel.blacklist_if_texture;
          case sk_shader_class::Geometry: return SK_D3D11_Shaders.geometry.blacklist_if_texture;
          case sk_shader_class::Hull:     return SK_D3D11_Shaders.hull.blacklist_if_texture;
          case sk_shader_class::Domain:   return SK_D3D11_Shaders.domain.blacklist_if_texture;
          case sk_shader_class::Compute:  return SK_D3D11_Shaders.compute.blacklist_if_texture;
          default:                        return invalid;
        }
      };

  auto GetShaderUsedResourceViews =
    [&](sk_shader_class& type)->
      std::vector <ID3D11ShaderResourceView *>&
      {
        static std::vector <ID3D11ShaderResourceView *> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return SK_D3D11_Shaders.vertex.tracked.used_views;
          case sk_shader_class::Pixel:    return SK_D3D11_Shaders.pixel.tracked.used_views;
          case sk_shader_class::Geometry: return SK_D3D11_Shaders.geometry.tracked.used_views;
          case sk_shader_class::Hull:     return SK_D3D11_Shaders.hull.tracked.used_views;
          case sk_shader_class::Domain:   return SK_D3D11_Shaders.domain.tracked.used_views;
          case sk_shader_class::Compute:  return SK_D3D11_Shaders.compute.tracked.used_views;
          default:                        return invalid;
        }
      };

  auto GetShaderResourceSet =
    [&](sk_shader_class& type)->
      std::set <ID3D11ShaderResourceView *>&
      {
        static std::set <ID3D11ShaderResourceView *> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return SK_D3D11_Shaders.vertex.tracked.set_of_views;
          case sk_shader_class::Pixel:    return SK_D3D11_Shaders.pixel.tracked.set_of_views;
          case sk_shader_class::Geometry: return SK_D3D11_Shaders.geometry.tracked.set_of_views;
          case sk_shader_class::Hull:     return SK_D3D11_Shaders.hull.tracked.set_of_views;
          case sk_shader_class::Domain:   return SK_D3D11_Shaders.domain.tracked.set_of_views;
          case sk_shader_class::Compute:  return SK_D3D11_Shaders.compute.tracked.set_of_views;
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
          size_t last_size     = 0; // Determines dirty state

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

  unsigned int& last_sel      =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].last_sel;
  bool&         sel_changed   =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].sel_changed;
  bool*         hide_inactive = &shader_state [sk_shader_state_s::ClassToIdx (shader_type)].hide_inactive;
  unsigned int& active_frames =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].active_frames;
  size_t&       last_size     =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].last_size;

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

    if (shader_type  != sk_shader_class::Compute)
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
    }
  }

  if (shaders.size () != last_size)
  {
    list->dirty = true;
    last_size   = shaders.size ();
  }

  if (true || list->dirty || GetShaderChange (shader_type) != 0)
  {
        list->sel = 0;
    int idx       = 0;
        list->contents.clear ();

    for ( auto it : shaders )
    {
      char szDesc [64] = { };

      const bool disabled =
        ( GetShaderBlacklist (shader_type).count (it) != 0 );

      sprintf (szDesc, "%s%08lx", disabled ? "*" : " ", it);

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

  if (! shaders.empty ())
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
      if (list->sel == 0)                   // Can't go lower
        dir = std::max (0, dir);
      if (list->sel >= shaders.size () - 1) // Can't go higher
        dir = std::min (0, dir);

      int last_active = list->sel;

      do
      {
        list->sel += dir;

        if (*hide_inactive && list->sel <= shaders.size ())
        {
          SK_D3D11_ShaderDesc& rDesc =
            GetShaderDesc (shader_type, (uint32_t)shaders [list->sel]);

          if (rDesc.usage.last_frame <= SK_GetFramesDrawn () - active_frames)
          {
            continue;
          }
        }

        last_active = list->sel;

        break;
      } while (list->sel > 0 && list->sel < shaders.size () - 1);

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

    for ( UINT line = 0; line < shaders.size (); line++ )
    {
      SK_D3D11_ShaderDesc& rDesc =
        GetShaderDesc (shader_type, (uint32_t)shaders [line]);

      bool active =
        ( rDesc.usage.last_frame > SK_GetFramesDrawn () - active_frames );

      if (IsSkipped   (shader_type, shaders [line]))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
      }

      else if (IsOnTop     (shader_type, shaders [line]))
      {
       ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
      }

      else if (IsWireframe (shader_type, shaders [line]))
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

      if (line == list->sel)
      {
        bool selected = true;

        if (active || (! *hide_inactive))
          ImGui::Selectable (list->contents [line].c_str (), &selected);

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

      if (active || (! *hide_inactive))
      {
        ImGui::PushID ((uint32_t)shaders [line]);

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
                       (uint32_t)shaders [line] );
          list->dirty = true;

          ImGui::EndPopup  ();
        }
        ImGui::PopID       ();
      }
      ImGui::PopStyleColor ();
    }

    CComPtr <ID3DBlob>               pDisasm  = nullptr;
    CComPtr <ID3D11ShaderReflection> pReflect = nullptr;

    HRESULT hr = E_FAIL;

    if (ReadAcquire ((volatile LONG *)&tracker->crc32c) != 0)
    {
      switch (shader_type)
      {
        case sk_shader_class::Vertex:
        {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_vs (cs_shader_vs);
            hr = D3DDisassemble ( SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.size (),
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
        } break;

        case sk_shader_class::Pixel:
        {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_ps (cs_shader_ps);
            hr = D3DDisassemble ( SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.size (),
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
        } break;

        case sk_shader_class::Geometry:
        {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_gs (cs_shader_gs);
            hr = D3DDisassemble ( SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
        } break;

        case sk_shader_class::Hull:
        {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_hs (cs_shader_hs);
            hr = D3DDisassemble ( SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
        } break;

        case sk_shader_class::Domain:
        {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_ds (cs_shader_ds);
            hr = D3DDisassemble ( SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
        } break;

        case sk_shader_class::Compute:
        {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_cs (cs_shader_cs);
            hr = D3DDisassemble ( SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);
            if (SUCCEEDED (hr))
                 D3DReflect     ( SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.data (), SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect);
        } break;
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

          disassembly->emplace ( ReadAcquire ((volatile LONG *)&tracker->crc32c),
                                   shader_disasm_s { szDisasm,
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

                    (*disassembly) [ReadAcquire ((volatile LONG *)&tracker->crc32c)].cbuffers.emplace_back (cbuffer);
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


  if (ReadAcquire ((volatile LONG *)&tracker->crc32c) != 0x00 && list->sel >= 0 && list->sel < (int)list->contents.size ())
  {
    ImGui::BeginGroup ();

    int num_used_textures = 0;

    std::set <ID3D11ShaderResourceView *> unique_views;

    if (! tracker->used_views.empty ())
    {
      for ( auto it : tracker->used_views )
      {
        if (unique_views.count (it))
          continue;

        unique_views.emplace (it);

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

    snprintf (szTextures,   64, "(%#li %s)", num_used_textures, num_used_textures != 1 ? "textures" : "texture");
    snprintf (szRuntime,    32,  "%0.4f ms", tracker->runtime_ms);
    snprintf (szAvgRuntime, 32,  "%0.4f ms", tracker->runtime_ms / tracker->num_draws);

    SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShader = nullptr;

    switch (shader_type)
    {
      case sk_shader_class::Vertex:   pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.vertex;   break;
      case sk_shader_class::Pixel:    pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.pixel;    break;
      case sk_shader_class::Geometry: pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.geometry; break;
      case sk_shader_class::Hull:     pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.hull;     break;
      case sk_shader_class::Domain:   pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.domain;   break;
      case sk_shader_class::Compute:  pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders.compute;  break;
    }

    ImGui::BeginGroup   ();
    ImGui::Separator    ();

    ImGui::Columns      (2, nullptr, false);
    ImGui::BeginGroup   ( );

    bool skip =
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

      bool wire = wireframe;

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

      bool top = on_top;

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
    }
    else
    {
      ImGui::Columns  (1);
    }

    if (SK_ReShade_PresentCallback.fn != nullptr)
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
          CComPtr <ID3D11Resource>  pRes = nullptr;
          CComPtr <ID3D11Texture2D> pTex = nullptr;

          it->GetResource (&pRes);

          if (pRes && SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)) && pTex)
          {
            D3D11_TEXTURE2D_DESC desc = { };
            pTex->GetDesc      (&desc);
                           fmt = desc.Format;

            if (desc.Height > 0 && desc.Width > 0)
            {
              if (unique_views.count (it))
                continue;

              unique_views.emplace (it);

              temp_resources.emplace_back (it);
                                           it->AddRef ();

              ImGui::Image ( it,         ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
        ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                         ImVec2  (0,0),             ImVec2  (1,1),
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

void
SK_D3D11_EndFrame (void)
{
  std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);

  dwFrameTime = timeGetTime ();

  static bool first = true;

  if (first)
  {
    SK_D3D11_LoadShaderState                  (          );
    SK_D3D11_Shaders.vertex.wireframe.emplace (0x11465b40);
    first = false;
  }


  SK_D3D11_MemoryThreads.clear_active   ();
  SK_D3D11_ShaderThreads.clear_active   ();
  SK_D3D11_DrawThreads.clear_active     ();
  SK_D3D11_DispatchThreads.clear_active ();


  SK_D3D11_Shaders.reshade_triggered = false;


  SK_D3D11_Shaders.vertex.tracked.deactivate   ();
  SK_D3D11_Shaders.pixel.tracked.deactivate    ();
  SK_D3D11_Shaders.geometry.tracked.deactivate ();
  SK_D3D11_Shaders.hull.tracked.deactivate     ();
  SK_D3D11_Shaders.domain.tracked.deactivate   ();
  SK_D3D11_Shaders.compute.tracked.deactivate  ();

  for ( auto&& it : SK_D3D11_Shaders.vertex.current.views   ) { for (auto&& it2 : it.second ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.pixel.current.views    ) { for (auto&& it2 : it.second ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.geometry.current.views ) { for (auto&& it2 : it.second ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.domain.current.views   ) { for (auto&& it2 : it.second ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.hull.current.views     ) { for (auto&& it2 : it.second ) it2 = nullptr; }
  for ( auto&& it : SK_D3D11_Shaders.compute.current.views  ) { for (auto&& it2 : it.second ) it2 = nullptr; }


  tracked_rtv.clear   ();
  used_textures.clear ();

  mem_map_stats.clear ();

  // True if the disjoint query is complete and we can get the results of
  //   each tracked shader's timing
  static bool disjoint_done = false;

  CComPtr <ID3D11Device>        pDev    = nullptr;
  CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (            rb.device                                         &&
       SUCCEEDED (rb.device->QueryInterface <ID3D11Device> (&pDev)) &&
                  rb.d3d11.immediate_ctx != nullptr )
  {
    rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pDevCtx);
  }

  if (! pDevCtx)
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

  if (pDevCtx && disjoint_done)
  {
    auto GetTimerDataStart = [](ID3D11DeviceContext* dev_ctx, d3d11_shader_tracking_s::duration_s* duration, bool& success) ->
      UINT64
      {
        if (! FAILED (dev_ctx->GetData ( (ID3D11Query *)ReadPointerAcquire ((volatile PVOID *)&duration->start.async), &duration->start.last_results, sizeof UINT64, 0x00 )))
        {
          SK_COM_ValidateRelease ((IUnknown **)&duration->start.async);

          success = true;
          
          return duration->start.last_results;
        }

        success = false;

        return 0;
      };

    auto GetTimerDataEnd = [](ID3D11DeviceContext* dev_ctx, d3d11_shader_tracking_s::duration_s* duration, bool& success) ->
      UINT64
      {
        if ((ID3D11Query *)ReadPointerAcquire ((volatile PVOID *)&duration->end.async) == nullptr)
          return duration->start.last_results;

        if (! FAILED (dev_ctx->GetData ( (ID3D11Query *)ReadPointerAcquire ((volatile PVOID *)&duration->end.async), &duration->end.last_results, sizeof UINT64, 0x00 )))
        {
          SK_COM_ValidateRelease ((IUnknown **)&duration->end.async);

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

    auto AccumulateRuntimeTicks = [&](ID3D11DeviceContext* dev_ctx, d3d11_shader_tracking_s* tracker, std::unordered_set <uint32_t>& blacklist) ->
      void
      {
        std::vector <d3d11_shader_tracking_s::duration_s> rejects;

        tracker->runtime_ticks = 0ULL;

        for (auto& it : tracker->timers)
        {
          bool   success0 = false, success1 = false;
          UINT64 time0    = 0ULL,  time1    = 0ULL;

          time0 = GetTimerDataEnd   (dev_ctx, &it, success0);
          time1 = GetTimerDataStart (dev_ctx, &it, success1);

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

    AccumulateRuntimeTicks (pDevCtx, &SK_D3D11_Shaders.vertex.tracked,   SK_D3D11_Shaders.vertex.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.vertex.tracked);

    AccumulateRuntimeTicks (pDevCtx, &SK_D3D11_Shaders.pixel.tracked,    SK_D3D11_Shaders.pixel.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.pixel.tracked);

    AccumulateRuntimeTicks (pDevCtx, &SK_D3D11_Shaders.geometry.tracked, SK_D3D11_Shaders.geometry.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.geometry.tracked);

    AccumulateRuntimeTicks (pDevCtx, &SK_D3D11_Shaders.hull.tracked,     SK_D3D11_Shaders.hull.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.hull.tracked);

    AccumulateRuntimeTicks (pDevCtx, &SK_D3D11_Shaders.domain.tracked,   SK_D3D11_Shaders.domain.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.domain.tracked);

    AccumulateRuntimeTicks (pDevCtx, &SK_D3D11_Shaders.compute.tracked,  SK_D3D11_Shaders.compute.blacklist);
    CalcRuntimeMS          (&SK_D3D11_Shaders.compute.tracked);

    disjoint_done = false;
  }


  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock_vs (cs_shader_vs);
    SK_D3D11_Shaders.vertex.tracked.clear   ();
  }

  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock_ps (cs_shader_ps);
    SK_D3D11_Shaders.pixel.tracked.clear    ();
  }

  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock_gs (cs_shader_gs);
    SK_D3D11_Shaders.geometry.tracked.clear ();
  }

  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock_hs (cs_shader_hs);
    SK_D3D11_Shaders.hull.tracked.clear     ();
  }

  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock_ds (cs_shader_ds);
    SK_D3D11_Shaders.domain.tracked.clear   ();
  }

  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock_cs (cs_shader_cs);
    SK_D3D11_Shaders.compute.tracked.clear  ();
  }

  SK_D3D11_Shaders.vertex.changes_last_frame   = 0;
  SK_D3D11_Shaders.pixel.changes_last_frame    = 0;
  SK_D3D11_Shaders.geometry.changes_last_frame = 0;
  SK_D3D11_Shaders.hull.changes_last_frame     = 0;
  SK_D3D11_Shaders.domain.changes_last_frame   = 0;
  SK_D3D11_Shaders.compute.changes_last_frame  = 0;


  for (auto it : temp_resources)
    it->Release ();

  temp_resources.clear ();

  for (auto& it : SK_D3D11_RenderTargets)
    it.second.clear ();


  if (! SK_D3D11_ShowShaderModDlg ())
    SK_D3D11_StateMachine.mmio.track = false;
}


bool
SK_D3D11_ShaderModDlg (void)
{
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_gp (cs_shader);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_vs (cs_shader_vs);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_ps (cs_shader_ps);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_gs (cs_shader_gs);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_ds (cs_shader_ds);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_hs (cs_shader_hs);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_cs (cs_shader_cs);

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
    SK_D3D11_StateMachine.enable = true;

    bool can_scroll = ImGui::IsWindowFocused () && ImGui::IsMouseHoveringRect ( ImVec2 (ImGui::GetWindowPos ().x,                             ImGui::GetWindowPos ().y),
                                                                                ImVec2 (ImGui::GetWindowPos ().x + ImGui::GetWindowSize ().x, ImGui::GetWindowPos ().y + ImGui::GetWindowSize ().y) );

    ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

    ImGui::Columns (2);

    ImGui::BeginChild ( ImGui::GetID ("Render_Left_Side"), ImVec2 (0,0), false,
                          ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

    if (ImGui::CollapsingHeader ("Live Shader View", ImGuiTreeNodeFlags_DefaultOpen))
    {
#if 0
      ImGui::InputInt ("Indexed Weight",                    &SK_D3D11_ReshadeDrawFactors.indexed);
      ImGui::InputInt ("Draw Weight",                       &SK_D3D11_ReshadeDrawFactors.draw);
      ImGui::InputInt ("Auto Draw Weight",                  &SK_D3D11_ReshadeDrawFactors.auto_draw);
      ImGui::InputInt ("Indexed Instanced Weight",          &SK_D3D11_ReshadeDrawFactors.indexed_instanced);
      ImGui::InputInt ("Indexed Instanced Indirect Weight", &SK_D3D11_ReshadeDrawFactors.indexed_instanced_indirect);
      ImGui::InputInt ("Instanced Weight",                  &SK_D3D11_ReshadeDrawFactors.instanced);
      ImGui::InputInt ("Instanced Indirect Weight",         &SK_D3D11_ReshadeDrawFactors.instanced_indirect);
      ImGui::InputInt ("Dispatch Weight",                   &SK_D3D11_ReshadeDrawFactors.dispatch);
      ImGui::InputInt ("Dispatch Indirect Weight",          &SK_D3D11_ReshadeDrawFactors.dispatch_indirect);
#endif

      // This causes the stats below to update
      SK_ImGui_Widgets.d3d11_pipeline->setActive (true);

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
            {
              SK_LiveShaderClassView (shader_type, can_scroll);
            }
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

        if (ImGui::Button (" Dump Shader State "))
        {
          SK_D3D11_DumpShaderState ();
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
        SK_D3D11_StateMachine.mmio.track = true;
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
        SK_D3D11_StateMachine.mmio.track = false;

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
        ImGui::CollapsingHeader ("Live Texture View", ImGuiTreeNodeFlags_DefaultOpen);

      if (uncollapsed_tex)
      {
        SK_LiveTextureView (can_scroll);
      }

      ImGui::EndChild ();

      scale = (uncollapsed_rtv ? (1.0f * (uncollapsed_tex ? 0.5f : 1.0f)) : -1.0f);

      ImGui::BeginChild     ( ImGui::GetID ("Live_RenderTarget_View_Panel"),
                              ImVec2 ( -1.0f, scale == -1.0f ? font_size_multiline * 1.666f : ( ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y ) * scale - (scale == 1.0f ? font_size_multiline * 1.666f : 0.0f) ),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

      uncollapsed_rtv =
        ImGui::CollapsingHeader ("Live RenderTarget View", ImGuiTreeNodeFlags_DefaultOpen);

      if (uncollapsed_rtv)
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

        static std::unordered_map <ID3D11RenderTargetView *, lifetime> render_lifetime;
        static std::vector        <ID3D11RenderTargetView *>           render_textures;

        //render_textures.reserve (128);
        //render_textures.clear   ();

        for (auto&& rtl : SK_D3D11_RenderTargets )
        for (auto&& it  : rtl.second.rt_views    )
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
              return (uintptr_t)a < (uintptr_t)b;
            }
          );


          for ( auto it : render_textures )
          {
            static char szDesc [48] = { };

#ifdef _WIN64
            sprintf (szDesc, "%07llx###rtv_%p", (uintptr_t)it, it);
#else
            sprintf (szDesc, "%07lx###rtv_%p", (uintptr_t)it, it);
#endif

            list_contents.emplace_back (szDesc);
  
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
  
          if (sel >= 0 && sel < (int)render_textures.size ())
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
  
          D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
          rt_view->GetDesc (&rtv_desc);
  
          if (rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
          {
            CComPtr <ID3D11Texture2D> pTex = nullptr;
            CComPtr <ID3D11Resource>  pRes = nullptr;

            rt_view->GetResource (&pRes);

            if (pRes && SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)) && pTex)
            {
              D3D11_TEXTURE2D_DESC desc = { };
              pTex->GetDesc (&desc);

              ID3D11ShaderResourceView*       pSRV     = nullptr;
              D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };
  
              srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              srv_desc.Format                    = rtv_desc.Format;
              srv_desc.Texture2D.MipLevels       = UINT_MAX;
              srv_desc.Texture2D.MostDetailedMip =  0;

              CComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

              if (pDev != nullptr)
              {
                size_t row0  = std::max (tracked_rtv.ref_vs.size (), tracked_rtv.ref_ps.size ());
                size_t row1  =           tracked_rtv.ref_gs.size ();
                size_t row2  = std::max (tracked_rtv.ref_hs.size (), tracked_rtv.ref_ds.size ());
                size_t row3  =           tracked_rtv.ref_cs.size ();

                size_t bottom_list = row0 + row1 + row2 + row3;

                bool success =
                  SUCCEEDED (pDev->CreateShaderResourceView (pTex, &srv_desc, &pSRV));
  
                float content_avail_y = ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y;
                float content_avail_x = ImGui::GetWindowContentRegionMax ().x - ImGui::GetWindowContentRegionMin ().x;
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
                ImGui::EndGroup   (                  );
  
                ImGui::SameLine   ( );
  
                ImGui::BeginGroup (                                              );
                ImGui::Text       ( "%lux%lu",
                                      desc.Width, desc.Height/*, effective_width, effective_height, 0.9875f * content_avail_y - ((float)(bottom_list + 3) * font_size * 1.125f), content_avail_y*//*,
                                        pTex->d3d9_tex->GetLevelCount ()*/       );
                ImGui::Text       ( "%ws",
                                      SK_DXGI_FormatToStr (desc.Format).c_str () );
                ImGui::EndGroup   (                                              );
    
                if (success)
                {
                  ImGui::Separator  ( );

                  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
                  ImGui::BeginChildFrame   (ImGui::GetID ("ShaderResourceView_Frame"),
                                              ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders );

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
  
                if (bottom_list)
                {
                  ImGui::Separator  ( );

                  ImGui::BeginChild ( ImGui::GetID ("RenderTargetContributors"),
                                    ImVec2 ( -1.0f ,//std::max (font_size * 30.0f, effective_width + 24.0f),
                                             -1.0f ),
                                      true,
                                        ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NavFlattened );
  
                  if ((! tracked_rtv.ref_vs.empty ()) || (! tracked_rtv.ref_ps.empty ()))
                  {
                    ImGui::Columns (2);
  
                    if (ImGui::CollapsingHeader ("Vertex Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
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

                        bool disabled = SK_D3D11_Shaders.vertex.blacklist.count (it) != 0;
  
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
                          ShaderMenu (SK_D3D11_Shaders.vertex.blacklist, SK_D3D11_Shaders.vertex.blacklist_if_texture, SK_D3D11_Shaders.vertex.tracked.used_views, SK_D3D11_Shaders.vertex.tracked.set_of_views, it);
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

                        bool disabled = SK_D3D11_Shaders.pixel.blacklist.count (it) != 0;
  
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
                          ShaderMenu (SK_D3D11_Shaders.pixel.blacklist, SK_D3D11_Shaders.pixel.blacklist_if_texture, SK_D3D11_Shaders.pixel.tracked.used_views, SK_D3D11_Shaders.pixel.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }
  
                    ImGui::Columns   (1);
                  }
  
                  if (! tracked_rtv.ref_gs.empty ())
                  {
                    ImGui::Separator ( );
  
                    if (ImGui::CollapsingHeader ("Geometry Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
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

                        bool disabled = SK_D3D11_Shaders.geometry.blacklist.count (it) != 0;
  
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
                          ShaderMenu (SK_D3D11_Shaders.geometry.blacklist, SK_D3D11_Shaders.geometry.blacklist_if_texture, SK_D3D11_Shaders.geometry.tracked.used_views, SK_D3D11_Shaders.geometry.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }
                  }
  
                  if ((! tracked_rtv.ref_hs.empty ()) || (! tracked_rtv.ref_ds.empty ()))
                  {
                    ImGui::Separator ( );
  
                    ImGui::Columns   (2);
  
                    if (ImGui::CollapsingHeader ("Hull Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_hs )
                      {
                        bool disabled = SK_D3D11_Shaders.hull.blacklist.count (it) != 0;

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
                          ShaderMenu (SK_D3D11_Shaders.hull.blacklist, SK_D3D11_Shaders.hull.blacklist_if_texture, SK_D3D11_Shaders.hull.tracked.used_views, SK_D3D11_Shaders.hull.tracked.set_of_views, it);
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
                          ShaderMenu (SK_D3D11_Shaders.domain.blacklist, SK_D3D11_Shaders.domain.blacklist_if_texture, SK_D3D11_Shaders.domain.tracked.used_views, SK_D3D11_Shaders.domain.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }
  
                    ImGui::Columns (1);
                  }
  
                  if (! tracked_rtv.ref_cs.empty ())
                  {
                    ImGui::Separator ( );
  
                    if (ImGui::CollapsingHeader ("Compute Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      for ( auto it : tracked_rtv.ref_cs )
                      {
                        bool disabled =
                          (SK_D3D11_Shaders.compute.blacklist.count (it) != 0);

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
                          ShaderMenu (SK_D3D11_Shaders.compute.blacklist, SK_D3D11_Shaders.compute.blacklist_if_texture, SK_D3D11_Shaders.compute.tracked.used_views, SK_D3D11_Shaders.compute.tracked.set_of_views, it);
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

      ImGui::EndChild     ( );
      ImGui::EndChild     ( );
      ImGui::Columns      (1);

      ImGui::PopItemWidth ( );
    }

  ImGui::End            ( );

  SK_D3D11_StateMachine.enable = show_dlg;

  return show_dlg;
}






// Not thread-safe
__declspec (dllexport)
void
__stdcall
SKX_ImGui_RegisterDiscardableResource (IUnknown* pRes)
{
  std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_render_view);
  temp_resources.push_back (pRes);
}

bool SK_D3D11_KnownShaders::reshade_triggered = false;








#include <specialk/injection/address_cache.h>

void
CALLBACK
RunDLL_HookManager_DXGI ( HWND  hwnd,        HINSTANCE hInst,
                          LPSTR lpszCmdLine, int )
{
  UNREFERENCED_PARAMETER (hInst);
  UNREFERENCED_PARAMETER (hwnd);

  if (StrStrA (lpszCmdLine, "dump"))
  {
    extern volatile ULONG  __SK_DLL_Refs;
    InterlockedIncrement (&__SK_DLL_Refs);

    extern void
    __stdcall
    SK_EstablishRootPath (void);

    config.system.central_repository = true;
    SK_EstablishRootPath ();

    // Setup unhooked function pointers
    SK_PreInitLoadLibrary ();

    QueryPerformanceCounter_Original =
      reinterpret_cast <QueryPerformanceCounter_pfn> (
        GetProcAddress (
          GetModuleHandle ( L"kernel32.dll"),
                              "QueryPerformanceCounter" )
      );

    config.apis.d3d9.hook                        = false; config.apis.d3d9ex.hook = false;
    config.apis.OpenGL.hook                      = false;
    config.apis.NvAPI.enable                     = false;
    config.steam.preload_overlay                 = false;
    config.steam.silent                          = true;
    config.system.trace_load_library             = false;
    config.system.handle_crashes                 = false;
    config.system.central_repository             = true;
    config.system.game_output                    = false;
    config.render.dxgi.rehook_present            = false;
    config.injection.global.use_static_addresses = false;
    config.input.gamepad.hook_dinput8            = false;
    config.input.gamepad.hook_hid                = false;
    config.input.gamepad.hook_xinput             = false;

    SK_Init_MinHook        ();
    SK_ApplyQueuedHooks    ();

    SK_SetDLLRole (DLL_ROLE::DXGI);

    BOOL
    __stdcall
    SK_Attach (DLL_ROLE role);

    extern bool __SK_RunDLL_Bypass;
                __SK_RunDLL_Bypass = true;

    BOOL bRet =
      SK_Attach (DLL_ROLE::DXGI);

    if (bRet)
    {
      WaitForInit ();

      extern PresentSwapChain_pfn Present_Target;

      SK_Inject_AddressManager = new SK_Inject_AddressCacheRegistry ();
      SK_Inject_AddressManager->storeNamedAddress (L"dxgi", "IDXGISwapChain::Present", reinterpret_cast <uintptr_t> (Present_Target));
      delete SK_Inject_AddressManager;

      dll_log.Log (L"IDXGISwapChain::Present = %ph", Present_Target);

      SK::DXGI::Shutdown ();

      extern iSK_INI* dll_ini;
      DeleteFileW (dll_ini->get_filename ());
    }

    ExitProcess (0x00);
  }
}



void
SK_D3D11_ResetShaders (void)
{
  for (auto& it : SK_D3D11_Shaders.vertex.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.vertex.rev.erase   ((ID3D11VertexShader *)it.second.pShader);
      SK_D3D11_Shaders.vertex.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.pixel.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.pixel.rev.erase   ((ID3D11PixelShader *)it.second.pShader);
      SK_D3D11_Shaders.pixel.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.geometry.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.geometry.rev.erase   ((ID3D11GeometryShader *)it.second.pShader);
      SK_D3D11_Shaders.geometry.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.hull.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.hull.rev.erase   ((ID3D11HullShader *)it.second.pShader);
      SK_D3D11_Shaders.hull.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.domain.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.domain.rev.erase   ((ID3D11DomainShader *)it.second.pShader);
      SK_D3D11_Shaders.domain.descs.erase (it.first);
    }
  }

  for (auto& it : SK_D3D11_Shaders.compute.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      SK_D3D11_Shaders.compute.rev.erase   ((ID3D11ComputeShader *)it.second.pShader);
      SK_D3D11_Shaders.compute.descs.erase (it.first);
    }
  }
}