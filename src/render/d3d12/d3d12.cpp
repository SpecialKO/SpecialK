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

#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/command.h>
#include <SpecialK/config.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/log.h>

extern LARGE_INTEGER SK_QueryPerf (void);
#include <SpecialK/framerate.h>

#include <Windows.h>
#include <atlbase.h>

#define D3D12_IGNORE_SDK_LAYERS
#include <SpecialK/d3d12_interfaces.h>

#include <SpecialK/diagnostics/compatibility.h>

#include <algorithm>

LPVOID                pfnD3D12CreateDevice     = nullptr;
D3D12CreateDevice_pfn D3D12CreateDevice_Import = nullptr;

HMODULE               SK::DXGI::hModD3D12      = nullptr;

IUnknown*             g_pD3D12Dev              = nullptr;

volatile
LONG                  __d3d12_hooked           = FALSE;
volatile
LONG                  __d3d12_ready            = FALSE;

void
WaitForInitD3D12 (void)
{
  return;

  while (! InterlockedCompareExchange (&__d3d12_ready, FALSE, FALSE))
    MsgWaitForMultipleObjectsEx (0, nullptr, config.system.init_delay, QS_ALLINPUT, MWMO_ALERTABLE);
}

namespace SK
{
  namespace DXGI
  {
    struct PipelineStatsD3D12
    {
      struct StatQueryD3D12
      {
        ID3D12QueryHeap* heap   = nullptr;
        bool             active = false;
      } query;

      D3D12_QUERY_DATA_PIPELINE_STATISTICS
                 last_results   = { };
    } pipeline_stats_d3d12;
  };
};


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

  dll_log.LogEx ( true,
                    L"[  D3D 12  ]  <~> Minimum Feature Level - %s\n",
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
      SK::DXGI::StartBudgetThread ( (IDXGIAdapter **)&pAdapter );
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
    dwRenderThread =
      GetCurrentThreadId ();

    if ( ppDevice != nullptr )
    {
      if ( *ppDevice != g_pD3D12Dev )
      {
        // TODO: This isn't the right way to get the feature level
        dll_log.Log ( L"[  D3D 12  ] >> Device = %ph (Feature Level:%s)",
                        *ppDevice,
                          SK_DXGI_FeatureLevelsToStr ( 1,
                                                        (DWORD *)&MinimumFeatureLevel//(DWORD *)&ret_level
                                                     ).c_str ()
                    );

        g_pD3D12Dev =
          (IUnknown *)*ppDevice;
      }
    }
  }

  return res;
}

volatile LONG SK_D3D12_initialized = FALSE;

bool
SK_D3D12_Init (void)
{
  if (SK::DXGI::hModD3D12 == nullptr)
  {
    SK::DXGI::hModD3D12 =
      LoadLibraryW_Original (L"d3d12.dll");
  }

  if (SK::DXGI::hModD3D12 != nullptr)
  {
    if ( MH_OK == 
           SK_CreateDLLHook ( L"d3d12.dll",
                               "D3D12CreateDevice",
                                D3D12CreateDevice_Detour,
                     (LPVOID *)&D3D12CreateDevice_Import,
                            &pfnD3D12CreateDevice )
       )
    {
      return true;
    }
  }

  return false;
}

void
SK_D3D12_Shutdown (void)
{
  if (! InterlockedCompareExchange (&SK_D3D12_initialized, FALSE, TRUE))
    return;

  FreeLibrary_Original (SK::DXGI::hModD3D12);
}

void
SK_D3D12_EnableHooks (void)
{
  if (pfnD3D12CreateDevice != nullptr)
    SK_EnableHook (pfnD3D12CreateDevice);
}

unsigned int
__stdcall
HookD3D12 (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);
#ifndef _WIN64
  return 0;
#else
  config.apis.dxgi.d3d12.hook = false;

  if (! config.apis.dxgi.d3d12.hook)
    return 0;

  // This only needs to be done once...
  if (InterlockedCompareExchange (&__d3d12_hooked, TRUE, FALSE)) {
    return 0;
  }

  if (SK::DXGI::hModD3D12 != nullptr)
  {
    //bool success = SUCCEEDED (CoInitializeEx (nullptr, COINIT_MULTITHREADED))

    dll_log.Log (L"[  D3D 12  ]   Hooking D3D12");

#if 0
    CComPtr <IDXGIFactory>  pFactory  = nullptr;
    CComPtr <IDXGIAdapter>  pAdapter  = nullptr;
    CComPtr <IDXGIAdapter1> pAdapter1 = nullptr;

    HRESULT hr =
      CreateDXGIFactory_Import ( IID_PPV_ARGS (&pFactory) );

    if (SUCCEEDED (hr)) {
      pFactory->EnumAdapters (0, &pAdapter);

      if (pFactory) {
        int iver = SK_GetDXGIFactoryInterfaceVer (pFactory);

        CComPtr <IDXGIFactory1> pFactory1 = nullptr;

        if (iver > 0) {
          if (SUCCEEDED (CreateDXGIFactory1_Import ( IID_PPV_ARGS (&pFactory1) ))) {
            pFactory1->EnumAdapters1 (0, &pAdapter1);
          }
        }
      }
    }

    CComPtr <ID3D11Device>        pDevice           = nullptr;
    D3D_FEATURE_LEVEL             featureLevel;
    CComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;

    HRESULT hrx = E_FAIL;
    {
      if (pAdapter1 != nullptr) {
        D3D_FEATURE_LEVEL test_11_1 = D3D_FEATURE_LEVEL_11_1;

        hrx =
          D3D11CreateDevice_Import (
            pAdapter1,
              D3D_DRIVER_TYPE_UNKNOWN,
                nullptr,
                  0,
                    &test_11_1,
                      1,
                        D3D11_SDK_VERSION,
                          &pDevice,
                            &featureLevel,
                              &pImmediateContext );

        if (SUCCEEDED (hrx)) {
          d3d11_caps.feature_level.d3d11_1 = true;
        }
      }

      if (! SUCCEEDED (hrx)) {
        hrx =
          D3D11CreateDevice_Import (
            pAdapter,
              D3D_DRIVER_TYPE_UNKNOWN,
                nullptr,
                  0,
                    nullptr,
                      0,
                        D3D11_SDK_VERSION,
                          &pDevice,
                            &featureLevel,
                              &pImmediateContext );
      }
    }

    if (SUCCEEDED (hrx)) {
    }
#endif
    InterlockedExchange (&__d3d12_ready, TRUE);
  }

  else {
    // Disable this on future runs, because the DLL is not present
    config.apis.dxgi.d3d12.hook = false;
  }

  return 0;
