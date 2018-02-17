#if 0
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

#include <Windows.h>

#include <SpecialK/framerate.h>
#include <SpecialK/render/backend.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/window.h>

#include <d3d9.h>
#include <d3d11.h>
#include <atlbase.h>


SK::Framerate::Stats frame_history;
SK::Framerate::Stats frame_history2;


// Dispatch through the trampoline, rather than hook
//
using WaitForVBlank_pfn = HRESULT (STDMETHODCALLTYPE *)(
  IDXGIOutput *This
);
extern WaitForVBlank_pfn WaitForVBlank_Original;


SK::Framerate::EventCounter SK::Framerate::events;

LPVOID pfnQueryPerformanceCounter = nullptr;
LPVOID pfnSleep                   = nullptr;

Sleep_pfn                   Sleep_Original                   = nullptr;
QueryPerformanceCounter_pfn QueryPerformanceCounter_Original = nullptr;

auto SK_CurrentPerf =
 []{
     LARGE_INTEGER                     time;
     QueryPerformanceCounter_Original (&time);
     return                            time;
   };

auto SK_DeltaPerf =
 [](auto delta, auto freq)->
  LARGE_INTEGER
   {
     LARGE_INTEGER time = SK_CurrentPerf ();
   
     time.QuadPart -= static_cast <LONGLONG> (delta * freq);
   
     return time;
   };

LARGE_INTEGER
SK_QueryPerf ()
{
  return SK_CurrentPerf ();
}

LARGE_INTEGER SK::Framerate::Stats::freq;

#include <SpecialK/utility.h>

HANDLE hModSteamAPI = nullptr;

#include <SpecialK/steam_api.h>

void
SK_Thread_WaitWhilePumpingMessages (DWORD dwMilliseconds)
{
  if (SK_Win32_IsGUIThread ())
  {
    HWND hWndThis = GetActiveWindow ();
    bool bUnicode =
      IsWindowUnicode (hWndThis);

    auto PeekAndDispatch =
    [&]
    {
      MSG msg     = {      };
      msg.hwnd    = hWndThis;
      msg.message = WM_NULL ;

      // Avoid having Windows marshal Unicode messages like a dumb ass
      if (bUnicode)
      {
        if ( PeekMessageW ( &msg, hWndThis, 0, 0,
                                              PM_REMOVE | PM_QS_INPUT)
                 &&          msg.message != WM_NULL
           )
        {
          DispatchMessageW (&msg);
        }
      }

      else
      {
        if ( PeekMessageA ( &msg, hWndThis, 0, 0,
                                              PM_REMOVE | PM_QS_INPUT)
                 &&          msg.message != WM_NULL
           )
        {
          DispatchMessageA (&msg);
        }
      }
    };

    DWORD dwStartTime = timeGetTime ();
    DWORD dwEndTime   = dwStartTime + dwMilliseconds;

    while (timeGetTime () <= dwEndTime)
    {
      DWORD dwMaxWait =
        dwEndTime - timeGetTime ();

      if (dwMaxWait < INT_MAX)
      {
        if (MsgWaitForMultipleObjectsEx (0, nullptr, dwMaxWait, QS_ALLEVENTS, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0)
        {
          PeekAndDispatch ();
        }
      }
    }
  }
}

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
  // SteamAPI has some unusual logic that will fail if a call to Sleep (...)
  //   is skipped -- if the user wants to replace or eliminate sleep from the
  //     render / window thread for better frame pacing, we have to wait for
  //       Steam first!
  if (SK_GetFramesDrawn () < 30)
    return Sleep_Original (dwMilliseconds);

  //
  // 0 is a special case that only yields if there are waiting threads at the
  //   EXACT SAME thread priority level.
  //
  //  Many developers do not know this and may attempt this from a thread with
  //    altered priority, causing major problems for everyone.
  //
  ///DWORD dwThreadPrio = 0;
  ///if (dwMilliseconds == 0 && (dwThreadPrio = GetThreadPriority (GetCurrentThread ())) != THREAD_PRIORITY_NORMAL)
  ///{
  ///  if (dwThreadPrio < THREAD_PRIORITY_NORMAL)
  ///  {
  ///    static bool reported = false;
  ///    if (! reported)
  ///    {
  ///      dll_log.Log ( L"[Compliance] Sleep (0) called from thread with "
  ///                    L"altered priority (tid=%x, prio=%lu)!",
  ///                      GetCurrentThreadId (),
  ///                        dwThreadPrio );
  ///      reported = true;
  ///    }
  ///    SwitchToThread ();
  ///  }
  ///  else
  ///  {
  ///    static bool reported = false;
  ///    if (! reported)
  ///    {
  ///      dll_log.Log ( L"[Compliance] Sleep (0) called from thread with "
  ///                    L"altered priority (tid=%x, prio=%lu)!",
  ///                      GetCurrentThreadId (),
  ///                        dwThreadPrio );
  ///      reported = true;
  ///    }
  ///    dwMilliseconds = 1;
  ///  }
  ///}

#if 0
  if (SK::SteamAPI::AppID () > 0)
  {
    if (hModSteamAPI == nullptr)
    {
      hModSteamAPI = 
#ifndef _WIN64
        GetModuleHandle (L"steam_api.dll");
#else
        GetModuleHandle (L"steam_api64.dll");
#endif
    }

    if (SK_GetCallingDLL () == hModSteamAPI)
      return Sleep_Original (dwMilliseconds);
  }
#endif


  BOOL bGUIThread    = SK_Win32_IsGUIThread ();
  BOOL bRenderThread = (SK_GetCurrentRenderBackend ().thread == GetCurrentThreadId ());

  if (bRenderThread)
  {
    if (config.render.framerate.sleepless_render && dwMilliseconds != INFINITE)
    {
      static bool reported = false;
            if (! reported)
            {
              dll_log.Log (L"[FrameLimit] Sleep called from render thread: %lu ms!", dwMilliseconds);
              reported = true;
            }

      SK::Framerate::events.getRenderThreadStats ().wake (dwMilliseconds);

      if (bGUIThread)
        SK::Framerate::events.getMessagePumpStats ().wake (dwMilliseconds);

      if (dwMilliseconds <= 1)
      {
        if (GetThreadPriority (GetCurrentThread ()) == THREAD_PRIORITY_NORMAL)
          SleepEx (0, TRUE);
        else
          YieldProcessor ();
      }

      return;
    }

    SK::Framerate::events.getRenderThreadStats ().sleep  (dwMilliseconds);
  }

  if (bGUIThread)
  {
    if (config.render.framerate.sleepless_window && dwMilliseconds != INFINITE)
    {
      static bool reported = false;
            if (! reported)
            {
              dll_log.Log (L"[FrameLimit] Sleep called from GUI thread: %lu ms!", dwMilliseconds);
              reported = true;
            }

      SK::Framerate::events.getMessagePumpStats ().wake   (dwMilliseconds);

      if (bRenderThread)
        SK::Framerate::events.getMessagePumpStats ().wake (dwMilliseconds);

      SK_Thread_WaitWhilePumpingMessages (dwMilliseconds);

      return;
    }

    SK::Framerate::events.getMessagePumpStats ().sleep (dwMilliseconds);
  }

  //if (config.framerate.yield_processor && dwMilliseconds == 0)
  if (dwMilliseconds == 0)
  {
    YieldProcessor ();
  //SleepEx (0, TRUE);
    return;
  }

  // TODO: Stop this nonsense and make an actual parameter for this...
  //         (min sleep?)
  if ( static_cast <DWORD> (config.render.framerate.max_delta_time) <=
                            dwMilliseconds
     )
  {
    Sleep_Original (dwMilliseconds);
  }
}

