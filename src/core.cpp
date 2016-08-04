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

#include "core.h"
#include "stdafx.h"

#include "diagnostics/crash_handler.h"
#include "diagnostics/debug_utils.h"

#include "log.h"

#include "steam_api.h"

#pragma warning   (push)
#pragma warning   (disable: 4091)
#  define PSAPI_VERSION 1
#  include <Psapi.h>
#  include <DbgHelp.h>
#
#  pragma comment (lib, "psapi.lib")
#  pragma comment (lib, "dbghelp.lib")
#pragma warning   (pop)

#include "config.h"
#include "osd.h"
#include "io_monitor.h"
#include "import.h"
#include "console.h"
#include "command.h"
#include "framerate.h"

#include "steam_api.h"

#include <atlbase.h>

memory_stats_t mem_stats [MAX_GPU_NODES];
mem_info_t     mem_info  [NumBuffers];

HANDLE           dll_heap      = { 0 };
CRITICAL_SECTION budget_mutex  = { 0 };
CRITICAL_SECTION init_mutex    = { 0 };
volatile HANDLE  hInitThread   = { 0 };
         HANDLE  hPumpThread   = { 0 };

struct budget_thread_params_t {
  IDXGIAdapter3   *pAdapter;
  DWORD            tid;
  HANDLE           handle;
  DWORD            cookie;
  HANDLE           event;
  volatile bool    ready;
} *budget_thread = nullptr;

std::wstring host_app;

// Disable SLI memory in Batman Arkham Knight
bool USE_SLI = true;


extern "C++" void SK_DS3_InitPlugin (void);


NV_GET_CURRENT_SLI_STATE sli_state;
BOOL                     nvapi_init = FALSE;
int                      gpu_prio;
uint32_t                 frames_drawn = 0;

HMODULE                  backend_dll  = 0;

char*   szOSD;

#include <d3d9.h>