#endif
}

extern void
SK_D3D11_SetPipelineStats (void* pData);

void
__stdcall
SK_D3D12_UpdateRenderStats (IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain && config.render.show))
    return;

  // Need more debug time with D3D12
  return;

#if 0
  CComPtr <ID3D12Device> dev = nullptr;

  if (SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&dev)))) {
    static CComPtr <ID3D12CommandAllocator>    cmd_alloc = nullptr;
    static CComPtr <ID3D12GraphicsCommandList> cmd_list  = nullptr;
    static CComPtr <ID3D12Resource>            query_res = nullptr;
    static CComPtr <ID3D12CommandQueue>        cmd_queue = nullptr;

    if (cmd_alloc == nullptr)
    {
      dev->CreateCommandAllocator ( D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IID_PPV_ARGS (&cmd_alloc) );

      D3D12_COMMAND_QUEUE_DESC queue_desc = { };

      queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
      queue_desc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;

      dev->CreateCommandQueue ( &queue_desc,
                                  IID_PPV_ARGS (&cmd_queue) );

      if (query_res == nullptr) {
        D3D12_HEAP_PROPERTIES heap_props = { D3D12_HEAP_TYPE_READBACK, 
                                             D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                             D3D12_MEMORY_POOL_UNKNOWN,
                                             0xFF,
                                             0xFF };

        D3D12_RESOURCE_DESC res_desc = { };
        res_desc.Width     = sizeof D3D12_QUERY_DATA_PIPELINE_STATISTICS;
        res_desc.Flags     = D3D12_RESOURCE_FLAG_NONE;
        res_desc.Alignment = 0;

        dev->CreateCommittedResource ( &heap_props,
                                       D3D12_HEAP_FLAG_NONE,
                                       &res_desc,
                                       D3D12_RESOURCE_STATE_COPY_DEST,
                                       nullptr,
                                       IID_PPV_ARGS (&query_res) );
      }
    }

    if (cmd_alloc != nullptr && cmd_list == nullptr)
    {
      dev->CreateCommandList ( 0,
                                 D3D12_COMMAND_LIST_TYPE_DIRECT,
                                   cmd_alloc,
                                     nullptr,
                                       IID_PPV_ARGS (&cmd_list) );
    }

    if (cmd_alloc == nullptr)
      return;

    SK::DXGI::PipelineStatsD3D12& pipeline_stats =
      SK::DXGI::pipeline_stats_d3d11;

    if (pipeline_stats.query.heap != nullptr) {
      if (pipeline_stats.query.active) {
        cmd_list->EndQuery (pipeline_stats.query.heap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0);
        cmd_list->Close    ();

        cmd_queue->ExecuteCommandLists (1, (ID3D12CommandList * const*)&cmd_list);

        //dev_ctx->End (pipeline_stats.query.heap[);
        pipeline_stats.query.active = false;
      } else {
        cmd_list->ResolveQueryData ( pipeline_stats.query.heap,
                                       D3D12_QUERY_TYPE_PIPELINE_STATISTICS,
                                         0, 1,
                                           query_res,
                                             0 );

        if (true) {//hr == S_OK) {
          pipeline_stats.query.heap->Release ();
          pipeline_stats.query.heap = nullptr;

          cmd_list.Release ();
          cmd_list = nullptr;

          void *pData = nullptr;

          D3D12_RANGE range = { 0, sizeof D3D12_QUERY_DATA_PIPELINE_STATISTICS };

          query_res->Map (0, &range, &pData);

          SK_D3D11_SetPipelineStats (pData);

          static const D3D12_RANGE unmap_range = { 0, 0 };

          query_res->Unmap (0, &unmap_range);
        }
      }
    }

    else {
      D3D12_QUERY_HEAP_DESC query_heap_desc {
        D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS, 1, 0xFF /* 255 GPUs seems like enough? :P */
      };

      if (SUCCEEDED (dev->CreateQueryHeap (&query_heap_desc, IID_PPV_ARGS (&pipeline_stats.query.heap)))) {
        cmd_list->BeginQuery (pipeline_stats.query.heap, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0);
        cmd_list->Close      ();

        cmd_queue->ExecuteCommandLists (1, (ID3D12CommandList * const*)&cmd_list);
        //dev->BeginQuery (Begin (pipeline_stats.query.heap);
        pipeline_stats.query.active = true;
      }
    }
  }
#endif
}