extern volatile LONG SK_BypassResult;

BOOL
WINAPI
QueryPerformanceCounter_Detour (_Out_ LARGE_INTEGER *lpPerformanceCount)
{
  return QueryPerformanceCounter_Original (lpPerformanceCount);
}

using NTSTATUS = _Return_type_success_ (return >= 0) LONG;

using NtQueryTimerResolution_pfn = NTSTATUS (NTAPI *)
(
  OUT PULONG              MinimumResolution,
  OUT PULONG              MaximumResolution,
  OUT PULONG              CurrentResolution
);

using NtSetTimerResolution_pfn = NTSTATUS (NTAPI *)
(
  IN  ULONG               DesiredResolution,
  IN  BOOLEAN             SetResolution,
  OUT PULONG              CurrentResolution
);

HMODULE                    NtDll                  = nullptr;

NtQueryTimerResolution_pfn NtQueryTimerResolution = nullptr;
NtSetTimerResolution_pfn   NtSetTimerResolution   = nullptr;


float target_fps = 0.0;

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

  pCommandProc->AddVariable ( "WaitForVBLANK",
          new SK_IVarStub <bool> (&config.render.framerate.wait_for_vblank));
  pCommandProc->AddVariable ( "MaxDeltaTime",
          new SK_IVarStub <int> (&config.render.framerate.max_delta_time));

  pCommandProc->AddVariable ( "LimiterTolerance",
          new SK_IVarStub <float> (&config.render.framerate.limiter_tolerance));
  pCommandProc->AddVariable ( "TargetFPS",
          new SK_IVarStub <float> (&target_fps));

#define NO_HOOK_QPC
#ifndef NO_HOOK_QPC
  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "QueryPerformanceCounter",
                             QueryPerformanceCounter_Detour,
    static_cast_p2p <void> (&QueryPerformanceCounter_Original),
    static_cast_p2p <void> (&pfnQueryPerformanceCounter) );
#endif

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "Sleep",
                             Sleep_Detour,
    static_cast_p2p <void> (&Sleep_Original),
    static_cast_p2p <void> (&pfnSleep) );

#ifdef NO_HOOK_QPC
    QueryPerformanceCounter_Original =
      reinterpret_cast <QueryPerformanceCounter_pfn> (
        GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                           "QueryPerformanceCounter" )
      );
#endif

  if (NtDll == nullptr)
  {
    NtDll = LoadLibrary (L"ntdll.dll");

    NtQueryTimerResolution =
      reinterpret_cast <NtQueryTimerResolution_pfn> (
        GetProcAddress (NtDll, "NtQueryTimerResolution")
      );

    NtSetTimerResolution =
      reinterpret_cast <NtSetTimerResolution_pfn> (
        GetProcAddress (NtDll, "NtSetTimerResolution")
      );

    if (NtQueryTimerResolution != nullptr &&
        NtSetTimerResolution   != nullptr)
    {
      ULONG min, max, cur;
      NtQueryTimerResolution (&min, &max, &cur);
      dll_log.Log ( L"[  Timing  ] Kernel resolution.: %f ms",
                      static_cast <float> (cur * 100)/1000000.0f );
      NtSetTimerResolution   (max, TRUE,  &cur);
      dll_log.Log ( L"[  Timing  ] New resolution....: %f ms",
                      static_cast <float> (cur * 100)/1000000.0f );

    }
  }
}

void
SK::Framerate::Shutdown (void)
{
  SK_DisableHook (pfnSleep);
  //SK_DisableHook (pfnQueryPerformanceCounter);
}

SK::Framerate::Limiter::Limiter (double target)
{
  init (target);
}

