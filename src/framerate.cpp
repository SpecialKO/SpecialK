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

#include <Windows.h>

#include "framerate.h"

#include "log.h"
#include "config.h"
#include "command.h"
#include "core.h" // Hooking

#include <d3d9.h>
#include <d3d11.h>
#include <atlbase.h>
extern IDirect3DDevice9 *g_pD3D9Dev;
extern IDXGISwapChain   *g_pDXGISwap;

typedef BOOL (WINAPI *QueryPerformanceCounter_t)(_Out_ LARGE_INTEGER *lpPerformanceCount);
QueryPerformanceCounter_t QueryPerformanceCounter_Original = nullptr;

auto SK_CurrentPerf = []()->
 LARGE_INTEGER
  {
    LARGE_INTEGER                     time;
    QueryPerformanceCounter_Original (&time);
    return                            time;
  };

auto SK_DeltaPerf = [](auto delta, auto freq)->
 LARGE_INTEGER
  {
    LARGE_INTEGER time = SK_CurrentPerf ();

    time.QuadPart -= (LONGLONG)(delta * freq);

    return time;
  };

LARGE_INTEGER
SK_QueryPerf (void)
{
  return SK_CurrentPerf ();
}

LARGE_INTEGER SK::Framerate::Stats::freq;

LPVOID pfnQueryPerformanceCounter       = nullptr;
LPVOID pfnSleep                         = nullptr;

#if 0
static __declspec (thread) int           last_sleep     =   1;
static __declspec (thread) LARGE_INTEGER last_perfCount = { 0 };
#endif

typedef void (WINAPI *Sleep_pfn)(DWORD dwMilliseconds);
Sleep_pfn Sleep_Original = nullptr;

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
#if 0
  extern DWORD dwRenderThread;
  if ( GetCurrentThreadId () == dwRenderThread ) {
    if (dwMilliseconds < 10) {
      YieldProcessor ();
      return;
    }
  }
#endif

  //dll_log.Log (L"Sleep (%lu) - tid: 0x%X", dwMilliseconds, GetCurrentThreadId ());

  //last_sleep = dwMilliseconds;

  //if (config.framerate.yield_processor && dwMilliseconds == 0)
  if (dwMilliseconds == 0) {
    YieldProcessor ();
    return;
  }

#define DUAL_USE_MAX_DELTA
#ifdef DUAL_USE_MAX_DELTA
  // TODO: Stop this nonsense and make an actual parameter for this...
  //         (min sleep?)
  if (dwMilliseconds >= (DWORD)config.render.framerate.max_delta_time) {
    //if (dwMilliseconds == 0)
      //return YieldProcessor ();
#endif

    Sleep_Original (dwMilliseconds);
#ifdef DUAL_USE_MAX_DELTA
  }
#endif
}

__declspec (dllexport)
BOOL
WINAPI
QueryPerformanceCounter_Detour (_Out_ LARGE_INTEGER *lpPerformanceCount)
{
#if 0
  BOOL ret = QueryPerformanceCounter_Original (lpPerformanceCount);

  if (last_sleep > 0 /*thread_sleep [GetCurrentThreadId ()] > 0 *//*|| (! (tzf::FrameRateFix::fullscreen ||
    tzf::FrameRateFix::driver_limit_setup ||
    config.framerate.allow_windowed_mode))*/) {
    memcpy (&last_perfCount, lpPerformanceCount, sizeof (LARGE_INTEGER) );
    //thread_perf [GetCurrentThreadId ()] = last_perfCount;
    return ret;
  } else {
    static LARGE_INTEGER freq = { 0 };
    if (freq.QuadPart == 0ULL)
      QueryPerformanceFrequency (&freq);

    const float fudge_factor = config.render.framerate.fudge_factor;// * freq.QuadPart;

    last_sleep = 1;
    //thread_sleep [GetCurrentThreadId ()] = 1;

    //last_perfCount = thread_perf [GetCurrentThreadId ()];

    // Mess with the numbers slightly to prevent hiccups
    lpPerformanceCount->QuadPart += (lpPerformanceCount->QuadPart - last_perfCount.QuadPart) *
      fudge_factor/* * freq.QuadPart*/;
    memcpy (&last_perfCount, lpPerformanceCount, sizeof (LARGE_INTEGER) );
    //thread_perf [GetCurrentThreadId ()] = last_perfCount;

    return ret;
  }
#endif
  return QueryPerformanceCounter_Original (lpPerformanceCount);
}

