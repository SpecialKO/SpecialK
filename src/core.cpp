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

#include <ShlObj.h>
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
#include <SpecialK/vulkan_backend.h>

#include <atlbase.h>
#include <comdef.h>

#include <delayimp.h>

#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>
#include <CEGUI/Logger.h>


static const GUID IID_ID3D11Device2 = { 0x9d06dffa, 0xd1e5, 0x4d07, { 0x83, 0xa8, 0x1b, 0xb1, 0x23, 0xf2, 0xf8, 0x41 } };
static const GUID IID_ID3D11Device3 = { 0xa05c8c37, 0xd2c6, 0x4732, { 0xb3, 0xa0, 0x9c, 0xe0, 0xb0, 0xdc, 0x9a, 0xe6 } };
static const GUID IID_ID3D11Device4 = { 0x8992ab71, 0x02e6, 0x4b8d, { 0xba, 0x48, 0xb0, 0x56, 0xdc, 0xda, 0x42, 0xc4 } };
static const GUID IID_ID3D11Device5 = { 0x8ffde202, 0xa0e7, 0x45df, { 0x9e, 0x01, 0xe8, 0x37, 0x80, 0x1b, 0x5e, 0xa0 } };

volatile HANDLE  hInitThread   = { INVALID_HANDLE_VALUE };
         HANDLE  hPumpThread   = { INVALID_HANDLE_VALUE };

// Disable SLI memory in Batman Arkham Knight
bool USE_SLI = true;

NV_GET_CURRENT_SLI_STATE sli_state;
BOOL                     nvapi_init       = FALSE;
HMODULE                  backend_dll      = 0;
const wchar_t*           __SK_DLL_Backend = nullptr;

// LOL -- This tests for RunDLL32 / SKIM
bool
__cdecl
SK_IsSuperSpecialK (void);


struct init_params_t {
  const wchar_t* backend;
  void*          callback;
} init_;


std::queue <DWORD> __SK_Init_Suspended_tids;

wchar_t SK_RootPath   [MAX_PATH + 2] = { };
wchar_t SK_ConfigPath [MAX_PATH + 2] = { };
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

#include <d3d9.h>
#include <d3d11.h>
#ifdef _WIN64
#define D3D12_IGNORE_SDK_LAYERS
#include <SpecialK/d3d12_interfaces.h>
#endif

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

// Stupid solution for games that inexplicibly draw to the screen
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

#include <SpecialK/io_monitor.h>

void
__stdcall
SK_StartPerfMonThreads (void)
{
  if (config.mem.show)
  {
    //
    // Spawn Process Monitor Thread
    //
    if (process_stats.hThread == INVALID_HANDLE_VALUE)
    {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Process Monitor...  ");

      process_stats.hThread = 
        CreateThread ( nullptr,
                         0,
                           SK_MonitorProcess,
                             nullptr,
                               0,
                                 nullptr );

      if (process_stats.hThread != INVALID_HANDLE_VALUE)
        dll_log.LogEx (false, L"tid=0x%04x\n", GetThreadId (process_stats.hThread));
      else
        dll_log.LogEx (false, L"Failed!\n");
    }
  }

  if (config.cpu.show)
  {
    //
    // Spawn CPU Refresh Thread
    //
    if (cpu_stats.hThread == INVALID_HANDLE_VALUE)
    {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning CPU Monitor...      ");

      cpu_stats.hThread = 
        CreateThread ( nullptr,
                         0,
                           SK_MonitorCPU,
                             nullptr,
                               0,
                                 nullptr );

      if (cpu_stats.hThread != INVALID_HANDLE_VALUE)
        dll_log.LogEx (false, L"tid=0x%04x\n", GetThreadId (cpu_stats.hThread));
      else
        dll_log.LogEx (false, L"Failed!\n");
    }
  }

  if (config.disk.show)
  {
    if (disk_stats.hThread == INVALID_HANDLE_VALUE)
    {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Disk Monitor...     ");

      disk_stats.hThread =
        CreateThread ( nullptr,
                         0,
                           SK_MonitorDisk,
                             nullptr,
                               0,
                                 nullptr );

      if (disk_stats.hThread != INVALID_HANDLE_VALUE)
        dll_log.LogEx (false, L"tid=0x%04x\n", GetThreadId (disk_stats.hThread));
      else
        dll_log.LogEx (false, L"failed!\n");
    }
  }

  if (config.pagefile.show)
  {
    if (pagefile_stats.hThread == INVALID_HANDLE_VALUE)
    {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Pagefile Monitor... ");

      pagefile_stats.hThread =
        CreateThread ( nullptr,
                         0,
                           SK_MonitorPagefile,
                             nullptr,
                               0,
                                 nullptr );

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
    base_addr = (uint8_t *)GetModuleHandle (nullptr);

    MEMORY_BASIC_INFORMATION basic_mem_info;
    VirtualQuery (base_addr, &basic_mem_info, sizeof basic_mem_info);

    base_addr = (uint8_t *)basic_mem_info.BaseAddress;
  }

  addr += (uintptr_t)base_addr;

  char result [512] = { };

  switch (type) {
    case 'b':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 1, PAGE_EXECUTE_READWRITE, &dwOld);
          uint8_t out;
          sscanf (val, "%cx", &out);
          *(uint8_t *)addr = out;
        VirtualProtect ((LPVOID)addr, 1, dwOld, &dwOld);
      }

      sprintf (result, "%u", *(uint8_t *)addr);

      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
    case 's':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 2, PAGE_EXECUTE_READWRITE, &dwOld);
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

        VirtualProtect ((LPVOID)addr, 4, PAGE_EXECUTE_READWRITE, &dwOld);
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

        VirtualProtect ((LPVOID)addr, 8, PAGE_EXECUTE_READWRITE, &dwOld);
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

        VirtualProtect ((LPVOID)addr, 8, PAGE_EXECUTE_READWRITE, &dwOld);
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

        VirtualProtect ((LPVOID)addr, 4, PAGE_EXECUTE_READWRITE, &dwOld);
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

        VirtualProtect ((LPVOID)addr, 256, PAGE_EXECUTE_READWRITE, &dwOld);
          strcpy ((char *)addr, val);
        VirtualProtect ((LPVOID)addr, 256, dwOld, &dwOld);
      }
      sprintf (result, "%s", (char *)addr);
      return SK_ICommandResult ("mem", szArgs, result, 1);
      break;
  }

  return SK_ICommandResult ("mem", szArgs);
}

