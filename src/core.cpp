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
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <SpecialK/core.h>
#include <SpecialK/stdafx.h>
#include <SpecialK/hooks.h>
#include <SpecialK/window.h>

#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/diagnostics/crash_handler.h>
#include <SpecialK/diagnostics/debug_utils.h>

#include <SpecialK/log.h>
#include <SpecialK/utility.h>
#include <SpecialK/sound.h>

#include <SpecialK/tls.h>

#include <LibLoaderAPI.h>

#pragma warning   (push)
#pragma warning   (disable: 4091)
#  include <DbgHelp.h>
#  pragma comment (lib, "dbghelp.lib")
#pragma warning   (pop)

#include <SpecialK/config.h>
#include <SpecialK/osd/text.h>
#include <SpecialK/io_monitor.h>
#include <SpecialK/import.h>
#include <SpecialK/console.h>
#include <SpecialK/command.h>
#include <SpecialK/framerate.h>
#include <SpecialK/render_backend.h>

#include <SpecialK/adl.h>

#include <SpecialK/steam_api.h>

#include <SpecialK/dxgi_backend.h>

#include <atlbase.h>
#include <comdef.h>

#include <delayimp.h>

#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>
#include <CEGUI/Logger.h>


static const GUID IID_ID3D11Device1 = { 0xa04bfb29, 0x08ef, 0x43d6, { 0xa4, 0x9c, 0xa9, 0xbd, 0xbd, 0xcb, 0xe6, 0x86 } };
static const GUID IID_ID3D11Device2 = { 0x9d06dffa, 0xd1e5, 0x4d07, { 0x83, 0xa8, 0x1b, 0xb1, 0x23, 0xf2, 0xf8, 0x41 } };
static const GUID IID_ID3D11Device3 = { 0xa05c8c37, 0xd2c6, 0x4732, { 0xb3, 0xa0, 0x9c, 0xe0, 0xb0, 0xdc, 0x9a, 0xe6 } };
static const GUID IID_ID3D11Device4 = { 0x8992ab71, 0x02e6, 0x4b8d, { 0xba, 0x48, 0xb0, 0x56, 0xdc, 0xda, 0x42, 0xc4 } };


extern CRITICAL_SECTION init_mutex;
extern CRITICAL_SECTION budget_mutex;
extern CRITICAL_SECTION loader_lock;
extern CRITICAL_SECTION wmi_cs;
extern CRITICAL_SECTION cs_dbghelp;

volatile HANDLE  hInitThread   = { 0 };
         HANDLE  hPumpThread   = { 0 };


// Disable SLI memory in Batman Arkham Knight
bool USE_SLI = true;


extern "C++" void SK_DS3_InitPlugin (void);


NV_GET_CURRENT_SLI_STATE sli_state;
BOOL                     nvapi_init = FALSE;
int                      gpu_prio;

HMODULE                  backend_dll  = 0;

// LOL -- This tests for RunDLL32 / SKIM
bool
__cdecl
SK_IsSuperSpecialK (void);

void
SK_PathRemoveExtension (wchar_t* wszInOut)
{
  wchar_t *wszEnd = wszInOut,
          *wszPrev;

  while (*CharNextW (wszEnd) != L'\0')
    wszEnd = CharNextW (wszEnd);

  wszPrev = wszEnd;

  while (  CharPrevW (wszInOut, wszPrev) > wszInOut &&
          *CharPrevW (wszInOut, wszPrev) != L'.' )
    wszPrev = CharPrevW (wszInOut, wszPrev);

  if (CharPrevW (wszInOut, wszPrev) > wszInOut) {
    if (*CharPrevW (wszInOut, wszPrev) == L'.')
      *CharPrevW (wszInOut, wszPrev) = L'\0';
  }
}

wchar_t SK_RootPath   [MAX_PATH + 2] = { L'\0' };
wchar_t SK_ConfigPath [MAX_PATH + 2] = { L'\0' };
wchar_t SK_Backend    [128];

__declspec (noinline)
const wchar_t*
__stdcall
SK_GetBackend (void)
{
  return SK_Backend;
}

__declspec (noinline)
const wchar_t*
__stdcall
SK_GetRootPath (void)
{
  return SK_RootPath;
}

//
// To be used internally only, by the time any plug-in has
//   been activated, Special K will have already established
//     this path and loaded its config.
//
void
__stdcall
SK_SetConfigPath (const wchar_t* path)
{
  lstrcpyW (SK_ConfigPath, path);
}

__declspec (noinline)
const wchar_t*
__stdcall
SK_GetConfigPath (void)
{
  return SK_ConfigPath;
}

volatile
ULONG frames_drawn = 0UL;

__declspec (noinline)
ULONG
__stdcall
SK_GetFramesDrawn (void)
{
  return InterlockedExchangeAdd (&frames_drawn, 0);
}

#define D3D12_IGNORE_SDK_LAYERS

#include <d3d9.h>
#include <d3d11.h>
#include <SpecialK/d3d12_interfaces.h>