float limiter_tolerance = 0.25f;
float target_fps        = 0.0;

void
SK::Framerate::Init (void)
{
  SK_ICommandProcessor* pCommandProc =
    SK_GetCommandProcessor ();

  // TEMP HACK BECAUSE THIS ISN'T STORED in D3D9.INI
  if (GetModuleHandle (L"AgDrag.dll"))
    config.render.framerate.max_delta_time = 5;

  if (GetModuleHandle (L"tsfix.dll"))
    config.render.framerate.max_delta_time = 0;

  pCommandProc->AddVariable ( "MaxDeltaTime",
          new SK_IVarStub <int> (&config.render.framerate.max_delta_time));

  pCommandProc->AddVariable ( "LimiterTolerance",
          new SK_IVarStub <float> (&limiter_tolerance));
  pCommandProc->AddVariable ( "TargetFPS",
          new SK_IVarStub <float> (&target_fps));

  SK_CreateDLLHook2 ( L"kernel32.dll", "QueryPerformanceCounter",
                     QueryPerformanceCounter_Detour,
          (LPVOID *)&QueryPerformanceCounter_Original,
          (LPVOID *)&pfnQueryPerformanceCounter );

  if (! GetModuleHandle (L"PrettyPrinny.dll")) {
    SK_CreateDLLHook2 ( L"kernel32.dll", "Sleep",
                       Sleep_Detour,
            (LPVOID *)&Sleep_Original,
            (LPVOID *)&pfnSleep );
  }
#if 0
  else {
    QueryPerformanceCounter_Original =
      (QueryPerformanceCounter_t)
        GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                           "QueryPerformanceCounter" );
  }
#endif

  MH_ApplyQueued ();
}

void
SK::Framerate::Shutdown (void)
{
  // Removing hooks is dangerous, likely because something else
  //   has hooked many of these functions...

////////  SK_DisableHook (pfnSleep);

  // Disabling the hook is no safer, apparently

#if 0
  SK_RemoveHook (pfnSleep);
  SK_RemoveHook (pfnQueryPerformanceCounter);
#endif
}

SK::Framerate::Limiter::Limiter (double target)
{
  init (target);
}

void
SK::Framerate::Limiter::init (double target)
{
  QueryPerformanceFrequency (&freq);

  ms  = 1000.0 / target;
  fps = target;

  frames = 0;

  CComPtr <IDirect3DDevice9Ex> d3d9ex = nullptr;

  // D3D9
  if (g_pD3D9Dev != nullptr) {
    g_pD3D9Dev->QueryInterface ( IID_PPV_ARGS (&d3d9ex) );

    // Align the start to VBlank for minimum input latency
    if (d3d9ex != nullptr) {
      d3d9ex->SetMaximumFrameLatency (1);
      d3d9ex->WaitForVBlank          (0);
      d3d9ex->SetMaximumFrameLatency (
        config.render.framerate.pre_render_limit == -1 ?
             2 : config.render.framerate.pre_render_limit );
    }
  }

  CComPtr <IDXGISwapChain> dxgi_swap = nullptr;

  // D3D11
  if (g_pDXGISwap != nullptr) {
    g_pDXGISwap->QueryInterface ( IID_PPV_ARGS (&dxgi_swap) );

    if (dxgi_swap != nullptr) {
      CComPtr <IDXGIOutput> output = nullptr;
      if (SUCCEEDED (dxgi_swap->GetContainingOutput (&output))) {
        output->WaitForVBlank ();
      }
    }
  }

  QueryPerformanceCounter_Original (&start);

  next.QuadPart = 0ULL;
  time.QuadPart = 0ULL;
  last.QuadPart = 0ULL;

  last.QuadPart = start.QuadPart - (ms / 1000.0) * freq.QuadPart;
  next.QuadPart = start.QuadPart + (ms / 1000.0) * freq.QuadPart;
}

bool
SK::Framerate::Limiter::try_wait (void)
{
  if (target_fps == 0)
    return false;

  LARGE_INTEGER next_;
  next_.QuadPart = (start.QuadPart + (double)(frames+1) * (ms / 1000.0) * (double)freq.QuadPart);

  QueryPerformanceCounter_Original (&time);

  if (time.QuadPart < next_.QuadPart)
    return true;

  return false;
}