DWORD
WINAPI
CheckVersionThread (LPVOID user);

volatile LONG    SK_bypass_dialog_active = FALSE;
volatile ULONG __SK_Init                 = FALSE;

void
__stdcall
SK_InitFinishCallback (void)
{
  //
  // TEMP HACK: dgVoodoo2
  //
  if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    SK_BootDXGI ();
  else if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    SK_BootDXGI ();

  SK_DeleteTemporaryFiles ();
  SK_DeleteTemporaryFiles (L"Version", L"*.old");

  SK::Framerate::Init ();

  dll_log.Log (L"[ SpecialK ] === Initialization Finished! ===");

  if (SK_IsSuperSpecialK ())
  {
    LeaveCriticalSection (&init_mutex);
    return;
  }

  SK_Input_Init ();

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

  else if (! lstrcmpW (SK_GetHostApp (), L"NieRAutomata.exe"))
    SK_FAR_InitPlugin ();

  else if (! lstrcmpW (SK_GetHostApp (), L"RiME.exe"))
    SK_REASON_InitPlugin ();

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

  const wchar_t* config_name = init_.backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  SK_SaveConfig (config_name);

  SK_Console* pConsole = SK_Console::getInstance ();
  pConsole->Start ();

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


  LeaveCriticalSection (&init_mutex);
}

