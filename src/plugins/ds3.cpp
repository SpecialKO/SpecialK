#include <SpecialK/stdafx.h>

#if (defined (NOGDI) || (! defined (_WINGDI_)))
#undef        NOGDI
#undef      _WINGDI_
#include    <wingdi.h>
#endif

//
// Hook Special K's shutdown function
//
typedef bool (WINAPI *ShutdownPlugin_pfn)(const wchar_t *);
               static ShutdownPlugin_pfn SK_ShutdownCore_Original = nullptr;

extern bool WINAPI SK_DS3_ShutdownPlugin (const wchar_t *);

typedef void (CALLBACK *SK_PluginKeyPress_pfn)(BOOL, BOOL, BOOL, BYTE);
static SK_PluginKeyPress_pfn SK_PluginKeyPress_Original = nullptr;

///////////////////////////////////////////
// WinAPI Hooks
///////////////////////////////////////////
typedef BOOL (WINAPI *EnumDisplaySettingsA_pfn)(
  _In_  LPCSTR    lpszDeviceName,
  _In_  DWORD     iModeNum,
  _Out_ DEVMODEA *lpDevMode
);

typedef BOOL (WINAPI *SetWindowPos_pfn)(
  _In_     HWND hWnd,
  _In_opt_ HWND hWndInsertAfter,
  _In_     int  X,
  _In_     int  Y,
  _In_     int  cx,
  _In_     int  cy,
  _In_     UINT uFlags
);
typedef HWND (WINAPI *SetActiveWindow_pfn)(
  HWND hWnd
);


static EnumDisplaySettingsA_pfn EnumDisplaySettingsA_Original = nullptr;
static SetWindowPos_pfn         SetWindowPos_DS3_Original     = nullptr;
static SetActiveWindow_pfn      SetActiveWindow_Original      = nullptr;


typedef void (STDMETHODCALLTYPE *SK_EndFrame_pfn)(void);
static SK_EndFrame_pfn          SK_EndFrame_Original          = nullptr;

extern BOOL
__stdcall
SK_DrawExternalOSD (std::string app_name, std::string text);


#include <d3d11.h>

typedef void (WINAPI *D3D11_RSSetViewports_pfn)(
        ID3D11DeviceContext*,
        UINT,
  const D3D11_VIEWPORT*
);

typedef HRESULT (STDMETHODCALLTYPE *DXGISwap_ResizeTarget_pfn)(
    _In_       IDXGISwapChain *,
    _In_ const DXGI_MODE_DESC *
);

typedef HRESULT (STDMETHODCALLTYPE *DXGISwap_ResizeBuffers_pfn)(
    _In_ IDXGISwapChain *,
    _In_ UINT,
    _In_ UINT,
    _In_ UINT,
    _In_ DXGI_FORMAT,
    _In_ UINT
);

typedef HRESULT (STDMETHODCALLTYPE *DXGISwap_GetFullscreenState_pfn)(
    _In_       IDXGISwapChain  *This,
    _Out_opt_  BOOL            *pFullscreen,
    _Out_opt_  IDXGIOutput    **ppTarget
);


typedef HRESULT (STDMETHODCALLTYPE *DXGISwap_SetFullscreenState_pfn)(
    _In_  IDXGISwapChain *This,
    _In_  BOOL            Fullscreen,
    _Out_ IDXGIOutput    *pTarget
);

interface ID3DX11ThreadPump;

static D3D11_RSSetViewports_pfn
       D3D11_RSSetViewports_Local = nullptr;

DXGISwap_ResizeTarget_pfn         DXGISwap_ResizeTarget_Original         = nullptr;
DXGISwap_ResizeBuffers_pfn        DXGISwap_ResizeBuffers_Original        = nullptr;
DXGISwap_GetFullscreenState_pfn   DXGISwap_GetFullscreenState_Original   = nullptr;
DXGISwap_SetFullscreenState_pfn   DXGISwap_SetFullscreenState_Original   = nullptr;

extern     void    WINAPI D3D11_RSSetViewports_Override     ( ID3D11DeviceContext*,
                                                              UINT,
                                                        const D3D11_VIEWPORT* );

extern HRESULT STDMETHODCALLTYPE
  DXGISwap_ResizeTarget_Override ( IDXGISwapChain *,
                        _In_ const DXGI_MODE_DESC * );

extern HRESULT STDMETHODCALLTYPE
  DXGISwap_ResizeBuffers_Override ( IDXGISwapChain *,
                               _In_ UINT,
                               _In_ UINT,
                               _In_ UINT,
                               _In_ DXGI_FORMAT,
                               _In_ UINT );

extern HRESULT STDMETHODCALLTYPE
  DXGISwap_GetFullscreenState_Override (
    _In_       IDXGISwapChain  *This,
    _Out_opt_  BOOL            *pFullscreen,
    _Out_opt_  IDXGIOutput    **ppTarget );

extern HRESULT STDMETHODCALLTYPE
  DXGISwap_SetFullscreenState_Override (
    _In_  IDXGISwapChain *This,
    _In_  BOOL            Fullscreen,
    _Out_ IDXGIOutput    *pTarget );


extern __declspec (noinline) void CALLBACK SK_PluginKeyPress (BOOL, BOOL, BOOL, BYTE);


void
WINAPI
SK_DS3_RSSetViewports ( ID3D11DeviceContext* This,
                        UINT                 NumViewports,
                  const D3D11_VIEWPORT*      pViewports );

HRESULT
STDMETHODCALLTYPE
SK_DS3_ResizeTarget (
    _In_       IDXGISwapChain *This,
    _In_ const DXGI_MODE_DESC *pNewTargetParameters );

HRESULT
STDMETHODCALLTYPE
SK_DS3_ResizeBuffers (
    _In_ IDXGISwapChain *This,
    _In_ UINT            BufferCount,
    _In_ UINT            Width,
    _In_ UINT            Height,
    _In_ DXGI_FORMAT     NewFormat,
    _In_ UINT            SwapChainFlags );

HRESULT
STDMETHODCALLTYPE
SK_DS3_GetFullscreenState (
    _In_       IDXGISwapChain  *This,
    _Out_opt_  BOOL            *pFullscreen,
    _Out_opt_  IDXGIOutput    **ppTarget );