const wchar_t*
__stdcall
SK_DescribeHRESULT (HRESULT result)
{
  switch (result)
  {
    /* Generic (SUCCEEDED) */

  case S_OK:
    return L"S_OK";

  case S_FALSE:
    return L"S_FALSE";


#ifndef SK_BUILD__INSTALLER
    /* DXGI */

  case DXGI_ERROR_DEVICE_HUNG:
    return L"DXGI_ERROR_DEVICE_HUNG";

  case DXGI_ERROR_DEVICE_REMOVED:
    return L"DXGI_ERROR_DEVICE_REMOVED";

  case DXGI_ERROR_DEVICE_RESET:
    return L"DXGI_ERROR_DEVICE_RESET";

  case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
    return L"DXGI_ERROR_DRIVER_INTERNAL_ERROR";

  case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
    return L"DXGI_ERROR_FRAME_STATISTICS_DISJOINT";

  case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
    return L"DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";

  case DXGI_ERROR_INVALID_CALL:
    return L"DXGI_ERROR_INVALID_CALL";

  case DXGI_ERROR_MORE_DATA:
    return L"DXGI_ERROR_MORE_DATA";

  case DXGI_ERROR_NONEXCLUSIVE:
    return L"DXGI_ERROR_NONEXCLUSIVE";

  case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
    return L"DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";

  case DXGI_ERROR_NOT_FOUND:
    return L"DXGI_ERROR_NOT_FOUND";

  case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
    return L"DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";

  case DXGI_ERROR_REMOTE_OUTOFMEMORY:
    return L"DXGI_ERROR_REMOTE_OUTOFMEMORY";

  case DXGI_ERROR_WAS_STILL_DRAWING:
    return L"DXGI_ERROR_WAS_STILL_DRAWING";

  case DXGI_ERROR_UNSUPPORTED:
    return L"DXGI_ERROR_UNSUPPORTED";

  case DXGI_ERROR_ACCESS_LOST:
    return L"DXGI_ERROR_ACCESS_LOST";

  case DXGI_ERROR_WAIT_TIMEOUT:
    return L"DXGI_ERROR_WAIT_TIMEOUT";

  case DXGI_ERROR_SESSION_DISCONNECTED:
    return L"DXGI_ERROR_SESSION_DISCONNECTED";

  case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
    return L"DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";

  case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
    return L"DXGI_ERROR_CANNOT_PROTECT_CONTENT";

  case DXGI_ERROR_ACCESS_DENIED:
    return L"DXGI_ERROR_ACCESS_DENIED";

  case DXGI_ERROR_NAME_ALREADY_EXISTS:
    return L"DXGI_ERROR_NAME_ALREADY_EXISTS";

  case DXGI_ERROR_SDK_COMPONENT_MISSING:
    return L"DXGI_ERROR_SDK_COMPONENT_MISSING";

  case DXGI_DDI_ERR_WASSTILLDRAWING:
    return L"DXGI_DDI_ERR_WASSTILLDRAWING";

  case DXGI_DDI_ERR_UNSUPPORTED:
    return L"DXGI_DDI_ERR_UNSUPPORTED";

  case DXGI_DDI_ERR_NONEXCLUSIVE:
    return L"DXGI_DDI_ERR_NONEXCLUSIVE";


    /* DXGI (Status) */
  case DXGI_STATUS_OCCLUDED:
    return L"DXGI_STATUS_OCCLUDED";

  case DXGI_STATUS_UNOCCLUDED:
    return L"DXGI_STATUS_UNOCCLUDED";

  case DXGI_STATUS_CLIPPED:
    return L"DXGI_STATUS_CLIPPED";

  case DXGI_STATUS_NO_REDIRECTION:
    return L"DXGI_STATUS_NO_REDIRECTION";

  case DXGI_STATUS_NO_DESKTOP_ACCESS:
    return L"DXGI_STATUS_NO_DESKTOP_ACCESS";

  case DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE:
    return L"DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE";

  case DXGI_STATUS_DDA_WAS_STILL_DRAWING:
    return L"DXGI_STATUS_DDA_WAS_STILL_DRAWING";

  case DXGI_STATUS_MODE_CHANGED:
    return L"DXGI_STATUS_MODE_CHANGED";

  case DXGI_STATUS_MODE_CHANGE_IN_PROGRESS:
    return L"DXGI_STATUS_MODE_CHANGE_IN_PROGRESS";


    /* D3D11 */

  case D3D11_ERROR_FILE_NOT_FOUND:
    return L"D3D11_ERROR_FILE_NOT_FOUND";

  case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
    return L"D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";

  case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
    return L"D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";

  case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
    return L"D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";


    /* D3D9 */

  case D3DERR_WRONGTEXTUREFORMAT:
    return L"D3DERR_WRONGTEXTUREFORMAT";

  case D3DERR_UNSUPPORTEDCOLOROPERATION:
    return L"D3DERR_UNSUPPORTEDCOLOROPERATION";

  case D3DERR_UNSUPPORTEDCOLORARG:
    return L"D3DERR_UNSUPPORTEDCOLORARG";

  case D3DERR_UNSUPPORTEDALPHAOPERATION:
    return L"D3DERR_UNSUPPORTEDALPHAOPERATION";

  case D3DERR_UNSUPPORTEDALPHAARG:
    return L"D3DERR_UNSUPPORTEDALPHAARG";

  case D3DERR_TOOMANYOPERATIONS:
    return L"D3DERR_TOOMANYOPERATIONS";

  case D3DERR_CONFLICTINGTEXTUREFILTER:
    return L"D3DERR_CONFLICTINGTEXTUREFILTER";

  case D3DERR_UNSUPPORTEDFACTORVALUE:
    return L"D3DERR_UNSUPPORTEDFACTORVALUE";

  case D3DERR_CONFLICTINGRENDERSTATE:
    return L"D3DERR_CONFLICTINGRENDERSTATE";

  case D3DERR_UNSUPPORTEDTEXTUREFILTER:
    return L"D3DERR_UNSUPPORTEDTEXTUREFILTER";

  case D3DERR_CONFLICTINGTEXTUREPALETTE:
    return L"D3DERR_CONFLICTINGTEXTUREPALETTE";

  case D3DERR_DRIVERINTERNALERROR:
    return L"D3DERR_DRIVERINTERNALERROR";


  case D3DERR_NOTFOUND:
    return L"D3DERR_NOTFOUND";

  case D3DERR_MOREDATA:
    return L"D3DERR_MOREDATA";

  case D3DERR_DEVICELOST:
    return L"D3DERR_DEVICELOST";

  case D3DERR_DEVICENOTRESET:
    return L"D3DERR_DEVICENOTRESET";

  case D3DERR_NOTAVAILABLE:
    return L"D3DERR_NOTAVAILABLE";

  case D3DERR_OUTOFVIDEOMEMORY:
    return L"D3DERR_OUTOFVIDEOMEMORY";

  case D3DERR_INVALIDDEVICE:
    return L"D3DERR_INVALIDDEVICE";

  case D3DERR_INVALIDCALL:
    return L"D3DERR_INVALIDCALL";

  case D3DERR_DRIVERINVALIDCALL:
    return L"D3DERR_DRIVERINVALIDCALL";

  case D3DERR_WASSTILLDRAWING:
    return L"D3DERR_WASSTILLDRAWING";


  case D3DOK_NOAUTOGEN:
    return L"D3DOK_NOAUTOGEN";


    /* D3D12 */

    //case D3D12_ERROR_FILE_NOT_FOUND:
    //return L"D3D12_ERROR_FILE_NOT_FOUND";

    //case D3D12_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
    //return L"D3D12_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";

    //case D3D12_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
    //return L"D3D12_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
#endif


    /* Generic (FAILED) */

  case E_FAIL:
    return L"E_FAIL";

  case E_INVALIDARG:
    return L"E_INVALIDARG";

  case E_OUTOFMEMORY:
    return L"E_OUTOFMEMORY";

  case E_POINTER:
    return L"E_POINTER";

  case E_ACCESSDENIED:
    return L"E_ACCESSDENIED";

  case E_HANDLE:
    return L"E_HANDLE";

  case E_NOTIMPL:
    return L"E_NOTIMPL";

  case E_NOINTERFACE:
    return L"E_NOINTERFACE";

  case E_ABORT:
    return L"E_ABORT";

  case E_UNEXPECTED:
    return L"E_UNEXPECTED";

  default:
    dll_log.Log (L" *** Encountered unknown HRESULT: (0x%08X)",
      (unsigned long)result);
    return L"UNKNOWN";
  }
}

// Stupid solution for games that inexplicibly draw to the screen
//   without ever swapping buffers.
unsigned int
__stdcall
osd_pump (LPVOID lpThreadParam)
{ 
  UNREFERENCED_PARAMETER (lpThreadParam);

  // TODO: This actually increases the number of frames rendered, which
  //         may interfere with other mod logic... the entire feature is
  //           a hack, but that behavior is not intended.
  while (true) {
    Sleep            ((DWORD)(config.osd.pump_interval * 1000.0f));
    SK_EndBufferSwap (S_OK, nullptr);
  }

  return 0;
}

extern void SK_ShutdownWMI (void);

void
__stdcall
SK_StartPerfMonThreads (void)
{
  if (config.mem.show) {
    //
    // Spawn Process Monitor Thread
    //
    if (process_stats.hThread == 0) {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Process Monitor...  ");

      process_stats.hThread = 
        (HANDLE)
          _beginthreadex ( nullptr,
                             0,
                               SK_MonitorProcess,
                                 nullptr,
                                   0,
                                     nullptr );

      if (process_stats.hThread != 0)
        dll_log.LogEx (false, L"tid=0x%04x\n", GetThreadId (process_stats.hThread));
      else
        dll_log.LogEx (false, L"Failed!\n");
    }
  }

  if (config.cpu.show) {
    //
    // Spawn CPU Refresh Thread
    //
    if (cpu_stats.hThread == 0) {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning CPU Monitor...      ");

      cpu_stats.hThread = 
        (HANDLE)
          _beginthreadex ( nullptr,
                             0,
                               SK_MonitorCPU,
                                 nullptr,
                                   0,
                                     nullptr );

      if (cpu_stats.hThread != 0)
        dll_log.LogEx (false, L"tid=0x%04x\n", GetThreadId (cpu_stats.hThread));
      else
        dll_log.LogEx (false, L"Failed!\n");
    }
  }

  if (config.disk.show) {
    if (disk_stats.hThread == 0) {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Disk Monitor...     ");

      disk_stats.hThread =
        (HANDLE)
          _beginthreadex ( nullptr,
                             0,
                               SK_MonitorDisk,
                                 nullptr,
                                   0,
                                     nullptr );

      if (disk_stats.hThread != 0)
        dll_log.LogEx (false, L"tid=0x%04x\n", GetThreadId (disk_stats.hThread));
      else
        dll_log.LogEx (false, L"failed!\n");
    }
  }

  if (config.pagefile.show) {
    if (pagefile_stats.hThread == 0) {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Pagefile Monitor... ");

      pagefile_stats.hThread =
        (HANDLE)
          _beginthreadex ( nullptr,
                             0,
                               SK_MonitorPagefile,
                                 nullptr,
                                   0,
                                     nullptr );

      if (pagefile_stats.hThread != 0)
        dll_log.LogEx ( false, L"tid=0x%04x\n",
                          GetThreadId (pagefile_stats.hThread) );
      else
        dll_log.LogEx (false, L"failed!\n");
    }
  }
}

void
__stdcall
SK_InitFinishCallback (void)
{
  dll_log.Log (L"[ SpecialK ] === Initialization Finished! ===");

  LeaveCriticalSection (&init_mutex);

  SK_Console* pConsole = SK_Console::getInstance ();
  pConsole->Start ();

  extern BOOL SK_UsingVulkan (void);

    // Create a thread that pumps the OSD
  if (config.osd.pump || SK_UsingVulkan ()) {
    dll_log.LogEx (true, L"[ Stat OSD ] Spawning Pump Thread...      ");
    hPumpThread =
      (HANDLE)
        _beginthreadex ( nullptr,
                           0,
                             osd_pump,
                               nullptr,
                                 0,
                                   nullptr );

    if (hPumpThread != nullptr)
      dll_log.LogEx ( false, L"tid=0x%04x, interval=%04.01f ms\n",
                        GetThreadId (hPumpThread),
                          config.osd.pump_interval * 1000.0f );
    else
      dll_log.LogEx (false, L"failed!\n");
  }

  SK_StartPerfMonThreads ();
}