void
__stdcall
SK_InitCore (const wchar_t* backend, void* callback)
{
  EnterCriticalSection (&init_mutex);

  wcscpy (SK_Backend, backend);

  typedef void (WINAPI *finish_pfn)  (void);
  typedef void (WINAPI *callback_pfn)(_Releases_exclusive_lock_ (init_mutex) finish_pfn);

  callback_pfn callback_fn =
    (callback_pfn)callback;

  if (backend_dll != NULL)
  {
    LeaveCriticalSection (&init_mutex);
    SK_ResumeThreads     (__SK_Init_Suspended_tids);
    return;
  }

  if (! lstrcmpW (SK_GetHostApp (), L"BatmanAK.exe"))
    USE_SLI = false;

  std::wstring   module_name   = SK_GetModuleName (SK_GetDLL ());
  const wchar_t* wszModuleName = module_name.c_str ();

  dll_log.LogEx (false,
    L"----------------------------------------------------------------------"
    L"---------------------\n");

  // If the module name is this, then we need to load the system-wide DLL...
  wchar_t   wszProxyName [MAX_PATH];
  wsprintf (wszProxyName, L"%s.dll", backend);


#ifndef _WIN64
  //
  // TEMP HACK: dgVoodoo
  //
  if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    wsprintf (wszProxyName, L"%s\\PlugIns\\ThirdParty\\dgVoodoo\\d3d8.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());
  else if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    wsprintf (wszProxyName, L"%s\\PlugIns\\ThirdParty\\dgVoodoo\\ddraw.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());
#endif


  wchar_t wszBackendDLL [MAX_PATH] = { };
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

  dll_log.Log (L" Working Directory:          %s", wszWorkDir);
  dll_log.Log (L" System Directory:           %s", wszBackendDLL);

  lstrcatW (wszBackendDLL, L"\\");
  lstrcatW (wszBackendDLL, backend);
  lstrcatW (wszBackendDLL, L".dll");

  const wchar_t* dll_name = wszBackendDLL;

  if (! SK_Path_wcsicmp (wszProxyName, wszModuleName))
    dll_name = wszBackendDLL;
  else
    dll_name = wszProxyName;

  bool load_proxy = false;

  if (! SK_IsInjected ())
  {
    for (int i = 0; i < SK_MAX_IMPORTS; i++)
    {
      if (imports [i].role != nullptr && imports [i].role->get_value () == backend)
      {
        dll_log.LogEx (true, L" Loading proxy %s.dll:    ", backend);
        dll_name   = _wcsdup (imports [i].filename->get_value ().c_str ());
        load_proxy = true;
        break;
      }
    }
  }

  if (! load_proxy)
    dll_log.LogEx (true, L" Loading default %s.dll: ", backend);

  // Pre-Load the original DLL into memory
  if (dll_name != wszBackendDLL)
  {
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


  dll_log.LogEx (true, L"[  NvAPI   ] Initializing NVIDIA API          (NvAPI): ");

  nvapi_init = sk::NVAPI::InitializeLibrary (SK_GetHostApp ());

  dll_log.LogEx (false, L" %s\n", nvapi_init ? L"Success" : L"Failed");

  if (nvapi_init)
  {
    int num_sli_gpus = sk::NVAPI::CountSLIGPUs ();

    dll_log.Log (L"[  NvAPI   ] >> NVIDIA Driver Version: %s",
      sk::NVAPI::GetDriverVersion ().c_str ());

    dll_log.Log (L"[  NvAPI   ]  * Number of Installed NVIDIA GPUs: %i "
      L"(%i are in SLI mode)",
      sk::NVAPI::CountPhysicalGPUs (), num_sli_gpus);

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
                           NULL,
                             NULL,
                               SW_SHOWDEFAULT );
      exit (0);
    }
  }

  else
  {
    dll_log.LogEx (true, L"[DisplayLib] Initializing AMD Display Library (ADL):   ");

    BOOL adl_init = SK_InitADL ();

    dll_log.LogEx (false, L" %s\n", adl_init ? L"Success" : L"Failed");

    if (adl_init)
    {
      dll_log.Log ( L"[DisplayLib]  * Number of Reported AMD Adapters: %i (%i active)",
                      SK_ADL_CountPhysicalGPUs (),
                        SK_ADL_CountActiveGPUs () );
    }
  }

  HMODULE hMod = GetModuleHandle (SK_GetHostApp ());

  if (hMod != NULL)
  {
    DWORD* dwOptimus = (DWORD *)GetProcAddress (hMod, "NvOptimusEnablement");

    if (dwOptimus != NULL)
    {
      dll_log.Log (L"[Hybrid GPU]  NvOptimusEnablement..................: 0x%02X (%s)",
        *dwOptimus,
        (*dwOptimus & 0x1 ? L"Max Perf." :
          L"Don't Care"));
    }

    else
    {
      dll_log.Log (L"[Hybrid GPU]  NvOptimusEnablement..................: UNDEFINED");
    }

    DWORD* dwPowerXpress =
      (DWORD *)GetProcAddress (hMod, "AmdPowerXpressRequestHighPerformance");

    if (dwPowerXpress != NULL)
    {
      dll_log.Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: 0x%02X (%s)",
        *dwPowerXpress,
        (*dwPowerXpress & 0x1 ? L"High Perf." :
          L"Don't Care"));
    }

    else
      dll_log.Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: UNDEFINED");
  }

  SK_ResumeThreads (__SK_Init_Suspended_tids);
         callback_fn (SK_InitFinishCallback);


  if (! InterlockedCompareExchange (&SK_bypass_dialog_active, 0, 0))
  {
    // Setup the compatibility backend, which monitors loaded libraries,
    //   blacklists bad DLLs and detects render APIs...
    SK_EnumLoadedModules (SK_ModuleEnum::PostLoad);
  }
}


