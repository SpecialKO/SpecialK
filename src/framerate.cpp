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

#include <SpecialK/stdafx.h>

#include <SpecialK/framerate.h>
#include <SpecialK/commands/limit_reset.inl>


LARGE_INTEGER
SK_QueryPerf ()
{
  return
    SK_CurrentPerf ();
}


LARGE_INTEGER                  SK::Framerate::Stats::freq             = {};
SK::Framerate::EventCounter*   SK::Framerate::events                  = nullptr;
SK::Framerate::DeepFrameState* SK::Framerate::frame_history_snapshots = nullptr;

SK::Framerate::Stats*          frame_history                          = nullptr;
SK::Framerate::Stats*          frame_history2                         = nullptr;


float __target_fps    = 0.0;
float __target_fps_bg = 0.0;
float fHPETWeight     = 0.875f;

void
SK::Framerate::Init (void)
{
  static SK::Framerate::Stats          _frame_history;
  static SK::Framerate::Stats          _frame_history2;

  static SK::Framerate::DeepFrameState _frame_history_snapshots;
  static SK::Framerate::Stats          _snapshot_mean;
  static SK::Framerate::Stats          _snapshot_max;
  static SK::Framerate::Stats          _snapshot_min;
  static SK::Framerate::Stats          _snapshot_percentile0;
  static SK::Framerate::Stats          _snapshot_percentile1;

  static SK::Framerate::EventCounter   _events;

  static bool basic_init = false;

  if (! basic_init)
  {
    basic_init = true;

    frame_history          = &_frame_history;
    frame_history2         = &_frame_history2;

    frame_history_snapshots              = &_frame_history_snapshots;
    frame_history_snapshots->mean        = &_snapshot_mean;
    frame_history_snapshots->max         = &_snapshot_max;
    frame_history_snapshots->min         = &_snapshot_min;
    frame_history_snapshots->percentile0 = &_snapshot_percentile0;
    frame_history_snapshots->percentile1 = &_snapshot_percentile1;

    SK::Framerate::events = &_events;

    SK_ICommandProcessor* pCommandProc =
      SK_GetCommandProcessor ();



    // TEMP HACK BECAUSE THIS ISN'T STORED in D3D9.INI
    if (GetModuleHandle (L"AgDrag.dll"))
      config.render.framerate.max_delta_time = 5;

    if (GetModuleHandle (L"tsfix.dll"))
      config.render.framerate.max_delta_time = 0;



    pCommandProc->AddVariable ( "WaitForVBLANK",
            new SK_IVarStub <bool> (&config.render.framerate.wait_for_vblank));

    pCommandProc->AddVariable ( "LimiterTolerance",
            new SK_IVarStub <float> (&config.render.framerate.limiter_tolerance));
    pCommandProc->AddVariable ( "TargetFPS",
            new SK_IVarStub <float> (&__target_fps));
    pCommandProc->AddVariable ( "BackgroundFPS",
            new SK_IVarStub <float> (&__target_fps_bg));

    pCommandProc->AddVariable ( "MaxRenderAhead",
            new SK_IVarStub <int> (&config.render.framerate.max_render_ahead));

    pCommandProc->AddVariable ( "FPS.HPETWeight",
            new SK_IVarStub <float> (&fHPETWeight));


    SK_Scheduler_Init ();


    pCommandProc->AddVariable ( "MaxDeltaTime",
        new SK_IVarStub <int> (&config.render.framerate.max_delta_time));
  }
}

void
SK::Framerate::Shutdown (void)
{
  SK_Scheduler_Shutdown ();
}


SK::Framerate::Limiter::Limiter (long double target)
{
  effective_ms = 0.0;

  init (target);
}


