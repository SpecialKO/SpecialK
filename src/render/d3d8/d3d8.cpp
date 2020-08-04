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

#include <corecrt.h>

#ifndef _WIN64

#include <SpecialK/render/d3d8/d3d8_backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/backend.h>

volatile LONG      __d3d8_ready  = FALSE;

unsigned int
__stdcall
HookD3D8 (LPVOID user);

typedef IUnknown*
(STDMETHODCALLTYPE *Direct3DCreate8PROC)(  UINT           SDKVersion);

Direct3DCreate8PROC      Direct3DCreate8_Import       = nullptr;

typedef void (WINAPI *finish_pfn)(void);

void
WINAPI
WaitForInit_D3D8 (void)
{
  if (Direct3DCreate8_Import == nullptr)
  {
    SK_RunOnce (SK_BootD3D8 ());
  }

  if (SK_TLS_Bottom ()->d3d8->ctx_init_thread)
    return;

  // This is a hybrid spin; it will spin for up to 250 iterations before sleeping
  SK_Thread_SpinUntilFlagged (&__d3d8_ready);
}

__declspec (noinline)
IUnknown*
STDMETHODCALLTYPE
Direct3DCreate8 (UINT SDKVersion)
{
  WaitForInit_D3D8 ();
  WaitForInit      ();

  dll_log->Log ( L"[   D3D8   ] [!] %s (%lu) - "
                 L"%s",
                   L"Direct3DCreate8",
                     SDKVersion,
                       SK_SummarizeCaller ().c_str () );

  if (Direct3DCreate8_Import)
    return Direct3DCreate8_Import (SDKVersion);

  return nullptr;
}

void
WINAPI
SK_HookD3D8 (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchangeAcquire (&hooked, TRUE, FALSE))
  {
    HMODULE hBackend =
      (SK_GetDLLRole () & DLL_ROLE::D3D8) ? backend_dll :
                                      SK_GetModuleHandle (L"d3d8.dll");

    dll_log->Log (L"[   D3D8   ] Importing Direct3DCreate8.......");
    dll_log->Log (L"[   D3D8   ] ================================");

    if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"d3d8.dll"))
    {
      Direct3DCreate8_Import =  \
        (Direct3DCreate8PROC)SK_GetProcAddress  (hBackend, "Direct3DCreate8");

      SK_LOG0 ( ( L"  Direct3DCreate8:   %s",
                    SK_MakePrettyAddress (Direct3DCreate8_Import).c_str ()  ),
                  L"   D3D8   " );
    }

    else if (hBackend != nullptr)
    {
      LPVOID pfnDirect3DCreate8 = nullptr;

      const bool bProxy =
        ( SK_GetModuleHandle (L"d3d8.dll") != hBackend );


      if ( MH_OK ==
              SK_CreateDLLHook2 (      L"d3d8.dll",
                                        "Direct3DCreate8",
                                         Direct3DCreate8,
                static_cast_p2p <void> (&Direct3DCreate8_Import),
                                     &pfnDirect3DCreate8 )
          )
      {
        MH_QueueEnableHook (pfnDirect3DCreate8);

        if (bProxy)
        {
          (Direct3DCreate8_Import) =  \
            reinterpret_cast <Direct3DCreate8PROC> (
              SK_GetProcAddress (hBackend, "Direct3DCreate8")
            );
        }

        SK_LOG0 ( ( L"  Direct3DCreate8:   %s  { Hooked }",
                      SK_MakePrettyAddress (pfnDirect3DCreate8).c_str ()  ),
                    L"   D3D8   " );
      }
    }

    imports->dgvoodoo_d3d8 = new import_s ();
    imports->dgvoodoo_d3d8->hLibrary     = hBackend;
    imports->dgvoodoo_d3d8->name         = L"API Support Plug-In";
    imports->dgvoodoo_d3d8->product_desc = SK_GetDLLVersionStr (SK_GetModuleFullName (hBackend).c_str ());

    HookD3D8            (nullptr);
    SK_BootDXGI         (       );
    SK_ApplyQueuedHooks (       );

    // Load user-defined DLLs (Plug-In)
    SK_RunLHIfBitness ( 64, SK_LoadPlugIns64 (),
                            SK_LoadPlugIns32 () );

    InterlockedIncrementRelease (&hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}

void
WINAPI
d3d8_init_callback (finish_pfn finish)
{
  if (! SK_IsHostAppSKIM ())
  {
    SK_BootD3D8 ();

    SK_Thread_SpinUntilFlagged (&__d3d8_ready);
  }

  finish ();
}


bool
SK::D3D8::Startup (void)
{
  const bool ret =
    SK_StartupCore (L"d3d8", d3d8_init_callback);

  return ret;
}

bool
SK::D3D8::Shutdown (void)
{
  return SK_ShutdownCore (L"d3d8");
}


unsigned int
__stdcall
HookD3D8 (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  if (! config.apis.d3d8.hook)
  {
    return 0;
  }

  extern bool __SK_bypass;
  if (__SK_bypass || ReadAcquire (&__d3d8_ready))
  {
    return 0;
  }

  SK_AutoCOMInit auto_com;
  {
    InterlockedExchange (&__d3d8_ready, TRUE);

    if (! (SK_GetDLLRole () & DLL_ROLE::DXGI))
      SK::DXGI::StartBudgetThread_NoAdapter ();
  }

  return 0;
}
#else
#include <Unknwn.h>
__declspec (noinline)
IUnknown*
STDMETHODCALLTYPE
Direct3DCreate8 (UINT SDKVersion)
{
  UNREFERENCED_PARAMETER (SDKVersion);
  return nullptr;
}
#endif