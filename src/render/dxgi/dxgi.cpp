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
#include <SpecialK/import.h>

#include <Windows.h>

#include <SpecialK/render/dxgi/dxgi_interfaces.h>
#include <SpecialK/render/dxgi/dxgi_swapchain.h> 
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/window.h>

#include <comdef.h>
#include <atlbase.h>

#include <SpecialK/nvapi.h>
#include <SpecialK/config.h>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory>
#include <set>
#include <queue>
#include <vector>
#include <limits>
#include <algorithm>
#include <functional>


#include <SpecialK/log.h>
#include <SpecialK/utility.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <SpecialK/tls.h>
#include <SpecialK/thread.h>
#include <SpecialK/command.h>
#include <SpecialK/console.h>
#include <SpecialK/framerate.h>
#include <SpecialK/steam_api.h>

#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/diagnostics/crash_handler.h>

#include <SpecialK/plugin/plugin_mgr.h>

#include <imgui/backends/imgui_d3d11.h>


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


#pragma data_seg (".SK_DXGI_Hooks")
extern "C"
{
  // Global DLL's cache
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGIFactory_CreateSwapChain)
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGIFactory2_CreateSwapChainForHwnd)
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGIFactory2_CreateSwapChainForCoreWindow)
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGIFactory2_CreateSwapChainForComposition)
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGISwapChain_Present)
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGISwapChain1_Present1)
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGISwapChain_ResizeTarget)
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGISwapChain_ResizeBuffers)
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGISwapChain_GetFullscreenState)
  __declspec (dllexport) SK_HookCacheEntryGlobal (IDXGISwapChain_SetFullscreenState)
  __declspec (dllexport) SK_HookCacheEntryGlobal (CreateDXGIFactory)
  __declspec (dllexport) SK_HookCacheEntryGlobal (CreateDXGIFactory1)
  __declspec (dllexport) SK_HookCacheEntryGlobal (CreateDXGIFactory2)
};
#pragma data_seg ()
#pragma comment  (linker, "/SECTION:.SK_DXGI_Hooks,RWS")


// Local DLL's cached addresses
SK_HookCacheEntryLocal (IDXGIFactory_CreateSwapChain,                L"dxgi.dll", DXGIFactory_CreateSwapChain_Override,                &CreateSwapChain_Original)
SK_HookCacheEntryLocal (IDXGIFactory2_CreateSwapChainForHwnd,        L"dxgi.dll", DXGIFactory2_CreateSwapChainForHwnd_Override,        &CreateSwapChainForHwnd_Original)
SK_HookCacheEntryLocal (IDXGIFactory2_CreateSwapChainForCoreWindow,  L"dxgi.dll", DXGIFactory2_CreateSwapChainForCoreWindow_Override,  &CreateSwapChainForCoreWindow_Original)
SK_HookCacheEntryLocal (IDXGIFactory2_CreateSwapChainForComposition, L"dxgi.dll", DXGIFactory2_CreateSwapChainForComposition_Override, &CreateSwapChainForComposition_Original)
SK_HookCacheEntryLocal (IDXGISwapChain_Present,                      L"dxgi.dll", PresentCallback,                                     &Present_Original);
SK_HookCacheEntryLocal (IDXGISwapChain1_Present1,                    L"dxgi.dll", Present1Callback,                                    &Present1_Original);
SK_HookCacheEntryLocal (IDXGISwapChain_ResizeTarget,                 L"dxgi.dll", DXGISwap_ResizeTarget_Override,                      &ResizeTarget_Original)
SK_HookCacheEntryLocal (IDXGISwapChain_ResizeBuffers,                L"dxgi.dll", DXGISwap_ResizeBuffers_Override,                     &ResizeBuffers_Original)
SK_HookCacheEntryLocal (IDXGISwapChain_GetFullscreenState,           L"dxgi.dll", DXGISwap_GetFullscreenState_Override,                &GetFullscreenState_Original)
SK_HookCacheEntryLocal (IDXGISwapChain_SetFullscreenState,           L"dxgi.dll", DXGISwap_SetFullscreenState_Override,                &SetFullscreenState_Original)
SK_HookCacheEntryLocal (CreateDXGIFactory,                           L"dxgi.dll", CreateDXGIFactory,                                   &CreateDXGIFactory_Import)
SK_HookCacheEntryLocal (CreateDXGIFactory1,                          L"dxgi.dll", CreateDXGIFactory1,                                  &CreateDXGIFactory1_Import)
SK_HookCacheEntryLocal (CreateDXGIFactory2,                          L"dxgi.dll", CreateDXGIFactory2,                                  &CreateDXGIFactory2_Import)

// Counter-intuitively, the fewer of these we cache the more compatible we get.
static
std::vector <sk_hook_cache_record_s *> global_dxgi_records =
  { &GlobalHook_IDXGIFactory_CreateSwapChain,               &GlobalHook_IDXGIFactory2_CreateSwapChainForHwnd,
    &GlobalHook_IDXGISwapChain_Present,                     &GlobalHook_IDXGISwapChain1_Present1,
    &GlobalHook_IDXGIFactory2_CreateSwapChainForCoreWindow,//&GlobalHook_IDXGIFactory2_CreateSwapChainForComposition,
  //&GlobalHook_IDXGISwapChain_ResizeTarget,                &GlobalHook_IDXGISwapChain_ResizeBuffers,
  //&GlobalHook_IDXGISwapChain_GetFullscreenState,          &GlobalHook_IDXGISwapChain_SetFullscreenState,
  //&GlobalHook_CreateDXGIFactory,                          &GlobalHook_CreateDXGIFactory1,
  //&GlobalHook_CreateDXGIFactory2
  };

static
std::vector <sk_hook_cache_record_s *> local_dxgi_records =
  { &LocalHook_IDXGIFactory_CreateSwapChain,               &LocalHook_IDXGIFactory2_CreateSwapChainForHwnd,
    &LocalHook_IDXGISwapChain_Present,                     &LocalHook_IDXGISwapChain1_Present1,
    &LocalHook_IDXGIFactory2_CreateSwapChainForCoreWindow,//&LocalHook_IDXGIFactory2_CreateSwapChainForComposition,
  //&LocalHook_IDXGISwapChain_ResizeTarget,                &LocalHook_IDXGISwapChain_ResizeBuffers,
  //&LocalHook_IDXGISwapChain_GetFullscreenState,          &LocalHook_IDXGISwapChain_SetFullscreenState,
  //&LocalHook_CreateDXGIFactory,                          &LocalHook_CreateDXGIFactory1,
  //&LocalHook_CreateDXGIFactory2
  };



#define SK_LOG_ONCE(x) { static bool logged = false; if (! logged) \
                       { dll_log.Log ((x)); logged = true; } }

  extern bool __SK_bypass;

extern SK_Thread_HybridSpinlock cs_mmio;

extern void SK_D3D11_EndFrame       (void);
extern void SK_DXGI_UpdateSwapChain (IDXGISwapChain*);

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

#include <SpecialK/osd/text.h>
#include <SpecialK/osd/popup.h>


#ifndef SK_BUILD__INSTALLER
#ifdef _WIN64
# define SK_CEGUI_LIB_BASE "CEGUI/x64/"
#else
# define SK_CEGUI_LIB_BASE "CEGUI/Win32/"
#endif

#define _SKC_MakeCEGUILib(library) \
  __pragma (comment (lib, SK_CEGUI_LIB_BASE #library ##".lib"))

_SKC_MakeCEGUILib ("CEGUIDirect3D11Renderer-0")
_SKC_MakeCEGUILib ("CEGUIBase-0")
_SKC_MakeCEGUILib ("CEGUICoreWindowRendererSet")
_SKC_MakeCEGUILib ("CEGUIRapidXMLParser")
_SKC_MakeCEGUILib ("CEGUICommonDialogs-0")
_SKC_MakeCEGUILib ("CEGUISTBImageCodec")

#include <delayimp.h>
#pragma comment (lib, "delayimp.lib")

#include <CEGUI/CEGUI.h>
#include <CEGUI/Rect.h>
#include <CEGUI/RendererModules/Direct3D11/Renderer.h>

static
CEGUI::Direct3D11Renderer* cegD3D11 = nullptr;
#endif

struct sk_hook_d3d11_t {
 ID3D11Device**        ppDevice;
 ID3D11DeviceContext** ppImmediateContext;
} d3d11_hook_ctx;

void SK_DXGI_HookSwapChain (IDXGISwapChain* pSwapChain);

void
ImGui_DX11Shutdown ( void )
{
  ImGui_ImplDX11_Shutdown ();
}

bool
ImGui_DX11Startup ( IDXGISwapChain* pSwapChain )
{
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();


  CComPtr <ID3D11Device>        pD3D11Dev         = nullptr;
  CComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;

  //assert (pSwapChain == rb.swapchain ||
  //                      rb.swapchain.IsEqualObject (pSwapChain));

  if ( SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&pD3D11Dev))) )
  {
    //assert (pD3D11Dev.IsEqualObject (rb.device) ||
    //                                 rb.device == nullptr);

    pD3D11Dev->GetImmediateContext (&pImmediateContext);

    //assert (pImmediateContext.IsEqualObject (rb.d3d11.immediate_ctx) ||
    //                                         rb.d3d11.immediate_ctx == nullptr);

    if ( pImmediateContext != nullptr )
    {
      if (! (( rb.device    == nullptr || rb.device.IsEqualObject    (pD3D11Dev)  ) ||
             ( rb.swapchain == nullptr || rb.swapchain.IsEqualObject (pSwapChain) )) )
      {
        DXGI_SWAP_CHAIN_DESC swap_desc = { };

        if (SUCCEEDED (pSwapChain->GetDesc (&swap_desc)))
        {
          if (swap_desc.OutputWindow != nullptr)
          {
            if (rb.windows.focus.hwnd != swap_desc.OutputWindow)
              rb.windows.setFocus (swap_desc.OutputWindow);

            if (rb.windows.device.hwnd == nullptr)
              rb.windows.setDevice (swap_desc.OutputWindow);
          }
        }
      }

      return
        ImGui_ImplDX11_Init (pSwapChain, pD3D11Dev, pImmediateContext);
    }
  }

  return false;
}

static volatile ULONG __gui_reset        = TRUE;
static volatile ULONG __osd_frames_drawn = 0;

void
SK_CEGUI_RelocateLog (void)
{
  if (! config.cegui.enable) return;


  // Move the log file that this darn thing just created...
  if (GetFileAttributesW (L"CEGUI.log") != INVALID_FILE_ATTRIBUTES)
  {
    char     szNewLogPath [MAX_PATH * 4] = { };
    wchar_t wszNewLogPath [MAX_PATH + 1] = { };

    wcscpy   (wszNewLogPath, SK_GetConfigPath ());
    strcpy   ( szNewLogPath, SK_WideCharToUTF8 (wszNewLogPath).c_str ());

    lstrcatA ( szNewLogPath,  R"(logs\CEGUI.log)");
    lstrcatW (wszNewLogPath, LR"(logs\CEGUI.log)");

    CopyFileExW ( L"CEGUI.log", wszNewLogPath,
                    nullptr, nullptr, nullptr,
                      0x00 );

    CEGUI::Logger::getDllSingleton ().setLogFilename (reinterpret_cast <const CEGUI::utf8 *> (szNewLogPath), true);

    CEGUI::Logger::getDllSingleton ().logEvent       ("[Special K] ---- Log File Moved ----");
    CEGUI::Logger::getDllSingleton ().logEvent       ("");

    DeleteFileW (L"CEGUI.log");
  }
}

CEGUI::Window* SK_achv_popup = nullptr;

CEGUI::System*
SK_CEGUI_GetSystem (void)
{
  if (! config.cegui.enable) return nullptr;


  return CEGUI::System::getDllSingletonPtr ();
}


void
__SEH_InitParser (CEGUI::XMLParser* parser)
{
  //__try
  //{
    if (parser->isPropertyPresent ("SchemaDefaultResourceGroup"))
      parser->setProperty ("SchemaDefaultResourceGroup", "schemas");
  //}

  //__except (EXCEPTION_EXECUTE_HANDLER)
  //{
  //  config.cegui.enable = false;
  //}
}

void
SK_CEGUI_InitParser (void)
{
  CEGUI::System& pSys =
    CEGUI::System::getDllSingleton ();
  
  // setup default group for validation schemas
  CEGUI::XMLParser* parser =
    pSys.getXMLParser ();

  if (SK_ValidatePointer (parser))
  {
    __SEH_InitParser (parser);
  }

  else
    config.cegui.enable = false;
}


void
SK_CEGUI_InitBase ()
{
  if (! config.cegui.enable) return;


  try
  {
    // initialise the required dirs for the DefaultResourceProvider
    auto* rp =
        dynamic_cast <CEGUI::DefaultResourceProvider *>
            (CEGUI::System::getDllSingleton ().getResourceProvider ());


    CEGUI::SchemeManager* pSchemeMgr =
      CEGUI::SchemeManager::getDllSingletonPtr ();

    CEGUI::FontManager* pFontMgr =
      CEGUI::FontManager::getDllSingletonPtr ();


         char szRootPath [MAX_PATH + 2] = { };
    snprintf (szRootPath, MAX_PATH, "%ws", _wgetenv (L"CEGUI_PARENT_DIR"));
              szRootPath [MAX_PATH] = '\0';

    CEGUI::String dataPathPrefix ( ( std::string (szRootPath) +
                                     std::string ("CEGUI/datafiles") ).c_str () );

    /* for each resource type, set a resource group directory. We cast strings
       to "const CEGUI::utf8*" in order to support general Unicode strings,
       rather than only ASCII strings (even though currently they're all ASCII).
       */
    rp->setResourceGroupDirectory("schemes",
      dataPathPrefix + reinterpret_cast<const CEGUI::utf8*>("/schemes/"));
    rp->setResourceGroupDirectory("imagesets",
      dataPathPrefix + reinterpret_cast<const CEGUI::utf8*>("/imagesets/"));
    rp->setResourceGroupDirectory("fonts",
      dataPathPrefix + reinterpret_cast<const CEGUI::utf8*>("/fonts/"));
    rp->setResourceGroupDirectory("layouts",
      dataPathPrefix + reinterpret_cast<const CEGUI::utf8*>("/layouts/"));
    rp->setResourceGroupDirectory("looknfeels",
      dataPathPrefix + reinterpret_cast<const CEGUI::utf8*>("/looknfeel/"));
    rp->setResourceGroupDirectory("lua_scripts",
      dataPathPrefix + reinterpret_cast<const CEGUI::utf8*>("/lua_scripts/"));
    rp->setResourceGroupDirectory("schemas",
      dataPathPrefix + reinterpret_cast<const CEGUI::utf8*>("/xml_schemas/"));
    rp->setResourceGroupDirectory("animations",
      dataPathPrefix + reinterpret_cast<const CEGUI::utf8*>("/animations/"));

    // set the default resource groups to be used
    CEGUI::ImageManager::setImagesetDefaultResourceGroup ("imagesets");
    //CEGUI::ImageManager::addImageType                    ("BasicImage");
    CEGUI::Font::setDefaultResourceGroup                 ("fonts");
    CEGUI::Scheme::setDefaultResourceGroup               ("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup    ("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup        ("layouts");
    CEGUI::ScriptModule::setDefaultResourceGroup         ("lua_scripts");
    CEGUI::AnimationManager::setDefaultResourceGroup     ("animations");
    
    pSchemeMgr->createFromFile ("VanillaSkin.scheme");
    pSchemeMgr->createFromFile ("TaharezLook.scheme");
    
    pFontMgr->createFromFile ("Consolas-12.font");
    pFontMgr->createFromFile ("DejaVuSans-10-NoScale.font");
    pFontMgr->createFromFile ("DejaVuSans-12-NoScale.font");
    pFontMgr->createFromFile ("Jura-18-NoScale.font");
    pFontMgr->createFromFile ("Jura-13-NoScale.font");
    pFontMgr->createFromFile ("Jura-10-NoScale.font");


    SK_CEGUI_InitParser ();

    // ^^^^ This is known to fail in debug builds; if it does,
    //        config.cegui.enable will be turned off.

    if (! config.cegui.enable)
      return;


    // Set a default window size if CEGUI cannot figure it out...
    if ( CEGUI::System::getDllSingleton ().getRenderer ()->getDisplaySize () ==
           CEGUI::Sizef (0.0f, 0.0f) )
    {
      CEGUI::System::getDllSingleton ().getRenderer ()->setDisplaySize (
          CEGUI::Sizef (
            static_cast <float> (game_window.render_x),
              static_cast <float> (game_window.render_y)
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

   //This window is never used, it is the prototype from which all
   //  achievement popup dialogs will be cloned. This makes the whole
   //    process of instantiating pop ups quicker.
    SK_achv_popup =
      window_mgr.loadLayoutFromFile ("Achievements.layout");
  }

  catch (CEGUI::GenericException& e)
  {
    SK_LOG0 ( (L"CEGUI Exception During Core Init"),
               L"   CEGUI  "  );
    SK_LOG0 ( (L" >> %hs (%hs:%lu): Exception %hs -- %hs",
                e.getFunctionName    ().c_str (),
                e.getFileName        ().c_str (),
                e.getLine            (),
                        e.getName    ().c_str (),
                        e.getMessage ().c_str () ),
               L"   CEGUI  "  );

    config.cegui.enable = false;
  }

  SK_Steam_ClearPopups ();
}

void ResetCEGUI_D3D11 (IDXGISwapChain* This)
{
  void
  __stdcall
  SK_D3D11_ResetTexCache (void);

  SK_D3D11_ResetTexCache (    );

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();


  assert (rb.swapchain == nullptr || This == rb.swapchain || rb.swapchain.IsEqualObject (This));

  CComPtr <ID3D11Device> pDev = nullptr;

  if (SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))))
  {
    assert (rb.device == nullptr || pDev.IsEqualObject (rb.device));

    rb.releaseOwnedResources (    );
    SK_DXGI_UpdateSwapChain  (This);
  }


  if (cegD3D11 != nullptr)
  {
    if ((uintptr_t)cegD3D11 > 1)
    {
      SK_TextOverlayManager::getInstance ()->destroyAllOverlays ();
      SK_PopupManager::getInstance ()->destroyAllPopups         ();
    }

    rb.releaseOwnedResources ();

    if ((uintptr_t)cegD3D11 > 1)
    {
      CEGUI::WindowManager::getDllSingleton ().cleanDeadPool ();

      cegD3D11->destroySystem  ();
    }

    cegD3D11 = nullptr;

    // XXX: TODO (Full shutdown isn't necessary, just invalidate)
    ImGui_DX11Shutdown ();
  }

  else
  {
    assert (rb.device != nullptr);

    if (rb.device == nullptr)
      SK_DXGI_UpdateSwapChain (This);

    CComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

    // For CEGUI to work correctly, it is necessary to set the viewport dimensions
    //   to the back buffer size prior to bootstrap.
    CComPtr <ID3D11Texture2D> pBackBuffer = nullptr;

    D3D11_VIEWPORT vp_orig = { };
    UINT           num_vp  =  1;

    pDevCtx->RSGetViewports (&num_vp, &vp_orig);

    if ( SUCCEEDED (This->GetBuffer (0, IID_PPV_ARGS (&pBackBuffer))) )
    {
      D3D11_VIEWPORT                    vp = { };
      D3D11_TEXTURE2D_DESC backbuffer_desc = { };

      pBackBuffer->GetDesc (&backbuffer_desc);

      vp.Width    = static_cast <float> (backbuffer_desc.Width);
      vp.Height   = static_cast <float> (backbuffer_desc.Height);
      vp.MinDepth = 0.0f;
      vp.MaxDepth = 1.0f;
      vp.TopLeftX = 0.0f;
      vp.TopLeftY = 0.0f;

      pDevCtx->RSSetViewports (1, &vp);

      if (config.cegui.enable)
      {
        CComQIPtr <ID3D11Device>        pCEGUIDev    (rb.device);
        CComQIPtr <ID3D11DeviceContext> pCEGUIDevCtx (rb.d3d11.immediate_ctx);

        try {
          cegD3D11 = dynamic_cast <CEGUI::Direct3D11Renderer *>
            (&CEGUI::Direct3D11Renderer::bootstrapSystem (
              pCEGUIDev,
                pCEGUIDevCtx
             )
            );
        }

        catch (CEGUI::GenericException& e)
        {
          SK_LOG0 ( (L"CEGUI Exception During D3D11 Bootstrap"),
                     L"   CEGUI  "  );
          SK_LOG0 ( (L" >> %hs (%hs:%lu): Exception %hs -- %hs",
                      e.getFunctionName    ().c_str (),
                      e.getFileName        ().c_str (),
                      e.getLine            (),
                              e.getName    ().c_str (),
                              e.getMessage ().c_str () ),
                     L"   CEGUI  "  );

          config.cegui.enable = false;
        }
      }
      else
        cegD3D11 = reinterpret_cast <CEGUI::Direct3D11Renderer *> (1);

      ImGui_DX11Startup    (This);

      if ((uintptr_t)cegD3D11 > 1)
        SK_CEGUI_RelocateLog (    );

      pDevCtx->RSSetViewports (1, &vp_orig);

      if ((uintptr_t)cegD3D11 > 1)
      {
        SK_CEGUI_InitBase    ();

        SK_PopupManager::getInstance       ()->destroyAllPopups (        );
        SK_TextOverlayManager::getInstance ()->resetAllOverlays (cegD3D11);
      }

      SK_Steam_ClearPopups ();
    }

    else
    {
      dll_log.Log ( L"[   DXGI   ]  ** Failed to acquire SwapChain's Backbuffer;"
                    L" will try again next frame." );
      SK_D3D11_ResetTexCache ();
    }
  }
}

DWORD dwRenderThread = 0x0000;

// For DXGI compliance, do not mix-and-match factory types
bool SK_DXGI_use_factory1 = false;
bool SK_DXGI_factory_init = false;

extern void  __stdcall SK_D3D11_TexCacheCheckpoint (void);
extern void  __stdcall SK_D3D11_UpdateRenderStats  (IDXGISwapChain* pSwapChain);
                                                   
extern void  __stdcall SK_D3D12_UpdateRenderStats  (IDXGISwapChain* pSwapChain);

extern BOOL __stdcall SK_NvAPI_SetFramerateLimit   (uint32_t        limit);
extern void __stdcall SK_NvAPI_SetAppFriendlyName  (const wchar_t*  wszFriendlyName);

static volatile LONG  __dxgi_ready  = FALSE;

void WaitForInitDXGI (void)
{
  if (SK_TLS_Bottom ()->d3d11.ctx_init_thread)
    return;

  // This is a hybrid spin; it will spin for up to 250 iterations before sleeping
  SK_Thread_SpinUntilFlagged (&__dxgi_ready);
}

DWORD __stdcall HookDXGI (LPVOID user);

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

const wchar_t*
SK_DXGI_DescribeSwapEffect (DXGI_SWAP_EFFECT swap_effect)
{
  switch (swap_effect)
  {
    case DXGI_SWAP_EFFECT_DISCARD:
      return    L"Discard  (BitBlt)";
    case DXGI_SWAP_EFFECT_SEQUENTIAL:
      return L"Sequential  (BitBlt)";
    case DXGI_SWAP_EFFECT_FLIP_DISCARD:
      return    L"Discard  (Flip)";
    case DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL:
      return L"Sequential  (Flip)";
    default:
      return L"UNKNOWN";
  }
}

std::wstring
SK_DXGI_DescribeSwapChainFlags (DXGI_SWAP_CHAIN_FLAG swap_flags)
{
  std::wstring out = L"";

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_NONPREROTATED)
    out += L"Non-Pre Rotated\n";

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
    out += L"Allow Fullscreen Mode Switch\n";

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE)
    out += L"GDI Compatible\n";

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT)
    out += L"DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT\n";

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER)
    out += L"DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER\n";

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
    out += L"Latency Waitable\n";

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER)
    out += L"DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER\n";

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO)
    out += L"DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO\n";

  #define DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED 1024

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED)
    out += L"DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED\n";

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
    out += L"Supports Tearing in Windowed Mode\n";

  return out;
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


