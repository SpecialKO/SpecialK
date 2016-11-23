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

#include "core.h"
#include "stdafx.h"

#include "diagnostics/crash_handler.h"
#include "diagnostics/debug_utils.h"

#include "log.h"
#include "utility.h"

#include "tls.h"

extern "C" BOOL WINAPI _CRT_INIT (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);


#pragma warning   (push)
#pragma warning   (disable: 4091)
#  include <DbgHelp.h>
#  pragma comment (lib, "dbghelp.lib")
#pragma warning   (pop)

#include "config.h"
#include "osd/text.h"
#include "io_monitor.h"
#include "import.h"
#include "console.h"
#include "command.h"
#include "framerate.h"

#include "steam_api.h"

#include <atlbase.h>
#include <comdef.h>

#include <delayimp.h>

#include "CEGUI/CEGUI.h"
#include "CEGUI/System.h"
#include "CEGUI/Logger.h"

memory_stats_t mem_stats [MAX_GPU_NODES];
mem_info_t     mem_info  [NumBuffers];

CRITICAL_SECTION budget_mutex  = { 0 };
CRITICAL_SECTION init_mutex    = { 0 };
volatile HANDLE  hInitThread   = { 0 };
         HANDLE  hPumpThread   = { 0 };

struct budget_thread_params_t {
  IDXGIAdapter3   *pAdapter = nullptr;
  DWORD            tid      = 0UL;
  HANDLE           handle   = INVALID_HANDLE_VALUE;
  DWORD            cookie   = 0UL;
  HANDLE           event    = INVALID_HANDLE_VALUE;
  volatile ULONG   ready    = FALSE;
} budget_thread;

// Disable SLI memory in Batman Arkham Knight
bool USE_SLI = true;


extern "C++" void SK_DS3_InitPlugin (void);


NV_GET_CURRENT_SLI_STATE sli_state;
BOOL                     nvapi_init = FALSE;
int                      gpu_prio;

HMODULE                  backend_dll  = 0;

// NOT the working directory, this is the directory that
//   the executable is located in.
const wchar_t*
SK_GetHostPath (void);

extern
bool
__stdcall
SK_IsInjected (void);

extern
HMODULE
__stdcall
SK_GetDLL (void);

extern
const wchar_t*
__stdcall
SK_GetRootPath (void);

volatile
ULONG frames_drawn = 0UL;

ULONG
__stdcall
SK_GetFramesDrawn (void)
{
  return InterlockedExchangeAdd (&frames_drawn, 0);
}

#include <d3d9.h>
#include <d3d11.h>

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


    /* DXGI (Status) */
  case DXGI_STATUS_OCCLUDED:
    return L"DXGI_STATUS_OCCLUDED";

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


    /* Generic (FAILED) */

  case E_FAIL:
    return L"E_FAIL";

  case E_INVALIDARG:
    return L"E_INVALIDARG";

  case E_OUTOFMEMORY:
    return L"E_OUTOFMEMORY";

  case E_NOTIMPL:
    return L"E_NOTIMPL";


  default:
    dll_log.Log (L" *** Encountered unknown HRESULT: (0x%08X)",
      (unsigned long)result);
    return L"UNKNOWN";
  }
}

void
__stdcall
SK_StartDXGI_1_4_BudgetThread (IDXGIAdapter** ppAdapter)
{
  //
  // If the adapter implements DXGI 1.4, then create a budget monitoring
  //  thread...
  //
  IDXGIAdapter3* pAdapter3 = nullptr;

  if (SUCCEEDED ((*ppAdapter)->QueryInterface (
       IID_PPV_ARGS (&pAdapter3) )))
  {
    // We darn sure better not spawn multiple threads!
    EnterCriticalSection (&budget_mutex);

    if (budget_thread.handle == INVALID_HANDLE_VALUE) {
      // We're going to Release this interface after thread spawnning, but
      //   the running thread still needs a reference counted.
      pAdapter3->AddRef ();

      unsigned int __stdcall BudgetThread (LPVOID user_data);

      ZeroMemory (&budget_thread, sizeof budget_thread_params_t);

      dll_log.LogEx (true,
        L"[ DXGI 1.4 ]   $ Spawning Memory Budget Change Thread..: ");

      budget_thread.pAdapter = pAdapter3;
      budget_thread.tid      = 0;
      budget_thread.event    = 0;
      InterlockedExchange (&budget_thread.ready, FALSE);
      budget_log.silent      = true;

      budget_thread.handle =
        (HANDLE)
          _beginthreadex ( nullptr,
                             0,
                               BudgetThread,
                                 (LPVOID)&budget_thread,
                                   0x00,
                                     nullptr );

      while (! InterlockedCompareExchange (&budget_thread.ready, FALSE, FALSE))
        ;

      if (budget_thread.tid != 0) {
        dll_log.LogEx (false, L"tid=0x%04x\n", budget_thread.tid);

        dll_log.LogEx (true,
          L"[ DXGI 1.4 ]   %% Setting up Budget Change Notification.: ");

        HRESULT result =
          pAdapter3->RegisterVideoMemoryBudgetChangeNotificationEvent (
            budget_thread.event, &budget_thread.cookie
            );

        if (SUCCEEDED (result)) {
          dll_log.LogEx (false, L"eid=0x%x, cookie=%u\n",
            budget_thread.event, budget_thread.cookie);
        } else {
          dll_log.LogEx (false, L"Failed! (%s)\n",
            SK_DescribeHRESULT (result));
        }
      } else {
        dll_log.LogEx (false, L"failed!\n");
      }
    }

    LeaveCriticalSection (&budget_mutex);

    dll_log.LogEx (true,  L"[ DXGI 1.2 ] GPU Scheduling...:"
      L" Pre-Emptive");

    DXGI_ADAPTER_DESC2 desc2;

    bool silent    = dll_log.silent;
    dll_log.silent = true;
    {
      // Don't log this call, because that would be silly...
      pAdapter3->GetDesc2 (&desc2);
    }
    dll_log.silent = silent;

    switch (desc2.GraphicsPreemptionGranularity)
    {
    case DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY:
      dll_log.LogEx (false, L" (DMA Buffer)\n");
      break;
    case DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY:
      dll_log.LogEx (false, L" (Graphics Primitive)\n");
      break;
    case DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY:
      dll_log.LogEx (false, L" (Triangle)\n");
      break;
    case DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY:
      dll_log.LogEx (false, L" (Fragment)\n");
      break;
    case DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY:
      dll_log.LogEx (false, L" (Instruction)\n");
      break;
    default:
      dll_log.LogEx (false, L"UNDEFINED\n");
      break;
    }

    int i = 0;

    dll_log.LogEx (true,
      L"[ DXGI 1.4 ] Local Memory.....:");

    DXGI_QUERY_VIDEO_MEMORY_INFO mem_info;
    while (SUCCEEDED (pAdapter3->QueryVideoMemoryInfo (
      i,
      DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
      &mem_info)
      )
      )
    {
      if (i > 0) {
        dll_log.LogEx (false, L"\n");
        dll_log.LogEx (true,  L"                               ");
      }

      dll_log.LogEx (false,
        L" Node%i (Reserve: %#5llu / %#5llu MiB - "
        L"Budget: %#5llu / %#5llu MiB)",
        i++,
        mem_info.CurrentReservation      >> 20ULL,
        mem_info.AvailableForReservation >> 20ULL,
        mem_info.CurrentUsage            >> 20ULL,
        mem_info.Budget                  >> 20ULL);

      pAdapter3->SetVideoMemoryReservation (
        (i - 1),
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
        (i == 1 || USE_SLI) ?
        uint64_t (mem_info.AvailableForReservation *
          config.mem.reserve * 0.01f) :
        0
        );
    }
    dll_log.LogEx (false, L"\n");

    i = 0;

    dll_log.LogEx (true,
      L"[ DXGI 1.4 ] Non-Local Memory.:");

    while (SUCCEEDED (pAdapter3->QueryVideoMemoryInfo (
      i,
      DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
      &mem_info )
      )
      )
    {
      if (i > 0) {
        dll_log.LogEx (false, L"\n");
        dll_log.LogEx (true,  L"                               ");
      }
      dll_log.LogEx (false,
        L" Node%i (Reserve: %#5llu / %#5llu MiB - "
        L"Budget: %#5llu / %#5llu MiB)",
        i++,
        mem_info.CurrentReservation      >> 20ULL,
        mem_info.AvailableForReservation >> 20ULL,
        mem_info.CurrentUsage            >> 20ULL,
        mem_info.Budget                  >> 20ULL );

      pAdapter3->SetVideoMemoryReservation (
        (i - 1),
        DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
        (i == 1 || USE_SLI) ?
        uint64_t (mem_info.AvailableForReservation *
          config.mem.reserve * 0.01f) :
        0
        );
    }

    ::mem_info [0].nodes = i-1;
    ::mem_info [1].nodes = i-1;

    dll_log.LogEx (false, L"\n");
  }
}

