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
#define PSAPI_VERSION 1

#include <Windows.h>

#include "dxgi_interfaces.h"
#include "dxgi_backend.h"

#include <atlbase.h>

#include "nvapi.h"
#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include "log.h"
#include "utility.h"

#include "core.h"
#include "command.h"
#include "console.h"
#include "framerate.h"

#include "diagnostics/crash_handler.h"

#undef min
#undef max

#include <set>
#include <queue>
#include <vector>
#include <limits>
#include <algorithm>
#include <functional>

#define DXGI_CENTERED

DWORD dwRenderThread = 0x0000;

// For DXGI compliance, do not mix-and-match factory types
bool SK_DXGI_use_factory1 = false;
bool SK_DXGI_factory_init = false;

extern void  __stdcall SK_D3D11_TexCacheCheckpoint    ( void );
extern void  __stdcall SK_D3D11_UpdateRenderStats     ( IDXGISwapChain*      pSwapChain );

extern void  __stdcall SK_D3D12_UpdateRenderStats     ( IDXGISwapChain*      pSwapChain );

extern BOOL __stdcall SK_NvAPI_SetFramerateLimit  (uint32_t limit);
extern void __stdcall SK_NvAPI_SetAppFriendlyName (const wchar_t* wszFriendlyName);

volatile LONG  __dxgi_ready  = FALSE;

void WaitForInitDXGI (void)
{
  while (! InterlockedCompareExchange (&__dxgi_ready, FALSE, FALSE))
    Sleep (config.system.init_delay);
}

unsigned int __stdcall HookDXGI (LPVOID user);

#define D3D_FEATURE_LEVEL_12_0 0xc000
#define D3D_FEATURE_LEVEL_12_1 0xc100

std::wstring
SK_DXGI_FeatureLevelsToStr (       int    FeatureLevels,
                             const DWORD* pFeatureLevels )
{
  if (FeatureLevels == 0 || pFeatureLevels == nullptr)
    return L"N/A";

  std::wstring out = L"";

  for (UINT i = 0; i < FeatureLevels; i++)
  {
    switch (pFeatureLevels [i])
    {
    case D3D_FEATURE_LEVEL_9_1:
      out += L" 9.1";
      break;
    case D3D_FEATURE_LEVEL_9_2:
      out += L" 9.2";
      break;
    case D3D_FEATURE_LEVEL_9_3:
      out += L" 9.3";
      break;
    case D3D_FEATURE_LEVEL_10_0:
      out += L" 10.0";
      break;
    case D3D_FEATURE_LEVEL_10_1:
      out += L" 10.1";
      break;
    case D3D_FEATURE_LEVEL_11_0:
      out += L" 11.0";
      break;
    case D3D_FEATURE_LEVEL_11_1:
      out += L" 11.1";
      break;
    case D3D_FEATURE_LEVEL_12_0:
      out += L" 12.0";
      break;
    case D3D_FEATURE_LEVEL_12_1:
      out += L" 12.1";
      break;
    }
  }

  return out;
}

extern "C++" bool SK_FO4_UseFlipMode        (void);
extern "C++" bool SK_FO4_IsFullscreen       (void);
extern "C++" bool SK_FO4_IsBorderlessWindow (void);

extern "C++" HRESULT STDMETHODCALLTYPE
                  SK_FO4_PresentFirstFrame   (IDXGISwapChain *, UINT, UINT);


extern "C++" bool SK_TW3_UseFlipMode         (void);

extern "C++" HRESULT STDMETHODCALLTYPE
                  SK_TW3_PresentFirstFrame   (IDXGISwapChain *, UINT, UINT);


// TODO: Get this stuff out of here, it's breaking what _DSlittle design work there is.
extern "C++" void SK_DS3_InitPlugin         (void);
extern "C++" bool SK_DS3_UseFlipMode        (void);
extern "C++" bool SK_DS3_IsBorderless       (void);

extern "C++" HRESULT STDMETHODCALLTYPE
                  SK_DS3_PresentFirstFrame   (IDXGISwapChain *, UINT, UINT);

extern "C" unsigned int __stdcall SK_DXGI_BringRenderWindowToTop_THREAD (LPVOID);

void
WINAPI
SK_DXGI_BringRenderWindowToTop (void)
{
  _beginthreadex ( nullptr,
                     0,
                       SK_DXGI_BringRenderWindowToTop_THREAD,
                         nullptr,
                           0,
                             nullptr );
}

extern int                      gpu_prio;

bool             bAlwaysAllowFullscreen = false;
HWND             hWndRender             = 0;
IDXGISwapChain*  g_pDXGISwap            = nullptr;

bool bFlipMode = false;
bool bWait     = false;

// Used for integrated GPU override
int              SK_DXGI_preferred_adapter = -1;

bool
WINAPI
SK_DXGI_EnableFlipMode (bool bFlip)
{
  bool before = bFlipMode;

  bFlipMode = bFlip;

  return before;
}

void
WINAPI
SKX_D3D11_EnableFullscreen (bool bFullscreen)
{
  bAlwaysAllowFullscreen = bFullscreen;
}

extern
unsigned int __stdcall HookD3D11                   (LPVOID user);
extern
unsigned int __stdcall HookD3D12                   (LPVOID user);

void                   SK_DXGI_HookPresent         (IDXGISwapChain* pSwapChain);
void         WINAPI    SK_DXGI_SetPreferredAdapter (int override_id);


enum SK_DXGI_ResType {
  WIDTH  = 0,
  HEIGHT = 1
};

auto SK_DXGI_RestrictResMax = []( SK_DXGI_ResType dim,
                                  auto&           last,
                                  auto            idx,
                                  auto            covered,
                                  DXGI_MODE_DESC* pDesc )->
bool
 {
   auto& val = dim == WIDTH ? pDesc [idx].Width :
                              pDesc [idx].Height;

   auto  max = dim == WIDTH ? config.render.dxgi.res.max.x :
                              config.render.dxgi.res.max.y;

   bool covered_already = covered.count (idx) > 0;

   if ( (max > 0 &&
         val > max) || covered_already ) {
     for ( int i = idx ; i > 0 ; --i ) {
       if ( config.render.dxgi.res.max.x >= pDesc [i].Width  &&
            config.render.dxgi.res.max.y >= pDesc [i].Height &&
            covered.count (i) == 0 ) {
         pDesc [idx] = pDesc [i];
         covered.insert (idx);
         covered.insert (i);
         return false;
       }
     }

     covered.insert (idx);

     pDesc [idx].Width  = config.render.dxgi.res.max.x;
     pDesc [idx].Height = config.render.dxgi.res.max.y;

     return true;
   }

   covered.insert (idx);

   return false;
 };

auto SK_DXGI_RestrictResMin = []( SK_DXGI_ResType dim,
                                  auto&           first,
                                  auto            idx,
                                  auto            covered,
                                  DXGI_MODE_DESC* pDesc )->
bool
 {
   auto& val = dim == WIDTH ? pDesc [idx].Width :
                              pDesc [idx].Height;

   auto  min = dim == WIDTH ? config.render.dxgi.res.min.x :
                              config.render.dxgi.res.min.y;

   bool covered_already = covered.count (idx) > 0;

   if ( (min > 0 &&
         val < min) || covered_already ) {
     for ( int i = 0 ; i < idx ; ++i ) {
       if ( config.render.dxgi.res.min.x <= pDesc [i].Width  &&
            config.render.dxgi.res.min.y <= pDesc [i].Height &&
            covered.count (i) == 0 ) {
         pDesc [idx] = pDesc [i];
         covered.insert (idx);
         covered.insert (i);
         return false;
       }
     }

     covered.insert (idx);

     pDesc [idx].Width  = config.render.dxgi.res.min.x;
     pDesc [idx].Height = config.render.dxgi.res.min.y;

     return true;
   }

   covered.insert (idx);

   return false;
 };


struct dxgi_caps_t {
  struct {
    bool latency_control = false;
    bool enqueue_event   = false;
  } device;

  struct {
    bool flip_sequential = false;
    bool flip_discard    = false;
    bool waitable        = false;
  } present;
} dxgi_caps;

extern "C" {
  typedef HRESULT (STDMETHODCALLTYPE *PresentSwapChain_pfn)(
                                         IDXGISwapChain *This,
                                         UINT            SyncInterval,
                                         UINT            Flags);

  typedef HRESULT (STDMETHODCALLTYPE *Present1SwapChain1_pfn)(
                                         IDXGISwapChain1         *This,
                                         UINT                     SyncInterval,
                                         UINT                     Flags,
                                   const DXGI_PRESENT_PARAMETERS *pPresentParameters);

  typedef HRESULT (STDMETHODCALLTYPE *CreateSwapChain_pfn)(
                                         IDXGIFactory          *This,
                                   _In_  IUnknown              *pDevice,
                                   _In_  DXGI_SWAP_CHAIN_DESC  *pDesc,
                                  _Out_  IDXGISwapChain       **ppSwapChain);

  typedef HRESULT (STDMETHODCALLTYPE *CreateSwapChainForHwnd_pfn)(
                                         IDXGIFactory2                    *This,
                              _In_       IUnknown                         *pDevice,
                              _In_       HWND                              hWnd,
                              _In_ const DXGI_SWAP_CHAIN_DESC1            *pDesc,
                          _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC  *pFullscreenDesc,
                          _In_opt_       IDXGIOutput                      *pRestrictToOutput,
                             _Out_       IDXGISwapChain1                 **ppSwapChain);

  typedef HRESULT (STDMETHODCALLTYPE *CreateSwapChainForCoreWindow_pfn)(
                                         IDXGIFactory2                   *This,
                              _In_       IUnknown                        *pDevice,
                              _In_       IUnknown                        *pWindow,
                              _In_ const DXGI_SWAP_CHAIN_DESC1           *pDesc,
                          _In_opt_       IDXGIOutput                     *pRestrictToOutput,
                             _Out_       IDXGISwapChain1                **ppSwapChain);

  typedef HRESULT (STDMETHODCALLTYPE *SetFullscreenState_pfn)(
                                         IDXGISwapChain *This,
                                         BOOL            Fullscreen,
                                         IDXGIOutput    *pTarget);

  typedef HRESULT (STDMETHODCALLTYPE *GetFullscreenState_pfn)(
                                         IDXGISwapChain  *This,
                              _Out_opt_  BOOL            *pFullscreen,
                              _Out_opt_  IDXGIOutput    **ppTarget );

  typedef HRESULT (STDMETHODCALLTYPE *ResizeBuffers_pfn)(
                                         IDXGISwapChain *This,
                              /* [in] */ UINT            BufferCount,
                              /* [in] */ UINT            Width,
                              /* [in] */ UINT            Height,
                              /* [in] */ DXGI_FORMAT     NewFormat,
                              /* [in] */ UINT            SwapChainFlags);

  typedef HRESULT (STDMETHODCALLTYPE *ResizeTarget_pfn)(
                                    _In_ IDXGISwapChain  *This,
                              _In_ const DXGI_MODE_DESC  *pNewTargetParameters );

  typedef HRESULT (STDMETHODCALLTYPE *GetDisplayModeList_pfn)(
                                         IDXGIOutput     *This,
                              /* [in] */ DXGI_FORMAT      EnumFormat,
                              /* [in] */ UINT             Flags,
                              /* [annotation][out][in] */
                                _Inout_  UINT            *pNumModes,
                              /* [annotation][out] */
_Out_writes_to_opt_(*pNumModes,*pNumModes)
                                         DXGI_MODE_DESC *pDesc );

  typedef HRESULT (STDMETHODCALLTYPE *FindClosestMatchingMode_pfn)(
                                         IDXGIOutput    *This,
                             /* [annotation][in] */
                             _In_  const DXGI_MODE_DESC *pModeToMatch,
                             /* [annotation][out] */
                             _Out_       DXGI_MODE_DESC *pClosestMatch,
                             /* [annotation][in] */
                              _In_opt_  IUnknown *pConcernedDevice );

  typedef HRESULT (STDMETHODCALLTYPE *WaitForVBlank_pfn)(
                                         IDXGIOutput    *This );


  typedef HRESULT (STDMETHODCALLTYPE *GetDesc1_pfn)(IDXGIAdapter1      *This,
                                           _Out_    DXGI_ADAPTER_DESC1 *pDesc);
  typedef HRESULT (STDMETHODCALLTYPE *GetDesc2_pfn)(IDXGIAdapter2      *This,
                                             _Out_  DXGI_ADAPTER_DESC2 *pDesc);
  typedef HRESULT (STDMETHODCALLTYPE *GetDesc_pfn) (IDXGIAdapter       *This,
                                             _Out_  DXGI_ADAPTER_DESC  *pDesc);

  typedef HRESULT (STDMETHODCALLTYPE *EnumAdapters_pfn)(
                                          IDXGIFactory  *This,
                                          UINT           Adapter,
                                    _Out_ IDXGIAdapter **ppAdapter);

  typedef HRESULT (STDMETHODCALLTYPE *EnumAdapters1_pfn)(
                                          IDXGIFactory1  *This,
                                          UINT            Adapter,
                                    _Out_ IDXGIAdapter1 **ppAdapter);

  CreateSwapChain_pfn     CreateSwapChain_Original     = nullptr;
  PresentSwapChain_pfn    Present_Original             = nullptr;
  Present1SwapChain1_pfn  Present1_Original            = nullptr;
  SetFullscreenState_pfn  SetFullscreenState_Original  = nullptr;
  GetFullscreenState_pfn  GetFullscreenState_Original  = nullptr;
  ResizeBuffers_pfn       ResizeBuffers_Original       = nullptr;
  ResizeTarget_pfn        ResizeTarget_Original        = nullptr;

  GetDisplayModeList_pfn           GetDisplayModeList_Original           = nullptr;
  FindClosestMatchingMode_pfn      FindClosestMatchingMode_Original      = nullptr;
  WaitForVBlank_pfn                WaitForVBlank_Original                = nullptr;
  CreateSwapChainForHwnd_pfn       CreateSwapChainForHwnd_Original       = nullptr;
  CreateSwapChainForCoreWindow_pfn CreateSwapChainForCoreWindow_Original = nullptr;

  GetDesc_pfn             GetDesc_Original             = nullptr;
  GetDesc1_pfn            GetDesc1_Original            = nullptr;
  GetDesc2_pfn            GetDesc2_Original            = nullptr;

  EnumAdapters_pfn        EnumAdapters_Original        = nullptr;
  EnumAdapters1_pfn       EnumAdapters1_Original       = nullptr;

  CreateDXGIFactory_pfn   CreateDXGIFactory_Import     = nullptr;
  CreateDXGIFactory1_pfn  CreateDXGIFactory1_Import    = nullptr;
  CreateDXGIFactory2_pfn  CreateDXGIFactory2_Import    = nullptr;

  const wchar_t*
  SK_DescribeVirtualProtectFlags (DWORD dwProtect)
  {
    switch (dwProtect)
    {
    case 0x10:
      return L"Execute";
    case 0x20:
      return L"Execute + Read-Only";
    case 0x40:
      return L"Execute + Read/Write";
    case 0x80:
      return L"Execute + Read-Only or Copy-on-Write)";
    case 0x01:
      return L"No Access";
    case 0x02:
      return L"Read-Only";
    case 0x04:
      return L"Read/Write";
    case 0x08:
      return L" Read-Only or Copy-on-Write";
    default:
      return L"UNKNOWN";
    }
  }

void
SK_DXGI_BeginHooking (void)
{
  volatile static ULONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE)) {
    HANDLE hHookInitDXGI =
      (HANDLE)
        _beginthreadex ( nullptr,
                           0,
                             HookDXGI,
                               nullptr,
                                 0x00,
                                   nullptr );

    HANDLE hHookInitD3D11 =
      (HANDLE)
        _beginthreadex ( nullptr,
                           0,
                             HookD3D11,
                               nullptr,
                                 0x00,
                                   nullptr );

    HANDLE hHookInitD3D12 =
      (HANDLE)
        _beginthreadex ( nullptr,
                           0,
                             HookD3D12,
                               nullptr,
                                 0x00,
                                   nullptr );
  }
}

#define WaitForInit() {    \
  ::WaitForInit        (); \
  SK_DXGI_BeginHooking (); \
}

#define DXGI_STUB(_Return, _Name, _Proto, _Args)                            \
  __declspec (nothrow) _Return STDMETHODCALLTYPE                            \
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
          L"Unable to locate symbol  %s in dxgi.dll",                       \
          L#_Name);                                                         \
        return (_Return)E_NOTIMPL;                                          \
      }                                                                     \
    }                                                                       \
                                                                            \
    dll_log.Log (L"[   DXGI   ] [!] %s %s - "                               \
             L"[Calling Thread: 0x%04x]",                                   \
      L#_Name, L#_Proto, GetCurrentThreadId ());                            \
                                                                            \
    return _default_impl _Args;                                             \
}

#define DXGI_STUB_(_Name, _Proto, _Args)                                    \
  __declspec (nothrow) void STDMETHODCALLTYPE                               \
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
          L"Unable to locate symbol  %s in dxgi.dll",                       \
          L#_Name );                                                        \
      }                                                                     \
    }                                                                       \
                                                                            \
    dll_log.Log (L"[   DXGI   ] [!] %s %s - "                               \
             L"[Calling Thread: 0x%04x]",                                   \
      L#_Name, L#_Proto, GetCurrentThreadId ());                            \
                                                                            \
    _default_impl _Args;                                                    \
}
}

int
SK_GetDXGIFactoryInterfaceVer (const IID& riid)
{
  if (riid == __uuidof (IDXGIFactory))
    return 0;
  if (riid == __uuidof (IDXGIFactory1))
    return 1;
  if (riid == __uuidof (IDXGIFactory2))
    return 2;
  if (riid == __uuidof (IDXGIFactory3))
    return 3;
  if (riid == __uuidof (IDXGIFactory4))
    return 4;

  //assert (false);
  return -1;
}

std::wstring
SK_GetDXGIFactoryInterfaceEx (const IID& riid)
{
  std::wstring interface_name;

  if (riid == __uuidof (IDXGIFactory))
    interface_name = L"IDXGIFactory";
  else if (riid == __uuidof (IDXGIFactory1))
    interface_name = L"IDXGIFactory1";
  else if (riid == __uuidof (IDXGIFactory2))
    interface_name = L"IDXGIFactory2";
  else if (riid == __uuidof (IDXGIFactory3))
    interface_name = L"IDXGIFactory3";
  else if (riid == __uuidof (IDXGIFactory4))
    interface_name = L"IDXGIFactory4";
  else {
    wchar_t *pwszIID;

    if (SUCCEEDED (StringFromIID (riid, (LPOLESTR *)&pwszIID)))
    {
      interface_name = pwszIID;
      CoTaskMemFree (pwszIID);
    }
  }

  return interface_name;
}

int
SK_GetDXGIFactoryInterfaceVer (IUnknown *pFactory)
{
  CComPtr <IUnknown> pTemp;

  if (SUCCEEDED (
    pFactory->QueryInterface (__uuidof (IDXGIFactory4), (void **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    dxgi_caps.present.waitable        = true;
    dxgi_caps.present.flip_discard    = true;
    return 4;
  }
  if (SUCCEEDED (
    pFactory->QueryInterface (__uuidof (IDXGIFactory3), (void **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    dxgi_caps.present.waitable        = true;
    return 3;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface (__uuidof (IDXGIFactory2), (void **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    return 2;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface (__uuidof (IDXGIFactory1), (void **)&pTemp)))
  {
    dxgi_caps.device.latency_control  = true;
    return 1;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface (__uuidof (IDXGIFactory), (void **)&pTemp)))
  {
    return 0;
  }

  //assert (false);
  return -1;
}

std::wstring
SK_GetDXGIFactoryInterface (IUnknown *pFactory)
{
  int iver = SK_GetDXGIFactoryInterfaceVer (pFactory);

  if (iver == 4)
    return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory4));

  if (iver == 3)
    return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory3));

  if (iver == 2)
    return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory2));

  if (iver == 1)
    return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory1));

  if (iver == 0)
    return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory));

  return L"{Invalid-Factory-UUID}";
}

int
SK_GetDXGIAdapterInterfaceVer (const IID& riid)
{
  if (riid == __uuidof (IDXGIAdapter))
    return 0;
  if (riid == __uuidof (IDXGIAdapter1))
    return 1;
  if (riid == __uuidof (IDXGIAdapter2))
    return 2;
  if (riid == __uuidof (IDXGIAdapter3))
    return 3;

  //assert (false);
  return -1;
}

std::wstring
SK_GetDXGIAdapterInterfaceEx (const IID& riid)
{
  std::wstring interface_name;

  if (riid == __uuidof (IDXGIAdapter))
    interface_name = L"IDXGIAdapter";
  else if (riid == __uuidof (IDXGIAdapter1))
    interface_name = L"IDXGIAdapter1";
  else if (riid == __uuidof (IDXGIAdapter2))
    interface_name = L"IDXGIAdapter2";
  else if (riid == __uuidof (IDXGIAdapter3))
    interface_name = L"IDXGIAdapter3";
  else {
    wchar_t* pwszIID;

    if (SUCCEEDED (StringFromIID (riid, (LPOLESTR *)&pwszIID)))
    {
      interface_name = pwszIID;
      CoTaskMemFree (pwszIID);
    }
  }

  return interface_name;
}

int
SK_GetDXGIAdapterInterfaceVer (IUnknown *pAdapter)
{
  CComPtr <IUnknown> pTemp;

  if (SUCCEEDED(
    pAdapter->QueryInterface (__uuidof (IDXGIAdapter3), (void **)&pTemp)))
  {
    return 3;
  }

  if (SUCCEEDED(
    pAdapter->QueryInterface (__uuidof (IDXGIAdapter2), (void **)&pTemp)))
  {
    return 2;
  }

  if (SUCCEEDED(
    pAdapter->QueryInterface (__uuidof (IDXGIAdapter1), (void **)&pTemp)))
  {
    return 1;
  }

  if (SUCCEEDED(
    pAdapter->QueryInterface (__uuidof (IDXGIAdapter), (void **)&pTemp)))
  {
    return 0;
  }

  //assert (false);
  return -1;
}

std::wstring
SK_GetDXGIAdapterInterface (IUnknown *pAdapter)
{
  int iver = SK_GetDXGIAdapterInterfaceVer (pAdapter);

  if (iver == 3)
    return SK_GetDXGIAdapterInterfaceEx (__uuidof (IDXGIAdapter3));

  if (iver == 2)
    return SK_GetDXGIAdapterInterfaceEx (__uuidof (IDXGIAdapter2));

  if (iver == 1)
    return SK_GetDXGIAdapterInterfaceEx (__uuidof (IDXGIAdapter1));

  if (iver == 0)
    return SK_GetDXGIAdapterInterfaceEx (__uuidof (IDXGIAdapter));

  return L"{Invalid-Adapter-UUID}";
}