IDirect3DDevice9Ex*
SK_D3D9_GetTimingDevice (void)
{
  static auto* pTimingDevice =
    reinterpret_cast <IDirect3DDevice9Ex *> (-1);

  if (pTimingDevice == reinterpret_cast <IDirect3DDevice9Ex *> (-1))
  {
    CComPtr <IDirect3D9Ex> pD3D9Ex = nullptr;

    using Direct3DCreate9ExPROC = HRESULT (STDMETHODCALLTYPE *)(UINT           SDKVersion,
                                                                IDirect3D9Ex** d3d9ex);

    extern Direct3DCreate9ExPROC Direct3DCreate9Ex_Import;

    // For OpenGL, bootstrap D3D9
    SK_BootD3D9 ();

    HRESULT hr = (config.apis.d3d9ex.hook) ?
      Direct3DCreate9Ex_Import (D3D_SDK_VERSION, &pD3D9Ex)
                                    :
                               E_NOINTERFACE;

    HWND hwnd = nullptr;

    IDirect3DDevice9Ex* pDev9Ex = nullptr;

    if (SUCCEEDED (hr))
    {
      hwnd = 
        SK_Win32_CreateDummyWindow ();

      D3DPRESENT_PARAMETERS pparams = { };
      
      pparams.SwapEffect       = D3DSWAPEFFECT_FLIPEX;
      pparams.BackBufferFormat = D3DFMT_UNKNOWN;
      pparams.hDeviceWindow    = hwnd;
      pparams.Windowed         = TRUE;
      pparams.BackBufferCount  = 2;
      pparams.BackBufferHeight = 2;
      pparams.BackBufferWidth  = 2;
      
      if ( FAILED ( pD3D9Ex->CreateDeviceEx (
                      D3DADAPTER_DEFAULT,
                        D3DDEVTYPE_HAL,
                          hwnd,
                            D3DCREATE_HARDWARE_VERTEXPROCESSING,
                              &pparams,
                                nullptr,
                                  &pDev9Ex )
                  )
          )
      {
        pTimingDevice = nullptr;
      } else {
        pDev9Ex->AddRef ();
        pTimingDevice = pDev9Ex;
      }
    }
    else {
      pTimingDevice = nullptr;
    }
  }

  return pTimingDevice;
}

void
SK::Framerate::Limiter::init (double target)
{
  QueryPerformanceFrequency (&freq);

  ms  = 1000.0 / target;
  fps = target;

  frames = 0;

  CComPtr <IDirect3DDevice9Ex> d3d9ex      = nullptr;
  CComPtr <IDXGISwapChain>     dxgi_swap   = nullptr;
  CComPtr <IDXGIOutput>        dxgi_output = nullptr;

  SK_RenderBackend& rb =
   SK_GetCurrentRenderBackend ();

  SK_RenderAPI api = rb.api;

  if (                    api ==                    SK_RenderAPI::D3D10  ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11) ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D12) )
  {
    if (rb.swapchain != nullptr)
    {
      HRESULT hr =
        rb.swapchain->QueryInterface <IDXGISwapChain> (&dxgi_swap);

      if (SUCCEEDED (hr))
      {
        if (SUCCEEDED (dxgi_swap->GetContainingOutput (&dxgi_output)))
        {
          //WaitForVBlank_Original (dxgi_output);
          dxgi_output->WaitForVBlank ();
        }
      }
    }
  }

  // Handle D3D9 and OpenGL, we'll create a fake D3D9Ex device if needed in
  //   order to get an object we can use to wait for VBLANK...
  //
  //   The DirectX Graphics Kernel exposes this capability in a more general
  //     purpose way, and is more friendly to OpenGL than the name suggests.
  //
  //   ** Future revisions should use D3DKMT
  //
  else
  {
    if (rb.device != nullptr)
    {
      rb.device->QueryInterface ( IID_PPV_ARGS (&d3d9ex) );
    }

    // Align the start to VBlank for minimum input latency
    if (d3d9ex != nullptr || (d3d9ex = SK_D3D9_GetTimingDevice ()))
    {
      d3d9ex->SetMaximumFrameLatency (1);
      d3d9ex->WaitForVBlank          (0);
      d3d9ex->SetMaximumFrameLatency (
        config.render.framerate.pre_render_limit == -1 ?
             2 : config.render.framerate.pre_render_limit );
    }
  }

  QueryPerformanceCounter_Original (&start);

  next.QuadPart = 0ULL;
  time.QuadPart = 0ULL;
  last.QuadPart = 0ULL;

  last.QuadPart = static_cast <LONGLONG> (start.QuadPart - (ms / 1000.0) * freq.QuadPart);
  next.QuadPart = static_cast <LONGLONG> (start.QuadPart + (ms / 1000.0) * freq.QuadPart);
}

#include <SpecialK/window.h>

bool
SK::Framerate::Limiter::try_wait (void)
{
  if (target_fps <= 0.0f)
    return false;

  LARGE_INTEGER next_;

  next_.QuadPart =
    static_cast <LONGLONG> (
      start.QuadPart                               +
        static_cast <long double> (  frames + 1  ) *
                                  ( ms  / 1000.0 ) *
        static_cast <long double> ( freq.QuadPart)
    );

  QueryPerformanceCounter_Original (&time);

  if (time.QuadPart < next_.QuadPart)
    return true;

  return false;
}