#include <ctime>
#define min_max(ref,min,max) if ((ref) > (max)) (max) = (ref); \
                             if ((ref) < (min)) (min) = (ref);

const uint32_t BUDGET_POLL_INTERVAL = 66UL; // How often to sample the budget
                                            //  in msecs

unsigned int
__stdcall
BudgetThread (LPVOID user_data)
{
  budget_thread_params_t* params =
    (budget_thread_params_t *)user_data;

  if (budget_log.init (L"logs/dxgi_budget.log", L"w")) {
    params->tid       = GetCurrentThreadId ();
    params->event     = CreateEvent (NULL, FALSE, FALSE, L"DXGIMemoryBudget");
    budget_log.silent = true;

    InterlockedExchange (&params->ready, TRUE);
  } else {
    params->tid    = 0;

    InterlockedExchange (&params->ready, TRUE); // Not really :P
    return -1;
  }

  while (InterlockedCompareExchange (&params->ready, FALSE, FALSE)) {
    if (params->event == 0)
      break;

    DWORD dwWaitStatus = WaitForSingleObject (params->event,
      BUDGET_POLL_INTERVAL);

    if (! InterlockedCompareExchange (&params->ready, FALSE, FALSE)) {
      ResetEvent (params->event);
      break;
    }

    buffer_t buffer = mem_info [0].buffer;

    if (buffer == Front)
      buffer = Back;
    else
      buffer = Front;

    GetLocalTime (&mem_info [buffer].time);

    int node = 0;

    while (node < MAX_GPU_NODES &&
      SUCCEEDED (params->pAdapter->QueryVideoMemoryInfo (
        node,
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
        &mem_info [buffer].local [node++] )
        )
      ) ;

    // Fix for AMD drivers, that don't allow querying non-local memory
    int nodes = std::max (0, node - 1);

    node = 0;

    while (node < MAX_GPU_NODES &&
      SUCCEEDED (params->pAdapter->QueryVideoMemoryInfo (
        node,
        DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
        &mem_info [buffer].nonlocal [node++] )
        )
      ) ;

    //int nodes = max (0, node - 1);

    // Set the number of SLI/CFX Nodes
    mem_info [buffer].nodes = nodes;

    static uint64_t last_budget =
      mem_info [buffer].local [0].Budget;

    if (dwWaitStatus == WAIT_OBJECT_0 && config.load_balance.use)
    {
      INT prio = 0;

      last_budget = mem_info [buffer].local [0].Budget;
    }

    if (nodes > 0) {
      int i = 0;

      budget_log.LogEx (true, L"[ DXGI 1.4 ] Local Memory.....:");

      while (i < nodes) {
        if (dwWaitStatus == WAIT_OBJECT_0) {
          static UINT64 LastBudget = 0ULL;

          mem_stats [i].budget_changes++;

          int64_t over_budget =
            mem_info [buffer].local [i].CurrentUsage -
            mem_info [buffer].local [i].Budget;

            //LastBudget -
            //mem_info [buffer].local [i].Budget;

          extern bool SK_D3D11_need_tex_reset;

          SK_D3D11_need_tex_reset = (over_budget > 0);

          //extern uint32_t SK_D3D11_amount_to_purge;
          //SK_D3D11_amount_to_purge += max (0, over_budget);

/*
          if (LastBudget > (mem_info [buffer].local [i].Budget + 1024 * 1024 * 128)) {
            extern bool   SK_D3D11_need_tex_reset;
            extern UINT64 SK_D3D11_amount_to_purge;
            SK_D3D11_need_tex_reset  = true;
            SK_D3D11_amount_to_purge = LastBudget - mem_info [buffer].local [i].Budget;
          }
*/

          LastBudget = mem_info [buffer].local [i].Budget;
        }

        if (i > 0) {
          budget_log.LogEx (false, L"\n");
          budget_log.LogEx (true,  L"                                 ");
        }

        budget_log.LogEx (false,
          L" Node%i (Reserve: %#5llu / %#5llu MiB - "
          L"Budget: %#5llu / %#5llu MiB)",
          i,
          mem_info [buffer].local [i].CurrentReservation      >> 20ULL,
          mem_info [buffer].local [i].AvailableForReservation >> 20ULL,
          mem_info [buffer].local [i].CurrentUsage            >> 20ULL,
          mem_info [buffer].local [i].Budget                  >> 20ULL);

        min_max (mem_info [buffer].local [i].AvailableForReservation,
          mem_stats [i].min_avail_reserve,
          mem_stats [i].max_avail_reserve);

        min_max (mem_info [buffer].local [i].CurrentReservation,
          mem_stats [i].min_reserve,
          mem_stats [i].max_reserve);

        min_max (mem_info [buffer].local [i].CurrentUsage,
          mem_stats [i].min_usage,
          mem_stats [i].max_usage);

        min_max (mem_info [buffer].local [i].Budget,
          mem_stats [i].min_budget,
          mem_stats [i].max_budget);

        if (mem_info [buffer].local [i].CurrentUsage >
          mem_info [buffer].local [i].Budget) {
          uint64_t over_budget =
            mem_info [buffer].local [i].CurrentUsage -
            mem_info [buffer].local [i].Budget;

          min_max (over_budget, mem_stats [i].min_over_budget,
            mem_stats [i].max_over_budget);
        }

        i++;
      }
      budget_log.LogEx (false, L"\n");

      i = 0;

      budget_log.LogEx (true,
        L"[ DXGI 1.4 ] Non-Local Memory.:");

      while (i < nodes) {
        if (i > 0) {
          budget_log.LogEx (false, L"\n");
          budget_log.LogEx (true,  L"                                 ");
        }

        budget_log.LogEx (false,
          L" Node%i (Reserve: %#5llu / %#5llu MiB - "
          L"Budget: %#5llu / %#5llu MiB)",
          i,
          mem_info [buffer].nonlocal [i].CurrentReservation      >> 20ULL,
          mem_info [buffer].nonlocal [i].AvailableForReservation >> 20ULL,
          mem_info [buffer].nonlocal [i].CurrentUsage            >> 20ULL,
          mem_info [buffer].nonlocal [i].Budget                  >> 20ULL);
        i++;
      }

      budget_log.LogEx (false, L"\n");
    }

    if (params->event != 0)
      ResetEvent (params->event);

    mem_info [0].buffer = buffer;
  }

  return 0;
}


