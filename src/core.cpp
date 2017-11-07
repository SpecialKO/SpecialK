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

#include <ShlObj.h>
#include <LibLoaderAPI.h>

#pragma warning   (push)
#pragma warning   (disable: 4091)
#  include <DbgHelp.h>
#  pragma comment (lib, "dbghelp.lib")
#pragma warning   (pop)

#include <SpecialK/config.h>
#include <SpecialK/osd/text.h>
#include <SpecialK/memory_monitor.h>
#include <SpecialK/io_monitor.h>
#include <SpecialK/import.h>
#include <SpecialK/console.h>
#include <SpecialK/command.h>

#include <SpecialK/framerate.h>
#include <SpecialK/render_backend.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/vulkan_backend.h>
#include <SpecialK/nvapi.h>
#include <d3d9.h>
#include <d3d11.h>
#include <wingdi.h>
#include <gl/gl.h>
#ifdef _WIN64
#define D3D12_IGNORE_SDK_LAYERS
#include <SpecialK/d3d12_interfaces.h>
#endif

#include <SpecialK/adl.h>
#include <SpecialK/steam_api.h>

#include <imgui/imgui.h>

#include <SpecialK/update/version.h>
#include <SpecialK/update/network.h>

#include <SpecialK/widgets/widget.h>

#include <SpecialK/injection/address_cache.h>

#include <atlbase.h>
#include <comdef.h>
#include <delayimp.h>

#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>
#include <CEGUI/Logger.h>


extern iSK_Logger game_debug;

extern void SK_InitWindow (HWND hWnd);


std::queue <DWORD> init_tids;


static const GUID IID_ID3D11Device2 = { 0x9d06dffa, 0xd1e5, 0x4d07, { 0x83, 0xa8, 0x1b, 0xb1, 0x23, 0xf2, 0xf8, 0x41 } };
static const GUID IID_ID3D11Device3 = { 0xa05c8c37, 0xd2c6, 0x4732, { 0xb3, 0xa0, 0x9c, 0xe0, 0xb0, 0xdc, 0x9a, 0xe6 } };
static const GUID IID_ID3D11Device4 = { 0x8992ab71, 0x02e6, 0x4b8d, { 0xba, 0x48, 0xb0, 0x56, 0xdc, 0xda, 0x42, 0xc4 } };
static const GUID IID_ID3D11Device5 = { 0x8ffde202, 0xa0e7, 0x45df, { 0x9e, 0x01, 0xe8, 0x37, 0x80, 0x1b, 0x5e, 0xa0 } };

volatile HANDLE  hInitThread   = { nullptr };
         HANDLE  hPumpThread   = { INVALID_HANDLE_VALUE };

NV_GET_CURRENT_SLI_STATE sli_state;
BOOL                     nvapi_init       = FALSE;
HMODULE                  backend_dll      = nullptr;
const wchar_t*           __SK_DLL_Backend = L"INVALID";

// LOL -- This tests for RunDLL32 / SKIM
bool
__cdecl
SK_IsSuperSpecialK (void);


struct init_params_t {
  const wchar_t* backend  = L"INVALID";
  void*          callback =    nullptr;
} init_;


std::queue <DWORD> __SK_Init_Suspended_tids;

wchar_t SK_RootPath   [MAX_PATH + 2] = { };
wchar_t SK_ConfigPath [MAX_PATH + 2] = { };
wchar_t SK_Backend    [     128    ] = { };

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
  lstrcpynW (SK_ConfigPath, path, MAX_PATH);
}


const wchar_t*
__stdcall
SK_GetConfigPath (void);


__declspec (noinline)
const wchar_t*
__stdcall
SK_GetNaiveConfigPath (void)
{
  return SK_ConfigPath;
}

volatile
LONG frames_drawn = 0L;

__declspec (noinline)
ULONG
__stdcall
SK_GetFramesDrawn (void)
{
  return
    static_cast <ULONG> (ReadNoFence (&frames_drawn));
}

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

HANDLE osd_shutdown = INVALID_HANDLE_VALUE;

// Stupid solution for games that inexplicably draw to the screen
//   without ever swapping buffers.
DWORD
WINAPI
osd_pump (LPVOID lpThreadParam)
{
  UNREFERENCED_PARAMETER (lpThreadParam);

  // TODO: This actually increases the number of frames rendered, which
  //         may interfere with other mod logic... the entire feature is
  //           a hack, but that behavior is not intended.
  while (true)
  {
    if (WaitForSingleObject (osd_shutdown, (DWORD)(config.osd.pump_interval * 1000.0f)) == WAIT_OBJECT_0)
      break;

    SK_EndBufferSwap (S_OK, nullptr);
  }

  return 0;
}


void
__stdcall
SK_StartPerfMonThreads (void)
{
  if (config.mem.show)
  {
    //
    // Spawn Process Monitor Thread
    //
    if ( InterlockedCompareExchangePointer (&process_stats.hThread, nullptr, INVALID_HANDLE_VALUE) ==
           INVALID_HANDLE_VALUE )
    {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Process Monitor...  ");

      InterlockedExchangePointer ( (void **)&process_stats.hThread,
        CreateThread ( nullptr,
                         0,
                           SK_MonitorProcess,
                             nullptr,
                               0,
                                 nullptr )
      );

      if (process_stats.hThread != INVALID_HANDLE_VALUE)
        dll_log.LogEx (false, L"tid=0x%04x\n", GetThreadId (process_stats.hThread));
      else
        dll_log.LogEx (false, L"Failed!\n");
    }
  }

  if (config.cpu.show || SK_ImGui_Widgets.cpu_monitor->isActive ())
  {
    //
    // Spawn CPU Refresh Thread
    //
    if ( InterlockedCompareExchangePointer (&cpu_stats.hThread, nullptr, INVALID_HANDLE_VALUE) ==
           INVALID_HANDLE_VALUE )
    {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning CPU Monitor...      ");

      InterlockedExchangePointer ( (void **)&cpu_stats.hThread,
        CreateThread ( nullptr,
                         0,
                           SK_MonitorCPU,
                             nullptr,
                               0,
                                 nullptr )
      );

      if (cpu_stats.hThread != INVALID_HANDLE_VALUE)
        dll_log.LogEx (false, L"tid=0x%04x\n", GetThreadId (cpu_stats.hThread));
      else
        dll_log.LogEx (false, L"Failed!\n");
    }
  }

  if (config.disk.show)
  {
    if ( InterlockedCompareExchangePointer (&disk_stats.hThread, nullptr, INVALID_HANDLE_VALUE) ==
           INVALID_HANDLE_VALUE )
    {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Disk Monitor...     ");

      InterlockedExchangePointer ( (void **)&disk_stats.hThread,
        CreateThread ( nullptr,
                         0,
                           SK_MonitorDisk,
                             nullptr,
                               0,
                                 nullptr )
      );

      if (disk_stats.hThread != INVALID_HANDLE_VALUE)
        dll_log.LogEx (false, L"tid=0x%04x\n", GetThreadId (disk_stats.hThread));
      else
        dll_log.LogEx (false, L"failed!\n");
    }
  }

  if (config.pagefile.show)
  {
    if ( InterlockedCompareExchangePointer (&pagefile_stats.hThread, nullptr, INVALID_HANDLE_VALUE) ==
           INVALID_HANDLE_VALUE )
    {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Pagefile Monitor... ");

      InterlockedExchangePointer ( (void **)&pagefile_stats.hThread,
        CreateThread ( nullptr,
                         0,
                           SK_MonitorPagefile,
                             nullptr,
                               0,
                                 nullptr )
      );

      if (pagefile_stats.hThread != INVALID_HANDLE_VALUE)
        dll_log.LogEx ( false, L"tid=0x%04x\n",
                          GetThreadId (pagefile_stats.hThread) );
      else
        dll_log.LogEx (false, L"failed!\n");
    }
  }
}

//
// TODO : Move me somewhere more sensible...
//
class skMemCmd : public SK_ICommand {
public:
  virtual SK_ICommandResult execute (const char* szArgs) override;

  virtual int getNumArgs         (void) override { return 2; }
  virtual int getNumOptionalArgs (void) override { return 1; }
  virtual int getNumRequiredArgs (void) override {
    return getNumArgs () - getNumOptionalArgs ();
  }

protected:
private:
};