void
SK::Framerate::Limiter::wait (void)
{
  SK_RunOnce ( SetThreadPriority ( GetCurrentThread (),
                                     THREAD_PRIORITY_ABOVE_NORMAL ) );

  bool restart      = false;
  bool full_restart = false;

  if (fps != target_fps)
    init (target_fps);

  if (target_fps <= 0.0f)
    return;

  frames++;

  QueryPerformanceCounter_Original (&time);

  // Actual frametime before we forced a delay
  effective_ms =
    1000.0 * ( static_cast <double> (time.QuadPart - last.QuadPart) /
               static_cast <double> (freq.QuadPart) );

  double
    overrun_ms = 
      1000.0 * ( static_cast <double> (time.QuadPart - next.QuadPart) /
                 static_cast <double> (freq.QuadPart) );
    
  if ( overrun_ms >
       config.render.framerate.limiter_tolerance * ms )
  {
    SK_LOG2 
     ( ( L" * Frame ran longer than expected (%06.3f ms overrun; "
         L"%06.3f ms max. tolerance) - restarting limiter...",
            overrun_ms,
              config.render.framerate.limiter_tolerance * ms ),
         L"FrameLimit" );

    restart = true;
  }


  // Prevent timing oscillations by limiting this to once every 15 seconds
  static ULONG last_reset        = 0UL;
  static bool  need_full_restart = false;

  double mean = frame_history2.calcMean     ();
  double sd   = frame_history2.calcSqStdDev (mean);

  // For unusually long timing events, re-sync the limiter's target time
  //   based on the display device's VBLANK period.
  //
  if (sd > config.render.framerate.limiter_tolerance && (! need_full_restart))
  {
    SK_LOG0
     ( ( L" Frametime standard deviation (%f) exceeds user tolerance [%f]"
         L", re-syncing the next frame with VBLANK",
            sd, config.render.framerate.limiter_tolerance ),
         L"FrameLimit" );

    need_full_restart = true;
  }

  if (need_full_restart && (last_reset < SK_GetFramesDrawn () - ( target_fps * 15 )))
  {
    last_reset        = SK_GetFramesDrawn ();
    full_restart      = true;
    need_full_restart = false;
  }


  if (restart || full_restart)
  {
    restart        = false;
    start.QuadPart = time.QuadPart;
    frames         = 0;

    if (full_restart)
    {
      init (target_fps);
      full_restart = false;
    }

    return;
  }

  next.QuadPart =
    static_cast <LONGLONG> (
      static_cast <long double> (start.QuadPart) +
      static_cast <long double> (    frames    ) *
                                (  ms / 1000.0 ) *
      static_cast <long double> ( freq.QuadPart)
    );

  if (next.QuadPart > 0ULL)
  {
    // If available (Windows 7+), wait on the swapchain
    CComPtr <IDirect3DDevice9Ex>  d3d9ex = nullptr;

    // D3D10/11/12
    CComPtr <IDXGISwapChain> dxgi_swap   = nullptr;
    CComPtr <IDXGIOutput>    dxgi_output = nullptr;

    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    if (config.render.framerate.wait_for_vblank)
    {
      SK_RenderAPI api = rb.api;

      if (                    api ==                    SK_RenderAPI::D3D10  ||
           static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11) ||
           static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D12) )
      {
        if (rb.swapchain != nullptr)
        {
          HRESULT hr =
            rb.swapchain->QueryInterface <IDXGISwapChain> (&dxgi_swap);

          if (SUCCEEDED (hr))
          {
            dxgi_swap->GetContainingOutput (&dxgi_output);
          }
        }
      }

      else if ( static_cast <int> (api)       &
                static_cast <int> (SK_RenderAPI::D3D9) )
      {
        if (rb.device != nullptr)
        {
          if (FAILED (rb.device->QueryInterface <IDirect3DDevice9Ex> (&d3d9ex)))
          {
            d3d9ex =
              SK_D3D9_GetTimingDevice ();
          }
        }
      }
    }

    bool bGUI =
      SK_Win32_IsGUIThread () && GetActiveWindow () == game_window.hWnd;

    bool bYielded = false;

    while (time.QuadPart < next.QuadPart)
    {
      // Attempt to use a deeper sleep when possible instead of hammering the
      //   CPU into submission ;)
      if ( ( static_cast <double> (next.QuadPart  - time.QuadPart) >
             static_cast <double> (freq.QuadPart) * 0.001 *
                                   config.render.framerate.busy_wait_limiter) &&
                                  (! (config.render.framerate.yield_once && bYielded))
         )
      {
        if ( config.render.framerate.wait_for_vblank )
        {
          if (d3d9ex != nullptr)
            d3d9ex->WaitForVBlank (0);

          else if (dxgi_output != nullptr)
            dxgi_output->WaitForVBlank ();
        }

        else if (! config.render.framerate.busy_wait_limiter)
        {                
          auto dwWaitMS =
            static_cast <DWORD>
              ( (config.render.framerate.max_sleep_percent * 10.0f) / target_fps ); // 10% of full frame

          if ( ( static_cast <double> (next.QuadPart - time.QuadPart) /
                 static_cast <double> (freq.QuadPart                ) ) * 1000.0 >
                   dwWaitMS )
          {
            if (bGUI && config.render.framerate.min_input_latency)
            {
              SK_Thread_WaitWhilePumpingMessages (dwWaitMS);
            }

            else
              SleepEx                            (dwWaitMS,   FALSE);

            bYielded = true;
          }
        }
      }

      QueryPerformanceCounter_Original (&time);
    }
  }

  else
  {
    dll_log.Log (L"[FrameLimit] Framerate limiter lost time?! (non-monotonic clock)");
    start.QuadPart += -next.QuadPart;
  }

  last.QuadPart = time.QuadPart;
}

void
SK::Framerate::Limiter::set_limit (double target)
{
  init (target);
}

double
SK::Framerate::Limiter::effective_frametime (void)
{
  return effective_ms;
}


SK::Framerate::Limiter*
SK::Framerate::GetLimiter (void)
{
  static Limiter* limiter = nullptr;

  if (limiter == nullptr)
  {
    limiter =
      new Limiter (config.render.framerate.target_fps);
  }

  return limiter;
}