// Stupid solution for games that inexplicibly draw to the screen
//   without ever swapping buffers.
unsigned int
__stdcall
osd_pump (LPVOID lpThreadParam)
{ 
  // TODO: This actually increases the number of frames rendered, which
  //         may interfere with other mod logic... the entire feature is
  //           a hack, but that behavior is not intended.
  while (true) {
    Sleep            ((DWORD)(config.osd.pump_interval * 1000.0f));
    SK_EndBufferSwap (S_OK, nullptr);
  }

  return 0;
}

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

std::queue <DWORD> suspended_tids;

void
__stdcall
SK_InitFinishCallback (_Releases_exclusive_lock_ (init_mutex) void)
{
  dll_log.Log (L"[ SpecialK ] === Initialization Finished! ===");

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
      dll_log.LogEx (false, L"tid=0x%04x, interval=%04.01f ms\n",
                       hPumpThread, config.osd.pump_interval * 1000.0f);
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

  HANDLE hProc = GetCurrentProcess ();

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
  BOOL bWOW64;
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

  if (! load_proxy)
    dll_log.LogEx (true, L" Loading default %s.dll: ", backend);

  backend_dll = LoadLibrary (dll_name);

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

  extern void __crc32_init (void);
  __crc32_init ();



  if (config.cegui.enable)
  {
    // Disable until we validate CEGUI's state
    config.cegui.enable = false;

    wchar_t wszCEGUIModPath [MAX_PATH] = { L'\0' };
    wchar_t wszCEGUITestDLL [MAX_PATH] = { L'\0' };

#ifdef _WIN64
    _swprintf (wszCEGUIModPath, L"%sCEGUI\\bin\\x64",   SK_GetRootPath ());
#else
    _swprintf (wszCEGUIModPath, L"%sCEGUI\\bin\\Win32", SK_GetRootPath ());
#endif

    lstrcatW      (wszCEGUITestDLL, wszCEGUIModPath);
    lstrcatW      (wszCEGUITestDLL, L"\\CEGUIBase-0.dll");

    if (GetFileAttributesW (wszCEGUITestDLL) != INVALID_FILE_ATTRIBUTES) {
      dll_log.Log (L"[  CEGUI   ] Enabling CEGUI: (%s)", wszCEGUITestDLL);

      config.cegui.enable = true;

      SetDllDirectoryW         (wszCEGUIModPath);

      wchar_t wszEnvVar     [MAX_PATH] = { L'\0' };
      wchar_t wszWorkingDir [MAX_PATH] = { L'\0' };

      _swprintf (wszEnvVar, L"CEGUI_MODULE_DIR=%s", wszCEGUIModPath);
      _wputenv  (wszEnvVar);

      GetCurrentDirectoryW (MAX_PATH, wszWorkingDir);

      // Some games will rebase themselves, it is important to know the
      //   game's working directory at the time CEGUI was initialized.
      _swprintf (wszEnvVar, L"CEGUI_WORKING_DIR=%s", wszWorkingDir);
      _wputenv  (wszEnvVar);

      __HrLoadAllImportsForDll ("CEGUIBase-0.dll");

      __HrLoadAllImportsForDll ("CEGUIDirect3D9Renderer-0.dll");
      __HrLoadAllImportsForDll ("CEGUIDirect3D11Renderer-0.dll");

      SetDllDirectoryW         (nullptr);
    }
  }

  SK::Framerate::Init ();

  // Load user-defined DLLs (Early)
#ifdef _WIN64
  SK_LoadEarlyImports64 ();
#else
  SK_LoadEarlyImports32 ();
#endif

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

    extern BOOL SK_InitADL (void);
    BOOL adl_init = SK_InitADL ();

    dll_log.LogEx (false, L" %s\n", adl_init ? L"Success" : L"Failed");

    if (adl_init) {
      extern int SK_ADL_CountPhysicalGPUs (void);
      extern int SK_ADL_CountActiveGPUs   (void);

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

  // Setup the compatibility backend, which monitors loaded libraries,
  //   blacklists bad DLLs and detects render APIs...
  extern void __stdcall EnumLoadedModules (void);
  EnumLoadedModules ();

  typedef void (WINAPI *finish_pfn)  (_Releases_exclusive_lock_ (init_mutex) void);
  typedef void (WINAPI *callback_pfn)(finish_pfn);
  callback_pfn callback_fn = (callback_pfn)callback;
  callback_fn (SK_InitFinishCallback);

  // Load user-defined DLLs (Plug-In)
#ifdef _WIN64
  SK_LoadPlugIns64 ();
#else
  SK_LoadPlugIns32 ();
#endif
}


volatile  LONG SK_bypass_dialog_active = FALSE;
volatile  LONG init                    = FALSE; // Do not use static storage,
                                                //   the C Runtime may not be
                                                //     init yet.

void
WaitForInit (void)
{
  if (InterlockedCompareExchange (&init, FALSE, FALSE)) {
    return;
  }

  // Prevent a race condition caused by undefined behavior in RTSS
  if (! InterlockedCompareExchangePointer ((volatile LPVOID *)&hInitThread, nullptr, nullptr)) {
    dll_log.Log ( L"[ MultiThr ] Race condition detected during startup (tid=%x)",
                    GetCurrentThreadId () );
#if 0
    while (! InterlockedCompareExchange (&init, FALSE, FALSE)) {
      dll_log.Log ( L"[ MultiThr ] Race condition avoided (tid=%x)",
                      GetCurrentThreadId () );
      Sleep (150);
    }
#endif
  }

  while (InterlockedCompareExchangePointer ((volatile LPVOID *)&hInitThread, nullptr, nullptr)) {
    if ( WAIT_OBJECT_0 == WaitForSingleObject (
      InterlockedCompareExchangePointer ((volatile LPVOID *)&hInitThread, nullptr, nullptr),
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
    CloseHandle (InterlockedExchangePointer ((void **)&hInitThread, nullptr));

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

    MEMORY_BASIC_INFORMATION mem_info;
    VirtualQuery (base_addr, &mem_info, sizeof mem_info);

    base_addr = (uint8_t *)mem_info.BaseAddress;
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
  // Initialize TLS for this thread
  SK_GetTLS ();

  init_params_t* params =
    &init_;

  SK_InitCore (params->backend, params->callback);

  if (SK_IsHostAppSKIM ())
    return 0;

  extern int32_t SK_D3D11_amount_to_purge;
  SK_GetCommandProcessor ()->AddVariable (
    "VRAM.Purge",
      new SK_IVarStub <int32_t> (
        (int32_t *)&SK_D3D11_amount_to_purge
      )
  );

  skMemCmd* mem = new skMemCmd ();

  SK_GetCommandProcessor ()->AddCommand ("mem", mem);

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

  if (! SK_IsHostAppSKIM ())
    SK_SaveConfig (config_name);

  // For the global injector, when not started by SKIM, check its version
  if (SK_IsInjected () && (! SK_IsHostAppSKIM ()))
    _beginthreadex (nullptr, 0, CheckVersionThread, nullptr, 0x00, nullptr);

  return 0;
}

DWORD
__stdcall
DllThread (LPVOID user)
{
  EnterCriticalSection (&init_mutex);
  {
    DllThread_CRT (&init_);
  }
  LeaveCriticalSection (&init_mutex);

  return 0;
}

#include <wingdi.h>
#include <gl/gl.h>

class SK_HookedFunction {
public:
  enum Type {
    DLL,
    VFTable,
    Generic,
    Invalid
  };

protected:
  Type             type     = Invalid;
  const char*      name     = "<Initialize_Me>";

  struct {
    uintptr_t      detour   = 0xCaFACADE;
    uintptr_t      original = 0x8badf00d;
    uintptr_t      target   = 0xdeadc0de;
  } addr;

  union {
    struct {
      int_fast16_t idx      = -1;
    } vftbl;

    struct {
      HANDLE       module   = 0; // Hold a reference; don't let
                                 //   software unload the DLL while it is
                                 //     hooked!
    } dll;
  };

  bool             enabled  = false;
};

class HookManager {
public:
  bool validateVFTables (void);
  void rehookVFTables   (void);

  void uninstallAll     (void);
  void reinstallAll     (void);
  void install          (SK_HookedFunction* pfn);

std::vector <SK_HookedFunction> hooks;
};

MH_STATUS
WINAPI
SK_CreateFuncHook ( LPCWSTR pwszFuncName,
                    LPVOID  pTarget,
                    LPVOID  pDetour,
                    LPVOID *ppOriginal )
{
  MH_STATUS status =
    MH_CreateHook ( pTarget,
                      pDetour,
                        ppOriginal );

  // Ignore the Already Created Error; happens A LOT as multiple
  //   Direct3D devices are created during runtime.
  if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED) {
    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for '%s' "
                  L"[Address: %04ph]!  (Status: \"%hs\")",
                    pwszFuncName,
                      pTarget,
                        MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
WINAPI
SK_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                   LPVOID  pDetour,    LPVOID *ppOriginal,
                   LPVOID *ppFuncAddr )
{
  HMODULE hMod = nullptr;

  // First try to get (and permanently hold) a reference to the hooked module
  if (! GetModuleHandleEx (
          GET_MODULE_HANDLE_EX_FLAG_PIN,
            pwszModule,
              &hMod ) ) {
    //
    // If that fails, partially load the module into memory to establish our
    //   function hook.
    //
    //  Defer the standard DllMain (...) entry-point until the
    //    software actually loads the library on its own.
    //
    hMod = LoadLibraryEx (
             pwszModule,
               nullptr,
                 /*DONT_RESOLVE_DLL_REFERENCES*/0 );
  }

  LPVOID    pFuncAddr = nullptr;
  MH_STATUS status    = MH_OK;

  if (hMod == 0)
    status = MH_ERROR_MODULE_NOT_FOUND;

  else {
    pFuncAddr =
      GetProcAddress (hMod, pszProcName);

    status =
      MH_CreateHook ( pFuncAddr,
                        pDetour,
                          ppOriginal );
  }

  if (status != MH_OK)
  {
    if (status == MH_ERROR_ALREADY_CREATED)
    {
      if (ppOriginal == nullptr) {
        SH_Introspect ( pFuncAddr,
                          SH_TRAMPOLINE,
                            ppOriginal );

        dll_log.Log ( L"[ Min Hook ] WARNING: Hook Already Exists for: '%hs' in '%s'! "
                      L"(Status: \"%hs\")",
                        pszProcName,
                          pwszModule,
                            MH_StatusToString (status) );

        return status;
      }
    }

    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for: '%hs' in '%s'! "
                  L"(Status: \"%hs\")",
                    pszProcName,
                      pwszModule,
                        MH_StatusToString (status) );
  }
  else if (ppFuncAddr != nullptr)
    *ppFuncAddr = pFuncAddr;
  else
    SK_EnableHook (pFuncAddr);

  return status;
}

MH_STATUS
WINAPI
SK_CreateDLLHook2 ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                    LPVOID  pDetour,    LPVOID *ppOriginal,
                    LPVOID *ppFuncAddr )
{
  HMODULE hMod = nullptr;

  // First try to get (and permanently hold) a reference to the hooked module
  if (! GetModuleHandleEx (
          GET_MODULE_HANDLE_EX_FLAG_PIN,
            pwszModule,
              &hMod ) ) {
    //
    // If that fails, partially load the module into memory to establish our
    //   function hook.
    //
    //  Defer the standard DllMain (...) entry-point until the
    //    software actually loads the library on its own.
    //
    hMod = LoadLibraryEx (
             pwszModule,
               nullptr,
                 /*DONT_RESOLVE_DLL_REFERENCES*/0 );
  }

  LPVOID    pFuncAddr = nullptr;
  MH_STATUS status    = MH_OK;

  if (hMod == 0)
    status = MH_ERROR_MODULE_NOT_FOUND;

  else {
    pFuncAddr =
      GetProcAddress (hMod, pszProcName);

    status =
      MH_CreateHook ( pFuncAddr,
                        pDetour,
                          ppOriginal );
  }

  if (status != MH_OK)
  {
    if (status == MH_ERROR_ALREADY_CREATED)
    {
      if (ppOriginal == nullptr) {
        SH_Introspect ( pFuncAddr,
                          SH_TRAMPOLINE,
                            ppOriginal );

        dll_log.Log ( L"[ Min Hook ] WARNING: Hook Already Exists for: '%hs' in '%s'! "
                      L"(Status: \"%hs\")",
                        pszProcName,
                          pwszModule,
                            MH_StatusToString (status) );

        return status;
      }
    }

    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for: '%hs' in '%s'! "
                  L"(Status: \"%hs\")",
                    pszProcName,
                      pwszModule,
                        MH_StatusToString (status) );
  }

  else {
    if (ppFuncAddr != nullptr)
      *ppFuncAddr = pFuncAddr;

    MH_QueueEnableHook (ppFuncAddr);
  }

  return status;
}

MH_STATUS
WINAPI
SK_CreateDLLHook3 ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                    LPVOID  pDetour,    LPVOID *ppOriginal,
                    LPVOID *ppFuncAddr )
{
  HMODULE hMod = nullptr;

  // First try to get (and permanently hold) a reference to the hooked module
  if (! GetModuleHandleEx (
          GET_MODULE_HANDLE_EX_FLAG_PIN,
            pwszModule,
              &hMod ) ) {
    //
    // If that fails, partially load the module into memory to establish our
    //   function hook.
    //
    //  Defer the standard DllMain (...) entry-point until the
    //    software actually loads the library on its own.
    //
    hMod = LoadLibraryEx (
             pwszModule,
               nullptr,
                 /*DONT_RESOLVE_DLL_REFERENCES*/0 );
  }

  LPVOID    pFuncAddr = nullptr;
  MH_STATUS status    = MH_OK;

  if (hMod == 0)
    status = MH_ERROR_MODULE_NOT_FOUND;

  else {
    pFuncAddr =
      GetProcAddress (hMod, pszProcName);

    status =
      MH_CreateHook ( pFuncAddr,
                        pDetour,
                          ppOriginal );
  }

  if (status != MH_OK) {
    // Silently ignore this problem
    if (status == MH_ERROR_ALREADY_CREATED && ppOriginal != nullptr)
      return MH_OK;

    if (status == MH_ERROR_ALREADY_CREATED)
    {
      if (ppOriginal == nullptr) {
        SH_Introspect ( pFuncAddr,
                          SH_TRAMPOLINE,
                            ppOriginal );

        dll_log.Log ( L"[ Min Hook ] WARNING: Hook Already Exists for: '%hs' in '%s'! "
                      L"(Status: \"%hs\")",
                        pszProcName,
                          pwszModule,
                            MH_StatusToString (status) );

        return status;
      }
    }

    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for: '%hs' in '%s'! "
                  L"(Status: \"%hs\")",
                    pszProcName,
                      pwszModule,
                        MH_StatusToString (status) );
  } else {
    if (ppFuncAddr != nullptr)
      *ppFuncAddr = pFuncAddr;

    MH_QueueEnableHook (ppFuncAddr);
  }

  return status;
}