IDirect3DDevice9Ex*
SK_D3D9_GetTimingDevice (void)
{
  static auto* pTimingDevice =
    reinterpret_cast <IDirect3DDevice9Ex *> (-1);

  if (pTimingDevice == reinterpret_cast <IDirect3DDevice9Ex *> (-1))
  {
    SK_ComPtr <IDirect3D9Ex> pD3D9Ex = nullptr;

    using Direct3DCreate9ExPROC =
      HRESULT (STDMETHODCALLTYPE *)(UINT           SDKVersion,
                                    IDirect3D9Ex** d3d9ex);

    extern Direct3DCreate9ExPROC Direct3DCreate9Ex_Import;

    // For OpenGL, bootstrap D3D9
    SK_BootD3D9 ();

    HRESULT hr = (config.apis.d3d9ex.hook) ?
      Direct3DCreate9Ex_Import (D3D_SDK_VERSION, &pD3D9Ex)
                                    :
                               E_NOINTERFACE;

    HWND                hwnd    = nullptr;
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
      }

      else
      {
        pDev9Ex->AddRef ();
        pTimingDevice = pDev9Ex;
      }
    }

    else
    {
      pTimingDevice = nullptr;
    }
  }

  return pTimingDevice;
}



bool
SK_Framerate_WaitForVBlank (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();


  // If available (Windows 8+), wait on the swapchain
  SK_ComQIPtr <IDirect3DDevice9Ex>  d3d9ex (rb.device);

  // D3D10/11/12
  SK_ComQIPtr <IDXGISwapChain> dxgi_swap   (rb.swapchain);
  SK_ComPtr   <IDXGIOutput>    dxgi_output = nullptr;



  SK_RenderAPI            api  = rb.api;
  if (                    api ==                    SK_RenderAPI::D3D10  ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11) ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D12) )
  {
    if (            dxgi_swap.p != nullptr &&
         SUCCEEDED (dxgi_swap->GetContainingOutput (&dxgi_output)) )
    {
      // Dispatch through the trampoline, rather than hook
      //
      extern WaitForVBlank_pfn
             WaitForVBlank_Original;
      if (   WaitForVBlank_Original != nullptr)
             WaitForVBlank_Original (dxgi_output);
      else                           dxgi_output->WaitForVBlank ();

      return true;
    }
  }

  else //if ( static_cast <int> (api)       &
       //     static_cast <int> (SK_RenderAPI::D3D9) )
  {
    // This can be used in graphics APIs other than D3D,
    //   but it would be preferable to simply use D3DKMT
    if (d3d9ex.p == nullptr)
    {
      d3d9ex =
        SK_D3D9_GetTimingDevice ();
    }

    if (d3d9ex.p != nullptr)
    {
      UINT                             orig_latency = 3;
      d3d9ex->GetMaximumFrameLatency (&orig_latency);

      d3d9ex->SetMaximumFrameLatency (1);
      d3d9ex->WaitForVBlank          (0);
      d3d9ex->SetMaximumFrameLatency (
        config.render.framerate.pre_render_limit == -1 ?
                                          orig_latency :
        config.render.framerate.pre_render_limit
      );

      d3d9ex->WaitForVBlank (0);

      return true;
    }
  }


  return false;
}


void
SK::Framerate::Limiter::init (long double target)
{
  LARGE_INTEGER _freq;

  QueryPerformanceFrequency (&_freq);

  ms  = 1000.0L / long_double_cast (target);
  fps =           long_double_cast (target);

  ticks_per_frame =
    static_cast <ULONGLONG> (
      ( ms / 1000.0L ) * long_double_cast ( _freq.QuadPart )
    );

  frames = 0;


  //
  // Align the start to VBlank for minimum input latency
  //
  SK_Framerate_WaitForVBlank ();



  LARGE_INTEGER _start;
  SK_QueryPerformanceCounter (&_start);

  WriteRelease64 (&start, _start.QuadPart);
  WriteRelease64 (&freq,   _freq.QuadPart);
  WriteRelease64 (&time,              0LL);
  WriteRelease64 (&last,
    static_cast <LONGLONG> (
      ReadAcquire64 (&start) - (ms / 1000.0L) * ReadAcquire64 (&freq)
    )
  );

  WriteRelease64 (&next,
    static_cast <LONGLONG> (
      ReadAcquire64 (&start) + (ms / 1000.0L) * ReadAcquire64 (&freq)
    )
  );
}


