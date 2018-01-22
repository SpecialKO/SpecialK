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

#include <SpecialK/d3d9_backend.h>
#include <SpecialK/D3D9/texmgr.h>
#include <SpecialK/render_backend.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/import.h>

#include <SpecialK/render/d3d9/d3d9_device.h>
#include <SpecialK/render/d3d9/d3d9_swapchain.h>

MIDL_INTERFACE("D0223B96-BF7A-43fd-92BD-A43B0D82B9EB") IDirect3DDevice9;
MIDL_INTERFACE("B18B10CE-2649-405a-870F-95F777D4313A") IDirect3DDevice9Ex;

#include <SpecialK/core.h>
#include <SpecialK/log.h>

#include <SpecialK/stdafx.h>
#include <SpecialK/nvapi.h>
#include <SpecialK/config.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include <atlbase.h>
#include <comdef.h>

#include <SpecialK/log.h>
#include <SpecialK/crc32.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/command.h>
#include <SpecialK/hooks.h>
#include <SpecialK/window.h>
#include <SpecialK/steam_api.h>

#include <imgui/backends/imgui_d3d9.h>

#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/compatibility.h>

using namespace SK::D3D9;

volatile LONG               __d3d9_ready = FALSE;
PipelineStatsD3D9 SK::D3D9::pipeline_stats_d3d9;

volatile LONG ImGui_Init = FALSE;

extern bool __SK_bypass;

using Direct3DCreate9_pfn   =
  IDirect3D9* (STDMETHODCALLTYPE *)(UINT           SDKVersion);
using Direct3DCreate9Ex_pfn =
  HRESULT     (STDMETHODCALLTYPE *)(UINT           SDKVersion,
                                    IDirect3D9Ex** d3d9ex);


#define SK_D3D9_DeclTrampoline(apiset,func) \
apiset##_##func##_pfn apiset##_##func##_Original = nullptr;

#define SK_D3D9_Trampoline(apiset,func) apiset##_##func##_Original
#define SK_D3D9_TrampolineForVFTblHook(apiset,func) SK_D3D9_Trampoline(apiset,func), apiset##_##func##_pfn

Direct3DCreate9_pfn             Direct3DCreate9_Import                 = nullptr;
Direct3DCreate9Ex_pfn           Direct3DCreate9Ex_Import               = nullptr;

SK_D3D9_DeclTrampoline (D3D9,         CreateDevice)
SK_D3D9_DeclTrampoline (D3D9Ex,       CreateDeviceEx)

SK_D3D9_DeclTrampoline (D3D9Device,   Present)
SK_D3D9_DeclTrampoline (D3D9Device,   Reset)
SK_D3D9_DeclTrampoline (D3D9Device,   SetGammaRamp)
SK_D3D9_DeclTrampoline (D3D9Device,   CreateAdditionalSwapChain)
SK_D3D9_DeclTrampoline (D3D9Device,   TestCooperativeLevel)
SK_D3D9_DeclTrampoline (D3D9Device,   BeginScene)
SK_D3D9_DeclTrampoline (D3D9Device,   EndScene)
SK_D3D9_DeclTrampoline (D3D9Device,   DrawPrimitive)
SK_D3D9_DeclTrampoline (D3D9Device,   DrawIndexedPrimitive)
SK_D3D9_DeclTrampoline (D3D9Device,   DrawPrimitiveUP)
SK_D3D9_DeclTrampoline (D3D9Device,   DrawIndexedPrimitiveUP)
SK_D3D9_DeclTrampoline (D3D9Device,   GetTexture)
SK_D3D9_DeclTrampoline (D3D9Device,   SetTexture)
SK_D3D9_DeclTrampoline (D3D9Device,   SetSamplerState)
SK_D3D9_DeclTrampoline (D3D9Device,   SetViewport)
SK_D3D9_DeclTrampoline (D3D9Device,   SetRenderState)
SK_D3D9_DeclTrampoline (D3D9Device,   SetVertexShaderConstantF)
SK_D3D9_DeclTrampoline (D3D9Device,   SetPixelShaderConstantF)
SK_D3D9_DeclTrampoline (D3D9Device,   SetPixelShader)
SK_D3D9_DeclTrampoline (D3D9Device,   SetVertexShader)
SK_D3D9_DeclTrampoline (D3D9Device,   SetScissorRect)
SK_D3D9_DeclTrampoline (D3D9Device,   CreateTexture)
SK_D3D9_DeclTrampoline (D3D9Device,   CreateVertexBuffer)
SK_D3D9_DeclTrampoline (D3D9Device,   SetStreamSource)
SK_D3D9_DeclTrampoline (D3D9Device,   SetStreamSourceFreq)
SK_D3D9_DeclTrampoline (D3D9Device,   SetFVF)
SK_D3D9_DeclTrampoline (D3D9Device,   SetVertexDeclaration)
SK_D3D9_DeclTrampoline (D3D9Device,   CreateVertexDeclaration)
SK_D3D9_DeclTrampoline (D3D9Device,   CreateRenderTarget)
SK_D3D9_DeclTrampoline (D3D9Device,   CreateDepthStencilSurface)
SK_D3D9_DeclTrampoline (D3D9Device,   SetRenderTarget)
SK_D3D9_DeclTrampoline (D3D9Device,   SetDepthStencilSurface)
SK_D3D9_DeclTrampoline (D3D9Device,   UpdateTexture)
SK_D3D9_DeclTrampoline (D3D9Device,   StretchRect)
SK_D3D9_DeclTrampoline (D3D9Device,   SetCursorPosition)
SK_D3D9_DeclTrampoline (D3D9Device,   CreateOffscreenPlainSurface)

SK_D3D9_DeclTrampoline (D3D9ExDevice, PresentEx)
SK_D3D9_DeclTrampoline (D3D9ExDevice, ResetEx)

SK_D3D9_DeclTrampoline (D3D9Swap,     Present)


#pragma data_seg (".SK_D3D9_Hooks")
extern "C"
{
  // Global DLL's cache
  __declspec (dllexport) SK_HookCacheEntryGlobal (Direct3DCreate9)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9CreateDevice)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9PresentSwap)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9Present)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9Reset)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9CreateAdditionalSwapChain)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9TestCooperativeLevel)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9BeginScene)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9EndScene)

  __declspec (dllexport) SK_HookCacheEntryGlobal (Direct3DCreate9Ex)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9CreateDeviceEx)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9PresentEx)
  __declspec (dllexport) SK_HookCacheEntryGlobal (D3D9ResetEx)
};
#pragma data_seg ()
#pragma comment  (linker, "/SECTION:.SK_D3D9_Hooks,RWS")

// Local DLL's cached addresses
SK_HookCacheEntryLocal (Direct3DCreate9,               L"d3d9.dll", Direct3DCreate9,                        &Direct3DCreate9_Import)
SK_HookCacheEntryLocal (D3D9CreateDevice,              L"d3d9.dll", D3D9CreateDevice_Override,              &SK_D3D9_Trampoline (D3D9,       CreateDevice))
SK_HookCacheEntryLocal (D3D9PresentSwap,               L"d3d9.dll", D3D9Swap_Present,                       &SK_D3D9_Trampoline (D3D9Swap,   Present))
SK_HookCacheEntryLocal (D3D9Present,                   L"d3d9.dll", D3D9Device_Present,                     &SK_D3D9_Trampoline (D3D9Device, Present))
SK_HookCacheEntryLocal (D3D9Reset,                     L"d3d9.dll", D3D9Reset_Override,                     &SK_D3D9_Trampoline (D3D9Device, Reset))
SK_HookCacheEntryLocal (D3D9CreateAdditionalSwapChain, L"d3d9.dll", D3D9CreateAdditionalSwapChain_Override, &SK_D3D9_Trampoline (D3D9Device, CreateAdditionalSwapChain))
SK_HookCacheEntryLocal (D3D9TestCooperativeLevel,      L"d3d9.dll", D3D9TestCooperativeLevel_Override,      &SK_D3D9_Trampoline (D3D9Device, TestCooperativeLevel))
SK_HookCacheEntryLocal (D3D9BeginScene,                L"d3d9.dll", D3D9BeginScene_Override,                &SK_D3D9_Trampoline (D3D9Device, BeginScene))
SK_HookCacheEntryLocal (D3D9EndScene,                  L"d3d9.dll", D3D9EndScene_Override,                  &SK_D3D9_Trampoline (D3D9Device, EndScene))

SK_HookCacheEntryLocal (Direct3DCreate9Ex,             L"d3d9.dll", Direct3DCreate9Ex,                      &Direct3DCreate9Ex_Import)
SK_HookCacheEntryLocal (D3D9CreateDeviceEx,            L"d3d9.dll", D3D9CreateDeviceEx_Override,            &SK_D3D9_Trampoline (D3D9Ex,       CreateDeviceEx))
SK_HookCacheEntryLocal (D3D9PresentEx,                 L"d3d9.dll", D3D9ExDevice_PresentEx,                 &SK_D3D9_Trampoline (D3D9ExDevice, PresentEx))
SK_HookCacheEntryLocal (D3D9ResetEx,                   L"d3d9.dll", D3D9ResetEx,                            &SK_D3D9_Trampoline (D3D9ExDevice, ResetEx))

static
std::vector <sk_hook_cache_record_s *> global_d3d9_records =
  { 
    &GlobalHook_D3D9PresentSwap,          &GlobalHook_D3D9Present,
    &GlobalHook_D3D9Reset,                &GlobalHook_D3D9CreateAdditionalSwapChain,
    &GlobalHook_D3D9TestCooperativeLevel, &GlobalHook_D3D9BeginScene,
    &GlobalHook_D3D9EndScene,
    &GlobalHook_D3D9PresentEx,            &GlobalHook_D3D9ResetEx,

    &GlobalHook_Direct3DCreate9,          &GlobalHook_D3D9CreateDeviceEx,
    &GlobalHook_D3D9CreateDevice,         &GlobalHook_Direct3DCreate9Ex  };

static
std::vector <sk_hook_cache_record_s *> local_d3d9_records =
  { &LocalHook_D3D9PresentSwap,          &LocalHook_D3D9Present,
    &LocalHook_D3D9Reset,                &LocalHook_D3D9CreateAdditionalSwapChain,
    &LocalHook_D3D9TestCooperativeLevel, &LocalHook_D3D9BeginScene,
    &LocalHook_D3D9EndScene,
    &LocalHook_D3D9PresentEx,            &LocalHook_D3D9ResetEx,
    &LocalHook_Direct3DCreate9,          &LocalHook_D3D9CreateDeviceEx,
    &LocalHook_D3D9CreateDevice,         &LocalHook_Direct3DCreate9Ex,   };


void
WINAPI
WaitForInit_D3D9 (void)
{
  if (Direct3DCreate9_Import == nullptr)
  {
    SK_RunOnce (SK_BootD3D9 ());
  }

  if (SK_TLS_Bottom ()->d3d9.ctx_init_thread)
    return;

  if (! ReadAcquire (&__d3d9_ready))
    SK_Thread_SpinUntilFlagged (&__d3d9_ready);
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateOffscreenPlainSurface_Override (
  _In_  IDirect3DDevice9   *This,
  _In_  UINT                Width,
  _In_  UINT                Height,
  _In_  D3DFORMAT           Format,
  _In_  D3DPOOL             Pool,
  _Out_ IDirect3DSurface9 **ppSurface,
  _In_  HANDLE             *pSharedHandle )
{
  if (SK_GetCurrentGameID () == SK_GAME_ID::YS_Seven)
  {
    CComPtr <IDirect3DSwapChain9> pSwapChain;

    if (SUCCEEDED (This->GetSwapChain (0, &pSwapChain)))
    {
      D3DPRESENT_PARAMETERS pparams = { };
      pSwapChain->GetPresentParameters (&pparams);

      if (Format == D3DFMT_R5G6B5 && ( Width == pparams.BackBufferWidth ) )
      {
        Format = D3DFMT_X8R8G8B8;
      }

      if (Format == D3DFMT_D16 && ( Width == pparams.BackBufferWidth ) )
      {
        Format = D3DFMT_D24X8;
      }
    }
  }

  return
    SK_D3D9_Trampoline (D3D9Device, CreateOffscreenPlainSurface)
      ( This, Width, Height, Format, Pool, ppSurface, pSharedHandle );
}


D3DPRESENT_PARAMETERS    g_D3D9PresentParams          = {  };

#define D3D9_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {    \
  void** _vftable = *(void***)*(_Base);                                      \
                                                                             \
  if ((_Original) == nullptr) {                                              \
    SK_CreateVFTableHook2 ( L##_Name,                                        \
                              _vftable,                                      \
                                (_Index),                                    \
                                  (_Override),                               \
                                    (LPVOID *)&(_Original));                 \
  }                                                                          \
}
CRITICAL_SECTION cs_vs = { };
CRITICAL_SECTION cs_ps = { };
CRITICAL_SECTION cs_vb = { };

KnownObjects        SK::D3D9::known_objs = { };
KnownShaders        SK::D3D9::Shaders    = { };
ShaderTracker       SK::D3D9::tracked_vs = { };
ShaderTracker       SK::D3D9::tracked_ps = { };

VertexBufferTracker SK::D3D9::tracked_vb = { };
RenderTargetTracker SK::D3D9::tracked_rt = { };

DrawState           SK::D3D9::draw_state = { };
FrameState          SK::D3D9::last_frame = { };

void
SK::D3D9::VertexBufferTracker::clear (void)
{
  active = false;

  InterlockedExchange (&num_draws, 0);

  instances = 0;

  vertex_shaders.clear ();
  pixel_shaders.clear  ();
  textures.clear       ();

  for ( auto& it : vertex_decls ) it->Release ();

  vertex_decls.clear ();
}

void
SK::D3D9::VertexBufferTracker::use (void)
{
  SK_AutoCriticalSection auto_cs (&cs_vb);

  IDirect3DVertexDeclaration9* decl = nullptr;
  CComPtr <IDirect3DDevice9>   pDev = nullptr;

  IUnknown *pUnkDev =
    SK_GetCurrentRenderBackend ().device;

  if (         pUnkDev == nullptr ||
       FAILED (pUnkDev->QueryInterface <IDirect3DDevice9> (&pDev)) )
  {
    return;
  }

  const uint32_t vs_checksum = Shaders.vertex.current.crc32c;
  const uint32_t ps_checksum = Shaders.pixel.current.crc32c;

  vertex_shaders.emplace (vs_checksum);
  pixel_shaders.emplace  (ps_checksum);

  if (SUCCEEDED (pDev->GetVertexDeclaration (&decl)))
  {
    static D3DVERTEXELEMENT9 elem_decl [MAXD3DDECLLENGTH];
    static UINT              num_elems;

    // Run through the vertex decl and figure out which samplers have texcoords,
    //   that is a pretty good indicator as to which textures are actually used...
    if (SUCCEEDED (decl->GetDeclaration (elem_decl, &num_elems)))
    {
      for ( UINT i = 0; i < num_elems; i++ )
      {
        if (elem_decl [i].Usage == D3DDECLUSAGE_TEXCOORD)
          textures.emplace (draw_state.current_tex [elem_decl [i].UsageIndex]);
      }
    }

    if (! vertex_decls.count   (decl))
          vertex_decls.emplace (decl);
    else
      decl->Release ();
  }
}

// For now, let's just focus on stream0 and pretend nothing else exists...
IDirect3DVertexBuffer9* vb_stream0 = nullptr;

std::unordered_map <uint32_t, ShaderDisassembly> ps_disassembly;
std::unordered_map <uint32_t, ShaderDisassembly> vs_disassembly;


void SK_D3D9_InitShaderModTools   (void);
bool SK_D3D9_ShouldSkipRenderPass (D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, UINT StartVertex, bool& wireframe);
void SK_D3D9_EndFrame             (void);
void SK_D3D9_EndScene             (void);
void SK_D3D9_SetPixelShader       ( IDirect3DDevice9       *pDev,
                                    IDirect3DPixelShader9  *pShader );
void SK_D3D9_SetVertexShader      ( IDirect3DDevice9       *pDev,
                                    IDirect3DVertexShader9 *pShader );

class SK_D3D9RenderBackend : public SK_IVariableListener
{
  virtual bool OnVarChange (SK_IVariable*, void*) override
  {
    return false;
  }

public:
  static SK_D3D9RenderBackend* getInstance (void)
  {
    if (pBackend == nullptr)
      pBackend = new SK_D3D9RenderBackend ();

    return pBackend;
  }

protected:
  SK_IVariable* osd_vidcap_;

private:
  SK_D3D9RenderBackend (void)
  {
    config.render.d3d9.osd_in_vidcap = true;

    SK_ICommandProcessor* cmd =
        SK_GetCommandProcessor ();

    osd_vidcap_ =
      SK_CreateVar (
        SK_IVariable::Boolean,
          static_cast <bool *> (&config.render.d3d9.osd_in_vidcap),
            this
      );

    // Tales of Berseria "Fix" uses this
    static bool broken_lod_bias = false;

    cmd->AddVariable ("d3d9.OSDInVidcap",            osd_vidcap_);
    cmd->AddVariable ("d3d9.HasBrokenMipmapLODBias", SK_CreateVar (SK_IVariable::Boolean, &broken_lod_bias));
  }

  static SK_D3D9RenderBackend* pBackend;
};

SK_D3D9RenderBackend* SK_D3D9RenderBackend::pBackend = nullptr;

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
#pragma comment (lib, "d3dx9.lib")

#ifdef _WIN64
# define SK_CEGUI_LIB_BASE "CEGUI/x64/"
#else
# define SK_CEGUI_LIB_BASE "CEGUI/Win32/"
#endif

#define _SKC_MakeCEGUILib(library) \
  __pragma (comment (lib, SK_CEGUI_LIB_BASE #library ##".lib"))

_SKC_MakeCEGUILib ("CEGUIDirect3D9Renderer-0")
_SKC_MakeCEGUILib ("CEGUIBase-0")
_SKC_MakeCEGUILib ("CEGUICoreWindowRendererSet")
_SKC_MakeCEGUILib ("CEGUIRapidXMLParser")
_SKC_MakeCEGUILib ("CEGUICommonDialogs-0")
_SKC_MakeCEGUILib ("CEGUISTBImageCodec")

#include <delayimp.h>
#include <CEGUI/CEGUI.h>
#include <CEGUI/Rect.h>
#include <CEGUI/RendererModules/Direct3D9/Renderer.h>

#pragma comment (lib, "delayimp.lib")

static
CEGUI::Direct3D9Renderer* cegD3D9 = nullptr;
#endif


IDirect3DStateBlock9*     cegD3D9_SB = nullptr;

static volatile LONG __gui_reset          = TRUE;
static volatile LONG __cegui_frames_drawn = 0L;

void ResetCEGUI_D3D9 (IDirect3DDevice9* pDev);

void
SK_CEGUI_DrawD3D9 (IDirect3DDevice9* pDev, IDirect3DSwapChain9* pSwapChain)
{
  static volatile LONG             __first_frame = TRUE;
  if (InterlockedCompareExchange (&__first_frame, false, true))
  {
    for ( auto it : local_d3d9_records )
    {
      SK_Hook_ResolveTarget (*it);

      // Don't cache addresses that were screwed with by other injectors
      const wchar_t* wszSection = 
        StrStrIW (it->target.module_path, LR"(\d3d9.dll)") ?
                                          L"D3D9.Hooks" : nullptr;

      SK_Hook_CacheTarget ( *it, wszSection );

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
    }

    if (SK_IsInjected ())
    {
      auto it_local  = std::begin (local_d3d9_records);
      auto it_global = std::begin (global_d3d9_records);

      while ( it_local != std::end (local_d3d9_records) )
      {
        if (( *it_local )->hits &&
  StrStrIW (( *it_local )->target.module_path, LR"(\d3d9.dll)") &&
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
    }
  }



  InterlockedIncrement (&__cegui_frames_drawn);

  if (InterlockedCompareExchange (&__gui_reset, FALSE, TRUE))
  {
    if ((uintptr_t)cegD3D9 > 1)
    {
      SK_TextOverlayManager::getInstance ()->destroyAllOverlays ();
      SK_PopupManager::getInstance       ()->destroyAllPopups   ();

      CEGUI::WindowManager::getDllSingleton ().cleanDeadPool    ();
    }

    if (cegD3D9_SB != nullptr) cegD3D9_SB->Release ();
        cegD3D9_SB  = nullptr;

    if ((uintptr_t)cegD3D9 > 1) cegD3D9->destroySystem ();
                   cegD3D9  = nullptr;
  }

  else if (cegD3D9 == nullptr)
  {
    extern void
    SK_InstallWindowHook (HWND hWnd);

    D3DPRESENT_PARAMETERS pparams;
    pSwapChain->GetPresentParameters (&pparams);

    SK_InstallWindowHook (pparams.hDeviceWindow);

    ResetCEGUI_D3D9 (pDev);
  }

  else if (pDev != nullptr)
  {
    CComPtr <IDirect3DStateBlock9> pStateBlock = nullptr;
    pDev->CreateStateBlock (D3DSBT_ALL, &pStateBlock);

    if (! pStateBlock)
      return;

    pStateBlock->Capture ();

    bool new_sb = (! cegD3D9_SB);

    if (new_sb)
    {
      pDev->CreateStateBlock (D3DSBT_ALL, &cegD3D9_SB);

      pDev->SetRenderState (D3DRS_CULLMODE,                 D3DCULL_NONE);

      pDev->SetRenderState (D3DRS_LIGHTING,                 FALSE);
      pDev->SetRenderState (D3DRS_SPECULARENABLE,           FALSE);
      pDev->SetRenderState (D3DRS_FOGENABLE,                FALSE);
      pDev->SetRenderState (D3DRS_AMBIENT,                  0);

      pDev->SetRenderState (D3DRS_ALPHABLENDENABLE,         TRUE);
      pDev->SetRenderState (D3DRS_ALPHATESTENABLE,          FALSE);

      pDev->SetRenderState (D3DRS_SRCBLEND,                 D3DBLEND_INVSRCALPHA);
      pDev->SetRenderState (D3DRS_DESTBLEND,                D3DBLEND_SRCALPHA);
      pDev->SetRenderState (D3DRS_BLENDOP,                  D3DBLENDOP_ADD);
      pDev->SetRenderState (D3DRS_BLENDOPALPHA,             D3DBLENDOP_ADD);

      pDev->SetRenderState (D3DRS_STENCILENABLE,            FALSE);
      pDev->SetRenderState (D3DRS_SCISSORTESTENABLE,        FALSE);

      pDev->SetRenderState (D3DRS_ZENABLE,                  FALSE);
      pDev->SetRenderState (D3DRS_ZWRITEENABLE,             FALSE);
      pDev->SetRenderState (D3DRS_SRGBWRITEENABLE,          FALSE);
      pDev->SetRenderState (D3DRS_COLORWRITEENABLE,         D3DCOLORWRITEENABLE_RED   |
                                                            D3DCOLORWRITEENABLE_GREEN |
                                                            D3DCOLORWRITEENABLE_BLUE  |
                                                            D3DCOLORWRITEENABLE_ALPHA );

      pDev->SetSamplerState        (0, D3DSAMP_SRGBTEXTURE, FALSE);

      pDev->SetTextureStageState   (0, D3DTSS_COLOROP,      D3DTOP_MODULATE);
      pDev->SetTextureStageState   (0, D3DTSS_COLORARG0,    D3DTA_CURRENT);
      pDev->SetTextureStageState   (0, D3DTSS_COLORARG1,    D3DTA_TEXTURE);
      pDev->SetTextureStageState   (0, D3DTSS_COLORARG2,    D3DTA_DIFFUSE);

      pDev->SetTextureStageState   (0, D3DTSS_ALPHAOP,      D3DTOP_MODULATE);
      pDev->SetTextureStageState   (0, D3DTSS_ALPHAARG0,    D3DTA_CURRENT);
      pDev->SetTextureStageState   (0, D3DTSS_ALPHAARG1,    D3DTA_TEXTURE);
      pDev->SetTextureStageState   (0, D3DTSS_ALPHAARG2,    D3DTA_DIFFUSE);

      pDev->SetTextureStageState   (0, D3DTSS_RESULTARG,    D3DTA_CURRENT);

      pDev->SetSamplerState        (0, D3DSAMP_MINFILTER,   D3DTEXF_POINT);
      pDev->SetSamplerState        (0, D3DSAMP_MAGFILTER,   D3DTEXF_POINT);

      cegD3D9_SB->Capture ();
    } else {
      cegD3D9_SB->Apply ();
    }

    D3DVIEWPORT9        vp_orig;
    pDev->GetViewport (&vp_orig);

    D3DCAPS9 caps;
    pDev->GetDeviceCaps (&caps);

    CComPtr <IDirect3DSurface9> pBackBuffer = nullptr;
    CComPtr <IDirect3DSurface9> rts [8];
    CComPtr <IDirect3DSurface9> ds          = nullptr;

    for (UINT target = 0; target < std::min (caps.NumSimultaneousRTs, 8UL); target++) {
      pDev->GetRenderTarget (target, &rts [target]);
    }

    pDev->GetDepthStencilSurface (&ds);

    D3DSURFACE_DESC surf_desc = { };

    if (SUCCEEDED (pDev->GetBackBuffer (0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)))
    {
      pBackBuffer->GetDesc (&surf_desc);

      pDev->SetRenderTarget (0, pBackBuffer);

      for (UINT target = 1; target < caps.NumSimultaneousRTs; target++)
        pDev->SetRenderTarget (target, nullptr);
    }

    D3DPRESENT_PARAMETERS pp;

    if (SUCCEEDED (pSwapChain->GetPresentParameters (&pp)) && pp.hDeviceWindow != nullptr)
    {
      extern HWND hWndRender;
      if (pp.hDeviceWindow != hWndRender)
        hWndRender = pp.hDeviceWindow;
    }

#if 0
    surf_desc.Width  = ImGui::GetIO ().DisplaySize.x;
    surf_desc.Height = ImGui::GetIO ().DisplaySize.y;

    D3DVIEWPORT9 vp_new;

    vp_new.X      = 0;
    vp_new.Y      = 0;
    vp_new.Width  = surf_desc.Width;
    vp_new.Height = surf_desc.Height;

    vp_new.MinZ =  0.0f;
    vp_new.MaxZ =  1.0f;

    pDev->SetViewport (&vp_new);
#else
    // Keep within the confines of the game's viewport, in case it is managing
    //   aspect ratio itself
    surf_desc.Width  = vp_orig.Width;
    surf_desc.Height = vp_orig.Height;

    if (config.cegui.enable && GetModuleHandle (L"CEGUIDirect3D9Renderer-0.dll"))
    {
      CEGUI::System::getDllSingleton ().getRenderer ()->setDisplaySize (
          CEGUI::Sizef (
            static_cast <float> (surf_desc.Width),
            static_cast <float> (surf_desc.Height)
          )
      );
    }
#endif

    if ((uintptr_t)cegD3D9 > 1)
    {
      cegD3D9->beginRendering ();
      SK_TextOverlayManager::getInstance ()->drawAllOverlays (0.0f, 0.0f);
    }

    pDev->SetRenderState (D3DRS_SRGBWRITEENABLE,          FALSE);
    pDev->SetRenderState (D3DRS_ALPHABLENDENABLE,         TRUE);
    pDev->SetRenderState (D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    pDev->SetRenderState (D3DRS_SRCBLEND,                 D3DBLEND_INVSRCALPHA);
    pDev->SetRenderState (D3DRS_DESTBLEND,                D3DBLEND_SRCALPHA);

    pDev->SetRenderState (D3DRS_ALPHATESTENABLE,          FALSE);

    if ((uintptr_t)cegD3D9 > 1)
      CEGUI::System::getDllSingleton ().renderAllGUIContexts ();


    if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
         static_cast <int> (SK_RenderAPI::D3D9)                  )
    {
      extern DWORD SK_ImGui_DrawFrame (DWORD dwFlags, void* user);
                   SK_ImGui_DrawFrame (       0x00,     nullptr );
    }


    if (SK_Steam_DrawOSD () != 0)
    {
      pDev->SetRenderState (D3DRS_SRGBWRITEENABLE,          FALSE);
      pDev->SetRenderState (D3DRS_ALPHABLENDENABLE,         TRUE);
      pDev->SetRenderState (D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
      pDev->SetRenderState (D3DRS_SRCBLEND,                 D3DBLEND_INVSRCALPHA);
      pDev->SetRenderState (D3DRS_DESTBLEND,                D3DBLEND_SRCALPHA);

      pDev->SetRenderState (D3DRS_ALPHATESTENABLE,          FALSE);

      if ((uintptr_t)cegD3D9 > 1)
        CEGUI::System::getDllSingleton ().renderAllGUIContexts ();
    }
    
    if ((uintptr_t)cegD3D9 > 1)
      cegD3D9->endRendering ();

    pDev->SetViewport (&vp_orig);

    for (UINT target = 0; target < std::min (8UL, caps.NumSimultaneousRTs); target++)
      pDev->SetRenderTarget (target, rts [target]);

    pDev->SetDepthStencilSurface (ds);
    pStateBlock->Apply ();
  }
}

void
SK_CEGUI_QueueResetD3D9 (void)
{
  InterlockedExchange (&__gui_reset, TRUE);
}

void
ResetCEGUI_D3D9 (IDirect3DDevice9* pDev)
{
  if (! ReadAcquire (&ImGui_Init))
  {
    //
    // Initialize ImGui for D3D9 games
    //
    D3DDEVICE_CREATION_PARAMETERS params;
    pDev->GetCreationParameters (&params);

    if ( ImGui_ImplDX9_Init ( (void *)params.hFocusWindow,
                                    pDev,
                                     nullptr )
       )
    {
      InterlockedExchange ( &ImGui_Init, TRUE );
    }
  }
  


  if (cegD3D9 != nullptr || (pDev == nullptr))
  {
    SK_Steam_ClearPopups ();

    if ((uintptr_t)cegD3D9 > 1)
    {
      SK_TextOverlayManager::getInstance ()->destroyAllOverlays ();
      SK_PopupManager::getInstance ()->destroyAllPopups         ();

      CEGUI::WindowManager::getDllSingleton ().cleanDeadPool    ();
    }

    if (cegD3D9_SB != nullptr) cegD3D9_SB->Release ();
        cegD3D9_SB  = nullptr;

    if ((uintptr_t)cegD3D9 > 1) cegD3D9->destroySystem ();
                   cegD3D9  = nullptr;
  }

  else if (cegD3D9 == nullptr)
  {
    if (cegD3D9_SB != nullptr) cegD3D9_SB->Release ();
        cegD3D9_SB  = nullptr;

    if (config.cegui.enable && GetModuleHandle (L"CEGUIDirect3D9Renderer-0.dll"))
    {
      cegD3D9 = dynamic_cast <CEGUI::Direct3D9Renderer *> (
        &CEGUI::Direct3D9Renderer::bootstrapSystem (pDev)
      );

      extern void
      SK_CEGUI_RelocateLog (void);

      SK_CEGUI_RelocateLog ();

      extern void
      SK_CEGUI_InitBase (void);

      SK_CEGUI_InitBase ();

      SK_PopupManager::getInstance ()->destroyAllPopups       ();
      SK_TextOverlayManager::getInstance ()->resetAllOverlays (cegD3D9);
    }

    else
      cegD3D9 = (CEGUI::Direct3D9Renderer *)1;


    SK_Steam_ClearPopups ();
  }
}



using finish_pfn =
  void (WINAPI *)(void);

void
WINAPI
SK_HookD3D9 (void)
{
  extern bool __SK_bypass;
  if (__SK_bypass)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    HMODULE hBackend =
      (SK_GetDLLRole () & DLL_ROLE::D3D9) ? backend_dll :
                                     GetModuleHandle (L"d3d9.dll");

    SK_LOG0 ( (L"Importing Direct3DCreate9{Ex}..."), L"   D3D9   ");
    SK_LOG0 ( (L"================================"), L"   D3D9   ");

    if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"d3d9.dll"))
    {
      if (! LocalHook_Direct3DCreate9.active)
      {
        Direct3DCreate9_Import =  \
          (Direct3DCreate9_pfn)GetProcAddress (hBackend, "Direct3DCreate9");

        LocalHook_Direct3DCreate9.target.addr = Direct3DCreate9_Import;
        LocalHook_Direct3DCreate9.active      = true;
      }

      SK_LOG0 ( ( L"  Direct3DCreate9:   %s",
                    SK_MakePrettyAddress (Direct3DCreate9_Import).c_str () ),
                  L"   D3D9   " );
      SK_LogSymbolName                   (Direct3DCreate9_Import);

      if (config.apis.d3d9ex.hook)
      {
        if (!LocalHook_Direct3DCreate9Ex.active)
        {
          Direct3DCreate9Ex_Import =  \
            (Direct3DCreate9Ex_pfn)GetProcAddress (hBackend, "Direct3DCreate9Ex");

          SK_LOG0 ( ( L"  Direct3DCreate9Ex: %s",
                        SK_MakePrettyAddress (Direct3DCreate9Ex_Import).c_str () ),
                      L"  D3D9Ex  " );
          SK_LogSymbolName                   (Direct3DCreate9Ex_Import);

          LocalHook_Direct3DCreate9Ex.target.addr = Direct3DCreate9Ex_Import;
          LocalHook_Direct3DCreate9Ex.active      = true;
        }
      }
    }

    else
    {
      LPVOID pfnDirect3DCreate9   = nullptr;
      LPVOID pfnDirect3DCreate9Ex = nullptr;

      if ( (! LocalHook_Direct3DCreate9.active) && GetProcAddress (hBackend, "Direct3DCreate9") &&
             MH_OK ==
             SK_CreateDLLHook2 ( L"d3d9.dll",
                                  "Direct3DCreate9",
                                   Direct3DCreate9,
          static_cast_p2p <void> (&Direct3DCreate9_Import),
                               &pfnDirect3DCreate9 )
         )
      {
        LocalHook_Direct3DCreate9.target.addr = pfnDirect3DCreate9;
        LocalHook_Direct3DCreate9.active      = true;
      }
      else if (LocalHook_Direct3DCreate9.active) {
        pfnDirect3DCreate9 = LocalHook_Direct3DCreate9.target.addr;
      }

      if ( (! LocalHook_Direct3DCreate9Ex.active) && GetProcAddress (hBackend, "Direct3DCreate9Ex") &&
             config.apis.d3d9ex.hook &&
             MH_OK ==
               SK_CreateDLLHook2 ( L"d3d9.dll",
                                    "Direct3DCreate9Ex",
                                     Direct3DCreate9Ex,
            static_cast_p2p <void> (&Direct3DCreate9Ex_Import),
                                 &pfnDirect3DCreate9Ex )
         )
      {
        LocalHook_Direct3DCreate9Ex.target.addr = pfnDirect3DCreate9Ex;
        LocalHook_Direct3DCreate9Ex.active      = true;
      }
      else if (LocalHook_Direct3DCreate9Ex.active) {
        pfnDirect3DCreate9Ex = LocalHook_Direct3DCreate9Ex.target.addr;
      }

      bool success = LocalHook_Direct3DCreate9.active ||
        ( MH_QueueEnableHook (pfnDirect3DCreate9) == MH_OK );

      if (config.apis.d3d9ex.hook)
        success &= ( LocalHook_Direct3DCreate9Ex.active ||
                    (MH_QueueEnableHook (pfnDirect3DCreate9Ex) == MH_OK) );

      SK_LOG0 ( ( L"  Direct3DCreate9:   %s  { Hooked  }",
                    SK_MakePrettyAddress (pfnDirect3DCreate9).c_str () ),
                  L"   D3D9   " );
      SK_LogSymbolName                   (pfnDirect3DCreate9);

      SK_LOG0 ( ( L"  Direct3DCreate9Ex: %s  { Hooked  }",
                    SK_MakePrettyAddress (pfnDirect3DCreate9Ex).c_str () ),
                  L"  D3D9Ex  " );
      SK_LogSymbolName                   (pfnDirect3DCreate9Ex);
    }

    HookD3D9            (nullptr);
    SK_ApplyQueuedHooks (       );

    // Load user-defined DLLs (Plug-In)
    SK_RunLHIfBitness ( 64, SK_LoadPlugIns64 (),
                            SK_LoadPlugIns32 () );

    InterlockedIncrement (&hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&hooked, 2);
  SK_Thread_SpinUntilFlagged   (&__d3d9_ready);
}

void
WINAPI
d3d9_init_callback (finish_pfn finish)
{
  if (! SK_IsHostAppSKIM ())
  {
    SK_BootD3D9 ();

    SK_Thread_SpinUntilFlagged (&__d3d9_ready);
  }

  finish ();
}


#include <SpecialK/ini.h>
#include <SpecialK/injection/address_cache.h>

bool
SK::D3D9::Startup (void)
{
  bool ret = SK_StartupCore (L"d3d9", d3d9_init_callback);

  return ret;
}

bool
SK::D3D9::Shutdown (void)
{
  // The texture manager built-in to SK is derived from these ...
  //   until those projects are modified to use THIS texture manager,
  //     they need special treatment.
  if ( GetModuleHandle (L"tzfix.dll") == nullptr &&
       GetModuleHandle (L"tsfix.dll") == nullptr )
  {
    if (tex_mgr.init)
      tex_mgr.Shutdown ();
  }


  if (SK_GetFramesDrawn () < 2)
  {
    SK_LOG0 ( ( L" !!! No frames drawn using D3D9 backend; purging injection address cache..." ),
                L"Hook Cache" );

    for ( auto it : local_d3d9_records )
    {
      SK_Hook_RemoveTarget (
        *it,
          L"D3D9.Hooks" );
    }
  }


  return SK_ShutdownCore (L"d3d9");
}


extern "C" const wchar_t* SK_DescribeVirtualProtectFlags (DWORD dwProtect);

#define __PTR_SIZE   sizeof LPCVOID
#define __PAGE_PRIVS PAGE_EXECUTE_READWRITE

#define D3D9_VIRTUAL_OVERRIDE(_Base,_Index,_Name,_Override,_Original,_Type) { \
  void** _vftable = *(void***)*_Base;                                         \
                                                                              \
  if (_vftable [_Index] != _Override) {                                       \
    DWORD dwProtect;                                                          \
                                                                              \
    VirtualProtect (&_vftable [_Index], __PTR_SIZE, __PAGE_PRIVS, &dwProtect);\
                                                                              \
    /*dll_log.Log (L" Old VFTable entry for %s: %ph  (Memory Policy: %s)",*/  \
                 /*L##_Name, vftable [_Index],                              */\
                 /*SK_DescribeVirtualProtectFlags (dwProtect));             */\
                                                                              \
    if (_Original == NULL)                                                    \
      _Original = (##_Type)_vftable [_Index];                                 \
                                                                              \
    /*dll_log.Log (L"  + %s: %ph", L#_Original, _Original);*/                 \
                                                                              \
    _vftable [_Index] = _Override;                                            \
                                                                              \
    VirtualProtect (&_vftable [_Index], __PTR_SIZE, dwProtect, &dwProtect);   \
                                                                              \
    /*dll_log.Log(L" New VFTable entry for %s: %ph  (Memory Policy: %s)\n"*/  \
                  /*,L##_Name, vftable [_Index],                            */\
                  /*SK_DescribeVirtualProtectFlags (dwProtect));            */\
  }                                                                           \
}

#define D3D9_INTERCEPT(_Base,_Index,_Name,_Override,_Original,_Type) { \
  if (config.render.d3d9.hook_type == 0) {                             \
    D3D9_VIRTUAL_HOOK (   _Base,   _Index, _Name, _Override,           \
                        _Original, _Type );                            \
  } else {                                                             \
    D3D9_VIRTUAL_OVERRIDE(_Base,   _Index, _Name, _Override,           \
                        _Original, _Type );                            \
  }                                                                    \
}

#define D3D9_CALL(_Ret, _Call) {                                      \
  (_Ret) = (_Call);                                                   \
  dll_log.Log ( L"[   D3D9   ] [@]  Return: %s  -  < " L#_Call L" >", \
                  SK_DescribeHRESULT (_Ret) );                        \
}

void
WINAPI
SK_D3D9_FixUpBehaviorFlags (DWORD& BehaviorFlags)
{
  BehaviorFlags &= ~D3DCREATE_FPU_PRESERVE;
  BehaviorFlags &= ~D3DCREATE_NOWINDOWCHANGES;

  //if (BehaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING)
  //{
  //  dll_log.Log (L"[CompatHack] D3D9 Fixup: "
  //               L"Software Vertex Processing Replaced with Mixed-Mode.");
  //  BehaviorFlags &= ~D3DCREATE_SOFTWARE_VERTEXPROCESSING;
  //  BehaviorFlags |=  D3DCREATE_MIXED_VERTEXPROCESSING;
  //}

  if (config.render.d3d9.force_impure)
    BehaviorFlags &= ~D3DCREATE_PUREDEVICE;

  if ( GetModuleHandle (L"tbfix.dll") ||
       GetModuleHandle (L"tzfix.dll") ||
       GetModuleHandle (L"tsfix.dll") )
  {
    BehaviorFlags |= D3DCREATE_MULTITHREADED;
  }
}

void
WINAPI
SK_D3D9_SetFPSTarget ( D3DPRESENT_PARAMETERS* pPresentationParameters,
                       D3DDISPLAYMODEEX*      pFullscreenMode = nullptr )
{
  auto TargetFPS =
    static_cast <int> (config.render.framerate.target_fps);

  // First consider D3D9Ex FullScreen Mode
  int Refresh   = pFullscreenMode != nullptr ?
                    pFullscreenMode->RefreshRate :
                    0;

  // Then, use the presentation parameters
  if (Refresh == 0)
  {
    if ( pPresentationParameters           != nullptr &&
         pPresentationParameters->Windowed == FALSE )
      Refresh = pPresentationParameters->FullScreen_RefreshRateInHz;
    else
      Refresh = 0;
  }

  if (config.render.framerate.refresh_rate != -1)
  {
    if ( pPresentationParameters           != nullptr &&
         pPresentationParameters->Windowed == FALSE)
    {
      dll_log.Log ( L"[  D3D9  ]  >> Refresh Rate Override: %li",
                      config.render.framerate.refresh_rate );

      Refresh = config.render.framerate.refresh_rate;

      if ( pFullscreenMode != nullptr )
        pFullscreenMode->RefreshRate = Refresh;

      pPresentationParameters->FullScreen_RefreshRateInHz = Refresh;
    }
  }

  if ( TargetFPS != 0 && Refresh != 0       &&
       pPresentationParameters   != nullptr &&
       pPresentationParameters->Windowed == FALSE)
  {
    //if (Refresh >= TargetFPS) {
    //  if (! (Refresh % TargetFPS)) {
    //    dll_log.Log ( L"[   D3D9   ]  >> Targeting %li FPS - using 1:%li VSYNC;"
    //                  L" (refresh = %li Hz)",
    //                    TargetFPS,
    //                      Refresh / TargetFPS,
    //                        Refresh );
    //
    //    pPresentationParameters->SwapEffect           = D3DSWAPEFFECT_DISCARD;
    //    pPresentationParameters->PresentationInterval = Refresh / TargetFPS;
    //
    //  } else {
    //    dll_log.Log ( L"[   D3D9   ]  >> Cannot target %li FPS using VSYNC - no such factor exists;"
    //                  L" (refresh = %li Hz)",
    //                    TargetFPS,
    //                      Refresh );
    //  }
    //} else {
    //  dll_log.Log ( L"[   D3D9   ]  >> Cannot target %li FPS using VSYNC - higher than refresh rate;"
    //                L" (refresh = %li Hz)",
    //                  TargetFPS,
    //                    Refresh );
    //}
  }

  if (pPresentationParameters != nullptr)
  {
    if (       config.render.framerate.buffer_count != -1 &&
         (UINT)config.render.framerate.buffer_count !=
           pPresentationParameters->BackBufferCount ) {
      dll_log.Log ( L"[   D3D9   ]  >> Backbuffer Override: (Requested=%lu, Override=%li)",
                      pPresentationParameters->BackBufferCount,
                        config.render.framerate.buffer_count );
      pPresentationParameters->BackBufferCount =
        config.render.framerate.buffer_count;
    }

    if (       config.render.framerate.present_interval != -1 &&
         (UINT)config.render.framerate.present_interval !=
            pPresentationParameters->PresentationInterval ) {
      dll_log.Log ( L"[   D3D9   ]  >> VSYNC Override: (Requested=1:%lu, Override=1:%li)",
                      pPresentationParameters->PresentationInterval,
                        config.render.framerate.present_interval );
      pPresentationParameters->PresentationInterval =
        config.render.framerate.present_interval;
    }
  }
}

extern bool WINAPI SK_CheckForGeDoSaTo (void);


HRESULT
STDMETHODCALLTYPE
SK_D3D9_DispatchPresentEx_DeviceEx (IDirect3DDevice9Ex     *This,
                              const RECT                   *pSourceRect,
                              const RECT                   *pDestRect,
                                    HWND                    hDestWindowOverride,
                              const RGNDATA                *pDirtyRegion,
                                    DWORD                   dwFlags,
                                D3D9ExDevice_PresentEx_pfn  D3D9ExPresentExDevice,
                                    SK_D3D9_PresentSource   Source)
{
  bool process = false;

  if (config.render.gl.osd_in_vidcap && (ReadAcquire (&SK_D3D9_LiveWrappedSwapChainsEx)))
  {
    process = (Source == SK_D3D9_PresentSource::Wrapper);
  }

  else
    process = (Source == SK_D3D9_PresentSource::Hook);


  auto CallFunc = [&](void) ->
  HRESULT
  {
    if (Source == SK_D3D9_PresentSource::Hook)
    {
      return
        D3D9ExPresentExDevice ( This, pSourceRect, pDestRect,
                                  hDestWindowOverride, pDirtyRegion,
                                    dwFlags );
    }

    return
      This->PresentEx ( pSourceRect, pDestRect, hDestWindowOverride,
                          pDirtyRegion, dwFlags );
  };


  if (process)
  {
#if 0
    SetThreadIdealProcessor (GetCurrentThread (),       6);
    SetThreadAffinityMask   (GetCurrentThread (), (1 << 7) | (1 << 6));//config.render.framerate.pin_render_thread);
#endif

    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    //SK_D3D9_UpdateRenderStats (This);

    CComPtr <IDirect3DDevice9>    pDev       = nullptr;
    CComPtr <IDirect3DSurface9>   pSurf      = nullptr;
    CComPtr <IDirect3DSwapChain9> pSwapChain = nullptr;

    pDev = This;
    pDev->GetSwapChain (0, &pSwapChain);

    if (SUCCEEDED (This->GetBackBuffer (0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurf)))
    {
      D3DPRESENT_PARAMETERS pparams = { };

      pSwapChain->GetPresentParameters (&pparams);

      rb.device    = pDev;
      rb.swapchain = pSwapChain;

      SK_CEGUI_DrawD3D9 (This, pSwapChain);

      //
      // Update G-Sync; doing this here prevents trying to do this on frames where
      //   the swapchain was resized, which would deadlock the software.
      //
      if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
      {
        NvAPI_D3D9_GetSurfaceHandle (pSurf, &rb.surface);
        rb.gsync_state.update ();
      }
    }

    reinterpret_cast <int &> (rb.api)  = ( static_cast <int> (SK_RenderAPI::D3D9  ) |
                                           static_cast <int> (SK_RenderAPI::D3D9Ex)   );

    SK_BeginBufferSwap ();

    HRESULT hr =
      CallFunc ();


    if (trigger_reset == reset_stage_e::Clear)
    {
      hr =
        SK_EndBufferSwap (hr, pDev);
    }

    else
      hr = D3DERR_DEVICELOST;

    SK_D3D9_EndFrame ();

    return hr;
  }

  else
  {
    return CallFunc ();
  }
}

__declspec (noinline)
HRESULT
WINAPI
D3D9ExDevice_PresentEx ( IDirect3DDevice9Ex *This,
              _In_ const RECT               *pSourceRect,
              _In_ const RECT               *pDestRect,
              _In_       HWND                hDestWindowOverride,
              _In_ const RGNDATA            *pDirtyRegion,
              _In_       DWORD               dwFlags )
{
  if (This == nullptr)
    return E_NOINTERFACE;

  return
    SK_D3D9_DispatchPresentEx_DeviceEx ( This,
                                           pSourceRect, pDestRect,
                                             hDestWindowOverride, pDirtyRegion,
                                               dwFlags, D3D9ExDevice_PresentEx_Original,
                                                 SK_D3D9_PresentSource::Hook );
}


__declspec (noinline)
HRESULT
WINAPI
SK_D3D9_Present (IDirect3DDevice9 *This,
                 _In_ const RECT             *pSourceRect,
                 _In_ const RECT             *pDestRect,
                 _In_       HWND              hDestWindowOverride,
                 _In_ const RGNDATA          *pDirtyRegion)
{
  HRESULT
    hr = S_OK;

  __try                                { hr = SK_D3D9_Trampoline (D3D9Device, Present) 
                                                                   ( This,
                                                                       pSourceRect,
                                                                         pDestRect,
                                                                           hDestWindowOverride,
                                                                             pDirtyRegion ); }
  __except (EXCEPTION_EXECUTE_HANDLER) {                                                     }

  return hr;
}


IDirect3DSurface9* pBackBuffer      = nullptr;
IDirect3DSurface9* pBackBufferCopy  = nullptr;


__declspec (noinline)
HRESULT
WINAPI
D3D9Device_Present ( IDirect3DDevice9 *This,
          _In_ const RECT             *pSourceRect,
          _In_ const RECT             *pDestRect,
          _In_       HWND              hDestWindowOverride,
          _In_ const RGNDATA          *pDirtyRegion )
{
  HRESULT
  STDMETHODCALLTYPE
  SK_D3D9_DispatchPresent_Device (IDirect3DDevice9       *This,
                            const RECT                   *pSourceRect,
                            const RECT                   *pDestRect,
                                  HWND                    hDestWindowOverride,
                            const RGNDATA                *pDirtyRegion,
                                  D3D9Device_Present_pfn  D3D9PresentDevice,
                                  SK_D3D9_PresentSource   Source);

    if (This == nullptr)
      return E_NOINTERFACE;

    return SK_D3D9_DispatchPresent_Device (This, pSourceRect, pDestRect,
                                           hDestWindowOverride, pDirtyRegion,
                                    SK_D3D9_Trampoline (D3D9Device, Present),
                                           SK_D3D9_PresentSource::Hook);
}


#define D3D9_STUB_HRESULT(_Return, _Name, _Proto, _Args)                  \
  __declspec (noinline) _Return STDMETHODCALLTYPE                         \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;          \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)                                   \
        GetProcAddress (backend_dll, szName);                             \
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[   D3D9   ] Unable to locate symbol '%s' in d3d9.dll",       \
          L#_Name);                                                       \
        return E_NOTIMPL;                                                 \
      }                                                                   \
    }                                                                     \
                                                                          \
    /*dll_log.Log (L"[!] %s %s - "                                      */\
             /*L"[Calling Thread: 0x%04x]",                             */\
      /*L#_Name, L#_Proto, GetCurrentThreadId ());                      */\
                                                                          \
    return _default_impl _Args;                                           \
}