MH_STATUS
WINAPI
SK_CreateVFTableHook ( LPCWSTR pwszFuncName,
                       LPVOID *ppVFTable,
                       DWORD   dwOffset,
                       LPVOID  pDetour,
                       LPVOID *ppOriginal )
{
  MH_STATUS ret =
    SK_CreateFuncHook (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal );

  if (ret == MH_OK)
    ret = SK_EnableHook (ppVFTable [dwOffset]);

  return ret;
}

MH_STATUS
WINAPI
SK_CreateVFTableHook2 ( LPCWSTR pwszFuncName,
                        LPVOID *ppVFTable,
                        DWORD   dwOffset,
                        LPVOID  pDetour,
                        LPVOID *ppOriginal )
{
  MH_STATUS ret =
    SK_CreateFuncHook (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal );

  if (ret == MH_OK)
    ret = MH_QueueEnableHook (ppVFTable [dwOffset]);

  return ret;
}

MH_STATUS
WINAPI
SK_ApplyQueuedHooks (void)
{
  MH_STATUS status =
    MH_ApplyQueued ();

  if (status != MH_OK) {
    dll_log.Log(L"[ Min Hook ] Failed to Enable Deferred Hooks!"
                  L" (Status: \"%hs\")",
                      MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
WINAPI
SK_EnableHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_EnableHook (pTarget);

  if (status != MH_OK && status != MH_ERROR_ENABLED) {
    if (pTarget != MH_ALL_HOOKS) {
      dll_log.Log(L"[ Min Hook ] Failed to Enable Hook with Address: %04ph!"
                  L" (Status: \"%hs\")",
                    pTarget,
                      MH_StatusToString (status) );
    } else {
      dll_log.Log ( L"[ Min Hook ] Failed to Enable All Hooks! "
                    L"(Status: \"%hs\")",
                      MH_StatusToString (status) );
    }
  }

  return status;
}

MH_STATUS
WINAPI
SK_DisableHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_DisableHook (pTarget);

  if (status != MH_OK && status != MH_ERROR_DISABLED) {
    if (pTarget != MH_ALL_HOOKS) {
      dll_log.Log(L"[ Min Hook ] Failed to Disable Hook with Address: %04Xh!"
                  L" (Status: \"%hs\")",
                    pTarget,
                      MH_StatusToString (status));
    } else {
      dll_log.Log ( L"[ Min Hook ] Failed to Disable All Hooks! "
                    L"(Status: \"%hs\")",
                      MH_StatusToString (status) );
    }
  }

  return status;
}

MH_STATUS
WINAPI
SK_RemoveHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_RemoveHook (pTarget);

  if (status != MH_OK) {
    dll_log.Log ( L"[ Min Hook ] Failed to Remove Hook with Address: %04Xh! "
                  L"(Status: \"%hs\")",
                    pTarget,
                      MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
WINAPI
SK_Init_MinHook (void)
{
  MH_STATUS status;

  if ((status = MH_Initialize ()) != MH_OK) {
#if 0
    dll_log.Log ( L"[ Min Hook ] Failed to Initialize MinHook Library! "
                  L"(Status: \"%hs\")",
                    MH_StatusToString (status) );
#endif
  }

  return status;
}

MH_STATUS
WINAPI
SK_UnInit_MinHook (void)
{
  MH_STATUS status;

  if ((status = MH_Uninitialize ()) != MH_OK) {
    dll_log.Log ( L"[ Min Hook ] Failed to Uninitialize MinHook Library! "
                  L"(Status: \"%hs\")",
                    MH_StatusToString (status) );
  }

  return status;
}



extern
HMODULE
__stdcall
SK_GetDLL (void);

extern std::pair <std::queue <DWORD>, BOOL> __stdcall SK_BypassInject (void);

// Brutal hack that assumes the executable has a .exe extension...
//   FIXME
void
SK_PathRemoveExtension (wchar_t* wszInOut)
{
  wszInOut [lstrlenW (wszInOut) - 3] = L'\0';
}

wchar_t SK_RootPath   [MAX_PATH + 2] = { L'\0' };
wchar_t SK_ConfigPath [MAX_PATH + 2] = { L'\0' };

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

const wchar_t*
__stdcall
SK_GetConfigPath (void)
{
  return SK_ConfigPath;
}

bool
__stdcall
SK_StartupCore (const wchar_t* backend, void* callback)
{
  wchar_t wszBlacklistFile [MAX_PATH] = { L'\0' };

  lstrcatW (wszBlacklistFile, L"SpecialK.deny.");
  lstrcatW (wszBlacklistFile, SK_GetHostApp ());

  SK_PathRemoveExtension (wszBlacklistFile);

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
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"Bethesda.net_Launcher.exe"))    ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"UbisoftGameLauncher.exe"))      ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"UbisoftGameLauncher64.exe"))    ||

       // Other Stuff
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"zosSteamStarter.exe"))          ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"notepad.exe"))                  ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"7zFM.exe"))                     ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"WinRar.exe"))                   ||

       // Misc. Tools
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"SleepOnLan.exe"))               ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"ds3t.exe"))                     ||
       (! SK_Path_wcsicmp (SK_GetHostApp(),L"tzt.exe")) )
    return false;

  // This is a fatal combination
  if (SK_IsInjected () && SK_IsHostAppSKIM ())
    return false;

  // Only the injector version can be bypassed, the wrapper version
  //   must fully initialize or the game will not be able to use the
  //     DLL it is wrapping.
  if ( SK_IsInjected    ()         &&
       GetAsyncKeyState (VK_SHIFT) &&
       GetAsyncKeyState (VK_CONTROL) ) {
    std::pair <std::queue <DWORD>, BOOL> retval =
      SK_BypassInject ();

    if (retval.second) {
      SK_ResumeThreads (retval.first);
      return false;
    }

    SK_ResumeThreads (retval.first);
  }

  else {
    bool blacklist =
      SK_IsInjected () &&
      (GetFileAttributesW (wszBlacklistFile) != INVALID_FILE_ATTRIBUTES);

    //
    // Internal blacklist, the user will have the ability to setup their
    //   own later...
    //
    if ( blacklist ) {
      return false;
    }
  }

  _CRT_INIT (SK_GetDLL (), DLL_PROCESS_ATTACH, nullptr);

  // Allow users to centralize all files if they want
  if ( GetFileAttributes ( L"SpecialK.central" ) != INVALID_FILE_ATTRIBUTES )
    config.system.central_repository = true;

  if (SK_IsInjected ())
    config.system.central_repository = true;


  // Don't start SteamAPI if we're running the installer...
  if (SK_IsHostAppSKIM ())
    config.steam.init_delay = 0;


  wchar_t wszConfigPath [MAX_PATH + 1] = { L'\0' };
          wszConfigPath [  MAX_PATH  ] = L'\0';

          SK_RootPath   [    0     ]   = L'\0';
          SK_RootPath   [ MAX_PATH ]   = L'\0';

  if (config.system.central_repository) {
    uint32_t dwLen = MAX_PATH;

    if (! SK_IsHostAppSKIM ()) {
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
    if (! SK_IsHostAppSKIM ()) {
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


  // Do this from the startup thread
  SK_Init_MinHook ();

  extern void __stdcall SK_InitCompatBlacklist (void);
  SK_InitCompatBlacklist ();

  // Hard-code the AppID for ToZ
  if (! lstrcmpW (SK_GetHostApp (), L"Tales of Zestiria.exe"))
    config.steam.appid = 351970;

  // Game won't start from the commandline without this...
  if (! lstrcmpW (SK_GetHostApp (), L"dis1_st.exe"))
    config.steam.appid = 405900;

  ZeroMemory (&init_, sizeof init_params_t);

  init_.backend  = backend;
  init_.callback = callback;

  InitializeCriticalSectionAndSpinCount (&budget_mutex, 4000);
  InitializeCriticalSectionAndSpinCount (&init_mutex,   50000);

  EnterCriticalSection (&init_mutex);

  extern HMODULE __stdcall SK_GetDLL ();

  HANDLE hProc = GetCurrentProcess ();

  wchar_t log_fname [MAX_PATH];
  log_fname [MAX_PATH - 1] = L'\0';

  swprintf (log_fname, L"logs/%s.log", SK_IsInjected () ? L"SpecialK" : backend);

  dll_log.init (log_fname, L"w");
  dll_log.Log  (L"%s.log created",     SK_IsInjected () ? L"SpecialK" : backend);

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

  if (! SK_IsHostAppSKIM ()) {
    dll_log.LogEx (true, L"Loading user preferences from %s.ini... ", config_name);

    if (SK_LoadConfig (config_name)) {
      dll_log.LogEx (false, L"done!\n");
    } else {
      dll_log.LogEx (true, L"Loading user preferences from %s.ini... ", config_name);
      dll_log.LogEx (false, L"failed!\n");
      // If no INI file exists, write one immediately.
      dll_log.LogEx (true, L"  >> Writing base INI file, because none existed... ");
      SK_SaveConfig (config_name);
      dll_log.LogEx (false, L"done!\n");
    }
  } else {
    extern void __crc32_init (void);
    __crc32_init ();

    LeaveCriticalSection (&init_mutex);
    return true;
  }

  // Don't let Steam prevent me from attaching a debugger at startup, damnit!
  SK::Diagnostics::Debugger::Allow ();

  game_debug.init (L"logs/game_output.log", L"w");

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Init ();

  if (config.system.display_debug_out)
    SK::Diagnostics::Debugger::SpawnConsole ();

  extern void SK_TestSteamImports (HMODULE hMod);
  SK_TestSteamImports (SK_GetDLL ());

  if (GetModuleHandle (L"CSteamworks.dll")) {
    extern void SK_HookCSteamworks (void);
    SK_HookCSteamworks ();
  }

  if ( GetModuleHandle (L"steam_api.dll")   ||
       GetModuleHandle (L"steam_api64.dll") ||
       GetModuleHandle (L"SteamNative.dll") ) {
    extern void SK_HookSteamAPI (void);
    SK_HookSteamAPI ();
  }

  SK_Console* pConsole = SK_Console::getInstance ();
  pConsole->Start ();

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

// Post-shutdown achievement statistics for things like friend unlock rate
extern void SK_SteamAPI_LogAllAchievements (void);

extern "C" {
bool
WINAPI
SK_ShutdownCore (const wchar_t* backend)
{
  // These games do not handle resolution correctly
  if ( (! lstrcmpW (SK_GetHostApp (), L"DarkSoulsIII.exe")) ||
       (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe"))     ||
       (! lstrcmpW (SK_GetHostApp (), L"FFX.exe"))          ||
       (! lstrcmpW (SK_GetHostApp (), L"FFX-2.exe"))        ||
       (! lstrcmpW (SK_GetHostApp (), L"dis1_st.exe")) )
    ChangeDisplaySettingsA (nullptr, CDS_RESET);

  SK_AutoClose_Log (game_debug);
  SK_AutoClose_Log (budget_log);
  SK_AutoClose_Log ( crash_log);
  SK_AutoClose_Log (   dll_log);

  SK_UnloadImports ();

  SK_Console* pConsole = SK_Console::getInstance ();
  pConsole->End ();

  if (hPumpThread != 0) {
    dll_log.LogEx   (true, L"[ Stat OSD ] Shutting down Pump Thread... ");

    TerminateThread (hPumpThread, 0);
    CloseHandle     (hPumpThread);
    hPumpThread = 0;

    dll_log.LogEx   (false, L"done!\n");
  }

  if (budget_thread.handle != INVALID_HANDLE_VALUE) {
    config.load_balance.use = false; // Turn this off while shutting down

    dll_log.LogEx (
      true,
        L"[ DXGI 1.4 ] Shutting down Memory Budget Change Thread... "
    );

    InterlockedExchange (&budget_thread.ready, FALSE);

    DWORD dwWaitState =
      SignalObjectAndWait (budget_thread.event, budget_thread.handle,
                           1000UL, TRUE); // Give 1 second, and
                                          // then we're killing
                                          // the thing!

    if (dwWaitState == WAIT_OBJECT_0)
      dll_log.LogEx (false, L"done!\n");
    else {
      dll_log.LogEx (false, L"timed out (killing manually)!\n");
      TerminateThread (budget_thread.handle, 0);
    }

    budget_thread.handle = INVALID_HANDLE_VALUE;

    // Record the final statistics always
    budget_log.silent = false;

    budget_log.Log   (L"--------------------");
    budget_log.Log   (L"Shutdown Statistics:");
    budget_log.Log   (L"--------------------\n");

    // in %10u seconds\n",
    budget_log.Log (L" Memory Budget Changed %llu times\n",
      mem_stats [0].budget_changes);

    for (int i = 0; i < 4; i++) {
      if (mem_stats [i].max_usage > 0) {
        if (mem_stats [i].min_reserve == UINT64_MAX)
          mem_stats [i].min_reserve = 0ULL;

        if (mem_stats [i].min_over_budget == UINT64_MAX)
          mem_stats [i].min_over_budget = 0ULL;

        budget_log.LogEx (true, L" GPU%i: Min Budget:        %05llu MiB\n", i,
          mem_stats [i].min_budget >> 20ULL);
        budget_log.LogEx (true, L"       Max Budget:        %05llu MiB\n",
          mem_stats [i].max_budget >> 20ULL);

        budget_log.LogEx (true, L"       Min Usage:         %05llu MiB\n",
          mem_stats [i].min_usage >> 20ULL);
        budget_log.LogEx (true, L"       Max Usage:         %05llu MiB\n",
          mem_stats [i].max_usage >> 20ULL);

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

        budget_log.LogEx (true, L"------------------------------------\n");
        budget_log.LogEx (true, L" Minimum Over Budget:     %05llu MiB\n",
          mem_stats [i].min_over_budget >> 20ULL);
        budget_log.LogEx (true, L" Maximum Over Budget:     %05llu MiB\n",
          mem_stats [i].max_over_budget >> 20ULL);
        budget_log.LogEx (true, L"------------------------------------\n");

        budget_log.LogEx (false, L"\n");
      }
    }

    budget_thread.handle = INVALID_HANDLE_VALUE;
  }

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

  SK::Framerate::Shutdown ();

  // Hopefully these things are done by now...
  DeleteCriticalSection (&init_mutex);
  DeleteCriticalSection (&budget_mutex);

  ////////SK_UnInit_MinHook ();

  if (nvapi_init)
    sk::NVAPI::UnloadLibrary ();

  dll_log.Log (L"[ SpecialK ] Custom %s.dll Detached (pid=0x%04x)",
    backend, GetCurrentProcessId ());

  SymCleanup (GetCurrentProcess ());

  FreeLibrary (backend_dll);

  return true;
}
}



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

  if (import_tries++ == 0) {
  // Load user-defined DLLs (Late)
#ifdef _WIN64
    SK_LoadLateImports64 ();
#else
    SK_LoadLateImports32 ();
#endif

    // Steam Init: Better late than never

    if (GetModuleHandle (L"CSteamworks.dll")) {
      extern void SK_HookCSteamworks (void);
      SK_HookCSteamworks ();
    }

    if ( GetModuleHandle (L"steam_api.dll")   ||
         GetModuleHandle (L"steam_api64.dll") ||
         GetModuleHandle (L"SteamNative.dll") ) {
      extern void SK_HookSteamAPI (void);
      SK_HookSteamAPI ();
    }

    if (config.system.handle_crashes)
      SK::Diagnostics::CrashHandler::Reinstall ();
  }

  extern void SK_AdjustWindow (void);
  extern void SK_HookWinAPI   (void);

  static bool first = true;

  if (first) {
    SK_HookWinAPI ();

    if (SK_GetGameWindow () != 0) {
      SK_AdjustWindow ();
      first = false;
    }
  }

  extern void SK_DrawConsole     (void);
  extern void SK_DrawTexMgrStats (void);
  SK_DrawTexMgrStats ();
  SK_DrawOSD         ();
  SK_DrawConsole     ();
}

extern void SK_UnlockSteamAchievement (uint32_t idx);

ULONGLONG poll_interval = 0;

#if 0
#define MT_KEYBOARD
#define USE_MT_KEYS
#endif

#ifdef MT_KEYBOARD
unsigned int
__stdcall
KeyboardThread (void* user)
{
  ULONGLONG last_osd_scale { 0ULL };
  ULONGLONG last_poll      { 0ULL };

  SYSTEMTIME    stNow;
  FILETIME      ftNow;
  LARGE_INTEGER ullNow;

while (true)
{
  GetSystemTime        (&stNow);
  SystemTimeToFileTime (&stNow, &ftNow);

  ullNow.HighPart = ftNow.dwHighDateTime;
  ullNow.LowPart  = ftNow.dwLowDateTime;

  if (ullNow.QuadPart - last_osd_scale > 25ULL * poll_interval) {
    if (HIWORD (GetAsyncKeyState (config.osd.keys.expand [0])) &&
        HIWORD (GetAsyncKeyState (config.osd.keys.expand [1])) &&
        HIWORD (GetAsyncKeyState (config.osd.keys.expand [2])))
    {
      last_osd_scale = ullNow.QuadPart;
      SK_ResizeOSD (+1);
    }

    if (HIWORD (GetAsyncKeyState (config.osd.keys.shrink [0])) &&
        HIWORD (GetAsyncKeyState (config.osd.keys.shrink [1])) &&
        HIWORD (GetAsyncKeyState (config.osd.keys.shrink [2])))
    {
      last_osd_scale = ullNow.QuadPart;
      SK_ResizeOSD (-1);
    }
  }

  if (ullNow.QuadPart < last_poll + poll_interval) {
    Sleep (10);
    last_poll = ullNow.QuadPart;
  }

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
    if (! toggle_mem)
      config.mem.show = (! config.mem.show);
    toggle_mem = true;
  } else {
    toggle_mem = false;
  }

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
    if (! toggle_cpu)
      config.cpu.show = (! config.cpu.show);
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
    if (! toggle_disk)
      config.disk.show = (! config.disk.show);
    toggle_disk = true;
  } else {
    toggle_disk = false;
  }

  static bool toggle_pagefile = false;
  if (HIWORD (GetAsyncKeyState (config.pagefile.keys.toggle [0])) &&
      HIWORD (GetAsyncKeyState (config.pagefile.keys.toggle [1])) &&
      HIWORD (GetAsyncKeyState (config.pagefile.keys.toggle [2])))
  {
    if (! toggle_pagefile)
      config.pagefile.show = (! config.pagefile.show);
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
}

return 0;
}
#else

extern bool com_init;

void
DoKeyboard (void)
{
  static ULONGLONG last_osd_scale { 0ULL };
  static ULONGLONG last_poll      { 0ULL };

  SYSTEMTIME    stNow;
  FILETIME      ftNow;
  LARGE_INTEGER ullNow;

  GetSystemTime        (&stNow);
  SystemTimeToFileTime (&stNow, &ftNow);

  ullNow.HighPart = ftNow.dwHighDateTime;
  ullNow.LowPart  = ftNow.dwLowDateTime;

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
#endif

std::wstring SK_RenderAPI;

__declspec (noinline)
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
SK_EndBufferSwap (HRESULT hr, IUnknown* device)
{
  if (device != nullptr) {
    CComPtr <IDirect3DDevice9>   pDev9   = nullptr;
    CComPtr <IDirect3DDevice9Ex> pDev9Ex = nullptr;
    CComPtr <ID3D11Device>       pDev11  = nullptr;

    if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev9Ex)))) {
      SK_RenderAPI = L"D3D9Ex";
    } else if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev9)))) {
      SK_RenderAPI = L"D3D9  ";
    } else if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (&pDev11)))) {
      SK_RenderAPI = L"D3D11 ";
    } else {
      SK_RenderAPI = L"UNKNOWN";
    }
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

  //
  // TZFix has its own limiter
  //
  if (! hModTZFix) {
    SK::Framerate::GetLimiter ()->wait ();
  }

  return hr;
}