void
__stdcall
SK_InitCore (const wchar_t* backend, void* callback)
{
  EnterCriticalSection (&init_mutex);

  wcscpy (SK_Backend, backend);

  typedef void (WINAPI *finish_pfn)  (void);
  typedef void (WINAPI *callback_pfn)(_Releases_exclusive_lock_ (init_mutex) finish_pfn);
  callback_pfn callback_fn = (callback_pfn)callback;

  if (backend_dll != NULL) {
    LeaveCriticalSection (&init_mutex);
    return;
  }

  if (config.system.central_repository) {
    // Create Symlink for end-user's convenience
    if ( GetFileAttributes ( ( std::wstring (SK_GetHostPath ()) +
                               std::wstring (L"\\SpecialK")
                             ).c_str ()
                           ) == INVALID_FILE_ATTRIBUTES ) {
      std::wstring link (SK_GetHostPath ());
      link += L"\\SpecialK\\";

      CreateSymbolicLink (
        link.c_str         (),
          SK_GetConfigPath (),
            SYMBOLIC_LINK_FLAG_DIRECTORY
      );
    }
  }

  if (! lstrcmpW (SK_GetHostApp (), L"BatmanAK.exe"))
    USE_SLI = false;

  std::wstring   module_name   = SK_GetModuleName (SK_GetDLL ());
  const wchar_t* wszModuleName = module_name.c_str ();

  dll_log.LogEx (false,
    L"----------------------------------------------------------------------"
    L"---------------------\n");

  // If the module name is this, then we need to load the system-wide DLL...
  wchar_t wszProxyName [MAX_PATH];
  wsprintf (wszProxyName, L"%s.dll", backend);

  wchar_t wszBackendDLL [MAX_PATH] = { L'\0' };
#ifdef _WIN64
  GetSystemDirectory (wszBackendDLL, MAX_PATH);
#else
  HANDLE hProc = GetCurrentProcess ();

  BOOL   bWOW64;
  ::IsWow64Process (hProc, &bWOW64);

  if (bWOW64)
    GetSystemWow64Directory (wszBackendDLL, MAX_PATH);
  else
    GetSystemDirectory (wszBackendDLL, MAX_PATH);
#endif

  wchar_t wszWorkDir [MAX_PATH + 2] = { L'\0' };
  GetCurrentDirectoryW (MAX_PATH, wszWorkDir);

  dll_log.Log (L" Working Directory:          %s", wszWorkDir);
  dll_log.Log (L" System Directory:           %s", wszBackendDLL);

  lstrcatW (wszBackendDLL, L"\\");
  lstrcatW (wszBackendDLL, backend);
  lstrcatW (wszBackendDLL, L".dll");

  const wchar_t* dll_name = wszBackendDLL;

  if (! SK_Path_wcsicmp (wszProxyName, wszModuleName)) {
    dll_name = wszBackendDLL;
  } else {
    dll_name = wszProxyName;
  }

  bool load_proxy = false;

  if (! SK_IsSuperSpecialK ()) {
    if (! SK_IsInjected ()) {
      extern import_t imports [SK_MAX_IMPORTS];

      for (int i = 0; i < SK_MAX_IMPORTS; i++) {
        if (imports [i].role != nullptr && imports [i].role->get_value () == backend) {
          dll_log.LogEx (true, L" Loading proxy %s.dll:    ", backend);
          dll_name   = _wcsdup (imports [i].filename->get_value ().c_str ());
          load_proxy = true;
          break;
        }
      }
    }
  }

  if (! load_proxy)
    dll_log.LogEx (true, L" Loading default %s.dll: ", backend);

  // Pre-Load the original DLL into memory
  if (dll_name != wszBackendDLL) {
                  LoadLibraryW_Original (wszBackendDLL);
    backend_dll = LoadLibraryW_Original (dll_name);
  }

  else
    backend_dll = LoadLibraryW_Original (dll_name);

  if (backend_dll != NULL)
    dll_log.LogEx (false, L" (%s)\n", dll_name);
  else
    dll_log.LogEx (false, L" FAILED (%s)!\n", dll_name);

  // Free the temporary string storage
  if (load_proxy)
    free ((void *)dll_name);

  dll_log.LogEx (false,
    L"----------------------------------------------------------------------"
    L"---------------------\n");

  if (SK_IsSuperSpecialK ()) {
    callback_fn (SK_InitFinishCallback);
    return;
  }

  if (config.system.silent) {
    dll_log.silent = true;

    std::wstring log_fnameW;

    if (! SK_IsInjected ())
      log_fnameW = backend;
    else
      log_fnameW = L"SpecialK";

    log_fnameW += L".log";

    DeleteFile (log_fnameW.c_str ());
  } else {
    dll_log.silent = false;
  }

  dll_log.LogEx (true, L"[  NvAPI   ] Initializing NVIDIA API          (NvAPI): ");

  nvapi_init = sk::NVAPI::InitializeLibrary (SK_GetHostApp ());

  dll_log.LogEx (false, L" %s\n", nvapi_init ? L"Success" : L"Failed");

  if (nvapi_init) {
    int num_sli_gpus = sk::NVAPI::CountSLIGPUs ();

    dll_log.Log (L"[  NvAPI   ] >> NVIDIA Driver Version: %s",
      sk::NVAPI::GetDriverVersion ().c_str ());

    dll_log.Log (L"[  NvAPI   ]  * Number of Installed NVIDIA GPUs: %i "
      L"(%i are in SLI mode)",
      sk::NVAPI::CountPhysicalGPUs (), num_sli_gpus);

    if (num_sli_gpus > 0) {
      DXGI_ADAPTER_DESC* sli_adapters =
        sk::NVAPI::EnumSLIGPUs ();

      int sli_gpu_idx = 0;

      while (*sli_adapters->Description != L'\0') {
        dll_log.Log ( L"[  NvAPI   ]   + SLI GPU %d: %s",
          sli_gpu_idx++,
          (sli_adapters++)->Description );
      }
    }

    //
    // Setup a framerate limiter and (if necessary) restart
    //
    bool restart = (! sk::NVAPI::SetFramerateLimit (0));

    //
    // Install SLI Override Settings
    //
    if (sk::NVAPI::CountSLIGPUs () && config.nvidia.sli.override) {
      if (! sk::NVAPI::SetSLIOverride
              ( SK_GetDLLRole (),
                  config.nvidia.sli.mode.c_str (),
                    config.nvidia.sli.num_gpus.c_str (),
                      config.nvidia.sli.compatibility.c_str ()
              )
         ) {
        restart = true;
      }
    }

    if (restart) {
      dll_log.Log (L"[  NvAPI   ] >> Restarting to apply NVIDIA driver settings <<");

      ShellExecute ( GetDesktopWindow (),
                       L"OPEN",
                         SK_GetHostApp (),
                           NULL,
                             NULL,
                               SW_SHOWDEFAULT );
      exit (0);
    }
  } else {
    dll_log.LogEx (true, L"[DisplayLib] Initializing AMD Display Library (ADL):   ");

    BOOL adl_init = SK_InitADL ();

    dll_log.LogEx (false, L" %s\n", adl_init ? L"Success" : L"Failed");

    if (adl_init) {
      dll_log.Log ( L"[DisplayLib]  * Number of Reported AMD Adapters: %i (%i active)",
                      SK_ADL_CountPhysicalGPUs (),
                        SK_ADL_CountActiveGPUs () );
    }
  }

  HMODULE hMod = GetModuleHandle (SK_GetHostApp ());

  if (hMod != NULL) {
    DWORD* dwOptimus = (DWORD *)GetProcAddress (hMod, "NvOptimusEnablement");

    if (dwOptimus != NULL) {
      dll_log.Log (L"[Hybrid GPU]  NvOptimusEnablement..................: 0x%02X (%s)",
        *dwOptimus,
        (*dwOptimus & 0x1 ? L"Max Perf." :
          L"Don't Care"));
    } else {
      dll_log.Log (L"[Hybrid GPU]  NvOptimusEnablement..................: UNDEFINED");
    }

    DWORD* dwPowerXpress =
      (DWORD *)GetProcAddress (hMod, "AmdPowerXpressRequestHighPerformance");

    if (dwPowerXpress != NULL) {
      dll_log.Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: 0x%02X (%s)",
        *dwPowerXpress,
        (*dwPowerXpress & 0x1 ? L"High Perf." :
          L"Don't Care"));
    }
    else
      dll_log.Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: UNDEFINED");
  }

  // Load user-defined DLLs (Early)
#ifdef _WIN64
  SK_LoadEarlyImports64 ();
#else
  SK_LoadEarlyImports32 ();
#endif

  callback_fn (SK_InitFinishCallback);

  // Setup the compatibility backend, which monitors loaded libraries,
  //   blacklists bad DLLs and detects render APIs...
  SK_EnumLoadedModules (SK_ModuleEnum::PostLoad);

  // Load user-defined DLLs (Plug-In)
#ifdef _WIN64
  SK_LoadPlugIns64 ();
#else
  SK_LoadPlugIns32 ();
#endif
}


volatile  LONG SK_bypass_dialog_active = FALSE;