class skUpdateCmd : public SK_ICommand {
public:
  virtual SK_ICommandResult execute (const char* szArgs) override;

  virtual int getNumArgs         (void) override { return 1; }
  virtual int getNumOptionalArgs (void) override { return 1; }
  virtual int getNumRequiredArgs (void) override {
    return getNumArgs () - getNumOptionalArgs ();
  }

protected:
private:
};


SK_ICommandResult
skUpdateCmd::execute (const char* szArgs)
{
  if (! strlen (szArgs))
  {
    SK_FetchVersionInfo1 (L"SpecialK", true);
    SK_UpdateSoftware1   (L"SpecialK", true);
  }

  else
  {
    wchar_t wszProduct [128] = { };

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

  uintptr_t addr      =  0;
  char      type      =  0;
  char      val [256] = { };

#ifdef _WIN64
  sscanf (szArgs, "%c %llx %s", &type, &addr, val);
#else
  sscanf (szArgs, "%c %lx %s", &type, &addr, val);
#endif

  static uint8_t* base_addr = nullptr;

  if (base_addr == nullptr)
  {
    base_addr = reinterpret_cast <uint8_t *> (GetModuleHandle (nullptr));

    MEMORY_BASIC_INFORMATION basic_mem_info;
    VirtualQuery (base_addr, &basic_mem_info, sizeof basic_mem_info);

    base_addr = reinterpret_cast <uint8_t *> (basic_mem_info.BaseAddress);
  }

  addr += reinterpret_cast <uintptr_t> (base_addr);

  char result [512] = { };

  switch (type)
  {
    case 'b':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 1, PAGE_EXECUTE_READWRITE, &dwOld);
          uint8_t out;
          sscanf (val, "%cx", &out);
          *reinterpret_cast <uint8_t *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 1, dwOld, &dwOld);
      }

      sprintf (result, "%u", *reinterpret_cast <uint8_t *> (addr));

      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 's':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 2, PAGE_EXECUTE_READWRITE, &dwOld);
          uint16_t out;
          sscanf (val, "%hx", &out);
          *reinterpret_cast <uint16_t *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 2, dwOld, &dwOld);
      }

      sprintf (result, "%u", *reinterpret_cast <uint16_t *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'i':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 4, PAGE_EXECUTE_READWRITE, &dwOld);
          uint32_t out;
          sscanf (val, "%x", &out);
          *reinterpret_cast <uint32_t *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 4, dwOld, &dwOld);
      }

      sprintf (result, "%u", *reinterpret_cast <uint32_t *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'l':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 8, PAGE_EXECUTE_READWRITE, &dwOld);
          uint64_t out;
          sscanf (val, "%llx", &out);
          *reinterpret_cast <uint64_t *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 8, dwOld, &dwOld);
      }

      sprintf (result, "%llu", *reinterpret_cast <uint64_t *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'd':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 8, PAGE_EXECUTE_READWRITE, &dwOld);
          double out;
          sscanf (val, "%lf", &out);
          *reinterpret_cast <double *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 8, dwOld, &dwOld);
      }

      sprintf (result, "%f", *reinterpret_cast <double *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 'f':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 4, PAGE_EXECUTE_READWRITE, &dwOld);
          float out;
          sscanf (val, "%f", &out);
          *reinterpret_cast <float *> (addr) = out;
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 4, dwOld, &dwOld);
      }

      sprintf (result, "%f", *reinterpret_cast <float *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 't':
      if (strlen (val))
      {
        DWORD dwOld;

        VirtualProtect (reinterpret_cast <LPVOID> (addr), 256, PAGE_EXECUTE_READWRITE, &dwOld);
          strcpy (reinterpret_cast <char *> (addr), val);
        VirtualProtect (reinterpret_cast <LPVOID> (addr), 256, dwOld, &dwOld);
      }
      sprintf (result, "%s", reinterpret_cast <char *> (addr));
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
  }

  return SK_ICommandResult ("mem", szArgs);
}

DWORD
WINAPI
CheckVersionThread (LPVOID user);

volatile LONG   SK_bypass_dialog_tid    =     0;
volatile LONG   SK_bypass_dialog_active = FALSE;
volatile LONG __SK_Init                 = FALSE;

