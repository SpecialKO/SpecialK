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
#define __SK_SUBSYSTEM__ L"Direct3D12"

#define _L2(w)  L ## w
#define  _L(w) _L2(w)

#define D3D12_IGNORE_SDK_LAYERS
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d12/d3d12_interfaces.h>

#include <SpecialK/render/d3d12/d3d12_device.h>
#include <SpecialK/render/d3d12/d3d12_dxil_shader.h>
#include <SpecialK/render/d3d12/d3d12_command_queue.h>
#include <SpecialK/render/d3d12/d3d12_pipeline_library.h>

#include <imgui/backends/imgui_d3d12.h>

volatile LONG  __d3d12_hooked      = FALSE;
volatile LONG SK_D3D12_initialized = FALSE;

bool
SK_D3D12_Init (void)
{
  if (SK::DXGI::hModD3D12 == nullptr)
  {
    SK::DXGI::hModD3D12 =
      SK_Modules->LoadLibraryLL (L"d3d12.dll");
  }

  if (SK::DXGI::hModD3D12 != nullptr)
  {
    if (SK_D3D12_HookDeviceCreation ())
    {
      return true;
    }
  }

  return false;
}

void
SK_D3D12_Shutdown (void)
{
  if ( FALSE !=
         InterlockedCompareExchange ( &SK_D3D12_initialized,
                                        FALSE, TRUE ) )
  {
    InterlockedExchange (&__d3d12_hooked, FALSE);
    SK_FreeLibrary (SK::DXGI::hModD3D12);
  }
}

unsigned int
__stdcall
HookD3D12 (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  if (! config.apis.dxgi.d3d12.hook)
    return 0;

  // This only needs to be done once...
  static bool        init = false;
  if (std::exchange (init,  true))
    return 0;

  if ( GetModuleHandle (L"d3d12.dll") &&
            SK::DXGI::hModD3D12 == nullptr )
                       SK_D3D12_Init ();

  if (SK::DXGI::hModD3D12 != nullptr)
  {
    //bool success = SUCCEEDED (CoInitializeEx (nullptr, COINIT_MULTITHREADED))

    dll_log->Log (L"[  D3D 12  ]   Hooking D3D12");

    bool hooked_create_device  = SK_D3D12_Init ();
    bool need_additional_hooks =
      SK_D3D12_InstallDeviceHooks       (nullptr) == false ||
      SK_D3D12_InstallCommandQueueHooks (nullptr) == false;

    if (hooked_create_device && need_additional_hooks)
    {
      SK_ComPtr <ID3D12Device> pDevice;

      if ( D3D12CreateDevice_Import != nullptr &&
// Requires Windows SDK 22000
#if defined (NTDDI_WIN10_CO) && (WDK_NTDDI_VERSION >= NTDDI_WIN10_CO)
          (SUCCEEDED (
             D3D12CreateDevice_Import (
               nullptr,     D3D_FEATURE_LEVEL_12_2,
                 __uuidof (ID3D12Device), (void **)&pDevice.p )
           ) ||
#endif
           SUCCEEDED (
             D3D12CreateDevice_Import (
               nullptr,     D3D_FEATURE_LEVEL_12_1,
                 __uuidof (ID3D12Device), (void **)&pDevice.p )
           ) ||
           SUCCEEDED (
             D3D12CreateDevice_Import (
               nullptr,     D3D_FEATURE_LEVEL_12_0,
                 __uuidof (ID3D12Device), (void **)&pDevice.p )
           ) ||
          SUCCEEDED (
             D3D12CreateDevice_Import (
               nullptr,     D3D_FEATURE_LEVEL_11_1,
                 __uuidof (ID3D12Device), (void **)&pDevice.p )
           ) ||
           SUCCEEDED (
             D3D12CreateDevice_Import (
               nullptr,     D3D_FEATURE_LEVEL_11_0,
                 __uuidof (ID3D12Device), (void **)&pDevice.p )
           )
          )
         )
      {
        SK_D3D12_InstallDeviceHooks       (pDevice.p);
        SK_D3D12_InstallCommandQueueHooks (pDevice.p);
      }
    }
  }

  else {
    // Disable this on future runs, because the DLL is not present
    //config.apis.dxgi.d3d12.hook = false;
  }

  return 0;
}

bool
SK_D3D12_HotSwapChainHook ( IDXGISwapChain3* pSwapChain,
                            ID3D12Device*    pDev12 )
{
  static bool                                               init = false;
  if ((! std::exchange (pLazyD3D12Chain, pSwapChain)) || (! init))
  {                     pLazyD3D12Device = pDev12;
    //
    // HACK for HDR RenderTargetView issues in D3D12
    //
    if (     pDev12                                  != nullptr &&
         D3D12Device_CreateRenderTargetView_Original == nullptr )
    {
      bool new_hooks = false;

      new_hooks |= SK_D3D12_InstallDeviceHooks       (pDev12);
      new_hooks |= SK_D3D12_InstallCommandQueueHooks (pDev12);

      if (new_hooks)
        SK_ApplyQueuedHooks ();

      init = true;
    }
  }

  return init;
}

void
__stdcall
SK_D3D12_UpdateRenderStats (IDXGISwapChain* pSwapChain)
{
  if (pSwapChain == nullptr || (! config.render.show))
    return;

  // Need more debug time with D3D12
  return;
}