HRESULT
STDMETHODCALLTYPE
SK_DS3_SetFullscreenState (
    _In_  IDXGISwapChain *This,
    _In_  BOOL            Fullscreen,
    _Out_ IDXGIOutput    *pTarget );



void
CALLBACK
SK_DS3_PluginKeyPress ( BOOL Control,
                        BOOL Shift,
                        BOOL Alt,
                        BYTE vkCode );

void
STDMETHODCALLTYPE
SK_DS3_EndFrame (void);



iSK_INI*              ds3_prefs                 =  nullptr;
wchar_t               ds3_prefs_file [MAX_PATH] = { L'\0' };

sk::ParameterInt*     ds3_hud_res_x             =  nullptr;
sk::ParameterInt*     ds3_hud_res_y             =  nullptr;
sk::ParameterInt*     ds3_hud_offset_x          =  nullptr;
sk::ParameterInt*     ds3_hud_offset_y          =  nullptr;
sk::ParameterBool*    ds3_hud_stretch           =  nullptr;

sk::ParameterInt*     ds3_default_res_x         =  nullptr;
sk::ParameterInt*     ds3_default_res_y         =  nullptr;
sk::ParameterInt*     ds3_sacrificial_x         =  nullptr;
sk::ParameterInt*     ds3_sacrificial_y         =  nullptr;

sk::ParameterBool*    ds3_fullscreen            =  nullptr;
sk::ParameterBool*    ds3_borderless            =  nullptr;
sk::ParameterBool*    ds3_center                =  nullptr;

sk::ParameterBool*    ds3_start_fullscreen      =  nullptr;

sk::ParameterBool*    ds3_flip_mode             =  nullptr;

sk::ParameterBool*    ds3_osd_disclaimer        =  nullptr;

sk::ParameterInt64*   ds3_last_addr             =  nullptr;

struct ds3_state_s {
  IDXGISwapChain* SwapChain  = nullptr;

  bool            Fullscreen = false;
  HWND&           Window     = SK_GetCurrentRenderBackend ().windows.focus.hwnd;

  int             Width      = 0;
  int             Height     = 0;

  struct monitor_s {
    int           Width     = 0;
    int           Height    = 0;
  } monitor;
};

ds3_state_s&
SK_DS3_GetState (void)
{
  static ds3_state_s ds3_state;
  return             ds3_state;
}


struct sus_state_s {
  bool Center     = false;
  bool MaxWindow  = false;
};

sus_state_s&
SK_SUS_GetState (void)
{
  static sus_state_s sus_state;
  return             sus_state;
}


struct ds3_cfg_s {
  struct {
    int  res_x       = 1280;
    int  res_y       = 720;
    int  offset_x    = 0;
    int  offset_y    = 0;
    bool stretch     = false;
  } hud;

  struct {
    bool fullscreen  = false; // Forcefully start in fullscreen
    bool flip_mode   = false;
    int  res_x       = 1920;
    int  res_y       = 1080;

    //
    // Sacrificial resolution:  This will no longer be selectable in-game;
    //                            it will be replaced with the resolution:
    //                              (<res_x> x <res_y>).
    uint32_t
         sacrifice_x = 800;
    uint32_t
         sacrifice_y = 450;
  } render;

  struct {
    bool borderless  = true;
    bool fullscreen  = false;
    bool center      = true;
  } window;

  struct {
    bool disclaimer  = true;
  } osd;
};

ds3_cfg_s&
SK_DS3_GetConfig (void)
{
  static ds3_cfg_s ds3_cfg;
  return           ds3_cfg;
}

#define ds3_cfg   SK_DS3_GetConfig ( )
#define ds3_state SK_DS3_GetState  ( )
#define sus_state SK_SUS_GetState  ( )


#include <SpecialK/core.h>

extern void
__stdcall
SK_SetPluginName (std::wstring name);

#define SUS_VERSION_NUM L"0.6.0"
#define SUS_VERSION_STR L"Souls Unsqueezed v " SUS_VERSION_NUM

// Block until update finishes, otherwise the update dialog
//   will be dismissed as the game crashes when it tries to
//     draw the first frame.
volatile LONG __SUS_init = FALSE;

DWORD
__stdcall
SK_DS3_CheckVersion (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  extern bool
  __stdcall
  SK_FetchVersionInfo (const wchar_t* wszProduct);

  extern HRESULT
  __stdcall
  SK_UpdateSoftware (const wchar_t* wszProduct);


  if (SK_FetchVersionInfo (L"SoulsUnsqueezed"))
    SK_UpdateSoftware (L"SoulsUnsqueezed");

  return 0;
}

static LPVOID __SK_DS3_base_img_addr = nullptr;
static LPVOID __SK_DS3_end_img_addr  = nullptr;

