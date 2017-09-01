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

typedef void* LPDIRECTDRAW;
typedef void* LPDDENUMCALLBACK;
typedef void* LPDDENUMCALLBACKEX;

#ifndef _WIN64

#include <SpecialK/ddraw_backend.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/render_backend.h>

#include <SpecialK/core.h>
#include <SpecialK/log.h>
#include <SpecialK/import.h>

#include <SpecialK/config.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include <atlbase.h>
#include <comdef.h>

#include <SpecialK/log.h>
#include <SpecialK/utility.h>
#include <SpecialK/command.h>
#include <SpecialK/hooks.h>
#include <SpecialK/window.h>
#include <SpecialK/steam_api.h>

#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/compatibility.h>

         import_s* dgvoodoo_ddraw  = nullptr;
         import_s* dgvoodoo_d3dimm = nullptr;
volatile LONG      __ddraw_ready   = FALSE;

unsigned int
__stdcall
HookDDraw (LPVOID user);

void
WINAPI
WaitForInit_DDraw (void)
{
  while (! InterlockedCompareExchange (&__ddraw_ready, FALSE, FALSE))
    MsgWaitForMultipleObjectsEx (0, nullptr, config.system.init_delay, QS_ALLINPUT, MWMO_ALERTABLE);
}

typedef void (WINAPI *finish_pfn)(void);

typedef HRESULT (WINAPI *DirectDrawCreate_pfn)(
  _In_  GUID         FAR *lpGUID,
 _Out_ LPDIRECTDRAW FAR *lplpDD,
 _In_  IUnknown     FAR *pUnkOuter
);

typedef HRESULT (WINAPI *DirectDrawCreateEx_pfn)(
  _In_  GUID       FAR *lpGUID,
  _Out_ LPVOID         *lplpDD,
  _In_  REFIID          iid,
  _In_  IUnknown   FAR *pUnkOuter
);

typedef HRESULT (WINAPI *DirectDrawEnumerate_pfn)(
  _In_ LPDDENUMCALLBACK lpCallback,
  _In_ LPVOID           lpContext
);

typedef HRESULT (WINAPI *DirectDrawEnumerateEx_pfn)(
  _In_ LPDDENUMCALLBACKEX lpCallback,
  _In_ LPVOID             lpContext,
  _In_ DWORD              dwFlags
);

DirectDrawCreate_pfn      DirectDrawCreate_Import       = nullptr;
DirectDrawCreateEx_pfn    DirectDrawCreateEx_Import     = nullptr;

DirectDrawEnumerate_pfn   DirectDrawEnumerateA_Import   = nullptr;
DirectDrawEnumerate_pfn   DirectDrawEnumerateW_Import   = nullptr;

DirectDrawEnumerateEx_pfn DirectDrawEnumerateExA_Import = nullptr;
DirectDrawEnumerateEx_pfn DirectDrawEnumerateExW_Import = nullptr;