void
WaitForInit (void)
{
  if (InterlockedCompareExchange (&__SK_Init, FALSE, FALSE))
    return;

  while (InterlockedCompareExchangePointer ((LPVOID *)&hInitThread, nullptr, nullptr) != INVALID_HANDLE_VALUE)
  {
    if ( WAIT_OBJECT_0 == WaitForSingleObject (
      InterlockedCompareExchangePointer ((LPVOID *)&hInitThread, nullptr, nullptr),
        150 ) )
      break;
  }

  while (InterlockedCompareExchange (&SK_bypass_dialog_active, FALSE, FALSE))
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
    CloseHandle (InterlockedExchangePointer ((LPVOID *)&hInitThread, INVALID_HANDLE_VALUE));

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

typedef HWND (WINAPI *CreateWindowW_pfn)(
  _In_opt_ LPCWSTR   lpClassName,
  _In_opt_ LPCWSTR   lpWindowName,
  _In_     DWORD     dwStyle,
  _In_     int       x,
  _In_     int       y,
  _In_     int       nWidth,
  _In_     int       nHeight,
  _In_opt_ HWND      hWndParent,
  _In_opt_ HMENU     hMenu,
  _In_opt_ HINSTANCE hInstance,
  _In_opt_ LPVOID    lpParam
);

typedef HWND (WINAPI *CreateWindowA_pfn)(
  _In_opt_ LPCSTR    lpClassName,
  _In_opt_ LPCSTR    lpWindowName,
  _In_     DWORD     dwStyle,
  _In_     int       x,
  _In_     int       y,
  _In_     int       nWidth,
  _In_     int       nHeight,
  _In_opt_ HWND      hWndParent,
  _In_opt_ HMENU     hMenu,
  _In_opt_ HINSTANCE hInstance,
  _In_opt_ LPVOID    lpParam
);

typedef HWND (WINAPI *CreateWindowExW_pfn)(
  _In_     DWORD     dwExStyle,
  _In_opt_ LPCWSTR   lpClassName,
  _In_opt_ LPCWSTR   lpWindowName,
  _In_     DWORD     dwStyle,
  _In_     int       x,
  _In_     int       y,
  _In_     int       nWidth,
  _In_     int       nHeight,
  _In_opt_ HWND      hWndParent,
  _In_opt_ HMENU     hMenu,
  _In_opt_ HINSTANCE hInstance,
  _In_opt_ LPVOID    lpParam
);

typedef HWND (WINAPI *CreateWindowExA_pfn)(
  _In_     DWORD     dwExStyle,
  _In_opt_ LPCSTR    lpClassName,
  _In_opt_ LPCSTR    lpWindowName,
  _In_     DWORD     dwStyle,
  _In_     int       x,
  _In_     int       y,
  _In_     int       nWidth,
  _In_     int       nHeight,
  _In_opt_ HWND      hWndParent,
  _In_opt_ HMENU     hMenu,
  _In_opt_ HINSTANCE hInstance,
  _In_opt_ LPVOID    lpParam
);

CreateWindowW_pfn    CreateWindowW_Original   = nullptr;
CreateWindowA_pfn    CreateWindowA_Original   = nullptr;
CreateWindowExW_pfn  CreateWindowExW_Original = nullptr;
CreateWindowExA_pfn  CreateWindowExA_Original = nullptr;

HWND
WINAPI
CreateWindowW_Detour (
  _In_opt_ LPCWSTR   lpClassName,
  _In_opt_ LPCWSTR   lpWindowName,
  _In_     DWORD     dwStyle,
  _In_     int       x,
  _In_     int       y,
  _In_     int       nWidth,
  _In_     int       nHeight,
  _In_opt_ HWND      hWndParent,
  _In_opt_ HMENU     hMenu,
  _In_opt_ HINSTANCE hInstance,
  _In_opt_ LPVOID    lpParam )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    if (InterlockedCompareExchange (&SK_bypass_dialog_active, 0, 0))
      MsgWaitForMultipleObjectsEx (0, nullptr, 0, QS_ALLINPUT, MWMO_ALERTABLE);
  }

  return CreateWindowW_Original ( lpClassName, lpWindowName, dwStyle,
                                    x, y, nWidth, nHeight,
                                      hWndParent, hMenu, hInstance,
                                        lpParam );
}

HWND
WINAPI
CreateWindowA_Detour (
  _In_opt_ LPCSTR    lpClassName,
  _In_opt_ LPCSTR    lpWindowName,
  _In_     DWORD     dwStyle,
  _In_     int       x,
  _In_     int       y,
  _In_     int       nWidth,
  _In_     int       nHeight,
  _In_opt_ HWND      hWndParent,
  _In_opt_ HMENU     hMenu,
  _In_opt_ HINSTANCE hInstance,
  _In_opt_ LPVOID    lpParam)
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    if (InterlockedCompareExchange (&SK_bypass_dialog_active, 0, 0))
      MsgWaitForMultipleObjectsEx (0, nullptr, 0, QS_ALLINPUT, MWMO_ALERTABLE);
  }

  return CreateWindowA_Original ( lpClassName, lpWindowName, dwStyle,
                                    x, y, nWidth, nHeight,
                                      hWndParent, hMenu, hInstance,
                                        lpParam );
}

HWND
WINAPI
CreateWindowExW_Detour (
  _In_     DWORD     dwExStyle,
  _In_opt_ LPCWSTR   lpClassName,
  _In_opt_ LPCWSTR   lpWindowName,
  _In_     DWORD     dwStyle,
  _In_     int       x,
  _In_     int       y,
  _In_     int       nWidth,
  _In_     int       nHeight,
  _In_opt_ HWND      hWndParent,
  _In_opt_ HMENU     hMenu,
  _In_opt_ HINSTANCE hInstance,
  _In_opt_ LPVOID    lpParam )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    if (InterlockedCompareExchange (&SK_bypass_dialog_active, 0, 0))
      MsgWaitForMultipleObjectsEx (0, nullptr, 0, QS_ALLINPUT, MWMO_ALERTABLE);
  }

  return CreateWindowExW_Original ( dwExStyle,
                                      lpClassName, lpWindowName,
                                        dwStyle,
                                          x, y, nWidth, nHeight,
                                            hWndParent, hMenu, hInstance,
                                              lpParam );
}

HWND
WINAPI
CreateWindowExA_Detour (
  _In_     DWORD     dwExStyle,
  _In_opt_ LPCSTR    lpClassName,
  _In_opt_ LPCSTR    lpWindowName,
  _In_     DWORD     dwStyle,
  _In_     int       x,
  _In_     int       y,
  _In_     int       nWidth,
  _In_     int       nHeight,
  _In_opt_ HWND      hWndParent,
  _In_opt_ HMENU     hMenu,
  _In_opt_ HINSTANCE hInstance,
  _In_opt_ LPVOID    lpParam )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    if (InterlockedCompareExchange (&SK_bypass_dialog_active, 0, 0))
      MsgWaitForMultipleObjectsEx (0, nullptr, 0, QS_ALLINPUT, MWMO_ALERTABLE);
  }

  return CreateWindowExA_Original ( dwExStyle,
                                      lpClassName, lpWindowName,
                                        dwStyle,
                                          x, y, nWidth, nHeight,
                                            hWndParent, hMenu, hInstance,
                                              lpParam );
}