extern int                      gpu_prio;

bool             bAlwaysAllowFullscreen = false;

bool bFlipMode = false;
bool bWait     = false;

// Used for integrated GPU override
int              SK_DXGI_preferred_adapter = -1;

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
#if 1
    //HANDLE hHookInitDXGI =
      CreateThread ( nullptr,
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
}

#define DXGI_STUB(_Return, _Name, _Proto, _Args)                          \
  _Return STDMETHODCALLTYPE                                               \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;          \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)GetProcAddress(backend_dll,szName);\
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"Unable to locate symbol  %s in dxgi.dll",                     \
          L#_Name);                                                       \
        return (_Return)E_NOTIMPL;                                        \
      }                                                                   \
    }                                                                     \
                                                                          \
    dll_log.Log (L"[   DXGI   ] [!] %s %s - "                             \
             L"[Calling Thread: 0x%04x]",                                 \
      L#_Name, L#_Proto, GetCurrentThreadId ());                          \
                                                                          \
    return _default_impl _Args;                                           \
}

#define DXGI_STUB_(_Name, _Proto, _Args)                                  \
  void STDMETHODCALLTYPE                                                  \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef void (STDMETHODCALLTYPE *passthrough_pfn) _Proto;             \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)GetProcAddress(backend_dll,szName);\
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"Unable to locate symbol  %s in dxgi.dll",                     \
          L#_Name );                                                      \
        return;                                                           \
      }                                                                   \
    }                                                                     \
                                                                          \
    dll_log.Log (L"[   DXGI   ] [!] %s %s - "                             \
             L"[Calling Thread: 0x%04x]",                                 \
      L#_Name, L#_Proto, GetCurrentThreadId ());                          \
                                                                          \
    _default_impl _Args;                                                  \
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

  assert (false);

  return -1;
}

std::wstring
SK_GetDXGIFactoryInterfaceEx (const IID& riid)
{
  std::wstring interface_name;

  if (riid == __uuidof (IDXGIFactory))
    interface_name = L"      IDXGIFactory";
  else if (riid == __uuidof (IDXGIFactory1))
    interface_name = L"     IDXGIFactory1";
  else if (riid == __uuidof (IDXGIFactory2))
    interface_name = L"     IDXGIFactory2";
  else if (riid == __uuidof (IDXGIFactory3))
    interface_name = L"     IDXGIFactory3";
  else if (riid == __uuidof (IDXGIFactory4))
    interface_name = L"     IDXGIFactory4";
  else if (riid == __uuidof (IDXGIFactory5))
    interface_name = L"     IDXGIFactory5";
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

    const HRESULT hr =
      static_cast <IDXGIFactory5 *>(pTemp.p)->CheckFeatureSupport (
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

  assert (false);

  return -1;
}

std::wstring
SK_GetDXGIFactoryInterface (IUnknown *pFactory)
{
  const int iver =
    SK_GetDXGIFactoryInterfaceVer (pFactory);

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

  assert (false);

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

  assert (false);

  return -1;
}

std::wstring
SK_GetDXGIAdapterInterface (IUnknown *pAdapter)
{
  const int iver =
    SK_GetDXGIAdapterInterfaceVer (pAdapter);

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
  if (cegD3D11 == (CEGUI::Direct3D11Renderer *)1)
    cegD3D11 = nullptr;

  InterlockedExchange (&__gui_reset, TRUE);
}



void
SK_DXGI_UpdateSwapChain (IDXGISwapChain* This)
{
  CComPtr <ID3D11Device> pDev = nullptr;

  if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev))) )
  {
    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    rb.device    = pDev;
    rb.swapchain = This;

    CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

    pDev->GetImmediateContext ((ID3D11DeviceContext **)&pDevCtx);
    rb.d3d11.immediate_ctx = pDevCtx;
  }
}


template <class _T>
static
__forceinline
UINT
calc_count (_T** arr, UINT max_count)
{
  for ( int i = static_cast <int> (max_count) - 1 ;
            i >= 0 ;
          --i )
  {
    if (arr [i] != 0)
      return i;
  }

  return max_count;
}

#define SK_D3D11_MAX_SCISSORS \
  D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE

struct StateBlockDataStore {
  UINT                       ScissorRectsCount, ViewportsCount;
  D3D11_RECT                 ScissorRects [SK_D3D11_MAX_SCISSORS];
  D3D11_VIEWPORT             Viewports    [SK_D3D11_MAX_SCISSORS];
  ID3D11RasterizerState*     RS;
  ID3D11BlendState*          BlendState;
  FLOAT                      BlendFactor  [4];
  UINT                       SampleMask;
  UINT                       StencilRef;
  ID3D11DepthStencilState*   DepthStencilState;
  ID3D11ShaderResourceView*  PSShaderResource;
  ID3D11SamplerState*        PSSampler;
  ID3D11PixelShader*         PS;
  ID3D11VertexShader*        VS;
  UINT                       PSInstancesCount, VSInstancesCount;
  ID3D11ClassInstance       *PSInstances  [D3D11_SHADER_MAX_INTERFACES],
                            *VSInstances  [D3D11_SHADER_MAX_INTERFACES];
  D3D11_PRIMITIVE_TOPOLOGY   PrimitiveTopology;
  ID3D11Buffer              *IndexBuffer,
                            *VertexBuffer,
                            *VSConstantBuffer;
  UINT                       IndexBufferOffset, VertexBufferStride,
                             VertexBufferOffset;
  DXGI_FORMAT                IndexBufferFormat;
  ID3D11InputLayout*         InputLayout;

  ID3D11DepthStencilView*    DepthStencilView;
  ID3D11RenderTargetView*    RenderTargetView;
};

struct SK_D3D11_Stateblock_Lite : StateBlockDataStore
{
  void capture (ID3D11DeviceContext* pCtx)
  {
    ScissorRectsCount = ViewportsCount =
      D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

    pCtx->RSGetScissorRects      (      &ScissorRectsCount, ScissorRects);
    pCtx->RSGetViewports         (      &ViewportsCount,    Viewports);
    pCtx->RSGetState             (      &RS);
    pCtx->OMGetBlendState        (      &BlendState,         BlendFactor,
                                                            &SampleMask);
    pCtx->OMGetDepthStencilState (      &DepthStencilState, &StencilRef);
    pCtx->PSGetShaderResources   (0, 1, &PSShaderResource);
    pCtx->PSGetSamplers          (0, 1, &PSSampler);

    PSInstancesCount = VSInstancesCount =
      D3D11_SHADER_MAX_INTERFACES;

    pCtx->PSGetShader            (&PS, PSInstances, &PSInstancesCount);
    pCtx->VSGetShader            (&VS, VSInstances, &VSInstancesCount);
    pCtx->VSGetConstantBuffers   (0, 1, &VSConstantBuffer);
    pCtx->IAGetPrimitiveTopology (      &PrimitiveTopology);
    pCtx->IAGetIndexBuffer       (      &IndexBuffer,  &IndexBufferFormat,
                                                       &IndexBufferOffset);
    pCtx->IAGetVertexBuffers     (0, 1, &VertexBuffer, &VertexBufferStride,
                                                       &VertexBufferOffset);
    pCtx->IAGetInputLayout       (      &InputLayout );
    pCtx->OMGetRenderTargets     (1, &RenderTargetView, &DepthStencilView);

    PSInstancesCount = calc_count (PSInstances, PSInstancesCount);
    VSInstancesCount = calc_count (VSInstances, VSInstancesCount);
  }

  void apply (ID3D11DeviceContext* pCtx)
  {
    pCtx->RSSetScissorRects      (ScissorRectsCount, ScissorRects);
    pCtx->RSSetViewports         (ViewportsCount,    Viewports);
    pCtx->OMSetDepthStencilState (DepthStencilState, StencilRef);
    pCtx->RSSetState             (RS);
    pCtx->PSSetShader            (PS, PSInstances,   PSInstancesCount);
    pCtx->VSSetShader            (VS, VSInstances,   VSInstancesCount);
    pCtx->OMSetBlendState        (BlendState,        BlendFactor,
                                                     SampleMask);
    pCtx->IASetIndexBuffer       (IndexBuffer,       IndexBufferFormat,
                                                     IndexBufferOffset);
    pCtx->IASetInputLayout       (InputLayout);
    pCtx->IASetPrimitiveTopology (PrimitiveTopology);
    pCtx->PSSetShaderResources   (0, 1, &PSShaderResource);
    pCtx->VSSetConstantBuffers   (0, 1, &VSConstantBuffer);
    pCtx->PSSetSamplers          (0, 1, &PSSampler);
    pCtx->IASetVertexBuffers     (0, 1, &VertexBuffer,    &VertexBufferStride,
                                                          &VertexBufferOffset);
    pCtx->OMSetRenderTargets     (1,    &RenderTargetView, DepthStencilView);

    //
    // Now balance the reference counts that D3D added even though we did not want them :P
    //
    for (UINT i = 0; i < VSInstancesCount; i++)
    {
      if (VSInstances [i])
          VSInstances [i]->Release ();
    }

    if (RS)                RS->Release                ();
    if (PS)                PS->Release                ();
    if (VS)                VS->Release                ();
    if (PSSampler)         PSSampler->Release         ();
    if (BlendState)        BlendState->Release        ();
    if (InputLayout)       InputLayout->Release       ();
    if (IndexBuffer)       IndexBuffer->Release       ();
    if (VertexBuffer)      VertexBuffer->Release      ();
    if (PSShaderResource)  PSShaderResource->Release  ();
    if (VSConstantBuffer)  VSConstantBuffer->Release  ();
    if (RenderTargetView)  RenderTargetView->Release  ();
    if (DepthStencilView)  DepthStencilView->Release  ();
    if (DepthStencilState) DepthStencilState->Release ();

    for (UINT i = 0; i < PSInstancesCount; i++)
    {
      if (PSInstances [i])
          PSInstances [i]->Release ();
    }
  }
};



struct D3DX11_STATE_BLOCK
{
  ID3D11VertexShader*        VS;
  ID3D11SamplerState*        VSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  VSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              VSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       VSInterfaces           [D3D11_SHADER_MAX_INTERFACES];
  UINT                       VSInterfaceCount;

  ID3D11GeometryShader*      GS;
  ID3D11SamplerState*        GSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  GSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              GSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       GSInterfaces           [D3D11_SHADER_MAX_INTERFACES];
  UINT                       GSInterfaceCount;

  ID3D11HullShader*          HS;
  ID3D11SamplerState*        HSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  HSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              HSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       HSInterfaces           [D3D11_SHADER_MAX_INTERFACES];
  UINT                       HSInterfaceCount;

  ID3D11DomainShader*        DS;
  ID3D11SamplerState*        DSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  DSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              DSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       DSInterfaces           [D3D11_SHADER_MAX_INTERFACES];
  UINT                       DSInterfaceCount;