const wchar_t*
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

    if (budget_thread == nullptr) {
      // We're going to Release this interface after thread spawnning, but
      //   the running thread still needs a reference counted.
      pAdapter3->AddRef ();

      DWORD WINAPI BudgetThread (LPVOID user_data);

      budget_thread =
        (budget_thread_params_t *)
        HeapAlloc ( dll_heap,
          HEAP_ZERO_MEMORY,
          sizeof (budget_thread_params_t) );

      dll_log.LogEx (true,
        L"[ DXGI 1.4 ]   $ Spawning Memory Budget Change Thread..: ");

      budget_thread->pAdapter = pAdapter3;
      budget_thread->tid      = 0;
      budget_thread->event    = 0;
      budget_thread->ready    = false;
      budget_log.silent       = true;

      budget_thread->handle =
        CreateThread (NULL, 0, BudgetThread, (LPVOID)budget_thread,
          0, NULL);

      while (! budget_thread->ready)
        ;

      if (budget_thread->tid != 0) {
        dll_log.LogEx (false, L"tid=0x%04x\n", budget_thread->tid);

        dll_log.LogEx (true,
          L"[ DXGI 1.4 ]   %% Setting up Budget Change Notification.: ");

        HRESULT result =
          pAdapter3->RegisterVideoMemoryBudgetChangeNotificationEvent (
            budget_thread->event, &budget_thread->cookie
            );

        if (SUCCEEDED (result)) {
          dll_log.LogEx (false, L"eid=0x%x, cookie=%u\n",
            budget_thread->event, budget_thread->cookie);
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

DWORD
WINAPI BudgetThread (LPVOID user_data)
{
  budget_thread_params_t* params =
    (budget_thread_params_t *)user_data;

  if (budget_log.init ("logs/dxgi_budget.log", "w")) {
    params->tid       = GetCurrentThreadId ();
    params->event     = CreateEvent (NULL, FALSE, FALSE, L"DXGIMemoryBudget");
    budget_log.silent = true;
    params->ready     = true;
  } else {
    params->tid    = 0;
    params->ready  = true; // Not really :P
    return -1;
  }

  HANDLE hThreadHeap = HeapCreate (0, 0, 0);

  while (params->ready) {
    if (params->event == 0)
      break;

    DWORD dwWaitStatus = WaitForSingleObject (params->event,
      BUDGET_POLL_INTERVAL);

    if (! params->ready) {
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
    int nodes = max (0, node - 1);

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

#if 0
    static time_t last_flush = time (NULL);
    static time_t last_empty = time (NULL);
    static uint64_t last_budget =
      mem_info [buffer].local [0].Budget;
    static bool queued_flush = false;
    if (dwWaitStatus == WAIT_OBJECT_0 && last_budget >
      mem_info [buffer].local [0].Budget)
      queued_flush = true;
    if (FlushAllCaches != nullptr
      && (last_budget > mem_info [buffer].local [0].Budget && time (NULL) -
        last_flush > 2 ||
        (queued_flush && time (NULL) - last_flush > 2)
        )
      )
    {
      bool silence = budget_log.silent;
      budget_log.silent = false;
      if (last_budget > mem_info [buffer].local [0].Budget) {
        budget_log.Log (
          L"Flushing caches because budget shrunk... (%05u MiB --> %05u MiB)",
          last_budget >> 20ULL,
          mem_info [buffer].local [0].Budget >> 20ULL);
      } else {
        budget_log.Log (
          L"Flushing caches due to deferred budget shrink... (%u second(s))",
          2);
      }
      SetSystemFileCacheSize (-1, -1, NULL);
      FlushAllCaches (StreamMgr);
      last_flush = time (NULL);
      budget_log.Log (L" >> Compacting Process Heap...");
      HANDLE hHeap = GetProcessHeap ();
      HeapCompact (hHeap, 0);
      struct heap_opt_t {
        DWORD version;
        DWORD length;
      } heap_opt;`

        heap_opt.version = 1;
      heap_opt.length  = sizeof (heap_opt_t);
      HeapSetInformation (NULL, (_HEAP_INFORMATION_CLASS)3, &heap_opt,
        heap_opt.length);
      budget_log.silent = silence;
      queued_flush = false;
    }
#endif

    static uint64_t last_budget =
      mem_info [buffer].local [0].Budget;

    if (dwWaitStatus == WAIT_OBJECT_0 && config.load_balance.use)
    {
      INT prio = 0;

#if 0
      if (g_pDXGIDev != nullptr &&
        SUCCEEDED (g_pDXGIDev->GetGPUThreadPriority (&prio)))
      {
        if (last_budget > mem_info [buffer].local [0].Budget &&
          mem_info [buffer].local [0].CurrentUsage >
          mem_info [buffer].local [0].Budget)
        {
          if (prio > -7)
          {
            g_pDXGIDev->SetGPUThreadPriority (--prio);
          }
        }

        else if (last_budget < mem_info [buffer].local [0].Budget &&
          mem_info [buffer].local [0].CurrentUsage <
          mem_info [buffer].local [0].Budget)
        {
          if (prio < 7)
          {
            g_pDXGIDev->SetGPUThreadPriority (++prio);
          }
        }
      }
#endif

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

#if 0
      if (g_pDXGIDev != nullptr)
      {
        if (config.load_balance.use)
        {
          if (SUCCEEDED (g_pDXGIDev->GetGPUThreadPriority (&gpu_prio)))
          {
          }
        } else {
          if (gpu_prio != 0) {
            gpu_prio = 0;
            g_pDXGIDev->SetGPUThreadPriority (gpu_prio);
          }
        }
      }
#endif

      budget_log.LogEx (false, L"\n");
    }

    if (params->event != 0)
      ResetEvent (params->event);

    mem_info [0].buffer = buffer;
  }

#if 0
  if (g_pDXGIDev != nullptr) {
    // Releasing this actually causes driver crashes, so ...
    //   let it leak, what do we care?
    //ULONG refs = g_pDXGIDev->AddRef ();
    //if (refs > 2)
    //g_pDXGIDev->Release ();
    //g_pDXGIDev->Release   ();

#ifdef ALLOW_DEVICE_TRANSITION
    g_pDXGIDev->Release ();
#endif
    g_pDXGIDev = nullptr;
  }
#endif

  if (hThreadHeap != 0)
    HeapDestroy (hThreadHeap);

  return 0;
}


// Stupid solution for games that inexplicibly draw to the screen
//   without ever swapping buffers.
DWORD
WINAPI
osd_pump (LPVOID lpThreadParam)
{
  while (true) {
    Sleep ((DWORD)(config.osd.pump_interval * 1000.0f));
    SK_EndBufferSwap (S_OK, nullptr);

    static int tries = 0, init = 0;
    if ((! init) && SK::SteamAPI::AppID () == 0 && tries++ < 1200) {
      SK::SteamAPI::Init (true);

      if (SK::SteamAPI::AppID () != 0)
        init = 1;
    }
  }

  return 0;
}

void
SK_StartPerfMonThreads (void)
{
  if (config.mem.show) {
    //
    // Spawn Process Monitor Thread
    //
    if (process_stats.hThread == 0) {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning Process Monitor...  ");
      process_stats.hThread = CreateThread (NULL, 0, SK_MonitorProcess, NULL, 0, NULL);
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
      cpu_stats.hThread = CreateThread (NULL, 0, SK_MonitorCPU, NULL, 0, NULL);
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
        CreateThread (NULL, 0, SK_MonitorDisk, NULL, 0, NULL);
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
        CreateThread (NULL, 0, SK_MonitorPagefile, NULL, 0, NULL);
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
//#define DEBUG_SYMS
#ifdef DEBUG_SYMS
  dll_log.LogEx (true, L" @ Loading Debug Symbols: ");

  SymInitializeW       (GetCurrentProcess (), NULL, TRUE);
  SymRefreshModuleList (GetCurrentProcess ());

  dll_log.LogEx (false, L"done!\n");
#endif

  dll_log.Log (L"[ SpecialK ] === Initialization Finished! ===");

    // Create a thread that pumps the OSD
  if (config.osd.pump) {
    dll_log.LogEx (true, L"[ Stat OSD ] Spawning Pump Thread...      ");
    hPumpThread = CreateThread (NULL, 0, osd_pump, NULL, 0, NULL);
    if (hPumpThread != nullptr)
      dll_log.LogEx (false, L"tid=0x%04x, interval=%04.01f ms\n",
                       hPumpThread, config.osd.pump_interval * 1000.0f);
    else
      dll_log.LogEx (false, L"failed!\n");
  }

  szOSD =
    (char *)
    HeapAlloc ( dll_heap,
      HEAP_ZERO_MEMORY,
      sizeof (char) * 16384 );

  SK_StartPerfMonThreads ();

  LeaveCriticalSection (&init_mutex);
}

void
SK_InitCore (const wchar_t* backend, void* callback)
{
  EnterCriticalSection (&init_mutex);

  if (backend_dll != NULL) {
    LeaveCriticalSection (&init_mutex);
    return;
  }

  char log_fname [MAX_PATH];
  log_fname [MAX_PATH - 1] = '\0';

  sprintf (log_fname, "logs/%ws.log", backend);

  dll_log.init (log_fname, "w");
  dll_log.Log  (L"%s.log created", backend);

  dll_log.LogEx (false,
    L"------------------------------------------------------------------------"
    L"-------------------\n");

  DWORD   dwProcessSize = MAX_PATH;
  wchar_t wszProcessName [MAX_PATH];

  HANDLE hProc = GetCurrentProcess ();

  QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

  wchar_t* pwszShortName = wszProcessName + lstrlenW (wszProcessName);

  while (  pwszShortName      >  wszProcessName &&
    *(pwszShortName - 1) != L'\\')
    --pwszShortName;

  host_app = pwszShortName;

  extern HMODULE hModSelf;
  wchar_t wszModuleName  [MAX_PATH];
  GetModuleBaseName (hProc, hModSelf, wszModuleName, MAX_PATH);

  dll_log.Log (L">> (%s) [%s] <<", pwszShortName, wszModuleName);

  dll_log.LogEx (true, L"Loading user preferences from %s.ini... ", backend);

  if (SK_LoadConfig (backend)) {
    dll_log.LogEx (false, L"done!\n");
  } else {
    dll_log.LogEx (false, L"failed!\n");
    // If no INI file exists, write one immediately.
    dll_log.LogEx (true, L"  >> Writing base INI file, because none existed... ");
    SK_SaveConfig (backend);
    dll_log.LogEx (false, L"done!\n");
  }

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Init ();

  if (config.system.display_debug_out)
    SK::Diagnostics::Debugger::SpawnConsole ();

  if (! lstrcmpW (pwszShortName, L"BatmanAK.exe"))
    USE_SLI = false;

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

  dll_log.Log (L" System Directory:           %s", wszBackendDLL);

  lstrcatW (wszBackendDLL, L"\\");
  lstrcatW (wszBackendDLL, backend);
  lstrcatW (wszBackendDLL, L".dll");

  const wchar_t* dll_name = wszBackendDLL;

  if (! lstrcmpiW (wszProxyName, wszModuleName)) {
    dll_name = wszBackendDLL;
  } else {
    dll_name = wszProxyName;
  }

  bool load_proxy = false;
  extern import_t imports [SK_MAX_IMPORTS];

  for (int i = 0; i < SK_MAX_IMPORTS; i++) {
    if (imports [i].role != nullptr && imports [i].role->get_value () == backend) {
      dll_log.LogEx (true, L" Loading proxy %s.dll:    ", backend);
      dll_name   = _wcsdup (imports [i].filename->get_value ().c_str ());
      load_proxy = true;
      break;
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

  SK::Framerate::Init ();

  // Hard-code the AppID for ToZ
  //if (! lstrcmpW (pwszShortName, L"Tales of Zestiria.exe"))
    //config.steam.appid = 351970;

  // Game won't start from the commandline without this...
  if (! lstrcmpW (pwszShortName, L"dis1_st.exe"))
    config.steam.appid = 405900;

  // Load user-defined DLLs (Early)
#ifdef _WIN64
  SK_LoadEarlyImports64 ();
#else
  SK_LoadEarlyImports32 ();
#endif

  if (config.system.silent) {
    dll_log.silent = true;

    std::wstring log_fnameW (backend);
    log_fnameW += L".log";

    DeleteFile     (log_fnameW.c_str ());
  } else {
    dll_log.silent = false;
  }

  dll_log.LogEx (true, L"[  NvAPI   ] Initializing NVIDIA API          (NvAPI): ");

  nvapi_init = sk::NVAPI::InitializeLibrary (pwszShortName);

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
              ( dll_role,
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
                         pwszShortName,
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

  HMODULE hMod = GetModuleHandle (pwszShortName);

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

  SK_Console* pConsole = SK_Console::getInstance ();
  pConsole->Start ();

  typedef void (WINAPI *finish_pfn)  (void);
  typedef void (WINAPI *callback_pfn)(finish_pfn);
  callback_pfn callback_fn = (callback_pfn)callback;
  callback_fn (SK_InitFinishCallback);
}



void
WaitForInit (void)
{
  static volatile LONG init = FALSE;

  if (init)
    return;

  if (hInitThread) {
    WaitForSingleObject (hInitThread, INFINITE);
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
  if (InterlockedCompareExchange (&init, TRUE, FALSE))
    return;

  // Load user-defined DLLs (Lazy)
#ifdef _WIN64
  SK_LoadLazyImports64 ();
#else
  SK_LoadLazyImports32 ();
#endif

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Reinstall ();
}


//
// TODO : Move me somewhere more sensible...
//
class skMemCmd : public SK_Command {
public:
  SK_CommandResult execute (const char* szArgs);

  int         getNumArgs         (void) { return 2; }
  int         getNumOptionalArgs (void) { return 1; }
  int         getNumRequiredArgs (void) {
    return getNumArgs () - getNumOptionalArgs ();
  }

protected:
private:
};

SK_CommandResult
skMemCmd::execute (const char* szArgs)
{
  if (szArgs == nullptr)
    return SK_CommandResult ("mem", szArgs);

  intptr_t addr;
  char     type;
  char     val [256] = { '\0' };

  sscanf (szArgs, "%c %x %s", &type, &addr, val);

  static uint8_t* base_addr = nullptr;

  if (base_addr == nullptr) {
    base_addr = (uint8_t *)GetModuleHandle (nullptr);

    MEMORY_BASIC_INFORMATION mem_info;
    VirtualQuery (base_addr, &mem_info, sizeof mem_info);

    base_addr = (uint8_t *)mem_info.BaseAddress;

    //IMAGE_DOS_HEADER* pDOS =
      //(IMAGE_DOS_HEADER *)mem_info.AllocationBase;
    //IMAGE_NT_HEADERS* pNT  =
      //(IMAGE_NT_HEADERS *)((intptr_t)(pDOS + pDOS->e_lfanew));
  }

  addr += (intptr_t)base_addr;

  char result [512];

  switch (type) {
    case 'b':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 1, PAGE_READWRITE, &dwOld);
          uint8_t out;
          sscanf (val, "%hhx", &out);
          *(uint8_t *)addr = out;
        VirtualProtect ((LPVOID)addr, 1, dwOld, &dwOld);
      }

      sprintf (result, "%u", *(uint8_t *)addr);

      return SK_CommandResult ("mem", szArgs, result, 1);
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
      return SK_CommandResult ("mem", szArgs, result, 1);
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
      return SK_CommandResult ("mem", szArgs, result, 1);
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
      return SK_CommandResult ("mem", szArgs, result, 1);
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
      return SK_CommandResult ("mem", szArgs, result, 1);
      break;
    case 't':
      if (strlen (val)) {
        DWORD dwOld;

        VirtualProtect ((LPVOID)addr, 256, PAGE_READWRITE, &dwOld);
          strcpy ((char *)addr, val);
        VirtualProtect ((LPVOID)addr, 256, dwOld, &dwOld);
      }
      sprintf (result, "%s", (char *)addr);
      return SK_CommandResult ("mem", szArgs, result, 1);
      break;
  }

  return SK_CommandResult ("mem", szArgs);
}


struct init_params_t {
  const wchar_t* backend;
  void*          callback;
};

DWORD
WINAPI DllThread (LPVOID user)
{
  EnterCriticalSection (&init_mutex);
  {
    init_params_t* params = (init_params_t *)user;

    SK_InitCore (params->backend, params->callback);

    extern int32_t SK_D3D11_amount_to_purge;
    SK_GetCommandProcessor ()->AddVariable ("VRAM.Purge", new SK_VarStub <int32_t> ((int32_t *)&SK_D3D11_amount_to_purge));

    skMemCmd* mem = new skMemCmd ();

    SK_GetCommandProcessor ()->AddCommand ("mem", mem);

    if (host_app == L"DarkSoulsIII.exe") {
      SK_DS3_InitPlugin ();
      SK_GetCommandProcessor ()->ProcessCommandFormatted ("TargetFPS %lu", config.render.framerate.target_fps);
    }

    HeapFree (dll_heap, 0, params);
  }
  LeaveCriticalSection (&init_mutex);

  return 0;
}

extern HMODULE hModSelf;

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
                  L"[Address: %04Xh]!  (Status: \"%hs\")",
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
#if 1
  HMODULE hMod = GetModuleHandle (pwszModule);

#if 0
  if (hMod == NULL) {
    if (LoadLibraryW_Original != nullptr) {
      hMod = LoadLibraryW_Original (pwszModule);
    } else {
      hMod = LoadLibraryW (pwszModule);
    }
  }
#endif

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
#else
  MH_STATUS status =
    MH_CreateHookApi ( pwszModule,
                         pszProcName,
                           pDetour,
                             ppOriginal );
#endif

  if (status != MH_OK) {
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
SK_EnableHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_EnableHook (pTarget);

  if (status != MH_OK) {
    if (pTarget != MH_ALL_HOOKS) {
      dll_log.Log(L"[ Min Hook ] Failed to Enable Hook with Address: %04Xh!"
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

  if (status != MH_OK) {
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
    dll_log.Log ( L"[ Min Hook ] Failed to Initialize MinHook Library! "
                  L"(Status: \"%hs\")",
                    MH_StatusToString (status) );
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



bool
SK_StartupCore (const wchar_t* backend, void* callback)
{
  dll_heap = HeapCreate (HEAP_CREATE_ENABLE_EXECUTE, 0, 0);

  if (! dll_heap)
    return false;

  // Do this from the startup thread
  SK_Init_MinHook ();

  // Don't let Steam prevent me from attaching a debugger at startup, damnit!
  SK::Diagnostics::Debugger::Allow ();

  // SteamAPI DLL was already loaded... yay!
#ifdef _WIN64
  if (GetModuleHandle (L"steam_api64.dll"))
#else
  if (GetModuleHandle (L"steam_api.dll"))
#endif
    SK::SteamAPI::Init (false);


  InitializeCriticalSectionAndSpinCount (&budget_mutex,  4000);
  InitializeCriticalSectionAndSpinCount (&init_mutex,    50000);

  init_params_t *init =
    (init_params_t *)HeapAlloc ( dll_heap,
                                   HEAP_ZERO_MEMORY,
                                     sizeof (init_params_t) );

  if (! init)
    return false;

  init->backend  = backend;
  init->callback = callback;

#if 1
  hInitThread = CreateThread (NULL, 0, DllThread, init, 0, NULL);

  // Give other DXGI hookers time to queue up before processing any calls
  //   that they make. But don't wait here infinitely, or we will deadlock!

  /* Default: 0.25 secs seems adequate */
  if (hInitThread != 0)
    WaitForSingleObject (hInitThread, config.system.init_delay);
#else
  DllThread (init);
#endif

  return true;
}

extern "C" {
bool
WINAPI
SK_ShutdownCore (const wchar_t* backend)
{
  ChangeDisplaySettingsA (nullptr, CDS_RESET);

  SK_AutoClose_Log (budget_log);
  SK_AutoClose_Log ( crash_log);
  SK_AutoClose_Log (   dll_log);

  SK_UnloadImports ();

  SK_Console* pConsole = SK_Console::getInstance ();
  pConsole->End ();

  if (hPumpThread != 0) {
    dll_log.LogEx   (true, L"[ Stat OSD ] Shutting down Pump Thread... ");

    TerminateThread (hPumpThread, 0);
    hPumpThread = 0;

    dll_log.LogEx   (false, L"done!\n");
  }

  dll_log.LogEx  (true,
    L"[   RTSS   ] Closing RivaTuner Statistics Server connection... ");
  // Shutdown the OSD as early as possible to avoid complications
  SK_ReleaseOSD ();
  dll_log.LogEx  (false, L"done!\n");

  if (budget_thread != nullptr) {
    config.load_balance.use = false; // Turn this off while shutting down

    dll_log.LogEx (
      true,
        L"[ DXGI 1.4 ] Shutting down Memory Budget Change Thread... "
    );

    budget_thread->ready = false;

    DWORD dwWaitState =
      SignalObjectAndWait (budget_thread->event, budget_thread->handle,
                           1000UL, TRUE); // Give 1 second, and
                                          // then we're killing
                                          // the thing!

    if (dwWaitState == WAIT_OBJECT_0)
      dll_log.LogEx (false, L"done!\n");
    else {
      dll_log.LogEx (false, L"timed out (killing manually)!\n");
      TerminateThread (budget_thread->handle, 0);
    }

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

    //ULONG refs = 0;
    //if (budget_thread->pAdapter != nullptr) {
    //refs = budget_thread->pAdapter->AddRef ();
    //budget_log.LogEx (true, L" >> Budget Adapter has %u refs. left...\n",
    //refs - 1);

    //budget_thread->pAdapter->
    //UnregisterVideoMemoryBudgetChangeNotification
    //(budget_thread->cookie);

    //if (refs > 2)
    //budget_thread->pAdapter->Release ();
    //budget_thread->pAdapter->Release   ();
    //}

    HeapFree (dll_heap, NULL, budget_thread);
    budget_thread = nullptr;
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
    pagefile_stats.hThread       = 0;
    pagefile_stats.num_pagefiles = 0;
    dll_log.LogEx (false, L"done!\n");
  }

  if (sk::NVAPI::app_name != L"ds3t.exe") {
    dll_log.LogEx  (true, L"[ SpecialK ] Saving user preferences to %s.ini... ", backend);
    SK_SaveConfig (backend);
    dll_log.LogEx  (false, L"done!\n");
  }

  dll_log.LogEx  (true, L"[ SteamAPI ] Shutting down Steam API... ");
  SK::SteamAPI::Shutdown ();
  dll_log.LogEx  (false, L"done!\n");

  SK::Framerate::Shutdown ();

  // Hopefully these things are done by now...
  DeleteCriticalSection (&init_mutex);
  DeleteCriticalSection (&budget_mutex);

  ////////SK_UnInit_MinHook ();

  if (nvapi_init)
    sk::NVAPI::UnloadLibrary ();

  dll_log.Log (L"[ SpecialK ] Custom %s.dll Detached (pid=0x%04x)",
    backend, GetCurrentProcessId ());

  HeapDestroy (dll_heap);

  SymCleanup (GetCurrentProcess ());

  return true;
}
}



COM_DECLSPEC_NOTHROW
void
STDMETHODCALLTYPE
SK_BeginBufferSwap (void)
{
  static int import_tries = 0;

  if (import_tries++ == 0) {
  // Load user-defined DLLs (Late)
#ifdef _WIN64
    SK_LoadLateImports64 ();
#else
    SK_LoadLateImports32 ();
#endif
  }

  static int steam_tries = 0, init = 0;
  if ((! init) && steam_tries++ < 12000) {
    // Every 15 frames, try to initialize SteamAPI again
    if (! (steam_tries % 15)) {
      if (SK::SteamAPI::AppID () != 0)
        init = 1;

      else
        SK::SteamAPI::Init (true);
    }
  }

  extern void SK_DrawConsole (void);
  SK_DrawConsole ();
}

extern void SK_UnlockSteamAchievement (int idx);

ULONGLONG poll_interval = 0;

#if 0
#define MT_KEYBOARD
#define USE_MT_KEYS
#endif

#ifdef MT_KEYBOARD
DWORD
WINAPI
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
} return 0; }
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

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
SK_EndBufferSwap (HRESULT hr, IUnknown* device)
{
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

    CreateThread (nullptr, 0, KeyboardThread, nullptr, 0, nullptr);
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

  frames_drawn++;

  //if (dll_role != DLL_ROLE::D3D9)
    SK_DrawOSD ();

  //SK::SteamAPI::Pump ();

  static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");

  //
  // TZFix has its own limiter
  //
  if (! hModTZFix) {
    SK::Framerate::GetLimiter ()->wait ();
  }

  return hr;
}