DWORD
WINAPI
CheckVersionThread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  InterlockedIncrement (&SK_bypass_dialog_active);

  // If a local repository is present, use that.
  if (GetFileAttributes (L"Version\\installed.ini") == INVALID_FILE_ATTRIBUTES)
  {
    if (SK_FetchVersionInfo (L"SpecialK"))
    {
      // ↑ Check, but ↓ don't update unless running the global injector version
      if ( (SK_IsInjected () && (! SK_IsSuperSpecialK ())) )
      {
        SK_UpdateSoftware (L"SpecialK");
      }
    }
  }

  InterlockedDecrement (&SK_bypass_dialog_active);

  CloseHandle (GetCurrentThread ());

  return 0;
}

DWORD
WINAPI
DllThread (LPVOID user)
{
  init_params_t* params =
    (init_params_t *)user;

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

#include <wingdi.h>
#include <gl/gl.h>

extern std::pair <std::queue <DWORD>, BOOL> __stdcall SK_BypassInject (void);


// True if the user has rebased their %UserProfile% directory the wrong way
static bool    altered_user_profile     = false;
static wchar_t wszProfile    [MAX_PATH] = { };
static wchar_t wszDocs       [MAX_PATH] = { };
static wchar_t wszEnvProfile [MAX_PATH] = { };
static wchar_t wszEnvDocs    [MAX_PATH] = { };

void
__stdcall
SK_EstablishRootPath (void)
{
  wchar_t wszConfigPath [MAX_PATH + 1] = { };
          wszConfigPath [  MAX_PATH  ] = { };

          SK_RootPath   [    0     ]   = { };

          SK_RootPath   [ MAX_PATH ]   = { };

  // Make expansion of %UserProfile% agree with the actual directory, for people
  //   who rebase their documents directory without fixing this env. variable.
  if (SUCCEEDED ( SHGetFolderPath ( nullptr, CSIDL_PROFILE, nullptr, 0, wszProfile ) ) )
  {
    ExpandEnvironmentStringsW (L"%USERPROFILE%", wszEnvProfile, MAX_PATH);
    SetEnvironmentVariableW   (L"USERPROFILE",   wszProfile);

    if (_wcsicmp (wszEnvProfile, wszProfile))
      altered_user_profile = true;

    ExpandEnvironmentStringsW (L"%USERPROFILE%\\Documents", wszEnvDocs, MAX_PATH);

    if (SUCCEEDED ( SHGetFolderPath ( nullptr, CSIDL_MYDOCUMENTS, nullptr, 0, wszDocs ) ) )
    {
      if (_wcsicmp (wszEnvDocs, wszDocs))
        altered_user_profile = true;
    }
  }


  if (config.system.central_repository)
  {
    if (! SK_IsSuperSpecialK ())
    {
#if 0
      if (SK_IsInjected ()) {
        wchar_t *wszPath = wcsdup (SK_GetModuleFullName (SK_GetDLL ()).c_str ());
        PathRemoveFileSpec (wszPath);
        wcsncpy ( SK_RootPath,
                    wszPath,
                      MAX_PATH - 1 );
        free (wszPath);
      }
      else
#endif
      {
        wcsncpy ( SK_RootPath,
                    std::wstring ( SK_GetDocumentsDir () + L"\\My Mods\\SpecialK" ).c_str (),
                      MAX_PATH - 1 );
      }
    }

    else
    {
      GetCurrentDirectory (MAX_PATH, SK_RootPath);
    }

    lstrcatW (wszConfigPath, SK_GetRootPath ());
    lstrcatW (wszConfigPath, L"\\Profiles\\");
    lstrcatW (wszConfigPath, SK_GetHostApp  ());
  }

  else
  {
    if (! SK_IsSuperSpecialK ())
    {
      lstrcatW (SK_RootPath,   SK_GetHostPath ());
    }

    else
    {
      GetCurrentDirectory (MAX_PATH, SK_RootPath);
    }

    lstrcatW (wszConfigPath, SK_GetRootPath ());
  }

  lstrcatW (SK_RootPath,   L"\\");
  lstrcatW (wszConfigPath, L"\\");

  SK_SetConfigPath     (wszConfigPath);
}

bool
SK_InitWMI (void);

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


  bool bypass = false;

  // Injection Compatibility Menu
  if ( GetAsyncKeyState (VK_SHIFT) &&
       GetAsyncKeyState (VK_CONTROL) )
  {
    SK_BypassInject ();

    //bypass = true;
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
      return true;
    }

    // For the global injector, when not started by SKIM, check its version
    //if ( (SK_IsInjected () && (! SK_IsSuperSpecialK ())) )
      CreateThread (nullptr, 0, CheckVersionThread, nullptr, 0x00, nullptr);
  }

  // Don't let Steam prevent me from attaching a debugger at startup
  game_debug.init                  (L"logs/game_output.log", L"w");
  game_debug.lockless = true;
  SK::Diagnostics::Debugger::Allow ();

  EnterCriticalSection (&init_mutex);

  ZeroMemory (&init_, sizeof init_params_t);

  init_.backend  = _wcsdup (backend);
  init_.callback =          callback;

  wchar_t log_fname [MAX_PATH];
          log_fname [MAX_PATH - 1] = L'\0';

  swprintf (log_fname, L"logs/%s.log", SK_IsInjected () ? L"SpecialK" : backend);

  dll_log.init (log_fname, L"w");
  dll_log.Log  (L"%s.log created",     SK_IsInjected () ? L"SpecialK" : backend);


  if (SK_IsSuperSpecialK ())
  {
    LeaveCriticalSection (&init_mutex);
    return TRUE;
  }


  SK::Diagnostics::CrashHandler::InitSyms ();


  if (config.steam.preload_overlay)
  {
    extern bool SK_Steam_LoadOverlayEarly (void);
    SK_Steam_LoadOverlayEarly ();
  }

  extern void SK_Input_PreInit (void); 
  SK_Input_PreInit    (); // Hook only symbols in user32 and kernel32


  budget_log.init ( L"logs\\dxgi_budget.log", L"w" );

  //dll_log.Log (L"LoadLibraryA Addres: %ph", GetProcAddress (GetModuleHandle (L"kernel32.dll"), "LoadLibraryA"));

  dll_log.LogEx (false,
    L"------------------------------------------------------------------------"
    L"-------------------\n");


  if (altered_user_profile)
  {
    dll_log.Log ( L"*** WARNING: User has altered their user profile directory the wrong way. ***");
    dll_log.Log ( L"  >> %%UserProfile%%       = '%ws'",
                    wszEnvProfile );
    dll_log.Log ( L"  >> Profile Directory   = '%ws'",
                    wszProfile );
    dll_log.Log ( L"  >> Documents Directory = '%ws'",
                    wszDocs );
    dll_log.LogEx ( false,
      L"------------------------------------------------------------------------"
      L"-------------------\n" );
  }


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


  if (config.system.display_debug_out)
    SK::Diagnostics::Debugger::SpawnConsole ();

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Init ();


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
        wchar_t wszDLLPath [MAX_PATH * 2 + 1] = { };

        if (SK_IsInjected ())
          wcsncpy (wszDLLPath, SK_GetModuleFullName (SK_GetDLL ()).c_str (), MAX_PATH * 2);
        else
        {
          _swprintf ( wszDLLPath, L"%s\\My Mods\\SpecialK\\SpecialK.dll",
                        SK_GetDocumentsDir ().c_str () );
        }

        if (PathRemoveFileSpec (wszDLLPath))
        {
          lstrcatW (wszDLLPath, L"\\");
          lstrcatW (wszDLLPath, wszSteamDLL);

          if (SK_GetFileSize (wszDLLPath) > 0)
          {
            HMODULE hMod = LoadLibraryW_Original (wszDLLPath);

            if (hMod)
            {
              dll_log.Log ( L"[DLL Loader]   Manually booted SteamAPI: '%s'",
                              wszDLLPath );
            }
          }
        }
      }
    }
  }

  SK_Steam_InitCommandConsoleVariables ();
  SK_TestSteamImports                  (GetModuleHandle (nullptr));



  SK_InitCompatBlacklist ();


  // Load user-defined DLLs (Early)
