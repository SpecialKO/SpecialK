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
#define __SK_SUBSYSTEM__ L"   DXGI   "

#include <SpecialK/render/dxgi/dxgi_interfaces.h>
#include <SpecialK/render/dxgi/dxgi_swapchain.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>

#include <imgui/backends/imgui_d3d11.h>
#include <imgui/backends/imgui_d3d12.h>

#include <SpecialK/nvapi.h>
#include <math.h>

// For querying the name of the monitor from the system registry when
//   EDID and other more purpose-built driver functions fail to help.
#pragma comment (lib, "SetupAPI.lib")

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
#define SK_HookCacheEntryGlobalDll(x) __declspec (dllexport)\
                                      SK_HookCacheEntryGlobal (x)
  // Global DLL's cache
  SK_HookCacheEntryGlobalDll (IDXGIFactory_CreateSwapChain)
  SK_HookCacheEntryGlobalDll (IDXGIFactory2_CreateSwapChainForHwnd)
  SK_HookCacheEntryGlobalDll (IDXGIFactory2_CreateSwapChainForCoreWindow)
  SK_HookCacheEntryGlobalDll (IDXGIFactory2_CreateSwapChainForComposition)
  SK_HookCacheEntryGlobalDll (IDXGISwapChain_Present)
  SK_HookCacheEntryGlobalDll (IDXGISwapChain1_Present1)
  SK_HookCacheEntryGlobalDll (IDXGISwapChain_ResizeTarget)
  SK_HookCacheEntryGlobalDll (IDXGISwapChain_ResizeBuffers)
  SK_HookCacheEntryGlobalDll (IDXGISwapChain_GetFullscreenState)
  SK_HookCacheEntryGlobalDll (IDXGISwapChain_SetFullscreenState)
  SK_HookCacheEntryGlobalDll (CreateDXGIFactory)
  SK_HookCacheEntryGlobalDll (CreateDXGIFactory1)
  SK_HookCacheEntryGlobalDll (CreateDXGIFactory2)
};
#pragma data_seg ()
#pragma comment  (linker, "/SECTION:.SK_DXGI_Hooks,RWS")


// Local DLL's cached addresses
SK_HookCacheEntryLocal (IDXGIFactory_CreateSwapChain,
  L"dxgi.dll",           DXGIFactory_CreateSwapChain_Override,
                                    &CreateSwapChain_Original)
SK_HookCacheEntryLocal (IDXGIFactory2_CreateSwapChainForHwnd,
  L"dxgi.dll",           DXGIFactory2_CreateSwapChainForHwnd_Override,
                                     &CreateSwapChainForHwnd_Original)
SK_HookCacheEntryLocal (IDXGIFactory2_CreateSwapChainForCoreWindow,
  L"dxgi.dll",           DXGIFactory2_CreateSwapChainForCoreWindow_Override,
                                     &CreateSwapChainForCoreWindow_Original)
SK_HookCacheEntryLocal (IDXGIFactory2_CreateSwapChainForComposition,
  L"dxgi.dll",           DXGIFactory2_CreateSwapChainForComposition_Override,
                                     &CreateSwapChainForComposition_Original)
SK_HookCacheEntryLocal (IDXGISwapChain_Present,
  L"dxgi.dll",                         PresentCallback,
                                      &Present_Original);
SK_HookCacheEntryLocal (IDXGISwapChain1_Present1,
  L"dxgi.dll",                          Present1Callback,
                                       &Present1_Original);
SK_HookCacheEntryLocal (IDXGISwapChain_ResizeTarget,
  L"dxgi.dll",           DXGISwap_ResizeTarget_Override,
                                 &ResizeTarget_Original)
SK_HookCacheEntryLocal (IDXGISwapChain_ResizeBuffers,
  L"dxgi.dll",                DXGISwap_ResizeBuffers_Override,
                                      &ResizeBuffers_Original)
SK_HookCacheEntryLocal (IDXGISwapChain_GetFullscreenState,
  L"dxgi.dll",                DXGISwap_GetFullscreenState_Override,
                                      &GetFullscreenState_Original)
SK_HookCacheEntryLocal (IDXGISwapChain_SetFullscreenState,
  L"dxgi.dll",                DXGISwap_SetFullscreenState_Override,
                                      &SetFullscreenState_Original)
SK_HookCacheEntryLocal (CreateDXGIFactory,
  L"dxgi.dll",          CreateDXGIFactory,
                       &CreateDXGIFactory_Import)
SK_HookCacheEntryLocal (CreateDXGIFactory1,
  L"dxgi.dll",          CreateDXGIFactory1,
                       &CreateDXGIFactory1_Import)
SK_HookCacheEntryLocal (CreateDXGIFactory2,
  L"dxgi.dll",          CreateDXGIFactory2,
                       &CreateDXGIFactory2_Import)

// Counter-intuitively, the fewer of these we cache the more compatible we get.
static
sk_hook_cache_array global_dxgi_records =
  { &GlobalHook_IDXGIFactory_CreateSwapChain,
    &GlobalHook_IDXGIFactory2_CreateSwapChainForHwnd,

    &GlobalHook_IDXGISwapChain_Present,
    &GlobalHook_IDXGISwapChain1_Present1,

    &GlobalHook_IDXGIFactory2_CreateSwapChainForCoreWindow,
  //&GlobalHook_IDXGIFactory2_CreateSwapChainForComposition,

  //&GlobalHook_IDXGISwapChain_ResizeTarget,
  //&GlobalHook_IDXGISwapChain_ResizeBuffers,
  //&GlobalHook_IDXGISwapChain_GetFullscreenState,
  //&GlobalHook_IDXGISwapChain_SetFullscreenState,
  //&GlobalHook_CreateDXGIFactory,
  //&GlobalHook_CreateDXGIFactory1,
  //&GlobalHook_CreateDXGIFactory2
  };

static
sk_hook_cache_array local_dxgi_records =
  { &LocalHook_IDXGIFactory_CreateSwapChain,
    &LocalHook_IDXGIFactory2_CreateSwapChainForHwnd,

    &LocalHook_IDXGISwapChain_Present,
    &LocalHook_IDXGISwapChain1_Present1,

    &LocalHook_IDXGIFactory2_CreateSwapChainForCoreWindow,
  //&LocalHook_IDXGIFactory2_CreateSwapChainForComposition,
  //&LocalHook_IDXGISwapChain_ResizeTarget,
  //&LocalHook_IDXGISwapChain_ResizeBuffers,
  //&LocalHook_IDXGISwapChain_GetFullscreenState,
  //&LocalHook_IDXGISwapChain_SetFullscreenState,
  //&LocalHook_CreateDXGIFactory,
  //&LocalHook_CreateDXGIFactory1,
  //&LocalHook_CreateDXGIFactory2
  };



#define SK_LOG_ONCE(x) { static bool logged = false; if (! logged) \
                       { dll_log->Log ((x)); logged = true; } }

extern void SK_D3D11_EndFrame            (SK_TLS* pTLS = SK_TLS_Bottom ());
extern void SK_DXGI_UpdateSwapChain      (IDXGISwapChain*);

extern void SK_COMPAT_FixUpFullscreen_DXGI (bool Fullscreen);
extern          HWND hWndUpdateDlg;
extern volatile LONG __SK_TaskDialogActive;


//struct cfg_helper_s {
//  using framerate_s = sk_config_t::render_s::framerate_s;
//  using window_s    = sk_config_t::window_s;
//
//  sk_config_t &base      = config;
//  framerate_s &framerate = config.render.framerate;
//  window_s    &window    = config.window;
//} static cfg;




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

#include <CEGUI/Rect.h>
#include <CEGUI/RendererModules/Direct3D11/Renderer.h>

CEGUI::Direct3D11Renderer* cegD3D11 = nullptr;
#endif

void __stdcall SK_D3D11_ResetTexCache (void);

void SK_DXGI_HookSwapChain (IDXGISwapChain* pSwapChain);

extern bool __SK_HDR_16BitSwap;

void
ImGui_DX11Shutdown ( void )
{
  ImGui_ImplDX11_Shutdown ();
}

void
ImGui_DX12Shutdown ( void )
{
  ImGui_ImplDX12_Shutdown ();
}

bool
ImGui_DX11Startup ( IDXGISwapChain* pSwapChain )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComPtr <ID3D11Device>        pD3D11Dev         = nullptr;
  SK_ComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;

  //assert (pSwapChain == rb.swapchain ||
  //                      rb.swapchain.IsEqualObject (pSwapChain));

  if ( SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&pD3D11Dev.p))) )
  {
    assert (pD3D11Dev == rb.device ||
                         rb.device == nullptr );

    if (pD3D11Dev == nullptr)
      pD3D11Dev = rb.device;

    // ------------
    //SK_ComQIPtr <ID3D11Device>        pTestDev0 (rb.device);
    //SK_ComPtr   <ID3D11Device>        pTestDev1 = nullptr;
    //SK_ComQIPtr <ID3D11DeviceContext> pDevCtx   (rb.d3d11.immediate_ctx);
    //
    //if (pDevCtx != nullptr)
    //{
    //  pDevCtx->GetDevice (&pTestDev1);
    //
    //  if (pTestDev0.IsEqualObject (pTestDev1))
    //  {
    //    if (pImmediateContext != pDevCtx)
    //        pImmediateContext  = pDevCtx;
    //  }
    //}
    // -----------

    if (  rb.swapchain != pSwapChain             ||
          pD3D11Dev    != rb.device              ||
          nullptr      == rb.d3d11.immediate_ctx )
    {
      pD3D11Dev->GetImmediateContext (&pImmediateContext);
    }

    else
    {
      pImmediateContext = rb.d3d11.immediate_ctx;
    }

    assert (pImmediateContext == rb.d3d11.immediate_ctx ||
                                 rb.d3d11.immediate_ctx == nullptr);

    if ( pImmediateContext != nullptr )
    {
      if (! (( rb.device    == nullptr || rb.device   == pD3D11Dev  ) ||
             ( rb.swapchain == nullptr || rb.swapchain== (IUnknown *)pSwapChain )) )
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
        ImGui_ImplDX11_Init ( pSwapChain,
                pD3D11Dev,
        pImmediateContext
      );
    }
  }

  return false;
}

static volatile ULONG __gui_reset_dxgi   = TRUE;
static volatile ULONG __osd_frames_drawn = 0;

void
SK_CEGUI_RelocateLog (void)
{
  if (! config.cegui.enable) return;


  // Move the log file that this darn thing just created...
  if (GetFileAttributesW (L"CEGUI.log") != INVALID_FILE_ATTRIBUTES)
  {
    char     szNewLogPath [MAX_PATH + 2] = { };
    wchar_t wszNewLogPath [MAX_PATH + 2] = { };

    wcsncpy_s(wszNewLogPath, MAX_PATH, SK_GetConfigPath (), _TRUNCATE);
    strncpy_s( szNewLogPath, MAX_PATH,
                SK_WideCharToUTF8 (wszNewLogPath).c_str (), _TRUNCATE);

    lstrcatA ( szNewLogPath,  R"(logs\CEGUI.log)");
    lstrcatW (wszNewLogPath, LR"(logs\CEGUI.log)");

    CopyFileExW ( L"CEGUI.log", wszNewLogPath,
                    nullptr, nullptr, nullptr,
                      0x00 );

    try {
      CEGUI::Logger::getDllSingleton ().setLogFilename (reinterpret_cast <const CEGUI::utf8 *> (szNewLogPath), true);

      CEGUI::Logger::getDllSingleton ().logEvent       ("[Special K] ---- Log File Moved ----");
      CEGUI::Logger::getDllSingleton ().logEvent       ("");

      DeleteFileW (L"CEGUI.log");
    }

    catch (const CEGUI::FileIOException& e)
    {
      SK_LOG0 ( (L"CEGUI Exception During Log File Relocation"),
                L"   CEGUI  "  );
      SK_LOG0 ( (L" >> %hs (%hs:%lu): Exception %hs -- %hs",
                  e.getFunctionName    ().c_str (),
                  e.getFileName        ().c_str (),
                  e.getLine            (),
                          e.getName    ().c_str (),
                          e.getMessage ().c_str () ),
                 L"   CEGUI  "  );
    }
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
  try
  {
    if (parser->isPropertyPresent ("SchemaDefaultResourceGroup"))
      parser->setProperty ("SchemaDefaultResourceGroup", "schemas");
  }

  catch (...)
  {
    config.cegui.enable = false;
  }
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
        static_cast <CEGUI::DefaultResourceProvider *>
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

  catch (CEGUI::Exception& e)
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

DXGI_FORMAT
SK_DXGI_PickHDRFormat (DXGI_FORMAT fmt_orig)
{
  bool TenBitSwap     = false;
  bool SixteenBitSwap = false;

  extern bool      __SK_HDR_10BitSwap;
  TenBitSwap     = __SK_HDR_10BitSwap;

  extern bool      __SK_HDR_16BitSwap;
  SixteenBitSwap = __SK_HDR_16BitSwap;

  DXGI_FORMAT fmt_new = fmt_orig;

  if      (SixteenBitSwap) fmt_new = DXGI_FORMAT_R16G16B16A16_FLOAT;
  else if (TenBitSwap)     fmt_new = DXGI_FORMAT_R10G10B10A2_UNORM;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <IDXGISwapChain3> pSwap3 (rb.swapchain);

  if (pSwap3 != nullptr)
  {
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_full_desc = { };
        pSwap3->GetFullscreenDesc (&swap_full_desc);

    if (swap_full_desc.Windowed)
    {
      if (rb.scanout.dwm_colorspace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 && SixteenBitSwap)
      {
        fmt_new = DXGI_FORMAT_R16G16B16A16_FLOAT;

        DXGI_COLOR_SPACE_TYPE orig_space =
          rb.scanout.colorspace_override;

        rb.scanout.colorspace_override =
          DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;

        pSwap3->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);

        rb.scanout.colorspace_override =
          orig_space;
      }

      else if (TenBitSwap)
      {
        fmt_new = DXGI_FORMAT_R10G10B10A2_UNORM;

        DXGI_COLOR_SPACE_TYPE orig_space =
          rb.scanout.colorspace_override;

        rb.scanout.colorspace_override =
          DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

        pSwap3->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);

        rb.scanout.colorspace_override =
          orig_space;
      }
    }

    else
    {
      if (fmt_new == DXGI_FORMAT_R16G16B16A16_FLOAT)
      {
        DXGI_COLOR_SPACE_TYPE orig_space =
          rb.scanout.colorspace_override;

        rb.scanout.colorspace_override =
          DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;

        pSwap3->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);

        rb.scanout.colorspace_override =
          orig_space;
      }
    }
  }

  if (fmt_new != fmt_orig)
  {
    SK_LOG0 ((L" >> HDR: Overriding Original Format: '%s' with '%s'",
             SK_DXGI_FormatToStr (fmt_orig).c_str (),
             SK_DXGI_FormatToStr (fmt_new).c_str  ()),
             L"   DXGI   ");
  }

  return
    fmt_new;
}

void ResetCEGUI_D3D11 (IDXGISwapChain* This)
{
  SK_D3D11_ResetTexCache (    );

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  assert ( rb.swapchain == nullptr ||
   This == rb.swapchain );

  SK_ComPtr <ID3D11Device> pDev;

  if (SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev.p))) )
  {
    assert ( rb.device == nullptr ||
             pDev      == rb.device );

    rb.releaseOwnedResources (    );
    SK_DXGI_UpdateSwapChain  (This);
  }

  ///else if (rb.api == SK_RenderAPI::D3D11On12)
  ///{
  ///  rb.releaseOwnedResources (    );
  ///  SK_DXGI_UpdateSwapChain  (This);
  ///}


  // D3D12
#if 1
  if ( rb.swapchain == nullptr ||
       rb.device    == nullptr )
  {
    return;
  }
#endif

  if (cegD3D11 != nullptr)
  {
    if ((uintptr_t)cegD3D11 > 1     )//&& rb.api != SK_RenderAPI::D3D11On12)
    {
      SK_TextOverlayManager::getInstance ()->destroyAllOverlays ();
      SK_PopupManager::getInstance ()->destroyAllPopups         ();
    }

    rb.releaseOwnedResources ();

    if ((uintptr_t)cegD3D11 > 1     )//&& rb.api != SK_RenderAPI::D3D11On12)
    {
      cegD3D11 = nullptr;

      CEGUI::WindowManager::getDllSingleton    ().cleanDeadPool ();
      CEGUI::Direct3D11Renderer::destroySystem ();
    }

    // XXX: TODO (Full shutdown isn't necessary, just invalidate)
    ImGui_DX11Shutdown ();
  }

  ///else if (rb.api == SK_RenderAPI::D3D11On12)
  ///{
  ///  cegD3D11 =
  ///    reinterpret_cast <CEGUI::Direct3D11Renderer *> (1);
  ///
  ///  ImGui_DX11Startup (This);
  ///}

  else
  {
    assert (rb.device != nullptr);

    if (rb.device == nullptr)
      SK_DXGI_UpdateSwapChain (This);

    SK_ComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

    // For CEGUI to work correctly, it is necessary to set the viewport dimensions
    //   to the back buffer size prior to bootstrap.
    SK_ComPtr <ID3D11Texture2D> pBackBuffer   = nullptr;

    D3D11_VIEWPORT vp_orig = { };
    UINT           num_vp  =  1;

    if (pDevCtx != nullptr)
        pDevCtx->RSGetViewports (&num_vp, &vp_orig);

    SK_ComQIPtr <IDXGISwapChain3> pSwap3 (This);

                      UINT currentBuffer = 0;
    if (pSwap3 != nullptr) currentBuffer = pSwap3->GetCurrentBackBufferIndex ();

    if ( SUCCEEDED (This->GetBuffer (currentBuffer, IID_PPV_ARGS (&pBackBuffer.p))) )
    {
      D3D11_VIEWPORT                    vp = { };
      D3D11_TEXTURE2D_DESC backbuffer_desc = { };

      if (pBackBuffer)
          pBackBuffer->GetDesc (&backbuffer_desc);

      vp.Width    = static_cast <float> (backbuffer_desc.Width);
      vp.Height   = static_cast <float> (backbuffer_desc.Height);
      vp.MinDepth = 0.0f;
      vp.MaxDepth = 1.0f;
      vp.TopLeftX = 0.0f;
      vp.TopLeftY = 0.0f;

      if (pDevCtx != nullptr)
          pDevCtx->RSSetViewports (1, &vp);

      int thread_locale =
        _configthreadlocale (0);
        _configthreadlocale (_ENABLE_PER_THREAD_LOCALE);

      char* szLocale =
        setlocale (LC_ALL, nullptr);

      std::string locale_orig (
        szLocale != nullptr ? szLocale : ""
      );

      if (! locale_orig.empty ())
        setlocale (LC_ALL, "C");

      if (config.cegui.enable)
      {
        SK_ComQIPtr <ID3D11Device>        pCEGUIDev    (rb.device);
        SK_ComQIPtr <ID3D11DeviceContext> pCEGUIDevCtx (rb.d3d11.immediate_ctx);

        try {
          cegD3D11 = static_cast <CEGUI::Direct3D11Renderer *>
            (&CEGUI::Direct3D11Renderer::bootstrapSystem (
              pCEGUIDev,
                pCEGUIDevCtx
             )
            );
        }

        catch (CEGUI::Exception& e)
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

          cegD3D11            = nullptr;
          config.cegui.enable = false;
        }
      }
      else
        cegD3D11 = reinterpret_cast <CEGUI::Direct3D11Renderer *> (1);

      ImGui_DX11Startup    (This);

      if ((uintptr_t)cegD3D11 > 1)
        SK_CEGUI_RelocateLog (    );

      if (pDevCtx != nullptr)
          pDevCtx->RSSetViewports (1, &vp_orig);

      if ((uintptr_t)cegD3D11 > 1)
      {
        if (! locale_orig.empty ())
          setlocale (LC_ALL, "C");

        SK_CEGUI_InitBase    ();

        SK_PopupManager::getInstance       ()->destroyAllPopups (        );
        SK_TextOverlayManager::getInstance ()->resetAllOverlays (cegD3D11);
      }

      SK_Steam_ClearPopups ();

      if (! locale_orig.empty ())
        setlocale (LC_ALL, locale_orig.c_str ());

      _configthreadlocale (thread_locale);
    }

    else
    {
      dll_log->Log ( L"[   DXGI   ]  ** Failed to acquire SwapChain's Backbuffer;"
                     L" will try again next frame." );
      SK_D3D11_ResetTexCache ();
    }
  }
}

DWORD dwRenderThread = 0x0000;

//extern void  __stdcall SK_D3D11_TexCacheCheckpoint (void);
//extern void  __stdcall SK_D3D11_UpdateRenderStats  (IDXGISwapChain* pSwapChain);

extern BOOL __stdcall SK_NvAPI_SetFramerateLimit   (uint32_t        limit);
extern void __stdcall SK_NvAPI_SetAppFriendlyName  (const wchar_t*  wszFriendlyName);

static volatile LONG  __dxgi_ready  = FALSE;

void WaitForInitDXGI (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (pTLS && pTLS->d3d11->ctx_init_thread)
    return;

  // This is a hybrid spin; it will spin for up to 250 iterations before sleeping
  SK_Thread_SpinUntilFlagged (&__dxgi_ready);
}

DWORD __stdcall HookDXGI (LPVOID user);

#define D3D_FEATURE_LEVEL_12_0 0xc000
#define D3D_FEATURE_LEVEL_12_1 0xc100

const wchar_t*
SK_DXGI_DescribeScalingMode (DXGI_MODE_SCALING mode) noexcept
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
SK_DXGI_DescribeScanlineOrder (DXGI_MODE_SCANLINE_ORDER order) noexcept
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

#define DXGI_SWAP_EFFECT_FLIP_DISCARD 4