void
WaitForInit (void)
{
  static volatile ULONG init = FALSE;

  if (InterlockedCompareExchange (&init, FALSE, FALSE)) {
    return;
  }

  while (InterlockedCompareExchangePointer ((LPVOID *)&hInitThread, nullptr, nullptr))
  {
    if ( WAIT_OBJECT_0 == WaitForSingleObject (
      InterlockedCompareExchangePointer ((LPVOID *)&hInitThread, nullptr, nullptr),
        150 ) )
      break;
  }

  while (InterlockedCompareExchange (&SK_bypass_dialog_active, FALSE, FALSE)) {
    dll_log.Log ( L"[ MultiThr ] Injection Bypass Dialog Active (tid=%x)",
                      GetCurrentThreadId () );
    Sleep (150);
  }

  // First thread to reach this point wins ... a shiny new car and
  //   various other initialization tasks.
  //
  // This is important because if this DLL is loaded at application start,
  //   there are potentially other threads queued up waiting to make calls
  //     to this one.
  //
  //   These other threads start out life suspended but Windows will resume
  //     them as soon as all DLL code is loaded. Then it becomes a sloppy race
  //       for each attached thread to finish its DllMain (...) function.
  //
  if (! InterlockedCompareExchange (&init, TRUE, FALSE)) {
    CloseHandle (InterlockedExchangePointer ((LPVOID *)&hInitThread, nullptr));

    // Load user-defined DLLs (Lazy)
#ifdef _WIN64
    SK_LoadLazyImports64 ();
#else
    SK_LoadLazyImports32 ();
#endif

    if (config.system.handle_crashes)
      SK::Diagnostics::CrashHandler::Reinstall ();
  }
}


//
// TODO : Move me somewhere more sensible...
//
class skMemCmd : public SK_ICommand {
public:
  SK_ICommandResult execute (const char* szArgs);

  int         getNumArgs         (void) { return 2; }
  int         getNumOptionalArgs (void) { return 1; }
  int         getNumRequiredArgs (void) {
    return getNumArgs () - getNumOptionalArgs ();
  }

protected:
private:
};

class skUpdateCmd : public SK_ICommand {
public:
  SK_ICommandResult execute (const char* szArgs);

  int         getNumArgs         (void) { return 1; }
  int         getNumOptionalArgs (void) { return 1; }
  int         getNumRequiredArgs (void) {
    return getNumArgs () - getNumOptionalArgs ();
  }

protected:
private:
};

#include <SpecialK/update/version.h>
#include <SpecialK/update/network.h>

SK_ICommandResult
skUpdateCmd::execute (const char* szArgs)
{
  if (! strlen (szArgs)) {
    SK_FetchVersionInfo1 (L"SpecialK", true);
    SK_UpdateSoftware1   (L"SpecialK", true);
  }

  else {
    wchar_t wszProduct [128] = { L'\0' };
    
    mbtowc (wszProduct, szArgs, strlen (szArgs));

    SK_FetchVersionInfo1 (wszProduct, true);
    SK_UpdateSoftware1   (wszProduct, true);
  }

  return SK_ICommandResult ("Manual update initiated...", szArgs);
}

SK_ICommandResult
skMemCmd::execute (const char* szArgs)
{
  if (szArgs == nullptr)
    return SK_ICommandResult ("mem", szArgs);

  uintptr_t addr;
  char      type;
  char      val [256] = { '\0' };

#ifdef _WIN64
  sscanf (szArgs, "%c %llx %s", &type, &addr, val);
#else
  sscanf (szArgs, "%c %lx %s", &type, &addr, val);
#endif

  static uint8_t* base_addr = nullptr;

  if (base_addr == nullptr) {
    base_addr = (uint8_t *)GetModuleHandle (nullptr);

    MEMORY_BASIC_INFORMATION basic_mem_info;
    VirtualQuery (base_addr, &basic_mem_info, sizeof basic_mem_info);

    base_addr = (uint8_t *)basic_mem_info.BaseAddress;
  }

  addr += (uintptr_t)base_addr;

  char result [512];

  switch (type) {
    case 'b':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 1, PAGE_READWRITE, &dwOld);
          uint8_t out;
          sscanf (val, "%hhux", &out);
          *(uint8_t *)addr = out;
        VirtualProtect ((LPVOID)addr, 1, dwOld, &dwOld);
      }

      sprintf (result, "%u", *(uint8_t *)addr);

      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 's':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 2, PAGE_READWRITE, &dwOld);
          uint16_t out;
          sscanf (val, "%hx", &out);
          *(uint16_t *)addr = out;
        VirtualProtect ((LPVOID)addr, 2, dwOld, &dwOld);
      }

      sprintf (result, "%u", *(uint16_t *)addr);
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'i':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 4, PAGE_READWRITE, &dwOld);
          uint32_t out;
          sscanf (val, "%x", &out);
          *(uint32_t *)addr = out;
        VirtualProtect ((LPVOID)addr, 4, dwOld, &dwOld);
      }

      sprintf (result, "%u", *(uint32_t *)addr);
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'l':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 8, PAGE_READWRITE, &dwOld);
          uint64_t out;
          sscanf (val, "%llx", &out);
          *(uint64_t *)addr = out;
        VirtualProtect ((LPVOID)addr, 8, dwOld, &dwOld);
      }

      sprintf (result, "%llu", *(uint64_t *)addr);
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'd':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 8, PAGE_READWRITE, &dwOld);
          double out;
          sscanf (val, "%lf", &out);
          *(double *)addr = out;
        VirtualProtect ((LPVOID)addr, 8, dwOld, &dwOld);
      }

      sprintf (result, "%f", *(double *)addr);
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'f':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 4, PAGE_READWRITE, &dwOld);
          float out;
          sscanf (val, "%f", &out);
          *(float *)addr = out;
        VirtualProtect ((LPVOID)addr, 4, dwOld, &dwOld);
      }

      sprintf (result, "%f", *(float *)addr);
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 't':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 256, PAGE_READWRITE, &dwOld);
          strcpy ((char *)addr, val);
        VirtualProtect ((LPVOID)addr, 256, dwOld, &dwOld);
      }
      sprintf (result, "%s", (char *)addr);
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
  }

  return SK_ICommandResult ("mem", szArgs);
}


struct init_params_t {
  const wchar_t* backend;
  void*          callback;
} init_;

unsigned int
__stdcall
CheckVersionThread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  extern bool
  __stdcall
  SK_FetchVersionInfo (const wchar_t* wszProduct);

  if (SK_FetchVersionInfo (L"SpecialK")) {
    extern HRESULT
      __stdcall
      SK_UpdateSoftware (const wchar_t* wszProduct);

    SK_UpdateSoftware (L"SpecialK");
  }

  CloseHandle (GetCurrentThread ());

  return 0;
}

