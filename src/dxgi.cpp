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
#define NOMINMAX

#include <SpecialK/stdafx.h>

#include <Windows.h>

#include <SpecialK/dxgi_interfaces.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/render_backend.h>
#include <SpecialK/window.h>

#include <comdef.h>
#include <atlbase.h>

#include <SpecialK/nvapi.h>
#include <SpecialK/config.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include <SpecialK/log.h>
#include <SpecialK/utility.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <SpecialK/command.h>
#include <SpecialK/console.h>
#include <SpecialK/framerate.h>
#include <SpecialK/steam_api.h>

#include <d3d11_1.h>

#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/diagnostics/crash_handler.h>

#include <SpecialK/tls.h>

#pragma comment (lib, "delayimp.lib")

#include <imgui/backends/imgui_d3d11.h>

#define SK_LOG_ONCE(x) { static bool logged = false; if (! logged) { dll_log.Log ((x)); logged = true; } }

extern volatile ULONG __SK_DLL_Ending;

extern void
ImGui_ImplDX11_Resize ( IDXGISwapChain *This,
                        UINT            BufferCount,
                        UINT            Width,
                        UINT            Height,
                        DXGI_FORMAT     NewFormat,
                        UINT            SwapChainFlags );

extern void SK_D3D11_EndFrame (void);

#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>
#include <CEGUI/DefaultResourceProvider.h>
#include <CEGUI/ImageManager.h>
#include <CEGUI/Image.h>
#include <CEGUI/Font.h>
#include <CEGUI/Scheme.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/falagard/WidgetLookManager.h>
#include <CEGUI/ScriptModule.h>
#include <CEGUI/XMLParser.h>
#include <CEGUI/GeometryBuffer.h>
#include <CEGUI/GUIContext.h>
#include <CEGUI/RenderTarget.h>
#include <CEGUI/AnimationManager.h>
#include <CEGUI/FontManager.h>
#include <CEGUI/RendererModules/Direct3D11/Renderer.h>

#include <SpecialK/osd/text.h>
#include <SpecialK/osd/popup.h>

#ifdef _WIN64

#pragma comment (lib, "CEGUI/x64/CEGUIDirect3D11Renderer-0.lib")

#pragma comment (lib, "CEGUI/x64/CEGUIBase-0.lib")
#pragma comment (lib, "CEGUI/x64/CEGUICoreWindowRendererSet.lib")
#pragma comment (lib, "CEGUI/x64/CEGUIRapidXMLParser.lib")
#pragma comment (lib, "CEGUI/x64/CEGUICommonDialogs-0.lib")
#pragma comment (lib, "CEGUI/x64/CEGUISTBImageCodec.lib")

#else

#pragma comment (lib, "CEGUI/Win32/CEGUIDirect3D11Renderer-0.lib")

#pragma comment (lib, "CEGUI/Win32/CEGUIBase-0.lib")
#pragma comment (lib, "CEGUI/Win32/CEGUICoreWindowRendererSet.lib")
#pragma comment (lib, "CEGUI/Win32/CEGUIRapidXMLParser.lib")
#pragma comment (lib, "CEGUI/Win32/CEGUICommonDialogs-0.lib")
#pragma comment (lib, "CEGUI/Win32/CEGUISTBImageCodec.lib")

#endif

static IDXGISwapChain* imgui_swap = nullptr;

void
ImGui_DX11Shutdown ( void )
{
  ImGui_ImplDX11_Shutdown ();
  imgui_swap = nullptr;
}

bool
ImGui_DX11Startup ( IDXGISwapChain* pSwapChain )
{
  CComPtr <ID3D11Device> pD3D11Dev = nullptr;
  
  if ( SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&pD3D11Dev))) )
  {
    CComPtr <ID3D11DeviceContext>    pImmediateContext = nullptr;
    pD3D11Dev->GetImmediateContext (&pImmediateContext);
  
    if (pImmediateContext != nullptr) {
      imgui_swap = pSwapChain;
      return ImGui_ImplDX11_Init (pSwapChain, pD3D11Dev, pImmediateContext);
    }
  }

  return false;
}

CEGUI::Direct3D11Renderer* cegD3D11 = nullptr;

static volatile ULONG __gui_reset          = TRUE;
static volatile ULONG __cegui_frames_drawn = 0;

ID3D11Device*        pGUIDev     = nullptr;
ID3D11DeviceContext* pCEG_DevCtx = nullptr;

void
SK_CEGUI_RelocateLog (void)
{
  // Move the log file that this darn thing just created...
  if (GetFileAttributesW (L"CEGUI.log") != INVALID_FILE_ATTRIBUTES) {
    char     szNewLogPath [MAX_PATH * 4] = {  '\0' };
    wchar_t wszNewLogPath [MAX_PATH]     = { L'\0' };

    wcscpy   (wszNewLogPath, SK_GetConfigPath ());
    wcstombs ( szNewLogPath, SK_GetConfigPath (), MAX_PATH * 4 - 1);

    lstrcatA ( szNewLogPath,  "logs\\CEGUI.log");
    lstrcatW (wszNewLogPath, L"logs\\CEGUI.log");

    CopyFileExW ( L"CEGUI.log", wszNewLogPath,
                    nullptr, nullptr, nullptr,
                      0x00 );

    CEGUI::Logger::getDllSingleton ().setLogFilename (szNewLogPath, true);

    CEGUI::Logger::getDllSingleton ().logEvent       ("[Special K] ---- Log File Moved ----");
    CEGUI::Logger::getDllSingleton ().logEvent       ("");

    DeleteFileW (L"CEGUI.log");
  }
}

CEGUI::Window* SK_achv_popup = nullptr;

CEGUI::System*
SK_CEGUI_GetSystem (void)
{
  return CEGUI::System::getDllSingletonPtr ();
}

void
SK_CEGUI_InitBase (void)
{
  try
  {
    // initialise the required dirs for the DefaultResourceProvider
    CEGUI::DefaultResourceProvider* rp =
        dynamic_cast <CEGUI::DefaultResourceProvider *>
            (CEGUI::System::getDllSingleton ().getResourceProvider ());

         char szRootPath [MAX_PATH + 2] = { 0 };
    snprintf (szRootPath, MAX_PATH, "%ws", _wgetenv (L"CEGUI_PARENT_DIR"));
              szRootPath [MAX_PATH] = '\0';

    CEGUI::String dataPathPrefix ( ( std::string (szRootPath) +
                                     std::string ("CEGUI/datafiles") ).c_str () );

    /* for each resource type, set a resource group directory. We cast strings
       to "const CEGUI::utf8*" in order to support general Unicode strings,
       rather than only ASCII strings (even though currently they're all ASCII).
       */
    rp->setResourceGroupDirectory("schemes",
      dataPathPrefix +reinterpret_cast<const CEGUI::utf8*>("/schemes/"));
    rp->setResourceGroupDirectory("imagesets",
      dataPathPrefix +reinterpret_cast<const CEGUI::utf8*>("/imagesets/"));
    rp->setResourceGroupDirectory("fonts",
      dataPathPrefix +reinterpret_cast<const CEGUI::utf8*>("/fonts/"));
    rp->setResourceGroupDirectory("layouts",
      dataPathPrefix +reinterpret_cast<const CEGUI::utf8*>("/layouts/"));
    rp->setResourceGroupDirectory("looknfeels",
      dataPathPrefix +reinterpret_cast<const CEGUI::utf8*>("/looknfeel/"));
    rp->setResourceGroupDirectory("lua_scripts",
      dataPathPrefix +reinterpret_cast<const CEGUI::utf8*>("/lua_scripts/"));
    rp->setResourceGroupDirectory("schemas",
      dataPathPrefix +reinterpret_cast<const CEGUI::utf8*>("/xml_schemas/"));
    rp->setResourceGroupDirectory("animations",
      dataPathPrefix +reinterpret_cast<const CEGUI::utf8*>("/animations/"));

    // set the default resource groups to be used
    CEGUI::ImageManager::setImagesetDefaultResourceGroup ("imagesets");
    //CEGUI::ImageManager::addImageType                    ("BasicImage");
    CEGUI::Font::setDefaultResourceGroup                 ("fonts");
    CEGUI::Scheme::setDefaultResourceGroup               ("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup    ("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup        ("layouts");
    CEGUI::ScriptModule::setDefaultResourceGroup         ("lua_scripts");
    CEGUI::AnimationManager::setDefaultResourceGroup     ("animations");

    CEGUI::SchemeManager* pSchemeMgr =
      CEGUI::SchemeManager::getDllSingletonPtr ();

    pSchemeMgr->createFromFile ("VanillaSkin.scheme");
    pSchemeMgr->createFromFile ("TaharezLook.scheme");

    CEGUI::FontManager* pFontMgr =
      CEGUI::FontManager::getDllSingletonPtr ();

    pFontMgr->createFromFile ("DejaVuSans-10-NoScale.font");
    pFontMgr->createFromFile ("DejaVuSans-12-NoScale.font");
    pFontMgr->createFromFile ("Jura-18-NoScale.font");
    pFontMgr->createFromFile ("Jura-13-NoScale.font");
    pFontMgr->createFromFile ("Jura-10-NoScale.font");

    CEGUI::System* pSys = CEGUI::System::getDllSingletonPtr ();

    // setup default group for validation schemas
    CEGUI::XMLParser* parser = pSys->getXMLParser ();

    if (parser->isPropertyPresent ("SchemaDefaultResourceGroup"))
      parser->setProperty ("SchemaDefaultResourceGroup", "schemas");

    // Set a default window size if CEGUI cannot figure it out...
    if ( CEGUI::System::getDllSingleton ().getRenderer ()->getDisplaySize () ==
           CEGUI::Sizef (0.0f, 0.0f) )
    {
      CEGUI::System::getDllSingleton ().getRenderer ()->setDisplaySize (
          CEGUI::Sizef (
            (float)(game_window.render_x),
              (float)(game_window.render_y)
          )
      );
    }

#if 0
    if ( config.window.borderless     &&
         config.window.res.override.x &&
         config.window.res.override.y )
    {
      CEGUI::System::getDllSingleton ().getRenderer ()->setDisplaySize (
          CEGUI::Sizef (
            config.window.res.override.x,
              config.window.res.override.y
          )
      );
    }
#endif

    CEGUI::WindowManager& window_mgr =
      CEGUI::WindowManager::getDllSingleton ();

    CEGUI::Window* root =
      window_mgr.createWindow ("DefaultWindow", "root");

    CEGUI::System::getDllSingleton ().getDefaultGUIContext ().setRootWindow (root);

    // This window is never used, it is the prototype from which all
    //   achievement popup dialogs will be cloned. This makes the whole
    //     process of instantiating popups quicker.
    SK_achv_popup =
      window_mgr.loadLayoutFromFile ("Achievements.layout");
 }

 catch (...)
 {
 }

  SK_Steam_ClearPopups ();
}

void ResetCEGUI_D3D11 (IDXGISwapChain* This)
{
  if (! config.cegui.enable)
  {
    // XXX: TODO (Full shutdown isn't necessary, just invalidate)
    ImGui_DX11Shutdown ();
    ImGui_DX11Startup  (This);
  }

  CComPtr <ID3D11Device> pDev = nullptr;

  if (SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev)))) {
    extern void SK_DXGI_UpdateSwapChain (IDXGISwapChain*);
    SK_DXGI_UpdateSwapChain (This);
  }

  //if (InterlockedCompareExchange (&__cegui_frames_drawn, 0, 0) < 2)
    //return;

  if (cegD3D11 != nullptr)
  {
    SK_TextOverlayManager::getInstance ()->destroyAllOverlays ();
    SK_PopupManager::getInstance ()->destroyAllPopups         ();

    if (pGUIDev != nullptr)
        pGUIDev  = nullptr;

    if (pCEG_DevCtx != nullptr)
    {
      pCEG_DevCtx->Release ();
      pCEG_DevCtx = nullptr;
    }

    CEGUI::WindowManager::getDllSingleton ().cleanDeadPool ();
    cegD3D11->destroySystem ();
    cegD3D11 = nullptr;

    // XXX: TODO (Full shutdown isn't necessary, just invalidate)
    ImGui_DX11Shutdown ();
  }

  else if (cegD3D11 == nullptr)
  {
    pGUIDev = nullptr;

    This->GetDevice (IID_PPV_ARGS (&pGUIDev));

    if (pGUIDev == nullptr)
      return;

    CComPtr <ID3D11Device> pDev = nullptr;

    // The process of bootstrapping this through CEGUI does not add a reference, so
    //   don't bother decrementing the reference count by releasing it.
    pGUIDev->GetImmediateContext (&pCEG_DevCtx);

    try
    {
      // For CEGUI to work correctly, it is necessary to set the viewport dimensions
      //   to the backbuffer size prior to bootstrap.
      CComPtr <ID3D11Texture2D> pBackBuffer = nullptr;

      if ( SUCCEEDED (This->GetBuffer (0, IID_PPV_ARGS (&pBackBuffer))) )
      {
        D3D11_TEXTURE2D_DESC backbuffer_desc;
        pBackBuffer->GetDesc (&backbuffer_desc);

        D3D11_VIEWPORT vp;
        ZeroMemory   (&vp, sizeof (vp));

        vp.Width    = (float)backbuffer_desc.Width;
        vp.Height   = (float)backbuffer_desc.Height;
        vp.MinDepth = 0;
        vp.MaxDepth = 1;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;

        pCEG_DevCtx->RSSetViewports (1, &vp);
      }

      cegD3D11 = (CEGUI::Direct3D11Renderer *)
        &CEGUI::Direct3D11Renderer::bootstrapSystem (pGUIDev, pCEG_DevCtx);

      ImGui_DX11Startup    (This);

      SK_CEGUI_RelocateLog ();
    }

    catch (...)
    {
      pGUIDev->Release     ();
      cegD3D11 = nullptr;
      return;
    }

    SK_CEGUI_InitBase    ();

    SK_PopupManager::getInstance       ()->destroyAllPopups (        );
    SK_TextOverlayManager::getInstance ()->resetAllOverlays (cegD3D11);

    SK_Steam_ClearPopups ();
    pGUIDev->Release     ();
  }
}

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

extern void  __stdcall SK_D3D11_TexCacheCheckpoint (void);
extern void  __stdcall SK_D3D11_UpdateRenderStats  (IDXGISwapChain* pSwapChain);
                                                   
extern void  __stdcall SK_D3D12_UpdateRenderStats  (IDXGISwapChain* pSwapChain);

extern BOOL __stdcall SK_NvAPI_SetFramerateLimit   (uint32_t        limit);
extern void __stdcall SK_NvAPI_SetAppFriendlyName  (const wchar_t*  wszFriendlyName);

volatile LONG  __dxgi_ready  = FALSE;