void
SK::Framerate::Tick (double& dt, LARGE_INTEGER& now)
{
  static LARGE_INTEGER last_frame = { };

  now = SK_CurrentPerf ();

  dt = static_cast  <double> (now.QuadPart - last_frame.QuadPart) /
        static_cast <double> (SK::Framerate::Stats::freq.QuadPart);


  // What the bloody hell?! How do we ever get a dt value near 0?
  if (dt > 0.000001)
    frame_history.addSample (1000.0 * dt, now);
  else // Less than single-precision FP epsilon, toss this frame out
    frame_history.addSample (INFINITY, now);


  frame_history2.addSample (SK::Framerate::GetLimiter ()->effective_frametime (), now);


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


LARGE_INTEGER&
SK_GetPerfFreq (void)
{
  static LARGE_INTEGER freq = { 0UL };
  static bool          init = false;
  
  if (!init)
  {
    QueryPerformanceFrequency (&freq);
    init = true;
  }

  return freq;
}
#else
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

#include <Windows.h>

#include <SpecialK/framerate.h>
#include <SpecialK/render/backend.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/window.h>

#include <d3d9.h>
#include <d3d11.h>
#include <atlbase.h>

#include <SpecialK/tls.h>

SK::Framerate::Stats frame_history;
SK::Framerate::Stats frame_history2;

// Dispatch through the trampoline, rather than hook
//
using WaitForVBlank_pfn = HRESULT (STDMETHODCALLTYPE *)(
  IDXGIOutput *This
);
extern WaitForVBlank_pfn WaitForVBlank_Original;


SK::Framerate::EventCounter SK::Framerate::events;

LPVOID pfnQueryPerformanceCounter = nullptr;
LPVOID pfnSleep                   = nullptr;

Sleep_pfn                   Sleep_Original                   = nullptr;
QueryPerformanceCounter_pfn QueryPerformanceCounter_Original = nullptr;

auto SK_CurrentPerf =
 []{
     LARGE_INTEGER                      time;
     QueryPerformanceCounter_Original (&time);
     return                             time;
   };

auto SK_DeltaPerf =
 [](auto delta, auto freq)->
  LARGE_INTEGER
   {
     LARGE_INTEGER time = SK_CurrentPerf ();
   
     time.QuadPart -= static_cast <LONGLONG> (delta * freq);
   
     return time;
   };

LARGE_INTEGER
SK_QueryPerf ()
{
  return SK_CurrentPerf ();
}

LARGE_INTEGER SK::Framerate::Stats::freq;

#include <SpecialK/utility.h>

HANDLE hModSteamAPI = nullptr;

#include <SpecialK/steam_api.h>

void
SK_Thread_WaitWhilePumpingMessages (DWORD dwMilliseconds)
{
  if (SK_Win32_IsGUIThread ())
  {
    HWND hWndThis = GetActiveWindow ();
    bool bUnicode =
      IsWindowUnicode (hWndThis);

    auto PeekAndDispatch =
    [&]
    {
      MSG msg     = {      };
      msg.hwnd    = hWndThis;
      msg.message = WM_NULL ;

      // Avoid having Windows marshal Unicode messages like a dumb ass
      if (bUnicode)
      {
        if ( PeekMessageW ( &msg, hWndThis, 0, 0,
                                              PM_REMOVE | PM_QS_INPUT)
                 &&          msg.message != WM_NULL
           )
        {
          DispatchMessageW (&msg);
        }
      }

      else
      {
        if ( PeekMessageA ( &msg, hWndThis, 0, 0,
                                              PM_REMOVE | PM_QS_INPUT)
                 &&          msg.message != WM_NULL
           )
        {
          DispatchMessageA (&msg);
        }
      }
    };

    DWORD dwStartTime = timeGetTime ();
    DWORD dwEndTime   = dwStartTime + dwMilliseconds;

    while (timeGetTime () <= dwEndTime)
    {
      DWORD dwMaxWait =
        dwEndTime - timeGetTime ();

      if (dwMaxWait < INT_MAX)
      {
        if (MsgWaitForMultipleObjectsEx (0, nullptr, dwMaxWait, QS_ALLEVENTS, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0)
        {
          PeekAndDispatch ();
        }
      }
    }
  }
}

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
  // SteamAPI has some unusual logic that will fail if a call to Sleep (...)
  //   is skipped -- if the user wants to replace or eliminate sleep from the
  //     render / window thread for better frame pacing, we have to wait for
  //       Steam first!
  if (SK_GetFramesDrawn () < 30)
    return Sleep_Original (dwMilliseconds);

  //
  // 0 is a special case that only yields if there are waiting threads at the
  //   EXACT SAME thread priority level.
  //
  //  Many developers do not know this and may attempt this from a thread with
  //    altered priority, causing major problems for everyone.
  //
  ///DWORD dwThreadPrio = 0;
  ///if (dwMilliseconds == 0 && (dwThreadPrio = GetThreadPriority (GetCurrentThread ())) != THREAD_PRIORITY_NORMAL)
  ///{
  ///  if (dwThreadPrio < THREAD_PRIORITY_NORMAL)
  ///  {
  ///    static bool reported = false;
  ///    if (! reported)
  ///    {
  ///      dll_log.Log ( L"[Compliance] Sleep (0) called from thread with "
  ///                    L"altered priority (tid=%x, prio=%lu)!",
  ///                      GetCurrentThreadId (),
  ///                        dwThreadPrio );
  ///      reported = true;
  ///    }
  ///    SwitchToThread ();
  ///  }
  ///  else
  ///  {
  ///    static bool reported = false;
  ///    if (! reported)
  ///    {
  ///      dll_log.Log ( L"[Compliance] Sleep (0) called from thread with "
  ///                    L"altered priority (tid=%x, prio=%lu)!",
  ///                      GetCurrentThreadId (),
  ///                        dwThreadPrio );
  ///      reported = true;
  ///    }
  ///    dwMilliseconds = 1;
  ///  }
  ///}

#if 0
  if (SK::SteamAPI::AppID () > 0)
  {
    if (hModSteamAPI == nullptr)
    {
      hModSteamAPI = 
#ifndef _WIN64
        GetModuleHandle (L"steam_api.dll");
#else
        GetModuleHandle (L"steam_api64.dll");
#endif
    }

    if (SK_GetCallingDLL () == hModSteamAPI)
      return Sleep_Original (dwMilliseconds);
  }
#endif


  BOOL bGUIThread    = SK_Win32_IsGUIThread ();
  BOOL bRenderThread = (SK_GetCurrentRenderBackend ().thread == GetCurrentThreadId ());

  if (bRenderThread)
  {
    if (config.render.framerate.sleepless_render && dwMilliseconds != INFINITE)
    {
      static bool reported = false;
            if (! reported)
            {
              dll_log.Log (L"[FrameLimit] Sleep called from render thread: %lu ms!", dwMilliseconds);
              reported = true;
            }

      SK::Framerate::events.getRenderThreadStats ().wake (dwMilliseconds);

      if (bGUIThread)
        SK::Framerate::events.getMessagePumpStats ().wake (dwMilliseconds);

      if (dwMilliseconds <= 1)
      {
        if (SK_TLS_Bottom ()->win32.getThreadPriority () == THREAD_PRIORITY_NORMAL)
          SleepEx (0, TRUE);
        else
          YieldProcessor ();
      }

      return;
    }

    SK::Framerate::events.getRenderThreadStats ().sleep  (dwMilliseconds);
  }

  if (bGUIThread)
  {
    if (config.render.framerate.sleepless_window && dwMilliseconds != INFINITE)
    {
      static bool reported = false;
            if (! reported)
            {
              dll_log.Log (L"[FrameLimit] Sleep called from GUI thread: %lu ms!", dwMilliseconds);
              reported = true;
            }

      SK::Framerate::events.getMessagePumpStats ().wake   (dwMilliseconds);

      if (bRenderThread)
        SK::Framerate::events.getMessagePumpStats ().wake (dwMilliseconds);

      SK_Thread_WaitWhilePumpingMessages (dwMilliseconds);

      return;
    }

    SK::Framerate::events.getMessagePumpStats ().sleep (dwMilliseconds);
  }

  //if (config.framerate.yield_processor && dwMilliseconds == 0)
  if (dwMilliseconds == 0)
  {
    YieldProcessor ();
  //SleepEx (0, TRUE);
    return;
  }

  // TODO: Stop this nonsense and make an actual parameter for this...
  //         (min sleep?)
  if ( static_cast <DWORD> (config.render.framerate.max_delta_time) <=
                            dwMilliseconds
     )
  {
    Sleep_Original (dwMilliseconds);
  }
}

extern volatile LONG SK_BypassResult;

BOOL
WINAPI
QueryPerformanceCounter_Detour (_Out_ LARGE_INTEGER *lpPerformanceCount)
{
  return QueryPerformanceCounter_Original (lpPerformanceCount);
}

using NTSTATUS = _Return_type_success_(return >= 0) LONG;

using NtQueryTimerResolution_pfn = NTSTATUS (NTAPI *)
(
  OUT PULONG              MinimumResolution,
  OUT PULONG              MaximumResolution,
  OUT PULONG              CurrentResolution
);

using NtSetTimerResolution_pfn = NTSTATUS (NTAPI *)
(
  IN  ULONG               DesiredResolution,
  IN  BOOLEAN             SetResolution,
  OUT PULONG              CurrentResolution
);

HMODULE                    NtDll                  = nullptr;

NtQueryTimerResolution_pfn NtQueryTimerResolution = nullptr;
NtSetTimerResolution_pfn   NtSetTimerResolution   = nullptr;


float target_fps = 0.0;

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

  pCommandProc->AddVariable ( "WaitForVBLANK",
          new SK_IVarStub <bool> (&config.render.framerate.wait_for_vblank));
  pCommandProc->AddVariable ( "MaxDeltaTime",
          new SK_IVarStub <int> (&config.render.framerate.max_delta_time));

  pCommandProc->AddVariable ( "LimiterTolerance",
          new SK_IVarStub <float> (&config.render.framerate.limiter_tolerance));
  pCommandProc->AddVariable ( "TargetFPS",
          new SK_IVarStub <float> (&target_fps));

#define NO_HOOK_QPC
#ifndef NO_HOOK_QPC
  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "QueryPerformanceCounter",
                             QueryPerformanceCounter_Detour,
    static_cast_p2p <void> (&QueryPerformanceCounter_Original),
    static_cast_p2p <void> (&pfnQueryPerformanceCounter) );