unsigned int
__stdcall
DllThread_CRT (LPVOID user)
{
  init_params_t* params =
    (init_params_t *)user;

  SK_InitCore (params->backend, params->callback);

  if (SK_IsSuperSpecialK ())
    return 0;

  SK_InitCompatBlacklist ();

  extern int32_t SK_D3D11_amount_to_purge;
  SK_GetCommandProcessor ()->AddVariable (
    "VRAM.Purge",
      new SK_IVarStub <int32_t> (
        (int32_t *)&SK_D3D11_amount_to_purge
      )
  );

  SK_GetCommandProcessor()->AddVariable(
    "GPU.StatPollFreq",
      new SK_IVarStub <float> (
        &config.gpu.interval
      )
  );

  SK_GetCommandProcessor ()->AddVariable (
    "ImGui.FontScale",
      new SK_IVarStub <float> (
        &config.imgui.scale
      )      
  );

  SK_InitRenderBackends ();

  skMemCmd*    mem    = new skMemCmd    ();
  skUpdateCmd* update = new skUpdateCmd ();

  SK_GetCommandProcessor ()->AddCommand ("mem",       mem);
  SK_GetCommandProcessor ()->AddCommand ("GetUpdate", update);

  //
  // Game-Specific Stuff that I am not proud of
  //
  if (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe"))
    SK_DS3_InitPlugin ();

  if (lstrcmpW (SK_GetHostApp (), L"Tales of Zestiria.exe")) {
    SK_GetCommandProcessor ()->ProcessCommandFormatted (
      "TargetFPS %f",
        config.render.framerate.target_fps
    );
  }

  // Get rid of the game output log if the user doesn't want it...
  if (! config.system.game_output) {
    game_debug.close ();
    game_debug.silent = true;
  }

  const wchar_t* config_name = params->backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  if (! SK_IsSuperSpecialK ())
    SK_SaveConfig (config_name);

  // For the global injector, when not started by SKIM, check its version
  if (SK_IsInjected () && (! SK_IsSuperSpecialK ()))
    _beginthreadex (nullptr, 0, CheckVersionThread, nullptr, 0x00, nullptr);

  return 0;
}

DWORD
__stdcall
DllThread (LPVOID user)
{
  DllThread_CRT (user);

  return 0;
}

#include <wingdi.h>
#include <gl/gl.h>

extern std::pair <std::queue <DWORD>, BOOL> __stdcall SK_BypassInject (void);

bool
__stdcall
SK_StartupCore (const wchar_t* backend, void* callback)
{
  // Allow users to centralize all files if they want
  if ( GetFileAttributes ( L"SpecialK.central" ) != INVALID_FILE_ATTRIBUTES )
    config.system.central_repository = true;

  if (SK_IsInjected ())
    config.system.central_repository = true;

  // Holy Rusted Metal Batman !!!
  //---------------------------------------------------------------------------
  //
  //  * <Black Lists Matter> *
  //
  //   Replace this with something faster such as a trie ... we cannot use
  //     STL at this point, that would bring in dependencies on other MSVCRT
  //       DLLs.
  //
  //   *** (A static trie or hashmap should be doable though.) ***
  //___________________________________________________________________________
       // Steam-Specific Stuff
  if ( (! SK_Path_wcsicmp (SK_GetHostApp(),L"steam.exe"))                    ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"GameOverlayUI.exe"))            ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"streaming_client.exe"))         ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"steamerrorreporter.exe"))       ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"steamerrorreporter64.exe"))     ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"steamservice.exe"))             ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"steam_monitor.exe"))            ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"steamwebhelper.exe"))           ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"html5app_steam.exe"))           ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"wow_helper.exe"))               ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"uninstall.exe"))                ||
        

       // Crash Helpers
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"WriteMiniDump.exe"))            ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"CrashReporter.exe"))            ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"SupportTool.exe"))              ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"CrashSender1400.exe"))          ||

       // Runtime Installers
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"DXSETUP.exe"))                  ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"setup.exe"))                    ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"vc_redist.x64.exe"))            ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"vc_redist.x86.exe"))            ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"vc2010redist_x64.exe"))         ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"vc2010redist_x86.exe"))         ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"vcredist_x64.exe"))             ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"vcredist_x86.exe"))             ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),
                       L"NDP451-KB2872776-x86-x64-AllOS-ENU.exe"))           ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"dotnetfx35.exe"))               ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"DotNetFx35Client.exe"))         ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"dotNetFx40_Full_x86_x64.exe"))  ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"dotNetFx40_Client_x86_x64.exe"))||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"oalinst.exe"))                  ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"EasyAntiCheat_Setup.exe"))      ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"UplayInstaller.exe"))           ||

       // Launchers
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"x64launcher.exe"))              ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"x86launcher.exe"))              ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"Launcher.exe"))                 ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"FFX&X-2_LAUNCHER.exe"))         ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"Fallout4Launcher.exe"))         ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"SkyrimSELauncher.exe"))         ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"ModLauncher.exe"))              ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"AkibaUU_Config.exe"))           ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"Obduction.exe"))                ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"Grandia2Launcher.exe"))         ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"FFXiii2Launcher.exe"))          ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"Bethesda.net_Launcher.exe"))    ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"UbisoftGameLauncher.exe"))      ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"UbisoftGameLauncher64.exe"))    ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"SplashScreen.exe"))             ||

       // Other Stuff
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"zosSteamStarter.exe"))          ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"notepad.exe"))                  ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"7zFM.exe"))                     ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"WinRar.exe"))                   ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"EAC.exe"))                      ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"vcpkgsrv.exe"))                 ||

       // Misc. Tools
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"SleepOnLan.exe"))               ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"ds3t.exe"))                     ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"tzt.exe")) ) {
    //FreeLibrary (SK_GetDLL ());
    return false;
  }

  // This is a fatal combination
  if (SK_IsInjected () && SK_IsSuperSpecialK ()) {
    //FreeLibrary (SK_GetDLL ());
    return false;
  }

  // Only the injector version can be bypassed, the wrapper version
  //   must fully initialize or the game will not be able to use the
  //     DLL it is wrapping.
  if ( SK_IsInjected    ()         &&
       GetAsyncKeyState (VK_SHIFT) &&
       GetAsyncKeyState (VK_CONTROL) ) {
    std::pair <std::queue <DWORD>, BOOL> retval =
      SK_BypassInject ();

    SK_ResumeThreads (retval.first);

    //FreeLibrary (SK_GetDLL ());
    return false;
  }

  else {
    bool blacklist =
      SK_IsInjected () &&
      (GetFileAttributesW (SK_GetBlacklistFilename ()) != INVALID_FILE_ATTRIBUTES);

    //
    // Internal blacklist, the user will have the ability to setup their
    //   own later...
    //
    if ( blacklist ) {
      //FreeLibrary_Original (SK_GetDLL ());
      return false;
    }
  }

  ZeroMemory (&init_, sizeof init_params_t);

  init_.backend  = _wcsdup (backend);
  init_.callback =          callback;

  EnterCriticalSection (&init_mutex);

  SK::Diagnostics::CrashHandler::InitSyms ();

  // Don't start SteamAPI if we're running the installer...
  if (SK_IsSuperSpecialK ())
    config.steam.init_delay = 0;

  wchar_t wszConfigPath [MAX_PATH + 1] = { L'\0' };
          wszConfigPath [  MAX_PATH  ] = L'\0';

          SK_RootPath   [    0     ]   = L'\0';
          SK_RootPath   [ MAX_PATH ]   = L'\0';

  if (config.system.central_repository) {
    if (! SK_IsSuperSpecialK ()) {
      ExpandEnvironmentStringsW (
        L"%USERPROFILE%\\Documents\\My Mods\\SpecialK",
          SK_RootPath,
            MAX_PATH - 1
      );
    } else {
      GetCurrentDirectory (MAX_PATH, SK_RootPath);
    }

    lstrcatW (wszConfigPath, SK_GetRootPath ());
    lstrcatW (wszConfigPath, L"\\Profiles\\");
    lstrcatW (wszConfigPath, SK_GetHostApp  ());
  }

  else {
    if (! SK_IsSuperSpecialK ()) {
      lstrcatW (SK_RootPath,   SK_GetHostPath ());
    } else {
      GetCurrentDirectory (MAX_PATH, SK_RootPath);
    }
    lstrcatW (wszConfigPath, SK_GetHostPath ());
  }

  lstrcatW (SK_RootPath,   L"\\");
  lstrcatW (wszConfigPath, L"\\");

  SK_CreateDirectories (wszConfigPath);
  SK_SetConfigPath     (wszConfigPath);


  wchar_t log_fname [MAX_PATH];
          log_fname [MAX_PATH - 1] = L'\0';

  swprintf (log_fname, L"logs/%s.log", SK_IsInjected () ? L"SpecialK" : backend);

  dll_log.init (log_fname, L"w");
  dll_log.Log  (L"%s.log created",     SK_IsInjected () ? L"SpecialK" : backend);

  budget_log.init ( L"logs\\dxgi_budget.log", L"w" );

  dll_log.LogEx (false,
    L"------------------------------------------------------------------------"
    L"-------------------\n");

  std::wstring   module_name   = SK_GetModuleName (SK_GetDLL ());
  const wchar_t* wszModuleName = module_name.c_str ();

  dll_log.Log   (      L">> (%s) [%s] <<",
                         SK_GetHostApp (),
                           wszModuleName );

  const wchar_t* config_name = backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  if (! SK_IsSuperSpecialK ())
  {
    dll_log.LogEx (true, L"Loading user preferences from %s.ini... ", config_name);

    if (SK_LoadConfig (config_name))
      dll_log.LogEx (false, L"done!\n");

    else
    {
      dll_log.LogEx (true, L"Loading user preferences from %s.ini... ", config_name);
      dll_log.LogEx (false, L"failed!\n");

      std::wstring default_name (L"default_");
                   default_name += config_name;

      std::wstring default_ini (default_name + L".ini");

      if (GetFileAttributesW (default_ini.c_str ()) != INVALID_FILE_ATTRIBUTES)
      {
        dll_log.LogEx (true, L"Loading default preferences from %s.ini... ", default_name.c_str ());

        if (! SK_LoadConfig (default_name))
          dll_log.LogEx (false, L"failed!\n");
      }

      // If no INI file exists, write one immediately.
      dll_log.LogEx (true, L"  >> Writing base INI file, because none existed... ");
      SK_SaveConfig (config_name);
      dll_log.LogEx (false, L"done!\n");
    }
  }


  if (SK_IsSuperSpecialK ()) {
    config.system.handle_crashes = false;
    config.system.game_output    = false;
    config.steam.silent          = true;
  }

  SK_Init_MinHook                       ();

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Init ();


  // Don't let Steam prevent me from attaching a debugger at startup, damnit!
  SK::Diagnostics::Debugger::Allow ();

  game_debug.init (L"logs/game_output.log", L"w");

  if (config.system.display_debug_out)
    SK::Diagnostics::Debugger::SpawnConsole ();


  if (! SK_IsSuperSpecialK ()) {
    SK_Steam_InitCommandConsoleVariables ();
    SK_TestSteamImports                  (GetModuleHandle (nullptr));
  }


  // Do this from the startup thread
  SK_HookWinAPI       ();
  SK::Framerate::Init ();



  // Hard-code the AppID for ToZ
  if (! lstrcmpW (SK_GetHostApp (), L"Tales of Zestiria.exe"))
    config.steam.appid = 351970;

  // Game won't start from the commandline without this...
  if (! lstrcmpW (SK_GetHostApp (), L"dis1_st.exe"))
    config.steam.appid = 405900;


  SK_EnumLoadedModules (SK_ModuleEnum::PreLoad);

  LeaveCriticalSection (&init_mutex);


  InterlockedExchangePointer (
    (void **)&hInitThread,
      (HANDLE)
        CreateThread ( nullptr,
                         0,
                           DllThread,
                             &init_,
                               0x00,
                                 nullptr )
  );

  return true;
}