static
void*
SK_DS3_Scan (uint8_t* pattern, size_t len, uint8_t* mask)
{
  uint8_t* base_addr = (uint8_t *)GetModuleHandle (nullptr);

  MEMORY_BASIC_INFORMATION mem_info;
  VirtualQuery (base_addr, &mem_info, sizeof mem_info);

           base_addr = (uint8_t *)mem_info.BaseAddress;//AllocationBase;
  uint8_t* end_addr  = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;

  if (base_addr != (uint8_t *)0x400000) {
    dll_log->Log ( L"[ Sig Scan ] Expected module base addr. 40000h, but got: %ph",
                     base_addr );
  }

  size_t pages = 0;

// Scan up to 256 MiB worth of data
#ifndef _WIN64
uint8_t* const PAGE_WALK_LIMIT = (base_addr + (uintptr_t)(1ULL << 26));
#else
  // Dark Souls 3 needs this, its address space is completely random to the point
  //   where it may be occupying a range well in excess of 36 bits. Probably a stupid
  //     anti-cheat attempt.
uint8_t* const PAGE_WALK_LIMIT = (base_addr + (uintptr_t)(1ULL << 36));
#endif

  //
  // For practical purposes, let's just assume that all valid games have less than 256 MiB of
  //   committed executable image data.
  //
  while (VirtualQuery (end_addr, &mem_info, sizeof mem_info) && end_addr < PAGE_WALK_LIMIT)
  {
    //if (mem_info.Protect & PAGE_NOACCESS || (! (mem_info.Type & MEM_IMAGE)))
      //break;

    pages += VirtualQuery (end_addr, &mem_info, sizeof mem_info);

    end_addr = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;
  }

  if (end_addr > PAGE_WALK_LIMIT) {
    dll_log->Log ( L"[ Sig Scan ] Module page walk resulted in end addr. out-of-range: %ph",
                     end_addr );
    dll_log->Log ( L"[ Sig Scan ]  >> Restricting to %ph",
                     PAGE_WALK_LIMIT );
    end_addr = (uint8_t *)PAGE_WALK_LIMIT;
  }

  dll_log->Log ( L"[ Sig Scan ] Module image consists of %zu pages, from %ph to %ph",
                   pages,
                     base_addr,
                       end_addr );

  __SK_DS3_base_img_addr = base_addr;
  __SK_DS3_end_img_addr  = end_addr;

  uint8_t*  begin = (uint8_t *)base_addr;
  uint8_t*  it    = begin;
  int       idx   = 0;

  while (it < end_addr)
  {
    VirtualQuery (it, &mem_info, sizeof mem_info);

    // Bail-out once we walk into an address range that is not resident, because
    //   it does not belong to the original executable.
    if (mem_info.RegionSize == 0)
      break;

    uint8_t* next_rgn =
     (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;

    if ( (! (mem_info.Type    & MEM_IMAGE))  ||
         (! (mem_info.State   & MEM_COMMIT)) ||
             mem_info.Protect & PAGE_NOACCESS ) {
      it    = next_rgn;
      idx   = 0;
      begin = it;
      continue;
    }

    // Do not search past the end of the module image!
    if (next_rgn >= end_addr)
      break;

    while (it < next_rgn) {
      uint8_t* scan_addr = it;

      bool match = (*scan_addr == pattern [idx]);

      // For portions we do not care about... treat them
      //   as matching.
      if (mask != nullptr && (! mask [idx]))
        match = true;

      if (match)
      {
        if (++idx == (int)len)
          return (void *)begin;

        ++it;
      }

      else {
        // No match?!
        if (it > end_addr - len)
          break;

        it  = ++begin;
        idx = 0;
      }
    }
  }

  return nullptr;
}

// This should be unnecessary on x86/x64 due to cache snooping, but
//   do it anyway for completeness.
void
SK_FlushInstructionCache ( LPCVOID base_addr,
                           size_t  code_size )
{
  FlushInstructionCache ( GetCurrentProcess (),
                            base_addr,
                              code_size );
}

void
SK_InjectMachineCode ( LPVOID   base_addr,
                       uint8_t* new_code,
                       size_t   code_size,
                       DWORD    permissions,
                       uint8_t* old_code = nullptr )
{
  if (SK_InjectMemory ( base_addr,
                          new_code,
                            code_size,
                              permissions,
                                old_code) )
    SK_FlushInstructionCache (base_addr, code_size);
}

struct monitor_dims_s {
  std::pair <int, int> work;
  std::pair <int, int> full;
};

monitor_dims_s
SK_DS3_GetMonitorDims (void)
{
  monitor_dims_s dims;

  MONITORINFO minfo = { };
  minfo.cbSize      = sizeof MONITORINFO;

  GetMonitorInfo ( MonitorFromWindow ( ds3_state.Window,
                                         MONITOR_DEFAULTTOPRIMARY ),
                     &minfo );

  ds3_state.monitor.Width  = minfo.rcMonitor.right  - minfo.rcMonitor.left;
  ds3_state.monitor.Height = minfo.rcMonitor.bottom - minfo.rcMonitor.top;

  dims.full.first  = ds3_state.monitor.Width;
  dims.full.second = ds3_state.monitor.Height;

  dims.work.first  = minfo.rcWork.right  - minfo.rcWork.left;
  dims.work.second = minfo.rcWork.bottom - minfo.rcWork.top;

  return dims;
}

DWORD
__stdcall
SK_DS3_CenterWindow_Thread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  SK_DS3_GetMonitorDims ();

  if (! sus_state.Center)
    return 0;

  dll_log->Log ( L"[SUS PlugIn] [!] SK_DS3_CenterWindow (void) -- [Calling Thread: 0x%04x]",
                   GetCurrentThreadId () );
  dll_log->Log ( L"[SUS PlugIn] \tMonitor: [%lux%lu] <-> Window: [%lux%lu] :: { %s }, <HWND: 0x%04X>",
                   ds3_state.monitor.Width, ds3_state.monitor.Height,
                     ds3_state.Width, ds3_state.Height,
                       ds3_state.Fullscreen ? L"Fullscreen" : L"Windowed",
                         ds3_state.Window );

  if ((! ds3_state.Fullscreen) || ds3_cfg.window.borderless) {
    int x_off = 0;
    int y_off = 0;

    if ( ds3_state.monitor.Width  > ds3_state.Width &&
         ds3_state.monitor.Height > ds3_state.Height ) {
      x_off = (ds3_state.monitor.Width  - ds3_state.Width)  / 2;
      y_off = (ds3_state.monitor.Height - ds3_state.Height) / 2;
    }

    DWORD dwFlags = SWP_NOSIZE | SWP_NOZORDER;

    BringWindowToTop         (ds3_state.Window);
    SetForegroundWindow      (ds3_state.Window);

    SetWindowPos (
      ds3_state.Window, HWND_TOP,
        0 + x_off, 0 + y_off,
          ds3_state.Width, ds3_state.Height,
            dwFlags
    );
  }

  return 0;
}

DWORD
__stdcall
SK_DS3_FinishResize_Thread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  DWORD dwFlags = SWP_NOMOVE | SWP_NOSENDCHANGING;

  if (ds3_cfg.window.borderless) {
    SetWindowLongW (ds3_state.Window, GWL_STYLE, WS_POPUP | WS_MINIMIZEBOX | WS_VISIBLE);
    dwFlags |= SWP_FRAMECHANGED;
  }

  BringWindowToTop    (ds3_state.Window);
  SetForegroundWindow (ds3_state.Window);

  dwFlags |= SWP_NOZORDER;

  SetWindowPos (
    ds3_state.Window, HWND_TOP,
      0, 0,
        ds3_state.Width, ds3_state.Height,
          dwFlags
  );

  //SK_DS3_CenterWindow ();

  return 0;
}

void
SK_DS3_FinishResize (void)
{
  // It is not safe to do this stuff from the render or message pump thread,
  //   so always spawn a worker thread to do it. This prevents deadlocks.
  CreateThread ( nullptr,
                    0,
                      SK_DS3_FinishResize_Thread,
                        nullptr,
                          0x00,
                            nullptr );
}