void
SK::Framerate::Limiter::wait (void)
{
  static bool restart = false;

  if (fps != target_fps)
    init (target_fps);

  if (target_fps == 0)
    return;

  frames++;

  QueryPerformanceCounter_Original (&time);

  // Actual frametime before we forced a delay
  effective_ms = 1000.0 * ((double)(time.QuadPart - last.QuadPart) / (double)freq.QuadPart);

  if ((double)(time.QuadPart - next.QuadPart) / (double)freq.QuadPart / (ms / 1000.0) > (limiter_tolerance * fps)) {
    //dll_log.Log ( L" * Frame ran long (%3.01fx expected) - restarting"
                  //L" limiter...",
            //(double)(time.QuadPart - next.QuadPart) / (double)freq.QuadPart / (ms / 1000.0) / fps );
    restart = true;
  }

  if (restart) {
    frames         = 0;
    start.QuadPart = time.QuadPart + (ms / 1000.0) * (double)freq.QuadPart;
    restart        = false;
    //init (target_fps);
    //return;
  }

  next.QuadPart = (start.QuadPart + (double)frames * (ms / 1000.0) * (double)freq.QuadPart);

  if (next.QuadPart > 0ULL) {
    // If available (Windows 7+), wait on the swapchain
    CComPtr <IDirect3DDevice9Ex> d3d9ex = nullptr;

    // D3D9Ex
    if (g_pD3D9Dev != nullptr) {
      g_pD3D9Dev->QueryInterface ( IID_PPV_ARGS (&d3d9ex) );
    }

    CComPtr <IDXGISwapChain> dxgi_swap   = nullptr;
    CComPtr <IDXGIOutput>    dxgi_output = nullptr;

    // D3D10/11/12
    if (g_pDXGISwap != nullptr) {
      if (SUCCEEDED (g_pDXGISwap->QueryInterface ( IID_PPV_ARGS (&dxgi_swap) ))) {
        dxgi_swap->GetContainingOutput (&dxgi_output);
      }
    }


    while (time.QuadPart < next.QuadPart) {
#if 0
      if ((double)(next.QuadPart - time.QuadPart) > (0.0166667 * (double)freq.QuadPart))
        Sleep (10);
#else
      if (true/*wait_for_vblank*/ && (double)(next.QuadPart - time.QuadPart) > (0.016666666667 * (double)freq.QuadPart)) {
        if (d3d9ex != nullptr) {
          d3d9ex->WaitForVBlank (0);
        } else if (dxgi_output != nullptr) {
          dxgi_output->WaitForVBlank ();
        }
      }
#endif

#if 0
      if (GetForegroundWindow () != tzf::RenderFix::hWndDevice &&
          tzf::FrameRateFix::fullscreen) {
        //dll_log.Log (L" # Restarting framerate limiter; fullscreen Alt+Tab...");
        restart = true;
        break;
      }
#endif

      QueryPerformanceCounter_Original (&time);
    }
  }

  else {
    dll_log.Log (L"[FrameLimit] Framerate limiter lost time?! (non-monotonic clock)");
    start.QuadPart += -next.QuadPart;
  }

  last.QuadPart = time.QuadPart;
}

void
SK::Framerate::Limiter::set_limit (double target) {
  init (target);
}

double
SK::Framerate::Limiter::effective_frametime (void) {
  return effective_ms;
}


SK::Framerate::Limiter*
SK::Framerate::GetLimiter (void)
{
  static Limiter* limiter = nullptr;

  if (limiter == nullptr) {
    limiter = new Limiter (config.render.framerate.target_fps);
  }

  return limiter;
}

void
SK::Framerate::Tick (double& dt, LARGE_INTEGER& now)
{
  static LARGE_INTEGER last_frame = { 0 };

  now = SK_CurrentPerf ();

  dt = (double)(now.QuadPart - last_frame.QuadPart) /
        (double)SK::Framerate::Stats::freq.QuadPart;

  last_frame = now;
};


double
SK::Framerate::Stats::calcMean (double seconds)
{
  return calcMean (SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcSqStdDev (double mean, double seconds)
{
  return calcSqStdDev (mean, SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcMin (double seconds)
{
  return calcMin (SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcMax (double seconds)
{
  return calcMax (SK_DeltaPerf (seconds, freq.QuadPart));
}

int
SK::Framerate::Stats::calcHitches (double tolerance, double mean, double seconds)
{
  return calcHitches (tolerance, mean, SK_DeltaPerf (seconds, freq.QuadPart));
}

int
SK::Framerate::Stats::calcNumSamples (double seconds)
{
  return calcNumSamples (SK_DeltaPerf (seconds, freq.QuadPart));
}