extern "C" {
bool
WINAPI
SK_ShutdownCore (const wchar_t* backend)
{
  if (config.window.background_mute)
    SK_SetGameMute (FALSE);

  // These games do not handle resolution correctly
  if ( (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe")) ||
       (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe"))     ||
       (! lstrcmpW (SK_GetHostApp (), L"FFX.exe"))          ||
       (! lstrcmpW (SK_GetHostApp (), L"FFX-2.exe"))        ||
       (! lstrcmpW (SK_GetHostApp (), L"dis1_st.exe")) )
    ChangeDisplaySettingsA (nullptr, CDS_RESET);

  SK_AutoClose_Log (game_debug);
  SK_AutoClose_Log (   dll_log);

  SK_Console* pConsole = SK_Console::getInstance ();
  pConsole->End ();

  if (hPumpThread != 0) {
    dll_log.LogEx   (true, L"[ Stat OSD ] Shutting down Pump Thread... ");

    TerminateThread (hPumpThread, 0);
    CloseHandle     (hPumpThread);
    hPumpThread = 0;

    dll_log.LogEx   (false, L"done!\n");
  }

  SK::DXGI::ShutdownBudgetThread ();

  if (process_stats.hThread != 0) {
    dll_log.LogEx (true,L"[ WMI Perf ] Shutting down Process Monitor... ");
    // Signal the thread to shutdown
    process_stats.lID = 0;
    WaitForSingleObject
      (process_stats.hThread, 1000UL); // Give 1 second, and
                                       // then we're killing
                                       // the thing!
    TerminateThread (process_stats.hThread, 0);
    CloseHandle     (process_stats.hThread);
    process_stats.hThread  = 0;
    dll_log.LogEx (false, L"done!\n");
  }

  if (cpu_stats.hThread != 0) {
    dll_log.LogEx (true,L"[ WMI Perf ] Shutting down CPU Monitor... ");
    // Signal the thread to shutdown
    cpu_stats.lID = 0;
    WaitForSingleObject (cpu_stats.hThread, 1000UL); // Give 1 second, and
                                                     // then we're killing
                                                     // the thing!
    TerminateThread (cpu_stats.hThread, 0);
    CloseHandle     (cpu_stats.hThread);
    cpu_stats.hThread  = 0;
    cpu_stats.num_cpus = 0;
    dll_log.LogEx (false, L"done!\n");
  }

  if (disk_stats.hThread != 0) {
    dll_log.LogEx (true, L"[ WMI Perf ] Shutting down Disk Monitor... ");
    // Signal the thread to shutdown
    disk_stats.lID = 0;
    WaitForSingleObject (disk_stats.hThread, 1000UL); // Give 1 second, and
                                                      // then we're killing
                                                      // the thing!
    TerminateThread (disk_stats.hThread, 0);
    CloseHandle     (disk_stats.hThread);
    disk_stats.hThread   = 0;
    disk_stats.num_disks = 0;
    dll_log.LogEx (false, L"done!\n");
  }

  if (pagefile_stats.hThread != 0) {
    dll_log.LogEx (true, L"[ WMI Perf ] Shutting down Pagefile Monitor... ");
    // Signal the thread to shutdown
    pagefile_stats.lID = 0;
    WaitForSingleObject (
      pagefile_stats.hThread, 1000UL); // Give 1 second, and
                                       // then we're killing
                                       // the thing!
    TerminateThread (pagefile_stats.hThread, 0);
    CloseHandle     (pagefile_stats.hThread);
    pagefile_stats.hThread       = 0;
    pagefile_stats.num_pagefiles = 0;
    dll_log.LogEx (false, L"done!\n");
  }

  // Breakbad Disable Disclaimer; pretend the log was empty :)
  if (crash_log.lines == 1)
    crash_log.lines = 0;

  crash_log.close ();

  const wchar_t* config_name = backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  if (sk::NVAPI::app_name != L"ds3t.exe") {
    dll_log.LogEx (true,  L"[ SpecialK ] Saving user preferences to %s.ini... ", config_name);
    SK_SaveConfig (config_name);
    dll_log.LogEx (false, L"done!\n");
  }

#if 0
  if ((! config.steam.silent)) {
    dll_log.LogEx  (true, L"[ SteamAPI ] Shutting down Steam API... ");
    SK::SteamAPI::Shutdown ();
    dll_log.LogEx  (false, L"done!\n");
  }
#endif

  // Don't care about crashes after this :)
  config.system.handle_crashes = false;

  SK_UnloadImports        ();
  SK::Framerate::Shutdown ();

  if (nvapi_init)
    sk::NVAPI::UnloadLibrary ();

  dll_log.Log (L"[ SpecialK ] Custom %s.dll Detached (pid=0x%04x)",
    backend, GetCurrentProcessId ());

  SymCleanup (GetCurrentProcess ());

  SK_ShutdownWMI ();
  

  FreeLibrary_Original (backend_dll);

  SK_UnInit_MinHook      ();
  //FreeLibrary_Original (SK_GetDLL ());

  return true;
}
}


extern void SK_InitWindow (HWND hWnd);
#include <SpecialK/render_backend.h>

__declspec (noinline)
COM_DECLSPEC_NOTHROW
void
STDMETHODCALLTYPE
SK_BeginBufferSwap (void)
{
  // Throttle, but do not deadlock the render loop
  if (InterlockedCompareExchangeNoFence (&SK_bypass_dialog_active, FALSE, FALSE))
    Sleep (166);

  static int import_tries = 0;

  if (import_tries++ == 0)
  {
  // Load user-defined DLLs (Late)
#ifdef _WIN64
    SK_LoadLateImports64 ();
#else
    SK_LoadLateImports32 ();
#endif

    extern volatile LONGLONG SK_SteamAPI_CallbackRunCount;

    if (InterlockedCompareExchange64 (&SK_SteamAPI_CallbackRunCount, 0, 0) == 0)
    {
      // Steam Init: Better late than never

      SK_TestSteamImports (nullptr);
    }


    if (config.system.handle_crashes)
      SK::Diagnostics::CrashHandler::Reinstall ();
  }

  static volatile ULONG first = TRUE;

  if (InterlockedCompareExchange (&first, 0, 0))
  {
    if (SK_GetGameWindow () != 0)
    {
      extern void SK_ResetWindow ();

      InterlockedExchange (&first, 0);

      SK_ResetWindow ();
    }
  }

  static volatile LONG cegui_init = FALSE;

  if (InterlockedCompareExchange (&frames_drawn, 0, 0) > 2 && (! InterlockedCompareExchange (&cegui_init, 1, 0)))
  {
    if (config.cegui.enable)
    {
      SK_LockDllLoader ();
  
      // Disable until we validate CEGUI's state
      config.cegui.enable = false;
  
      wchar_t wszCEGUIModPath [MAX_PATH] = { L'\0' };
      wchar_t wszCEGUITestDLL [MAX_PATH] = { L'\0' };
  
  #ifdef _WIN64
      _swprintf (wszCEGUIModPath, L"%sCEGUI\\bin\\x64",   SK_GetRootPath ());
  #else
      _swprintf (wszCEGUIModPath, L"%sCEGUI\\bin\\Win32",  SK_GetRootPath ());
  #endif
  
      lstrcatW      (wszCEGUITestDLL, wszCEGUIModPath);
      lstrcatW      (wszCEGUITestDLL, L"\\CEGUIBase-0.dll");
  
      // This is only guaranteed to be supported on Windows 8, but Win7 and Vista
      //   do support it if a certain Windows Update (KB2533623) is installed.
      typedef DLL_DIRECTORY_COOKIE (WINAPI *AddDllDirectory_pfn)          (_In_ PCWSTR               NewDirectory);
      typedef BOOL                 (WINAPI *RemoveDllDirectory_pfn)       (_In_ DLL_DIRECTORY_COOKIE Cookie);
      typedef BOOL                 (WINAPI *SetDefaultDllDirectories_pfn) (_In_ DWORD                DirectoryFlags);
  
      static AddDllDirectory_pfn k32_AddDllDirectory =
        (AddDllDirectory_pfn)
          GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                             "AddDllDirectory" );
  
      static RemoveDllDirectory_pfn k32_RemoveDllDirectory =
        (RemoveDllDirectory_pfn)
          GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                             "RemoveDllDirectory" );
  
      static SetDefaultDllDirectories_pfn k32_SetDefaultDllDirectories =
        (SetDefaultDllDirectories_pfn)
          GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                             "SetDefaultDllDirectories" );
  
      if ( k32_AddDllDirectory          && k32_RemoveDllDirectory &&
           k32_SetDefaultDllDirectories &&
  
             GetFileAttributesW (wszCEGUITestDLL) != INVALID_FILE_ATTRIBUTES )
      {
        dll_log.Log (L"[  CEGUI   ] Enabling CEGUI: (%s)", wszCEGUITestDLL);
  
        wchar_t wszEnvVar [ MAX_PATH + 32 ] = { L'\0' };
  
        _swprintf (wszEnvVar, L"CEGUI_MODULE_DIR=%s", wszCEGUIModPath);
        _wputenv  (wszEnvVar);
  
        // This tests for the existence of the DLL before attempting to load it...
        auto DelayLoadDLL = [&](const char* szDLL)->
          bool
            {
              k32_SetDefaultDllDirectories (
                LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                LOAD_LIBRARY_SEARCH_SYSTEM32        | LOAD_LIBRARY_SEARCH_USER_DIRS
              );
  
              DLL_DIRECTORY_COOKIE cookie = 0;
              bool                 ret    = false;
  
              __try {
                    char szFullDLL [MAX_PATH] = { '\0' };
                sprintf (szFullDLL, "%ws\\%s", wszCEGUIModPath, szDLL);
  
                cookie =               k32_AddDllDirectory    (wszCEGUIModPath);
                ret    = SUCCEEDED ( __HrLoadAllImportsForDll (szDLL)           );
              }
  
              __except (EXCEPTION_EXECUTE_HANDLER) {
              }
  
              k32_RemoveDllDirectory (cookie);
  
              return ret;
            };
  
        if (DelayLoadDLL ("CEGUIBase-0.dll"))
        {
          if (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::OpenGL)
          {
            if (config.apis.OpenGL.hook)
            {
              if (DelayLoadDLL ("CEGUIOpenGLRenderer-0.dll"))
                config.cegui.enable = true;
            }
          }

          if ( SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D9 ||
               SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D9Ex )
          {
            if (config.apis.d3d9.hook || config.apis.d3d9ex.hook)
            {
              if (DelayLoadDLL ("CEGUIDirect3D9Renderer-0.dll"))
                config.cegui.enable = true;
            }
          }

          if ( SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D11 )
          {
            if (DelayLoadDLL ("CEGUIDirect3D11Renderer-0.dll"))
              config.cegui.enable = true;
          }
        }
      }
  
      SK_UnlockDllLoader ();
    }
  }


  SK_DrawOSD         ();
  SK_DrawConsole     ();
  extern void SK_DrawTexMgrStats (void);
  SK_DrawTexMgrStats ();

  static HMODULE hModTBFix = GetModuleHandle( L"tbfix.dll");

  if (hModTBFix) {
    SK::Framerate::GetLimiter ()->wait ();
  }

  extern bool SK_ImGui_Visible;

  if (SK_ImGui_Visible)
  {
    //
    // TEMP HACK: There is only one opportune time to do this in DXGI-based APIs
    //     
    if (SK_GetCurrentRenderBackend ().api != SK_RenderAPI::D3D11) {
      extern DWORD SK_ImGui_DrawFrame (DWORD dwFlags, void* user);
                   SK_ImGui_DrawFrame (       0x00,          nullptr );
    }
  }
}