#endif

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "Sleep",
                             Sleep_Detour,
    static_cast_p2p <void> (&Sleep_Original),
    static_cast_p2p <void> (&pfnSleep) );

#ifdef NO_HOOK_QPC
    QueryPerformanceCounter_Original =
      reinterpret_cast <QueryPerformanceCounter_pfn> (
        GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                           "QueryPerformanceCounter" )
      );
#endif

  if (NtDll == nullptr)
  {
    NtDll = LoadLibrary (L"ntdll.dll");

    NtQueryTimerResolution =
      reinterpret_cast <NtQueryTimerResolution_pfn> (
        GetProcAddress (NtDll, "NtQueryTimerResolution")
      );

    NtSetTimerResolution =
      reinterpret_cast <NtSetTimerResolution_pfn> (
        GetProcAddress (NtDll, "NtSetTimerResolution")
      );

    if (NtQueryTimerResolution != nullptr &&
        NtSetTimerResolution   != nullptr)
    {
      ULONG min, max, cur;
      NtQueryTimerResolution (&min, &max, &cur);
      dll_log.Log ( L"[  Timing  ] Kernel resolution.: %f ms",
                      static_cast <float> (cur * 100)/1000000.0f );
      NtSetTimerResolution   (max, TRUE,  &cur);
      dll_log.Log ( L"[  Timing  ] New resolution....: %f ms",
                      static_cast <float> (cur * 100)/1000000.0f );

    }
  }
}

void
SK::Framerate::Shutdown (void)
{
  SK_DisableHook (pfnSleep);
  //SK_DisableHook (pfnQueryPerformanceCounter);
}

SK::Framerate::Limiter::Limiter (double target)
{
  init (target);
}