void
SK_DS3_CenterWindow (void)
{
  // It is not safe to do this stuff from the render or message pump thread,
  //   so always spawn a worker thread to do it. This prevents deadlocks.
  CreateThread ( nullptr,
                    0,
                      SK_DS3_CenterWindow_Thread,
                        nullptr,
                          0x00,
                            nullptr );
}

BOOL
WINAPI
SK_DS3_SetWindowPos (
  _In_     HWND hWnd,
  _In_opt_ HWND hWndInsertAfter,
  _In_     int  X,
  _In_     int  Y,
  _In_     int  cx,
  _In_     int  cy,
  _In_     UINT uFlags )
{
  if (hWnd == ds3_state.Window)
    return TRUE;
  else
    return SetWindowPos_DS3_Original (
      hWnd,
        hWndInsertAfter,
          X, Y,
            cx, cy,
              uFlags );
}

HWND
WINAPI
SK_DS3_SetActiveWindow (
  HWND hWnd )
{
  HWND hWndRet = hWnd;

  if (hWnd == ds3_state.Window) {
    if (ds3_cfg.window.borderless || (! ds3_state.Fullscreen)) {
      SK_DS3_FinishResize ();
      SK_DS3_CenterWindow ();
    }
  }

  hWndRet =
    SetActiveWindow_Original (hWnd);

  //if (ds3_state.Fullscreen)
    //DXGISwap_SetFullscreenState_Original (ds3_state.SwapChain, TRUE, nullptr);


  return hWndRet;
}



extern void SK_DisableDPIScaling (void);

void
SK_DS3_InitPlugin (void)
{
  ds3_state.Width  = ds3_cfg.render.res_x;
  ds3_state.Height = ds3_cfg.render.res_y;

  if (ds3_prefs == nullptr)
  {
    SK_DisableDPIScaling ();

    // Make the graphics config file read-only while running
    DWORD    dwConfigAttribs;
    uint32_t dwLen                = MAX_PATH;
    wchar_t  wszGraphicsConfigPath [MAX_PATH + 1];

    SK_GetUserProfileDir (wszGraphicsConfigPath, &dwLen);

    wcscat ( wszGraphicsConfigPath,
               L"\\AppData\\Roaming\\DarkSoulsIII\\GraphicsConfig.xml" );

    dwConfigAttribs = GetFileAttributesW (wszGraphicsConfigPath);
                      SetFileAttributesW ( wszGraphicsConfigPath,
                                             dwConfigAttribs |
                                             FILE_ATTRIBUTE_READONLY );

    SK_SetPluginName (SUS_VERSION_STR);

    lstrcatW (ds3_prefs_file, SK_GetConfigPath ());
    lstrcatW (ds3_prefs_file, L"SoulsUnsqueezed.ini");

    ds3_prefs =
      SK_CreateINI (ds3_prefs_file);
  }

  ds3_hud_res_x =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory->create_parameter <int> (L"HUDResX"));
  ds3_hud_res_x->register_to_ini ( ds3_prefs,
                                    L"SUS.Display",
                                      L"HUDResX" );

  ds3_hud_res_x->load (ds3_cfg.hud.res_x);

  ds3_hud_res_y =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory->create_parameter <int> (L"HUDResY"));
  ds3_hud_res_y->register_to_ini ( ds3_prefs,
                                    L"SUS.Display",
                                      L"HUDResY" );

  ds3_hud_res_y->load (ds3_cfg.hud.res_y);

  ds3_hud_offset_x =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory->create_parameter <int> (L"HUDOffsetX"));
  ds3_hud_offset_x->register_to_ini ( ds3_prefs,
                                        L"SUS.Display",
                                          L"HUDOffsetX" );

  ds3_hud_offset_x->load (ds3_cfg.hud.offset_x);

  ds3_hud_offset_y =
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory->create_parameter <int> (L"HUDOffsetY"));
  ds3_hud_offset_y->register_to_ini ( ds3_prefs,
                                        L"SUS.Display",
                                          L"HUDOffsetY" );

  ds3_hud_offset_y->load (ds3_cfg.hud.offset_y);

  ds3_hud_stretch =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"StretchHUD"));
  ds3_hud_stretch->register_to_ini ( ds3_prefs,
                                        L"SUS.Display",
                                          L"StretchHUD" );

  ds3_hud_stretch->load (ds3_cfg.hud.stretch);


  ds3_flip_mode =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory->create_parameter <bool> (L"FlipMode"));
  ds3_flip_mode->register_to_ini ( ds3_prefs,
                                     L"SUS.Render",
                                       L"FlipMode" );

  ds3_flip_mode->load (ds3_cfg.render.flip_mode);

  ds3_start_fullscreen =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory->create_parameter <bool> (L"Start in Fullscreen"));
  ds3_start_fullscreen->register_to_ini ( ds3_prefs,
                                            L"SUS.Render",
                                              L"StartFullscreen" );

  ds3_start_fullscreen->load (ds3_cfg.render.fullscreen);


  ds3_borderless =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory->create_parameter <bool> (L"Borderless"));
  ds3_borderless->register_to_ini ( ds3_prefs,
                                      L"SUS.Window",
                                        L"Borderless" );

  ds3_borderless->load (ds3_cfg.window.borderless);


  ds3_fullscreen =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory->create_parameter <bool> (L"Forceful Fullscreen Windows"));
  ds3_fullscreen->register_to_ini ( ds3_prefs,
                                      L"SUS.Window",
                                        L"Fullscreen" );

  if (ds3_fullscreen->load (ds3_cfg.window.fullscreen))
  {
    sus_state.MaxWindow = ds3_cfg.window.fullscreen;
  }


  ds3_center =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory->create_parameter <bool> (L"Center Windows"));
  ds3_center->register_to_ini ( ds3_prefs,
                                  L"SUS.Window",
                                    L"Center" );

  if (ds3_center->load (ds3_cfg.window.center))
  {
    sus_state.Center = ds3_cfg.window.center;
  }



  ds3_last_addr =
    dynamic_cast <sk::ParameterInt64 *>
      (g_ParameterFactory->create_parameter <int64_t> (L"Last Known Address"));
  ds3_last_addr->register_to_ini ( ds3_prefs,
                                     L"SUS.System",
                                       L"LastKnownAddr" );


  ds3_osd_disclaimer =
    dynamic_cast <sk::ParameterBool *>
      (g_ParameterFactory->create_parameter <bool> (L"Show OSD Disclaimer"));
  ds3_osd_disclaimer->register_to_ini ( ds3_prefs,
                                          L"SUS.System",
                                            L"ShowOSDDisclaimer" );

  if (! ds3_osd_disclaimer->load (ds3_cfg.osd.disclaimer))
  {
    ds3_cfg.osd.disclaimer = true;

    ds3_osd_disclaimer->store (true);

    ds3_prefs->write (ds3_prefs_file);
  }