extern "C" {
#define DARK_SOULS
#ifdef DARK_SOULS
  extern "C++" int* __DS3_WIDTH;
  extern "C++" int* __DS3_HEIGHT;
#endif

  HRESULT
    STDMETHODCALLTYPE Present1Callback (IDXGISwapChain1         *This,
                                        UINT                     SyncInterval,
                                        UINT                     PresentFlags,
                                  const DXGI_PRESENT_PARAMETERS *pPresentParameters)
  {
    g_pDXGISwap = This;

    //
    // Early-out for games that use testing to minimize blocking
    //
    if (PresentFlags & DXGI_PRESENT_TEST) {
      return Present1_Original (
               This,
                 SyncInterval,
                   PresentFlags,
                     pPresentParameters );
    }

    // Start / End / Readback Pipeline Stats
    SK_D3D11_UpdateRenderStats (This);
    SK_D3D12_UpdateRenderStats (This);

    SK_BeginBufferSwap ();

    HRESULT hr = E_FAIL;

    CComPtr <ID3D11Device> pDev;

    int interval = config.render.framerate.present_interval;
    int flags    = PresentFlags;

    // Application preference
    if (interval == -1)
      interval = SyncInterval;

    if (bFlipMode) {
      flags = PresentFlags | DXGI_PRESENT_RESTART;

      if (bWait)
        flags |= DXGI_PRESENT_DO_NOT_WAIT;
    }

    static bool first_frame = true;

    if (first_frame) {
      if (sk::NVAPI::app_name == L"Fallout4.exe") {
        SK_FO4_PresentFirstFrame (This, SyncInterval, flags);
      }

      else if (sk::NVAPI::app_name == L"DarkSoulsIII.exe") {
        SK_DS3_PresentFirstFrame (This, SyncInterval, flags);
      }

      else if (sk::NVAPI::app_name == L"witcher3.exe") {
        SK_TW3_PresentFirstFrame (This, SyncInterval, flags);
      }

      // TODO: Clean this code up
      if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))) )
      {
        CComPtr <IDXGIDevice>  pDevDXGI;
        CComPtr <IDXGIAdapter> pAdapter;
        CComPtr <IDXGIFactory> pFactory;

        if ( SUCCEEDED (pDev->QueryInterface (IID_PPV_ARGS (&pDevDXGI))) &&
             SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter))  &&
             SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory))) ) {
          DXGI_SWAP_CHAIN_DESC desc;
          This->GetDesc (&desc);

          if (bAlwaysAllowFullscreen) {
            pFactory->MakeWindowAssociation (
              desc.OutputWindow,
                DXGI_MWA_NO_WINDOW_CHANGES
            );
          }

          if (game_window.hWnd == 0) {
            hWndRender       = desc.OutputWindow;
            game_window.hWnd = hWndRender;
          }
        }
      }
    }

    hr = Present1_Original (This, interval, flags, pPresentParameters);

    first_frame = false;

    if (SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))))
    {
      HRESULT ret = E_FAIL;

      if (pDev != nullptr)
        ret = SK_EndBufferSwap (hr, pDev);

      SK_D3D11_TexCacheCheckpoint ();

      return ret;
    }

    // Not a D3D11 device -- weird...
    HRESULT ret = SK_EndBufferSwap (hr);

    return ret;
  }

  HRESULT
    STDMETHODCALLTYPE PresentCallback (IDXGISwapChain *This,
                                       UINT            SyncInterval,
                                       UINT            Flags)
  {
    g_pDXGISwap = This;

    //
    // Early-out for games that use testing to minimize blocking
    //
    if (Flags & DXGI_PRESENT_TEST)
      return Present_Original (This, SyncInterval, Flags);


#ifdef DARK_SOULS
    if (__DS3_HEIGHT != nullptr) {
      DXGI_SWAP_CHAIN_DESC swap_desc;
      if (SUCCEEDED (This->GetDesc (&swap_desc))) {
        *__DS3_WIDTH  = swap_desc.BufferDesc.Width;
        *__DS3_HEIGHT = swap_desc.BufferDesc.Height;
      }
    }
#endif

    // Start / End / Readback Pipeline Stats
    SK_D3D11_UpdateRenderStats (This);
    SK_D3D12_UpdateRenderStats (This);

    SK_BeginBufferSwap ();

    HRESULT hr = E_FAIL;

    CComPtr <ID3D11Device> pDev;

    int interval = config.render.framerate.present_interval;
    int flags    = Flags;

    // Application preference
    if (interval == -1)
      interval = SyncInterval;

    if (bFlipMode) {
      flags = Flags | DXGI_PRESENT_RESTART;

      if (bWait)
        flags |= DXGI_PRESENT_DO_NOT_WAIT;
    }

    if (! bFlipMode) {
      hr = S_OK;

      // Test first, then do
      if (SUCCEEDED (hr)) {
        hr = Present_Original (This, interval, flags);
      }

      if (hr != S_OK && hr != DXGI_STATUS_OCCLUDED) {
          dll_log.Log ( L"[   DXGI   ] *** IDXGISwapChain::Present (...) "
                        L"returned non-S_OK (%s :: %s)",
                          SK_DescribeHRESULT (hr),
                            SUCCEEDED (hr) ? L"Success" :
                                             L"Fail" );
      }
    }

    else {
      DXGI_PRESENT_PARAMETERS pparams;
      pparams.DirtyRectsCount = 0;
      pparams.pDirtyRects     = nullptr;
      pparams.pScrollOffset   = nullptr;
      pparams.pScrollRect     = nullptr;

      CComPtr <IDXGISwapChain1> pSwapChain1;
      if (SUCCEEDED (This->QueryInterface (IID_PPV_ARGS (&pSwapChain1))))
      {
        bool can_present = true;//SUCCEEDED (hr);

        if (can_present) {
        // No overlays will work if we don't do this...
        /////if (config.osd.show) {
          hr =
            Present_Original (
              This,
                0,
                  flags | DXGI_PRESENT_DO_NOT_SEQUENCE | DXGI_PRESENT_DO_NOT_WAIT );

        /////}

          hr = Present1_Original (pSwapChain1, interval, flags, &pparams);
        }
      }
    }

    if (bWait) {
      CComPtr <IDXGISwapChain2> pSwapChain2;

      if (SUCCEEDED (This->QueryInterface (IID_PPV_ARGS (&pSwapChain2))))
      {
        if (pSwapChain2 != nullptr) {
          HANDLE hWait = pSwapChain2->GetFrameLatencyWaitableObject ();

          pSwapChain2.Release ();

          WaitForSingleObjectEx ( hWait,
                                    500,//config.render.framerate.swapchain_wait,
                                      TRUE );
        }
      }
    }

    static bool first_frame = true;

    if (first_frame) {
      if (sk::NVAPI::app_name == L"Fallout4.exe") {
        SK_FO4_PresentFirstFrame (This, SyncInterval, Flags);
      }

      else if (sk::NVAPI::app_name == L"DarkSoulsIII.exe") {
        SK_DS3_PresentFirstFrame (This, SyncInterval, Flags);
      }

      else if (sk::NVAPI::app_name == L"witcher3.exe") {
        SK_TW3_PresentFirstFrame (This, SyncInterval, Flags);
      }


      // TODO: Clean this code up
      if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))) )
      {
        CComPtr <IDXGIDevice>  pDevDXGI;
        CComPtr <IDXGIAdapter> pAdapter;
        CComPtr <IDXGIFactory> pFactory;

        if ( SUCCEEDED (pDev->QueryInterface (IID_PPV_ARGS (&pDevDXGI))) &&
             SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter))  &&
             SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory))) ) {
          DXGI_SWAP_CHAIN_DESC desc;
          This->GetDesc (&desc);

          if (bAlwaysAllowFullscreen)
            pFactory->MakeWindowAssociation (desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES);

          hWndRender       = desc.OutputWindow;
          game_window.hWnd = hWndRender;

          SK_DXGI_BringRenderWindowToTop ();
        }
      }
    }

    first_frame = false;

    if (SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))))
    {
      HRESULT ret = E_FAIL;

      if (pDev != nullptr)
        ret = SK_EndBufferSwap (hr, pDev);

      SK_D3D11_TexCacheCheckpoint ();

      return ret;
    }

    // Not a D3D11 device -- weird...
    HRESULT ret = SK_EndBufferSwap (hr);

    return ret;
  }


  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGIOutput_GetDisplayModeList_Override ( IDXGIOutput    *This,
                                /* [in] */ DXGI_FORMAT     EnumFormat,
                                /* [in] */ UINT            Flags,
                                /* [annotation][out][in] */
                                  _Inout_  UINT           *pNumModes,
                                /* [annotation][out] */
_Out_writes_to_opt_(*pNumModes,*pNumModes)
                                           DXGI_MODE_DESC *pDesc)
  {
    if (pDesc != nullptr) {
      dll_log.Log ( L"[   DXGI   ] [!] IDXGIOutput::GetDisplayModeList (%ph, "
                                         L"EnumFormat=%lu, Flags=%lu, *pNumModes=%lu, "
                                         L"%ph)",
                    This,
                    EnumFormat,
                        Flags,
                          *pNumModes,
                             pDesc );
    }

    HRESULT hr =
      GetDisplayModeList_Original (
        This,
          EnumFormat,
            Flags /*| DXGI_ENUM_MODES_SCALING*/,
              pNumModes,
                pDesc );

    DXGI_MODE_DESC* pDescLocal = nullptr;

    if (pDesc == nullptr && SUCCEEDED (hr)) {
      pDescLocal = 
        (DXGI_MODE_DESC *)
          malloc (sizeof DXGI_MODE_DESC * *pNumModes);
      pDesc      = pDescLocal;

      hr =
        GetDisplayModeList_Original (
          This,
            EnumFormat,
              Flags /*| DXGI_ENUM_MODES_SCALING*/,
                pNumModes,
                  pDesc );
    }

    if (SUCCEEDED (hr)) {
      int removed_count = 0;

      if ( ! ( config.render.dxgi.res.min.isZero () &&
               config.render.dxgi.res.max.isZero () ) &&
           *pNumModes != 0 ) {
        int last  = *pNumModes;
        int first = 0;

        std::set <int> coverage_min;
        std::set <int> coverage_max;

        if (! config.render.dxgi.res.min.isZero ()) {
          // Restrict MAX (Sequential Scan: Last->First)
          for ( int i = *pNumModes - 1 ; i >= 0 ; --i ) {
            if (SK_DXGI_RestrictResMin (WIDTH,  first, i,  coverage_min, pDesc) |
                SK_DXGI_RestrictResMin (HEIGHT, first, i,  coverage_min, pDesc))
              ++removed_count;
          }
        }

        if (! config.render.dxgi.res.max.isZero ()) {
          // Restrict MAX (Sequential Scan: First->Last)
          for ( int i = 0 ; i < *pNumModes ; ++i ) {
            if (SK_DXGI_RestrictResMax (WIDTH,  last, i, coverage_max, pDesc) |
                SK_DXGI_RestrictResMax (HEIGHT, last, i, coverage_max, pDesc))
              ++removed_count;
          }
        }
      }

      if (pDesc != nullptr && pDescLocal == nullptr) {
        dll_log.Log ( L"[   DXGI   ]      >> %lu modes (%lu removed)",
                        *pNumModes,
                          removed_count );
      }
    }

    if (pDescLocal != nullptr) {
      free (pDescLocal);
      pDescLocal = nullptr;
      pDesc      = nullptr;
    }

    return hr;
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGIOutput_FindClosestMatchingMode_Override ( IDXGIOutput    *This,
                                                /* [annotation][in]  */
                                    _In_  const DXGI_MODE_DESC *pModeToMatch,
                                                /* [annotation][out] */
                                         _Out_  DXGI_MODE_DESC *pClosestMatch,
                                                /* [annotation][in]  */
                                      _In_opt_  IUnknown       *pConcernedDevice )
  {
//    dll_log.Log (L"[   DXGI   ] [!] IDXGIOutput::FindClosestMatchingMode (...)");
    return FindClosestMatchingMode_Original (This, pModeToMatch, pClosestMatch, pConcernedDevice );
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGIOutput_WaitForVBlank_Override ( IDXGIOutput    *This )
  {
//    dll_log.Log (L"[   DXGI   ] [!] IDXGIOutput::WaitForVBlank (...)");
    return WaitForVBlank_Original (This);
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_GetFullscreenState_Override ( IDXGISwapChain  *This,
                              _Out_opt_  BOOL            *pFullscreen,
                              _Out_opt_  IDXGIOutput    **ppTarget )
  {
    return GetFullscreenState_Original (This, pFullscreen, ppTarget);
  }

  COM_DECLSPEC_NOTHROW
__declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_SetFullscreenState_Override ( IDXGISwapChain *This,
                                         BOOL            Fullscreen,
                                         IDXGIOutput    *pTarget )
  {
    DXGI_LOG_CALL_I2 (L"IDXGISwapChain", L"SetFullscreenState", L"%lu, %08Xh",
                      Fullscreen, pTarget);

#if 0
    DXGI_MODE_DESC mode_desc = { 0 };

    if (Fullscreen) {
      DXGI_SWAP_CHAIN_DESC desc;
      if (SUCCEEDED (This->GetDesc (&desc))) {
        mode_desc.Format                  = desc.BufferDesc.Format;
        mode_desc.Width                   = desc.BufferDesc.Width;
        mode_desc.Height                  = desc.BufferDesc.Height;

        mode_desc.RefreshRate.Denominator = desc.BufferDesc.RefreshRate.Denominator;
        mode_desc.RefreshRate.Numerator   = desc.BufferDesc.RefreshRate.Numerator;

        mode_desc.Scaling                 = desc.BufferDesc.Scaling;
        mode_desc.ScanlineOrdering        = desc.BufferDesc.ScanlineOrdering;
        ResizeTarget_Original           (This, &mode_desc);
      }
    }
#endif

    //UINT BufferCount = max (1, min (6, config.render.framerate.buffer_count));


    HRESULT ret;
    DXGI_CALL (ret, SetFullscreenState_Original (This, Fullscreen, pTarget));

    //
    // Necessary provisions for Fullscreen Flip Mode
    //
    if (/*Fullscreen &&*/SUCCEEDED (ret)) {
      //mode_desc.RefreshRate = { 0 };
      //ResizeTarget_Original (This, &mode_desc);

      //if (bFlipMode)
        ResizeBuffers_Original (This, 0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    }

    return ret;
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_ResizeBuffers_Override ( IDXGISwapChain *This,
                               _In_ UINT            BufferCount,
                               _In_ UINT            Width,
                               _In_ UINT            Height,
                               _In_ DXGI_FORMAT     NewFormat,
                               _In_ UINT            SwapChainFlags )
  {
    DXGI_LOG_CALL_I5 (L"IDXGISwapChain", L"ResizeBuffers", L"%lu,%lu,%lu,...,0x%08X,0x%08X",
                        BufferCount, Width, Height, NewFormat, SwapChainFlags);

    if ( config.render.framerate.buffer_count != -1           &&
         config.render.framerate.buffer_count !=  BufferCount &&
         BufferCount                          !=  0 ) {
      BufferCount = config.render.framerate.buffer_count;
      dll_log.Log (L"[   DXGI   ] >> Buffer Count Override: %lu buffers", BufferCount);
    }

    // Fake it
    //if (bWait)
      //return S_OK;

    HRESULT ret;
    DXGI_CALL (ret, ResizeBuffers_Original (This, BufferCount, Width, Height, NewFormat, SwapChainFlags));

    SK_DXGI_HookPresent (This);
    MH_ApplyQueued      ();

    return ret;
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_ResizeTarget_Override ( IDXGISwapChain *This,
                        _In_ const DXGI_MODE_DESC *pNewTargetParameters )
  {
    DXGI_LOG_CALL_I5 (L"IDXGISwapChain", L"ResizeTarget", L"{ (%lux%lu@%3.1fHz), fmt=%lu, scaling=0x%02x }",
                        pNewTargetParameters->Width, pNewTargetParameters->Height,
                        pNewTargetParameters->RefreshRate.Denominator != 0 ?
                          (float)pNewTargetParameters->RefreshRate.Numerator /
                          (float)pNewTargetParameters->RefreshRate.Denominator :
                            0.0f,
                        pNewTargetParameters->Format,
                        pNewTargetParameters->Scaling );

    HRESULT ret;

    if (! lstrcmpW (SK_GetHostApp (), L"DXMD.exe")) {
      DXGI_MODE_DESC new_new_params =
        *pNewTargetParameters;

      new_new_params.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

      DXGI_MODE_DESC* pNewNewTargetParameters =
        &new_new_params;

      DXGI_CALL (ret, ResizeTarget_Original (This, pNewNewTargetParameters));
    } else {
#ifdef DXGI_CENTERED
      DXGI_MODE_DESC new_new_params =
        *pNewTargetParameters;

      new_new_params.Scaling = DXGI_MODE_SCALING_CENTERED;

      DXGI_MODE_DESC* pNewNewTargetParameters =
        &new_new_params;

      DXGI_CALL (ret, ResizeTarget_Original (This, pNewNewTargetParameters));
#else
      DXGI_CALL (ret, ResizeTarget_Original (This, pNewTargetParameters));
#endif
    }

    SK_DXGI_HookPresent (This);
    MH_ApplyQueued      ();

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE
  DXGIFactory_CreateSwapChain_Override ( IDXGIFactory          *This,
                                   _In_  IUnknown              *pDevice,
                                   _In_  DXGI_SWAP_CHAIN_DESC  *pDesc,
                                  _Out_  IDXGISwapChain       **ppSwapChain )
  {
    std::wstring iname = SK_GetDXGIFactoryInterface (This);

    bool fake = false;

    // Avoid logging for swapchains we created for the purpose of hooking...
    if ( pDesc->BufferCount                        == 1                        &&
         pDesc->SwapEffect                         == DXGI_SWAP_EFFECT_DISCARD &&
         pDesc->BufferDesc.Width                   == 800                      &&
         pDesc->BufferDesc.Height                  == 600                      &&
         pDesc->BufferDesc.RefreshRate.Denominator == 1                        &&
         pDesc->BufferDesc.RefreshRate.Numerator   == 60 )
      fake = true;

    if (! fake) {
      DXGI_LOG_CALL_I3 (iname.c_str (), L"CreateSwapChain", L"%08Xh, %08Xh, %08Xh",
                      pDevice, pDesc, ppSwapChain);
    }

    HRESULT ret;

    if (pDesc != nullptr) {
      if (! fake) {
      dll_log.LogEx ( true,
        L"[   DXGI   ]  SwapChain: (%lux%lu @ %4.1f Hz - Scaling: %s) - {%s}"
        L" [%lu Buffers] :: Flags=0x%04X, SwapEffect: %s\n",
        pDesc->BufferDesc.Width,
        pDesc->BufferDesc.Height,
        pDesc->BufferDesc.RefreshRate.Denominator > 0 ?
        (float)pDesc->BufferDesc.RefreshRate.Numerator /
        (float)pDesc->BufferDesc.RefreshRate.Denominator :
        (float)pDesc->BufferDesc.RefreshRate.Numerator,
        pDesc->BufferDesc.Scaling == 0 ?
        L"Unspecified" :
        pDesc->BufferDesc.Scaling == 1 ?
        L"Centered" : L"Stretched",
        pDesc->Windowed ? L"Windowed" : L"Fullscreen",
        pDesc->BufferCount,
        pDesc->Flags,
        pDesc->SwapEffect == 0 ?
        L"Discard" :
        pDesc->SwapEffect == 1 ?
        L"Sequential" :
        pDesc->SwapEffect == 2 ?
        L"<Unknown>" :
        pDesc->SwapEffect == 3 ?
        L"Flip Sequential" :
        pDesc->SwapEffect == 4 ?
        L"Flip Discard" : L"<Unknown>");
      }

      // Set things up to make the swap chain Alt+Enter friendly
      if (bAlwaysAllowFullscreen) {
        pDesc->Flags                             |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        pDesc->Windowed                           = true;
        pDesc->BufferDesc.RefreshRate.Denominator = 0;
        pDesc->BufferDesc.RefreshRate.Numerator   = 0;
      }

      if (! bFlipMode)
        bFlipMode =
          ( dxgi_caps.present.flip_sequential && (
            ( ! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe")) ||
              SK_DS3_UseFlipMode ()        ||
              SK_TW3_UseFlipMode () ) );

      if (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe")) {
        if (bFlipMode) {
          bFlipMode = (! SK_FO4_IsFullscreen ()) && SK_FO4_UseFlipMode ();
        }

        //bFlipMode = bFlipMode && pDesc->BufferDesc.Scaling == 0;
      } else {
        if (config.render.framerate.flip_discard)
          bFlipMode = dxgi_caps.present.flip_sequential;
      }

      //if (bFlipMode)
        //pDesc->BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;

#ifdef DXGI_CENTERED
      pDesc->BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
#endif

      bWait = bFlipMode && dxgi_caps.present.waitable;

      // We cannot change the swapchain parameters if this is used...
      bWait = bWait && config.render.framerate.swapchain_wait > 0;

      if (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe")) {
        if (SK_DS3_IsBorderless ())
          pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      if (bFlipMode) {
        if (bWait)
          pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        // Flip Presentation Model requires 3 Buffers
        config.render.framerate.buffer_count =
          std::max (3, config.render.framerate.buffer_count);

        if (config.render.framerate.flip_discard &&
            dxgi_caps.present.flip_discard)
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        else
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      }

      else {
        // Resort to triple-buffering if flip mode is not available
        if (config.render.framerate.buffer_count > 2)
          config.render.framerate.buffer_count = 2;

        pDesc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
      }

      if (config.render.framerate.buffer_count != -1)
        pDesc->BufferCount = config.render.framerate.buffer_count;

      // We cannot switch modes on a waitable swapchain
      if (bFlipMode && bWait) {
        pDesc->Flags |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      if (! fake) {
        dll_log.Log ( L"[ DXGI 1.2 ] >> Using %s Presentation Model  [Waitable: %s - %lu ms]",
                       bFlipMode ? L"Flip" : L"Traditional",
                         bWait ? L"Yes" : L"No",
                           bWait ? config.render.framerate.swapchain_wait : 0 );
      }
    }


    if (fake) {
      HRESULT hr = 
        CreateSwapChain_Original (This, pDevice, pDesc, ppSwapChain);

      if ( SUCCEEDED (hr)         &&
           ppSwapChain != nullptr &&
        (*ppSwapChain) != nullptr )
      {
        SK_DXGI_HookPresent (*ppSwapChain);
        MH_ApplyQueued      ();
      }

      return hr;
    }


    DXGI_CALL(ret, CreateSwapChain_Original (This, pDevice, pDesc, ppSwapChain));

    if ( SUCCEEDED (ret)         &&
         ppSwapChain  != nullptr &&
       (*ppSwapChain) != nullptr )
    {
      SK_DXGI_HookPresent (*ppSwapChain);
      MH_ApplyQueued      ();

      //if (bFlipMode || bWait)
        //DXGISwap_ResizeBuffers_Override (*ppSwapChain, config.render.framerate.buffer_count, pDesc->BufferDesc.Width, pDesc->BufferDesc.Height, pDesc->BufferDesc.Format, pDesc->Flags);

      const uint32_t max_latency = config.render.framerate.pre_render_limit;

      CComPtr <IDXGISwapChain2> pSwapChain2;
      if ( bFlipMode && bWait &&
           SUCCEEDED ( (*ppSwapChain)->QueryInterface (IID_PPV_ARGS (&pSwapChain2)) )
          )
      {
        if (max_latency < 16) {
          dll_log.Log (L"[   DXGI   ] Setting Swapchain Frame Latency: %lu", max_latency);
          pSwapChain2->SetMaximumFrameLatency (max_latency);
        }

        HANDLE hWait = pSwapChain2->GetFrameLatencyWaitableObject ();

        pSwapChain2.Release ();

        WaitForSingleObjectEx ( hWait,
                                  500,//config.render.framerate.swapchain_wait,
                                    TRUE );
      }
      //else
      {
        if (max_latency != -1) {
          CComPtr <IDXGIDevice1> pDevice1;

          if (SUCCEEDED ( (*ppSwapChain)->GetDevice (
                             IID_PPV_ARGS (&pDevice1)
                          )
                        )
             )
          {
            dll_log.Log (L"[   DXGI   ] Setting Device Frame Latency: %lu", max_latency);
            pDevice1->SetMaximumFrameLatency (max_latency);
          }
        }
      }

      CComPtr <ID3D11Device> pDev;

      if (SUCCEEDED ( pDevice->QueryInterface ( IID_PPV_ARGS (&pDev) )
                    )
         )
      {
        // Dangerous to hold a reference to this don't you think?!
        g_pD3D11Dev = pDev;
      }
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE
  DXGIFactory_CreateSwapChainForCoreWindow_Override ( IDXGIFactory2             *This,
                                           _In_       IUnknown                  *pDevice,
                                           _In_       IUnknown                  *pWindow,
                                           _In_ /*const*/ DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                       _In_opt_       IDXGIOutput               *pRestrictToOutput,
                                          _Out_       IDXGISwapChain1          **ppSwapChain )
  {
    std::wstring iname = SK_GetDXGIFactoryInterface (This);

    // Wrong prototype, but who cares right now? :P
    DXGI_LOG_CALL_I3 (iname.c_str (), L"CreateSwapChainForCoreWindow", L"%08Xh, %08Xh, %08Xh",
                      pDevice, pDesc, ppSwapChain);

    HRESULT ret;

    if (pDesc != nullptr) {
      dll_log.LogEx ( true,
        L"[   DXGI   ]  SwapChain: (%lux%lu - Scaling: %s) - {%s}"
        L" [%lu Buffers] :: Flags=0x%04X, SwapEffect: %s\n",
        pDesc->Width,
        pDesc->Height,
        pDesc->Scaling == 0 ?
        L"Unspecified" :
        pDesc->Scaling == 1 ?
        L"Centered" : L"Stretched",
        L"Windowed",
        pDesc->BufferCount,
        pDesc->Flags,
        pDesc->SwapEffect == 0 ?
        L"Discard" :
        pDesc->SwapEffect == 1 ?
        L"Sequential" :
        pDesc->SwapEffect == 2 ?
        L"<Unknown>" :
        pDesc->SwapEffect == 3 ?
        L"Flip Sequential" :
        pDesc->SwapEffect == 4 ?
        L"Flip Discard" : L"<Unknown>");

#if 0
      // Set things up to make the swap chain Alt+Enter friendly
      if (bAlwaysAllowFullscreen) {
        pDesc->Flags                             |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        pDesc->Windowed                           = true;
        pDesc->BufferDesc.RefreshRate.Denominator = 0;
        pDesc->BufferDesc.RefreshRate.Numerator   = 0;
      }
#endif

      if (! bFlipMode)
        bFlipMode =
          ( dxgi_caps.present.flip_sequential && (
            ( (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe")) ||
              SK_DS3_UseFlipMode ()                            ||
              SK_TW3_UseFlipMode () ) ) );

      if (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe")) {
        if (bFlipMode) {
          bFlipMode = (! SK_FO4_IsFullscreen ()) && SK_FO4_UseFlipMode ();
        }

        bFlipMode = bFlipMode && pDesc->Scaling == 0;
      } else {
        if (config.render.framerate.flip_discard)
         bFlipMode = dxgi_caps.present.flip_sequential;
      }

      bWait = bFlipMode && dxgi_caps.present.waitable;

      // We cannot change the swapchain parameters if this is used...
      bWait = bWait && config.render.framerate.swapchain_wait > 0;

      if (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe")) {
        if (SK_DS3_IsBorderless ())
          pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      if (bFlipMode) {
        if (bWait)
          pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        // Flip Presentation Model requires 3 Buffers
        config.render.framerate.buffer_count =
          std::max (3, config.render.framerate.buffer_count);

        if (config.render.framerate.flip_discard &&
            dxgi_caps.present.flip_discard)
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        else
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      }

      else {
        // Resort to triple-buffering if flip mode is not available
        if (config.render.framerate.buffer_count > 2)
          config.render.framerate.buffer_count = 2;

        pDesc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
      }

      if (config.render.framerate.buffer_count != -1)
        pDesc->BufferCount = config.render.framerate.buffer_count;

      // We cannot switch modes on a waitable swapchain
      if (bFlipMode && bWait) {
        pDesc->Flags |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      dll_log.Log ( L"[ DXGI 1.2 ] >> Using %s Presentation Model  [Waitable: %s - %lu ms]",
                     bFlipMode ? L"Flip" : L"Traditional",
                       bWait ? L"Yes" : L"No",
                         bWait ? config.render.framerate.swapchain_wait : 0 );
    }

    DXGI_CALL( ret, CreateSwapChainForCoreWindow_Original (
                      This,
                        pDevice,
                          pWindow,
                            pDesc,
                              pRestrictToOutput,
                                ppSwapChain ) );

    if ( SUCCEEDED (ret)      &&
         ppSwapChain  != NULL &&
       (*ppSwapChain) != NULL )
    {
      //if (bFlipMode || bWait)
        //DXGISwap_ResizeBuffers_Override (*ppSwapChain, config.render.framerate.buffer_count, pDesc->BufferDesc.Width, pDesc->BufferDesc.Height, pDesc->BufferDesc.Format, pDesc->Flags);

      const uint32_t max_latency = config.render.framerate.pre_render_limit;

      CComPtr <IDXGISwapChain2> pSwapChain2;
      if ( bFlipMode && bWait &&
           SUCCEEDED ( (*ppSwapChain)->QueryInterface (IID_PPV_ARGS (&pSwapChain2)) )
          )
      {
        if (max_latency < 16) {
          dll_log.Log (L"[   DXGI   ] Setting Swapchain Frame Latency: %lu", max_latency);
          pSwapChain2->SetMaximumFrameLatency (max_latency);
        }

        HANDLE hWait = pSwapChain2->GetFrameLatencyWaitableObject ();

        pSwapChain2.Release ();

        WaitForSingleObjectEx ( hWait,
                                  config.render.framerate.swapchain_wait,
                                    TRUE );
      }
      else
      {
        if (max_latency != -1) {
          CComPtr <IDXGIDevice1> pDevice1;

          if (SUCCEEDED ( (*ppSwapChain)->GetDevice (
                             IID_PPV_ARGS (&pDevice1)
                          )
                        )
             )
          {
            dll_log.Log (L"[   DXGI   ] Setting Device Frame Latency: %lu", max_latency);
            pDevice1->SetMaximumFrameLatency (max_latency);
          }
        }
      }

      CComPtr <ID3D11Device> pDev;

      if (SUCCEEDED ( pDevice->QueryInterface ( IID_PPV_ARGS (&pDev) )
                    )
         )
      {
        // Dangerous to hold a reference to this don't you think?!
        g_pD3D11Dev = pDev;
      }
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE
  DXGIFactory_CreateSwapChainForHwnd_Override ( IDXGIFactory2            *This,
                              _In_       IUnknown                        *pDevice,
                              _In_       HWND                             hWnd,
                              _In_       DXGI_SWAP_CHAIN_DESC1           *pDesc,
                          _In_opt_       DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
                          _In_opt_       IDXGIOutput                     *pRestrictToOutput,
                             _Out_       IDXGISwapChain1                 **ppSwapChain )
  {
    std::wstring iname = SK_GetDXGIFactoryInterface (This);

    // Wrong prototype, but who cares right now? :P
    DXGI_LOG_CALL_I3 (iname.c_str (), L"CreateSwapChainForHwnd", L"%08Xh, %08Xh, %08Xh",
                      pDevice, pDesc, ppSwapChain);

    HRESULT ret;

    if (pDesc != nullptr) {
      dll_log.LogEx ( true,
        L"[   DXGI   ]  SwapChain: (%lux%lu @ %4.1f Hz - Scaling: %s) - {%s}"
        L" [%lu Buffers] :: Flags=0x%04X, SwapEffect: %s\n",
        pDesc->Width,
        pDesc->Height,
        pFullscreenDesc != nullptr ?
          pFullscreenDesc->RefreshRate.Denominator > 0 ?
            (float)pFullscreenDesc->RefreshRate.Numerator /
            (float)pFullscreenDesc->RefreshRate.Denominator :
            (float)pFullscreenDesc->RefreshRate.Numerator   :
              0.0f,
        pDesc->Scaling == 0 ?
        L"Unspecified" :
        pDesc->Scaling == 1 ?
        L"Centered" : L"Stretched",
        pFullscreenDesc != nullptr ?
          pFullscreenDesc->Windowed ? L"Windowed" : L"Fullscreen" :
                                      L"Windowed",
        pDesc->BufferCount,
        pDesc->Flags,
        pDesc->SwapEffect == 0 ?
        L"Discard" :
        pDesc->SwapEffect == 1 ?
        L"Sequential" :
        pDesc->SwapEffect == 2 ?
        L"<Unknown>" :
        pDesc->SwapEffect == 3 ?
        L"Flip Sequential" :
        pDesc->SwapEffect == 4 ?
        L"Flip Discard" : L"<Unknown>");

      // D3D12 apps will actually use flip model, hurray!
      if ( pDesc->SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
           pDesc->SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL )
        bFlipMode = true;

      if (! bFlipMode)
        bFlipMode =
          ( dxgi_caps.present.flip_sequential && (
            ( (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe")) ||
              SK_DS3_UseFlipMode ()                            ||
              SK_TW3_UseFlipMode () ) ) );

      if (config.render.framerate.flip_discard)
        bFlipMode = dxgi_caps.present.flip_sequential;

      bWait = bFlipMode && dxgi_caps.present.waitable;

      // We cannot change the swapchain parameters if this is used...
      bWait = bWait && config.render.framerate.swapchain_wait > 0;

      if (bFlipMode) {
        if (bWait)
          pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        // Flip Presentation Model requires 3 Buffers
        config.render.framerate.buffer_count =
          std::max (3, config.render.framerate.buffer_count);

        if (config.render.framerate.flip_discard &&
            dxgi_caps.present.flip_discard)
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        else
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      }

      else {
        // Resort to triple-buffering if flip mode is not available
        if (config.render.framerate.buffer_count > 2)
          config.render.framerate.buffer_count = 2;

        pDesc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
      }

      if (config.render.framerate.buffer_count != -1)
        pDesc->BufferCount = config.render.framerate.buffer_count;

      // We cannot switch modes on a waitable swapchain
      if (bFlipMode && bWait) {
        pDesc->Flags |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      dll_log.Log ( L"[ DXGI 1.2 ] >> Using %s Presentation Model  [Waitable: %s - %lu ms]",
                     bFlipMode ? L"Flip" : L"Traditional",
                       bWait ? L"Yes" : L"No",
                         bWait ? config.render.framerate.swapchain_wait : 0 );
    }

    DXGI_CALL(ret, CreateSwapChainForHwnd_Original (This, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain));

    if ( SUCCEEDED (ret)      &&
         ppSwapChain  != NULL &&
       (*ppSwapChain) != NULL )
    {
      //if (bFlipMode || bWait)
        //DXGISwap_ResizeBuffers_Override (*ppSwapChain, config.render.framerate.buffer_count, pDesc->BufferDesc.Width, pDesc->BufferDesc.Height, pDesc->BufferDesc.Format, pDesc->Flags);

      const uint32_t max_latency = config.render.framerate.pre_render_limit;

      CComPtr <IDXGISwapChain2> pSwapChain2;
      if ( bFlipMode && bWait &&
           SUCCEEDED ( (*ppSwapChain)->QueryInterface (IID_PPV_ARGS (&pSwapChain2)) )
          )
      {
        if (max_latency < 16) {
          dll_log.Log (L"[   DXGI   ] Setting Swapchain Frame Latency: %lu", max_latency);
          pSwapChain2->SetMaximumFrameLatency (max_latency);
        }

        HANDLE hWait = pSwapChain2->GetFrameLatencyWaitableObject ();

        pSwapChain2.Release ();

        WaitForSingleObjectEx ( hWait,
                                  config.render.framerate.swapchain_wait,
                                    TRUE );
      }
      else
      {
        if (max_latency != -1) {
          CComPtr <IDXGIDevice1> pDevice1;

          if (SUCCEEDED ( (*ppSwapChain)->GetDevice (
                             IID_PPV_ARGS (&pDevice1)
                          )
                        )
             )
          {
            dll_log.Log (L"[   DXGI   ] Setting Device Frame Latency: %lu", max_latency);
            pDevice1->SetMaximumFrameLatency (max_latency);
          }
        }
      }

      CComPtr <ID3D11Device> pDev;

      if (SUCCEEDED ( pDevice->QueryInterface ( IID_PPV_ARGS (&pDev) )
                    )
         )
      {
        // Dangerous to hold a reference to this don't you think?!
        g_pD3D11Dev = pDev;
      }
    }

    return ret;
  }

  typedef enum skUndesirableVendors {
    Microsoft = 0x1414,
    Intel     = 0x8086
  } Vendors;

  __declspec (nothrow)
  HRESULT
  STDMETHODCALLTYPE CreateDXGIFactory (REFIID   riid,
                                 _Out_ void   **ppFactory);

  __declspec (nothrow)
    HRESULT
    STDMETHODCALLTYPE CreateDXGIFactory1 (REFIID   riid,
                                    _Out_ void   **ppFactory);

  // Do this in a thread because it is not safe to do from
  //   the thread that created the window or drives the message
  //     pump...
  unsigned int
  __stdcall
  SK_DXGI_BringRenderWindowToTop_THREAD (LPVOID user)
  {
    if (hWndRender != 0) {
      SetActiveWindow     (hWndRender);
      SetForegroundWindow (hWndRender);
      BringWindowToTop    (hWndRender);
    }

    CloseHandle (GetCurrentThread ());

    return 0;
  }

  void
  WINAPI
  SK_DXGI_AdapterOverride ( IDXGIAdapter**   ppAdapter,
                            D3D_DRIVER_TYPE* DriverType )
  {
    if (SK_DXGI_preferred_adapter == -1)
      return;

    if (EnumAdapters_Original == nullptr)
      WaitForInitDXGI ();

    IDXGIAdapter* pGameAdapter     = (*ppAdapter);
    IDXGIAdapter* pOverrideAdapter = nullptr;

    IDXGIFactory* pFactory = nullptr;

    HRESULT res;

    if ((*ppAdapter) == nullptr)
      res = E_FAIL;
    else
      res = (*ppAdapter)->GetParent (IID_PPV_ARGS (&pFactory));

    if (FAILED (res)) {
      if (SK_DXGI_use_factory1)
        res = CreateDXGIFactory1_Import (__uuidof (IDXGIFactory1), (void **)&pFactory);
      else
        res = CreateDXGIFactory_Import  (__uuidof (IDXGIFactory),  (void **)&pFactory);
    }

    if (SUCCEEDED (res)) {
      if (pFactory != nullptr) {
        if ((*ppAdapter) == nullptr)
          EnumAdapters_Original (pFactory, 0, &pGameAdapter);

        DXGI_ADAPTER_DESC game_desc;

        if (pGameAdapter != nullptr) {
          *ppAdapter  = pGameAdapter;
          *DriverType = D3D_DRIVER_TYPE_UNKNOWN;

          pGameAdapter->GetDesc (&game_desc);
        }

        if ( SK_DXGI_preferred_adapter != -1 &&
             SUCCEEDED (EnumAdapters_Original (pFactory, SK_DXGI_preferred_adapter, &pOverrideAdapter)) )
        {
          DXGI_ADAPTER_DESC override_desc;
          pOverrideAdapter->GetDesc (&override_desc);

          if ( game_desc.VendorId     == Vendors::Intel     &&
               override_desc.VendorId != Vendors::Microsoft &&
               override_desc.VendorId != Vendors::Intel )
          {
            dll_log.Log ( L"[   DXGI   ] !!! DXGI Adapter Override: (Using '%s' instead of '%s') !!!",
                          override_desc.Description, game_desc.Description );

            *ppAdapter = pOverrideAdapter;
            pGameAdapter->Release ();
          } else {
            dll_log.Log ( L"[   DXGI   ] !!! DXGI Adapter Override: (Tried '%s' instead of '%s') !!!",
                          override_desc.Description, game_desc.Description );
            //SK_DXGI_preferred_adapter = -1;
            pOverrideAdapter->Release ();
          }
        } else {
          dll_log.Log ( L"[   DXGI   ] !!! DXGI Adapter Override Failed, returning '%s' !!!",
                          game_desc.Description );
        }

        pFactory->Release ();
      }
    }
  }

  HRESULT
  STDMETHODCALLTYPE GetDesc2_Override (IDXGIAdapter2      *This,
                                _Out_  DXGI_ADAPTER_DESC2 *pDesc)
  {
    std::wstring iname = SK_GetDXGIAdapterInterface (This);

    DXGI_LOG_CALL_I2 (iname.c_str (), L"GetDesc2", L"%08Xh, %08Xh", This, pDesc);

    HRESULT ret;
    DXGI_CALL (ret, GetDesc2_Original (This, pDesc));

    //// OVERRIDE VRAM NUMBER
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0) {
      dll_log.LogEx ( true,
        L" <> GetDesc2_Override: Looking for matching NVAPI GPU for %s...: ",
        pDesc->Description );

      DXGI_ADAPTER_DESC* match =
        sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

      if (match != NULL) {
        dll_log.LogEx (false, L"Success! (%s)\n", match->Description);
        pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
      }
      else
        dll_log.LogEx (false, L"Failure! (No Match Found)\n");
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE GetDesc1_Override (IDXGIAdapter1      *This,
                                _Out_  DXGI_ADAPTER_DESC1 *pDesc)
  {
    std::wstring iname = SK_GetDXGIAdapterInterface (This);

    DXGI_LOG_CALL_I2 (iname.c_str (), L"GetDesc1", L"%08Xh, %08Xh", This, pDesc);

    HRESULT ret;
    DXGI_CALL (ret, GetDesc1_Original (This, pDesc));

    //// OVERRIDE VRAM NUMBER
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0) {
      dll_log.LogEx ( true,
        L" <> GetDesc1_Override: Looking for matching NVAPI GPU for %s...: ",
        pDesc->Description );

      DXGI_ADAPTER_DESC* match =
        sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

      if (match != NULL) {
        dll_log.LogEx (false, L"Success! (%s)\n", match->Description);
        pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
      }
      else
        dll_log.LogEx (false, L"Failure! (No Match Found)\n");
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE GetDesc_Override (IDXGIAdapter      *This,
                               _Out_  DXGI_ADAPTER_DESC *pDesc)
  {
    std::wstring iname = SK_GetDXGIAdapterInterface (This);

    DXGI_LOG_CALL_I2 (iname.c_str (), L"GetDesc",L"%08Xh, %08Xh", This, pDesc);

    HRESULT ret;
    DXGI_CALL (ret, GetDesc_Original (This, pDesc));

    //// OVERRIDE VRAM NUMBER
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0) {
      dll_log.LogEx ( true,
        L" <> GetDesc_Override: Looking for matching NVAPI GPU for %s...: ",
        pDesc->Description );

      DXGI_ADAPTER_DESC* match =
        sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

      if (match != NULL) {
        dll_log.LogEx (false, L"Success! (%s)\n", match->Description);
        pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
      }
      else
        dll_log.LogEx (false, L"Failure! (No Match Found)\n");
    }

    if (SK_GetHostApp () == L"Fallout4.exe" && SK_GetCallerName () == L"Fallout4.exe") {
      dll_log.Log ( L"[   DXGI   ] Dedicated Video: %llu, Dedicated System: %llu, Shared System: %llu",
                      pDesc->DedicatedVideoMemory,
                        pDesc->DedicatedSystemMemory,
                          pDesc->SharedSystemMemory );

      pDesc->DedicatedVideoMemory = pDesc->SharedSystemMemory;
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE EnumAdapters_Common (IDXGIFactory       *This,
                                         UINT                Adapter,
                                _Inout_  IDXGIAdapter      **ppAdapter,
                                         EnumAdapters_pfn    pFunc)
  {
    DXGI_ADAPTER_DESC desc;

    bool silent = dll_log.silent;
    dll_log.silent = true;
    {
      // Don't log this call
      (*ppAdapter)->GetDesc (&desc);
    }
    dll_log.silent = false;

    int iver = SK_GetDXGIAdapterInterfaceVer (*ppAdapter);

    // Only do this for NVIDIA SLI GPUs on Windows 10 (DXGI 1.4)
    if (false) {//nvapi_init && sk::NVAPI::CountSLIGPUs () > 0 && iver >= 3) {
      if (! GetDesc_Original) {
        DXGI_VIRTUAL_HOOK (ppAdapter, 8, "(*ppAdapter)->GetDesc",
          GetDesc_Override, GetDesc_Original, GetDesc_pfn);
      }

      if (! GetDesc1_Original) {
        CComPtr <IDXGIAdapter1> pAdapter1;

       if (SUCCEEDED ((*ppAdapter)->QueryInterface (IID_PPV_ARGS (&pAdapter1)))) {
          DXGI_VIRTUAL_HOOK (&pAdapter1, 10, "(pAdapter1)->GetDesc1",
            GetDesc1_Override, GetDesc1_Original, GetDesc1_pfn);
       }
      }

      if (! GetDesc2_Original) {
        CComPtr <IDXGIAdapter2> pAdapter2;
        if (SUCCEEDED ((*ppAdapter)->QueryInterface (IID_PPV_ARGS (&pAdapter2)))) {

          DXGI_VIRTUAL_HOOK (ppAdapter, 11, "(*pAdapter2)->GetDesc2",
            GetDesc2_Override, GetDesc2_Original, GetDesc2_pfn);
        }
      }
    }

    // Logic to skip Intel and Microsoft adapters and return only AMD / NV
    //if (lstrlenW (pDesc->Description)) {
    if (true) {
      if (! lstrlenW (desc.Description))
        dll_log.LogEx (false, L" >> Assertion filed: Zero-length adapter name!\n");

#ifdef SKIP_INTEL
      if ((desc.VendorId == Microsoft || desc.VendorId == Intel) && Adapter == 0) {
#else
      if (false) {
#endif
        // We need to release the reference we were just handed before
        //   skipping it.
        (*ppAdapter)->Release ();

        dll_log.LogEx (false,
          L"[   DXGI   ] >> (Host Application Tried To Enum Intel or Microsoft Adapter "
          L"as Adapter 0) -- Skipping Adapter '%s' <<\n", desc.Description);

        return (pFunc (This, Adapter + 1, ppAdapter));
      }
      else {
        // Only do this for NVIDIA SLI GPUs on Windows 10 (DXGI 1.4)
        if (false) { //nvapi_init && sk::NVAPI::CountSLIGPUs () > 0 && iver >= 3) {
          DXGI_ADAPTER_DESC* match =
            sk::NVAPI::FindGPUByDXGIName (desc.Description);

          if (match != NULL &&
            desc.DedicatedVideoMemory > match->DedicatedVideoMemory) {
// This creates problems in 32-bit environments...
#ifdef _WIN64
            if (sk::NVAPI::app_name != L"Fallout4.exe") {
              dll_log.Log (
                L"   # SLI Detected (Corrected Memory Total: %llu MiB -- "
                L"Original: %llu MiB)",
                match->DedicatedVideoMemory >> 20ULL,
                desc.DedicatedVideoMemory   >> 20ULL);
            } else {
              match->DedicatedVideoMemory = desc.DedicatedVideoMemory;
            }
#endif
          }
        }
      }

      dll_log.LogEx(true,L"[   DXGI   ]  @ Returned Adapter %lu: '%s' (LUID: %08X:%08X)",
        Adapter,
          desc.Description,
            desc.AdapterLuid.HighPart,
              desc.AdapterLuid.LowPart );

      //
      // Windows 8 has a software implementation, which we can detect.
      //
      CComPtr <IDXGIAdapter1> pAdapter1;
      HRESULT hr =
        (*ppAdapter)->QueryInterface (IID_PPV_ARGS (&pAdapter1));

      if (SUCCEEDED (hr)) {
        bool silence = dll_log.silent;
        dll_log.silent = true; // Temporarily disable logging

        DXGI_ADAPTER_DESC1 desc1;

        if (SUCCEEDED (pAdapter1->GetDesc1 (&desc1))) {
          dll_log.silent = silence; // Restore logging
#define DXGI_ADAPTER_FLAG_REMOTE   0x1
#define DXGI_ADAPTER_FLAG_SOFTWARE 0x2
          if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            dll_log.LogEx (false, L" <Software>");
          else
            dll_log.LogEx (false, L" <Hardware>");
          if (desc1.Flags & DXGI_ADAPTER_FLAG_REMOTE)
            dll_log.LogEx (false, L" [Remote]");
        }

        dll_log.silent = silence; // Restore logging
      }
    }

    dll_log.LogEx (false, L"\n");

    MH_ApplyQueued ();

    return S_OK;
  }

  HRESULT
  STDMETHODCALLTYPE EnumAdapters1_Override (IDXGIFactory1  *This,
                                            UINT            Adapter,
                                     _Out_  IDXGIAdapter1 **ppAdapter)
  {
    std::wstring iname = SK_GetDXGIFactoryInterface    (This);

    DXGI_LOG_CALL_I3 (iname.c_str (), L"EnumAdapters1", L"%08Xh, %u, %08Xh",
      This, Adapter, ppAdapter);

    HRESULT ret;
    DXGI_CALL (ret, EnumAdapters1_Original (This,Adapter,ppAdapter));

#if 0
    // For games that try to enumerate all adapters until the API returns failure,
    //   only override valid adapters...
    if ( SUCCEEDED (ret) &&
         SK_DXGI_preferred_adapter != -1 &&
         SK_DXGI_preferred_adapter != Adapter )
    {
      IDXGIAdapter1* pAdapter1 = nullptr;

      if (SUCCEEDED (EnumAdapters1_Original (This, SK_DXGI_preferred_adapter, &pAdapter1))) {
        dll_log.Log ( L"[   DXGI   ] (Reported values reflect user override: DXGI Adapter %lu -> %lu)",
                        Adapter, SK_DXGI_preferred_adapter );
        Adapter = SK_DXGI_preferred_adapter;

        if (pAdapter1 != nullptr)
          pAdapter1->Release ();
      }

      ret = EnumAdapters1_Original (This, Adapter, ppAdapter);
    }
#endif

    if (SUCCEEDED (ret) && ppAdapter != nullptr && (*ppAdapter) != nullptr) {
      return EnumAdapters_Common (This, Adapter, (IDXGIAdapter **)ppAdapter,
                                  (EnumAdapters_pfn)EnumAdapters1_Override);
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE EnumAdapters_Override (IDXGIFactory  *This,
                                           UINT           Adapter,
                                    _Out_  IDXGIAdapter **ppAdapter)
  {
    std::wstring iname = SK_GetDXGIFactoryInterface    (This);

    DXGI_LOG_CALL_I3 (iname.c_str (), L"EnumAdapters", L"%08Xh, %u, %08Xh",
      This, Adapter, ppAdapter);

    HRESULT ret;
    DXGI_CALL (ret, EnumAdapters_Original (This, Adapter, ppAdapter));

#if 0
    // For games that try to enumerate all adapters until the API returns failure,
    //   only override valid adapters...
    if ( SUCCEEDED (ret) &&
         SK_DXGI_preferred_adapter != -1 &&
         SK_DXGI_preferred_adapter != Adapter )
    {
      IDXGIAdapter* pAdapter = nullptr;

      if (SUCCEEDED (EnumAdapters_Original (This, SK_DXGI_preferred_adapter, &pAdapter))) {
        dll_log.Log ( L"[   DXGI   ] (Reported values reflect user override: DXGI Adapter %lu -> %lu)",
                        Adapter, SK_DXGI_preferred_adapter );
        Adapter = SK_DXGI_preferred_adapter;

        if (pAdapter != nullptr)
          pAdapter->Release ();
      }

      ret = EnumAdapters_Original (This, Adapter, ppAdapter);
    }
#endif

    if (SUCCEEDED (ret) && ppAdapter != nullptr && (*ppAdapter) != nullptr) {
      return EnumAdapters_Common (This, Adapter, ppAdapter,
                                  (EnumAdapters_pfn)EnumAdapters_Override);
    }

    return ret;
  }

  __declspec (nothrow)
    HRESULT
    STDMETHODCALLTYPE CreateDXGIFactory (REFIID   riid,
                                   _Out_ void   **ppFactory)
  {
    // For DXGI compliance, do not mix-and-match
    if (SK_DXGI_use_factory1)
      return CreateDXGIFactory1 (riid, ppFactory);

    SK_DXGI_factory_init = true;

    WaitForInit ();

    std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
    int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

    DXGI_LOG_CALL_2 (L"CreateDXGIFactory", L"%s, %08Xh",
      iname.c_str (), ppFactory);

    HRESULT ret;
    DXGI_CALL (ret, CreateDXGIFactory_Import (riid, ppFactory));

    return ret;
  }

  __declspec (nothrow)
    HRESULT
    STDMETHODCALLTYPE CreateDXGIFactory1 (REFIID   riid,
                                    _Out_ void   **ppFactory)
  {
    // For DXGI compliance, do not mix-and-match
    if ((! SK_DXGI_use_factory1) && (SK_DXGI_factory_init))
      return CreateDXGIFactory (riid, ppFactory);

    SK_DXGI_use_factory1 = true;
    SK_DXGI_factory_init = true;

    WaitForInit ();

    std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
    int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

    DXGI_LOG_CALL_2 (L"CreateDXGIFactory1", L"%s, %08Xh",
      iname.c_str (), ppFactory);

    // Windows Vista does not have this function -- wrap it with CreateDXGIFactory
    if (CreateDXGIFactory1_Import == nullptr) {
      dll_log.Log (L"[   DXGI   ]  >> Falling back to CreateDXGIFactory on Vista...");
      return CreateDXGIFactory (riid, ppFactory);
    }

    HRESULT ret;
    DXGI_CALL (ret, CreateDXGIFactory1_Import (riid, ppFactory));

    return ret;
  }

  __declspec (nothrow)
    HRESULT
    STDMETHODCALLTYPE CreateDXGIFactory2 (UINT     Flags,
                                          REFIID   riid,
                                    _Out_ void   **ppFactory)
  {
    SK_DXGI_use_factory1 = true;
    SK_DXGI_factory_init = true;

    WaitForInit ();

    std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
    int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

    DXGI_LOG_CALL_3 (L"CreateDXGIFactory2", L"0x%04X, %s, %08Xh",
      Flags, iname.c_str (), ppFactory);

    // Windows 7 does not have this function -- wrap it with CreateDXGIFactory1
    if (CreateDXGIFactory2_Import == nullptr) {
      dll_log.Log (L"[   DXGI   ]  >> Falling back to CreateDXGIFactory1 on Vista/7...");
      return CreateDXGIFactory1 (riid, ppFactory);
    }

    HRESULT ret;
    DXGI_CALL (ret, CreateDXGIFactory2_Import (Flags, riid, ppFactory));

    return ret;
  }

  DXGI_STUB (HRESULT, DXGID3D10CreateDevice,
    (HMODULE hModule, IDXGIFactory *pFactory, IDXGIAdapter *pAdapter,
      UINT    Flags,   void         *unknown,  void         *ppDevice),
    (hModule, pFactory, pAdapter, Flags, unknown, ppDevice));

  struct UNKNOWN5 {
    DWORD unknown [5];
  };

  DXGI_STUB (HRESULT, DXGID3D10CreateLayeredDevice,
    (UNKNOWN5 Unknown),
    (Unknown))

  DXGI_STUB (SIZE_T, DXGID3D10GetLayeredDeviceSize,
    (const void *pLayers, UINT NumLayers),
    (pLayers, NumLayers))

  DXGI_STUB (HRESULT, DXGID3D10RegisterLayers,
    (const void *pLayers, UINT NumLayers),
    (pLayers, NumLayers))

  DXGI_STUB_ (             DXGIDumpJournal,
               (const char *szPassThrough),
                           (szPassThrough) );
  DXGI_STUB (HRESULT, DXGIReportAdapterConfiguration,
               (DWORD dwUnknown),
                     (dwUnknown) );
}


LPVOID pfnChangeDisplaySettingsA        = nullptr;

typedef LONG (WINAPI *ChangeDisplaySettingsA_pfn)(
  _In_ DEVMODEA *lpDevMode,
  _In_ DWORD    dwFlags
);
ChangeDisplaySettingsA_pfn ChangeDisplaySettingsA_Original = nullptr;

LONG
WINAPI
ChangeDisplaySettingsA_Detour (
  _In_ DEVMODEA *lpDevMode,
  _In_ DWORD     dwFlags )
{
  static bool called = false;

  DEVMODEW dev_mode;
  dev_mode.dmSize = sizeof (DEVMODEW);

  EnumDisplaySettings (nullptr, 0, &dev_mode);

  if (dwFlags != CDS_TEST) {
    if (called)
      ChangeDisplaySettingsA_Original (NULL, CDS_RESET);

    called = true;

    return ChangeDisplaySettingsA_Original (lpDevMode, CDS_FULLSCREEN);
  } else {
    return ChangeDisplaySettingsA_Original (lpDevMode, dwFlags);
  }
}

typedef void (WINAPI *finish_pfn)(void);

void
WINAPI
SK_HookDXGI (void)
{
  static volatile ULONG hooked = FALSE;

  if (InterlockedCompareExchange (&hooked, TRUE, FALSE))
    return;

extern HMODULE __stdcall SK_GetDLL (void);

  SK_D3D11_Init ();
  SK_D3D12_Init ();

  HMODULE hBackend = 
    (SK_GetDLLRole () & DLL_ROLE::DXGI) ? backend_dll :
                                            GetModuleHandle (L"dxgi.dll");


  dll_log.Log (L"[   DXGI   ] Importing CreateDXGIFactory{1|2}");
  dll_log.Log (L"[   DXGI   ] ================================");


  if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dxgi.dll")) {
    dll_log.Log (L"[ DXGI 1.0 ]   CreateDXGIFactory:  %08Xh",
      (CreateDXGIFactory_Import =  \
        (CreateDXGIFactory_pfn)GetProcAddress (hBackend, "CreateDXGIFactory")));
    dll_log.Log (L"[ DXGI 1.1 ]   CreateDXGIFactory1: %08Xh",
      (CreateDXGIFactory1_Import = \
        (CreateDXGIFactory1_pfn)GetProcAddress (hBackend, "CreateDXGIFactory1")));
    dll_log.Log (L"[ DXGI 1.3 ]   CreateDXGIFactory2: %08Xh",
      (CreateDXGIFactory2_Import = \
        (CreateDXGIFactory2_pfn)GetProcAddress (hBackend, "CreateDXGIFactory2")));
  } else {
    if (GetProcAddress (hBackend, "CreateDXGIFactory"))
      SK_CreateDLLHook2 ( L"dxgi.dll",
                          "CreateDXGIFactory",
                          CreateDXGIFactory,
               (LPVOID *)&CreateDXGIFactory_Import );

    if (GetProcAddress (hBackend, "CreateDXGIFactory1"))
      SK_CreateDLLHook2 ( L"dxgi.dll",
                          "CreateDXGIFactory1",
                          CreateDXGIFactory1,
               (LPVOID *)&CreateDXGIFactory1_Import );

    if (GetProcAddress (hBackend, "CreateDXGIFactory2"))
      SK_CreateDLLHook2 ( L"dxgi.dll",
                          "CreateDXGIFactory2",
                          CreateDXGIFactory2,
               (LPVOID *)&CreateDXGIFactory2_Import );

    dll_log.Log (L"[ DXGI 1.0 ]   CreateDXGIFactory:  %08Xh  %s",
      (CreateDXGIFactory_Import),
        (CreateDXGIFactory_Import ? L"{ Hooked }" : L"" ) );
    dll_log.Log (L"[ DXGI 1.1 ]   CreateDXGIFactory1: %08Xh  %s",
      (CreateDXGIFactory1_Import),
        (CreateDXGIFactory1_Import ? L"{ Hooked }" : L"" ) );
    dll_log.Log (L"[ DXGI 1.3 ]   CreateDXGIFactory2: %08Xh  %s",
      (CreateDXGIFactory2_Import),
        (CreateDXGIFactory2_Import ? L"{ Hooked }" : L"" ) );

    MH_ApplyQueued ();
  }

  if (CreateDXGIFactory1_Import != nullptr) {
    SK_DXGI_use_factory1 = true;
    SK_DXGI_factory_init = true;
  }

  SK_ICommandProcessor* pCommandProc =
    SK_GetCommandProcessor ();

  pCommandProc->AddVariable ( "PresentationInterval",
          new SK_IVarStub <int> (&config.render.framerate.present_interval));
  pCommandProc->AddVariable ( "PreRenderLimit",
          new SK_IVarStub <int> (&config.render.framerate.pre_render_limit));
  pCommandProc->AddVariable ( "BufferCount",
          new SK_IVarStub <int> (&config.render.framerate.buffer_count));
  pCommandProc->AddVariable ( "UseFlipDiscard",
          new SK_IVarStub <bool> (&config.render.framerate.flip_discard));

  SK_DXGI_BeginHooking ();

  while (! InterlockedCompareExchange (&__dxgi_ready, FALSE, FALSE))
    Sleep (100UL);

  SK_D3D11_EnableHooks ();
  SK_D3D12_EnableHooks ();
}

void
WINAPI
dxgi_init_callback (finish_pfn finish)
{
  extern void SK_BootDXGI (void);
  SK_BootDXGI ();

  while (! InterlockedCompareExchange (&__dxgi_ready, FALSE, FALSE))
    Sleep (100UL);

  finish ();
}


HMODULE local_dxgi = 0;

//#define DXMD

HMODULE
SK_LoadRealDXGI (void)
{
#ifndef DXMD
  wchar_t wszBackendDLL [MAX_PATH] = { L'\0' };

#ifdef _WIN64
  GetSystemDirectory (wszBackendDLL, MAX_PATH);
#else
  BOOL bWOW64;
  ::IsWow64Process (GetCurrentProcess (), &bWOW64);

  if (bWOW64)
    GetSystemWow64Directory (wszBackendDLL, MAX_PATH);
  else
    GetSystemDirectory (wszBackendDLL, MAX_PATH);
#endif

  lstrcatW (wszBackendDLL, L"\\dxgi.dll");

  if (local_dxgi == 0)
    local_dxgi = LoadLibraryW (wszBackendDLL);
  else {
    HMODULE hMod;
    GetModuleHandleEx (0x00, wszBackendDLL, &hMod);
  }
#endif

  return local_dxgi;
}

void
SK_FreeRealDXGI (void)
{
#ifndef DXMD
  FreeLibrary (local_dxgi);
#endif
}

bool
SK::DXGI::Startup (void)
{
  return SK_StartupCore (L"dxgi", dxgi_init_callback);
}

extern "C" bool WINAPI SK_DS3_ShutdownPlugin (const wchar_t* backend);


void
SK_DXGI_HookPresent (IDXGISwapChain* pSwapChain)
{
  static LPVOID vftable_8  = nullptr;
  static LPVOID vftable_22 = nullptr;

  void** vftable = *(void***)*&pSwapChain;

  if (Present_Original != nullptr) {
    //dll_log.Log (L"Rehooking IDXGISwapChain::Present (...)");

    if (MH_OK == SK_RemoveHook (vftable [8]))
      Present_Original = nullptr;
    else {
      dll_log.Log ( L"[   DXGI   ] Altered vftable detected, rehooking "
                    L"IDXGISwapChain::Present (...)!" );
      if (MH_OK == SK_RemoveHook (vftable_8))
        Present_Original = nullptr;
    }
  }

  DXGI_VIRTUAL_HOOK ( &pSwapChain, 8,
                      "IDXGISwapChain::Present",
                       PresentCallback,
                       Present_Original,
                       PresentSwapChain_pfn );

  vftable_8 = vftable [8];


  CComPtr <IDXGISwapChain1> pSwapChain1 = nullptr;

  if (SUCCEEDED (pSwapChain->QueryInterface (IID_PPV_ARGS (&pSwapChain1)))) {
    vftable = *(void***)*&pSwapChain1;

    if (Present1_Original != nullptr) {
      //dll_log.Log (L"Rehooking IDXGISwapChain::Present1 (...)");

      if (MH_OK == SK_RemoveHook (vftable [22]))
        Present1_Original = nullptr;
      else {
        dll_log.Log ( L"[   DXGI   ] Altered vftable detected, rehooking "
                      L"IDXGISwapChain1::Present1 (...)!" );
        if (MH_OK == SK_RemoveHook (vftable_22))
          Present1_Original = nullptr;
      }
    }

    DXGI_VIRTUAL_HOOK ( &pSwapChain1, 22,
                        "IDXGISwapChain1::Present1",
                         Present1Callback,
                         Present1_Original,
                         Present1SwapChain1_pfn );

    vftable_22 = vftable [22];
  }

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Reinstall ();
}

std::wstring
SK_DXGI_FormatToString (DXGI_FORMAT fmt)
{

}

unsigned int
__stdcall
HookDXGI (LPVOID user)
{
  if (InterlockedCompareExchange (&__dxgi_ready, TRUE, TRUE)) {
    CloseHandle (GetCurrentThread ());
    return 0;
  }

CoInitializeEx (nullptr, COINIT_MULTITHREADED);
{
  dll_log.Log (L"[   DXGI   ]   Installing DXGI Hooks");

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

      DXGI_VIRTUAL_HOOK ( &pFactory,     10,
                          "IDXGIFactory::CreateSwapChain",
                           DXGIFactory_CreateSwapChain_Override,
                                       CreateSwapChain_Original,
                                       CreateSwapChain_pfn );

      // DXGI 1.1+
      if (iver > 0) {
        DXGI_VIRTUAL_HOOK ( &pFactory,   12,
                            "IDXGIFactory1::EnumAdapters1",
                             EnumAdapters1_Override,
                             EnumAdapters1_Original,
                             EnumAdapters1_pfn );

        void** vftable = *(void***)*&pFactory;

        if (EnumAdapters_Original == nullptr)
          EnumAdapters_Original = (EnumAdapters_pfn)(vftable [7]);
      } else {
        //
        // EnumAdapters actually calls EnumAdapters1 if the interface
        //   implements IDXGIFactory1...
        //
        //  >> Avoid some nasty recursion and only hook EnumAdapters if the
        //       interface version is DXGI 1.0.
        //
        DXGI_VIRTUAL_HOOK ( &pFactory,     7,
                            "IDXGIFactory::EnumAdapters",
                             EnumAdapters_Override,
                             EnumAdapters_Original,
                             EnumAdapters_pfn );
      }

      //  0 QueryInterface 
      //  1 AddRef
      //  2 Release
      //  3 SetPrivateData
      //  4 SetPrivateDataInterface
      //  5 GetPrivateData
      //  6 GetParent
      //  7 EnumAdapters
      //  8 MakeWindowAssociation
      //  9 GetWindowAssociation
      // 10 CreateSwapChain
      // 11 CreateSoftwareAdapter
      // 12 EnumAdapters1
      // 13 IsCurrent
      // 14 IsWindowedStereoEnabled
      // 15 CreateSwapChainForHwnd
      // 16 CreateSwapChainForCoreWindow
      // 17 GetSharedResourceAdapterLuid
      // 18 RegisterStereoStatusWindow
      // 19 RegisterStereoStatusEvent
      // 20 UnregisterStereoStatus
      // 21 RegisterOcclusionStatusWindow
      // 22 RegisterOcclusionStatusEvent
      // 23 UnregisterOcclusionStatus
      // 24 CreateSwapChainForComposition

      // DXGI 1.2+
      if (iver > 1) {
        DXGI_VIRTUAL_HOOK ( &pFactory,   15,
                            "IDXGIFactory2::CreateSwapChainForHwnd",
                             DXGIFactory_CreateSwapChainForHwnd_Override,
                                         CreateSwapChainForHwnd_Original,
                                         CreateSwapChainForHwnd_pfn );
        DXGI_VIRTUAL_HOOK ( &pFactory,   16,
                            "IDXGIFactory2::CreateSwapChainForCoreWindow",
                             DXGIFactory_CreateSwapChainForCoreWindow_Override,
                                         CreateSwapChainForCoreWindow_Original,
                                         CreateSwapChainForCoreWindow_pfn );
      }
    }

    DXGI_SWAP_CHAIN_DESC desc;
    ZeroMemory (&desc, sizeof desc);

    desc.BufferDesc.Width                   = 800;
    desc.BufferDesc.Height                  = 600;

    desc.BufferDesc.RefreshRate.Numerator   = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;

    desc.SampleDesc.Count                   = 1;
    desc.SampleDesc.Quality                 = 0;

    desc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount                        = 1;

    HWND hwnd =
      CreateWindowW ( L"STATIC", L"Dummy DXGI Window",
                        WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                            800, 600, 0,
                              nullptr, nullptr, 0x00 );

    desc.OutputWindow = hwnd;
    desc.Windowed     = TRUE;

    desc.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;

    CComPtr <ID3D11Device>        pDevice           = nullptr;
    D3D_FEATURE_LEVEL             featureLevel;
    CComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;

    HRESULT hrx = 
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


    CComPtr <IDXGISwapChain> pSwapChain = nullptr;

    if ( SUCCEEDED (pFactory->CreateSwapChain ( pDevice, &desc, &pSwapChain )) &&
         pSwapChain != nullptr) {
      DXGI_VIRTUAL_HOOK ( &pSwapChain, 10, "IDXGISwapChain::SetFullscreenState",
                                DXGISwap_SetFullscreenState_Override,
                                         SetFullscreenState_Original,
                                           SetFullscreenState_pfn );

      DXGI_VIRTUAL_HOOK ( &pSwapChain, 11, "IDXGISwapChain::GetFullscreenState",
                                DXGISwap_GetFullscreenState_Override,
                                         GetFullscreenState_Original,
                                           GetFullscreenState_pfn );

      DXGI_VIRTUAL_HOOK ( &pSwapChain, 13, "IDXGISwapChain::ResizeBuffers",
                               DXGISwap_ResizeBuffers_Override,
                                        ResizeBuffers_Original,
                                          ResizeBuffers_pfn );

      DXGI_VIRTUAL_HOOK ( &pSwapChain, 14, "IDXGISwapChain::ResizeTarget",
                               DXGISwap_ResizeTarget_Override,
                                        ResizeTarget_Original,
                                          ResizeTarget_pfn );

      CComPtr <IDXGIOutput> pOutput = nullptr;
      if (SUCCEEDED (pSwapChain->GetContainingOutput (&pOutput))) {
        if (pOutput != nullptr) {
          DXGI_VIRTUAL_HOOK ( &pOutput, 8, "IDXGIOutput::GetDisplayModeList",
                                    DXGIOutput_GetDisplayModeList_Override,
                                               GetDisplayModeList_Original,
                                               GetDisplayModeList_pfn );

          DXGI_VIRTUAL_HOOK ( &pOutput, 9, "IDXGIOutput::FindClosestMatchingMode",
                                    DXGIOutput_FindClosestMatchingMode_Override,
                                               FindClosestMatchingMode_Original,
                                               FindClosestMatchingMode_pfn );

          DXGI_VIRTUAL_HOOK ( &pOutput, 10, "IDXGIOutput::WaitForVBlank",
                                   DXGIOutput_WaitForVBlank_Override,
                                              WaitForVBlank_Original,
                                              WaitForVBlank_pfn );
        }
      }

      SK_DXGI_HookPresent (pSwapChain);

      MH_ApplyQueued ();

      InterlockedExchange (&__dxgi_ready, TRUE);
    }

    DestroyWindow (hwnd);
  }

}
CoUninitialize ();

  CloseHandle (GetCurrentThread ());

  return 0;
}


bool
SK::DXGI::Shutdown (void)
{
  if (sk::NVAPI::app_name == L"DarkSoulsIII.exe") {
    SK_DS3_ShutdownPlugin (L"dxgi");
  }

  SK_D3D11_Shutdown ();
  SK_D3D12_Shutdown ();

  return SK_ShutdownCore (L"dxgi");
}

void
WINAPI
SK_DXGI_SetPreferredAdapter (int override_id)
{
  SK_DXGI_preferred_adapter = override_id;
}


#if 0
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
#define PSAPI_VERSION 1

#include <Windows.h>
#include <psapi.h>

#pragma comment (lib, "psapi.lib")


#include "dxgi_interfaces.h"
#include "dxgi_backend.h"

#include <d3d11.h>
#include <d3d11_1.h>

#include <atlbase.h>

#include "nvapi.h"
#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include "log.h"

#include "core.h"
#include "command.h"
#include "framerate.h"

#include <set>
#include <queue>
#include <vector>
#include <algorithm>
#include <functional>


extern LARGE_INTEGER SK_QueryPerf (void);

struct sk_window_s {
  HWND    hWnd;
  WNDPROC WndProc_Original;
};

HMODULE                      SK::DXGI::hModD3D11 = 0;
SK::DXGI::PipelineStatsD3D11 SK::DXGI::pipeline_stats_d3d11;

DWORD         dwRenderThread = 0x0000;
volatile bool __d3d11_ready  = false;

void WaitForInitD3D11 (void)
{
  while (! __d3d11_ready) ;
}

extern sk_window_s game_window;

extern std::wstring host_app;

extern BOOL __stdcall SK_NvAPI_SetFramerateLimit (uint32_t limit);
extern void __stdcall SK_NvAPI_SetAppFriendlyName (const wchar_t* wszFriendlyName);

HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D );

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

extern "C++" bool SK_FO4_UseFlipMode        (void);
extern "C++" bool SK_FO4_IsFullscreen       (void);
extern "C++" bool SK_FO4_IsBorderlessWindow (void);

extern "C++" HRESULT STDMETHODCALLTYPE
                  SK_FO4_PresentFirstFrame   (IDXGISwapChain *, UINT, UINT);


extern "C++" bool SK_TW3_UseFlipMode         (void);

extern "C++" HRESULT STDMETHODCALLTYPE
                  SK_TW3_PresentFirstFrame   (IDXGISwapChain *, UINT, UINT);


// TODO: Get this stuff out of here, it's breaking what _DSlittle design work there is.
extern "C++" void SK_DS3_InitPlugin         (void);
extern "C++" bool SK_DS3_UseFlipMode        (void);
extern "C++" bool SK_DS3_IsBorderless       (void);

extern "C++" HRESULT STDMETHODCALLTYPE
                  SK_DS3_PresentFirstFrame   (IDXGISwapChain *, UINT, UINT);

void  __stdcall SK_D3D11_TexCacheCheckpoint    ( void);
bool  __stdcall SK_D3D11_TextureIsCached       ( ID3D11Texture2D*     pTex );
void  __stdcall SK_D3D11_UseTexture            ( ID3D11Texture2D*     pTex );
void  __stdcall SK_D3D11_RemoveTexFromCache    ( ID3D11Texture2D*     pTex );

void  __stdcall SK_D3D11_UpdateRenderStats     ( IDXGISwapChain*      pSwapChain );

//#define FULL_RESOLUTION

D3DX11CreateTextureFromFileW_pfn D3DX11CreateTextureFromFileW = nullptr;
D3DX11GetImageInfoFromFileW_pfn  D3DX11GetImageInfoFromFileW  = nullptr;
HMODULE                          hModD3DX11_43                = nullptr;

/*static __declspec (thread)*/ bool tex_inject = false;

#pragma comment (lib, "dxguid.lib")

// {C3108560-7335-4EE1-A934-A1236D2D3C08}
static const GUID IID_SKTextureD3D11 = 
{ 0xc3108560, 0x7335, 0x4ee1, { 0xa9, 0x34, 0xa1, 0x23, 0x6d, 0x2d, 0x3c, 0x8 } };

interface ISKTextureD3D11 : public ID3D11Texture2D
{
public:
  ISKTextureD3D11 (ID3D11Texture2D **ppTex, SIZE_T size, uint32_t crc32) {
     pTexOverride  = nullptr;
     can_free      = true;
     override_size = 0;
     last_used.QuadPart
                   = 0ULL;
     pTex          = *ppTex;
    *ppTex         =  this;
     tex_size      = size;
     tex_crc32     = crc32;
     must_block    = false;
     refs          =  1;
  };

  /*** IUnknown methods ***/
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) {
    if (IsEqualGUID (riid, IID_SKTextureD3D11)) {
      *ppvObj = this;
      return S_OK;
    }

    if ( /*IsEqualGUID (riid, IID_IUnknown)        ||*/
         IsEqualGUID (riid, IID_ID3D11Texture2D) /*||
         IsEqualGUID (riid, IID_ID3D11Resource)  ||
         IsEqualGUID (riid, IID_ID3D11DeviceChild)*/ )
    {
      AddRef ();
      *ppvObj = this;
      return S_OK;
    }

    return pTex->QueryInterface (riid, ppvObj);
  }
  STDMETHOD_(ULONG,AddRef)(THIS) {
    can_free = false;

    ULONG ret = InterlockedIncrement (&refs);

    SK_D3D11_UseTexture (this);

    return ret;
  }
  STDMETHOD_(ULONG,Release)(THIS) {
    ULONG ret = InterlockedDecrement (&refs);

    if (ret == 1) {
      can_free = true;

      SK_D3D11_RemoveTexFromCache (this);

      can_free = false;

      // Does not delete this immediately; defers the
      //   process until the next cached texture load.
///////      sk::d3d9::tex_mgr.removeTexture (this);
    }

    return ret;
  }

  /*** ID3D11DeviceChild methods ***/

  STDMETHOD_(void,GetDevice)(THIS_ ID3D11Device **ppDevice) {
    return pTex->GetDevice (ppDevice);
  }

  STDMETHOD(GetPrivateData)(THIS_ REFGUID guid, UINT *pDataSize, void *pData) {
    return pTex->GetPrivateData (guid, pDataSize, pData);
  }

  STDMETHOD(SetPrivateData)(THIS_ REFGUID guid, UINT DataSize, const void *pData) {
    return pTex->SetPrivateData (guid, DataSize, pData);
  }

  STDMETHOD(SetPrivateDataInterface)(THIS_ REFGUID guid, const IUnknown *pData) {
    return pTex->SetPrivateDataInterface (guid, pData);
  }

  STDMETHOD_(void,GetType)(THIS_ D3D11_RESOURCE_DIMENSION *pResourceDimension) {
    return pTex->GetType (pResourceDimension);
  }

  STDMETHOD_(void,SetEvictionPriority)(THIS_ UINT EvictionPriority) {
    return pTex->SetEvictionPriority (EvictionPriority);
  }

  STDMETHOD_(UINT,GetEvictionPriority)(THIS) {
    return pTex->GetEvictionPriority ();
  }

  STDMETHOD_(void,GetDesc)(THIS_ D3D11_TEXTURE2D_DESC *pDesc) {
    return pTex->GetDesc (pDesc);
  }

  bool             can_free;      // Whether or not we can free this texture
  bool             must_block;    // Whether or not to draw using this texture before its
                                  //  override finishes streaming

  ID3D11Texture2D* pTex;          // The original texture data
  SSIZE_T          tex_size;      //   Original data size
  uint32_t         tex_crc32;     //   Original data checksum

  ID3D11Texture2D* pTexOverride;  // The overridden texture data (nullptr if unchanged)
  SSIZE_T          override_size; //   Override data size

  ULONG            refs;
  LARGE_INTEGER    last_used;     // The last time this texture was used (for rendering)
                                  //   different from the last time referenced, this is
                                  //     set when SetTexture (...) is called.
};

extern "C" unsigned int __stdcall SK_DXGI_BringRenderWindowToTop_THREAD (LPVOID);

void
WINAPI
SK_DXGI_BringRenderWindowToTop (void)
{
  _beginthreadex ( nullptr,
                     0,
                       SK_DXGI_BringRenderWindowToTop_THREAD,
                         nullptr,
                           0,
                             nullptr );
}


class SK_AutoCriticalSection {
public:
  SK_AutoCriticalSection ( CRITICAL_SECTION* pCS,
                           bool              try_only = false )
  {
    cs_ = pCS;

    if (try_only)
      TryEnter ();
    else {
      Enter ();
    }
  }

  ~SK_AutoCriticalSection (void)
  {
    Leave ();
  }

  bool try_result (void)
  {
    return acquired_;
  }

protected:
  bool TryEnter (_Acquires_lock_(* this->cs_) void)
  {
    return (acquired_ = (TryEnterCriticalSection (cs_) != FALSE));
  }

  void Enter (_Acquires_lock_(* this->cs_) void)
  {
    EnterCriticalSection (cs_);

    acquired_ = true;
  }

  void Leave (_Releases_lock_(* this->cs_) void)
  {
    if (acquired_ != false)
      LeaveCriticalSection (cs_);

    acquired_ = false;
  }

private:
  bool              acquired_;
  CRITICAL_SECTION* cs_;
};

extern int                      gpu_prio;

bool             bAlwaysAllowFullscreen = false;
HWND             hWndRender             = 0;
ID3D11Device*    g_pD3D11Dev            = nullptr;
IDXGISwapChain*  g_pDXGISwap            = nullptr;

bool bFlipMode = false;
bool bWait     = false;

// Used for integrated GPU override
int              SK_DXGI_preferred_adapter = -1;

bool
WINAPI
SK_DXGI_EnableFlipMode (bool bFlip)
{
  bool before = bFlipMode;

  bFlipMode = bFlip;

  return before;
}

void
WINAPI
SKX_D3D11_EnableFullscreen (bool bFullscreen)
{
  bAlwaysAllowFullscreen = bFullscreen;
}

bool __stdcall SK_D3D11_TextureIsCached    (ID3D11Texture2D* pTex);
void __stdcall SK_D3D11_RemoveTexFromCache (ID3D11Texture2D* pTex);


struct dxgi_caps_t {
  struct {
    bool latency_control = false;
    bool enqueue_event   = false;
  } device;

  struct {
    bool flip_sequential = false;
    bool flip_discard    = false;
    bool waitable        = false;
  } present;
} dxgi_caps;

extern "C" {
  typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory2_pfn) \
    (UINT Flags, REFIID riid,  void** ppFactory);
  typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory1_pfn) \
    (REFIID riid,  void** ppFactory);
  typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory_pfn)  \
    (REFIID riid,  void** ppFactory);

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

  typedef HRESULT (STDMETHODCALLTYPE *PresentSwapChain_pfn)(
                                         IDXGISwapChain *This,
                                         UINT            SyncInterval,
                                         UINT            Flags);

  typedef HRESULT (STDMETHODCALLTYPE *Present1SwapChain1_pfn)(
                                         IDXGISwapChain1         *This,
                                         UINT                     SyncInterval,
                                         UINT                     Flags,
                                   const DXGI_PRESENT_PARAMETERS *pPresentParameters);

  typedef HRESULT (STDMETHODCALLTYPE *CreateSwapChain_pfn)(
                                         IDXGIFactory          *This,
                                   _In_  IUnknown              *pDevice,
                                   _In_  DXGI_SWAP_CHAIN_DESC  *pDesc,
                                  _Out_  IDXGISwapChain       **ppSwapChain);

  typedef HRESULT (STDMETHODCALLTYPE *CreateSwapChainForHwnd_pfn)(
                                         IDXGIFactory2                   *This,
                              _In_       IUnknown                        *pDevice,
                              _In_       HWND                             hWnd,
                              _In_ const DXGI_SWAP_CHAIN_DESC1           *pDesc,
                          _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
                          _In_opt_       IDXGIOutput                     *pRestrictToOutput,
                             _Out_       IDXGISwapChain1                 **ppSwapChain);

  typedef HRESULT (STDMETHODCALLTYPE *SetFullscreenState_pfn)(
                                         IDXGISwapChain *This,
                                         BOOL            Fullscreen,
                                         IDXGIOutput    *pTarget);

  typedef HRESULT (STDMETHODCALLTYPE *GetFullscreenState_pfn)(
                                         IDXGISwapChain  *This,
                              _Out_opt_  BOOL            *pFullscreen,
                              _Out_opt_  IDXGIOutput    **ppTarget );

  typedef HRESULT (STDMETHODCALLTYPE *ResizeBuffers_pfn)(
                                         IDXGISwapChain *This,
                              /* [in] */ UINT            BufferCount,
                              /* [in] */ UINT            Width,
                              /* [in] */ UINT            Height,
                              /* [in] */ DXGI_FORMAT     NewFormat,
                              /* [in] */ UINT            SwapChainFlags);

  typedef HRESULT (STDMETHODCALLTYPE *ResizeTarget_pfn)(
                                    _In_ IDXGISwapChain  *This,
                              _In_ const DXGI_MODE_DESC  *pNewTargetParameters );

  typedef HRESULT (STDMETHODCALLTYPE *GetDisplayModeList_pfn)(
                                         IDXGIOutput     *This,
                              /* [in] */ DXGI_FORMAT      EnumFormat,
                              /* [in] */ UINT             Flags,
                              /* [annotation][out][in] */
                                _Inout_  UINT            *pNumModes,
                              /* [annotation][out] */
_Out_writes_to_opt_(*pNumModes,*pNumModes)
                                         DXGI_MODE_DESC *pDesc );

  typedef HRESULT (STDMETHODCALLTYPE *FindClosestMatchingMode_pfn)(
                                         IDXGIOutput    *This,
                             /* [annotation][in] */
                             _In_  const DXGI_MODE_DESC *pModeToMatch,
                             /* [annotation][out] */
                             _Out_       DXGI_MODE_DESC *pClosestMatch,
                             /* [annotation][in] */
                              _In_opt_  IUnknown *pConcernedDevice );

  typedef HRESULT (STDMETHODCALLTYPE *WaitForVBlank_pfn)(
                                         IDXGIOutput    *This );


  typedef HRESULT (STDMETHODCALLTYPE *GetDesc1_pfn)(IDXGIAdapter1      *This,
                                           _Out_    DXGI_ADAPTER_DESC1 *pDesc);
  typedef HRESULT (STDMETHODCALLTYPE *GetDesc2_pfn)(IDXGIAdapter2      *This,
                                             _Out_  DXGI_ADAPTER_DESC2 *pDesc);
  typedef HRESULT (STDMETHODCALLTYPE *GetDesc_pfn) (IDXGIAdapter       *This,
                                             _Out_  DXGI_ADAPTER_DESC  *pDesc);

  typedef HRESULT (STDMETHODCALLTYPE *EnumAdapters_pfn)(
                                          IDXGIFactory  *This,
                                          UINT           Adapter,
                                    _Out_ IDXGIAdapter **ppAdapter);

  typedef HRESULT (STDMETHODCALLTYPE *EnumAdapters1_pfn)(
                                          IDXGIFactory1  *This,
                                          UINT            Adapter,
                                    _Out_ IDXGIAdapter1 **ppAdapter);

  volatile
    D3D11CreateDeviceAndSwapChain_pfn
    D3D11CreateDeviceAndSwapChain_Import = nullptr;

  volatile
    D3D11CreateDevice_pfn
    D3D11CreateDevice_Import = nullptr;

  CreateSwapChain_pfn     CreateSwapChain_Original     = nullptr;
  PresentSwapChain_pfn    Present_Original             = nullptr;
  Present1SwapChain1_pfn  Present1_Original            = nullptr;
  SetFullscreenState_pfn  SetFullscreenState_Original  = nullptr;
  GetFullscreenState_pfn  GetFullscreenState_Original  = nullptr;
  ResizeBuffers_pfn       ResizeBuffers_Original       = nullptr;
  ResizeTarget_pfn        ResizeTarget_Original        = nullptr;

  GetDisplayModeList_pfn      GetDisplayModeList_Original      = nullptr;
  FindClosestMatchingMode_pfn FindClosestMatchingMode_Original = nullptr;
  WaitForVBlank_pfn           WaitForVBlank_Original           = nullptr;
  CreateSwapChainForHwnd_pfn  CreateSwapChainForHwnd_Original  = nullptr;

  GetDesc_pfn             GetDesc_Original             = nullptr;
  GetDesc1_pfn            GetDesc1_Original            = nullptr;
  GetDesc2_pfn            GetDesc2_Original            = nullptr;

  EnumAdapters_pfn        EnumAdapters_Original        = nullptr;
  EnumAdapters1_pfn       EnumAdapters1_Original       = nullptr;

  CreateDXGIFactory_pfn   CreateDXGIFactory_Import     = nullptr;
  CreateDXGIFactory1_pfn  CreateDXGIFactory1_Import    = nullptr;
  CreateDXGIFactory2_pfn  CreateDXGIFactory2_Import    = nullptr;

  const wchar_t*
  SK_DescribeVirtualProtectFlags (DWORD dwProtect)
  {
    switch (dwProtect)
    {
    case 0x10:
      return L"Execute";
    case 0x20:
      return L"Execute + Read-Only";
    case 0x40:
      return L"Execute + Read/Write";
    case 0x80:
      return L"Execute + Read-Only or Copy-on-Write)";
    case 0x01:
      return L"No Access";
    case 0x02:
      return L"Read-Only";
    case 0x04:
      return L"Read/Write";
    case 0x08:
      return L" Read-Only or Copy-on-Write";
    default:
      return L"UNKNOWN";
    }
  }
#define DXGI_CALL(_Ret, _Call) {                                     \
  dll_log.LogEx (true, L"[   DXGI   ]  Calling original function: ");\
  (_Ret) = (_Call);                                                  \
  dll_log.LogEx (false, L" (ret=%s)\n", SK_DescribeHRESULT (_Ret)); \
}

  // Interface-based DXGI call
#define DXGI_LOG_CALL_I(_Interface,_Name,_Format)                           \
  dll_log.LogEx (true, L"[   DXGI   ] [!] %s::%s (", _Interface, _Name);    \
  dll_log.LogEx (false, _Format
  // Global DXGI call
#define DXGI_LOG_CALL(_Name,_Format)                                        \
  dll_log.LogEx (true, L"[   DXGI   ] [!] %s (", _Name);                    \
  dll_log.LogEx (false, _Format
#define DXGI_LOG_CALL_END                                                   \
  dll_log.LogEx (false, L") -- [Calling Thread: 0x%04x]\n",                 \
    GetCurrentThreadId ());

#define DXGI_LOG_CALL_I0(_Interface,_Name) {                                 \
  DXGI_LOG_CALL_I   (_Interface,_Name, L"void"));                            \
  DXGI_LOG_CALL_END                                                          \
}

#define DXGI_LOG_CALL_I1(_Interface,_Name,_Format,_Args) {                   \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args);                     \
  DXGI_LOG_CALL_END                                                          \
}

#define DXGI_LOG_CALL_I2(_Interface,_Name,_Format,_Args0,_Args1) {           \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args0, _Args1);            \
  DXGI_LOG_CALL_END                                                          \
}

#define DXGI_LOG_CALL_I3(_Interface,_Name,_Format,_Args0,_Args1,_Args2) {    \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args0, _Args1, _Args2);    \
  DXGI_LOG_CALL_END                                                          \
}
#define DXGI_LOG_CALL_I5(_Interface,_Name,_Format,_Args0,_Args1,_Args2,      \
                         _Args3,_Args4) {                                    \
  DXGI_LOG_CALL_I   (_Interface,_Name, _Format), _Args0, _Args1, _Args2,     \
                                                 _Args3, _Args4);            \
  DXGI_LOG_CALL_END                                                          \
}


#define DXGI_LOG_CALL_0(_Name) {                               \
  DXGI_LOG_CALL   (_Name, L"void"));                           \
  DXGI_LOG_CALL_END                                            \
}

#define DXGI_LOG_CALL_1(_Name,_Format,_Args0) {                \
  DXGI_LOG_CALL   (_Name, _Format), _Args0);                   \
  DXGI_LOG_CALL_END                                            \
}

#define DXGI_LOG_CALL_2(_Name,_Format,_Args0,_Args1) {         \
  DXGI_LOG_CALL     (_Name, _Format), _Args0, _Args1);         \
  DXGI_LOG_CALL_END                                            \
}

#define DXGI_LOG_CALL_3(_Name,_Format,_Args0,_Args1,_Args2) {  \
  DXGI_LOG_CALL     (_Name, _Format), _Args0, _Args1, _Args2); \
  DXGI_LOG_CALL_END                                            \
}

#define DXGI_STUB(_Return, _Name, _Proto, _Args)                            \
  __declspec (nothrow) _Return STDMETHODCALLTYPE                            \
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
          L"Unable to locate symbol  %s in dxgi.dll",                       \
          L#_Name);                                                         \
        return E_NOTIMPL;                                                   \
      }                                                                     \
    }                                                                       \
                                                                            \
    dll_log.Log (L"[   DXGI   ] [!] %s %s - "                               \
             L"[Calling Thread: 0x%04x]",                                   \
      L#_Name, L#_Proto, GetCurrentThreadId ());                            \
                                                                            \
    return _default_impl _Args;                                             \
}

  extern "C++" {
    int
    SK_GetDXGIFactoryInterfaceVer (const IID& riid)
    {
      if (riid == __uuidof (IDXGIFactory))
        return 0;
      if (riid == __uuidof (IDXGIFactory1))
        return 1;
      if (riid == __uuidof (IDXGIFactory2))
        return 2;
      if (riid == __uuidof (IDXGIFactory3))
        return 3;
      if (riid == __uuidof (IDXGIFactory4))
        return 4;

      //assert (false);
      return -1;
    }

    std::wstring
    SK_GetDXGIFactoryInterfaceEx (const IID& riid)
    {
      std::wstring interface_name;

      if (riid == __uuidof (IDXGIFactory))
        interface_name = L"IDXGIFactory";
      else if (riid == __uuidof (IDXGIFactory1))
        interface_name = L"IDXGIFactory1";
      else if (riid == __uuidof (IDXGIFactory2))
        interface_name = L"IDXGIFactory2";
      else if (riid == __uuidof (IDXGIFactory3))
        interface_name = L"IDXGIFactory3";
      else if (riid == __uuidof (IDXGIFactory4))
        interface_name = L"IDXGIFactory4";
      else {
        wchar_t *pwszIID;

        if (SUCCEEDED (StringFromIID (riid, (LPOLESTR *)&pwszIID)))
        {
          interface_name = pwszIID;
          CoTaskMemFree (pwszIID);
        }
      }

      return interface_name;
    }

    int
    SK_GetDXGIFactoryInterfaceVer (IUnknown *pFactory)
    {
      CComPtr <IUnknown> pTemp;

      if (SUCCEEDED (
        pFactory->QueryInterface (__uuidof (IDXGIFactory4), (void **)&pTemp)))
      {
        dxgi_caps.device.enqueue_event    = true;
        dxgi_caps.device.latency_control  = true;
        dxgi_caps.present.flip_sequential = true;
        dxgi_caps.present.waitable        = true;
        dxgi_caps.present.flip_discard    = true;
        return 4;
      }
      if (SUCCEEDED (
        pFactory->QueryInterface (__uuidof (IDXGIFactory3), (void **)&pTemp)))
      {
        dxgi_caps.device.enqueue_event    = true;
        dxgi_caps.device.latency_control  = true;
        dxgi_caps.present.flip_sequential = true;
        dxgi_caps.present.waitable        = true;
        return 3;
      }

      if (SUCCEEDED (
        pFactory->QueryInterface (__uuidof (IDXGIFactory2), (void **)&pTemp)))
      {
        dxgi_caps.device.enqueue_event    = true;
        dxgi_caps.device.latency_control  = true;
        dxgi_caps.present.flip_sequential = true;
        return 2;
      }

      if (SUCCEEDED (
        pFactory->QueryInterface (__uuidof (IDXGIFactory1), (void **)&pTemp)))
      {
        dxgi_caps.device.latency_control  = true;
        return 1;
      }

      if (SUCCEEDED (
        pFactory->QueryInterface (__uuidof (IDXGIFactory), (void **)&pTemp)))
      {
        return 0;
      }

      //assert (false);
      return -1;
    }

    std::wstring
    SK_GetDXGIFactoryInterface (IUnknown *pFactory)
    {
      int iver = SK_GetDXGIFactoryInterfaceVer (pFactory);

      if (iver == 4)
        return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory4));

      if (iver == 3)
        return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory3));

      if (iver == 2)
        return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory2));

      if (iver == 1)
        return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory1));

      if (iver == 0)
        return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory));

      return L"{Invalid-Factory-UUID}";
    }

    int
    SK_GetDXGIAdapterInterfaceVer (const IID& riid)
    {
      if (riid == __uuidof (IDXGIAdapter))
        return 0;
      if (riid == __uuidof (IDXGIAdapter1))
        return 1;
      if (riid == __uuidof (IDXGIAdapter2))
        return 2;
      if (riid == __uuidof (IDXGIAdapter3))
        return 3;

      //assert (false);
      return -1;
    }

    std::wstring
    SK_GetDXGIAdapterInterfaceEx (const IID& riid)
    {
      std::wstring interface_name;

      if (riid == __uuidof (IDXGIAdapter))
        interface_name = L"IDXGIAdapter";
      else if (riid == __uuidof (IDXGIAdapter1))
        interface_name = L"IDXGIAdapter1";
      else if (riid == __uuidof (IDXGIAdapter2))
        interface_name = L"IDXGIAdapter2";
      else if (riid == __uuidof (IDXGIAdapter3))
        interface_name = L"IDXGIAdapter3";
      else {
        wchar_t* pwszIID;

        if (SUCCEEDED (StringFromIID (riid, (LPOLESTR *)&pwszIID)))
        {
          interface_name = pwszIID;
          CoTaskMemFree (pwszIID);
        }
      }

      return interface_name;
    }

    int
    SK_GetDXGIAdapterInterfaceVer (IUnknown *pAdapter)
    {
      CComPtr <IUnknown> pTemp;

      if (SUCCEEDED(
        pAdapter->QueryInterface (__uuidof (IDXGIAdapter3), (void **)&pTemp)))
      {
        return 3;
      }

      if (SUCCEEDED(
        pAdapter->QueryInterface (__uuidof (IDXGIAdapter2), (void **)&pTemp)))
      {
        return 2;
      }

      if (SUCCEEDED(
        pAdapter->QueryInterface (__uuidof (IDXGIAdapter1), (void **)&pTemp)))
      {
        return 1;
      }

      if (SUCCEEDED(
        pAdapter->QueryInterface (__uuidof (IDXGIAdapter), (void **)&pTemp)))
      {
        return 0;
      }

      //assert (false);
      return -1;
    }

    std::wstring
    SK_GetDXGIAdapterInterface (IUnknown *pAdapter)
    {
      int iver = SK_GetDXGIAdapterInterfaceVer (pAdapter);

      if (iver == 3)
        return SK_GetDXGIAdapterInterfaceEx (__uuidof (IDXGIAdapter3));

      if (iver == 2)
        return SK_GetDXGIAdapterInterfaceEx (__uuidof (IDXGIAdapter2));

      if (iver == 1)
        return SK_GetDXGIAdapterInterfaceEx (__uuidof (IDXGIAdapter1));

      if (iver == 0)
        return SK_GetDXGIAdapterInterfaceEx (__uuidof (IDXGIAdapter));

      return L"{Invalid-Adapter-UUID}";
    }
  }