const wchar_t*
SK_DXGI_DescribeSwapEffect (DXGI_SWAP_EFFECT swap_effect) noexcept
{
  switch ((int)swap_effect)
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
SK_DXGI_DescribeSwapChainFlags (DXGI_SWAP_CHAIN_FLAG swap_flags, INT* pLines)
{
  std::wstring out;

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_NONPREROTATED)
    out += L"Non-Pre Rotated\n", pLines ? (*pLines)++ : 0;

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
    out += L"Allow Fullscreen Mode Switch\n", pLines ? (*pLines)++ : 0;

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE)
    out += L"GDI Compatible\n", pLines ? (*pLines)++ : 0;

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT)
    out += L"DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT\n", pLines ? (*pLines)++ : 0;

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER)
    out += L"DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER\n", pLines ? (*pLines)++ : 0;

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
    out += L"Latency Waitable\n", pLines ? (*pLines)++ : 0;

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER)
    out += L"DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER\n", pLines ? (*pLines)++ : 0;

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO)
    out += L"DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO\n", pLines ? (*pLines)++ : 0;

  #define DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED 1024

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED)
    out += L"DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED\n", pLines ? (*pLines)++ : 0;

  if (swap_flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
    out += L"Supports Tearing in Windowed Mode\n", pLines ? (*pLines)++ : 0;

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



#define __NIER_HACK
//#define __CROSSCODE_HACK

UINT
SK_DXGI_FixUpLatencyWaitFlag (IDXGISwapChain *pSwapChain, UINT Flags);


extern int                      gpu_prio;

bool bAlwaysAllowFullscreen = false;
bool bFlipMode              = false;
bool bWait                  = false;

// Used for integrated GPU override
int              SK_DXGI_preferred_adapter = -1;


void
WINAPI
SKX_D3D11_EnableFullscreen (bool bFullscreen)
{
  bAlwaysAllowFullscreen = bFullscreen;
}

//extern
//DWORD __stdcall HookD3D11                   (LPVOID user);

void            SK_DXGI_HookPresent         (IDXGISwapChain* pSwapChain);
void  WINAPI    SK_DXGI_SetPreferredAdapter (int override_id) noexcept;


enum SK_DXGI_ResType {
  WIDTH  = 0,
  HEIGHT = 1
};

inline bool
SK_DXGI_RestrictResMax ( SK_DXGI_ResType dim,
                                   int&  last,
                                   int   idx,
                         std::set <int>& covered,
             gsl::not_null <DXGI_MODE_DESC*> pDescRaw )
 {
  auto pDesc =
    pDescRaw.get ();

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

inline bool
SK_DXGI_RestrictResMin ( SK_DXGI_ResType dim,
                                    int& first,
                                    int  idx,
                         std::set <int>& covered,
                         DXGI_MODE_DESC* pDesc )
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
SK_DescribeVirtualProtectFlags (DWORD dwProtect) noexcept
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
      SK_Thread_Create ( HookDXGI );
#else
    HookDXGI (nullptr);
#endif
  }

  else
    WaitForInitDXGI ();
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
     _default_impl=(passthrough_pfn)SK_GetProcAddress(backend_dll,szName);\
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log->Log (                                                    \
          L"Unable to locate symbol  %s in dxgi.dll",                     \
          L#_Name);                                                       \
        return (_Return)E_NOTIMPL;                                        \
      }                                                                   \
    }                                                                     \
                                                                          \
    dll_log->Log (L"[   DXGI   ] [!] %s %s - "                            \
             L"[Calling Thread: 0x%04x]",                                 \
      L#_Name, L#_Proto, SK_Thread_GetCurrentId ());                      \
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
     _default_impl=(passthrough_pfn)SK_GetProcAddress(backend_dll,szName);\
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log->Log (                                                    \
          L"Unable to locate symbol  %s in dxgi.dll",                     \
          L#_Name );                                                      \
        return;                                                           \
      }                                                                   \
    }                                                                     \
                                                                          \
    dll_log->Log (L"[   DXGI   ] [!] %s %s - "                            \
              L"[Calling Thread: 0x%04x]",                                \
      L#_Name, L#_Proto, SK_Thread_GetCurrentId ());                      \
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
  SK_ComPtr <IDXGIFactory5> pTemp;

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory5> ((IDXGIFactory5 **)(void **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    dxgi_caps.present.waitable        = true;
    dxgi_caps.present.flip_discard    = true;

    const HRESULT hr =
      pTemp->CheckFeatureSupport (
        DXGI_FEATURE_PRESENT_ALLOW_TEARING,
          &dxgi_caps.swapchain.allow_tearing,
            sizeof (dxgi_caps.swapchain.allow_tearing)
      );

    dxgi_caps.swapchain.allow_tearing =
      SUCCEEDED (hr) && dxgi_caps.swapchain.allow_tearing;

    return 5;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory4> ((IDXGIFactory4 **)(void **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    dxgi_caps.present.waitable        = true;
    dxgi_caps.present.flip_discard    = true;
    return 4;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory3> ((IDXGIFactory3 **)(void **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    dxgi_caps.present.waitable        = true;
    return 3;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory2> ((IDXGIFactory2 **)(void **)&pTemp)))
  {
    dxgi_caps.device.enqueue_event    = true;
    dxgi_caps.device.latency_control  = true;
    dxgi_caps.present.flip_sequential = true;
    return 2;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory1> ((IDXGIFactory1 **)(void **)&pTemp)))
  {
    dxgi_caps.device.latency_control  = true;
    return 1;
  }

  if (SUCCEEDED (
    pFactory->QueryInterface <IDXGIFactory> ((IDXGIFactory **)(void **)&pTemp)))
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
  if (riid == __uuidof (IDXGIAdapter4))
    return 4;

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
  else if (riid == __uuidof (IDXGIAdapter4))
    interface_name = L"IDXGIAdapter4";
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
  SK_ComPtr <IUnknown> pTemp;

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
  SK_D3D11_EndFrame       ();

  if (cegD3D11 == (CEGUI::Direct3D11Renderer *)1)
    cegD3D11 = nullptr;

  InterlockedExchange (&__gui_reset_dxgi, TRUE);
}

typedef HRESULT (WINAPI *IDXGISwapChain3_CheckColorSpaceSupport_pfn)
                        (IDXGISwapChain3*, DXGI_COLOR_SPACE_TYPE, UINT*);
                         IDXGISwapChain3_CheckColorSpaceSupport_pfn
                         IDXGISwapChain3_CheckColorSpaceSupport_Original =nullptr;

typedef HRESULT (WINAPI *IDXGISwapChain3_SetColorSpace1_pfn)
                        (IDXGISwapChain3*, DXGI_COLOR_SPACE_TYPE);
                         IDXGISwapChain3_SetColorSpace1_pfn
                         IDXGISwapChain3_SetColorSpace1_Original = nullptr;

typedef HRESULT (WINAPI *IDXGISwapChain4_SetHDRMetaData_pfn)
                        (IDXGISwapChain4*, DXGI_HDR_METADATA_TYPE, UINT, void*);
                         IDXGISwapChain4_SetHDRMetaData_pfn
                         IDXGISwapChain4_SetHDRMetaData_Original = nullptr;

const uint8_t
  edid_v1_header [] =
    { 0x00, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0x00 };

const uint8_t
  edid_v1_descriptor_flag [] =
    { 0x00, 0x00 };

const wchar_t*
DXGIColorSpaceToStr (DXGI_COLOR_SPACE_TYPE space) noexcept;



#define EDID_LENGTH                        0x80
#define EDID_HEADER                        0x00
#define EDID_HEADER_END                    0x07

#define ID_MANUFACTURER_NAME               0x08
#define ID_MANUFACTURER_NAME_END           0x09

#define EDID_STRUCT_VERSION                0x12
#define EDID_STRUCT_REVISION               0x13

#define UNKNOWN_DESCRIPTOR           (uint8_t)1
#define DETAILED_TIMING_BLOCK        (uint8_t)2
#define DESCRIPTOR_DATA                       5

#define DETAILED_TIMING_DESCRIPTIONS_START 0x36
#define DETAILED_TIMING_DESCRIPTION_SIZE     18
#define NUM_DETAILED_TIMING_DESCRIPTIONS      4

const uint8_t MONITOR_NAME                = 0xfc;

SK_LazyGlobal <std::string> edid_name;

static uint8_t
blockType (uint8_t* block) noexcept
{
  if (  block [0] == 0 &&
        block [1] == 0 &&
        block [2] == 0 &&
        block [3] != 0 &&
        block [4] == 0    )
  {
    if (block [3] >= (uint8_t) 0xFA)
    {
      return
        block [3];
    }
  }

  return
    UNKNOWN_DESCRIPTOR;
}

static char*
SK_EDID_GetMonitorNameFromBlock ( uint8_t const* block )
{
  static char name [13] = { };
  unsigned    i;

  if (name [0] != '\0')
    return name;

  uint8_t const* ptr =
    block + DESCRIPTOR_DATA;

  for (i = 0; i < 13; ++i, ++ptr)
  {
    if (*ptr == 0xa)
    {
      name [i] = '\0';

      return
        name;
    }

    name [i] = *ptr;
  }

  return name;
}

static bool
SK_EDID_Parse (uint8_t *edid, size_t length)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  unsigned int i        = 0;
  uint8_t*     block    = 0;
  uint8_t      checksum = 0;

  for (i = 0; i < length; ++i)
    checksum += edid [i];

  // Bad checksum, fail EDID
  if (checksum != 0)
  {
    dll_log->Log (L"SK_EDID_Parse (...): Checksum fail");
    return false;
  }

  if ( 0 != memcmp ( (const char*)edid          + EDID_HEADER,
                     (const char*)edid_v1_header, EDID_HEADER_END + 1 ) )

  {
    dll_log->Log (L"SK_EDID_Parse (...): Not V1 Header");

    // Not a V1 header
    return false;
  }

  // Monitor name and timings
  block =
    &edid [DETAILED_TIMING_DESCRIPTIONS_START];

  uint8_t *end =
    &edid [length - 1];

#if 0
  int byte_idx = 0;

  while (block < end)
  {
    dll_log->Log (L"Byte %lu : %u", byte_idx++, (uint32_t) (*block));
    ++block;
  }
#endif

  while (block < end)
  {
    uint8_t type =
      blockType (block);

    switch (type)
    {
      case DETAILED_TIMING_BLOCK:
        block += DETAILED_TIMING_DESCRIPTION_SIZE;
        break;

      case UNKNOWN_DESCRIPTOR:
        ++block;
        break;

      case MONITOR_NAME:
      {
        uint8_t vendorString [5] = { };

        vendorString [0] =  (edid [8] >> 2   & 31)
                                             + 64;
        vendorString [1] = ((edid [8]  & 3) << 3) |
                            (edid [9] >> 5)  + 64;
        vendorString [2] =  (edid [9]  & 31) + 64;

        if ( vendorString [0] == 'A' &&
             vendorString [1] == 'U' &&
             vendorString [2] == 'S' )
        {
          vendorString [1] = 'S'; vendorString [2] = 'U';
          vendorString [3] = 'S'; vendorString [4] = '\0';
        }

        *edid_name = (const char *)vendorString;
        *edid_name += " ";
        *edid_name +=
          SK_EDID_GetMonitorNameFromBlock (block);

        bool one_pt_4 =
          ((((unsigned) edid [10]) & 0xffU) == 4);

#define EDID_LOG2(x) {                              \
          if (one_pt_4) SK_LOG2 (x, L" EDID 1.4 "); \
          else          SK_LOG2 (x, L" EDID 1.3 "); }

        EDID_LOG2 ( ( L"SK_EDID_Parse (...): [ Name: %hs ]",
                      edid_name->c_str () ) );

        wsprintf ( rb.display_name,
                      L"%hs", edid_name->c_str () );

      //dll_log->Log (L"EDID Name: %hs", edid_name->c_str ());

      block += DETAILED_TIMING_DESCRIPTION_SIZE;
    } break;

    default:
      ++block;
      break;
    }
  }

  return true;
}

//#include "../depends/include/nvapi/nvapi_lite_common.h"

static
const GUID
      GUID_CLASS_MONITOR =
    { 0x4d36e96e, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 };

std::wstring
SK_GetKeyPathFromHKEY (HKEY& key)
{
#define STATUS_SUCCESS          ((NTSTATUS)0x00000000L)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)

  std::wstring keyPath;

  if (key != nullptr)
  {
    typedef DWORD (__stdcall *NtQueryKey_pfn)(
                       HANDLE KeyHandle,
                       int    KeyInformationClass,
                       PVOID  KeyInformation,
                       ULONG  Length,
                       PULONG ResultLength );

    static
      NtQueryKey_pfn    NtQueryKey =
      reinterpret_cast <NtQueryKey_pfn> (
        SK_GetProcAddress ( L"NtDll.dll",
                             "NtQueryKey" ) );

    if (NtQueryKey != nullptr)
    {
      DWORD size   = 0;
      DWORD result = 0;

      result =
        NtQueryKey (key, 3, nullptr, 0, &size);

      if (result == STATUS_BUFFER_TOO_SMALL)
      {
        size += 2;

        wchar_t* buffer =
          new (std::nothrow) wchar_t [size / sizeof (wchar_t)];

        if (buffer != nullptr)
        {
          result =
            NtQueryKey (key, 3, buffer, size, &size);

          if (result == STATUS_SUCCESS)
          {
            buffer [size / sizeof (wchar_t)] = L'\0';

            keyPath =
              std::wstring (buffer + 2);
          }

          delete [] buffer;
        }
      }
    }
  }

  return
    keyPath;
}


void
SK_DXGI_UpdateColorSpace (IDXGISwapChain3* This)
{
  if (This != nullptr)
  {
    static auto& rb =
      SK_GetCurrentRenderBackend ();

    SK_ComPtr <IDXGIOutput> pOutput;

    if (SUCCEEDED (This->GetContainingOutput (&pOutput.p)))
    {
      SK_ComQIPtr <IDXGIOutput6> pOut6 (pOutput);

      if (pOut6 != nullptr)
      {
        DXGI_OUTPUT_DESC1                outDesc1 = { };
        if (SUCCEEDED (pOut6->GetDesc1 (&outDesc1)))
        {
          // SDR (sRGB)
          if (outDesc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
          {
            rb.setHDRCapable (false);
          }

          else if (
            outDesc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020     ||
            outDesc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709        ||
            outDesc1.ColorSpace == DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601 ||
            outDesc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020)
          {
            rb.setHDRCapable (true);
          }

          else
          {
            rb.setHDRCapable (true);

            SK_LOG0 ( ( L"Unexpected IDXGIOutput6 ColorSpace: %s",
                          DXGIColorSpaceToStr (outDesc1.ColorSpace) ),
                        L"   DXGI   ");
          }

          rb.scanout.bpc             = outDesc1.BitsPerColor;
          rb.scanout.dxgi_colorspace = outDesc1.ColorSpace;

          if (! edid_name->empty ())
          {
            wsprintf ( rb.display_name,
                         LR"(%hs)",
                           edid_name->c_str () );
          }

          if (*rb.display_name == L'\0')
          {
            wsprintf (rb.display_name, L"UNKNOWN");

            HDEVINFO devInfo =
              SetupDiGetClassDevsEx ( &GUID_CLASS_MONITOR,
                                        nullptr, nullptr,
                                          DIGCF_PRESENT,
                                            nullptr, nullptr, nullptr );

            if (devInfo != nullptr)
            {
              for ( ULONG i = 0 ; ERROR_NO_MORE_ITEMS != GetLastError (); ++i )
              {
                SP_DEVINFO_DATA devInfoData = { };

                devInfoData.cbSize = sizeof (devInfoData);

                if ( SetupDiEnumDeviceInfo ( devInfo,
                                               i,
                                                 &devInfoData )
                   )
                {
                  HKEY hDevRegKey =
                    SetupDiOpenDevRegKey ( devInfo,
                                          &devInfoData,
                                             DICS_FLAG_GLOBAL, 0,
                                               DIREG_DEV,      KEY_READ );


                  if ( (! hDevRegKey) ||
                         (hDevRegKey == INVALID_HANDLE_VALUE) )
                    continue;

                  //std::wstring fullRegistryKey =
                  //  SK_GetKeyPathFromHKEY (hDevRegKey);

                  uint8_t EDID_Data [256] = { };
                  wchar_t valueName [512] = { };
                  DWORD   valueLen        =  511;
                  DWORD   edid_size       =  sizeof (EDID_Data);

                  for ( LONG j         = 0,
                             retValue  = ERROR_SUCCESS;
                             retValue != ERROR_NO_MORE_ITEMS;
                                       ++j )
                  {
                    *valueName = L'\0';

                    DWORD dwType;

                    retValue =
                      RegEnumValue ( hDevRegKey, j, valueName,
                                     &valueLen,  nullptr,
                                       &dwType,  EDID_Data,
                                         &edid_size );

                    if ( retValue != ERROR_SUCCESS  ||
                         wcscmp (valueName, L"EDID")  )
                      continue;

                    if (! wcscmp (valueName, L"EDID"))
                    {
                      if ( SK_EDID_Parse ( EDID_Data,
                                             edid_size )
                         )
                      {
                        break;
                      }
                    }
                  }

                  RegCloseKey (hDevRegKey);
                }
              }
            }

            *rb.display_name = L'\0';

             //@TODO - ELSE: Test support for various HDR colorspaces.

            DISPLAY_DEVICEW        disp_desc = { };
            disp_desc.cb = sizeof (disp_desc);

            UINT devIdx = 0;

            swscanf ( outDesc1.DeviceName,
                        LR"(\\.\DISPLAY%ui)",
                          &devIdx );

            if (devIdx != 0)
                devIdx --;

            if (EnumDisplayDevices (nullptr, devIdx, &disp_desc, 0))
            {
              std::wstring wszDevName (
                disp_desc.DeviceName
              );

              int mon_idx = 0;

              if (EnumDisplayDevices (wszDevName.c_str (), mon_idx, &disp_desc, 0))
              {
                wsprintf ( rb.display_name, edid_name->empty () ?
                                              LR"(%ws (%ws))" :
                                                L"%hs",
                             edid_name->empty () ?
                               disp_desc.DeviceString :
                      SK_UTF8ToWideChar (*edid_name).c_str (),
                                 disp_desc.DeviceName );
              }
            }
          }
        }
      }

      if (rb.scanout.colorspace_override != DXGI_COLOR_SPACE_CUSTOM)
      {
        SK_ComQIPtr <IDXGISwapChain3> pSwap3 (This);

        if ( pSwap3 != nullptr &&
             SUCCEEDED (
               IDXGISwapChain3_SetColorSpace1_Original ( pSwap3,
               //pSwap3->SetColorSpace1 (
                 rb.scanout.colorspace_override
               )
             )
           )
        {
          rb.scanout.dxgi_colorspace =
            rb.scanout.colorspace_override;
        }
      }
    }
  }
}


volatile LONG __SK_NVAPI_UpdateGSync = FALSE;

void
SK_DXGI_UpdateSwapChain (IDXGISwapChain* This)
{
  if (This == nullptr)
    return;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <IDXGISwapChain> pRealSwap (This);
  SK_ComPtr   <ID3D11Device>   pDev;

  if ( SUCCEEDED (This->GetDevice (IID_PPV_ARGS (&pDev.p))) )
  {
    // These operations are not atomic / cache coherent in
    //   ReShade (bug!!), so to avoid prematurely freeing
    //     this stuff don't release and re-acquire a
    //       reference to the same object.

    if (! rb.interop.d3d12.dev)
    {
      if (! pDev.IsEqualObject (rb.device))
          rb.device    = pDev.p;
    }
    else pDev = rb.device;

    if (! pRealSwap.IsEqualObject (rb.swapchain))
        rb.swapchain = pRealSwap.p;

    SK_ComPtr <ID3D11DeviceContext> pDevCtx;

    pDev->GetImmediateContext (
           (ID3D11DeviceContext **)&pDevCtx.p);
    rb.d3d11.immediate_ctx    =     pDevCtx;

    //rb.d3d11.deferred_ctx     =   nullptr;
    //pDev->CreateDeferredContext (0x0, &rb.d3d11.deferred_ctx.p);

    //if (rb.d3d11.deferred_ctx != nullptr)
    //    rb.d3d11.deferred_ctx.Release ();
    //
    //pDev->CreateDeferredContext (0x0, &rb.d3d11.deferred_ctx);

    if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
    {
      InterlockedExchange (&__SK_NVAPI_UpdateGSync, TRUE);
    }
  }

  else
  {
    assert (rb.device != nullptr);

    if (! pRealSwap.IsEqualObject (rb.swapchain))
          rb.swapchain =         pRealSwap.p;

    //dll_log->Log (
    //  L"UpdateSwapChain FAIL :: No D3D11 Device [ Actually: %ph ]",
    //    rb.device
    //);
  }

  SK_ComQIPtr <IDXGISwapChain3> pSwap3 (This);

  if (pSwap3 != nullptr) SK_DXGI_UpdateColorSpace (
      pSwap3.p
  );
}


template <class _T>
static
__forceinline
UINT
calc_count (_T** arr, UINT max_count) noexcept
{
  for ( int i = gsl::narrow_cast <int> (max_count) - 1 ;
            i >= 0 ;
          --i )
  {
    if (arr [i] != nullptr)
      return i + 1;
  }

  return max_count;
}

#define SK_D3D11_MAX_SCISSORS \
  D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE

#define D3D11_SHADER_MAX_INSTANCES_PER_CLASS 256

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
  ID3D11ShaderResourceView*  PSShaderResources [2];
  ID3D11SamplerState*        PSSampler;
  ID3D11PixelShader*         PS;
  ID3D11VertexShader*        VS;
  ID3D11GeometryShader*      GS;
  ID3D11HullShader*          HS;
  ID3D11DomainShader*        DS;
  UINT                       PSInstancesCount, VSInstancesCount, GSInstancesCount,
                             HSInstancesCount, DSInstancesCount;
  ID3D11ClassInstance       *PSInstances  [D3D11_SHADER_MAX_INTERFACES],
                            *VSInstances  [D3D11_SHADER_MAX_INTERFACES],
                            *GSInstances  [D3D11_SHADER_MAX_INTERFACES],
                            *HSInstances  [D3D11_SHADER_MAX_INTERFACES],
                            *DSInstances  [D3D11_SHADER_MAX_INTERFACES];
  D3D11_PRIMITIVE_TOPOLOGY   PrimitiveTopology;
  ID3D11Buffer              *IndexBuffer,
                            *VertexBuffer,
                            *VSConstantBuffer,
                            *PSConstantBuffer;
  UINT                       IndexBufferOffset, VertexBufferStride,
                             VertexBufferOffset;
  DXGI_FORMAT                IndexBufferFormat;
  ID3D11InputLayout*         InputLayout;

  ID3D11DepthStencilView*    DepthStencilView;
  ID3D11RenderTargetView*    RenderTargetView;
};

//extern D3D11_PSSetSamplers_pfn D3D11_PSSetSamplers_Original;

struct SK_D3D11_Stateblock_Lite : StateBlockDataStore
{
  void capture (ID3D11DeviceContext* pCtx)
  {
    if (pCtx == nullptr)
      return;

    SK_ComPtr <ID3D11Device> pDev;

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    ///if (rb.api != SK_RenderAPI::D3D11On12)
      pCtx->GetDevice (&pDev.p);
    ///else
    ///{
    ///  rb.d3d11.wrapper_dev->QueryInterface <ID3D11Device> (&pDev.p);
    ///  pDev->GetImmediateContext (&pCtx);
    ///}

    if ( pDev == nullptr )
      return;

    D3D_FEATURE_LEVEL ft_lvl =
      pDev->GetFeatureLevel ();

    ScissorRectsCount = ViewportsCount =
      D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

    pCtx->RSGetScissorRects      (      &ScissorRectsCount, ScissorRects);
    pCtx->RSGetViewports         (      &ViewportsCount,    Viewports);
    pCtx->RSGetState             (      &RS);
    pCtx->OMGetBlendState        (      &BlendState,         BlendFactor,
                                                            &SampleMask);
    pCtx->OMGetDepthStencilState (      &DepthStencilState, &StencilRef);
  //pCtx->PSGetShaderResources   (0, 2, PSShaderResources);
    pCtx->PSGetConstantBuffers   (0, 1, &PSConstantBuffer);
    pCtx->PSGetSamplers          (0, 1, &PSSampler);

    PSInstancesCount = VSInstancesCount = GSInstancesCount =
    HSInstancesCount = DSInstancesCount  =
      D3D11_SHADER_MAX_INTERFACES;// D3D11_SHADER_MAX_INSTANCES_PER_CLASS;

    pCtx->PSGetShader            (&PS, PSInstances, &PSInstancesCount);
    pCtx->VSGetShader            (&VS, VSInstances, &VSInstancesCount);

    if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
    {
      pCtx->GSGetShader             (&GS, GSInstances, &GSInstancesCount);
      GSInstancesCount =     calc_count ( GSInstances,  GSInstancesCount);
    }

    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
    {
      pCtx->HSGetShader            (&HS, HSInstances, &HSInstancesCount);
      HSInstancesCount =     calc_count (HSInstances,  HSInstancesCount);

      pCtx->DSGetShader            (&DS, DSInstances, &DSInstancesCount);
      DSInstancesCount =     calc_count (DSInstances,  DSInstancesCount);
    }

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
    if (pCtx == nullptr)
      return;

    SK_ComPtr <ID3D11Device> pDev;

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    ///if (rb.api != SK_RenderAPI::D3D11On12)
      pCtx->GetDevice (&pDev.p);
    ///else
    ///{
    ///  rb.d3d11.wrapper_dev->QueryInterface <ID3D11Device> (&pDev.p);
    ///  pDev->GetImmediateContext                           (&pCtx);
    ///}

    if ( pDev == nullptr )
      return;

    D3D_FEATURE_LEVEL ft_lvl =
      pDev->GetFeatureLevel ();

    pCtx->RSSetScissorRects      (ScissorRectsCount, ScissorRects);
    pCtx->RSSetViewports         (ViewportsCount,    Viewports);
    pCtx->OMSetDepthStencilState (DepthStencilState, StencilRef);
    pCtx->RSSetState             (RS);
    pCtx->PSSetShader            (PS, PSInstances,   PSInstancesCount);
    pCtx->VSSetShader            (VS, VSInstances,   VSInstancesCount);
    if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
      pCtx->GSSetShader          (GS, GSInstances,   GSInstancesCount);
    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
    {
      pCtx->HSSetShader          (HS, HSInstances,   HSInstancesCount);
      pCtx->DSSetShader          (DS, DSInstances,   DSInstancesCount);
    }
    pCtx->OMSetBlendState        (BlendState,        BlendFactor,
                                                     SampleMask);
    pCtx->IASetIndexBuffer       (IndexBuffer,       IndexBufferFormat,
                                                     IndexBufferOffset);
    pCtx->IASetInputLayout       (InputLayout);
    pCtx->IASetPrimitiveTopology (PrimitiveTopology);
  //pCtx->PSSetShaderResources   (0, 2, PSShaderResources);
    pCtx->PSSetConstantBuffers   (0, 1, &PSConstantBuffer);
    pCtx->VSSetConstantBuffers   (0, 1, &VSConstantBuffer);
    pCtx->PSSetSamplers          (0, 1, &PSSampler);
    pCtx->IASetVertexBuffers     (0, 1, &VertexBuffer,    &VertexBufferStride,
                                                          &VertexBufferOffset);
    pCtx->OMSetRenderTargets     (1,    &RenderTargetView, DepthStencilView);

    if (RS)                    RS->Release                    ();
    if (PS)                    PS->Release                    ();
    if (VS)                    VS->Release                    ();
    if (ft_lvl >= D3D_FEATURE_LEVEL_10_0 &&
        GS)                    GS->Release                    ();
    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0 &&
        HS)                    HS->Release                    ();
    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0 &&
        DS)                    DS->Release                    ();
    if (PSSampler)             PSSampler->Release             ();
    if (BlendState)            BlendState->Release            ();
    if (InputLayout)           InputLayout->Release           ();
    if (IndexBuffer)           IndexBuffer->Release           ();
    if (VertexBuffer)          VertexBuffer->Release          ();
  //if (PSShaderResources [0]) PSShaderResources [0]->Release ();
  //if (PSShaderResources [1]) PSShaderResources [1]->Release ();
    if (VSConstantBuffer)      VSConstantBuffer->Release      ();
    if (PSConstantBuffer)      PSConstantBuffer->Release      ();
    if (RenderTargetView)      RenderTargetView->Release      ();
    if (DepthStencilView)      DepthStencilView->Release      ();
    if (DepthStencilState)     DepthStencilState->Release     ();

    //
    // Now balance the reference counts that D3D added even though we did not want them :P
    //
    for (UINT i = 0; i < VSInstancesCount; i++)
    {
      if (VSInstances [i])
          VSInstances [i]->Release ();
    }

    for (UINT i = 0; i < PSInstancesCount; i++)
    {
      if (PSInstances [i])
          PSInstances [i]->Release ();
    }

    if (ft_lvl >= D3D_FEATURE_LEVEL_10_0)
    {
      for (UINT i = 0; i < GSInstancesCount; i++)
      {
        if (GSInstances [i])
            GSInstances [i]->Release ();
      }
    }

    if (ft_lvl >= D3D_FEATURE_LEVEL_11_0)
    {
      for (UINT i = 0; i < HSInstancesCount; i++)
      {
        if (HSInstances [i])
            HSInstances [i]->Release ();
      }

      for (UINT i = 0; i < DSInstancesCount; i++)
      {
        if (DSInstances [i])
            DSInstances [i]->Release ();
      }
    }
  }
};

void
SK_D3D11_CaptureStateBlock ( ID3D11DeviceContext*       pImmediateContext,
                             SK_D3D11_Stateblock_Lite** pSB )
{
  if (pSB != nullptr)
  {
    if (*pSB == nullptr)
    {
      *pSB = new SK_D3D11_Stateblock_Lite ();
    }

    RtlSecureZeroMemory ( *pSB,
                      sizeof (SK_D3D11_Stateblock_Lite) );

    (*pSB)->capture (pImmediateContext);
  }
}

void
SK_D3D11_ApplyStateBlock ( SK_D3D11_Stateblock_Lite* pBlock,
                           ID3D11DeviceContext*      pDevCtx )
{
  if (pBlock != nullptr && pDevCtx != nullptr)
      pBlock->apply (pDevCtx);
}

SK_D3D11_Stateblock_Lite*
SK_D3D11_CreateAndCaptureStateBlock (ID3D11DeviceContext* pImmediateContext)
{
  ///// Uses TLS to reduce dynamic memory pressure as much as possible
  ///auto* sb =
    //SK_TLS_Bottom ()->d3d11.getStateBlock ();
  SK_D3D11_Stateblock_Lite* sb = new SK_D3D11_Stateblock_Lite ();

  RtlSecureZeroMemory ( sb,
                  sizeof (SK_D3D11_Stateblock_Lite) );

  sb->capture (pImmediateContext);

  return sb;
}

void
SK_D3D11_ReleaseAndApplyStateBlock ( SK_D3D11_Stateblock_Lite* pBlock,
                                     ID3D11DeviceContext*      pDevCtx )
{
  if (pBlock == nullptr)
    return;

         pBlock->apply (pDevCtx);
  delete pBlock;
}