void
__stdcall
SK_InitCore (const wchar_t* backend, void* callback)
{
  init_mutex->lock ();

  wcscpy (SK_Backend, backend);


  BOOL
  SK_Steam_PreHookCore (void);

  if (config.steam.spoof_BLoggedOn)
  {
    SK_Steam_PreHookCore ();
  }

  using finish_pfn   = void (WINAPI *)  (void);
  using callback_pfn = void (WINAPI *)(_Releases_exclusive_lock_ (init_mutex) finish_pfn);

  auto callback_fn =
    (callback_pfn)callback;

  dll_log.Log (L"[  NvAPI   ] Initializing NVIDIA API          (NvAPI)...");

  nvapi_init =
    sk::NVAPI::InitializeLibrary (SK_GetHostApp ());

  dll_log.Log (L"[  NvAPI   ]              NvAPI Init         { %s }",
                                                     nvapi_init ? L"Success" :
                                                                  L"Failed ");

  if (nvapi_init)
  {
    int num_sli_gpus =
      sk::NVAPI::CountSLIGPUs ();

    dll_log.Log ( L"[  NvAPI   ] >> NVIDIA Driver Version: %s",
                    sk::NVAPI::GetDriverVersion ().c_str () );

    int gpu_count =
      sk::NVAPI::CountPhysicalGPUs ();

    dll_log.Log ( gpu_count > 1 ? L"[  NvAPI   ]  * Number of Installed NVIDIA GPUs: %i  "
                                  L"{ SLI: '%s' }"
                                :
                                  L"[  NvAPI   ]  * Number of Installed NVIDIA GPUs: %i  { '%s' }",
                                    gpu_count > 1 ? num_sli_gpus :
                                                    num_sli_gpus + 1,
                                      sk::NVAPI::EnumGPUs_DXGI ()[0].Description );

    if (num_sli_gpus > 0)
    {
      DXGI_ADAPTER_DESC* sli_adapters =
        sk::NVAPI::EnumSLIGPUs ();

      int sli_gpu_idx = 0;

      while (*sli_adapters->Description != L'\0')
      {
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
    if (sk::NVAPI::CountSLIGPUs () && config.nvidia.sli.override)
    {
      if (! sk::NVAPI::SetSLIOverride
              ( SK_GetDLLRole (),
                  config.nvidia.sli.mode.c_str (),
                    config.nvidia.sli.num_gpus.c_str (),
                      config.nvidia.sli.compatibility.c_str ()
              )
         )
      {
        restart = true;
      }
    }

    if (restart)
    {
      dll_log.Log (L"[  NvAPI   ] >> Restarting to apply NVIDIA driver settings <<");

      ShellExecute ( GetDesktopWindow (),
                       L"OPEN",
                         SK_GetHostApp (),
                           nullptr,
                             nullptr,
                               SW_SHOWDEFAULT );
      exit (0);
    }
  }

  // Not NVIDIA, maybe AMD?
  else
  {
    dll_log.Log (L"[DisplayLib] Initializing AMD Display Library (ADL)...");

    BOOL adl_init =
      SK_InitADL ();

    dll_log.Log   (L"[DisplayLib]              ADL   Init         { %s }",
                                                     adl_init ? L"Success" :
                                                                L"Failed ");

    // Yes, AMD driver is in working order ...
    if (adl_init)
    {
      dll_log.Log ( L"[DisplayLib]  * Number of Reported AMD Adapters: %i (%i active)",
                      SK_ADL_CountPhysicalGPUs (),
                        SK_ADL_CountActiveGPUs () );
    }
  }

  HMODULE hMod =
    GetModuleHandle (SK_GetHostApp ());

  if (hMod != nullptr)
  {
    auto* dwOptimus =
      reinterpret_cast <DWORD *> (
        GetProcAddress ( hMod,
                           "NvOptimusEnablement" )
      );

    if (dwOptimus != nullptr)
    {
      dll_log.Log ( L"[Hybrid GPU]  NvOptimusEnablement..................: 0x%02X (%s)",
                      *dwOptimus,
                    ((*dwOptimus) & 0x1) ? L"Max Perf." :
                                           L"Don't Care" );
    }

    else
    {
      dll_log.Log (L"[Hybrid GPU]  NvOptimusEnablement..................: UNDEFINED");
    }

    auto* dwPowerXpress =
      reinterpret_cast <DWORD *> (
        GetProcAddress ( hMod,
                           "AmdPowerXpressRequestHighPerformance" )
      );

    if (dwPowerXpress != nullptr)
    {
      dll_log.Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: 0x%02X (%s)",
        *dwPowerXpress,
        (*dwPowerXpress & 0x1) ? L"High Perf." :
                                 L"Don't Care" );
    }

    else
      dll_log.Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: UNDEFINED");
  }


#ifdef _WIN64
  if (! lstrcmpW (SK_GetHostApp (), L"NieRAutomata.exe"))
    SK_FAR_InitPlugin ();

  if (SK_GetCurrentGameID () == SK_GAME_ID::BlueReflection)
  {
    extern void
    SK_IT_InitPlugin (void);

    SK_IT_InitPlugin ();
  }

  if (SK_GetCurrentGameID () == SK_GAME_ID::DotHackGU)
  {
    extern void
    SK_DGPU_InitPlugin (void);

    SK_DGPU_InitPlugin ();
  }
#endif


  //
  // NOT-SO-TEMP HACK: dgVoodoo2
  //
  if ( SK_GetDLLRole () == DLL_ROLE::D3D8 ||
       SK_GetDLLRole () == DLL_ROLE::DDraw  )
    SK_BootDXGI ();

  void
  __stdcall
  SK_InitFinishCallback (void);

  SK_ResumeThreads (__SK_Init_Suspended_tids);
         callback_fn (SK_InitFinishCallback);


  if ((! ReadAcquire (&SK_bypass_dialog_active)) && (GetCurrentThreadId () != ReadAcquire (&SK_bypass_dialog_tid)))
  {
    // Setup the compatibility backend, which monitors loaded libraries,
    //   blacklists bad DLLs and detects render APIs...
    SK_EnumLoadedModules (SK_ModuleEnum::PostLoad);
  }
}


void
WaitForInit (void)
{
  if (ReadNoFence (&__SK_Init))
    return;

  while (ReadPointerAcquire ((LPVOID *)&hInitThread) == nullptr)
    SleepEx (150, TRUE);

  while (ReadPointerAcquire ((LPVOID *)&hInitThread) != INVALID_HANDLE_VALUE)
  {
    if ( WAIT_OBJECT_0 == WaitForSingleObject (
      ReadPointerAcquire ((LPVOID *)&hInitThread),
        150 ) )
      break;
  }

  while (ReadAcquire (&SK_bypass_dialog_active) && (GetCurrentThreadId () != ReadAcquire (&SK_bypass_dialog_tid)))
  {
    dll_log.Log ( L"[ MultiThr ] Injection Bypass Dialog Active (tid=%x)",
                      GetCurrentThreadId () );
    MsgWaitForMultipleObjectsEx (0, nullptr, 150, QS_ALLINPUT, MWMO_ALERTABLE);
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
  if (! InterlockedCompareExchange (&__SK_Init, TRUE, FALSE))
  {
    SK_ApplyQueuedHooks ();

    CloseHandle (
      InterlockedExchangePointer ( const_cast <LPVOID *> (&hInitThread),
                                     INVALID_HANDLE_VALUE )
    );

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



void
__stdcall
SK_InitFinishCallback (void)
{
  SK_Input_Init       ();
  SK_ApplyQueuedHooks ();

  SK_DeleteTemporaryFiles ();
  SK_DeleteTemporaryFiles (L"Version", L"*.old");

  SK::Framerate::Init ();

  dll_log.Log (L"[ SpecialK ] === Initialization Finished! ===");

  if (SK_IsSuperSpecialK ())
  {
    init_mutex->unlock ();
    return;
  }

  extern int32_t SK_D3D11_amount_to_purge;
  SK_GetCommandProcessor ()->AddVariable (
    "VRAM.Purge",
      new SK_IVarStub <int32_t> (
        (int32_t *)&SK_D3D11_amount_to_purge
      )
  );

  SK_GetCommandProcessor ()->AddVariable (
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

  SK_GetCommandProcessor ()->AddCommand ("mem",       new skMemCmd    ());
  SK_GetCommandProcessor ()->AddCommand ("GetUpdate", new skUpdateCmd ());

  //
  // Game-Specific Stuff that I am not proud of
  //
#ifdef _WIN64
  if (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe"))
    SK_DS3_InitPlugin ();
#endif

  if (lstrcmpW (SK_GetHostApp (), L"Tales of Zestiria.exe"))
  {
    SK_GetCommandProcessor ()->ProcessCommandFormatted (
      "TargetFPS %f",
        config.render.framerate.target_fps
    );
  }

  // Get rid of the game output log if the user doesn't want it...
  if (! config.system.game_output)
  {
    game_debug.close ();
    game_debug.silent = true;
  }

  const wchar_t* config_name =
    init_.backend;

  // Use a generic "SpecialK" name instead of the primary wrapped/hooked API name
  //   for this DLL when it is injected at run-time rather than a proxy DLL.
  if (SK_IsInjected ())
    config_name = L"SpecialK";

  SK_SaveConfig (config_name);

  SK_Console::getInstance ()->Start ();

    // Create a thread that pumps the OSD
  if (config.osd.pump || SK_UsingVulkan ())
  {
    osd_shutdown =
      CreateEvent (nullptr, FALSE, FALSE, L"OSD Pump Shutdown");

    dll_log.LogEx (true, L"[ Stat OSD ] Spawning Pump Thread...      ");
    hPumpThread =
        CreateThread ( nullptr,
                         0,
                           osd_pump,
                             nullptr,
                               0,
                                 nullptr );

    if (hPumpThread != INVALID_HANDLE_VALUE)
      dll_log.LogEx ( false, L"tid=0x%04x, interval=%04.01f ms\n",
                        GetThreadId (hPumpThread),
                          config.osd.pump_interval * 1000.0f );
    else
      dll_log.LogEx (false, L"failed!\n");
  }

  SK_StartPerfMonThreads ();

  init_mutex->unlock ();

  SK_ResumeThreads    (init_tids);
}

DWORD
WINAPI
CheckVersionThread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  // If a local repository is present, use that.
  if (GetFileAttributes (L"Version\\installed.ini") == INVALID_FILE_ATTRIBUTES)
  {
    if (SK_FetchVersionInfo (L"SpecialK"))
    {
      // ↑ Check, but ↓ don't update unless running the global injector version
      if ( (SK_IsInjected () && (! SK_IsSuperSpecialK ())) )
      {
        InterlockedIncrement (&SK_bypass_dialog_active);
        InterlockedExchange  (&SK_bypass_dialog_tid, GetCurrentThreadId ());

        SK_UpdateSoftware (L"SpecialK");

        InterlockedDecrement (&SK_bypass_dialog_active);
      }
    }
  }

  CloseHandle (GetCurrentThread ());

  return 0;
}

init_params_t reentrant_core;

DWORD
WINAPI
DllThread (LPVOID user)
{
  auto* params =
    static_cast <init_params_t *> (user);

  reentrant_core = *params;

  SK_InitCore (params->backend, params->callback);

  return 0;
}


bool
__stdcall
SK_HasGlobalInjector (void)
{
  static int last_test = 0;

  if (last_test == 0)
  {
    wchar_t wszBasePath [MAX_PATH * 2 - 1] = { };

    wcsncpy ( wszBasePath,
                std::wstring ( SK_GetDocumentsDir () + L"\\My Mods\\SpecialK\\" ).c_str (),
                  MAX_PATH - 1 );

#ifdef _WIN64
    lstrcatW (wszBasePath, L"SpecialK64.dll");
#else
    lstrcatW (wszBasePath, L"SpecialK32.dll");
#endif

    bool result = (GetFileAttributesW (wszBasePath) != INVALID_FILE_ATTRIBUTES);
    last_test   = result ? 1 : -1;
  }

  return (last_test != -1);
}

extern std::pair <std::queue <DWORD>, BOOL> __stdcall SK_BypassInject (void);

struct sk_user_profile_s
{
  wchar_t wszProfile    [MAX_PATH] = { };
  wchar_t wszDocs       [MAX_PATH] = { };
  wchar_t wszEnvProfile [MAX_PATH] = { };
  wchar_t wszEnvDocs    [MAX_PATH] = { };
} static user_profile;

void
__stdcall
SK_EstablishRootPath (void)
{
  wchar_t wszConfigPath [MAX_PATH + 1] = { };
          wszConfigPath [  MAX_PATH  ] = { };

  // Store config profiles in a centralized location rather than relative to the game's executable
  //
  //   Currently, this location is always Documents\My Mods\SpecialK\
  //
  if (config.system.central_repository)
  {
    if (! SK_IsSuperSpecialK ())
    {
      wcsncpy ( SK_RootPath,
                  std::wstring ( SK_GetDocumentsDir () + L"\\My Mods\\SpecialK" ).c_str (),
                    MAX_PATH - 1 );
    }

    else
    {
      GetCurrentDirectory (MAX_PATH, SK_RootPath);
    }

    lstrcatW (wszConfigPath, SK_GetRootPath ());
    lstrcatW (wszConfigPath, L"\\Profiles\\");
    lstrcatW (wszConfigPath, SK_GetHostApp  ());
  }


  // Relative to game's executable path
  //
  else
  {
    if (! SK_IsSuperSpecialK ())
    {
      lstrcatW (SK_RootPath, SK_GetHostPath ());
    }

    else
    {
      GetCurrentDirectory (MAX_PATH, SK_RootPath);
    }

    lstrcatW (wszConfigPath, SK_GetRootPath ());
  }


  // Not using the ShellW API because at this stage we can only reliably use Kernel32 functions
  lstrcatW (SK_RootPath,   L"\\");
  lstrcatW (wszConfigPath, L"\\");


  SK_SetConfigPath     (wszConfigPath);
}

void
__stdcall
SK_ReenterCore  (void) // During startup, we have the option of bypassing init and resuming later
{
  auto* params =
    static_cast <init_params_t *> (&reentrant_core);

  SK_StartupCore (params->backend, params->callback);
}

bool
__stdcall
SK_StartupCore (const wchar_t* backend, void* callback)
{
  __SK_DLL_Backend = backend;

  // Allow users to centralize all files if they want
  //
  //   Stupid hack, if the application is running with a different working-directory than
  //     the executable -- compensate!
  if ( GetFileAttributes ( ( std::wstring (SK_GetHostPath ()) +
                             std::wstring (L"\\SpecialK.central") ).c_str () ) != INVALID_FILE_ATTRIBUTES )
    config.system.central_repository = true;

  if (SK_IsInjected ())
    config.system.central_repository = true;

  SK_EstablishRootPath ();
  SK_CreateDirectories (SK_GetConfigPath ());


  if (config.system.central_repository)
  {
    // Create Symlink for end-user's convenience
    if ( GetFileAttributes ( ( std::wstring (SK_GetHostPath ()) +
                               std::wstring (L"\\SpecialK\\")
                             ).c_str ()
                           ) == INVALID_FILE_ATTRIBUTES )
    {
      std::wstring link (SK_GetHostPath ());
      link += L"\\SpecialK\\";

      CreateSymbolicLink (
        link.c_str         (),
          SK_GetConfigPath (),
            SYMBOLIC_LINK_FLAG_DIRECTORY
      );
    }

    if ( GetFileAttributes ( ( std::wstring (SK_GetConfigPath ()) +
                               std::wstring (L"Game\\") ).c_str ()
                           ) == INVALID_FILE_ATTRIBUTES )
    {
      std::wstring link (SK_GetConfigPath ());
      link += L"Game\\";

      CreateSymbolicLink (
        link.c_str         (),
          SK_GetHostPath   (),
            SYMBOLIC_LINK_FLAG_DIRECTORY
      );
    }
  }


  static bool bypass = false;

  // Injection Compatibility Menu
  if ( (! bypass) && (GetAsyncKeyState (VK_SHIFT  ) & 0x8000) != 0 &&
                     (GetAsyncKeyState (VK_CONTROL) & 0x8000) != 0 )
  {
    bypass = true;

    SK_BypassInject ();

    bypass = false;

    return true;
  }

  else
  {
    bool blacklist =
      SK_IsInjected () &&
      (GetFileAttributesW (SK_GetBlacklistFilename ()) != INVALID_FILE_ATTRIBUTES);

    //
    // Internal blacklist, the user will have the ability to setup their
    //   own later...
    //
    if ( blacklist )
    {
      //FreeLibrary_Original (SK_GetDLL ());
      return false;
    }

    // Do this from the startup thread
    SK_HookWinAPI  ();
    MH_ApplyQueued ();

    // For the global injector, when not started by SKIM, check its version
    if ( (SK_IsInjected () && (! SK_IsSuperSpecialK ())) )
      CreateThread (nullptr, 0, CheckVersionThread, nullptr, 0x00, nullptr);
  }

  // Don't let Steam prevent me from attaching a debugger at startup
  game_debug.init                  (L"logs/game_output.log", L"w");
  game_debug.lockless = true;
  SK::Diagnostics::Debugger::Allow ();

  init_mutex->lock ();

  init_          = {               };
  init_.backend  = _wcsdup (backend);
  init_.callback =          callback;

  wchar_t log_fname [MAX_PATH + 2] = { };

  swprintf (log_fname, L"logs/%s.log", SK_IsInjected () ? L"SpecialK" : backend);

  dll_log.init (log_fname, L"w");
  dll_log.Log  (L"%s.log created",     SK_IsInjected () ? L"SpecialK" : backend);


  if (SK_IsSuperSpecialK ())
  {
    init_mutex->unlock ();
    return TRUE;
  }

  extern bool SK_Steam_LoadOverlayEarly (void);
  extern void SK_Input_PreInit          (void);

  budget_log.init ( L"logs\\dxgi_budget.log", L"w" );

  //dll_log.Log (L"LoadLibraryA Addres: %ph", GetProcAddress (GetModuleHandle (L"kernel32.dll"), "LoadLibraryA"));

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

  dll_log.LogEx (true, L"Loading user preferences from %s.ini... ", config_name);

  if (SK_LoadConfig (config_name))
    dll_log.LogEx (false, L"done!\n");

  else
  {
    dll_log.LogEx (false, L"failed!\n");

    std::wstring default_name (L"default_");
                 default_name += config_name;

    std::wstring default_ini (default_name + L".ini");

    if (GetFileAttributesW (default_ini.c_str ()) != INVALID_FILE_ATTRIBUTES)
    {
      dll_log.LogEx (true, L"Loading default preferences from %s.ini... ", default_name.c_str ());

      if (! SK_LoadConfig (default_name))
        dll_log.LogEx (false, L"failed!\n");
      else
        dll_log.LogEx (false, L"done!\n");
    }

    // If no INI file exists, write one immediately.
    dll_log.LogEx (true, L"  >> Writing base INI file, because none existed... ");
    SK_SaveConfig (config_name);
    dll_log.LogEx (false, L"done!\n");
  }


  if (bypass)
    goto BACKEND_INIT;


  init_tids = SK_SuspendAllOtherThreads ();


  if (config.system.display_debug_out)
    SK::Diagnostics::Debugger::SpawnConsole ();

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Init ();


  SK::Diagnostics::CrashHandler::InitSyms ();


  if (config.steam.preload_overlay)
  {

    CreateThread (nullptr, 0x00, [](LPVOID) -> DWORD {
                                   SK_Steam_LoadOverlayEarly ();
                                   SK_Input_Init             ();

                                   CloseHandle (GetCurrentThread ());

                                   return 0;
                                 }, nullptr, 0x00, nullptr);
  }

  SK_Input_PreInit (); // Hook only symbols in user32 and kernel32


  if (config.compatibility.init_while_suspended)
  {
    //
  }


  // Lazy-load SteamAPI into a process that doesn't use it, this brings
  //   a number of benefits.
  if (config.steam.force_load_steamapi)
  {
    static bool tried = false;

    if (! tried)
    {
      tried = true;

#ifdef _WIN64
      static const wchar_t* wszSteamDLL = L"steam_api64.dll";
#else
      static const wchar_t* wszSteamDLL = L"steam_api.dll";
#endif

      if (! GetModuleHandle (wszSteamDLL))
      {
        wchar_t wszDLLPath [MAX_PATH * 2 + 4] = { };

        if (SK_IsInjected ())
          wcsncpy (wszDLLPath, SK_GetModuleFullName (SK_GetDLL ()).c_str (), MAX_PATH * 2);
        else
        {
          _swprintf ( wszDLLPath, L"%s\\My Mods\\SpecialK\\SpecialK.dll",
                        SK_GetDocumentsDir ().c_str () );
        }

        if (PathRemoveFileSpec (wszDLLPath))
        {
          PathAppendW (wszDLLPath, wszSteamDLL);

          if (SK_GetFileSize (wszDLLPath) > 0)
          {
            HMODULE hMod = LoadLibraryW_Original (wszSteamDLL);

            if (hMod)
            {
              dll_log.Log ( L"[DLL Loader]   Manually booted SteamAPI: '%s'",
                              wszSteamDLL );//wszDLLPath );
            }
          }
        }
      }
    }
  }

  SK_Steam_InitCommandConsoleVariables ();
  SK_TestSteamImports                  (GetModuleHandle (nullptr));


  SK_EnumLoadedModules (SK_ModuleEnum::PreLoad);


BACKEND_INIT:
  dll_log.LogEx (false,
    L"----------------------------------------------------------------------"
    L"---------------------\n");

  // If the module name is this, then we need to load the system-wide DLL...
  wchar_t   wszProxyName [MAX_PATH];
  wsprintf (wszProxyName, L"%s.dll", backend);


#ifndef _WIN64
  //
  // TEMP HACK: dgVoodoo2
  //
  if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    wsprintf (wszProxyName, L"%s\\PlugIns\\ThirdParty\\dgVoodoo\\d3d8.dll",
                std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());

  else if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    wsprintf (wszProxyName, L"%s\\PlugIns\\ThirdParty\\dgVoodoo\\ddraw.dll",
                std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());
#endif


  wchar_t wszBackendDLL [MAX_PATH + 2] = { };
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

  wchar_t wszWorkDir   [MAX_PATH + 2] = { };
  GetCurrentDirectoryW (MAX_PATH, wszWorkDir);
       SK_StripUserNameFromPathW (wszWorkDir);

  dll_log.Log (L" Working Directory:          %s", wszWorkDir);
  dll_log.Log (L" System Directory:           %s", wszBackendDLL);

  lstrcatW (wszBackendDLL, L"\\");
  lstrcatW (wszBackendDLL, backend);
  lstrcatW (wszBackendDLL, L".dll");

        bool     use_system_dll = true;
  const wchar_t* dll_name       = wszBackendDLL;

  if (! SK_Path_wcsicmp (wszProxyName, wszModuleName))
    dll_name = wszBackendDLL;
  else
  {
    dll_name       = wszProxyName;
    use_system_dll = false;
  }

  bool load_proxy = false;

  if (! SK_IsInjected ())
  {
    for (auto& import : imports)
    {
      if (import.role != nullptr && import.role->get_value () == backend)
      {
        dll_log.LogEx (true, L" Loading proxy %s.dll:    ", backend);
        dll_name   = _wcsdup (import.filename->get_value ().c_str ());
        load_proxy = true;
        break;
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

  //// Pre-Load the original DLL into memory
  //if (dll_name != wszBackendDLL)
  //{
  //                LoadLibraryExW_Original ( wszBackendDLL, nullptr, use_system_dll ?
  //                                            LOAD_LIBRARY_SEARCH_SYSTEM32 : 0x00 );
  //  backend_dll = LoadLibraryExW_Original (dll_name,      nullptr, 0x0);
  //                GetModuleHandleExW      (GET_MODULE_HANDLE_EX_FLAG_PIN, wszBackendDLL, &backend_dll);
  //}
  //
  //else
  //  backend_dll = LoadLibraryExW_Original ( dll_name, nullptr, use_system_dll ?
  //                                           LOAD_LIBRARY_SEARCH_SYSTEM32 : 0x00);

  if (backend_dll != nullptr)
    dll_log.LogEx (false, L" (%s)\n", dll_name);
  else
    dll_log.LogEx (false, L" FAILED (%s)!\n", dll_name);

  // Free the temporary string storage
  if (load_proxy)
    free ((void *)dll_name);

  dll_log.LogEx (false,
    L"----------------------------------------------------------------------"
    L"---------------------\n");

  if (config.system.silent)
  {
    dll_log.silent = true;

    std::wstring log_fnameW;

    if (! SK_IsInjected ())
      log_fnameW = backend;
    else
      log_fnameW = L"SpecialK";

    log_fnameW += L".log";

    DeleteFile (log_fnameW.c_str ());
  }

  else
    dll_log.silent = false;

  InterlockedExchangePointer (
    (LPVOID *)&hInitThread,
      CreateThread ( nullptr,
                       0,
                         DllThread,
                           &init_,
                             0x00,
                               nullptr )
  ); // Avoid the temptation to wait on this thread

  init_mutex->unlock ();


  //// Performance monitorng pre-init  (Steam overlay hack; it hooks Wbem and blows stuff up almost immediately)
  CreateThread ( nullptr, 0,
                   [](LPVOID) -> DWORD
                   {
                     WaitForInputIdle (GetCurrentProcess (), INFINITE);

                     bool SK_InitWMI (void);
                       SK_InitWMI    (    );

                     CloseHandle (GetCurrentThread ());

                     return 0;
                   }, nullptr,
                 0x00,
               nullptr );
  return true;
}

using CreateWindowExW_pfn = HWND (WINAPI *)(
    _In_     DWORD     dwExStyle,
    _In_opt_ LPCWSTR   lpClassName,
    _In_opt_ LPCWSTR   lpWindowName,
    _In_     DWORD     dwStyle,
    _In_     int       X,
    _In_     int       Y,
    _In_     int       nWidth,
    _In_     int       nHeight,
    _In_opt_ HWND      hWndParent,
    _In_opt_ HMENU     hMenu,
    _In_opt_ HINSTANCE hInstance,
    _In_opt_ LPVOID    lpParam );

extern CreateWindowExW_pfn CreateWindowExW_Original;


std::set <HWND> dummy_windows;

HWND
SK_Win32_CreateDummyWindow (void)
{
  WNDCLASSW wc = { };

  wc.style         = CS_CLASSDC | CS_GLOBALCLASS;
  wc.lpfnWndProc   = DefWindowProcW;
  wc.hInstance     = SK_GetDLL ();
  wc.lpszClassName = L"Special K Dummy Window Class 2";

  if (RegisterClassW (&wc))
  {
    HWND hWnd =
      CreateWindowExW_Original ( 0L, L"Special K Dummy Window Class 2",
                                     L"Special K Dummy Window",
                                       WS_POPUP | WS_CLIPCHILDREN |
                                       WS_CLIPSIBLINGS,
                                         0, 0,
                                         1, 1,
                                           HWND_DESKTOP,   nullptr,
                                             SK_GetDLL (), nullptr );

    if (hWnd != HWND_DESKTOP)
      dummy_windows.emplace (hWnd);

    return hWnd;
  }

  else
  {
    dll_log.Log (L"Window Class Registration Failed!");
  }

  return nullptr;
}

void
SK_Win32_CleanupDummyWindow (void)
{
  for (auto& it : dummy_windows)
  {
    if (DestroyWindow (it))
    {
      dummy_windows.erase (it);
    }
  }

  UnregisterClassW ( L"Special K Dummy Window Class", SK_GetDLL () );
}


#include <SpecialK/gpu_monitor.h>

bool
__stdcall
SK_ShutdownCore (const wchar_t* backend)
{
  // Fast path for DLLs that were never really attached.
  extern __time64_t __SK_DLL_AttachTime;

  if (! __SK_DLL_AttachTime)
  {
    SK_UnInit_MinHook ();
    return true;
  }


  dll_log.Log (L"[ SpecialK ] *** Initiating DLL Shutdown ***");
  SK_Win32_CleanupDummyWindow ();



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

  SK_Console::getInstance ()->End ();

  SK::DXGI::ShutdownBudgetThread ();

  dll_log.LogEx    (true, L"[ GPU Stat ] Shutting down Prognostics Thread...          ");

  DWORD dwTime =
       timeGetTime ();
  SK_EndGPUPolling ();

  dll_log.LogEx    (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);

  if (hPumpThread != INVALID_HANDLE_VALUE)
  {
    dll_log.LogEx   (true, L"[ Stat OSD ] Shutting down Pump Thread... ");

    CloseHandle     (hPumpThread);
                     hPumpThread = INVALID_HANDLE_VALUE;

    dll_log.LogEx   (false, L"done!\n");
  }

  auto ShutdownWMIThread =
  []( volatile HANDLE&  hSignal,
      volatile HANDLE&  hThread,
               wchar_t* wszName )
  {
    wchar_t wszFmtName [32] = { };

    lstrcatW (wszFmtName, wszName);
    lstrcatW (wszFmtName, L"...");

    dll_log.LogEx (true, L"[ WMI Perf ] Shutting down %-30s ", wszFmtName);

    DWORD dwTime = timeGetTime ();

    // Signal the thread to shutdown
    SetEvent (hSignal);

    if (SignalObjectAndWait (hSignal, hThread, 1000UL, TRUE) != WAIT_OBJECT_0) // Give 1 second, and
    {                                                                          // then we're killing
      TerminateThread (hThread, 0x00);                                         // the thing!
    }

    CloseHandle (hThread);
                 hThread  = INVALID_HANDLE_VALUE;

    dll_log.LogEx (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);
  };

  ShutdownWMIThread (process_stats.hShutdownSignal,  process_stats.hThread,  L"Process Monitor" );
  ShutdownWMIThread (cpu_stats.hShutdownSignal,      cpu_stats.hThread,      L"CPU Monitor"     );
  ShutdownWMIThread (disk_stats.hShutdownSignal,     disk_stats.hThread,     L"Disk Monitor"    );
  ShutdownWMIThread (pagefile_stats.hShutdownSignal, pagefile_stats.hThread, L"Pagefile Monitor");

  const wchar_t* config_name = backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  if (sk::NVAPI::app_name != L"ds3t.exe" && SK_GetFramesDrawn () > 0)
  {
    dll_log.LogEx        (true,  L"[ SpecialK ] Saving user preferences to %10s.ini... ", config_name);
    dwTime = timeGetTime ();
    SK_SaveConfig        (config_name);
    dll_log.LogEx        (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);
  }


  SK_UnloadImports        ();
  SK::Framerate::Shutdown ();

  dll_log.LogEx        (true, L"[ SpecialK ] Shutting down MinHook...                     ");

  dwTime = timeGetTime ();
  SK_UnInit_MinHook    ();
  dll_log.LogEx        (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);


  dll_log.LogEx        (true, L"[ WMI Perf ] Shutting down WMI WbemLocator...             ");
  dwTime = timeGetTime ();
  SK_ShutdownWMI       ();
  dll_log.LogEx        (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);


  SK_GetCurrentRenderBackend ().releaseOwnedResources ();


  if (nvapi_init)
    sk::NVAPI::UnloadLibrary ();


  dll_log.Log (L"[ SpecialK ] Custom %s.dll Detached (pid=0x%04x)",
    backend, GetCurrentProcessId ());

  //SymCleanup (GetCurrentProcess ());


  // Breakpad Disable Disclaimer; pretend the log was empty :)
  if (crash_log.lines == 1)
    crash_log.lines = 0;

  crash_log.close ();

  config.system.handle_crashes = false;

  InterlockedExchange (&__SK_Init, FALSE);

  return true;
}



__declspec (noinline)
void
STDMETHODCALLTYPE
SK_BeginBufferSwap (void)
{
  // Throttle, but do not deadlock the render loop
  while (ReadNoFence (&SK_bypass_dialog_active) && (GetCurrentThreadId () != ReadAcquire (&SK_bypass_dialog_tid)))
    MsgWaitForMultipleObjectsEx (0, nullptr, 166, QS_ALLINPUT, MWMO_ALERTABLE);

  // ^^^ Use condition variable instead


  // Maybe make this into an option, but for now just get this the hell out of there
  //   almost no software should be shipping with FP exceptions, it causes compatibility problems.
  _controlfp (MCW_EM, MCW_EM);

  ImGuiIO& io =
    ImGui::GetIO ();

  if (io.Fonts == nullptr)
  {
    extern void
    SK_ImGui_LoadFonts (void);

    SK_ImGui_LoadFonts ();
  }


  static int import_tries = 0;

  if (import_tries++ == 0)
  {
  // Load user-defined DLLs (Late)
#ifdef _WIN64
    SK_LoadLateImports64 ();
#else
    SK_LoadLateImports32 ();
#endif

    if (ReadAcquire64 (&SK_SteamAPI_CallbackRunCount) < 1)
    {
      // Steam Init: Better late than never

      SK_TestSteamImports (GetModuleHandle (nullptr));
    }

    if (config.system.handle_crashes)
      SK::Diagnostics::CrashHandler::Reinstall ();
  }


  static volatile LONG first = TRUE;

  if (ReadNoFence (&first))
  {
    if (SK_GetGameWindow () != nullptr)
    {
      extern void SK_ResetWindow ();

      if (InterlockedCompareExchange (&first, FALSE, TRUE))
        SK_ResetWindow ();
    }
  }


  static volatile LONG cegui_init = FALSE;
  static SK_RenderAPI  last_api   = SK_RenderAPI::Reserved;

  if ( (SK_GetCurrentRenderBackend ().api != SK_RenderAPI::Reserved) &&
       ( (! InterlockedCompareExchange (&cegui_init, 1, 0)) ||
          last_api != SK_GetCurrentRenderBackend ().api ) )
  {
    if (config.cegui.enable)
    {
      // Brutally stupid hack for brutally stupid OS (Windows 7)
      //
      //   1. Lock the DLL loader + Suspend all Threads
      //   2. Change Working Dir  + Delay-Load CEGUI DLLs
      //   3. Restore Working Dir
      //   4. Resume all Threads  + Unlock DLL Loader
      //
      //     >> Not necessary if the kernel supports altered DLL serarch
      //          paths <<
      //

      SK_LockDllLoader ();

      // Disable until we validate CEGUI's state
      config.cegui.enable = false;

      wchar_t wszCEGUIModPath [MAX_PATH]        = { };
      wchar_t wszCEGUITestDLL [MAX_PATH]        = { };
      wchar_t wszEnvPath      [ MAX_PATH + 32 ] = { };


  #ifdef _WIN64
      _swprintf (wszCEGUIModPath, L"%sCEGUI\\bin\\x64",   SK_GetRootPath ());

      if (GetFileAttributes (wszCEGUIModPath) == INVALID_FILE_ATTRIBUTES)
      {
        _swprintf ( wszCEGUIModPath, L"%s\\My Mods\\SpecialK\\CEGUI\\bin\\x64",
                      SK_GetDocumentsDir ().c_str () );

        _swprintf (wszEnvPath, L"CEGUI_PARENT_DIR=%s\\My Mods\\SpecialK\\", SK_GetDocumentsDir ().c_str ());
      }

      else
      {
        _swprintf (wszEnvPath, L"CEGUI_PARENT_DIR=%s", SK_GetRootPath ());
      }
  #else
      _swprintf (wszCEGUIModPath, L"%sCEGUI\\bin\\Win32",  SK_GetRootPath ());

      if (GetFileAttributes (wszCEGUIModPath) == INVALID_FILE_ATTRIBUTES)
      {
        _swprintf ( wszCEGUIModPath, L"%s\\My Mods\\SpecialK\\CEGUI\\bin\\Win32",
                      SK_GetDocumentsDir ().c_str () );

        _swprintf (wszEnvPath, L"CEGUI_PARENT_DIR=%s\\My Mods\\SpecialK\\", SK_GetDocumentsDir ().c_str ());
      }
      else
        _swprintf (wszEnvPath, L"CEGUI_PARENT_DIR=%s", SK_GetRootPath ());
  #endif

      _wputenv  (wszEnvPath);

      lstrcatW      (wszCEGUITestDLL, wszCEGUIModPath);
      lstrcatW      (wszCEGUITestDLL, L"\\CEGUIBase-0.dll");

      wchar_t wszWorkingDir [MAX_PATH + 2] = { };

      if (! config.cegui.safe_init)
      {
        //std::queue <DWORD> tids =
        //  SK_SuspendAllOtherThreads ();

        GetCurrentDirectory    (MAX_PATH, wszWorkingDir);
        SetCurrentDirectory    (        wszCEGUIModPath);
      }

      // This is only guaranteed to be supported on Windows 8, but Win7 and Vista
      //   do support it if a certain Windows Update (KB2533623) is installed.
      using AddDllDirectory_pfn          = DLL_DIRECTORY_COOKIE (WINAPI *)(_In_ PCWSTR               NewDirectory);
      using RemoveDllDirectory_pfn       = BOOL                 (WINAPI *)(_In_ DLL_DIRECTORY_COOKIE Cookie);
      using SetDefaultDllDirectories_pfn = BOOL                 (WINAPI *)(_In_ DWORD                DirectoryFlags);

      static auto k32_AddDllDirectory =
        (AddDllDirectory_pfn)
          GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                             "AddDllDirectory" );

      static auto k32_RemoveDllDirectory =
        (RemoveDllDirectory_pfn)
          GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                             "RemoveDllDirectory" );

      static auto k32_SetDefaultDllDirectories =
        (SetDefaultDllDirectories_pfn)
          GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                             "SetDefaultDllDirectories" );

      if ( k32_AddDllDirectory          && k32_RemoveDllDirectory &&
           k32_SetDefaultDllDirectories &&

             GetFileAttributesW (wszCEGUITestDLL) != INVALID_FILE_ATTRIBUTES )
      {
        SK_StripUserNameFromPathW (wszCEGUITestDLL);

        dll_log.Log (L"[  CEGUI   ] Enabling CEGUI: (%s)", wszCEGUITestDLL);

        wchar_t wszEnvVar [ MAX_PATH + 32 ] = { };

        _swprintf (wszEnvVar, L"CEGUI_MODULE_DIR=%s", wszCEGUIModPath);
        _wputenv  (wszEnvVar);

        // This tests for the existence of the DLL before attempting to load it...
        auto DelayLoadDLL = [&](const char* szDLL)->
          bool
            {
              if (! config.cegui.safe_init)
                return SUCCEEDED ( __HrLoadAllImportsForDll (szDLL) );

              k32_SetDefaultDllDirectories (
                LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                LOAD_LIBRARY_SEARCH_SYSTEM32        | LOAD_LIBRARY_SEARCH_USER_DIRS
              );

              DLL_DIRECTORY_COOKIE cookie = nullptr;
              bool                 ret    = false;

              __try
              {
                cookie =               k32_AddDllDirectory    (wszCEGUIModPath);
                ret    = SUCCEEDED ( __HrLoadAllImportsForDll (szDLL)           );
              }

              __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ?
                         EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
              {
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

          if (config.apis.dxgi.d3d11.hook)
          {
            if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
                 static_cast <int> (SK_RenderAPI::D3D11              ) )
            {
              if (DelayLoadDLL ("CEGUIDirect3D11Renderer-0.dll"))
                config.cegui.enable = true;
            }
          }
        }
      }

      if (! config.cegui.safe_init)
      {
        SetCurrentDirectory (wszWorkingDir);
        //SK_ResumeThreads  (tids);
      }

      SK_UnlockDllLoader  ();
    }
  }

  last_api = SK_GetCurrentRenderBackend ().api;


  SK_DrawOSD         ();
  SK_DrawConsole     ();

  static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");

  if (hModTBFix)
  {
    const char* szFirst = "First-frame Done";

    if (SK_Steam_PiratesAhoy () != 0x00)
    {
      extern float target_fps;
      target_fps = static_cast <float> (*reinterpret_cast <const uint8_t *>(szFirst+5));
    }

    SK::Framerate::GetLimiter ()->wait ();
  }

  extern uint32_t WINAPI SK_Steam_PiratesAhoy (void);
  if (SK_Steam_PiratesAhoy () && (! SK_ImGui_Active ()))
  {
    extern void SK_ImGui_Toggle (void);
                SK_ImGui_Toggle ();
  }

  extern void SK_ImGui_PollGamepad_EndFrame (void);
  SK_ImGui_PollGamepad_EndFrame ();
}


extern void SK_ImGui_Toggle (void);

ULONGLONG poll_interval = 0;

void
DoKeyboard (void)
{
  //
  // Do not poll the keyboard while the game window is inactive
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

  static bool toggle_drag = false;

  ImGuiIO& io (ImGui::GetIO ());

  if (io.KeysDown [VK_CONTROL] && io.KeysDown [VK_SHIFT] && io.KeysDown [VK_SCROLL])
  {
    if (! toggle_drag)
      config.window.drag_lock = (! config.window.drag_lock);
    toggle_drag = true;

    if (config.window.drag_lock)
      ClipCursor (nullptr);
  } else {
    toggle_drag = false;
  }

  if (config.window.drag_lock)
    SK_CenterWindowAtMouse (config.window.persistent_drag);


  if (ullNow.QuadPart - last_osd_scale > 25ULL * poll_interval)
  {
    if (io.KeysDown [config.osd.keys.expand [0]] &&
        io.KeysDown [config.osd.keys.expand [1]] &&
        io.KeysDown [config.osd.keys.expand [2]])
    {
      last_osd_scale = ullNow.QuadPart;
      SK_ResizeOSD (+0.1f);
    }

    if (io.KeysDown [config.osd.keys.shrink [0]] &&
        io.KeysDown [config.osd.keys.shrink [1]] &&
        io.KeysDown [config.osd.keys.shrink [2]])
    {
      last_osd_scale = ullNow.QuadPart;
      SK_ResizeOSD (-0.1f);
    }
  }

#if 0
  if (ullNow.QuadPart < last_poll + poll_interval)
  {
    SleepEx (10, TRUE);
    last_poll = ullNow.QuadPart;
  }
#endif

  static bool toggle_time = false;
  if (io.KeysDown [config.time.keys.toggle [0]] &&
      io.KeysDown [config.time.keys.toggle [1]] &&
      io.KeysDown [config.time.keys.toggle [2]])
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
  if (io.KeysDown [config.mem.keys.toggle [0]] &&
      io.KeysDown [config.mem.keys.toggle [1]] &&
      io.KeysDown [config.mem.keys.toggle [2]])
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
  if (io.KeysDown [config.load_balance.keys.toggle [0] &&
      io.KeysDown [config.load_balance.keys.toggle [1] &&
      io.KeysDown [config.load_balance.keys.toggle [2])
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
    if (io.KeysDown [config.sli.keys.toggle [0]] &&
        io.KeysDown [config.sli.keys.toggle [1]] &&
        io.KeysDown [config.sli.keys.toggle [2]])
    {
      if (! toggle_sli)
        config.sli.show = (! config.sli.show);
      toggle_sli = true;
    }

    else
    {
      toggle_sli = false;
    }
  }

  static bool toggle_io = false;
  if (io.KeysDown [config.io.keys.toggle [0]] &&
      io.KeysDown [config.io.keys.toggle [1]] &&
      io.KeysDown [config.io.keys.toggle [2]])
  {
    if (! toggle_io)
      config.io.show = (! config.io.show);
    toggle_io = true;
  }

  else
  {
    toggle_io = false;
  }

  static bool toggle_cpu = false;
  if (io.KeysDown [config.cpu.keys.toggle [0]] &&
      io.KeysDown [config.cpu.keys.toggle [1]] &&
      io.KeysDown [config.cpu.keys.toggle [2]])
  {
    if (! toggle_cpu)
    {
      config.cpu.show = (! config.cpu.show);

      if (config.cpu.show)
        SK_StartPerfMonThreads ();
    }

    toggle_cpu = true;
  }

  else
  {
    toggle_cpu = false;
  }

  static bool toggle_gpu = false;
  if (io.KeysDown [config.gpu.keys.toggle [0]] &&
      io.KeysDown [config.gpu.keys.toggle [1]] &&
      io.KeysDown [config.gpu.keys.toggle [2]])
  {
    if (! toggle_gpu)
      config.gpu.show = (! config.gpu.show);
    toggle_gpu = true;
  }

  else
  {
    toggle_gpu = false;
  }

  static bool toggle_fps = false;
  if (io.KeysDown [config.fps.keys.toggle [0]] &&
      io.KeysDown [config.fps.keys.toggle [1]] &&
      io.KeysDown [config.fps.keys.toggle [2]])
  {
    if (! toggle_fps)
      config.fps.show = (! config.fps.show);
    toggle_fps = true;
  }

  else
  {
    toggle_fps = false;
  }

  static bool toggle_disk = false;
  if (io.KeysDown [config.disk.keys.toggle [0]] &&
      io.KeysDown [config.disk.keys.toggle [1]] &&
      io.KeysDown [config.disk.keys.toggle [2]] &&
      io.KeysDown [config.disk.keys.toggle [3]])
  {
    if (! toggle_disk)
    {
      config.disk.show = (! config.disk.show);

      if (config.disk.show)
        SK_StartPerfMonThreads ();
    }

    toggle_disk = true;
  }

  else
  {
    toggle_disk = false;
  }

  static bool toggle_pagefile = false;
  if (io.KeysDown [config.pagefile.keys.toggle [0]] &&
      io.KeysDown [config.pagefile.keys.toggle [1]] &&
      io.KeysDown [config.pagefile.keys.toggle [2]] &&
      io.KeysDown [config.pagefile.keys.toggle [3]])
  {
    if (! toggle_pagefile)
    {
      config.pagefile.show = (! config.pagefile.show);

      if (config.pagefile.show)
        SK_StartPerfMonThreads ();
    }

    toggle_pagefile = true;
  } else {
    toggle_pagefile = false;
  }

  static bool toggle_osd = false;
  if (io.KeysDown [config.osd.keys.toggle [0]] &&
      io.KeysDown [config.osd.keys.toggle [1]] &&
      io.KeysDown [config.osd.keys.toggle [2]])
  {
    if (! toggle_osd)
    {
      config.osd.show = (! config.osd.show);

      if (config.osd.show)
        SK_InstallOSD ();
    }
    toggle_osd = true;
  }

  else
  {
    toggle_osd = false;
  }

  static bool toggle_render = false;
  if (io.KeysDown [config.render.keys.toggle [0]] &&
      io.KeysDown [config.render.keys.toggle [1]] &&
      io.KeysDown [config.render.keys.toggle [2]])
  {
    if (! toggle_render)
      config.render.show = (! config.render.show);
    toggle_render = true;
  }

  else
  {
    toggle_render = false;
  }
}

IUnknown* g_iRenderDevice = nullptr;
IUnknown* g_iSwapChain    = nullptr;



__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
SK_EndBufferSwap (HRESULT hr, IUnknown* device)
{
  extern SK_RenderBackend __SK_RBkEnd;

  __SK_RBkEnd.thread = GetCurrentThreadId ();

  if (device != nullptr)
  {
    CComPtr <IDirect3DDevice9>   pDev9   = nullptr;
    CComPtr <IDirect3DDevice9Ex> pDev9Ex = nullptr;
    CComPtr <ID3D11Device>       pDev11  = nullptr;
#ifdef _WIN64
    CComPtr <ID3D12Device>       pDev12  = nullptr;
#endif

    if (SUCCEEDED (device->QueryInterface <IDirect3DDevice9Ex> (&pDev9Ex)))
    {
      reinterpret_cast <int &> (__SK_RBkEnd.api) =
        ( static_cast <int> (SK_RenderAPI::D3D9  ) |
          static_cast <int> (SK_RenderAPI::D3D9Ex)  );

      wcsncpy (__SK_RBkEnd.name, L"D3D9Ex", 8);
    }

    else if (SUCCEEDED (device->QueryInterface <IDirect3DDevice9> (&pDev9)))
    {
               __SK_RBkEnd.api  = SK_RenderAPI::D3D9;
      wcsncpy (__SK_RBkEnd.name, L"D3D9  ", 8);
    }

    else if (SUCCEEDED (device->QueryInterface <ID3D11Device> (&pDev11)))
    {
      // Establish the API used this frame (and handle possible translation layers)
      //
      switch (SK_GetDLLRole ())
      {
        case DLL_ROLE::D3D8:
          __SK_RBkEnd.api = SK_RenderAPI::D3D8On11;
          break;
        case DLL_ROLE::DDraw:
          __SK_RBkEnd.api = SK_RenderAPI::DDrawOn11;
          break;
        default:
          __SK_RBkEnd.api = SK_RenderAPI::D3D11;
          break;
      }

      IUnknown* pTest = nullptr;

      if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device5,        (void **)&pTest))) {
        wcsncpy (__SK_RBkEnd.name, L"D3D11.4", 8);
        pTest->Release ();
      } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device4, (void **)&pTest))) {
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

      if (SK_GetDLLRole () == DLL_ROLE::D3D8)
      {
        wcscpy (__SK_RBkEnd.name, L"D3D8");
      }

      else if (SK_GetDLLRole () == DLL_ROLE::DDraw)
      {
        wcscpy (__SK_RBkEnd.name, L"DDraw");
      }


      if ( static_cast <int> (__SK_RBkEnd.api)  &
           static_cast <int> (SK_RenderAPI::D3D11) )
      {
        BOOL fullscreen = FALSE;

        CComPtr                                 <IDXGISwapChain>   pSwapChain = nullptr;
        if (__SK_RBkEnd.swapchain)
          __SK_RBkEnd.swapchain->QueryInterface <IDXGISwapChain> (&pSwapChain);

        if ( pSwapChain &&
  SUCCEEDED (pSwapChain->GetFullscreenState (&fullscreen, nullptr)) )
        {
          __SK_RBkEnd.fullscreen_exclusive = fullscreen;
        }

        else
          __SK_RBkEnd.fullscreen_exclusive = false;
      }

      extern void SK_D3D11_EndFrame (void);
                  SK_D3D11_EndFrame ();
#ifdef _WIN64
    }


    else if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev12))))
    {
               __SK_RBkEnd.api  = SK_RenderAPI::D3D12;
      wcsncpy (__SK_RBkEnd.name, L"D3D12 ", 8);
#endif
    }

    else
    {
               __SK_RBkEnd.api  = SK_RenderAPI::Reserved;
      wcsncpy (__SK_RBkEnd.name, L"UNKNOWN", 8);
    }
  }

  else
  {
    if (config.apis.OpenGL.hook && SK_GetCurrentGLContext () != nullptr)
    {
               __SK_RBkEnd.api  = SK_RenderAPI::OpenGL;
      wcsncpy (__SK_RBkEnd.name, L"OpenGL", 8);
    }
  }

  config.apis.last_known =
    __SK_RBkEnd.api;

  static volatile
    ULONG budget_init = FALSE;

  if (! InterlockedCompareExchange (&budget_init, TRUE, FALSE))
  {
    SK::DXGI::StartBudgetThread_NoAdapter ();
  }


  DoKeyboard ();

  InterlockedIncrement (&frames_drawn);

  if (config.sli.show && device != nullptr)
  {
    // Get SLI status for the frame we just displayed... this will show up
    //   one frame late, but this is the safest approach.
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0)
    {
      sli_state = sk::NVAPI::GetSLIState (device);
    }
  }

  static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");
  static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");

  //
  // TZFix has its own limiter
  //
  if (! (hModTZFix || hModTBFix))
  {
    SK::Framerate::GetLimiter ()->wait ();
  }

  return hr;
}

DLL_ROLE dll_role = DLL_ROLE::INVALID;

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
//   the Steam client throwing a temper tantrum.
void
CALLBACK
RunDLL_ElevateMe ( HWND  hwnd,        HINSTANCE hInst,
                   LPSTR lpszCmdLine, int       nCmdShow )
{
  UNREFERENCED_PARAMETER (hInst);

  ShellExecuteA ( hwnd, "runas", lpszCmdLine, nullptr, nullptr, nCmdShow );
}

void
CALLBACK
RunDLL_RestartGame ( HWND  hwnd,        HINSTANCE hInst,
                     LPSTR lpszCmdLine, int       nCmdShow )
{
  UNREFERENCED_PARAMETER (hInst);

  ShellExecuteA ( hwnd, "open", lpszCmdLine, nullptr, nullptr, nCmdShow );
}



SK_ImGui_WidgetRegistry SK_ImGui_Widgets;