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
  if (CreateDXGIFactory_Import == nullptr) {
    extern void SK_BootDXGI (void);
    SK_BootDXGI ();
  }

  while (! InterlockedCompareExchange (&__dxgi_ready, FALSE, FALSE)) {
    Sleep (config.system.init_delay);
  }
}

unsigned int __stdcall HookDXGI (LPVOID user);

#define D3D_FEATURE_LEVEL_12_0 0xc000
#define D3D_FEATURE_LEVEL_12_1 0xc100

const wchar_t*
SK_DXGI_DescribeScalingMode (DXGI_MODE_SCALING mode)
{
  switch (mode)
  {
  case DXGI_MODE_SCALING_CENTERED:
    return L"DXGI_MODE_SCALING_CENTERED";
  case DXGI_MODE_SCALING_UNSPECIFIED:
    return L"DXGI_MODE_SCALING_UNSPECIFIED";
  case DXGI_MODE_SCALING_STRETCHED:
    return L"DXGI_MODE_SCALING_STRETCHED";
  default:
    return L"UNKNOWN";
  }
}


std::wstring
SK_DXGI_FeatureLevelsToStr (       int    FeatureLevels,
                             const DWORD* pFeatureLevels )
{
  if (FeatureLevels == 0 || pFeatureLevels == nullptr)
    return L"N/A";

  std::wstring out = L"";

  for (int i = 0; i < FeatureLevels; i++)
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
  }
}