ULONGLONG poll_interval = 0;

void
DoKeyboard (void)
{
  //
  // Do not pol the keyboard while the game window is inactive
  //
  bool skip = true;

  if ( SK_GetGameWindow () == GetForegroundWindow () )
    skip = false;

  if (skip)
    return;




  static ULONGLONG last_osd_scale { 0ULL };
  static ULONGLONG last_poll      { 0ULL };
  static ULONGLONG last_drag      { 0ULL };

  SYSTEMTIME    stNow;
  FILETIME      ftNow;
  LARGE_INTEGER ullNow;

  GetSystemTime        (&stNow);
  SystemTimeToFileTime (&stNow, &ftNow);

  ullNow.HighPart = ftNow.dwHighDateTime;
  ullNow.LowPart  = ftNow.dwLowDateTime;

  static bool drag_lock   = false;
  static bool toggle_drag = false;

  if (HIWORD (GetAsyncKeyState (VK_CONTROL)) && HIWORD (GetAsyncKeyState (VK_SHIFT)) && HIWORD (GetAsyncKeyState (VK_SCROLL)))
  {
    if (! toggle_drag)
      drag_lock = (! drag_lock);
    toggle_drag = true;

    if (drag_lock)
      ClipCursor (nullptr); 
  } else {
    toggle_drag = false;
  }

  if (drag_lock)
    SK_CenterWindowAtMouse (FALSE);


  if (ullNow.QuadPart - last_osd_scale > 25ULL * poll_interval) {
    if (HIWORD (GetAsyncKeyState (config.osd.keys.expand [0])) &&
        HIWORD (GetAsyncKeyState (config.osd.keys.expand [1])) &&
        HIWORD (GetAsyncKeyState (config.osd.keys.expand [2])))
    {
      last_osd_scale = ullNow.QuadPart;
      SK_ResizeOSD (+0.1f);
    }

    if (HIWORD (GetAsyncKeyState (config.osd.keys.shrink [0])) &&
        HIWORD (GetAsyncKeyState (config.osd.keys.shrink [1])) &&
        HIWORD (GetAsyncKeyState (config.osd.keys.shrink [2])))
    {
      last_osd_scale = ullNow.QuadPart;
      SK_ResizeOSD (-0.1f);
    }
  }

#if 0
  if (ullNow.QuadPart < last_poll + poll_interval) {
    Sleep (10);
    last_poll = ullNow.QuadPart;
  }
#endif

  static bool toggle_time = false;
  if (HIWORD (GetAsyncKeyState (config.time.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.time.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.time.keys.toggle [2])))
  {
    if (! toggle_time) {
      SK_UnlockSteamAchievement (0);

      config.time.show = (! config.time.show);
    }
    toggle_time = true;
  } else {
    toggle_time = false;
  }

  static bool toggle_mem = false;
  if (HIWORD (GetAsyncKeyState (config.mem.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.mem.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.mem.keys.toggle [2])))
  {
    if (! toggle_mem) {
      config.mem.show = (! config.mem.show);

      if (config.mem.show)
        SK_StartPerfMonThreads ();
    }

    toggle_mem = true;
  } else {
    toggle_mem = false;
  }

#if 0
  static bool toggle_balance = false;
  if (HIWORD (GetAsyncKeyState (config.load_balance.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.load_balance.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.load_balance.keys.toggle [2])))
  {
    if (! toggle_balance)
      config.load_balance.use = (! config.load_balance.use);
    toggle_balance = true;
  } else {
    toggle_balance = false;
  }
#endif

  if (nvapi_init && sk::NVAPI::nv_hardware && sk::NVAPI::CountSLIGPUs () > 1)
  {
    static bool toggle_sli = false;
    if (HIWORD (GetAsyncKeyState (config.sli.keys.toggle [0])) &&
        HIWORD (GetAsyncKeyState (config.sli.keys.toggle [1])) &&
        HIWORD (GetAsyncKeyState (config.sli.keys.toggle [2])))
    {
      if (! toggle_sli)
        config.sli.show = (! config.sli.show);
      toggle_sli = true;
    } else {
      toggle_sli = false;
    }
  }

  static bool toggle_io = false;
  if (HIWORD (GetAsyncKeyState (config.io.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.io.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.io.keys.toggle [2])))
  {
    if (! toggle_io)
      config.io.show = (! config.io.show);
    toggle_io = true;
  } else {
    toggle_io = false;
  }

  static bool toggle_cpu = false;
  if (HIWORD (GetAsyncKeyState (config.cpu.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.cpu.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.cpu.keys.toggle [2])))
  {
    if (! toggle_cpu) {
      config.cpu.show = (! config.cpu.show);

      if (config.cpu.show)
        SK_StartPerfMonThreads ();
    }

    toggle_cpu = true;
  } else {
    toggle_cpu = false;
  }

  static bool toggle_gpu = false;
  if (HIWORD (GetAsyncKeyState (config.gpu.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.gpu.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.gpu.keys.toggle [2])))
  {
    if (! toggle_gpu)
      config.gpu.show = (! config.gpu.show);
    toggle_gpu = true;
  } else {
    toggle_gpu = false;
  }

  static bool toggle_fps = false;
  if (HIWORD (GetAsyncKeyState (config.fps.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.fps.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.fps.keys.toggle [2])))
  {
    if (! toggle_fps)
      config.fps.show = (! config.fps.show);
    toggle_fps = true;
  } else {
    toggle_fps = false;
  }

  static bool toggle_disk = false;
  if (HIWORD (GetAsyncKeyState (config.disk.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.disk.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.disk.keys.toggle [2])))
  {
    if (! toggle_disk) {
      config.disk.show = (! config.disk.show);

      if (config.disk.show)
        SK_StartPerfMonThreads ();
    }

    toggle_disk = true;
  } else {
    toggle_disk = false;
  }

  static bool toggle_pagefile = false;
  if (HIWORD (GetAsyncKeyState (config.pagefile.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.pagefile.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.pagefile.keys.toggle [2])))
  {
    if (! toggle_pagefile) {
      config.pagefile.show = (! config.pagefile.show);

      if (config.pagefile.show)
        SK_StartPerfMonThreads ();
    }

    toggle_pagefile = true;
  } else {
    toggle_pagefile = false;
  }

  static bool toggle_osd = false;
  if (HIWORD (GetAsyncKeyState (config.osd.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.osd.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.osd.keys.toggle [2])))
  {
    if (! toggle_osd) {
      config.osd.show = (! config.osd.show);
      if (config.osd.show)
        SK_InstallOSD ();
    }
    toggle_osd = true;
  } else {
    toggle_osd = false;
  }

  static bool toggle_render = false;
  if (HIWORD (GetAsyncKeyState (config.render.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.render.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.render.keys.toggle [2])))
  {
    if (! toggle_render)
      config.render.show = (! config.render.show);
    toggle_render = true;
  } else {
    toggle_render = false;
  }

}