#if 0
  sk::ParameterStringW ini_ver =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory->create_parameter <std::wstring> (L"Last Version"));
  ds3_last_addr->register_to_ini ( ds3_prefs,
                                     L"SUS.System",
                                       L"Version" );
#endif

  ds3_default_res_x =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory->create_parameter <int> (L"Base (Windowed) Resolution"));
  ds3_default_res_x->register_to_ini ( ds3_prefs,
                                         L"SUS.Render",
                                           L"DefaultResX" );

  ((sk::iParameter *)ds3_default_res_x)->load ();

  ds3_default_res_y =
    dynamic_cast <sk::ParameterInt *>
      (g_ParameterFactory->create_parameter <int> (L"Base (Windowed) Resolution"));
  ds3_default_res_y->register_to_ini ( ds3_prefs,
                                         L"SUS.Render",
                                           L"DefaultResY" );

  ((sk::iParameter *)ds3_default_res_y)->load ();


  ds3_sacrificial_x =
    dynamic_cast <sk::ParameterInt *>
    (g_ParameterFactory->create_parameter <int> (L"Sacrificial (Windowed) Resolution"));
  ds3_sacrificial_x->register_to_ini ( ds3_prefs,
                                         L"SUS.Render",
                                           L"SacrificialResX" );

  ds3_sacrificial_x->load ((int &)ds3_cfg.render.sacrifice_x);

  ds3_sacrificial_y =
    dynamic_cast <sk::ParameterInt *>
    (g_ParameterFactory->create_parameter <int> (L"Sacrificial (Windowed) Resolution"));
  ds3_sacrificial_y->register_to_ini ( ds3_prefs,
                                         L"SUS.Render",
                                           L"SacrificialResY" );

  ds3_sacrificial_y->load ((int &)ds3_cfg.render.sacrifice_y);


  uint8_t* sac_x = (uint8_t *)&ds3_cfg.render.sacrifice_x;
  uint8_t* sac_y = (uint8_t *)&ds3_cfg.render.sacrifice_y;

  // We are going to scan for an array of resolutions, it should be the only array
  //   in the executable image of the form <uint32_t : ResX>,<uint32_t : ResY>,...
  uint8_t res_sig  [] = { sac_x [0], sac_x [1], sac_x [2], sac_x [3],
                          sac_y [0], sac_y [1], sac_y [2], sac_y [3] };
  uint8_t res_mask [] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

  uint32_t res_x = ds3_default_res_x->get_value ();
  uint32_t res_y = ds3_default_res_y->get_value ();

  void* res_addr = nullptr;

  if (((sk::iParameter *)ds3_last_addr)->load ())
  {
    res_addr =
      (void *)ds3_last_addr->get_value ();

    MEMORY_BASIC_INFORMATION mem_info;
    VirtualQuery (res_addr, &mem_info, sizeof mem_info);

    if (mem_info.Protect & PAGE_NOACCESS)
      res_addr = nullptr;

    if (res_addr != nullptr && (! memcmp (res_addr, res_sig, 8))) {
      dll_log->Log ( L"[Asp. Ratio] Skipping Signature Scan,"
                     L" Last Known Address = GOOD" );
    } else {
      res_addr = nullptr;
    }
  }

  if (res_addr == nullptr)
    res_addr = SK_DS3_Scan (res_sig, 8, nullptr);

  if (res_addr != nullptr)
  {
    ds3_last_addr->store ((int64_t)res_addr);
    ds3_prefs->write     (ds3_prefs_file);

    void* res_addr_x = res_addr;
    void* res_addr_y = (uint8_t *)res_addr + 4;

    if (res_x != ds3_cfg.render.sacrifice_x || res_y != ds3_cfg.render.sacrifice_y)
    {
      SK_InjectMemory (res_addr_x, (uint8_t *)&res_x, 4, PAGE_EXECUTE_READWRITE);
      SK_InjectMemory (res_addr_y, (uint8_t *)&res_y, 4, PAGE_EXECUTE_READWRITE);
      dll_log->Log ( L"[Asp. Ratio] Custom Default Resolution: (%lux%lu) {%3.2f}",
                       res_x, res_y,
                         (float)res_x / (float)res_y );
      dll_log->Log ( L"[Asp. Ratio]   >> Sacrifice Address: %ph", res_addr);
    }
  }

  else {
    dll_log->Log ( L"[Asp. Ratio] >> ERROR: Unable to locate memory address for %lux%lu... <<",
                   *(uint32_t *)res_sig, *((uint32_t *)res_sig+1) );
  }


  SK_CreateDLLHook2 (      L"user32.dll",
                            "SetActiveWindow",
                             SK_DS3_SetActiveWindow,
    static_cast_p2p <void> (&SetActiveWindow_Original) );

  SK_CreateFuncHook ( L"ID3D11DeviceContext::RSSetViewports",
                                       D3D11_RSSetViewports_Override,
                                      SK_DS3_RSSetViewports,
              static_cast_p2p <void> (&D3D11_RSSetViewports_Local) );
  MH_QueueEnableHook (                 D3D11_RSSetViewports_Override);

  SK_CreateFuncHook ( L"IDXGISwapChain::ResizeTarget",
                               DXGISwap_ResizeTarget_Override,
                                 SK_DS3_ResizeTarget,
      static_cast_p2p <void> (&DXGISwap_ResizeTarget_Original) );
  MH_QueueEnableHook (         DXGISwap_ResizeTarget_Override);

  SK_CreateFuncHook ( L"IDXGISwapChain::ResizeBuffers",
                        DXGISwap_ResizeBuffers_Override,
                          SK_DS3_ResizeBuffers,
                            static_cast_p2p <void> (&DXGISwap_ResizeBuffers_Original) );
  MH_QueueEnableHook (DXGISwap_ResizeBuffers_Override);


  SK_CreateFuncHook ( L"IDXGISwapChain::GetFullscreenState",
                               DXGISwap_GetFullscreenState_Override,
                                 SK_DS3_GetFullscreenState,
      static_cast_p2p <void> (&DXGISwap_GetFullscreenState_Original) );
  MH_QueueEnableHook (         DXGISwap_GetFullscreenState_Override);

  SK_CreateFuncHook ( L"IDXGISwapChain::SetFullscreenState",
                               DXGISwap_SetFullscreenState_Override,
                                 SK_DS3_SetFullscreenState,
      static_cast_p2p <void> (&DXGISwap_SetFullscreenState_Original) );
  MH_QueueEnableHook (         DXGISwap_SetFullscreenState_Override);


  SK_CreateFuncHook (      L"SK_PluginKeyPress",
                             SK_PluginKeyPress,
                         SK_DS3_PluginKeyPress,
    static_cast_p2p <void> (&SK_PluginKeyPress_Original) );
  MH_QueueEnableHook (       SK_PluginKeyPress);



