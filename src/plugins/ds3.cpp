#include <string>

#include <Windows.h>
#include <SpecialK/ini.h>
#include <SpecialK/parameter.h>
#include <SpecialK/utility.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/hooks.h>
#include <SpecialK/core.h>

#include <process.h>

//
// Hook Special K's shutdown function
//
typedef bool (WINAPI *ShutdownPlugin_pfn)(const wchar_t *);
               static ShutdownPlugin_pfn SK_ShutdownCore_Original = nullptr;

extern bool WINAPI SK_DS3_ShutdownPlugin (const wchar_t *);

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
static SetWindowPos_pfn         SetWindowPos_Original         = nullptr;
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

static D3D11_RSSetViewports_pfn   D3D11_RSSetViewports_Original          = nullptr;

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



sk::ParameterFactory  ds3_factory;

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

extern HWND hWndRender;

struct ds3_state_s {
  IDXGISwapChain* SwapChain  = nullptr;

  bool            Fullscreen = false;
  HWND&           Window     = hWndRender;

  int             Width      = 0;
  int             Height     = 0;

  struct monitor_s {
    int           Width     = 0;
    int           Height    = 0;
  } monitor;
} ds3_state;


int* __DS3_WIDTH  = nullptr;
int* __DS3_HEIGHT = nullptr;


struct sus_state_s {
  bool Center     = false;
  bool MaxWindow  = false;
} sus_state;


struct {
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
} ds3_cfg;


#include <SpecialK/core.h>

extern void
__stdcall
SK_SetPluginName (std::wstring name);

#define SUS_VERSION_NUM L"0.5.0.1"
#define SUS_VERSION_STR L"Souls Unsqueezed v " SUS_VERSION_NUM

// Block until update finishes, otherwise the update dialog
//   will be dismissed as the game crashes when it tries to
//     draw the first frame.
volatile LONG __SUS_init = FALSE;

unsigned int
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

  //
  // VMProtect kills this, so let's do something else...
  //
#ifdef VMPROTECT_IS_NOT_A_FACTOR
  IMAGE_DOS_HEADER* pDOS =
    (IMAGE_DOS_HEADER *)mem_info.AllocationBase;
  IMAGE_NT_HEADERS* pNT  =
    (IMAGE_NT_HEADERS *)((uintptr_t)(pDOS + pDOS->e_lfanew));

  uint8_t* end_addr = base_addr + pNT->OptionalHeader.SizeOfImage;
#else
           base_addr = (uint8_t *)mem_info.BaseAddress;//AllocationBase;
  uint8_t* end_addr  = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;

  if (base_addr != (uint8_t *)0x400000) {
    dll_log.Log ( L"[ Sig Scan ] Expected module base addr. 40000h, but got: %ph",
                    base_addr );
  }

  size_t pages = 0;

// Scan up to 256 MiB worth of data
#ifndef __WIN64
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
  while (VirtualQuery (end_addr, &mem_info, sizeof mem_info) && end_addr < PAGE_WALK_LIMIT) {
    //if (mem_info.Protect & PAGE_NOACCESS || (! (mem_info.Type & MEM_IMAGE)))
      //break;

    pages += VirtualQuery (end_addr, &mem_info, sizeof mem_info);

    end_addr = (uint8_t *)mem_info.BaseAddress + mem_info.RegionSize;
  } 

  if (end_addr > PAGE_WALK_LIMIT) {
    dll_log.Log ( L"[ Sig Scan ] Module page walk resulted in end addr. out-of-range: %ph",
                    end_addr );
    dll_log.Log ( L"[ Sig Scan ]  >> Restricting to %ph",
                    PAGE_WALK_LIMIT );
    end_addr = (uint8_t *)PAGE_WALK_LIMIT;
  }

  dll_log.Log ( L"[ Sig Scan ] Module image consists of %zu pages, from %ph to %ph",
                  pages,
                    base_addr,
                      end_addr );
#endif

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