__declspec (noinline)
HRESULT
WINAPI
DirectDrawCreate (_In_  GUID         FAR *lpGUID,
                  _Out_ LPDIRECTDRAW FAR *lplpDD,
                  _In_  IUnknown     FAR *pUnkOuter)
{
  WaitForInit_DDraw ();
  WaitForInit       ();

  //StringFromGUID2

  dll_log.Log ( L"[   DDraw  ] [!] %s (...) - "
                L"%s",
                  L"DirectDrawCreate",
                    /*SDKVersion,*/
                      SK_SummarizeCaller ().c_str () );

  HRESULT hr = E_NOTIMPL;
  
  if (DirectDrawCreate_Import)
  {
    hr = DirectDrawCreate_Import (lpGUID, lplpDD, pUnkOuter);
  }
  
  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawCreateEx (_In_  GUID         FAR *lpGUID,
                    _Out_ LPDIRECTDRAW FAR *lplpDD,
                    _In_  REFIID            iid,
                    _In_  IUnknown     FAR *pUnkOuter)
{
  WaitForInit_DDraw ();
  WaitForInit       ();

  //StringFromGUID2

  dll_log.Log ( L"[   DDraw  ] [!] %s (...) - "
                L"%s",
                  L"DirectDrawCreateEx",
                    /*SDKVersion,*/
                      SK_SummarizeCaller ().c_str () );

  HRESULT hr = E_NOTIMPL;
  
  if (DirectDrawCreateEx_Import)
  {
    hr = DirectDrawCreateEx_Import (lpGUID, lplpDD, iid, pUnkOuter);
  }
  
  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawEnumerateA (_In_ LPDDENUMCALLBACK lpCallback,
                      _In_ LPVOID           lpContext)
{
  WaitForInit_DDraw ();
  WaitForInit       ();

  HRESULT hr = E_NOTIMPL;
  
  if (DirectDrawEnumerateA_Import)
  {
    hr = DirectDrawEnumerateA_Import (lpCallback, lpContext);
  }
  
  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawEnumerateW (_In_ LPDDENUMCALLBACK lpCallback,
                      _In_ LPVOID           lpContext)
{
  WaitForInit_DDraw ();
  WaitForInit       ();

  HRESULT hr = E_NOTIMPL;
  
  if (DirectDrawEnumerateW_Import)
  {
    hr = DirectDrawEnumerateW_Import (lpCallback, lpContext);
  }
  
  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawEnumerateExA (_In_ LPDDENUMCALLBACKEX lpCallback,
                        _In_ LPVOID             lpContext,
                        _In_ DWORD              dwFlags)
{
  WaitForInit_DDraw ();
  WaitForInit       ();

  HRESULT hr = E_NOTIMPL;
  
  if (DirectDrawEnumerateExA_Import)
  {
    hr = DirectDrawEnumerateExA_Import (lpCallback, lpContext, dwFlags);
  }
  
  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawEnumerateExW (_In_ LPDDENUMCALLBACKEX lpCallback,
                        _In_ LPVOID             lpContext,
                        _In_ DWORD              dwFlags)
{
  WaitForInit_DDraw ();
  WaitForInit       ();

  HRESULT hr = E_NOTIMPL;
  
  if (DirectDrawEnumerateExW_Import) {
    hr = DirectDrawEnumerateExW_Import (lpCallback, lpContext, dwFlags);
  }
  
  return hr;
}

void
WINAPI
SK_HookDDraw (void)
{
  static volatile ULONG hooked = FALSE;

  if (InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    return;
  }

  HMODULE hBackend = 
    (SK_GetDLLRole () & DLL_ROLE::DDraw) ? backend_dll :
                                   GetModuleHandle (L"ddraw.dll");

  auto LoadSupplementalImports =
  [&]
  {
    (DirectDrawEnumerateA_Import) =  \
      reinterpret_cast <DirectDrawEnumerate_pfn> (
        GetProcAddress (hBackend, "DirectDrawEnumerateA")
      );
    (DirectDrawEnumerateW_Import) =  \
      reinterpret_cast <DirectDrawEnumerate_pfn> (
        GetProcAddress (hBackend, "DirectDrawEnumerateW") 
      );

    (DirectDrawEnumerateExA_Import) =  \
      reinterpret_cast <DirectDrawEnumerateEx_pfn> (
        GetProcAddress (hBackend, "DirectDrawEnumerateExA")
      );
    (DirectDrawEnumerateExW_Import) =  \
      reinterpret_cast <DirectDrawEnumerateEx_pfn> (
        GetProcAddress (hBackend, "DirectDrawEnumerateExW")
      );
  };

  dll_log.Log (L"[   DDraw  ] Importing DirectDrawCreate{Ex}..");
  dll_log.Log (L"[   DDraw  ] ================================");

  if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"ddraw.dll"))
  {
    dll_log.Log (L"[   DDraw  ]   DirectDrawCreate:   %ph",
      (DirectDrawCreate_Import) =  \
        reinterpret_cast <DirectDrawCreate_pfn> (
          GetProcAddress (hBackend, "DirectDrawCreate")
        )
    );
    dll_log.Log (L"[   DDraw  ]   DirectDrawCreateEx: %ph",
      (DirectDrawCreateEx_Import) =  \
        reinterpret_cast <DirectDrawCreateEx_pfn> (
          GetProcAddress (hBackend, "DirectDrawCreateEx")
        )
    );

    LoadSupplementalImports ();
  }

  else
  {
    bool bProxy = GetModuleHandle (L"ddraw.dll") != hBackend;

    if ( MH_OK ==
           SK_CreateDLLHook2 (      L"ddraw.dll",
                                     "DirectDrawCreate",
                                      DirectDrawCreate,
             static_cast_p2p <void> (&DirectDrawCreate_Import) ) &&
         MH_OK ==
           SK_CreateDLLHook2 (      L"ddraw.dll",
                                     "DirectDrawCreateEx",
                                      DirectDrawCreateEx,
             static_cast_p2p <void> (&DirectDrawCreateEx_Import) ) &&
         MH_OK ==
           SK_CreateDLLHook2 (      L"ddraw.dll",
                                     "DirectDrawEnumerateA",
                                      DirectDrawEnumerateA,
             static_cast_p2p <void> (&DirectDrawEnumerateA_Import) ) &&
         MH_OK ==
           SK_CreateDLLHook2 (      L"ddraw.dll",
                                     "DirectDrawEnumerateW",
                                      DirectDrawEnumerateW,
             static_cast_p2p <void> (&DirectDrawEnumerateW_Import) ) &&
         MH_OK ==
           SK_CreateDLLHook2 (      L"ddraw.dll",
                                     "DirectDrawEnumerateExA",
                                      DirectDrawEnumerateExA,
             static_cast_p2p <void> (&DirectDrawEnumerateExA_Import) ) &&
         MH_OK ==
           SK_CreateDLLHook2 (      L"ddraw.dll",
                                     "DirectDrawEnumerateExW",
                                      DirectDrawEnumerateExW,
             static_cast_p2p <void> (&DirectDrawEnumerateExW_Import) )
       )
    {
      if (bProxy)
      {
        (DirectDrawCreate_Import)   =  \
          reinterpret_cast <DirectDrawCreate_pfn> (
            GetProcAddress (hBackend, "DirectDrawCreate")
          );
        (DirectDrawCreateEx_Import) =  \
          reinterpret_cast <DirectDrawCreateEx_pfn> (
            GetProcAddress (hBackend, "DirectDrawCreateEx")
          );

        LoadSupplementalImports ();
      }

      dll_log.Log (L"[   DDraw  ]   DirectDrawCreate:   %p  { Hooked }",
        (DirectDrawCreate_Import) );
      dll_log.Log (L"[   DDraw  ]   DirectDrawCreateEx: %p  { Hooked }",
        (DirectDrawCreateEx_Import) );
    }
  }

  dgvoodoo_ddraw = new import_s ();
  dgvoodoo_ddraw->hLibrary     = hBackend;
  dgvoodoo_ddraw->name         = L"API Support Plug-In";
  dgvoodoo_ddraw->product_desc = SK_GetDLLVersionStr (SK_GetModuleFullName (hBackend).c_str ());

// Load user-defined DLLs (Plug-In)
#ifdef _WIN64
  SK_LoadPlugIns64 ();
#else
  SK_LoadPlugIns32 ();
#endif

  HookDDraw (nullptr);
}

void
WINAPI
ddraw_init_callback (finish_pfn finish)
{
  if (! SK_IsHostAppSKIM ())
  {
    SK_BootDDraw ();

    while (! InterlockedCompareExchange (&__ddraw_ready, FALSE, FALSE))
      MsgWaitForMultipleObjectsEx (0, nullptr, 100, QS_ALLINPUT, MWMO_ALERTABLE);
  }

  finish ();
}


bool
SK::DDraw::Startup (void)
{
  wchar_t wszImmediateMode [MAX_PATH * 2 + 1] = { };

  wsprintf (wszImmediateMode, L"%s\\PlugIns\\ThirdParty\\dgVoodoo\\d3dimm.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());

  dgvoodoo_d3dimm               = new import_s ();
  dgvoodoo_d3dimm->hLibrary     = LoadLibraryW (wszImmediateMode);
  dgvoodoo_d3dimm->name         = L"API Support Plug-In";
  dgvoodoo_d3dimm->product_desc = SK_GetDLLVersionStr (SK_GetModuleFullName (dgvoodoo_d3dimm->hLibrary).c_str ());

  wsprintf (wszImmediateMode, L"%s\\PlugIns\\ThirdParty\\dgVoodoo\\d3d8.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());
  
  dgvoodoo_d3d8               = new import_s ();
  dgvoodoo_d3d8->hLibrary     = LoadLibraryW (wszImmediateMode);
  dgvoodoo_d3d8->name         = L"API Support Plug-In";
  dgvoodoo_d3d8->product_desc = SK_GetDLLVersionStr (SK_GetModuleFullName (dgvoodoo_d3d8->hLibrary).c_str ());

  config.apis.d3d8.hook = false;

  const bool ret = SK_StartupCore (L"ddraw", ddraw_init_callback);

  return ret;
}

bool
SK::DDraw::Shutdown (void)
{
  if (dgvoodoo_ddraw)
  {
    FreeLibrary (dgvoodoo_ddraw->hLibrary);
    delete dgvoodoo_ddraw;
  }

  if (dgvoodoo_d3d8)
  {
    FreeLibrary (dgvoodoo_d3d8->hLibrary);
    delete dgvoodoo_d3d8;
  }

  if (dgvoodoo_d3dimm)
  {
    FreeLibrary (dgvoodoo_d3dimm->hLibrary);
    delete dgvoodoo_d3dimm;
  }

  return SK_ShutdownCore (L"ddraw");
}


unsigned int
__stdcall
HookDDraw (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  if (! config.apis.ddraw.hook)
  {
    return 0;
  }

  const bool success = SUCCEEDED (
    CoInitializeEx (nullptr, COINIT_MULTITHREADED)
  );

  {
    //Direct3DCreate8_Import (0x0800);

    InterlockedExchange (&__ddraw_ready, TRUE);

    if (! (SK_GetDLLRole () & DLL_ROLE::DXGI))
      SK::DXGI::StartBudgetThread_NoAdapter ();
  }

  if (success)
    CoUninitialize ();

  return 0;
}
#else
#include <Windows.h>
#include <combaseapi.h>
__declspec (noinline)
HRESULT
WINAPI
DirectDrawCreate (_In_  GUID         FAR *lpGUID,
                  _Out_ LPDIRECTDRAW FAR *lplpDD,
                  _In_  IUnknown     FAR *pUnkOuter)
{
  UNREFERENCED_PARAMETER (lpGUID);
  UNREFERENCED_PARAMETER (lplpDD);
  UNREFERENCED_PARAMETER (pUnkOuter);

  return E_NOTIMPL;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawCreateEx (_In_  GUID         FAR *lpGUID,
                    _Out_ LPDIRECTDRAW FAR *lplpDD,
                    _In_  REFIID            iid,
                    _In_  IUnknown     FAR *pUnkOuter)
{
  UNREFERENCED_PARAMETER (lpGUID);
  UNREFERENCED_PARAMETER (lplpDD);
  UNREFERENCED_PARAMETER (iid);
  UNREFERENCED_PARAMETER (pUnkOuter);

  return E_NOTIMPL;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawEnumerateA (_In_ LPDDENUMCALLBACK lpCallback,
                      _In_ LPVOID           lpContext)
{
  UNREFERENCED_PARAMETER (lpCallback);
  UNREFERENCED_PARAMETER (lpContext);

  return E_NOTIMPL;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawEnumerateW (_In_ LPDDENUMCALLBACK lpCallback,
                      _In_ LPVOID           lpContext)
{
  UNREFERENCED_PARAMETER (lpCallback);
  UNREFERENCED_PARAMETER (lpContext);

  return E_NOTIMPL;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawEnumerateExA (_In_ LPDDENUMCALLBACKEX lpCallback,
                        _In_ LPVOID             lpContext,
                        _In_ DWORD              dwFlags)
{
  UNREFERENCED_PARAMETER (lpCallback);
  UNREFERENCED_PARAMETER (lpContext);
  UNREFERENCED_PARAMETER (dwFlags);

  return E_NOTIMPL;
}

__declspec (noinline)
HRESULT
WINAPI
DirectDrawEnumerateExW (_In_ LPDDENUMCALLBACKEX lpCallback,
                        _In_ LPVOID             lpContext,
                        _In_ DWORD              dwFlags)
{
  UNREFERENCED_PARAMETER (lpCallback);
  UNREFERENCED_PARAMETER (lpContext);
  UNREFERENCED_PARAMETER (dwFlags);

  return E_NOTIMPL;
}
#endif