#if 0
  SK_CreateFuncHook (      L"SK_ShutdownCore",
                             SK_ShutdownCore,
                         SK_DS3_ShutdownPlugin,
    static_cast_p2p <void> (&SK_ShutdownCore_Original) );
  MH_QueueEnableHook (       SK_ShutdownCore);
#endif

  SK_CreateFuncHook (      L"SK_BeginBufferSwap",
                             SK_BeginBufferSwap,
                         SK_DS3_EndFrame,
    static_cast_p2p <void> (&SK_EndFrame_Original) );
  MH_QueueEnableHook (       SK_BeginBufferSwap);

  SK_ApplyQueuedHooks ();
}

HRESULT
STDMETHODCALLTYPE
SK_DS3_GetFullscreenState (
  _In_      IDXGISwapChain  *This,
  _Out_opt_ BOOL            *pFullscreen,
  _Out_opt_  IDXGIOutput   **ppTarget )
{
  HRESULT hr = S_OK;

  if (! ds3_cfg.window.borderless)
  {
    BOOL bFullscreen = TRUE;

    hr =
      DXGISwap_GetFullscreenState_Original (This, &bFullscreen, ppTarget);

    //if (SUCCEEDED (hr))
    ds3_state.Fullscreen = (bFullscreen != FALSE);
  }

  else
  {
    DXGISwap_GetFullscreenState_Original (This, nullptr, ppTarget);
  }

  if (pFullscreen != nullptr)
    *pFullscreen = ds3_state.Fullscreen;

  return S_OK;// hr;
  //return DXGISwap_GetFullscreenState_Original (This, nullptr, nullptr);
}

typedef BOOL (WINAPI *EnumDisplaySettingsW_pfn) (
                        _In_opt_ LPWSTR    lpszDeviceName,
                        _In_     DWORD      iModeNum,
                        _Inout_  DEVMODEW *lpDevMode
);
extern EnumDisplaySettingsW_pfn EnumDisplaySettingsW_Original;

using ChangeDisplaySettingsW_pfn = LONG (WINAPI *)(
  _In_opt_ DEVMODEW *lpDevMode,
  _In_     DWORD     dwFlags
);
extern ChangeDisplaySettingsW_pfn ChangeDisplaySettingsW_Original;