wchar_t host_app [ MAX_PATH + 2 ] = { L'\0' };

bool
__cdecl
SK_IsHostAppSKIM (void)
{
  return (wcsstr (SK_GetHostApp (), L"SKIM") != nullptr);
}

const wchar_t*
SK_GetHostApp (void)
{
  static volatile ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE)) {
    DWORD   dwProcessSize = MAX_PATH;
    wchar_t wszProcessName [MAX_PATH] = { L'\0' };

    HANDLE hProc = GetCurrentProcess ();

    QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

    wchar_t* pwszShortName = wszProcessName + lstrlenW (wszProcessName);

    while (  pwszShortName      >  wszProcessName &&
           *(pwszShortName - 1) != L'\\')
      --pwszShortName;

    lstrcpynW (host_app, pwszShortName, MAX_PATH);
  }

  return host_app;
}

// NOT the working directory, this is the directory that
//   the executable is located in.

const wchar_t*
SK_GetHostPath (void)
{
  static volatile ULONG init                = FALSE;
  static wchar_t  wszProcessName [MAX_PATH] = { L'\0' };

  if (! InterlockedCompareExchange (&init, TRUE, FALSE)) {
           DWORD   dwProcessSize = MAX_PATH;

    HANDLE hProc = GetCurrentProcess ();

    QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

    *(wcsrchr (wszProcessName, L'\\')) = L'\0';
  }

  return wszProcessName;
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