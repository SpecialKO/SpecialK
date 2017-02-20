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

#include <SpecialK/render_backend.h>
#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/command.h>

SK_RenderBackend __SK_RBkEnd;

SK_RenderBackend
__stdcall
SK_GetCurrentRenderBackend (void)
{
  return __SK_RBkEnd;
}

#include <SpecialK/log.h>

extern void WINAPI SK_HookGL     (void);
extern void WINAPI SK_HookVulkan (void);
extern void WINAPI SK_HookD3D9   (void);
extern void WINAPI SK_HookDXGI   (void);

void
__stdcall
SK_InitRenderBackends (void)
{
  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.D3D9Ex",
                                           new SK_IVarStub <bool> (&config.apis.d3d9ex.hook ) );
  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.D3D9",
                                           new SK_IVarStub <bool> (&config.apis.d3d9.hook ) );

  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.D3D11",
                                           new SK_IVarStub <bool> (&config.apis.dxgi.d3d11.hook ) );
  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.D3D12",
                                           new SK_IVarStub <bool> (&config.apis.dxgi.d3d12.hook ) );

  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.OpenGL",
                                           new SK_IVarStub <bool> (&config.apis.OpenGL.hook ) );

  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.Vulkan",
                                           new SK_IVarStub <bool> (&config.apis.Vulkan.hook ) );
}

void
SK_BootD3D9 (void)
{
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as d3d9.dll
  if (SK_GetDLLRole () == DLL_ROLE::D3D9)
    config.apis.d3d9.hook = true;

  if (! (config.apis.d3d9.hook || config.apis.d3d9ex.hook))
    return;

  //
  // SANITY CHECK: D3D9 must be enabled to hook D3D9Ex...
  //
  if (config.apis.d3d9ex.hook && (! config.apis.d3d9.hook))
    config.apis.d3d9.hook = true;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 9 (d3d9.dll) ] <!>");

  SK_HookD3D9 ();
}

extern HMODULE backend_dll;

void
SK_BootDXGI (void)
{
  while (backend_dll == 0) {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (dxgi.dll) ***");
    Sleep (100UL);
  }

  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as dxgi.dll
  if (SK_GetDLLRole () == DLL_ROLE::DXGI)
    config.apis.dxgi.d3d11.hook = true;

  if (! (config.apis.dxgi.d3d11.hook || config.apis.dxgi.d3d12.hook))
    return;

  //
  // TEMP HACK: D3D11 must be enabled to hook D3D12...
  //
  if (config.apis.dxgi.d3d12.hook && (! config.apis.dxgi.d3d11.hook))
    config.apis.dxgi.d3d11.hook = true;

  dll_log.Log (L"[API Detect]  <!> [    Bootstrapping DXGI (dxgi.dll)    ] <!>");

  SK_HookDXGI ();
}


void
SK_BootOpenGL (void)
{
#ifndef SK_BUILD__INSTALLER
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as OpenGL32.dll
  if (SK_GetDLLRole () == DLL_ROLE::OpenGL)
    config.apis.OpenGL.hook = true;

  if (! config.apis.OpenGL.hook)
    return;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping OpenGL (OpenGL32.dll) ] <!>");

  SK_HookGL ();
#endif
}


void
SK_BootVulkan (void)
{
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as vulkan-1.dll
  if (SK_GetDLLRole () == DLL_ROLE::Vulkan)
    config.apis.Vulkan.hook = true;

  if (! config.apis.Vulkan.hook)
    return;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Vulkan 1.x (vulkan-1.dll) ] <!>");

  SK_HookVulkan ();
}