HRESULT
STDMETHODCALLTYPE
SK_DS3_SetFullscreenState (
  _In_ IDXGISwapChain *This,
  _In_ BOOL            Fullscreen,
  _In_ IDXGIOutput    *pTarget )
{
  // No need to check if the mode switch actually worked, we're faking it
  if (ds3_cfg.window.borderless)
    ds3_state.Fullscreen = (Fullscreen != FALSE);

  ds3_state.SwapChain  = This;

  DXGI_SWAP_CHAIN_DESC swap_desc;
  if (SUCCEEDED (This->GetDesc (&swap_desc)))
  {
    if (swap_desc.OutputWindow != 0)
      ds3_state.Window = swap_desc.OutputWindow;

    // Reset the temporary monitor mode change we may have made earlier
    if ((! ds3_state.Fullscreen) && ds3_cfg.window.borderless && sus_state.MaxWindow)
      ChangeDisplaySettingsW_Original (0, CDS_RESET);

    int num_monitors = GetSystemMetrics (SM_CMONITORS);
    int virtual_x    = GetSystemMetrics (SM_CXVIRTUALSCREEN);
    int virtual_y    = GetSystemMetrics (SM_CYVIRTUALSCREEN);

    dll_log->Log ( L"[ Monitors ]  + Display Topology: %lu Monitors - "
                                       L"(Virtual Res: %lux%lu)",
                     num_monitors, virtual_x, virtual_y );

    if (ds3_cfg.window.borderless && sus_state.MaxWindow)
    {
      DEVMODEW devmode = { };
               devmode.dmSize  = sizeof DEVMODEW;

      EnumDisplaySettingsW_Original (nullptr, ENUM_CURRENT_SETTINGS, &devmode);

      // We may need to do a full-on temporary device mode change, since I don't want to
      //   bother with viewport hacks at the moment.
      //
      //  XXX: Later on, we can restore the game's usual viewport hackery.
      if (ds3_state.Fullscreen)
      {
        bool multi_mon_match = ( (int)swap_desc.BufferDesc.Width  == virtual_x &&
                                 (int)swap_desc.BufferDesc.Height == virtual_y );

        //
        // This logic really only works correctly on single-monitor setups,
        //   or setups where the additional monitors are not meant for rendering.
        //
        if ( (! multi_mon_match) && ( devmode.dmPelsHeight != swap_desc.BufferDesc.Height ||
                                      devmode.dmPelsWidth  != swap_desc.BufferDesc.Width ) )
        {
          devmode.dmPelsWidth  = swap_desc.BufferDesc.Width;
          devmode.dmPelsHeight = swap_desc.BufferDesc.Height;

          ChangeDisplaySettingsW_Original (&devmode, CDS_FULLSCREEN);

          ds3_state.monitor.Width  = swap_desc.BufferDesc.Width;
          ds3_state.monitor.Height = swap_desc.BufferDesc.Height;
        }

        else if (multi_mon_match)
        {
          ds3_state.monitor.Width  = swap_desc.BufferDesc.Width;
          ds3_state.monitor.Height = swap_desc.BufferDesc.Height;
        }
      }
    }
  }

  if (ds3_cfg.window.borderless)
  {
    SK_DS3_FinishResize ();
    SK_DS3_CenterWindow ();

    HRESULT ret = S_OK;
    //DXGI_CALL (ret, (S_OK))
    return ret;
  }

  bool original_state  = ds3_state.Fullscreen;
  ds3_state.Fullscreen = (Fullscreen != FALSE);

  HRESULT ret =
    DXGISwap_SetFullscreenState_Original (This, Fullscreen, pTarget);

  if (! SUCCEEDED (ret))
    ds3_state.Fullscreen = original_state;
//  SK_DS3_CenterWindow   ();

  return ret;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
SK_DS3_ResizeBuffers (IDXGISwapChain *This,
                 _In_ UINT            BufferCount,
                 _In_ UINT            Width,
                 _In_ UINT            Height,
                 _In_ DXGI_FORMAT     NewFormat,
                 _In_ UINT            SwapChainFlags)
{
  HRESULT hr =
    DXGISwap_ResizeBuffers_Original ( This,
                                        BufferCount,
                                          Width, Height,
                                            NewFormat,
                                              SwapChainFlags );

  if (SUCCEEDED (hr))
  {
    if (Width != 0)
      ds3_state.Width  = Width;

    if (Height != 0)
      ds3_state.Height = Height;

    SK_DS3_CenterWindow ();
  }

  return hr;
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
SK_DS3_ResizeTarget ( IDXGISwapChain *This,
           _In_ const DXGI_MODE_DESC *pNewTargetParameters )
{
  HRESULT ret =
    DXGISwap_ResizeTarget_Original (This, pNewTargetParameters);

  if (SUCCEEDED (ret)) {
    if (pNewTargetParameters != nullptr) {
      if (pNewTargetParameters->Width > 0)
        ds3_state.Width = pNewTargetParameters->Width;

      if (pNewTargetParameters->Height > 0)
        ds3_state.Height = pNewTargetParameters->Height;
    }

    if (ds3_cfg.window.borderless || (! ds3_state.Fullscreen))
      SK_DS3_FinishResize ();

    SK_DS3_CenterWindow ();

    //DXGISwap_ResizeBuffers_Override (This, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0x02);
  }

  return ret;
}

void
WINAPI
SK_DS3_RSSetViewports ( ID3D11DeviceContext* This,
                        UINT                 NumViewports,
                  const D3D11_VIEWPORT*      pViewports )
{
  D3D11_VIEWPORT* pNewViewports = new D3D11_VIEWPORT [NumViewports];

  for (UINT i = 0; i < NumViewports; i++) {
    pNewViewports [i] = pViewports [i];

    //bool is_16by9 = false;

    //dll_log.Log (L"[!] BEFORE { %i <%f,%f::%f,%f [%f,%f]> }",
                    //i, pNewViewports [i].Width,    pNewViewports [i].Height,
                      //pNewViewports [i].TopLeftX, pNewViewports [i].TopLeftY,
                      //pNewViewports [i].MinDepth, pNewViewports [i].MaxDepth );

    //if (pNewViewports [i].Width >= 16.0f * (pNewViewports [i].Height / 9.0f) - 0.001f &&
        //pNewViewports [i].Width <= 16.0f * (pNewViewports [i].Height / 9.0f) + 0.001f)
    //is_16by9 = true;

    // The game may do this to the UI for certain resolutions, we need to be proactive.
    bool incorrectly_centered = false;
      //( (pViewports [i].TopLeftY != 0.0f && pViewports [i].Height == (float)__DS3_HEIGHT + (-2.0f * pViewports [i].TopLeftY)) ||
        //(pViewports [i].TopLeftX != 0.0f && pViewports [i].Width  == (float)__DS3_WIDTH  + (-2.0f * pViewports [i].TopLeftX)) );


    float aspect0 = pViewports [i].Width   / pViewports [i].Height;
    float aspect1 = (float)ds3_state.Width / (float)ds3_state.Height;

    if (pViewports [i].Width == 1280.0f && pViewports [i].Height == 720.0f &&
        pViewports [i].MinDepth == 0.0f && pViewports [i].MaxDepth == 0.0f)
    {
      pNewViewports [i].TopLeftX = (float)ds3_cfg.hud.offset_x;
      pNewViewports [i].TopLeftY = (float)ds3_cfg.hud.offset_y;
      pNewViewports [i].Width    = (float)ds3_cfg.hud.res_x;
      pNewViewports [i].Height   = (float)ds3_cfg.hud.res_y;
    }

    else if (ds3_cfg.hud.stretch &&
             (((pViewports [i].MinDepth == pViewports [i].MaxDepth && pViewports [i].MinDepth == 0.0f) &&
#if 1
               fabs (aspect0 - aspect1) <  0.1f
#else
                pViewports [i].Width  > (float)ds3_state.Width  - 4.0f &&
                pViewports [i].Width  < (float)ds3_state.Width  + 4.0f &&
                pViewports [i].Height > (float)ds3_state.Height - 4.0f &&
                pViewports [i].Height < (float)ds3_state.Height + 4.0f
#endif
               ) ||
               (incorrectly_centered))) {
      if ((float)ds3_state.Width / (float)ds3_state.Height >= (16.0f / 9.0f))
      {
        float rescaled_width = pNewViewports [i].Width * ((float)ds3_state.Width / (float)ds3_state.Height) / (16.0f / 9.0f);
        float excess_width   = rescaled_width - pNewViewports [i].Width;

        pNewViewports [i].Width    *= ((float)ds3_state.Width / (float)ds3_state.Height) / (16.0f / 9.0f);
        pNewViewports [i].Height   = (float)ds3_state.Height;
        pNewViewports [i].TopLeftX = -excess_width / 2.0f;
        pNewViewports [i].TopLeftY = 0.0f;
      }

      else
      {
        float rescaled_height = pNewViewports [i].Height * (16.0f / 9.0f) / ((float)ds3_state.Width / (float)ds3_state.Height);
        float excess_height   = rescaled_height - pNewViewports [i].Height;

        pNewViewports [i].Width    = (float)ds3_state.Width;
        pNewViewports [i].Height   *= (16.0f / 9.0f) / ((float)ds3_state.Width / (float)ds3_state.Height);
        pNewViewports [i].TopLeftX = 0.0f;
        pNewViewports [i].TopLeftY = -excess_height / 2.0f;
      }

      pNewViewports [i].MinDepth = pViewports [i].MaxDepth;
      pNewViewports [i].MaxDepth = pViewports [i].MaxDepth;
    }

#if 0
    dll_log.Log (L"[!] AFTER { %i <%f,%f::%f,%f [%f,%f]> }",
                    i, pNewViewports [i].Width,    pNewViewports [i].Height,
                      pNewViewports [i].TopLeftX, pNewViewports [i].TopLeftY,
                      pNewViewports [i].MinDepth, pNewViewports [i].MaxDepth );
#endif
  }

  D3D11_RSSetViewports_Local (This, NumViewports, pNewViewports);

  delete [] pNewViewports;
}


bool
SK_DS3_UseFlipMode (void)
{
  return ds3_cfg.render.flip_mode;
}

void
CALLBACK
SK_DS3_PluginKeyPress ( BOOL Control,
                        BOOL Shift,
                        BOOL Alt,
                        BYTE vkCode )
{
  if (Control && Shift && Alt && vkCode == VK_OEM_PERIOD) {
    ds3_cfg.hud.stretch = (! ds3_cfg.hud.stretch);
  }

  SK_PluginKeyPress_Original (Control, Shift, Alt,vkCode);
}

#define DS3_SendScancodeMake(vk,x)   { keybd_event ((vk), (x), KEYEVENTF_SCANCODE,                   0); }
#define DS3_SendScancodeBreak(vk, y) { keybd_event ((vk), (y), KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP, 0); }

//
// Make and break
//
//  Due to input quirks in DS3, we have to issue the key release (break) event from a separate thread.
//
//    A simple FIFO queue amortized across multiple frames is insufficient and this kludge must stay.
//
#define DS3_SendScancode(vk,x,y) {                         DS3_SendScancodeMake  ((vk), (x));                          \
          CreateThread (nullptr, 0, [](LPVOID) -> DWORD {             SleepEx (66, FALSE);                             \
                                                           DS3_SendScancodeBreak ((vk), (x));                          \
                                                           SK_Thread_CloseSelf ();                                     \
                                                                                  return 0; }, nullptr, 0x0, nullptr); \
};

DWORD
__stdcall
SK_DS3_FullscreenToggle_Thread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  // Don't do any of this stuff if we cannot bring the window into focus
  if ( ! (BringWindowToTop    (ds3_state.Window) ||
          SetForegroundWindow (ds3_state.Window) ||
          SetActiveWindow     (ds3_state.Window) ) )
    return std::numeric_limits <unsigned int>::max ();

  SK_Sleep (66);

  SetFocus (ds3_state.Window);

  DS3_SendScancode (VK_MENU,   0x38, 0xb8);
  DS3_SendScancode (VK_RETURN, 0x1c, 0x9c);

  return 0;
}