void WaitForInitDXGI (void)
{
  if (CreateDXGIFactory_Import == nullptr) {
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

const wchar_t*
SK_DXGI_DescribeScanlineOrder (DXGI_MODE_SCANLINE_ORDER order)
{
  switch (order)
  {
  case DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED:
    return L"DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED";
  case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE:
    return L"DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE";
  case DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST:
    return L"DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST";
  case DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST:
    return L"DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST";
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
      out += L" 9_1";
      break;
    case D3D_FEATURE_LEVEL_9_2:
      out += L" 9_2";
      break;
    case D3D_FEATURE_LEVEL_9_3:
      out += L" 9_3";
      break;
    case D3D_FEATURE_LEVEL_10_0:
      out += L" 10_0";
      break;
    case D3D_FEATURE_LEVEL_10_1:
      out += L" 10_1";
      break;
    case D3D_FEATURE_LEVEL_11_0:
      out += L" 11_0";
      break;
    case D3D_FEATURE_LEVEL_11_1:
      out += L" 11_1";
      break;
    case D3D_FEATURE_LEVEL_12_0:
      out += L" 12_0";
      break;
    case D3D_FEATURE_LEVEL_12_1:
      out += L" 12_1";
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


// TODO: Get this stuff out of here, it's breaking what little design work there is.
extern "C++" void SK_DS3_InitPlugin         (void);
extern "C++" bool SK_DS3_UseFlipMode        (void);
extern "C++" bool SK_DS3_IsBorderless       (void);

extern "C++" HRESULT STDMETHODCALLTYPE
                  SK_DS3_PresentFirstFrame   (IDXGISwapChain *, UINT, UINT);

extern "C++" HRESULT STDMETHODCALLTYPE
                  SK_FAR_PresentFirstFrame   (IDXGISwapChain *, UINT, UINT);

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
   UNREFERENCED_PARAMETER (last);

   auto& val = dim == WIDTH ? pDesc [idx].Width :
                              pDesc [idx].Height;

   auto  max = dim == WIDTH ? config.render.dxgi.res.max.x :
                              config.render.dxgi.res.max.y;

   bool covered_already = covered.count (idx) > 0;

   if ( (max > 0 &&
         val > max) || covered_already )
   {
     for ( int i = idx ; i > 0 ; --i )
     {
       if ( config.render.dxgi.res.max.x >= pDesc [i].Width  &&
            config.render.dxgi.res.max.y >= pDesc [i].Height &&
            covered.count (i) == 0 )
       {
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
   UNREFERENCED_PARAMETER (first);

   auto& val = dim == WIDTH ? pDesc [idx].Width :
                              pDesc [idx].Height;

   auto  min = dim == WIDTH ? config.render.dxgi.res.min.x :
                              config.render.dxgi.res.min.y;

   bool covered_already = covered.count (idx) > 0;

   if ( (min > 0 &&
         val < min) || covered_already )
   {
     for ( int i = 0 ; i < idx ; ++i )
     {
       if ( config.render.dxgi.res.min.x <= pDesc [i].Width  &&
            config.render.dxgi.res.min.y <= pDesc [i].Height &&
            covered.count (i) == 0 )
       {
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

  struct {
    BOOL allow_tearing   = FALSE;
  } swapchain;
} dxgi_caps;

BOOL
SK_DXGI_SupportsTearing (void)
{
  return dxgi_caps.swapchain.allow_tearing;
}

extern "C" {
  CreateSwapChain_pfn               CreateSwapChain_Original               = nullptr;
  PresentSwapChain_pfn              Present_Original                       = nullptr;
  Present1SwapChain1_pfn            Present1_Original                      = nullptr;
  SetFullscreenState_pfn            SetFullscreenState_Original            = nullptr;
  GetFullscreenState_pfn            GetFullscreenState_Original            = nullptr;
  ResizeBuffers_pfn                 ResizeBuffers_Original                 = nullptr;
  ResizeTarget_pfn                  ResizeTarget_Original                  = nullptr;

  GetDisplayModeList_pfn            GetDisplayModeList_Original            = nullptr;
  FindClosestMatchingMode_pfn       FindClosestMatchingMode_Original       = nullptr;
  WaitForVBlank_pfn                 WaitForVBlank_Original                 = nullptr;
  CreateSwapChainForHwnd_pfn        CreateSwapChainForHwnd_Original        = nullptr;
  CreateSwapChainForCoreWindow_pfn  CreateSwapChainForCoreWindow_Original  = nullptr;
  CreateSwapChainForComposition_pfn CreateSwapChainForComposition_Original = nullptr;

  GetDesc_pfn                       GetDesc_Original                       = nullptr;
  GetDesc1_pfn                      GetDesc1_Original                      = nullptr;
  GetDesc2_pfn                      GetDesc2_Original                      = nullptr;

  EnumAdapters_pfn                  EnumAdapters_Original                  = nullptr;
  EnumAdapters1_pfn                 EnumAdapters1_Original                 = nullptr;

  CreateDXGIFactory_pfn             CreateDXGIFactory_Import               = nullptr;
  CreateDXGIFactory1_pfn            CreateDXGIFactory1_Import              = nullptr;
  CreateDXGIFactory2_pfn            CreateDXGIFactory2_Import              = nullptr;

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

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
#if 0
    HANDLE hHookInitDXGI =
      (HANDLE)
        _beginthreadex ( nullptr,
                           0,
                             HookDXGI,
                               nullptr,
                                 0x00,
                                   nullptr );
#else
    HookDXGI (nullptr);
#endif
  }
}

#define WaitForInit() {      \
    WaitForInitDXGI      (); \
    ::WaitForInit        (); \
}

#define DXGI_STUB(_Return, _Name, _Proto, _Args)                            \
  _Return STDMETHODCALLTYPE                                                 \
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
  void STDMETHODCALLTYPE                                                    \
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
  if (riid == __uuidof (IDXGIFactory5))
    return 5;

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
  else if (riid == __uuidof (IDXGIFactory5))
    interface_name = L"IDXGIFactory5";
  else
  {
    wchar_t *pwszIID = nullptr;

    if (SUCCEEDED (StringFromIID (riid, (LPOLESTR *)&pwszIID)))
    {
      interface_name = pwszIID;
      CoTaskMemFree   (pwszIID);
    }
  }

  return interface_name;
}

int
SK_GetDXGIFactoryInterfaceVer (IUnknown *pFactory)
{
  CComPtr <IUnknown> pTemp = nullptr;

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory5> ((IDXGIFactory5 **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    dxgi_caps.present.waitable        = true;
    dxgi_caps.present.flip_discard    = true;

    HRESULT hr =
      ((IDXGIFactory5 *)pTemp.p)->CheckFeatureSupport (
        DXGI_FEATURE_PRESENT_ALLOW_TEARING,
          &dxgi_caps.swapchain.allow_tearing,
            sizeof (dxgi_caps.swapchain.allow_tearing)
      );

    dxgi_caps.swapchain.allow_tearing = 
      SUCCEEDED (hr) && dxgi_caps.swapchain.allow_tearing;

    return 5;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory4> ((IDXGIFactory4 **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    dxgi_caps.present.waitable        = true;
    dxgi_caps.present.flip_discard    = true;
    return 4;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory3> ((IDXGIFactory3 **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    dxgi_caps.present.waitable        = true;
    return 3;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory2> ((IDXGIFactory2 **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    return 2;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory1> ((IDXGIFactory1 **)&pTemp)))
  {
    dxgi_caps.device.latency_control  = true;
    return 1;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory> ((IDXGIFactory **)&pTemp)))
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

  if (iver == 5)
    return SK_GetDXGIFactoryInterfaceEx (__uuidof (IDXGIFactory5));

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
  else
  {
    wchar_t *pwszIID = nullptr;

    if (SUCCEEDED (StringFromIID (riid, (LPOLESTR *)&pwszIID)))
    {
      interface_name = pwszIID;
      CoTaskMemFree   (pwszIID);
    }
  }

  return interface_name;
}

int
SK_GetDXGIAdapterInterfaceVer (IUnknown *pAdapter)
{
  CComPtr <IUnknown> pTemp = nullptr;

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

void
SK_CEGUI_QueueResetD3D11 (void)
{
  InterlockedExchange (&__gui_reset, TRUE);
}



void
SK_DXGI_UpdateSwapChain (IDXGISwapChain* This)
{
  CComPtr <ID3D11Device> pDev = nullptr;

  if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))) )
  {
    SK_GetCurrentRenderBackend ().device    = pDev;
    SK_GetCurrentRenderBackend ().swapchain = This;

    if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
    {
      CComPtr <IDXGISurface> pSurf = nullptr;

      NvAPI_D3D_GetObjectHandleForResource (pDev, This, &SK_GetCurrentRenderBackend ().surface);

      // NVIDIA's drivers deadlock us if you try to do this, so just assume that fullscreen mode is
      //   always G-Sync'd.
      //
      //if (SK_GetCurrentRenderBackend ().fullscreen_exclusive)
      //{
        //if (SUCCEEDED (This->GetBuffer (0, IID_PPV_ARGS (&pSurf))))
          //NvAPI_D3D_GetObjectHandleForResource (pDev, pSurf, &SK_GetCurrentRenderBackend ().surface);
      //}
    }
  }
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
  FLOAT                      OMBlendFactor  [4];
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

void CreateStateblock(ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
  ZeroMemory (sb, sizeof D3DX11_STATE_BLOCK);

  dc->VSGetShader          (&sb->VS, sb->VSInterfaces, &sb->VSInterfaceCount);

  dc->VSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->VSSamplers);
  dc->VSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->VSShaderResources);
  dc->VSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->VSConstantBuffers);

  dc->GSGetShader          (&sb->GS, sb->GSInterfaces, &sb->GSInterfaceCount);

  dc->GSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->GSSamplers);
  dc->GSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->GSShaderResources);
  dc->GSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->GSConstantBuffers);

  dc->HSGetShader          (&sb->HS, sb->HSInterfaces, &sb->HSInterfaceCount);

  dc->HSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->HSSamplers);
  dc->HSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->HSShaderResources);
  dc->HSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->HSConstantBuffers);

  dc->DSGetShader          (&sb->DS, sb->DSInterfaces, &sb->DSInterfaceCount);
  dc->DSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->DSSamplers);
  dc->DSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->DSShaderResources);
  dc->DSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->DSConstantBuffers);

  dc->PSGetShader          (&sb->PS, sb->PSInterfaces, &sb->PSInterfaceCount);

  dc->PSGetSamplers        (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->PSSamplers);
  dc->PSGetShaderResources (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->PSShaderResources);
  dc->PSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->PSConstantBuffers);

  dc->CSGetShader          (&sb->CS, sb->CSInterfaces, &sb->CSInterfaceCount);

  dc->CSGetSamplers             (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,             sb->CSSamplers);
  dc->CSGetShaderResources      (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,      sb->CSShaderResources);
  dc->CSGetConstantBuffers      (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, sb->CSConstantBuffers);
  dc->CSGetUnorderedAccessViews (0, D3D11_PS_CS_UAV_REGISTER_COUNT,                    sb->CSUnorderedAccessViews);

  dc->IAGetVertexBuffers     (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, sb->IAVertexBuffers,
                                                                            sb->IAVertexBuffersStrides,
                                                                            sb->IAVertexBuffersOffsets);
  dc->IAGetIndexBuffer       (&sb->IAIndexBuffer, &sb->IAIndexBufferFormat, &sb->IAIndexBufferOffset);
  dc->IAGetInputLayout       (&sb->IAInputLayout);
  dc->IAGetPrimitiveTopology (&sb->IAPrimitiveTopology);

  dc->OMGetRenderTargetsAndUnorderedAccessViews ( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, sb->OMRenderTargets,
               &sb->OMRenderTargetStencilView, 0, D3D11_PS_CS_UAV_REGISTER_COUNT,         sb->OMUnorderedAccessViews );

  dc->OMGetDepthStencilState (&sb->OMDepthStencilState, &sb->OMDepthStencilRef);
  dc->OMGetBlendState        (&sb->OMBlendState,         sb->OMBlendFactor, &sb->OMSampleMask);

  sb->RSViewportCount    = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  dc->RSGetViewports         (&sb->RSViewportCount, sb->RSViewports);

  sb->RSScissorRectCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  dc->RSGetScissorRects      (&sb->RSScissorRectCount, sb->RSScissorRects);
  dc->RSGetState             (&sb->RSRasterizerState);

  //dc->SOGetTargets         (4, sb->SOBuffers);
  dc->GetPredication         (&sb->Predication, &sb->PredicationValue);
}

template <class _T>
UINT
calc_count (_T** arr, UINT max_count)
{
  for ( UINT i = 0         ;
             i < max_count ;
           ++i )           {
    if (arr [i] == 0)
      return i;
  }

  return max_count;
}