unsigned int
__stdcall
SK_DS3_CenterWindow_Thread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  SK_DS3_GetMonitorDims ();

  if (! sus_state.Center)
    return 0;

  dll_log.Log ( L"[SUS PlugIn] [!] SK_DS3_CenterWindow (void) -- [Calling Thread: 0x%04x]",
                  GetCurrentThreadId () );
  dll_log.Log ( L"[SUS PlugIn] \tMonitor: [%lux%lu] <-> Window: [%lux%lu] :: { %s }, <HWND: 0x%04X>",
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

unsigned int
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
  _beginthreadex ( nullptr,
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
  _beginthreadex ( nullptr,
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
    return SetWindowPos_Original (
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



void
SK_DisableDPIScaling (void)
{
  DWORD   dwProcessSize = MAX_PATH;
  wchar_t wszProcessName [MAX_PATH];

  HANDLE hProc =
   GetCurrentProcess ();

  QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

  DWORD dwDisposition = 0x00;
  HKEY  key           = nullptr;

  wchar_t wszKey [1024];
  lstrcpyW (wszKey, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");

  LSTATUS status =
    RegCreateKeyExW ( HKEY_CURRENT_USER,
                        wszKey,
                          0, NULL, 0x00L,
                            KEY_READ | KEY_WRITE,
                               nullptr, &key, &dwDisposition );

  if (status == ERROR_SUCCESS && key != nullptr) {
    const wchar_t* wszKillDPI = L"~ HIGHDPIAWARE";

    RegSetValueExW (key, wszProcessName,  0, REG_SZ, (BYTE *)wszKillDPI, sizeof (wchar_t) * (lstrlenW (wszKillDPI) + 1));

    RegFlushKey (key);
    RegCloseKey (key);
  }
}

void
SK_DS3_InitPlugin (void)
{
  if (! SK_IsInjected ())
    SK_DS3_CheckVersion (nullptr);

  SK_DisableDPIScaling ();

  __DS3_WIDTH  = &ds3_state.Width;
  __DS3_HEIGHT = &ds3_state.Height;

  ds3_state.Width  = ds3_cfg.render.res_x;
  ds3_state.Height = ds3_cfg.render.res_y;

  if (ds3_prefs == nullptr)
  {
    // Make the graphics config file read-only while running
    DWORD    dwConfigAttribs;
    uint32_t dwLen = MAX_PATH;
    wchar_t  wszGraphicsConfigPath [MAX_PATH];

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
      static_cast <sk::ParameterInt *>
        (ds3_factory.create_parameter <int> (L"HUDResX"));
  ds3_hud_res_x->register_to_ini ( ds3_prefs,
                                    L"SUS.Display",
                                      L"HUDResX" );

  if (ds3_hud_res_x->load ())
    ds3_cfg.hud.res_x = ds3_hud_res_x->get_value ();

  ds3_hud_res_y = 
      static_cast <sk::ParameterInt *>
        (ds3_factory.create_parameter <int> (L"HUDResY"));
  ds3_hud_res_y->register_to_ini ( ds3_prefs,
                                    L"SUS.Display",
                                      L"HUDResY" );

  if (ds3_hud_res_y->load ())
    ds3_cfg.hud.res_y = ds3_hud_res_y->get_value ();

  ds3_hud_offset_x = 
      static_cast <sk::ParameterInt *>
        (ds3_factory.create_parameter <int> (L"HUDOffsetX"));
  ds3_hud_offset_x->register_to_ini ( ds3_prefs,
                                        L"SUS.Display",
                                          L"HUDOffsetX" );

  if (ds3_hud_offset_x->load ())
    ds3_cfg.hud.offset_x = ds3_hud_offset_x->get_value ();

  ds3_hud_offset_y = 
      static_cast <sk::ParameterInt *>
        (ds3_factory.create_parameter <int> (L"HUDOffsetY"));
  ds3_hud_offset_y->register_to_ini ( ds3_prefs,
                                        L"SUS.Display",
                                          L"HUDOffsetY" );

  if (ds3_hud_offset_y->load ())
    ds3_cfg.hud.offset_y = ds3_hud_offset_y->get_value ();

  ds3_hud_stretch = 
      static_cast <sk::ParameterBool *>
        (ds3_factory.create_parameter <bool> (L"StretchHUD"));
  ds3_hud_stretch->register_to_ini ( ds3_prefs,
                                        L"SUS.Display",
                                          L"StretchHUD" );

  if (ds3_hud_stretch->load ())
    ds3_cfg.hud.stretch = ds3_hud_stretch->get_value ();


  ds3_flip_mode =
    static_cast <sk::ParameterBool *>
      (ds3_factory.create_parameter <bool> (L"FlipMode"));
  ds3_flip_mode->register_to_ini ( ds3_prefs,
                                     L"SUS.Render",
                                       L"FlipMode" );

  if (ds3_flip_mode->load ())
    ds3_cfg.render.flip_mode = ds3_flip_mode->get_value ();

  ds3_start_fullscreen =
    static_cast <sk::ParameterBool *>
      (ds3_factory.create_parameter <bool> (L"Start in Fullscreen"));
  ds3_start_fullscreen->register_to_ini ( ds3_prefs,
                                            L"SUS.Render",
                                              L"StartFullscreen" );

  if (ds3_start_fullscreen->load ())
    ds3_cfg.render.fullscreen = ds3_start_fullscreen->get_value ();


  ds3_borderless =
    static_cast <sk::ParameterBool *>
      (ds3_factory.create_parameter <bool> (L"Borderless"));
  ds3_borderless->register_to_ini ( ds3_prefs,
                                      L"SUS.Window",
                                        L"Borderless" );

  if (ds3_borderless->load ())
    ds3_cfg.window.borderless = ds3_borderless->get_value ();


  ds3_fullscreen =
    static_cast <sk::ParameterBool *>
      (ds3_factory.create_parameter <bool> (L"Forceful Fullscreen Windows"));
  ds3_fullscreen->register_to_ini ( ds3_prefs,
                                      L"SUS.Window",
                                        L"Fullscreen" );

  if (ds3_fullscreen->load ()) {
    ds3_cfg.window.fullscreen = ds3_fullscreen->get_value ();

    sus_state.MaxWindow = ds3_cfg.window.fullscreen;
  }


  ds3_center =
    static_cast <sk::ParameterBool *>
      (ds3_factory.create_parameter <bool> (L"Center Windows"));
  ds3_center->register_to_ini ( ds3_prefs,
                                  L"SUS.Window",
                                    L"Center" );

  if (ds3_center->load ()) {
    ds3_cfg.window.center = ds3_center->get_value ();

    sus_state.Center = ds3_cfg.window.center;
  }



  ds3_last_addr =
    static_cast <sk::ParameterInt64 *>
      (ds3_factory.create_parameter <int64_t> (L"Last Known Address"));
  ds3_last_addr->register_to_ini ( ds3_prefs,
                                     L"SUS.System",
                                       L"LastKnownAddr" );


  ds3_osd_disclaimer =
    static_cast <sk::ParameterBool *>
      (ds3_factory.create_parameter <bool> (L"Show OSD Disclaimer"));
  ds3_osd_disclaimer->register_to_ini ( ds3_prefs,
                                          L"SUS.System",
                                            L"ShowOSDDisclaimer" );

  if (ds3_osd_disclaimer->load ())
    ds3_cfg.osd.disclaimer = ds3_osd_disclaimer->get_value ();
  else
  {
    ds3_cfg.osd.disclaimer = true;

    ds3_osd_disclaimer->set_value (true);
    ds3_osd_disclaimer->store     ();

    ds3_prefs->write (ds3_prefs_file);
  }

#if 0
  sk::ParameterStringW ini_ver =
    static_cast <sk::ParameterStringW *>
      (ds3_factory.create_parameter <std::wstring> (L"Last Version"));
  ds3_last_addr->register_to_ini ( ds3_prefs,
                                     L"SUS.System",
                                       L"Version" );
#endif

  ds3_default_res_x =
    static_cast <sk::ParameterInt *>
      (ds3_factory.create_parameter <int> (L"Base (Windowed) Resolution"));
  ds3_default_res_x->register_to_ini ( ds3_prefs,
                                         L"SUS.Render",
                                           L"DefaultResX" );

  ds3_default_res_x->load ();

  ds3_default_res_y =
    static_cast <sk::ParameterInt *>
      (ds3_factory.create_parameter <int> (L"Base (Windowed) Resolution"));
  ds3_default_res_y->register_to_ini ( ds3_prefs,
                                         L"SUS.Render",
                                           L"DefaultResY" );

  ds3_default_res_y->load ();


  ds3_sacrificial_x =
    static_cast <sk::ParameterInt *>
    (ds3_factory.create_parameter <int> (L"Sacrificial (Windowed) Resolution"));
  ds3_sacrificial_x->register_to_ini ( ds3_prefs,
                                         L"SUS.Render",
                                           L"SacrificialResX" );

  if (ds3_sacrificial_x->load ())
    ds3_cfg.render.sacrifice_x = ds3_sacrificial_x->get_value ();

  ds3_sacrificial_y =
    static_cast <sk::ParameterInt *>
    (ds3_factory.create_parameter <int> (L"Sacrificial (Windowed) Resolution"));
  ds3_sacrificial_y->register_to_ini ( ds3_prefs,
                                         L"SUS.Render",
                                           L"SacrificialResY" );

  if (ds3_sacrificial_y->load ())
    ds3_cfg.render.sacrifice_y = ds3_sacrificial_y->get_value ();


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

  if (ds3_last_addr->load ()) {
    res_addr = (void *)ds3_last_addr->get_value ();

    MEMORY_BASIC_INFORMATION mem_info;
    VirtualQuery (res_addr, &mem_info, sizeof mem_info);

    if (mem_info.Protect & PAGE_NOACCESS)
      res_addr = nullptr;

    if (res_addr != nullptr && (! memcmp (res_addr, res_sig, 8))) {
      dll_log.Log ( L"[Asp. Ratio] Skipping Signature Scan,"
                    L" Last Known Address = GOOD" );
    } else {
      res_addr = nullptr;
    }
  }

  if (res_addr == nullptr)
    res_addr = SK_DS3_Scan (res_sig, 8, nullptr);

  if (res_addr != nullptr) {
    ds3_last_addr->set_value ((int64_t)res_addr);
    ds3_last_addr->store     ();

    ds3_prefs->write (ds3_prefs_file);
  }

  void* res_addr_x = res_addr;
  void* res_addr_y = (uint8_t *)res_addr + 4;

  if (res_addr != nullptr) {
    if (res_x != ds3_cfg.render.sacrifice_x || res_y != ds3_cfg.render.sacrifice_y) {
      SK_InjectMemory (res_addr_x, (uint8_t *)&res_x, 4, PAGE_EXECUTE_READWRITE);
      SK_InjectMemory (res_addr_y, (uint8_t *)&res_y, 4, PAGE_EXECUTE_READWRITE);
      dll_log.Log ( L"[Asp. Ratio] Custom Default Resolution: (%lux%lu) {%3.2f}",
                      res_x, res_y,
                        (float)res_x / (float)res_y );
      dll_log.Log ( L"[Asp. Ratio]   >> Sacrifice Address: %ph", res_addr);
    }
  } else {
    dll_log.Log ( L"[Asp. Ratio] >> ERROR: Unable to locate memory address for %lux%lu... <<",
                  *(uint32_t *)res_sig, *((uint32_t *)res_sig+1) );
  }


  SK_CreateDLLHook2 (      L"user32.dll",
                            "SetActiveWindow",
                             SK_DS3_SetActiveWindow,
    static_cast_p2p <void> (&SetActiveWindow_Original) );

  SK_CreateFuncHook ( L"ID3D11DeviceContext::RSSetViewports",
                                       D3D11_RSSetViewports_Override,
                                      SK_DS3_RSSetViewports,
              static_cast_p2p <void> (&D3D11_RSSetViewports_Original) );
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


  LPVOID lpvPluginKeyPress = nullptr;

  SK_CreateFuncHook (      L"SK_PluginKeyPress",
                             SK_PluginKeyPress,
                         SK_DS3_PluginKeyPress,
    static_cast_p2p <void> (&lpvPluginKeyPress) );
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

  InterlockedExchange (&__SUS_init, 1);
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
    //ds3_state.Window = swap_desc.OutputWindow;

    // Reset the temporary monitor mode change we may have made earlier
    if ((! ds3_state.Fullscreen) && ds3_cfg.window.borderless && sus_state.MaxWindow)
      ChangeDisplaySettings (0, CDS_RESET);

    int num_monitors = GetSystemMetrics (SM_CMONITORS);
    int virtual_x    = GetSystemMetrics (SM_CXVIRTUALSCREEN);
    int virtual_y    = GetSystemMetrics (SM_CYVIRTUALSCREEN);

    dll_log.Log ( L"[ Monitors ]  + Display Topology: %lu Monitors - "
                                      L"(Virtual Res: %lux%lu)",
                    num_monitors, virtual_x, virtual_y );

    if (ds3_cfg.window.borderless && sus_state.MaxWindow)
    {
      DEVMODE devmode = { };
      devmode.dmSize  = sizeof DEVMODE;

      EnumDisplaySettings (nullptr, ENUM_CURRENT_SETTINGS, &devmode);

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

          ChangeDisplaySettings (&devmode, CDS_FULLSCREEN);

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

  D3D11_RSSetViewports_Original (This, NumViewports, pNewViewports);

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
}

unsigned int
__stdcall
SK_DS3_FullscreenToggle_Thread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  // Don't do any of this stuff if we cannot bring the window into foucs
  if ( ! (BringWindowToTop    (ds3_state.Window) &&
          SetForegroundWindow (ds3_state.Window)) )
    return std::numeric_limits <unsigned int>::max ();

  Sleep (66);

  SetFocus (ds3_state.Window);

  INPUT keys [2];

  keys [0].type           = INPUT_KEYBOARD;
  keys [0].ki.wVk         = 0;
  keys [0].ki.dwFlags     = KEYEVENTF_SCANCODE;
  keys [0].ki.time        = 0;
  keys [0].ki.dwExtraInfo = 0;

  keys [1].type           = INPUT_KEYBOARD;
  keys [1].ki.wVk         = 0;
  keys [1].ki.dwFlags     = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
  keys [1].ki.time        = 0;
  keys [1].ki.dwExtraInfo = 0;

  keys [0].ki.wScan = 0x38;
  SendInput (1, &keys [0], sizeof INPUT);

  keys [0].ki.wScan = 0x1c;
  SendInput (1, &keys [0], sizeof INPUT);

  Sleep (133);

  keys [1].ki.wScan = 0x1c;
  SendInput (1, &keys [1], sizeof INPUT);

  keys [1].ki.wScan = 0x38;
  SendInput (1, &keys [1], sizeof INPUT);

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
    Sleep (66);

  ds3_osd_disclaimer->set_value (false);
  ds3_osd_disclaimer->store     ();

  ds3_prefs->write              (ds3_prefs_file);

  CloseHandle (GetCurrentThread ());

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

  // Wait for the mod to init, it may be held up during version check
  while (! InterlockedAdd (&__SUS_init, 0)) Sleep (16);

  static bool first = true;

  DXGI_SWAP_CHAIN_DESC desc;
  This->GetDesc (&desc);

  ds3_state.SwapChain = This;
  ds3_state.Window    = desc.OutputWindow;

  if (ds3_cfg.window.borderless || (! ds3_state.Fullscreen))
  {
    SK_DS3_FinishResize ();
    SK_DS3_CenterWindow ();
  }

  if (first)
  {
    first = false;

    //
    // Engage Fullscreen Mode At Startup (ARC Hack)
    //
    if (ds3_cfg.render.fullscreen)
    {
      _beginthreadex ( nullptr,
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
  }

  //DXGISwap_ResizeBuffers_Original (This, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

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
  wchar_t  wszGraphicsConfigPath [MAX_PATH];

  SK_GetUserProfileDir (wszGraphicsConfigPath, &dwLen);

  wcscat ( wszGraphicsConfigPath,
             L"\\AppData\\Roaming\\DarkSoulsIII\\GraphicsConfig.xml" );

  dwConfigAttribs = GetFileAttributesW (wszGraphicsConfigPath);
                      SetFileAttributesW ( wszGraphicsConfigPath,
                                             dwConfigAttribs &
                                             (~FILE_ATTRIBUTE_READONLY) );


  return true;
}