// Sit and spin until the user figures out what an OSD is
//
DWORD
WINAPI
SK_DS3_OSD_Disclaimer (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  while ((volatile bool&)config.osd.show)
    SK_Sleep (66);

  ds3_osd_disclaimer->store     (false);

  ds3_prefs->write              (ds3_prefs_file);

  SK_Thread_CloseSelf ();

  return 0;
}

void
STDMETHODCALLTYPE
SK_DS3_EndFrame (void)
{
  SK_EndFrame_Original ();

  if (ds3_osd_disclaimer->get_value ())
    SK_DrawExternalOSD ( "SUS", "  Press Ctrl + Shift + O         to toggle In-Game OSD\n"
                                "  Press Ctrl + Shift + Backspace to access In-Game Config Menu\n\n"
                                "   * This message will go away the first time you actually read it and successfully toggle the OSD.\n" );
  else
    SK_DrawExternalOSD ( "SUS", "" );
}

HRESULT
STDMETHODCALLTYPE
SK_DS3_PresentFirstFrame ( IDXGISwapChain *This,
                           UINT            SyncInterval,
                           UINT            Flags )
{
  UNREFERENCED_PARAMETER (Flags);
  UNREFERENCED_PARAMETER (SyncInterval);

  if (! InterlockedCompareExchange (&__SUS_init, 1, 0))
  {
    DXGI_SWAP_CHAIN_DESC desc;
    This->GetDesc (&desc);

    ds3_state.SwapChain = This;
    ds3_state.Window    = desc.OutputWindow;

    if (ds3_cfg.window.borderless || (! ds3_state.Fullscreen))
    {
      SK_DS3_FinishResize ();
      SK_DS3_CenterWindow ();
    }

    //
    // Engage Fullscreen Mode At Startup (ARC Hack)
    //
    if (ds3_cfg.render.fullscreen)
    {
      CreateThread ( nullptr,
                         0,
                           SK_DS3_FullscreenToggle_Thread,
                             nullptr,
                               0x00,
                                 nullptr );
    }

    // Since people don't read guides, nag them to death...
    if (ds3_cfg.osd.disclaimer)
    {
      CreateThread ( nullptr,                 0,
                       SK_DS3_OSD_Disclaimer, nullptr,
                         0x00,                nullptr );
    }

    //DXGISwap_ResizeBuffers_Original (This, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

    if (! SK_IsInjected ())
      SK_DS3_CheckVersion (nullptr);
  }

  return S_OK;
}

bool
SK_DS3_IsBorderless (void)
{
  return ds3_cfg.window.borderless;
}

bool
WINAPI
SK_DS3_ShutdownPlugin (const wchar_t* backend)
{
  UNREFERENCED_PARAMETER (backend);


  // Allow the graphics config file to be written again at shutdown...
  DWORD    dwConfigAttribs;
  uint32_t dwLen =                MAX_PATH;
  wchar_t  wszGraphicsConfigPath [MAX_PATH + 1];

  SK_GetUserProfileDir (wszGraphicsConfigPath, &dwLen);

  wcscat ( wszGraphicsConfigPath,
             L"\\AppData\\Roaming\\DarkSoulsIII\\GraphicsConfig.xml" );

  dwConfigAttribs = GetFileAttributesW (wszGraphicsConfigPath);
                      SetFileAttributesW ( wszGraphicsConfigPath,
                                             dwConfigAttribs &
                                             (~FILE_ATTRIBUTE_READONLY) );


  return true;
}