void ApplyStateblock(ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
  UINT minus_one [D3D11_PS_CS_UAV_REGISTER_COUNT] = { std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
                                                      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
                                                      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
                                                      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max () };
  
  dc->VSSetShader            (sb->VS, sb->VSInterfaces, sb->VSInterfaceCount);

  UINT VSSamplerCount =
    calc_count               (sb->VSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

  if (VSSamplerCount)
    dc->VSSetSamplers        (0, VSSamplerCount, sb->VSSamplers);

  UINT VSShaderResourceCount =
    calc_count               (sb->VSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

  if (VSShaderResourceCount)
    dc->VSSetShaderResources (0, VSShaderResourceCount, sb->VSShaderResources);

  UINT VSConstantBufferCount =
    calc_count               (sb->VSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

  if (VSConstantBufferCount)
    dc->VSSetConstantBuffers (0, VSConstantBufferCount, sb->VSConstantBuffers);

  dc->GSSetShader            (sb->GS, sb->GSInterfaces, sb->GSInterfaceCount);

  UINT GSSamplerCount =
    calc_count               (sb->GSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

  if (GSSamplerCount)
    dc->GSSetSamplers        (0, GSSamplerCount, sb->GSSamplers);

  UINT GSShaderResourceCount =
    calc_count               (sb->GSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

  if (GSShaderResourceCount)
    dc->GSSetShaderResources (0, GSShaderResourceCount, sb->GSShaderResources);

  UINT GSConstantBufferCount =
    calc_count               (sb->GSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

  if (GSConstantBufferCount)
    dc->GSSetConstantBuffers (0, GSConstantBufferCount, sb->GSConstantBuffers);

  dc->HSSetShader            (sb->HS, sb->HSInterfaces, sb->HSInterfaceCount);

  UINT HSSamplerCount =
    calc_count               (sb->HSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

  if (HSSamplerCount)
    dc->HSSetSamplers        (0, HSSamplerCount, sb->HSSamplers);

  UINT HSShaderResourceCount =
    calc_count               (sb->HSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

  if (HSShaderResourceCount)
    dc->HSSetShaderResources (0, HSShaderResourceCount, sb->HSShaderResources);

  UINT HSConstantBufferCount =
    calc_count               (sb->HSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

  if (HSConstantBufferCount)
    dc->HSSetConstantBuffers (0, HSConstantBufferCount, sb->HSConstantBuffers);

  dc->DSSetShader            (sb->DS, sb->DSInterfaces, sb->DSInterfaceCount);

  UINT DSSamplerCount =
    calc_count               (sb->DSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

  if (DSSamplerCount)
    dc->DSSetSamplers        (0, DSSamplerCount, sb->DSSamplers);

  UINT DSShaderResourceCount =
    calc_count               (sb->DSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

  if (DSShaderResourceCount)
    dc->DSSetShaderResources (0, DSShaderResourceCount, sb->DSShaderResources);

  UINT DSConstantBufferCount =
    calc_count               (sb->DSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

  if (DSConstantBufferCount)
    dc->DSSetConstantBuffers (0, DSConstantBufferCount, sb->DSConstantBuffers);

  dc->PSSetShader            (sb->PS, sb->PSInterfaces, sb->PSInterfaceCount);

  UINT PSSamplerCount =
    calc_count               (sb->PSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

  if (PSSamplerCount)
    dc->PSSetSamplers        (0, PSSamplerCount, sb->PSSamplers);

  UINT PSShaderResourceCount =
    calc_count               (sb->PSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

  if (PSShaderResourceCount)
    dc->PSSetShaderResources (0, PSShaderResourceCount, sb->PSShaderResources);

  //D3D11_PSSetShaderResources_Original (dc, 0, PSShaderResourceCount, sb->PSShaderResources);
  UINT PSConstantBufferCount =
    calc_count               (sb->PSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

  if (PSConstantBufferCount)
    dc->PSSetConstantBuffers (0, PSConstantBufferCount, sb->PSConstantBuffers);

  dc->CSSetShader            (sb->CS, sb->CSInterfaces, sb->CSInterfaceCount);

  UINT CSSamplerCount =
    calc_count               (sb->CSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

  if (CSSamplerCount)
    dc->CSSetSamplers        (0, CSSamplerCount, sb->CSSamplers);

  UINT CSShaderResourceCount =
    calc_count               (sb->CSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

  if (CSShaderResourceCount)
    dc->CSSetShaderResources (0, CSShaderResourceCount, sb->CSShaderResources);

  UINT CSConstantBufferCount =
    calc_count               (sb->CSConstantBuffers, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);

  if (CSConstantBufferCount)
    dc->CSSetConstantBuffers (0, CSConstantBufferCount, sb->CSConstantBuffers);

  dc->CSSetUnorderedAccessViews (0, D3D11_PS_CS_UAV_REGISTER_COUNT, sb->CSUnorderedAccessViews, minus_one);

  UINT IAVertexBufferCount =
    calc_count               (sb->IAVertexBuffers, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

  if (IAVertexBufferCount)
    dc->IASetVertexBuffers   (0, IAVertexBufferCount, sb->IAVertexBuffers, sb->IAVertexBuffersStrides, sb->IAVertexBuffersOffsets);

  dc->IASetIndexBuffer       (sb->IAIndexBuffer, sb->IAIndexBufferFormat, sb->IAIndexBufferOffset);
  dc->IASetInputLayout       (sb->IAInputLayout);
  dc->IASetPrimitiveTopology (sb->IAPrimitiveTopology);

  dc->OMSetRenderTargetsAndUnorderedAccessViews ( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, sb->OMRenderTargets, 
                sb->OMRenderTargetStencilView, 0, D3D11_PS_CS_UAV_REGISTER_COUNT,         sb->OMUnorderedAccessViews, minus_one );

  UINT OMRenderTargetCount =
    calc_count (sb->OMRenderTargets, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

  if (OMRenderTargetCount)
    dc->OMSetRenderTargets   (OMRenderTargetCount, sb->OMRenderTargets, sb->OMRenderTargetStencilView);

  dc->OMSetDepthStencilState (sb->OMDepthStencilState, sb->OMDepthStencilRef);
  dc->OMSetBlendState        (sb->OMBlendState,        sb->OMBlendFactor, sb->OMSampleMask);

  dc->RSSetViewports         (sb->RSViewportCount,     sb->RSViewports);
  dc->RSSetScissorRects      (sb->RSScissorRectCount,  sb->RSScissorRects);

  dc->RSSetState             (sb->RSRasterizerState);

  //UINT SOBuffersOffsets [4] = { 0 }; /* (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 0,
  //                                      (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 1, 
  //                                      (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 2, 
  //                                      (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 3 };*/
  //
  //UINT SOBufferCount =
  //  calc_count (sb->SOBuffers, 4);
  //
  //if (SOBufferCount)
  //  dc->SOSetTargets (SOBufferCount, sb->SOBuffers, SOBuffersOffsets);

  dc->SetPredication (sb->Predication, sb->PredicationValue);
}





void
SK_CEGUI_DrawD3D11 (IDXGISwapChain* This)
{
  CComPtr <ID3D11Device> pDev = nullptr;

  if (! config.cegui.enable)
    return;

  if (InterlockedExchangeAdd (&__SK_DLL_Ending, 0))
    return;

  InterlockedIncrement (&__cegui_frames_drawn);

  if (InterlockedCompareExchange (&__gui_reset, FALSE, TRUE))
  {
    SK_TextOverlayManager::getInstance ()->destroyAllOverlays ();

    if (pGUIDev != nullptr)
    {
      //pGUIDev->Release ();
      pGUIDev = nullptr;
    }

    if (pCEG_DevCtx != nullptr)
    {
      pCEG_DevCtx->Release ();
      pCEG_DevCtx = nullptr;
    }

    if (cegD3D11 != nullptr)
    {
      CEGUI::WindowManager::getDllSingleton ().cleanDeadPool ();
      cegD3D11->destroySystem ();
      cegD3D11 = nullptr;
    }
  }

  else if (cegD3D11 == nullptr)
  {
    extern void
    SK_InstallWindowHook (HWND hWnd);

    DXGI_SWAP_CHAIN_DESC desc;
    This->GetDesc (&desc);

    SK_InstallWindowHook (desc.OutputWindow);

    ResetCEGUI_D3D11 (This);
  }

  else if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))) )
  {
    if (pGUIDev != pDev)
    {
      ResetCEGUI_D3D11 (This);
      return;
    }

    HRESULT hr;

    CComPtr <ID3D11Texture2D>        pBackBuffer       = nullptr;
    CComPtr <ID3D11RenderTargetView> pRenderTargetView = nullptr;
    CComPtr <ID3D11BlendState>       pBlendState       = nullptr;

    CComPtr <ID3D11Device1>          pDevice1          = nullptr;
    CComPtr <ID3DDeviceContextState> pCtxState         = nullptr;
    CComPtr <ID3DDeviceContextState> pCtxStateOrig     = nullptr;

    hr = This->GetBuffer (0, IID_PPV_ARGS (&pBackBuffer));

    if (FAILED (hr)) {
      SK_LOG_ONCE (L"[   DXGI   ]  *** Backbuffer unavailable! ***");
      return;
    }

    CComPtr <ID3D11DeviceContext>  pImmediateContext  = nullptr;
    CComPtr <ID3D11DeviceContext1> pImmediateContext1 = nullptr;

    if (! config.render.dxgi.slow_state_cache)
    {
                          pDev->GetImmediateContext (&pImmediateContext);
        hr =
        pImmediateContext->QueryInterface (IID_PPV_ARGS (&pImmediateContext1));
                     pDev->QueryInterface (IID_PPV_ARGS (&pDevice1));

        if (FAILED (hr)) {
          SK_LOG_ONCE (L"[   DXGI   ]  *** Could not query ID3D11Device1 interface! ***");
          return;
        }

      D3D_FEATURE_LEVEL ft_lvl =
        pDev->GetFeatureLevel ();
      
      GUID              dev_lvl = (ft_lvl == D3D_FEATURE_LEVEL_11_1) ?
                                     __uuidof (ID3D11Device1) :
                                     __uuidof (ID3D11Device);

      //
      // DXGI state blocks the (fun?) way :)  -- Performance implications are unknown, as
      //                                           is compatibility with other injectors that
      //                                             may try to cache state.
      //
      if (FAILED (pDevice1->CreateDeviceContextState ( 0x00,
                                                         &ft_lvl,
                                                           1,
                                                             D3D11_SDK_VERSION,
                                                               dev_lvl,
                                                                 nullptr,
                                                                   &pCtxState )))
      {
        SK_LOG_ONCE (L"[   DXGI   ]  *** CreateDeviceContextState (...) failed! ***");
        return;
      }
    }

    D3D11_TEXTURE2D_DESC tex2d_desc;
    pBackBuffer->GetDesc (&tex2d_desc);

    // SRGB Correction for UIs
    switch (tex2d_desc.Format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      {
        D3D11_RENDER_TARGET_VIEW_DESC rtdesc = { };
        rtdesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        hr = pDev->CreateRenderTargetView (pBackBuffer, &rtdesc, &pRenderTargetView);

        SK_GetCurrentRenderBackend ().framebuffer_flags |= SK_FRAMEBUFFER_FLAG_SRGB;
      } break;

      default:
        hr = pDev->CreateRenderTargetView (pBackBuffer, nullptr, &pRenderTargetView);

        SK_GetCurrentRenderBackend ().framebuffer_flags &= (~SK_FRAMEBUFFER_FLAG_SRGB);
        break;
    }

    if (SUCCEEDED (hr))
    {
      static D3DX11_STATE_BLOCK sb;

      if (config.render.dxgi.slow_state_cache)
        CreateStateblock (pCEG_DevCtx, &sb);
      else
        pImmediateContext1->SwapDeviceContextState (pCtxState, &pCtxStateOrig);

      pCEG_DevCtx->OMSetRenderTargets (1, &pRenderTargetView, nullptr);

      D3D11_TEXTURE2D_DESC   backbuffer_desc;
      pBackBuffer->GetDesc (&backbuffer_desc);

      D3D11_BLEND_DESC blend;
      ZeroMemory     (&blend, sizeof (blend));

      blend.RenderTarget [0].BlendEnable           = TRUE;
      blend.RenderTarget [0].SrcBlend              = D3D11_BLEND_ONE;
      blend.RenderTarget [0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
      blend.RenderTarget [0].BlendOp               = D3D11_BLEND_OP_ADD;
      blend.RenderTarget [0].SrcBlendAlpha         = D3D11_BLEND_ONE;
      blend.RenderTarget [0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
      blend.RenderTarget [0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
      blend.RenderTarget [0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

      if (SUCCEEDED (pDev->CreateBlendState (&blend, &pBlendState)))
        pCEG_DevCtx->OMSetBlendState (pBlendState, NULL, 0xffffffff);

      D3D11_VIEWPORT vp;
      ZeroMemory   (&vp, sizeof (vp));

      vp.Width    = (float)backbuffer_desc.Width;
      vp.Height   = (float)backbuffer_desc.Height;
      vp.MinDepth = 0;
      vp.MaxDepth = 1;
      vp.TopLeftX = 0;
      vp.TopLeftY = 0;

      pCEG_DevCtx->RSSetViewports (1, &vp);

      cegD3D11->beginRendering ();
      {
        SK_TextOverlayManager::getInstance ()->drawAllOverlays (0.0f, 0.0f);

        SK_Steam_DrawOSD ();

        CEGUI::System::getDllSingleton ().renderAllGUIContexts ();
      }
      cegD3D11->endRendering ();

      extern bool SK_ImGui_Visible;

      if (SK_ImGui_Visible)
      {
        // XXX: TODO (Full startup isn't necessary, just update framebuffer dimensions).
        if (ImGui_DX11Startup             ( This                         ))
        {
          extern DWORD SK_ImGui_DrawFrame ( DWORD dwFlags, void* user    );
                       SK_ImGui_DrawFrame (       0x00,          nullptr );
        }
      }

      if (config.render.dxgi.slow_state_cache)
        ApplyStateblock (pCEG_DevCtx, &sb);
      else
        pImmediateContext1->SwapDeviceContextState (pCtxStateOrig, nullptr);
    }
  }

  // The config UI may change this, apply the setting AFTER the UI is drawn.
  extern bool SK_DXGI_SlowStateCache;
  config.render.dxgi.slow_state_cache = SK_DXGI_SlowStateCache;
}

void
SK_DXGI_BorderCompensation (UINT& x, UINT& y)
{
  if (! config.window.borderless)
    return;

  return;

  RECT game_rect = *SK_GetGameRect ();

  int x_dlg = SK_GetSystemMetrics (SM_CXDLGFRAME);
  int y_dlg = SK_GetSystemMetrics (SM_CYDLGFRAME);
  int title = SK_GetSystemMetrics (SM_CYCAPTION);

  if ( SK_DiscontEpsilon (
          x,
            (game_rect.right - game_rect.left),
              2 * x_dlg + 1
       )

      ||

       SK_DiscontEpsilon (
          y,
            (game_rect.bottom - game_rect.top),
              2 * y_dlg + title + 1
       )
     )
  {
    x = game_rect.right  - game_rect.left;
    y = game_rect.bottom - game_rect.top;

    dll_log.Log ( L"[Window Mgr] Border Compensated Resolution ==> (%lu x %lu)",
                    x, y );
  }
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
    SK_LOG_ONCE (L"Present1");

    //
    // Early-out for games that use testing to minimize blocking
    //
    if (PresentFlags & DXGI_PRESENT_TEST)
    {
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

    CComPtr <ID3D11Device> pDev = nullptr;

    int interval = config.render.framerate.present_interval;
    int flags    = PresentFlags;

    // Application preference
    if (interval == -1)
      interval = SyncInterval;

    if (bFlipMode)
    {
      flags = PresentFlags | DXGI_PRESENT_RESTART;

      if (bWait)
        flags |= DXGI_PRESENT_DO_NOT_WAIT;
    }

    static bool first_frame = true;

    if (first_frame)
    {
      if (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe"))
        SK_FO4_PresentFirstFrame (This, SyncInterval, flags);

      else if (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe"))
        SK_DS3_PresentFirstFrame (This, SyncInterval, flags);

      // TODO: Clean this code up
      if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))) )
      {
        CComPtr <IDXGIDevice>  pDevDXGI = nullptr;
        CComPtr <IDXGIAdapter> pAdapter = nullptr;
        CComPtr <IDXGIFactory> pFactory = nullptr;

        if ( SUCCEEDED (pDev->QueryInterface (IID_PPV_ARGS (&pDevDXGI))) &&
             SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter))  &&
             SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory))) )
        {
          DXGI_SWAP_CHAIN_DESC desc;
          This->GetDesc (&desc);


          static bool bIsLegoUndercover = 
              (StrStrIW (SK_GetHostApp (), L"LEGOLCUR_DX11.exe") != nullptr);
          
          if (bIsLegoUndercover)  pFactory->MakeWindowAssociation ( 0, 0 );


          if (bAlwaysAllowFullscreen)
          {
            pFactory->MakeWindowAssociation (
              desc.OutputWindow,
                DXGI_MWA_NO_WINDOW_CHANGES
            );
          }

          if (hWndRender == 0 || (! IsWindow (hWndRender)))
          {
            hWndRender       = desc.OutputWindow;

            SK_InstallWindowHook (hWndRender);
            game_window.hWnd =    hWndRender;
          }
        }
      }
    }

    else
      SK_CEGUI_DrawD3D11 (This);

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
    //dll_log.Log (L"SWAP %x, %lu, %p", Flags, SyncInterval, This);

    //
    // Early-out for games that use testing to minimize blocking
    //
    if (Flags & DXGI_PRESENT_TEST)
      return Present_Original (This, SyncInterval, Flags);


#ifdef DARK_SOULS
    if (__DS3_HEIGHT != nullptr)
    {
      DXGI_SWAP_CHAIN_DESC swap_desc;
      if (SUCCEEDED (This->GetDesc (&swap_desc)))
      {
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

    CComPtr <ID3D11Device> pDev = nullptr;

    static bool first_frame = true;

    if (first_frame)
    {
      if (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe"))
        SK_FO4_PresentFirstFrame (This, SyncInterval, Flags);

      else if (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe"))
        SK_DS3_PresentFirstFrame (This, SyncInterval, Flags);

      else if (! lstrcmpW (SK_GetHostApp (), L"NieRAutomata.exe"))
        SK_FAR_PresentFirstFrame (This, SyncInterval, Flags);

      // TODO: Clean this code up
      if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))) )
      {
        CComPtr <IDXGIDevice>  pDevDXGI = nullptr;
        CComPtr <IDXGIAdapter> pAdapter = nullptr;
        CComPtr <IDXGIFactory> pFactory = nullptr;

        if ( SUCCEEDED (pDev->QueryInterface (IID_PPV_ARGS (&pDevDXGI))) &&
             SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter))  &&
             SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory))) )
        {
          DXGI_SWAP_CHAIN_DESC desc;
          This->GetDesc      (&desc);

          SK_GetCurrentRenderBackend ().device    = pDev;
          SK_GetCurrentRenderBackend ().swapchain = This;

          if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
            NvAPI_D3D_GetObjectHandleForResource (pDev, This, &SK_GetCurrentRenderBackend ().surface);



          static bool bIsLegoUndercover = 
              (StrStrIW (SK_GetHostApp (), L"LEGOLCUR_DX11.exe") != nullptr);
          
          if (bIsLegoUndercover)  pFactory->MakeWindowAssociation ( 0, 0 );



          if (bAlwaysAllowFullscreen)
            pFactory->MakeWindowAssociation (desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES);

          hWndRender       = desc.OutputWindow;

          SK_InstallWindowHook (hWndRender);
          game_window.hWnd =    hWndRender;

          SK_DXGI_BringRenderWindowToTop ();
        }
      }
    }

    first_frame = false;

    int interval = config.render.framerate.present_interval;

    if ( config.render.framerate.flip_discard && config.render.dxgi.allow_tearing )
    {
      DXGI_SWAP_CHAIN_DESC desc;
      if (SUCCEEDED (This->GetDesc (&desc)))
      {
        if (desc.Windowed) {
          Flags       |= DXGI_PRESENT_ALLOW_TEARING;
          SyncInterval = 0;
          interval     = 0;
        }
      }
    }

    int flags    = Flags;

    // Application preference
    if (interval == -1)
      interval = SyncInterval;

    if (bFlipMode)
    {
      flags = Flags | DXGI_PRESENT_RESTART;

      if (bWait)
        flags |= DXGI_PRESENT_DO_NOT_WAIT;
    }

    // Test first, then do (if test_present is true)
    hr = config.render.dxgi.test_present ? 
           Present_Original (This, 0, Flags | DXGI_PRESENT_TEST) :
           S_OK;

    bool can_present = SUCCEEDED (hr) || hr == DXGI_STATUS_OCCLUDED;

    if (! bFlipMode)
    {
      if (can_present)
      {
             SK_CEGUI_DrawD3D11 (This);
        hr = Present_Original   (This, interval, flags);
      }

      else
      {
        dll_log.Log ( L"[   DXGI   ] *** IDXGISwapChain::Present (...) "
                      L"returned non-S_OK (%s :: %s)",
                        SK_DescribeHRESULT (hr),
                          SUCCEEDED (hr) ? L"Success" :
                                           L"Fail" );

        if (FAILED (hr) && hr == DXGI_ERROR_DEVICE_REMOVED)
        {
          if (pDev != nullptr)
          {
            // D3D11 Device Removed, let's find out why...
            HRESULT hr_removed = pDev->GetDeviceRemovedReason ();

            dll_log.Log ( L"[   DXGI   ] (*) >> Reason For Removal: %s",
                           SK_DescribeHRESULT (hr_removed) );
          }
        }
      }
    }

    else
    {
      DXGI_PRESENT_PARAMETERS pparams;
      pparams.DirtyRectsCount = 0;
      pparams.pDirtyRects     = nullptr;
      pparams.pScrollOffset   = nullptr;
      pparams.pScrollRect     = nullptr;

      CComPtr <IDXGISwapChain1> pSwapChain1 = nullptr;

      if (SUCCEEDED (This->QueryInterface (IID_PPV_ARGS (&pSwapChain1))))
      {
        if (can_present)
        {
          SK_CEGUI_DrawD3D11 (This);

          // No overlays will work if we don't do this...
          /////if (config.osd.show) {
            hr =
              Present_Original (
                This,
                  0,
                    flags | DXGI_PRESENT_DO_NOT_SEQUENCE       | DXGI_PRESENT_DO_NOT_WAIT |
                            DXGI_PRESENT_STEREO_TEMPORARY_MONO | DXGI_PRESENT_USE_DURATION );

                  // ^^^ Deliberately invalid set of flags so that this call will fail, we just
                  //       want third-party overlays that don't hook Present1 to understand what
                  //         is going on.

            // Draw here to hide from Steam screenshots
            //SK_CEGUI_DrawD3D11 (This);

          /////}

          hr = Present1_Original (pSwapChain1, interval, flags | DXGI_PRESENT_DO_NOT_WAIT, &pparams);
        }
      }
    }

    if (bWait)
    {
      CComPtr <IDXGISwapChain2> pSwapChain2 = nullptr;

      if (SUCCEEDED (This->QueryInterface (IID_PPV_ARGS (&pSwapChain2))))
      {
        if (pSwapChain2 != nullptr)
        {
          HANDLE hWait = pSwapChain2->GetFrameLatencyWaitableObject ();

          WaitForSingleObjectEx ( hWait,
                                    500,//config.render.framerate.swapchain_wait,
                                      TRUE );
        }
      }
    }

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

    if (pDesc == nullptr && SUCCEEDED (hr))
    {
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

    if (SUCCEEDED (hr))
    {
      int removed_count = 0;

      if ( ! ( config.render.dxgi.res.min.isZero ()   &&
               config.render.dxgi.res.max.isZero () ) &&
           *pNumModes != 0 )
      {
        int last  = *pNumModes;
        int first = 0;

        std::set <int> coverage_min;
        std::set <int> coverage_max;

        if (! config.render.dxgi.res.min.isZero ())
        {
          // Restrict MIN (Sequential Scan: Last->First)
          for ( int i = *pNumModes - 1 ; i >= 0 ; --i )
          {
            if (SK_DXGI_RestrictResMin (WIDTH,  first, i,  coverage_min, pDesc) |
                SK_DXGI_RestrictResMin (HEIGHT, first, i,  coverage_min, pDesc))
              ++removed_count;
          }
        }

        if (! config.render.dxgi.res.max.isZero ())
        {
          // Restrict MAX (Sequential Scan: First->Last)
          for ( UINT i = 0 ; i < *pNumModes ; ++i )
          {
            if (SK_DXGI_RestrictResMax (WIDTH,  last, i, coverage_max, pDesc) |
                SK_DXGI_RestrictResMax (HEIGHT, last, i, coverage_max, pDesc))
              ++removed_count;
          }
        }
      }

      if (config.render.dxgi.scaling_mode != -1)
      {
        if (config.render.dxgi.scaling_mode != DXGI_MODE_SCALING_UNSPECIFIED)
        {
          for ( INT i = (INT)*pNumModes - 1 ; i >= 0 ; --i )
          {
            if ( pDesc [i].Scaling != DXGI_MODE_SCALING_UNSPECIFIED &&
                 pDesc [i].Scaling != config.render.dxgi.scaling_mode )
            {
              pDesc [i] = pDesc [i + 1];
              ++removed_count;
            }
          }
        }
      }

      if (pDesc != nullptr && pDescLocal == nullptr)
      {
        dll_log.Log ( L"[   DXGI   ]      >> %lu modes (%li removed)",
                        *pNumModes,
                          removed_count );
      }
    }

    if (pDescLocal != nullptr)
    {
      free (pDescLocal);
      pDescLocal = nullptr;
      pDesc      = nullptr;
    }

    return hr;
  }

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

    if ( config.render.framerate.refresh_rate != -1 &&
         mode_to_match.RefreshRate.Numerator  != config.render.framerate.refresh_rate )
    {
      dll_log.Log ( L"[   DXGI   ]  >> Refresh Override "
                    L"(Requested: %f, Using: %li)",
                      mode_to_match.RefreshRate.Denominator != 0 ?
                        (float)mode_to_match.RefreshRate.Numerator /
                        (float)mode_to_match.RefreshRate.Denominator :
                          std::numeric_limits <float>::quiet_NaN (),
                        config.render.framerate.refresh_rate
                  );

      mode_to_match.RefreshRate.Numerator   = config.render.framerate.refresh_rate;
      mode_to_match.RefreshRate.Denominator = 1;
    }

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

  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGIOutput_WaitForVBlank_Override ( IDXGIOutput    *This )
  {
//    dll_log.Log (L"[   DXGI   ] [!] IDXGIOutput::WaitForVBlank (...)");
    return WaitForVBlank_Original (This);
  }

  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_GetFullscreenState_Override ( IDXGISwapChain  *This,
                              _Out_opt_  BOOL            *pFullscreen,
                              _Out_opt_  IDXGIOutput    **ppTarget )
  {
    return GetFullscreenState_Original (This, pFullscreen, ppTarget);
  }

  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_SetFullscreenState_Override ( IDXGISwapChain *This,
                                         BOOL            Fullscreen,
                                         IDXGIOutput    *pTarget )
  {
    DXGI_LOG_CALL_I2 (L"IDXGISwapChain", L"SetFullscreenState", L"%s, %ph",
                      Fullscreen ? L"{ Fullscreen }" : L"{ Windowed }", pTarget);

    SK_D3D11_EndFrame ();

    InterlockedExchange (&__gui_reset, TRUE);

    if ( config.render.framerate.flip_discard && dxgi_caps.swapchain.allow_tearing ) {
      Fullscreen = FALSE;
      dll_log.Log ( L"[ DXGI 1.5 ]  >> Tearing Override:  Enable" );
      pTarget = nullptr;
    }

    HRESULT ret;
    DXGI_CALL (ret, SetFullscreenState_Original (This, Fullscreen, pTarget));

    //
    // Necessary provisions for Fullscreen Flip Mode
    //
    if (SUCCEEDED (ret))
    {
      if (bFlipMode)
        ResizeBuffers_Original (This, 0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

      DXGI_SWAP_CHAIN_DESC desc;
      if (SUCCEEDED (This->GetDesc (&desc)))
      {
        SK_SetWindowResX (desc.BufferDesc.Width);
        SK_SetWindowResY (desc.BufferDesc.Height);
      }

      SK_GetCurrentRenderBackend ().fullscreen_exclusive = Fullscreen;
    }

    return ret;
  }

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
    DXGI_LOG_CALL_I5 (L"IDXGISwapChain", L"ResizeBuffers", L"%lu,%lu,%lu,fmt=%lu,0x%08X",
                        BufferCount, Width, Height, NewFormat, SwapChainFlags);

    SK_D3D11_EndFrame ();

    if (       config.render.framerate.buffer_count != -1           &&
         (UINT)config.render.framerate.buffer_count !=  BufferCount &&
         BufferCount                          !=  0 )
    {
      BufferCount = config.render.framerate.buffer_count;
      dll_log.Log (L"[   DXGI   ]  >> Buffer Count Override: %lu buffers", BufferCount);
    }

    InterlockedExchange (&__gui_reset, TRUE);

    if (config.render.framerate.flip_discard && dxgi_caps.swapchain.allow_tearing) {
      SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
      dll_log.Log ( L"[ DXGI 1.5 ]  >> Tearing Option:  Enable" );
    }

    // TODO: Something if Fullscreen
    if (config.window.borderless && (! config.window.fullscreen))
    {
      if (! config.window.res.override.isZero ())
      {
        Width  = config.window.res.override.x;
        Height = config.window.res.override.y;
      }

      else
      {
        SK_DXGI_BorderCompensation (
          Width,
            Height
        );
      }
    }

    HRESULT ret;
    DXGI_CALL (ret, ResizeBuffers_Original (This, BufferCount, Width, Height, NewFormat, SwapChainFlags));

    if (SUCCEEDED (ret))
    {
      if (Width != 0 && Height != 0)
      {
        SK_SetWindowResX (Width);
        SK_SetWindowResY (Height);
      }

      SK_DXGI_HookPresent (This);
      MH_ApplyQueued      ();
    }

    return ret;
  }

  __declspec (noinline)
  HRESULT
  STDMETHODCALLTYPE
  DXGISwap_ResizeTarget_Override ( IDXGISwapChain *This,
                        _In_ const DXGI_MODE_DESC *pNewTargetParameters )
  {
    SK_D3D11_EndFrame ();

    if (pNewTargetParameters == nullptr) {
      HRESULT ret;
      DXGI_CALL (ret, ResizeTarget_Original (This, pNewTargetParameters));
      return ret;
    }

    DXGI_LOG_CALL_I6 (L"IDXGISwapChain", L"ResizeTarget", L"{ (%lux%lu@%3.1fHz), fmt=%lu, scaling=0x%02x, scanlines=0x%02x }",
                        pNewTargetParameters->Width, pNewTargetParameters->Height,
                        pNewTargetParameters->RefreshRate.Denominator != 0 ?
                          (float)pNewTargetParameters->RefreshRate.Numerator /
                          (float)pNewTargetParameters->RefreshRate.Denominator :
                            std::numeric_limits <float>::quiet_NaN (),
                        pNewTargetParameters->Format,
                        pNewTargetParameters->Scaling,
                        pNewTargetParameters->ScanlineOrdering );

    InterlockedExchange (&__gui_reset, TRUE);

    HRESULT ret;

    if ( config.window.borderless ||
         ( config.render.dxgi.scaling_mode != -1 &&
            pNewTargetParameters->Scaling  != 
              (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
                                  ||
         ( config.render.framerate.refresh_rate != -1 &&
             pNewTargetParameters->RefreshRate.Numerator !=
               config.render.framerate.refresh_rate )
      )
    {
      DXGI_MODE_DESC new_new_params =
        *pNewTargetParameters;

      if ( config.render.framerate.refresh_rate != -1 &&
           new_new_params.RefreshRate.Numerator != config.render.framerate.refresh_rate )
      {
        dll_log.Log ( L"[   DXGI   ]  >> Refresh Override "
                      L"(Requested: %f, Using: %li)",
                        new_new_params.RefreshRate.Denominator != 0 ?
                          (float)new_new_params.RefreshRate.Numerator /
                          (float)new_new_params.RefreshRate.Denominator :
                            std::numeric_limits <float>::quiet_NaN (),
                          config.render.framerate.refresh_rate
                    );

        new_new_params.RefreshRate.Numerator   = config.render.framerate.refresh_rate;
        new_new_params.RefreshRate.Denominator = 1;
      }

      if ( config.render.dxgi.scanline_order != -1 &&
            pNewTargetParameters->ScanlineOrdering  != 
              (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order )
      {
        dll_log.Log ( L"[   DXGI   ]  >> Scanline Override "
                      L"(Requested: %s, Using: %s)",
                        SK_DXGI_DescribeScanlineOrder (
                          pNewTargetParameters->ScanlineOrdering
                        ),
                          SK_DXGI_DescribeScanlineOrder (
                            (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order
                          )
                    );

        new_new_params.ScanlineOrdering =
          (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order;
      }

      if ( config.render.dxgi.scaling_mode != -1 &&
            pNewTargetParameters->Scaling  != 
              (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
      {
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
      }

      if ( (! config.window.fullscreen) &&
              config.window.borderless )
      {
        if (! config.window.res.override.isZero ())
        {
          new_new_params.Width  = config.window.res.override.x;
          new_new_params.Height = config.window.res.override.y;
        } else {
          SK_DXGI_BorderCompensation (new_new_params.Width, new_new_params.Height);
        }
      }

      const DXGI_MODE_DESC* pNewNewTargetParameters =
        &new_new_params;

      DXGI_CALL (ret, ResizeTarget_Original (This, pNewNewTargetParameters));

      if (SUCCEEDED (ret))
      {
        if (pNewNewTargetParameters->Width != 0 && pNewNewTargetParameters->Height != 0)
        {
          SK_SetWindowResX (pNewNewTargetParameters->Width);
          SK_SetWindowResY (pNewNewTargetParameters->Height);
        }
      }
    }

    else
    {
      DXGI_CALL (ret, ResizeTarget_Original (This, pNewTargetParameters));

      if (SUCCEEDED (ret))
      {
        if (pNewTargetParameters->Width != 0 && pNewTargetParameters->Height != 0)
        {
          SK_SetWindowResX (pNewTargetParameters->Width);
          SK_SetWindowResY (pNewTargetParameters->Height);
        }
      }
    }

    if (SUCCEEDED (ret))
    {
      SK_DXGI_HookPresent (This);
      MH_ApplyQueued      ();
    }

    return ret;
  }

void
SK_DXGI_CreateSwapChain_PreInit ( _Inout_opt_ DXGI_SWAP_CHAIN_DESC            *pDesc,
                                  _Inout_opt_ DXGI_SWAP_CHAIN_DESC1           *pDesc1,
                                  _Inout_opt_ HWND&                            hWnd,
                                  _Inout_opt_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc )
{
  DXGI_SWAP_CHAIN_DESC stub_desc; // Stores common attributes between DESC and DESC1
  bool                 translated = false;

  if (pDesc1 != nullptr)
  {
    if (pDesc == nullptr)
    {
      pDesc = &stub_desc;

      stub_desc.BufferCount                        = pDesc1->BufferCount;
      stub_desc.BufferUsage                        = pDesc1->BufferUsage;
      stub_desc.Flags                              = pDesc1->Flags;
      stub_desc.SwapEffect                         = pDesc1->SwapEffect;
      stub_desc.SampleDesc.Count                   = pDesc1->SampleDesc.Count;
      stub_desc.SampleDesc.Quality                 = pDesc1->SampleDesc.Quality;
      stub_desc.BufferDesc.Format                  = pDesc1->Format;
      stub_desc.BufferDesc.Height                  = pDesc1->Height;
      stub_desc.BufferDesc.Width                   = pDesc1->Width;
      stub_desc.OutputWindow                       = hWnd;

      if (pFullscreenDesc != nullptr)
      {
        stub_desc.Windowed                           = pFullscreenDesc->Windowed;
        stub_desc.BufferDesc.RefreshRate.Denominator = pFullscreenDesc->RefreshRate.Denominator;
        stub_desc.BufferDesc.RefreshRate.Numerator   = pFullscreenDesc->RefreshRate.Numerator;
        stub_desc.BufferDesc.Scaling                 = pFullscreenDesc->Scaling;
        stub_desc.BufferDesc.ScanlineOrdering        = pFullscreenDesc->ScanlineOrdering;
      }

      else
      {
        stub_desc.Windowed = TRUE;
      }

      // Need to take this stuff and put it back in the appropriate structures before returning :)
      translated = true;
    }
  }


  static bool bIsLegoUndercover = 
    (StrStrIW (SK_GetHostApp (), L"LEGOLCUR_DX11.exe") != nullptr);

  if (bIsLegoUndercover)  pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;



  if (pDesc != nullptr)
  {
    if (pDesc->Windowed && config.window.borderless && (! config.window.fullscreen))
    {
      if (! config.window.res.override.isZero ())
      {
        pDesc->BufferDesc.Width  = config.window.res.override.x;
        pDesc->BufferDesc.Height = config.window.res.override.y;
      }

      else
      {
        SK_DXGI_BorderCompensation (
          pDesc->BufferDesc.Width,
            pDesc->BufferDesc.Height
        );
      }
    }

    dll_log.LogEx ( true,
      L"[   DXGI   ]  SwapChain: (%lux%lu @ %4.1f Hz - Scaling: %s - Scanlines: %s) - {%s}"
      L" [%lu Buffers] :: Flags=0x%04X, SwapEffect: %s\n",
      pDesc->BufferDesc.Width,
      pDesc->BufferDesc.Height,
      pDesc->BufferDesc.RefreshRate.Denominator != 0 ?
        (float)pDesc->BufferDesc.RefreshRate.Numerator /
        (float)pDesc->BufferDesc.RefreshRate.Denominator :
          std::numeric_limits <float>::quiet_NaN (),
      pDesc->BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED ?
        L"Unspecified" :
        pDesc->BufferDesc.Scaling == DXGI_MODE_SCALING_CENTERED ?
          L"Centered" :
          L"Stretched",
      pDesc->BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED ?
        L"Unspecified" :
        pDesc->BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE ?
          L"Progressive" :
          pDesc->BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST ?
            L"Interlaced Even" :
            L"Interlaced Odd",
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

    // Set things up to make the swap chain Alt+Enter friendly
    if (bAlwaysAllowFullscreen)
    {
      pDesc->Flags                             |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      pDesc->Windowed                           = true;
      pDesc->BufferDesc.RefreshRate.Denominator = 0;
      pDesc->BufferDesc.RefreshRate.Numerator   = 0;
    }

    if (! bFlipMode)
      bFlipMode =
        ( dxgi_caps.present.flip_sequential && (
          ( ! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe")) ||
            SK_DS3_UseFlipMode ()        ) );

    if (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe"))
    {
      if (bFlipMode)
          bFlipMode = (! SK_FO4_IsFullscreen ()) && SK_FO4_UseFlipMode ();
    }

    else
    {
      // If forcing flip-model, then force multisampling off
      if (config.render.framerate.flip_discard)
      {
        bFlipMode = dxgi_caps.present.flip_sequential;
        pDesc->SampleDesc.Count = 1; pDesc->SampleDesc.Quality = 0;
      }
    }

    if (       config.render.framerate.buffer_count != -1                  &&
         (UINT)config.render.framerate.buffer_count !=  pDesc->BufferCount &&
         pDesc->BufferCount                         !=  0 )
    {
      pDesc->BufferCount = config.render.framerate.buffer_count;
      dll_log.Log (L"[   DXGI   ]  >> Buffer Count Override: %lu buffers", pDesc->BufferCount);
    }

    if ( config.render.framerate.flip_discard &&  dxgi_caps.swapchain.allow_tearing )
    {
      pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
      dll_log.Log ( L"[ DXGI 1.5 ]  >> Tearing Option:  Enable" );
      pDesc->Windowed = TRUE;
    }

    if ( config.render.dxgi.scaling_mode != -1 &&
          pDesc->BufferDesc.Scaling      !=
            (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
    {
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

    if ( config.render.dxgi.scanline_order != -1 &&
          pDesc->BufferDesc.ScanlineOrdering      !=
            (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order )
    {
      dll_log.Log ( L"[   DXGI   ]  >> Scanline Override "
                    L"(Requested: %s, Using: %s)",
                      SK_DXGI_DescribeScanlineOrder (
                        pDesc->BufferDesc.ScanlineOrdering
                      ),
                        SK_DXGI_DescribeScanlineOrder (
                          (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order
                        )
                  );

      pDesc->BufferDesc.ScanlineOrdering =
        (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order;
    }

    if ( config.render.framerate.refresh_rate != -1 &&
         pDesc->BufferDesc.RefreshRate.Numerator != config.render.framerate.refresh_rate )
    {
      dll_log.Log ( L"[   DXGI   ]  >> Refresh Override "
                    L"(Requested: %f, Using: %li)",
                 pDesc->BufferDesc.RefreshRate.Denominator != 0 ?
                   (float)pDesc->BufferDesc.RefreshRate.Numerator /
                   (float)pDesc->BufferDesc.RefreshRate.Denominator :
                     std::numeric_limits <float>::quiet_NaN (),
                        config.render.framerate.refresh_rate
                  );

      pDesc->BufferDesc.RefreshRate.Numerator   = config.render.framerate.refresh_rate;
      pDesc->BufferDesc.RefreshRate.Denominator = 1;
    }

    bWait = bFlipMode && dxgi_caps.present.waitable;

    // We cannot change the swapchain parameters if this is used...
    bWait = bWait && config.render.framerate.swapchain_wait > 0;

    if (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe"))
    {
      if (SK_DS3_IsBorderless ())
        pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    }

    if (bFlipMode)
    {
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
    if (bFlipMode && bWait)
    {
      pDesc->Flags |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
      pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    }
  }

  dll_log.Log ( L"[ DXGI 1.2 ] >> Using %s Presentation Model  [Waitable: %s - %li ms]",
                 bFlipMode ? L"Flip" : L"Traditional",
                   bWait ? L"Yes" : L"No",
                     bWait ? config.render.framerate.swapchain_wait : 0 );


  if (translated)
  {
    pDesc1->BufferCount                      = pDesc->BufferCount;
    pDesc1->BufferUsage                      = pDesc->BufferUsage;
    pDesc1->Flags                            = pDesc->Flags;
    pDesc1->SwapEffect                       = pDesc->SwapEffect;
    pDesc1->SampleDesc.Count                 = pDesc->SampleDesc.Count;
    pDesc1->SampleDesc.Quality               = pDesc->SampleDesc.Quality;
    pDesc1->Format                           = pDesc->BufferDesc.Format;
    pDesc1->Height                           = pDesc->BufferDesc.Height;
    pDesc1->Width                            = pDesc->BufferDesc.Width;

    hWnd                                     = pDesc->OutputWindow;

    if (pFullscreenDesc != nullptr)
    {
      pFullscreenDesc->Windowed                = pDesc->Windowed;
      pFullscreenDesc->RefreshRate.Denominator = pDesc->BufferDesc.RefreshRate.Denominator;
      pFullscreenDesc->RefreshRate.Numerator   = pDesc->BufferDesc.RefreshRate.Numerator;
      pFullscreenDesc->Scaling                 = pDesc->BufferDesc.Scaling;
      pFullscreenDesc->ScanlineOrdering        = pDesc->BufferDesc.ScanlineOrdering;
    }
  }

  //game_window.hWnd = pDesc->OutputWindow;
}

void
SK_DXGI_CreateSwapChain_PostInit ( _In_  IUnknown              *pDevice,
                                   _In_  DXGI_SWAP_CHAIN_DESC  *pDesc,
                                   _In_  IDXGISwapChain       **ppSwapChain )
{
  SK_SetWindowResX (pDesc->BufferDesc.Width);
  SK_SetWindowResY (pDesc->BufferDesc.Height);

  SK_DXGI_HookPresent (*ppSwapChain);
  MH_ApplyQueued      ();

  //if (bFlipMode || bWait)
    //DXGISwap_ResizeBuffers_Override (*ppSwapChain, config.render.framerate.buffer_count, pDesc->BufferDesc.Width, pDesc->BufferDesc.Height, pDesc->BufferDesc.Format, pDesc->Flags);

  const uint32_t max_latency = config.render.framerate.pre_render_limit;

  CComPtr <IDXGISwapChain2> pSwapChain2 = nullptr;

  if ( bFlipMode && bWait &&
       SUCCEEDED ( (*ppSwapChain)->QueryInterface (IID_PPV_ARGS (&pSwapChain2)) )
      )
  {
    if (max_latency < 16)
    {
      dll_log.Log (L"[   DXGI   ] Setting Swapchain Frame Latency: %lu", max_latency);
      pSwapChain2->SetMaximumFrameLatency (max_latency);
    }

    HANDLE hWait = pSwapChain2->GetFrameLatencyWaitableObject ();

    WaitForSingleObjectEx ( hWait,
                              500,//config.render.framerate.swapchain_wait,
                                TRUE );
  }

  {
    if (max_latency != -1)
    {
      CComPtr <IDXGIDevice1> pDevice1 = nullptr;

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

  CComPtr <ID3D11Device> pDev = nullptr;

  if (SUCCEEDED ( pDevice->QueryInterface ( IID_PPV_ARGS (&pDev) )
                )
     )
  {
    // Dangerous to hold a reference to this don't you think?!
    g_pD3D11Dev = pDev;
  }
}

void
SK_DXGI_CreateSwapChain1_PostInit ( _In_     IUnknown                         *pDevice,
                                    _In_     DXGI_SWAP_CHAIN_DESC1            *pDesc1,
                                    _In_opt_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC  *pFullscreenDesc,
                                    _In_     IDXGISwapChain1                 **ppSwapChain1 )
{
  // ONLY AS COMPLETE AS NEEDED, if new code is added to PostInit, this will probably need changing.
  DXGI_SWAP_CHAIN_DESC desc;

  desc.BufferDesc.Width  = pDesc1->Width;
  desc.BufferDesc.Height = pDesc1->Height;

  CComPtr <IDXGISwapChain> pSwapChain = nullptr;

  (*ppSwapChain1)->QueryInterface <IDXGISwapChain> (&pSwapChain);

  return SK_DXGI_CreateSwapChain_PostInit ( pDevice, &desc, &pSwapChain );
}

  HRESULT
  STDMETHODCALLTYPE
  DXGIFactory_CreateSwapChain_Override (             IDXGIFactory          *This,
                                         _In_        IUnknown              *pDevice,
                                         _In_  const DXGI_SWAP_CHAIN_DESC  *pDesc,
                                         _Out_       IDXGISwapChain       **ppSwapChain )
  {
    std::wstring iname = SK_GetDXGIFactoryInterface (This);

    DXGI_LOG_CALL_I3 (iname.c_str (), L"CreateSwapChain", L"%ph, %ph, %ph",
                    pDevice, pDesc, ppSwapChain);


    DXGI_SWAP_CHAIN_DESC new_desc =
      pDesc != nullptr ?
        *pDesc :
          DXGI_SWAP_CHAIN_DESC { };

    if (pDesc != nullptr)
      SK_DXGI_CreateSwapChain_PreInit (&new_desc, nullptr, new_desc.OutputWindow, nullptr);


    HRESULT ret;
    DXGI_CALL (ret, CreateSwapChain_Original (This, pDevice, &new_desc, ppSwapChain));


    if ( SUCCEEDED (ret)         &&
         ppSwapChain  != nullptr &&
       (*ppSwapChain) != nullptr )
    {
      SK_DXGI_CreateSwapChain_PostInit (pDevice, &new_desc, ppSwapChain);
    }

    return ret;
  }


  HRESULT
  STDMETHODCALLTYPE
  DXGIFactory2_CreateSwapChainForCoreWindow_Override ( IDXGIFactory2             *This,
                                            _In_       IUnknown                  *pDevice,
                                            _In_       IUnknown                  *pWindow,
                                            _In_ const DXGI_SWAP_CHAIN_DESC1     *pDesc,
                                        _In_opt_       IDXGIOutput               *pRestrictToOutput,
                                           _Out_       IDXGISwapChain1          **ppSwapChain )
  {
    std::wstring iname = SK_GetDXGIFactoryInterface (This);

    // Wrong prototype, but who cares right now? :P
    DXGI_LOG_CALL_I3 (iname.c_str (), L"CreateSwapChainForCoreWindow", L"%ph, %ph, %ph",
                      pDevice, pDesc, ppSwapChain);

    HRESULT ret;

    DXGI_SWAP_CHAIN_DESC1 new_desc1 =
      pDesc != nullptr ?
        *pDesc :
          DXGI_SWAP_CHAIN_DESC1 { };

    HWND hWnd = 0;
    SK_DXGI_CreateSwapChain_PreInit (nullptr, &new_desc1, hWnd, nullptr);


    DXGI_CALL( ret, CreateSwapChainForCoreWindow_Original (
                      This,
                        pDevice,
                          pWindow,
                            &new_desc1,
                              pRestrictToOutput,
                                ppSwapChain ) );

    if ( SUCCEEDED (ret)      &&
         ppSwapChain  != NULL &&
       (*ppSwapChain) != NULL )
    {
      SK_DXGI_CreateSwapChain1_PostInit (pDevice, &new_desc1, nullptr, ppSwapChain);
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE
  DXGIFactory2_CreateSwapChainForHwnd_Override ( IDXGIFactory2                   *This,
                                      _In_       IUnknown                        *pDevice,
                                      _In_       HWND                             hWnd,
                                      _In_ const DXGI_SWAP_CHAIN_DESC1           *pDesc,
                                  _In_opt_       DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
                                  _In_opt_       IDXGIOutput                     *pRestrictToOutput,
                                     _Out_       IDXGISwapChain1                 **ppSwapChain )
  {
    std::wstring iname = SK_GetDXGIFactoryInterface (This);

    // Wrong prototype, but who cares right now? :P
    DXGI_LOG_CALL_I3 (iname.c_str (), L"CreateSwapChainForHwnd", L"%ph, %ph, %ph",
                      pDevice, pDesc, ppSwapChain);

    HRESULT ret;

    DXGI_SWAP_CHAIN_DESC1           new_desc1           = *pDesc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC new_fullscreen_desc =
      pFullscreenDesc ? *pFullscreenDesc :
                         DXGI_SWAP_CHAIN_FULLSCREEN_DESC { };

    assert (pDesc != nullptr);

    SK_DXGI_CreateSwapChain_PreInit (nullptr, &new_desc1, hWnd, &new_fullscreen_desc);

    DXGI_CALL (ret, CreateSwapChainForHwnd_Original (This, pDevice, hWnd, &new_desc1, &new_fullscreen_desc, pRestrictToOutput, ppSwapChain));

    if ( SUCCEEDED (ret)      &&
         ppSwapChain  != NULL &&
       (*ppSwapChain) != NULL )
    {
      SK_DXGI_CreateSwapChain1_PostInit (pDevice, &new_desc1, &new_fullscreen_desc, ppSwapChain);
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE
  DXGIFactory2_CreateSwapChainForComposition_Override ( IDXGIFactory2          *This,                           
                                         _In_           IUnknown               *pDevice,
                                         _In_     const DXGI_SWAP_CHAIN_DESC1  *pDesc,
                                         _In_opt_       IDXGIOutput            *pRestrictToOutput,
                                         _Outptr_       IDXGISwapChain1       **ppSwapChain )
  {
    std::wstring iname = SK_GetDXGIFactoryInterface (This);

    // Wrong prototype, but who cares right now? :P
    DXGI_LOG_CALL_I3 (iname.c_str (), L"CreateSwapChainForComposition", L"%ph, %ph, %ph",
                      pDevice, pDesc, ppSwapChain);

    HRESULT ret;

    DXGI_SWAP_CHAIN_DESC1           new_desc1           = *pDesc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC new_fullscreen_desc = {    };

    assert (pDesc != nullptr);

    HWND hWnd = 0;
    SK_DXGI_CreateSwapChain_PreInit (nullptr, &new_desc1, hWnd, nullptr);


    DXGI_CALL (ret, CreateSwapChainForComposition_Original (This, pDevice, &new_desc1, pRestrictToOutput, ppSwapChain));

    if ( SUCCEEDED (ret)      &&
         ppSwapChain  != NULL &&
       (*ppSwapChain) != NULL )
    {
      SK_DXGI_CreateSwapChain1_PostInit (pDevice, &new_desc1, &new_fullscreen_desc, ppSwapChain);
    }

    return ret;
  }

  typedef enum skUndesirableVendors {
    Microsoft = 0x1414,
    Intel     = 0x8086
  } Vendors;

  HRESULT
  STDMETHODCALLTYPE CreateDXGIFactory (REFIID   riid,
                                 _Out_ void   **ppFactory);

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
    UNREFERENCED_PARAMETER (user);

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
    {
      WaitForInitDXGI ();

      if (EnumAdapters_Original == nullptr)
        return;
    }

    IDXGIAdapter* pGameAdapter     = (*ppAdapter);
    IDXGIAdapter* pOverrideAdapter = nullptr;
    IDXGIFactory* pFactory         = nullptr;

    HRESULT res;

    if ((*ppAdapter) == nullptr)
      res = E_FAIL;
    else
      res = (*ppAdapter)->GetParent (IID_PPV_ARGS (&pFactory));

    if (FAILED (res))
    {
      if (SK_DXGI_use_factory1)
        res = CreateDXGIFactory1_Import (__uuidof (IDXGIFactory1), (void **)&pFactory);
      else
        res = CreateDXGIFactory_Import  (__uuidof (IDXGIFactory),  (void **)&pFactory);
    }

    if (SUCCEEDED (res))
    {
      if (pFactory != nullptr)
      {
        if ((*ppAdapter) == nullptr)
          EnumAdapters_Original (pFactory, 0, &pGameAdapter);

        DXGI_ADAPTER_DESC game_desc { 0 };

        if (pGameAdapter != nullptr)
        {
          *ppAdapter  = pGameAdapter;
          *DriverType = D3D_DRIVER_TYPE_UNKNOWN;

          GetDesc_Original (pGameAdapter, &game_desc);
        }

        if ( SK_DXGI_preferred_adapter != -1 &&
             SUCCEEDED (EnumAdapters_Original (pFactory, SK_DXGI_preferred_adapter, &pOverrideAdapter)) )
        {
          DXGI_ADAPTER_DESC override_desc;
          GetDesc_Original (pOverrideAdapter, &override_desc);

          if ( game_desc.VendorId     == Vendors::Intel     &&
               override_desc.VendorId != Vendors::Microsoft &&
               override_desc.VendorId != Vendors::Intel )
          {
            dll_log.Log ( L"[   DXGI   ] !!! DXGI Adapter Override: (Using '%s' instead of '%s') !!!",
                          override_desc.Description, game_desc.Description );

            *ppAdapter = pOverrideAdapter;
            pGameAdapter->Release ();
          }

          else
          {
            dll_log.Log ( L"[   DXGI   ] !!! DXGI Adapter Override: (Tried '%s' instead of '%s') !!!",
                          override_desc.Description, game_desc.Description );
            //SK_DXGI_preferred_adapter = -1;
            pOverrideAdapter->Release ();
          }
        }

        else
        {
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
#if 0
    std::wstring iname = SK_GetDXGIAdapterInterface (This);

    DXGI_LOG_CALL_I2 (iname.c_str (), L"GetDesc2", L"%ph, %ph", This, pDesc);

    HRESULT    ret;
    DXGI_CALL (ret, GetDesc2_Original (This, pDesc));
#else
    HRESULT ret = GetDesc2_Original (This, pDesc);
#endif

    //// OVERRIDE VRAM NUMBER
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0)
    {
      dll_log.LogEx ( true,
        L" <> GetDesc2_Override: Looking for matching NVAPI GPU for %s...: ",
        pDesc->Description );

      DXGI_ADAPTER_DESC* match =
        sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

      if (match != NULL)
      {
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
#if 0
    std::wstring iname = SK_GetDXGIAdapterInterface (This);

    DXGI_LOG_CALL_I2 (iname.c_str (), L"GetDesc1", L"%ph, %ph", This, pDesc);

    HRESULT    ret;
    DXGI_CALL (ret, GetDesc1_Original (This, pDesc));
#else
    HRESULT ret = GetDesc1_Original (This, pDesc);
#endif

    //// OVERRIDE VRAM NUMBER
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0)
    {
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
#if 0
    std::wstring iname = SK_GetDXGIAdapterInterface (This);

    DXGI_LOG_CALL_I2 (iname.c_str (), L"GetDesc",L"%ph, %ph", This, pDesc);

    HRESULT    ret;
    DXGI_CALL (ret, GetDesc_Original (This, pDesc));
#else
    HRESULT ret = GetDesc_Original (This, pDesc);
#endif

    //// OVERRIDE VRAM NUMBER
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0)
    {
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

    if (config.system.log_level >= 1)
    {
      dll_log.Log ( L"[   DXGI   ] Dedicated Video: %zu MiB, Dedicated System: %zu MiB, Shared System: %zu MiB",
                      pDesc->DedicatedVideoMemory    >> 20UL,
                        pDesc->DedicatedSystemMemory >> 20UL,
                          pDesc->SharedSystemMemory  >> 20UL );
    }

    if ( (! lstrcmpW (SK_GetHostApp (),   L"Fallout4.exe") ) &&
                   SK_GetCallerName () == L"Fallout4.exe"  )
    {
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
    int iver = SK_GetDXGIAdapterInterfaceVer (*ppAdapter);

    DXGI_ADAPTER_DESC desc = { 0 };

    switch (iver)
    {
      default:
      case 2:
      {
        if (! GetDesc2_Original)
        {
          CComPtr <IDXGIAdapter2> pAdapter2 = nullptr;

          if (SUCCEEDED ((*ppAdapter)->QueryInterface (IID_PPV_ARGS (&pAdapter2)))) {

            DXGI_VIRTUAL_HOOK (ppAdapter, 11, "(*pAdapter2)->GetDesc2",
              GetDesc2_Override, GetDesc2_Original, GetDesc2_pfn);
          }
        }
      }

      case 1:
      {
        if (! GetDesc1_Original)
        {
          CComPtr <IDXGIAdapter1> pAdapter1 = nullptr;

          if (SUCCEEDED ((*ppAdapter)->QueryInterface (IID_PPV_ARGS (&pAdapter1)))) {
            DXGI_VIRTUAL_HOOK (&pAdapter1, 10, "(*pAdapter1)->GetDesc1",
              GetDesc1_Override, GetDesc1_Original, GetDesc1_pfn);
          }
        }
      }

      case 0:
      {
        if (! GetDesc_Original)
        {
          DXGI_VIRTUAL_HOOK (ppAdapter, 8, "(*ppAdapter)->GetDesc",
            GetDesc_Override, GetDesc_Original, GetDesc_pfn);
        }

        if (GetDesc_Original)
          GetDesc_Original (*ppAdapter, &desc);
      }
    }

    // Logic to skip Intel and Microsoft adapters and return only AMD / NV
    if (! lstrlenW (desc.Description))
      dll_log.LogEx (false, L" >> Assertion filed: Zero-length adapter name!\n");

#ifdef SKIP_INTEL
    if ((desc.VendorId == Microsoft || desc.VendorId == Intel) && Adapter == 0) {
#else
    if (false)
    {
#endif
      // We need to release the reference we were just handed before
      //   skipping it.
      (*ppAdapter)->Release ();

      dll_log.LogEx (false,
        L"[   DXGI   ] >> (Host Application Tried To Enum Intel or Microsoft Adapter "
        L"as Adapter 0) -- Skipping Adapter '%s' <<\n", desc.Description);

      return (pFunc (This, Adapter + 1, ppAdapter));
    }

    dll_log.LogEx(true,L"[   DXGI   ]  @ Returned Adapter %lu: '%s' (LUID: %08X:%08X)",
      Adapter,
        desc.Description,
          desc.AdapterLuid.HighPart,
            desc.AdapterLuid.LowPart );

    //
    // Windows 8 has a software implementation, which we can detect.
    //
    CComPtr <IDXGIAdapter1> pAdapter1 = nullptr;

    HRESULT hr =
      (*ppAdapter)->QueryInterface (IID_PPV_ARGS (&pAdapter1));

    if (SUCCEEDED (hr))
    {
      DXGI_ADAPTER_DESC1 desc1;

      if (SUCCEEDED (GetDesc1_Original (pAdapter1, &desc1)))
      {
#define DXGI_ADAPTER_FLAG_REMOTE   0x1
#define DXGI_ADAPTER_FLAG_SOFTWARE 0x2
        if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
          dll_log.LogEx (false, L" <Software>");
        else
          dll_log.LogEx (false, L" <Hardware>");
        if (desc1.Flags & DXGI_ADAPTER_FLAG_REMOTE)
          dll_log.LogEx (false, L" [Remote]");
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

    DXGI_LOG_CALL_I3 (iname.c_str (), L"EnumAdapters1", L"%ph, %u, %ph",
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
                                  (EnumAdapters_pfn)EnumAdapters1_Original);
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE EnumAdapters_Override (IDXGIFactory  *This,
                                           UINT           Adapter,
                                    _Out_  IDXGIAdapter **ppAdapter)
  {
    std::wstring iname = SK_GetDXGIFactoryInterface    (This);

    DXGI_LOG_CALL_I3 (iname.c_str (), L"EnumAdapters", L"%ph, %u, %ph",
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

    if (SUCCEEDED (ret) && ppAdapter != nullptr && (*ppAdapter) != nullptr)
    {
      return EnumAdapters_Common (This, Adapter, ppAdapter,
                                  (EnumAdapters_pfn)EnumAdapters_Original);
    }

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE CreateDXGIFactory (REFIID   riid,
                                 _Out_ void   **ppFactory)
  {
    // For DXGI compliance, do not mix-and-match
    if (SK_DXGI_use_factory1)
      return CreateDXGIFactory1 (riid, ppFactory);

    SK_DXGI_factory_init = true;

    std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
    int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

    UNREFERENCED_PARAMETER (iver);

    DXGI_LOG_CALL_2 (L"CreateDXGIFactory", L"%s, %ph",
      iname.c_str (), ppFactory);

    SK_BootDXGI ();

    HRESULT ret;
    DXGI_CALL (ret, CreateDXGIFactory_Import (riid, ppFactory));

    return ret;
  }

  HRESULT
  STDMETHODCALLTYPE CreateDXGIFactory1 (REFIID   riid,
                                  _Out_ void   **ppFactory)
  {
    // For DXGI compliance, do not mix-and-match
    if ((! SK_DXGI_use_factory1) && (SK_DXGI_factory_init))
      return CreateDXGIFactory (riid, ppFactory);

    SK_DXGI_use_factory1 = true;
    SK_DXGI_factory_init = true;

    std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
    int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

    UNREFERENCED_PARAMETER (iver);

    DXGI_LOG_CALL_2 (L"CreateDXGIFactory1", L"%s, %ph",
      iname.c_str (), ppFactory);

    SK_BootDXGI ();

    // Windows Vista does not have this function -- wrap it with CreateDXGIFactory
    if (CreateDXGIFactory1_Import == nullptr) {
      dll_log.Log (L"[   DXGI   ]  >> Falling back to CreateDXGIFactory on Vista...");
      return CreateDXGIFactory (riid, ppFactory);
    }

    HRESULT    ret;
    DXGI_CALL (ret, CreateDXGIFactory1_Import (riid, ppFactory));
    return     ret;
  }

  HRESULT
  STDMETHODCALLTYPE CreateDXGIFactory2 (UINT     Flags,
                                        REFIID   riid,
                                  _Out_ void   **ppFactory)
  {
    SK_DXGI_use_factory1 = true;
    SK_DXGI_factory_init = true;

    std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
    int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

    UNREFERENCED_PARAMETER (iver);

    DXGI_LOG_CALL_3 (L"CreateDXGIFactory2", L"0x%04X, %s, %ph",
      Flags, iname.c_str (), ppFactory);

    SK_BootDXGI ();

    // Windows 7 does not have this function -- wrap it with CreateDXGIFactory1
    if (CreateDXGIFactory2_Import == nullptr) {
      dll_log.Log (L"[   DXGI   ]  >> Falling back to CreateDXGIFactory1 on Vista/7...");
      return CreateDXGIFactory1 (riid, ppFactory);
    }

    HRESULT    ret;
    DXGI_CALL (ret, CreateDXGIFactory2_Import (Flags, riid, ppFactory));
    return     ret;
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

// SAL notation in Win32 API docs is wrong
typedef LONG (WINAPI *ChangeDisplaySettingsA_pfn)(
  _In_opt_ DEVMODEA *lpDevMode,
  _In_     DWORD     dwFlags
);
ChangeDisplaySettingsA_pfn ChangeDisplaySettingsA_Original = nullptr;

LONG
WINAPI
ChangeDisplaySettingsA_Detour (
  _In_opt_ DEVMODEA *lpDevMode,
  _In_     DWORD     dwFlags )
{
  static bool called = false;

  DEVMODEW dev_mode;
  dev_mode.dmSize = sizeof (DEVMODEW);

  EnumDisplaySettings (nullptr, 0, &dev_mode);

  if (dwFlags != CDS_TEST)
  {
    if (called)
      ChangeDisplaySettingsA_Original (NULL, CDS_RESET);

    called = true;

    return ChangeDisplaySettingsA_Original (lpDevMode, CDS_FULLSCREEN);
  }

  else
    return ChangeDisplaySettingsA_Original (lpDevMode, dwFlags);
}

typedef void (WINAPI *finish_pfn)(void);

void
WINAPI
SK_HookDXGI (void)
{
  static volatile ULONG hooked = FALSE;

  if (InterlockedCompareExchange (&hooked, TRUE, FALSE))
    return;

  if (! config.apis.dxgi.d3d11.hook)
    config.apis.dxgi.d3d12.hook = false;

  if (! config.apis.dxgi.d3d11.hook)
    return;

  HMODULE hBackend = 
    (SK_GetDLLRole () & DLL_ROLE::DXGI) ? backend_dll :
                                            GetModuleHandle (L"dxgi.dll");


  dll_log.Log (L"[   DXGI   ] Importing CreateDXGIFactory{1|2}");
  dll_log.Log (L"[   DXGI   ] ================================");


  if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dxgi.dll"))
  {
    dll_log.Log (L"[ DXGI 1.0 ]   CreateDXGIFactory:  %ph",
      (CreateDXGIFactory_Import =  \
        (CreateDXGIFactory_pfn)GetProcAddress (hBackend, "CreateDXGIFactory")));
    dll_log.Log (L"[ DXGI 1.1 ]   CreateDXGIFactory1: %ph",
      (CreateDXGIFactory1_Import = \
        (CreateDXGIFactory1_pfn)GetProcAddress (hBackend, "CreateDXGIFactory1")));
    dll_log.Log (L"[ DXGI 1.3 ]   CreateDXGIFactory2: %ph",
      (CreateDXGIFactory2_Import = \
        (CreateDXGIFactory2_pfn)GetProcAddress (hBackend, "CreateDXGIFactory2")));
  }

  else
  {
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

    dll_log.Log (L"[ DXGI 1.0 ]   CreateDXGIFactory:  %ph  %s",
      (CreateDXGIFactory_Import),
        (CreateDXGIFactory_Import ? L"{ Hooked }" : L"" ) );
    dll_log.Log (L"[ DXGI 1.1 ]   CreateDXGIFactory1: %ph  %s",
      (CreateDXGIFactory1_Import),
        (CreateDXGIFactory1_Import ? L"{ Hooked }" : L"" ) );
    dll_log.Log (L"[ DXGI 1.3 ]   CreateDXGIFactory2: %ph  %s",
      (CreateDXGIFactory2_Import),
        (CreateDXGIFactory2_Import ? L"{ Hooked }" : L"" ) );

    MH_ApplyQueued ();
  }

  if (CreateDXGIFactory1_Import != nullptr)
  {
    SK_DXGI_use_factory1 = true;
    SK_DXGI_factory_init = true;
  }

  SK_D3D11_Init         ();
  SK_D3D11_InitTextures ();
  SK_D3D12_Init         ();

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
}

static std::queue <DWORD> old_threads;

void
WINAPI
dxgi_init_callback (finish_pfn finish)
{
  if (! SK_IsHostAppSKIM ())
  {
    SK_BootDXGI ();

    while (! InterlockedCompareExchange (&__dxgi_ready, FALSE, FALSE))
      Sleep (100UL);
  }

  finish ();
}


bool
SK::DXGI::Startup (void)
{
  bool ret = SK_StartupCore (L"dxgi", dxgi_init_callback);

  return ret;
}

extern "C" bool WINAPI SK_DS3_ShutdownPlugin (const wchar_t* backend);


void
SK_DXGI_HookPresent (IDXGISwapChain* pSwapChain)
{
  static LPVOID vftable_8  = nullptr;
  static LPVOID vftable_22 = nullptr;

  void** vftable = *(void***)*&pSwapChain;

  if (Present_Original != nullptr)
  {
    //dll_log.Log (L"Rehooking IDXGISwapChain::Present (...)");

#if 0
    if (MH_OK == SK_RemoveHook (vftable [8]))
      Present_Original = nullptr;
    else {
      dll_log.Log ( L"[   DXGI   ] Altered vftable detected, rehooking "
                    L"IDXGISwapChain::Present (...)!" );
      if (MH_OK == SK_RemoveHook (vftable_8))
        Present_Original = nullptr;
    }
#endif
  }

  else
  {
    DXGI_VIRTUAL_HOOK ( &pSwapChain, 8,
                        "IDXGISwapChain::Present",
                         PresentCallback,
                         Present_Original,
                         PresentSwapChain_pfn );

    vftable_8 = vftable [8];
  }


  CComPtr <IDXGISwapChain1> pSwapChain1 = nullptr;

  if (SUCCEEDED (pSwapChain->QueryInterface (IID_PPV_ARGS (&pSwapChain1))))
  {
    vftable = *(void***)*&pSwapChain1;

    if (Present1_Original != nullptr)
    {
      //dll_log.Log (L"Rehooking IDXGISwapChain::Present1 (...)");

#if 0
      if (MH_OK == SK_RemoveHook (vftable [22]))
        Present1_Original = nullptr;
      else {
        dll_log.Log ( L"[   DXGI   ] Altered vftable detected, rehooking "
                      L"IDXGISwapChain1::Present1 (...)!" );
        if (MH_OK == SK_RemoveHook (vftable_22))
          Present1_Original = nullptr;
      }
#endif
    }

    else
    {
      DXGI_VIRTUAL_HOOK ( &pSwapChain1, 22,
                          "IDXGISwapChain1::Present1",
                           Present1Callback,
                           Present1_Original,
                           Present1SwapChain1_pfn );

      vftable_22 = vftable [22];
    }
  }

  //if (config.system.handle_crashes)
    //SK::Diagnostics::CrashHandler::Reinstall ();
}

std::wstring
SK_DXGI_FormatToString (DXGI_FORMAT fmt)
{
  UNREFERENCED_PARAMETER (fmt);
  return L"<NOT IMPLEMENTED>";
}

void
SK_DXGI_HookSwapChain (IDXGISwapChain* pSwapChain)
{
  static volatile ULONG init = FALSE;

  if (InterlockedCompareExchange (&init, TRUE, FALSE))
    return;

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

  if (SUCCEEDED (pSwapChain->GetContainingOutput (&pOutput)))
  {
    if (pOutput != nullptr)
    {
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
  MH_ApplyQueued      (          );
}

void
SK_DXGI_HookFactory (IDXGIFactory* pFactory)
{
  static volatile ULONG init = FALSE;

  if (InterlockedCompareExchange (&init, TRUE, FALSE))
    return;

  int iver = SK_GetDXGIFactoryInterfaceVer (pFactory);

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
  DXGI_VIRTUAL_HOOK ( &pFactory,     10,
                      "IDXGIFactory::CreateSwapChain",
                       DXGIFactory_CreateSwapChain_Override,
                                   CreateSwapChain_Original,
                                   CreateSwapChain_pfn );

  CComPtr <IDXGIFactory1> pFactory1 = nullptr;

  // 12 EnumAdapters1
  // 13 IsCurrent
  if (SUCCEEDED (pFactory->QueryInterface <IDXGIFactory1> (&pFactory1)))
  {
    DXGI_VIRTUAL_HOOK ( &pFactory1,     12,
                        "IDXGIFactory1::EnumAdapters1",
                         EnumAdapters1_Override,
                         EnumAdapters1_Original,
                         EnumAdapters1_pfn );
  }

  else
  {
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
  

  // DXGI 1.2+

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
  CComPtr <IDXGIFactory2> pFactory2 = nullptr;

  if (CreateDXGIFactory2_Import)
  {
    if (SUCCEEDED (CreateDXGIFactory1_Import (IID_IDXGIFactory2, (void **)&pFactory2)))
    {
      DXGI_VIRTUAL_HOOK ( &pFactory2,   15,
                          "IDXGIFactory2::CreateSwapChainForHwnd",
                           DXGIFactory2_CreateSwapChainForHwnd_Override,
                                        CreateSwapChainForHwnd_Original,
                                        CreateSwapChainForHwnd_pfn );

      DXGI_VIRTUAL_HOOK ( &pFactory2,   16,
                          "IDXGIFactory2::CreateSwapChainForCoreWindow",
                           DXGIFactory2_CreateSwapChainForCoreWindow_Override,
                                        CreateSwapChainForCoreWindow_Original,
                                        CreateSwapChainForCoreWindow_pfn );

      DXGI_VIRTUAL_HOOK ( &pFactory2,   24,
                          "IDXGIFactory2::CreateSwapChainForComposition",
                           DXGIFactory2_CreateSwapChainForComposition_Override,
                                        CreateSwapChainForComposition_Original,
                                        CreateSwapChainForComposition_pfn );
    }
  }


  // DXGI 1.3+
  CComPtr <IDXGIFactory3> pFactory3 = nullptr;

  // 25 GetCreationFlags


  // DXGI 1.4+
  CComPtr <IDXGIFactory4> pFactory4 = nullptr;

  // 26 EnumAdapterByLuid
  // 27 EnumWarpAdapter


  // DXGI 1.5+
  CComPtr <IDXGIFactory5> pFactory5 = nullptr;

  // 28 CheckFeatureSupport

  MH_ApplyQueued ();
}

struct sk_hook_d3d11_t {
 ID3D11Device**        ppDevice;
 ID3D11DeviceContext** ppImmediateContext;
} d3d11_hook_ctx;

volatile DWORD SK_D3D11_init_tid = 0;

unsigned int
__stdcall
HookDXGI (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  if (! config.apis.dxgi.d3d11.hook)
  {
    return 0;
  }

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

  if (InterlockedCompareExchange (&__dxgi_ready, TRUE, TRUE))
  {
    return 0;
  }

  bool success =
    SUCCEEDED ( CoInitializeEx (nullptr, COINIT_MULTITHREADED) );

  dll_log.Log (L"[   DXGI   ]   Installing DXGI Hooks");

  DXGI_SWAP_CHAIN_DESC desc = { };

  desc.BufferDesc.Width                   = 640;
  desc.BufferDesc.Height                  = 480;

  desc.BufferDesc.RefreshRate.Numerator   = 0;
  desc.BufferDesc.RefreshRate.Denominator = 0;
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
                      WS_POPUP | WS_MINIMIZEBOX,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                          640, 480, 0,
                            nullptr, nullptr, 0x00 );

  desc.OutputWindow = hwnd;
  desc.Windowed     = TRUE;

  desc.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;

  D3D_FEATURE_LEVEL             featureLevel;
  CComPtr <ID3D11Device>        pDevice           = nullptr;
  CComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;
  CComPtr <IDXGISwapChain>      pSwapChain        = nullptr;

  // DXI stuff is ready at this point, we'll hook the swapchain stuff
  //   after this call.

  extern LPVOID pfnD3D11CreateDevice;

  HRESULT hr =
    ((D3D11CreateDevice_pfn)(pfnD3D11CreateDevice)) (
      0,
        D3D_DRIVER_TYPE_HARDWARE,
          nullptr,
            0,
              nullptr,
                0,
                  D3D11_SDK_VERSION,
                    &pDevice,
                      &featureLevel,
                        &pImmediateContext );

  if (SUCCEEDED (hr))
  {
    CComPtr <IDXGIDevice>  pDevDXGI = nullptr;
    CComPtr <IDXGIAdapter> pAdapter = nullptr;
    CComPtr <IDXGIFactory> pFactory = nullptr;

    if ( SUCCEEDED (pDevice->QueryInterface (IID_PPV_ARGS (&pDevDXGI))) &&
         SUCCEEDED (pDevDXGI->GetAdapter                  (&pAdapter))  &&
         SUCCEEDED (pAdapter->GetParent     (IID_PPV_ARGS (&pFactory))) )
    {
      if (SUCCEEDED (pFactory->CreateSwapChain (pDevice, &desc, &pSwapChain)))
        SK_DXGI_HookSwapChain (pSwapChain);

      SK_DXGI_HookFactory (pFactory);

      d3d11_hook_ctx.ppDevice           = &pDevice;
      d3d11_hook_ctx.ppImmediateContext = &pImmediateContext;

      HookD3D11 (&d3d11_hook_ctx);

      // These don't do anything (anymore)
      if (config.apis.dxgi.d3d11.hook) SK_D3D11_EnableHooks ();
      if (config.apis.dxgi.d3d12.hook) SK_D3D12_EnableHooks ();
    }
  }

  else
  {
    _com_error err (hr);

    dll_log.Log (L"[   DXGI   ] Unable to hook D3D11?! (0x%04x :: '%s')",
                             err.WCode (), err.ErrorMessage () );
  }

  DestroyWindow (hwnd);

  InterlockedExchange (&__dxgi_ready, TRUE);

  return 0;
}


bool
SK::DXGI::Shutdown (void)
{
  if (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe"))
  {
    SK_DS3_ShutdownPlugin (L"dxgi");
  }

  if (config.apis.dxgi.d3d11.hook) SK_D3D11_Shutdown ();
  if (config.apis.dxgi.d3d12.hook) SK_D3D12_Shutdown ();

  return SK_ShutdownCore (L"dxgi");
}

void
WINAPI
SK_DXGI_SetPreferredAdapter (int override_id)
{
  SK_DXGI_preferred_adapter = override_id;
}

extern bool SK_D3D11_need_tex_reset;

memory_stats_t   mem_stats [MAX_GPU_NODES];
mem_info_t       mem_info  [NumBuffers];

struct budget_thread_params_t
{
           IDXGIAdapter3 *pAdapter = nullptr;
           DWORD          tid      = 0UL;
           HANDLE         handle   = INVALID_HANDLE_VALUE;
           DWORD          cookie   = 0UL;
           HANDLE         event    = INVALID_HANDLE_VALUE;
  volatile ULONG          ready    = FALSE;
} budget_thread;


HRESULT
SK::DXGI::StartBudgetThread ( IDXGIAdapter** ppAdapter )
{
  //
  // If the adapter implements DXGI 1.4, then create a budget monitoring
  //  thread...
  //
  IDXGIAdapter3* pAdapter3 = nullptr;
  HRESULT        hr        = E_NOTIMPL;

  if ( SUCCEEDED ( (*ppAdapter)->QueryInterface (
                                     IID_PPV_ARGS ( &pAdapter3 ) 
                   )
                 )
     )
  {
    // We darn sure better not spawn multiple threads!
    EnterCriticalSection ( &budget_mutex );

    if ( budget_thread.handle == INVALID_HANDLE_VALUE )
    {
      // We're going to Release this interface after thread spawnning, but
      //   the running thread still needs a reference counted.
      pAdapter3->AddRef ();

      ZeroMemory ( &budget_thread,
                     sizeof budget_thread_params_t );

      dll_log.LogEx ( true,
                        L"[ DXGI 1.4 ]   "
                        L"$ Spawning Memory Budget Change Thread..: " );

      InterlockedExchange ( &budget_thread.ready,
                              FALSE );

      budget_thread.pAdapter = pAdapter3;
      budget_thread.tid      = 0;
      budget_thread.event    = 0;
      budget_log.silent      = true;


      budget_thread.handle =
        (HANDLE)
          _beginthreadex ( nullptr,
                             0,
                               BudgetThread,
                                 (LPVOID)&budget_thread,
                                   0x00,
                                     nullptr );


      while ( ! InterlockedCompareExchange ( &budget_thread.ready,
                                               FALSE,
                                                 FALSE )
            ) Sleep (100);


      if ( budget_thread.tid != 0 )
      {
        dll_log.LogEx ( false,
                          L"tid=0x%04x\n",
                            budget_thread.tid );

        dll_log.LogEx ( true,
                          L"[ DXGI 1.4 ]   "
                            L"%% Setting up Budget Change Notification.: " );

        HRESULT result =
          pAdapter3->RegisterVideoMemoryBudgetChangeNotificationEvent (
                            budget_thread.event,
                           &budget_thread.cookie
          );

        if ( SUCCEEDED ( result ) )
        {
          dll_log.LogEx ( false,
                            L"eid=0x%p, cookie=%u\n",
                              budget_thread.event,
                                        budget_thread.cookie );

          hr = S_OK;
        }

        else
        {
          dll_log.LogEx ( false,
                            L"Failed! (%s)\n",
                              SK_DescribeHRESULT ( result ) );

          hr = result;
        }
      }

      else
      {
        dll_log.LogEx (false, L"failed!\n");

        hr = E_FAIL;
      }

      dll_log.LogEx ( true,
                        L"[ DXGI 1.2 ] GPU Scheduling...:"
                                     L" Pre-Emptive" );

      DXGI_QUERY_VIDEO_MEMORY_INFO
              _mem_info;
      DXGI_ADAPTER_DESC2
              desc2;

      int     i      = 0;
      bool    silent = dll_log.silent;
      dll_log.silent = true;
      {
        // Don't log this call, because that would be silly...
        pAdapter3->GetDesc2 ( &desc2 );
      }
      dll_log.silent = silent;


      switch ( desc2.GraphicsPreemptionGranularity )
      {
        case DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY:
          dll_log.LogEx ( false, L" (DMA Buffer)\n"         );
          break;

        case DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY:
          dll_log.LogEx ( false, L" (Graphics Primitive)\n" );
          break;

        case DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY:
          dll_log.LogEx ( false, L" (Triangle)\n"           );
          break;

        case DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY:
          dll_log.LogEx ( false, L" (Fragment)\n"           );
          break;

        case DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY:
          dll_log.LogEx ( false, L" (Instruction)\n"        );
          break;

        default:
          dll_log.LogEx (false, L"UNDEFINED\n");
          break;
      }

      dll_log.LogEx ( true,
                        L"[ DXGI 1.4 ] Local Memory.....:" );

      while ( SUCCEEDED (
                pAdapter3->QueryVideoMemoryInfo (
                  i,
                    DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
                      &_mem_info
                )
              )
            )
      {
        if ( i > 0 )
        {
          dll_log.LogEx ( false, L"\n"                              );
          dll_log.LogEx ( true,  L"                               " );
        }

        dll_log.LogEx ( false,
                          L" Node%i (Reserve: %#5llu / %#5llu MiB - "
                                    L"Budget: %#5llu / %#5llu MiB)",
                            i++,
                              _mem_info.CurrentReservation      >> 20ULL,
                              _mem_info.AvailableForReservation >> 20ULL,
                              _mem_info.CurrentUsage            >> 20ULL,
                              _mem_info.Budget                  >> 20ULL
                      );

        pAdapter3->SetVideoMemoryReservation (
              ( i - 1 ),
                DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
                  ( i == 1 || USE_SLI ) ?
                    uint64_t ( _mem_info.AvailableForReservation *
                                 config.mem.reserve * 0.01f ) 
                           :
                           0
        );
      }

      i = 0;

      dll_log.LogEx ( false, L"\n"                              );
      dll_log.LogEx ( true,  L"[ DXGI 1.4 ] Non-Local Memory.:" );

      while ( SUCCEEDED (
                pAdapter3->QueryVideoMemoryInfo (
                  i,
                    DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
                      &_mem_info
                )
              )
            )
      {
        if ( i > 0 )
        {
          dll_log.LogEx ( false, L"\n"                              );
          dll_log.LogEx ( true,  L"                               " );
        }

        dll_log.LogEx ( false,
                          L" Node%i (Reserve: %#5llu / %#5llu MiB - "
                                    L"Budget: %#5llu / %#5llu MiB)",
                            i++,
                              _mem_info.CurrentReservation      >> 20ULL,
                              _mem_info.AvailableForReservation >> 20ULL,
                              _mem_info.CurrentUsage            >> 20ULL,
                              _mem_info.Budget                  >> 20ULL
                      );

        pAdapter3->SetVideoMemoryReservation (
              ( i - 1 ),
                DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
                  ( i == 1 || USE_SLI ) ?
                    uint64_t ( _mem_info.AvailableForReservation *
                                 config.mem.reserve * 0.01f )
                           :
                           0
        );
      }

      ::mem_info [0].nodes = ( i - 1 );
      ::mem_info [1].nodes = ( i - 1 );

      dll_log.LogEx ( false, L"\n" );
    }

    LeaveCriticalSection ( &budget_mutex );
  }

  return hr;
}

#include <ctime>
#define min_max(ref,min,max) if ((ref) > (max)) (max) = (ref); \
                             if ((ref) < (min)) (min) = (ref);

const uint32_t
  BUDGET_POLL_INTERVAL = 66UL; // How often to sample the budget
                               //  in msecs

unsigned int
__stdcall
SK::DXGI::BudgetThread ( LPVOID user_data )
{
  budget_thread_params_t* params =
    (budget_thread_params_t *)user_data;

  if (true)
  {
    budget_log.silent = true;
    params->tid       = GetCurrentThreadId ();
    params->event     =
      CreateEvent ( nullptr,
                      FALSE,
                        FALSE,
                          L"DXGIMemoryBudget"
                  );

    InterlockedExchange ( &params->ready, TRUE );
  }

  else
  {
    params->tid = 0;

    //
    // The thread is not ready (we cannot write tracking info),
    //   but signal it as such anyway...
    //
    InterlockedExchange ( &params->ready, TRUE );

    return 0;
  }


  bool success =
    SUCCEEDED ( CoInitializeEx (nullptr, COINIT_MULTITHREADED ) );


  while ( InterlockedCompareExchange ( &params->ready, FALSE, FALSE ) )
  {
    if (InterlockedExchangeAdd (&__SK_DLL_Ending, 0))
      break;

    if ( params->event == 0 )
      break;

    DWORD dwWaitStatus =
      WaitForSingleObject ( params->event,
                              BUDGET_POLL_INTERVAL * 2 );

    if (! InterlockedCompareExchange ( &params->ready, FALSE, FALSE ) )
    {
      ResetEvent ( params->event );
      break;
    }

    int         node = 0;

    buffer_t buffer  =
      mem_info [node].buffer;


    InterlockedExchange64 (&SK_D3D11_Textures.Budget, mem_info [buffer].local [0].Budget -
                                                      mem_info [buffer].local [0].CurrentUsage );


    // Double-Buffer Updates
    if ( buffer == Front )
      buffer = Back;
    else
      buffer = Front;


    GetLocalTime ( &mem_info [buffer].time );

    //
    // Sample Fast nUMA (On-GPU / Dedicated) Memory
    //
    for ( node = 0; node < MAX_GPU_NODES; )
    {
      if ( FAILED (
             params->pAdapter->QueryVideoMemoryInfo (
               node,
                 DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
                   &mem_info [buffer].local [node++]
             )
           )
         )
      {
        break;
      }
    }


    // Fix for AMD drivers, that don't allow querying non-local memory
    int nodes =
      std::max (0, node - 1);

    //
    // Sample Slow nUMA (Off-GPU / Shared) Memory
    //
    for ( node = 0; node < MAX_GPU_NODES; )
    {
      if ( FAILED (
             params->pAdapter->QueryVideoMemoryInfo (
               node,
                 DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
                   &mem_info [buffer].nonlocal [node++]
             )
           )
         )
      {
        break;
      }
    }


    // Set the number of SLI/CFX Nodes
    mem_info [buffer].nodes = nodes;

    static uint64_t
      last_budget = mem_info [buffer].local [0].Budget;


    if ( nodes > 0 )
    {
      int i;

      budget_log.LogEx ( true,
                           L"[ DXGI 1.4 ] Local Memory.....:" );

      for ( i = 0; i < nodes; i++ )
      {
        if ( dwWaitStatus == WAIT_OBJECT_0 )
        {
          static UINT64
            LastBudget = 0ULL;

          mem_stats [i].budget_changes++;

          int64_t over_budget =
            ( mem_info [buffer].local [i].CurrentUsage -
              mem_info [buffer].local [i].Budget );

            //LastBudget -
            //mem_info [buffer].local [i].Budget;

          SK_D3D11_need_tex_reset = ( over_budget > 0 );

          LastBudget =
            mem_info [buffer].local [i].Budget;
        }

        if ( i > 0 )
        {
          budget_log.LogEx ( false, L"\n"                                );
          budget_log.LogEx ( true,  L"                                 " );
        }

        budget_log.LogEx (
          false,
            L" Node%i (Reserve: %#5llu / %#5llu MiB - "
                      L"Budget: %#5llu / %#5llu MiB)",
          i,
            mem_info       [buffer].local [i].CurrentReservation      >> 20ULL,
              mem_info     [buffer].local [i].AvailableForReservation >> 20ULL,
                mem_info   [buffer].local [i].CurrentUsage            >> 20ULL,
                  mem_info [buffer].local [i].Budget                  >> 20ULL
        );

        min_max ( mem_info [buffer].local [i].AvailableForReservation,
                                mem_stats [i].min_avail_reserve,
                                mem_stats [i].max_avail_reserve );

        min_max ( mem_info [buffer].local [i].CurrentReservation,
                                mem_stats [i].min_reserve,
                                mem_stats [i].max_reserve );

        min_max ( mem_info [buffer].local [i].CurrentUsage,
                                mem_stats [i].min_usage,
                                mem_stats [i].max_usage );

        min_max ( mem_info [buffer].local [i].Budget,
                                mem_stats [i].min_budget,
                                mem_stats [i].max_budget );

        if ( mem_info [buffer].local [i].CurrentUsage >
             mem_info [buffer].local [i].Budget)
        {
          uint64_t over_budget =
             ( mem_info [buffer].local [i].CurrentUsage -
               mem_info [buffer].local [i].Budget );

          min_max ( over_budget,
                           mem_stats [i].min_over_budget,
                           mem_stats [i].max_over_budget );
        }
      }

      budget_log.LogEx ( false, L"\n"                              );
      budget_log.LogEx ( true,  L"[ DXGI 1.4 ] Non-Local Memory.:" );

      for ( i = 0; i < nodes; i++ )
      {
        if ( i > 0 )
        {
          budget_log.LogEx ( false, L"\n"                                );
          budget_log.LogEx ( true,  L"                                 " );
        }

        budget_log.LogEx (
          false,
            L" Node%i (Reserve: %#5llu / %#5llu MiB - "
                      L"Budget: %#5llu / %#5llu MiB)",    i,
         mem_info    [buffer].nonlocal [i].CurrentReservation      >> 20ULL,
          mem_info   [buffer].nonlocal [i].AvailableForReservation >> 20ULL,
           mem_info  [buffer].nonlocal [i].CurrentUsage            >> 20ULL,
            mem_info [buffer].nonlocal [i].Budget                  >> 20ULL );
      }

      budget_log.LogEx ( false, L"\n" );
    }

               ( params->event != 0 ) ?
    ResetEvent ( params->event )      :
                         (void)0;

    mem_info [0].buffer =
                 buffer;
  }

  if (success)
    CoUninitialize ();

  return 0;
}

HRESULT
SK::DXGI::StartBudgetThread_NoAdapter (void)
{
  HRESULT hr = E_NOTIMPL;

  bool success =
    SUCCEEDED ( CoInitializeEx ( nullptr, COINIT_MULTITHREADED ) );


  static HMODULE
    hDXGI = LoadLibraryW_Original ( L"dxgi.dll" );

  static CreateDXGIFactory_pfn
    CreateDXGIFactory =
      (CreateDXGIFactory_pfn) GetProcAddress ( hDXGI,
                                                 "CreateDXGIFactory" );
  CComPtr <IDXGIFactory> factory = nullptr;
  CComPtr <IDXGIAdapter> adapter = nullptr;

  // Only spawn the DXGI 1.4 budget thread if ... DXGI 1.4 is implemented.
  if ( SUCCEEDED (
         CreateDXGIFactory ( IID_PPV_ARGS (&factory) )
       )
     )
  {
    if ( SUCCEEDED (
           factory->EnumAdapters ( 0,
                                     &adapter )
         )
       )
    {
      hr = StartBudgetThread ( &adapter );
    }
  }

  if (success)
    CoUninitialize ();

  return hr;
}

void
SK::DXGI::ShutdownBudgetThread ( void )
{
  SK_AutoClose_Log (
    budget_log
  );


  if ( budget_thread.handle != INVALID_HANDLE_VALUE )
  {
    dll_log.LogEx (
      true,
        L"[ DXGI 1.4 ] Shutting down Memory Budget Change Thread... "
    );

    InterlockedExchange ( &budget_thread.ready, FALSE );

    DWORD dwWaitState =
      SignalObjectAndWait ( budget_thread.event,
                              budget_thread.handle, // Give 1 second, and
                                1000UL,             // then we're killing
                                  TRUE );           // the thing!

    if ( dwWaitState == WAIT_OBJECT_0 )
    {
      dll_log.LogEx   ( false, L"done!\n"                         );
    }

    else
    {
      dll_log.LogEx   ( false, L"timed out (killing manually)!\n" );
      TerminateThread ( budget_thread.handle,                   0 );
    }

    CloseHandle ( budget_thread.handle );
                  budget_thread.handle = INVALID_HANDLE_VALUE;

    // Record the final statistics always
    budget_log.silent    = false;

    budget_log.Log   ( L"--------------------"   );
    budget_log.Log   ( L"Shutdown Statistics:"   );
    budget_log.Log   ( L"--------------------\n" );

    // in %10u seconds\n",
    budget_log.Log ( L" Memory Budget Changed %llu times\n",
                       mem_stats [0].budget_changes );

    for ( int i = 0; i < 4; i++ )
    {
      if ( mem_stats [i].max_usage > 0 )
      {
        if ( mem_stats [i].min_reserve     == UINT64_MAX )
             mem_stats [i].min_reserve     =  0ULL;

        if ( mem_stats [i].min_over_budget == UINT64_MAX )
             mem_stats [i].min_over_budget =  0ULL;

        budget_log.LogEx ( true,
                             L" GPU%i: Min Budget:        %05llu MiB\n",
                                          i,
                               mem_stats [i].min_budget >> 20ULL );
        budget_log.LogEx ( true,
                             L"       Max Budget:        %05llu MiB\n",
                               mem_stats [i].max_budget >> 20ULL );

        budget_log.LogEx ( true,
                             L"       Min Usage:         %05llu MiB\n",
                               mem_stats [i].min_usage  >> 20ULL );
        budget_log.LogEx ( true,
                             L"       Max Usage:         %05llu MiB\n",
                               mem_stats [i].max_usage  >> 20ULL );

        /*
        SK_BLogEx (params, true, L"       Min Reserve:       %05u MiB\n",
        mem_stats [i].min_reserve >> 20ULL);
        SK_BLogEx (params, true, L"       Max Reserve:       %05u MiB\n",
        mem_stats [i].max_reserve >> 20ULL);
        SK_BLogEx (params, true, L"       Min Avail Reserve: %05u MiB\n",
        mem_stats [i].min_avail_reserve >> 20ULL);
        SK_BLogEx (params, true, L"       Max Avail Reserve: %05u MiB\n",
        mem_stats [i].max_avail_reserve >> 20ULL);
        */

        budget_log.LogEx ( true,  L"------------------------------------\n" );
        budget_log.LogEx ( true,  L" Minimum Over Budget:     %05llu MiB\n",
                                    mem_stats [i].min_over_budget >> 20ULL  );
        budget_log.LogEx ( true,  L" Maximum Over Budget:     %05llu MiB\n",
                                    mem_stats [i].max_over_budget >> 20ULL  );
        budget_log.LogEx ( true,  L"------------------------------------\n" );
        budget_log.LogEx ( false, L"\n"                                     );
      }
    }
  }
}