IDirect3DDevice9Ex*
SK_D3D9_GetTimingDevice (void)
{
  return nullptr;

  static auto* pTimingDevice =
    reinterpret_cast <IDirect3DDevice9Ex *> (-1);

  if (pTimingDevice == reinterpret_cast <IDirect3DDevice9Ex *> (-1))
  {
    CComPtr <IDirect3D9Ex> pD3D9Ex = nullptr;

    using Direct3DCreate9ExPROC = HRESULT (STDMETHODCALLTYPE *)(UINT           SDKVersion,
                                                                IDirect3D9Ex** d3d9ex);

    extern Direct3DCreate9ExPROC Direct3DCreate9Ex_Import;

    // For OpenGL, bootstrap D3D9
    SK_BootD3D9 ();

    HRESULT hr = (config.apis.d3d9ex.hook) ?
      Direct3DCreate9Ex_Import (D3D_SDK_VERSION, &pD3D9Ex)
                                    :
                               E_NOINTERFACE;

    HWND hwnd = nullptr;

    IDirect3DDevice9Ex* pDev9Ex = nullptr;

    if (SUCCEEDED (hr))
    {
      hwnd = 
        SK_Win32_CreateDummyWindow ();

      D3DPRESENT_PARAMETERS pparams = { };
      
      pparams.SwapEffect       = D3DSWAPEFFECT_FLIPEX;
      pparams.BackBufferFormat = D3DFMT_UNKNOWN;
      pparams.hDeviceWindow    = hwnd;
      pparams.Windowed         = TRUE;
      pparams.BackBufferCount  = 2;
      pparams.BackBufferHeight = 2;
      pparams.BackBufferWidth  = 2;
      
      if ( FAILED ( pD3D9Ex->CreateDeviceEx (
                      D3DADAPTER_DEFAULT,
                        D3DDEVTYPE_HAL,
                          hwnd,
                            D3DCREATE_HARDWARE_VERTEXPROCESSING,
                              &pparams,
                                nullptr,
                                  &pDev9Ex )
                  )
          )
      {
        pTimingDevice = nullptr;
      } else {
        pDev9Ex->AddRef ();
        pTimingDevice = pDev9Ex;
      }
    }
    else {
      pTimingDevice = nullptr;
    }
  }

  return pTimingDevice;
}


void
SK::Framerate::Limiter::init (double target)
{
  QueryPerformanceFrequency (&freq);

  ms  = 1000.0 / target;
  fps = target;

  frames = 0;

  CComPtr <IDirect3DDevice9Ex> d3d9ex      = nullptr;
  CComPtr <IDXGISwapChain>     dxgi_swap   = nullptr;
  CComPtr <IDXGIOutput>        dxgi_output = nullptr;

  SK_RenderBackend& rb =
   SK_GetCurrentRenderBackend ();

  SK_RenderAPI api = rb.api;

  if (                    api ==                    SK_RenderAPI::D3D10  ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11) ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D12) )
  {
    if (rb.swapchain != nullptr)
    {
      HRESULT hr =
        rb.swapchain->QueryInterface <IDXGISwapChain> (&dxgi_swap);

      if (SUCCEEDED (hr))
      {
        if (SUCCEEDED (dxgi_swap->GetContainingOutput (&dxgi_output)))
        {
          //WaitForVBlank_Original (dxgi_output);
          dxgi_output->WaitForVBlank ();
        }
      }
    }
  }

  else if (static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D9))
  {
    if (rb.device != nullptr)
    {
      rb.device->QueryInterface ( IID_PPV_ARGS (&d3d9ex) );

      // Align the start to VBlank for minimum input latency
      if (d3d9ex != nullptr || (d3d9ex = SK_D3D9_GetTimingDevice ()))
      {
        UINT orig_latency = 3;
        d3d9ex->GetMaximumFrameLatency (&orig_latency);

        d3d9ex->SetMaximumFrameLatency (1);
        d3d9ex->WaitForVBlank          (0);
        d3d9ex->SetMaximumFrameLatency (
          config.render.framerate.pre_render_limit == -1 ?
               orig_latency : config.render.framerate.pre_render_limit );
      }
    }
  }

  QueryPerformanceCounter_Original (&start);

  next.QuadPart = 0ULL;
  time.QuadPart = 0ULL;
  last.QuadPart = 0ULL;

  last.QuadPart = static_cast <LONGLONG> (start.QuadPart - (ms / 1000.0) * freq.QuadPart);
  next.QuadPart = static_cast <LONGLONG> (start.QuadPart + (ms / 1000.0) * freq.QuadPart);
}

#include <SpecialK/window.h>

bool
SK::Framerate::Limiter::try_wait (void)
{
  if (target_fps <= 0.0f)
    return false;

  LARGE_INTEGER next_;

  next_.QuadPart =
    static_cast <LONGLONG> (
      start.QuadPart                               +
        static_cast <long double> (  frames + 1  ) *
                                  ( ms  / 1000.0 ) *
        static_cast <long double> ( freq.QuadPart)
    );

  QueryPerformanceCounter_Original (&time);

  if (time.QuadPart < next_.QuadPart)
    return true;

  return false;
}