  ID3D11PixelShader*         PS;
  ID3D11SamplerState*        PSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  PSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              PSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       PSInterfaces           [D3D11_SHADER_MAX_INTERFACES];
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
  D3D11_VIEWPORT             RSViewports            [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
  UINT                       RSScissorRectCount;
  D3D11_RECT                 RSScissorRects         [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
  ID3D11RasterizerState*     RSRasterizerState;
  ID3D11Buffer*              SOBuffers              [4];
  ID3D11Predicate*           Predication;
  BOOL                       PredicationValue;
};

void CreateStateblock (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
            CComPtr <ID3D11Device> pDev = nullptr;
                   dc->GetDevice (&pDev);
  const D3D_FEATURE_LEVEL ft_lvl = pDev->GetFeatureLevel ();

  ZeroMemory (sb, sizeof D3DX11_STATE_BLOCK);

  sb->OMBlendFactor [0] = 0.0f;
  sb->OMBlendFactor [1] = 0.0f;
  sb->OMBlendFactor [2] = 0.0f;
  sb->OMBlendFactor [3] = 0.0f;


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
  
  dc->IAGetVertexBuffers     (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,  sb->IAVertexBuffers,
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

  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    dc->SOGetTargets           (4, sb->SOBuffers);
  }

  dc->GetPredication         (&sb->Predication, &sb->PredicationValue);
}

void ApplyStateblock (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
            CComPtr <ID3D11Device> pDev = nullptr;
                   dc->GetDevice (&pDev);
  const D3D_FEATURE_LEVEL ft_lvl = pDev->GetFeatureLevel ();

  UINT minus_one [D3D11_PS_CS_UAV_REGISTER_COUNT] =
    { std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
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

    for (auto& CSUnorderedAccessView : sb->CSUnorderedAccessViews)
    {
      if (CSUnorderedAccessView != nullptr)
          CSUnorderedAccessView->Release ();
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

  if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
  {
    UINT SOBuffersOffsets [4] = {   }; /* (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 0,
                                          (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 1, 
                                          (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 2, 
                                          (sizeof(sb->SOBuffers) / sizeof(sb->SOBuffers[0])) * 3 };*/
  
    UINT SOBufferCount =
      calc_count (sb->SOBuffers, 4);
  
    if (SOBufferCount)
    {
      dc->SOSetTargets (SOBufferCount, sb->SOBuffers, SOBuffersOffsets);
  
      for (UINT i = 0; i < SOBufferCount; i++)
      {
        if (sb->SOBuffers [i] != nullptr)
        sb->SOBuffers [i]->Release ();
      }
    }
  }

  dc->SetPredication (sb->Predication, sb->PredicationValue);

  if (sb->Predication != nullptr)
    sb->Predication->Release ();
}


void
SK_CEGUI_DrawD3D11 (IDXGISwapChain* This)
{
  if (ReadAcquire (&__SK_DLL_Ending) != 0)
    return;


  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  CComPtr <ID3D11Device> pDev = nullptr;

  InterlockedIncrement (&__osd_frames_drawn);

  SK_ScopedBool auto_bool (&SK_TLS_Bottom ()->imgui.drawing);
                            SK_TLS_Bottom ()->imgui.drawing = true;


  if (InterlockedCompareExchange (&__gui_reset, FALSE, TRUE))
  {
    if ((uintptr_t)cegD3D11 > 1)
      SK_TextOverlayManager::getInstance ()->destroyAllOverlays ();

    rb.releaseOwnedResources ();

    if (cegD3D11 != nullptr)
    {
      if ((uintptr_t)cegD3D11 > 1)
      {
        CEGUI::WindowManager::getDllSingleton ().cleanDeadPool ();
        cegD3D11->destroySystem ();
      }

      cegD3D11 = nullptr;
    }
  }

  else if (cegD3D11 == nullptr)
  {
    DXGI_SWAP_CHAIN_DESC desc;
    This->GetDesc      (&desc);

    SK_InstallWindowHook (desc.OutputWindow);

    ResetCEGUI_D3D11     (This);
  }


  else if ( rb.device != nullptr && 
 SUCCEEDED (rb.device->QueryInterface <ID3D11Device> (&pDev)) )
  {
    assert (rb.device.IsEqualObject (pDev));

    // If the swapchain or device changed, bail-out and wait until the next frame for
    //   things to normalize.
    if ((! rb.device.IsEqualObject (pDev)) || rb.swapchain == nullptr)
    {
      SK_LOG0 ( ( L" D3D11 Device or Primary SwapChain Changed => "
                  L"Releasing resources..." ),
                  L"   DXGI   " );

      ResetCEGUI_D3D11        (This);
      SK_DXGI_UpdateSwapChain (This);

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

    if (FAILED (hr))
    {
      SK_LOG_ONCE (L"[   DXGI   ]  *** Back buffer unavailable! ***");
      return;
    }

    CComPtr <ID3D11DeviceContext>  pImmediateContext  = nullptr;
    CComPtr <ID3D11DeviceContext1> pImmediateContext1 = nullptr;

    rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pImmediateContext);

    if (config.render.dxgi.full_state_cache)
    {
        hr =
        pImmediateContext->QueryInterface <ID3D11DeviceContext1> (&pImmediateContext1);
                     pDev->QueryInterface <ID3D11Device1>        (&pDevice1);

        if (FAILED (hr))
        {
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
        config.render.dxgi.full_state_cache = false;
      }
    }

    D3D11_TEXTURE2D_DESC          tex2d_desc = { };
    D3D11_RENDER_TARGET_VIEW_DESC rtdesc     = { };

    pBackBuffer->GetDesc (&tex2d_desc);

    // sRGB Correction for UIs
    switch (tex2d_desc.Format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      {
        rtdesc.Format        = tex2d_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ?
                                                      DXGI_FORMAT_R8G8B8A8_UNORM :
                                                      DXGI_FORMAT_B8G8R8A8_UNORM;

        rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        hr = pDev->CreateRenderTargetView (pBackBuffer, &rtdesc, &pRenderTargetView);

        rb.framebuffer_flags |= SK_FRAMEBUFFER_FLAG_SRGB;
      } break;

      default:
      {
        hr = pDev->CreateRenderTargetView (pBackBuffer, nullptr, &pRenderTargetView);

        rb.framebuffer_flags &= (~SK_FRAMEBUFFER_FLAG_SRGB);
      } break;
    }

    if (SUCCEEDED (hr))
    {
      std::unique_ptr <SK_D3D11_Stateblock_Lite> sb (
        new SK_D3D11_Stateblock_Lite { }
      );

      sb->capture (pImmediateContext);

      pImmediateContext->OMSetRenderTargets (1, &pRenderTargetView.p, nullptr);

      D3D11_VIEWPORT         vp              = { };
      D3D11_BLEND_DESC       blend           = { };
      D3D11_TEXTURE2D_DESC   backbuffer_desc = { };

      pBackBuffer->GetDesc (&backbuffer_desc);

      blend.RenderTarget [0].BlendEnable           = TRUE;
      blend.RenderTarget [0].SrcBlend              = D3D11_BLEND_ONE;
      blend.RenderTarget [0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
      blend.RenderTarget [0].BlendOp               = D3D11_BLEND_OP_ADD;
      blend.RenderTarget [0].SrcBlendAlpha         = D3D11_BLEND_ONE;
      blend.RenderTarget [0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
      blend.RenderTarget [0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
      blend.RenderTarget [0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

      if (SUCCEEDED (pDev->CreateBlendState (&blend, &pBlendState)))
        pImmediateContext->OMSetBlendState (pBlendState, nullptr, 0xffffffff);

      vp.Width    = static_cast <float> (backbuffer_desc.Width);
      vp.Height   = static_cast <float> (backbuffer_desc.Height);
      vp.MinDepth = 0.0f;
      vp.MaxDepth = 1.0f;
      vp.TopLeftX = 0.0f;
      vp.TopLeftY = 0.0f;

      pImmediateContext->RSSetViewports (1, &vp);
      {
        if ((uintptr_t)cegD3D11 > 1)
        {
                    cegD3D11->beginRendering ();
          SK_TextOverlayManager::getInstance ()->drawAllOverlays (0.0f, 0.0f);
              CEGUI::System::getDllSingleton ().renderAllGUIContexts ();
                      cegD3D11->endRendering ();
        }

        // XXX: TODO (Full startup isn't necessary, just update framebuffer dimensions).
        if (ImGui_DX11Startup             ( This                         ))
        {
          extern DWORD SK_ImGui_DrawFrame ( DWORD dwFlags, void* user    );
                       SK_ImGui_DrawFrame (       0x00,          nullptr );
        }

        if (SK_Steam_DrawOSD ())
        {
          if ((uintptr_t)cegD3D11 > 1)
          {
                  cegD3D11->beginRendering ();
            CEGUI::System::getDllSingleton ().renderAllGUIContexts ();
                  cegD3D11->endRendering   ();
          }
        }
      }

      sb->apply (pImmediateContext);

      //
      // Update G-Sync; doing this here prevents trying to do this on frames where
      //   the swapchain was resized, which would deadlock the software.
      //
      if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
      {
        CComPtr <IDXGISurface> pBackBufferSurf = nullptr;

        if (SUCCEEDED (This->GetBuffer (0, IID_PPV_ARGS (&pBackBufferSurf))))
          NvAPI_D3D_GetObjectHandleForResource (pDev, pBackBufferSurf, &rb.surface);

        rb.gsync_state.update ();
      }
    }
  }

  // The config UI may change this, apply the setting AFTER the UI is drawn.
  extern bool SK_DXGI_FullStateCache;
  config.render.dxgi.full_state_cache = SK_DXGI_FullStateCache;
}

void
SK_DXGI_BorderCompensation (UINT& x, UINT& y)
{
  UNREFERENCED_PARAMETER (x);
  UNREFERENCED_PARAMETER (y);

#if 0
  if (! config.window.borderless)
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
#endif
}


HRESULT
STDMETHODCALLTYPE
SK_DXGI_Present ( IDXGISwapChain *This,
                  UINT            SyncInterval,
                  UINT            Flags )
{
  HRESULT hr = S_OK;

  __try                                {
    hr = Present_Original (This, SyncInterval, Flags);
  }

  // Release all resources, claim the presentation was successful and
  //   hope the game recovers. AWFUL hack, but needed for some overlays.
  //
  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                     EXCEPTION_EXECUTE_HANDLER :
                     EXCEPTION_CONTINUE_SEARCH )
  {
    SK_CEGUI_QueueResetD3D11 ();
    hr = S_OK;
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
SK_DXGI_Present1 ( IDXGISwapChain1         *This,
                   UINT                     SyncInterval,
                   UINT                     Flags,
             const DXGI_PRESENT_PARAMETERS *pPresentParameters )
{
  HRESULT hr = S_OK;

  __try                                {
    hr = Present1_Original (This, SyncInterval, Flags, pPresentParameters);
  }

  // Release all resources, claim the presentation was successful and
  //   hope the game recovers. AWFUL hack, but needed for some overlays.
  //
  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                     EXCEPTION_EXECUTE_HANDLER :
                     EXCEPTION_CONTINUE_SEARCH )
  {
    SK_CEGUI_QueueResetD3D11 ();
    hr = S_OK;
  }

  return hr;
}

static bool first_frame = true;

enum class SK_DXGI_PresentSource
{
  Wrapper = 0,
  Hook    = 1
};

bool
SK_DXGI_TestSwapChainCreationFlags (DWORD dwFlags)
{
  if ( (dwFlags & DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO)   ||
       (dwFlags & DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO)          ||
       (dwFlags & DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT) ||
       (dwFlags & DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER) )
  {
    static bool logged = false;

    if (! logged)
    {
      logged = true;

      SK_LOG0 ( ( L"Skipping SwapChain Present due to "
                    L"SwapChain Creation Flags: %x", dwFlags ),
                    L"   DXGI   " );
    }

    return false;
  }

  return true;
}

bool
SK_DXGI_TestPresentFlags (DWORD Flags)
{
  if (Flags & DXGI_PRESENT_TEST)
  {
    SK_LOG_ONCE ( L"[   DXGI   ] Skipping SwapChain Present due to "
                  L"SwapChain Present Flag (DXGI_PRESENT_TEST)" );

    return false;
  }

  return true;
}


HRESULT
STDMETHODCALLTYPE
SK_DXGI_DispatchPresent1 (IDXGISwapChain1         *This,
                          UINT                    SyncInterval,
                          UINT                    Flags,
           const DXGI_PRESENT_PARAMETERS         *pPresentParameters,
                          Present1SwapChain1_pfn  DXGISwapChain1_Present1,
                          SK_DXGI_PresentSource   Source)
{
  auto Present1 = [&](UINT                    SyncInterval,
                      UINT                    Flags,
       const DXGI_PRESENT_PARAMETERS         *pPresentParameters) ->
  HRESULT
  {
    if (Source == SK_DXGI_PresentSource::Hook)
    {
      return DXGISwapChain1_Present1 ( This,
                                         SyncInterval,
                                           Flags,
                                             pPresentParameters );
    }

    return This->Present1 (SyncInterval, Flags, pPresentParameters);
  };

  //
  // Early-out for games that use testing to minimize blocking
  //
  if (! SK_DXGI_TestPresentFlags (Flags))
  {
    return
      Present1 ( SyncInterval,
                   Flags,
                     pPresentParameters );
  }

  DXGI_SWAP_CHAIN_DESC desc = { };
       This->GetDesc (&desc);

  if (! SK_DXGI_TestSwapChainCreationFlags (desc.Flags))
  {
    return
      Present1 ( SyncInterval,
                   Flags,
                     pPresentParameters );
  }


  bool process                = false;
  bool has_wrapped_swapchains = ReadAcquire (&SK_DXGI_LiveWrappedSwapChain1s);

  if (config.render.osd.draw_in_vidcap && has_wrapped_swapchains)
    process = (Source == SK_DXGI_PresentSource::Wrapper);

  else
    process = (Source == SK_DXGI_PresentSource::Hook) || (! has_wrapped_swapchains);


  // Our vidcap hook & wrap mechanism is borked (we may need to change the setting)!
  if (config.render.osd._last_vidcap_frame < SK_GetFramesDrawn () - 1)
    config.render.osd.draw_in_vidcap = !config.render.osd.draw_in_vidcap;



  if (process || SK_GetFramesDrawn () < 2)
  {
    // Start / End / Readback Pipeline Stats
    SK_D3D11_UpdateRenderStats (This);
    SK_D3D12_UpdateRenderStats (This);

    // Establish the API used this frame (and handle possible translation layers)
    //
    switch (SK_GetDLLRole ())
    {
      case DLL_ROLE::D3D8:
        SK_GetCurrentRenderBackend ().api = SK_RenderAPI::D3D8On11;
        break;
      case DLL_ROLE::DDraw:
        SK_GetCurrentRenderBackend ().api = SK_RenderAPI::DDrawOn11;
        break;
      default:
        SK_GetCurrentRenderBackend ().api = SK_RenderAPI::D3D11;
        break;
    }

    SK_BeginBufferSwap ();

    HRESULT hr = E_FAIL;

    CComPtr <ID3D11Device> pDev = nullptr;
    This->GetDevice (IID_PPV_ARGS (&pDev));

    if (pDev && first_frame)
    {
      int hooked = 0;

      SK_D3D11_PresentFirstFrame (This);

      for ( auto& it : local_dxgi_records )
      {
        if (it->active)
        {
          SK_Hook_ResolveTarget (*it);

          // Don't cache addresses that were screwed with by other injectors
          const wchar_t* wszSection =
            StrStrIW (it->target.module_path, LR"(\sys)") ?
                                              L"DXGI.Hooks" : nullptr;

          if (! wszSection)
          {
            SK_LOG0 ( ( L"Hook for '%hs' resides in '%s', will not cache!",
                          it->target.symbol_name,
              SK_StripUserNameFromPathW (
                std::wstring (
                          it->target.module_path
                             ).data ()
              )                                                             ),
                        L"Hook Cache" );
          }
          SK_Hook_CacheTarget ( *it, wszSection );

          ++hooked;
        }
      }

      if (SK_IsInjected ())
      {
        auto it_local  = std::begin (local_dxgi_records);
        auto it_global = std::begin (global_dxgi_records);

        while ( it_local != std::end (local_dxgi_records) )
        {
          if (( *it_local )->hits &&
    StrStrIW (( *it_local )->target.module_path, LR"(\sys)") &&
              ( *it_local )->active)
            SK_Hook_PushLocalCacheOntoGlobal ( **it_local,
                                                 **it_global );
          else
          {
            ( *it_global )->target.addr = nullptr;
            ( *it_global )->hits        = 0;
            ( *it_global )->active      = false;
          }

          it_global++, it_local++;
        }

        SK_D3D11_UpdateHookAddressCache ();
      }

      if (hooked > 0)
        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());

#ifdef _WIN64
      switch (SK_GetCurrentGameID ())
      {
        case SK_GAME_ID::Fallout4:
          SK_FO4_PresentFirstFrame (This, SyncInterval, Flags);
          break;

        case SK_GAME_ID::DarkSouls3:
          SK_DS3_PresentFirstFrame (This, SyncInterval, Flags);
          break;

        case SK_GAME_ID::NieRAutomata:
          SK_FAR_PresentFirstFrame (This, SyncInterval, Flags);
          break;

        case SK_GAME_ID::BlueReflection:
          SK_IT_PresentFirstFrame (This, SyncInterval, Flags);
          break;

        case SK_GAME_ID::DotHackGU:
          SK_DGPU_PresentFirstFrame (This, SyncInterval, Flags);
          break;

        case SK_GAME_ID::WorldOfFinalFantasy:
        {
          SK_DeferCommand ("Window.Borderless toggle");
          SleepEx (33, FALSE);
          SK_DeferCommand ("Window.Borderless toggle");
        } break;
      }
#endif

      // TODO: Clean this code up
      CComPtr <IDXGIDevice>  pDevDXGI = nullptr;
      CComPtr <IDXGIAdapter> pAdapter = nullptr;
      CComPtr <IDXGIFactory> pFactory = nullptr;

      if ( SUCCEEDED (pDev->QueryInterface <IDXGIDevice> (&pDevDXGI)) &&
           SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter)) &&
           SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory))) )
      {
        SK_GetCurrentRenderBackend ().device    = pDev;
        SK_GetCurrentRenderBackend ().swapchain = This;

        //if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
        //  NvAPI_D3D_GetObjectHandleForResource (pDev, This, &SK_GetCurrentRenderBackend ().surface);


        if (config.render.dxgi.safe_fullscreen) pFactory->MakeWindowAssociation ( nullptr, 0 );
        
        
        if (bAlwaysAllowFullscreen)
          pFactory->MakeWindowAssociation (desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES);
      }

      SK_DXGI_HookSwapChain (This);

      first_frame = false;
    }

    int interval = config.render.framerate.present_interval;

    if ( config.render.framerate.flip_discard && config.render.dxgi.allow_tearing )
    {
      if (desc.Windowed)
      {
        Flags       |= DXGI_PRESENT_ALLOW_TEARING;
        SyncInterval = 0;
        interval     = 0;
      }
    }

    int flags    = Flags;

    // Application preference
    if (interval == -1)
      interval = SyncInterval;

    SK_GetCurrentRenderBackend ().present_interval = interval;

    if (bFlipMode)
    {
      flags |= DXGI_PRESENT_USE_DURATION | DXGI_PRESENT_RESTART;

      if (bWait)
        flags |= DXGI_PRESENT_DO_NOT_WAIT;
    }

    // Test first, then do (if test_present is true)
    hr = config.render.dxgi.test_present ? 
           Present1 (0, Flags | DXGI_PRESENT_TEST, pPresentParameters) :
           S_OK;

    const bool can_present =
      SUCCEEDED (hr) || hr == DXGI_STATUS_OCCLUDED;

    if (can_present)
    {
      SK_CEGUI_DrawD3D11 (This);
      hr =
        Present1 (interval, flags, pPresentParameters);
    }

    else
    {
      dll_log.Log ( L"[   DXGI   ] *** IDXGISwapChain1::Present1 (...) "
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

    if (bWait)
    {
      CComQIPtr <IDXGISwapChain2> pSwapChain2 (This);

      if (pSwapChain2 != nullptr)
      {
        if (pSwapChain2 != nullptr)
        {
          CHandle hWait (pSwapChain2->GetFrameLatencyWaitableObject ());

          MsgWaitForMultipleObjectsEx ( 1,
                                          &hWait.m_h,
                                            config.render.framerate.swapchain_wait,
                                              QS_ALLEVENTS, MWMO_ALERTABLE );
        }
      }
    }

    if ( pDev != nullptr )
    {
      HRESULT ret =
        SK_EndBufferSwap (hr, pDev);

      SK_D3D11_TexCacheCheckpoint ();

      return ret;
    }

    // Not a D3D11 device -- weird...
    return SK_EndBufferSwap (hr);
  }

  else
  {
    HRESULT hr =
      Present1 (SyncInterval, Flags, pPresentParameters);

    return hr;
  }
}

HRESULT
  STDMETHODCALLTYPE Present1Callback (IDXGISwapChain1         *This,
                                      UINT                     SyncInterval,
                                      UINT                     Flags,
                                const DXGI_PRESENT_PARAMETERS *pPresentParameters)
{
  SK_LOG_ONCE (L"Present1");

  return
    SK_DXGI_DispatchPresent1 ( This, SyncInterval, Flags, pPresentParameters,
                               SK_DXGI_Present1, SK_DXGI_PresentSource::Hook );
}

HRESULT
STDMETHODCALLTYPE
SK_DXGI_DispatchPresent (IDXGISwapChain        *This,
                         UINT                   SyncInterval,
                         UINT                   Flags,
                         PresentSwapChain_pfn   DXGISwapChain_Present,
                         SK_DXGI_PresentSource  Source)
{
  auto Present = [&](UINT                    SyncInterval,
                     UINT                    Flags) ->
  HRESULT
  {
    if (Source == SK_DXGI_PresentSource::Hook)
    {
      return DXGISwapChain_Present ( This,
                                       SyncInterval,
                                         Flags );
    }

    return This->Present (SyncInterval, Flags);
  };


  //
  // Early-out for games that use testing to minimize blocking
  //
  if (! SK_DXGI_TestPresentFlags (Flags))
    return Present (SyncInterval, Flags);

  DXGI_SWAP_CHAIN_DESC desc = { };
       This->GetDesc (&desc);

  if (! SK_DXGI_TestSwapChainCreationFlags (desc.Flags))
  {
    return Present (SyncInterval, Flags);
  }


  bool process                = false;
  bool has_wrapped_swapchains = ReadAcquire (&SK_DXGI_LiveWrappedSwapChain1s) ||
                                ReadAcquire (&SK_DXGI_LiveWrappedSwapChains);
  // ^^^ It's not required that IDXGISwapChain1 or higher invokes Present1!

  if (config.render.osd.draw_in_vidcap && has_wrapped_swapchains)
    process = (Source == SK_DXGI_PresentSource::Wrapper);

  else
    process = (Source == SK_DXGI_PresentSource::Hook) || (! has_wrapped_swapchains);


  if (process || SK_GetFramesDrawn () < 2)
  {
    // Start / End / Readback Pipeline Stats
    SK_D3D11_UpdateRenderStats (This);
    SK_D3D12_UpdateRenderStats (This);

    // Establish the API used this frame (and handle possible translation layers)
    //
    switch (SK_GetDLLRole ())
    {
      case DLL_ROLE::D3D8:
        SK_GetCurrentRenderBackend ().api = SK_RenderAPI::D3D8On11;
        break;
      case DLL_ROLE::DDraw:
        SK_GetCurrentRenderBackend ().api = SK_RenderAPI::DDrawOn11;
        break;
      default:
        SK_GetCurrentRenderBackend ().api = SK_RenderAPI::D3D11;
        break;
    }

    SK_BeginBufferSwap ();

    HRESULT hr = E_FAIL;

    CComPtr <ID3D11Device> pDev = nullptr;
    This->GetDevice (IID_PPV_ARGS (&pDev));

    if (pDev && first_frame)
    {
      int hooked = 0;

      SK_D3D11_PresentFirstFrame (This);

      for ( auto& it : local_dxgi_records )
      {
        if (it->active)
        {
          SK_Hook_ResolveTarget (*it);

          // Don't cache addresses that were screwed with by other injectors
          const wchar_t* wszSection =
            StrStrIW (it->target.module_path, LR"(\sys)") ?
                                              L"DXGI.Hooks" : nullptr;

          if (! wszSection)
          {
            SK_LOG0 ( ( L"Hook for '%hs' resides in '%s', will not cache!",
                          it->target.symbol_name,
              SK_StripUserNameFromPathW (
                std::wstring (
                          it->target.module_path
                             ).data ()
              )                                                             ),
                        L"Hook Cache" );
          }
          SK_Hook_CacheTarget ( *it, wszSection );

          ++hooked;
        }
      }

      if (SK_IsInjected ())
      {
        auto it_local  = std::begin (local_dxgi_records);
        auto it_global = std::begin (global_dxgi_records);

        while ( it_local != std::end (local_dxgi_records) )
        {
          if (( *it_local )->hits &&
    StrStrIW (( *it_local )->target.module_path, LR"(\sys)") &&
              ( *it_local )->active)
            SK_Hook_PushLocalCacheOntoGlobal ( **it_local,
                                                 **it_global );
          else
          {
            ( *it_global )->target.addr = nullptr;
            ( *it_global )->hits        = 0;
            ( *it_global )->active      = false;
          }

          it_global++, it_local++;
        }

        SK_D3D11_UpdateHookAddressCache ();
      }

      if (hooked > 0)
        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());

#ifdef _WIN64
      switch (SK_GetCurrentGameID ())
      {
        case SK_GAME_ID::Fallout4:
          SK_FO4_PresentFirstFrame (This, SyncInterval, Flags);
          break;
  
        case SK_GAME_ID::DarkSouls3:
          SK_DS3_PresentFirstFrame (This, SyncInterval, Flags);
          break;
  
        case SK_GAME_ID::NieRAutomata:
          SK_FAR_PresentFirstFrame (This, SyncInterval, Flags);
          break;
  
        case SK_GAME_ID::BlueReflection:
          SK_IT_PresentFirstFrame (This, SyncInterval, Flags);
          break;
  
        case SK_GAME_ID::DotHackGU:
          SK_DGPU_PresentFirstFrame (This, SyncInterval, Flags);
          break;
  
        case SK_GAME_ID::WorldOfFinalFantasy:
        {  
          SK_DeferCommand ("Window.Borderless toggle");
          SleepEx (33, FALSE);
          SK_DeferCommand ("Window.Borderless toggle");
        } break;
      }
#endif

      // TODO: Clean this code up
      CComPtr <IDXGIDevice>  pDevDXGI = nullptr;
      CComPtr <IDXGIAdapter> pAdapter = nullptr;
      CComPtr <IDXGIFactory> pFactory = nullptr;
  
      if ( SUCCEEDED (pDev->QueryInterface <IDXGIDevice> (&pDevDXGI)) &&
           SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter)) &&
           SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory))) )
      {
        SK_GetCurrentRenderBackend ().device    = pDev;
        SK_GetCurrentRenderBackend ().swapchain = This;
  
        //if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
        //  NvAPI_D3D_GetObjectHandleForResource (pDev, This, &SK_GetCurrentRenderBackend ().surface);
  
  
        if (config.render.dxgi.safe_fullscreen) pFactory->MakeWindowAssociation ( nullptr, 0 );
        
        
        if (bAlwaysAllowFullscreen)
          pFactory->MakeWindowAssociation (desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES);
      }
  