#include <SpecialK/render_backend.h>

__declspec (noinline)
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
SK_EndBufferSwap (HRESULT hr, IUnknown* device)
{
  extern SK_RenderBackend __SK_RBkEnd;

  if (device != nullptr) {
    CComPtr <IDirect3DDevice9>   pDev9   = nullptr;
    CComPtr <IDirect3DDevice9Ex> pDev9Ex = nullptr;
    CComPtr <ID3D11Device>       pDev11  = nullptr;
    CComPtr <ID3D12Device>       pDev12  = nullptr;

    if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev9Ex)))) {
         (int&)__SK_RBkEnd.api  = ( (int)SK_RenderAPI::D3D9 |
                                    (int)SK_RenderAPI::D3D9Ex );
      wcsncpy (__SK_RBkEnd.name, L"D3D9Ex", 8);
    } else if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev9)))) {
               __SK_RBkEnd.api  = SK_RenderAPI::D3D9;
      wcsncpy (__SK_RBkEnd.name, L"D3D9  ", 8);
    } else if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev11)))) {
               __SK_RBkEnd.api  = SK_RenderAPI::D3D11;

      IUnknown* pTest = nullptr;

      if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device4,        (void **)&pTest))) {
        wcsncpy (__SK_RBkEnd.name, L"D3D11.4", 8);
        pTest->Release ();
      } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device3, (void **)&pTest))) {
        wcsncpy (__SK_RBkEnd.name, L"D3D11.3", 8);
        pTest->Release ();
      } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device2, (void **)&pTest))) {
        wcsncpy (__SK_RBkEnd.name, L"D3D11.2", 8);
        pTest->Release ();
      } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device1, (void **)&pTest))) {
        wcsncpy (__SK_RBkEnd.name, L"D3D11.1", 8);
        pTest->Release ();
      } else {
        wcsncpy (__SK_RBkEnd.name, L"D3D11 ", 8);
      }
    } else if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev12)))) {
               __SK_RBkEnd.api  = SK_RenderAPI::D3D12;
      wcsncpy (__SK_RBkEnd.name, L"D3D12 ", 8);
    } else {
               __SK_RBkEnd.api  = SK_RenderAPI::Reserved;
      wcsncpy (__SK_RBkEnd.name, L"UNKNOWN", 8);
    }
  }

#ifndef SK_BUILD__INSTALLER
  else {
    if (wglGetCurrentContext () != 0) {
               __SK_RBkEnd.api  = SK_RenderAPI::OpenGL;
      wcsncpy (__SK_RBkEnd.name, L"OpenGL", 8);
    }
  }
#endif

  static volatile ULONG budget_init = FALSE;

  if (! InterlockedCompareExchange (&budget_init, TRUE, FALSE)) {
    SK::DXGI::StartBudgetThread_NoAdapter ();
  }

  // Draw after present, this may make stuff 1 frame late, but... it
  //   helps with VSYNC performance.

  // Treat resize and obscured statuses as failures; DXGI does not, but
  //  we should not draw the OSD when these events happen.
  //if (FAILED (hr))
    //return hr;

#ifdef USE_MT_KEYS
  //
  // Do this in a different thread for the handful of game that use GetAsyncKeyState
  //   from the rendering thread to handle keyboard input.
  //
  if (poll_interval == 0ULL) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency (&freq);

    // Every 10 ms
    poll_interval = freq.QuadPart / 100ULL;

    _beginthreadex (
      nullptr,
        0,
          KeyboardThread,
            nullptr,
              0,
                nullptr );
  }
#else
  DoKeyboard ();
#endif

  if (config.sli.show && device != nullptr)
  {
    // Get SLI status for the frame we just displayed... this will show up
    //   one frame late, but this is the safest approach.
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0) {
      sli_state = sk::NVAPI::GetSLIState (device);
    }
  }

  InterlockedIncrement (&frames_drawn);

  static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");
  static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");

  //
  // TZFix has its own limiter
  //
  if (! (hModTZFix || hModTBFix)) {
    SK::Framerate::GetLimiter ()->wait ();
  }

  return hr;
}

struct sk_host_process_s {
  wchar_t wszApp       [ MAX_PATH * 2 ] = { L'\0' };
  wchar_t wszPath      [ MAX_PATH * 2 ] = { L'\0' };
  wchar_t wszFullName  [ MAX_PATH * 2 ] = { L'\0' };
  wchar_t wszBlacklist [ MAX_PATH * 2 ] = { L'\0' };
} host_proc;

bool
__cdecl
SK_IsHostAppSKIM (void)
{
  return (StrStrIW (SK_GetHostApp (), L"SKIM") != nullptr);
}

bool
__cdecl
SK_IsRunDLLInvocation (void)
{
  return (StrStrIW (SK_GetHostApp (), L"Rundll32") != nullptr);
}

bool
__cdecl
SK_IsSuperSpecialK (void)
{
  return (SK_IsRunDLLInvocation () || SK_IsHostAppSKIM ());
}


const wchar_t*
SK_GetHostApp (void)
{
  static volatile
    ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    DWORD   dwProcessSize =  MAX_PATH * 2;
    wchar_t wszProcessName [ MAX_PATH * 2 ] = { L'\0' };

    HANDLE hProc =
      GetCurrentProcess ();

    QueryFullProcessImageName (
      hProc,
        0,
          wszProcessName,
            &dwProcessSize );

    int      len           = lstrlenW (wszProcessName);
    wchar_t* pwszShortName =           wszProcessName;

    for (int i = 0; i < len; i++)
      pwszShortName = CharNextW (pwszShortName);

    while (  pwszShortName > wszProcessName ) {
      wchar_t* wszPrev =
        CharPrevW (wszProcessName, pwszShortName);

      if (wszPrev < wszProcessName)
        break;

      if (*wszPrev != L'\\' && *wszPrev != L'/') {
        pwszShortName = wszPrev;
        continue;
      }

      break;
    }

    lstrcpynW (
      host_proc.wszApp,
        pwszShortName,
          MAX_PATH * 2 - 1
    );
  }

  return host_proc.wszApp;
}

const wchar_t*
SK_GetFullyQualifiedApp (void)
{
  static volatile
    ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    DWORD   dwProcessSize =  MAX_PATH * 2;
    wchar_t wszProcessName [ MAX_PATH * 2 ] = { L'\0' };

    HANDLE hProc =
      GetCurrentProcess ();

    QueryFullProcessImageName (
      hProc,
        0,
          wszProcessName,
            &dwProcessSize );

    lstrcpynW (
      host_proc.wszFullName,
        wszProcessName,
          MAX_PATH * 2 - 1
    );
  }

  return host_proc.wszFullName;
}

// NOT the working directory, this is the directory that
//   the executable is located in.

const wchar_t*
SK_GetHostPath (void)
{
  static volatile
    ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    DWORD   dwProcessSize =  MAX_PATH * 2;
    wchar_t wszProcessName [ MAX_PATH * 2 ] = { L'\0' };

    HANDLE hProc =
      GetCurrentProcess ();

    QueryFullProcessImageName (
      hProc,
        0,
          wszProcessName,
            &dwProcessSize );

    int      len           = lstrlenW (wszProcessName);
    wchar_t* pwszShortName =           wszProcessName;

    for (int i = 0; i < len; i++)
      pwszShortName = CharNextW (pwszShortName);

    wchar_t* wszFirstSep = nullptr;

    while (  pwszShortName > wszProcessName )
    {
      wchar_t* wszPrev =
        CharPrevW (wszProcessName, pwszShortName);

      if (wszPrev < wszProcessName)
        break;

      if (*wszPrev == L'\\' || *wszPrev == L'/')
      {                              // Leave the trailing separator
        wszFirstSep = wszPrev; 
        break;
      }

      pwszShortName = wszPrev;
    }

    if (wszFirstSep != nullptr)
      *wszFirstSep = L'\0';

    lstrcpynW (
      host_proc.wszPath,
        wszProcessName,
          MAX_PATH * 2 - 1
    );
  }

  return host_proc.wszPath;
}

const wchar_t*
SK_GetBlacklistFilename (void)
{
  static volatile
    ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    lstrcatW (host_proc.wszBlacklist, SK_GetHostPath ());
    lstrcatW (host_proc.wszBlacklist, L"\\SpecialK.deny.");
    lstrcatW (host_proc.wszBlacklist, SK_GetHostApp  ());

    SK_PathRemoveExtension (host_proc.wszBlacklist);
  }

  return host_proc.wszBlacklist;
}

DLL_ROLE dll_role;

DLL_ROLE
__stdcall
SK_GetDLLRole (void)
{
  return dll_role;
}

void
__cdecl
SK_SetDLLRole (DLL_ROLE role)
{
  dll_role = role;
}


// Stupid solution, but very effective way of re-launching a game as admin without
//   the Steam client throwing a tempur tantrum.
void
CALLBACK
RunDLL_ElevateMe ( HWND  hwnd,        HINSTANCE hInst,
                   LPSTR lpszCmdLine, int       nCmdShow )
{
  ShellExecuteA ( nullptr, "runas", lpszCmdLine, nullptr, nullptr, SW_SHOWNORMAL );
}