#define D3D9_STUB_VOIDP(_Return, _Name, _Proto, _Args)                    \
  __declspec (noinline) _Return STDMETHODCALLTYPE                         \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;          \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)                                   \
        GetProcAddress (backend_dll, szName);                             \
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[   D3D9   ] Unable to locate symbol '%s' in d3d9.dll",       \
          L#_Name);                                                       \
        return nullptr;                                                   \
      }                                                                   \
    }                                                                     \
                                                                          \
    /*dll_log.Log (L"[!] %s %s - "                                      */\
           /*L"[Calling Thread: 0x%04x]",                               */\
      /*L#_Name, L#_Proto, GetCurrentThreadId ());                      */\
                                                                          \
    return _default_impl _Args;                                           \
}

#define D3D9_STUB_VOID(_Return, _Name, _Proto, _Args)                     \
  __declspec (noinline) _Return STDMETHODCALLTYPE                         \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;          \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)                                   \
        GetProcAddress (backend_dll, szName);                             \
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[   D3D9   ] Unable to locate symbol '%s' in d3d9.dll",       \
          L#_Name);                                                       \
        return;                                                           \
      }                                                                   \
    }                                                                     \
                                                                          \
    /*dll_log.Log (L"[!] %s %s - "                                      */\
             /*L"[Calling Thread: 0x%04x]",                             */\
      /*L#_Name, L#_Proto, GetCurrentThreadId ());                      */\
                                                                          \
    _default_impl _Args;                                                  \
}

#define D3D9_STUB_INT(_Return, _Name, _Proto, _Args)                      \
  __declspec (noinline) _Return STDMETHODCALLTYPE                         \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;          \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)                                   \
        GetProcAddress (backend_dll, szName);                             \
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[   D3D9   ] Unable to locate symbol '%s' in d3d9.dll",       \
          L#_Name);                                                       \
        return 0;                                                         \
      }                                                                   \
    }                                                                     \
                                                                          \
    /*dll_log.Log (L"[!] %s %s - "                                      */\
             /*L"[Calling Thread: 0x%04x]",                             */\
      /*L#_Name, L#_Proto, GetCurrentThreadId ());                      */\
                                                                          \
    return _default_impl _Args;                                           \
}

D3D9_STUB_VOIDP   (void*, Direct3DShaderValidatorCreate, (void),
                                                         (    ))

D3D9_STUB_INT     (int,   D3DPERF_BeginEvent, (D3DCOLOR color, LPCWSTR name),
                                                       (color,         name))
D3D9_STUB_INT     (int,   D3DPERF_EndEvent,   (void),          ( ))

D3D9_STUB_INT     (DWORD, D3DPERF_GetStatus,  (void),          ( ))
D3D9_STUB_VOID    (void,  D3DPERF_SetOptions, (DWORD options), (options))

D3D9_STUB_INT     (BOOL,  D3DPERF_QueryRepeatFrame, (void),    ( ))
D3D9_STUB_VOID    (void,  D3DPERF_SetMarker, (D3DCOLOR color, LPCWSTR name),
                                                      (color,         name))
D3D9_STUB_VOID    (void,  D3DPERF_SetRegion, (D3DCOLOR color, LPCWSTR name),
                                                      (color,         name))