      first_frame = false;
    }
  
    int interval = config.render.framerate.present_interval;
  
    if ( config.render.framerate.flip_discard && config.render.dxgi.allow_tearing )
    {
      if (desc.Windowed)
      {
        Flags       |= DXGI_PRESENT_ALLOW_TEARING;
        SyncInterval = 0;
        interval     = 0;
      }
    }
  
    int flags    = Flags;
  
    // Application preference
    if (interval == -1)
      interval = SyncInterval;
  
    SK_GetCurrentRenderBackend ().present_interval = interval;
  
    if (bFlipMode)
    {
      flags |= DXGI_PRESENT_USE_DURATION | DXGI_PRESENT_RESTART;
  
      if (bWait)
        flags |= DXGI_PRESENT_DO_NOT_WAIT;
    }
  
    // Test first, then do (if test_present is true)
    hr = config.render.dxgi.test_present ? 
           Present (0, Flags | DXGI_PRESENT_TEST) :
           S_OK;
  
    const bool can_present =
      SUCCEEDED (hr) || hr == DXGI_STATUS_OCCLUDED;
  
    if (! bFlipMode)
    {
      if (can_present)
      {
             SK_CEGUI_DrawD3D11 (This);
        hr = Present            (interval, flags);
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
      DXGI_PRESENT_PARAMETERS     pparams      = { };
      CComQIPtr <IDXGISwapChain1> pSwapChain1 (This);
  
      if (pSwapChain1 != nullptr)
      {
        if (can_present)
        {
               SK_CEGUI_DrawD3D11 (This);
          hr = Present            (interval, flags);
        }
      }

      else
      {
        // Fallback for something that will probably only ever happen on Windows 7.
        hr = Present (interval, Flags);
      }
    }

    if (bWait)
    {
      CComQIPtr <IDXGISwapChain2> pSwapChain2 (This);
  
      if (pSwapChain2 != nullptr)
      {
        if (pSwapChain2 != nullptr)
        {
          CHandle hWait (pSwapChain2->GetFrameLatencyWaitableObject ());
  
          MsgWaitForMultipleObjectsEx ( 1,
                                          &hWait.m_h,
                                            config.render.framerate.swapchain_wait,
                                              QS_ALLEVENTS, MWMO_ALERTABLE );
        }
      }
    }
  
    if ( pDev != nullptr )
    {
      HRESULT ret =
        SK_EndBufferSwap (hr, pDev);
  
      SK_D3D11_TexCacheCheckpoint ();
  
      return ret;
    }
  
    // Not a D3D11 device -- weird...
    return SK_EndBufferSwap (hr);
  }

  HRESULT hr =
    Present (SyncInterval, Flags);

  config.render.osd._last_vidcap_frame = SK_GetFramesDrawn ();

  return hr;
}