bool
SK::Framerate::Limiter::try_wait (void)
{
  if (limit_behavior != LIMIT_APPLY)
    return false;

  if (game_window.active || __target_fps_bg == 0.0f)
  {
    if (__target_fps <= 0.0f)
      return false;
  }

  LARGE_INTEGER next_;
  LARGE_INTEGER time_;

  next_.QuadPart =
    ReadAcquire64 (&start) + ReadAcquire (&frames) * ticks_per_frame;

  SK_QueryPerformanceCounter (&time_);

  WriteRelease64 (&time, time_.QuadPart);

  return
    ( ReadAcquire64 (&time) < ReadAcquire64 (&next) );
}


void
SK::Framerate::Limiter::wait (void)
{
  //SK_Win32_AssistStalledMessagePump (100);

  if (limit_behavior != LIMIT_APPLY)
    return;

  if (background == game_window.active)
  {
    background = (! background);
  }

  // Don't limit under certain circumstances or exiting / alt+tabbing takes
  //   longer than it should.
  if (ReadAcquire (&__SK_DLL_Ending))
    return;

  if (! background)
  {
    if (fps != __target_fps)
         init (__target_fps);
  }

  else
  {
    if (__target_fps_bg > 0.0f)
    {
      if (fps != __target_fps_bg)
           init (__target_fps_bg);
    }

    else
    {
      if (fps != __target_fps)
           init (__target_fps);
    }
  }

  if (__target_fps <= 0.0f)
    return;


  static auto& rb =
    SK_GetCurrentRenderBackend ();


  frames++;

  LARGE_INTEGER _time;
  SK_QueryPerformanceCounter (&_time);

  WriteRelease64 (&time, _time.QuadPart);

  // Actual frametime before we forced a delay
  effective_ms =
    1000.0L * ( long_double_cast (ReadAcquire64 (&time) - ReadAcquire64 (&last)) /
                long_double_cast (ReadAcquire64 (&freq))                       );

  WriteRelease64 (&next,
    ReadAcquire64 (&start) + ReadAcquire (&frames) * ticks_per_frame
  );

  long double missed_frames,
              missing_time =
    long_double_cast ( ReadAcquire64 (&time) - ReadAcquire64 (&next) ) /
    long_double_cast ( ticks_per_frame ),
              edge_distance =
      modfl ( missing_time, &missed_frames );

  static DWORD dwLastFullReset        = timeGetTime ();
   const DWORD dwMinTimeBetweenResets = 333L;

  if (missing_time > config.render.framerate.limiter_tolerance)
  {
    if (edge_distance > 0.333L && edge_distance < 0.666L)
    {
      if (timeGetTime () - dwMinTimeBetweenResets > dwLastFullReset)
      {
        SK_LOG1 ( ( L"Framerate limiter is running too far behind... "
                    L"(%f frames late)", missed_frames ),
                    L"Frame Rate" );

        if (missing_time > 3 * config.render.framerate.limiter_tolerance)
          full_restart = true;

        restart         = true;
        dwLastFullReset = timeGetTime ();
      }
    }
  }

  if (restart || full_restart)
  {
    if (full_restart)
    {
      init (__target_fps);
      full_restart = false;
    }

    restart        = false;
    frames         = 0;
    WriteRelease64 (&start, SK_QueryPerf ().QuadPart - ticks_per_frame);
    WriteRelease64 (&time,  ReadAcquire64 (&start)   + ticks_per_frame);
    WriteRelease64 (&next,  ReadAcquire64 (&time));
  }



  auto
    SK_RecalcTimeToNextFrame =
    [&](void)->
      long double
      {
        long double ldRet =
          ( long_double_cast ( ReadAcquire64 (&next) -
                               SK_QueryPerf ().QuadPart ) /
            long_double_cast ( ReadAcquire64 (&freq)    ) );

        if (ldRet < 0.0L)
          ldRet = 0.0L;

        return ldRet;
      };


  if (ReadAcquire64 (&next) > 0LL)
  {
    long double
      to_next_in_secs =
        SK_RecalcTimeToNextFrame ();

    // HPETWeight controls the ratio of scheduler-wait to busy-wait,
    //   extensive testing shows 87.5% is most reasonable on the widest
    //     selection of hardware.
    LARGE_INTEGER liDelay;
                  liDelay.QuadPart =
                    static_cast <LONGLONG> (
                      to_next_in_secs * 1000.0L * (long double)fHPETWeight
                    );


    // Create an unnamed waitable timer.
    static HANDLE hLimitTimer =
      CreateWaitableTimer (nullptr, FALSE, nullptr);


    // First use a kernel-waitable timer to scrub off most of the
    //   wait time without completely decimating a CPU core.
    if ( hLimitTimer != 0 && liDelay.QuadPart > 0)
    {
        liDelay.QuadPart =
      -(liDelay.QuadPart * 10000);

      // Light-weight and high-precision -- but it's not a good idea to
      //   spend more than ~90% of our projected wait time in here or
      //     we will miss deadlines frequently.
      if ( SetWaitableTimer ( hLimitTimer, &liDelay,
                                0, nullptr, nullptr, TRUE ) )
      {
        DWORD  dwWait  = WAIT_FAILED;
        while (dwWait != WAIT_OBJECT_0)
        {
          to_next_in_secs =
            SK_RecalcTimeToNextFrame ();

          if (to_next_in_secs <= 0.0L)
          {
            break;
          }

          // Negative values are used to tell the Nt systemcall
          //   to use relative rather than absolute time offset.
          LARGE_INTEGER uSecs;
                        uSecs.QuadPart =
            -static_cast <LONGLONG> (to_next_in_secs * 1000.0L * 10000.0L);


          // System Call:  NtWaitForSingleObject  [Delay = 100 ns]
          dwWait =
            SK_WaitForSingleObject_Micro ( hLimitTimer,
                  &uSecs );


          YieldProcessor ();
        }
      }
    }


    // Any remaining wait-time will degenerate into a hybrid busy-wait,
    //   this is also when VBlank synchronization is applied if user wants.
    while (ReadAcquire64 (&time) <= ReadAcquire64 (&next))
    {
      DWORD dwWaitMS =
        static_cast <DWORD> (
          std::max (0.0L, SK_RecalcTimeToNextFrame () * 1000.0L - 1.0L)
        );


      // 2+ ms remain: we can let the kernel reschedule us and still get
      //   control of the thread back before deadline in most cases.
      if (dwWaitMS > 2)
      {
        YieldProcessor ();

        // SK's Multimedia Class Scheduling Task for this thread prevents
        //   CPU starvation, but if the service is turned off, implement
        //     a fail-safe for very low framerate limits.
        if (! config.render.framerate.enable_mmcss)
        {
          // This is only practical @ 30 FPS or lower.
          if (dwWaitMS > 4)
            SK_SleepEx (1, FALSE);
        }
      }


      if ( config.render.framerate.wait_for_vblank )
      {
        SK_Framerate_WaitForVBlank ();
      }


      SK_QueryPerformanceCounter (&_time);
      WriteRelease64 (&time, _time.QuadPart);
    }
  }

  else
  {
    dll_log->Log ( L"[FrameLimit] Framerate limiter lost time?! "
                   L"(non-monotonic clock)" );
    InterlockedAdd64 (&start, -ReadAcquire64 (&next));
    //start.QuadPart += -next.QuadPart;
  }

  WriteRelease64 (&last, ReadAcquire64 (&time));
}