struct D3DX11_STATE_BLOCK
{
  ID3D11VertexShader*        VS;
  ID3D11SamplerState*        VSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  VSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              VSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       VSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS];
  UINT                       VSInterfaceCount;

  ID3D11GeometryShader*      GS;
  ID3D11SamplerState*        GSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  GSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              GSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       GSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS];
  UINT                       GSInterfaceCount;

  ID3D11HullShader*          HS;
  ID3D11SamplerState*        HSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  HSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              HSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       HSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS];
  UINT                       HSInterfaceCount;

  ID3D11DomainShader*        DS;
  ID3D11SamplerState*        DSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  DSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              DSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       DSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS];
  UINT                       DSInterfaceCount;

  ID3D11PixelShader*         PS;
  ID3D11SamplerState*        PSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  PSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              PSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       PSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS];
  UINT                       PSInterfaceCount;

  ID3D11ComputeShader*       CS;
  ID3D11SamplerState*        CSSamplers             [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11ShaderResourceView*  CSShaderResources      [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11Buffer*              CSConstantBuffers      [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
  ID3D11ClassInstance*       CSInterfaces           [D3D11_SHADER_MAX_INSTANCES_PER_CLASS];
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
  if (dc == nullptr)
    return;

  SK_ComPtr <ID3D11Device> pDev;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  ///if (rb.api != SK_RenderAPI::D3D11On12)
    dc->GetDevice (&pDev.p);
  ///else
  ///  rb.d3d11.wrapper_dev->QueryInterface <ID3D11Device> (&pDev.p);

  if ( pDev == nullptr )
    return;

  const D3D_FEATURE_LEVEL ft_lvl = pDev->GetFeatureLevel ();

  RtlSecureZeroMemory (sb, sizeof D3DX11_STATE_BLOCK);

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
    dc->SOGetTargets         (4, sb->SOBuffers);
  }

  dc->GetPredication         (&sb->Predication, &sb->PredicationValue);
}

void ApplyStateblock (ID3D11DeviceContext* dc, D3DX11_STATE_BLOCK* sb)
{
  SK_ComPtr <ID3D11Device> pDev;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  ///if (rb.api != SK_RenderAPI::D3D11On12)
    dc->GetDevice (&pDev.p);
  ///else
  ///  rb.d3d11.wrapper_dev->QueryInterface <ID3D11Device> (&pDev.p);

  if ( pDev == nullptr )
    return;

  const D3D_FEATURE_LEVEL ft_lvl = pDev->GetFeatureLevel ();

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

    UINT minus_one [D3D11_PS_CS_UAV_REGISTER_COUNT] =
    { std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max (),
      std::numeric_limits <UINT>::max (), std::numeric_limits <UINT>::max () };

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
    dc->IASetVertexBuffers   (0, IAVertexBufferCount, sb->IAVertexBuffers,
                                                      sb->IAVertexBuffersStrides,
                                                      sb->IAVertexBuffersOffsets);

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


  dc->OMSetBlendState        (sb->OMBlendState,        sb->OMBlendFactor,
                                                       sb->OMSampleMask);
  dc->OMSetDepthStencilState (sb->OMDepthStencilState, sb->OMDepthStencilRef);

  if (sb->OMBlendState)        sb->OMBlendState->Release        ();
  if (sb->OMDepthStencilState) sb->OMDepthStencilState->Release ();

  UINT OMRenderTargetCount =
    calc_count (sb->OMRenderTargets, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

  if (OMRenderTargetCount)
  {
    dc->OMSetRenderTargets   (OMRenderTargetCount, sb->OMRenderTargets,
                                                   sb->OMRenderTargetStencilView);

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
    UINT SOBufferCount =
      calc_count (sb->SOBuffers, 4);

    if (SOBufferCount)
    {
      UINT SOBuffersOffsets [4] = {   };

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

// The struct is implemented here, so that's where we allocate it.
SK_D3D11_Stateblock_Lite*
SK_D3D11_AllocStateBlock (size_t& size) noexcept
{
  size =
    sizeof (                SK_D3D11_Stateblock_Lite );
  return new (std::nothrow) SK_D3D11_Stateblock_Lite { };
}

void
SK_D3D11_FreeStateBlock (SK_D3D11_Stateblock_Lite* sb) noexcept
{
  assert (sb != nullptr);

  delete sb;
}

void
SK_D3D11_PostPresent (ID3D11Device* pDev, IDXGISwapChain* pSwap, HRESULT hr)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (SUCCEEDED (hr))
  {
    bool __WantGSyncUpdate =
      ( (config.fps.show && config.osd.show ) || SK_ImGui_Visible )
                         && ReadAcquire (&__SK_NVAPI_UpdateGSync);

    if (__WantGSyncUpdate)
    {
      UINT currentBuffer = 0;

      if ( SUCCEEDED ( pSwap->GetBuffer (
                         currentBuffer,
                           IID_PPV_ARGS (&rb.surface.dxgi.p)
                                        )
                     )
         )
      {
        if ( NVAPI_OK ==
               NvAPI_D3D_GetObjectHandleForResource (
                 pDev, rb.surface.dxgi, &rb.surface.nvapi
               )
           )
        {
          rb.gsync_state.update ();
          InterlockedExchange (&__SK_NVAPI_UpdateGSync, FALSE);
        }

        else rb.surface.dxgi = nullptr;
      }
    }

    SK_Screenshot_ProcessQueue  (SK_ScreenshotStage::_FlushQueue);
    SK_D3D11_TexCacheCheckpoint (                               );
  }
}

//#define _SK_D3D11_LITE_STATEBLOCKS

void
SK_CEGUI_DrawD3D11 (IDXGISwapChain* This)
{
  if (ReadAcquire (&__SK_DLL_Ending) != 0)
    return;

  static auto & rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <ID3D11Device> pDev (rb.device);

  InterlockedIncrement (&__osd_frames_drawn);

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

#define _SetupThreadContext()                                                   \
                             pTLS->imgui->drawing                      = FALSE; \
                             pTLS->texture_management.injection_thread = FALSE; \
  SK_ScopedBool auto_bool0 (&pTLS->imgui->drawing);                             \
  SK_ScopedBool auto_bool1 (&pTLS->texture_management.injection_thread);        \
                             pTLS->imgui->drawing                      = TRUE;  \
                             pTLS->texture_management.injection_thread = TRUE;  \

  _SetupThreadContext ();

  extern std::pair <BOOL*, BOOL>
  SK_ImGui_FlagDrawing_OnD3D11Ctx (UINT dev_idx);

  auto flag_result =
    SK_ImGui_FlagDrawing_OnD3D11Ctx (
      SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx)
    );

  SK_ScopedBool auto_bool2 (flag_result.first);
                           *flag_result.first = flag_result.second;

  if (InterlockedCompareExchange (&__gui_reset_dxgi, FALSE, TRUE))
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
    DXGI_SWAP_CHAIN_DESC desc = { };
    This->GetDesc      (&desc);

    SK_InstallWindowHook (desc.OutputWindow);

    if ((! config.cegui.enable) || SK_GetModuleHandle (L"CEGUIDirect3D11Renderer-0.dll"))
      ResetCEGUI_D3D11  (This);
  }


  else if ( pDev != nullptr )
  {
    assert (rb.device == pDev);

    // If the swapchain or device changed, bail-out and wait until the next frame for
    //   things to normalize.
    if ((rb.device != pDev) || rb.swapchain == nullptr)
    {
      SK_LOG0 ( ( L" D3D11 Device or Primary SwapChain Changed => "
                  L"Releasing resources..." ),
                  L"   DXGI   " );


      if ((! config.cegui.enable) || SK_GetModuleHandle (L"CEGUIDirect3D11Renderer-0.dll"))
        ResetCEGUI_D3D11      (This);
      SK_DXGI_UpdateSwapChain (This);

      return;
    }


    HRESULT hr;

    SK_ComPtr <ID3D11Texture2D>        pBackBuffer       = nullptr;
    SK_ComPtr <ID3D11RenderTargetView> pRenderTargetView = nullptr;
    SK_ComPtr <ID3D11BlendState>       pBlendState       = nullptr;

    SK_ComPtr <ID3D11Device1>          pDevice1          = nullptr;
    SK_ComPtr <ID3DDeviceContextState> pCtxState         = nullptr;
    SK_ComPtr <ID3DDeviceContextState> pCtxStateOrig     = nullptr;

    SK_ComQIPtr <IDXGISwapChain3> pSwap3 (This);

                      UINT currentBuffer = 0;
    if (pSwap3 != nullptr) currentBuffer = pSwap3->GetCurrentBackBufferIndex ();

    hr =
      This->GetBuffer (currentBuffer, IID_PPV_ARGS (&pBackBuffer.p));

    D3D11_TEXTURE2D_DESC          tex2d_desc = { };
    D3D11_RENDER_TARGET_VIEW_DESC rtdesc     = { };

    if (SUCCEEDED (hr))
    {
      pBackBuffer->GetDesc (&tex2d_desc);
    }

    SK_ComQIPtr <ID3D11DeviceContext>  pImmediateContext  (rb.d3d11.immediate_ctx);

    rb.framebuffer_flags &= (~SK_FRAMEBUFFER_FLAG_FLOAT);
    rb.framebuffer_flags &= (~SK_FRAMEBUFFER_FLAG_RGB10A2);
    rb.framebuffer_flags &= (~SK_FRAMEBUFFER_FLAG_SRGB);
    rb.framebuffer_flags &= (~SK_FRAMEBUFFER_FLAG_HDR);

    rtdesc.ViewDimension = tex2d_desc.SampleDesc.Count > 1 ?
                             D3D11_RTV_DIMENSION_TEXTURE2DMS :
                             D3D11_RTV_DIMENSION_TEXTURE2D;

    // sRGB Correction for UIs
    switch (tex2d_desc.Format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      {
        rtdesc.Format = tex2d_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ?
                                               DXGI_FORMAT_R8G8B8A8_UNORM :
                                               DXGI_FORMAT_B8G8R8A8_UNORM;

        if ( FAILED ( pDev->CreateRenderTargetView (
                        pBackBuffer,
                          &rtdesc, &pRenderTargetView.p )
                    )
           )
        {
          hr =
            pDev->CreateRenderTargetView (
              pBackBuffer,
                nullptr, &pRenderTargetView.p );
        }

        else { hr = S_OK; }

        rb.framebuffer_flags |= SK_FRAMEBUFFER_FLAG_SRGB;
      } break;

      case DXGI_FORMAT_R16G16B16A16_FLOAT:
        rb.framebuffer_flags |= SK_FRAMEBUFFER_FLAG_FLOAT;

        if (rb.isHDRCapable () && rb.scanout.dxgi_colorspace != DXGI_COLOR_SPACE_CUSTOM)
        {
          rb.framebuffer_flags |= SK_FRAMEBUFFER_FLAG_HDR;
        }

        rtdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

        if ( FAILED ( pDev->CreateRenderTargetView (
                        pBackBuffer,
                          &rtdesc, &pRenderTargetView.p )
                    )
           )
        {
          hr =
            pDev->CreateRenderTargetView (
              pBackBuffer,
                nullptr, &pRenderTargetView.p );
        }

        else { hr = S_OK; }
        break;

      case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      case DXGI_FORMAT_R10G10B10A2_UNORM:
        if (rb.isHDRCapable ())
        {
          rb.framebuffer_flags |= SK_FRAMEBUFFER_FLAG_HDR;
        }

        rb.framebuffer_flags |=  SK_FRAMEBUFFER_FLAG_RGB10A2;
        // Deliberately fall-through to default

      default:
      {
        rtdesc.Format = tex2d_desc.Format;

        if ( FAILED ( pDev->CreateRenderTargetView (
                        pBackBuffer,
                          &rtdesc, &pRenderTargetView.p )
                    )
           )
        {
          hr =
            pDev->CreateRenderTargetView (
              pBackBuffer,
                nullptr, &pRenderTargetView.p );
        }

        else { hr = S_OK; }
      } break;
    }

    DXGI_SWAP_CHAIN_DESC swapDesc = { };
         This->GetDesc (&swapDesc);

    if ( tex2d_desc.SampleDesc.Count > 1 ||
         swapDesc.SampleDesc.Count   > 1 )
    {
      rb.framebuffer_flags |= SK_FRAMEBUFFER_FLAG_MSAA;
    }

    else
    {
      rb.framebuffer_flags &= ~SK_FRAMEBUFFER_FLAG_MSAA;
    }


    // Test for colorspaces that are NOT HDR and then unflag HDR if necessary
    if (! rb.scanout.nvapi_hdr.active)
    {
      if ( (rb.fullscreen_exclusive    &&
            rb.scanout.dxgi_colorspace == DXGI_COLOR_SPACE_CUSTOM) ||
            rb.scanout.dwm_colorspace  == DXGI_COLOR_SPACE_CUSTOM )
      {
        rb.framebuffer_flags &= ~SK_FRAMEBUFFER_FLAG_HDR;
      }
    }


    //if (SUCCEEDED (hr))
    {
#ifdef _SK_D3D11_LITE_STATEBLOCKS
      //// Uses TLS to reduce dynamic memory pressure as much as possible
      SK_D3D11_Stateblock_Lite sb_ = { };
                   auto* sb = &sb_;
                   //pTLS->d3d11->getStateBlock ();
                   //RtlSecureZeroMemory ( sb,
                   //                  sizeof (SK_D3D11_Stateblock_Lite) );
#else
      D3DX11_STATE_BLOCK sblock = { };
      auto* sb = &sblock;
#endif

#ifdef _SK_D3D11_LITE_STATEBLOCKS
      sb->capture (pImmediateContext);
#else
      CreateStateblock (pImmediateContext, sb);
#endif

      pImmediateContext->OMSetRenderTargets (1, &pRenderTargetView.p, nullptr);

      D3D11_VIEWPORT   vp    = { };
      D3D11_BLEND_DESC blend = { };

      blend.RenderTarget [0].BlendEnable           = TRUE;
      blend.RenderTarget [0].SrcBlend              = D3D11_BLEND_ONE;
      blend.RenderTarget [0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
      blend.RenderTarget [0].BlendOp               = D3D11_BLEND_OP_ADD;
      blend.RenderTarget [0].SrcBlendAlpha         = D3D11_BLEND_ONE;
      blend.RenderTarget [0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
      blend.RenderTarget [0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
      blend.RenderTarget [0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

      if (SUCCEEDED (pDev->CreateBlendState (&blend, &pBlendState)))
         pImmediateContext->OMSetBlendState (         pBlendState, nullptr, 0xffffffff);

      vp.Width    = static_cast <float> (tex2d_desc.Width);
      vp.Height   = static_cast <float> (tex2d_desc.Height);
      vp.MinDepth = 0.0f;
      vp.MaxDepth = 1.0f;
      vp.TopLeftX = 0.0f;
      vp.TopLeftY = 0.0f;

      auto DrawSteamPopups = [&](void) ->
      void
      {
        if (SK_Steam_DrawOSD ())
        {
          if ((uintptr_t)cegD3D11 > 1)
          {
                  cegD3D11->beginRendering ();
            CEGUI::System::getDllSingleton ().renderAllGUIContexts ();
                  cegD3D11->endRendering   ();
          }
        }
      };

      pImmediateContext->RSSetViewports (1, &vp);
      {
        bool hudless  =
          SK_Screenshot_IsCapturingHUDless (),

        hdr_mode =
         ( rb.isHDRCapable () &&
          (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR) );

        if ((uintptr_t)cegD3D11 > 1)
        {
          if (hdr_mode || (! hudless))
          {
                      cegD3D11->beginRendering ();
            SK_TextOverlayManager::getInstance ()->drawAllOverlays     (0.0f, 0.0f);
                CEGUI::System::getDllSingleton ().renderAllGUIContexts ();
                        cegD3D11->endRendering ();
          }
        }

        if (hdr_mode)
        {
          //if (! hudless)
          //{
            DrawSteamPopups ();

            // Last-ditch effort to get the HDR post-process done before the UI.
            void SK_HDR_SnapshotSwapchain (void);
                 SK_HDR_SnapshotSwapchain (    );
          //}
        }

        // XXX: TODO (Full startup isn't necessary, just update framebuffer dimensions).
        if (ImGui_DX11Startup             ( This                         ))
        {
          extern DWORD SK_ImGui_DrawFrame ( DWORD dwFlags, void* user    );
                       SK_ImGui_DrawFrame (       0x00,          nullptr );
        }

        if ((! hdr_mode))// && hudless)
        {
          DrawSteamPopups ();
        }
      }

      // Queue-up Post-SK OSD Screenshots
      SK_Screenshot_ProcessQueue (SK_ScreenshotStage::EndOfFrame);

#ifdef _SK_D3D11_LITE_STATEBLOCKS
      sb->apply (pImmediateContext);
#else
      ApplyStateblock (pImmediateContext, sb);
#endif

      //
      // Update G-Sync; doing this here prevents trying to do this on frames where
      //   the swapchain was resized, which would deadlock the software.
      //
      if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status &&
               ((config.fps.show && config.osd.show)
                                 || SK_ImGui_Visible))
      {
        static DWORD dwLastUpdate = 0;
               DWORD dwNow        = timeGetTime ();

        if (dwLastUpdate < dwNow - 2000UL)
        {   dwLastUpdate = dwNow;
          InterlockedExchange (&__SK_NVAPI_UpdateGSync, TRUE);
        }
      }
    }
  }
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

    dll_log->Log ( L"[Window Mgr] Border Compensated Resolution ==> (%lu x %lu)",
                    x, y );
  }
#endif
}

volatile LONG
__SK_SHENMUE_EarlyPresent = 0;

HRESULT
STDMETHODCALLTYPE
SK_DXGI_Present ( IDXGISwapChain *This,
                  UINT            SyncInterval,
                  UINT            Flags )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  //SK_ReleaseAssert (Flags <= (2 * DXGI_PRESENT_ALLOW_TEARING));

  HRESULT hr = S_OK;

  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_BasicStructuredExceptionTranslator
  );
  try                                {
    hr =
      Present_Original (This, SyncInterval, Flags);
  }

  // Release all resources, claim the presentation was successful and
  //   hope the game recovers. AWFUL hack, but needed for some overlays.
  //
  catch (const SK_SEH_IgnoredException&)
  {
    if (rb.api != SK_RenderAPI::D3D11On12)
    {
      SK_D3D11_EndFrame        ();
      SK_D3D11_ResetTexCache   ();
      SK_CEGUI_QueueResetD3D11 (); // Prior to the next present, reset the UI
      hr = S_OK;
    }
  }
  SK_SEH_RemoveTranslator (orig_se);

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

  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (
      EXCEPTION_ACCESS_VIOLATION
    )
  );
  try
  {
    hr =
      Present1_Original (This, SyncInterval, Flags, pPresentParameters);
  }

  // Release all resources, claim the presentation was successful and
  //   hope the game recovers. AWFUL hack, but needed for some overlays.
  //
  catch (const SK_SEH_IgnoredException&)
  {
    SK_D3D11_EndFrame        ();
    SK_D3D11_ResetTexCache   ();
    SK_CEGUI_QueueResetD3D11 (); // Prior to the next present, reset the UI
    hr = S_OK;
  }
  SK_SEH_RemoveTranslator (orig_se);

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
      ( dwFlags & DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT ) ||
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
SK_DXGI_TestPresentFlags (DWORD Flags) noexcept
{
  if (Flags & DXGI_PRESENT_TEST)
  {
    SK_LOG_ONCE ( L"[   DXGI   ] Skipping SwapChain Present due to "
                  L"SwapChain Present Flag (DXGI_PRESENT_TEST)" );

    return false;
  }

  return true;
}

BOOL
SK_DXGI_IsFlipModelSwapEffect (DXGI_SWAP_EFFECT swapEffect)
{
  return
    ( swapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
      swapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL );
}

BOOL
SK_DXGI_IsFlipModelSwapChain (DXGI_SWAP_CHAIN_DESC& desc)
{
  return
    SK_DXGI_IsFlipModelSwapEffect (desc.SwapEffect);
};


void
SK_DXGI_SetupPluginOnFirstFrame ( IDXGISwapChain *This,
                                  UINT            SyncInterval,
                                  UINT            Flags )
{
  UNREFERENCED_PARAMETER (Flags);
  UNREFERENCED_PARAMETER (This);
  UNREFERENCED_PARAMETER (SyncInterval);

#ifdef _WIN64
  static auto game_id =
    SK_GetCurrentGameID ();

  switch (game_id)
  {
    case SK_GAME_ID::DarkSouls3:
      SK_DS3_PresentFirstFrame (This, SyncInterval, Flags);
      break;

    case SK_GAME_ID::WorldOfFinalFantasy:
    {
      SK_DeferCommand ("Window.Borderless toggle");
      SK_Sleep        (33);
      SK_DeferCommand ("Window.Borderless toggle");
    } break;
  }
#endif

  for ( auto first_frame_fn : plugin_mgr->first_frame_fns )
  {
    first_frame_fn (
      This, SyncInterval, Flags
    );
  }
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

    return
      This->Present1 (SyncInterval, Flags, pPresentParameters);
  };

  //
  // Early-out for games that use testing to minimize blocking
  //
  if (! SK_DXGI_TestPresentFlags (Flags))
  {
    SK_D3D11_EndFrame ();

    // Third-party software doesn't always behave compliantly in games that
    //   use presentation testing... so we may need to resort to this or
    //     the game's performance derails itself.
    if ( (Flags & DXGI_PRESENT_TEST) && config.render.dxgi.present_test_skip )
      return S_OK;

    return
      Present1 ( SyncInterval,
                   Flags,
                     pPresentParameters );
  }

  DXGI_SWAP_CHAIN_DESC desc = { };
       This->GetDesc (&desc);

  if (! SK_DXGI_TestSwapChainCreationFlags (desc.Flags))
  {
    SK_D3D11_EndFrame ();

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

    // Queue-up Pre-SK OSD Screenshots
    SK_Screenshot_ProcessQueue (SK_ScreenshotStage::BeforeOSD);

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    if (rb.api != SK_RenderAPI::D3D11On12)
    {
      // Establish the API used this frame (and handle possible translation layers)
      //
      switch (SK_GetDLLRole ())
      {
        case DLL_ROLE::D3D8:
          rb.api = SK_RenderAPI::D3D8On11;
          break;
        case DLL_ROLE::DDraw:
          rb.api = SK_RenderAPI::DDrawOn11;
          break;
        default:
          rb.api = SK_RenderAPI::D3D11;
          break;
        case DLL_ROLE::D3D12:
          rb.api = SK_RenderAPI::D3D12;
          break;
      }
    }


    SK_BeginBufferSwap ();

    HRESULT hr = E_FAIL;


    SK_ComPtr        <ID3D11Device> pDev;
    This->GetDevice (IID_PPV_ARGS (&pDev.p));

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
          const wchar_t* wszSection = (
            StrStrIW (it->target.module_path, LR"(\sys)") ) ?
                                              L"DXGI.Hooks" : nullptr;

          if (! wszSection)
          {
            SK_LOG0 ( ( L"Hook for '%hs' resides in '%s', will not cache!",
                          it->target.symbol_name,
           SK_ConcealUserDir (
                std::wstring (
                          it->target.module_path
                             ).data ()
              ) ),
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
          if (( *it_local )->hits && (
    StrStrIW (( *it_local )->target.module_path, LR"(\sys)") ) &&
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
      {
        SK_GetDLLConfig ()->write (
        SK_GetDLLConfig ()->get_filename ()
                                  );
      }

      SK_DXGI_SetupPluginOnFirstFrame (
        This, SyncInterval, Flags
      );

      // TODO: Clean this code up
      SK_ComQIPtr <IDXGIDevice>  pDevDXGI (pDev);
      SK_ComPtr   <IDXGIAdapter> pAdapter;
      SK_ComPtr   <IDXGIFactory> pFactory;

      if (            pDevDXGI != nullptr                               &&
           SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter  )) &&
           SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory.p))) )
      {
        rb.device    = pDev;
        rb.swapchain = This; // ?

        if (config.render.dxgi.safe_fullscreen)
          pFactory->MakeWindowAssociation ( nullptr, 0 );

        if (bAlwaysAllowFullscreen)
        {
          pFactory->MakeWindowAssociation (
            desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES
          );
        }
      }

      SK_DXGI_HookSwapChain (This);

      first_frame = false;
    }

    int interval =
         config.render.framerate.present_interval;

    int flags    = Flags;

    // Application preference
    if (interval == -1)
        interval = SyncInterval;

    rb.present_interval = interval;

    if ( SK_DXGI_IsFlipModelSwapChain (desc) && SK::Framerate::GetLimiter ()->get_limit () > 0.0L )
    {
      SK_ComQIPtr <IDXGISwapChain1> pSwap1 (This);

      static DXGI_FRAME_STATISTICS sequence_start = { };
      static auto                  queue_depth    = desc.BufferCount + 1;
      static std::vector <UINT>    frame_ids (queue_depth);

      SK_RunOnce (pSwap1->GetFrameStatistics (&sequence_start));

      bool bLimited =
        SK::Framerate::GetLimiter ()->get_limit () > 0.0L,
           bGlitch  = false;

      DXGI_FRAME_STATISTICS        frame_stats = { };
      pSwap1->GetFrameStatistics (&frame_stats);

      UINT                          ulLastPresent;
      pSwap1->GetLastPresentCount (&ulLastPresent);

      ////// Depth changed
      ////if (queue_depth != desc.BufferCount + 1)
      ////{
      ////  queue_depth = desc.BufferCount + 1;
      ////  frame_ids.resize (queue_depth);
      ////
      ////  bGlitch = true;
      ////}
      ////
      ////else
      ////{
      ////  if (frame_ids [ulLastPresent % queue_depth] != frame_stats.PresentRefreshCount)
      ////  {
      ////    //dll_log->Log (L"Glitch, Expected: %lu, Got: %lu for frame %lu [queue_depth=%lu]",
      ////    //              frame_ids [ulLastPresent % queue_depth], frame_stats.PresentRefreshCount/*ulLastPresent*/, ulLastPresent, queue_depth );
      ////    bGlitch = true;
      ////  }
      ////}

      if (bGlitch || bLimited)
      {
        if (config.render.framerate.present_interval)
          flags |= DXGI_PRESENT_RESTART;
      }

      ////UINT idx =
      ////  ((ulLastPresent + 1) % queue_depth);
      ////
      ////if (bGlitch)
      ////{
      ////  frame_ids.clear ();
      ////
      ////  int sync_id =
      ////    frame_stats.SyncRefreshCount + 1;
      ////
      ////  for ( UINT i = idx ; i < queue_depth; ++i )
      ////    frame_ids [i] = sync_id++;
      ////  for ( UINT j = 0   ; j < idx        ; ++j )
      ////    frame_ids [j] = sync_id++;
      ////}
      ////else
      ////  frame_ids [idx] = frame_stats.SyncRefreshCount + 1;
    }

    // Test first, then do (if test_present is true)
    hr =
      config.render.dxgi.test_present ?
        Present1 (0, Flags | DXGI_PRESENT_TEST, pPresentParameters) :
          S_OK;

    const bool can_present =
      ( SUCCEEDED (hr) && hr != DXGI_STATUS_OCCLUDED );

    if (can_present)
    {
      SK_CEGUI_DrawD3D11 (This);

      hr =
        Present1 (interval, flags, pPresentParameters);
    }

    else
    {
      dll_log->Log ( L"[   DXGI   ] *** IDXGISwapChain1::Present1 (...) "
                     L"returned non-S_OK (%s :: %s)",
                       SK_DescribeHRESULT (hr),
                         SUCCEEDED (hr) ? L"Success" :
                                          L"Fail" );

      if (hr == DXGI_ERROR_DEVICE_REMOVED)
      {
        if (pDev != nullptr)
        {
          // D3D11 Device Removed, let's find out why...
          HRESULT hr_removed =
            pDev->GetDeviceRemovedReason ();

          dll_log->Log ( L"[   DXGI   ] (*) >> Reason For Removal: %s",
                          SK_DescribeHRESULT (hr_removed) );
        }
      }
    }

    if ( pDev != nullptr || rb.api == SK_RenderAPI::D3D12 )
    {
      HRESULT ret =
        SK_EndBufferSwap (hr, pDev);
      SK_D3D11_PostPresent   (pDev, This, hr);

      if (bWait)
      {
        SK_ComQIPtr <IDXGISwapChain2>
            pSwapChain2 (This);
        if (pSwapChain2 != nullptr)
        {
          SK_AutoHandle hWait (
            pSwapChain2->GetFrameLatencyWaitableObject ()
          );

          MsgWaitForMultipleObjectsEx ( 1,
                                          &hWait.m_h,
                                            config.render.framerate.swapchain_wait,
                                              QS_ALLINPUT, MWMO_INPUTAVAILABLE );
        }
      }

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
  SK_LOG_ONCE (L"Present1"); // Almost never used by anything, so log it if it happens.

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
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto Present = [&](UINT                    SyncInterval,
                     UINT                    Flags) ->
  HRESULT
  {
    if (Source == SK_DXGI_PresentSource::Hook)
    {
      return
        DXGISwapChain_Present ( This,
                                  SyncInterval,
                                    Flags );
    }

    return
      This->Present (SyncInterval, Flags);
  };


  //
  // Early-out for games that use testing to minimize blocking
  //
  if (! SK_DXGI_TestPresentFlags (Flags))
  {
    SK_D3D11_EndFrame ();

    // Third-party software doesn't always behave compliantly in games that
    //   use presentation testing... so we may need to resort to this or
    //     the game's performance derails itself.
    if ( (Flags & DXGI_PRESENT_TEST) && config.render.dxgi.present_test_skip )
     return S_OK;

   return
     Present (SyncInterval, Flags);
  }

  DXGI_SWAP_CHAIN_DESC desc = { };
       This->GetDesc (&desc);

  if (! SK_DXGI_TestSwapChainCreationFlags (desc.Flags))
  {
    SK_D3D11_EndFrame ();

    return
      Present (SyncInterval, Flags);
  }

  bool process                = false;
  bool has_wrapped_swapchains =
    ReadAcquire (&SK_DXGI_LiveWrappedSwapChain1s) ||
    ReadAcquire (&SK_DXGI_LiveWrappedSwapChains);
  // ^^^ It's not required that IDXGISwapChain1 or higher invokes Present1!

  if (config.render.osd.draw_in_vidcap && has_wrapped_swapchains)
  {
    process =
      (Source == SK_DXGI_PresentSource::Wrapper);
  }

  else
  {
    process =
      ( Source == SK_DXGI_PresentSource::Hook ||
        (! has_wrapped_swapchains)               );
  }


  if (process || SK_GetFramesDrawn () < 2)
  {
    // Start / End / Readback Pipeline Stats
    SK_D3D11_UpdateRenderStats (This);

    // Queue-up Pre-SK OSD Screenshots
    SK_Screenshot_ProcessQueue (SK_ScreenshotStage::BeforeOSD);

    HRESULT hr = E_FAIL;

    SK_ComPtr <ID3D11Device>       pDev     = nullptr;
    SK_ComPtr <ID3D12CommandQueue> p12Queue = nullptr;

    if ( FAILED (This->GetDevice (IID_ID3D11Device, (void **)&pDev.p)) )
    {
      //// D3D11On12?
      This->GetDevice (__uuidof (ID3D12CommandQueue), (void **)&p12Queue.p);
    }

    if (pDev == nullptr && p12Queue != nullptr)
    {
      pDev = rb.device;
      //rb.api = SK_RenderAPI::D3D12;
    }

    else
    {
      if (rb.api != SK_RenderAPI::D3D12 &&
          rb.api != SK_RenderAPI::D3D11On12)
      {
        // Establish the API used this frame (and handle translation layers)
        //
        switch (SK_GetDLLRole ())
        {
          case DLL_ROLE::D3D8:
            rb.api = SK_RenderAPI::D3D8On11;
            break;
          case DLL_ROLE::DDraw:
            rb.api = SK_RenderAPI::DDrawOn11;
            break;
          default:
            rb.api = SK_RenderAPI::D3D11;
            break;
        }
      }
    }

    SK_BeginBufferSwap ();

    if (first_frame)
    {
      if (pDev != nullptr /*|| pDev12*/ || rb.api == SK_RenderAPI::D3D12)
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
             SK_ConcealUserDir (
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
            {
              SK_Hook_PushLocalCacheOntoGlobal ( **it_local,
                                                   **it_global );
            }

            else
            {
              ( *it_global )->target.addr = nullptr;
              ( *it_global )->hits        = 0;
              ( *it_global )->active      = false;
            }

            it_global++, it_local++;
          }
        }

        SK_D3D11_UpdateHookAddressCache ();

        if (hooked > 0)
        {
          SK_GetDLLConfig   ()->write (
            SK_GetDLLConfig ()->get_filename ()
          );
        }
      }

      SK_DXGI_SetupPluginOnFirstFrame (This, SyncInterval, Flags);

      SK_ComPtr                     <IDXGIDevice> pDevDXGI = nullptr;
      This->GetDevice (IID_IDXGIDevice, (void **)&pDevDXGI.p);

      if (pDev != nullptr)
      {
        // TODO: Clean this code up
        SK_ComPtr <IDXGIAdapter> pAdapter = nullptr;
        SK_ComPtr <IDXGIFactory> pFactory = nullptr;

        rb.swapchain = This;

        if ( pDevDXGI != nullptr                                          &&
             SUCCEEDED (pDevDXGI->GetAdapter               (&pAdapter.p)) &&
             SUCCEEDED (pAdapter->GetParent  (IID_PPV_ARGS (&pFactory.p))) )
        {
          rb.device    = pDev;
          rb.swapchain = This; // ?

          if (config.render.dxgi.safe_fullscreen)
          {
            pFactory->MakeWindowAssociation ( nullptr, 0 );
          }

          if (bAlwaysAllowFullscreen)
          {
            pFactory->MakeWindowAssociation (
              desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES
            );
          }
        }
      }

      first_frame = false;
    }

    int interval =
      config.render.framerate.present_interval;

    // Fix flags for compliance in broken games
    //
    if (SK_DXGI_IsFlipModelSwapChain (desc))
    {
      if (! desc.Windowed)
          Flags &= ~DXGI_PRESENT_ALLOW_TEARING;
      else if (SyncInterval == 0)
          Flags |=  DXGI_PRESENT_ALLOW_TEARING;

      if (Flags & DXGI_PRESENT_ALLOW_TEARING)
      {
        // User wants no override, present flag implies interval = 0
        if (interval == -1)
        {
          SyncInterval = 0;
          interval     = 0;
        }

        else if (interval != 0)
        {
          // Turn this off because the user wants an override
          Flags &= ~DXGI_PRESENT_ALLOW_TEARING;
        }
      }
    }

    else
      Flags &= ~DXGI_PRESENT_ALLOW_TEARING;


    int flags    = Flags;

    // Application preference
    if (interval == -1)
        interval = SyncInterval;

    rb.present_interval = interval;

    if ( SK_DXGI_IsFlipModelSwapChain (desc) && SK::Framerate::GetLimiter ()->get_limit () > 0.0L )
    {
      bool bLimited =
        SK::Framerate::GetLimiter ()->get_limit () > 0.0;

      if (bLimited)
      {
        if (config.render.framerate.present_interval)
          flags |= DXGI_PRESENT_RESTART;
      }
    }

    SK_CEGUI_DrawD3D11 (This);

    if (bWait) flags |= DXGI_PRESENT_DO_NOT_WAIT;

    hr =
      Present (interval, flags);

    if ( pDev != nullptr || rb.api == SK_RenderAPI::D3D12 )
    {
      bool bGoAgain =
        (hr == DXGI_ERROR_WAS_STILL_DRAWING);

      HRESULT ret;

      auto _EndSwap = [&](void)
      {
        ret =
          SK_EndBufferSwap (hr, pDev);

        if (SUCCEEDED (hr) && rb.api != SK_RenderAPI::D3D12)
          SK_D3D11_PostPresent (pDev, This, hr);
      };

      if (! bGoAgain)
        _EndSwap ();

      if (bWait)
      {
        SK_ComQIPtr <IDXGISwapChain2>
            pSwapChain2 (This);
        if (pSwapChain2 != nullptr)
        {
          pSwapChain2->SetMaximumFrameLatency (
            std::max (1, config.render.framerate.pre_render_limit)
          );

          // Don't bother closing this handle, it would cause problems
          //   with DXVK.
          CHandle hWait (
            pSwapChain2->GetFrameLatencyWaitableObject ()
          );

          DWORD dwDontCare = 0x0;

          if (GetHandleInformation ( hWait.m_h, &dwDontCare ))
          {
            SK_WaitForSingleObject ( hWait,
              config.render.framerate.swapchain_wait );
          }
          else
            hWait.m_h = 0;
        }
      }

      if (bGoAgain)
      {
        flags &= ~DXGI_PRESENT_DO_NOT_WAIT;

        Present (interval, flags);

        _EndSwap ();
      }

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
    SK_DXGI_DispatchPresent (
      This, SyncInterval, Flags,
        SK_DXGI_Present, SK_DXGI_PresentSource::Hook
    );
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
  struct callframe_s {
    DXGI_FORMAT ef;
    UINT         f;
  } frame = { EnumFormat, Flags };

  uint32_t tag =
    crc32c (0x0, &frame, sizeof (callframe_s));

  static concurrency::concurrent_unordered_map <
           uint32_t, std::vector <DXGI_MODE_DESC>
         > cached_descs;


  static const auto game_id =
    SK_GetCurrentGameID ();

#ifdef _M_AMD64
  extern bool SK_NNK2_CacheModes;

  if (game_id == SK_GAME_ID::NiNoKuni2 && SK_NNK2_CacheModes)
  {
    if (! cached_descs [tag].empty ())
    {
      int modes = 0;

      if (*pNumModes == 0)
      {
        *pNumModes = (UINT)cached_descs [tag].size ();
             modes = *pNumModes;
      }

      else
      {
        modes =
          (int)std::min (cached_descs [tag].size (), (size_t)*pNumModes);
      }


      if (pDesc)
      {
        for (int i = 0; i < modes; i++)
        {
          pDesc [i] = cached_descs [tag][i];
        }
      }

      return S_OK;
    }
  }
#endif


  if (pDesc != nullptr)
  {
    DXGI_LOG_CALL_I5 ( L"       IDXGIOutput", L"GetDisplayModeList       ",
                         L"  %08" PRIxPTR L"h, %i, %02x, NumModes=%lu, %08"
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

  DXGI_MODE_DESC*
    pDescLocal = nullptr;

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

      std::set <int> min_cover;
      std::set <int> max_cover;

      if (! config.render.dxgi.res.min.isZero ())
      {
        // Restrict MIN (Sequential Scan: Last->First)
        for ( int i = *pNumModes - 1 ; i >= 0 ; --i )
        {
          if (SK_DXGI_RestrictResMin (WIDTH,  first, i, min_cover, pDesc) ||
              SK_DXGI_RestrictResMin (HEIGHT, first, i, min_cover, pDesc))
          {
            ++removed_count;
          }
        }
      }

      if (! config.render.dxgi.res.max.isZero ())
      {
        // Restrict MAX (Sequential Scan: First->Last)
        for ( UINT i = 0 ; i < *pNumModes ; ++i )
        {
          if (SK_DXGI_RestrictResMax (WIDTH,  last, i, max_cover, pDesc) ||
              SK_DXGI_RestrictResMax (HEIGHT, last, i, max_cover, pDesc))
          {
            ++removed_count;
          }
        }
      }
    }

    if (pDesc != nullptr)
    {
      if (config.render.dxgi.scaling_mode != -1)
      {
        if ( config.render.dxgi.scaling_mode != DXGI_MODE_SCALING_UNSPECIFIED &&
             config.render.dxgi.scaling_mode != DXGI_MODE_SCALING_CENTERED )
        {
          for ( INT i  = gsl::narrow_cast <INT> (*pNumModes) - 1 ;
                    i >= 0                                       ;
                  --i )
          {
            if ( pDesc [i].Scaling != DXGI_MODE_SCALING_UNSPECIFIED &&
                 pDesc [i].Scaling != DXGI_MODE_SCALING_CENTERED    &&
                 pDesc [i].Scaling != config.render.dxgi.scaling_mode )
            {
              pDesc    [i] =
                pDesc  [i + 1];

              ++removed_count;
            }
          }
        }
      }

      if (pDescLocal == nullptr)
      {
        dll_log->Log ( L"[   DXGI   ]      >> %lu modes (%li removed)",
                         *pNumModes,
                           removed_count );

        cached_descs [tag].resize (*pNumModes);

        for (UINT i = 0; i < *pNumModes; i++)
        {
          cached_descs [tag][i] = pDesc [i];
        }
      }
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

HRESULT
STDMETHODCALLTYPE
SK_DXGI_FindClosestMode ( IDXGISwapChain *pSwapChain,
              _In_  const DXGI_MODE_DESC *pModeToMatch,
                   _Out_  DXGI_MODE_DESC *pClosestMatch,
                _In_opt_  IUnknown       *pConcernedDevice,
                          BOOL            bApplyOverrides )
{
  SK_ComPtr <IDXGIDevice> pSwapDevice;
  SK_ComPtr <IDXGIOutput> pSwapOutput;

  if ( SUCCEEDED (
         pSwapChain->GetContainingOutput (&pSwapOutput.p)
                 )
           &&
       SUCCEEDED (
         pSwapChain->GetDevice (
           __uuidof (IDXGIDevice), (void**)& pSwapDevice.p)
                 )
     )
  {
    DXGI_MODE_DESC mode_to_match = *pModeToMatch;

    if (bApplyOverrides)
    {
      if (config.render.framerate.rescan_.Denom != 1)
      {
              mode_to_match.RefreshRate.Numerator   =
        config.render.framerate.rescan_.Numerator;
              mode_to_match.RefreshRate.Denominator =
        config.render.framerate.rescan_.Denom;
      }

      else if (config.render.framerate.refresh_rate != -1 &&
                mode_to_match.RefreshRate.Numerator !=
         (UINT)config.render.framerate.refresh_rate)
      {
        mode_to_match.RefreshRate.Numerator   =
          (UINT)config.render.framerate.refresh_rate;
        mode_to_match.RefreshRate.Denominator = 1;
      }

      if ( config.render.dxgi.scaling_mode != -1                         &&
                mode_to_match.Scaling != config.render.dxgi.scaling_mode &&
                      (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode !=
                       DXGI_MODE_SCALING_CENTERED )
      {
        mode_to_match.Scaling =
          (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode;
      }
    }

    pModeToMatch = &mode_to_match;

    HRESULT hr =
      FindClosestMatchingMode_Original (
        pSwapOutput, pModeToMatch,
                    pClosestMatch, pConcernedDevice
      );

    // Overrides forced _AFTER_ asking DXGI for a match
    if (bApplyOverrides)
    {
      if (SK_GetCurrentGameID () == SK_GAME_ID::Sekiro)
        pClosestMatch->Format = DXGI_FORMAT_R10G10B10A2_UNORM;
    }

    return
      hr;
  }

  return DXGI_ERROR_DEVICE_REMOVED;
}

HRESULT
STDMETHODCALLTYPE
SK_DXGI_ResizeTarget ( IDXGISwapChain *This,
                  _In_ DXGI_MODE_DESC *pNewTargetParameters,
                       BOOL            bApplyOverrides )
{
  ///if (bApplyOverrides)
  ///{
  ///  // Can't do this if waitable
  ///  if ( dxgi_caps.present.waitable &&
  ///       config.render.framerate.swapchain_wait > 0 )
  ///  {
  ///    return S_OK;
  ///  }
  ///}
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.scanout.colorspace_override != DXGI_COLOR_SPACE_CUSTOM)
  {
    SK_ComQIPtr <IDXGISwapChain3> pSwap3 (This);

    if (pSwap3 != nullptr)
    {
      SK_DXGI_UpdateColorSpace (pSwap3);
    }
  }

  assert (pNewTargetParameters != nullptr);

  HRESULT ret =
    E_UNEXPECTED;

  DXGI_MODE_DESC new_new_params =
    *pNewTargetParameters;

  DXGI_MODE_DESC* pNewNewTargetParameters =
    &new_new_params;

  if (bApplyOverrides)
  {
    if ( config.window.borderless ||
         ( config.render.dxgi.scaling_mode != -1 &&
            pNewTargetParameters->Scaling  !=
              (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
                                  ||
         ( config.render.framerate.refresh_rate          != -1 &&
             pNewTargetParameters->RefreshRate.Numerator !=
               static_cast <UINT> (
           config.render.framerate.refresh_rate )
         )
      )
    {
      if ( config.render.framerate.rescan_.Denom          !=  1 ||
           (config.render.framerate.refresh_rate          != -1 &&
                     new_new_params.RefreshRate.Numerator != static_cast <UINT>
           (config.render.framerate.refresh_rate) )
         )
      {
        DXGI_MODE_DESC modeDesc  = { };
        DXGI_MODE_DESC modeMatch = { };

        modeDesc.Format           = new_new_params.Format;
        modeDesc.Width            = new_new_params.Width;
        modeDesc.Height           = new_new_params.Height;
        modeDesc.Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;
        modeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

        if (config.render.framerate.rescan_.Denom != 1)
        {
                     modeDesc.RefreshRate.Numerator   =
          config.render.framerate.rescan_.Numerator;
                     modeDesc.RefreshRate.Denominator =
          config.render.framerate.rescan_.Denom;
        }

        else
        {
          modeDesc.RefreshRate.Numerator   =
            static_cast <UINT> (config.render.framerate.refresh_rate);
          modeDesc.RefreshRate.Denominator = 1;
        }

        if ( SUCCEEDED (
               SK_DXGI_FindClosestMode ( This, &modeDesc,
                                               &modeMatch, FALSE )
                       )
           )
        {
          new_new_params.RefreshRate.Numerator   =
               modeMatch.RefreshRate.Numerator;
          new_new_params.RefreshRate.Denominator =
               modeMatch.RefreshRate.Denominator;
        }
      }

      if ( config.render.dxgi.scanline_order        != -1 &&
            pNewTargetParameters->ScanlineOrdering  !=
              (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order )
      {
        new_new_params.ScanlineOrdering =
          (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order;
      }

      if ( config.render.dxgi.scaling_mode != -1 &&
            pNewTargetParameters->Scaling  !=
              (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
      {
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
        SK_DXGI_BorderCompensation ( new_new_params.Width,
                                     new_new_params.Height );
      }

      // Clamp the buffer dimensions if the user has a min/max preference
      const UINT
        max_x = config.render.dxgi.res.max.x < new_new_params.Width  ?
                config.render.dxgi.res.max.x : new_new_params.Width,
        min_x = config.render.dxgi.res.min.x > new_new_params.Width  ?
                config.render.dxgi.res.min.x : new_new_params.Width,
        max_y = config.render.dxgi.res.max.y < new_new_params.Height ?
                config.render.dxgi.res.max.y : new_new_params.Height,
        min_y = config.render.dxgi.res.min.y > new_new_params.Height ?
                config.render.dxgi.res.min.y : new_new_params.Height;

      new_new_params.Width   =  std::max ( max_x , min_x );
      new_new_params.Height  =  std::max ( max_y , min_y );

      pNewNewTargetParameters->Format =
        SK_DXGI_PickHDRFormat (pNewNewTargetParameters->Format);
    }


    ret =
      ResizeTarget_Original (This, pNewNewTargetParameters);

    if (FAILED (ret))
    {
      SK_D3D11_EndFrame        ();
      SK_D3D11_ResetTexCache   ();
      SK_CEGUI_QueueResetD3D11 (); // Prior to next present, reset the UI

      ret =
        ResizeTarget_Original (This, pNewNewTargetParameters);
    }

    if (SUCCEEDED (ret))
    {
      rb.swapchain = This;

      if ( pNewNewTargetParameters->Width  != 0 &&
           pNewNewTargetParameters->Height != 0 )
      {
        SK_SetWindowResX (pNewNewTargetParameters->Width);
        SK_SetWindowResY (pNewNewTargetParameters->Height);
      }

      else
      {
        RECT client = { };

        GetClientRect (game_window.hWnd, &client);

        SK_SetWindowResX (client.right  - client.left);
        SK_SetWindowResY (client.bottom - client.top);
      }
    }
  }

  return ret;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGIOutput_FindClosestMatchingMode_Override (
                IDXGIOutput    *This,
 _In_     const DXGI_MODE_DESC *pModeToMatch,
 _Out_          DXGI_MODE_DESC *pClosestMatch,
 _In_opt_       IUnknown       *pConcernedDevice )
{
  DXGI_LOG_CALL_I4 (
    L"       IDXGIOutput", L"FindClosestMatchingMode         ",
    L"%p, %08" PRIxPTR L"h, %08" PRIxPTR L"h, %08"
                                 PRIxPTR L"h",
      This, (uintptr_t)pModeToMatch, (uintptr_t)pClosestMatch,
            (uintptr_t)pConcernedDevice
  );

  SK_LOG0 (
    ( L"[?]  Desired Mode:  %lux%lu@%.2f Hz, Format=%s, Scaling=%s, "
                                           L"Scanlines=%s",
        pModeToMatch->Width, pModeToMatch->Height,
          pModeToMatch->RefreshRate.Denominator != 0 ?
            static_cast <float> (pModeToMatch->RefreshRate.Numerator) /
            static_cast <float> (pModeToMatch->RefreshRate.Denominator) :
              std::numeric_limits <float>::quiet_NaN (),
        SK_DXGI_FormatToStr           (pModeToMatch->Format).c_str (),
        SK_DXGI_DescribeScalingMode   (pModeToMatch->Scaling),
        SK_DXGI_DescribeScanlineOrder (pModeToMatch->ScanlineOrdering) ),
      L"   DXGI   "
  );


  DXGI_MODE_DESC mode_to_match = *pModeToMatch;

  if (  config.render.framerate.rescan_.Denom !=  1 ||
       (config.render.framerate.refresh_rate  != -1 &&
         mode_to_match.RefreshRate.Numerator  !=
  (UINT)config.render.framerate.refresh_rate) )
  {
    dll_log->Log (
      L"[   DXGI   ]  >> Refresh Override "
      L"(Requested: %f, Using: %f)",
        mode_to_match.RefreshRate.Denominator != 0 ?
          static_cast <float> (mode_to_match.RefreshRate.Numerator) /
          static_cast <float> (mode_to_match.RefreshRate.Denominator) :
            std::numeric_limits <float>::quiet_NaN (),
          static_cast <float> (config.render.framerate.rescan_.Numerator) /
          static_cast <float> (config.render.framerate.rescan_.Denom) );

    if (config.render.framerate.rescan_.Denom != 1)
    {
            mode_to_match.RefreshRate.Numerator     =
      config.render.framerate.rescan_.Numerator;
              mode_to_match.RefreshRate.Denominator =
        config.render.framerate.rescan_.Denom;
    }

    else
    {
      mode_to_match.RefreshRate.Numerator =
        (UINT)config.render.framerate.refresh_rate;
      mode_to_match.RefreshRate.Denominator = 1;
    }
  }

  if ( config.render.dxgi.scaling_mode != -1 &&
       mode_to_match.Scaling           != config.render.dxgi.scaling_mode &&
                       (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode !=
                        DXGI_MODE_SCALING_CENTERED )
  {
    dll_log->Log ( L"[   DXGI   ]  >> Scaling Override "
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


  HRESULT     ret = E_UNEXPECTED;
  DXGI_CALL ( ret, FindClosestMatchingMode_Original (
                     This, pModeToMatch, pClosestMatch,
                                         pConcernedDevice ) );

  if (SK_GetCurrentGameID () == SK_GAME_ID::Sekiro)
  {
    pClosestMatch->Format = DXGI_FORMAT_R10G10B10A2_UNORM;
  }

  SK_LOG0 (
    ( L"[#]  Closest Match: %lux%lu@%.2f Hz, Format=%s, Scaling=%s, "
                                           L"Scanlines=%s",
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
DXGIOutput_WaitForVBlank_Override ( IDXGIOutput *This ) noexcept
{
  //DXGI_LOG_CALL_I0 (L"       IDXGIOutput", L"WaitForVBlank         ");

  return
    WaitForVBlank_Original (This);
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGISwap_GetFullscreenState_Override ( IDXGISwapChain  *This,
                            _Out_opt_  BOOL            *pFullscreen,
                            _Out_opt_  IDXGIOutput    **ppTarget )
{
  return
    GetFullscreenState_Original (This, pFullscreen, ppTarget);
}

// Classifies a swapchain as dummy (used by some libraries during init) or
//   real (potentially used to do actual rendering).
bool
SK_DXGI_IsSwapChainReal (const DXGI_SWAP_CHAIN_DESC& desc) noexcept
{
  // 0x0 is implicitly sized to match its HWND's client rect,
  //
  //   => Anything ( 1x1 - 31x31 ) has no practical application.
  //
  if (desc.BufferDesc.Height > 0 && desc.BufferDesc.Height < 32)
    return false;

  if (desc.BufferDesc.Width > 0  && desc.BufferDesc.Width  < 32)
    return false;

  wchar_t              wszClass [64] = { };
  RealGetWindowClassW (
    desc.OutputWindow, wszClass, 63);

  bool dummy_window =
    StrStrIW (wszClass, L"Special K Dummy Window Class") ||
    StrStrIW (wszClass, L"RTSSWndClass");

  return
    (! dummy_window);
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
  //if (Fullscreen)
  //{
  //  void SK_NvAPI_ReIssueLastHdrColorControl (void);
  //       SK_NvAPI_ReIssueLastHdrColorControl (    );
  //}

  DXGI_LOG_CALL_I2 ( L"    IDXGISwapChain", L"SetFullscreenState         ",
                     L"%s, %08" PRIxPTR L"h",
                      Fullscreen ? L"{ Fullscreen }" :
                                   L"{ Windowed }",   (uintptr_t)pTarget );

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.scanout.colorspace_override != DXGI_COLOR_SPACE_CUSTOM)
  {
    SK_ComQIPtr <IDXGISwapChain3> pSwap3 (This);

    if (pSwap3 != nullptr)
    {
      SK_DXGI_UpdateColorSpace (pSwap3);
    }
  }


  SK_COMPAT_FixUpFullscreen_DXGI (Fullscreen);


  // Dummy init swapchains (i.e. 1x1 pixel) should not have
  //   override parameters applied or it's usually fatal.
  bool no_override = (! SK_DXGI_IsSwapChainReal (This));


  // If auto-update prompt is visible, don't go fullscreen.
  if ( hWndUpdateDlg != static_cast <HWND> (INVALID_HANDLE_VALUE) ||
                               ReadAcquire (&__SK_TaskDialogActive) )
  {
    Fullscreen = FALSE;
    pTarget    = nullptr;
  }

  BOOL                       bFullscreen;
  SK_ComPtr <IDXGIOutput>                  pOutput;
  This->GetFullscreenState (&bFullscreen, &pOutput.p);

  if (! no_override)
  {
    ////if (config.render.framerate.flip_discard)
    ////{
    ////  dll_log->Log ( L"[ DXGI 1.2 ]  >> Flip Model Override: Fullscreen Mode Switch Ignored");
    ////
    ////  Fullscreen = FALSE;
    ////  pTarget    = nullptr;
    ////}

    if (config.display.force_fullscreen && Fullscreen == FALSE)
    {

      dll_log->Log ( L"[   DXGI   ]  >> Display Override "
                     L"(Requested: Windowed, Using: Fullscreen)" );
      Fullscreen = TRUE;
      pTarget    = pOutput.p;
    }
    else if ((__SK_HDR_16BitSwap || config.display.force_windowed || config.render.framerate.flip_discard) && Fullscreen != FALSE)
    {
      Fullscreen = FALSE;
      pTarget    = nullptr;
      dll_log->Log ( L"[   DXGI   ]  >> Display Override "
                     L"(Requested: Fullscreen, Using: Windowed)" );
    }

    else if (request_mode_change == mode_change_request_e::Fullscreen &&
                 Fullscreen == FALSE)
    {
      dll_log->Log ( L"[   DXGI   ]  >> Display Override "
               L"User Initiated Fulllscreen Switch" );
      Fullscreen = TRUE;
      pTarget    = pOutput.p;
    }
    else if (request_mode_change == mode_change_request_e::Windowed &&
                      Fullscreen != FALSE)
    {
      dll_log->Log ( L"[   DXGI   ]  >> Display Override "
               L"User Initiated Windowed Switch" );
      Fullscreen = FALSE;
      pTarget    = nullptr;
    }
  }

  HRESULT    ret;

  if (bFullscreen == Fullscreen)
    ret = S_OK;
  else
  {
    SK_CEGUI_QueueResetD3D11 (); // Prior to the next present, reset the UI

    DXGI_CALL (ret, SetFullscreenState_Original (This, Fullscreen, pTarget));

    if (pOutput.p != nullptr)
        pOutput    = nullptr;
  }

  //
  // Necessary provisions for Fullscreen Flip Mode
  //
  if (bFullscreen != Fullscreen && SUCCEEDED (ret))
  {
    DXGI_SWAP_CHAIN_DESC           desc = { };
    if (SUCCEEDED (This->GetDesc (&desc)))
    {
      auto _FillInWindowResolution = [&](void)->void
      {
        if (desc.BufferDesc.Width != 0)
        {
          SK_SetWindowResX (desc.BufferDesc.Width);
          SK_SetWindowResY (desc.BufferDesc.Height);
        }

        else
        {
          RECT                                  client = { };
          GetClientRect    (desc.OutputWindow, &client);

          SK_SetWindowResX (client.right  - client.left);
          SK_SetWindowResY (client.bottom - client.top);
        }
      };

      UINT _Flags     =
        SK_DXGI_FixUpLatencyWaitFlag (This, desc.Flags);

      if (SK_DXGI_IsFlipModelSwapChain (desc))
      {
        if (SUCCEEDED (This->ResizeBuffers ( 0, 0, 0,
                                             desc.BufferDesc.Format, _Flags ) )
           )
        {
          _FillInWindowResolution ();
        }
      }

      else
      {
        _FillInWindowResolution ();
      }
    }

    if (SUCCEEDED (ret))
      rb.fullscreen_exclusive = Fullscreen;
  }

  return ret;
}


static UINT  _uiDPI     =     96;
static float _fDPIScale = 100.0f;

static auto SK_DPI_Update = [&](void) ->
void
{
  using  GetDpiForSystem_pfn = UINT (WINAPI *)(void);
  static GetDpiForSystem_pfn
         GetDpiForSystem = (GetDpiForSystem_pfn)
    SK_GetProcAddress ( SK_GetModuleHandleW (L"user32"),
                                    "GetDpiForSystem" );

  using  GetDpiForWindow_pfn = UINT (WINAPI *)(HWND);
  static GetDpiForWindow_pfn
         GetDpiForWindow = (GetDpiForWindow_pfn)
    SK_GetProcAddress ( SK_GetModuleHandleW (L"user32"),
                                    "GetDpiForWindow" );

  if (GetDpiForSystem != nullptr)
  {
    extern float
      g_fDPIScale;

    UINT dpi = ( GetDpiForWindow != nullptr &&
                        IsWindow (game_window.hWnd) ) ?
                 GetDpiForWindow (game_window.hWnd)   :
                 GetDpiForSystem (                );

    if (dpi != 0)
    {
      _uiDPI     = dpi;
      _fDPIScale =
        ( (float)dpi /
          (float)USER_DEFAULT_SCREEN_DPI ) * 100.0f;
    }
  }
};

typedef enum { PROCESS_DPI_UNAWARE           = 0,
               PROCESS_SYSTEM_DPI_AWARE      = 1,
               PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
typedef enum { MDT_EFFECTIVE_DPI = 0,
               MDT_ANGULAR_DPI   = 1,
               MDT_RAW_DPI       = 2,
               MDT_DEFAULT       = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;


typedef HRESULT (WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);        // Shcore.lib+dll, Windows 8.1


BOOL
SK_IsWindows8Point1OrGreater (void)
{
  SK_RunOnce (SetLastError (NO_ERROR));

  static BOOL
    bResult =
      GetProcAddress (
        GetModuleHandleW (L"kernel32.dll"),
                           "GetSystemTimePreciseAsFileTime"
                     ) != nullptr &&
      GetLastError  () == NO_ERROR;

  return bResult;
}

BOOL
SK_IsWindows10OrGreater (void)
{
  SK_RunOnce (SetLastError (NO_ERROR));

  static BOOL
    bResult =
      GetProcAddress (
        GetModuleHandleW (L"kernel32.dll"),
                           "SetThreadDescription"
                     ) != nullptr &&
      GetLastError  () == NO_ERROR;

  return bResult;
}

float
ImGui_ImplWin32_GetDpiScaleForMonitor (void* monitor)
{
  UINT xdpi = 96,
       ydpi = 96;

  if (SK_IsWindows8Point1OrGreater ())
  {
    static HINSTANCE     shcore_dll =
      SK_LoadLibraryW (L"shcore.dll"); // Reference counted per-process

    if ( PFN_GetDpiForMonitor GetDpiForMonitorFn =
        (PFN_GetDpiForMonitor)SK_GetProcAddress ( shcore_dll,
            "GetDpiForMonitor"                  )
       )
    {
      GetDpiForMonitorFn ( (HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi,
                                                                 &ydpi );
    }
  }

  else
  {
    const HDC dc   = GetDC         (          NULL);
              xdpi = GetDeviceCaps (dc, LOGPIXELSX);
              ydpi = GetDeviceCaps (dc, LOGPIXELSY);
                   ReleaseDC (NULL, dc);
  }

  IM_ASSERT(xdpi == ydpi); // Please contact me if you hit this assert!

  return    xdpi / 96.0f;
}

float
ImGui_ImplWin32_GetDpiScaleForHwnd (void* hwnd)
{
  HMONITOR monitor =
    MonitorFromWindow ((HWND)hwnd, MONITOR_DEFAULTTONEAREST);

  return
    ImGui_ImplWin32_GetDpiScaleForMonitor (monitor);
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGISwap_ResizeBuffers_Override (IDXGISwapChain* This,
  _In_ UINT            BufferCount,
  _In_ UINT            Width,
  _In_ UINT            Height,
  _In_ DXGI_FORMAT     NewFormat,
  _In_ UINT            SwapChainFlags)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.scanout.colorspace_override != DXGI_COLOR_SPACE_CUSTOM)
  {
    SK_ComQIPtr <
      IDXGISwapChain3
    >     pSwapChain3 (This);
    if (  pSwapChain3 != nullptr  )
    {
      SK_DXGI_UpdateColorSpace (pSwapChain3);
    }
  }

  // Can't do this if waitable
  //if ( dxgi_caps.present.waitable &&
  //     config.render.framerate.swapchain_wait > 0 )
  //{
  //  return S_OK;
  //}

  SwapChainFlags =
    SK_DXGI_FixUpLatencyWaitFlag (This, SwapChainFlags);

  NewFormat =
    SK_DXGI_PickHDRFormat (NewFormat);


  DXGI_LOG_CALL_I5 ( L"    IDXGISwapChain", L"ResizeBuffers         ",
    L"%lu,%lu,%lu,%s,0x%08X",
    BufferCount, Width, Height,
    SK_DXGI_FormatToStr (NewFormat).c_str (), SwapChainFlags );



  if (       config.render.framerate.buffer_count != -1           &&
       (UINT)config.render.framerate.buffer_count !=  BufferCount &&
       BufferCount                                !=  0           &&

           config.render.framerate.buffer_count   >   0           &&
           config.render.framerate.buffer_count   <   16 )
  {
    BufferCount =
      config.render.framerate.buffer_count;

    dll_log->Log ( L"[   DXGI   ]  >> Buffer Count Override: %lu buffers",
                                      BufferCount );
  }

  DXGI_SWAP_CHAIN_DESC swap_desc = { };
  This->GetDesc      (&swap_desc);

  // Fix-up BufferCount and Flags in Flip Model
  if (SK_DXGI_IsFlipModelSwapEffect (swap_desc.SwapEffect))
  {
    if (! dxgi_caps.swapchain.allow_tearing)
    {
      SwapChainFlags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    BufferCount =
      std::max (2ui32, BufferCount);
  }

  if (swap_desc.Windowed == FALSE)
  {
    // We need to clamp this in fullscreen mode.
    BufferCount =
      std::max ( 1Ui32,
                   std::min ( 3Ui32, BufferCount )
      );
  }

  if ( ( config.render.framerate.flip_discard ||
           SK_DXGI_IsFlipModelSwapEffect (
                    swap_desc.SwapEffect ) )  &&
           dxgi_caps.swapchain.allow_tearing
     )
  {
    SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    dll_log->Log ( L"[ DXGI 1.5 ]  >> Tearing Option:  Enable" );
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

  // Clamp the buffer dimensions if the user has a min/max preference
  const UINT
    max_x = config.render.dxgi.res.max.x < Width  ?
            config.render.dxgi.res.max.x : Width,
    min_x = config.render.dxgi.res.min.x > Width  ?
            config.render.dxgi.res.min.x : Width,
    max_y = config.render.dxgi.res.max.y < Height ?
            config.render.dxgi.res.max.y : Height,
    min_y = config.render.dxgi.res.min.y > Height ?
            config.render.dxgi.res.min.y : Height;

  Width   =  std::max ( max_x , min_x );
  Height  =  std::max ( max_y , min_y );


  static const auto game_id =
    SK_GetCurrentGameID ();

  if (game_id == SK_GAME_ID::DotHackGU)
  {
    NewFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
  }


  // If forcing flip-model, kill multisampling (of primary framebuffer)
  if (config.render.framerate.flip_discard)
  {
    bFlipMode =
      dxgi_caps.present.flip_sequential;

    // Format overrides must be performed in some cases (sRGB / 10:10:10:2)
    switch (NewFormat)
    {
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        NewFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        dll_log->Log ( L"[ DXGI 1.2 ]  >> sRGB (B8G8R8A8) Override "
                       L"Required to Enable Flip Model" );
        break;
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        NewFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        dll_log->Log ( L"[ DXGI 1.2 ]  >> sRGB (R8G8B8A8) Override "
                       L"Required to Enable Flip Model" );
        break;
      //case DXGI_FORMAT_R10G10B10A2_UNORM:
      //case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      //  NewFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
      //  dll_log->Log ( L"[ DXGI 1.2 ]  >> RGBA 10:10:10:2 Override "
      //                 L"(to 8:8:8:8) Required to Enable Flip Model" );
      //  break;

    }

  }
  //DXGI_FORMAT oldFormat = NewFormat;

  HRESULT     ret =
    ResizeBuffers_Original ( This, BufferCount, Width, Height,
                               NewFormat, SwapChainFlags );

  if (ret == DXGI_ERROR_INVALID_CALL)
  {
    SK_D3D11_EndFrame        ();
    SK_D3D11_ResetTexCache   ();
    SK_CEGUI_QueueResetD3D11 (); // Prior to the next present, reset the UI

    rb.releaseOwnedResources ();

    DXGI_CALL ( ret, ResizeBuffers_Original (
                       This, BufferCount, Width, Height,
                               NewFormat, SwapChainFlags )
              );

    //if (SK_GetCurrentGameID () == SK_GAME_ID::OctopathTraveler)
    //{
      if (FAILED (ret))
        return S_OK;
    //}
  }

  else
  {
    DXGI_CALL (ret, ret);
  }


  if (SUCCEEDED (ret))
  {
    if (Width != 0 && Height != 0)
    {
      SK_SetWindowResX (Width);
      SK_SetWindowResY (Height);
    }

    else
    {
      extern void SK_Display_PushDPIScaling         (void);
      extern void SK_Display_PopDPIScaling          (void);
      extern void SK_Display_SetMonitorDPIAwareness (bool bOnlyIfWin10);

      SK_Display_PushDPIScaling         (    );
      SK_Display_SetMonitorDPIAwareness (true);
      {
        UINT xdpi = 96,
             ydpi = 96;

        HMONITOR monitor =
          ::MonitorFromWindow (
            (HWND)game_window.hWnd, MONITOR_DEFAULTTONEAREST
          );

        static HINSTANCE     shcore_dll =
          SK_LoadLibraryW (L"shcore.dll"); // Reference counted per-process
        static PFN_GetDpiForMonitor GetDpiForMonitorFn =
              (PFN_GetDpiForMonitor)SK_GetProcAddress (shcore_dll,
                  "GetDpiForMonitor");

        if (GetDpiForMonitorFn != nullptr)
        {
          GetDpiForMonitorFn ( (HMONITOR)monitor,
            MDT_EFFECTIVE_DPI, &xdpi,
                               &ydpi );
        }

        RECT                               rect = { };
        GetClientRect ( game_window.hWnd, &rect );

        Width  = MulDiv (rect.right  - rect.left, USER_DEFAULT_SCREEN_DPI, xdpi);
        Height = MulDiv (rect.bottom - rect.top,  USER_DEFAULT_SCREEN_DPI, ydpi);
      }
      SK_Display_PopDPIScaling         (    );

      SK_SetWindowResX (Width);
      SK_SetWindowResY (Height);
    }
  }

  return ret;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGISwap_ResizeTarget_Override ( IDXGISwapChain *This,
                      _In_ const DXGI_MODE_DESC *pNewTargetParameters )
{
  // Can't do this if waitable
  //if ( dxgi_caps.present.waitable &&
  //     config.render.framerate.swapchain_wait > 0 )
  //{
  //  return S_OK;
  //}

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.scanout.colorspace_override != DXGI_COLOR_SPACE_CUSTOM)
  {
    SK_ComQIPtr <IDXGISwapChain3> pSwap3 (This);

    if (pSwap3 != nullptr)
    {
      SK_DXGI_UpdateColorSpace (pSwap3.p);
    }
  }

  assert (pNewTargetParameters != nullptr);

  DXGI_LOG_CALL_I6 (
    L"    IDXGISwapChain", L"ResizeTarget         ",
      L"{ (%lux%lu@%3.1f Hz),"
      L"fmt=%s,scaling=0x%02x,scanlines=0x%02x }",
                     pNewTargetParameters->Width,
                     pNewTargetParameters->Height,
                     pNewTargetParameters->RefreshRate.Denominator != 0 ?
      static_cast <float> (pNewTargetParameters->RefreshRate.Numerator) /
      static_cast <float> (pNewTargetParameters->RefreshRate.Denominator) :
                         std::numeric_limits <float>::quiet_NaN (),
      SK_DXGI_FormatToStr (pNewTargetParameters->Format).c_str  (),
                           pNewTargetParameters->Scaling,
                           pNewTargetParameters->ScanlineOrdering
  );

  HRESULT ret =
    E_UNEXPECTED;

  DXGI_MODE_DESC new_new_params =
      *pNewTargetParameters;

  // I don't remember why borderless is included here :)
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
    if (  config.render.framerate.rescan_.Denom != 1  ||
        ( config.render.framerate.refresh_rate  != -1 &&
          new_new_params.RefreshRate.Numerator  !=
    (UINT)config.render.framerate.refresh_rate )
       )
    {
      dll_log->Log (
        L"[   DXGI   ]  >> Refresh Override "
        L"(Requested: %f, Using: %f)",
                             new_new_params.RefreshRate.Denominator != 0 ?
        static_cast <float> (new_new_params.RefreshRate.Numerator)  /
        static_cast <float> (new_new_params.RefreshRate.Denominator)     :
          std::numeric_limits <float>::quiet_NaN (),
        static_cast <float> (config.render.framerate.rescan_.Numerator) /
        static_cast <float> (config.render.framerate.rescan_.Denom)
      );

      if (config.render.framerate.rescan_.Denom != 1)
      {
             new_new_params.RefreshRate.Numerator   =
        config.render.framerate.rescan_.Numerator;
             new_new_params.RefreshRate.Denominator =
        config.render.framerate.rescan_.Denom;
      }

      else
      {
        new_new_params.RefreshRate.Numerator   =
          (UINT)std::ceilf (config.render.framerate.refresh_rate);
        new_new_params.RefreshRate.Denominator = 1;
      }
    }

    if ( config.render.dxgi.scanline_order        != -1 &&
          pNewTargetParameters->ScanlineOrdering  !=
            (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order )
    {
      dll_log->Log (
        L"[   DXGI   ]  >> Scanline Override "
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
      dll_log->Log (
        L"[   DXGI   ]  >> Scaling Override "
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
  }

  if (! config.window.res.override.isZero ())
  {
    new_new_params.Width  = config.window.res.override.x;
    new_new_params.Height = config.window.res.override.y;
  }

  else if ( (! config.window.fullscreen) &&
               config.window.borderless )
  {
    SK_DXGI_BorderCompensation ( new_new_params.Width,
                                 new_new_params.Height );
  }

  DXGI_MODE_DESC* pNewNewTargetParameters =
    &new_new_params;


  // Clamp the buffer dimensions if the user has a min/max preference
  const UINT
    max_x = config.render.dxgi.res.max.x < new_new_params.Width  ?
            config.render.dxgi.res.max.x : new_new_params.Width,
    min_x = config.render.dxgi.res.min.x > new_new_params.Width  ?
            config.render.dxgi.res.min.x : new_new_params.Width,
    max_y = config.render.dxgi.res.max.y < new_new_params.Height ?
            config.render.dxgi.res.max.y : new_new_params.Height,
    min_y = config.render.dxgi.res.min.y > new_new_params.Height ?
            config.render.dxgi.res.min.y : new_new_params.Height;

  new_new_params.Width   =  std::max ( max_x , min_x );
  new_new_params.Height  =  std::max ( max_y , min_y );


  pNewNewTargetParameters->Format =
    SK_DXGI_PickHDRFormat (pNewNewTargetParameters->Format);

  ret =
    ResizeTarget_Original (This, pNewNewTargetParameters);

  if (FAILED (ret))
  {
    SK_D3D11_EndFrame        ();
    SK_D3D11_ResetTexCache   ();
    SK_CEGUI_QueueResetD3D11 (); // Prior to next present, reset the UI

    DXGI_CALL ( ret,
                  ResizeTarget_Original ( This, pNewNewTargetParameters )
              );
  }

  else
    DXGI_CALL ( ret, ret );


  if (SUCCEEDED (ret))
  {
    if ( pNewNewTargetParameters->Width  != 0 &&
         pNewNewTargetParameters->Height != 0 )
    {
      SK_SetWindowResX (pNewNewTargetParameters->Width);
      SK_SetWindowResY (pNewNewTargetParameters->Height);
    }

    else
    {
      RECT client = { };

      GetClientRect (game_window.hWnd, &client);

      SK_SetWindowResX (client.right  - client.left);
      SK_SetWindowResY (client.bottom - client.top);
    }
  }

  return ret;
}

__forceinline
void
SK_DXGI_CreateSwapChain_PreInit (
  _Inout_opt_ DXGI_SWAP_CHAIN_DESC            *pDesc,
  _Inout_opt_ DXGI_SWAP_CHAIN_DESC1           *pDesc1,
  _Inout_opt_ HWND&                            hWnd,
  _Inout_opt_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc )
{
  WaitForInitDXGI ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  rb.releaseOwnedResources ();
  SK_D3D11_ResetTexCache   ();
  SK_CEGUI_QueueResetD3D11 (); // Prior to the next present, reset the UI

  // Stores common attributes between DESC and DESC1
  DXGI_SWAP_CHAIN_DESC stub_desc  = {   };
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

        if (stub_desc.Windowed)
        {
          stub_desc.BufferDesc.RefreshRate.Denominator = 0;
          stub_desc.BufferDesc.RefreshRate.Numerator   = 0;
        }
      }

      else
      {
        stub_desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
        stub_desc.Windowed                           = TRUE;
        stub_desc.BufferDesc.RefreshRate.Denominator = 0;
        stub_desc.BufferDesc.RefreshRate.Numerator   = 0;
      }

      // Need to take this stuff and put it back in the appropriate structures before returning :)
      translated = true;
    }
  }

  if (pDesc != nullptr)
  {
    // If auto-update prompt is visible, don't go fullscreen.
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

    dll_log->LogEx ( true,
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


    if (config.render.framerate.disable_flip)
    {
      if (     pDesc->SwapEffect == (DXGI_SWAP_EFFECT)DXGI_SWAP_EFFECT_FLIP_DISCARD)
               pDesc->SwapEffect =                         DXGI_SWAP_EFFECT_DISCARD;
      else if (pDesc->SwapEffect == (DXGI_SWAP_EFFECT)DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL)
               pDesc->SwapEffect =                         DXGI_SWAP_EFFECT_SEQUENTIAL;
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


    if (config.render.dxgi.msaa_samples > 0)
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
        dll_log->Log ( L"[   DXGI   ]  >> User-Requested Mode Change: Fullscreen" );
        pDesc->Windowed = FALSE;
        pDesc->Flags   |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }

      if (config.display.force_fullscreen && pDesc->Windowed)
      {
        dll_log->Log ( L"[   DXGI   ]  >> Display Override "
                       L"(Requested: Windowed, Using: Fullscreen)" );
        pDesc->Flags   |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        pDesc->Windowed = FALSE;
      }

      else if (__SK_HDR_16BitSwap || config.display.force_windowed || config.render.framerate.flip_discard)
      {
        dll_log->Log ( L"[   DXGI   ]  >> Display Override "
                       L"(Requested: Fullscreen, Using: Windowed)" );
        pDesc->Windowed = TRUE;
      }

      static const auto
        game_id = SK_GetCurrentGameID ();
#ifdef _WIN64
      if (! bFlipMode)
      {
        bFlipMode =
          ( dxgi_caps.present.flip_sequential &&
            ( game_id == SK_GAME_ID::Fallout4 ||
              SK_DS3_UseFlipMode  ()        ) );

        // For the one in a million games that actually use this (:-\)
        if ( SK_DXGI_IsFlipModelSwapEffect (pDesc->SwapEffect) )
        {
          bFlipMode = true;
        }
      }

      if (game_id == SK_GAME_ID::Fallout4)
      {
        if (bFlipMode)
            bFlipMode = (! SK_FO4_IsFullscreen ()) && SK_FO4_UseFlipMode ();
      }

      else
#endif
      {
        // If forcing flip-model, then force multisampling (of the primary framebuffer) off
        if (config.render.framerate.flip_discard)
        {
          bFlipMode =
            dxgi_caps.present.flip_sequential;

          pDesc->SampleDesc.Count   = 1;
          pDesc->SampleDesc.Quality = 0;

          // Format overrides must be performed in certain cases (sRGB / 10:10:10:2)
          switch (pDesc->BufferDesc.Format)
          {
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
              dll_log->Log ( L"[ DXGI 1.2 ]  >> sRGB (B8G8R8A8) Override Required to Enable Flip Model" );
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
              pDesc->BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
              break;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
              pDesc->BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
              dll_log->Log ( L"[ DXGI 1.2 ]  >> sRGB (R8G8B8A8) Override Required to Enable Flip Model" );
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
              pDesc->BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
              break;
              //pDesc->BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
              //dll_log->Log ( L"[ DXGI 1.2 ]  >> BGRA (R8G8B8A8) Override Required to Enable Flip Model" );
              break;
          }
        }
      }

      if (       config.render.framerate.buffer_count != -1                  &&
           (UINT)config.render.framerate.buffer_count !=  pDesc->BufferCount &&
           pDesc->BufferCount                         !=  0                  &&

           config.render.framerate.buffer_count       >   0                  &&
           config.render.framerate.buffer_count       <   16 )
      {
        pDesc->BufferCount = config.render.framerate.buffer_count;
        dll_log->Log (L"[   DXGI   ]  >> Buffer Count Override: %lu buffers", pDesc->BufferCount);
      }

      if ( ( config.render.framerate.flip_discard ||
                                      bFlipMode ) &&
              dxgi_caps.swapchain.allow_tearing )
      {
        /////if (config.render.framerate.flip_discard)
        /////  pDesc->Windowed = TRUE;

        if (pDesc->Windowed)
        {
          pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
          dll_log->Log ( L"[ DXGI 1.5 ]  >> Tearing Option:  Enable" );
        }
      }

      if ( config.render.dxgi.scaling_mode != -1 &&
            pDesc->BufferDesc.Scaling      !=
              (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
      {
        dll_log->Log ( L"[   DXGI   ]  >> Scaling Override "
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
        dll_log->Log ( L"[   DXGI   ]  >> Scanline Override "
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

      if ( config.render.framerate.rescan_.Denom   !=  1 ||
          (config.render.framerate.refresh_rate    != -1 &&
           pDesc->BufferDesc.RefreshRate.Numerator != static_cast <UINT> (
           config.render.framerate.refresh_rate                          )
          )
        )
      {
        dll_log->Log ( L"[   DXGI   ]  >> Refresh Override "
                       L"(Requested: %f, Using: %f)",
                    pDesc->BufferDesc.RefreshRate.Denominator != 0 ?
            static_cast <float> (pDesc->BufferDesc.RefreshRate.Numerator) /
            static_cast <float> (pDesc->BufferDesc.RefreshRate.Denominator) :
                        std::numeric_limits <float>::quiet_NaN (),
            static_cast <float> (config.render.framerate.rescan_.Numerator) /
            static_cast <float> (config.render.framerate.rescan_.Denom)
                     );

        if (config.render.framerate.rescan_.Denom != 1)
        {
          pDesc->BufferDesc.RefreshRate.Numerator   = config.render.framerate.rescan_.Numerator;
          pDesc->BufferDesc.RefreshRate.Denominator = config.render.framerate.rescan_.Denom;
        }

        else
        {
          pDesc->BufferDesc.RefreshRate.Numerator   =
            static_cast <UINT> (
              std::ceil (config.render.framerate.refresh_rate)
            );
          pDesc->BufferDesc.RefreshRate.Denominator = 1;
        }
      }

      bWait =
        bFlipMode && dxgi_caps.present.waitable;

      // We cannot change the swapchain parameters if this is used...
      bWait =
        bWait && ( config.render.framerate.swapchain_wait > 0 );

#ifdef _WIN64
      if (game_id == SK_GAME_ID::DarkSouls3)
      {
        if (SK_DS3_IsBorderless ())
          pDesc->Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      }
#endif

      // User is forcing flip mode
      //
      if (bFlipMode)
      {
        if (bWait)
          pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

        // Flip Presentation Model requires 3 Buffers (1 is implicit)
        config.render.framerate.buffer_count =
          std::max (2, std::min (16, config.render.framerate.buffer_count));

        if (config.render.framerate.flip_discard &&
            dxgi_caps.present.flip_discard)
          pDesc->SwapEffect  = (DXGI_SWAP_EFFECT)DXGI_SWAP_EFFECT_FLIP_DISCARD;

        else // On Windows 8.1 and older, sequential must substitute for discard
          pDesc->SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      }

      // User is not forcing flip mode, and the game is not using it either
      //
      else if ( pDesc->SwapEffect != DXGI_SWAP_EFFECT_FLIP_DISCARD &&
                pDesc->SwapEffect != DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL )
      {
        // Resort to triple-buffering if flip mode is not available
        //   because AMD drivers tend not to work correctly if a game ever
        //     tries to use more than 3 buffers in BitBlit mode.
        if (config.render.framerate.buffer_count > 3)
            config.render.framerate.buffer_count = 3;
      }

      if (config.render.framerate.buffer_count > 0)
        pDesc->BufferCount = config.render.framerate.buffer_count;

      if (bFlipMode && bWait)
      {
        pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
      }
    }

    SK_LOG0 ( ( L"  >> Using %s Presentation Model  [Waitable: %s - %li ms]",
                   (pDesc->SwapEffect >= DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL) ?
                     L"Flip" : L"Traditional",
                       bWait ? L"Yes" : L"No",
                       bWait ? config.render.framerate.swapchain_wait : 0 ),
                L" DXGI 1.2 " );


    // HDR override requires Flip Model
    //
    //  --> Flip Model requires no MSAA
    //
    if (SK_DXGI_IsFlipModelSwapEffect (pDesc->SwapEffect))
    {
      pDesc->BufferDesc.Format =
        SK_DXGI_PickHDRFormat (pDesc->BufferDesc.Format);

      if (pDesc->SampleDesc.Count != 1)
      {
        SK_LOG0 ( ( L"  >> Disabling SwapChain-based MSAA for Flip Model compliance."),
                    L" DXGI 1.2 " );

        pDesc->SampleDesc.Count = 1;
      }
    }

    // Clamp the buffer dimensions if the user has a min/max preference
    const UINT
      max_x = config.render.dxgi.res.max.x < pDesc->BufferDesc.Width  ?
              config.render.dxgi.res.max.x : pDesc->BufferDesc.Width,
      min_x = config.render.dxgi.res.min.x > pDesc->BufferDesc.Width  ?
              config.render.dxgi.res.min.x : pDesc->BufferDesc.Width,
      max_y = config.render.dxgi.res.max.y < pDesc->BufferDesc.Height ?
              config.render.dxgi.res.max.y : pDesc->BufferDesc.Height,
      min_y = config.render.dxgi.res.min.y > pDesc->BufferDesc.Height ?
              config.render.dxgi.res.min.y : pDesc->BufferDesc.Height;

    pDesc->BufferDesc.Width   =  std::max ( max_x , min_x );
    pDesc->BufferDesc.Height  =  std::max ( max_y , min_y );
  }


  if (translated)
  {
    pDesc1->BufferCount = pDesc->BufferCount;
    pDesc1->BufferUsage = pDesc->BufferUsage;
    pDesc1->Flags       = pDesc->Flags;
    pDesc1->SwapEffect  = pDesc->SwapEffect;
    pDesc1->Height      = pDesc->BufferDesc.Height;
    pDesc1->Width       = pDesc->BufferDesc.Width;
    pDesc1->Format      = pDesc->BufferDesc.Format;
    pDesc1->SampleDesc.Count
                        = pDesc->SampleDesc.Count;
    pDesc1->SampleDesc.Quality
                        = pDesc->SampleDesc.Quality;
  }
}

void
SK_DXGI_UpdateLatencies (IDXGISwapChain *pSwapChain)
{
  if (! pSwapChain)
    return;

  const uint32_t max_latency =
    config.render.framerate.pre_render_limit;

  SK_ComQIPtr <IDXGISwapChain2> pSwapChain2 (pSwapChain);

  if (pSwapChain2 != nullptr)
  {
    if (max_latency < 16 && max_latency > 0)
    {
      dll_log->Log ( L"[   DXGI   ] Setting Swapchain Frame Latency: %lu",
                       max_latency );
      pSwapChain2->SetMaximumFrameLatency (max_latency);
    }
  }

  if (max_latency < 16 && max_latency > 0)
  {
    SK_ComPtr <IDXGIDevice1> pDevice1;

    if (SUCCEEDED ( pSwapChain->GetDevice (
                       IID_PPV_ARGS (&pDevice1.p)
                                          )
                  )
       )
    {
      dll_log->Log ( L"[   DXGI   ] Setting Device    Frame Latency: %lu",
                       max_latency );
      pDevice1->SetMaximumFrameLatency (max_latency);
    }
  }
}

__forceinline
void
SK_DXGI_CreateSwapChain_PostInit (
  _In_  IUnknown              *pDevice,
  _In_  DXGI_SWAP_CHAIN_DESC  *pDesc,
  _In_  IDXGISwapChain       **ppSwapChain )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (ppSwapChain != nullptr)
    SK_DXGI_HookSwapChain (*ppSwapChain);

  wchar_t wszClass [MAX_PATH + 2] = { };

  if (pDesc != nullptr)
    RealGetWindowClassW (pDesc->OutputWindow, wszClass, MAX_PATH);

  bool dummy_window =
    StrStrIW (wszClass, L"Special K Dummy Window Class") ||
    StrStrIW (wszClass, L"RTSSWndClass");

  if ( (! dummy_window) && pDesc != nullptr &&
       ( dwRenderThread == 0 ||
         dwRenderThread == SK_Thread_GetCurrentId () )
     )
  {
    if ( dwRenderThread == 0x00 ||
         dwRenderThread == SK_Thread_GetCurrentId () )
    {
      auto& windows =
        rb.windows;

      if (      windows.device != nullptr &&
           pDesc->OutputWindow != nullptr &&
           pDesc->OutputWindow != windows.device )
      {
        dll_log->Log (L"[   DXGI   ] Game created a new window?!");
      }

      SK_InstallWindowHook (pDesc->OutputWindow);

      windows.setDevice (pDesc->OutputWindow);
      windows.setFocus  (pDesc->OutputWindow);
    }

    if (ppSwapChain != nullptr)
      SK_DXGI_HookSwapChain (*ppSwapChain);

    // Assume the first thing to create a D3D11 render device is
    //   the game and that devices never migrate threads; for most games
    //     this assumption holds.
    if ( dwRenderThread == 0x00 )
    {
      dwRenderThread = SK_Thread_GetCurrentId ();
    }
  }

  rb.releaseOwnedResources ();
  SK_D3D11_ResetTexCache   ();
  SK_CEGUI_QueueResetD3D11 (); // Prior to the next present, reset the UI

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

  if (  ppSwapChain != nullptr &&
       *ppSwapChain != nullptr )
  {
    SK_ComQIPtr <IDXGISwapChain2> pSwapChain2 (*ppSwapChain);

    SK_DXGI_UpdateLatencies (pSwapChain2);

    if (pSwapChain2 != nullptr)
    {
      if (bFlipMode && bWait)
      {
        SK_AutoHandle hWait (
          pSwapChain2->GetFrameLatencyWaitableObject ()
        );

        MsgWaitForMultipleObjectsEx (
          1, &hWait.m_h,
            config.render.framerate.swapchain_wait,
              QS_ALLINPUT, MWMO_INPUTAVAILABLE
        );
      }
    }
  }

  SK_ComQIPtr <ID3D11Device> pDev (pDevice);

  if (pDev != nullptr)
  {
    g_pD3D11Dev = pDev;

    if (pDesc != nullptr)
      rb.fullscreen_exclusive = (! pDesc->Windowed);
  }
}

__forceinline
void
SK_DXGI_CreateSwapChain1_PostInit (
  _In_     IUnknown                         *pDevice,
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

  // ONLY AS COMPLETE AS NEEDED, if new code is added to PostInit, this
  //   will probably need changing.
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
    desc.BufferDesc.RefreshRate.Numerator
                                     = pFullscreenDesc->RefreshRate.Numerator;
    desc.BufferDesc.RefreshRate.Denominator
                                     = pFullscreenDesc->RefreshRate.Denominator;
    desc.BufferDesc.ScanlineOrdering = pFullscreenDesc->ScanlineOrdering;
  }

  else
  {
    desc.Windowed                    = TRUE;
  }

  if ( ppSwapChain1 != nullptr &&
      *ppSwapChain1 != nullptr )
  {
    SK_ComQIPtr <IDXGISwapChain> pSwapChain (*ppSwapChain1);

    return
      SK_DXGI_CreateSwapChain_PostInit ( pDevice, &desc, &pSwapChain.p );
  }
}

#include <d3d12.h>

void
SK_DXGI_LazyHookFactory (IDXGIFactory *pFactory)
{
  void       SK_DXGI_HookFactory  (IDXGIFactory * pFactory);
  SK_RunOnce (SK_DXGI_HookFactory (pFactory));
  SK_RunOnce (SK_ApplyQueuedHooks (        ));
}

void
SK_DXGI_LazyHookPresent (IDXGISwapChain *pSwapChain)
{
  void SK_DXGI_HookPresentBase (IDXGISwapChain  *pSwapChain);
  void SK_DXGI_HookPresent1    (IDXGISwapChain1 *pSwapChain1);

  SK_RunOnce (SK_DXGI_HookSwapChain (pSwapChain));

  // This won't catch Present1 (...), but no games use that
  //   and we can deal with it later if it happens.
  SK_DXGI_HookPresentBase ((IDXGISwapChain *)pSwapChain);

  SK_ComQIPtr <IDXGISwapChain1> pSwapChain1 (pSwapChain);

  if (pSwapChain1 != nullptr)
    SK_DXGI_HookPresent1 (pSwapChain1);
}

IWrapDXGISwapChain*
SK_DXGI_WrapSwapChain ( IUnknown        *pDevice,
                        IDXGISwapChain  *pSwapChain,
                        IDXGISwapChain **ppDest )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_RunOnce (SK_DXGI_LazyHookPresent (pSwapChain));

  DXGI_SWAP_CHAIN_DESC  desc = { };
  pSwapChain->GetDesc (&desc);

  SK_ComQIPtr <ID3D11Device>       pDev11    (pDevice);
  SK_ComQIPtr <ID3D12CommandQueue> pCmdQueue (pDevice);

  if (pCmdQueue.p != nullptr)
  {
    rb.api       = SK_RenderAPI::D3D12;
    rb.swapchain = pSwapChain;

    *ppDest =
      new IWrapDXGISwapChain ((ID3D11Device *)pDevice, pSwapChain);

    return
      (IWrapDXGISwapChain*)*ppDest;
  }

  if ( pDev11 != nullptr && SK_DXGI_IsSwapChainReal (desc) )
  {
    *ppDest =
      new IWrapDXGISwapChain ((ID3D11Device *)pDevice, pSwapChain);

    SK_LOG0 ( (L" + SwapChain <IDXGISwapChain> (%08" PRIxPTR L"h) wrapped",
                 (uintptr_t)pSwapChain ),
               L"   DXGI   " );

    return (IWrapDXGISwapChain *)*ppDest;
  }

  return nullptr;
}

#include <SpecialK\render\d3d12\d3d12_interfaces.h>

IWrapDXGISwapChain*
SK_DXGI_WrapSwapChain1 ( IUnknown         *pDevice,
                         IDXGISwapChain1  *pSwapChain,
                         IDXGISwapChain1 **ppDest )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_RunOnce (SK_DXGI_LazyHookPresent (pSwapChain));

  DXGI_SWAP_CHAIN_DESC  desc = { };
  pSwapChain->GetDesc (&desc);

  SK_ComQIPtr <ID3D11Device>       pDev11    (pDevice);
  SK_ComQIPtr <ID3D12CommandQueue> pCmdQueue (pDevice);

  if (pCmdQueue.p != nullptr)
  {
    rb.api       = SK_RenderAPI::D3D12;
    rb.swapchain = pSwapChain;

    ////SK_ComPtr <ID3D12Device>           pD3D12Dev;
    ////pCmdQueue->GetDevice
    ////      (IID_ID3D12Device, (void **)&pD3D12Dev.p);
    ////
    ////if ( SUCCEEDED (
    ////       D3D11On12CreateDevice ( pD3D12Dev.p, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
    ////                                                     nullptr, 0,
    ////                                (IUnknown **)&pCmdQueue.p, 1, 0,
    ////                            (ID3D11Device **)&rb.device.p,
    ////                                             &rb.d3d11.immediate_ctx, nullptr )
    ////               )
    ////   )
    ////{
    ////  rb.interop.d3d12.dev =
    ////    pD3D12Dev;
    ////
    //////SK_LOG0 ( ( L"D3D11On12 Interop" ), L"  D3D 12  " );
    ////}

    *ppDest =
      new IWrapDXGISwapChain ((ID3D11Device*)pDevice, pSwapChain);

    return
      (IWrapDXGISwapChain*)*ppDest;
  }

  if ( pDev11 != nullptr && SK_DXGI_IsSwapChainReal (desc) )
  {
    *ppDest =
      new IWrapDXGISwapChain ((ID3D11Device *)pDevice, pSwapChain);

    SK_LOG0 ( (L" + SwapChain <IDXGISwapChain1> (%08" PRIxPTR L"h) wrapped",
                 (uintptr_t)pSwapChain ),
               L"   DXGI   " );

    return (IWrapDXGISwapChain *)*ppDest;
  }

  return nullptr;
}

#include <d3d12.h>

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
DXGIFactory_CreateSwapChain_Override (
              IDXGIFactory          *This,
  _In_        IUnknown              *pDevice,
  _In_  const DXGI_SWAP_CHAIN_DESC  *pDesc,
  _Out_       IDXGISwapChain       **ppSwapChain )
{
  std::wstring iname =
    SK_GetDXGIFactoryInterface (This);

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

  if (! SK_DXGI_IsSwapChainReal (*pDesc))
  {
    return
      CreateSwapChain_Original ( This, pDevice,
                                       pDesc, ppSwapChain );
  }

  SK_ComQIPtr <ID3D11Device>       pD3D11Dev   (pDevice);
  SK_ComQIPtr <ID3D12CommandQueue> pD3D12Queue (pDevice);

  if (pD3D12Queue != nullptr)
  {
    SK_LOG0 ( ( L" <*> Native D3D12 SwapChain Captured" ), L"Direct3D12" );
  }

  auto                 orig_desc = pDesc;
  DXGI_SWAP_CHAIN_DESC new_desc  =
    pDesc != nullptr ?
      *pDesc :
        DXGI_SWAP_CHAIN_DESC { };

  SK_DXGI_CreateSwapChain_PreInit (
    &new_desc,              nullptr,
     new_desc.OutputWindow, nullptr
  );

#ifdef  __NIER_HACK
  static SK_ComPtr <IDXGISwapChain> pSwapToRecycle = nullptr;

  if (pSwapToRecycle != nullptr)
  {
    DXGI_SWAP_CHAIN_DESC      recycle_desc = { };
    pSwapToRecycle->GetDesc (&recycle_desc);

    ///if ( recycle_desc.BufferDesc.Format == DXGI_FORMAT_UNKNOWN ||
    ///     new_desc.BufferDesc.Format     == DXGI_FORMAT_UNKNOWN ||
    ///     recycle_desc.BufferDesc.Format == new_desc.BufferDesc.Format )
    {
      if (recycle_desc.OutputWindow == new_desc.OutputWindow)
      {
        if (ppSwapChain != nullptr)
        {
          // Add Ref because game expects to create a new object and we are
          //   returning an existing ref-counted object in its place.

                          pSwapToRecycle.p->AddRef ();
           *ppSwapChain = pSwapToRecycle.p;
        }

        return S_OK;
      }
    }

    // Copy, Swap & Chop - Wham, Bam, Thank You Garbage Man - SwapChain Murderer
    //
    //  * Recycle candidate not a clean match; we only give out clean garbage!
    //
    //     1) Stop holding an extra reference to prolong its lifetime
    //     2) Stop trying to substitute new SwapChain creation using it
    //        \--> By resetting the ptr.
    //
    SK_ComPtr <IDXGISwapChain> pSwapToKill =
                               pSwapToRecycle;
    pSwapToRecycle = nullptr;
    pSwapToKill.p->Release ();
  }
#endif

  if (pDesc != nullptr) pDesc = &new_desc;

  auto CreateSwapChain_Lambchop =
    [&] (void) ->
      BOOL
      {
        IDXGISwapChain* pTemp = nullptr;

        DXGI_CALL ( ret,
                      CreateSwapChain_Original ( This, pDevice,
                                                       pDesc, &pTemp )
                  );

        if ( SUCCEEDED (ret) &&
             pTemp != nullptr )
        {
          SK_DXGI_CreateSwapChain_PostInit (pDevice, &new_desc, &pTemp);
          SK_DXGI_WrapSwapChain            (pDevice,             pTemp,
                                                   ppSwapChain);

#ifdef  __NIER_HACK
          if (  pSwapToRecycle == nullptr &&
               ppSwapChain     != nullptr )
          {
            pSwapToRecycle = *ppSwapChain;

            // We are going to reuse this, it needs a long and prosperous life.
            pSwapToRecycle.p->AddRef ();
          }
#endif

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
DXGIFactory2_CreateSwapChainForCoreWindow_Override (
                IDXGIFactory2             *This,
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
  {
    return
      CreateSwapChainForCoreWindow_Original (
        This, pDevice, pWindow,
              pDesc,   pRestrictToOutput, ppSwapChain
      );
  }

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
          SK_DXGI_CreateSwapChain1_PostInit (pDevice, nullptr, &new_desc1,
                                                      nullptr, &pTemp);
          SK_DXGI_WrapSwapChain1            (pDevice,           pTemp,
                                                      ppSwapChain);

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
DXGIFactory2_CreateSwapChainForHwnd_Override (
  IDXGIFactory2                                *This,
    _In_       IUnknown                        *pDevice,
    _In_       HWND                             hWnd,
    _In_ const DXGI_SWAP_CHAIN_DESC1           *pDesc,
_In_opt_       DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
_In_opt_       IDXGIOutput                     *pRestrictToOutput,
   _Out_       IDXGISwapChain1                 **ppSwapChain )
{
  std::wstring iname =
    SK_GetDXGIFactoryInterface (This);

  // Wrong prototype, but who cares right now? :P
  DXGI_LOG_CALL_I3 ( iname.c_str (), L"CreateSwapChainForHwnd         ",
                       L"%08" PRIxPTR L"h, %08" PRIxPTR L"h, %08"
                              PRIxPTR L"h",
                         (uintptr_t)pDevice, (uintptr_t)pDesc, (uintptr_t)ppSwapChain );

//  if (iname == L"{Invalid-Factory-UUID}")
//    return CreateSwapChainForHwnd_Original (This, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);

  HRESULT ret = E_FAIL;

  DXGI_SWAP_CHAIN_DESC
    res_only_desc                   = { };
    res_only_desc.BufferDesc.Width  = pDesc->Width;
    res_only_desc.BufferDesc.Height = pDesc->Height;

  if (! SK_DXGI_IsSwapChainReal (res_only_desc))
  {
    return
      CreateSwapChainForHwnd_Original ( This, pDevice, hWnd, pDesc,
                                                pFullscreenDesc, pRestrictToOutput,
                                                  ppSwapChain );
  }

  assert (pDesc != nullptr);

  auto                            orig_desc1           = pDesc;
  auto                            orig_fullscreen_desc = pFullscreenDesc;

  DXGI_SWAP_CHAIN_DESC1           new_desc1           = *pDesc;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC new_fullscreen_desc =
    pFullscreenDesc ? *pFullscreenDesc :
                       DXGI_SWAP_CHAIN_FULLSCREEN_DESC { };

  pDesc           = &new_desc1;
  pFullscreenDesc = orig_fullscreen_desc ? &new_fullscreen_desc : nullptr;

  SK_TLS_Bottom ()->d3d11->ctx_init_thread = true;

  SK_DXGI_CreateSwapChain_PreInit (nullptr, &new_desc1, hWnd, pFullscreenDesc);

  IDXGISwapChain1* pTemp = nullptr;

  SK_ComQIPtr <ID3D12CommandQueue>
      pCmdQueue (pDevice);
  if (pCmdQueue != nullptr)
  {
    SK_LOG0 ( ( L" <*> Native D3D12 SwapChain Captured" ), L"Direct3D12" );
  }

#ifdef  __CROSSCODE_HACK
  static SK_ComPtr <IDXGISwapChain1> pSwapToRecycle_Win  = nullptr;
  static SK_ComPtr <IDXGISwapChain1> pSwapToRecycle_Full = nullptr;
  static HWND                        hWndLast_Win        = nullptr;
  static HWND                        hWndLast_Full       = nullptr;

  SK_ComPtr <IDXGISwapChain1>& pSwapToRecycle =
       pFullscreenDesc != nullptr &&
    (! pFullscreenDesc->Windowed) ? pSwapToRecycle_Full :
                                    pSwapToRecycle_Win;

  HWND& hWndLast =
     pFullscreenDesc != nullptr &&
  (! pFullscreenDesc->Windowed) ? hWndLast_Full :
                                  hWndLast_Win;

  if (pSwapToRecycle != nullptr && hWndLast != nullptr)
  {
    DXGI_SWAP_CHAIN_DESC1      recycle_desc = { };
    pSwapToRecycle->GetDesc1 (&recycle_desc);

    if (hWndLast == hWnd)
    {
      if (ppSwapChain != nullptr)
      {
        // Add Ref because game expects to create a new object and we are
        //   returning an existing ref-counted object in its place.

        pSwapToRecycle.p->AddRef ();
        *ppSwapChain = pSwapToRecycle.p;
      }

      return S_OK;
    }

    SK_ComPtr <IDXGISwapChain1> pSwapToKill =
                                pSwapToRecycle;
    hWndLast       = nullptr;
    pSwapToRecycle = nullptr;
    pSwapToKill.p->Release ();
  }
#endif


  DXGI_CALL ( ret, CreateSwapChainForHwnd_Original ( This, pDevice, hWnd, pDesc, pFullscreenDesc,
                                                       pRestrictToOutput, &pTemp ) );

  if ( SUCCEEDED (ret) )
  {
    SK_DXGI_CreateSwapChain1_PostInit (pDevice, hWnd, &new_desc1, &new_fullscreen_desc, &pTemp);
      SK_DXGI_WrapSwapChain1          (pDevice,                                          pTemp, ppSwapChain);


#ifdef  __CROSSCODE_HACK
    if (  pSwapToRecycle == nullptr &&
         ppSwapChain     != nullptr )
    {
      hWndLast       =  hWnd;
      pSwapToRecycle = *ppSwapChain;

      // We are going to reuse this, it needs a long and prosperous life.
      pSwapToRecycle.p->AddRef ();
    }
#endif


    return ret;
  }

  DXGI_CALL ( ret, CreateSwapChainForHwnd_Original ( This, pDevice, hWnd, orig_desc1/*pDesc*/, orig_fullscreen_desc/*pFullscreenDesc*/,
                                                       pRestrictToOutput, ppSwapChain ) );

#ifdef  __CROSSCODE_HACK
    if (  pSwapToRecycle == nullptr &&
         ppSwapChain     != nullptr )
    {
      hWndLast       =  hWnd;
      pSwapToRecycle = *ppSwapChain;

      // We are going to reuse this, it needs a long and prosperous life.
      pSwapToRecycle.p->AddRef ();
    }
#endif

  return ret;
}

HRESULT
STDMETHODCALLTYPE
DXGIFactory2_CreateSwapChainForComposition_Override (
               IDXGIFactory2          *This,
_In_           IUnknown               *pDevice,
_In_     const DXGI_SWAP_CHAIN_DESC1  *pDesc,
_In_opt_       IDXGIOutput            *pRestrictToOutput,
_Outptr_       IDXGISwapChain1       **ppSwapChain )
{
  std::wstring iname =
    SK_GetDXGIFactoryInterface (This);

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
  __Microsoft = 0x1414,
  __Intel     = 0x8086
} Vendors;

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
    if (CreateDXGIFactory2_Import != nullptr)
      res = CreateDXGIFactory2 (0x0, __uuidof (IDXGIFactory1), static_cast_p2p <void> (&pFactory));

    else if (CreateDXGIFactory1_Import != nullptr)
      res = CreateDXGIFactory1 (     __uuidof (IDXGIFactory),  static_cast_p2p <void> (&pFactory));

    else
      res = CreateDXGIFactory  (     __uuidof (IDXGIFactory),  static_cast_p2p <void> (&pFactory));
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

        if ( game_desc.VendorId     == Vendors::__Intel     &&
             override_desc.VendorId != Vendors::__Microsoft &&
             override_desc.VendorId != Vendors::__Intel )
        {
          dll_log->Log ( L"[   DXGI   ] !!! DXGI Adapter Override: (Using '%s' instead of '%s') !!!",
                         override_desc.Description, game_desc.Description );

          *ppAdapter = pOverrideAdapter;
          pGameAdapter->Release ();
        }

        else
        {
          dll_log->Log ( L"[   DXGI   ] !!! DXGI Adapter Override: (Tried '%s' instead of '%s') !!!",
                        override_desc.Description, game_desc.Description );
          //SK_DXGI_preferred_adapter = -1;
          pOverrideAdapter->Release ();
        }
      }

      else
      {
        dll_log->Log ( L"[   DXGI   ] !!! DXGI Adapter Override Failed, returning '%s' !!!",
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
    dll_log->LogEx ( true,
      L" <> GetDesc2_Override: Looking for matching NVAPI GPU for %s...: ",
      pDesc->Description );

    DXGI_ADAPTER_DESC* match =
      sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

    if (match != nullptr)
    {
      dll_log->LogEx (false, L"Success! (%s)\n", match->Description);
      pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
    }
    else
      dll_log->LogEx (false, L"Failure! (No Match Found)\n");
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
    dll_log->LogEx ( true,
      L" <> GetDesc1_Override: Looking for matching NVAPI GPU for %s...: ",
      pDesc->Description );

    DXGI_ADAPTER_DESC* match =
      sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

    if (match != nullptr)
    {
      dll_log->LogEx (false, L"Success! (%s)\n", match->Description);
      pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
    }

    else
      dll_log->LogEx (false, L"Failure! (No Match Found)\n");
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
    dll_log->LogEx ( true,
      L" <> GetDesc_Override: Looking for matching NVAPI GPU for %s...: ",
      pDesc->Description );

    DXGI_ADAPTER_DESC* match =
      sk::NVAPI::FindGPUByDXGIName (pDesc->Description);

    if (match != nullptr)
    {
      dll_log->LogEx (false, L"Success! (%s)\n", match->Description);
      pDesc->DedicatedVideoMemory = match->DedicatedVideoMemory;
    }

    else
    {
      dll_log->LogEx (false, L"Failure! (No Match Found)\n");
    }
  }

  if (config.system.log_level >= 1)
  {
    dll_log->Log ( L"[   DXGI   ] Dedicated Video: %zu MiB, Dedicated System: %zu MiB, "
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
  int iver =
    SK_GetDXGIAdapterInterfaceVer (*ppAdapter);

  DXGI_ADAPTER_DESC desc = { };

  switch (iver)
  {
    default:
    case 2:
    {
      if ((! GetDesc2_Original) && ppAdapter != nullptr &&
                                  *ppAdapter != nullptr)
      {
        SK_ComQIPtr <IDXGIAdapter2> pAdapter2 (*ppAdapter);

        if (pAdapter2 != nullptr)
        {
          DXGI_VIRTUAL_HOOK (ppAdapter, 11, "(*pAdapter2)->GetDesc2",
            GetDesc2_Override, GetDesc2_Original, GetDesc2_pfn);
        }
      }
    }

    case 1:
    {
      if ((! GetDesc1_Original) && ppAdapter != nullptr &&
                                  *ppAdapter != nullptr)
      {
        SK_ComQIPtr <IDXGIAdapter1> pAdapter1 (*ppAdapter);

        if (pAdapter1 != nullptr)
        {
          DXGI_VIRTUAL_HOOK (&pAdapter1.p, 10, "(*pAdapter1)->GetDesc1",
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

      if (GetDesc_Original && ppAdapter != nullptr &&
                             *ppAdapter != nullptr)
          GetDesc_Original ( *ppAdapter, &desc );
    }
  }

  // Logic to skip Intel and Microsoft adapters and return only AMD / NV
  if (! lstrlenW (desc.Description))
    dll_log->LogEx (false, L" >> Assertion filed: Zero-length adapter name!\n");

#ifdef SKIP_INTEL
  if ((desc.VendorId == Microsoft || desc.VendorId == Intel) && Adapter == 0) {
#else
  if (false)
  {
#endif
    // We need to release the reference we were just handed before
    //   skipping it.
    (*ppAdapter)->Release ();

    dll_log->LogEx (false,
      L"[   DXGI   ] >> (Host Application Tried To Enum Intel or Microsoft Adapter "
      L"as Adapter 0) -- Skipping Adapter '%s' <<\n", desc.Description);

    return (pFunc (This, Adapter + 1, ppAdapter));
  }

  dll_log->LogEx (true, L"[   DXGI   ]  @ Returned Adapter %lu: '%32s' (LUID: %08X:%08X)",
    Adapter,
      desc.Description,
        desc.AdapterLuid.HighPart,
          desc.AdapterLuid.LowPart );

  //
  // Windows 8 has a software implementation, which we can detect.
  //
  SK_ComQIPtr <IDXGIAdapter1> pAdapter1 (*ppAdapter);
  // XXX

  if (pAdapter1 != nullptr)
  {
    DXGI_ADAPTER_DESC1 desc1 = { };

    if (            GetDesc1_Original != nullptr &&
         SUCCEEDED (GetDesc1_Original (pAdapter1, &desc1)) )
    {
#define DXGI_ADAPTER_FLAG_REMOTE   0x1
#define DXGI_ADAPTER_FLAG_SOFTWARE 0x2
      if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        dll_log->LogEx (false, L" <Software>");
      else
        dll_log->LogEx (false, L" <Hardware>");
      if (desc1.Flags & DXGI_ADAPTER_FLAG_REMOTE)
        dll_log->LogEx (false, L" [Remote]");
    }
  }

  dll_log->LogEx (false, L"\n");

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

    if (SUCCEEDED (EnumAdapters1_Original (This, SK_DXGI_preferred_adapter, &pAdapter1)))
    {
      dll_log->Log ( L"[   DXGI   ] (Reported values reflect user override: DXGI Adapter %lu -> %lu)",
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
      dll_log->Log ( L"[   DXGI   ] (Reported values reflect user override: DXGI Adapter %lu -> %lu)",
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
                                   EnumAdapters_Original );
  }

  return ret;
}

HMODULE
SK_D3D11_GetSystemDLL (void)
{
  static HMODULE
      hModSystemD3D11  = nullptr;
  if (hModSystemD3D11 == nullptr)
  {
    wchar_t    wszPath [MAX_PATH + 2] = { };
    wcsncpy_s (wszPath, MAX_PATH, SK_GetSystemDirectory (), _TRUNCATE);
    lstrcatW  (wszPath, LR"(\d3d11.dll)");

    hModSystemD3D11 =
      SK_Modules->LoadLibraryLL (wszPath);
  }

  return hModSystemD3D11;
}

HMODULE
SK_D3D11_GetLocalDLL (void)
{
  static HMODULE
      hModLocalD3D11  = nullptr;
  if (hModLocalD3D11 == nullptr)
  {
    hModLocalD3D11 =
      SK_Modules->LoadLibraryLL (L"d3d11.dll");
  }

  return hModLocalD3D11;
}

HRESULT
WINAPI CreateDXGIFactory (REFIID   riid,
                    _Out_ void   **ppFactory)
{
  std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
  int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

  UNREFERENCED_PARAMETER (iver);

  DXGI_LOG_CALL_2 ( L"                    CreateDXGIFactory        ",
                    L"%s, %08" PRIxPTR L"h",
                      iname.c_str (), (uintptr_t)ppFactory );

  if (CreateDXGIFactory_Import == nullptr)
  {
    SK_RunOnce (SK_BootDXGI ())
            WaitForInitDXGI ();
  }

  HRESULT ret =
    DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;

  if (CreateDXGIFactory_Import != nullptr)
  {
    DXGI_CALL (ret, CreateDXGIFactory_Import (riid, ppFactory));
  }

  else SK_ReleaseAssert (CreateDXGIFactory_Import != nullptr);

  if (SUCCEEDED (ret))
    SK_DXGI_LazyHookFactory ((IDXGIFactory *)*ppFactory);


  return ret;
}

HRESULT
WINAPI CreateDXGIFactory1 (REFIID   riid,
                     _Out_ void   **ppFactory)

{
  std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
  int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

  UNREFERENCED_PARAMETER (iver);

  DXGI_LOG_CALL_2 ( L"                    CreateDXGIFactory1       ",
                    L"%s, %08" PRIxPTR L"h",
                      iname.c_str (), (uintptr_t)ppFactory );

  if (CreateDXGIFactory1_Import == nullptr)
  {
    SK_RunOnce (SK_BootDXGI ());
            WaitForInitDXGI ();
  }

  // Windows Vista does not have this function -- wrap it with CreateDXGIFactory
  if (CreateDXGIFactory1_Import == nullptr)
  {
    dll_log->Log ( L"[   DXGI   ] >> Falling back to CreateDXGIFactory on "
                   L"Vista..." );

    return
      CreateDXGIFactory (riid, ppFactory);
  }

  HRESULT    ret;
  DXGI_CALL (ret, CreateDXGIFactory1_Import (riid, ppFactory));

  if (SUCCEEDED (ret))
    SK_DXGI_LazyHookFactory ((IDXGIFactory *)*ppFactory);

  return     ret;
}

HRESULT
WINAPI CreateDXGIFactory2 (UINT     Flags,
                           REFIID   riid,
                     _Out_ void   **ppFactory)
{
  std::wstring iname = SK_GetDXGIFactoryInterfaceEx  (riid);
  int          iver  = SK_GetDXGIFactoryInterfaceVer (riid);

  UNREFERENCED_PARAMETER (iver);

  DXGI_LOG_CALL_3 ( L"                    CreateDXGIFactory2       ",
                    L"0x%04X, %s, %08" PRIxPTR L"h",
                      Flags, iname.c_str (), (uintptr_t)ppFactory );

  if (CreateDXGIFactory2_Import == nullptr)
  {
    SK_RunOnce (SK_BootDXGI ())
            WaitForInitDXGI ();
  }

  // Windows 7 does not have this function -- wrap it with CreateDXGIFactory1
  if (CreateDXGIFactory2_Import == nullptr)
  {
    dll_log->Log ( L"[   DXGI   ] >> Falling back to CreateDXGIFactory1 on"
                   L" Vista/7..." );

    return
      CreateDXGIFactory1 (riid, ppFactory);
  }

  HRESULT    ret;
  DXGI_CALL (ret, CreateDXGIFactory2_Import (Flags, riid, ppFactory));

  if (SUCCEEDED (ret))
    SK_DXGI_LazyHookFactory ((IDXGIFactory *)*ppFactory);

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

  if (! (config.apis.dxgi.d3d11.hook ||
         config.apis.dxgi.d3d12.hook))
  {
    return;
  }

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchangeAcquire (&hooked, TRUE, FALSE))
  {
////#ifdef _WIN64
////    if (! config.apis.dxgi.d3d11.hook)
////      config.apis.dxgi.d3d12.hook = false;
////#endif

    // Serves as both D3D11 and DXGI
    bool d3d11 =
      ( SK_GetDLLRole () & DLL_ROLE::D3D11 );


    HMODULE hBackend =
      ( (SK_GetDLLRole () & DLL_ROLE::DXGI) && (! d3d11) ) ?
             backend_dll : SK_Modules->LoadLibraryLL (L"dxgi.dll");


    dll_log->Log (L"[   DXGI   ] Importing CreateDXGIFactory{1|2}");
    dll_log->Log (L"[   DXGI   ] ================================");

    if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dxgi.dll"))
    {
      CreateDXGIFactory_Import  =  (CreateDXGIFactory_pfn)
        SK_GetProcAddress  (hBackend, "CreateDXGIFactory");
      CreateDXGIFactory1_Import =  (CreateDXGIFactory1_pfn)
        SK_GetProcAddress (hBackend,  "CreateDXGIFactory1");
      CreateDXGIFactory2_Import =  (CreateDXGIFactory2_pfn)
        SK_GetProcAddress (hBackend,  "CreateDXGIFactory2");

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

      if ( (! LocalHook_CreateDXGIFactory.active) && SK_GetProcAddress (
             hBackend, "CreateDXGIFactory" ) )
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

      if ( (! LocalHook_CreateDXGIFactory1.active) && SK_GetProcAddress (
             hBackend, "CreateDXGIFactory1" ) )
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

      if ( (! LocalHook_CreateDXGIFactory2.active) && SK_GetProcAddress (
             hBackend, "CreateDXGIFactory2" ) )
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

    SK_ApplyQueuedHooks ();

    if (config.apis.dxgi.d3d11.hook)
    {
      SK_D3D11_InitTextures ();
      SK_D3D11_Init         ();
    }

    SK_ICommandProcessor* pCommandProc =
      SK_GetCommandProcessor ();


    pCommandProc->AddVariable ( "SwapChainWait",
            new SK_IVarStub <int> (
              &config.render.framerate.swapchain_wait )
                              );
    pCommandProc->AddVariable ( "PresentationInterval",
            new SK_IVarStub <int> (
              &config.render.framerate.present_interval )
                              );
    pCommandProc->AddVariable ( "PreRenderLimit",
            new SK_IVarStub <int> (
              &config.render.framerate.pre_render_limit )
                              );
    pCommandProc->AddVariable ( "BufferCount",
            new SK_IVarStub <int> (
              &config.render.framerate.buffer_count )
                              );
    pCommandProc->AddVariable ( "UseFlipDiscard",
            new SK_IVarStub <bool> (
              &config.render.framerate.flip_discard )
                              );

    SK_DXGI_BeginHooking ();

    InterlockedIncrementRelease (&hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}

void
WINAPI
dxgi_init_callback (finish_pfn finish)
{
  if (! SK_IsHostAppSKIM ())
  {
    SK_BootDXGI     ();
    //WaitForInitDXGI ();
  }

  __HrLoadAllImportsForDll ("d3dx11_43.dll");

  finish ();
}

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

HRESULT
WINAPI
IDXGISwapChain4_SetHDRMetaData ( IDXGISwapChain4*        This,
                        _In_     DXGI_HDR_METADATA_TYPE  Type,
                        _In_     UINT                    Size,
                        _In_opt_ void                   *pMetaData )
{
  SK_LOG_FIRST_CALL

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  rb.framebuffer_flags &= ~SK_FRAMEBUFFER_FLAG_HDR;

  HRESULT hr =
    IDXGISwapChain4_SetHDRMetaData_Original (This, Type, Size, pMetaData);

  //SK_HDR_GetControl ()->meta._AdjustmentCount++;

  if (SUCCEEDED (hr) && Type == DXGI_HDR_METADATA_TYPE_HDR10)
  {
    if (Size == sizeof (DXGI_HDR_METADATA_HDR10))
    {
      rb.framebuffer_flags |= SK_FRAMEBUFFER_FLAG_HDR;
    }
  }

  return hr;
}


const wchar_t*
DXGIColorSpaceToStr (DXGI_COLOR_SPACE_TYPE space) noexcept
{
  switch (space)
  {
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
      return L"DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709";
    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
      return L"DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709:
      return L"DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020:
      return L"DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020";
    case DXGI_COLOR_SPACE_RESERVED:
      return L"DXGI_COLOR_SPACE_RESERVED";
    case DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601:
      return L"DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601";
    case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601:
      return L"DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709";
    case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709:
      return L"DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020";
    case DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020:
      return L"DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020";
    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
      return L"DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020:
      return L"DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020";
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:
      return L"DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020";
    case DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020:
      return L"DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709:
      return L"DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020:
      return L"DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020:
      return L"DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020";
    case DXGI_COLOR_SPACE_CUSTOM:
      return L"DXGI_COLOR_SPACE_CUSTOM";
    default:
      return L"Unknown?!";
  };
};

HRESULT
WINAPI
IDXGISwapChain3_CheckColorSpaceSupport_Override (
  IDXGISwapChain3       *This,
  DXGI_COLOR_SPACE_TYPE  ColorSpace,
  UINT                  *pColorSpaceSupported )
{
  //SK_LOG0 ( ( "[!] IDXGISwapChain3::CheckColorSpaceSupport (%s) ",
  //              DXGIColorSpaceToStr (ColorSpace) ),
  //            L"   DXGI   " );

  //if ( (INT)ColorSpace < DXGI_COLOR_SPACE_CUSTOM ||
  //          ColorSpace > DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020 )
  //{
  //    SK_LOG0 ( ( L" >>> CRITICAL WARNING: Faulty software that does not handle HDR correctly < %s > "
  //                L"is trying to query support for a non-existent colorspace (idx: %u) !!",
  //                  SK_SummarizeCaller ().c_str (),
  //                  ColorSpace  ),
  //                L"HesDeadJim" );
  //}

  HRESULT hr =
    IDXGISwapChain3_CheckColorSpaceSupport_Original (
      This,
        ColorSpace,
          pColorSpaceSupported
    );

  return hr;
}

HRESULT
WINAPI
IDXGISwapChain3_SetColorSpace1_Override (
  IDXGISwapChain3       *This,
  DXGI_COLOR_SPACE_TYPE  ColorSpace )
{
  SK_LOG0 ( ( "[!] IDXGISwapChain3::SetColorSpace1 (%s)",
                DXGIColorSpaceToStr (ColorSpace) ),
              L"   DXGI   " );

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if ( rb.scanout.colorspace_override != DXGI_COLOR_SPACE_CUSTOM &&
                           ColorSpace != rb.scanout.colorspace_override )
  {
    SK_LOG0 ((L" >> HDR: Overriding Original Color Space: '%s' with '%s'",
             DXGIColorSpaceToStr (ColorSpace),
             DXGIColorSpaceToStr ((DXGI_COLOR_SPACE_TYPE)
                                  rb.scanout.colorspace_override)),
             L"   DXGI   ");

    ColorSpace = (DXGI_COLOR_SPACE_TYPE)rb.scanout.colorspace_override;
  }

  HRESULT hr =
    IDXGISwapChain3_SetColorSpace1_Original (
      This, ColorSpace
    );

  SK_ComPtr <IDXGIOutput>     pOutput;
  This->GetContainingOutput (&pOutput);

  SK_ComQIPtr <IDXGIOutput6> pOutput6 (pOutput);

  if (pOutput6 != nullptr)
  {
    DXGI_OUTPUT_DESC1    out_desc1 = { };
    pOutput6->GetDesc1 (&out_desc1);

    ////dll_log->Log ( L"HR=%x, Containing Output \"%s\" is %s to the Desktop",
    ////             hr, rb.display_name,
    ////             out_desc1.AttachedToDesktop ? L"attached" :
    ////             L"not attached" );

    rb.scanout.dwm_colorspace =
      out_desc1.ColorSpace;
  }

  if (SUCCEEDED (hr))
  {
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC full_desc = { };
          This->GetFullscreenDesc (&full_desc);

    if (full_desc.Windowed)
    {
      rb.scanout.dxgi_colorspace =
        rb.scanout.dwm_colorspace;
    }

    else
      rb.scanout.dxgi_colorspace = ColorSpace;
  }

  else
  {
    if (pOutput6 != nullptr)
    {
      DXGI_OUTPUT_DESC1    out_desc1 = { };
      pOutput6->GetDesc1 (&out_desc1);

      dll_log->Log ( L"HR=%x, Containing Output \"%s\" is %s to the Desktop"
                     L"\t\t[ColorSpace May be Wrong]",
                       hr, rb.display_name,
                       out_desc1.AttachedToDesktop ? L"attached" :
                                                     L"not attached" );

      rb.scanout.dxgi_colorspace =
        out_desc1.ColorSpace;
    }
  }

  return hr;
}

using IDXGIOutput6_GetDesc1_pfn = HRESULT (WINAPI *)
     (IDXGIOutput6*,              DXGI_OUTPUT_DESC1*);
      IDXGIOutput6_GetDesc1_pfn
      IDXGIOutput6_GetDesc1_Original = nullptr;


extern glm::vec3 SK_Color_XYZ_from_RGB (
  const SK_ColorSpace& cs, glm::vec3 RGB
);

HRESULT
WINAPI
IDXGIOutput6_GetDesc1_Override ( IDXGIOutput6      *This,
                           _Out_ DXGI_OUTPUT_DESC1 *pDesc )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    IDXGIOutput6_GetDesc1_Original (This, pDesc);

  if (SUCCEEDED (hr))
  {
    SK_RunOnce (
      dll_log->LogEx ( true,
        L"[Swap Chain]\n"
        L"  +-----------------+---------------------\n"
        L"  | Bits Per Color. |  %u\n"
        L"  | Color Space.... |  %s\n"
        L"  | Red Primary.... |  %f, %f\n"
        L"  | Green Primary.. |  %f, %f\n"
        L"  | Blue Primary... |  %f, %f\n"
        L"  | White Point.... |  %f, %f\n"
        L"  | Min Luminance.. |  %f\n"
        L"  | Max Luminance.. |  %f\n"
        L"  |  \"  FullFrame.. |  %f\n"
        L"  +-----------------+---------------------\n",
          pDesc->BitsPerColor,
          DXGIColorSpaceToStr (pDesc->ColorSpace),
          pDesc->RedPrimary   [0], pDesc->RedPrimary   [1],
          pDesc->GreenPrimary [0], pDesc->GreenPrimary [1],
          pDesc->BluePrimary  [0], pDesc->BluePrimary  [1],
          pDesc->WhitePoint   [0], pDesc->WhitePoint   [1],
          pDesc->MinLuminance,     pDesc->MaxLuminance,
          pDesc->MaxFullFrameLuminance )
    );

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    rb.display_gamut.xr = pDesc->RedPrimary   [0];
    rb.display_gamut.yr = pDesc->RedPrimary   [1];
    rb.display_gamut.xg = pDesc->GreenPrimary [0];
    rb.display_gamut.yg = pDesc->GreenPrimary [1];
    rb.display_gamut.xb = pDesc->BluePrimary  [0];
    rb.display_gamut.yb = pDesc->BluePrimary  [1];
    rb.display_gamut.Xw = pDesc->WhitePoint   [0];
    rb.display_gamut.Yw = pDesc->WhitePoint   [1];
    rb.display_gamut.Zw = 1.0f - rb.display_gamut.Xw - rb.display_gamut.Yw;

    rb.display_gamut.minY      = pDesc->MinLuminance;
    rb.display_gamut.maxLocalY = pDesc->MaxLuminance;
    rb.display_gamut.maxY      = pDesc->MaxFullFrameLuminance;

    if ( (pDesc->ColorSpace   == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ||
          // This is pretty much never going to happen in windowed mode
          pDesc->ColorSpace   == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709     ) )
    ///if (pDesc->ColorSpace != DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
    {
      rb.setHDRCapable (true);
    }
  }

  return hr;
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

  SK_ComQIPtr <IDXGISwapChain1>
      pSwapChain1 (pSwapChain);
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

  if (! InterlockedCompareExchangeAcquire (&hooked, TRUE, FALSE))
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

    // 23 IsTemporaryMonoSupported
    // 24 GetRestrictToOutput
    // 25 SetBackgroundColor
    // 26 GetBackgroundColor
    // 27 SetRotation
    // 28 GetRotation
    // 29 SetSourceSize
    // 30 GetSourceSize
    // 31 SetMaximumFrameLatency
    // 32 GetMaximumFrameLatency
    // 33 GetFrameLatencyWaitableObject
    // 34 SetMatrixTransform
    // 35 GetMatrixTransform

    // 36 GetCurrentBackBufferIndex
    // 37 CheckColorSpaceSupport
    // 38 SetColorSpace1
    // 39 ResizeBuffers1

    // 40 SetHDRMetaData

    SK_ComQIPtr <IDXGISwapChain3> pSwapChain3 (pSwapChain);

    if ( pSwapChain3                                 != nullptr &&
     IDXGISwapChain3_CheckColorSpaceSupport_Original == nullptr )
    {
      DXGI_VIRTUAL_HOOK ( &pSwapChain3.p, 37,
                          "IDXGISwapChain3::CheckColorSpaceSupport",
                           IDXGISwapChain3_CheckColorSpaceSupport_Override,
                           IDXGISwapChain3_CheckColorSpaceSupport_Original,
                           IDXGISwapChain3_CheckColorSpaceSupport_pfn );

      DXGI_VIRTUAL_HOOK ( &pSwapChain3.p, 38,
                          "IDXGISwapChain3::SetColorSpace1",
                           IDXGISwapChain3_SetColorSpace1_Override,
                           IDXGISwapChain3_SetColorSpace1_Original,
                           IDXGISwapChain3_SetColorSpace1_pfn );
    }

    SK_ComQIPtr <IDXGISwapChain4> pSwapChain4 (pSwapChain);
  //pSwapChain->QueryInterface <IDXGISwapChain4> (&pSwapChain4);

    if ( pSwapChain4                         != nullptr &&
     IDXGISwapChain4_SetHDRMetaData_Original == nullptr )
    {
      DXGI_VIRTUAL_HOOK ( &pSwapChain4.p, 40,
                       //&pSwapChain, 40,
                          "IDXGISwapChain4::SetHDRMetaData",
                           IDXGISwapChain4_SetHDRMetaData,
                           IDXGISwapChain4_SetHDRMetaData_Original,
                           IDXGISwapChain4_SetHDRMetaData_pfn );
    }

    SK_ComPtr                          <IDXGIOutput> pOutput;
    if (SUCCEEDED (pSwapChain->GetContainingOutput (&pOutput)))
    {
      if (pOutput != nullptr)
      {
        DXGI_VIRTUAL_HOOK ( &pOutput.p, 8,
                               "IDXGIOutput::GetDisplayModeList",
                                  DXGIOutput_GetDisplayModeList_Override,
                                             GetDisplayModeList_Original,
                                             GetDisplayModeList_pfn );

        DXGI_VIRTUAL_HOOK ( &pOutput.p, 9,
                               "IDXGIOutput::FindClosestMatchingMode",
                                  DXGIOutput_FindClosestMatchingMode_Override,
                                             FindClosestMatchingMode_Original,
                                             FindClosestMatchingMode_pfn );

        // Don't hook this unless you want nvspcap to crash the game.

        DXGI_VIRTUAL_HOOK ( &pOutput.p, 10,
                              "IDXGIOutput::WaitForVBlank",
                                 DXGIOutput_WaitForVBlank_Override,
                                            WaitForVBlank_Original,
                                            WaitForVBlank_pfn );

        // 11 TakeOwnership
        // 12 ReleaseOwnership
        // 13 GetGammaControlCapabilities
        // 14 SetGammaControl
        // 15 GetGammaControl
        // 16 SetDisplaySurface
        // 17 GetDisplaySurfaceData
        // 18 GetFrameStatistics
        // 19 GetDisplayModeList1
        // 20 FindClosestMatchingMode1
        // 21 GetDisplaySurfaceData1
        // 22 DuplicateOutput
        // 23 SupportsOverlays
        // 24 CheckOverlaySupport
        // 25 CheckOverlayColorSpaceSupport
        // 26 DuplicateOutput1
        // 27 GetDesc1

        SK_ComQIPtr <IDXGIOutput6>
            pOutput6 (pOutput);
        if (pOutput6 != nullptr)
        {
          DXGI_VIRTUAL_HOOK ( &pOutput6.p, 27,
                               "IDXGIOutput6::GetDesc1",
                                 IDXGIOutput6_GetDesc1_Override,
                                 IDXGIOutput6_GetDesc1_Original,
                                 IDXGIOutput6_GetDesc1_pfn );
        }
      }
    }

    InterlockedIncrementRelease (&hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}


void
SK_DXGI_HookFactory (IDXGIFactory* pFactory)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchangeAcquire (&hooked, TRUE, FALSE))
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

    SK_ComQIPtr <IDXGIFactory1> pFactory1 (pFactory);

    // 12 EnumAdapters1
    // 13 IsCurrent
    if (pFactory1 != nullptr)
    {
      DXGI_VIRTUAL_HOOK ( /*&pFactory1.p*/&pFactory,     12,
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
#if 1
      SK_ComQIPtr <IDXGIFactory2> pFactory2 (pFactory);
      if (pFactory2 != nullptr)
#else
      SK_ComPtr <IDXGIFactory2> pFactory2;
      if ( SUCCEEDED (CreateDXGIFactory1_Import (IID_IDXGIFactory2, (void **)&pFactory2.p)) )
#endif
      {
        if (! LocalHook_IDXGIFactory2_CreateSwapChainForHwnd.active)
        {
          DXGI_VIRTUAL_HOOK ( /*&pFactory2.p*/&pFactory, 15,
                              "IDXGIFactory2::CreateSwapChainForHwnd",
                               DXGIFactory2_CreateSwapChainForHwnd_Override,
                                            CreateSwapChainForHwnd_Original,
                                            CreateSwapChainForHwnd_pfn );

          SK_Hook_TargetFromVFTable (
            LocalHook_IDXGIFactory2_CreateSwapChainForHwnd,
              (void **)/*&pFactory2.p*/&pFactory, 15 );
        }

        if (! LocalHook_IDXGIFactory2_CreateSwapChainForCoreWindow.active)
        {
          DXGI_VIRTUAL_HOOK ( /*&pFactory2.p*/&pFactory, 16,
                              "IDXGIFactory2::CreateSwapChainForCoreWindow",
                               DXGIFactory2_CreateSwapChainForCoreWindow_Override,
                                            CreateSwapChainForCoreWindow_Original,
                                            CreateSwapChainForCoreWindow_pfn );
          SK_Hook_TargetFromVFTable (
            LocalHook_IDXGIFactory2_CreateSwapChainForCoreWindow,
              (void **)/*&pFactory2.p*/&pFactory, 16 );
        }

        if (! LocalHook_IDXGIFactory2_CreateSwapChainForComposition.active)
        {
          DXGI_VIRTUAL_HOOK ( /*&pFactory2.p*/&pFactory, 24,
                              "IDXGIFactory2::CreateSwapChainForComposition",
                               DXGIFactory2_CreateSwapChainForComposition_Override,
                                            CreateSwapChainForComposition_Original,
                                            CreateSwapChainForComposition_pfn );

          SK_Hook_TargetFromVFTable (
            LocalHook_IDXGIFactory2_CreateSwapChainForComposition,
              (void **)/*&pFactory2.p*/&pFactory, 24 );
        }
      }
    }


    // DXGI 1.3+
    SK_ComPtr <IDXGIFactory3> pFactory3;

    // 25 GetCreationFlags


    // DXGI 1.4+
    SK_ComPtr <IDXGIFactory4> pFactory4;

    // 26 EnumAdapterByLuid
    // 27 EnumWarpAdapter


    // DXGI 1.5+
    SK_ComPtr <IDXGIFactory5> pFactory5;

    // 28 CheckFeatureSupport

    InterlockedIncrementRelease (&hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}


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
  SetCurrentThreadDescription (L"[SK] DXGI Hook Crawler");

  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
  {
    SK_Thread_CloseSelf ();
    return 0;
  }


  UNREFERENCED_PARAMETER (user);

  if (! (config.apis.dxgi.d3d11.hook ||
         config.apis.dxgi.d3d12.hook) )
  {
    SK_Thread_CloseSelf ();
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
      dll_log->Log (L"[  D3D 11  ]  >> Implicit Initialization Triggered <<");
      SK_BootDXGI ();
    }

    while (CreateDXGIFactory_Import == nullptr)
      MsgWaitForMultipleObjectsEx (0, nullptr, 33, QS_ALLEVENTS, MWMO_INPUTAVAILABLE);

    // TODO: Handle situation where CreateDXGIFactory is unloadable
  }

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if ( __SK_bypass     || ReadAcquire (&__dxgi_ready) ||
       pTLS == nullptr || pTLS->d3d11->ctx_init_thread )
  {
    SK_Thread_CloseSelf ();
    return 0;
  }


  static volatile LONG __hooked = FALSE;

  if (! InterlockedCompareExchangeAcquire (&__hooked, TRUE, FALSE))
  {
    pTLS->d3d11->ctx_init_thread = true;

    SK_AutoCOMInit auto_com;

    SK_D3D11_Init ();

    if (D3D11CreateDeviceAndSwapChain_Import == nullptr)
    {
      pTLS->d3d11->ctx_init_thread = false;

      SK_ApplyQueuedHooks ();
      return 0;
    }

    dll_log->Log (L"[   DXGI   ]   Installing DXGI Hooks");

    D3D_FEATURE_LEVEL            levels [] = { D3D_FEATURE_LEVEL_11_0 };

    D3D_FEATURE_LEVEL            featureLevel;
    SK_ComPtr <ID3D11Device>        pDevice           = nullptr;
    SK_ComPtr <ID3D11DeviceContext> pImmediateContext = nullptr;
//    ID3D11DeviceContext           *pDeferredContext  = nullptr;

    // DXGI stuff is ready at this point, we'll hook the swapchain stuff
    //   after this call.

    HRESULT hr = E_NOTIMPL;

    SK_ComPtr <IDXGISwapChain> pSwapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC       desc       = { };

    desc.BufferDesc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count            = 1;
    desc.SampleDesc.Quality          = 0;
    desc.BufferDesc.Width            = 2;
    desc.BufferDesc.Height           = 2;
    desc.BufferUsage                 = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount                 = 1;
    desc.OutputWindow                = SK_Win32_CreateDummyWindow ();
    desc.Windowed                    = TRUE;
    desc.SwapEffect                  = DXGI_SWAP_EFFECT_DISCARD;

    extern LPVOID pfnD3D11CreateDeviceAndSwapChain;

    SK_COMPAT_UnloadFraps ();

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
                  _ARRAYSIZE(levels),
                    D3D11_SDK_VERSION, &desc,
                      &pSwapChain.p,
                        &pDevice.p,
                          &featureLevel,
                            &pImmediateContext.p );

    sk_hook_d3d11_t d3d11_hook_ctx = { };

    d3d11_hook_ctx.ppDevice           = &pDevice.p;
    d3d11_hook_ctx.ppImmediateContext = &pImmediateContext.p;

    SK_ComPtr <IDXGIDevice>  pDevDXGI = nullptr;
    SK_ComPtr <IDXGIAdapter> pAdapter = nullptr;
    SK_ComPtr <IDXGIFactory> pFactory = nullptr;

    if ( pDevice != nullptr &&
         SUCCEEDED (pDevice->QueryInterface <IDXGIDevice> (&pDevDXGI)) &&
         SUCCEEDED (pDevDXGI->GetAdapter                  (&pAdapter)) &&
         SUCCEEDED (pAdapter->GetParent     (IID_PPV_ARGS (&pFactory))) )
    {
      //if (config.render.dxgi.deferred_isolation)
      //{
        //    pDevice->CreateDeferredContext (0x0, &pDeferredContext);
        //d3d11_hook_ctx.ppImmediateContext = &pDeferredContext;
      //}

      HookD3D11             (&d3d11_hook_ctx);
      SK_DXGI_HookFactory   (pFactory);
      //if (SUCCEEDED (pFactory->CreateSwapChain (pDevice, &desc, &pSwapChain)))
      SK_DXGI_HookSwapChain (pSwapChain);

      // This won't catch Present1 (...), but no games use that
      //   and we can deal with it later if it happens.
      SK_DXGI_HookPresentBase ((IDXGISwapChain *)pSwapChain);

      SK_ComQIPtr <IDXGISwapChain1> pSwapChain1 (pSwapChain);

      if (pSwapChain1 != nullptr)
        SK_DXGI_HookPresent1 (pSwapChain1);

      SK_ApplyQueuedHooks ();


      extern volatile LONG          SK_D3D11_initialized;
      InterlockedIncrementRelease (&SK_D3D11_initialized);

      if (config.apis.dxgi.d3d11.hook) SK_D3D11_EnableHooks ();

/////#ifdef _WIN64
/////      if (config.apis.dxgi.d3d12.hook) SK_D3D12_EnableHooks ();
/////#endif

      WriteRelease (&__dxgi_ready, TRUE);
    }

    else
    {
      _com_error err (hr);

      std::wstring err_desc (err.Description ());
      std::wstring err_src  (err.Source      ());

      dll_log->Log (L"[   DXGI   ] Unable to hook D3D11?! HRESULT=%x ('%s')",
                                err.Error (), err.ErrorMessage () );
      dll_log->Log (L"[   DXGI   ]  >> %s, in %s",
                               err_desc.c_str (), err_src.c_str () );
    }

    SK_Win32_CleanupDummyWindow (desc.OutputWindow);

    InterlockedIncrementRelease (&__hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&__hooked, 2);

  SK_Thread_CloseSelf ();

  return 0;
}


bool
SK::DXGI::Shutdown (void)
{
  static const auto&
    game_id = SK_GetCurrentGameID ();

#ifdef _WIN64
  if (game_id == SK_GAME_ID::DarkSouls3)
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

    iSK_INI* ini =
      SK_GetDLLConfig ();

    if (ini != nullptr)
      ini->write (ini->get_filename ());
  }


  if (config.apis.dxgi.d3d11.hook) SK_D3D11_Shutdown ();

////#ifdef _WIN64
////  if (config.apis.dxgi.d3d12.hook) SK_D3D12_Shutdown ();
////#endif

  if (StrStrIW (SK_GetBackend (), L"d3d11"))
    return SK_ShutdownCore (L"d3d11");

  return SK_ShutdownCore (L"dxgi");
}



void
WINAPI
SK_DXGI_SetPreferredAdapter (int override_id) noexcept
{
  SK_DXGI_preferred_adapter = override_id;
}

//extern bool SK_D3D11_need_tex_reset;

memory_stats_t   dxgi_mem_stats [MAX_GPU_NODES] = { };
mem_info_t       dxgi_mem_info  [NumBuffers]    = { };

struct budget_thread_params_t
{
           IDXGIAdapter3 *pAdapter = nullptr;
           HANDLE         handle   = INVALID_HANDLE_VALUE;
           HANDLE         event    = INVALID_HANDLE_VALUE;
           HANDLE         manual   = INVALID_HANDLE_VALUE;
           HANDLE         shutdown = INVALID_HANDLE_VALUE;
           DWORD          tid      = 0UL;
           DWORD          cookie   = 0UL;
  volatile LONG           ready    = FALSE;
};

SK_LazyGlobal <budget_thread_params_t> budget_thread;

void
SK_DXGI_SignalBudgetThread (void)
{
  if (budget_thread->manual != INVALID_HANDLE_VALUE)
    SetEvent (budget_thread->manual);
}

bool
WINAPI
SK_DXGI_IsTrackingBudget (void) noexcept
{
  return
    ( budget_thread->tid != 0UL );
}


HRESULT
SK::DXGI::StartBudgetThread ( IDXGIAdapter** ppAdapter )
{
  if (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0)
    return E_ACCESSDENIED;

  ////////if (config.apis.dxgi.d3d12.hook)
  ////////  return E_ACCESSDENIED;

  //
  // If the adapter implements DXGI 1.4, then create a budget monitoring
  //  thread...
  //
  IDXGIAdapter3* pAdapter3 = nullptr;
  HRESULT        hr        = E_NOTIMPL;

  if (SUCCEEDED ((*ppAdapter)->QueryInterface <IDXGIAdapter3> (&pAdapter3)) && pAdapter3 != nullptr)
  {
    // We darn sure better not spawn multiple threads!
    std::scoped_lock <SK_Thread_CriticalSection> auto_lock (*budget_mutex);

    if ( budget_thread->handle == INVALID_HANDLE_VALUE )
    {
      // We're going to Release this interface after thread spawnning, but
      //   the running thread still needs a reference counted.
      pAdapter3->AddRef ();

      SecureZeroMemory ( budget_thread.getPtr (),
                     sizeof budget_thread_params_t );

      dll_log->LogEx ( true,
                         L"[ DXGI 1.4 ]   "
                         L"$ Spawning Memory Budget Change Thread..: " );

      WriteRelease ( &budget_thread->ready,
                       FALSE );

      budget_thread->pAdapter = pAdapter3;
      budget_thread->tid      = 0;
      budget_log->silent      = true;
      budget_thread->event    =
        SK_CreateEvent ( nullptr,
                           FALSE,
                             TRUE, nullptr );
                               //L"DXGIMemoryBudget" );
      budget_thread->manual   =
        SK_CreateEvent ( nullptr,
                           FALSE,
                             TRUE, nullptr );
                               //L"DXGIMemoryWakeUp!" );
      budget_thread->shutdown =
        SK_CreateEvent ( nullptr,
                           TRUE,
                             FALSE, nullptr );
                               //L"DXGIMemoryBudget_Shutdown" );

      budget_thread->handle =
        SK_Thread_CreateEx
          ( BudgetThread,
              nullptr,
                (LPVOID)budget_thread.getPtr () );


      SK_Thread_SpinUntilFlagged (&budget_thread->ready);


      if ( budget_thread->tid != 0 )
      {
        dll_log->LogEx ( false,
                           L"tid=0x%04x\n",
                             budget_thread->tid );

        dll_log->LogEx ( true,
                           L"[ DXGI 1.4 ]   "
                             L"%% Setting up Budget Change Notification.: " );

        HRESULT result =
          pAdapter3->RegisterVideoMemoryBudgetChangeNotificationEvent (
                            budget_thread->event,
                           &budget_thread->cookie
          );

        if ( SUCCEEDED ( result ) )
        {
          dll_log->LogEx ( false,
                             L"eid=0x%08" PRIxPTR L", cookie=%u\n",
                               (uintptr_t)budget_thread->event,
                                          budget_thread->cookie );

          hr = S_OK;
        }

        else
        {
          dll_log->LogEx ( false,
                             L"Failed! (%s)\n",
                               SK_DescribeHRESULT ( result ) );

          hr = result;
        }
      }

      else
      {
        dll_log->LogEx (false, L"failed!\n");

        hr = E_FAIL;
      }

      dll_log->LogEx ( true,
                         L"[ DXGI 1.2 ] GPU Scheduling...:"
                                      L" Pre-Emptive" );

      DXGI_QUERY_VIDEO_MEMORY_INFO
              _mem_info = { };
      DXGI_ADAPTER_DESC2
                  desc2 = { };

      int      i      = 0;
      bool     silent = dll_log->silent;
      dll_log->silent = true;
      {
        // Don't log this call, because that would be silly...
        pAdapter3->GetDesc2 ( &desc2 );
      }
      dll_log->silent = silent;


      switch ( desc2.GraphicsPreemptionGranularity )
      {
        case DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY:
          dll_log->LogEx ( false, L" (DMA Buffer)\n"         );
          break;

        case DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY:
          dll_log->LogEx ( false, L" (Graphics Primitive)\n" );
          break;

        case DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY:
          dll_log->LogEx ( false, L" (Triangle)\n"           );
          break;

        case DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY:
          dll_log->LogEx ( false, L" (Fragment)\n"           );
          break;

        case DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY:
          dll_log->LogEx ( false, L" (Instruction)\n"        );
          break;

        default:
          dll_log->LogEx (false, L"UNDEFINED\n");
          break;
      }

      dll_log->LogEx ( true,
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
          dll_log->LogEx ( false, L"\n"                              );
          dll_log->LogEx ( true,  L"                               " );
        }

        dll_log->LogEx ( false,
                           L" Node%i       (Reserve: %#5llu / %#5llu MiB - "
                                          L"Budget: %#5llu / %#5llu MiB)",
                             i++,
                               _mem_info.CurrentReservation      >> 20ULL,
                               _mem_info.AvailableForReservation >> 20ULL,
                               _mem_info.CurrentUsage            >> 20ULL,
                               _mem_info.Budget                  >> 20ULL
                       );

        if (config.mem.reserve > 0.0f)
        {
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
      }

      i = 0;

      dll_log->LogEx ( false, L"\n"                              );
      dll_log->LogEx ( true,  L"[ DXGI 1.4 ] Non-Local Memory.:" );

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
          dll_log->LogEx ( false, L"\n"                              );
          dll_log->LogEx ( true,  L"                               " );
        }

        dll_log->LogEx ( false,
                           L" Node%i       (Reserve: %#5llu / %#5llu MiB - "
                                           L"Budget: %#5llu / %#5llu MiB)",
                             i++,
                               _mem_info.CurrentReservation      >> 20ULL,
                               _mem_info.AvailableForReservation >> 20ULL,
                               _mem_info.CurrentUsage            >> 20ULL,
                               _mem_info.Budget                  >> 20ULL
                       );

        if (config.mem.reserve > .0f)
        {
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
      }

      dxgi_mem_info [0].nodes = ( i - 1 );
      dxgi_mem_info [1].nodes = ( i - 1 );

      dll_log->LogEx ( false, L"\n" );
    }
  }

  return hr;
}

#define min_max(ref,min,max) if ((ref) > (max)) (max) = (ref); \
                             if ((ref) < (min)) (min) = (ref);

const uint32_t
  BUDGET_POLL_INTERVAL = 133UL; // How often to sample the budget
                                //  in msecs

DWORD
WINAPI
SK::DXGI::BudgetThread ( LPVOID user_data )
{
  SetCurrentThreadDescription (L"[SK] DXGI Budget Tracker");

  auto* params =
    static_cast <budget_thread_params_t *> (user_data);

  budget_log->silent = true;
  params->tid        = SK_Thread_GetCurrentId ();

  InterlockedExchangeAcquire ( &params->ready, TRUE );


  SK_AutoCOMInit auto_com;


  SetThreadPriority      ( SK_GetCurrentThread (), THREAD_PRIORITY_LOWEST );
  SetThreadPriorityBoost ( SK_GetCurrentThread (), TRUE                   );

  DWORD dwWaitStatus = DWORD_MAX;

  while ( ReadAcquire ( &params->ready ) )
  {
    if (ReadAcquire (&__SK_DLL_Ending))
      break;

    if ( params->event == nullptr )
      break;

    HANDLE phEvents [] = {
      params->event, params->shutdown,
      params->manual // Update memory totals, but not due to a budget change
    };

    dwWaitStatus =
      WaitForMultipleObjects ( 3,
                                 phEvents,
                                   FALSE,
                                     INFINITE );

    if (! ReadAcquire ( &params->ready ) )
    {
      break;
    }

    if (dwWaitStatus == WAIT_OBJECT_0 + 1)
    {
      WriteRelease ( &params->ready, FALSE );
      ResetEvent   (  params->shutdown     );
      break;
    }


    //static DWORD dwLastEvict = 0;
    //static DWORD dwLastTest  = 0;
    //static DWORD dwLastSize  = SK_D3D11_Textures.Entries_2D.load ();
    //
    //DWORD dwNow = timeGetTime ();
    //
    //if ( ( SK_D3D11_Textures.Evicted_2D.load () != dwLastEvict ||
    //       SK_D3D11_Textures.Entries_2D.load () != dwLastSize     ) &&
    //                                        dwLastTest < dwNow - 750UL )
    ////if (ImGui::Button ("Compute Residency Statistics"))
    //{
    //  dwLastTest  = dwNow;
    //  dwLastEvict = SK_D3D11_Textures.Evicted_2D.load ();
    //  dwLastSize  = SK_D3D11_Textures.Entries_2D.load ();

    int         node = 0;

    buffer_t buffer  =
      dxgi_mem_info [node].buffer;


    SK_D3D11_Textures->Budget = dxgi_mem_info [buffer].local [0].Budget -
                                dxgi_mem_info [buffer].local [0].CurrentUsage;


    // Double-Buffer Updates
    if ( buffer == Front )
      buffer = Back;
    else
      buffer = Front;


    GetLocalTime ( &dxgi_mem_info [buffer].time );

    //
    // Sample Fast nUMA (On-GPU / Dedicated) Memory
    //
    for ( node = 0; node < MAX_GPU_NODES; )
    {
      int next_node =
               node + 1;

      if ( FAILED (
             params->pAdapter->QueryVideoMemoryInfo (
               node,
                 DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
                   &dxgi_mem_info [buffer].local [node]
             )
           )
         )
      {
        node = next_node;
        break;
      }

      node = next_node;
    }


    // Fix for AMD drivers, that don't allow querying non-local memory
    int nodes =
      std::max (0, node - 1);

    //
    // Sample Slow nUMA (Off-GPU / Shared) Memory
    //
    for ( node = 0; node < MAX_GPU_NODES; )
    {
      int next_node =
               node + 1;

      if ( FAILED (
             params->pAdapter->QueryVideoMemoryInfo (
               node,
                 DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
                   &dxgi_mem_info [buffer].nonlocal [node]
             )
           )
         )
      {
        node = next_node;
        break;
      }

      node = next_node;
    }


    // Set the number of SLI/CFX Nodes
    dxgi_mem_info [buffer].nodes = nodes;

    static uint64_t
      last_budget = dxgi_mem_info [buffer].local [0].Budget;


    if ( nodes > 0 )
    {
      int i;

      budget_log->LogEx ( true,
                            L"[ DXGI 1.4 ] Local Memory.....:" );

      for ( i = 0; i < nodes; i++ )
      {
        if ( dwWaitStatus == WAIT_OBJECT_0 )
        {
          static UINT64
            LastBudget = 0ULL;

          dxgi_mem_stats [i].budget_changes++;

          const int64_t over_budget =
            ( dxgi_mem_info [buffer].local [i].CurrentUsage -
              dxgi_mem_info [buffer].local [i].Budget );

            //LastBudget -
            //dxgi_mem_info [buffer].local [i].Budget;

          SK_D3D11_need_tex_reset = ( over_budget > 0 );

          LastBudget =
            dxgi_mem_info [buffer].local [i].Budget;
        }

        if ( i > 0 )
        {
          budget_log->LogEx ( false, L"\n"                                );
          budget_log->LogEx ( true,  L"                                 " );
        }

        budget_log->LogEx (
          false,
            L" Node%i (Reserve: %#5llu / %#5llu MiB - "
                      L"Budget: %#5llu / %#5llu MiB)",
          i,
            dxgi_mem_info       [buffer].local [i].CurrentReservation      >> 20ULL,
              dxgi_mem_info     [buffer].local [i].AvailableForReservation >> 20ULL,
                dxgi_mem_info   [buffer].local [i].CurrentUsage            >> 20ULL,
                  dxgi_mem_info [buffer].local [i].Budget                  >> 20ULL
        );

        min_max ( dxgi_mem_info [buffer].local [i].AvailableForReservation,
                                dxgi_mem_stats [i].min_avail_reserve,
                                dxgi_mem_stats [i].max_avail_reserve );

        min_max ( dxgi_mem_info [buffer].local [i].CurrentReservation,
                                dxgi_mem_stats [i].min_reserve,
                                dxgi_mem_stats [i].max_reserve );

        min_max ( dxgi_mem_info [buffer].local [i].CurrentUsage,
                                dxgi_mem_stats [i].min_usage,
                                dxgi_mem_stats [i].max_usage );

        min_max ( dxgi_mem_info [buffer].local [i].Budget,
                                dxgi_mem_stats [i].min_budget,
                                dxgi_mem_stats [i].max_budget );

        if ( dxgi_mem_info [buffer].local [i].CurrentUsage >
             dxgi_mem_info [buffer].local [i].Budget)
        {
          uint64_t over_budget =
             ( dxgi_mem_info [buffer].local [i].CurrentUsage -
               dxgi_mem_info [buffer].local [i].Budget );

          min_max ( over_budget,
                           dxgi_mem_stats [i].min_over_budget,
                           dxgi_mem_stats [i].max_over_budget );
        }
      }

      budget_log->LogEx ( false, L"\n"                              );
      budget_log->LogEx ( true,  L"[ DXGI 1.4 ] Non-Local Memory.:" );

      for ( i = 0; i < nodes; i++ )
      {
        if ( i > 0 )
        {
          budget_log->LogEx ( false, L"\n"                                );
          budget_log->LogEx ( true,  L"                                 " );
        }

        budget_log->LogEx (
          false,
            L" Node%i (Reserve: %#5llu / %#5llu MiB - "
                      L"Budget: %#5llu / %#5llu MiB)",    i,
         dxgi_mem_info    [buffer].nonlocal [i].CurrentReservation      >> 20ULL,
          dxgi_mem_info   [buffer].nonlocal [i].AvailableForReservation >> 20ULL,
           dxgi_mem_info  [buffer].nonlocal [i].CurrentUsage            >> 20ULL,
            dxgi_mem_info [buffer].nonlocal [i].Budget                  >> 20ULL );
      }

      budget_log->LogEx ( false, L"\n" );
    }

    dxgi_mem_info [0].buffer =
                      buffer;
  }


  return 0;
}

HRESULT
SK::DXGI::StartBudgetThread_NoAdapter (void)
{
  if (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0)
    return E_ACCESSDENIED;

  HRESULT hr = E_NOTIMPL;

  SK_AutoCOMInit auto_com;

  static HMODULE
    hDXGI = SK_Modules->LoadLibraryLL ( L"dxgi.dll" );

  if (hDXGI)
  {
    static auto
      CreateDXGIFactory2 =
        (CreateDXGIFactory2_pfn) SK_GetProcAddress ( hDXGI,
                                                       "CreateDXGIFactory2" );

    // If we somehow got here on Windows 7, get the hell out immediately!
    if (! CreateDXGIFactory2)
      return hr;

    SK_ComPtr <IDXGIFactory> factory = nullptr;
    SK_ComPtr <IDXGIAdapter> adapter = nullptr;

    // Only spawn the DXGI 1.4 budget thread if ... DXGI 1.4 is implemented.
    if ( SUCCEEDED (
           CreateDXGIFactory2 ( 0x0, IID_PPV_ARGS (&factory.p) )
         )
       )
    {
      if ( SUCCEEDED (
             factory->EnumAdapters ( 0, &adapter.p )
           )
         )
      {
        hr =
          StartBudgetThread ( &adapter.p );
      }
    }
  }

  return hr;
}

void
SK::DXGI::ShutdownBudgetThread ( void )
{
  SK_AutoClose_LogEx (budget_log, budget);

  if (                                     budget_thread->handle != INVALID_HANDLE_VALUE &&
       InterlockedCompareExchangeRelease (&budget_thread->ready, 0, 1) )
  {
    dll_log->LogEx (
      true,
        L"[ DXGI 1.4 ] Shutting down Memory Budget Change Thread... "
    );

    DWORD dwWaitState =
      SignalObjectAndWait ( budget_thread->shutdown,
                              budget_thread->handle, // Give 1 second, and
                                1000UL,              // then we're killing
                                  TRUE );            // the thing!


    if (budget_thread->shutdown != INVALID_HANDLE_VALUE)
    {
      CloseHandle (budget_thread->shutdown);
                   budget_thread->shutdown = INVALID_HANDLE_VALUE;
    }

    if ( dwWaitState == WAIT_OBJECT_0 )
    {
      dll_log->LogEx   ( false, L"done!\n"                         );
    }

    else
    {
      dll_log->LogEx  ( false, L"timed out (killing manually)!\n" );
      TerminateThread ( budget_thread->handle,                  0 );
    }

    if (budget_thread->handle != INVALID_HANDLE_VALUE)
    {
      CloseHandle ( budget_thread->handle );
                    budget_thread->handle = INVALID_HANDLE_VALUE;
    }

    // Record the final statistics always
    budget_log->silent    = false;

    budget_log->Log   ( L"--------------------"   );
    budget_log->Log   ( L"Shutdown Statistics:"   );
    budget_log->Log   ( L"--------------------\n" );

    // in %10u seconds\n",
    budget_log->Log ( L" Memory Budget Changed %llu times\n",
                        dxgi_mem_stats [0].budget_changes );

    for ( int i = 0; i < 4; i++ )
    {
      if ( dxgi_mem_stats [i].max_usage > 0 )
      {
        if ( dxgi_mem_stats [i].min_reserve     == UINT64_MAX )
             dxgi_mem_stats [i].min_reserve     =  0ULL;

        if ( dxgi_mem_stats [i].min_over_budget == UINT64_MAX )
             dxgi_mem_stats [i].min_over_budget =  0ULL;

        budget_log->LogEx ( true,
                             L" GPU%i: Min Budget:        %05llu MiB\n",
                                          i,
                               dxgi_mem_stats [i].min_budget >> 20ULL );
        budget_log->LogEx ( true,
                             L"       Max Budget:        %05llu MiB\n",
                               dxgi_mem_stats [i].max_budget >> 20ULL );

        budget_log->LogEx ( true,
                             L"       Min Usage:         %05llu MiB\n",
                               dxgi_mem_stats [i].min_usage  >> 20ULL );
        budget_log->LogEx ( true,
                             L"       Max Usage:         %05llu MiB\n",
                               dxgi_mem_stats [i].max_usage  >> 20ULL );

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

        budget_log->LogEx ( true,  L"------------------------------------\n" );
        budget_log->LogEx ( true,  L" Minimum Over Budget:     %05llu MiB\n",
                                dxgi_mem_stats [i].min_over_budget >> 20ULL  );
        budget_log->LogEx ( true,  L" Maximum Over Budget:     %05llu MiB\n",
                                dxgi_mem_stats [i].max_over_budget >> 20ULL  );
        budget_log->LogEx ( true,  L"------------------------------------\n" );
        budget_log->LogEx ( false, L"\n"                                     );
      }
    }
  }
}









bool
SK_D3D11_QuickHooked (void);

void
SK_DXGI_QuickHook (void)
{
  // We don't want to hook this, and we certainly don't want to hook it using
  //   cached addresses!
  if (! (config.apis.dxgi.d3d11.hook ||
         config.apis.dxgi.d3d12.hook))
    return;

  if (config.steam.preload_overlay)
    return;

  extern BOOL
      __SK_DisableQuickHook;
  if (__SK_DisableQuickHook)
    return;


  static volatile LONG quick_hooked = FALSE;

  if (! InterlockedCompareExchangeAcquire (&quick_hooked, TRUE, FALSE))
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
      SK_ApplyQueuedHooks ();
    }

    else
    {
      for ( auto& it : local_dxgi_records )
      {
        it->active = false;
      }
    }

    InterlockedIncrementRelease (&quick_hooked);
  }

  //if (SK_GetModuleHandle (L"d3d11.dll") != nullptr)
  //{
  //  SK_D3D11_Init       ();
#ifdef SK_AGGRESSIVE_HOOKS
    SK_ApplyQueuedHooks ();
#endif

  //  SK_Thread_SpinUntilAtomicMin (&quick_hooked, 2);
  //}
}