#define WaitForInit() {      \
    WaitForInitDXGI      (); \
    ::WaitForInit        (); \
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
        return;                                                             \
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

          if (hWndRender == 0 || (! IsWindow (hWndRender))) {
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

    if (config.render.dxgi.scaling_mode != -1)
      Flags |= DXGI_ENUM_MODES_SCALING;

    HRESULT hr =
      GetDisplayModeList_Original (
        This,
          EnumFormat,
            Flags,
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
              Flags,
                pNumModes,
                  pDesc );
    }

    if (SUCCEEDED (hr)) {
      int removed_count = 0;

      if ( ! ( config.render.dxgi.res.min.isZero ()   &&
               config.render.dxgi.res.max.isZero () ) &&
           *pNumModes != 0 ) {
        int last  = *pNumModes;
        int first = 0;

        std::set <int> coverage_min;
        std::set <int> coverage_max;

        if (! config.render.dxgi.res.min.isZero ()) {
          // Restrict MIN (Sequential Scan: Last->First)
          for ( int i = *pNumModes - 1 ; i >= 0 ; --i ) {
            if (SK_DXGI_RestrictResMin (WIDTH,  first, i,  coverage_min, pDesc) |
                SK_DXGI_RestrictResMin (HEIGHT, first, i,  coverage_min, pDesc))
              ++removed_count;
          }
        }

        if (! config.render.dxgi.res.max.isZero ()) {
          // Restrict MAX (Sequential Scan: First->Last)
          for ( UINT i = 0 ; i < *pNumModes ; ++i ) {
            if (SK_DXGI_RestrictResMax (WIDTH,  last, i, coverage_max, pDesc) |
                SK_DXGI_RestrictResMax (HEIGHT, last, i, coverage_max, pDesc))
              ++removed_count;
          }
        }
      }

      if (config.render.dxgi.scaling_mode != -1) {
        if (config.render.dxgi.scaling_mode != DXGI_MODE_SCALING_UNSPECIFIED) {
          for ( INT i = (INT)*pNumModes - 1 ; i >= 0 ; --i ) {
            if ( pDesc [i].Scaling != DXGI_MODE_SCALING_UNSPECIFIED &&
                 pDesc [i].Scaling != config.render.dxgi.scaling_mode ) {
              pDesc [i] = pDesc [i + 1];
              ++removed_count;
            }
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
    dll_log.Log (L"[   DXGI   ] [!] IDXGIOutput::FindClosestMatchingMode (...)");

    DXGI_MODE_DESC mode_to_match = *pModeToMatch;

    if ( config.render.dxgi.scaling_mode != -1 &&
         mode_to_match.Scaling           != config.render.dxgi.scaling_mode )
    {
      dll_log.Log ( L"[   DXGI   ]  >> Scaling Override "
                    L"(Requested: %s, Using: %s)",
                      SK_DXGI_DescribeScalingMode (
                        mode_to_match.Scaling
                      ),
                        SK_DXGI_DescribeScalingMode (
                          (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode
                        )
                  );

      mode_to_match.Scaling =
        (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode;
    }

    return FindClosestMatchingMode_Original (This, &mode_to_match, pClosestMatch, pConcernedDevice );
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

    if ( config.render.dxgi.scaling_mode != -1 &&
          pNewTargetParameters->Scaling  != 
            (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode ) {
      DXGI_MODE_DESC new_new_params =
        *pNewTargetParameters;

      dll_log.Log ( L"[   DXGI   ]  >> Scaling Override "
                    L"(Requested: %s, Using: %s)",
                      SK_DXGI_DescribeScalingMode (
                        pNewTargetParameters->Scaling
                      ),
                        SK_DXGI_DescribeScalingMode (
                          (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode
                        )
                  );

      new_new_params.Scaling =
        (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode;

      DXGI_MODE_DESC* pNewNewTargetParameters =
        &new_new_params;

      DXGI_CALL (ret, ResizeTarget_Original (This, pNewNewTargetParameters));
    } else {
      DXGI_CALL (ret, ResizeTarget_Original (This, pNewTargetParameters));
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
        pDesc->BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED ?
          L"Unspecified" :
          pDesc->BufferDesc.Scaling == DXGI_MODE_SCALING_CENTERED ?
            L"Centered" :
            L"Stretched",
        pDesc->Windowed ? L"Windowed" : L"Fullscreen",
        pDesc->BufferCount,
        pDesc->Flags,
        pDesc->SwapEffect         == 0 ?
          L"Discard" :
          pDesc->SwapEffect       == 1 ?
            L"Sequential" :
            pDesc->SwapEffect     == 2 ?
              L"<Unknown>" :
              pDesc->SwapEffect   == 3 ?
                L"Flip Sequential" :
                pDesc->SwapEffect == 4 ?
                  L"Flip Discard" :
                  L"<Unknown>" );
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
      } else {
        if (config.render.framerate.flip_discard)
          bFlipMode = dxgi_caps.present.flip_sequential;
      }

      if ( config.render.dxgi.scaling_mode != -1 &&
            pDesc->BufferDesc.Scaling      !=
              (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode ) {
        dll_log.Log ( L"[   DXGI   ]  >> Scaling Override "
                      L"(Requested: %s, Using: %s)",
                        SK_DXGI_DescribeScalingMode (
                          pDesc->BufferDesc.Scaling
                        ),
                          SK_DXGI_DescribeScalingMode (
                            (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode
                          )
                    );

        pDesc->BufferDesc.Scaling =
          (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode;
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

    if (EnumAdapters_Original == nullptr) {
      WaitForInitDXGI ();

      if (EnumAdapters_Original == nullptr)
        return;
    }

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
      dll_log.Log ( L"[   DXGI   ] Dedicated Video: %zu, Dedicated System: %zu, Shared System: %zu",
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
  std::queue <DWORD> tids = SK_SuspendAllOtherThreads ();

  bool ret = SK_StartupCore (L"dxgi", dxgi_init_callback);

  SK_ResumeThreads (tids);

  return ret;
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

__declspec (nothrow)
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
      void** vftable = *(void***)*&pFactory;

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
    desc.BufferDesc.Scaling                 = config.render.dxgi.scaling_mode == -1 ?
                                                DXGI_MODE_SCALING_UNSPECIFIED :
                             (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode;

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
    CComPtr <IDXGISwapChain>      pSwapChain        = nullptr;

    InterlockedExchange (&__dxgi_ready, TRUE);

    // DXI stuff is ready at this point, we'll hook the swapchain stuff
    //   after this call.

    HRESULT hrx =
      D3D11CreateDeviceAndSwapChain_Import (
        pAdapter,
          D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
              0,
                nullptr,
                  0,
                    D3D11_SDK_VERSION,
                      &desc, &pSwapChain,
                        &pDevice,
                          &featureLevel,
                            &pImmediateContext );

    if ( SUCCEEDED (hrx) &&
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
    }

    DestroyWindow (hwnd);
  }

    HookD3D11 (nullptr);
  }
  CoUninitialize ();

  InterlockedExchange (&__dxgi_ready, TRUE);

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