HRESULT
STDMETHODCALLTYPE
SK_D3D9_DispatchPresent_Device (IDirect3DDevice9       *This,
                          const RECT                   *pSourceRect,
                          const RECT                   *pDestRect,
                                HWND                    hDestWindowOverride,
                          const RGNDATA                *pDirtyRegion,
                                D3D9Device_Present_pfn  D3D9PresentDevice,
                                SK_D3D9_PresentSource   Source)
{
  bool process = false;

  if (config.render.gl.osd_in_vidcap && (ReadAcquire (&SK_D3D9_LiveWrappedSwapChains) ||
                                         ReadAcquire (&SK_D3D9_LiveWrappedSwapChainsEx)))
  {
    process = (Source == SK_D3D9_PresentSource::Wrapper);
  }

  else
    process = (Source == SK_D3D9_PresentSource::Hook);


  auto CallFunc = [&](void) ->
  HRESULT
  {
    if (Source == SK_D3D9_PresentSource::Hook)
    {
      return
        SK_D3D9_Present ( This,
                            pSourceRect, pDestRect,
                              hDestWindowOverride,
                                pDirtyRegion );
    }

    return This->Present (pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
  };


  if (process)
  {
#if 0
    SetThreadIdealProcessor (GetCurrentThread (),       6);
    SetThreadAffinityMask   (GetCurrentThread (), (1 << 7) | (1 << 6));//config.render.framerate.pin_render_thread);
#endif

    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();


    if (g_D3D9PresentParams.SwapEffect == D3DSWAPEFFECT_FLIPEX)
    {
      HRESULT hr =
        D3D9ExDevice_PresentEx ( static_cast <IDirect3DDevice9Ex *> (This),
                                  nullptr,
                                    nullptr,
                                      nullptr,
                                        nullptr,
                                            D3DPRESENT_FORCEIMMEDIATE |
                                            D3DPRESENT_DONOTWAIT );

      SK_D3D9_EndFrame ();

      return hr;
    }

    //SK_D3D9_UpdateRenderStats (This);

    CComPtr <IDirect3DDevice9>    pDev       = nullptr;
    CComPtr <IDirect3DDevice9Ex>  pDev9Ex    = nullptr;
    CComPtr <IDirect3DSurface9>   pSurf      = nullptr;
    CComPtr <IDirect3DSwapChain9> pSwapChain = nullptr;

    pDev = This;
    pDev->GetSwapChain (0, &pSwapChain);

    if (SUCCEEDED (This->GetBackBuffer (0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurf)))
    {
      D3DPRESENT_PARAMETERS pparams = { };

      pSwapChain->GetPresentParameters (&pparams);

      rb.device    = This;
      rb.swapchain = pSwapChain;

      SK_CEGUI_DrawD3D9 (This, pSwapChain);

      //
      // Update G-Sync; doing this here prevents trying to do this on frames where
      //   the swapchain was resized, which would deadlock the software.
      //
      if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
      {
        NvAPI_D3D9_GetSurfaceHandle (pSurf, &rb.surface);
        rb.gsync_state.update ();
      }
    }

    if (SUCCEEDED (rb.device->QueryInterface <IDirect3DDevice9Ex> (&pDev9Ex)))
    {
      reinterpret_cast <int &> (rb.api)  = ( static_cast <int> (SK_RenderAPI::D3D9  ) |
                                             static_cast <int> (SK_RenderAPI::D3D9Ex)   );
    }

    else
    {
      rb.api = SK_RenderAPI::D3D9;
    }

    SK_BeginBufferSwap ();

    HRESULT hr =
      CallFunc ();

    if (trigger_reset == reset_stage_e::Clear)
    {
      hr =
        SK_EndBufferSwap (hr, pDev);
    }

    else
      hr = D3DERR_DEVICELOST;

    SK_D3D9_EndFrame ();

    return hr;
  }

  else
  {
    return CallFunc ();
  }
}

HRESULT
STDMETHODCALLTYPE
SK_D3D9_DispatchPresent_Chain (IDirect3DSwapChain9  *This,
                         const RECT                 *pSourceRect,
                         const RECT                 *pDestRect,
                               HWND                  hDestWindowOverride,
                         const RGNDATA              *pDirtyRegion,
                               DWORD                 dwFlags,
                               D3D9Swap_Present_pfn  D3D9PresentSwapChain,
                               SK_D3D9_PresentSource Source)
{
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  bool process = false;

  if (config.render.gl.osd_in_vidcap && (ReadAcquire (&SK_D3D9_LiveWrappedSwapChains) ||
                                         ReadAcquire (&SK_D3D9_LiveWrappedSwapChainsEx)))
  {
    process = (Source == SK_D3D9_PresentSource::Wrapper);
  }

  else
    process = (Source == SK_D3D9_PresentSource::Hook);


  auto CallFunc = [&](void) ->
  HRESULT
  {
    if (Source == SK_D3D9_PresentSource::Hook)
      return D3D9PresentSwapChain ( This, pSourceRect,
                                      pDestRect, hDestWindowOverride,
                                        pDirtyRegion, dwFlags );

    else
      return This->Present ( pSourceRect, pDestRect, hDestWindowOverride,
                               pDirtyRegion, dwFlags );
  };


  if (process)
  {
    //SK_D3D9_UpdateRenderStats (This);

    CComPtr <IDirect3DDevice9>   pDev    = nullptr;
    CComPtr <IDirect3DDevice9Ex> pDev9Ex = nullptr;

    if (SUCCEEDED (This->GetDevice (&pDev)))
    {
      CComPtr <IDirect3DSurface9> pSurf = nullptr;

      D3DPRESENT_PARAMETERS pparams = { };

      This->GetPresentParameters (&pparams);

      if (SUCCEEDED (This->GetBackBuffer (0, D3DBACKBUFFER_TYPE_MONO, &pSurf)))
      {
        rb.device    = pDev;
        rb.swapchain = This;

        SK_CEGUI_DrawD3D9 (pDev, This);

        //
        // Update G-Sync; doing this here prevents trying to do this on frames where
        //   the swapchain was resized, which would deadlock the software.
        //
        if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
        {
          NvAPI_D3D9_GetSurfaceHandle (pSurf, &rb.surface);
          rb.gsync_state.update ();
        }
      }
    }

    if (SUCCEEDED (rb.device->QueryInterface <IDirect3DDevice9Ex> (&pDev9Ex)))
    {
      reinterpret_cast <int &> (rb.api)  = ( static_cast <int> (SK_RenderAPI::D3D9  ) |
                                             static_cast <int> (SK_RenderAPI::D3D9Ex)   );
    }

    else
    {
      rb.api = SK_RenderAPI::D3D9;
    }


    if ( (dwFlags & D3DPRESENT_DONOTWAIT) && SK_GetFramesDrawn () )
    {
      HRESULT hr =
        CallFunc ();

      SK_D3D9_EndFrame ();

      // On a failure (i.e. Must wait), skip the frame
      //
      //  Only do input processing and UI drawing on real frames
      if (SUCCEEDED (hr))
      {
        SK_BeginBufferSwap ();

        hr =
          SK_EndBufferSwap (hr, pDev);
      }

      return hr;
    }



    SK_BeginBufferSwap ();

    HRESULT hr =
      CallFunc ();

    if (pDev != nullptr)
    {
      if (trigger_reset == reset_stage_e::Clear)
      {
        hr =
          SK_EndBufferSwap (hr, pDev);
      }

      SK_D3D9_EndFrame ();

      return hr;
    }

    // pDev should be nullptr ... I'm not exactly sure what I was trying to accomplish? :P
    if (trigger_reset == reset_stage_e::Clear)
    {
      hr =
        SK_EndBufferSwap (hr, pDev);
    }

    else
      hr = D3DERR_DEVICELOST;

    SK_D3D9_EndFrame ();

    return hr;
  }

  else
  {
    return CallFunc ();
  }
}

  __declspec (noinline)
  HRESULT
  WINAPI
  D3D9Swap_Present ( IDirect3DSwapChain9 *This,
          _In_ const RECT                *pSourceRect,
          _In_ const RECT                *pDestRect,
          _In_       HWND                 hDestWindowOverride,
          _In_ const RGNDATA             *pDirtyRegion,
          _In_       DWORD                dwFlags )
  {
    if (This == nullptr)
      return E_NOINTERFACE;

    return SK_D3D9_DispatchPresent_Chain (This, pSourceRect, pDestRect,
                                          hDestWindowOverride, pDirtyRegion,
                                          dwFlags,
                      SK_D3D9_Trampoline (D3D9Swap, Present),
                                          SK_D3D9_PresentSource::Hook);
  }

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateAdditionalSwapChain_Override ( IDirect3DDevice9       *This,
                                         D3DPRESENT_PARAMETERS  *pPresentationParameters,
                                         IDirect3DSwapChain9   **pSwapChain )
{
  dll_log.Log (L"[   D3D9   ] [!] %s (%ph, %ph, %ph) - "
    L"%s",
    L"IDirect3DDevice9::CreateAdditionalSwapChain", This,
      pPresentationParameters, pSwapChain,
        SK_SummarizeCaller ().c_str ()
  );

  HRESULT    hr = E_FAIL;
  D3D9_CALL (hr,D3D9Device_CreateAdditionalSwapChain_Original ( This,
                                                                  pPresentationParameters,
                                                                    pSwapChain ) );


  if (! LocalHook_D3D9PresentSwap.active)
  {
    if (SUCCEEDED (hr))
    {
      void** vftable = ReadAcquire (&SK_D3D9_LiveWrappedSwapChains) ?
        (void **)&(*(IWrapDirect3DSwapChain9 **)pSwapChain)->pReal :
        (void **)pSwapChain;

      if (vftable != nullptr && vftable [3] != nullptr)
      {
        D3D9_INTERCEPT ( vftable, 3,
                         "IDirect3DSwapChain9::Present",
                          D3D9Swap_Present,
                          D3D9Swap_Present_Original,
                          D3D9Swap_Present_pfn );

        SK_Hook_TargetFromVFTable (
          LocalHook_D3D9PresentSwap,
            vftable, 3 );
      }
    }
  }

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9TestCooperativeLevel_Override (IDirect3DDevice9 *This)
{
  if (trigger_reset == reset_stage_e::Initiate)
  {
    trigger_reset = reset_stage_e::Respond;

    return D3DERR_DEVICELOST;
  }


  else if (trigger_reset == reset_stage_e::Respond)
  {
    return D3DERR_DEVICENOTRESET;
  }

  return D3D9Device_TestCooperativeLevel_Original (This);
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9BeginScene_Override (IDirect3DDevice9 *This)
{
  if (tex_mgr.init)
  {
    if (tex_mgr.injector.hasPendingLoads ())
      tex_mgr.loadQueuedTextures ();
  }

  HRESULT hr =
    D3D9Device_BeginScene_Original (This);

  //D3D9_CALL (hr, D3D9Begincene_Original (This));

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9EndScene_Override (IDirect3DDevice9 *This)
{
  //dll_log.Log (L"[   D3D9   ] [!] %s (%ph) - "
    //L"[Calling Thread: 0x%04x]",
    //L"IDirect3DDevice9::EndScene", This,
    //GetCurrentThreadId ()
  //);

  SK_D3D9_EndScene ();

  HRESULT hr = D3D9Device_EndScene_Original (This);

  if (tex_mgr.init)
  {
    if (tex_mgr.injector.hasPendingLoads ())
      tex_mgr.loadQueuedTextures ();
  }

  return hr;
}

void
SK_D3D9_HookReset (IDirect3DDevice9 *pDev)
{
  static LPVOID vftable_16  = nullptr;
  static LPVOID vftable_132 = nullptr;

  void** vftable =
    *(reinterpret_cast <void ***> (*&pDev));

  if (D3D9Device_Reset_Original != nullptr)
  {
    if ( config.render.d3d9.hook_type == 0              &&
         (! config.compatibility.d3d9.hook_reset_vftbl) &&
            config.compatibility.d3d9.rehook_reset )
    {
      //dll_log.Log (L"Rehooking IDirect3DDevice9::Present (...)");

      if (MH_OK == SK_RemoveHook (vftable [16]))
      {
        D3D9Device_Reset_Original = nullptr;
      }

      else
      {
        dll_log.Log ( L"[   D3D9   ] Altered vftable detected, re-hooking "
                      L"IDirect3DDevice9::Reset (...)!" );
        if (MH_OK == SK_RemoveHook (vftable_16))
          D3D9Device_Reset_Original = nullptr;
      }
    }
  }

  int hook_type =
    config.render.d3d9.hook_type;

  if (config.compatibility.d3d9.hook_reset_vftbl)
      config.render.d3d9.hook_type = 1;

  if ( D3D9Device_Reset_Original == nullptr ||
       config.compatibility.d3d9.rehook_reset )
  {
    if (! LocalHook_D3D9Reset.active)
    {
      D3D9_INTERCEPT ( &pDev, 16,
                       "IDirect3DDevice9::Reset",
                        D3D9Reset_Override,
                        D3D9Device_Reset_Original,
                        D3D9Device_Reset_pfn );

      vftable_16 = vftable [16];

      SK_Hook_TargetFromVFTable (
        LocalHook_D3D9Reset,
          (void **)&pDev, 16 );
    }
  }

  config.render.d3d9.hook_type = hook_type;


  if (! config.apis.d3d9ex.hook)
    return;

  //
  // D3D9Ex Specific Stuff
  //

  CComPtr <IDirect3DDevice9Ex> pDevEx = nullptr;

  if (SUCCEEDED ((static_cast <IUnknown *> (pDev))->QueryInterface <IDirect3DDevice9Ex> (&pDevEx)))
  {
    vftable =
      *( reinterpret_cast <void ***> (*&pDevEx) );

    if (D3D9ExDevice_ResetEx_Original != nullptr)
    {
      if ( config.render.d3d9.hook_type == 0              &&
           (! config.compatibility.d3d9.hook_reset_vftbl) &&
              config.compatibility.d3d9.rehook_reset )
      {
        //dll_log.Log (L"Rehooking IDirect3DDevice9Ex::ResetEx (...)");

        if (MH_OK == SK_RemoveHook (vftable [132]))
        {
          D3D9ExDevice_ResetEx_Original = nullptr;
        }

        else
        {
          dll_log.Log ( L"[   D3D9   ] Altered vftable detected, re-hooking "
                        L"IDirect3DDevice9Ex::ResetEx (...)!" );
          if (MH_OK == SK_RemoveHook (vftable_132))
            D3D9ExDevice_ResetEx_Original = nullptr;
        }
      }
    }

    if (config.compatibility.d3d9.hook_reset_vftbl)
        config.render.d3d9.hook_type = 1;

    if ( SK_D3D9_Trampoline (D3D9ExDevice, ResetEx) == nullptr ||
         config.compatibility.d3d9.rehook_reset )
    {
      if (! LocalHook_D3D9ResetEx.active)
      {
        D3D9_INTERCEPT ( &pDev, 132,
                         "IDirect3DDevice9Ex::ResetEx",
                          D3D9ResetEx,
                          D3D9ExDevice_ResetEx_Original,
                          D3D9ExDevice_ResetEx_pfn );

        vftable_132 = vftable [132];

        SK_Hook_TargetFromVFTable (
          LocalHook_D3D9ResetEx,
            (void **)&pDev, 132 );
      }
    }

    config.render.d3d9.hook_type = hook_type;
  }
}

void
WINAPI
SK_D3D9_HookPresent (IDirect3DDevice9 *pDev)
{
  static LPVOID vftable_17  = nullptr;
  static LPVOID vftable_121 = nullptr;

  void** vftable =
    *( reinterpret_cast <void ***> (*&pDev) );

  if (D3D9Device_Present_Original != nullptr)
  {
    if ( config.render.d3d9.hook_type == 0                &&
         (! config.compatibility.d3d9.hook_present_vftbl) &&
            config.compatibility.d3d9.rehook_present )
    {
      //dll_log.Log (L"Rehooking IDirect3DDevice9::Present (...)");

      if (MH_OK == SK_RemoveHook (vftable [17]))
      {
        D3D9Device_Present_Original = nullptr;
      }

      else
      {
        dll_log.Log ( L"[   D3D9   ] Altered vftable detected, re-hooking "
                      L"IDirect3DDevice9::Present (...)!" );
        if (MH_OK == SK_RemoveHook (vftable_17))
          D3D9Device_Present_Original = nullptr;
      }
    }
  }

  int hook_type =
    config.render.d3d9.hook_type;

  if (config.compatibility.d3d9.hook_present_vftbl)
      config.render.d3d9.hook_type = 1;

  if ( D3D9Device_Present_Original == nullptr ||
       config.compatibility.d3d9.rehook_present )
  {
    if (! LocalHook_D3D9Present.active)
    {
      D3D9_INTERCEPT ( &pDev, 17,
                       "IDirect3DDevice9::Present",
                        D3D9Device_Present,
                        D3D9Device_Present_Original,
                        D3D9Device_Present_pfn );
      MH_ApplyQueued ();

      vftable_17 = vftable [17];

      SK_Hook_TargetFromVFTable (
        LocalHook_D3D9Present,
          (void **)&pDev, 17 );
    }
  }

  config.render.d3d9.hook_type = hook_type;


  if (! config.apis.d3d9ex.hook)
    return;

  CComPtr <IDirect3DDevice9Ex> pDevEx = nullptr;

  if (SUCCEEDED (static_cast <IUnknown *> (pDev)->QueryInterface <IDirect3DDevice9Ex> (&pDevEx)))
  {
    vftable =
      *( reinterpret_cast <void ***> (*&pDevEx) );

    if (D3D9ExDevice_PresentEx_Original != nullptr)
    {
      if (  config.render.d3d9.hook_type == 0             &&
         (! config.compatibility.d3d9.hook_present_vftbl) &&
            config.compatibility.d3d9.rehook_present )
      {
        //dll_log.Log (L"Rehooking IDirect3DDevice9Ex::PresentEx (...)");

        if (MH_OK == SK_RemoveHook (vftable [121]))
        {
          D3D9ExDevice_PresentEx_Original = nullptr;
        }

        else
        {
          dll_log.Log ( L"[   D3D9   ] Altered vftable detected, re-hooking "
                        L"IDirect3DDevice9Ex::PresentEx (...)!" );
          if (MH_OK == SK_RemoveHook (vftable_121))
            D3D9ExDevice_PresentEx_Original = nullptr;
        }
      }
    }

    if (config.compatibility.d3d9.hook_present_vftbl)
      config.render.d3d9.hook_type = 1;

    if (D3D9ExDevice_PresentEx_Original == nullptr || config.compatibility.d3d9.rehook_present)
    {
      if (! LocalHook_D3D9PresentEx.active)
      {
        //
        // D3D9Ex Specific Stuff
        //
        D3D9_INTERCEPT ( &pDevEx, 121,
                           "IDirect3DDevice9Ex::PresentEx",
                            D3D9ExDevice_PresentEx,
                            D3D9ExDevice_PresentEx_Original,
                            D3D9ExDevice_PresentEx_pfn );

        vftable_121 = vftable [121];

        SK_Hook_TargetFromVFTable (
          LocalHook_D3D9PresentEx,
            (void **)&pDevEx, 121 );
      }
    }

    config.render.d3d9.hook_type = hook_type;
  }


  if (! LocalHook_D3D9PresentSwap.active)
  {
    CComPtr <IDirect3DSwapChain9> pSwapChain = nullptr;

    if (SUCCEEDED (pDev->GetSwapChain (0, &pSwapChain)))
    {
      void** vftable_ = ReadAcquire (&SK_D3D9_LiveWrappedSwapChains) ?
        (void **)&((IWrapDirect3DSwapChain9 *)pSwapChain.p)->pReal :
        (void **)&pSwapChain;

      if (vftable_ != nullptr)
      {
        D3D9_INTERCEPT ( vftable_, 3,
                         "IDirect3DSwapChain9::Present",
                          D3D9Swap_Present,
                          D3D9Swap_Present_Original,
                          D3D9Swap_Present_pfn );

        SK_Hook_TargetFromVFTable (
          LocalHook_D3D9PresentSwap,
            vftable_, 3 );
      }
    }
  }
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D9Reset_Pre ( IDirect3DDevice9      *This,
                D3DPRESENT_PARAMETERS *pPresentationParameters,
                D3DDISPLAYMODEEX      *pFullscreenDisplayMode )
{
  UNREFERENCED_PARAMETER (This);
  UNREFERENCED_PARAMETER (pFullscreenDisplayMode);

  if (ReadAcquire (&ImGui_Init))
  {
    ResetCEGUI_D3D9                       (nullptr);
    ImGui_ImplDX9_InvalidateDeviceObjects (pPresentationParameters);
  }

  if (tex_mgr.init)
  {
    if (tex_mgr.injector.hasPendingLoads ())
      tex_mgr.loadQueuedTextures ();

    tex_mgr.reset ();

  //need_reset.textures = false;

    tex_mgr.resetUsedTextures ( );

  //need_reset.graphics = false;
  }


  //if (This == SK_GetCurrentRenderBackend ().device)
  //{
    known_objs.clear ();

    last_frame.clear ();
    tracked_rt.clear ();
    tracked_vs.clear ();
    tracked_ps.clear ();
    tracked_vb.clear ();

    // Clearing the tracked VB only clears state, it doesn't
    //   get rid of any data pointers.
    //
    //  (WE DID NOT QUERY THIS FROM THE D3D RUNTIME, DO NOT RELEASE)
    tracked_vb.vertex_buffer = nullptr;
    tracked_vb.wireframe     = false;
    tracked_vb.wireframes.clear ();
    // ^^^^ This is stupid, add a reset method.

    Shaders.vertex.clear ();
    Shaders.pixel.clear  ();
  //}
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D9Reset_Post ( IDirect3DDevice9      *This,
                 D3DPRESENT_PARAMETERS *pPresentationParameters,
                 D3DDISPLAYMODEEX      *pFullscreenDisplayMode )
{
  UNREFERENCED_PARAMETER (pFullscreenDisplayMode);

  if (ImGui_ImplDX9_Init ( (void *)pPresentationParameters->hDeviceWindow,
                             This,
                               pPresentationParameters) )
  {
    InterlockedExchange ( &ImGui_Init, TRUE );
  }

  trigger_reset       = reset_stage_e::Clear;
  request_mode_change = mode_change_request_e::None;
}


__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9Reset_Override ( IDirect3DDevice9      *This,
                     D3DPRESENT_PARAMETERS *pPresentationParameters )
{
  if (This == nullptr)
    return E_NOINTERFACE;

 if (pPresentationParameters == nullptr)
   return D3DERR_INVALIDCALL;

  dll_log.Log ( L"[   D3D9   ] [!] %s (%ph, %ph) - "
                L"%s",
                L"IDirect3DDevice9::Reset", This, pPresentationParameters,
                  SK_SummarizeCaller ().c_str ()
  );

  if (ReadAcquire (&__d3d9_ready))
  {
    SK_InitWindow ( pPresentationParameters->hDeviceWindow,
                 (! pPresentationParameters->Windowed) );

    SK_D3D9_SetFPSTarget    (      pPresentationParameters);
    SK_SetPresentParamsD3D9 (This, pPresentationParameters);
  }

#if 0
  // The texture manager built-in to SK is derived from these ...
  //   until those projects are modified to use THIS texture manager,
  //     they need special treatment.
  if ( GetModuleHandle (L"tzfix.dll") == NULL &&
       GetModuleHandle (L"tsfix.dll") == NULL ) {
    sk::d3d9::tex_mgr.reset ();
  }
#endif

  HRESULT hr = E_FAIL;

  if (ReadAcquire (&__d3d9_ready))
  {
    D3D9Reset_Pre ( This, pPresentationParameters, nullptr );
  }

  D3D9_CALL (hr, D3D9Device_Reset_Original (This,
                                              pPresentationParameters));

  if (SUCCEEDED (hr))
  {
    if (config.compatibility.d3d9.rehook_reset)
      SK_D3D9_HookReset   (This);

    if ( config.compatibility.d3d9.rehook_present ||
         D3D9Device_Present_Original == nullptr )
      SK_D3D9_HookPresent (This);

    if (ReadAcquire (&__d3d9_ready))
    {
      SK_SetPresentParamsD3D9 (This, pPresentationParameters);
      D3D9Reset_Post          (This, pPresentationParameters, nullptr);
    }
  }

  else
  {
    if (ReadAcquire (&__d3d9_ready))
    {
      D3D9Reset_Pre ( This, pPresentationParameters, nullptr );
    }
  }

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9ResetEx ( IDirect3DDevice9Ex    *This,
              D3DPRESENT_PARAMETERS *pPresentationParameters,
              D3DDISPLAYMODEEX      *pFullscreenDisplayMode )
{
  if (This == nullptr || pPresentationParameters == nullptr)
    return E_NOINTERFACE;

  dll_log.Log ( L"[   D3D9   ] [!] %s (%ph, %ph, %ph) - "
                L"%s",
                  L"IDirect3DDevice9Ex::ResetEx",
                    This, pPresentationParameters, pFullscreenDisplayMode,
                      SK_SummarizeCaller ().c_str () );

  if (ReadAcquire (&__d3d9_ready))
  {
    SK_InitWindow ( pPresentationParameters->hDeviceWindow,
                 (! pPresentationParameters->Windowed) );

    SK_D3D9_SetFPSTarget    (      pPresentationParameters, pFullscreenDisplayMode);
    SK_SetPresentParamsD3D9 (This, pPresentationParameters);
  }

  CComPtr <IDirect3DDevice9> pDev = nullptr;

  if (SUCCEEDED (static_cast <IUnknown *> (This)->QueryInterface <IDirect3DDevice9> (&pDev)))
  {
    if (config.compatibility.d3d9.rehook_reset)
      SK_D3D9_HookReset   (pDev);

    if (config.compatibility.d3d9.rehook_present)
      SK_D3D9_HookPresent (pDev);

    pDev.Release ();
  }

  if (ReadAcquire (&__d3d9_ready))
  {
    D3D9Reset_Pre ( This, pPresentationParameters, pFullscreenDisplayMode );
  }

  HRESULT    hr = E_FAIL;
  D3D9_CALL (hr, D3D9ExDevice_ResetEx_Original ( This,
                                                   pPresentationParameters,
                                                     pFullscreenDisplayMode ));

  if (SUCCEEDED (hr))
  {
    if (ReadAcquire (&__d3d9_ready))
    {
      SK_SetPresentParamsD3D9 (This, pPresentationParameters);
      D3D9Reset_Post          (This, pPresentationParameters, pFullscreenDisplayMode);
    }
  }

  else
  {
    if (ReadAcquire (&__d3d9_ready))
    {
      D3D9Reset_Pre ( This, pPresentationParameters, pFullscreenDisplayMode );
    }
  }


  return hr;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D9SetGammaRamp_Override ( IDirect3DDevice9 *This,
                 _In_       UINT              iSwapChain,
                 _In_       DWORD             Flags,
                 _In_ const D3DGAMMARAMP     *pRamp )
{
  dll_log.Log (L"[   D3D9   ] SetGammaRamp (...) ");

  return
    D3D9Device_SetGammaRamp_Original ( This,
                                         iSwapChain,
                                           Flags,
                                             pRamp );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9DrawPrimitive_Override ( IDirect3DDevice9 *This,
                             D3DPRIMITIVETYPE  PrimitiveType,
                             UINT              StartVertex,
                             UINT              PrimitiveCount )
{
  ++draw_state.draws;
  ++draw_state.draw_count;

  bool wireframe = false;

  if (SK_D3D9_ShouldSkipRenderPass (PrimitiveType, PrimitiveCount, StartVertex, wireframe))
    return S_OK;

  HRESULT hr =
    D3D9Device_DrawPrimitive_Original ( This,
                                          PrimitiveType,
                                            StartVertex,
                                              PrimitiveCount );

  if (wireframe)
    This->SetRenderState (D3DRS_FILLMODE, D3DFILL_SOLID);

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9DrawIndexedPrimitive_Override ( IDirect3DDevice9 *This,
                                    D3DPRIMITIVETYPE  Type,
                                    INT               BaseVertexIndex,
                                    UINT              MinVertexIndex,
                                    UINT              NumVertices,
                                    UINT              startIndex,
                                    UINT              primCount )
{
  ++draw_state.draws;
  ++draw_state.draw_count;

  bool wireframe = false;

  if (SK_D3D9_ShouldSkipRenderPass (Type, primCount, startIndex, wireframe))
    return S_OK;

  HRESULT hr =
    D3D9Device_DrawIndexedPrimitive_Original ( This,
                                                 Type,
                                                   BaseVertexIndex,
                                                     MinVertexIndex,
                                                       NumVertices,
                                                         startIndex,
                                                           primCount );

  if (wireframe)
    This->SetRenderState (D3DRS_FILLMODE, D3DFILL_SOLID);

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9DrawPrimitiveUP_Override (       IDirect3DDevice9 *This,
                                     D3DPRIMITIVETYPE  PrimitiveType,
                                     UINT              PrimitiveCount,
                               const void             *pVertexStreamZeroData,
                                     UINT              VertexStreamZeroStride )
{
  ++draw_state.draws;
  ++draw_state.draw_count;

  bool wireframe = false;

  if (SK_D3D9_ShouldSkipRenderPass (PrimitiveType, PrimitiveCount, 0, wireframe))
    return S_OK;

  HRESULT hr =
    D3D9Device_DrawPrimitiveUP_Original ( This,
                                            PrimitiveType,
                                              PrimitiveCount,
                                                pVertexStreamZeroData,
                                                  VertexStreamZeroStride );

  if (wireframe)
    This->SetRenderState (D3DRS_FILLMODE, D3DFILL_SOLID);

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9DrawIndexedPrimitiveUP_Override (       IDirect3DDevice9 *This,
                                            D3DPRIMITIVETYPE  PrimitiveType,
                                            UINT              MinVertexIndex,
                                            UINT              NumVertices,
                                            UINT              PrimitiveCount,
                                      const void             *pIndexData,
                                            D3DFORMAT         IndexDataFormat,
                                      const void             *pVertexStreamZeroData,
                                            UINT              VertexStreamZeroStride )
{
  ++draw_state.draws;
  ++draw_state.draw_count;

  bool wireframe = false;

  if (SK_D3D9_ShouldSkipRenderPass (PrimitiveType, PrimitiveCount, MinVertexIndex, wireframe))
    return S_OK;

  HRESULT hr =
    D3D9Device_DrawIndexedPrimitiveUP_Original (
      This,
        PrimitiveType,
          MinVertexIndex,
            NumVertices,
              PrimitiveCount,
                pIndexData,
                  IndexDataFormat,
                    pVertexStreamZeroData,
                      VertexStreamZeroStride );

  if (wireframe)
    This->SetRenderState (D3DRS_FILLMODE, D3DFILL_SOLID);

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9GetTexture_Override ( IDirect3DDevice9      *This,
                     _In_ DWORD                  Stage,
                    _Out_ IDirect3DBaseTexture9 *pTexture )
{
//  dll_log.Log (L"Impure Get Texture");

  return
    D3D9Device_GetTexture_Original ( This,
                                       Stage,
                                         pTexture );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetTexture_Override ( IDirect3DDevice9      *This,
                    _In_  DWORD                  Stage,
                    _In_  IDirect3DBaseTexture9 *pTexture )
{
  return
    D3D9Device_SetTexture_Original ( This,
                                       Stage,
                                         pTexture );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetSamplerState_Override ( IDirect3DDevice9    *This,
                               DWORD                Sampler,
                               D3DSAMPLERSTATETYPE  Type,
                               DWORD                Value )
{
  return
    D3D9Device_SetSamplerState_Original ( This,
                                            Sampler,
                                              Type,
                                                Value );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetViewport_Override (       IDirect3DDevice9 *This,
                           CONST D3DVIEWPORT9     *pViewport )
{
#if 0
  if (SK_GetCurrentGameID () == SK_GAME_ID::YS_Seven)
  {
    D3DVIEWPORT9 scaled_vp = *pViewport;

    scaled_vp.Width  <<= 1;
    scaled_vp.Height <<= 1;

    float left_ndc = 2.0f * ((float)pViewport->X / pViewport->Width)  - 1.0f;
    float top_ndc  = 2.0f * ((float)pViewport->Y / pViewport->Height) - 1.0f;

    scaled_vp.Y = (LONG)((top_ndc  * scaled_vp.Height + scaled_vp.Height) / 2.0f);
    scaled_vp.X = (LONG)((left_ndc * scaled_vp.Width  + scaled_vp.Width)  / 2.0f);

    return
      D3D9SetViewport_Original ( This,
                                   &scaled_vp );
  }
#endif

  return
    D3D9Device_SetViewport_Original ( This,
                                        pViewport );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetRenderState_Override ( IDirect3DDevice9   *This,
                              D3DRENDERSTATETYPE  State,
                              DWORD               Value )
{
  return
    D3D9Device_SetRenderState_Original ( This,
                                           State,
                                             Value );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShaderConstantF_Override (       IDirect3DDevice9 *This,
                                              UINT              StartRegister,
                                        CONST float            *pConstantData,
                                              UINT              Vector4fCount )
{
  return
    D3D9Device_SetVertexShaderConstantF_Original ( This,
                                                     StartRegister,
                                                       pConstantData,
                                                         Vector4fCount );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShaderConstantF_Override (       IDirect3DDevice9 *This,
                                             UINT              StartRegister,
                                       CONST float            *pConstantData,
                                             UINT              Vector4fCount )
{
  return
    SK_D3D9_Trampoline (D3D9Device, SetPixelShaderConstantF)
                         ( This,
                             StartRegister,
                               pConstantData,
                                 Vector4fCount );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShader_Override ( IDirect3DDevice9      *This,
                              IDirect3DPixelShader9 *pShader )
{
  HRESULT hr =
    SK_D3D9_Trampoline (D3D9Device, SetPixelShader)
                         ( This,
                             pShader );

  if (SUCCEEDED (hr))
  {
    SK_D3D9_SetPixelShader (This, pShader);
  }

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShader_Override ( IDirect3DDevice9       *This,
                               IDirect3DVertexShader9 *pShader )
{
  HRESULT hr =
    D3D9Device_SetVertexShader_Original ( This,
                                            pShader );

  if (SUCCEEDED (hr))
  {
    SK_D3D9_SetVertexShader (This, pShader);
  }

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetScissorRect_Override ( IDirect3DDevice9 *This,
                        const RECT             *pRect)
{
  return
    D3D9Device_SetScissorRect_Original ( This,
                                           pRect );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateTexture_Override ( IDirect3DDevice9   *This,
                             UINT                Width,
                             UINT                Height,
                             UINT                Levels,
                             DWORD               Usage,
                             D3DFORMAT           Format,
                             D3DPOOL             Pool,
                             IDirect3DTexture9 **ppTexture,
                             HANDLE             *pSharedHandle )
{
  return
    D3D9Device_CreateTexture_Original ( This,
                                          Width, Height, Levels,
                                            Usage, Format, Pool,
                                              ppTexture, pSharedHandle );
}


std::unordered_set <IDirect3DVertexBuffer9 *>        ffxiii_dynamic;
std::unordered_map <IDirect3DVertexBuffer9 *, ULONG> ffxiii_dynamic_updates;

using D3D9VertexBuffer_Lock_pfn = HRESULT (STDMETHODCALLTYPE *)( IDirect3DVertexBuffer9 *This,
                                                                 UINT                    OffsetToLock,
                                                                 UINT                    SizeToLock,
                                                                 void**                  ppbData,
                                                                 DWORD                   Flags );

D3D9VertexBuffer_Lock_pfn D3D9VertexBuffer_Lock_Original = nullptr;

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9VertexBuffer_Lock_Override ( IDirect3DVertexBuffer9 *This,
                                 UINT                    OffsetToLock,
                                 UINT                    SizeToLock,
                                 void**                  ppbData,
                                 DWORD                   Flags )
{
  SK_AutoCriticalSection auto_cs (&cs_vb);

  if (ffxiii_dynamic.count (This))
  {
    ULONG current_frame = SK_GetFramesDrawn ();
    DWORD dwFlags       = D3DLOCK_NOOVERWRITE;

    if (ffxiii_dynamic_updates [This] != current_frame)
    {
      // Discard each frame, and no-overwrite updates mid-frame
      dwFlags                       = D3DLOCK_DISCARD;
      ffxiii_dynamic_updates [This] = current_frame;
    }

    return D3D9VertexBuffer_Lock_Original (This, OffsetToLock, SizeToLock, ppbData, dwFlags);
  }

  else
    return D3D9VertexBuffer_Lock_Original (This, OffsetToLock, SizeToLock, ppbData, Flags);
}


__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateVertexBuffer_Override
(
  _In_  IDirect3DDevice9        *This,
  _In_  UINT                     Length,
  _In_  DWORD                    Usage,
  _In_  DWORD                    FVF,
  _In_  D3DPOOL                  Pool,
  _Out_ IDirect3DVertexBuffer9 **ppVertexBuffer,
  _In_  HANDLE                  *pSharedHandle )
{
  static bool ffxiii =
    GetModuleHandle (L"ffxiiiimg.exe") != nullptr;

  if (ffxiii)
  {
    // This is the UI vertex buffer
    //if (Length == 358400)
    if (Length >= 10240)
    {
      Usage = D3DUSAGE_DYNAMIC | Usage | D3DUSAGE_WRITEONLY;
      Pool  = D3DPOOL_DEFAULT;
    }
  }


  HRESULT hr =
    D3D9Device_CreateVertexBuffer_Original ( This,
                                               Length, Usage,
                                               FVF,    Pool,
                                                 ppVertexBuffer,
                                                   pSharedHandle );

  if (SUCCEEDED (hr) && ffxiii)
  {
    SK_AutoCriticalSection auto_cs (&cs_vb);

    if (Length >= 10240)
    {
      static bool hooked = false;

      if (! hooked)
      {
        hooked = true;

        D3D9_INTERCEPT ( ppVertexBuffer, 11,
                         "IDirect3DVertexBuffer9::Lock",
                          D3D9VertexBuffer_Lock_Override,
                          D3D9VertexBuffer_Lock_Original,
                          D3D9VertexBuffer_Lock_pfn );
        SK_ApplyQueuedHooks ( );
      }

      ffxiii_dynamic.emplace (*ppVertexBuffer);
      ffxiii_dynamic_updates [*ppVertexBuffer] = 0;
    }

    else
    {
      ffxiii_dynamic.erase   (*ppVertexBuffer);
      ffxiii_dynamic_updates [*ppVertexBuffer] = 0;
    }
  }

  if (SUCCEEDED (hr))
  {
    SK_AutoCriticalSection auto_cs (&cs_vb);

    if (Usage & D3DUSAGE_DYNAMIC)
      known_objs.dynamic_vbs.emplace (*ppVertexBuffer);
    else
      known_objs.static_vbs.emplace  (*ppVertexBuffer);
  }

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetStreamSource_Override
(
  IDirect3DDevice9       *This,
  UINT                    StreamNumber,
  IDirect3DVertexBuffer9 *pStreamData,
  UINT                    OffsetInBytes,
  UINT                    Stride )
{
  HRESULT hr =
    D3D9Device_SetStreamSource_Original ( This,
                                            StreamNumber,
                                              pStreamData,
                                                OffsetInBytes,
                                                  Stride );

  if (pStreamData != nullptr)
  {
    if (SUCCEEDED (hr))
    {
      SK_AutoCriticalSection auto_cs (&cs_vb);

      if (known_objs.dynamic_vbs.count (pStreamData))
        last_frame.vertex_buffers.dynamic.emplace (pStreamData);
      else if (known_objs.static_vbs.count (pStreamData))
        last_frame.vertex_buffers.immutable.emplace (pStreamData);

      else
      {
        D3DVERTEXBUFFER_DESC   desc = { };
        pStreamData->GetDesc (&desc);

        if (desc.Usage & D3DUSAGE_DYNAMIC)
        {
                         known_objs.dynamic_vbs.emplace (pStreamData);
          last_frame.vertex_buffers.dynamic.emplace     (pStreamData);
        }

        else
        {
                        known_objs.static_vbs.emplace (pStreamData);
          last_frame.vertex_buffers.immutable.emplace (pStreamData);
        }
      }

      if (StreamNumber == 0)
        vb_stream0 = pStreamData;
    }
  }

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetStreamSourceFreq_Override
(
  IDirect3DDevice9 *This,
  UINT              StreamNumber,
  UINT              FrequencyParameter )
{
  SK_AutoCriticalSection auto_cs (&cs_vb);

  if (StreamNumber == 0 && FrequencyParameter & D3DSTREAMSOURCE_INDEXEDDATA)
  {
    draw_state.instances = ( FrequencyParameter & ( ~D3DSTREAMSOURCE_INDEXEDDATA ) );
  }

  if (StreamNumber == 1 && FrequencyParameter & D3DSTREAMSOURCE_INSTANCEDATA)
  {
  }

  return
    D3D9Device_SetStreamSourceFreq_Original ( This,
                                                StreamNumber,
                                                  FrequencyParameter );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetFVF_Override ( IDirect3DDevice9 *This,
                      DWORD             FVF )
{
  return
    D3D9Device_SetFVF_Original ( This,
                                   FVF );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexDeclaration_Override ( IDirect3DDevice9            *This,
                                    IDirect3DVertexDeclaration9 *pDecl )
{
  return
    D3D9Device_SetVertexDeclaration_Original ( This,
                                                 pDecl );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateVertexDeclaration_Override
(
        IDirect3DDevice9             *This,
  CONST D3DVERTEXELEMENT9            *pVertexElements,
        IDirect3DVertexDeclaration9 **ppDecl )
{
  return
    D3D9Device_CreateVertexDeclaration_Original ( This,
                                                    pVertexElements,
                                                      ppDecl );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateRenderTarget_Override (IDirect3DDevice9     *This,
                                 UINT                  Width,
                                 UINT                  Height,
                                 D3DFORMAT             Format,
                                 D3DMULTISAMPLE_TYPE   MultiSample,
                                 DWORD                 MultisampleQuality,
                                 BOOL                  Lockable,
                                 IDirect3DSurface9   **ppSurface,
                                 HANDLE               *pSharedHandle)
{
  if (SK_GetCurrentGameID () == SK_GAME_ID::YS_Seven)
  {
    CComPtr <IDirect3DSwapChain9> pSwapChain;

    if (SUCCEEDED (This->GetSwapChain (0, &pSwapChain)))
    {
      D3DPRESENT_PARAMETERS pparams = { };
      pSwapChain->GetPresentParameters (&pparams);

      if (Format == D3DFMT_R5G6B5 && ( Width == pparams.BackBufferWidth ) )
      {
        Format = D3DFMT_X8R8G8B8;
#if 0
        Width  <<= 1;
        Height <<= 1;
#endif
      }
    }
  }

  return
    D3D9Device_CreateRenderTarget_Original ( This,
                                               Width, Height,
                                                 Format,
                                                   MultiSample, MultisampleQuality,
                                                     Lockable,
                                                       ppSurface, pSharedHandle );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateDepthStencilSurface_Override ( IDirect3DDevice9     *This,
                                         UINT                  Width,
                                         UINT                  Height,
                                         D3DFORMAT             Format,
                                         D3DMULTISAMPLE_TYPE   MultiSample,
                                         DWORD                 MultisampleQuality,
                                         BOOL                  Discard,
                                         IDirect3DSurface9   **ppSurface,
                                         HANDLE               *pSharedHandle )
{
  if (SK_GetCurrentGameID () == SK_GAME_ID::YS_Seven)
  {
    CComPtr <IDirect3DSwapChain9> pSwapChain;

    if (SUCCEEDED (This->GetSwapChain (0, &pSwapChain)))
    {
      D3DPRESENT_PARAMETERS pparams = { };
      pSwapChain->GetPresentParameters (&pparams);

      if (Format == D3DFMT_D16 && ( Width == pparams.BackBufferWidth ) )
      {
        Format = D3DFMT_D24X8;
#if 0
        Width  <<= 1;
        Height <<= 1;
#endif
      }
    }
  }

  return
    D3D9Device_CreateDepthStencilSurface_Original ( This,
                                                      Width, Height,
                                                        Format,
                                                          MultiSample, MultisampleQuality,
                                                            Discard,
                                                              ppSurface, pSharedHandle );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetRenderTarget_Override ( IDirect3DDevice9  *This,
                               DWORD              RenderTargetIndex,
                               IDirect3DSurface9 *pRenderTarget )
{
  return
    D3D9Device_SetRenderTarget_Original ( This,
                                            RenderTargetIndex,
                                              pRenderTarget );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetDepthStencilSurface_Override ( IDirect3DDevice9  *This,
                                      IDirect3DSurface9 *pNewZStencil )
{
  return
    D3D9Device_SetDepthStencilSurface_Original ( This,
                                                   pNewZStencil );
}

extern std::wstring SK_D3D11_res_root;

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9UpdateTexture_Override ( IDirect3DDevice9      *This,
                             IDirect3DBaseTexture9 *pSourceTexture,
                             IDirect3DBaseTexture9 *pDestinationTexture )
{
  if ((! config.textures.d3d9_mod) || (! SK::D3D9::tex_mgr.init) || SK::D3D9::tex_mgr.injector.isInjectionThread ())
  {
    return
      D3D9Device_UpdateTexture_Original ( This,
                                            pSourceTexture,
                                              pDestinationTexture );
  }


  IDirect3DBaseTexture9* pRealSource = pSourceTexture;
  IDirect3DBaseTexture9* pRealDest   = pDestinationTexture;

  bool src_is_wrapped = false;
  bool dst_is_wrapped = false;

  void* dontcare;
  if (pSourceTexture != nullptr &&
      pSourceTexture->QueryInterface (IID_SKTextureD3D9, &dontcare) == S_OK)
  {
    src_is_wrapped = true;
    pRealSource    = dynamic_cast <ISKTextureD3D9 *> (pSourceTexture)->pTex;
  }

  if (pDestinationTexture != nullptr &&
      pDestinationTexture->QueryInterface (IID_SKTextureD3D9, &dontcare) == S_OK)
  {
    dst_is_wrapped = true;
    pRealDest      = dynamic_cast <ISKTextureD3D9 *> (pDestinationTexture)->pTex;
  }


  HRESULT hr =
    D3D9Device_UpdateTexture_Original ( This,
                                          pRealSource,
                                            pRealDest );


  if (SUCCEEDED (hr) && src_is_wrapped && dst_is_wrapped)
  {
    auto* pSrc = dynamic_cast <ISKTextureD3D9 *> (pSourceTexture);
    auto* pDst = dynamic_cast <ISKTextureD3D9 *> (pDestinationTexture);

    extern
    LARGE_INTEGER                      liLastReset;
    LARGE_INTEGER                      liNow;
    QueryPerformanceCounter_Original (&liNow);

                                                                      // Rudimentary protection against video textures 
    if (((ISKTextureD3D9 *)pDestinationTexture)->tex_crc32c == 0x0)
    {
      pSrc->last_used.QuadPart = liNow.QuadPart;
      pDst->last_used.QuadPart = liNow.QuadPart;

      pSrc->uses++;
      pDst->uses++;

      Texture* pTex = nullptr;

      if (pSrc->tex_crc32c != 0x0)
      {
        pTex =
          new Texture ();

        pDst->tex_crc32c = pSrc->tex_crc32c;
        pDst->tex_size   = pSrc->tex_size;
        pTex->crc32c     = pSrc->tex_crc32c;
        pTex->size       = pSrc->tex_size;
        pTex->d3d9_tex   = pDst;
        pTex->refs++;
        pTex->d3d9_tex->AddRef ();

        tex_mgr.addTexture (pDst->tex_crc32c, pTex, /*SrcDataSize*/pDst->tex_size);

        if (injected_textures.count (pDst->tex_crc32c) && injected_textures [pDst->tex_crc32c] != nullptr)
        {
          pDst->pTexOverride  = injected_textures   [pDst->tex_crc32c];
          pDst->override_size = injected_sizes      [pDst->tex_crc32c];
          pTex->load_time     = injected_load_times [pDst->tex_crc32c];

          tex_mgr.refTextureEx (pTex);
        }

        else
        {
          tex_mgr.missTexture ();

          TexLoadRequest* load_op =
            nullptr;

          wchar_t wszInjectFileName [MAX_PATH] = { L'\0' };

          bool remap_stream =
            tex_mgr.injector.isStreaming (pDst->tex_crc32c);

          //
          // Generic injectable textures
          //
          if (tex_mgr.isTextureInjectable (pDst->tex_crc32c))
          {
            tex_log.LogEx (true, L"[Inject Tex] Injectable texture for checksum (%08x)... ",
                           pDst->tex_crc32c);

            TexRecord& record =
              tex_mgr.getInjectableTexture (pDst->tex_crc32c);

            if (record.method == TexLoadMethod::DontCare)
                record.method =  TexLoadMethod::Streaming;

            // If -1, load from disk...
            if (record.archive == -1)
            {
              if (record.method == TexLoadMethod::Streaming)
              {
                _swprintf ( wszInjectFileName, LR"(%s\inject\textures\streaming\%08x%s)",
                              SK_D3D11_res_root.c_str (),
                                pDst->tex_crc32c,
                                  L".dds" );
              }

              else if (record.method == TexLoadMethod::Blocking)
              {
                _swprintf ( wszInjectFileName, LR"(%s\inject\textures\blocking\%08x%s)",
                              SK_D3D11_res_root.c_str (),
                                pDst->tex_crc32c,
                                  L".dds");
              }
            }

            load_op =
              new TexLoadRequest ();

            load_op->pDevice  = This;
            load_op->checksum = pDst->tex_crc32c;

            if (record.method == TexLoadMethod::Streaming)
              load_op->type    = TexLoadRequest::Stream;
            else
              load_op->type = TexLoadRequest::Immediate;

            wcscpy (load_op->wszFilename, wszInjectFileName);

            if (load_op->type == TexLoadRequest::Stream)
            {
              if (( !remap_stream ))
                tex_log.LogEx (false, L"streaming\n");
              else
                tex_log.LogEx (false, L"in-flight already\n");
            }

            else
            {
              tex_log.LogEx (false, L"blocking (deferred)\n");
            }

            if ( load_op != nullptr && ( load_op->type == TexLoadRequest::Stream ||
                                         load_op->type == TexLoadRequest::Immediate ) )
            {
              load_op->SrcDataSize =
                static_cast <UINT> (
                  record.size
                );

              load_op->pSrc =
                 ((ISKTextureD3D9 *)pDestinationTexture);

              load_op->pDest =
                 ((ISKTextureD3D9 *)pDestinationTexture);

              tex_mgr.injector.lockStreaming ();

              if (load_op->type == TexLoadRequest::Immediate)
                dynamic_cast <ISKTextureD3D9 *> (pDestinationTexture)->must_block = true;

              if (tex_mgr.injector.isStreaming (load_op->checksum))
              {
              //tex_mgr.injector.lockStreaming ();

                auto* pTexOrig =
                  static_cast <ISKTextureD3D9 *> (
                    tex_mgr.injector.getTextureInFlight (load_op->checksum)->pDest
                   );

                // Remap the output of the in-flight texture
                tex_mgr.injector.getTextureInFlight (load_op->checksum)->pDest =
                  pDst;

              //tex_mgr.injector.unlockStreaming ();

                Texture* pExistingTex =
                  tex_mgr.getTexture (load_op->checksum);

                if (pExistingTex != nullptr)
                {
                  for (int i = 0;
                           i < pExistingTex->refs;
                         ++i)
                  {
                    dynamic_cast <ISKTextureD3D9 *> (pDestinationTexture)->AddRef ();
                  }
                }

                tex_mgr.removeTexture (pTexOrig);
              }

              else
              {
                tex_mgr.injector.addTextureInFlight (load_op);
                pTex->d3d9_tex->AddRef              (       );
                stream_pool.postJob                 (load_op);
              }

              tex_mgr.injector.unlockStreaming ();
            }
          }
        }
      }
    }
  }

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9StretchRect_Override (      IDirect3DDevice9    *This,
                                IDirect3DSurface9   *pSourceSurface,
                          const RECT                *pSourceRect,
                                IDirect3DSurface9   *pDestSurface,
                          const RECT                *pDestRect,
                                D3DTEXTUREFILTERTYPE Filter )
{
  if (SK_GetCurrentGameID () == SK_GAME_ID::YS_Seven)
  {
    if (pSourceRect != nullptr && pDestRect != nullptr)
    {
      dll_log.Log ( L"[CompatHack] StretchRect: { %i, %i, %ix%i } -> { %i, %i, %ix%i }",
                      pSourceRect->left, pSourceRect->top, pSourceRect->right  - pSourceRect->left,
                                                           pSourceRect->bottom - pSourceRect->top,
                      pDestRect->left,   pDestRect->top,   pDestRect->right    - pDestRect->left,
                                                           pDestRect->bottom   - pDestRect->top );
    }
  }

  return
    D3D9Device_StretchRect_Original ( This,
                                        pSourceSurface,
                                        pSourceRect,
                                          pDestSurface,
                                          pDestRect,
                                            Filter );
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetCursorPosition_Override (      IDirect3DDevice9 *This,
                                 _In_ INT               X,
                                 _In_ INT               Y,
                                 _In_ DWORD             Flags )
{
  return
    D3D9Device_SetCursorPosition_Original ( This,
                                              X,
                                                Y,
                                                  Flags );
}

//
// Returns true if x1 != x2, but x1 is within (tolerance)-many units of x2
//
bool
SK_DiscontEpsilon (int x1, int x2, int tolerance)
{
  if (x1 == x2)
    return false;

  if ( x1 <= (x2 + tolerance) &&
       x1 >= (x2 - tolerance) )
    return true;

  return false;
}

__declspec (noinline)
D3DPRESENT_PARAMETERS*
WINAPI
SK_SetPresentParamsD3D9 (IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pparams)
{
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (ReadAcquire (&__d3d9_ready))
  {
    CComPtr <IDirect3DDevice9Ex> pDevEx = nullptr;

    if (pparams != nullptr && pDevice != nullptr && ( SUCCEEDED (((IUnknown *)pDevice)->QueryInterface <IDirect3DDevice9Ex> (&pDevEx)) && config.render.d3d9.force_d3d9ex ))
    {
      if (config.render.d3d9.force_d3d9ex)
      {
        pparams->Windowed                   = TRUE;
        pparams->BackBufferFormat           = D3DFMT_X8R8G8B8;
        pparams->BackBufferCount            = 3;
        pparams->PresentationInterval       = 1;
        pparams->FullScreen_RefreshRateInHz = 0;
        pparams->Flags                     &= ~D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
        pparams->Flags                     &= ~D3DPRESENTFLAG_DEVICECLIP;
        pparams->SwapEffect                 = D3DSWAPEFFECT_FLIPEX;
        pparams->MultiSampleType            = D3DMULTISAMPLE_NONE;
        pparams->MultiSampleQuality         = 0;
      }
    }

    if (! config.render.d3d9.force_d3d9ex)
    {
      if (SK_GetCurrentGameID () == SK_GAME_ID::YS_Seven && pparams != nullptr)
      {
        dll_log.Log (L"[CompatHack] D3D9 Backbuffer using format %s changed to %s.",
          SK_D3D9_FormatToStr (pparams->BackBufferFormat).c_str (),
          SK_D3D9_FormatToStr (D3DFMT_X8R8G8B8).c_str ());

        pparams->BackBufferFormat       = D3DFMT_X8R8G8B8;
        pparams->BackBufferCount        = 1;
        pparams->SwapEffect             = D3DSWAPEFFECT_COPY;
        pparams->MultiSampleType        = D3DMULTISAMPLE_NONE;
        pparams->MultiSampleQuality     = 0;
        pparams->EnableAutoDepthStencil = true;
        pparams->AutoDepthStencilFormat = D3DFMT_D24X8;
      }
    }

    if (pparams != nullptr)
    {
      extern HWND hWndRender;

      if (! ReadAcquire (&ImGui_Init))
      {
        //
        // Initialize ImGui for D3D9 games
        //
        if ( ImGui_ImplDX9_Init ( static_cast <void *> (pparams->hDeviceWindow),
                                    pDevice,
                                      pparams )
           )
        {
          InterlockedExchange ( &ImGui_Init, TRUE );
        }
      }

      if (game_window.hWnd == nullptr || (! IsWindow (game_window.hWnd)))
      {
        if (pparams->hDeviceWindow)
          game_window.hWnd = pparams->hDeviceWindow;
        else if (GetFocus () != HWND_DESKTOP && GetActiveWindow () != HWND_DESKTOP)
          game_window.hWnd = GetFocus ();
      }

      if (hWndRender == nullptr || (! IsWindow (hWndRender)))
      {
        hWndRender  =  pparams->hDeviceWindow != nullptr ?
                            pparams->hDeviceWindow :
                                     hWndRender;
      }

      bool switch_to_fullscreen = config.display.force_fullscreen  ||
                                 ( (! rb.fullscreen_exclusive)  &&
                                      request_mode_change       ==   mode_change_request_e::Fullscreen );

      bool switch_to_windowed   = config.display.force_windowed    ||
                                 (    rb.fullscreen_exclusive   &&
                                      request_mode_change       ==   mode_change_request_e::Windowed   );

      if (switch_to_fullscreen)
      {
        HMONITOR hMonitor =
          MonitorFromWindow ( game_window.hWnd,
                                MONITOR_DEFAULTTONEAREST );

        MONITORINFO mi  = { };
        mi.cbSize       = sizeof (mi);
        GetMonitorInfo (hMonitor, &mi);

        pparams->Windowed                   = FALSE;
        pparams->BackBufferCount            = 1;
        pparams->EnableAutoDepthStencil     = true;
        pparams->FullScreen_RefreshRateInHz = 60;

        if (pparams->BackBufferWidth  < 512)
            pparams->BackBufferWidth  =      ( mi.rcMonitor.right  - mi.rcMonitor.left );
        if (pparams->BackBufferHeight < 256)
            pparams->BackBufferHeight =      ( mi.rcMonitor.bottom - mi.rcMonitor.top  );
      }

      else if (switch_to_windowed)
      {
        if (pparams->hDeviceWindow)
        {
          pparams->Windowed                   = TRUE;
          pparams->FullScreen_RefreshRateInHz = 0;
        }

        else
        {
          SK_LOG0 ( ( L" *** Could not force windowed mode, game has no device window?!" ),
                      L"   D3D9   ");
          pparams->Windowed                   = FALSE;
          pparams->FullScreen_RefreshRateInHz = std::max (pparams->FullScreen_RefreshRateInHz, 60U);
        }
      }

      //
      //  <-(+] Forced Borderless Window [+)->
      //     ------------------------------
      //        -> ( NOT FULLSCREEN ) <-
      //
      if (pparams->Windowed && config.window.borderless && (! config.window.fullscreen))
      {
        //  <- @  [No Resolution Override]  @ ->
        //
        if (config.window.res.override.isZero ())
        {
          RECT wnd_rect =
            *SK_GetGameRect ();

          int x_dlg = SK_GetSystemMetrics (SM_CXDLGFRAME);
          int y_dlg = SK_GetSystemMetrics (SM_CYDLGFRAME);
          int title = SK_GetSystemMetrics (SM_CYCAPTION );

          if ( SK_DiscontEpsilon ( pparams->BackBufferWidth,
                                     ( wnd_rect.right  - wnd_rect.left ),
                                     (       2 * x_dlg + 1             ) )

                                 ||

               SK_DiscontEpsilon ( pparams->BackBufferHeight,
                                     ( wnd_rect.bottom - wnd_rect.top  ),
                                     (       2 * y_dlg + title + 1     ) )
             )
          {
            pparams->BackBufferWidth  = ( wnd_rect.right  - wnd_rect.left );
            pparams->BackBufferHeight = ( wnd_rect.bottom - wnd_rect.top  );

            dll_log.Log ( L"[Window Mgr] Border Compensated Resolution ==> (%lu x %lu)",
                            pparams->BackBufferWidth,
                              pparams->BackBufferHeight );
          }
        }

        //  <- @  [OVERRIDE Resolution]  @ ->
        //
        else
        {
          pparams->BackBufferWidth  = config.window.res.override.x;
          pparams->BackBufferHeight = config.window.res.override.y;
        }
      }


      extern GetClientRect_pfn    GetClientRect_Original;

             RECT                 client = {        };
      GetClientRect_Original ( pparams->hDeviceWindow,
                                 &client );


      //  Non-Zero Values for Backbuffer Width / Height
      //  =============================================
      //
      //   An override may be necessary; at the very least we will make
      //     note of the explicit values provided by the game.
      //
      //
      //  NOTE:    If (Zero, Zero) is supplied for (Width, Height) -- it is a
      // ~~~~~~~     special-case indicating to Windows that the client rect.
      //               determines SwapChain resolution
      //
      //     -=> * The client rectangle can be spoofed by Special K even +
      //         +   to the D3D9 Runtime itself !                        * <=-
      //
      //
      //    -------------------------------------------------------------------
      //    (  The 0x0 case is one we have to consider carefully in order     )
      //    (    to figure out how the game is designed to behave vis-a-vis.  )
      //    (      resolution scaling if the window is resized.               )
      //
      if (pparams->BackBufferWidth != 0 && pparams->BackBufferHeight != 0)
      {
        /* User wants an override, so let's get down to brass tacks... */
        if (! config.window.res.override.isZero ())
        {
          pparams->BackBufferWidth  = config.window.res.override.x;
          pparams->BackBufferHeight = config.window.res.override.y;
        }

        /* If this is Zero, we need to actually create the render device / swapchain and
             then get the value Windows assigned us... */
        SK_SetWindowResX (pparams->BackBufferWidth);
        SK_SetWindowResY (pparams->BackBufferHeight);
      }

      // Implicit Resolution
      //
      //
      else
      {
        // If this is zero, we need to actually create the render device / swapchain and
        //   then get the value Windows assigned us...
        SK_SetWindowResX (client.right  - client.left);
        SK_SetWindowResY (client.bottom - client.top);;
      }

      // Range Restrict to prevent D3DERR_INVALID_CALL
      if (pparams->PresentationInterval > 16)
          pparams->PresentationInterval = 0;

      if (pparams->Windowed)
      {
        //SetWindowPos_Original ( hWndRender,
                                  //HWND_TOP,
                                    //0, 0,
                                      //pparams->BackBufferWidth, pparams->BackBufferHeight,
                                        //SWP_NOZORDER | SWP_NOSENDCHANGING );
      }

      else if (switch_to_fullscreen && (! pparams->Windowed))
      {
        if ( SetWindowLongPtrW_Original == nullptr ||
             GetWindowLongPtrW_Original == nullptr )
        {
          SetWindowLongPtrW (pparams->hDeviceWindow, GWL_EXSTYLE, (GetWindowLongPtrW (pparams->hDeviceWindow, GWL_EXSTYLE) & ~(WS_EX_TOPMOST)) | (WS_EX_APPWINDOW));
          SetWindowPos      (pparams->hDeviceWindow, HWND_TOP, 0, 0, 0, 0, SWP_NOSENDCHANGING | SWP_NOMOVE             | SWP_NOSIZE     | SWP_DEFERERASE |
                                                                           SWP_NOCOPYBITS     | SWP_ASYNCWINDOWPOS     | SWP_SHOWWINDOW | SWP_NOREPOSITION );
          SK_InstallWindowHook (pparams->hDeviceWindow);
        }

        else
        {
          SetWindowLongPtrW_Original (game_window.hWnd, GWL_EXSTYLE, (GetWindowLongPtrW_Original (game_window.hWnd, GWL_EXSTYLE) & ~(WS_EX_TOPMOST)) | (WS_EX_APPWINDOW));
          SetWindowPos_Original      (game_window.hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSENDCHANGING | SWP_NOMOVE         | SWP_NOSIZE     | SWP_DEFERERASE |
                                                                              SWP_NOCOPYBITS     | SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW | SWP_NOREPOSITION );
        }
      }

      memcpy ( &g_D3D9PresentParams,
                pparams,
                  sizeof D3DPRESENT_PARAMETERS );

      rb.fullscreen_exclusive = (! pparams->Windowed);
    }
  }

  return pparams;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateDeviceEx_Override ( IDirect3D9Ex           *This,
                              UINT                    Adapter,
                              D3DDEVTYPE              DeviceType,
                              HWND                    hFocusWindow,
                              DWORD                   BehaviorFlags,
                              D3DPRESENT_PARAMETERS  *pPresentationParameters,
                              D3DDISPLAYMODEEX       *pFullscreenDisplayMode,
                              IDirect3DDevice9Ex    **ppReturnedDeviceInterface )
{
  dll_log.Log ( L"[   D3D9   ] [!] %s (%ph, %lu, %lu, %ph, 0x%04X, %ph, %ph, %ph) - "
                L"%s",
                L"IDirect3D9Ex::CreateDeviceEx",
                  This, Adapter, (DWORD)DeviceType,
                    hFocusWindow, BehaviorFlags, pPresentationParameters,
                      pFullscreenDisplayMode, ppReturnedDeviceInterface,
                        SK_SummarizeCaller ().c_str () );

  HRESULT ret = E_FAIL;

  // Don't do this for the dummy context we create during init.
  if (ReadAcquire (&__d3d9_ready))
  {
    SK_D3D9_SetFPSTarget       (pPresentationParameters, pFullscreenDisplayMode);
    SK_D3D9_FixUpBehaviorFlags (BehaviorFlags);
    SK_SetPresentParamsD3D9    ( nullptr,
                                   pPresentationParameters );
  }

  if (config.display.force_windowed)
    hFocusWindow = pPresentationParameters->hDeviceWindow;

  IDirect3DDevice9Ex* pTemp = nullptr;

  D3D9_CALL ( ret, D3D9Ex_CreateDeviceEx_Original ( This,
                                                      Adapter,
                                                        DeviceType,
                                                          hFocusWindow,
                                                            BehaviorFlags,
                                                              pPresentationParameters,
                                                                pFullscreenDisplayMode,
                                                                  &pTemp ) );

  if (SUCCEEDED (ret))
  {
    *ppReturnedDeviceInterface =
       (IDirect3DDevice9Ex *)new IWrapDirect3DDevice9 (pTemp);
  }

  // Ignore video swapchains
  if (pPresentationParameters->Flags & D3DPRESENTFLAG_VIDEO)
    return ret;

  if (! SUCCEEDED (ret))
    return ret;

  if (hFocusWindow != nullptr)
    SK_InstallWindowHook (hFocusWindow);


  if (! LocalHook_D3D9PresentSwap.active)
  {
    CComPtr <IDirect3DSwapChain9> pSwapChain = nullptr;

    // For games that present frames using the actual Swapchain...
    //
    if (SUCCEEDED (pTemp->GetSwapChain (0, &pSwapChain)))
    {
      void** vftable = ReadAcquire (&SK_D3D9_LiveWrappedSwapChains) ?
        (void **)&(*(IWrapDirect3DSwapChain9 **)&pSwapChain)->pReal :
        (void **)&pSwapChain;

      if (vftable != nullptr)
      {
        D3D9_INTERCEPT ( vftable, 3,
                         "IDirect3DSwapChain9::Present",
                          D3D9Swap_Present,
                          D3D9Swap_Present_Original,
                          D3D9Swap_Present_pfn );

        SK_Hook_TargetFromVFTable (
          LocalHook_D3D9PresentSwap,
            vftable, 3 );
      }
    }
  }

  if (! LocalHook_D3D9TestCooperativeLevel.active)
  {
    D3D9_INTERCEPT ( &pTemp, 3,
                        "IDirect3DDevice9::TestCooperativeLevel",
                        D3D9TestCooperativeLevel_Override,
                        D3D9Device_TestCooperativeLevel_Original,
                        D3D9Device_TestCooperativeLevel_pfn );

    SK_Hook_TargetFromVFTable (
      LocalHook_D3D9TestCooperativeLevel,
        (void **)&pTemp, 3 );
  }

//  D3D9_INTERCEPT ( &pTemp, 11,
//                      "IDirect3DDevice9::SetCursorPosition",
//                      D3D9SetCursorPosition_Override,
//                      D3D9SetCursorPosition_Original,
//                      SetCursorPosition_pfn );

  if (! LocalHook_D3D9CreateAdditionalSwapChain.active)
  {
    D3D9_INTERCEPT ( &pTemp, 13,
                        "IDirect3DDevice9::CreateAdditionalSwapChain",
                        D3D9CreateAdditionalSwapChain_Override,
                        D3D9Device_CreateAdditionalSwapChain_Original,
                        D3D9Device_CreateAdditionalSwapChain_pfn );

    SK_Hook_TargetFromVFTable (
      LocalHook_D3D9CreateAdditionalSwapChain,
        (void **)&pTemp, 13 );
  }

  D3D9_INTERCEPT ( &pTemp, 21,
                      "IDirect3DDevice9::SetGammaRamp",
                      D3D9SetGammaRamp_Override,
                      D3D9Device_SetGammaRamp_Original,
                      D3D9Device_SetGammaRamp_pfn );

  D3D9_INTERCEPT ( &pTemp, 23,
                      "IDirect3DDevice9::CreateTexture",
                      D3D9CreateTexture_Override,
                      D3D9Device_CreateTexture_Original,
                      D3D9Device_CreateTexture_pfn );

  D3D9_INTERCEPT ( &pTemp, 26,
                      "IDirect3DDevice9::CreateVertexBuffer",
                      D3D9CreateVertexBuffer_Override,
                      D3D9Device_CreateVertexBuffer_Original,
                      D3D9Device_CreateVertexBuffer_pfn );

  D3D9_INTERCEPT ( &pTemp, 28,
                      "IDirect3DDevice9::CreateRenderTarget",
                      D3D9CreateRenderTarget_Override,
                      D3D9Device_CreateRenderTarget_Original,
                      D3D9Device_CreateRenderTarget_pfn );

  D3D9_INTERCEPT ( &pTemp, 29,
                      "IDirect3DDevice9::CreateDepthStencilSurface",
                      D3D9CreateDepthStencilSurface_Override,
                      D3D9Device_CreateDepthStencilSurface_Original,
                      D3D9Device_CreateDepthStencilSurface_pfn );

  D3D9_INTERCEPT ( &pTemp, 31,
                      "IDirect3DDevice9::UpdateTexture",
                      D3D9UpdateTexture_Override,
                      D3D9Device_UpdateTexture_Original,
                      D3D9Device_UpdateTexture_pfn );

  D3D9_INTERCEPT ( &pTemp, 34,
                      "IDirect3DDevice9::StretchRect",
                      D3D9StretchRect_Override,
                      D3D9Device_StretchRect_Original,
                      D3D9Device_StretchRect_pfn );

  D3D9_INTERCEPT ( &pTemp, 36,
                      "IDirect3DDevice9::CreateOffscreenPlainSurface",
                      D3D9CreateOffscreenPlainSurface_Override,
                      D3D9Device_CreateOffscreenPlainSurface_Original,
                      D3D9Device_CreateOffscreenPlainSurface_pfn );

  D3D9_INTERCEPT ( &pTemp, 37,
                      "IDirect3DDevice9::SetRenderTarget",
                      D3D9SetRenderTarget_Override,
                      D3D9Device_SetRenderTarget_Original,
                      D3D9Device_SetRenderTarget_pfn );

  D3D9_INTERCEPT ( &pTemp, 39,
                   "IDirect3DDevice9::SetDepthStencilSurface",
                    D3D9SetDepthStencilSurface_Override,
                    D3D9Device_SetDepthStencilSurface_Original,
                    D3D9Device_SetDepthStencilSurface_pfn );

  if (! LocalHook_D3D9BeginScene.active)
  {
    D3D9_INTERCEPT ( &pTemp,
                     41,
                     "IDirect3DDevice9::BeginScene",
                      D3D9BeginScene_Override,
                      D3D9Device_BeginScene_Original,
                      D3D9Device_BeginScene_pfn );

    SK_Hook_TargetFromVFTable (
      LocalHook_D3D9BeginScene,
        (void **)&pTemp, 41 );
  }

  if (! LocalHook_D3D9EndScene.active)
  {
    D3D9_INTERCEPT ( &pTemp,
                     42,
                     "IDirect3DDevice9::EndScene",
                      D3D9EndScene_Override,
                      D3D9Device_EndScene_Original,
                      D3D9Device_EndScene_pfn );

    SK_Hook_TargetFromVFTable (
      LocalHook_D3D9EndScene,
        (void **)&pTemp, 42 );
  }

  D3D9_INTERCEPT ( &pTemp, 47,
                   "IDirect3DDevice9::SetViewport",
                    D3D9SetViewport_Override,
                    D3D9Device_SetViewport_Original,
                    D3D9Device_SetViewport_pfn );

  D3D9_INTERCEPT ( &pTemp, 57,
                   "IDirect3DDevice9::SetRenderState",
                    D3D9SetRenderState_Override,
                    D3D9Device_SetRenderState_Original,
                    D3D9Device_SetRenderState_pfn );

  D3D9_INTERCEPT ( &pTemp, 65,
                   "IDirect3DDevice9::SetTexture",
                    D3D9SetTexture_Override,
                    D3D9Device_SetTexture_Original,
                    D3D9Device_SetTexture_pfn );

  D3D9_INTERCEPT ( &pTemp, 69,
                   "IDirect3DDevice9::SetSamplerState",
                    D3D9SetSamplerState_Override,
                    D3D9Device_SetSamplerState_Original,
                    D3D9Device_SetSamplerState_pfn );

  D3D9_INTERCEPT ( &pTemp, 75,
                   "IDirect3DDevice9::SetScissorRect",
                    D3D9SetScissorRect_Override,
                    D3D9Device_SetScissorRect_Original,
                    D3D9Device_SetScissorRect_pfn );

  D3D9_INTERCEPT ( &pTemp, 81,
                   "IDirect3DDevice9::DrawPrimitive",
                    D3D9DrawPrimitive_Override,
                    D3D9Device_DrawPrimitive_Original,
                    D3D9Device_DrawPrimitive_pfn );

  D3D9_INTERCEPT ( &pTemp, 82,
                   "IDirect3DDevice9::DrawIndexedPrimitive",
                    D3D9DrawIndexedPrimitive_Override,
                    D3D9Device_DrawIndexedPrimitive_Original,
                    D3D9Device_DrawIndexedPrimitive_pfn );

  D3D9_INTERCEPT ( &pTemp, 83,
                   "IDirect3DDevice9::DrawPrimitiveUP",
                    D3D9DrawPrimitiveUP_Override,
                    D3D9Device_DrawPrimitiveUP_Original,
                    D3D9Device_DrawPrimitiveUP_pfn );

  D3D9_INTERCEPT ( &pTemp, 84,
                   "IDirect3DDevice9::DrawIndexedPrimitiveUP",
                    D3D9DrawIndexedPrimitiveUP_Override,
                    D3D9Device_DrawIndexedPrimitiveUP_Original,
                    D3D9Device_DrawIndexedPrimitiveUP_pfn );

  D3D9_INTERCEPT ( &pTemp, 86,
                   "IDirect3DDevice9::CreateVertexDeclaration",
                    D3D9CreateVertexDeclaration_Override,
                    D3D9Device_CreateVertexDeclaration_Original,
                    D3D9Device_CreateVertexDeclaration_pfn );

  D3D9_INTERCEPT ( &pTemp, 87,
                   "IDirect3DDevice9::SetVertexDeclaration",
                    D3D9SetVertexDeclaration_Override,
                    D3D9Device_SetVertexDeclaration_Original,
                    D3D9Device_SetVertexDeclaration_pfn );

  D3D9_INTERCEPT ( &pTemp, 89,
                   "IDirect3DDevice9::SetFVF",
                    D3D9SetFVF_Override,
                    D3D9Device_SetFVF_Original,
                    D3D9Device_SetFVF_pfn );

  D3D9_INTERCEPT ( &pTemp, 92,
                   "IDirect3DDevice9::SetVertexShader",
                    D3D9SetVertexShader_Override,
                    D3D9Device_SetVertexShader_Original,
                    D3D9Device_SetVertexShader_pfn );

  D3D9_INTERCEPT ( &pTemp, 94,
                   "IDirect3DDevice9::SetSetVertexShaderConstantF",
                    D3D9SetVertexShaderConstantF_Override,
                    D3D9Device_SetVertexShaderConstantF_Original,
                    D3D9Device_SetVertexShaderConstantF_pfn );

  D3D9_INTERCEPT ( &pTemp, 100,
                   "IDirect3DDevice9::SetStreamSource",
                    D3D9SetStreamSource_Override,
                    D3D9Device_SetStreamSource_Original,
                    D3D9Device_SetStreamSource_pfn );

  D3D9_INTERCEPT ( &pTemp, 102,
                   "IDirect3DDevice9::SetStreamSourceFreq",
                    D3D9SetStreamSourceFreq_Override,
                    D3D9Device_SetStreamSourceFreq_Original,
                    D3D9Device_SetStreamSourceFreq_pfn );

  D3D9_INTERCEPT ( &pTemp, 107,
                   "IDirect3DDevice9::SetPixelShader",
                    D3D9SetPixelShader_Override,
                    D3D9Device_SetPixelShader_Original,
                    D3D9Device_SetPixelShader_pfn );

  D3D9_INTERCEPT ( &pTemp, 109,
                   "IDirect3DDevice9::SetPixelShaderConstantF",
                    D3D9SetPixelShaderConstantF_Override,
                    D3D9Device_SetPixelShaderConstantF_Original,
                    D3D9Device_SetPixelShaderConstantF_pfn );

  SK_SetPresentParamsD3D9 (*&pTemp, pPresentationParameters);
  SK_D3D9_HookPresent     (*&pTemp);

  SK_ApplyQueuedHooks     ();

  //if (InterlockedExchangeAdd (&__d3d9_ready, 0))
    //(*&pTemp)->ResetEx (pPresentationParameters, nullptr);

  if (ReadAcquire (&__d3d9_ready))
  {
    ResetCEGUI_D3D9 (nullptr);

    //
    // Initialize ImGui for D3D9 games
    //
    if ( ImGui_ImplDX9_Init ( (void *)pPresentationParameters->hDeviceWindow,
                                    *&pTemp,
                                     pPresentationParameters )
       )
    {
      InterlockedExchange ( &ImGui_Init, TRUE );
    }
    //ResetCEGUI_D3D9 (*&pTemp);
  }

  return ret;
}

HRESULT
__declspec (noinline)
WINAPI
D3D9CreateDevice_Override ( IDirect3D9*            This,
                            UINT                   Adapter,
                            D3DDEVTYPE             DeviceType,
                            HWND                   hFocusWindow,
                            DWORD                  BehaviorFlags,
                            D3DPRESENT_PARAMETERS* pPresentationParameters,
                            IDirect3DDevice9**     ppReturnedDeviceInterface )
{
  HRESULT ret = E_FAIL;

  dll_log.Log ( L"[   D3D9   ] [!] %s (%ph, %lu, %lu, %ph, 0x%04X, %ph, %ph) - "
                L"%s",
                  L"IDirect3D9::CreateDevice", This, Adapter, (DWORD)DeviceType,
                    hFocusWindow, BehaviorFlags, pPresentationParameters,
                      ppReturnedDeviceInterface,
                        SK_SummarizeCaller ().c_str () );

  if (ReadAcquire (&__d3d9_ready))
  {
    SK_D3D9_SetFPSTarget       (pPresentationParameters);
    SK_D3D9_FixUpBehaviorFlags (BehaviorFlags);

    SK_SetPresentParamsD3D9    ( nullptr,
                                   pPresentationParameters );
  }

  if (config.display.force_windowed)
    hFocusWindow = pPresentationParameters->hDeviceWindow;


  IDirect3DDevice9* pTemp = nullptr;

  if (ret == E_FAIL)
    D3D9_CALL ( ret, D3D9_CreateDevice_Original ( This, Adapter,
                                                    DeviceType,
                                                      hFocusWindow,
                                                        BehaviorFlags,
                                                          pPresentationParameters,
                                                            &pTemp ) );

  if (SUCCEEDED (ret))
  {
    *ppReturnedDeviceInterface =
       (IDirect3DDevice9 *)new IWrapDirect3DDevice9 (pTemp);
  }

  // Ignore video swapchains
  if (pPresentationParameters->Flags & D3DPRESENTFLAG_VIDEO)
    return ret;

  // Do not attempt to do vftable override stuff if this failed,
  //   that will cause an immediate crash! Instead log some information that
  //     might help diagnose the problem.
  if (! SUCCEEDED (ret))
  {
    if (pPresentationParameters != nullptr)
    {
      dll_log.LogEx (true,
                L"[   D3D9   ]  SwapChain Settings:   Res=(%ux%u), Format=%ws, "
                                        L"Count=%lu - "
                                        L"SwapEffect: %ws, Flags: 0x%04X, "
                                        L"AutoDepthStencil: %ws "
                                        L"PresentationInterval: %u\n",
                         pPresentationParameters->BackBufferWidth,
                         pPresentationParameters->BackBufferHeight,
    SK_D3D9_FormatToStr (pPresentationParameters->BackBufferFormat).c_str (),
                         pPresentationParameters->BackBufferCount,
SK_D3D9_SwapEffectToStr (pPresentationParameters->SwapEffect).c_str (),
                         pPresentationParameters->Flags,
                         pPresentationParameters->EnableAutoDepthStencil ?
    SK_D3D9_FormatToStr (pPresentationParameters->AutoDepthStencilFormat).c_str () :
                         L"N/A",
                         pPresentationParameters->PresentationInterval);

      if (! pPresentationParameters->Windowed)
      {
        dll_log.LogEx (true,
                L"[   D3D9   ]  Fullscreen Settings:  Refresh Rate: %u\n",
                   pPresentationParameters->FullScreen_RefreshRateInHz);
        dll_log.LogEx (true,
                L"[   D3D9   ]  Multisample Settings: Type: %X, Quality: %u\n",
                   pPresentationParameters->MultiSampleType,
                   pPresentationParameters->MultiSampleQuality);
      }
    }

    return ret;
  }

  if (hFocusWindow != nullptr)
    SK_InstallWindowHook (hFocusWindow);


  if (! LocalHook_D3D9PresentSwap.active)
  {
    CComPtr <IDirect3DSwapChain9> pSwapChain = nullptr;

    // For games that present frames using the actual Swapchain...
    //
    if (SUCCEEDED (pTemp->GetSwapChain (0, &pSwapChain)))
    {
      void** vftable = ReadAcquire (&SK_D3D9_LiveWrappedSwapChains) ?
        (void **)&(*(IWrapDirect3DSwapChain9 **)&pSwapChain)->pReal :
        (void **)&pSwapChain;

      if (vftable != nullptr)
      {
        D3D9_INTERCEPT ( vftable, 3,
                         "IDirect3DSwapChain9::Present",
                          D3D9Swap_Present,
                          D3D9Swap_Present_Original,
                          D3D9Swap_Present_pfn );

        SK_Hook_TargetFromVFTable (
          LocalHook_D3D9PresentSwap,
            vftable, 3 );
      }
    }
  }

  if (! LocalHook_D3D9TestCooperativeLevel.active)
  {
    D3D9_INTERCEPT ( &pTemp, 3,
                        "IDirect3DDevice9::TestCooperativeLevel",
                        D3D9TestCooperativeLevel_Override,
                        D3D9Device_TestCooperativeLevel_Original,
                        D3D9Device_TestCooperativeLevel_pfn );

    SK_Hook_TargetFromVFTable (
      LocalHook_D3D9TestCooperativeLevel,
        (void **)&pTemp, 3 );
  }

//  D3D9_INTERCEPT ( &pTemp, 11,
//                      "IDirect3DDevice9::SetCursorPosition",
//                      D3D9SetCursorPosition_Override,
//                      D3D9SetCursorPosition_Original,
//                      SetCursorPosition_pfn );

  if (! LocalHook_D3D9CreateAdditionalSwapChain.active)
  {
    D3D9_INTERCEPT ( &pTemp, 13,
                        "IDirect3DDevice9::CreateAdditionalSwapChain",
                        D3D9CreateAdditionalSwapChain_Override,
                        D3D9Device_CreateAdditionalSwapChain_Original,
                        D3D9Device_CreateAdditionalSwapChain_pfn );

    SK_Hook_TargetFromVFTable (
      LocalHook_D3D9CreateAdditionalSwapChain,
        (void **)&pTemp, 13 );
  }

  D3D9_INTERCEPT ( &pTemp, 21,
                      "IDirect3DDevice9::SetGammaRamp",
                      D3D9SetGammaRamp_Override,
                      D3D9Device_SetGammaRamp_Original,
                      D3D9Device_SetGammaRamp_pfn );

  D3D9_INTERCEPT ( &pTemp, 23,
                      "IDirect3DDevice9::CreateTexture",
                      D3D9CreateTexture_Override,
                      D3D9Device_CreateTexture_Original,
                      D3D9Device_CreateTexture_pfn );

  D3D9_INTERCEPT ( &pTemp, 26,
                      "IDirect3DDevice9::CreateVertexBuffer",
                      D3D9CreateVertexBuffer_Override,
                      D3D9Device_CreateVertexBuffer_Original,
                      D3D9Device_CreateVertexBuffer_pfn );

  D3D9_INTERCEPT ( &pTemp, 28,
                      "IDirect3DDevice9::CreateRenderTarget",
                      D3D9CreateRenderTarget_Override,
                      D3D9Device_CreateRenderTarget_Original,
                      D3D9Device_CreateRenderTarget_pfn );

  D3D9_INTERCEPT ( &pTemp, 29,
                      "IDirect3DDevice9::CreateDepthStencilSurface",
                      D3D9CreateDepthStencilSurface_Override,
                      D3D9Device_CreateDepthStencilSurface_Original,
                      D3D9Device_CreateDepthStencilSurface_pfn );

  D3D9_INTERCEPT ( &pTemp, 31,
                      "IDirect3DDevice9::UpdateTexture",
                      D3D9UpdateTexture_Override,
                      D3D9Device_UpdateTexture_Original,
                      D3D9Device_UpdateTexture_pfn );

  D3D9_INTERCEPT ( &pTemp, 34,
                      "IDirect3DDevice9::StretchRect",
                      D3D9StretchRect_Override,
                      D3D9Device_StretchRect_Original,
                      D3D9Device_StretchRect_pfn );

  D3D9_INTERCEPT ( &pTemp, 36,
                      "IDirect3DDevice9::CreateOffscreenPlainSurface",
                      D3D9CreateOffscreenPlainSurface_Override,
                      D3D9Device_CreateOffscreenPlainSurface_Original,
                      D3D9Device_CreateOffscreenPlainSurface_pfn );

  D3D9_INTERCEPT ( &pTemp, 37,
                      "IDirect3DDevice9::SetRenderTarget",
                      D3D9SetRenderTarget_Override,
                      D3D9Device_SetRenderTarget_Original,
                      D3D9Device_SetRenderTarget_pfn );

  D3D9_INTERCEPT ( &pTemp, 39,
                   "IDirect3DDevice9::SetDepthStencilSurface",
                    D3D9SetDepthStencilSurface_Override,
                    D3D9Device_SetDepthStencilSurface_Original,
                    D3D9Device_SetDepthStencilSurface_pfn );

  if (! LocalHook_D3D9BeginScene.active)
  {
    D3D9_INTERCEPT ( &pTemp,
                     41,
                     "IDirect3DDevice9::BeginScene",
                      D3D9BeginScene_Override,
                      D3D9Device_BeginScene_Original,
                      D3D9Device_BeginScene_pfn );

    SK_Hook_TargetFromVFTable (
      LocalHook_D3D9BeginScene,
        (void **)&pTemp, 41 );
  }

  if (! LocalHook_D3D9EndScene.active)
  {
    D3D9_INTERCEPT ( &pTemp,
                     42,
                     "IDirect3DDevice9::EndScene",
                      D3D9EndScene_Override,
                      D3D9Device_EndScene_Original,
                      D3D9Device_EndScene_pfn );

    SK_Hook_TargetFromVFTable (
      LocalHook_D3D9EndScene,
        (void **)&pTemp, 42 );
  }

  D3D9_INTERCEPT ( &pTemp, 47,
                   "IDirect3DDevice9::SetViewport",
                    D3D9SetViewport_Override,
                    D3D9Device_SetViewport_Original,
                    D3D9Device_SetViewport_pfn );

  D3D9_INTERCEPT ( &pTemp, 57,
                   "IDirect3DDevice9::SetRenderState",
                    D3D9SetRenderState_Override,
                    D3D9Device_SetRenderState_Original,
                    D3D9Device_SetRenderState_pfn );

  D3D9_INTERCEPT ( &pTemp, 65,
                   "IDirect3DDevice9::SetTexture",
                    D3D9SetTexture_Override,
                    D3D9Device_SetTexture_Original,
                    D3D9Device_SetTexture_pfn );

  D3D9_INTERCEPT ( &pTemp, 69,
                   "IDirect3DDevice9::SetSamplerState",
                    D3D9SetSamplerState_Override,
                    D3D9Device_SetSamplerState_Original,
                    D3D9Device_SetSamplerState_pfn );

  D3D9_INTERCEPT ( &pTemp, 75,
                   "IDirect3DDevice9::SetScissorRect",
                    D3D9SetScissorRect_Override,
                    D3D9Device_SetScissorRect_Original,
                    D3D9Device_SetScissorRect_pfn );

  D3D9_INTERCEPT ( &pTemp, 81,
                   "IDirect3DDevice9::DrawPrimitive",
                    D3D9DrawPrimitive_Override,
                    D3D9Device_DrawPrimitive_Original,
                    D3D9Device_DrawPrimitive_pfn );

  D3D9_INTERCEPT ( &pTemp, 82,
                   "IDirect3DDevice9::DrawIndexedPrimitive",
                    D3D9DrawIndexedPrimitive_Override,
                    D3D9Device_DrawIndexedPrimitive_Original,
                    D3D9Device_DrawIndexedPrimitive_pfn );

  D3D9_INTERCEPT ( &pTemp, 83,
                   "IDirect3DDevice9::DrawPrimitiveUP",
                    D3D9DrawPrimitiveUP_Override,
                    D3D9Device_DrawPrimitiveUP_Original,
                    D3D9Device_DrawPrimitiveUP_pfn );

  D3D9_INTERCEPT ( &pTemp, 84,
                   "IDirect3DDevice9::DrawIndexedPrimitiveUP",
                    D3D9DrawIndexedPrimitiveUP_Override,
                    D3D9Device_DrawIndexedPrimitiveUP_Original,
                    D3D9Device_DrawIndexedPrimitiveUP_pfn );

  D3D9_INTERCEPT ( &pTemp, 86,
                   "IDirect3DDevice9::CreateVertexDeclaration",
                    D3D9CreateVertexDeclaration_Override,
                    D3D9Device_CreateVertexDeclaration_Original,
                    D3D9Device_CreateVertexDeclaration_pfn );

  D3D9_INTERCEPT ( &pTemp, 87,
                   "IDirect3DDevice9::SetVertexDeclaration",
                    D3D9SetVertexDeclaration_Override,
                    D3D9Device_SetVertexDeclaration_Original,
                    D3D9Device_SetVertexDeclaration_pfn );

  D3D9_INTERCEPT ( &pTemp, 89,
                   "IDirect3DDevice9::SetFVF",
                    D3D9SetFVF_Override,
                    D3D9Device_SetFVF_Original,
                    D3D9Device_SetFVF_pfn );

  D3D9_INTERCEPT ( &pTemp, 92,
                   "IDirect3DDevice9::SetVertexShader",
                    D3D9SetVertexShader_Override,
                    D3D9Device_SetVertexShader_Original,
                    D3D9Device_SetVertexShader_pfn );

  D3D9_INTERCEPT ( &pTemp, 94,
                   "IDirect3DDevice9::SetSetVertexShaderConstantF",
                    D3D9SetVertexShaderConstantF_Override,
                    D3D9Device_SetVertexShaderConstantF_Original,
                    D3D9Device_SetVertexShaderConstantF_pfn );

  D3D9_INTERCEPT ( &pTemp, 100,
                   "IDirect3DDevice9::SetStreamSource",
                    D3D9SetStreamSource_Override,
                    D3D9Device_SetStreamSource_Original,
                    D3D9Device_SetStreamSource_pfn );

  D3D9_INTERCEPT ( &pTemp, 102,
                   "IDirect3DDevice9::SetStreamSourceFreq",
                    D3D9SetStreamSourceFreq_Override,
                    D3D9Device_SetStreamSourceFreq_Original,
                    D3D9Device_SetStreamSourceFreq_pfn );

  D3D9_INTERCEPT ( &pTemp, 107,
                   "IDirect3DDevice9::SetPixelShader",
                    D3D9SetPixelShader_Override,
                    D3D9Device_SetPixelShader_Original,
                    D3D9Device_SetPixelShader_pfn );

  D3D9_INTERCEPT ( &pTemp, 109,
                   "IDirect3DDevice9::SetPixelShaderConstantF",
                    D3D9SetPixelShaderConstantF_Override,
                    D3D9Device_SetPixelShaderConstantF_Original,
                    D3D9Device_SetPixelShaderConstantF_pfn );


  SK_SetPresentParamsD3D9 (pTemp, pPresentationParameters);
  SK_D3D9_HookPresent     (pTemp);

  SK_ApplyQueuedHooks     ();

  //if (InterlockedExchangeAdd (&__d3d9_ready, 0))
    //(*pTemp)->Reset (pPresentationParameters);

  if (ReadAcquire (&__d3d9_ready))
  {
    ResetCEGUI_D3D9 (nullptr);
    //ResetCEGUI_D3D9 (*pTemp);

    //
    // Initialize ImGui for D3D9 games
    //
    if ( ImGui_ImplDX9_Init ( reinterpret_cast <void *> (pPresentationParameters->hDeviceWindow),
                                                         pTemp,
                                                         pPresentationParameters )
       )
    {
      InterlockedExchange ( &ImGui_Init, TRUE );
    }
  }

  return ret;
}



// This is only called when forcing D3D9Ex on, which is generally only desirable for
//   G-Sync in windowed mode.
HRESULT
__declspec (noinline)
WINAPI
D3D9ExCreateDevice_Override ( IDirect3D9*            This,
                              UINT                   Adapter,
                              D3DDEVTYPE             DeviceType,
                              HWND                   hFocusWindow,
                              DWORD                  BehaviorFlags,
                              D3DPRESENT_PARAMETERS* pPresentationParameters,
                              IDirect3DDevice9**     ppReturnedDeviceInterface )
{
  HRESULT ret = E_FAIL;

  dll_log.Log ( L"[   D3D9   ] [!] %s (%ph, %lu, %lu, %ph, 0x%04X, %ph, %ph) - "
                L"%s",
                  L"IDirect3D9Ex::CreateDevice", This, Adapter, (DWORD)DeviceType,
                    hFocusWindow, BehaviorFlags, pPresentationParameters,
                      ppReturnedDeviceInterface,
                        SK_SummarizeCaller ().c_str () );

  DeviceType = D3DDEVTYPE_HAL;

  if (hFocusWindow)
    pPresentationParameters->hDeviceWindow = hFocusWindow;

  SK_D3D9_FixUpBehaviorFlags (BehaviorFlags);

  pPresentationParameters->Windowed                   = TRUE;
  pPresentationParameters->BackBufferFormat           = D3DFMT_X8R8G8B8;
  pPresentationParameters->BackBufferCount            = 3;
  pPresentationParameters->PresentationInterval       = 1;
  pPresentationParameters->FullScreen_RefreshRateInHz = 0;
  pPresentationParameters->Flags                     &= ~D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  pPresentationParameters->Flags                     &= ~D3DPRESENTFLAG_DEVICECLIP;
  pPresentationParameters->SwapEffect                 = D3DSWAPEFFECT_FLIPEX;
  pPresentationParameters->MultiSampleType            = D3DMULTISAMPLE_NONE;
  pPresentationParameters->MultiSampleQuality         = 0;

  if (ret == E_FAIL)
    D3D9_CALL ( ret, D3D9Ex_CreateDeviceEx_Original ( (IDirect3D9Ex *)This, Adapter,
                                                        DeviceType,
                                                          nullptr,
                                                            BehaviorFlags,
                                                              pPresentationParameters,
                                                                nullptr,
                                                                  (IDirect3DDevice9Ex **)ppReturnedDeviceInterface ) );

  // Do not attempt to do vtable override stuff if this failed,
  //   that will cause an immediate crash! Instead log some information that
  //     might help diagnose the problem.
  if (! SUCCEEDED (ret))
  {
    if (pPresentationParameters != nullptr)
    {
      dll_log.LogEx (true,
                L"[   D3D9   ]  SwapChain Settings:   Res=(%ux%u), Format=%04i, "
                                        L"Count=%lu - "
                                        L"SwapEffect: 0x%02X, Flags: 0x%04X,"
                                        L"AutoDepthStencil: %s "
                                        L"PresentationInterval: %u\n",
                   pPresentationParameters->BackBufferWidth,
                   pPresentationParameters->BackBufferHeight,
                   pPresentationParameters->BackBufferFormat,
                   pPresentationParameters->BackBufferCount,
                   pPresentationParameters->SwapEffect,
                   pPresentationParameters->Flags,
                   pPresentationParameters->EnableAutoDepthStencil ? L"true" :
                                                                     L"false",
                   pPresentationParameters->PresentationInterval);

      if (! pPresentationParameters->Windowed)
      {
        dll_log.LogEx (true,
                L"[   D3D9   ]  Fullscreen Settings:  Refresh Rate: %u\n",
                   pPresentationParameters->FullScreen_RefreshRateInHz);
        dll_log.LogEx (true,
                L"[   D3D9   ]  Multisample Settings: Type: %X, Quality: %u\n",
                   pPresentationParameters->MultiSampleType,
                   pPresentationParameters->MultiSampleQuality);
      }
    }

    return ret;
  }

  if (hFocusWindow != nullptr)
    SK_InstallWindowHook (hFocusWindow);

  else if (pPresentationParameters->hDeviceWindow)
    SK_InstallWindowHook (pPresentationParameters->hDeviceWindow);

  CComPtr <IDirect3DSwapChain9> pSwapChain = nullptr;

  if (SUCCEEDED ((*ppReturnedDeviceInterface)->GetSwapChain (0, &pSwapChain)))
  {
    void** vftable = ReadAcquire (&SK_D3D9_LiveWrappedSwapChains) ?
      (void **)&(*(IWrapDirect3DSwapChain9 **)&pSwapChain)->pReal :
      (void **)&pSwapChain;

    if (vftable != nullptr && vftable [3] != nullptr)
    {
      D3D9_INTERCEPT ( vftable, 3,
                       "IDirect3DSwapChain9::Present",
                        D3D9Swap_Present,
                        D3D9Swap_Present_Original,
                        D3D9Swap_Present_pfn );

      SK_Hook_TargetFromVFTable (
        LocalHook_D3D9PresentSwap,
          vftable, 3 );
    }
  }

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 3,
                      "IDirect3DDevice9::TestCooperativeLevel",
                      D3D9TestCooperativeLevel_Override,
                      D3D9Device_TestCooperativeLevel_Original,
                      D3D9Device_TestCooperativeLevel_pfn );

//  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 11,
//                      "IDirect3DDevice9::SetCursorPosition",
//                      D3D9SetCursorPosition_Override,
//                      D3D9SetCursorPosition_Original,
//                      SetCursorPosition_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 13,
                      "IDirect3DDevice9::CreateAdditionalSwapChain",
                      D3D9CreateAdditionalSwapChain_Override,
                      D3D9Device_CreateAdditionalSwapChain_Original,
                      D3D9Device_CreateAdditionalSwapChain_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 21,
                      "IDirect3DDevice9::SetGammaRamp",
                      D3D9SetGammaRamp_Override,
                      D3D9Device_SetGammaRamp_Original,
                      D3D9Device_SetGammaRamp_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 23,
                      "IDirect3DDevice9::CreateTexture",
                      D3D9CreateTexture_Override,
                      D3D9Device_CreateTexture_Original,
                      D3D9Device_CreateTexture_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 26,
                      "IDirect3DDevice9::CreateVertexBuffer",
                      D3D9CreateVertexBuffer_Override,
                      D3D9Device_CreateVertexBuffer_Original,
                      D3D9Device_CreateVertexBuffer_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 28,
                      "IDirect3DDevice9::CreateRenderTarget",
                      D3D9CreateRenderTarget_Override,
                      D3D9Device_CreateRenderTarget_Original,
                      D3D9Device_CreateRenderTarget_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 29,
                      "IDirect3DDevice9::CreateDepthStencilSurface",
                      D3D9CreateDepthStencilSurface_Override,
                      D3D9Device_CreateDepthStencilSurface_Original,
                      D3D9Device_CreateDepthStencilSurface_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 31,
                      "IDirect3DDevice9::UpdateTexture",
                      D3D9UpdateTexture_Override,
                      D3D9Device_UpdateTexture_Original,
                      D3D9Device_UpdateTexture_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 34,
                      "IDirect3DDevice9::StretchRect",
                      D3D9StretchRect_Override,
                      D3D9Device_StretchRect_Original,
                      D3D9Device_StretchRect_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 36,
                      "IDirect3DDevice9::CreateOffscreenPlainSurface",
                      D3D9CreateOffscreenPlainSurface_Override,
                      D3D9Device_CreateOffscreenPlainSurface_Original,
                      D3D9Device_CreateOffscreenPlainSurface_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 37,
                      "IDirect3DDevice9::SetRenderTarget",
                      D3D9SetRenderTarget_Override,
                      D3D9Device_SetRenderTarget_Original,
                      D3D9Device_SetRenderTarget_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 39,
                   "IDirect3DDevice9::SetDepthStencilSurface",
                    D3D9SetDepthStencilSurface_Override,
                    D3D9Device_SetDepthStencilSurface_Original,
                    D3D9Device_SetDepthStencilSurface_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface,
                   41,
                   "IDirect3DDevice9::BeginScene",
                    D3D9BeginScene_Override,
                    D3D9Device_BeginScene_Original,
                    D3D9Device_BeginScene_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface,
                   42,
                   "IDirect3DDevice9::EndScene",
                    D3D9EndScene_Override,
                    D3D9Device_EndScene_Original,
                    D3D9Device_EndScene_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 47,
                   "IDirect3DDevice9::SetViewport",
                    D3D9SetViewport_Override,
                    D3D9Device_SetViewport_Original,
                    D3D9Device_SetViewport_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 57,
                   "IDirect3DDevice9::SetRenderState",
                    D3D9SetRenderState_Override,
                    D3D9Device_SetRenderState_Original,
                    D3D9Device_SetRenderState_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 64,
                   "IDirect3DDevice9::GetTexture",
                    D3D9GetTexture_Override,
                    D3D9Device_GetTexture_Original,
                    D3D9Device_GetTexture_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 65,
                   "IDirect3DDevice9::SetTexture",
                    D3D9SetTexture_Override,
                    D3D9Device_SetTexture_Original,
                    D3D9Device_SetTexture_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 69,
                   "IDirect3DDevice9::SetSamplerState",
                    D3D9SetSamplerState_Override,
                    D3D9Device_SetSamplerState_Original,
                    D3D9Device_SetSamplerState_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 75,
                   "IDirect3DDevice9::SetScissorRect",
                    D3D9SetScissorRect_Override,
                    D3D9Device_SetScissorRect_Original,
                    D3D9Device_SetScissorRect_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 81,
                   "IDirect3DDevice9::DrawPrimitive",
                    D3D9DrawPrimitive_Override,
                    D3D9Device_DrawPrimitive_Original,
                    D3D9Device_DrawPrimitive_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 82,
                   "IDirect3DDevice9::DrawIndexedPrimitive",
                    D3D9DrawIndexedPrimitive_Override,
                    D3D9Device_DrawIndexedPrimitive_Original,
                    D3D9Device_DrawIndexedPrimitive_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 83,
                   "IDirect3DDevice9::DrawPrimitiveUP",
                    D3D9DrawPrimitiveUP_Override,
                    D3D9Device_DrawPrimitiveUP_Original,
                    D3D9Device_DrawPrimitiveUP_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 84,
                   "IDirect3DDevice9::DrawIndexedPrimitiveUP",
                    D3D9DrawIndexedPrimitiveUP_Override,
                    D3D9Device_DrawIndexedPrimitiveUP_Original,
                    D3D9Device_DrawIndexedPrimitiveUP_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 86,
                   "IDirect3DDevice9::CreateVertexDeclaration",
                    D3D9CreateVertexDeclaration_Override,
                    D3D9Device_CreateVertexDeclaration_Original,
                    D3D9Device_CreateVertexDeclaration_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 87,
                   "IDirect3DDevice9::SetVertexDeclaration",
                    D3D9SetVertexDeclaration_Override,
                    D3D9Device_SetVertexDeclaration_Original,
                    D3D9Device_SetVertexDeclaration_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 89,
                   "IDirect3DDevice9::SetFVF",
                    D3D9SetFVF_Override,
                    D3D9Device_SetFVF_Original,
                    D3D9Device_SetFVF_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 92,
                   "IDirect3DDevice9::SetVertexShader",
                    D3D9SetVertexShader_Override,
                    D3D9Device_SetVertexShader_Original,
                    D3D9Device_SetVertexShader_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 94,
                   "IDirect3DDevice9::SetSetVertexShaderConstantF",
                    D3D9SetVertexShaderConstantF_Override,
                    D3D9Device_SetVertexShaderConstantF_Original,
                    D3D9Device_SetVertexShaderConstantF_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 100,
                   "IDirect3DDevice9::SetStreamSource",
                    D3D9SetStreamSource_Override,
                    D3D9Device_SetStreamSource_Original,
                    D3D9Device_SetStreamSource_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 102,
                   "IDirect3DDevice9::SetStreamSourceFreq",
                    D3D9SetStreamSourceFreq_Override,
                    D3D9Device_SetStreamSourceFreq_Original,
                    D3D9Device_SetStreamSourceFreq_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 107,
                   "IDirect3DDevice9::SetPixelShader",
                    D3D9SetPixelShader_Override,
                    D3D9Device_SetPixelShader_Original,
                    D3D9Device_SetPixelShader_pfn );

  D3D9_INTERCEPT ( ppReturnedDeviceInterface, 109,
                   "IDirect3DDevice9::SetPixelShaderConstantF",
                    D3D9SetPixelShaderConstantF_Override,
                    D3D9Device_SetPixelShaderConstantF_Original,
                    D3D9Device_SetPixelShaderConstantF_pfn );

  SK_SetPresentParamsD3D9 (*ppReturnedDeviceInterface, pPresentationParameters);
  SK_D3D9_HookPresent     (*ppReturnedDeviceInterface);

  SK_ApplyQueuedHooks     ();

  if (ReadAcquire (&__d3d9_ready))
  {
    ResetCEGUI_D3D9 (nullptr);
    //ResetCEGUI_D3D9 (*ppReturnedDeviceInterface);

    //
    // Initialize ImGui for D3D9 games
    //
    if ( ImGui_ImplDX9_Init ( reinterpret_cast <void *> (pPresentationParameters->hDeviceWindow),
                                                        *ppReturnedDeviceInterface,
                                                         pPresentationParameters )
       )
    {
      InterlockedExchange ( &ImGui_Init, TRUE );
    }
  }

  return ret;
}


bool tex_init = false;

__declspec (noinline)
IDirect3D9*
STDMETHODCALLTYPE
Direct3DCreate9 (UINT SDKVersion)
{
  WaitForInit_D3D9 ();
  WaitForInit      ();

  dll_log.Log ( L"[   D3D9   ] [!] %s (%lu) - "
                L"%s",
                  L"Direct3DCreate9",
                    SDKVersion,
                      SK_SummarizeCaller ().c_str () );

  bool force_d3d9ex = config.render.d3d9.force_d3d9ex;

  // Disable D3D9EX whenever GeDoSaTo is present
  if (force_d3d9ex)
  {
    if (GetModuleHandle (L"GeDoSaTo.dll"))
    {
      dll_log.Log ( L"[CompatHack] <!> Disabling D3D9Ex optimizations "
                                       "because GeDoSaTo.dll is present!" );
      force_d3d9ex = false;
    }
  }

  IDirect3D9*   d3d9    = nullptr;
  IDirect3D9Ex* pD3D9Ex = nullptr;

  if ((! force_d3d9ex) || FAILED (Direct3DCreate9Ex (SDKVersion, &pD3D9Ex)))
  {
    if (Direct3DCreate9_Import)
    {
      if (force_d3d9ex)
      {
        dll_log.Log ( L"[   D3D9   ] [!] %s (%lu) - "
          L"%s",
          L"Direct3DCreate9", SDKVersion,
            SK_SummarizeCaller ().c_str () );
      }

      d3d9 = Direct3DCreate9_Import (SDKVersion);
    }
  }

  else if (force_d3d9ex)
  {
    return pD3D9Ex;
  }

  if (d3d9 == nullptr)
  {
    d3d9 = Direct3DCreate9_Import (SDKVersion);
  }

  return d3d9;
}

HRESULT
__declspec (noinline)
STDMETHODCALLTYPE
Direct3DCreate9Ex (_In_ UINT SDKVersion, _Out_ IDirect3D9Ex **ppD3D)
{
  WaitForInit_D3D9 ();
  WaitForInit      ();

  dll_log.Log ( L"[   D3D9   ] [!] %s (%lu, %ph) - "
                L"%s",
                  L"Direct3DCreate9Ex",
                    SDKVersion,
                      ppD3D,
                        SK_SummarizeCaller ().c_str () );

  HRESULT       hr     = E_FAIL;
  IDirect3D9Ex* d3d9ex = nullptr;

  if (Direct3DCreate9Ex_Import)
  {
    D3D9_CALL (hr, Direct3DCreate9Ex_Import (SDKVersion, &d3d9ex));

    if (SUCCEEDED (hr) && d3d9ex != nullptr)
    {
      *ppD3D = d3d9ex;
    }
  }

  return hr;
}

void
__stdcall
SK_D3D9_UpdateRenderStats (IDirect3DSwapChain9* pSwapChain, IDirect3DDevice9* pDevice)
{
  if (! ((pDevice || pSwapChain) && config.render.show))
    return;

  PipelineStatsD3D9& pipeline_stats =
    pipeline_stats_d3d9;

  CComPtr <IDirect3DDevice9> dev = pDevice;

  if (pDevice != nullptr || SUCCEEDED (pSwapChain->GetDevice (&dev)))
  {
    if (pipeline_stats.query.object != nullptr)
    {
      if (pipeline_stats.query.active)
      {
        pipeline_stats.query.object->Issue (D3DISSUE_END);
        pipeline_stats.query.active = false;
      }

      else
      {
        HRESULT hr =
          pipeline_stats.query.object->GetData (
                              &pipeline_stats.last_results,
                                sizeof D3DDEVINFO_D3D9PIPELINETIMINGS,
                                  D3DGETDATA_FLUSH );
        if (hr == S_OK)
        {
          pipeline_stats.query.object->Release ();
          pipeline_stats.query.object = nullptr;
        }
      }
    }

    else
    {
      if (SUCCEEDED (dev->CreateQuery (D3DQUERYTYPE_PIPELINETIMINGS, &pipeline_stats.query.object)))
      {
        pipeline_stats.query.object->Issue (D3DISSUE_BEGIN);
        pipeline_stats.query.active = true;
      }
    }
  }
}

std::wstring
WINAPI
SK::D3D9::getPipelineStatsDesc (void)
{
  wchar_t wszDesc [1024] = { };

  D3DDEVINFO_D3D9PIPELINETIMINGS& stats =
    pipeline_stats_d3d9.last_results;

  //
  // VERTEX SHADING
  //
  if (stats.VertexProcessingTimePercent > 0.0f)
  {
    _swprintf ( wszDesc,
                  L"  VERTEX : %5.2f%%\n",
                    stats.VertexProcessingTimePercent );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"  VERTEX : <Unused>\n" );
  }

  //
  // PIXEL SHADING
  //
  if (stats.PixelProcessingTimePercent > 0.0f)
  {
    _swprintf ( wszDesc,
                  L"%s  PIXEL  : %5.2f%%\n",
                    wszDesc,
                      stats.PixelProcessingTimePercent );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  PIXEL  : <Unused>\n",
                    wszDesc );
  }

  //
  // OTHER
  //
  if (stats.OtherGPUProcessingTimePercent > 0.0f)
  {
    _swprintf ( wszDesc,
                  L"%s  OTHER  : %5.2f%%\n",
                    wszDesc, stats.OtherGPUProcessingTimePercent);
  }

  //
  // IDLE
  //
  if (stats.GPUIdleTimePercent > 0.0f)
  {
    _swprintf ( wszDesc,
                  L"%s  IDLE   : %5.2f%%\n",
                    wszDesc,
                      stats.GPUIdleTimePercent );
  }

  return wszDesc;
}

unsigned int
__stdcall
HookD3D9 (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  if (! config.apis.d3d9.hook)
  {
    return 0;
  }

  if (__SK_bypass || ReadAcquire (&__d3d9_ready))
  {
    return 0;
  }

  static volatile LONG __hooked = FALSE;

  if (! InterlockedCompareExchange (&__hooked, TRUE, FALSE))
  {
    SK_TLS_Bottom ()->d3d9.ctx_init_thread = true;

    const bool success = SUCCEEDED (
      CoInitializeEx (nullptr, COINIT_MULTITHREADED)
    );
    {
      CComPtr <IDirect3D9> pD3D9 = nullptr;

      pD3D9 =
        Direct3DCreate9_Import (D3D_SDK_VERSION);

      HWND hwnd = nullptr;

      if (pD3D9 != nullptr)
      {
        hwnd =
          SK_Win32_CreateDummyWindow ();

        D3DPRESENT_PARAMETERS pparams = { };

        pparams.SwapEffect            = D3DSWAPEFFECT_FLIP;
        pparams.BackBufferFormat      = D3DFMT_UNKNOWN;
        pparams.Windowed              = TRUE;
        pparams.BackBufferCount       = 2;
        pparams.hDeviceWindow         = hwnd;
        pparams.BackBufferHeight      = 2;
        pparams.BackBufferWidth       = 2;

        CComPtr <IDirect3DDevice9> pD3D9Dev = nullptr;

        dll_log.Log (L"[   D3D9   ]  Hooking D3D9...");

        HRESULT hr =
          pD3D9->CreateDevice (
                  D3DADAPTER_DEFAULT,
                    D3DDEVTYPE_HAL,
                      nullptr,
                        D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED |
                        D3DCREATE_NOWINDOWCHANGES           | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
                          &pparams,
                            &pD3D9Dev );

        if (SUCCEEDED (hr))
        {
          if (! LocalHook_D3D9CreateDevice.active)
          {
            D3D9_INTERCEPT ( &pD3D9, 16,
                            "IDirect3D9::CreateDevice",
                              D3D9CreateDevice_Override,
                              D3D9_CreateDevice_Original,
                              D3D9_CreateDevice_pfn );

            SK_Hook_TargetFromVFTable (
              LocalHook_D3D9CreateDevice,
                (void **)&pD3D9, 16 );
          }

          //vtbl (This)
          //-----------
          // 3   TestCooperativeLevel
          // 4   GetAvailableTextureMem
          // 5   EvictManagedResources
          // 6   GetDirect3D
          // 7   GetDeviceCaps
          // 8   GetDisplayMode
          // 9   GetCreationParameters
          // 10  SetCursorProperties
          // 11  SetCursorPosition
          // 12  ShowCursor
          // 13  CreateAdditionalSwapChain
          // 14  GetSwapChain
          // 15  GetNumberOfSwapChains
          // 16  Reset
          // 17  Present
          // 18  GetBackBuffer
          // 19  GetRasterStatus
          // 20  SetDialogBoxMode
          // 21  SetGammaRamp
          // 22  GetGammaRamp
          // 23  CreateTexture
          // 24  CreateVolumeTexture
          // 25  CreateCubeTexture
          // 26  CreateVertexBuffer
          // 27  CreateIndexBuffer
          // 28  CreateRenderTarget
          // 29  CreateDepthStencilSurface
          // 30  UpdateSurface
          // 31  UpdateTexture
          // 32  GetRenderTargetData
          // 33  GetFrontBufferData
          // 34  StretchRect
          // 35  ColorFill
          // 36  CreateOffscreenPlainSurface
          // 37  SetRenderTarget
          // 38  GetRenderTarget
          // 39  SetDepthStencilSurface
          // 40  GetDepthStencilSurface
          // 41  BeginScene
          // 42  EndScene
          // 43  Clear
          // 44  SetTransform
          // 45  GetTransform
          // 46  MultiplyTransform
          // 47  SetViewport
          // 48  GetViewport
          // 49  SetMaterial
          // 50  GetMaterial
          // 51  SetLight
          // 52  GetLight
          // 53  LightEnable
          // 54  GetLightEnable
          // 55  SetClipPlane
          // 56  GetClipPlane
          // 57  SetRenderState
          // 58  GetRenderState
          // 59  CreateStateBlock
          // 60  BeginStateBlock
          // 61  EndStateBlock
          // 62  SetClipStatus
          // 63  GetClipStatus
          // 64  GetTexture
          // 65  SetTexture
          // 66  GetTextureStageState
          // 67  SetTextureStageState
          // 68  GetSamplerState
          // 69  SetSamplerState
          // 70  ValidateDevice
          // 71  SetPaletteEntries
          // 72  GetPaletteEntries
          // 73  SetCurrentTexturePalette
          // 74  GetCurrentTexturePalette
          // 75  SetScissorRect
          // 76  GetScissorRect
          // 77  SetSoftwareVertexProcessing
          // 78  GetSoftwareVertexProcessing
          // 79  SetNPatchMode
          // 80  GetNPatchMode
          // 81  DrawPrimitive
          // 82  DrawIndexedPrimitive
          // 83  DrawPrimitiveUP
          // 84  DrawIndexedPrimitiveUP
          // 85  ProcessVertices
          // 86  CreateVertexDeclaration
          // 87  SetVertexDeclaration
          // 88  GetVertexDeclaration
          // 89  SetFVF
          // 90  GetFVF
          // 91  CreateVertexShader
          // 92  SetVertexShader
          // 93  GetVertexShader
          // 94  SetVertexShaderConstantF
          // 95  GetVertexShaderConstantF
          // 96  SetVertexShaderConstantI
          // 97  GetVertexShaderConstantI
          // 98  SetVertexShaderConstantB
          // 99  GetVertexShaderConstantB
          // 100 SetStreamSource
          // 101 GetStreamSource
          // 102 SetStreamSourceFreq
          // 103 GetStreamSourceFreq
          // 104 SetIndices
          // 105 GetIndices
          // 106 CreatePixelShader
          // 107 SetPixelShader
          // 108 GetPixelShader
          // 109 SetPixelShaderConstantF
          // 110 GetPixelShaderConstantF
          // 111 SetPixelShaderConstantI
          // 112 GetPixelShaderConstantI
          // 113 SetPixelShaderConstantB
          // 114 GetPixelShaderConstantB
          // 115 DrawRectPatch
          // 116 DrawTriPatch
          // 117 DeletePatch
          // 118 CreateQuery

          dll_log.Log (L"[   D3D9   ]   * Success");
        }

        else
        {
          _com_error err (hr);
          dll_log.Log ( L"[   D3D9   ]   * Failure  (%s)",
                        err.ErrorMessage () );

          if (! LocalHook_D3D9CreateDevice.active)
          {
            D3D9_INTERCEPT ( &pD3D9, 16,
                            "IDirect3D9::CreateDevice",
                              D3D9CreateDevice_Override,
                              D3D9_CreateDevice_Original,
                              D3D9_CreateDevice_pfn );

            SK_Hook_TargetFromVFTable (
              LocalHook_D3D9CreateDevice,
                (void **)&pD3D9, 16 );
          }
        }

        SK_Win32_CleanupDummyWindow ();

        CComPtr <IDirect3D9Ex> pD3D9Ex = nullptr;

        hr = (config.apis.d3d9ex.hook) ?
          Direct3DCreate9Ex_Import (D3D_SDK_VERSION, &pD3D9Ex)
                           :
                      E_NOINTERFACE;

        hwnd = nullptr;

        if (SUCCEEDED (hr))
        {
          dll_log.Log (L"[   D3D9   ]  Hooking D3D9Ex...");

          hwnd    =
            SK_Win32_CreateDummyWindow ();
          pparams = { };

          pparams.SwapEffect            = D3DSWAPEFFECT_FLIPEX;
          pparams.BackBufferFormat      = D3DFMT_UNKNOWN;
          pparams.Windowed              = TRUE;
          pparams.BackBufferCount       = 2;
          pparams.hDeviceWindow         = hwnd;
          pparams.BackBufferHeight      = 2;
          pparams.BackBufferWidth       = 2;

          CComPtr <IDirect3DDevice9Ex> pD3D9DevEx = nullptr;

          if ( SUCCEEDED ( pD3D9Ex->CreateDeviceEx (
                            D3DADAPTER_DEFAULT,
                              D3DDEVTYPE_HAL,
                                 hwnd,
                                  D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                    &pparams,
                                      nullptr,
                                        &pD3D9DevEx )
                        )
            )
          {
            if (! LocalHook_D3D9CreateDeviceEx.active)
            {
              D3D9_INTERCEPT ( &pD3D9Ex, 20,
                               "IDirect3D9Ex::CreateDeviceEx",
                                D3D9CreateDeviceEx_Override,
                                D3D9Ex_CreateDeviceEx_Original,
                                D3D9Ex_CreateDeviceEx_pfn );

              SK_Hook_TargetFromVFTable (
                LocalHook_D3D9CreateDeviceEx,
                  (void **)&pD3D9Ex, 20 );
            }

            //IDirect3DDevice9Ex **ppReturnedDeviceInterface = &pD3D9DevEx;

            SK_D3D9_HookReset   (pD3D9DevEx);
            SK_D3D9_HookPresent (pD3D9DevEx);

            // Initialize stuff...
            SK_D3D9RenderBackend::getInstance ();

            dll_log.Log (L"[   D3D9   ]   * Success");
          }

          else
          {
            dll_log.Log (L"[   D3D9   ]   * Failure");

            if (! LocalHook_D3D9CreateDeviceEx.active)
            {
              D3D9_INTERCEPT ( &pD3D9Ex, 20,
                               "IDirect3D9Ex::CreateDeviceEx",
                                D3D9CreateDeviceEx_Override,
                                D3D9Ex_CreateDeviceEx_Original,
                                D3D9Ex_CreateDeviceEx_pfn );

              SK_Hook_TargetFromVFTable (
                LocalHook_D3D9CreateDeviceEx,
                  (void **)&pD3D9Ex, 20 );
            }
          }

          SK_Win32_CleanupDummyWindow ();
        }

        else
        {
          if (pD3D9Dev != nullptr)
          {
            SK_D3D9_HookReset   (pD3D9Dev);
            SK_D3D9_HookPresent (pD3D9Dev);
          }

          // Initialize stuff...
          SK_D3D9RenderBackend::getInstance ();
        }

        SK_ApplyQueuedHooks  (                   );
        InterlockedExchange  (&__d3d9_ready, TRUE);
        InterlockedIncrement (&__hooked);

        if (! (SK_GetDLLRole () & DLL_ROLE::DXGI))
          SK::DXGI::StartBudgetThread_NoAdapter ();
      }
    }
    //if (success)
    //  CoUninitialize ();
  }

  SK_Thread_SpinUntilAtomicMin (&__hooked, 2);

  return 0;
}

unsigned int
__stdcall
HookD3D9Ex (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  return 0;
}

void
SK_D3D9_TriggerReset (bool temporary)
{
  trigger_reset =
    reset_stage_e::Initiate;

  if (temporary)
  {
    request_mode_change =
      SK_GetCurrentRenderBackend ().fullscreen_exclusive ?
         mode_change_request_e::Windowed :
         mode_change_request_e::Fullscreen;
  }
}






#include "DLL_VERSION.H"
#include <imgui/imgui.h>

#include <string>
#include <vector>
#include <algorithm>

#include "config.h"
#include "command.h"

#include <atlbase.h>

extern bool
SK_ImGui_IsItemClicked (void);

void
SK_D3D9_DrawFileList (bool& can_scroll)
{
  const float font_size = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;

  ImGui::PushItemWidth (500.0f);

  struct enumerated_source_s
  {
    std::string            name       = "invalid";
    std::vector <uint32_t> checksums;

    struct {
      std::vector < std::pair < uint32_t, SK::D3D9::TexRecord > >
                 records;
      uint64_t   size                 = 0ULL;
    } streaming, blocking;

    uint64_t     totalSize (void) { return streaming.size + blocking.size; };
  };

  static std::vector <enumerated_source_s> sources;
  static SK::D3D9::TexList                 injectable;
  static std::vector < std::wstring >      archives;
  static bool                              list_dirty = true;
  static int                               sel        = 0;

  auto EnumerateSource =
    [](unsigned int archive_no) ->
      enumerated_source_s
      {
        enumerated_source_s source;

        char szFileName [MAX_PATH] = { '\0' };

        if (archive_no != std::numeric_limits <unsigned int>::max ()) {
          sprintf (szFileName, "%ws", archives [archive_no].c_str ());
        }

        else strncpy (szFileName, "Regular Filesystem", MAX_PATH);

        source.name = szFileName;

        for ( auto it : injectable )
        {
          if (it.second.archive == archive_no)
          {
            switch (it.second.method)
            {
              case DontCare:
              case Streaming:
                source.streaming.records.emplace_back (std::make_pair (it.first, it.second));
                source.streaming.size += it.second.size;
                break;

              case Blocking:
                source.blocking.records.emplace_back (std::make_pair (it.first, it.second));
                source.blocking.size += it.second.size;
                break;
            }

            source.checksums.emplace_back (it.first);
          }
        }

        return source;
      };

  if (list_dirty)
  {
    SK::D3D9::tex_mgr.getInjectableTextures (injectable);
    SK::D3D9::tex_mgr.getTextureArchives    (archives);

    sources.clear  ();

        sel = 0;
    int idx = 0;

    // First the .7z Data Sources
    for ( auto it : archives )
    {
      sources.emplace_back (EnumerateSource (idx++));
    }

    // Then the Straight Filesystem
    sources.emplace_back (EnumerateSource (std::numeric_limits <unsigned int>::max ()));

    list_dirty = false;
  }

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border,                   ImVec4 (0.4f, 0.6f, 0.9f, 1.0f));

#define FILE_LIST_WIDTH  font_size * 20
#define FILE_LIST_HEIGHT font_size * (sources.size () + 3)

  ImGui::BeginChild ( ImGui::GetID ("Source List"),
                        ImVec2 ( FILE_LIST_WIDTH, FILE_LIST_HEIGHT ),
                          true,
                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

  if (ImGui::IsWindowHovered ())
    can_scroll = false;

  auto DataSourceTooltip =
    [](int sel) ->
      void
      {
        ImGui::BeginTooltip ();
        ImGui::TextColored  (ImVec4 (0.9f, 0.7f, 0.3f, 1.f), "Data Source  (%s)", sources [sel].name.c_str ());
        ImGui::Separator    ();

        ImGui::BeginGroup      (    );
        ImGui::TextUnformatted ( "Total Size:     ");
        ImGui::TextUnformatted ( "Blocking Data:  ");
        ImGui::TextUnformatted ( "Streaming Data: ");
        ImGui::EndGroup        (    );

        ImGui::SameLine        (    );

        ImGui::BeginGroup      (    );
        ImGui::TextUnformatted ( "" );
        ImGui::Text            ( "%4lu File%c", sources [sel].blocking.records.size (),
                                                sources [sel].blocking.records.size () != 1 ? 's' : ' ' );
        ImGui::Text            ( "%4lu File%c", sources [sel].streaming.records.size (),
                                                sources [sel].streaming.records.size () != 1 ? 's' : ' ' );
        ImGui::EndGroup        (    );

        ImGui::SameLine        (    );

        ImGui::BeginGroup      (    );
        ImGui::Text            ( "  %#5.2f MiB",
                                   (double)sources [sel].totalSize () / ( 1024.0 * 1024.0 ) );
        ImGui::Text            ( " (%#5.2f MiB)",
                                   (double)sources [sel].blocking.size / (1024.0 * 1024.0) );
        ImGui::Text            ( " (%#5.2f MiB)",
                                   (double)sources [sel].streaming.size / (1024.0 * 1024.0) );
        ImGui::EndGroup        (    );

        ImGui::EndTooltip    ();
      };

  if (! sources.empty ())
  {
    static      int last_sel = 0;
    static bool sel_changed  = false;

    if (sel != last_sel)
      sel_changed = true;

    last_sel = sel;

    for ( int line = 0; line < static_cast <int> (sources.size ()); line++)
    {
      if (line == sel)
      {
        bool selected = true;
        ImGui::Selectable (sources [line].name.c_str (), &selected);

        if (sel_changed)
        {
          ImGui::SetScrollHere        (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
          ImGui::SetKeyboardFocusHere (    );

          sel_changed = false;
        }
      }

      else
      {
        bool selected = false;

        if (ImGui::Selectable (sources [line].name.c_str (), &selected))
        {
          sel_changed = true;
          //tex_dbg_idx                 =  line;
          sel                         =  line;
          //debug_tex_id                =  textures_used_last_dump [line];
        }
      }

      if (ImGui::IsItemHovered ())
        DataSourceTooltip (line);
    }

    ImGui::EndChild      ();

    ImVec2 list_size = ImGui::GetItemRectSize ();

    ImGui::PopStyleColor ();
    ImGui::PopStyleVar   ();

    ImGui::SameLine      ();

    ImGui::BeginGroup    ();

    ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::BeginChild ( ImGui::GetID ("Texture Selection"),
                           ImVec2 (font_size * 30, list_size.y - font_size * 2),
                             true );

    if (ImGui::IsWindowHovered ())
      can_scroll = false;

    ImGui::BeginGroup ();
    for ( auto it : sources [sel].checksums )
    {
      const TexRecord& inject_tex =
        tex_mgr.getInjectableTexture (it);

      if (inject_tex.size != 0)
      {
        ImGui::TextColored ( ImVec4 (0.9f, 0.6f, 0.3f, 1.0f), " %08x    ", it );
      }
    }
    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    for ( auto it : sources [sel].checksums )
    {
      const TexRecord& inject_tex =
        tex_mgr.getInjectableTexture (it);

      if (inject_tex.size != 0)
      {
        bool streaming =
          inject_tex.method == Streaming;

        ImGui::TextColored ( streaming ?
                               ImVec4 ( 0.2f,  0.90f, 0.3f, 1.0f ) :
                               ImVec4 ( 0.90f, 0.3f,  0.2f, 1.0f ),
                                 "  %s    ",
                                   streaming ? "Streaming" :
                                               " Blocking" );
      }
    }
    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    for ( auto it : sources [sel].checksums )
    {
      const TexRecord& inject_tex =
        tex_mgr.getInjectableTexture (it);

      if (inject_tex.size != 0)
      {
        ImGui::TextColored ( ImVec4 (1.f, 1.f, 1.f, 1.f), "%#5.2f MiB  ",
                            (double)inject_tex.size / (1024.0 * 1024.0) );
      }
    }
    ImGui::EndGroup   ();


    ImGui::EndChild      (   );
    ImGui::PopStyleColor ( 1 );

    if (trigger_reset == reset_stage_e::Clear)
    {
      if (ImGui::Button ("  Refresh Data Sources  "))
      {
        tex_mgr.refreshDataSources ();
        list_dirty = true;
      }

      ImGui::SameLine ();

      if (ImGui::Button ("  Reload All Textures  "))
      {
        SK_D3D9_TriggerReset (false);
      }
    }

    ImGui::EndGroup ();
  }

  ImGui::PopItemWidth ();
}



void
SK_D3D9_LiveShaderClassView (SK::D3D9::ShaderClass shader_type, bool& can_scroll)
{
  ImGui::BeginGroup ();

  static float last_width = 256.0f;
  const  float font_size  = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;

  struct shader_class_imp_s
  {
    std::vector <std::string> contents;
    bool                      dirty      = true;
    uint32_t                  last_sel   =    0;
    int                            sel   =   -1;
    float                     last_ht    = 256.0f;
    ImVec2                    last_min   = ImVec2 (0.0f, 0.0f);
    ImVec2                    last_max   = ImVec2 (0.0f, 0.0f);
  };

  struct {
    shader_class_imp_s vs;
    shader_class_imp_s ps;
  } static list_base;

  shader_class_imp_s*
    list    = ( shader_type == SK::D3D9::ShaderClass::Pixel ? &list_base.ps :
                                                              &list_base.vs );

  SK::D3D9::ShaderTracker*
    tracker = ( shader_type == SK::D3D9::ShaderClass::Pixel ? &tracked_ps :
                                                              &tracked_vs );

  std::vector <uint32_t>
    shaders   ( shader_type == SK::D3D9::ShaderClass::Pixel ? last_frame.pixel_shaders.begin  () :
                                                              last_frame.vertex_shaders.begin (),
                shader_type == SK::D3D9::ShaderClass::Pixel ? last_frame.pixel_shaders.end    () :
                                                              last_frame.vertex_shaders.end   () );

  std::unordered_map <uint32_t, SK::D3D9::ShaderDisassembly>&
    disassembly = ( shader_type == SK::D3D9::ShaderClass::Pixel ? ps_disassembly :
                                                                  vs_disassembly );

  const char*
    szShaderWord =  shader_type == SK::D3D9::ShaderClass::Pixel ? "Pixel" :
                                                                  "Vertex";

  if (list->dirty)
  {
        list->sel = -1;
    int idx       =  0;
        list->contents.clear ();

    // The underlying list is unsorted for speed, but that's not at all
    //   intuitive to humans, so sort the thing when we have the RT view open.
    std::sort ( shaders.begin (),
                shaders.end   () );



    for ( auto it : shaders )
    {
      char szDesc [16] = { };

#ifdef _WIN64
      sprintf (szDesc, "%08llx", (uintptr_t)it);
#else
      sprintf (szDesc, "%08lx",  (uintptr_t)it);
#endif

      list->contents.emplace_back (szDesc);

      if ((uint32_t)it == list->last_sel)
      {
        list->sel = idx;
        //tracked_rt.tracking_tex = render_textures [sel];
      }

      ++idx;
    }
  }

  if (ImGui::IsMouseHoveringRect (list->last_min, list->last_max))
  {
         if (ImGui::GetIO ().KeysDown [VK_OEM_4] && ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) { list->sel--;  ImGui::GetIO ().WantCaptureKeyboard = true; }
    else if (ImGui::GetIO ().KeysDown [VK_OEM_6] && ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) { list->sel++;  ImGui::GetIO ().WantCaptureKeyboard = true; }
  }

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  ImGui::BeginChild ( ImGui::GetID (szShaderWord),
                      ImVec2 ( font_size * 7.0f, std::max (font_size * 15.0f, list->last_ht)), true,
                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

  if (ImGui::IsWindowHovered ())
  {
    can_scroll = false;

    ImGui::BeginTooltip ();
    ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected %s shader to disable an effect", szShaderWord);
    ImGui::Separator    ();
    ImGui::BulletText   ("Press [ while the mouse is hovering this list to select the previous shader");
    ImGui::BulletText   ("Press ] while the mouse is hovering this list to select the next shader");
    ImGui::EndTooltip   ();

         if (ImGui::GetIO ().KeysDown [VK_OEM_4] && ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) { list->sel--;  ImGui::GetIO ().WantCaptureKeyboard = true; }
    else if (ImGui::GetIO ().KeysDown [VK_OEM_6] && ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) { list->sel++;  ImGui::GetIO ().WantCaptureKeyboard = true; }
  }

  if (! shaders.empty ())
  {
    struct {
      int  last_sel    = 0;
      bool sel_changed = false;
    } static shader_state [3];

    int&  last_sel    = shader_state [static_cast <int> (shader_type)].last_sel;
    bool& sel_changed = shader_state [static_cast <int> (shader_type)].sel_changed;

    if (list->sel != last_sel)
      sel_changed = true;

    last_sel = list->sel;

    for ( size_t line = 0; line < shaders.size (); line++ )
    {
      if (line == static_cast <size_t> (list->sel))
      {
        bool selected    = true;

        ImGui::Selectable (list->contents [line].c_str (), &selected);

        if (sel_changed)
        {
          ImGui::SetScrollHere        (0.5f);
          ImGui::SetKeyboardFocusHere (    );

          sel_changed     = false;
          list->last_sel  = (uint32_t)shaders [list->sel];
          tracker->crc32c = (uint32_t)shaders [list->sel];
        }
      }

      else
      {
        bool selected    = false;

        if (ImGui::Selectable (list->contents [line].c_str (), &selected))
        {
          sel_changed     = true;
          list->sel       = (int)line;
          list->last_sel  = (uint32_t)shaders [list->sel];
          tracker->crc32c = (uint32_t)shaders [list->sel];
        }
      }
    }
  }

  ImGui::EndChild      ();
  ImGui::PopStyleColor ();

  ImGui::SameLine      ();
  ImGui::BeginGroup    ();

  if (ImGui::IsItemHoveredRect ()) {
         if (ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) list->sel--;
    else if (ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) list->sel++;
  }

  if (tracker->crc32c != 0x00)
  {
    ImGui::BeginGroup ();
    ImGui::Checkbox ( shader_type == SK::D3D9::ShaderClass::Pixel ? "Cancel Draws Using Selected Pixel Shader" :
                                                                    "Cancel Draws Using Selected Vertex Shader",
                        &tracker->cancel_draws );  ImGui::SameLine ();

    LONG num_draws =
     ReadAcquire (&tracker->num_draws);

    if (tracker->cancel_draws)
      ImGui::TextDisabled ("%lu Skipped Draw%c Last Frame (%lu textures)", num_draws, num_draws != 1 ? 's' : ' ', tracker->used_textures.size () );
    else
      ImGui::TextDisabled ("%lu Draw%c Last Frame         (%lu textures)", num_draws, num_draws != 1 ? 's' : ' ', tracker->used_textures.size () );

    ImGui::Checkbox ( shader_type == SK::D3D9::ShaderClass::Pixel ? "Clamp Texture Coordinates For Selected Pixel Shader" :
                                                                    "Clamp Texture Coordinates For Selected Vertex Shader",
                        &tracker->clamp_coords );

    ImGui::Separator      ();
    ImGui::EndGroup       ();

    if (ImGui::IsItemHoveredRect () && ! tracker->used_textures.empty ())
    {
      ImGui::BeginTooltip ();

      D3DFORMAT fmt = D3DFMT_UNKNOWN;

      for ( auto it : tracker->used_textures )
      {
        IDirect3DBaseTexture9* pTex = it;//ISKTextureD3D9* pTex = tex_mgr.getTexture (it)->d3d9_tex;

        if (pTex /*&& pTex->pTex*/)
        {
          D3DSURFACE_DESC desc;
          if (SUCCEEDED (((IDirect3DTexture9 *)pTex/*->pTex*/)->GetLevelDesc (0, &desc)))
          {
            fmt = desc.Format;
            ImGui::Image ( pTex/*->pTex*/, ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
      ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (242,242,13,255) );
          }

          ImGui::SameLine ();

          ImGui::BeginGroup ();
          ImGui::TextUnformatted ("Texture:");
          ImGui::TextUnformatted ("Format: ");
          ImGui::EndGroup   ();

          ImGui::SameLine    ();

          ImGui::BeginGroup  ();
          ImGui::Text        ("%08lx", it);
          ImGui::Text        ("%ws",   SK_D3D9_FormatToStr (fmt).c_str ());
          ImGui::EndGroup    ();
        }
      }

      ImGui::EndTooltip ();
    }

    ImGui::PushFont (ImGui::GetIO ().Fonts->Fonts [1]); // Fixed-width font

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.80f, 0.80f, 1.0f, 1.0f));
    ImGui::TextWrapped    (disassembly [tracker->crc32c].header.c_str ());

    ImGui::SameLine       ();
    ImGui::BeginGroup     ();
    ImGui::TreePush       ("");
    ImGui::Spacing        (); ImGui::Spacing ();
    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.666f, 0.666f, 0.666f, 1.0f));

    ImGui::PushItemWidth (font_size * 25);

    char szName [128] = { };

    ImGui::PushID (tracker->crc32c);

    for ( auto& it : tracker->constants )
    {
      ImGui::PushID (it.RegisterIndex);

      if (! it.struct_members.empty ())
      {
        ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.9f, 0.1f, 0.7f, 1.0f));
        ImGui::TextUnformatted (it.Name);
        ImGui::PopStyleColor   ();

        for ( auto& it2 : it.struct_members )
        {
          ImGui::PushID (it2.RegisterIndex);

          bool tweakable = false;

          if (it2.Type == D3DXPT_FLOAT)
          {
            snprintf ( szName, 127, "[%s] %-32s",
                         shader_type == SK::D3D9::ShaderClass::Pixel ? "ps" :
                                                                       "vs",
                           it2.Name );

            if (it2.Class == D3DXPC_VECTOR ||
                it2.Class == D3DXPC_SCALAR)
            {
              switch (it2.Columns)
              {
                case 1:
                  tweakable = true;
                  ImGui::Checkbox    (szName, &it2.Override);
                  ImGui::SameLine    ( );
                  ImGui::InputFloat  ("", it2.Data);
                  break;
                case 2:
                  tweakable = true;
                  ImGui::Checkbox    (szName, &it2.Override);
                  ImGui::SameLine    ( );
                  ImGui::InputFloat2 ("", it2.Data);
                  break;
                case 3:
                  tweakable = true;
                  ImGui::Checkbox    (szName, &it2.Override);
                  ImGui::SameLine    ( );
                  ImGui::InputFloat3 ("", it2.Data);
                  break;
                case 4:
                  tweakable = true;
                  ImGui::Checkbox    (szName, &it2.Override);
                  ImGui::SameLine    ( );
                  ImGui::InputFloat4 ("", it2.Data);
                  break;
              }
            }
          }

          if (! tweakable)
          {
            snprintf ( szName, 127, "[%s] %-32s :(%c%-3lu)",
                         shader_type == SK::D3D9::ShaderClass::Pixel ? "ps" :
                                                                       "vs",
                           it2.Name,
                             it2.RegisterSet != D3DXRS_SAMPLER ? 'c' : 's',
                               it2.RegisterIndex );

            ImGui::TreePush (""); ImGui::TextColored (ImVec4 (0.45f, 0.75f, 0.45f, 1.0f), szName); ImGui::TreePop ();
          }

          ImGui::PopID ();
        }

        ImGui::Separator ();
      }

      else
      {
        bool tweakable = false;

        snprintf ( szName, 127, "[%s] %-32s",
                     shader_type == SK::D3D9::ShaderClass::Pixel ? "ps" :
                                                                   "vs",
                       it.Name );

        if (it.Type == D3DXPT_FLOAT)
        {
          if (it.Class == D3DXPC_VECTOR ||
              it.Class == D3DXPC_SCALAR)
          {
            switch (it.Columns)
            {
              case 1:
                tweakable = true;
                ImGui::Checkbox    (szName, &it.Override);
                ImGui::SameLine    ( );
                ImGui::InputFloat  ("", it.Data);
                break;
              case 2:
                tweakable = true;
                ImGui::Checkbox    (szName, &it.Override);
                ImGui::SameLine    ( );
                ImGui::InputFloat2 ("", it.Data);
                break;
              case 3:
                tweakable = true;
                ImGui::Checkbox    (szName, &it.Override);
                ImGui::SameLine    ( );
                ImGui::InputFloat3 ("", it.Data);
                break;
              case 4:
                tweakable = true;
                ImGui::Checkbox    (szName, &it.Override);
                ImGui::SameLine    ( );
                ImGui::InputFloat4 ("", it.Data);
                break;
            }
          }
        }

        if (it.Type == D3DXPT_BOOL)
        {
          if (it.Class == D3DXPC_SCALAR)
          {
            switch (it.Columns)
            {
              case 1:
                tweakable = true;
                ImGui::Checkbox (szName, &it.Override);
                ImGui::SameLine ( );
                ImGui::Checkbox ("Enable", reinterpret_cast <bool *> (it.Data));
                break;
            }
          }
        }

        if (! tweakable)
        {
          snprintf ( szName, 127, "[%s] %-32s :(%c%-3lu)",
                       shader_type == SK::D3D9::ShaderClass::Pixel ? "ps" :
                                                                     "vs",
                         it.Name,
                           it.RegisterSet != D3DXRS_SAMPLER ? 'c' : 's',
                             it.RegisterIndex );

          ImGui::TreePush (""); ImGui::TextColored (ImVec4 (0.45f, 0.75f, 0.45f, 1.0f), szName); ImGui::TreePop ();
        }
      }
      ImGui::PopID ();
    }
    ImGui::PopItemWidth ();
    ImGui::TreePop      ();
    ImGui::EndGroup     ();

    ImGui::Separator      ();

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.99f, 0.99f, 0.01f, 1.0f));
    ImGui::TextWrapped    (disassembly [tracker->crc32c].code.c_str ());

    ImGui::Separator      ();

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.5f, 0.95f, 0.5f, 1.0f));
    ImGui::TextWrapped    (disassembly [tracker->crc32c].footer.c_str ());
    ImGui::PopFont        ();

    ImGui::PopStyleColor (4);
  }
  else
    tracker->cancel_draws = false;

  ImGui::EndGroup      ();

  list->last_ht    = ImGui::GetItemRectSize ().y;

  list->last_min   = ImGui::GetItemRectMin ();
  list->last_max   = ImGui::GetItemRectMax ();

  ImGui::PopStyleVar   ();
  ImGui::EndGroup      ();
}

void
SK_LiveVertexStreamView (bool& can_scroll)
{
  static int filter_type = 0; // ALL

  ImGui::BeginGroup ();

  static float last_width = 256.0f;
  const  float font_size  = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;

  struct vertex_stream_s
  {
    std::vector <std::string> contents;
    bool                      dirty      = true;
    uintptr_t                 last_sel   =    0;
    int                            sel   =   -1;
    float                     last_ht    = 256.0f;
    ImVec2                    last_min   = ImVec2 (0.0f, 0.0f);
    ImVec2                    last_max   = ImVec2 (0.0f, 0.0f);
  };

  struct {
    vertex_stream_s stream0;
  } static list_base;

  vertex_stream_s*
    list    = &list_base.stream0;

  SK::D3D9::VertexBufferTracker*
    tracker = &tracked_vb;

  std::vector <IDirect3DVertexBuffer9 *> buffers;

  switch (filter_type)
  {
    case 2:
      for (auto it : last_frame.vertex_buffers.immutable) if (it != nullptr) buffers.emplace_back (it);
      break;
    case 1:
      for (auto it : last_frame.vertex_buffers.dynamic)   if (it != nullptr) buffers.emplace_back (it);
      break;
    case 0:
      for (auto it : last_frame.vertex_buffers.immutable) if (it != nullptr) buffers.emplace_back (it);
      for (auto it : last_frame.vertex_buffers.dynamic)   if (it != nullptr) buffers.emplace_back (it);
      break;
  };

  if (list->dirty)
  {
        list->sel = -1;
    int idx    =  0;
        list->contents.clear ();

    // The underlying list is unsorted for speed, but that's not at all
    //   intuitive to humans, so sort the thing when we have the RT view open.
    std::sort ( buffers.begin (),
                buffers.end   () );



    for ( auto it : buffers )
    {
      char szDesc [16] = { };

#ifdef _WIN64
      sprintf (szDesc, "%08llx", (uintptr_t)it);
#else
      sprintf (szDesc, "%08lx",  (uintptr_t)it);
#endif

      list->contents.emplace_back (szDesc);

      if ((uintptr_t)it == list->last_sel)
      {
        list->sel = idx;
        //tracked_rt.tracking_tex = render_textures [sel];
      }

      ++idx;
    }
  }

  if (ImGui::IsMouseHoveringRect (list->last_min, list->last_max))
  {
         if (ImGui::GetIO ().KeysDown [VK_OEM_4] && ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) { list->sel--;  ImGui::GetIO ().WantCaptureKeyboard = true; }
    else if (ImGui::GetIO ().KeysDown [VK_OEM_6] && ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) { list->sel++;  ImGui::GetIO ().WantCaptureKeyboard = true; }
  }

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  ImGui::BeginChild ( ImGui::GetID ("Stream 0"),
                      ImVec2 ( font_size * 7.0f, std::max (font_size * 15.0f, list->last_ht)),
                        true,
                          ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

  if (ImGui::IsWindowHovered ())
  {
    can_scroll = false;

    ImGui::BeginTooltip ();
    ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected vertex buffer to debug a model");
    ImGui::Separator    ();
    ImGui::BulletText   ("Press [ while the mouse is hovering this list to select the previous shader");
    ImGui::BulletText   ("Press ] while the mouse is hovering this list to select the next shader");
    ImGui::EndTooltip   ();

         if (ImGui::GetIO ().KeysDown [VK_OEM_4] && ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) { list->sel--;  ImGui::GetIO ().WantCaptureKeyboard = true; }
    else if (ImGui::GetIO ().KeysDown [VK_OEM_6] && ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) { list->sel++;  ImGui::GetIO ().WantCaptureKeyboard = true; }
  }

  if (! buffers.empty ())
  {
    struct {
      int  last_sel    = 0;
      bool sel_changed = false;
    } static stream [3];

    int&  last_sel    = stream [0].last_sel;
    bool& sel_changed = stream [0].sel_changed;

    if (list->sel != last_sel)
      sel_changed = true;

    last_sel = list->sel;

    for ( size_t line = 0; line < buffers.size (); line++ )
    {
      if (line == static_cast <size_t> (list->sel))
      {
        bool selected    = true;

        ImGui::Selectable (list->contents [line].c_str (), &selected);

        if (sel_changed)
        {
          ImGui::SetScrollHere (0.5f);

          sel_changed            = false;
          list->last_sel         = (uintptr_t)buffers [list->sel];
          tracker->vertex_buffer =            buffers [list->sel];
        }
      }

      else
      {
        bool selected    = false;

        if (ImGui::Selectable (list->contents [line].c_str (), &selected))
        {
          sel_changed            = true;
          list->sel              = (int)line;
          list->last_sel         = (uintptr_t)buffers [list->sel];
          tracker->vertex_buffer =            buffers [list->sel];
        }
      }
    }
  }

  else {
    tracker->vertex_buffer = nullptr;
  }

  ImGui::EndChild      ();
  ImGui::PopStyleColor ();

  ImGui::SameLine      ();
  ImGui::BeginGroup    ();

  if (ImGui::IsItemHoveredRect ())
  {
         if (ImGui::GetIO ().KeysDown [VK_OEM_4] && ImGui::GetIO ().KeysDownDuration [VK_OEM_4] == 0.0f) { list->sel--;  ImGui::GetIO ().WantCaptureKeyboard = true; }
    else if (ImGui::GetIO ().KeysDown [VK_OEM_6] && ImGui::GetIO ().KeysDownDuration [VK_OEM_6] == 0.0f) { list->sel++;  ImGui::GetIO ().WantCaptureKeyboard = true; }

    if ( ImGui::GetIO ().KeysDownDuration [    'W'   ] == 0.0f &&
         ImGui::GetIO ().KeysDown         [VK_CONTROL]         &&
         ImGui::GetIO ().KeysDown         [VK_SHIFT  ]            )
    {
      ImGui::GetIO ().WantCaptureKeyboard = true;

      if (tracker->vertex_buffer != nullptr)
      {
        bool wireframe =
          tracker->wireframes.count (tracker->vertex_buffer);

        if (wireframe)
          tracker->wireframes.erase   (tracker->vertex_buffer);
        else
          tracker->wireframes.emplace (tracker->vertex_buffer);
      }
    }
  }

  ImGui::Combo ("Vertex Buffer Filter", &filter_type, "  All   Geometry (Huge List)\0"
                                                      "Dynamic Geometry (Models)\0"
                                                      " Static Geometry (World)\0\0", 3 );

  ImGui::Checkbox ("Cancel Draws Using Selected Vertex Buffer",  &tracker->cancel_draws);  ImGui::SameLine ();

  LONG num_draws = ReadAcquire (&tracker->num_draws);
  LONG instanced = ReadAcquire (&tracker->instanced);

  if (tracker->cancel_draws)
    ImGui::TextDisabled ("%lu Skipped Draw%c Last Frame [%lu Instanced]", num_draws, num_draws != 1 ? 's' : ' ', instanced);
  else
    ImGui::TextDisabled ("%lu Draw%c Last Frame [%lu Instanced]        ", num_draws, num_draws != 1 ? 's' : ' ', instanced);

  ImGui::Checkbox ("Highlight Selected Vertex Buffer (Wireframe)", &tracker->wireframe);

  ImGui::Separator ();


  if ( tracker->vertex_buffer != nullptr &&
         ( last_frame.vertex_buffers.dynamic.count   (tracker->vertex_buffer) ||
           last_frame.vertex_buffers.immutable.count (tracker->vertex_buffer) ) )
  {
    bool wireframe =
      tracker->wireframes.count (tracker->vertex_buffer);

    extern std::wstring
    SK_D3D9_UsageToStr (DWORD dwUsage);

    D3DVERTEXBUFFER_DESC desc = { };

    if (SUCCEEDED (tracker->vertex_buffer->GetDesc (&desc)))
    {
      ImGui::BeginGroup   ();
      ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 20.0f);

      ImVec4 border_color = wireframe ? ImVec4 (1.0f, 0.5f, 0.5f, 1.0f) :
                              tracker->wireframe ?
                                ImVec4 (0.5f, 0.5f, 1.0f, 1.0f) :
                                ImVec4 (0.6f, 0.6f, 0.6f, 1.0f);

      ImGui::PushStyleColor (ImGuiCol_Border, border_color);

      static int last_count = 1;

      ImGui::BeginChild     ( ImGui::GetID ("Buffer Overview"),
                              ImVec2 ( font_size * 44.0f,
                                       font_size * 8.0f +
                                       font_size * (float)last_count ),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize );

      ImGui::BeginGroup  ();
      ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), "Format:  ");
      ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), "Type:    ");
      ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), "Usage:   ");
      ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), "Size:    ");
      ImGui::EndGroup    ();

      ImGui::SameLine    ();

      ImGui::BeginGroup  ();
      ImGui::TextColored (ImVec4 (1.0f, 1.0f, 0.4f, 1.0f), "%ws",  SK_D3D9_FormatToStr (desc.Format).c_str ());
      ImGui::TextColored (ImVec4 (1.0f, 1.0f, 0.4f, 1.0f), "%s",  desc.Type == D3DRTYPE_VERTEXBUFFER ? "Vertex Buffer" :
                                                                  desc.Type == D3DRTYPE_INDEXBUFFER  ? "Index Buffer"  :
                                                                                                       "Unknown?!" );
      ImGui::TextColored (ImVec4 (1.0f, 1.0f, 0.4f, 1.0f), "%ws",  SK_D3D9_UsageToStr (desc.Usage).c_str ());
      ImGui::TextColored (ImVec4 (1.0f, 1.0f, 0.4f, 1.0f), "%lu", desc.Size);
      ImGui::EndGroup    ();

      last_count = 0;

      for (auto vtx_decl : tracker->vertex_decls)
      {
        ++last_count;

        static D3DVERTEXELEMENT9 elem_decl [MAXD3DDECLLENGTH];
        static UINT              num_elems;

        auto SK_D3D9_DeclTypeToStr = [](D3DDECLTYPE type) ->
          const char*
          {
            switch (type)
            {
              case D3DDECLTYPE_FLOAT1:     return "float";
              case D3DDECLTYPE_FLOAT2:     return "float2";
              case D3DDECLTYPE_FLOAT3:     return "float3";
              case D3DDECLTYPE_FLOAT4:     return "float4";
              case D3DDECLTYPE_D3DCOLOR:   return "D3DCOLOR";
              case D3DDECLTYPE_UBYTE4:     return "ubyte4";
              case D3DDECLTYPE_SHORT2:     return "short2";
              case D3DDECLTYPE_SHORT4:     return "short4";
              case D3DDECLTYPE_UBYTE4N:    return "ubyte4 (UNORM)";
              case D3DDECLTYPE_SHORT2N:    return "short2 (SNORM)";
              case D3DDECLTYPE_SHORT4N:    return "short2 (SNORM)";
              case D3DDECLTYPE_USHORT2N:   return "short2 (UNORM)";
              case D3DDECLTYPE_USHORT4N:   return "short4 (UNORM)";
              case D3DDECLTYPE_UDEC3:      return "udec3";
              case D3DDECLTYPE_DEC3N:      return "dec3 (NORM)";
              case D3DDECLTYPE_FLOAT16_2:  return "half2";
              case D3DDECLTYPE_FLOAT16_4:  return "half4";
              case D3DDECLTYPE_UNUSED:     return "UNUSED";
            }

            return "UNKNOWN";
          };

        auto SK_D3D9_DeclUsageToStr = [](D3DDECLUSAGE usage, int idx, int part) ->
          const char*
          {
            static char szOut [64] = { '\0' };

            switch (part)
            {
              case 0:
                switch (usage)
                {
                  case D3DDECLUSAGE_POSITION:     snprintf (szOut, 64, "POSITION      "); break;
                  case D3DDECLUSAGE_BLENDWEIGHT:  snprintf (szOut, 64, "BLENDWEIGHT   "); break;
                  case D3DDECLUSAGE_BLENDINDICES: snprintf (szOut, 64, "BLENDINDICES  "); break;
                  case D3DDECLUSAGE_NORMAL:       snprintf (szOut, 64, "NORMAL        "); break;
                  case D3DDECLUSAGE_PSIZE:        snprintf (szOut, 64, "PSIZE         "); break;
                  case D3DDECLUSAGE_TEXCOORD:     snprintf (szOut, 64, "TEXCOORD      "); break;
                  case D3DDECLUSAGE_TANGENT:      snprintf (szOut, 64, "TANGENT       "); break;
                  case D3DDECLUSAGE_BINORMAL:     snprintf (szOut, 64, "BINORMAL      "); break;
                  case D3DDECLUSAGE_TESSFACTOR:   snprintf (szOut, 64, "TESSFACTOR    "); break;
                  case D3DDECLUSAGE_POSITIONT:    snprintf (szOut, 64, "POSITIONT     "); break;
                  case D3DDECLUSAGE_COLOR:        snprintf (szOut, 64, "COLOR         "); break;
                  case D3DDECLUSAGE_FOG:          snprintf (szOut, 64, "FOG           "); break;
                  case D3DDECLUSAGE_DEPTH:        snprintf (szOut, 64, "DEPTH         "); break;
                  case D3DDECLUSAGE_SAMPLE:       snprintf (szOut, 64, "SV_SampleIndex"); break;
                };
                break;

              case 1:
                switch (usage)
                {
                  case D3DDECLUSAGE_POSITION:     snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_BLENDWEIGHT:  snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_BLENDINDICES: snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_NORMAL:       snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_PSIZE:        snprintf (szOut, 64, " "          ); break;
                  case D3DDECLUSAGE_TEXCOORD:     snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_TANGENT:      snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_BINORMAL:     snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_TESSFACTOR:   snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_POSITIONT:    snprintf (szOut, 64, " "          ); break;
                  case D3DDECLUSAGE_COLOR:        snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_FOG:          snprintf (szOut, 64, " "          ); break;
                  case D3DDECLUSAGE_DEPTH:        snprintf (szOut, 64, " [%lu]", idx); break;
                  case D3DDECLUSAGE_SAMPLE:       snprintf (szOut, 64, " "          ); break;
                };
                break;
            }

            return szOut;
          };

        if (SUCCEEDED (vtx_decl->GetDeclaration (elem_decl, &num_elems)))
        {
          ImGui::Separator  ();
          ImGui::BeginGroup ();
          for (UINT i = 0; i < num_elems; i++)
          {
            if (elem_decl [i].Type != D3DDECLTYPE_UNUSED)
            {
              ++last_count;

              ImGui::TextColored (ImVec4 (0.9f, 0.9f, 0.9f, 1.0f),    "Stream %3lu ", elem_decl [i].Stream);
            }
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for (UINT i = 0; i < num_elems; i++)
          {
            if (elem_decl [i].Type != D3DDECLTYPE_UNUSED)
            {
              ImGui::TextColored (ImVec4 (0.66f, 0.66f, 0.66f, 1.0f), "(+%02lu): ",
                                  elem_decl [i].Offset);
            }
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for (UINT i = 0; i < num_elems; i++)
          {
            if (elem_decl [i].Type != D3DDECLTYPE_UNUSED)
            {
              ImGui::TextColored (ImVec4 (0.35f, 0.85f, 0.35f, 1.0f), "%16s ",
                                  SK_D3D9_DeclTypeToStr ((D3DDECLTYPE)elem_decl [i].Type));
            }
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for (UINT i = 0; i < num_elems; i++)
          {
            if (elem_decl [i].Type != D3DDECLTYPE_UNUSED)
            {
              ImGui::TextColored (ImVec4 (0.6f, 0.6f, 1.0f, 1.0f), R"("Attrib%02lu")", i);
            }
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for (UINT i = 0; i < num_elems; i++)
          {
            if (elem_decl [i].Type != D3DDECLTYPE_UNUSED)
            {
              ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), " : %s",
                    SK_D3D9_DeclUsageToStr ((D3DDECLUSAGE)elem_decl [i].Usage, elem_decl [i].UsageIndex, 0));
            }
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for (UINT i = 0; i < num_elems; i++)
          {
            if (elem_decl [i].Type != D3DDECLTYPE_UNUSED)
            {
              ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), " : %s",
                    SK_D3D9_DeclUsageToStr ((D3DDECLUSAGE)elem_decl [i].Usage, elem_decl [i].UsageIndex, 1));
            }
          }
          ImGui::EndGroup   ();

          --last_count;
        }
      }
      ImGui::EndChild ();

      ImGui::PopStyleColor ();
      ImGui::PopStyleVar   ();
      ImGui::EndGroup      ();

#if 0
      if (ImGui::IsItemHoveredRect () && tracker->textures.size ())
      {
        ImGui::BeginTooltip ();

        D3DFORMAT fmt = D3DFMT_UNKNOWN;

        for ( auto it : tracker->textures )
        {
          ISKTextureD3D9* pTex = tex_mgr.getTexture (it)->d3d9_tex;

          if (pTex && pTex->pTex)
          {
            D3DSURFACE_DESC desc;
            if (SUCCEEDED (pTex->pTex->GetLevelDesc (0, &desc)))
            {
              fmt = desc.Format;
              ImGui::Image ( pTex->pTex, ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
        ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                         ImVec2  (0,0),             ImVec2  (1,1),
                                         ImColor (255,255,255,255), ImColor (242,242,13,255) );
            }

            ImGui::SameLine ();

            ImGui::BeginGroup ();
            ImGui::Text       ("Texture: %08lx", it);
            ImGui::Text       ("Format:  %ws",   SK_D3D9_FormatToStr (fmt).c_str ());
            ImGui::EndGroup   ();
          }
        }

        ImGui::EndTooltip ();
      }
#endif

      ImGui::SameLine ();
      ImGui::Checkbox ("Always Draw This Buffer In Wireframe", &wireframe);

      if (wireframe)
        tracker->wireframes.emplace (tracker->vertex_buffer);
      else if (tracker->wireframes.count (tracker->vertex_buffer))
        tracker->wireframes.erase   (tracker->vertex_buffer);
    }

    if ((! tracker->vertex_shaders.empty ()) || (! tracker->pixel_shaders.empty ()))
    {
      ImGui::Separator ();

      ImGui::Columns (2);

      for ( auto it : tracker->vertex_shaders )
        ImGui::Text ("Vertex Shader: %08x", it);

      ImGui::NextColumn ();

      for ( auto it : tracker->pixel_shaders )
        ImGui::Text ("Pixel Shader: %08x", it);

      ImGui::Columns (1);
    }
  }
  else
    tracker->cancel_draws = false;

  ImGui::EndGroup      ();

  list->last_ht    = ImGui::GetItemRectSize ().y;

  list->last_min   = ImGui::GetItemRectMin ();
  list->last_max   = ImGui::GetItemRectMax ();

  ImGui::PopStyleVar   ();
  ImGui::EndGroup      ();
}



bool
SK_D3D9_TextureModDlg (void)
{
  const float font_size = ImGui::GetFont ()->FontSize * ImGui::GetIO ().FontGlobalScale;

  bool show_dlg = true;

  ImGuiIO& io =
    ImGui::GetIO ();



  auto HandleKeyboard = [&](void)
  {
    extern std::set <uint32_t> textures_used_last_dump;
    extern int32_t             tex_dbg_idx;

    if (io.KeyCtrl && io.KeyShift)
    {
      if ( io.KeysDownDuration [VK_OEM_6] == 0.0f ||
           io.KeysDownDuration [VK_OEM_4] == 0.0f )
      {
        tex_dbg_idx += (io.KeysDownDuration [VK_OEM_6] == 0.0f) ? 1 : 0;
        tex_dbg_idx -= (io.KeysDownDuration [VK_OEM_4] == 0.0f) ? 1 : 0;

        extern uint32_t debug_tex_id;

        if (tex_dbg_idx < 0 || (textures_used_last_dump.empty ()))
        {
          tex_dbg_idx  = -1;
          debug_tex_id =  0;
        }

        else
        {
          if (tex_dbg_idx >= static_cast <int32_t> (textures_used_last_dump.size ()))
          {
            tex_dbg_idx =
              std::max (0UL, static_cast <uint32_t> (textures_used_last_dump.size ()) - 1UL);
          }
        }

        if (tex_dbg_idx >= 0)
        {
          debug_tex_id = 0;
          int idx = 0;
          for (auto it : textures_used_last_dump)
          {
            if (tex_dbg_idx == idx++)
            {
              debug_tex_id = it;
              break;
            }
          }
        }

        SK::D3D9::tex_mgr.updateOSD ();
      }
    }
  };



  ImGui::SetNextWindowSize            (ImVec2 (io.DisplaySize.x * 0.66f, io.DisplaySize.y * 0.42f), ImGuiSetCond_Appearing);
  ImGui::SetNextWindowSizeConstraints ( /*ImVec2 (768.0f, 384.0f),*/
                                       ImVec2 (io.DisplaySize.x * 0.16f, io.DisplaySize.y * 0.16f),
                                       ImVec2 (io.DisplaySize.x * 0.96f, io.DisplaySize.y * 0.96f));

  ImGui::Begin ( "D3D9 Render Mod Toolkit (v " SK_VERSION_STR_A ")",
                   &show_dlg,
                     ImGuiWindowFlags_ShowBorders );

  bool can_scroll = ImGui::IsWindowFocused () && ImGui::IsMouseHoveringRect ( ImVec2 (ImGui::GetWindowPos ().x,                             ImGui::GetWindowPos ().y),
                                                                              ImVec2 (ImGui::GetWindowPos ().x + ImGui::GetWindowSize ().x, ImGui::GetWindowPos ().y + ImGui::GetWindowSize ().y) );

  ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

#if 0
  if (ImGui::CollapsingHeader ("Preliminary Documentation"))
  {
    ImGui::BeginChild (ImGui::GetID ("ModDescription"), ImVec2 (font_size * 66.0f, font_size * 25.0f), true);
      ImGui::TextColored    (ImVec4 (0.9f, 0.7f, 0.5f, 1.0f), "Texture Modding Overview"); ImGui::SameLine ();
      ImGui::Text           ("    (Full Writeup Pending)");

      ImGui::Separator      ();

      ImGui::TextWrapped    ("\nReplacement textures go in (TBFix_Res\\inject\\textures\\{blocking|streaming}\\<checksum>.dds)\n\n");

      ImGui::TreePush ("");
        ImGui::BulletText ("Blocking textures have a high performance penalty, but zero chance of visible pop-in.");
        ImGui::BulletText ("Streaming textures will replace the game's original texture whenever Disk/CPU loads finish.");
        ImGui::TreePush   ("");
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.6f, 0.9f, 0.2f, 1.0f));
          ImGui::BulletText     ("Use streaming whenever possible or performance will bite you in the ass.");
          ImGui::PopStyleColor  ();
        ImGui::TreePop    (  );
      ImGui::TreePop  ();

      ImGui::TextWrapped    ("\n\nLoading modified textures from separate files is inefficient; entire groups of textures may also be packaged into \".7z\" files (See TBFix_Res\\inject\\00_License.7z as an example, and use low/no compression ratio or you will kill the game's performance).\n");

      ImGui::Separator      ();

      ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.6f, 0.3f, 1.0f));
      ImGui::TextWrapped    ( "\n\nA more detailed synopsis will follow in future versions, for now please refer to the GitHub release notes for Tales of Symphonia "
                              "\"Fix\" v 0.9.0 for a thorough description on authoring texture mods.\n\n" );
      ImGui::PopStyleColor  ();

      ImGui::Separator      ();

      ImGui::Bullet         (); ImGui::SameLine ();
      ImGui::TextWrapped    ( "If texture mods are enabled, you can click on the Injected and Base buttons on the texture cache "
                                "summary panel to compare modified and unmodified." );
    ImGui::EndChild         ();
  }
#endif

  if (ImGui::CollapsingHeader("Injectable Data Sources", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    SK_D3D9_DrawFileList  (can_scroll);
  }

  if (ImGui::CollapsingHeader ("Live Texture View", ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen))
  {
    HandleKeyboard ();

    static float last_ht    = 256.0f;
    static float last_width = 256.0f;

    static std::vector <std::string> list_contents;
    static bool                      list_dirty     = false;
    static int                       sel            =     0;

    extern std::set <uint32_t> textures_used_last_dump;
    extern               int32_t  tex_dbg_idx;
    extern              uint32_t  debug_tex_id;

    ImGui::BeginChild ( ImGui::GetID ("D3D9_ToolHeadings"),
                          ImVec2 (font_size * 66.0f, font_size * 2.5f),
                            false,
                              ImGuiWindowFlags_AlwaysUseWindowPadding |
                              ImGuiWindowFlags_NavFlattened           | ImGuiWindowFlags_AlwaysAutoResize );

    if (ImGui::Button ("  Refresh Textures  "))
    {
      SK_ICommandProcessor& command =
        *SK_GetCommandProcessor ();

      command.ProcessCommandLine ("Textures.Trace true");

      SK::D3D9::tex_mgr.updateOSD ();

      list_dirty = true;
    }

    if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Refreshes the set of texture checksums used in the last frame drawn.");

    ImGui::SameLine ();

    if (ImGui::Button (" Clear Debug "))
    {
      sel                         = -1;
      debug_tex_id                =  0;
      textures_used_last_dump.clear ();
      last_ht                     =  0;
      last_width                  =  0;
    }

    if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Exits texture debug mode.");

    ImGui::SameLine ();

    bool need_reset_graphics = false;

    if (ImGui::Checkbox ("Enable On-Demand Texture Dumping",    &config.textures.on_demand_dump)) need_reset_graphics = true;

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.7f, 0.3f, 1.f), "Enable dumping DXT compressed textures from VRAM.");
      ImGui::Separator    ();
      ImGui::Text         ("Drivers may not be able to manage texture memory as efficiently, and you should turn this option off when not modifying textures.\n\n");
      ImGui::BulletText   ("If this is your first time enabling this feature, the dump button will not work until you reload all textures in-game.");
      ImGui::EndTooltip   ();
    }

    ImGui::SameLine ();

    ImGui::Checkbox ("Highlight Selected Texture in Game",    &config.textures.highlight_debug_tex);

    ImGui::Separator ();
    ImGui::EndChild  ();

    if (list_dirty)
    {
      list_contents.clear ();
           sel = tex_dbg_idx;

      if (debug_tex_id == 0)
        last_ht = 0;

      for ( auto it : textures_used_last_dump )
      {
        char szDesc [16] = { };

        sprintf (szDesc, "%08x", it);

        list_contents.emplace_back (szDesc);
      }
    }

    ImGui::BeginGroup ();

    ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
    ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

    ImGui::BeginChild ( ImGui::GetID ("Item List"),
                        ImVec2 ( font_size * 6.0f, std::max (font_size * 15.0f, last_ht)),
                          true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

    ImGui::PopStyleColor  ();

    if (ImGui::IsWindowHovered ())
      can_scroll = false;

   if (! textures_used_last_dump.empty ())
   {
     static      int last_sel = 0;
     static bool sel_changed  = false;

     if (sel != last_sel)
       sel_changed = true;

     last_sel = sel;

     for ( int line = 0; line < static_cast <int> (textures_used_last_dump.size ()); line++)
     {
       if (line == sel)
       {
         bool selected = true;
         ImGui::Selectable (list_contents [line].c_str (), &selected);

         if (sel_changed)
         {
           ImGui::SetScrollHere (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
           sel_changed = false;
         }
       }

       else
       {
         bool selected = false;

         if (ImGui::Selectable (list_contents [line].c_str (), &selected))
         {
           sel_changed  = true;
           tex_dbg_idx  =  line;
           sel          =  line;

           int idx = 0;
           for (auto it : textures_used_last_dump)
           {
             if (line == idx++)
               debug_tex_id = it;
           }
           //debug_tex_id =  textures_used_last_dump [line];
         }
       }
     }
   }

   ImGui::EndChild ();

   if (ImGui::IsItemHovered ())
   {
     ImGui::BeginTooltip ();
     ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), R"(If highlighting is enabled, the "debug" texture will blink to make identifying textures easier.)");
     ImGui::Separator    ();
     ImGui::BulletText   ("Press Ctrl + Shift + [ to select the previous texture from this list");
     ImGui::BulletText   ("Press Ctrl + Shift + ] to select the next texture from this list");
     ImGui::EndTooltip   ();
   }

   ImGui::SameLine     ();
   ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 20.0f);

   last_ht    = std::max (last_ht,    16.0f);
   last_width = std::max (last_width, 16.0f);

   if (debug_tex_id != 0x00)
   {
     SK::D3D9::Texture* pTex =
       SK::D3D9::tex_mgr.getTexture (debug_tex_id);

     extern bool __remap_textures;
            bool has_alternate = (pTex != nullptr && pTex->d3d9_tex->pTexOverride != nullptr);

     if (pTex != nullptr && pTex->d3d9_tex != nullptr && pTex->d3d9_tex->pTex != nullptr)
     {
        D3DSURFACE_DESC desc;

        if (SUCCEEDED (pTex->d3d9_tex->pTex->GetLevelDesc (0, &desc)))
        {
          ImVec4 border_color = config.textures.highlight_debug_tex ?
                                    ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                    (pTex->d3d9_tex->getDrawTexture () != pTex->d3d9_tex->pTex)
                                                                    ? ImVec4 (0.5f,  0.5f,  0.5f, 1.0f) :
                                                                      ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);

          ImGui::PushStyleColor (ImGuiCol_Border, border_color);

          ImGui::BeginGroup     ();
          ImGui::BeginChild     ( ImGui::GetID ("Item Selection"),
                                  ImVec2 ( std::max (font_size * 19.0f, (float)desc.Width + 24.0f),
                                (float)desc.Height + font_size * 11.0f),
                                    true,
                                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NavFlattened );

          ImGui::PopStyleColor  ();

          if ((! config.textures.highlight_debug_tex) && has_alternate)
          {
            if (ImGui::IsItemHovered ())
            {
              if (pTex->d3d9_tex->img_to_use != ISKTextureD3D9::ContentPreference::Original)
                ImGui::SetTooltip ("Click me to make this the always visible version");
              else
              {
                ImGui::BeginTooltip ( );
                ImGui::Text         ("Click me to use the system-wide texture preference");
                ImGui::Separator    ( );
                ImGui::BulletText   ( "The system-wide preference is currently:  (%s)",
                                        __remap_textures ? "Draw injected textures" :
                                                           "Draw original textures" );
                ImGui::EndTooltip   ( );
              }
            }

            // Allow the user to toggle texture override by clicking the frame
            if (SK_ImGui_IsItemClicked ())
            {
              pTex->d3d9_tex->toggleOriginal ();
            }
          }

          last_width  = (float)desc.Width;
          last_ht     = (float)desc.Height + font_size * 11.0f;


          int num_lods =
            pTex->d3d9_tex->pTex->GetLevelCount ();

          ImGui::BeginGroup      (                  );
          ImGui::TextUnformatted ( "Dimensions:   " );
          ImGui::TextUnformatted ( "Format:       " );
          ImGui::TextUnformatted ( "Data Size:    " );
          ImGui::TextUnformatted ( "Load Time:    " );
          ImGui::TextUnformatted ( "References:   " );
          ImGui::EndGroup        (                  );

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          ImGui::Text ("%lux%lu (%lu %s)",
                       desc.Width, desc.Height,
                       num_lods, num_lods > 1 ? "LODs" : "LOD");
          ImGui::Text ("%ws",
                       SK_D3D9_FormatToStr (desc.Format).c_str ( ));
          ImGui::Text ("%.3f MiB",
            (double)pTex->d3d9_tex->tex_size / ( 1024.0f * 1024.0f ));
          ImGui::Text ("%.3f ms",
                       pTex->load_time);
          ImGui::Text ("%lu",
                       pTex->refs);


          //extern std::unordered_map <uint32_t, int32_t>            injected_refs;
          //ImGui::Text ("%lu", injected_refs [pTex->crc32c]);
          ImGui::EndGroup   ();

          ImGui::Separator  ();

          static bool flip_vertical0   = false;
          static bool flip_horizontal0 = false;

          ImGui::Checkbox ("Flip Vertically##D3D9_FlipVertical0",     &flip_vertical0);   ImGui::SameLine ( );
          ImGui::Checkbox ("Flip Horizontally##D3D9_FlipHorizontal0", &flip_horizontal0);

          if (! SK::D3D9::tex_mgr.isTextureDumped (debug_tex_id))
          {
            if ( ImGui::Button ("  Dump Texture to Disk  ###DumpTexture") )
            {
              SK::D3D9::tex_mgr.dumpTexture (desc.Format, debug_tex_id, pTex->d3d9_tex->pTex);
            }

            //if (config.textures.quick_load && ImGui::IsItemHovered ())
            //  ImGui::SetTooltip ("Turn off Texture QuickLoad to use this feature.");
          }

          else
          {
            if ( ImGui::Button ("  Delete Dumped Texture from Disk  ###DumpTexture") )
            {
              SK::D3D9::tex_mgr.deleteDumpedTexture (desc.Format, debug_tex_id);
            }
          }

          ImVec2 uv0 (flip_horizontal0 ? 1.0f : 0.0f, flip_vertical0 ? 1.0f : 0.0f);
          ImVec2 uv1 (flip_horizontal0 ? 0.0f : 1.0f, flip_vertical0 ? 0.0f : 1.0f);

          ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
          ImGui::BeginChildFrame (ImGui::GetID ("ChildFrame_XXX"), ImVec2 ((float)desc.Width + 8, (float)desc.Height + 8),
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoScrollbar );
          ImGui::Image           ( pTex->d3d9_tex->pTex,
                                     ImVec2 ((float)desc.Width, (float)desc.Height),
                                       uv0,                       uv1,
                                       ImColor (255,255,255,255), ImColor (255,255,255,128)
                                 );
          ImGui::EndChildFrame   ();
          ImGui::EndChild        ();
          ImGui::EndGroup        ();
          ImGui::PopStyleColor   (1);
        }
     }

     if (has_alternate)
     {
       ImGui::SameLine ();

        D3DSURFACE_DESC desc;

        if (SUCCEEDED (pTex->d3d9_tex->pTexOverride->GetLevelDesc (0, &desc)))
        {
          bool override_tex = (pTex->d3d9_tex->getDrawTexture () == pTex->d3d9_tex->pTexOverride);

          ImVec4 border_color = config.textures.highlight_debug_tex ?
                                  ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                   override_tex ? ImVec4 (0.3f, 1.0f, 0.3f, 1.0f) :
                                  ImVec4 (0.5f, 0.5f, 0.5f, 1.0f);

          ImGui::PushStyleColor  (ImGuiCol_Border, border_color);

          ImGui::BeginGroup ();
          ImGui::BeginChild ( ImGui::GetID ("Item Selection2"),
                              ImVec2 ( std::max (font_size * 19.0f, (float)desc.Width  + 24.0f),
                                                                    (float)desc.Height + font_size * 10.0f),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NavFlattened );
          ImGui::PopStyleColor ();

          if (! config.textures.highlight_debug_tex)
          {
            if (ImGui::IsItemHovered ())
            {
              if (pTex->d3d9_tex->img_to_use != ISKTextureD3D9::ContentPreference::Override)
                ImGui::SetTooltip ("Click me to make this the always visible version");
              else
              {
                ImGui::BeginTooltip ( );
                ImGui::Text         ("Click me to use the system-wide texture preference");
                ImGui::Separator    ( );
                ImGui::BulletText   ( "The system-wide preference is currently:  (%s)",
                                        __remap_textures ? "Draw injected textures" :
                                                           "Draw original textures" );
                ImGui::EndTooltip   ( );
              }
            }

            // Allow the user to toggle texture override by clicking the frame
            if (SK_ImGui_IsItemClicked ())
            {
              pTex->d3d9_tex->toggleOverride ();
            }
          }


          last_width  = std::max (last_width, (float)desc.Width);
          last_ht     = std::max (last_ht,    (float)desc.Height + font_size * 10.0f);


          extern std::wstring
          SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);


          bool injected  =
            (SK::D3D9::tex_mgr.getInjectableTexture (debug_tex_id).size != 0),
               reloading = false;

          int num_lods =
            pTex->d3d9_tex->pTexOverride->GetLevelCount ();

          ImGui::BeginGroup      ();
          ImGui::TextUnformatted ( "Dimensions:   " );
          ImGui::TextUnformatted ( "Format:       " );
          ImGui::TextUnformatted ( "Data Size:    " );
          ImGui::EndGroup        ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          ImGui::Text       ( "%lux%lu  (%lu %s)",
                                desc.Width, desc.Height,
                                   num_lods, num_lods > 1 ? "LODs" : "LOD" );
          ImGui::Text       ( "%ws",
                                SK_D3D9_FormatToStr (desc.Format).c_str () );
          ImGui::Text       ( "%.3f MiB",
                                (double)pTex->d3d9_tex->override_size / (1024.0f * 1024.0f) );
          ImGui::EndGroup   ();

          ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), injected ? "Injected Texture" :
                                                                          "Resampled Texture" );

          ImGui::Separator  ();

          static bool flip_vertical1   = false;
          static bool flip_horizontal1 = false;

          ImGui::Checkbox ("Flip Vertically##D3D9_FlipVertical1",     &flip_vertical1);   ImGui::SameLine ( );
          ImGui::Checkbox ("Flip Horizontally##D3D9_FlipHorizontal1", &flip_horizontal1);

          if (injected)
          {
            if ( ImGui::Button ("  Reload This Texture  ") && SK::D3D9::tex_mgr.reloadTexture (debug_tex_id) )
            {
              reloading = true;

              SK::D3D9::tex_mgr.updateOSD ();
            }
          }

          else {
            ImGui::Button ("  Resample This Texture  "); // NO-OP, but preserves alignment :P
          }

          if (! reloading)
          {
            ImVec2 uv0 (flip_horizontal1 ? 1.0f : 0.0f, flip_vertical1 ? 1.0f : 0.0f);
            ImVec2 uv1 (flip_horizontal1 ? 0.0f : 1.0f, flip_vertical1 ? 0.0f : 1.0f);

            ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
            ImGui::BeginChildFrame (ImGui::GetID ("ChildFrame_YYY"), ImVec2 ((float)desc.Width + 8, (float)desc.Height + 8),
                                    ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoInputs |
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Image           ( pTex->d3d9_tex->pTexOverride,
                                       ImVec2 ((float)desc.Width, (float)desc.Height),
                                       uv0,                       uv1,
                                       ImColor (255,255,255,255), ImColor (255,255,255,128)
                                   );
            ImGui::EndChildFrame   ();
            ImGui::PopStyleColor   ();
          }

          ImGui::EndChild        ();
          ImGui::EndGroup        ();
        }
      }
    }
    ImGui::EndGroup      ();
    ImGui::PopStyleVar   (2);
  }

  if (ImGui::CollapsingHeader ("Live Render Target View"))
  {
    static float last_ht    = 256.0f;
    static float last_width = 256.0f;

    static std::vector <std::string> list_contents;
    static bool                      list_dirty     = true;
    static uintptr_t                 last_sel_ptr   =    0;
    static int                            sel       =   -1;

    std::vector <IDirect3DBaseTexture9*> render_textures;
           tex_mgr.getUsedRenderTargets (render_textures);

    tracked_rt.tracking_tex = nullptr;

    if (list_dirty)
    {
          sel = -1;
      int idx =  0;
          list_contents.clear ();

      // The underlying list is unsorted for speed, but that's not at all
      //   intuitive to humans, so sort the thing when we have the RT view open.
      std::sort ( render_textures.begin (),
                  render_textures.end   (),
        []( IDirect3DBaseTexture9 *a,
            IDirect3DBaseTexture9 *b )
        {
          return (uintptr_t)a < (uintptr_t)b;
        }
      );


      for ( auto it : render_textures )
      {
        char szDesc [16] = { };

#ifdef _WIN64
        sprintf (szDesc, "%llx", (uintptr_t)it);
#else
        sprintf (szDesc, "%lx",  (uintptr_t)it);
#endif

        list_contents.emplace_back (szDesc);

        if ((uintptr_t)it == last_sel_ptr)
        {
          sel = idx;
          tracked_rt.tracking_tex = render_textures [sel];
        }

        ++idx;
      }
    }

    ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
    ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

    ImGui::BeginChild ( ImGui::GetID ("Item List2"),
                        ImVec2 ( font_size * 6.0f, std::max (font_size * 15.0f, last_ht)),
                          true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

    if (ImGui::IsWindowHovered ())
      can_scroll = false;

   if (! render_textures.empty ())
   {
     static      int last_sel = 0;
     static bool sel_changed  = false;

     if (sel != last_sel)
       sel_changed = true;

     last_sel = sel;

     for ( int line = 0; line < static_cast <int> (render_textures.size ()); line++ )
     {
       D3DSURFACE_DESC desc;

       CComPtr <IDirect3DTexture9> pTex = nullptr;

       if (SUCCEEDED (render_textures [line]->QueryInterface (IID_PPV_ARGS (&pTex))))
       {
         if (SUCCEEDED (pTex->GetLevelDesc (0, &desc)))
         {
           if (line == sel)
           {
             bool selected = true;
             ImGui::Selectable (list_contents [line].c_str (), &selected);

             if (sel_changed)
             {
               ImGui::SetScrollHere (0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
               ImGui::SetKeyboardFocusHere ( );

               sel_changed = false;
             }
           }

           else
           {
             bool selected = false;

             if (ImGui::Selectable (list_contents [line].c_str (), &selected))
             {
               sel_changed  = true;
               sel          =  line;
               last_sel_ptr = (uintptr_t)render_textures [sel];
               tracked_rt.tracking_tex = render_textures [sel];
             }
           }
         }
       }
     }
   }

   ImGui::EndChild ();

   ImGui::BeginGroup ();

   ImGui::PopStyleColor ();
   ImGui::PopStyleVar   ();

   CComPtr <IDirect3DTexture9> pTex = nullptr;

   if ((! render_textures.empty ()) && sel >= 0)
     render_textures [sel]->QueryInterface (IID_PPV_ARGS (&pTex));

   if (pTex != nullptr)
   {
      D3DSURFACE_DESC desc;

      if (SUCCEEDED (pTex->GetLevelDesc (0, &desc)))
      {
        size_t shaders = std::max ( tracked_rt.pixel_shaders.size  (),
                                    tracked_rt.vertex_shaders.size () );

        // Some Render Targets are MASSIVE, let's try to keep the damn things on the screen ;)
        float effective_width  = std::min (0.75f * ImGui::GetIO ().DisplaySize.x, (float)desc.Width  / 2.0f);
        float effective_height = std::min (0.75f * ImGui::GetIO ().DisplaySize.y, (float)desc.Height / 2.0f);

        ImGui::SameLine ();

        ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::BeginChild ( ImGui::GetID ("Item Selection3"),
                            ImVec2 ( std::max (font_size * 30.0f, effective_width  + 24.0f),
                                     std::max (256.0f,            effective_height + font_size * 4.0f + (float)shaders * font_size) ),
                              true,
                                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

        last_width  = effective_width;
        last_ht     = effective_height + font_size * 4.0f + (float)shaders * font_size;

        extern std::wstring
        SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);


        ImGui::Text ( "Dimensions:   %lux%lu",
                        desc.Width, desc.Height/*,
                          pTex->d3d9_tex->GetLevelCount ()*/ );
        ImGui::Text ( "Format:       %ws",
                        SK_D3D9_FormatToStr (desc.Format).c_str () );

        ImGui::Separator     ();

        ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
        ImGui::BeginChildFrame (ImGui::GetID ("ChildFrame_ZZZ"), ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                  ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNavInputs |
                                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize );
        ImGui::Image           ( pTex,
                                   ImVec2 (effective_width, effective_height),
                                     ImVec2  (0,0),             ImVec2  (1,1),
                                     ImColor (255,255,255,255), ImColor (255,255,255,128)
                               );
        ImGui::EndChildFrame   ();

        if (shaders > 0)
        {
          ImGui::Columns (2);

          for ( auto it : tracked_rt.vertex_shaders )
            ImGui::Text ("Vertex Shader: %08x", it);

          ImGui::NextColumn ();

          for ( auto it : tracked_rt.pixel_shaders )
            ImGui::Text ("Pixel Shader: %08x", it);

          ImGui::Columns (1);
        }

        ImGui::EndChild        ();
        ImGui::PopStyleColor   (2);
      }
    }

    ImGui::EndGroup ();
  }

  if (ImGui::CollapsingHeader ("Live Shader View"))
  {
    ImGui::TreePush ("");

    if (ImGui::CollapsingHeader ("Pixel Shaders"))
    {
      SK_AutoCriticalSection auto_cs (&cs_ps);
      SK_D3D9_LiveShaderClassView (SK::D3D9::ShaderClass::Pixel, can_scroll);
    }

    if (ImGui::CollapsingHeader ("Vertex Shaders"))
    {
      SK_AutoCriticalSection auto_cs (&cs_vs);
      SK_D3D9_LiveShaderClassView (SK::D3D9::ShaderClass::Vertex, can_scroll);
    }

    ImGui::TreePop ();
  }

  if (ImGui::CollapsingHeader ("Live Vertex Buffer View"))
  {
    ImGui::TreePush ("");

    if (ImGui::CollapsingHeader ("Stream 0"))
    {
      SK_AutoCriticalSection auto_cs (&cs_vb);
      SK_LiveVertexStreamView (can_scroll);
    }

    ImGui::TreePop ();
  }

  if (ImGui::CollapsingHeader ("Misc. Settings"))
  {
    ImGui::TreePush ("");
    //if (ImGui::Checkbox ("Dump ALL Shaders   (TBFix_Res\\dump\\shaders\\<ps|vs>_<checksum>.html)", &config.render.dump_shaders)) need_reset.graphics = true;
    if (ImGui::Checkbox (R"(Dump ALL Textures at Load  (<ResourceRoot>\dump\textures\<format>\*.dds))", &config.textures.dump_on_load))

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("Enabling this will cause the game to run slower and waste disk space, only enable if you know what you are doing.");
      ImGui::EndTooltip   ();
    }

    ImGui::TreePop ();
  }

  ImGui::PopItemWidth ();

  if (can_scroll)
    ImGui::SetScrollY (ImGui::GetScrollY () + 5.0f * ImGui::GetFont ()->FontSize * -ImGui::GetIO ().MouseWheel);

  ImGui::End          ();

  SK_AutoCriticalSection auto_cs0 (&cs_vs);
  SK_AutoCriticalSection auto_cs1 (&cs_ps);
  SK_AutoCriticalSection auto_cs2 (&cs_vb);

  tracked_ps.clear (); tracked_vs.clear (); last_frame.clear ();
  tracked_rt.clear (); tracked_vb.clear (); known_objs.clear ();

  return show_dlg;
}





std::wstring
SK_D3D9_UsageToStr (DWORD dwUsage)
{
  std::wstring usage;

  if (dwUsage & D3DUSAGE_RENDERTARGET)
    usage += L"RenderTarget ";

  if (dwUsage & D3DUSAGE_DEPTHSTENCIL)
    usage += L"Depth/Stencil ";

  if (dwUsage & D3DUSAGE_DYNAMIC)
    usage += L"Dynamic";

  if (usage.empty ())
    usage = L"Don't Care";

  return usage;
}

INT
__stdcall
SK_D3D9_BytesPerPixel (D3DFORMAT Format)
{
  switch (Format)
  {
    case D3DFMT_UNKNOWN:
      return 0;

    case D3DFMT_R8G8B8:       return 3;
    case D3DFMT_A8R8G8B8:     return 4;
    case D3DFMT_X8R8G8B8:     return 4;
    case D3DFMT_R5G6B5:       return 2;
    case D3DFMT_X1R5G5B5:     return 2;
    case D3DFMT_A1R5G5B5:     return 2;
    case D3DFMT_A4R4G4B4:     return 2;
    case D3DFMT_R3G3B2:       return 8;
    case D3DFMT_A8:           return 1;
    case D3DFMT_A8R3G3B2:     return 2;
    case D3DFMT_X4R4G4B4:     return 2;
    case D3DFMT_A2B10G10R10:  return 4;
    case D3DFMT_A8B8G8R8:     return 4;
    case D3DFMT_X8B8G8R8:     return 4;
    case D3DFMT_G16R16:       return 4;
    case D3DFMT_A2R10G10B10:  return 4;
    case D3DFMT_A16B16G16R16: return 8;
    case D3DFMT_A8P8:         return 2;
    case D3DFMT_P8:           return 1;
    case D3DFMT_L8:           return 1;
    case D3DFMT_A8L8:         return 2;
    case D3DFMT_A4L4:         return 1;
    case D3DFMT_V8U8:         return 2;
    case D3DFMT_L6V5U5:       return 2;
    case D3DFMT_X8L8V8U8:     return 4;
    case D3DFMT_Q8W8V8U8:     return 4;
    case D3DFMT_V16U16:       return 4;
    case D3DFMT_A2W10V10U10:  return 4;

#if 0
    case D3DFMT_UYVY                 :
      return std::wstring (L"FourCC 'UYVY'");
    case D3DFMT_R8G8_B8G8            :
      return std::wstring (L"FourCC 'RGBG'");
    case D3DFMT_YUY2                 :
      return std::wstring (L"FourCC 'YUY2'");
    case D3DFMT_G8R8_G8B8            :
      return std::wstring (L"FourCC 'GRGB'");
#endif
    case D3DFMT_DXT1:          return -1;
    case D3DFMT_DXT2:          return -2;
    case D3DFMT_DXT3:          return -2;
    case D3DFMT_DXT4:          return -1;
    case D3DFMT_DXT5:          return -2;

    case D3DFMT_D16_LOCKABLE:  return  2;
    case D3DFMT_D32:           return  4;
    case D3DFMT_D15S1:         return  2;
    case D3DFMT_D24S8:         return  4;
    case D3DFMT_D24X8:         return  4;
    case D3DFMT_D24X4S4:       return  4;
    case D3DFMT_D16:           return  2;
    case D3DFMT_D32F_LOCKABLE: return  4;
    case D3DFMT_D24FS8:        return  4;

/* D3D9Ex only -- */
#if !defined(D3D_DISABLE_9EX)

    /* Z-Stencil formats valid for CPU access */
    case D3DFMT_D32_LOCKABLE:  return 4;
    case D3DFMT_S8_LOCKABLE:   return 1;
#endif // !D3D_DISABLE_9EX



    case D3DFMT_L16:           return 2;

#if 0
    case D3DFMT_VERTEXDATA           :
      return std::wstring (L"VERTEXDATA") +
                (include_ordinal ? L" (100)" : L"");
#endif
    case D3DFMT_INDEX16:       return 2;
    case D3DFMT_INDEX32:       return 4;

    case D3DFMT_Q16W16V16U16:  return 8;

#if 0
    case D3DFMT_MULTI2_ARGB8         :
      return std::wstring (L"FourCC 'MET1'");
#endif

    // Floating point surface formats

    // s10e5 formats (16-bits per channel)
    case D3DFMT_R16F:          return 2;
    case D3DFMT_G16R16F:       return 4;
    case D3DFMT_A16B16G16R16F: return 8;

    // IEEE s23e8 formats (32-bits per channel)
    case D3DFMT_R32F:          return 4;
    case D3DFMT_G32R32F:       return 8;
    case D3DFMT_A32B32G32R32F: return 16;

#if 0
    case D3DFMT_CxV8U8               :
      return std::wstring (L"CxV8U8") +
                (include_ordinal ? L" (117)" : L"");
#endif

/* D3D9Ex only -- */
#if !defined(D3D_DISABLE_9EX)

    // Monochrome 1 bit per pixel format
    case D3DFMT_A1:            return -8;

#if 0
    // 2.8 biased fixed point
    case D3DFMT_A2B10G10R10_XR_BIAS  :
      return std::wstring (L"A2B10G10R10_XR_BIAS") +
                (include_ordinal ? L" (119)" : L"");
#endif


#if 0
    // Binary format indicating that the data has no inherent type
    case D3DFMT_BINARYBUFFER         :
      return std::wstring (L"BINARYBUFFER") +
                (include_ordinal ? L" (199)" : L"");
#endif

#endif // !D3D_DISABLE_9EX
/* -- D3D9Ex only */
  }

  return 0;
}

std::wstring
SK_D3D9_SwapEffectToStr (D3DSWAPEFFECT Effect)
{
  switch (Effect)
  {
    case D3DSWAPEFFECT_COPY:
      return std::wstring (L"Copy");
    case D3DSWAPEFFECT_FLIP:
      return std::wstring (L"Flip");
    case D3DSWAPEFFECT_DISCARD:
      return std::wstring (L"Discard");
    case D3DSWAPEFFECT_OVERLAY:
      return std::wstring (L"Overlay");
    case D3DSWAPEFFECT_FLIPEX:
      return std::wstring   (L"FlipEx");
    default:
      return L"UNKNOWN";
  }
};

std::wstring
SK_D3D9_PresentParameterFlagsToStr (DWORD dwFlags)
{
  std::wstring out = L"";

  if (dwFlags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
    out += L"D3DPRESENTFLAG_LOCKABLE_BACKBUFFER  ";

  if (dwFlags & D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL)
    out += L"D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL  ";

  if (dwFlags & D3DPRESENTFLAG_DEVICECLIP)
    out += L"D3DPRESENTFLAG_DEVICECLIP  ";

  if (dwFlags & D3DPRESENTFLAG_VIDEO)
    out += L"D3DPRESENTFLAG_VIDEO  ";

  if (dwFlags & D3DPRESENTFLAG_NOAUTOROTATE)
    out += L"D3DPRESENTFLAG_NOAUTOROTATE  ";

  if (dwFlags & D3DPRESENTFLAG_UNPRUNEDMODE)
    out += L"D3DPRESENTFLAG_UNPRUNEDMODE  ";

  if (dwFlags & D3DPRESENTFLAG_OVERLAY_LIMITEDRGB)
    out += L"D3DPRESENTFLAG_OVERLAY_LIMITEDRGB  ";

  if (dwFlags & D3DPRESENTFLAG_OVERLAY_YCbCr_BT709)
    out += L"D3DPRESENTFLAG_OVERLAY_YCbCr_BT709  ";

  if (dwFlags & D3DPRESENTFLAG_OVERLAY_YCbCr_xvYCC)
    out += L"D3DPRESENTFLAG_OVERLAY_YCbCr_xvYCC  ";

  if (dwFlags & D3DPRESENTFLAG_RESTRICTED_CONTENT)
    out += L"D3DPRESENTFLAG_RESTRICTED_CONTENT  ";

  if (dwFlags & D3DPRESENTFLAG_RESTRICT_SHARED_RESOURCE_DRIVER)
    out += L"D3DPRESENTFLAG_RESTRICT_SHARED_RESOURCE_DRIVER  ";

  return out;
};

std::wstring
SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal)
{
  switch (Format)
  {
    case D3DFMT_UNKNOWN:
      return std::wstring (L"Unknown") + (include_ordinal ? L" (0)" :
                                                            L"");

    case D3DFMT_R8G8B8:
      return std::wstring (L"R8G8B8")   +
                (include_ordinal ? L" (20)" : L"");
    case D3DFMT_A8R8G8B8:
      return std::wstring (L"A8R8G8B8") +
                (include_ordinal ? L" (21)" : L"");
    case D3DFMT_X8R8G8B8:
      return std::wstring (L"X8R8G8B8") +
                (include_ordinal ? L" (22)" : L"");
    case D3DFMT_R5G6B5               :
      return std::wstring (L"R5G6B5")   +
                (include_ordinal ? L" (23)" : L"");
    case D3DFMT_X1R5G5B5             :
      return std::wstring (L"X1R5G5B5") +
                (include_ordinal ? L" (24)" : L"");
    case D3DFMT_A1R5G5B5             :
      return std::wstring (L"A1R5G5B5") +
                (include_ordinal ? L" (25)" : L"");
    case D3DFMT_A4R4G4B4             :
      return std::wstring (L"A4R4G4B4") +
                (include_ordinal ? L" (26)" : L"");
    case D3DFMT_R3G3B2               :
      return std::wstring (L"R3G3B2")   +
                (include_ordinal ? L" (27)" : L"");
    case D3DFMT_A8                   :
      return std::wstring (L"A8")       +
                (include_ordinal ? L" (28)" : L"");
    case D3DFMT_A8R3G3B2             :
      return std::wstring (L"A8R3G3B2") +
                (include_ordinal ? L" (29)" : L"");
    case D3DFMT_X4R4G4B4             :
      return std::wstring (L"X4R4G4B4") +
                (include_ordinal ? L" (30)" : L"");
    case D3DFMT_A2B10G10R10          :
      return std::wstring (L"A2B10G10R10") +
                (include_ordinal ? L" (31)" : L"");
    case D3DFMT_A8B8G8R8             :
      return std::wstring (L"A8B8G8R8") +
                (include_ordinal ? L" (32)" : L"");
    case D3DFMT_X8B8G8R8             :
      return std::wstring (L"X8B8G8R8") +
                (include_ordinal ? L" (33)" : L"");
    case D3DFMT_G16R16               :
      return std::wstring (L"G16R16") +
                (include_ordinal ? L" (34)" : L"");
    case D3DFMT_A2R10G10B10          :
      return std::wstring (L"A2R10G10B10") +
                (include_ordinal ? L" (35)" : L"");
    case D3DFMT_A16B16G16R16         :
      return std::wstring (L"A16B16G16R16") +
                (include_ordinal ? L" (36)" : L"");

    case D3DFMT_A8P8                 :
      return std::wstring (L"A8P8") +
                (include_ordinal ? L" (40)" : L"");
    case D3DFMT_P8                   :
      return std::wstring (L"P8") +
                (include_ordinal ? L" (41)" : L"");

    case D3DFMT_L8                   :
      return std::wstring (L"L8") +
                (include_ordinal ? L" (50)" : L"");
    case D3DFMT_A8L8                 :
      return std::wstring (L"A8L8") +
                (include_ordinal ? L" (51)" : L"");
    case D3DFMT_A4L4                 :
      return std::wstring (L"A4L4") +
                (include_ordinal ? L" (52)" : L"");

    case D3DFMT_V8U8                 :
      return std::wstring (L"V8U8") +
                (include_ordinal ? L" (60)" : L"");
    case D3DFMT_L6V5U5               :
      return std::wstring (L"L6V5U5") +
                (include_ordinal ? L" (61)" : L"");
    case D3DFMT_X8L8V8U8             :
      return std::wstring (L"X8L8V8U8") +
                (include_ordinal ? L" (62)" : L"");
    case D3DFMT_Q8W8V8U8             :
      return std::wstring (L"Q8W8V8U8") +
                (include_ordinal ? L" (63)" : L"");
    case D3DFMT_V16U16               :
      return std::wstring (L"V16U16") +
                (include_ordinal ? L" (64)" : L"");
    case D3DFMT_A2W10V10U10          :
      return std::wstring (L"A2W10V10U10") +
                (include_ordinal ? L" (67)" : L"");

    case D3DFMT_UYVY                 :
      return std::wstring (L"FourCC 'UYVY'");
    case D3DFMT_R8G8_B8G8            :
      return std::wstring (L"FourCC 'RGBG'");
    case D3DFMT_YUY2                 :
      return std::wstring (L"FourCC 'YUY2'");
    case D3DFMT_G8R8_G8B8            :
      return std::wstring (L"FourCC 'GRGB'");
    case D3DFMT_DXT1                 :
      return std::wstring (L"DXT1");
    case D3DFMT_DXT2                 :
      return std::wstring (L"DXT2");
    case D3DFMT_DXT3                 :
      return std::wstring (L"DXT3");
    case D3DFMT_DXT4                 :
      return std::wstring (L"DXT4");
    case D3DFMT_DXT5                 :
      return std::wstring (L"DXT5");

    case D3DFMT_D16_LOCKABLE         :
      return std::wstring (L"D16_LOCKABLE") +
                (include_ordinal ? L" (70)" : L"");
    case D3DFMT_D32                  :
      return std::wstring (L"D32") +
                (include_ordinal ? L" (71)" : L"");
    case D3DFMT_D15S1                :
      return std::wstring (L"D15S1") +
                (include_ordinal ? L" (73)" : L"");
    case D3DFMT_D24S8                :
      return std::wstring (L"D24S8") +
                (include_ordinal ? L" (75)" : L"");
    case D3DFMT_D24X8                :
      return std::wstring (L"D24X8") +
                (include_ordinal ? L" (77)" : L"");
    case D3DFMT_D24X4S4              :
      return std::wstring (L"D24X4S4") +
                (include_ordinal ? L" (79)" : L"");
    case D3DFMT_D16                  :
      return std::wstring (L"D16") +
                (include_ordinal ? L" (80)" : L"");

    case D3DFMT_D32F_LOCKABLE        :
      return std::wstring (L"D32F_LOCKABLE") +
                (include_ordinal ? L" (82)" : L"");
    case D3DFMT_D24FS8               :
      return std::wstring (L"D24FS8") +
                (include_ordinal ? L" (83)" : L"");

/* D3D9Ex only -- */
#if !defined(D3D_DISABLE_9EX)

    /* Z-Stencil formats valid for CPU access */
    case D3DFMT_D32_LOCKABLE         :
      return std::wstring (L"D32_LOCKABLE") +
                (include_ordinal ? L" (84)" : L"");
    case D3DFMT_S8_LOCKABLE          :
      return std::wstring (L"S8_LOCKABLE") +
                (include_ordinal ? L" (85)" : L"");

#endif // !D3D_DISABLE_9EX



    case D3DFMT_L16                  :
      return std::wstring (L"L16") +
                (include_ordinal ? L" (81)" : L"");

    case D3DFMT_VERTEXDATA           :
      return std::wstring (L"VERTEXDATA") +
                (include_ordinal ? L" (100)" : L"");
    case D3DFMT_INDEX16              :
      return std::wstring (L"INDEX16") +
                (include_ordinal ? L" (101)" : L"");
    case D3DFMT_INDEX32              :
      return std::wstring (L"INDEX32") +
                (include_ordinal ? L" (102)" : L"");

    case D3DFMT_Q16W16V16U16         :
      return std::wstring (L"Q16W16V16U16") +
                (include_ordinal ? L" (110)" : L"");

    case D3DFMT_MULTI2_ARGB8         :
      return std::wstring (L"FourCC 'MET1'");

    // Floating point surface formats

    // s10e5 formats (16-bits per channel)
    case D3DFMT_R16F                 :
      return std::wstring (L"R16F") +
                (include_ordinal ? L" (111)" : L"");
    case D3DFMT_G16R16F              :
      return std::wstring (L"G16R16F") +
                (include_ordinal ? L" (112)" : L"");
    case D3DFMT_A16B16G16R16F        :
      return std::wstring (L"A16B16G16R16F") +
               (include_ordinal ? L" (113)" : L"");

    // IEEE s23e8 formats (32-bits per channel)
    case D3DFMT_R32F                 :
      return std::wstring (L"R32F") +
                (include_ordinal ? L" (114)" : L"");
    case D3DFMT_G32R32F              :
      return std::wstring (L"G32R32F") +
                (include_ordinal ? L" (115)" : L"");
    case D3DFMT_A32B32G32R32F        :
      return std::wstring (L"A32B32G32R32F") +
                (include_ordinal ? L" (116)" : L"");

    case D3DFMT_CxV8U8               :
      return std::wstring (L"CxV8U8") +
                (include_ordinal ? L" (117)" : L"");

/* D3D9Ex only -- */
#if !defined(D3D_DISABLE_9EX)

    // Monochrome 1 bit per pixel format
    case D3DFMT_A1                   :
      return std::wstring (L"A1") +
                (include_ordinal ? L" (118)" : L"");

    // 2.8 biased fixed point
    case D3DFMT_A2B10G10R10_XR_BIAS  :
      return std::wstring (L"A2B10G10R10_XR_BIAS") +
                (include_ordinal ? L" (119)" : L"");


    // Binary format indicating that the data has no inherent type
    case D3DFMT_BINARYBUFFER         :
      return std::wstring (L"BINARYBUFFER") +
                (include_ordinal ? L" (199)" : L"");

#endif // !D3D_DISABLE_9EX
/* -- D3D9Ex only */
  }

  return std::wstring (L"UNKNOWN?!");
}

const wchar_t*
SK_D3D9_PoolToStr (D3DPOOL pool)
{
  switch (pool)
  {
    case D3DPOOL_DEFAULT:
      return L"    Default   (0)";
    case D3DPOOL_MANAGED:
      return L"    Managed   (1)";
    case D3DPOOL_SYSTEMMEM:
      return L"System Memory (2)";
    case D3DPOOL_SCRATCH:
      return L"   Scratch    (3)";
    default:
      return L"   UNKNOWN?!     ";
  }
}

void
SK_D3D9_DumpShader ( const wchar_t* wszPrefix,
                           uint32_t crc32c,
                           LPVOID   pbFunc )
{
  static bool dump = false;//config.render.dump_shaders;

  /////if (dump)
  /////{
  /////  if (GetFileAttributes (L"TBFix_Res\\dump\\shaders") !=
  /////       FILE_ATTRIBUTE_DIRECTORY)
  /////  {
  /////    CreateDirectoryW (L"TBFix_Res",                nullptr);
  /////    CreateDirectoryW (L"TBFix_Res\\dump",          nullptr);
  /////    CreateDirectoryW (L"TBFix_Res\\dump\\shaders", nullptr);
  /////  }
  /////
  /////  wchar_t wszDumpName [MAX_PATH] = { L'\0' };
  /////
  /////  swprintf_s ( wszDumpName,
  /////                 MAX_PATH,
  /////                   L"TBFix_Res\\dump\\shaders\\%s_%08x.html",
  /////                     wszPrefix, crc32c );
  /////
  /////  if ( GetFileAttributes (wszDumpName) == INVALID_FILE_ATTRIBUTES )
  /////  {
  /////    CComPtr <ID3DXBuffer> pDisasm = nullptr;
  /////
  /////    HRESULT hr =
  /////      D3DXDisassembleShader ((DWORD *)pbFunc, TRUE, "", &pDisasm);
  /////
  /////    if (SUCCEEDED (hr))
  /////    {
  /////      FILE* fDump = _wfsopen (wszDumpName,  L"wb", _SH_DENYWR);
  /////
  /////      if (fDump != NULL)
  /////      {
  /////        fwrite ( pDisasm->GetBufferPointer (),
  /////                   pDisasm->GetBufferSize  (),
  /////                     1,
  /////                       fDump );
  /////        fclose (fDump);
  /////      }
  /////    }
  /////  }
  /////}

  CComPtr <ID3DXBuffer> pDisasm = nullptr;

  HRESULT hr =
    D3DXDisassembleShader ((DWORD *)pbFunc, FALSE, "", &pDisasm);

  if (SUCCEEDED (hr) && strlen ((const char *)pDisasm->GetBufferPointer ()))
  {
    char* szDisasm = _strdup ((const char *)pDisasm->GetBufferPointer ());

    char* comments_end  =                strstr (szDisasm,          "\n ");
    char* footer_begins = comments_end ? strstr (comments_end + 1, "\n\n") : nullptr;

    if (comments_end)  *comments_end  = '\0'; else (comments_end  = "  ");
    if (footer_begins) *footer_begins = '\0'; else (footer_begins = "  ");

    if (! _wcsicmp (wszPrefix, L"ps"))
    {
      ps_disassembly.emplace ( crc32c, SK::D3D9::ShaderDisassembly {
                                         szDisasm,
                                           comments_end + 1,
                                             footer_begins + 1 }
                             );
    }

    else
    {
      vs_disassembly.emplace ( crc32c, SK::D3D9::ShaderDisassembly {
                                         szDisasm,
                                           comments_end + 1,
                                             footer_begins + 1 }
                             );
    }

    free (szDisasm);
  }
}

void
SK_D3D9_SetVertexShader ( IDirect3DDevice9*       /*pDev*/,
                          IDirect3DVertexShader9 *pShader )
{
  if (Shaders.vertex.current.ptr != pShader)
  {
    if (pShader != nullptr)
    {
      EnterCriticalSection (&cs_vs);

      if (Shaders.vertex.rev.find (pShader) == Shaders.vertex.rev.end ())
      {
        LeaveCriticalSection (&cs_vs);

        UINT len = 0;

        pShader->GetFunction (nullptr, &len);

        void* pbFunc =
          malloc (len);

        if (pbFunc != nullptr)
        {
          pShader->GetFunction (pbFunc, &len);

          uint32_t checksum =
            safe_crc32c (0, pbFunc, len);

          SK_D3D9_DumpShader (L"vs", checksum, pbFunc);

          free (pbFunc);

          EnterCriticalSection (&cs_vs);

          Shaders.vertex.rev [pShader] = checksum;
        }
      }

      LeaveCriticalSection (&cs_vs);
    }

    else
      Shaders.vertex.current.crc32c = 0;
  }


  SK_AutoCriticalSection csVS (&cs_vs);

  const uint32_t vs_checksum =
    Shaders.vertex.rev [pShader];

  Shaders.vertex.current.crc32c = vs_checksum;
  Shaders.vertex.current.ptr    = pShader;


  if (vs_checksum != 0x00)
  {
    last_frame.vertex_shaders.emplace (vs_checksum);

    if (tracked_rt.active)
      tracked_rt.vertex_shaders.emplace (vs_checksum);

    if (vs_checksum == tracked_vs.crc32c)
    {
      tracked_vs.use (pShader);

      for ( auto& current_texture : tracked_vs.current_textures )
        current_texture = nullptr;
    }
  }
}

void
SK_D3D9_SetPixelShader ( IDirect3DDevice9*     /*pDev*/,
                         IDirect3DPixelShader9 *pShader )
{
  if (Shaders.pixel.current.ptr != pShader)
  {
    if (pShader != nullptr)
    {
      EnterCriticalSection (&cs_ps);

      if (Shaders.pixel.rev.find (pShader) == Shaders.pixel.rev.end ())
      {
        LeaveCriticalSection (&cs_ps);

        UINT len = 0;

        pShader->GetFunction (nullptr, &len);

        void* pbFunc =
          malloc (len);

        if (pbFunc != nullptr)
        {
          pShader->GetFunction (pbFunc, &len);

          uint32_t checksum =
            safe_crc32c (0, pbFunc, len);

          SK_D3D9_DumpShader (L"ps", checksum, pbFunc);

          free (pbFunc);

          EnterCriticalSection (&cs_ps);

          Shaders.pixel.rev  [pShader] = checksum;
        }
      }

      LeaveCriticalSection (&cs_ps);
    }

    else
      Shaders.pixel.current.crc32c = 0x0;
  }


  SK_AutoCriticalSection csPS (&cs_ps);

  const uint32_t ps_checksum =
    Shaders.pixel.rev [pShader];

  Shaders.pixel.current.crc32c = ps_checksum;
  Shaders.pixel.current.ptr    = pShader;



  if (ps_checksum != 0x00)
  {
    last_frame.pixel_shaders.emplace (ps_checksum);

    if (tracked_rt.active)
      tracked_rt.pixel_shaders.emplace (ps_checksum);

    if (ps_checksum == tracked_ps.crc32c)
    {
      tracked_ps.use (pShader);

      for ( auto& current_texture : tracked_ps.current_textures )
        current_texture = nullptr;
    }
  }
}

void
SK_D3D9_EndScene (void)
{
  //
  //
  //
  //
  //
  //
  //
}

void
SK_D3D9_EndFrame (void)
{
  //draw_state.last_vs     = 0;
  draw_state.scene_count = 0;

  draw_state.draw_count = 0;
  draw_state.next_draw  = 0;

  Shaders.vertex.clear_state ();
  Shaders.pixel.clear_state  ();

  {
    SK_AutoCriticalSection csVS (&cs_vs);
    last_frame.clear ();

//    for (auto& it : tracked_vs.used_textures) it->Release ();

    tracked_vs.clear ();
    tracked_vb.clear ();
    tracked_rt.clear ();
  }

  {
    SK_AutoCriticalSection csVS (&cs_ps);

//    for (auto& it : tracked_ps.used_textures) it->Release ();

    tracked_ps.clear ();
  }

  if (trigger_reset == Clear)
  {
    if (tex_mgr.init)
    {
      //if (tex_mgr.injector.hasPendingLoads ())
        tex_mgr.loadQueuedTextures ();

      tex_mgr.logUsedTextures ();
    }

    if ( (static_cast <IDirect3DDevice9 *>(SK_GetCurrentRenderBackend ().device)->GetAvailableTextureMem () / 1048576UL) < 64UL )
    {
      void
      SK_D3D9_TriggerReset (bool temporary);
      SK_D3D9_TriggerReset (false);
    }
  }

  if (tex_mgr.init)
    tex_mgr.resetUsedTextures ();
}

__declspec (noinline)
bool
SK_D3D9_ShouldSkipRenderPass (D3DPRIMITIVETYPE /*PrimitiveType*/, UINT/* PrimitiveCount*/, UINT /*StartVertex*/, bool& wireframe)
{
  if (SK_GetCurrentRenderBackend ().device == nullptr)
    return false;

  CComPtr <IDirect3DDevice9> pDevice = nullptr;

  if (FAILED (SK_GetCurrentRenderBackend ().device->QueryInterface <IDirect3DDevice9> (&pDevice)))
    return false;

  const uint32_t vs_checksum = Shaders.vertex.current.crc32c;
  const uint32_t ps_checksum = Shaders.pixel.current.crc32c;

  const bool tracking_vs = ( vs_checksum == tracked_vs.crc32c );
  const bool tracking_ps = ( ps_checksum == tracked_ps.crc32c );
  const bool tracking_vb = { vb_stream0  == tracked_vb.vertex_buffer };


  bool skip = false;

  {
    SK_AutoCriticalSection auto_cs (&cs_vs);

    if (Shaders.vertex.blacklist.count (vs_checksum))
      skip      = true;

    if (Shaders.vertex.wireframe.count (vs_checksum))
      wireframe = true;
  }

  {
    SK_AutoCriticalSection auto_cs (&cs_ps);

    if (Shaders.pixel.blacklist.count (ps_checksum))
      skip      = true;

    if (Shaders.pixel.wireframe.count (ps_checksum))
      wireframe = true;
  }


  if (tracking_vs)
  {
    InterlockedIncrement (&tracked_vs.num_draws);

    for (auto& current_texture : tracked_vs.current_textures)
    {
      if (current_texture != nullptr)
      {
        if (! tracked_vs.used_textures.count (current_texture))
        {
          tracked_vs.used_textures.emplace (current_texture);
          //current_texture->AddRef ();
        }
      }
    }

    //
    // TODO: Make generic and move into class -- must pass shader type to function
    //
    for (auto&& it : tracked_vs.constants)
    {
      for (auto&& it2 : it.struct_members)
      {
        if (it2.Override)
          pDevice->SetVertexShaderConstantF (it2.RegisterIndex, it2.Data, 1);
      }

      if (it.Override)
        pDevice->SetVertexShaderConstantF (it.RegisterIndex, it.Data, 1);
    }
  }

  if (tracking_ps)
  {
    SK_AutoCriticalSection auto_cs (&cs_ps);

    InterlockedIncrement (&tracked_ps.num_draws);

    for ( auto& current_texture : tracked_ps.current_textures )
    {
      if (current_texture != nullptr)
      {
        if (! tracked_ps.used_textures.count (current_texture))
        {
          tracked_ps.used_textures.emplace (current_texture);
          //current_texture->AddRef ();
        }
      }
    }

    //
    // TODO: Make generic and move into class -- must pass shader type to function
    //
    for (auto& it : tracked_ps.constants)
    {
      for (auto& it2 : it.struct_members)
      {
        if (it2.Override)
          pDevice->SetPixelShaderConstantF (it2.RegisterIndex, it2.Data, 1);
      }

      if (it.Override)
        pDevice->SetPixelShaderConstantF (it.RegisterIndex, it.Data, 1);
    }
  }


  if (tracked_vb.wireframes.count (vb_stream0))
    wireframe = true;

  if (tracking_vb)
  {
    tracked_vb.use ();

    tracked_vb.instances  = draw_state.instances;

    InterlockedExchangeAdd (&tracked_vb.instanced, static_cast <ULONG> (draw_state.instances));
    InterlockedIncrement   (&tracked_vb.num_draws);

    if (tracked_vb.wireframe)
      wireframe = true;
  }


  if (skip)
    return true;


  if (tracking_vb && tracked_vb.cancel_draws)
    return true;


  if (tracking_vs && tracked_vs.cancel_draws)
    return true;

  if (tracking_ps && tracked_ps.cancel_draws)
    return true;


  //last_vs = vs_checksum;


  if (wireframe)
    pDevice->SetRenderState (D3DRS_FILLMODE, D3DFILL_WIREFRAME);


  return false;
}

void
SK_D3D9_InitShaderModTools (void)
{
  last_frame.vertex_shaders.reserve           (256);
  last_frame.pixel_shaders.reserve            (256);
  last_frame.vertex_buffers.dynamic.reserve   (128);
  last_frame.vertex_buffers.immutable.reserve (256);
  known_objs.dynamic_vbs.reserve              (2048);
  known_objs.static_vbs.reserve               (8192);
  ps_disassembly.reserve                      (512);
  vs_disassembly.reserve                      (512);
  Shaders.vertex.rev.reserve                  (8192);
  Shaders.pixel.rev.reserve                   (8192);

  InitializeCriticalSectionAndSpinCount (&cs_vs, 1024 * 2048 * 2);
  InitializeCriticalSectionAndSpinCount (&cs_ps, 1024 * 2048 * 4);
  InitializeCriticalSectionAndSpinCount (&cs_vb, 1024 * 32);
}



void
EnumConstant ( SK::D3D9::ShaderTracker           *pShader,
               ID3DXConstantTable                *pConstantTable,
               D3DXHANDLE                         hConstant,
               SK::D3D9::ShaderTracker::
               shader_constant_s&                 constant,
               std::vector <
                 SK::D3D9::ShaderTracker::
                             shader_constant_s >& list )
{
  if (! (hConstant && pConstantTable))
    return;

  UINT              one           =  1;
  D3DXCONSTANT_DESC constant_desc = { };

  if (SUCCEEDED (pConstantTable->GetConstantDesc (hConstant, &constant_desc, &one)))
  {
    strncpy (constant.Name, constant_desc.Name, 128);
    constant.Class         = constant_desc.Class;
    constant.Type          = constant_desc.Type;
    constant.RegisterSet   = constant_desc.RegisterSet;
    constant.RegisterIndex = constant_desc.RegisterIndex;
    constant.RegisterCount = constant_desc.RegisterCount;
    constant.Rows          = constant_desc.Rows;
    constant.Columns       = constant_desc.Columns;
    constant.Elements      = constant_desc.Elements;

    if (constant_desc.DefaultValue != nullptr)
      memcpy (constant.Data, constant_desc.DefaultValue, std::min (static_cast <size_t> (constant_desc.Bytes), sizeof (float) * 4));

    for ( UINT j = 0; j < constant_desc.StructMembers; j++ )
    {
      D3DXHANDLE hConstantStruct =
        pConstantTable->GetConstant (hConstant, j);

      SK::D3D9::ShaderTracker::shader_constant_s struct_constant = { };

      if (hConstantStruct != nullptr)
        EnumConstant (pShader, pConstantTable, hConstantStruct, struct_constant, constant.struct_members );
    }

    list.emplace_back (constant);
  }
};



void
SK::D3D9::ShaderTracker::use (IUnknown *pShader)
{
  if (shader_obj != pShader)
  {
    SK_AutoCriticalSection auto_crit0 (&cs_vs);
    SK_AutoCriticalSection auto_crit1 (&cs_ps);

    constants.clear ();

    shader_obj = pShader;

    UINT len = 0;

    if (SUCCEEDED (((IDirect3DVertexShader9 *)pShader)->GetFunction (nullptr, &len)))
    {
      void* pbFunc =
        malloc (len);

      if (pbFunc != nullptr)
      {
        if ( SUCCEEDED ( ((IDirect3DVertexShader9 *)pShader)->GetFunction ( pbFunc,
                                                                              &len )
                       )
           )
        {
          CComPtr <ID3DXConstantTable> pConstantTable = nullptr;

          if (SUCCEEDED (D3DXGetShaderConstantTable ((DWORD *)pbFunc, &pConstantTable)))
          {
            D3DXCONSTANTTABLE_DESC ct_desc = { };

            if (SUCCEEDED (pConstantTable->GetDesc (&ct_desc)))
            {
              UINT constant_count = ct_desc.Constants;

              for (UINT i = 0; i < constant_count; i++)
              {
                D3DXHANDLE hConstant =
                  pConstantTable->GetConstant (nullptr, i);

                shader_constant_s constant = { };

                EnumConstant (this, pConstantTable, hConstant, constant, constants);
              }
            }
          }
        }

        free (pbFunc);
      }
    }
  }
}




#include <SpecialK/ini.h>

void
CALLBACK
RunDLL_HookManager_D3D9 ( HWND  hwnd,        HINSTANCE hInst,
                          LPSTR lpszCmdLine, int )
{
  UNREFERENCED_PARAMETER (hInst);
  UNREFERENCED_PARAMETER (hwnd);

#if 1
  UNREFERENCED_PARAMETER (lpszCmdLine);
#else
  if (StrStrA (lpszCmdLine, "dump"))
  {
    extern void
    __stdcall
    SK_EstablishRootPath (void);
  
    config.system.central_repository = true;
    SK_EstablishRootPath ();
  
    // Setup unhooked function pointers
    SK_PreInitLoadLibrary ();
  
    QueryPerformanceCounter_Original =
      reinterpret_cast <QueryPerformanceCounter_pfn> (
        GetProcAddress (
          GetModuleHandle ( L"kernel32.dll"),
                              "QueryPerformanceCounter" )
      );
  
    config.apis.d3d9.hook    = true;  config.apis.d3d9ex.hook     = true;
    config.apis.OpenGL.hook  = false; config.apis.dxgi.d3d11.hook = false;
    config.apis.NvAPI.enable = false;
    config.steam.preload_overlay                 = false;
    config.steam.silent                          = true;
    config.system.trace_load_library             = false;
    config.system.handle_crashes                 = false;
    config.system.central_repository             = true;
    config.system.game_output                    = false;
    config.render.dxgi.rehook_present            = false;
    config.injection.global.use_static_addresses = false;
    config.input.gamepad.hook_dinput8            = false;
    config.input.gamepad.hook_hid                = false;
    config.input.gamepad.hook_xinput             = false;
  
    SK_Init_MinHook        ();
    SK_ApplyQueuedHooks    ();
  
    SK_SetDLLRole (DLL_ROLE::D3D9);
  
    BOOL
    __stdcall
    SK_Attach (DLL_ROLE role);
  
    extern bool __SK_RunDLL_Bypass;
                __SK_RunDLL_Bypass = true;
  
    BOOL bRet =
      SK_Attach (DLL_ROLE::D3D9);
  
    if (bRet)
    {
      WaitForInit ();
  
      SK_Inject_AddressManager = new SK_Inject_AddressCacheRegistry ();
  
      SK_Inject_AddressManager->storeNamedAddress (L"d3d9", "IDirect3DDevice9::Present",     reinterpret_cast <uintptr_t> (D3D9Present_Target));
      SK_Inject_AddressManager->storeNamedAddress (L"d3d9", "IDirect3DDevice9Ex::PresentEx", reinterpret_cast <uintptr_t> (D3D9PresentEx_Target));
      SK_Inject_AddressManager->storeNamedAddress (L"d3d9", "IDirect3DSwapChain9::Present",  reinterpret_cast <uintptr_t> (D3D9PresentSwap_Target));
  
      SK_Inject_AddressManager->storeNamedAddress (L"d3d9", "IDirect3DDevice9::Reset",       reinterpret_cast <uintptr_t> (D3D9Reset_Target));
      SK_Inject_AddressManager->storeNamedAddress (L"d3d9", "IDirect3DDevice9Ex::ResetEx",   reinterpret_cast <uintptr_t> (D3D9ResetEx_Target));
  
      delete SK_Inject_AddressManager;
  
      SK::D3D9::Shutdown ();
  
      extern iSK_INI* dll_ini;
      DeleteFileW (dll_ini->get_filename ());
    }
  
    ExitProcess (0x00);
  }
#endif
}



void
SK_D3D9_QuickHook (void)
{
  static volatile LONG quick_hooked = FALSE;

  if (! InterlockedCompareExchange (&quick_hooked, TRUE, FALSE))
  {
//  if (GetAsyncKeyState (VK_MENU))
//    return;

    sk_hook_cache_enablement_s state =
      SK_Hook_PreCacheModule ( L"D3D9",
                                 local_d3d9_records,
                                   global_d3d9_records );

    if ( state.hooks_loaded.from_shared_dll > 0 ||
         state.hooks_loaded.from_game_ini   > 0 )
    {
      SK_ApplyQueuedHooks ();
    }

    else
    {
      for ( auto& it : local_d3d9_records )
      {
        it->active = false;
      }
    }

    InterlockedIncrement (&quick_hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&quick_hooked, 2);
}