#ifdef _WIN64
  SK_LoadEarlyImports64 ();
#else
  SK_LoadEarlyImports32 ();
#endif


  // Performance monitorng pre-init
  CreateThread (nullptr, 0, [](LPVOID) -> DWORD { SK_InitWMI (); CloseHandle (GetCurrentThread ()); return 0; }, nullptr, 0x00, nullptr);


  // Do this from the startup thread
  SK_HookWinAPI       ();


  // Hard-code the AppID for ToZ
  if (! lstrcmpW (SK_GetHostApp (), L"Tales of Zestiria.exe"))
    config.steam.appid = 351970;

  // Game won't start from the commandline without this...
  if (! lstrcmpW (SK_GetHostApp (), L"dis1_st.exe"))
    config.steam.appid = 405900;

  if (! lstrcmpW (SK_GetHostApp (), L"FairyFencerAD.exe"))
  {
    extern void SK_FFAD_InitPlugin (void);
    SK_FFAD_InitPlugin ();
  }


  SK_EnumLoadedModules (SK_ModuleEnum::PreLoad);


BACKEND_INIT:
  InterlockedExchangePointer (
    (LPVOID *)&hInitThread,
      CreateThread ( nullptr,
                       0,
                         DllThread,
                           &init_,
                             0x00,
                               nullptr )
  ); // Avoid the temptation to wait on this thread

  LeaveCriticalSection (&init_mutex);

  return true;
}