__declspec (noinline)
HRESULT
STDMETHODCALLTYPE PresentCallback ( IDXGISwapChain *This,
                                    UINT            SyncInterval,
                                    UINT            Flags )
{
  return
    SK_DXGI_DispatchPresent ( This, SyncInterval, Flags,
                              SK_DXGI_Present, SK_DXGI_PresentSource::Hook );
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
  if (pDesc != nullptr)
  {
    DXGI_LOG_CALL_I5 ( L"       IDXGIOutput", L"GetDisplayModeList         ",
                         L"%08" PRIxPTR L"h, %i, %02x, NumModes=%lu, %08"
                                PRIxPTR L"h)",
                (uintptr_t)This,
                           EnumFormat,
                               Flags,
                                 *pNumModes,
                                    (uintptr_t)pDesc );
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
      reinterpret_cast <DXGI_MODE_DESC *>(
        new uint8_t [sizeof (DXGI_MODE_DESC) * *pNumModes] { }
      );
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
      if ( config.render.dxgi.scaling_mode != DXGI_MODE_SCALING_UNSPECIFIED &&
           config.render.dxgi.scaling_mode != DXGI_MODE_SCALING_CENTERED )
      {
        for ( INT i = static_cast <INT> (*pNumModes) - 1 ; i >= 0 ; --i )
        {
          if ( pDesc [i].Scaling != DXGI_MODE_SCALING_UNSPECIFIED &&
               pDesc [i].Scaling != DXGI_MODE_SCALING_CENTERED    &&
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
    delete [] pDescLocal;

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
  DXGI_LOG_CALL_I4 ( L"       IDXGIOutput", L"FindClosestMatchingMode         ",
                       L"%p, %08" PRIxPTR L"h, %08" PRIxPTR L"h, %08"
                                  PRIxPTR L"h",
      This, (uintptr_t)pModeToMatch, (uintptr_t)pClosestMatch,
            (uintptr_t)pConcernedDevice );

  SK_LOG0 ( (L"[?]  Desired Mode:  %lux%lu@%.2f Hz, Format=%s, Scaling=%s, Scanlines=%s",
               pModeToMatch->Width, pModeToMatch->Height,
                 pModeToMatch->RefreshRate.Denominator != 0 ?
                   static_cast <float> (pModeToMatch->RefreshRate.Numerator) /
                   static_cast <float> (pModeToMatch->RefreshRate.Denominator) :
                     std::numeric_limits <float>::quiet_NaN (),
               SK_DXGI_FormatToStr           (pModeToMatch->Format).c_str (),
               SK_DXGI_DescribeScalingMode   (pModeToMatch->Scaling),
               SK_DXGI_DescribeScanlineOrder (pModeToMatch->ScanlineOrdering) ),
             L"   DXGI   " );


  DXGI_MODE_DESC mode_to_match = *pModeToMatch;

  if ( config.render.framerate.refresh_rate != -1 &&
       mode_to_match.RefreshRate.Numerator  != (UINT)config.render.framerate.refresh_rate )
  {
    dll_log.Log ( L"[   DXGI   ]  >> Refresh Override "
                  L"(Requested: %f, Using: %li)",
                    mode_to_match.RefreshRate.Denominator != 0 ?
                      static_cast <float> (mode_to_match.RefreshRate.Numerator) /
                      static_cast <float> (mode_to_match.RefreshRate.Denominator) :
                        std::numeric_limits <float>::quiet_NaN (),
                      config.render.framerate.refresh_rate
                );

    mode_to_match.RefreshRate.Numerator   = (UINT)config.render.framerate.refresh_rate;
    mode_to_match.RefreshRate.Denominator =       1;
  }

  if ( config.render.dxgi.scaling_mode != -1 &&
       mode_to_match.Scaling           != config.render.dxgi.scaling_mode &&
                       (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode != DXGI_MODE_SCALING_CENTERED )
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

  pModeToMatch = &mode_to_match;


  HRESULT ret;
  DXGI_CALL (ret, FindClosestMatchingMode_Original (This, pModeToMatch, pClosestMatch, pConcernedDevice));

  SK_LOG0 ( ( L"[#]  Closest Match: %lux%lu@%.2f Hz, Format=%s, Scaling=%s, Scanlines=%s",
              pClosestMatch->Width, pClosestMatch->Height,
                pClosestMatch->RefreshRate.Denominator != 0 ?
                  static_cast <float> (pClosestMatch->RefreshRate.Numerator) /
                  static_cast <float> (pClosestMatch->RefreshRate.Denominator) :
                    std::numeric_limits <float>::quiet_NaN (),
              SK_DXGI_FormatToStr           (pClosestMatch->Format).c_str (),
              SK_DXGI_DescribeScalingMode   (pClosestMatch->Scaling),
              SK_DXGI_DescribeScanlineOrder (pClosestMatch->ScanlineOrdering) ),
            L"   DXGI   " );

  return ret;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGIOutput_WaitForVBlank_Override ( IDXGIOutput *This )
{
  //DXGI_LOG_CALL_I0 (L"       IDXGIOutput", L"WaitForVBlank         ");

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

#include <shellapi.h>
#include <SpecialK/utility.h>

// Classifies a swapchain as dummy (used by some libraries during init) or
//   real (potentially used to do actual rendering).
// Classifies a swapchain as dummy (used by some libraries during init) or
//   real (potentially used to do actual rendering).
bool
SK_DXGI_IsSwapChainReal (DXGI_SWAP_CHAIN_DESC& desc)
{
  // 0x0 is implicitly sized to match its HWND's client rect,
  //
  //   => Anything ( 1x1 - 31x31 ) has no practical application.
  //
  if (desc.BufferDesc.Height > 0 && desc.BufferDesc.Height < 32)
    return false;
  if (desc.BufferDesc.Width > 0  && desc.BufferDesc.Width  < 32)
    return false;

  return true;
}

bool
SK_DXGI_IsSwapChainReal (IDXGISwapChain* pSwapChain)
{
  if (! pSwapChain)
    return false;

  DXGI_SWAP_CHAIN_DESC       desc = { };
       pSwapChain->GetDesc (&desc);

  return SK_DXGI_IsSwapChainReal (desc);
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGISwap_SetFullscreenState_Override ( IDXGISwapChain *This,
                                       BOOL            Fullscreen,
                                       IDXGIOutput    *pTarget )
{
  DXGI_LOG_CALL_I2 ( L"    IDXGISwapChain", L"SetFullscreenState         ",
                     L"%s, %08" PRIxPTR L"h",
                      Fullscreen ? L"{ Fullscreen }" :
                                   L"{ Windowed }",   (uintptr_t)pTarget );

  SK_CEGUI_QueueResetD3D11 ();


  extern void
  SK_COMPAT_FixUpFullscreen_DXGI (bool Fullscreen);

  SK_COMPAT_FixUpFullscreen_DXGI (Fullscreen);


  // Dummy init swapchains (i.e. 1x1 pixel) should not have
  //   override parameters applied or it's usually fatal.
  bool no_override = (! SK_DXGI_IsSwapChainReal (This));


  // If auto-update prompt is visible, don't go fullscreen.
  extern          HWND hWndUpdateDlg;
  extern volatile LONG __SK_TaskDialogActive;
  if ( hWndUpdateDlg != static_cast <HWND> (INVALID_HANDLE_VALUE) ||
                               ReadAcquire (&__SK_TaskDialogActive) )
  {
    Fullscreen = false;
  }



  if (! no_override)
  {
    if (config.render.framerate.swapchain_wait != 0)
    {
      dll_log.Log ( L"[ DXGI 1.2 ]  >> Waitable SwapChain In Use, Skipping..." );
      return S_OK;
    }

    if ( config.render.framerate.flip_discard && dxgi_caps.swapchain.allow_tearing )
    {
      Fullscreen = FALSE;
      dll_log.Log ( L"[ DXGI 1.5 ]  >> Tearing Override:  Enable" );
      pTarget = nullptr;
    }

    if (config.display.force_fullscreen && Fullscreen == FALSE)
    {
      Fullscreen = TRUE;
      dll_log.Log ( L"[   DXGI   ]  >> Display Override "
                    L"(Requested: Windowed, Using: Fullscreen)" );
    }
    else if (config.display.force_windowed && Fullscreen == TRUE)
    {
      Fullscreen = FALSE;
      dll_log.Log ( L"[   DXGI   ]  >> Display Override "
                    L"(Requested: Fullscreen, Using: Windowed)" );
    }

    if (request_mode_change == mode_change_request_e::Fullscreen && Fullscreen == FALSE)
    {
      dll_log.Log ( L"[   DXGI   ]  >> Display Override "
              L"User Initiated Fulllscreen Switch" );
      Fullscreen = TRUE;
    }
    else if (request_mode_change == mode_change_request_e::Windowed && Fullscreen == TRUE)
    {
      dll_log.Log ( L"[   DXGI   ]  >> Display Override "
              L"User Initiated Windowed Switch" );
      Fullscreen = FALSE;
    }
  }

  HRESULT    ret;
  DXGI_CALL (ret, SetFullscreenState_Original (This, Fullscreen, pTarget));

  //
  // Necessary provisions for Fullscreen Flip Mode
  //
  if (SUCCEEDED (ret))
  {
    if (bFlipMode)
    {
      // Steam Overlay does not like this, even though for compliance sake we are supposed to do it :(
      if (Fullscreen)
      {
        ResizeBuffers_Original ( This, 0, 0, 0, DXGI_FORMAT_UNKNOWN,
                                   DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH );
      }
    }

    DXGI_SWAP_CHAIN_DESC desc = { };

    ///ResizeBuffers_Original (This, desc.BufferCount, desc.BufferDesc.Width,
    ///                          desc.BufferDesc.Height, desc.BufferDesc.Format, desc.Flags);
    if (SUCCEEDED (This->GetDesc (&desc)))
    {
      if (desc.BufferDesc.Width != 0)
      {
        SK_SetWindowResX (desc.BufferDesc.Width);
        SK_SetWindowResY (desc.BufferDesc.Height);
      }

      else
      {
        RECT                               client = { };
        GetClientRect (desc.OutputWindow, &client);

        SK_SetWindowResX (client.right  - client.left);
        SK_SetWindowResY (client.bottom - client.top);
      }
    }

    SK_GetCurrentRenderBackend ().fullscreen_exclusive = Fullscreen;
  }

  return ret;
}


HRESULT
SK_DXGI_ValidateSwapChainResize ( IDXGISwapChain* pSwapChain, UINT BufferCount,
                                  UINT            Width,      UINT Height,
                                  DXGI_FORMAT     Format,     INT  Fullscreen = -1 )
{
  static CRITICAL_SECTION cs_resize = { };
  static volatile LONG    init      = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    InitializeCriticalSectionAndSpinCount (&cs_resize, 1024UL);

    InterlockedExchange (&init, -1);
  }

  while (ReadAcquire (&init) != -1)
    MsgWaitForMultipleObjectsEx (0, nullptr, 2UL, QS_ALLINPUT, MWMO_ALERTABLE);


  EnterCriticalSection (&cs_resize);

  static std::unordered_map <IDXGISwapChain *, UINT>        last_width;
  static std::unordered_map <IDXGISwapChain *, UINT>        last_height;
  static std::unordered_map <IDXGISwapChain *, UINT>        last_buffers;
  static std::unordered_map <IDXGISwapChain *, DXGI_FORMAT> last_format;
  static std::unordered_map <IDXGISwapChain *, BOOL>        last_fullscreen;


  DXGI_SWAP_CHAIN_DESC desc     = { };
  RECT                 rcClient = { };

  pSwapChain->GetDesc (&desc);
         GetClientRect (desc.OutputWindow, &rcClient);

  //
  //  Resolve default values for any optional parameters (assigned 0)
  //
  if (Width  == 0) Width  = ( rcClient.right  - rcClient.left );
  if (Height == 0) Height = ( rcClient.bottom - rcClient.top  );

  if (last_format.count (pSwapChain) && Format == DXGI_FORMAT_UNKNOWN)
    Format      = last_format  [pSwapChain];

  if (last_buffers.count (pSwapChain) && BufferCount == 0)
    BufferCount = last_buffers [pSwapChain];


  //
  // Get the current Fullscreen state if it wasn't provided
  //
  if (Fullscreen == -1) Fullscreen = desc.Windowed ? FALSE : TRUE;


  bool skip = true;


  //
  //  Test to make sure something actually changed
  //
  if ((! last_format.count       (pSwapChain)) || last_format  [pSwapChain] != Format)
  {
    if (last_format.count (pSwapChain))
    {
      if (Format != last_format [pSwapChain])
        skip = false;
    }

    else
    {
      if  (desc.BufferDesc.Format != Format)
        skip = false;
    }
  }

  else if ((! last_width.count   (pSwapChain)) || last_width   [pSwapChain] != Width)
  {
    if (last_width.count (pSwapChain))
    {
      if (Width != last_width [pSwapChain])
        skip = false;
    }

    else
    {
      if  (desc.BufferDesc.Width != Width)
        skip = false;
    }
  }

  else if ((! last_height.count  (pSwapChain)) || last_height  [pSwapChain] != Height)
  {
    if (last_height.count (pSwapChain))
    {
      if (Height != last_height [pSwapChain])
        skip = false;
    }

    else
    {
      if  (desc.BufferDesc.Height != Height)
        skip = false;
    }
  }

  else if ((! last_buffers.count (pSwapChain)) || last_buffers [pSwapChain] != BufferCount)
  {
    if (last_buffers.count (pSwapChain))
    {
      if (BufferCount != last_buffers [pSwapChain])
        skip = false;
    }

    else
    {
      if  (desc.BufferCount != BufferCount)
        skip = false;
    }
  }

  else if ((! last_fullscreen.count (pSwapChain)) || last_fullscreen [pSwapChain] != Fullscreen)
  {
    if (last_fullscreen.count (pSwapChain))
    {
      if (Fullscreen != last_fullscreen [pSwapChain])
        skip = false;
    }

    else
    {
      if (desc.Windowed == Fullscreen)
        skip = false;
    }
  }


  last_width      [pSwapChain] = Width;
  last_height     [pSwapChain] = Height;
  last_buffers    [pSwapChain] = BufferCount;
  last_format     [pSwapChain] = Format;
  last_fullscreen [pSwapChain] = Fullscreen;

  LeaveCriticalSection (&cs_resize);

  if (skip)
    return S_OK;

  return E_UNEXPECTED;
}


bool
SK_DXGI_FilterRedundant_ResizeBuffers ( IDXGISwapChain *This,
                                   _In_ UINT            BufferCount,
                                   _In_ UINT            Width,
                                   _In_ UINT            Height,
                                   _In_ DXGI_FORMAT     NewFormat,
                                   _In_ UINT            SwapChainFlags )
{
  #ifndef _DEBUG
    return false;
  #endif

  DXGI_SWAP_CHAIN_DESC desc = { };
       This->GetDesc (&desc);

  if ( desc.BufferCount       == BufferCount &&
       desc.BufferDesc.Width  == Width       &&
       desc.BufferDesc.Height == Height      &&
       desc.BufferDesc.Format == NewFormat )
  {
    if (desc.Flags != SwapChainFlags)
    {
      if ( (desc.Flags     & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH) !=
           (SwapChainFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH) )
      {
        return true;
      }

      dll_log.Log ( L"[   DXGI   ] _ALMOST_ Redundant resize; OLD Flags: %x,"
                                                           L" NEW Flags: %x",
                      desc.Flags,
                        SwapChainFlags );

      return false;
    }

    return true;
  }

  return false;
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
  if (SK_DXGI_FilterRedundant_ResizeBuffers ( This, BufferCount, Width,
                                                Height, NewFormat,
                                                  SwapChainFlags )
     )
  {
    DXGI_SWAP_CHAIN_DESC desc = { };
         This->GetDesc (&desc);

    SK_LOG0 ((L"Eliminated redundant ResizeBuffers (...) call"),L"   DXGI   ");
    SK_LOG0 ((L" >> (%lux%lu*%lu {%lu} 0x%x) ==> (%lux%lu*%lu {%lu} 0x%x)",
                desc.BufferDesc.Width, desc.BufferDesc.Height, desc.BufferCount,
                desc.BufferDesc.Format, desc.Flags,
                  Width, Height, BufferCount, NewFormat, SwapChainFlags),
              L"   DXGI   ");

    return S_OK;//ResizeBuffers_Original (This, 0, 0, 0, (DXGI_FORMAT)0, 0);
  }


  DXGI_LOG_CALL_I5 ( L"    IDXGISwapChain", L"ResizeBuffers         ",
                       L"%lu,%lu,%lu,fmt=%lu,0x%08X",
                         BufferCount, Width, Height,
                   (UINT)NewFormat, SwapChainFlags );


  // Can't do this if waitable
  if (dxgi_caps.present.waitable && config.render.framerate.swapchain_wait > 0)
    return S_OK;

  SK_CEGUI_QueueResetD3D11 (); // Prior to the next present, reset the UI


  if (       config.render.framerate.buffer_count != -1           &&
       (UINT)config.render.framerate.buffer_count !=  BufferCount &&
       BufferCount                          !=  0 )
  {
    BufferCount = config.render.framerate.buffer_count;
    dll_log.Log (L"[   DXGI   ]  >> Buffer Count Override: %lu buffers", BufferCount);
  }

  if (config.render.framerate.flip_discard && dxgi_caps.swapchain.allow_tearing)
  {
    SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    dll_log.Log ( L"[ DXGI 1.5 ]  >> Tearing Option:  Enable" );
  }




  if (! config.window.res.override.isZero ())
  {
    Width  = config.window.res.override.x;
    Height = config.window.res.override.y;
  }

  // TODO: Something if Fullscreen
  else if (config.window.borderless && (! config.window.fullscreen))
  {
    SK_DXGI_BorderCompensation (
      Width,
        Height
    );
  }


  if ((! config.render.dxgi.res.max.isZero ()) && Width > config.render.dxgi.res.max.x)
    Width = config.render.dxgi.res.max.x;
  if ((! config.render.dxgi.res.max.isZero ()) && Height > config.render.dxgi.res.max.y)
    Height = config.render.dxgi.res.max.y;

  if ((! config.render.dxgi.res.min.isZero ()) && Width < config.render.dxgi.res.min.x)
    Width = config.render.dxgi.res.min.x;
  if ((! config.render.dxgi.res.min.isZero ()) && Height < config.render.dxgi.res.min.y)
    Height = config.render.dxgi.res.min.y;


  if (SK_GetCurrentGameID () == SK_GAME_ID::DotHackGU)
    NewFormat = DXGI_FORMAT_B8G8R8A8_UNORM;


  HRESULT     ret;
  DXGI_CALL ( ret, ResizeBuffers_Original ( This, BufferCount, Width, Height,
                                              NewFormat, SwapChainFlags ) );

  if (SUCCEEDED (ret))
  {
    if (Width != 0 && Height != 0)
    {
      SK_SetWindowResX (Width);
      SK_SetWindowResY (Height);
    }

    else
    {
      RECT                              client = { };
      GetClientRect (game_window.hWnd, &client);

      SK_SetWindowResX (client.right  - client.left);
      SK_SetWindowResY (client.bottom - client.top);
    }

    //SK_RunOnce (SK_DXGI_HookPresent (This));
    //SK_RunOnce (SK_ApplyQueuedHooks (    ));
  }

  return ret;
}


__forceinline
bool
SK_DXGI_FilterRedundant_ResizeTarget ( IDXGISwapChain *This,
                            _In_ const DXGI_MODE_DESC *pNewTargetParameters )
{
#ifndef _DEBUG
  return false;
#endif

  DXGI_SWAP_CHAIN_DESC desc = { };
       This->GetDesc (&desc);

  if ( desc.BufferDesc.Format                  == pNewTargetParameters->Format                  &&
       desc.BufferDesc.Height                  == pNewTargetParameters->Height                  &&
       desc.BufferDesc.Width                   == pNewTargetParameters->Width                   &&
       desc.BufferDesc.RefreshRate.Numerator   == pNewTargetParameters->RefreshRate.Numerator   &&
       desc.BufferDesc.RefreshRate.Denominator == pNewTargetParameters->RefreshRate.Denominator &&
       desc.BufferDesc.Scaling                 == pNewTargetParameters->Scaling )
       //desc.BufferDesc.ScanlineOrdering        == pNewTargetParameters->ScanlineOrdering )
  {
    return true;
  }

  return false;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGISwap_ResizeTarget_Override ( IDXGISwapChain *This,
                      _In_ const DXGI_MODE_DESC *pNewTargetParameters )
{
  if ( SK_DXGI_FilterRedundant_ResizeTarget ( This, pNewTargetParameters ) )
  {
    SK_LOG_ONCE (L"[   DXGI   ] Eliminated redundant ResizeTarget (...) call");
    return S_OK;
  }

  // Can't do this if waitable
  if (dxgi_caps.present.waitable && config.render.framerate.swapchain_wait > 0)
    return S_OK;

  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

    SK_D3D11_EndFrame        ();
    SK_CEGUI_QueueResetD3D11 (); // Prior to the next present, reset the UI
  }

  assert (pNewTargetParameters != nullptr);

  DXGI_LOG_CALL_I6 ( L"    IDXGISwapChain", L"ResizeTarget         ",
                       L"{ (%lux%lu@%3.1f Hz),"
                       L"fmt=%lu,scaling=0x%02x,scanlines=0x%02x }",
                          pNewTargetParameters->Width, pNewTargetParameters->Height,
                          pNewTargetParameters->RefreshRate.Denominator != 0 ?
           static_cast <float> (pNewTargetParameters->RefreshRate.Numerator) /
           static_cast <float> (pNewTargetParameters->RefreshRate.Denominator) :
                              std::numeric_limits <float>::quiet_NaN (),
            static_cast <UINT> (pNewTargetParameters->Format),
                                pNewTargetParameters->Scaling,
                                pNewTargetParameters->ScanlineOrdering );

  HRESULT ret;

  if ( config.window.borderless ||
       ( config.render.dxgi.scaling_mode != -1 &&
          pNewTargetParameters->Scaling  != 
            (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
                                ||
       ( config.render.framerate.refresh_rate          != -1 &&
           pNewTargetParameters->RefreshRate.Numerator !=
             (UINT)config.render.framerate.refresh_rate )
    )
  {
    DXGI_MODE_DESC new_new_params =
      *pNewTargetParameters;

    if ( config.render.framerate.refresh_rate != -1 &&
         new_new_params.RefreshRate.Numerator != (UINT)config.render.framerate.refresh_rate )
    {
      dll_log.Log ( L"[   DXGI   ]  >> Refresh Override "
                    L"(Requested: %f, Using: %li)",
                      new_new_params.RefreshRate.Denominator != 0 ?
                        static_cast <float> (new_new_params.RefreshRate.Numerator) /
                        static_cast <float> (new_new_params.RefreshRate.Denominator) :
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




    if (! config.window.res.override.isZero ())
    {
      new_new_params.Width  = config.window.res.override.x;
      new_new_params.Height = config.window.res.override.y;
    }

    else if ( (! config.window.fullscreen) &&
                 config.window.borderless )
    {
      SK_DXGI_BorderCompensation (new_new_params.Width, new_new_params.Height);
    }

    const DXGI_MODE_DESC* pNewNewTargetParameters =
      &new_new_params;



    //SK_DXGI_ValidateSwapChainResize (This, 0, pNewNewTargetParameters->Width, pNewNewTargetParameters->Height, pNewNewTargetParameters->Format);

    DXGI_CALL (ret, ResizeTarget_Original (This, pNewNewTargetParameters));

    if (SUCCEEDED (ret))
    {
      if (pNewNewTargetParameters->Width != 0 && pNewNewTargetParameters->Height != 0)
      {
        SK_SetWindowResX (pNewNewTargetParameters->Width);
        SK_SetWindowResY (pNewNewTargetParameters->Height);
      }

      else
      {
        RECT client;

        GetClientRect (game_window.hWnd, &client);
        SK_SetWindowResX (client.right  - client.left);
        SK_SetWindowResY (client.bottom - client.top);
      }
    }
  }

  else
  {
    //SK_DXGI_ValidateSwapChainResize (This, 0, pNewTargetParameters->Width, pNewTargetParameters->Height, pNewTargetParameters->Format);
    
    DXGI_CALL (ret, ResizeTarget_Original (This, pNewTargetParameters));

    if (SUCCEEDED (ret))
    {
      if (pNewTargetParameters->Width != 0 && pNewTargetParameters->Height != 0)
      {
        SK_SetWindowResX (pNewTargetParameters->Width);
        SK_SetWindowResY (pNewTargetParameters->Height);
      }

      else
      {
        RECT client;

        GetClientRect    (game_window.hWnd, &client);
        SK_SetWindowResX (client.right  - client.left);
        SK_SetWindowResY (client.bottom - client.top);
      }
    }
  }

  if (SUCCEEDED (ret))
  {
    //SK_RunOnce (SK_DXGI_HookPresent (This));
    //SK_RunOnce (SK_ApplyQueuedHooks (    ));
  }

  return ret;
}

__forceinline
void
SK_DXGI_CreateSwapChain_PreInit ( _Inout_opt_ DXGI_SWAP_CHAIN_DESC            *pDesc,
                                  _Inout_opt_ DXGI_SWAP_CHAIN_DESC1           *pDesc1,
                                  _Inout_opt_ HWND&                            hWnd,
                                  _Inout_opt_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc )
{
  WaitForInitDXGI ();

  DXGI_SWAP_CHAIN_DESC stub_desc  = {   }; // Stores common attributes between DESC and DESC1
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

  if (pDesc != nullptr)
  {
    // If auto-update prompt is visible, don't go fullscreen.
    extern          HWND hWndUpdateDlg;
    extern volatile LONG __SK_TaskDialogActive;
    if ( hWndUpdateDlg != static_cast <HWND> (INVALID_HANDLE_VALUE) ||
                                 ReadAcquire (&__SK_TaskDialogActive) )
    {
      pDesc->Windowed = true;
    }

    wchar_t wszMSAA [128] = { };

    _swprintf ( wszMSAA, pDesc->SampleDesc.Count > 1 ?
                           L"%u Samples" :
                           L"Not Used (or Offscreen)",
                  pDesc->SampleDesc.Count );

    dll_log.LogEx ( true,
      L"[Swap Chain]\n"
      L"  +-------------+-------------------------------------------------------------------------+\n"
      L"  | Resolution. |  %4lux%4lu @ %6.2f Hz%-50ws|\n"
      L"  | Format..... |  %-71ws|\n"
      L"  | Buffers.... |  %-2lu%-69ws|\n"
      L"  | MSAA....... |  %-71ws|\n"
      L"  | Mode....... |  %-71ws|\n"
      L"  | Scaling.... |  %-71ws|\n"
      L"  | Scanlines.. |  %-71ws|\n"
      L"  | Flags...... |  0x%04x%-65ws|\n"
      L"  | SwapEffect. |  %-71ws|\n"
      L"  +-------------+-------------------------------------------------------------------------+\n",
      pDesc->BufferDesc.Width,
      pDesc->BufferDesc.Height,
      pDesc->BufferDesc.RefreshRate.Denominator != 0 ?
        static_cast <float> (pDesc->BufferDesc.RefreshRate.Numerator) /
        static_cast <float> (pDesc->BufferDesc.RefreshRate.Denominator) :
          std::numeric_limits <float>::quiet_NaN (), L" ",
SK_DXGI_FormatToStr (pDesc->BufferDesc.Format).c_str (),
      pDesc->BufferCount, L" ",
      wszMSAA,
      pDesc->Windowed ? L"Windowed" : L"Fullscreen",
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
      pDesc->Flags, L" ",
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
    if (bAlwaysAllowFullscreen && pDesc->Windowed)
    {
      pDesc->Flags                             |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      //pDesc->BufferDesc.RefreshRate.Denominator = 0;
      //pDesc->BufferDesc.RefreshRate.Numerator   = 0;
    }


    if (! config.window.res.override.isZero ())
    {
      pDesc->BufferDesc.Width  = config.window.res.override.x;
      pDesc->BufferDesc.Height = config.window.res.override.y;
    }

    else if (pDesc->Windowed && config.window.borderless && (! config.window.fullscreen))
    {
      SK_DXGI_BorderCompensation (
        pDesc->BufferDesc.Width,
          pDesc->BufferDesc.Height
      );
    }


    if (config.render.dxgi.msaa_samples != -1 && config.render.dxgi.msaa_samples > 0)
    {
      pDesc->SampleDesc.Count = config.render.dxgi.msaa_samples;
    }


    // Dummy init swapchains (i.e. 1x1 pixel) should not have
    //   override parameters applied or it's usually fatal.
    bool no_override = (! SK_DXGI_IsSwapChainReal (*pDesc));



    if (! no_override)
    {
      if (config.render.dxgi.safe_fullscreen)
        pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

      if (request_mode_change == mode_change_request_e::Fullscreen)
      {
        dll_log.Log ( L"[   DXGI   ]  >> User-Requested Mode Change: Fullscreen" );
        pDesc->Windowed = FALSE;
        pDesc->Flags   |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      if (config.display.force_fullscreen && pDesc->Windowed)
      {
        dll_log.Log ( L"[   DXGI   ]  >> Display Override "
                      L"(Requested: Windowed, Using: Fullscreen)" );
        pDesc->Flags   |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        pDesc->Windowed = FALSE;
      }

      else if (config.display.force_windowed)
      {
        dll_log.Log ( L"[   DXGI   ]  >> Display Override "
                      L"(Requested: Fullscreen, Using: Windowed)" );
        pDesc->Windowed = TRUE;
      }

#ifdef _WIN64
      if (! bFlipMode)
        bFlipMode =
          ( dxgi_caps.present.flip_sequential &&
            ( SK_GetCurrentGameID () == SK_GAME_ID::Fallout4 ||
              SK_DS3_UseFlipMode  ()        ) );

      if (SK_GetCurrentGameID () == SK_GAME_ID::Fallout4)
      {
        if (bFlipMode)
            bFlipMode = (! SK_FO4_IsFullscreen ()) && SK_FO4_UseFlipMode ();
      }

      else
#endif
      {
        // If forcing flip-model, then force multisampling off
        if (config.render.framerate.flip_discard)
        {
          bFlipMode = dxgi_caps.present.flip_sequential;
          pDesc->SampleDesc.Count = 1; pDesc->SampleDesc.Quality = 0;

          // Format overrides must be performed in certain cases (sRGB / 10:10:10:2)
          switch (pDesc->BufferDesc.Format)
          {
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
              pDesc->BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
              dll_log.Log ( L"[ DXGI 1.2 ]  >> sRGB (B8G8R8A8) Override Required to Enable Flip Model" );
              break;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
              pDesc->BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
              dll_log.Log ( L"[ DXGI 1.2 ]  >> sRGB (R8G8B8A8) Override Required to Enable Flip Model" );
              break;
            case DXGI_FORMAT_R10G10B10A2_UNORM:
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
              pDesc->BufferDesc.Format =  DXGI_FORMAT_R8G8B8A8_UNORM;
              dll_log.Log ( L"[ DXGI 1.2 ]  >> RGBA 10:10:10:2 Override (to 8:8:8:8) Required to Enable Flip Model" );
              break;
          }
        }
      }


      if (SK_GetCurrentGameID () == SK_GAME_ID::DotHackGU)
        pDesc->BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;


      if (       config.render.framerate.buffer_count != -1                  &&
           (UINT)config.render.framerate.buffer_count !=  pDesc->BufferCount &&
           pDesc->BufferCount                         !=  0 )
      {
        pDesc->BufferCount = config.render.framerate.buffer_count;
        dll_log.Log (L"[   DXGI   ]  >> Buffer Count Override: %lu buffers", pDesc->BufferCount);
      }

      if ( config.render.framerate.flip_discard && dxgi_caps.swapchain.allow_tearing )
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

      if ( config.render.dxgi.scanline_order   != -1 &&
            pDesc->BufferDesc.ScanlineOrdering !=
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

      if ( config.render.framerate.refresh_rate    != -1 &&
           pDesc->BufferDesc.RefreshRate.Numerator != (UINT)config.render.framerate.refresh_rate )
      {
        dll_log.Log ( L"[   DXGI   ]  >> Refresh Override "
                      L"(Requested: %f, Using: %li)",
                   pDesc->BufferDesc.RefreshRate.Denominator != 0 ?
           static_cast <float> (pDesc->BufferDesc.RefreshRate.Numerator) /
           static_cast <float> (pDesc->BufferDesc.RefreshRate.Denominator) :
                       std::numeric_limits <float>::quiet_NaN (),
                          config.render.framerate.refresh_rate
                    );

        pDesc->BufferDesc.RefreshRate.Numerator   = config.render.framerate.refresh_rate;
        pDesc->BufferDesc.RefreshRate.Denominator = 1;
      }

      bWait = bFlipMode && dxgi_caps.present.waitable;

      // We cannot change the swapchain parameters if this is used...
      bWait = bWait && config.render.framerate.swapchain_wait > 0;

#ifdef _WIN64
      if (SK_GetCurrentGameID () == SK_GAME_ID::DarkSouls3)
      {
        if (SK_DS3_IsBorderless ())
          pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }
#endif

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

      else if ( pDesc->SwapEffect != DXGI_SWAP_EFFECT_FLIP_DISCARD &&
                pDesc->SwapEffect != DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL )
      {
        // Resort to triple-buffering if flip mode is not available
        if (config.render.framerate.buffer_count > 3)
          config.render.framerate.buffer_count = 3;

        pDesc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
      }

      if (config.render.framerate.buffer_count > 0)
        pDesc->BufferCount = config.render.framerate.buffer_count;

      // We cannot switch modes on a waitable swapchain
      if (bFlipMode && bWait)
      {
        pDesc->Flags |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }
    }

    SK_LOG0 ( ( L"  >> Using %s Presentation Model  [Waitable: %s - %li ms]",
                   bFlipMode ? L"Flip" : L"Traditional",
                     bWait ? L"Yes" : L"No",
                       bWait ? config.render.framerate.swapchain_wait : 0 ),
                L" DXGI 1.2 " );


    if ((! config.render.dxgi.res.max.isZero ()) && pDesc->BufferDesc.Width > config.render.dxgi.res.max.x)
      pDesc->BufferDesc.Width = config.render.dxgi.res.max.x;
    if ((! config.render.dxgi.res.max.isZero ()) && pDesc->BufferDesc.Height > config.render.dxgi.res.max.y)
      pDesc->BufferDesc.Height = config.render.dxgi.res.max.y;

    if ((! config.render.dxgi.res.min.isZero ()) && pDesc->BufferDesc.Width < config.render.dxgi.res.min.x)
      pDesc->BufferDesc.Width = config.render.dxgi.res.min.x;
    if ((! config.render.dxgi.res.min.isZero ()) && pDesc->BufferDesc.Height < config.render.dxgi.res.min.y)
      pDesc->BufferDesc.Height = config.render.dxgi.res.min.y;
  }


  if (translated)
  {
    pDesc1->BufferCount                        = pDesc->BufferCount;
    pDesc1->BufferUsage                        = pDesc->BufferUsage;
    pDesc1->Flags                              = pDesc->Flags;
    pDesc1->SwapEffect                         = pDesc->SwapEffect;
    pDesc1->SampleDesc.Count                   = pDesc->SampleDesc.Count;
    pDesc1->SampleDesc.Quality                 = pDesc->SampleDesc.Quality;
    pDesc1->Format                             = pDesc->BufferDesc.Format;
    pDesc1->Height                             = pDesc->BufferDesc.Height;
    pDesc1->Width                              = pDesc->BufferDesc.Width;

    hWnd                                       = pDesc->OutputWindow;

    if (pFullscreenDesc != nullptr)
    {
      pFullscreenDesc->Windowed                = pDesc->Windowed;
      pFullscreenDesc->RefreshRate.Denominator = pDesc->BufferDesc.RefreshRate.Denominator;
      pFullscreenDesc->RefreshRate.Numerator   = pDesc->BufferDesc.RefreshRate.Numerator;
      pFullscreenDesc->Scaling                 = pDesc->BufferDesc.Scaling;
      pFullscreenDesc->ScanlineOrdering        = pDesc->BufferDesc.ScanlineOrdering;
    }

    else
      pDesc->Windowed = TRUE;
  }
}

void SK_DXGI_HookSwapChain (IDXGISwapChain* pSwapChain);

void
SK_DXGI_UpdateLatencies (IDXGISwapChain *pSwapChain)
{
  if (! pSwapChain)
    return;

  const uint32_t max_latency =
    config.render.framerate.pre_render_limit;

  CComQIPtr <IDXGISwapChain2> pSwapChain2 (pSwapChain);

  if (pSwapChain2 != nullptr)
  {
    if (max_latency < 16 && max_latency > 0)
    {
      dll_log.Log (L"[   DXGI   ] Setting Swapchain Frame Latency: %lu", max_latency);
      pSwapChain2->SetMaximumFrameLatency (max_latency);
    }
  }

  if (max_latency < 16 && max_latency > 0)
  {
    CComPtr <IDXGIDevice1> pDevice1 = nullptr;

    if (SUCCEEDED ( pSwapChain->GetDevice (
                       IID_PPV_ARGS (&pDevice1)
                                          )
                  )
       )
    {
      dll_log.Log (L"[   DXGI   ] Setting Device    Frame Latency: %lu", max_latency);
      pDevice1->SetMaximumFrameLatency (max_latency);
    }
  }
}

__forceinline
void
SK_DXGI_CreateSwapChain_PostInit ( _In_  IUnknown              *pDevice,
                                   _In_  DXGI_SWAP_CHAIN_DESC  *pDesc,
                                   _In_  IDXGISwapChain       **ppSwapChain )
{
  wchar_t wszClass [MAX_PATH * 2] = { };

  if (pDesc != nullptr)
    RealGetWindowClassW (pDesc->OutputWindow, wszClass, MAX_PATH);

  bool dummy_window = 
    StrStrIW (wszClass, L"Special K Dummy Window Class") ||
    StrStrIW (wszClass, L"RTSSWndClass");

  if ((! dummy_window) && pDesc != nullptr && ( dwRenderThread == 0 || dwRenderThread == GetCurrentThreadId () ))
  {
    if ( dwRenderThread == 0x00 ||
         dwRenderThread == GetCurrentThreadId () )
    {
      if ( SK_GetCurrentRenderBackend ().windows.device != nullptr &&
           pDesc->OutputWindow                          != nullptr &&
           pDesc->OutputWindow                          != SK_GetCurrentRenderBackend ().windows.device )
      {
        dll_log.Log (L"[   DXGI   ] Game created a new window?!");
      }

      SK_InstallWindowHook (pDesc->OutputWindow);

      SK_GetCurrentRenderBackend ().windows.setDevice (pDesc->OutputWindow);
      SK_GetCurrentRenderBackend ().windows.setFocus  (pDesc->OutputWindow);
    }

    if (ppSwapChain != nullptr)
      SK_DXGI_HookSwapChain (*ppSwapChain);

    // Assume the first thing to create a D3D11 render device is
    //   the game and that devices never migrate threads; for most games
    //     this assumption holds.
    if ( dwRenderThread == 0x00 )
    {
      dwRenderThread = GetCurrentThreadId ();
    }
  }



  SK_CEGUI_QueueResetD3D11 ();

  if (pDesc != nullptr && pDesc->BufferDesc.Width != 0)
  {
    SK_SetWindowResX (pDesc->BufferDesc.Width);
    SK_SetWindowResY (pDesc->BufferDesc.Height);
  }

  else
  {
    RECT client = { };

    GetClientRect    (game_window.hWnd, &client);
    SK_SetWindowResX (client.right  - client.left);
    SK_SetWindowResY (client.bottom - client.top);
  }


  //if (bFlipMode || bWait)
    //DXGISwap_ResizeBuffers_Override (*ppSwapChain, config.render.framerate.buffer_count,
    //pDesc->BufferDesc.Width, pDesc->BufferDesc.Height, pDesc->BufferDesc.Format, pDesc->Flags);

  SK_DXGI_UpdateLatencies (*ppSwapChain);

  CComQIPtr <IDXGISwapChain2> pSwapChain2 (*ppSwapChain);

  if (pSwapChain2 != nullptr)
  {
    if (bFlipMode && bWait)
    {
      CHandle hWait (pSwapChain2->GetFrameLatencyWaitableObject ());

      MsgWaitForMultipleObjectsEx ( 1,
                                      &hWait.m_h,
                                        config.render.framerate.swapchain_wait,
                                          QS_ALLEVENTS, MWMO_ALERTABLE );
    }
  }

  CComQIPtr <ID3D11Device> pDev (pDevice);

  if (pDev != nullptr)
  {
    g_pD3D11Dev = pDev;

    if (pDesc != nullptr)
      SK_GetCurrentRenderBackend ().fullscreen_exclusive = (! pDesc->Windowed);
  }
}

__forceinline
void
SK_DXGI_CreateSwapChain1_PostInit ( _In_     IUnknown                         *pDevice,
                                    _In_     HWND                              hwnd,
                                    _In_     DXGI_SWAP_CHAIN_DESC1            *pDesc1,
                                    _In_opt_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC  *pFullscreenDesc,
                                    _In_     IDXGISwapChain1                 **ppSwapChain1 )
{
  // According to some Unreal engine games, ppSwapChain is optional...
  //   the docs say otherwise, but we can't argue with engines now can we?
  //
  if (! ppSwapChain1)
    return;

  if (! pDesc1)
    return;

  // ONLY AS COMPLETE AS NEEDED, if new code is added to PostInit, this will probably need changing.
  DXGI_SWAP_CHAIN_DESC desc;

  desc.BufferDesc.Width   = pDesc1->Width;
  desc.BufferDesc.Height  = pDesc1->Height;

  desc.BufferDesc.Format  = pDesc1->Format;
  //desc.BufferDesc.Scaling = pDesc1->Scaling;

  desc.BufferCount        = pDesc1->BufferCount;
  desc.BufferUsage        = pDesc1->BufferUsage;
  desc.Flags              = pDesc1->Flags;
  desc.SampleDesc         = pDesc1->SampleDesc;
  desc.SwapEffect         = pDesc1->SwapEffect;
  desc.OutputWindow       = hwnd;

  if (pFullscreenDesc)
  {
    desc.Windowed                    = pFullscreenDesc->Windowed;
    desc.BufferDesc.RefreshRate      = pFullscreenDesc->RefreshRate;
    desc.BufferDesc.ScanlineOrdering = pFullscreenDesc->ScanlineOrdering;
  }

  CComQIPtr <IDXGISwapChain> pSwapChain ((*ppSwapChain1));

  return SK_DXGI_CreateSwapChain_PostInit ( pDevice, &desc, &pSwapChain.p );
}

IWrapDXGISwapChain*
SK_DXGI_WrapSwapChain ( IUnknown        *pDevice,
                        IDXGISwapChain  *pSwapChain,
                        IDXGISwapChain **ppDest )
{
  DXGI_SWAP_CHAIN_DESC  desc = { };
  pSwapChain->GetDesc (&desc);

  CComQIPtr <ID3D11Device> pDev11 (pDevice);
  if (pDev11 != nullptr && SK_DXGI_IsSwapChainReal (desc))
  {
    *ppDest =
      new IWrapDXGISwapChain ((ID3D11Device *)pDevice, pSwapChain);

    SK_LOG0 ( (L" + SwapChain <IDXGISwapChain> (%08" PRIxPTR L"h) wrapped",
                 (uintptr_t)pSwapChain ),
               L"   DXGI   " );

    return (IWrapDXGISwapChain *)*ppDest;
  }

  else
  {
    if (pDev11 == nullptr)
      SK_LOG0 ( ("non-D3D11 SwapChain created"), L"   DXGI   ");

    *ppDest = pSwapChain;
  }

  return nullptr;
}

IWrapDXGISwapChain*
SK_DXGI_WrapSwapChain1 ( IUnknown         *pDevice,
                         IDXGISwapChain1  *pSwapChain,
                         IDXGISwapChain1 **ppDest )
{
  DXGI_SWAP_CHAIN_DESC  desc = { };
  pSwapChain->GetDesc (&desc);

  CComQIPtr <ID3D11Device> pDev11 (pDevice);
  if (pDev11 != nullptr && SK_DXGI_IsSwapChainReal (desc))
  {
    *ppDest =
      new IWrapDXGISwapChain ((ID3D11Device *)pDevice, pSwapChain);

    SK_LOG0 ( (L" + SwapChain <IDXGISwapChain1> (%08" PRIxPTR L"h) wrapped",
                 (uintptr_t)pSwapChain ),
               L"   DXGI   " );

    return (IWrapDXGISwapChain *)*ppDest;
  }

  else
  {
    if (pDev11 == nullptr)
      SK_LOG0 ( ("non-D3D11 SwapChain created"), L"   DXGI   ");

    *ppDest = pSwapChain;
  }

  return nullptr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGIFactory_CreateSwapChain_Override (             IDXGIFactory          *This,
                                       _In_        IUnknown              *pDevice,
                                       _In_  const DXGI_SWAP_CHAIN_DESC  *pDesc,
                                       _Out_       IDXGISwapChain       **ppSwapChain )
{
  std::wstring iname = SK_GetDXGIFactoryInterface (This);

  if (iname == L"{Invalid-Factory-UUID}")
  {
    static bool run_once = false;
    if (! run_once)
    {
      run_once = true;
      DXGI_LOG_CALL_I3 ( iname.c_str (), L"CreateSwapChain         ",
                           L"%08" PRIxPTR L"h, %08" PRIxPTR L"h, %08"
                                  PRIxPTR L"h",
                             (uintptr_t)pDevice, (uintptr_t)pDesc,
                             (uintptr_t)ppSwapChain );
    }

    IDXGISwapChain* pTemp = nullptr;
    HRESULT         hr    =
      CreateSwapChain_Original (This, pDevice, pDesc, &pTemp);

    if (SUCCEEDED (hr))
    {
      *ppSwapChain = pTemp;
    }

    return hr;
  }


  DXGI_LOG_CALL_I3 ( iname.c_str (), L"CreateSwapChain         ",
                       L"%08" PRIxPTR L"h, %08" PRIxPTR L"h, %08"
                              PRIxPTR L"h",
                         (uintptr_t)pDevice, (uintptr_t)pDesc,
                         (uintptr_t)ppSwapChain );

  HRESULT ret = E_FAIL;

  auto                 orig_desc = pDesc;
  DXGI_SWAP_CHAIN_DESC new_desc  =
    pDesc != nullptr ?
      *pDesc :
        DXGI_SWAP_CHAIN_DESC { };

  SK_DXGI_CreateSwapChain_PreInit (&new_desc, nullptr, new_desc.OutputWindow, nullptr);


  if (pDesc != nullptr) pDesc = &new_desc;

  auto CreateSwapChain_Lambchop =
    [&] (void) ->
      BOOL
      {
        IDXGISwapChain* pTemp = nullptr;

        DXGI_CALL (ret, CreateSwapChain_Original (This, pDevice, pDesc, &pTemp));
      
        if ( SUCCEEDED (ret) &&
             pTemp != nullptr )
        {
          SK_DXGI_CreateSwapChain_PostInit (pDevice, &new_desc, &pTemp);
          SK_DXGI_WrapSwapChain            (pDevice,             pTemp, ppSwapChain);

          return TRUE;
        }

        return FALSE;
      };


  if (! CreateSwapChain_Lambchop ())
  {
    // Fallback-on-Fail
    pDesc = orig_desc;

    CreateSwapChain_Lambchop ();
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
  DXGI_LOG_CALL_I3 ( iname.c_str (), L"CreateSwapChainForCoreWindow         ",
                       L"%08" PRIxPTR L"h, %08" PRIxPTR L"h, %08" PRIxPTR L"h",
                         (uintptr_t)pDevice, (uintptr_t)pDesc, (uintptr_t)ppSwapChain );

  if (iname == L"{Invalid-Factory-UUID}")
    return CreateSwapChainForCoreWindow_Original (This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

  HRESULT ret = E_FAIL;

  auto                   orig_desc = pDesc;
  DXGI_SWAP_CHAIN_DESC1  new_desc1 =
    pDesc != nullptr ?
      *pDesc :
        DXGI_SWAP_CHAIN_DESC1 { };

  HWND hWnd = nullptr;
  SK_DXGI_CreateSwapChain_PreInit (nullptr, &new_desc1, hWnd, nullptr);


  if (pDesc != nullptr) pDesc = &new_desc1;

  auto CreateSwapChain_Lambchop =
    [&] (void) ->
      BOOL
      {
        IDXGISwapChain1* pTemp = nullptr;

        DXGI_CALL( ret, CreateSwapChainForCoreWindow_Original (
                          This,
                            pDevice,
                              pWindow,
                                pDesc,
                                  pRestrictToOutput,
                                    &pTemp ) );
      
        if ( SUCCEEDED (ret)         &&
             pTemp != nullptr )
        {
          SK_DXGI_CreateSwapChain1_PostInit (pDevice, nullptr, &new_desc1, nullptr, &pTemp);
          SK_DXGI_WrapSwapChain1            (pDevice,                          pTemp,   ppSwapChain);

          return TRUE;
        }

        return FALSE;
      };


  if (! CreateSwapChain_Lambchop ())
  {
    // Fallback-on-Fail
    pDesc = orig_desc;

    CreateSwapChain_Lambchop ();
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
  DXGI_LOG_CALL_I3 ( iname.c_str (), L"CreateSwapChainForHwnd         ",
                       L"%08" PRIxPTR L"h, %08" PRIxPTR L"h, %08"
                              PRIxPTR L"h",
                         (uintptr_t)pDevice, (uintptr_t)pDesc, (uintptr_t)ppSwapChain );

  if (iname == L"{Invalid-Factory-UUID}")
    return CreateSwapChainForHwnd_Original (This, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);


  HRESULT ret = E_FAIL;

  assert (pDesc != nullptr);

  auto                            orig_desc1           = pDesc;
  auto                            orig_fullscreen_desc = pFullscreenDesc;

  DXGI_SWAP_CHAIN_DESC1           new_desc1           = *pDesc;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC new_fullscreen_desc =
    pFullscreenDesc ? *pFullscreenDesc :
                       DXGI_SWAP_CHAIN_FULLSCREEN_DESC { };

  pDesc           = &new_desc1;
  pFullscreenDesc = orig_fullscreen_desc ? &new_fullscreen_desc : nullptr;

  SK_DXGI_CreateSwapChain_PreInit (nullptr, &new_desc1, hWnd, &new_fullscreen_desc);

  auto CreateSwapChain_Lambchop =
    [&] (void) ->
      BOOL
      {
        IDXGISwapChain1* pTemp = nullptr;

        DXGI_CALL ( ret, CreateSwapChainForHwnd_Original ( This, pDevice, hWnd, pDesc, pFullscreenDesc,
                                                             pRestrictToOutput, &pTemp ) );

        if ( SUCCEEDED (ret) )
        {
          SK_DXGI_CreateSwapChain1_PostInit (pDevice, hWnd, &new_desc1, &new_fullscreen_desc, &pTemp);
          SK_DXGI_WrapSwapChain1            (pDevice,                                          pTemp, ppSwapChain);

          return TRUE;
        }

        return FALSE;
      };


  if (! CreateSwapChain_Lambchop ())
  {
    // Fallback-on-Fail
    pDesc           = orig_desc1;
    pFullscreenDesc = orig_fullscreen_desc;

    CreateSwapChain_Lambchop ();
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
  DXGI_LOG_CALL_I3 ( iname.c_str (), L"CreateSwapChainForComposition         ",
                       L"%08" PRIxPTR L"h, %08" PRIxPTR L"h, %08" PRIxPTR L"h",
                         (uintptr_t)pDevice, (uintptr_t)pDesc, (uintptr_t)ppSwapChain );

  HRESULT ret = E_FAIL;

  assert (pDesc != nullptr);

  DXGI_SWAP_CHAIN_DESC1           new_desc1           = {    };
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC new_fullscreen_desc = {    };

  if (pDesc != nullptr)
    new_desc1 = *pDesc;

  HWND hWnd = nullptr;
  SK_DXGI_CreateSwapChain_PreInit (nullptr, &new_desc1, hWnd, nullptr);


  DXGI_CALL (ret, CreateSwapChainForComposition_Original ( This, pDevice, &new_desc1,
                                                             pRestrictToOutput, ppSwapChain ));

  if ( SUCCEEDED (ret) )
  {
    if (ppSwapChain != nullptr)
      SK_DXGI_CreateSwapChain1_PostInit (pDevice, hWnd, &new_desc1, &new_fullscreen_desc, ppSwapChain);
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
      res = CreateDXGIFactory1_Import (__uuidof (IDXGIFactory1), static_cast_p2p <void> (&pFactory));
    else
      res = CreateDXGIFactory_Import  (__uuidof (IDXGIFactory),  static_cast_p2p <void> (&pFactory));
  }

  if (SUCCEEDED (res))
  {
    if (pFactory != nullptr)
    {
      if ((*ppAdapter) == nullptr)
        EnumAdapters_Original (pFactory, 0, &pGameAdapter);

      DXGI_ADAPTER_DESC game_desc { };

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

    if (match != nullptr)
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

    if (match != nullptr)
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

    if (match != nullptr)
    {
      dll_log.LogEx (false, L"Success! (%s)\n", match->Description);
      pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
    }

    else
    {
      dll_log.LogEx (false, L"Failure! (No Match Found)\n");
    }
  }

  if (config.system.log_level >= 1)
  {
    dll_log.Log ( L"[   DXGI   ] Dedicated Video: %zu MiB, Dedicated System: %zu MiB, "
                  L"Shared System: %zu MiB",
                    pDesc->DedicatedVideoMemory    >> 20UL,
                      pDesc->DedicatedSystemMemory >> 20UL,
                        pDesc->SharedSystemMemory  >> 20UL );
  }

  if ( SK_GetCurrentGameID () == SK_GAME_ID::Fallout4 &&
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

  DXGI_ADAPTER_DESC desc = { };

  switch (iver)
  {
    default:
    case 2:
    {
      if (! GetDesc2_Original)
      {
        CComQIPtr <IDXGIAdapter2> pAdapter2 (*ppAdapter);

        if (pAdapter2 != nullptr)
        {
          DXGI_VIRTUAL_HOOK (ppAdapter, 11, "(*pAdapter2)->GetDesc2",
            GetDesc2_Override, GetDesc2_Original, GetDesc2_pfn);
        }
      }
    }

    case 1:
    {
      if (! GetDesc1_Original)
      {
        CComQIPtr <IDXGIAdapter1> pAdapter1 (*ppAdapter);

        if (pAdapter1 != nullptr)
        {
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

  dll_log.LogEx (true, L"[   DXGI   ]  @ Returned Adapter %lu: '%32s' (LUID: %08X:%08X)",
    Adapter,
      desc.Description,
        desc.AdapterLuid.HighPart,
          desc.AdapterLuid.LowPart );

  //
  // Windows 8 has a software implementation, which we can detect.
  //
  CComQIPtr <IDXGIAdapter1> pAdapter1 (*ppAdapter);

  if (pAdapter1 != nullptr)
  {
    DXGI_ADAPTER_DESC1 desc1 = { };

    if (            GetDesc1_Original != nullptr &&
         SUCCEEDED (GetDesc1_Original (pAdapter1, &desc1)) )
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

  return S_OK;
}

HRESULT
STDMETHODCALLTYPE EnumAdapters1_Override (IDXGIFactory1  *This,
                                          UINT            Adapter,
                                   _Out_  IDXGIAdapter1 **ppAdapter)
{
  std::wstring iname = SK_GetDXGIFactoryInterface    (This);

  DXGI_LOG_CALL_I3 ( iname.c_str (), L"EnumAdapters1         ",
                       L"%08" PRIxPTR L"h, %u, %08" PRIxPTR L"h",
                         (uintptr_t)This, Adapter, (uintptr_t)ppAdapter );

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
    return EnumAdapters_Common (This, Adapter, reinterpret_cast <IDXGIAdapter **>  (ppAdapter),
                                               reinterpret_cast <EnumAdapters_pfn> (EnumAdapters1_Original));
  }

  return ret;
}

HRESULT
STDMETHODCALLTYPE EnumAdapters_Override (IDXGIFactory  *This,
                                         UINT           Adapter,
                                  _Out_  IDXGIAdapter **ppAdapter)
{
  std::wstring iname = SK_GetDXGIFactoryInterface    (This);

  DXGI_LOG_CALL_I3 ( iname.c_str (), L"EnumAdapters         ",
                       L"%08" PRIxPTR L"h, %u, %08" PRIxPTR L"h",
                         (uintptr_t)This, Adapter, (uintptr_t)ppAdapter );

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

    if (SUCCEEDED (EnumAdapters_Original (This, SK_DXGI_preferred_adapter, &pAdapter)))
    {
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
    return EnumAdapters_Common ( This, Adapter, ppAdapter,
                                 static_cast <EnumAdapters_pfn> (EnumAdapters_Original) );
  }

  return ret;
}

HMODULE
SK_D3D11_GetSystemDLL (void)
{
  static HMODULE hModSystemD3D11 = nullptr;

  if (hModSystemD3D11 == nullptr)
  {
    wchar_t   wszPath [MAX_PATH * 2] = { };
    wcsncpy  (wszPath, SK_GetSystemDirectory (), MAX_PATH);
    lstrcatW (wszPath, LR"(\d3d11.dll)");

    hModSystemD3D11 =
      LoadLibraryW_Original (wszPath);
  }

  return hModSystemD3D11;
}

HMODULE
SK_D3D11_GetLocalDLL (void)
{
  static HMODULE hModLocalD3D11 = nullptr;

  if (hModLocalD3D11 == nullptr)
  {
    hModLocalD3D11 =
      LoadLibraryW_Original (L"d3d11.dll");
  }

  return hModLocalD3D11;
}

HRESULT
STDMETHODCALLTYPE CreateDXGIFactory (REFIID   riid,
                               _Out_ void   **ppFactory)
{
  // For DXGI compliance, do not mix-and-match
  if (SK_DXGI_use_factory1)
    return CreateDXGIFactory1 (riid, ppFactory);

  std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
  int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

  UNREFERENCED_PARAMETER (iver);

  DXGI_LOG_CALL_2 ( L"                    CreateDXGIFactory        ", 
                    L"%s, %08" PRIxPTR L"h",
                      iname.c_str (), (uintptr_t)ppFactory );

  if (CreateDXGIFactory_Import == nullptr)
  {
    SK_RunOnce (SK_BootDXGI ());
                WaitForInitDXGI ();
  }

  SK_DXGI_factory_init = true;

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

  std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
  int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

  UNREFERENCED_PARAMETER (iver);

  DXGI_LOG_CALL_2 ( L"                    CreateDXGIFactory1       ",
                    L"%s, %08" PRIxPTR L"h",
                      iname.c_str (), (uintptr_t)ppFactory );

  if (CreateDXGIFactory_Import == nullptr)
  {
    SK_RunOnce (SK_BootDXGI ());
                WaitForInitDXGI ();
  }

  SK_DXGI_factory_init = true;

  // Windows Vista does not have this function -- wrap it with CreateDXGIFactory
  if (CreateDXGIFactory1_Import == nullptr)
  {
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

  std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
  int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

  UNREFERENCED_PARAMETER (iver);

  DXGI_LOG_CALL_3 ( L"                    CreateDXGIFactory2       ",
                    L"0x%04X, %s, %08" PRIxPTR L"h",
                      Flags, iname.c_str (), (uintptr_t)ppFactory );

  if (CreateDXGIFactory_Import == nullptr)
  {
    SK_RunOnce (SK_BootDXGI ());
                WaitForInitDXGI ();
  }

  SK_DXGI_factory_init = true;

  // Windows 7 does not have this function -- wrap it with CreateDXGIFactory1
  if (CreateDXGIFactory2_Import == nullptr)
  {
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

using finish_pfn = void (WINAPI *)(void);

void
WINAPI
SK_HookDXGI (void)
{
  // Shouldn't be calling this if hooking is turned off!
  assert (config.apis.dxgi.d3d11.hook);

  if (! config.apis.dxgi.d3d11.hook)
  {
    return;
  }

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
#ifdef _WIN64
    if (! config.apis.dxgi.d3d11.hook)
      config.apis.dxgi.d3d12.hook = false;
#endif

    // Serves as both D3D11 and DXGI
    bool d3d11 =
      ( SK_GetDLLRole () & DLL_ROLE::D3D11 );


    HMODULE hBackend = 
      ( (SK_GetDLLRole () & DLL_ROLE::DXGI) && (! d3d11) ) ? backend_dll :
                                                               LoadLibraryW_Original (L"dxgi.dll");


    dll_log.Log (L"[   DXGI   ] Importing CreateDXGIFactory{1|2}");
    dll_log.Log (L"[   DXGI   ] ================================");

    if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dxgi.dll"))
    {
      CreateDXGIFactory_Import =  \
        (CreateDXGIFactory_pfn)GetProcAddress  (hBackend, "CreateDXGIFactory");
      CreateDXGIFactory1_Import = \
        (CreateDXGIFactory1_pfn)GetProcAddress (hBackend, "CreateDXGIFactory1");
      CreateDXGIFactory2_Import = \
        (CreateDXGIFactory2_pfn)GetProcAddress (hBackend, "CreateDXGIFactory2");

      SK_LOG0 ( ( L"  CreateDXGIFactory:  %s",
                    SK_MakePrettyAddress (CreateDXGIFactory_Import).c_str ()  ),
                  L" DXGI 1.0 " );
      SK_LogSymbolName                   (CreateDXGIFactory_Import);

      SK_LOG0 ( ( L"  CreateDXGIFactory1: %s",
                    SK_MakePrettyAddress (CreateDXGIFactory1_Import).c_str () ),
                  L" DXGI 1.1 " );
      SK_LogSymbolName                   (CreateDXGIFactory1_Import);

      SK_LOG0 ( ( L"  CreateDXGIFactory2: %s",
                    SK_MakePrettyAddress (CreateDXGIFactory2_Import).c_str () ),
                  L" DXGI 1.3 " );
      SK_LogSymbolName                   (CreateDXGIFactory2_Import);

      LocalHook_CreateDXGIFactory.target.addr  = CreateDXGIFactory_Import;
      LocalHook_CreateDXGIFactory1.target.addr = CreateDXGIFactory1_Import;
      LocalHook_CreateDXGIFactory2.target.addr = CreateDXGIFactory2_Import;
    }

    else
    {
      LPVOID pfnCreateDXGIFactory    = nullptr;
      LPVOID pfnCreateDXGIFactory1   = nullptr;
      LPVOID pfnCreateDXGIFactory2   = nullptr;

      if ((! LocalHook_CreateDXGIFactory.active) && GetProcAddress (hBackend, "CreateDXGIFactory"))
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 (      L"dxgi.dll",
                                         "CreateDXGIFactory",
                                          CreateDXGIFactory,
                 static_cast_p2p <void> (&CreateDXGIFactory_Import),
                                      &pfnCreateDXGIFactory ) )
        {
          MH_QueueEnableHook (pfnCreateDXGIFactory);
        }
      }
      else if (LocalHook_CreateDXGIFactory.active) {
        pfnCreateDXGIFactory = LocalHook_CreateDXGIFactory.target.addr;
      }

      if ((! LocalHook_CreateDXGIFactory1.active) && GetProcAddress (hBackend, "CreateDXGIFactory1"))
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 (      L"dxgi.dll",
                                         "CreateDXGIFactory1",
                                          CreateDXGIFactory1,
                 static_cast_p2p <void> (&CreateDXGIFactory1_Import),
                                      &pfnCreateDXGIFactory1 ) )
        {
          MH_QueueEnableHook (pfnCreateDXGIFactory1);
        }
      }
      else if (LocalHook_CreateDXGIFactory1.active) {
        pfnCreateDXGIFactory1 = LocalHook_CreateDXGIFactory1.target.addr;
      }

      if ((! LocalHook_CreateDXGIFactory2.active) && GetProcAddress (hBackend, "CreateDXGIFactory2"))
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 (      L"dxgi.dll",
                                         "CreateDXGIFactory2",
                                          CreateDXGIFactory2,
                 static_cast_p2p <void> (&CreateDXGIFactory2_Import),
                                      &pfnCreateDXGIFactory2 ) )
        {
          MH_QueueEnableHook (pfnCreateDXGIFactory2);
        }
      }
      else if (LocalHook_CreateDXGIFactory2.active) {
        pfnCreateDXGIFactory2 = LocalHook_CreateDXGIFactory2.target.addr;
      }

      SK_LOG0 ( ( L"  CreateDXGIFactory:  %s  %s",
                    SK_MakePrettyAddress (pfnCreateDXGIFactory).c_str  (),
                                             CreateDXGIFactory_Import ?
                                               L"{ Hooked }" :
                                               L"" ),
                  L" DXGI 1.0 " );
      SK_LogSymbolName                   (pfnCreateDXGIFactory);

      SK_LOG0 ( ( L"  CreateDXGIFactory1: %s  %s",
                    SK_MakePrettyAddress (pfnCreateDXGIFactory1).c_str  (),
                                             CreateDXGIFactory1_Import ?
                                               L"{ Hooked }" :
                                               L"" ),
                  L" DXGI 1.1 " );
      SK_LogSymbolName                   (pfnCreateDXGIFactory1);

      SK_LOG0 ( ( L"  CreateDXGIFactory2: %s  %s",
                    SK_MakePrettyAddress (pfnCreateDXGIFactory2).c_str  (),
                                             CreateDXGIFactory2_Import ?
                                               L"{ Hooked }" :
                                               L"" ),
                  L" DXGI 1.3 " );
      SK_LogSymbolName                   (pfnCreateDXGIFactory2);

      LocalHook_CreateDXGIFactory.target.addr  = pfnCreateDXGIFactory;
      LocalHook_CreateDXGIFactory1.target.addr = pfnCreateDXGIFactory1;
      LocalHook_CreateDXGIFactory2.target.addr = pfnCreateDXGIFactory2;
    }

  //SK_ApplyQueuedHooks ();

    if (CreateDXGIFactory1_Import != nullptr)
    {
      SK_DXGI_use_factory1 = true;
      SK_DXGI_factory_init = true;
    }

    SK_D3D11_InitTextures ();
    SK_D3D11_Init         ();

    if (GetModuleHandle (L"d3d12.dll"))
      SK_D3D12_Init       ();

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

    InterlockedIncrement (&hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}

static std::queue <DWORD> old_threads;

void
WINAPI
dxgi_init_callback (finish_pfn finish)
{
  if (! SK_IsHostAppSKIM ())
  {
    SK_BootDXGI     ();
    WaitForInitDXGI ();
  }

  finish ();
}


#include <SpecialK/ini.h>

bool
SK::DXGI::Startup (void)
{
  if (SK_GetDLLRole () & DLL_ROLE::D3D11)
    return SK_StartupCore (L"d3d11", dxgi_init_callback);

  return SK_StartupCore (L"dxgi", dxgi_init_callback);
}


#ifdef _WIN64
extern bool WINAPI SK_DS3_ShutdownPlugin (const wchar_t* backend);
#endif

#pragma warning (disable:4091)
#include <DbgHelp.h>

void
SK_DXGI_HookPresentBase (IDXGISwapChain* pSwapChain)
{
  if (! LocalHook_IDXGISwapChain_Present.active)
  {
    DXGI_VIRTUAL_HOOK ( &pSwapChain, 8,
                        "IDXGISwapChain::Present",
                         PresentCallback,
                         Present_Original,
                         PresentSwapChain_pfn );

    SK_Hook_TargetFromVFTable   (
      LocalHook_IDXGISwapChain_Present,
        (void **)&pSwapChain, 8 );
  }
}

void
SK_DXGI_HookPresent1 (IDXGISwapChain1* pSwapChain1)
{
  if (! LocalHook_IDXGISwapChain1_Present1.active)
  {
    DXGI_VIRTUAL_HOOK ( &pSwapChain1, 22,
                        "IDXGISwapChain1::Present1",
                         Present1Callback,
                         Present1_Original,
                         Present1SwapChain1_pfn );

    SK_Hook_TargetFromVFTable     (
      LocalHook_IDXGISwapChain1_Present1,
        (void **)&pSwapChain1, 22 );
  }
}

void
SK_DXGI_HookPresent (IDXGISwapChain* pSwapChain)
{
  SK_DXGI_HookPresentBase (pSwapChain);

  CComQIPtr <IDXGISwapChain1> pSwapChain1 (pSwapChain);

  if (pSwapChain1 != nullptr)
  {
    SK_DXGI_HookPresent1 (pSwapChain1);
  }

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Reinstall ();
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
  if (! first_frame)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    if (! LocalHook_IDXGISwapChain_SetFullscreenState.active)
    {
      DXGI_VIRTUAL_HOOK ( &pSwapChain, 10, "IDXGISwapChain::SetFullscreenState",
                                DXGISwap_SetFullscreenState_Override,
                                         SetFullscreenState_Original,
                                           SetFullscreenState_pfn );

      SK_Hook_TargetFromVFTable (
        LocalHook_IDXGISwapChain_SetFullscreenState,
          (void **)&pSwapChain, 10 );
    }

    if (! LocalHook_IDXGISwapChain_GetFullscreenState.active)
    {
      DXGI_VIRTUAL_HOOK ( &pSwapChain, 11, "IDXGISwapChain::GetFullscreenState",
                                DXGISwap_GetFullscreenState_Override,
                                         GetFullscreenState_Original,
                                           GetFullscreenState_pfn );
      SK_Hook_TargetFromVFTable (
        LocalHook_IDXGISwapChain_GetFullscreenState,
          (void **)&pSwapChain, 11 );
    }

    if (! LocalHook_IDXGISwapChain_ResizeBuffers.active)
    {
      DXGI_VIRTUAL_HOOK ( &pSwapChain, 13, "IDXGISwapChain::ResizeBuffers",
                               DXGISwap_ResizeBuffers_Override,
                                        ResizeBuffers_Original,
                                          ResizeBuffers_pfn );
      SK_Hook_TargetFromVFTable (
        LocalHook_IDXGISwapChain_ResizeBuffers,
          (void **)&pSwapChain, 13 );
    }

    if (! LocalHook_IDXGISwapChain_ResizeTarget.active)
    {
      DXGI_VIRTUAL_HOOK ( &pSwapChain, 14, "IDXGISwapChain::ResizeTarget",
                               DXGISwap_ResizeTarget_Override,
                                        ResizeTarget_Original,
                                          ResizeTarget_pfn );
      SK_Hook_TargetFromVFTable (
        LocalHook_IDXGISwapChain_ResizeTarget,
          (void **)&pSwapChain, 14 );
    }


    CComPtr <IDXGIOutput> pOutput = nullptr;
    
    if (SUCCEEDED (pSwapChain->GetContainingOutput (&pOutput.p)))
    {
      if (pOutput != nullptr)
      {
        DXGI_VIRTUAL_HOOK ( &pOutput.p, 8, "IDXGIOutput::GetDisplayModeList",
                                  DXGIOutput_GetDisplayModeList_Override,
                                             GetDisplayModeList_Original,
                                             GetDisplayModeList_pfn );
    
        DXGI_VIRTUAL_HOOK ( &pOutput.p, 9, "IDXGIOutput::FindClosestMatchingMode",
                                  DXGIOutput_FindClosestMatchingMode_Override,
                                             FindClosestMatchingMode_Original,
                                             FindClosestMatchingMode_pfn );
    
        // Don't hook this unless you want nvspcap to crash the game.

        DXGI_VIRTUAL_HOOK ( &pOutput.p, 10, "IDXGIOutput::WaitForVBlank",
                                 DXGIOutput_WaitForVBlank_Override,
                                            WaitForVBlank_Original,
                                            WaitForVBlank_pfn );
      }
    }

    InterlockedIncrement (&hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}


void
SK_DXGI_HookFactory (IDXGIFactory* pFactory)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    //int iver = SK_GetDXGIFactoryInterfaceVer (pFactory);

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
    if (! LocalHook_IDXGIFactory_CreateSwapChain.active)
    {
      DXGI_VIRTUAL_HOOK ( &pFactory,     10,
                          "IDXGIFactory::CreateSwapChain",
                           DXGIFactory_CreateSwapChain_Override,
                                       CreateSwapChain_Original,
                                       CreateSwapChain_pfn );

      SK_Hook_TargetFromVFTable (
        LocalHook_IDXGIFactory_CreateSwapChain,
          (void **)&pFactory, 10 );
    }

    CComQIPtr <IDXGIFactory1> pFactory1 (pFactory);

    // 12 EnumAdapters1
    // 13 IsCurrent
    if (pFactory1 != nullptr)
    {
      DXGI_VIRTUAL_HOOK ( &pFactory1.p,     12,
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
    if ( CreateDXGIFactory1_Import != nullptr )
    {
      CComPtr <IDXGIFactory2> pFactory2 = nullptr;

      if ( SUCCEEDED (CreateDXGIFactory1_Import (IID_IDXGIFactory2, (void **)&pFactory2)) )
      {
        if (! LocalHook_IDXGIFactory2_CreateSwapChainForHwnd.active)
        {
          DXGI_VIRTUAL_HOOK ( &pFactory2.p, 15,
                              "IDXGIFactory2::CreateSwapChainForHwnd",
                               DXGIFactory2_CreateSwapChainForHwnd_Override,
                                            CreateSwapChainForHwnd_Original,
                                            CreateSwapChainForHwnd_pfn );

          SK_Hook_TargetFromVFTable (
            LocalHook_IDXGIFactory2_CreateSwapChainForHwnd,
              (void **)&pFactory2.p, 15 );
        }

        if (! LocalHook_IDXGIFactory2_CreateSwapChainForCoreWindow.active)
        {
          DXGI_VIRTUAL_HOOK ( &pFactory2.p, 16,
                              "IDXGIFactory2::CreateSwapChainForCoreWindow",
                               DXGIFactory2_CreateSwapChainForCoreWindow_Override,
                                            CreateSwapChainForCoreWindow_Original,
                                            CreateSwapChainForCoreWindow_pfn );
          SK_Hook_TargetFromVFTable (
            LocalHook_IDXGIFactory2_CreateSwapChainForCoreWindow,
              (void **)&pFactory2.p, 16 );
        }

        if (! LocalHook_IDXGIFactory2_CreateSwapChainForComposition.active)
        {
          DXGI_VIRTUAL_HOOK ( &pFactory2.p, 24,
                              "IDXGIFactory2::CreateSwapChainForComposition",
                               DXGIFactory2_CreateSwapChainForComposition_Override,
                                            CreateSwapChainForComposition_Original,
                                            CreateSwapChainForComposition_pfn );

          SK_Hook_TargetFromVFTable (
            LocalHook_IDXGIFactory2_CreateSwapChainForComposition,
              (void **)&pFactory2.p, 24 );
        }
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

    InterlockedIncrement (&hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}


#include <mmsystem.h>
#include <d3d11_2.h>
#include <SpecialK/com_util.h>

void
SK_DXGI_InitHooksBeforePlugIn (void)
{
  extern int SK_Import_GetNumberOfPlugIns (void);

  if (SK_Import_GetNumberOfPlugIns () > 0)
    SK_ApplyQueuedHooks ();
}

DWORD
__stdcall
HookDXGI (LPVOID user)
{
  SetCurrentThreadDescription (L"[SK] DXGI Hook Crawler Thread");

  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
    return 0;


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
      MsgWaitForMultipleObjectsEx (0, nullptr, 33, QS_ALLINPUT, MWMO_ALERTABLE);

    // TODO: Handle situation where CreateDXGIFactory is unloadable
  }

  if (__SK_bypass || ReadAcquire (&__dxgi_ready) || SK_TLS_Bottom ()->d3d11.ctx_init_thread)
  {
    return 0;
  }


  static volatile LONG __hooked = FALSE;

  if (! InterlockedCompareExchange (&__hooked, TRUE, FALSE))
  {
    SK_TLS_Bottom ()->d3d11.ctx_init_thread = true;

    SK_AutoCOMInit auto_com;

    SK_D3D11_Init ();

    dll_log.Log (L"[   DXGI   ]   Installing DXGI Hooks");

    D3D_FEATURE_LEVEL             levels [] = { D3D_FEATURE_LEVEL_11_0 };

    D3D_FEATURE_LEVEL             featureLevel;
    CComPtr <ID3D11Device>        pDevice           = nullptr;
    CComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;

    // DXGI stuff is ready at this point, we'll hook the swapchain stuff
    //   after this call.

    HRESULT hr = E_NOTIMPL;

    CComPtr <IDXGISwapChain> pSwapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC     desc       = { };
    
    desc.BufferDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count            = 1;
    desc.SampleDesc.Quality          = 0;
    desc.BufferDesc.Width            = 2;
    desc.BufferDesc.Height           = 2;
    desc.BufferUsage                 = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
    desc.BufferCount                 = 1;
    desc.OutputWindow                = SK_Win32_CreateDummyWindow ();
    desc.Windowed                    = TRUE;
    desc.SwapEffect                  = DXGI_SWAP_EFFECT_DISCARD;

    extern LPVOID pfnD3D11CreateDeviceAndSwapChain;


    if ((SK_GetDLLRole () & DLL_ROLE::DXGI) || (SK_GetDLLRole () & DLL_ROLE::DInput8))
    {
      // PlugIns need to be loaded AFTER we've hooked the device creation functions
      SK_DXGI_InitHooksBeforePlugIn ();

      // Load user-defined DLLs (Plug-In)
      SK_RunLHIfBitness ( 64, SK_LoadPlugIns64 (),
                              SK_LoadPlugIns32 () );
    }

    hr =
      D3D11CreateDeviceAndSwapChain_Import (
        nullptr,
          D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
              0x0,
                levels,
                  1,
                    D3D11_SDK_VERSION,
                      &desc, &pSwapChain,
                        &pDevice,
                          &featureLevel,
                            &pImmediateContext );

    d3d11_hook_ctx.ppDevice           = &pDevice.p;
    d3d11_hook_ctx.ppImmediateContext = &pImmediateContext.p;

    CComQIPtr <IDXGIDevice>  pDevDXGI = nullptr;
    CComPtr   <IDXGIAdapter> pAdapter = nullptr;
    CComPtr   <IDXGIFactory> pFactory = nullptr;
    
    if ( pDevice != nullptr &&
         SUCCEEDED (pDevice->QueryInterface <IDXGIDevice> (&pDevDXGI)) &&
         SUCCEEDED (pDevDXGI->GetAdapter                  (&pAdapter)) &&
         SUCCEEDED (pAdapter->GetParent     (IID_PPV_ARGS (&pFactory))) )
    {
      CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;
      CComPtr <ID3D11Device>        pDev    = pDevice;

      if (config.render.dxgi.deferred_isolation)
      {
        pImmediateContext->GetDevice (&pDev.p);

        pDev->CreateDeferredContext (0x00,  &pDevCtx.p);
        d3d11_hook_ctx.ppImmediateContext = &pDevCtx.p;
      }

      else
      {
        pDev->GetImmediateContext (&pDevCtx.p);
        d3d11_hook_ctx.ppImmediateContext = (ID3D11DeviceContext **)&pDevCtx.p;
      }

      HookD3D11             (&d3d11_hook_ctx);
      SK_DXGI_HookFactory   (pFactory);
      SK_DXGI_HookSwapChain (pSwapChain.p);

      // This won't catch Present1 (...), but no games use that
      //   and we can deal with it later if it happens.
      SK_DXGI_HookPresentBase ((IDXGISwapChain *)pSwapChain.p);

      CComQIPtr <IDXGISwapChain1> pSwapChain1 (pSwapChain);

      if (pSwapChain1 != nullptr)
        SK_DXGI_HookPresent1 (pSwapChain1);

      SK_ApplyQueuedHooks ();

      extern volatile LONG   SK_D3D11_initialized;
      InterlockedIncrement (&SK_D3D11_initialized);

      if (config.apis.dxgi.d3d11.hook) SK_D3D11_EnableHooks ();
        
#ifdef _WIN64
      if (config.apis.dxgi.d3d12.hook) SK_D3D12_EnableHooks ( );
#endif

      InterlockedExchange (&__dxgi_ready, TRUE);
    }

    else
    {
      _com_error err (hr);

      std::wstring err_desc (err.Description ());
      std::wstring err_src  (err.Source      ());
    
      dll_log.Log (L"[   DXGI   ] Unable to hook D3D11?! HRESULT=%x ('%s')",
                               err.Error (), err.ErrorMessage () );
      dll_log.Log (L"[   DXGI   ]  >> %s, in %s",
                               err_desc.c_str (), err_src.c_str () );
    }

    SK_Win32_CleanupDummyWindow (desc.OutputWindow);

    InterlockedIncrement (&__hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&__hooked, 2);

  return 0;
}


bool
SK::DXGI::Shutdown (void)
{
#ifdef _WIN64
  if (SK_GetCurrentGameID () == SK_GAME_ID::DarkSouls3)
  {
    SK_DS3_ShutdownPlugin (L"dxgi");
  }
#endif


  // Video capture software usually lets one frame through before @#$%'ing
  //   up the Present (...) hook.
  if (SK_GetFramesDrawn () < 2)
  {
    SK_LOG0 ( ( L" !!! No frames drawn using DXGI backend; purging "
                L"injection address cache..."
              ),
                L"Hook Cache" );

    for ( auto& it : local_dxgi_records )
    {
      SK_Hook_RemoveTarget ( *it, L"DXGI.Hooks" );
    }

    SK_D3D11_PurgeHookAddressCache ();

    SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
  }


  if (config.apis.dxgi.d3d11.hook) SK_D3D11_Shutdown ();

#ifdef _WIN64
  if (config.apis.dxgi.d3d12.hook) SK_D3D12_Shutdown ();
#endif

  if (StrStrIW (SK_GetBackend (), L"d3d11"))
    return SK_ShutdownCore (L"d3d11");

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
           HANDLE         shutdown = INVALID_HANDLE_VALUE;
  volatile LONG           ready    = FALSE;
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

  if (SUCCEEDED ((*ppAdapter)->QueryInterface <IDXGIAdapter3> (&pAdapter3)))
  {
    // We darn sure better not spawn multiple threads!
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (*budget_mutex);

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
      budget_thread.event    = nullptr;
      budget_log.silent      = true;


      budget_thread.handle =
          CreateThread ( nullptr,
                           0,
                             BudgetThread,
                               (LPVOID)&budget_thread,
                                 0x00,
                                   nullptr );


      SK_Thread_SpinUntilFlagged (&budget_thread.ready);


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
                            L"eid=0x%08" PRIxPTR L", cookie=%u\n",
                              (uintptr_t)budget_thread.event,
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
              _mem_info = { };
      DXGI_ADAPTER_DESC2
                  desc2 = { };

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
                          L" Node%i       (Reserve: %#5llu / %#5llu MiB - "
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
                  ( i == 1 ) ?
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
                          L" Node%i       (Reserve: %#5llu / %#5llu MiB - "
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
                  ( i == 1 ) ?
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
  }

  return hr;
}

#include <ctime>
#define min_max(ref,min,max) if ((ref) > (max)) (max) = (ref); \
                             if ((ref) < (min)) (min) = (ref);

const uint32_t
  BUDGET_POLL_INTERVAL = 133UL; // How often to sample the budget
                                //  in msecs

DWORD
WINAPI
SK::DXGI::BudgetThread ( LPVOID user_data )
{
  SetCurrentThreadDescription (L"[SK] DXGI Budget Tracking Thread");

  auto* params =
    static_cast <budget_thread_params_t *> (user_data);

  budget_log.silent = true;
  params->tid       = GetCurrentThreadId ();
  params->event     =
    CreateEvent ( nullptr,
                    FALSE,
                      FALSE,
                        L"DXGIMemoryBudget"
                );
  params->shutdown  = 
    CreateEvent ( nullptr,
                    FALSE,
                      FALSE,
                        L"DXGIMemoryBudget_Shutdown" );

  InterlockedExchange ( &params->ready, TRUE );


  SK_AutoCOMInit auto_com;

  while ( ReadAcquire ( &params->ready ) )
  {
    if (ReadAcquire (&__SK_DLL_Ending))
      break;

    if ( params->event == nullptr )
      break;

    HANDLE phEvents [] = { params->event, params->shutdown };

    DWORD dwWaitStatus =
      MsgWaitForMultipleObjects ( 2,
                                    phEvents,
                                      FALSE,
                                        BUDGET_POLL_INTERVAL * 3, QS_ALLINPUT );

    if (! ReadAcquire ( &params->ready ) )
    {
      ResetEvent ( params->event );
      break;
    }

    if (dwWaitStatus == WAIT_OBJECT_0 + 1)
    {
      InterlockedExchange ( &params->ready, FALSE );
      ResetEvent          (  params->shutdown     );
      break;
    }

    int         node = 0;

    buffer_t buffer  =
      mem_info [node].buffer;


    SK_D3D11_Textures.Budget = mem_info [buffer].local [0].Budget -
                               mem_info [buffer].local [0].CurrentUsage;


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

          const int64_t over_budget =
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

  CloseHandle (params->shutdown);
               params->shutdown = INVALID_HANDLE_VALUE;

  return 0;
}

HRESULT
SK::DXGI::StartBudgetThread_NoAdapter (void)
{
  HRESULT hr = E_NOTIMPL;

  SK_AutoCOMInit auto_com;

  static HMODULE
    hDXGI = LoadLibraryW_Original ( L"dxgi.dll" );

  if (hDXGI)
  {
    static auto
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
                                       &adapter.p )
           )
         )
      {
        hr = StartBudgetThread ( &adapter.p );
      }
    }
  }

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

    SetEvent (budget_thread.shutdown );

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





















#include <SpecialK/ini.h>

bool
SK_D3D11_QuickHooked (void);

void
SK_DXGI_QuickHook (void)
{
  // We don't want to hook this, and we certainly don't want to hook it using
  //   cached addresses!
  if (! config.apis.dxgi.d3d11.hook)
    return;

  if (config.steam.preload_overlay)
    return;


  static volatile LONG quick_hooked = FALSE;

  if (! InterlockedCompareExchange (&quick_hooked, TRUE, FALSE))
  {
    SK_D3D11_QuickHook    ();
  
    sk_hook_cache_enablement_s state =
      SK_Hook_PreCacheModule ( L"DXGI",
                                 local_dxgi_records,
                                   global_dxgi_records );
  
    if ( state.hooks_loaded.from_shared_dll > 0 ||
         state.hooks_loaded.from_game_ini   > 0 ||
         SK_D3D11_QuickHooked () )
    {
      SK_D3D11_Init       ();
      SK_ApplyQueuedHooks ();
    }
  
    else
    {
      for ( auto& it : local_dxgi_records )
      {
        it->active = false;
      }
    }

    InterlockedIncrement (&quick_hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&quick_hooked, 2);
}