MH_STATUS
WINAPI
SK_CreateVFTableHook ( LPCWSTR pwszFuncName,
                       LPVOID *ppVFTable,
                       DWORD   dwOffset,
                       LPVOID  pDetour,
                       LPVOID *ppOriginal )
{
  MH_STATUS ret =
    SK_CreateFuncHook (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal );

  if (ret == MH_OK)
    ret = SK_EnableHook (ppVFTable [dwOffset]);

  return ret;
}

#define __PTR_SIZE   sizeof LPCVOID
#define __PAGE_PRIVS PAGE_EXECUTE_READWRITE

#if 1
#define DXGI_VIRTUAL_OVERRIDE(_Base,_Index,_Name,_Override,_Original,_Type) { \
  void** vftable = *(void***)*_Base;                                          \
                                                                              \
  if (vftable [_Index] != _Override) {                                        \
    DWORD dwProtect;                                                          \
                                                                              \
    VirtualProtect (&vftable [_Index], __PTR_SIZE, __PAGE_PRIVS, &dwProtect); \
                                                                              \
    /*dll_log.Log (L" Old VFTable entry for %s: %08Xh  (Memory Policy: %s)",*/\
                 /*L##_Name, vftable [_Index],                              */\
                 /*SK_DescribeVirtualProtectFlags (dwProtect));             */\
                                                                              \
    if (_Original == NULL)                                                    \
      _Original = (##_Type)vftable [_Index];                                  \
                                                                              \
    /*dll_log.Log (L"  + %s: %08Xh", L#_Original, _Original);*/               \
                                                                              \
    vftable [_Index] = _Override;                                             \
                                                                              \
    VirtualProtect (&vftable [_Index], __PTR_SIZE, dwProtect, &dwProtect);    \
                                                                              \
    /*dll_log.Log (L" New VFTable entry for %s: %08Xh  (Memory Policy: %s)\n",*/\
                  /*L##_Name, vftable [_Index],                               */\
                  /*SK_DescribeVirtualProtectFlags (dwProtect));              */\
  }                                                                           \
}