extern "C" {
bool
__stdcall
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

  if (hPumpThread != INVALID_HANDLE_VALUE)
  {
    dll_log.LogEx   (true, L"[ Stat OSD ] Shutting down Pump Thread... ");
  
    CloseHandle     (hPumpThread);
                     hPumpThread = INVALID_HANDLE_VALUE;
  
    dll_log.LogEx   (false, L"done!\n");
  }

  SK::DXGI::ShutdownBudgetThread ();

  if (process_stats.hThread != INVALID_HANDLE_VALUE)
  {
    dll_log.LogEx (true,L"[ WMI Perf ] Shutting down Process Monitor... ");

    // Signal the thread to shutdown
    SetEvent (process_stats.hShutdownSignal);

    WaitForSingleObject
      (process_stats.hThread, 1000UL); // Give 1 second, and
                                       // then we're killing
                                       // the thing!

    CloseHandle (process_stats.hThread);
                 process_stats.hThread  = INVALID_HANDLE_VALUE;

    dll_log.LogEx (false, L"done!\n");
  }

  if (cpu_stats.hThread != INVALID_HANDLE_VALUE)
  {
    dll_log.LogEx (true,L"[ WMI Perf ] Shutting down CPU Monitor... ");

    // Signal the thread to shutdown
    SetEvent (cpu_stats.hShutdownSignal);

    WaitForSingleObject (cpu_stats.hThread, 1000UL); // Give 1 second, and
                                                     // then we're killing
                                                     // the thing!

    CloseHandle (cpu_stats.hThread);
                 cpu_stats.hThread  = INVALID_HANDLE_VALUE;
                 cpu_stats.num_cpus = 0;

    dll_log.LogEx (false, L"done!\n");
  }

  if (disk_stats.hThread != INVALID_HANDLE_VALUE)
  {
    dll_log.LogEx (true, L"[ WMI Perf ] Shutting down Disk Monitor... ");

    // Signal the thread to shutdown
    SetEvent (disk_stats.hShutdownSignal);

    WaitForSingleObject (disk_stats.hThread, 1000UL); // Give 1 second, and
                                                      // then we're killing
                                                      // the thing!
    CloseHandle (disk_stats.hThread);
                 disk_stats.hThread   = INVALID_HANDLE_VALUE;
                 disk_stats.num_disks = 0;

    dll_log.LogEx (false, L"done!\n");
  }

  if (pagefile_stats.hThread != INVALID_HANDLE_VALUE)
  {
    dll_log.LogEx (true, L"[ WMI Perf ] Shutting down Pagefile Monitor... ");

    // Signal the thread to shutdown
    SetEvent (pagefile_stats.hShutdownSignal);

    WaitForSingleObject (
      pagefile_stats.hThread, 1000UL); // Give 1 second, and
                                       // then we're killing
                                       // the thing!

    CloseHandle (pagefile_stats.hThread);
                 pagefile_stats.hThread       = INVALID_HANDLE_VALUE;
                 pagefile_stats.num_pagefiles = 0;

    dll_log.LogEx (false, L"done!\n");
  }

  const wchar_t* config_name = backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  if (sk::NVAPI::app_name != L"ds3t.exe")
  {
    dll_log.LogEx (true,  L"[ SpecialK ] Saving user preferences to %s.ini... ", config_name);
    SK_SaveConfig (config_name);
    dll_log.LogEx (false, L"done!\n");
  }

  // We generally don't start SteamAPI -- but someday might want this as a feature, so
  //    let this code rot here ;)
#if 0
  if ((! config.steam.silent)) {
    dll_log.LogEx  (true, L"[ SteamAPI ] Shutting down Steam API... ");
    SK::SteamAPI::Shutdown ();
    dll_log.LogEx  (false, L"done!\n");
  }
#endif

  SK_UnloadImports        ();
  SK::Framerate::Shutdown ();

  if (nvapi_init)
    sk::NVAPI::UnloadLibrary ();

  dll_log.Log (L"[ SpecialK ] Custom %s.dll Detached (pid=0x%04x)",
    backend, GetCurrentProcessId ());

  SymCleanup (GetCurrentProcess ());

  SK_ShutdownWMI    ();

  // Breakpad Disable Disclaimer; pretend the log was empty :)
  if (crash_log.lines == 1)
    crash_log.lines = 0;

  crash_log.close ();

  SK_UnInit_MinHook ();

  return true;
}
}


extern void SK_InitWindow (HWND hWnd);
#include <SpecialK/render_backend.h>
#include <imgui/imgui.h>