void
SK::Framerate::Limiter::set_limit (long double target)
{
  init (target);
}

long double
SK::Framerate::Limiter::effective_frametime (void)
{
  return effective_ms;
}


SK::Framerate::Limiter*
SK::Framerate::GetLimiter (void)
{
  static          std::unique_ptr <Limiter> limiter = nullptr;
  static volatile LONG                      init    = 0;

  if (! InterlockedCompareExchangeAcquire (&init, 1, 0))
  {
    SK_GetCommandProcessor ()->AddCommand (
      "SK::Framerate::ResetLimit", new skLimitResetCmd ());

    limiter =
      std::make_unique <Limiter> (config.render.framerate.target_fps);

    SK_ReleaseAssert (limiter != nullptr)

    if (limiter != nullptr)
      InterlockedIncrementRelease (&init);
    else
      InterlockedDecrementRelease (&init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&init, 2);

  return
    limiter.get ();
}

void
SK::Framerate::Tick (long double& dt, LARGE_INTEGER& now)
{
  if ( frame_history  == nullptr ||
       frame_history2 == nullptr )
  {
    // Late initialization
    Init ();
  }

  static LARGE_INTEGER last_frame = { };

  now = SK_CurrentPerf ();
  dt  =
    long_double_cast (now.QuadPart -  last_frame.QuadPart) /
    long_double_cast (SK::Framerate::Stats::freq.QuadPart);


  // Statistics can be done across multiple frames, so let's
  //   do that and avoid adding extra work to the game.
  static int amortized_stats = 0;


  // Prevent inserting infinity into the dataset
  if ( dt >= std::numeric_limits <long double>::epsilon () )
  {
    if (frame_history->addSample (1000.0L * dt, now))
    {
      amortized_stats = 5;
    }
    frame_history2->addSample (
      SK::Framerate::GetLimiter ()->effective_frametime (),
        now
    );
  }

  if (amortized_stats > 0)
  {
    static constexpr LARGE_INTEGER all_samples = { 0ULL };

    switch (amortized_stats--)
    {
      case 5:
        frame_history_snapshots->percentile0->addSample (
          frame_history->calcPercentile (
            SK_Framerate_GetPercentileByIdx (0),
              all_samples
          ), now
        ); break;

      case 4:
        frame_history_snapshots->percentile1->addSample (
          frame_history->calcPercentile (
            SK_Framerate_GetPercentileByIdx (1),
              all_samples
          ), now
        ); break;

      case 3:
        frame_history_snapshots->mean->addSample (
          frame_history->calcMean (all_samples), now
        ); break;

      case 2:
        frame_history_snapshots->max->addSample (
          frame_history->calcMax (all_samples), now
        ); break;

      case 1:
        frame_history_snapshots->min->addSample (
          frame_history->calcMin (all_samples), now
        ); break;
    }
  }

  last_frame = now;
};


long double
SK::Framerate::Stats::calcMean (long double seconds)
{
  return
    calcMean (SK_DeltaPerf (seconds, freq.QuadPart));
}

long double
SK::Framerate::Stats::calcSqStdDev (long double mean, long double seconds)
{
  return
    calcSqStdDev (mean, SK_DeltaPerf (seconds, freq.QuadPart));
}

long double
SK::Framerate::Stats::calcMin (long double seconds)
{
  return
    calcMin (SK_DeltaPerf (seconds, freq.QuadPart));
}

long double
SK::Framerate::Stats::calcMax (long double seconds)
{
  return
    calcMax (SK_DeltaPerf (seconds, freq.QuadPart));
}

long double
SK::Framerate::Stats::calcOnePercentLow (long double seconds)
{
  return
    calcOnePercentLow (SK_DeltaPerf (seconds, freq.QuadPart));
}

long double
SK::Framerate::Stats::calcPointOnePercentLow (long double seconds)
{
  return
    calcPointOnePercentLow (SK_DeltaPerf (seconds, freq.QuadPart));
}

int
SK::Framerate::Stats::calcHitches ( long double tolerance,
                                    long double mean,
                                    long double seconds )
{
  return
    calcHitches (tolerance, mean, SK_DeltaPerf (seconds, freq.QuadPart));
}

int
SK::Framerate::Stats::calcNumSamples (long double seconds)
{
  return
    calcNumSamples (SK_DeltaPerf (seconds, freq.QuadPart));
}

void
SK::Framerate::DeepFrameState::reset (void)
{
  auto _clear =
    [&](SK::Framerate::Stats* pStats, auto idx) ->
    void
    {
      pStats->data [idx].when = LARGE_INTEGER { 0LL };
      pStats->data [idx].val  = 0.0L;
    };

  for ( auto i = 0 ; i < MAX_SAMPLES ; ++i )
  {
    _clear (mean,       i);
    _clear (min,        i);
    _clear (max,        i);
    _clear (percentile0, i);
    _clear (percentile1, i);
  }

  mean->samples        = 0;
  min->samples         = 0;
  max->samples         = 0;
  percentile0->samples = 0;
  percentile1->samples = 0;
}

std::pair <ULONG, std::vector <long double>>
 SK::Framerate::_sorted_frame_history;