void
SK::Framerate::Limiter::wait (void)
{
  SK_RunOnce ( SetThreadPriority ( GetCurrentThread (),
                                     THREAD_PRIORITY_ABOVE_NORMAL ) );

  static bool restart      = false;
  static bool full_restart = false;

  if (fps != target_fps)
    init (target_fps);

  if (target_fps <= 0.0f)
    return;

  frames++;

  QueryPerformanceCounter_Original (&time);

  // Actual frametime before we forced a delay
  effective_ms =
    1000.0 * ( static_cast <double> (time.QuadPart - last.QuadPart) /
               static_cast <double> (freq.QuadPart)                 );

  if ( static_cast <double> (time.QuadPart - next.QuadPart) /
       static_cast <double> (freq.QuadPart)                 /
                            ( ms / 1000.0 )                 >
      ( config.render.framerate.limiter_tolerance * fps )
     )
  {
    //dll_log.Log ( L" * Frame ran long (%3.01fx expected) - restarting"
                  //L" limiter...",
            //(double)(time.QuadPart - next.QuadPart) /
            //(double)freq.QuadPart / (ms / 1000.0) / fps );
    restart = true;

#if 0
    extern SK::Framerate::Stats frame_history;
    extern SK::Framerate::Stats frame_history2;

    double mean    = frame_history.calcMean     ();
    double sd      = frame_history.calcSqStdDev (mean);

    if (sd > 5.0f)
      full_restart = true;
#endif
  }

  if (restart || full_restart)
  {
    frames         = 0;
    start.QuadPart = static_cast <LONGLONG> (
                       static_cast <double> (time.QuadPart) +
                                            ( ms / 1000.0 ) *
                       static_cast <double> (freq.QuadPart)
                     );
    restart        = false;

    if (full_restart)
    {
      init (target_fps);
      full_restart = false;
    }
    //return;
  }

  next.QuadPart =
    static_cast <LONGLONG> (
      static_cast <long double> (start.QuadPart) +
      static_cast <long double> (    frames    ) *
                                (  ms / 1000.0 ) *
      static_cast <long double> ( freq.QuadPart)
    );

  if (next.QuadPart > 0ULL)
  {
    // If available (Windows 7+), wait on the swapchain
    CComPtr <IDirect3DDevice9Ex>  d3d9ex = nullptr;

    // D3D10/11/12
    CComPtr <IDXGISwapChain> dxgi_swap   = nullptr;
    CComPtr <IDXGIOutput>    dxgi_output = nullptr;

    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    if (config.render.framerate.wait_for_vblank)
    {
      SK_RenderAPI api = rb.api;

      if (                    api ==                    SK_RenderAPI::D3D10  ||
           static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11) ||
           static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D12) )
      {
        if (rb.swapchain != nullptr)
        {
          HRESULT hr =
            rb.swapchain->QueryInterface <IDXGISwapChain> (&dxgi_swap);

          if (SUCCEEDED (hr))
          {
            dxgi_swap->GetContainingOutput (&dxgi_output);
          }
        }
      }

      else if ( static_cast <int> (api)       &
                static_cast <int> (SK_RenderAPI::D3D9) )
      {
        if (rb.device != nullptr)
        {
          if (FAILED (rb.device->QueryInterface <IDirect3DDevice9Ex> (&d3d9ex)))
          {
            d3d9ex =
              SK_D3D9_GetTimingDevice ();
          }
        }
      }
    }

    bool bGUI =
      SK_Win32_IsGUIThread () && GetActiveWindow () == game_window.hWnd;

    bool bYielded = false;

    while (time.QuadPart < next.QuadPart)
    {
      // Attempt to use a deeper sleep when possible instead of hammering the
      //   CPU into submission ;)
      if ( ( static_cast <double> (next.QuadPart  - time.QuadPart) >
             static_cast <double> (freq.QuadPart) * 0.001 *
                                   config.render.framerate.busy_wait_limiter) &&
                                  (! (config.render.framerate.yield_once && bYielded))
         )
      {
        if ( config.render.framerate.wait_for_vblank )
        {
          if (d3d9ex != nullptr)
            d3d9ex->WaitForVBlank (0);

          else if (dxgi_output != nullptr)
            dxgi_output->WaitForVBlank ();
        }

        else if (! config.render.framerate.busy_wait_limiter)
        {                
          auto dwWaitMS =
            static_cast <DWORD>
              ( (config.render.framerate.max_sleep_percent * 10.0f) / target_fps ); // 10% of full frame

          if ( ( static_cast <double> (next.QuadPart - time.QuadPart) /
                 static_cast <double> (freq.QuadPart                ) ) * 1000.0 >
                   dwWaitMS )
          {
            if (bGUI && config.render.framerate.min_input_latency)
            {
              SK_Thread_WaitWhilePumpingMessages (dwWaitMS);
            }

            else
              SleepEx                            (dwWaitMS,   FALSE);

            bYielded = true;
          }
        }
      }

      QueryPerformanceCounter_Original (&time);
    }
  }

  else
  {
    dll_log.Log (L"[FrameLimit] Framerate limiter lost time?! (non-monotonic clock)");
    start.QuadPart += -next.QuadPart;
  }

  last.QuadPart = time.QuadPart;
}

void
SK::Framerate::Limiter::set_limit (double target)
{
  init (target);
}

double
SK::Framerate::Limiter::effective_frametime (void)
{
  return effective_ms;
}


SK::Framerate::Limiter*
SK::Framerate::GetLimiter (void)
{
  static Limiter* limiter = nullptr;

  if (limiter == nullptr)
  {
    limiter =
      new Limiter (config.render.framerate.target_fps);
  }

  return limiter;
}

void
SK::Framerate::Tick (double& dt, LARGE_INTEGER& now)
{
  static LARGE_INTEGER last_frame = { };

  now = SK_CurrentPerf ();

  dt = static_cast  <double> (now.QuadPart - last_frame.QuadPart) /
        static_cast <double> (SK::Framerate::Stats::freq.QuadPart);


  // What the bloody hell?! How do we ever get a dt value near 0?
  if (dt > 0.000001)
    frame_history.addSample (1000.0 * dt, now);
  else // Less than single-precision FP epsilon, toss this frame out
    frame_history.addSample (INFINITY, now);


  frame_history2.addSample (SK::Framerate::GetLimiter ()->effective_frametime (), now);


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


LARGE_INTEGER&
SK_GetPerfFreq (void)
{
  static LARGE_INTEGER freq = { 0UL };
  static bool          init = false;
  
  if (!init)
  {
    QueryPerformanceFrequency (&freq);
    init = true;
  }

  return freq;
}
#endif