__declspec (noinline)
void
STDMETHODCALLTYPE
SK_BeginBufferSwap (void)
{
  // Throttle, but do not deadlock the render loop
  while (InterlockedCompareExchangeNoFence (&SK_bypass_dialog_active, FALSE, FALSE))
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

    if (InterlockedAdd64 (&SK_SteamAPI_CallbackRunCount, 0) < 1)
    {
      // Steam Init: Better late than never

      SK_TestSteamImports (GetModuleHandle (nullptr));
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
  
              DLL_DIRECTORY_COOKIE cookie = 0;
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
            if ( (int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D11 )
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
  extern void SK_DrawTexMgrStats (void);
  SK_DrawTexMgrStats ();

  static HMODULE hModTBFix = GetModuleHandle( L"tbfix.dll");

  const char* szFirst = "First-frame Done";

  if (hModTBFix)
  {
    if (SK_Steam_PiratesAhoy () != 0x00)
    {
      extern float target_fps;
      target_fps = (float)*(uint8_t*)(szFirst+5);
    }

    SK::Framerate::GetLimiter ()->wait ();
  }

  extern bool SK_ImGui_Visible;

  extern uint32_t WINAPI SK_Steam_PiratesAhoy (void);
  if (SK_Steam_PiratesAhoy () && (! SK_ImGui_Visible))
  {
    extern void SK_ImGui_Toggle (void);
                SK_ImGui_Toggle ();
  }

  extern void SK_ImGui_PollGamepad_EndFrame (void);
  SK_ImGui_PollGamepad_EndFrame ();
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

  static bool toggle_drag = false;

  if (HIWORD (GetAsyncKeyState_Original (VK_CONTROL)) && HIWORD (GetAsyncKeyState_Original (VK_SHIFT)) && HIWORD (GetAsyncKeyState_Original (VK_SCROLL)))
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
    if (HIWORD (GetAsyncKeyState_Original (config.osd.keys.expand [0])) &&
        HIWORD (GetAsyncKeyState_Original (config.osd.keys.expand [1])) &&
        HIWORD (GetAsyncKeyState_Original (config.osd.keys.expand [2])))
    {
      last_osd_scale = ullNow.QuadPart;
      SK_ResizeOSD (+0.1f);
    }

    if (HIWORD (GetAsyncKeyState_Original (config.osd.keys.shrink [0])) &&
        HIWORD (GetAsyncKeyState_Original (config.osd.keys.shrink [1])) &&
        HIWORD (GetAsyncKeyState_Original (config.osd.keys.shrink [2])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.time.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.time.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.time.keys.toggle [2])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.mem.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.mem.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.mem.keys.toggle [2])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.load_balance.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.load_balance.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.load_balance.keys.toggle [2])))
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
    if (HIWORD (GetAsyncKeyState_Original (config.sli.keys.toggle [0])) &&
        HIWORD (GetAsyncKeyState_Original (config.sli.keys.toggle [1])) &&
        HIWORD (GetAsyncKeyState_Original (config.sli.keys.toggle [2])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.io.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.io.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.io.keys.toggle [2])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.cpu.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.cpu.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.cpu.keys.toggle [2])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.gpu.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.gpu.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.gpu.keys.toggle [2])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.fps.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.fps.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.fps.keys.toggle [2])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.disk.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.disk.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.disk.keys.toggle [2])) &&
      HIWORD (GetAsyncKeyState_Original (config.disk.keys.toggle [3])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.pagefile.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.pagefile.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.pagefile.keys.toggle [2])) &&
      HIWORD (GetAsyncKeyState_Original (config.pagefile.keys.toggle [3])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.osd.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.osd.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.osd.keys.toggle [2])))
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
  if (HIWORD (GetAsyncKeyState_Original (config.render.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState_Original (config.render.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState_Original (config.render.keys.toggle [2])))
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

#include <SpecialK/render_backend.h>

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

    if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev9Ex))))
    {
         (int&)__SK_RBkEnd.api  = ( (int)SK_RenderAPI::D3D9 |
                                    (int)SK_RenderAPI::D3D9Ex );
      wcsncpy (__SK_RBkEnd.name, L"D3D9Ex", 8);
    }

    else if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev9))))
    {
               __SK_RBkEnd.api  = SK_RenderAPI::D3D9;
      wcsncpy (__SK_RBkEnd.name, L"D3D9  ", 8);
    }

    else if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev11))))
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
    if (config.apis.OpenGL.hook && SK_GetCurrentGLContext () != 0)
    {
               __SK_RBkEnd.api  = SK_RenderAPI::OpenGL;
      wcsncpy (__SK_RBkEnd.name, L"OpenGL", 8);
    }
  }

  config.apis.last_known = __SK_RBkEnd.api;

  static volatile ULONG budget_init = FALSE;

  if (! InterlockedCompareExchange (&budget_init, TRUE, FALSE))
  {
    SK::DXGI::StartBudgetThread_NoAdapter ();
  }

  // Draw after present, this may make stuff 1 frame late, but... it
  //   helps with VSYNC performance.

  // Treat resize and obscured statuses as failures; DXGI does not, but
  //  we should not draw the OSD when these events happen.
  if (FAILED (hr))
    return hr;

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
//   the Steam client throwing a tempur tantrum.
void
CALLBACK
RunDLL_ElevateMe ( HWND  hwnd,        HINSTANCE hInst,
                   LPSTR lpszCmdLine, int       nCmdShow )
{
  ShellExecuteA ( nullptr, "runas", lpszCmdLine, nullptr, nullptr, SW_SHOWNORMAL );
}

void
CALLBACK
RunDLL_RestartGame ( HWND  hwnd,        HINSTANCE hInst,
                     LPSTR lpszCmdLine, int       nCmdShow )
{
  ShellExecuteA ( nullptr, "open", lpszCmdLine, nullptr, nullptr, SW_SHOWNORMAL );
}