#define DXGI_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) { \
  void** vftable = *(void***)*(_Base);                                        \
                                                                              \
  if ((_Original) == nullptr) {                                               \
    SK_CreateVFTableHook ( L##_Name,                                          \
                             vftable,                                         \
                               (_Index),                                      \
                                 (_Override),                                 \
                                   (LPVOID *)&(_Original));                   \
  }                                                                           \
}
#endif

#define DARK_SOULS
#ifdef DARK_SOULS
  extern "C++" int* __DS3_WIDTH;
  extern "C++" int* __DS3_HEIGHT;
#endif

  HRESULT
    STDMETHODCALLTYPE Present1Callback (IDXGISwapChain1         *This,
                                        UINT                     SyncInterval,
                                        UINT                     PresentFlags,
                                  const DXGI_PRESENT_PARAMETERS *pPresentParameters)
  {
    g_pDXGISwap = This;

    // Start / End / Readback Pipeline Stats
    SK_D3D11_UpdateRenderStats (This);

    SK_BeginBufferSwap ();

    HRESULT hr = E_FAIL;

    CComPtr <ID3D11Device> pDev;

    int interval = config.render.framerate.present_interval;
    int flags    = PresentFlags;

    // Application preference
    if (interval == -1)
      interval = SyncInterval;

    if (bFlipMode) {
      flags = PresentFlags | DXGI_PRESENT_RESTART;

      if (bWait)
        flags |= DXGI_PRESENT_DO_NOT_WAIT;
    }

    static bool first_frame = true;

    if (first_frame) {
      if (sk::NVAPI::app_name == L"Fallout4.exe") {
        SK_FO4_PresentFirstFrame (This, SyncInterval, flags);
      }

      else if (sk::NVAPI::app_name == L"DarkSoulsIII.exe") {
        SK_DS3_PresentFirstFrame (This, SyncInterval, flags);
      }

      else if (sk::NVAPI::app_name == L"witcher3.exe") {
        SK_TW3_PresentFirstFrame (This, SyncInterval, flags);
      }

      // TODO: Clean this code up
      if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))) )
      {
        CComPtr <IDXGIDevice>  pDevDXGI;
        CComPtr <IDXGIAdapter> pAdapter;
        CComPtr <IDXGIFactory> pFactory;

        if ( SUCCEEDED (pDev->QueryInterface (IID_PPV_ARGS (&pDevDXGI))) &&
             SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter))  &&
             SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory))) ) {
          DXGI_SWAP_CHAIN_DESC desc;
          This->GetDesc (&desc);

          if (bAlwaysAllowFullscreen)
            pFactory->MakeWindowAssociation (desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES);

#if 0
          hWndRender       = desc.OutputWindow;
          game_window.hWnd = hWndRender;
#endif
        }
      }
    }

    hr = Present1_Original (This, interval, flags, pPresentParameters);

    first_frame = false;

    if (SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))))
    {
      HRESULT ret = E_FAIL;

      if (pDev != nullptr)
        ret = SK_EndBufferSwap (hr, pDev);

      SK_D3D11_TexCacheCheckpoint ();

      return ret;
    }

    // Not a D3D11 device -- weird...
    HRESULT ret = SK_EndBufferSwap (hr);

    return ret;
  }

  HRESULT
    STDMETHODCALLTYPE PresentCallback (IDXGISwapChain *This,
                                       UINT            SyncInterval,
                                       UINT            Flags)
  {
    g_pDXGISwap = This;

#ifdef DARK_SOULS
    if (__DS3_HEIGHT != nullptr) {
      DXGI_SWAP_CHAIN_DESC swap_desc;
      if (SUCCEEDED (This->GetDesc (&swap_desc))) {
        *__DS3_WIDTH  = swap_desc.BufferDesc.Width;
        *__DS3_HEIGHT = swap_desc.BufferDesc.Height;
      }
    }
#endif

    // Start / End / Readback Pipeline Stats
    SK_D3D11_UpdateRenderStats (This);

    SK_BeginBufferSwap ();

    HRESULT hr = E_FAIL;

    CComPtr <ID3D11Device> pDev;

    int interval = config.render.framerate.present_interval;
    int flags    = Flags;

    // Application preference
    if (interval == -1)
      interval = SyncInterval;

    if (bFlipMode) {
      flags = Flags | DXGI_PRESENT_RESTART;

      if (bWait)
        flags |= DXGI_PRESENT_DO_NOT_WAIT;
    }

    if (! bFlipMode) {
      // Test first, then do
      //if (S_OK == ((HRESULT (*)(IDXGISwapChain *, UINT, UINT))Present_Original)
                    //(This, interval, flags | DXGI_PRESENT_TEST)) {
        hr =
          ((HRESULT (*)(IDXGISwapChain *, UINT, UINT))Present_Original)
                       (This, interval, flags);
      //}

      if (hr != S_OK) {
          dll_log.Log ( L"[   DXGI   ] *** IDXGISwapChain::Present (...) "
                        L"returned non-S_OK (%s :: %s)",
                          SK_DescribeHRESULT (hr),
                            SUCCEEDED (hr) ? L"Success" :
                                             L"Fail" );
      }
    } else {
      // No overlays will work if we don't do this...
      /////if (config.osd.show) {
        hr =
          ((HRESULT (*)(IDXGISwapChain *, UINT, UINT))Present_Original)
          (This, 0, flags | DXGI_PRESENT_DO_NOT_SEQUENCE | DXGI_PRESENT_DO_NOT_WAIT);
      /////}

      DXGI_PRESENT_PARAMETERS pparams;
      pparams.DirtyRectsCount = 0;
      pparams.pDirtyRects     = nullptr;
      pparams.pScrollOffset   = nullptr;
      pparams.pScrollRect     = nullptr;

      CComPtr <IDXGISwapChain1> pSwapChain1;
      if (SUCCEEDED (This->QueryInterface (IID_PPV_ARGS (&pSwapChain1))))
      {
        hr = Present1_Original (pSwapChain1, interval, flags, &pparams);
      }
    }

    if (bWait) {
      CComPtr <IDXGISwapChain2> pSwapChain2;

      if (SUCCEEDED (This->QueryInterface (IID_PPV_ARGS (&pSwapChain2))))
      {
        if (pSwapChain2 != nullptr) {
          HANDLE hWait = pSwapChain2->GetFrameLatencyWaitableObject ();

          pSwapChain2.Release ();

          WaitForSingleObjectEx ( hWait,
                                    config.render.framerate.swapchain_wait,
                                      TRUE );
        }
      }
    }

    static bool first_frame = true;

    if (first_frame) {
      if (sk::NVAPI::app_name == L"Fallout4.exe") {
        SK_FO4_PresentFirstFrame (This, SyncInterval, Flags);
      }

      else if (sk::NVAPI::app_name == L"DarkSoulsIII.exe") {
        SK_DS3_PresentFirstFrame (This, SyncInterval, Flags);
      }

      else if (sk::NVAPI::app_name == L"witcher3.exe") {
        SK_TW3_PresentFirstFrame (This, SyncInterval, Flags);
      }


      // TODO: Clean this code up
      if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))) )
      {
        CComPtr <IDXGIDevice>  pDevDXGI;
        CComPtr <IDXGIAdapter> pAdapter;
        CComPtr <IDXGIFactory> pFactory;

        if ( SUCCEEDED (pDev->QueryInterface (IID_PPV_ARGS (&pDevDXGI))) &&
             SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter))  &&
             SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory))) ) {
          DXGI_SWAP_CHAIN_DESC desc;
          This->GetDesc (&desc);

          if (bAlwaysAllowFullscreen)
            pFactory->MakeWindowAssociation (desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES);

          hWndRender       = desc.OutputWindow;
          game_window.hWnd = hWndRender;

          SK_DXGI_BringRenderWindowToTop ();
        }
      }
    }

    first_frame = false;

    if (SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))))
    {
      HRESULT ret = E_FAIL;

      if (pDev != nullptr)
        ret = SK_EndBufferSwap (hr, pDev);

      SK_D3D11_TexCacheCheckpoint ();

      return ret;
    }

    // Not a D3D11 device -- weird...
    HRESULT ret = SK_EndBufferSwap (hr);

    return ret;
  }


  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGIOutput_GetDisplayModeList_Override ( IDXGIOutput    *This,
                                /* [in] */ DXGI_FORMAT     EnumFormat,
                                /* [in] */ UINT            Flags,
                                /* [annotation][out][in] */
                                  _Inout_  UINT           *pNumModes,
                                /* [annotation][out] */
_Out_writes_to_opt_(*pNumModes,*pNumModes)
                                           DXGI_MODE_DESC *pDesc)
  {
//    dll_log.Log (L"[   DXGI   ] [!] IDXGIOutput::GetDisplayModeList (...)");

    HRESULT hr =
      GetDisplayModeList_Original (This, EnumFormat, DXGI_ENUM_MODES_SCALING, pNumModes, pDesc);

//    dll_log.Log (L" >> %lu modes", *pNumModes);
    return hr;
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGIOutput_FindClosestMatchingMode_Override ( IDXGIOutput    *This,
                                                /* [annotation][in]  */
                                    _In_  const DXGI_MODE_DESC *pModeToMatch,
                                                /* [annotation][out] */
                                         _Out_  DXGI_MODE_DESC *pClosestMatch,
                                                /* [annotation][in]  */
                                      _In_opt_  IUnknown       *pConcernedDevice )
  {
//    dll_log.Log (L"[   DXGI   ] [!] IDXGIOutput::FindClosestMatchingMode (...)");
    return FindClosestMatchingMode_Original (This, pModeToMatch, pClosestMatch, pConcernedDevice );
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGIOutput_WaitForVBlank_Override ( IDXGIOutput    *This )
  {
//    dll_log.Log (L"[   DXGI   ] [!] IDXGIOutput::WaitForVBlank (...)");
    return WaitForVBlank_Original (This);
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_GetFullscreenState_Override ( IDXGISwapChain  *This,
                              _Out_opt_  BOOL            *pFullscreen,
                              _Out_opt_  IDXGIOutput    **ppTarget )
  {
    return GetFullscreenState_Original (This, pFullscreen, ppTarget);// nullptr, nullptr);
  }

  COM_DECLSPEC_NOTHROW
__declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_SetFullscreenState_Override ( IDXGISwapChain *This,
                                         BOOL            Fullscreen,
                                         IDXGIOutput    *pTarget )
  {
    DXGI_LOG_CALL_I2 (L"IDXGISwapChain", L"SetFullscreenState", L"%lu, %08Xh",
                      Fullscreen, pTarget);

#if 0
    DXGI_MODE_DESC mode_desc = { 0 };

    if (Fullscreen) {
      DXGI_SWAP_CHAIN_DESC desc;
      if (SUCCEEDED (This->GetDesc (&desc))) {
        mode_desc.Format                  = desc.BufferDesc.Format;
        mode_desc.Width                   = desc.BufferDesc.Width;
        mode_desc.Height                  = desc.BufferDesc.Height;

        mode_desc.RefreshRate.Denominator = desc.BufferDesc.RefreshRate.Denominator;
        mode_desc.RefreshRate.Numerator   = desc.BufferDesc.RefreshRate.Numerator;

        mode_desc.Scaling                 = desc.BufferDesc.Scaling;
        mode_desc.ScanlineOrdering        = desc.BufferDesc.ScanlineOrdering;
        ResizeTarget_Original           (This, &mode_desc);
      }
    }
#endif

    //UINT BufferCount = max (1, min (6, config.render.framerate.buffer_count));


    HRESULT ret;
    DXGI_CALL (ret, SetFullscreenState_Original (This, Fullscreen, pTarget));

    //
    // Necessary provisions for Fullscreen Flip Mode
    //
    if (Fullscreen && SUCCEEDED (ret)) {
      //mode_desc.RefreshRate = { 0 };
      //ResizeTarget_Original (This, &mode_desc);

      if (bFlipMode)
        ResizeBuffers_Original (This, 0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    }

    return ret;
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_ResizeBuffers_Override ( IDXGISwapChain *This,
                               _In_ UINT            BufferCount,
                               _In_ UINT            Width,
                               _In_ UINT            Height,
                               _In_ DXGI_FORMAT     NewFormat,
                               _In_ UINT            SwapChainFlags )
  {
    DXGI_LOG_CALL_I5 (L"IDXGISwapChain", L"ResizeBuffers", L"%lu,%lu,%lu,...,0x%08X,0x%08X",
                        BufferCount, Width, Height, NewFormat, SwapChainFlags);

    if ( config.render.framerate.buffer_count != -1           &&
         config.render.framerate.buffer_count !=  BufferCount &&
         BufferCount                          !=  0 ) {
      BufferCount = config.render.framerate.buffer_count;
      dll_log.Log (L"[   DXGI   ] >> Buffer Count Override: %lu buffers", BufferCount);
    }

    // Fake it
    if (bWait)
      return S_OK;

    HRESULT ret;
    DXGI_CALL (ret, ResizeBuffers_Original (This, BufferCount, Width, Height, NewFormat, SwapChainFlags));

    return ret;
  }

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_ResizeTarget_Override ( IDXGISwapChain *This,
                        _In_ const DXGI_MODE_DESC *pNewTargetParameters )
  {
    DXGI_LOG_CALL_I5 (L"IDXGISwapChain", L"ResizeTarget", L"{ (%lux%lu@%3.1fHz), fmt=%lu, scaling=0x%02x }",
                        pNewTargetParameters->Width, pNewTargetParameters->Height,
                        pNewTargetParameters->RefreshRate.Denominator != 0 ?
                          (float)pNewTargetParameters->RefreshRate.Numerator /
                          (float)pNewTargetParameters->RefreshRate.Denominator :
                            0.0f,
                        pNewTargetParameters->Format,
                        pNewTargetParameters->Scaling );

    HRESULT ret;
    DXGI_CALL (ret, ResizeTarget_Original (This, pNewTargetParameters));

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE
  DXGIFactory_CreateSwapChain_Override ( IDXGIFactory          *This,
                                   _In_  IUnknown              *pDevice,
                                   _In_  DXGI_SWAP_CHAIN_DESC  *pDesc,
                                  _Out_  IDXGISwapChain       **ppSwapChain )
  {
    std::wstring iname = SK_GetDXGIFactoryInterface (This);

    DXGI_LOG_CALL_I3 (iname.c_str (), L"CreateSwapChain", L"%08Xh, %08Xh, %08Xh",
                      pDevice, pDesc, ppSwapChain);

    HRESULT ret;

    if (pDesc != nullptr) {
      dll_log.LogEx ( true,
        L"[   DXGI   ]  SwapChain: (%lux%lu @ %4.1f Hz - Scaling: %s) - {%s}"
        L" [%lu Buffers] :: Flags=0x%04X, SwapEffect: %s\n",
        pDesc->BufferDesc.Width,
        pDesc->BufferDesc.Height,
        pDesc->BufferDesc.RefreshRate.Denominator > 0 ?
        (float)pDesc->BufferDesc.RefreshRate.Numerator /
        (float)pDesc->BufferDesc.RefreshRate.Denominator :
        (float)pDesc->BufferDesc.RefreshRate.Numerator,
        pDesc->BufferDesc.Scaling == 0 ?
        L"Unspecified" :
        pDesc->BufferDesc.Scaling == 1 ?
        L"Centered" : L"Stretched",
        pDesc->Windowed ? L"Windowed" : L"Fullscreen",
        pDesc->BufferCount,
        pDesc->Flags,
        pDesc->SwapEffect == 0 ?
        L"Discard" :
        pDesc->SwapEffect == 1 ?
        L"Sequential" :
        pDesc->SwapEffect == 2 ?
        L"<Unknown>" :
        pDesc->SwapEffect == 3 ?
        L"Flip Sequential" :
        pDesc->SwapEffect == 4 ?
        L"Flip Discard" : L"<Unknown>");

      // Set things up to make the swap chain Alt+Enter friendly
      if (bAlwaysAllowFullscreen) {
        pDesc->Flags                             |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        pDesc->Windowed                           = true;
        pDesc->BufferDesc.RefreshRate.Denominator = 0;
        pDesc->BufferDesc.RefreshRate.Numerator   = 0;
      }

      if (! bFlipMode)
        bFlipMode =
          ( dxgi_caps.present.flip_sequential && (
            ( host_app == L"Fallout4.exe") ||
              SK_DS3_UseFlipMode ()        ||
              SK_TW3_UseFlipMode () ) );

      if (host_app == L"Fallout4.exe") {
        if (bFlipMode) {
          bFlipMode = (! SK_FO4_IsFullscreen ()) && SK_FO4_UseFlipMode ();
        }

        bFlipMode = bFlipMode && pDesc->BufferDesc.Scaling == 0;
      } else {
        if (config.render.framerate.flip_discard)
         bFlipMode = dxgi_caps.present.flip_sequential;
      }

      bWait = bFlipMode && dxgi_caps.present.waitable;

      // We cannot change the swapchain parameters if this is used...
      bWait = bWait && config.render.framerate.swapchain_wait > 0;

      if (host_app == L"DarkSoulsIII.exe") {
        if (SK_DS3_IsBorderless ())
          pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      if (bFlipMode) {
        if (bWait)
          pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        // Flip Presentation Model requires 3 Buffers
        config.render.framerate.buffer_count =
          max (3, config.render.framerate.buffer_count);

        if (config.render.framerate.flip_discard &&
            dxgi_caps.present.flip_discard)
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        else
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      }

      else {
        // Resort to triple-buffering if flip mode is not available
        if (config.render.framerate.buffer_count > 2)
          config.render.framerate.buffer_count = 2;

        pDesc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
      }

      if (config.render.framerate.buffer_count != -1)
        pDesc->BufferCount = config.render.framerate.buffer_count;

      // We cannot switch modes on a waitable swapchain
      if (bFlipMode && bWait) {
        pDesc->Flags |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      dll_log.Log ( L"[ DXGI 1.2 ] >> Using %s Presentation Model  [Waitable: %s]",
                     bFlipMode ? L"Flip" : L"Traditional",
                       bWait ? L"Yes" : L"No" );
    }

    DXGI_CALL(ret, CreateSwapChain_Original (This, pDevice, pDesc, ppSwapChain));

    if ( SUCCEEDED (ret)      &&
         ppSwapChain  != NULL &&
       (*ppSwapChain) != NULL )
    {
      //if (bFlipMode || bWait)
        //DXGISwap_ResizeBuffers_Override (*ppSwapChain, config.render.framerate.buffer_count, pDesc->BufferDesc.Width, pDesc->BufferDesc.Height, pDesc->BufferDesc.Format, pDesc->Flags);

      const uint32_t max_latency = config.render.framerate.pre_render_limit;

      CComPtr <IDXGISwapChain2> pSwapChain2;
      if ( bFlipMode && bWait &&
           SUCCEEDED ( (*ppSwapChain)->QueryInterface (IID_PPV_ARGS (&pSwapChain2)) )
          )
      {
        if (max_latency < 16) {
          dll_log.Log (L"[   DXGI   ] Setting Swapchain Frame Latency: %lu", max_latency);
          pSwapChain2->SetMaximumFrameLatency (max_latency);
        }

        HANDLE hWait = pSwapChain2->GetFrameLatencyWaitableObject ();

        pSwapChain2.Release ();

        WaitForSingleObjectEx ( hWait,
                                  config.render.framerate.swapchain_wait,
                                    TRUE );
      }
      else
      {
        if (max_latency != -1) {
          CComPtr <IDXGIDevice1> pDevice1;

          if (SUCCEEDED ( (*ppSwapChain)->GetDevice (
                             IID_PPV_ARGS (&pDevice1)
                          )
                        )
             )
          {
            dll_log.Log (L"[   DXGI   ] Setting Device Frame Latency: %lu", max_latency);
            pDevice1->SetMaximumFrameLatency (max_latency);
          }
        }
      }

      CComPtr <ID3D11Device> pDev;

      if (SUCCEEDED ( pDevice->QueryInterface ( IID_PPV_ARGS (&pDev) )
                    )
         )
      {
        // Dangerous to hold a reference to this don't you think?!
        g_pD3D11Dev = pDev;
      }
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE
  DXGIFactory_CreateSwapChainForHwnd_Override ( IDXGIFactory2            *This,
                              _In_       IUnknown                        *pDevice,
                              _In_       HWND                             hWnd,
                              _In_       DXGI_SWAP_CHAIN_DESC1           *pDesc,
                          _In_opt_       DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
                          _In_opt_       IDXGIOutput                     *pRestrictToOutput,
                             _Out_       IDXGISwapChain1                 **ppSwapChain )
  {
    std::wstring iname = SK_GetDXGIFactoryInterface (This);

    // Wrong prototype, but who cares right now? :P
    DXGI_LOG_CALL_I3 (iname.c_str (), L"CreateSwapChainForHwnd", L"%08Xh, %08Xh, %08Xh",
                      pDevice, pDesc, ppSwapChain);

    HRESULT ret;

    if (pDesc != nullptr) {
      dll_log.LogEx ( true,
        L"[   DXGI   ]  SwapChain: (%lux%lu @ %4.1f Hz - Scaling: %s) - {%s}"
        L" [%lu Buffers] :: Flags=0x%04X, SwapEffect: %s\n",
        pDesc->Width,
        pDesc->Height,
        pFullscreenDesc != nullptr ?
          pFullscreenDesc->RefreshRate.Denominator > 0 ?
            (float)pFullscreenDesc->RefreshRate.Numerator /
            (float)pFullscreenDesc->RefreshRate.Denominator :
            (float)pFullscreenDesc->RefreshRate.Numerator   :
              0.0f,
        pDesc->Scaling == 0 ?
        L"Unspecified" :
        pDesc->Scaling == 1 ?
        L"Centered" : L"Stretched",
        pFullscreenDesc != nullptr ?
          pFullscreenDesc->Windowed ? L"Windowed" : L"Fullscreen" :
                                      L"Windowed",
        pDesc->BufferCount,
        pDesc->Flags,
        pDesc->SwapEffect == 0 ?
        L"Discard" :
        pDesc->SwapEffect == 1 ?
        L"Sequential" :
        pDesc->SwapEffect == 2 ?
        L"<Unknown>" :
        pDesc->SwapEffect == 3 ?
        L"Flip Sequential" :
        pDesc->SwapEffect == 4 ?
        L"Flip Discard" : L"<Unknown>");

#if 0
      // Set things up to make the swap chain Alt+Enter friendly
      if (bAlwaysAllowFullscreen) {
        pDesc->Flags                             |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        pDesc->Windowed                           = true;
        pDesc->BufferDesc.RefreshRate.Denominator = 0;
        pDesc->BufferDesc.RefreshRate.Numerator   = 0;
      }
#endif

      if (! bFlipMode)
        bFlipMode =
          ( dxgi_caps.present.flip_sequential && (
            ( host_app == L"Fallout4.exe") ||
              SK_DS3_UseFlipMode ()        ||
              SK_TW3_UseFlipMode () ) );

      if (host_app == L"Fallout4.exe") {
        if (bFlipMode) {
          bFlipMode = (! SK_FO4_IsFullscreen ()) && SK_FO4_UseFlipMode ();
        }

        bFlipMode = bFlipMode && pDesc->Scaling == 0;
      } else {
        if (config.render.framerate.flip_discard)
         bFlipMode = dxgi_caps.present.flip_sequential;
      }

      bWait = bFlipMode && dxgi_caps.present.waitable;

      // We cannot change the swapchain parameters if this is used...
      bWait = bWait && config.render.framerate.swapchain_wait > 0;

      if (host_app == L"DarkSoulsIII.exe") {
        if (SK_DS3_IsBorderless ())
          pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      if (bFlipMode) {
        if (bWait)
          pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        // Flip Presentation Model requires 3 Buffers
        config.render.framerate.buffer_count =
          max (3, config.render.framerate.buffer_count);

        if (config.render.framerate.flip_discard &&
            dxgi_caps.present.flip_discard)
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        else
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      }

      else {
        // Resort to triple-buffering if flip mode is not available
        if (config.render.framerate.buffer_count > 2)
          config.render.framerate.buffer_count = 2;

        pDesc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
      }

      if (config.render.framerate.buffer_count != -1)
        pDesc->BufferCount = config.render.framerate.buffer_count;

      // We cannot switch modes on a waitable swapchain
      if (bFlipMode && bWait) {
        pDesc->Flags |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      dll_log.Log ( L"[ DXGI 1.2 ] >> Using %s Presentation Model  [Waitable: %s]",
                     bFlipMode ? L"Flip" : L"Traditional",
                       bWait ? L"Yes" : L"No" );
    }

    DXGI_CALL(ret, CreateSwapChainForHwnd_Original (This, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain));

    if ( SUCCEEDED (ret)      &&
         ppSwapChain  != NULL &&
       (*ppSwapChain) != NULL )
    {
      //if (bFlipMode || bWait)
        //DXGISwap_ResizeBuffers_Override (*ppSwapChain, config.render.framerate.buffer_count, pDesc->BufferDesc.Width, pDesc->BufferDesc.Height, pDesc->BufferDesc.Format, pDesc->Flags);

      const uint32_t max_latency = config.render.framerate.pre_render_limit;

      CComPtr <IDXGISwapChain2> pSwapChain2;
      if ( bFlipMode && bWait &&
           SUCCEEDED ( (*ppSwapChain)->QueryInterface (IID_PPV_ARGS (&pSwapChain2)) )
          )
      {
        if (max_latency < 16) {
          dll_log.Log (L"[   DXGI   ] Setting Swapchain Frame Latency: %lu", max_latency);
          pSwapChain2->SetMaximumFrameLatency (max_latency);
        }

        HANDLE hWait = pSwapChain2->GetFrameLatencyWaitableObject ();

        pSwapChain2.Release ();

        WaitForSingleObjectEx ( hWait,
                                  config.render.framerate.swapchain_wait,
                                    TRUE );
      }
      else
      {
        if (max_latency != -1) {
          CComPtr <IDXGIDevice1> pDevice1;

          if (SUCCEEDED ( (*ppSwapChain)->GetDevice (
                             IID_PPV_ARGS (&pDevice1)
                          )
                        )
             )
          {
            dll_log.Log (L"[   DXGI   ] Setting Device Frame Latency: %lu", max_latency);
            pDevice1->SetMaximumFrameLatency (max_latency);
          }
        }
      }

      CComPtr <ID3D11Device> pDev;

      if (SUCCEEDED ( pDevice->QueryInterface ( IID_PPV_ARGS (&pDev) )
                    )
         )
      {
        // Dangerous to hold a reference to this don't you think?!
        g_pD3D11Dev = pDev;
      }
    }

    return ret;
  }

  typedef HRESULT (WINAPI *D3D11Dev_CreateTexture2D_pfn)(
    _In_            ID3D11Device           *This,
    _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
    _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
    _Out_opt_       ID3D11Texture2D        **ppTexture2D
  );
  D3D11Dev_CreateTexture2D_pfn D3D11Dev_CreateTexture2D_Original = nullptr;

  typedef void (WINAPI *D3D11_RSSetScissorRects_pfn)(
    _In_           ID3D11DeviceContext *This,
    _In_           UINT                 NumRects,
    _In_opt_ const D3D11_RECT          *pRects
  );
  D3D11_RSSetScissorRects_pfn D3D11_RSSetScissorRects_Original = nullptr;


  typedef void (WINAPI *D3D11_RSSetViewports_pfn)(
  _In_           ID3D11DeviceContext* This,
  _In_           UINT                 NumViewports,
  _In_opt_ const D3D11_VIEWPORT     * pViewports
  );
  D3D11_RSSetViewports_pfn D3D11_RSSetViewports_Original = nullptr;

  typedef void (WINAPI *D3D11_VSSetConstantBuffers_pfn)(
  _In_     ID3D11DeviceContext* This,
  _In_     UINT                 StartSlot,
  _In_     UINT                 NumBuffers,
  _In_opt_ ID3D11Buffer *const *ppConstantBuffers
  );
  D3D11_VSSetConstantBuffers_pfn D3D11_VSSetConstantBuffers_Original = nullptr;

  typedef void (WINAPI *D3D11_UpdateSubresource_pfn)(
    _In_           ID3D11DeviceContext *This,
    _In_           ID3D11Resource      *pDstResource,
    _In_           UINT                 DstSubresource,
    _In_opt_ const D3D11_BOX           *pDstBox,
    _In_     const void                *pSrcData,
    _In_           UINT                 SrcRowPitch,
    _In_           UINT                 SrcDepthPitch
  );
  D3D11_UpdateSubresource_pfn D3D11_UpdateSubresource_Original = nullptr;


  typedef HRESULT (WINAPI *D3D11_Map_pfn)(
     _In_ ID3D11DeviceContext      *This,
     _In_ ID3D11Resource           *pResource,
     _In_ UINT                      Subresource,
     _In_ D3D11_MAP                 MapType,
     _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource
  );

  D3D11_Map_pfn D3D11_Map_Original = nullptr;

  typedef void (WINAPI *D3D11_CopyResource_pfn)(
    _In_ ID3D11DeviceContext *This,
    _In_ ID3D11Resource      *pDstResource,
    _In_ ID3D11Resource      *pSrcResource
  );

  D3D11_CopyResource_pfn D3D11_CopyResource_Original = nullptr;


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
  D3D11_UpdateSubresource1_pfn D3D11_UpdateSubresource1_Original = nullptr;

  void
  WINAPI
  D3D11_RSSetScissorRects_Override (
          ID3D11DeviceContext *This,
          UINT                 NumRects,
    const D3D11_RECT          *pRects )
  {
  }

  void
  WINAPI
  D3D11_VSSetConstantBuffers_Override (
    ID3D11DeviceContext*  This,
    UINT                  StartSlot,
    UINT                  NumBuffers,
    ID3D11Buffer *const  *ppConstantBuffers )
  {
    //dll_log.Log (L"[   DXGI   ] [!]D3D11_VSSetConstantBuffers (%lu, %lu, ...)", StartSlot, NumBuffers);
    D3D11_VSSetConstantBuffers_Original (This, StartSlot, NumBuffers, ppConstantBuffers );
  }

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

    //CComPtr <ID3D11Texture2D> pTex;
    //if (SUCCEEDED (pDstResource->QueryInterface (IID_PPV_ARGS (&pTex)))) {
      //if (SK_D3D11_TextureIsCached (pTex)) {
        //dll_log.Log (L"[DX11TexMgr] Cached texture was updated... removing from cache!");
        //SK_D3D11_RemoveTexFromCache (pTex);
      //}
    //}

    D3D11_UpdateSubresource_Original (This, pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
  }

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
    //CComPtr <ID3D11Texture2D> pTex;
    //if (SUCCEEDED (pResource->QueryInterface (IID_PPV_ARGS (&pTex)))) {
      //if (SK_D3D11_TextureIsCached (pTex)) {
        //dll_log.Log (L"[DX11TexMgr] Cached texture was updated... removing from cache!");
        //SK_D3D11_RemoveTexFromCache (pTex);
      //}
    //}

    return D3D11_Map_Original (This, pResource, Subresource, MapType, MapFlags, pMappedResource);
  }

  void
  WINAPI
  D3D11_CopyResource_Override (
         ID3D11DeviceContext *This,
    _In_ ID3D11Resource      *pDstResource,
    _In_ ID3D11Resource      *pSrcResource )
  {
    dll_log.Log (L"[DX11TexMgr] Cached texture was updated... removing from cache!");

    D3D11_CopyResource_Original (This, pDstResource, pSrcResource);
  }




  void
  WINAPI
  D3D11_RSSetViewports_Override (
    ID3D11DeviceContext*  This,
    UINT                  NumViewports,
    const D3D11_VIEWPORT* pViewports )
  {
    D3D11_RSSetViewports_Original (This, NumViewports, pViewports);
  }

    typedef enum skUndesirableVendors {
    Microsoft = 0x1414,
    Intel     = 0x8086
  } Vendors;

  __declspec (nothrow)
  HRESULT
  STDMETHODCALLTYPE CreateDXGIFactory (REFIID   riid,
                                 _Out_ void   **ppFactory);

  // Do this in a thread because it is not safe to do from
  //   the thread that created the window or drives the message
  //     pump...
  unsigned int
  __stdcall
  SK_DXGI_BringRenderWindowToTop_THREAD (LPVOID user)
  {
    if (hWndRender != 0) {
      SetActiveWindow     (hWndRender);
      SetForegroundWindow (hWndRender);
      BringWindowToTop    (hWndRender);
    }

    return 0;
  }

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
    WaitForInitD3D11 ();

    DXGI_LOG_CALL_0 (L"D3D11CreateDevice");

    dll_log.LogEx (true, L"[  D3D 11  ]  <~> Preferred Feature Level(s): <%u> - ", FeatureLevels);

    for (UINT i = 0; i < FeatureLevels; i++) {
      switch (pFeatureLevels [i])
      {
      case D3D_FEATURE_LEVEL_9_1:
        dll_log.LogEx (false, L" 9_1");
        break;
      case D3D_FEATURE_LEVEL_9_2:
        dll_log.LogEx (false, L" 9_2");
        break;
      case D3D_FEATURE_LEVEL_9_3:
        dll_log.LogEx (false, L" 9_3");
        break;
      case D3D_FEATURE_LEVEL_10_0:
        dll_log.LogEx (false, L" 10_0");
        break;
      case D3D_FEATURE_LEVEL_10_1:
        dll_log.LogEx (false, L" 10_1");
        break;
      case D3D_FEATURE_LEVEL_11_0:
        dll_log.LogEx (false, L" 11_0");
        break;
      case D3D_FEATURE_LEVEL_11_1:
        dll_log.LogEx (false, L" 11_1");
        break;
        //case D3D_FEATURE_LEVEL_12_0:
        //dll_log.LogEx (false, L" 12_0");
        //break;
        //case D3D_FEATURE_LEVEL_12_1:
        //dll_log.LogEx (false, L" 12_1");
        //break;
      }
    }

    dll_log.LogEx (false, L"\n");


    if (! wcsicmp (host_app.c_str (), L"Batman_win7.exe")) {
      dll_log.Log (L"[   DXGI   ] Feature Level Override for Win7 Compliance!");

      FeatureLevels = 1;
      D3D_FEATURE_LEVEL pOverride [] = { D3D_FEATURE_LEVEL_11_0 };

      pFeatureLevels = pOverride;
    }

    //
    // DXGI Adapter Override (for performance)
    //

    IDXGIAdapter* pGameAdapter     = pAdapter;
    IDXGIAdapter* pOverrideAdapter = nullptr;

    if (true) {//pAdapter == nullptr) {
      IDXGIFactory* pFactory = nullptr;

      if (SUCCEEDED (CreateDXGIFactory (__uuidof (IDXGIFactory), (void **)&pFactory))) {
        if (pFactory != nullptr) {
          if (pAdapter == nullptr)
            EnumAdapters_Original (pFactory, 0, &pGameAdapter);

          pAdapter   = pGameAdapter;
          DriverType = D3D_DRIVER_TYPE_UNKNOWN;

          if ( SK_DXGI_preferred_adapter != -1 &&
               SUCCEEDED (EnumAdapters_Original (pFactory, SK_DXGI_preferred_adapter, &pOverrideAdapter)) )
          {
            DXGI_ADAPTER_DESC game_desc;
            DXGI_ADAPTER_DESC override_desc;

            pGameAdapter->GetDesc     (&game_desc);
            pOverrideAdapter->GetDesc (&override_desc);

            if ( game_desc.VendorId     == Vendors::Intel     &&
                 override_desc.VendorId != Vendors::Microsoft &&
                 override_desc.VendorId != Vendors::Intel )
            {
              dll_log.Log ( L"[   DXGI   ] !!! DXGI Adapter Override: (Using '%s' instead of '%s') !!!",
                            override_desc.Description, game_desc.Description );

              pAdapter = pOverrideAdapter;
              pGameAdapter->Release ();
            } else {
              SK_DXGI_preferred_adapter = -1;
              pOverrideAdapter->Release ();
            }
          }

          pFactory->Release ();
        }
      }
    }

    if (pAdapter != nullptr) {
      int iver = SK_GetDXGIAdapterInterfaceVer (pAdapter);

      // IDXGIAdapter3 = DXGI 1.4 (Windows 10+)
      if (iver >= 3) {
        SK_StartDXGI_1_4_BudgetThread (&pAdapter);
      }
    }

    HRESULT res;

    DXGI_CALL(res, 
      D3D11CreateDevice_Import (pAdapter,
                                DriverType,
                                Software,
                                Flags,
                                pFeatureLevels,
                                FeatureLevels,
                                SDKVersion,
                                ppDevice,
                                pFeatureLevel,
                                ppImmediateContext));

    if (SUCCEEDED (res)) {
      dwRenderThread = GetCurrentThreadId ();

      if (ppDevice != nullptr)
        dll_log.Log (L"[  D3D 11  ] >> Device = 0x%08Xh", *ppDevice);
    }

    return res;
  }

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

    DXGI_LOG_CALL_0 (L"D3D11CreateDeviceAndSwapChain");

    dll_log.LogEx (true, L"[  D3D 11  ]  <~> Preferred Feature Level(s): <%u> - ", FeatureLevels);

    for (UINT i = 0; i < FeatureLevels; i++) {
      switch (pFeatureLevels [i])
      {
      case D3D_FEATURE_LEVEL_9_1:
        dll_log.LogEx (false, L" 9_1");
        break;
      case D3D_FEATURE_LEVEL_9_2:
        dll_log.LogEx (false, L" 9_2");
        break;
      case D3D_FEATURE_LEVEL_9_3:
        dll_log.LogEx (false, L" 9_3");
        break;
      case D3D_FEATURE_LEVEL_10_0:
        dll_log.LogEx (false, L" 10_0");
        break;
      case D3D_FEATURE_LEVEL_10_1:
        dll_log.LogEx (false, L" 10_1");
        break;
      case D3D_FEATURE_LEVEL_11_0:
        dll_log.LogEx (false, L" 11_0");
        break;
      case D3D_FEATURE_LEVEL_11_1:
        dll_log.LogEx (false, L" 11_1");
        break;
        //case D3D_FEATURE_LEVEL_12_0:
        //dll_log.LogEx (false, L" 12_0");
        //break;
        //case D3D_FEATURE_LEVEL_12_1:
        //dll_log.LogEx (false, L" 12_1");
        //break;
      }
    }

    dll_log.LogEx (false, L"\n");


    if (! wcsicmp (host_app.c_str (), L"Batman_win7.exe")) {
      dll_log.Log (L"[   DXGI   ] Feature Level Override for Win7 Compliance!");

      FeatureLevels = 1;
      D3D_FEATURE_LEVEL pOverride [] = { D3D_FEATURE_LEVEL_11_0 };

      pFeatureLevels = pOverride;
    }

    //
    // DXGI Adapter Override (for performance)
    //

    IDXGIAdapter* pGameAdapter     = pAdapter;
    IDXGIAdapter* pOverrideAdapter = nullptr;

    if (true) {//pAdapter == nullptr) {
      IDXGIFactory* pFactory = nullptr;

      if (SUCCEEDED (CreateDXGIFactory (__uuidof (IDXGIFactory), (void **)&pFactory))) {
        if (pFactory != nullptr) {
          if (pAdapter == nullptr)
            EnumAdapters_Original (pFactory, 0, &pGameAdapter);

          pAdapter   = pGameAdapter;
          DriverType = D3D_DRIVER_TYPE_UNKNOWN;

          if ( SK_DXGI_preferred_adapter != -1 &&
               SUCCEEDED (EnumAdapters_Original (pFactory, SK_DXGI_preferred_adapter, &pOverrideAdapter)) )
          {
            DXGI_ADAPTER_DESC game_desc;
            DXGI_ADAPTER_DESC override_desc;

            pGameAdapter->GetDesc     (&game_desc);
            pOverrideAdapter->GetDesc (&override_desc);

            if ( game_desc.VendorId     == Vendors::Intel     &&
                 override_desc.VendorId != Vendors::Microsoft &&
                 override_desc.VendorId != Vendors::Intel )
            {
              dll_log.Log ( L"[   DXGI   ] !!! DXGI Adapter Override: (Using '%s' instead of '%s') !!!",
                            override_desc.Description, game_desc.Description );

              pAdapter = pOverrideAdapter;
              pGameAdapter->Release ();
            } else {
              SK_DXGI_preferred_adapter = -1;
              pOverrideAdapter->Release ();
            }
          }

          pFactory->Release ();
        }
      }
    }

    if (pAdapter != nullptr) {
      int iver = SK_GetDXGIAdapterInterfaceVer (pAdapter);

      // IDXGIAdapter3 = DXGI 1.4 (Windows 10+)
      if (iver >= 3) {
        SK_StartDXGI_1_4_BudgetThread (&pAdapter);
      }
    }

    if (pSwapChainDesc != nullptr) {
      dll_log.LogEx ( true,
                        L"[   DXGI   ]  SwapChain: (%lux%lu@%4.1f Hz - Scaling: %s) - "
                        L"[%lu Buffers] :: Flags=0x%04X, SwapEffect: %s\n",
                          pSwapChainDesc->BufferDesc.Width,
                          pSwapChainDesc->BufferDesc.Height,
                          pSwapChainDesc->BufferDesc.RefreshRate.Denominator > 0 ?
                   (float)pSwapChainDesc->BufferDesc.RefreshRate.Numerator /
                   (float)pSwapChainDesc->BufferDesc.RefreshRate.Denominator :
                   (float)pSwapChainDesc->BufferDesc.RefreshRate.Numerator,
                          pSwapChainDesc->BufferDesc.Scaling == 0 ?
                            L"Unspecified" :
                          pSwapChainDesc->BufferDesc.Scaling == 1 ?
                            L"Centered" : L"Stretched",
                          pSwapChainDesc->BufferCount,
                          pSwapChainDesc->Flags,
                          pSwapChainDesc->SwapEffect == 0 ?
                            L"Discard" :
                          pSwapChainDesc->SwapEffect == 1 ?
                            L"Sequential" :
                          pSwapChainDesc->SwapEffect == 2 ?
                            L"<Unknown>" :
                          pSwapChainDesc->SwapEffect == 3 ?
                            L"Flip Sequential" :
                          pSwapChainDesc->SwapEffect == 4 ?
                            L"Flip Discard" : L"<Unknown>");
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
                                            pSwapChainDesc,
                                            ppSwapChain,
                                            ppDevice,
                                            pFeatureLevel,
                                            ppImmediateContext));

    if (res == S_OK)
    {
      // Detect devices that were only created for the purpose of creating hooks....
      bool bFake = false;

      if (pSwapChainDesc != nullptr) {
        // Fake, created by (UNKNOWN)
        if ( pSwapChainDesc->BufferDesc.Width  == 256 &&
             pSwapChainDesc->BufferDesc.Height == 256 )
          bFake = true;

        if ((! bFake) && ( dwRenderThread == 0x00 ||
                           dwRenderThread == GetCurrentThreadId () )) {
          if ( hWndRender                   != 0 &&
               pSwapChainDesc->OutputWindow != 0 &&
               pSwapChainDesc->OutputWindow != hWndRender )
            dll_log.Log (L"[  D3D 11  ] Game created a new window?!");

          //hWndRender       = pSwapChainDesc->OutputWindow;
          //game_window.hWnd = hWndRender;
        }
      }

      // Assume the first thing to create a D3D11 render device is
      //   the game and that devices never migrate threads; for most games
      //     this assumption holds.
      if ((! bFake) && ( dwRenderThread == 0x00 ||
                         dwRenderThread == GetCurrentThreadId ()) ) {
        dwRenderThread = GetCurrentThreadId ();

        if (ppDevice != nullptr)
          dll_log.Log (L"[  D3D 11  ] >> Device = 0x%08Xh", *ppDevice);
      }
    }

    return res;
  }

  HRESULT
  STDMETHODCALLTYPE GetDesc2_Override (IDXGIAdapter2      *This,
                                _Out_  DXGI_ADAPTER_DESC2 *pDesc)
  {
    std::wstring iname = SK_GetDXGIAdapterInterface (This);

    DXGI_LOG_CALL_I2 (iname.c_str (), L"GetDesc2", L"%08Xh, %08Xh", This, pDesc);

    HRESULT ret;
    DXGI_CALL (ret, GetDesc2_Original (This, pDesc));

    //// OVERRIDE VRAM NUMBER
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0) {
      dll_log.LogEx ( true,
        L" <> GetDesc2_Override: Looking for matching NVAPI GPU for %s...: ",
        pDesc->Description );

      DXGI_ADAPTER_DESC* match =
        sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

      if (match != NULL) {
        dll_log.LogEx (false, L"Success! (%s)\n", match->Description);
        pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
      }
      else
        dll_log.LogEx (false, L"Failure! (No Match Found)\n");
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE GetDesc1_Override (IDXGIAdapter1      *This,
                                _Out_  DXGI_ADAPTER_DESC1 *pDesc)
  {
    std::wstring iname = SK_GetDXGIAdapterInterface (This);

    DXGI_LOG_CALL_I2 (iname.c_str (), L"GetDesc1", L"%08Xh, %08Xh", This, pDesc);

    HRESULT ret;
    DXGI_CALL (ret, GetDesc1_Original (This, pDesc));

    //// OVERRIDE VRAM NUMBER
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0) {
      dll_log.LogEx ( true,
        L" <> GetDesc1_Override: Looking for matching NVAPI GPU for %s...: ",
        pDesc->Description );

      DXGI_ADAPTER_DESC* match =
        sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

      if (match != NULL) {
        dll_log.LogEx (false, L"Success! (%s)\n", match->Description);
        pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
      }
      else
        dll_log.LogEx (false, L"Failure! (No Match Found)\n");
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE GetDesc_Override (IDXGIAdapter      *This,
                               _Out_  DXGI_ADAPTER_DESC *pDesc)
  {
    std::wstring iname = SK_GetDXGIAdapterInterface (This);

    DXGI_LOG_CALL_I2 (iname.c_str (), L"GetDesc",L"%08Xh, %08Xh", This, pDesc);

    HRESULT ret;
    DXGI_CALL (ret, GetDesc_Original (This, pDesc));

    //// OVERRIDE VRAM NUMBER
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0) {
      dll_log.LogEx ( true,
        L" <> GetDesc_Override: Looking for matching NVAPI GPU for %s...: ",
        pDesc->Description );

      DXGI_ADAPTER_DESC* match =
        sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

      if (match != NULL) {
        dll_log.LogEx (false, L"Success! (%s)\n", match->Description);
        pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
      }
      else
        dll_log.LogEx (false, L"Failure! (No Match Found)\n");
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE EnumAdapters_Common (IDXGIFactory       *This,
                                         UINT                Adapter,
                                _Inout_  IDXGIAdapter      **ppAdapter,
                                         EnumAdapters_pfn    pFunc)
  {
    DXGI_ADAPTER_DESC desc;

    bool silent = dll_log.silent;
    dll_log.silent = true;
    {
      // Don't log this call
      (*ppAdapter)->GetDesc (&desc);
    }
    dll_log.silent = false;

    int iver = SK_GetDXGIAdapterInterfaceVer (*ppAdapter);

    // Only do this for NVIDIA SLI GPUs on Windows 10 (DXGI 1.4)
    if (false) {//nvapi_init && sk::NVAPI::CountSLIGPUs () > 0 && iver >= 3) {
      if (! GetDesc_Original) {
        DXGI_VIRTUAL_HOOK (ppAdapter, 8, "(*ppAdapter)->GetDesc",
          GetDesc_Override, GetDesc_Original, GetDesc_pfn);
      }

      if (! GetDesc1_Original) {
        CComPtr <IDXGIAdapter1> pAdapter1;

       if (SUCCEEDED ((*ppAdapter)->QueryInterface (IID_PPV_ARGS (&pAdapter1)))) {
          DXGI_VIRTUAL_HOOK (&pAdapter1, 10, "(pAdapter1)->GetDesc1",
            GetDesc1_Override, GetDesc1_Original, GetDesc1_pfn);
       }
      }

      if (! GetDesc2_Original) {
        CComPtr <IDXGIAdapter2> pAdapter2;
        if (SUCCEEDED ((*ppAdapter)->QueryInterface (IID_PPV_ARGS (&pAdapter2)))) {

          DXGI_VIRTUAL_HOOK (ppAdapter, 11, "(*pAdapter2)->GetDesc2",
            GetDesc2_Override, GetDesc2_Original, GetDesc2_pfn);
        }
      }
    }

    // Logic to skip Intel and Microsoft adapters and return only AMD / NV
    //if (lstrlenW (pDesc->Description)) {
    if (true) {
      if (! lstrlenW (desc.Description))
        dll_log.LogEx (false, L" >> Assertion filed: Zero-length adapter name!\n");

#ifdef SKIP_INTEL
      if ((desc.VendorId == Microsoft || desc.VendorId == Intel) && Adapter == 0) {
#else
      if (false) {
#endif
        // We need to release the reference we were just handed before
        //   skipping it.
        (*ppAdapter)->Release ();

        dll_log.LogEx (false,
          L"[   DXGI   ] >> (Host Application Tried To Enum Intel or Microsoft Adapter "
          L"as Adapter 0) -- Skipping Adapter '%s' <<\n", desc.Description);

        return (pFunc (This, Adapter + 1, ppAdapter));
      }
      else {
        // Only do this for NVIDIA SLI GPUs on Windows 10 (DXGI 1.4)
        if (false) { //nvapi_init && sk::NVAPI::CountSLIGPUs () > 0 && iver >= 3) {
          DXGI_ADAPTER_DESC* match =
            sk::NVAPI::FindGPUByDXGIName (desc.Description);

          if (match != NULL &&
            desc.DedicatedVideoMemory > match->DedicatedVideoMemory) {
// This creates problems in 32-bit environments...
#ifdef _WIN64
            if (sk::NVAPI::app_name != L"Fallout4.exe") {
              dll_log.Log (
                L"   # SLI Detected (Corrected Memory Total: %llu MiB -- "
                L"Original: %llu MiB)",
                match->DedicatedVideoMemory >> 20ULL,
                desc.DedicatedVideoMemory   >> 20ULL);
            } else {
              match->DedicatedVideoMemory = desc.DedicatedVideoMemory;
            }
#endif
          }
        }
      }

      dll_log.LogEx(true,L"[   DXGI   ]  @ Returned Adapter %lu: '%s' (LUID: %08X:%08X)",
        Adapter,
          desc.Description,
            desc.AdapterLuid.HighPart,
              desc.AdapterLuid.LowPart );

      //
      // Windows 8 has a software implementation, which we can detect.
      //
      CComPtr <IDXGIAdapter1> pAdapter1;
      HRESULT hr =
        (*ppAdapter)->QueryInterface (IID_PPV_ARGS (&pAdapter1));

      if (SUCCEEDED (hr)) {
        bool silence = dll_log.silent;
        dll_log.silent = true; // Temporarily disable logging

        DXGI_ADAPTER_DESC1 desc1;

        if (SUCCEEDED (pAdapter1->GetDesc1 (&desc1))) {
          dll_log.silent = silence; // Restore logging
#define DXGI_ADAPTER_FLAG_REMOTE   0x1
#define DXGI_ADAPTER_FLAG_SOFTWARE 0x2
          if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            dll_log.LogEx (false, L" <Software>");
          else
            dll_log.LogEx (false, L" <Hardware>");
          if (desc1.Flags & DXGI_ADAPTER_FLAG_REMOTE)
            dll_log.LogEx (false, L" [Remote]");
        }

        dll_log.silent = silence; // Restore logging
      }
    }

    dll_log.LogEx (false, L"\n");

    return S_OK;
  }

  HRESULT
  STDMETHODCALLTYPE EnumAdapters1_Override (IDXGIFactory1  *This,
                                            UINT            Adapter,
                                     _Out_  IDXGIAdapter1 **ppAdapter)
  {
    std::wstring iname = SK_GetDXGIFactoryInterface    (This);

    DXGI_LOG_CALL_I3 (iname.c_str (), L"EnumAdapters1", L"%08Xh, %u, %08Xh",
      This, Adapter, ppAdapter);

    HRESULT ret;
    DXGI_CALL (ret, EnumAdapters1_Original (This,Adapter,ppAdapter));

#if 0
    // For games that try to enumerate all adapters until the API returns failure,
    //   only override valid adapters...
    if ( SUCCEEDED (ret) &&
         SK_DXGI_preferred_adapter != -1 &&
         SK_DXGI_preferred_adapter != Adapter )
    {
      IDXGIAdapter1* pAdapter1 = nullptr;

      if (SUCCEEDED (EnumAdapters1_Original (This, SK_DXGI_preferred_adapter, &pAdapter1))) {
        dll_log.Log ( L"[   DXGI   ] (Reported values reflect user override: DXGI Adapter %lu -> %lu)",
                        Adapter, SK_DXGI_preferred_adapter );
        Adapter = SK_DXGI_preferred_adapter;

        if (pAdapter1 != nullptr)
          pAdapter1->Release ();
      }

      ret = EnumAdapters1_Original (This, Adapter, ppAdapter);
    }
#endif

    if (SUCCEEDED (ret) && ppAdapter != nullptr && (*ppAdapter) != nullptr) {
      return EnumAdapters_Common (This, Adapter, (IDXGIAdapter **)ppAdapter,
                                  (EnumAdapters_pfn)EnumAdapters1_Override);
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE EnumAdapters_Override (IDXGIFactory  *This,
                                           UINT           Adapter,
                                    _Out_  IDXGIAdapter **ppAdapter)
  {
    std::wstring iname = SK_GetDXGIFactoryInterface    (This);

    DXGI_LOG_CALL_I3 (iname.c_str (), L"EnumAdapters", L"%08Xh, %u, %08Xh",
      This, Adapter, ppAdapter);

    HRESULT ret;
    DXGI_CALL (ret, EnumAdapters_Original (This, Adapter, ppAdapter));

#if 0
    // For games that try to enumerate all adapters until the API returns failure,
    //   only override valid adapters...
    if ( SUCCEEDED (ret) &&
         SK_DXGI_preferred_adapter != -1 &&
         SK_DXGI_preferred_adapter != Adapter )
    {
      IDXGIAdapter* pAdapter = nullptr;

      if (SUCCEEDED (EnumAdapters_Original (This, SK_DXGI_preferred_adapter, &pAdapter))) {
        dll_log.Log ( L"[   DXGI   ] (Reported values reflect user override: DXGI Adapter %lu -> %lu)",
                        Adapter, SK_DXGI_preferred_adapter );
        Adapter = SK_DXGI_preferred_adapter;

        if (pAdapter != nullptr)
          pAdapter->Release ();
      }

      ret = EnumAdapters_Original (This, Adapter, ppAdapter);
    }
#endif

    if (SUCCEEDED (ret) && ppAdapter != nullptr && (*ppAdapter) != nullptr) {
      return EnumAdapters_Common (This, Adapter, ppAdapter,
                                  (EnumAdapters_pfn)EnumAdapters_Override);
    }

    return ret;
  }

  __declspec (nothrow)
    HRESULT
    STDMETHODCALLTYPE CreateDXGIFactory (REFIID   riid,
                                   _Out_ void   **ppFactory)
  {
    WaitForInit ();

    std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
    int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

    DXGI_LOG_CALL_2 (L"CreateDXGIFactory", L"%s, %08Xh",
      iname.c_str (), ppFactory);

    HRESULT ret;
    DXGI_CALL (ret, CreateDXGIFactory_Import (riid, ppFactory));

    DXGI_VIRTUAL_HOOK ( ppFactory, 7, "IDXGIFactory::EnumAdapters",
                              EnumAdapters_Override,
                              EnumAdapters_Original,
                                EnumAdapters_pfn );

    DXGI_VIRTUAL_HOOK ( ppFactory, 10, "IDXGIFactory::CreateSwapChain",
                              DXGIFactory_CreateSwapChain_Override,
                                          CreateSwapChain_Original,
                                            CreateSwapChain_pfn );

    // DXGI 1.1+
    if (iver > 0) {
      DXGI_VIRTUAL_HOOK ( ppFactory, 12, "IDXGIFactory1::EnumAdapters1",
                                EnumAdapters1_Override,
                                EnumAdapters1_Original,
                                  EnumAdapters1_pfn );
    }

    // DXGI 1.2+
    if (iver > 1) {
      DXGI_VIRTUAL_HOOK ( ppFactory, 15, "IDXGIFactory::CreateSwapChainForHwnd",
                    DXGIFactory_CreateSwapChainForHwnd_Override,
                                CreateSwapChainForHwnd_Original,
                                CreateSwapChainForHwnd_pfn );
    }

    return ret;
  }

  __declspec (nothrow)
    HRESULT
    STDMETHODCALLTYPE CreateDXGIFactory1 (REFIID   riid,
                                    _Out_ void   **ppFactory)
  {
    WaitForInit ();

    std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
    int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

    DXGI_LOG_CALL_2 (L"CreateDXGIFactory1", L"%s, %08Xh",
      iname.c_str (), ppFactory);

    // Windows Vista does not have this function -- wrap it with CreateDXGIFactory
    if (CreateDXGIFactory1_Import == nullptr) {
      dll_log.Log (L"[   DXGI   ]  >> Falling back to CreateDXGIFactory on Vista...");
      return CreateDXGIFactory (riid, ppFactory);
    }

    HRESULT ret;
    DXGI_CALL (ret, CreateDXGIFactory1_Import (riid, ppFactory));

    DXGI_VIRTUAL_HOOK ( ppFactory, 7,  "IDXGIFactory1::EnumAdapters",
                              EnumAdapters_Override,  EnumAdapters_Original,
                              EnumAdapters_pfn );

    DXGI_VIRTUAL_HOOK ( ppFactory, 10, "IDXGIFactory1::CreateSwapChain",
                              DXGIFactory_CreateSwapChain_Override,
                                          CreateSwapChain_Original,
                                            CreateSwapChain_pfn );

    // DXGI 1.1+
    if (iver > 0) {
      DXGI_VIRTUAL_HOOK ( ppFactory, 12, "IDXGIFactory1::EnumAdapters1",
                                EnumAdapters1_Override, EnumAdapters1_Original,
                                EnumAdapters1_pfn );
    }

    // DXGI 1.2+
    if (iver > 1) {
      DXGI_VIRTUAL_HOOK ( ppFactory, 15, "IDXGIFactory1::CreateSwapChainForHwnd",
                    DXGIFactory_CreateSwapChainForHwnd_Override,
                                CreateSwapChainForHwnd_Original,
                                CreateSwapChainForHwnd_pfn );
    }

    return ret;
  }

  __declspec (nothrow)
    HRESULT
    STDMETHODCALLTYPE CreateDXGIFactory2 (UINT     Flags,
                                          REFIID   riid,
                                    _Out_ void   **ppFactory)
  {
    WaitForInit ();

    std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
    int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

    DXGI_LOG_CALL_3 (L"CreateDXGIFactory2", L"0x%04X, %s, %08Xh",
      Flags, iname.c_str (), ppFactory);

    // Windows 7 does not have this function -- wrap it with CreateDXGIFactory1
    if (CreateDXGIFactory2_Import == nullptr) {
      dll_log.Log (L"[   DXGI   ]  >> Falling back to CreateDXGIFactory1 on Vista/7...");
      return CreateDXGIFactory1 (riid, ppFactory);
    }

    HRESULT ret;
    DXGI_CALL (ret, CreateDXGIFactory2_Import (Flags, riid, ppFactory));

    DXGI_VIRTUAL_HOOK ( ppFactory, 7, "IDXGIFactory2::EnumAdapters",
                              EnumAdapters_Override, EnumAdapters_Original,
                              EnumAdapters_pfn );

    DXGI_VIRTUAL_HOOK ( ppFactory, 10, "IDXGIFactory2::CreateSwapChain",
                              DXGIFactory_CreateSwapChain_Override,
                                          CreateSwapChain_Original,
                                            CreateSwapChain_pfn );

    // DXGI 1.1+
    if (iver > 0) {
      DXGI_VIRTUAL_HOOK ( ppFactory, 12, "IDXGIFactory2::EnumAdapters1",
                                EnumAdapters1_Override, EnumAdapters1_Original,
                                EnumAdapters1_pfn );
    }

    // DXGI 1.2+
    if (iver > 1) {
      DXGI_VIRTUAL_HOOK ( ppFactory, 15, "IDXGIFactory2::CreateSwapChainForHwnd",
                    DXGIFactory_CreateSwapChainForHwnd_Override,
                                CreateSwapChainForHwnd_Original,
                                CreateSwapChainForHwnd_pfn );
    }

    return ret;
  }

  DXGI_STUB (HRESULT, DXGID3D10CreateDevice,
    (HMODULE hModule, IDXGIFactory *pFactory, IDXGIAdapter *pAdapter,
      UINT    Flags,   void         *unknown,  void         *ppDevice),
    (hModule, pFactory, pAdapter, Flags, unknown, ppDevice));

  struct UNKNOWN5 {
    DWORD unknown [5];
  };

  DXGI_STUB (HRESULT, DXGID3D10CreateLayeredDevice,
    (UNKNOWN5 Unknown),
    (Unknown))

  DXGI_STUB (SIZE_T, DXGID3D10GetLayeredDeviceSize,
    (const void *pLayers, UINT NumLayers),
    (pLayers, NumLayers))

  DXGI_STUB (HRESULT, DXGID3D10RegisterLayers,
    (const void *pLayers, UINT NumLayers),
    (pLayers, NumLayers))

  HRESULT
  STDMETHODCALLTYPE DXGIDumpJournal (void)
  {
    DXGI_LOG_CALL_0 (L"DXGIDumpJournal");

    return E_NOTIMPL;
  }

  HRESULT
  STDMETHODCALLTYPE DXGIReportAdapterConfiguration (void)
  {
    DXGI_LOG_CALL_0 (L"DXGIReportAdapterConfiguration");

    return E_NOTIMPL;
  }
}


LPVOID pfnD3D11CreateDevice             = nullptr;
LPVOID pfnD3D11CreateDeviceAndSwapChain = nullptr;
LPVOID pfnChangeDisplaySettingsA        = nullptr;

typedef LONG (WINAPI *ChangeDisplaySettingsA_pfn)(
  _In_ DEVMODEA *lpDevMode,
  _In_ DWORD    dwFlags
);
ChangeDisplaySettingsA_pfn ChangeDisplaySettingsA_Original = nullptr;

LONG
WINAPI
ChangeDisplaySettingsA_Detour (
  _In_ DEVMODEA *lpDevMode,
  _In_ DWORD     dwFlags )
{
  static bool called = false;

  DEVMODEW dev_mode;
  dev_mode.dmSize = sizeof (DEVMODEW);

  EnumDisplaySettings (nullptr, 0, &dev_mode);

  if (dwFlags != CDS_TEST) {
    if (called)
      ChangeDisplaySettingsA_Original (0, CDS_RESET);

    called = true;

    return ChangeDisplaySettingsA_Original (lpDevMode, CDS_FULLSCREEN);
  } else {
    return ChangeDisplaySettingsA_Original (lpDevMode, dwFlags);
  }
}

struct cache_params_t {
  uint32_t max_entries = 4096;
  uint32_t min_entries = 1024;
   int32_t max_size    = 2048L;
   int32_t min_size    = 512L;
  uint32_t min_evict   = 16;
  uint32_t max_evict   = 64;
} cache_opts;

CRITICAL_SECTION tex_cs;
CRITICAL_SECTION hash_cs;
CRITICAL_SECTION dump_cs;
CRITICAL_SECTION cache_cs;
CRITICAL_SECTION inject_cs;

void WINAPI SK_D3D11_SetResourceRoot      (std::wstring root);
void WINAPI SK_D3D11_EnableTexDump        (bool enable);
void WINAPI SK_D3D11_EnableTexInject      (bool enable);
void WINAPI SK_D3D11_EnableTexCache       (bool enable);
void WINAPI SK_D3D11_PopulateResourceList (void);

typedef void (WINAPI *finish_pfn)(void);

unsigned int WINAPI         HookD3D11           (LPVOID user);
void         WINAPI SK_DXGI_SetPreferredAdapter (int override_id);

void
WINAPI
dxgi_init_callback (finish_pfn finish)
{
  HMODULE hBackend = backend_dll;

  if (config.render.dxgi.adapter_override != -1)
    SK_DXGI_SetPreferredAdapter (config.render.dxgi.adapter_override);


  dll_log.Log (L"[   DXGI   ] Importing CreateDXGIFactory{1|2}");
  dll_log.Log (L"[   DXGI   ] ================================");

  dll_log.Log (L"[ DXGI 1.0 ]   CreateDXGIFactory:  %08Xh",
    (CreateDXGIFactory_Import =  \
      (CreateDXGIFactory_pfn)GetProcAddress (hBackend, "CreateDXGIFactory")));
  dll_log.Log (L"[ DXGI 1.1 ]   CreateDXGIFactory1: %08Xh",
    (CreateDXGIFactory1_Import = \
      (CreateDXGIFactory1_pfn)GetProcAddress (hBackend, "CreateDXGIFactory1")));
  dll_log.Log (L"[ DXGI 1.3 ]   CreateDXGIFactory2: %08Xh",
    (CreateDXGIFactory2_Import = \
      (CreateDXGIFactory2_pfn)GetProcAddress (hBackend, "CreateDXGIFactory2")));


  SK::DXGI::hModD3D11 = LoadLibrary (L"d3d11.dll");

  SK_CreateDLLHook ( L"d3d11.dll", "D3D11CreateDevice",
                       D3D11CreateDevice_Detour,
            (LPVOID *)&D3D11CreateDevice_Import,
                      &pfnD3D11CreateDevice );

  SK_CreateDLLHook ( L"d3d11.dll", "D3D11CreateDeviceAndSwapChain",
                       D3D11CreateDeviceAndSwapChain_Detour,
            (LPVOID *)&D3D11CreateDeviceAndSwapChain_Import,
                      &pfnD3D11CreateDeviceAndSwapChain );

  SK_EnableHook (pfnD3D11CreateDevice);
  SK_EnableHook (pfnD3D11CreateDeviceAndSwapChain);



  SK_CommandProcessor* pCommandProc =
    SK_GetCommandProcessor ();

  pCommandProc->AddVariable ( "PresentationInterval",
          new SK_IVarStub <int> (&config.render.framerate.present_interval));
  pCommandProc->AddVariable ( "PreRenderLimit",
          new SK_IVarStub <int> (&config.render.framerate.pre_render_limit));
  pCommandProc->AddVariable ( "BufferCount",
          new SK_IVarStub <int> (&config.render.framerate.buffer_count));
  pCommandProc->AddVariable ( "UseFlipDiscard",
          new SK_IVarStub <bool> (&config.render.framerate.flip_discard));

  cache_opts.max_entries = config.textures.cache.max_entries;
  cache_opts.min_entries = config.textures.cache.min_entries;
  cache_opts.max_evict   = config.textures.cache.max_evict;
  cache_opts.min_evict   = config.textures.cache.min_evict;
  cache_opts.max_size    = config.textures.cache.max_size;
  cache_opts.min_size    = config.textures.cache.min_size;

  //
  // Legacy Hack for Untitled Project X (FFX/FFX-2)
  //
  extern bool SK_D3D11_inject_textures_ffx;
  if (! SK_D3D11_inject_textures_ffx) {
    SK_D3D11_EnableTexCache  (config.textures.d3d11.cache);
    SK_D3D11_EnableTexDump   (config.textures.d3d11.dump);
    SK_D3D11_EnableTexInject (config.textures.d3d11.inject);
    SK_D3D11_SetResourceRoot (config.textures.d3d11.res_root);
  }

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

  SK_D3D11_PopulateResourceList ();

  if (hModD3DX11_43 == nullptr) {
    hModD3DX11_43 =
      LoadLibrary (L"d3dx11_43.dll");

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

  _beginthreadex ( nullptr, 
                     0,
                       HookD3D11,
                         nullptr,
                           0x00,
                             nullptr );

  finish ();
}

__declspec (thread) HMODULE local_dxgi = 0;

HMODULE
SK_LoadRealDXGI (void)
{
  wchar_t wszBackendDLL [MAX_PATH] = { L'\0' };

#ifdef _WIN64
  GetSystemDirectory (wszBackendDLL, MAX_PATH);
#else
  BOOL bWOW64;
  ::IsWow64Process (GetCurrentProcess (), &bWOW64);

  if (bWOW64)
    GetSystemWow64Directory (wszBackendDLL, MAX_PATH);
  else
    GetSystemDirectory (wszBackendDLL, MAX_PATH);
#endif

  lstrcatW (wszBackendDLL, L"\\dxgi.dll");

  return (local_dxgi = LoadLibraryW (wszBackendDLL));
}

void
SK_FreeRealDXGI (void)
{
  FreeLibrary (local_dxgi);
}

bool
SK::DXGI::Startup (void)
{
  InitializeCriticalSectionAndSpinCount (&tex_cs,    0x2000);
  InitializeCriticalSectionAndSpinCount (&hash_cs,   0x4000);
  InitializeCriticalSectionAndSpinCount (&dump_cs,   0x0200);
  InitializeCriticalSectionAndSpinCount (&cache_cs,  0x4000);
  InitializeCriticalSectionAndSpinCount (&inject_cs, 0x1000);

  return SK_StartupCore (L"dxgi", dxgi_init_callback);
}

extern "C" bool WINAPI SK_DS3_ShutdownPlugin (const wchar_t* backend);



unsigned int
__stdcall
HookD3D11 (LPVOID user)
{
  CComPtr <IDXGIFactory> pFactory = nullptr;
  CComPtr <IDXGIAdapter> pAdapter = nullptr;

  HRESULT hr =
    CreateDXGIFactory_Import ( IID_PPV_ARGS (&pFactory) );

  if (SUCCEEDED (hr)) {
    if (pFactory)
      pFactory->EnumAdapters (0, &pAdapter);
  }

  DXGI_SWAP_CHAIN_DESC desc;
  ZeroMemory (&desc, sizeof desc);

  desc.BufferDesc.Width  = 1;
  desc.BufferDesc.Height = 1;

  desc.BufferDesc.RefreshRate.Numerator   = 1;
  desc.BufferDesc.RefreshRate.Denominator = 1;
  desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_CENTERED;

  desc.SampleDesc.Count   = 1;
  desc.SampleDesc.Quality = 0;

  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = 2;

  HWND hwnd =
    CreateWindowW ( L"STATIC", L"Dummy DXGI Window",
                      WS_OVERLAPPEDWINDOW | WS_ICONIC | WS_DISABLED,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                          1, 1, 0,
                            nullptr, nullptr, 0x00 );

  desc.OutputWindow = hwnd;
  desc.Windowed     = TRUE;

  desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  CComPtr <IDXGISwapChain>      pSwapChain       = nullptr;
  CComPtr <ID3D11Device>        pDevice          = nullptr;
  D3D_FEATURE_LEVEL             featureLevel;
  CComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;

  HRESULT hrx =
    D3D11CreateDeviceAndSwapChain_Import (
      pAdapter,
        D3D_DRIVER_TYPE_UNKNOWN,
          nullptr,
            0,
              nullptr,
                0,
                  D3D11_SDK_VERSION,
                    &desc,
                      &pSwapChain,
                        &pDevice,
                          &featureLevel,
                            &pImmediateContext );

  if (SUCCEEDED (hrx)) {
    if (pDevice && pImmediateContext && pSwapChain) {
      if (pDevice != nullptr) {
        DXGI_VIRTUAL_HOOK (&pDevice, 5, "ID3D11Device::CreateTexture2D",
                               D3D11Dev_CreateTexture2D_Override, D3D11Dev_CreateTexture2D_Original,
                               D3D11Dev_CreateTexture2D_pfn);
      }

      if (pImmediateContext != nullptr) {
        DXGI_VIRTUAL_HOOK (&pImmediateContext, 7, "ID3D11DeviceContext::VSSetConstantBuffers",
                               D3D11_VSSetConstantBuffers_Override, D3D11_VSSetConstantBuffers_Original,
                               D3D11_VSSetConstantBuffers_pfn);

#if 0
        DXGI_VIRTUAL_HOOK (&pImmediateContext, 14, "ID3D11DeviceContext::Map",
                             D3D11_Map_Override, D3D11_Map_Original,
                             D3D11_Map_pfn);
#endif

        DXGI_VIRTUAL_HOOK (&pImmediateContext, 44, "ID3D11DeviceContext::RSSetViewports",
                             D3D11_RSSetViewports_Override, D3D11_RSSetViewports_Original,
                             D3D11_RSSetViewports_pfn);

#if 0
        DXGI_VIRTUAL_HOOK (&pImmediateContext, 45, "ID3D11DeviceContext::RSSetScissorRects",
                             D3D11_RSSetScissorRects_Override, D3D11_RSSetScissorRects_Original,
                             D3D11_RSSetScissorRects_pfn);

        DXGI_VIRTUAL_HOOK (&pImmediateContext, 47, "ID3D11DeviceContext::CopyResource",
                             D3D11_CopyResource_Override, D3D11_CopyResource_Original,
                             D3D11_CopyResource_pfn);

        DXGI_VIRTUAL_HOOK (&pImmediateContext, 48, "ID3D11DeviceContext::UpdateSubresource",
                             D3D11_UpdateSubresource_Override, D3D11_UpdateSubresource_Original,
                             D3D11_UpdateSubresource_pfn);
#endif
      }

      if (pSwapChain != nullptr) {
        DXGI_VIRTUAL_HOOK ( &pSwapChain, 10, "IDXGISwapChain::SetFullscreenState",
                                  DXGISwap_SetFullscreenState_Override,
                                           SetFullscreenState_Original,
                                             SetFullscreenState_pfn );

        DXGI_VIRTUAL_HOOK ( &pSwapChain, 11, "IDXGISwapChain::GetFullscreenState",
                                  DXGISwap_GetFullscreenState_Override,
                                           GetFullscreenState_Original,
                                             GetFullscreenState_pfn );

        DXGI_VIRTUAL_HOOK ( &pSwapChain, 13, "IDXGISwapChain::ResizeBuffers",
                                 DXGISwap_ResizeBuffers_Override,
                                          ResizeBuffers_Original,
                                            ResizeBuffers_pfn );

        DXGI_VIRTUAL_HOOK ( &pSwapChain, 14, "IDXGISwapChain::ResizeTarget",
                                  DXGISwap_ResizeTarget_Override,
                                           ResizeTarget_Original,
                                             ResizeTarget_pfn );

        CComPtr <IDXGIOutput> pOutput = nullptr;
        if (SUCCEEDED (pSwapChain->GetContainingOutput (&pOutput))) {
          if (pOutput != nullptr) {
            DXGI_VIRTUAL_HOOK ( &pOutput, 8, "IDXGIOutput::GetDisplayModeList",
                                      DXGIOutput_GetDisplayModeList_Override,
                                                 GetDisplayModeList_Original,
                                                 GetDisplayModeList_pfn );

            DXGI_VIRTUAL_HOOK ( &pOutput, 9, "IDXGIOutput::FindClosestMatchingMode",
                                      DXGIOutput_FindClosestMatchingMode_Override,
                                                 FindClosestMatchingMode_Original,
                                                 FindClosestMatchingMode_pfn );

            DXGI_VIRTUAL_HOOK ( &pOutput, 10, "IDXGIOutput::WaitForVBlank",
                                     DXGIOutput_WaitForVBlank_Override,
                                                WaitForVBlank_Original,
                                                WaitForVBlank_pfn );
          }
        }

        DXGI_VIRTUAL_HOOK (&pSwapChain, 8, "IDXGISwapChain::Present",
                                PresentCallback, Present_Original,
                                PresentSwapChain_pfn);

        CComPtr <IDXGISwapChain1> pSwapChain1 = nullptr;
        if (SUCCEEDED (pSwapChain->QueryInterface (IID_PPV_ARGS (&pSwapChain1)))) {
          DXGI_VIRTUAL_HOOK (&pSwapChain1, 22, "IDXGISwapChain1::Present1",
                                  Present1Callback, Present1_Original,
                                  Present1SwapChain1_pfn);
        }
      }
    }
  }

  DestroyWindow (hwnd);

  __d3d11_ready = true;

  HANDLE hThread =
    GetCurrentThread ();

  CloseHandle  (hThread);

  return hrx;
}


#include <unordered_set>

std::unordered_map <uint32_t, std::wstring> tex_hashes;
std::unordered_map <uint32_t, std::wstring> tex_hashes_ex;

std::unordered_map <uint32_t, std::wstring> tex_hashes_ffx;

std::unordered_set <uint32_t>               dumped_textures;
std::unordered_set <uint32_t>               dumped_collisions;
std::unordered_set <uint32_t>               injectable_textures;
std::unordered_set <uint32_t>               injected_collisions;

std::unordered_set <uint32_t>               injectable_ffx; // HACK FOR FFX


// Actually more of a cache manager at the moment...
class SK_D3D11_TexMgr {
public:
  SK_D3D11_TexMgr (void) {
    QueryPerformanceFrequency (&PerfFreq);
  }

  bool             isTexture2D  (uint32_t crc32);

  ID3D11Texture2D* getTexture2D ( uint32_t              crc32,
                            const D3D11_TEXTURE2D_DESC *pDesc,
                                  size_t               *pMemSize   = nullptr,
                                  float                *pTimeSaved = nullptr );

  void             refTexture2D ( ID3D11Texture2D      **pTex,
                            const D3D11_TEXTURE2D_DESC  *pDesc,
                                  uint32_t               crc32,
                                  size_t                 mem_size,
                                  uint64_t               load_time );

  void             reset         (void);
  bool             purgeTextures (size_t size_to_free, int* pCount, size_t* pFreed);

  struct tex2D_descriptor_s {
    ID3D11Texture2D      *texture    = nullptr;
    D3D11_TEXTURE2D_DESC  desc       = { 0 };
    size_t                mem_size   = 0L;
    uint64_t              load_time  = 0ULL;
    uint32_t              crc32      = 0x00;
    uint32_t              hits       = 0;
    uint64_t              last_used  = 0ULL;
  };

  std::unordered_set <ID3D11Texture2D *>      TexRefs_2D;

  std::unordered_map < uint32_t,
                       ID3D11Texture2D *  >   HashMap_2D;
  std::unordered_map < ID3D11Texture2D *,
                       tex2D_descriptor_s  >  Textures_2D;

  size_t                                      AggregateSize_2D  = 0ULL;
  size_t                                      RedundantData_2D  = 0ULL;
  uint32_t                                    RedundantLoads_2D = 0UL;
  uint32_t                                    Evicted_2D        = 0UL;
  float                                       RedundantTime_2D  = 0.0f;
  LARGE_INTEGER                               PerfFreq;
} SK_D3D11_Textures;

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






bool
SK::DXGI::Shutdown (void)
{
  if (SK_D3D11_Textures.RedundantLoads_2D > 0) {
    dll_log.Log ( L"[Perf Stats] At shutdown: %7.2f seconds and %7.2f MiB of"
                  L" CPU->GPU I/O avoided by %lu texture cache hits.",
                    SK_D3D11_Textures.RedundantTime_2D / 1000.0f,
                      (float)SK_D3D11_Textures.RedundantData_2D /
                                 (1024.0f * 1024.0f),
                        SK_D3D11_Textures.RedundantLoads_2D );

    std::unordered_map      < ID3D11Texture2D *,
                            SK_D3D11_TexMgr::tex2D_descriptor_s >::iterator it
      = SK_D3D11_Textures.Textures_2D.begin ();
  }

  if (sk::NVAPI::app_name == L"DarkSoulsIII.exe") {
    SK_DS3_ShutdownPlugin (L"dxgi");
  }

  DeleteCriticalSection (&tex_cs);
  DeleteCriticalSection (&hash_cs);
  DeleteCriticalSection (&dump_cs);
  DeleteCriticalSection (&cache_cs);
  DeleteCriticalSection (&inject_cs);

  FreeLibrary (hModD3D11);

  return SK_ShutdownCore (L"dxgi");
}

#include "utility.h"
extern uint32_t __cdecl crc32 (uint32_t crc, const void *buf, size_t size);

bool         SK_D3D11_need_tex_reset      = false;
int32_t      SK_D3D11_amount_to_purge     = 0L;

bool         SK_D3D11_dump_textures       = false;// true;
bool         SK_D3D11_inject_textures_ffx = false;
bool         SK_D3D11_inject_textures     = false;
bool         SK_D3D11_cache_textures      = false;
bool         SK_D3D11_mark_textures       = false;
std::wstring SK_D3D11_res_root            = L"";

// From core.cpp
extern std::wstring host_app;

bool
SK_D3D11_TexMgr::isTexture2D (uint32_t crc32)
{
  if (! SK_D3D11_cache_textures)
    return false;

  SK_AutoCriticalSection critical (&tex_cs);

  if (crc32 != 0x00 && HashMap_2D.count (crc32)) {
    return true;
  }

  return false;
}

void
__stdcall
SK_D3D11_ResetTexCache (void)
{
  SK_D3D11_Textures.reset ();
}

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

  PROCESS_MEMORY_COUNTERS pmc = { 0 };
  pmc.cb = sizeof pmc;

  GetProcessMemoryInfo (hProc, &pmc, sizeof pmc);

  if ( (SK_D3D11_Textures.AggregateSize_2D >> 20ULL) > cache_opts.max_size    ||
        SK_D3D11_Textures.TexRefs_2D.size ()         > cache_opts.max_entries ||
        SK_D3D11_need_tex_reset                                               ||
       (config.mem.reserve / 100.0f) * ullMemoryTotal_KiB 
                                                     < 
                          (pmc.PagefileUsage >> 10UL) )
  {
    //dll_log.Log (L"[DX11TexMgr] DXGI 1.4 Budget Change: Triggered a texture manager purge...");

    SK_D3D11_amount_to_purge =
      (pmc.PagefileUsage >> 10UL) - (float)(ullMemoryTotal_KiB >> 10ULL) *
                         (config.mem.reserve / 100.0f);

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

    ISKTextureD3D11* pSKTexture = nullptr;

    if ( desc.texture != nullptr &&
         SUCCEEDED (desc.texture->QueryInterface (IID_SKTextureD3D11, (void **)&pSKTexture)))
    {
      pSKTexture = (ISKTextureD3D11 *)desc.texture;

      pSKTexture->Release ();

      bool can_free = pSKTexture->can_free;

      if (can_free) {
        count++;

        purged += mem_size;

        AggregateSize_2D = max (0, AggregateSize_2D);

        if ( ( (AggregateSize_2D >> 20ULL) <= cache_opts.min_size &&
                                     count >= cache_opts.min_evict ) ||
             (SK_D3D11_amount_to_purge     <= purged              &&
                                     count >= cache_opts.min_evict ) ||
                                     count >= max_count )
        {
          SK_D3D11_amount_to_purge = max (0, SK_D3D11_amount_to_purge);
          //dll_log.Log ( L"[DX11TexMgr] Purged %llu MiB of texture "
                          //L"data across %lu textures",
                          //purged >> 20ULL, count );

          break;
        }
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

  if (isTexture2D (crc32)) {
    std::unordered_map <uint32_t, ID3D11Texture2D *>::iterator it =
      HashMap_2D.begin ();

    while (it != HashMap_2D.end ()) {
      if (! SK_D3D11_TextureIsCached (it->second)) {
        ++it;
        continue;
      }

      tex2D_descriptor_s desc2d;

      {
        SK_AutoCriticalSection critical (&tex_cs);

        desc2d =
          Textures_2D [it->second];
      }

      D3D11_TEXTURE2D_DESC desc = desc2d.desc;

      if ( desc2d.crc32        == crc32                 &&
           desc.Format         == pDesc->Format         &&
           desc.Width          == pDesc->Width          &&
           desc.Height         == pDesc->Height         &&
           desc.MipLevels      >= pDesc->MipLevels      &&
           desc.BindFlags      == pDesc->BindFlags      &&
           desc.CPUAccessFlags == pDesc->CPUAccessFlags &&
           desc.Usage          == pDesc->Usage ) {
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

        ((ISKTextureD3D11 *)pTex2D)->last_used.QuadPart = SK_QueryPerf ().QuadPart;

        desc2d.last_used = SK_QueryPerf ().QuadPart;
        desc2d.hits++;

        RedundantData_2D += size;
        RedundantTime_2D += fTime;
        RedundantLoads_2D++;

        return pTex2D;
      }

      else if (desc2d.crc32 == crc32) {
        dll_log.Log ( L"[DX11TexMgr] ## Hash Collision for Texture: "
                          L"'%08X'!! ## ",
                            crc32 );
      }

      ++it;
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

  void* dontcare = nullptr;
  if ( pTex != nullptr && 
       SUCCEEDED (pTex->QueryInterface (IID_SKTextureD3D11, &dontcare)) ) {
    return true;
  }

  return false;
}

void
__stdcall
SK_D3D11_UseTexture (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return;

  if (pTex == nullptr)
    return;

  void* dontcare = nullptr;
  if (SUCCEEDED (pTex->QueryInterface (IID_SKTextureD3D11, &dontcare))) {
    ISKTextureD3D11* pSKTex =
      (ISKTextureD3D11 *)pTex;

    pSKTex->last_used.QuadPart =
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

    ISKTextureD3D11* pSKTex =
      (ISKTextureD3D11 *)pTex;

    if (pSKTex->pTex != nullptr) {
      pSKTex->pTex->Release ();
//      pSKTex->pTex = nullptr;
    }

    if (pSKTex->pTexOverride != nullptr) {
      pSKTex->pTexOverride->Release ();
//      pSKTex->pTexOverride = nullptr;
    }

    SK_D3D11_Textures.Textures_2D.erase (pTex);
    SK_D3D11_Textures.HashMap_2D.erase  (crc32);
    SK_D3D11_Textures.TexRefs_2D.erase  (pTex);
  }
}

void
SK_D3D11_TexMgr::refTexture2D ( ID3D11Texture2D**     ppTex,
                          const D3D11_TEXTURE2D_DESC* pDesc,
                                uint32_t              crc32,
                                size_t                mem_size,
                                uint64_t              load_time )
{
  if (! SK_D3D11_cache_textures)
    return;

  if (ppTex == nullptr || crc32 == 0x00)
    return;

  //if (! injectable_textures.count (crc32))
    //return;

  if (SK_D3D11_TextureIsCached (*ppTex)) {
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

  new ISKTextureD3D11 (ppTex, mem_size, crc32);

  AggregateSize_2D += mem_size;

  tex2D_descriptor_s desc2d;

  desc2d.desc      =  *pDesc;
  desc2d.texture   =  *ppTex;
  desc2d.load_time =  load_time;
  desc2d.mem_size  =  mem_size;
  desc2d.crc32     =  crc32;

  SK_AutoCriticalSection critical (&cache_cs);

  TexRefs_2D.insert  (*ppTex);
  HashMap_2D.insert  (std::make_pair (crc32, *ppTex));
  Textures_2D.insert (std::make_pair (*ppTex, desc2d));

  // Hold a reference ourselves so that the game cannot free it
  (*ppTex)->AddRef ();
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

  wchar_t wszTexDumpDir [MAX_PATH] = { L'\0' };


  lstrcatW (wszTexDumpDir, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexDumpDir, L"\\dump\\textures\\");
  lstrcatW (wszTexDumpDir, host_app.c_str ());

  //
  // Walk custom textures so we don't have to query the filesystem on every
  //   texture load to check if a custom one exists.
  //
  if ( GetFileAttributesW (wszTexDumpDir) !=
         INVALID_FILE_ATTRIBUTES ) {
    WIN32_FIND_DATA fd;
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    int             files  = 0;
    LARGE_INTEGER   liSize = { 0 };

    LARGE_INTEGER   liCompressed   = { 0 };
    LARGE_INTEGER   liUncompressed = { 0 };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating dumped...    " );

    lstrcatW (wszTexDumpDir, L"\\*");

    hFind = FindFirstFileW (wszTexDumpDir, &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES) {
          if (StrStrIW (fd.cFileName, L".dds")) {
            uint32_t top_crc32;
            uint32_t checksum;

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

  wchar_t wszTexInjectDir [MAX_PATH] = { L'\0' };

  lstrcatW (wszTexInjectDir, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir, L"\\inject\\textures");

  if ( GetFileAttributesW (wszTexInjectDir) !=
         INVALID_FILE_ATTRIBUTES ) {
    WIN32_FIND_DATA fd;
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    int             files  = 0;
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

  wchar_t wszTexInjectDir_FFX [MAX_PATH] = { L'\0' };

  lstrcatW (wszTexInjectDir_FFX, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir_FFX, L"\\inject\\textures\\UnX_Old");

  if ( GetFileAttributesW (wszTexInjectDir_FFX) !=
         INVALID_FILE_ATTRIBUTES ) {
    WIN32_FIND_DATA fd;
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    int             files  = 0;
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
SK_D3D11_SetResourceRoot (std::wstring root)
{
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
  return;
}


bool
WINAPI
SK_D3D11_IsTexHashed (uint32_t top_crc32, uint32_t hash)
{
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
SK_D3D11_AddTexHash ( std::wstring name, uint32_t top_crc32, uint32_t hash )
{
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
    case DXGI_FORMAT_R8_SINT:                    return 1
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
crc32_tex (  _In_        const D3D11_TEXTURE2D_DESC   *pDesc,
             _In_opt_    const D3D11_SUBRESOURCE_DATA *pInitialData,
             _Out_opt_         size_t                 *pSize,
             _Out_opt_         uint32_t               *pLOD0_CRC32 )
{
  // Ignore Cubemaps for Now
  if (pDesc->MiscFlags == 0x04) {
    dll_log.Log (L"[ Tex Hash ] >> Will not hash cubemap");
    return 0;
  }

  if (pDesc->MiscFlags != 0x00) {
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

  int bpp = ( (pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS &&
               pDesc->Format <= DXGI_FORMAT_BC1_UNORM_SRGB) ||
              (pDesc->Format >= DXGI_FORMAT_BC4_TYPELESS &&
               pDesc->Format <= DXGI_FORMAT_BC4_SNORM) ) ? 0 : 1;

  unsigned int width  = pDesc->Width;
  unsigned int height = pDesc->Height;

        size_t size   = 0;

  if (compressed) {
    for (unsigned int i = 0; i < pDesc->MipLevels; i++) {
      char* pData    = (char *)pInitialData [i].pSysMem;
      int stride = bpp == 0 ?
          max (1, ((width + 3) / 4) ) * 8 :
          max (1, ((width + 3) / 4) ) * 16;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (stride == pInitialData [i].SysMemPitch) {
        unsigned int lod_size = stride * (height / 4 +
                                          height % 4);

        checksum = crc32c (checksum, (const uint8_t *)pData, lod_size);
        size    += lod_size;
      }

      else {
        // We are running through the compressed image block-by-block,
        //  the lines we are "scanning" actually represent 4 rows of image data.
        for (unsigned int j = 0; j < height; j += 4) {
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

      if (width  > 1) width  >>= 1;
      if (height > 1) height >>= 1;
    }
  }

  else {
    for (unsigned int i = 0; i < pDesc->MipLevels; i++) {
      char* pData      = (char *)pInitialData [i].pSysMem;
      int   scanlength = SK_D3D11_BytesPerPixel (pDesc->Format) * width;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (scanlength == pInitialData [i].SysMemPitch) {
        unsigned int lod_size = (scanlength * height);

        checksum = crc32c (checksum, (const uint8_t *)pData, lod_size);
        size    += lod_size;
      }

      else {
        for (unsigned int j = 0; j < height; j++) {
          checksum =
            crc32c (checksum, (const uint8_t *)pData, scanlength);

          pData += pInitialData [i].SysMemPitch;
          size  += scanlength;
        }
      }

      if (i == 0 && pLOD0_CRC32 != nullptr)
        *pLOD0_CRC32 = checksum;

      if (width  > 1) width  >>= 1;
      if (height > 1) height >>= 1;
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
crc32_ffx (  _In_        const D3D11_TEXTURE2D_DESC   *pDesc,
             _In_opt_    const D3D11_SUBRESOURCE_DATA *pInitialData,
             _Out_opt_         size_t                 *pSize )
{
  uint32_t checksum = 0;

  bool compressed = false;

  if (pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS && pDesc->Format <= DXGI_FORMAT_BC5_SNORM)
    compressed = true;

  if (pDesc->Format >= DXGI_FORMAT_BC6H_TYPELESS && pDesc->Format <= DXGI_FORMAT_BC7_UNORM_SRGB)
    compressed = true;

  int block_size = pDesc->Format == DXGI_FORMAT_BC1_UNORM ? 8 : 16;

  int width      = pDesc->Width;
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

#include "../depends/DirectXTex/DirectXTex.h"

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

      size_t lines =
        DirectX::ComputeScanlines (mdata.format, height);

      if  (! lines) {
        error = true;
        break;
      }

      auto sptr =
        reinterpret_cast <const uint8_t *>(
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

  wchar_t wszPath [MAX_PATH] = { L'\0' };

  lstrcatW (wszPath, SK_D3D11_res_root.c_str ());

  if (GetFileAttributes (wszPath) == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (wszPath, nullptr);

  lstrcatW (wszPath, L"/dump");

  if (GetFileAttributes (wszPath) == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (wszPath, nullptr);

  lstrcatW (wszPath, L"/textures");

  if (GetFileAttributes (wszPath) == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (wszPath, nullptr);

  lstrcatW (wszPath, L"/");
  lstrcatW (wszPath, host_app.c_str ());

  if (GetFileAttributes (wszPath) == INVALID_FILE_ATTRIBUTES)
    CreateDirectoryW (wszPath, nullptr);

  bool compressed = false;

  if ( ( pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS &&
         pDesc->Format <= DXGI_FORMAT_BC5_SNORM )  ||
       ( pDesc->Format >= DXGI_FORMAT_BC6H_TYPELESS &&
         pDesc->Format <= DXGI_FORMAT_BC7_UNORM_SRGB ) )
    compressed = true;

  wchar_t wszOutPath [MAX_PATH] = { L'\0' };
  wchar_t wszOutName [MAX_PATH] = { L'\0' };

  lstrcatW (wszOutPath, SK_D3D11_res_root.c_str ());
  lstrcatW (wszOutPath, L"\\dump\\textures\\");
  lstrcatW (wszOutPath, host_app.c_str ());

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

      return SaveToDDSFile ( image.GetImages (), image.GetImageCount (),
                               image.GetMetadata (), DirectX::DDS_FLAGS_NONE,
                                 wszOutName );
    }
  }

  return E_FAIL;
}

HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  bool early_out = false;

  if ((! (SK_D3D11_cache_textures || SK_D3D11_dump_textures || SK_D3D11_inject_textures)) ||
         tex_inject)
    early_out = true;

  if (early_out)
    return D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);

  LARGE_INTEGER load_start = SK_QueryPerf ();

  uint32_t      checksum  = 0;
  uint32_t      cache_tag = 0;
  size_t        size      = 0;

  ID3D11Texture2D* pCachedTex = nullptr;

  bool cacheable = pInitialData          != nullptr &&
                   pInitialData->pSysMem != nullptr &&
                   pDesc->Width > 0 && pDesc->Height > 0;

  cacheable &=
    (! ((pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) ||
        (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET)) ) &&
        (pDesc->CPUAccessFlags == 0x0);

  cacheable &= ppTexture2D != nullptr;

  bool dumpable = 
              cacheable && pDesc->Usage != D3D11_USAGE_DYNAMIC &&
                           pDesc->Usage != D3D11_USAGE_STAGING;

  uint32_t top_crc32 = 0x00;
  uint32_t ffx_crc32 = 0x00;

  if (cacheable) {
    checksum = crc32_tex (pDesc, pInitialData, &size, &top_crc32);

    if (SK_D3D11_inject_textures_ffx) {
      ffx_crc32 = crc32_ffx (pDesc, pInitialData, &size);
    }

    if ( checksum != 0x00 &&
         ( SK_D3D11_cache_textures                         ||
           SK_D3D11_IsInjectable     (top_crc32, checksum) ||
           SK_D3D11_IsInjectable     (top_crc32, 0x00)     ||
           SK_D3D11_IsInjectable_FFX (ffx_crc32)
         )
       )
    {
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

  if (cacheable) {
    if (D3DX11CreateTextureFromFileW != nullptr && SK_D3D11_res_root.length ()) {
      wchar_t wszTex [MAX_PATH] = { L'\0' };

      {
        if (SK_D3D11_IsTexHashed (ffx_crc32, 0x00)) {
          _swprintf ( wszTex, L"%s\\%s",
              SK_D3D11_res_root.c_str (),
                  SK_D3D11_TexHashToName (ffx_crc32, 0x00).c_str ()
          );
        }

        else if (SK_D3D11_IsTexHashed (top_crc32, checksum)) {
          _swprintf ( wszTex, L"%s\\%s",
                        SK_D3D11_res_root.c_str (),
                            SK_D3D11_TexHashToName (top_crc32,checksum).c_str ()
                    );
        }

        else if (SK_D3D11_IsTexHashed (top_crc32, 0x00)) {
          _swprintf ( wszTex, L"%s\\%s",
                        SK_D3D11_res_root.c_str (),
                            SK_D3D11_TexHashToName (top_crc32, 0x00).c_str ()
                    );
        }

        else if ( /*config.textures.d3d11.precise_hash &&*/
                  SK_D3D11_inject_textures           &&
                  SK_D3D11_IsInjectable (top_crc32, checksum) ) {
          _swprintf ( wszTex,
                        L"%s\\inject\\textures\\%08X_%08X.dds",
                          SK_D3D11_res_root.c_str (),
                            top_crc32, checksum );
        }

        else if ( SK_D3D11_inject_textures &&
                  SK_D3D11_IsInjectable (top_crc32, 0x00) ) {
          _swprintf ( wszTex,
                        L"%s\\inject\\textures\\%08X.dds",
                          SK_D3D11_res_root.c_str (),
                            top_crc32 );
        }

        else if ( SK_D3D11_inject_textures           &&
                  SK_D3D11_IsInjectable_FFX (ffx_crc32) ) {
          _swprintf ( wszTex,
                        L"%s\\inject\\textures\\Unx_Old\\%08X.dds",
                          SK_D3D11_res_root.c_str (),
                            ffx_crc32 );
        }

      }

      if (                   *wszTex  != L'\0' &&
           GetFileAttributes (wszTex) != INVALID_FILE_ATTRIBUTES )
      {
        SK_AutoCriticalSection inject_critical (&inject_cs);

        ID3D11Resource* pRes = nullptr;

#define D3DX11_DEFAULT -1

        D3DX11_IMAGE_INFO img_info;

        D3DX11GetImageInfoFromFileW (wszTex, nullptr, &img_info, nullptr);

        D3DX11_IMAGE_LOAD_INFO load_info;

        load_info.BindFlags      = pDesc->BindFlags;
        load_info.CpuAccessFlags = pDesc->CPUAccessFlags;
        load_info.Depth          = img_info.Depth;//D3DX11_DEFAULT;
        load_info.Filter         = D3DX11_DEFAULT;
        load_info.FirstMipLevel  = 0;
        load_info.Format         = pDesc->Format;
        load_info.Height         = img_info.Height;//D3DX11_DEFAULT;
        load_info.MipFilter      = D3DX11_DEFAULT;
        load_info.MipLevels      = img_info.MipLevels;//D3DX11_DEFAULT;
        load_info.MiscFlags      = img_info.MiscFlags;//pDesc->MiscFlags;
        load_info.pSrcInfo       = &img_info;
        load_info.Usage          = pDesc->Usage;
        load_info.Width          = img_info.Width;//D3DX11_DEFAULT;

#if 0
        tex_inject = true;

        if ( SUCCEEDED ( D3DX11CreateTextureFromFileW (
                           This, wszTex,
                             &load_info, nullptr,
             (ID3D11Resource**)ppTexture2D, nullptr )
                       )
           )
        {
          LARGE_INTEGER load_end = SK_QueryPerf ();

          tex_inject = false;

          SK_D3D11_Textures.refTexture2D (
            ppTexture2D,
              pDesc,
                cache_tag,
                  size,
                    load_end.QuadPart - load_start.QuadPart
          );

          return S_OK;
        }

        tex_inject = false;
#endif
      }
    }
  }

  HRESULT ret =
    D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);

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
      ppTexture2D,
        pDesc,
          cache_tag,
            size,
              load_end.QuadPart - load_start.QuadPart
    );
  }

  return ret;
}

void
WINAPI
SK_DXGI_SetPreferredAdapter (int override_id)
{
  SK_DXGI_preferred_adapter = override_id;
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
  } else {
    _swprintf ( wszDesc,
                  L"  VERTEX : <Unused>\n" );
  }

  //
  // GEOMETRY SHADING
  //
  if (stats.GSInvocations > 0) {
    _swprintf ( wszDesc,
                  L"%s  GEOM   : %s   (%s Prims)\n",
                    wszDesc,
                      SK_CountToString (stats.GSInvocations).c_str (),
                        SK_CountToString (stats.GSPrimitives).c_str () );
  } else {
    _swprintf ( wszDesc,
                  L"%s  GEOM   : <Unused>\n",
                    wszDesc );
  }

  //
  // TESSELLATION
  //
  if (stats.HSInvocations > 0 || stats.DSInvocations > 0) {
    _swprintf ( wszDesc,
                  L"%s  TESS   : %s Hull ==> %s Domain\n",
                    wszDesc,
                      SK_CountToString (stats.HSInvocations).c_str (),
                        SK_CountToString (stats.DSInvocations).c_str () ) ;
  } else {
    _swprintf ( wszDesc,
                  L"%s  TESS   : <Unused>\n",
                    wszDesc );
  }

  //
  // RASTERIZATION
  //
  if (stats.CInvocations > 0) {
    _swprintf ( wszDesc,
                  L"%s  RASTER : %5.1f%% Filled     (%s Triangles IN )\n",
                    wszDesc, 100.0f *
                        ( (float)stats.CPrimitives /
                          (float)stats.CInvocations ),
                      SK_CountToString (stats.CInvocations).c_str () );
  } else {
    _swprintf ( wszDesc,
                  L"%s  RASTER : <Unused>\n",
                    wszDesc );
  }

  //
  // PIXEL SHADING
  //
  if (stats.PSInvocations > 0) {
    _swprintf ( wszDesc,
                  L"%s  PIXEL  : %s   (%s Triangles OUT)\n",
                    wszDesc,
                      SK_CountToString (stats.PSInvocations).c_str (),
                        SK_CountToString (stats.CPrimitives).c_str () );
  } else {
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
#endif