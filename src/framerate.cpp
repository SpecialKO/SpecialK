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


SK::Framerate::Limiter::frame_journal_s
SK::Framerate::Limiter::frames_of_fame;

LARGE_INTEGER                                 SK::Framerate::Stats::freq = {};
SK_LazyGlobal <SK::Framerate::EventCounter>   SK::Framerate::events;
SK_LazyGlobal <SK::Framerate::DeepFrameState> SK::Framerate::frame_history_snapshots;

SK_LazyGlobal <SK::Framerate::Stats>          frame_history;
SK_LazyGlobal <SK::Framerate::Stats>          frame_history2;


float __target_fps    = 0.0;
float __target_fps_bg = 0.0;

enum class SK_LimitApplicationSite {
  BeforeBufferSwap,
  DuringBufferSwap,
  AfterBufferSwap,
  DontCare,
  EndOfFrame // = 4 (Default)
};

void
SK::Framerate::Init (void)
{
  static std::once_flag the_wuncler;

  std::call_once (the_wuncler, [&](void)
  {
    SK_ICommandProcessor* pCommandProc =
      SK_GetCommandProcessor ();


    // TEMP HACK BECAUSE THIS ISN'T STORED in D3D9.INI
    if (SK_GetModuleHandle (L"AgDrag.dll"))
      config.render.framerate.max_delta_time = 5;

    if (SK_GetModuleHandle (L"tsfix.dll"))
      config.render.framerate.max_delta_time = 0;



    pCommandProc->AddVariable ( "LimitSite",
            new SK_IVarStub <int> (&config.render.framerate.enforcement_policy));

    pCommandProc->AddVariable ( "WaitForVBLANK",
            new SK_IVarStub <bool> (&config.render.framerate.wait_for_vblank));


    pCommandProc->AddVariable ( "TargetFPS",
            new SK_IVarStub <float> (&__target_fps));
    pCommandProc->AddVariable ( "BackgroundFPS",
            new SK_IVarStub <float> (&__target_fps_bg));

    pCommandProc->AddVariable ( "BusyWaitRatio",
            new SK_IVarStub <float> (&config.render.framerate.busy_wait_ratio));


    SK_Scheduler_Init ();


    pCommandProc->AddVariable ( "MaxDeltaTime",
        new SK_IVarStub <int> (&config.render.framerate.max_delta_time));
  });
}

void
SK::Framerate::Shutdown (void)
{
  SK_Scheduler_Shutdown ();
}


SK::Framerate::Limiter::Limiter (double target)
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
      Direct3DCreate9Ex_Import (D3D_SDK_VERSION, &pD3D9Ex.p)
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
      pparams.BackBufferFormat = D3DFMT_X8R8G8B8;
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
        pD3D9Ex.p->AddRef ();
        pDev9Ex->AddRef   ();
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


  SK_Thread_ScopedPriority
     thread_prio_boost (THREAD_PRIORITY_TIME_CRITICAL);

  // If available (Windows 8+), wait on the swapchain
  SK_ComQIPtr <IDirect3DDevice9Ex> d3d9ex (rb.device);

  // D3D10/11/12
  SK_ComQIPtr <IDXGISwapChain>     dxgi_swap (rb.swapchain);
  SK_ComPtr   <IDXGIOutput>        dxgi_output = nullptr;

  SK_RenderAPI            api  = rb.api;
  if (                    api ==                    SK_RenderAPI::D3D10  ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11) ||
       static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D12) )
  {
    if (            dxgi_swap != nullptr &&
         SUCCEEDED (dxgi_swap->GetContainingOutput (&dxgi_output)) )
    {
      //static const
      //  DXGI_PRESENT_PARAMETERS
      //    pparams { 0      , nullptr,
      //              nullptr, nullptr };
      //
      //SK_ComQIPtr      <IDXGISwapChain1>
      //     dxgi_swap1 ( dxgi_swap );
      //if ( dxgi_swap1.p != nullptr)
      //     dxgi_swap1->Present1 ( 0, DXGI_PRESENT_RESTART, &pparams);
      //dxgi_swap ->Present ( 0, DXGI_PRESENT_DO_NOT_SEQUENCE |
      //                         DXGI_PRESENT_DO_NOT_WAIT    );

      UINT                          chain_latency = 3;
      UINT                          dev_latency   = 3;
      SK_ComQIPtr <IDXGISwapChain2> dxgi_swap2 (dxgi_swap);
      SK_ComPtr   <IDXGIDevice1>    dxgi_dev1;

      dxgi_swap->GetDevice (IID_PPV_ARGS (&dxgi_dev1.p));

      if (dxgi_dev1 != nullptr)
      {   dxgi_dev1->GetMaximumFrameLatency (&dev_latency);
          dxgi_dev1->SetMaximumFrameLatency (1);
      }

      if (dxgi_swap2 != nullptr)
      {   dxgi_swap2->GetMaximumFrameLatency (&chain_latency);
          dxgi_swap2->SetMaximumFrameLatency (1);
      }

      // Dispatch through the trampoline, rather than hook
      //
      extern WaitForVBlank_pfn
             WaitForVBlank_Original;
      if (   WaitForVBlank_Original != nullptr)
             WaitForVBlank_Original (dxgi_output);
      else                           dxgi_output->WaitForVBlank ();


      if (dxgi_dev1 != nullptr)
      {   dxgi_dev1->SetMaximumFrameLatency (
            config.render.framerate.pre_render_limit == -1 ?
                                               dev_latency :
                    config.render.framerate.pre_render_limit
                                            );
      }

      if (dxgi_swap2 != nullptr)
      {   dxgi_swap2->SetMaximumFrameLatency (
            config.render.framerate.pre_render_limit == -1 ?
                                             chain_latency :
                    config.render.framerate.pre_render_limit
                                             );
      }

      return true;
    }
  }

  else //if ( static_cast <int> (api)       &
       //     static_cast <int> (SK_RenderAPI::D3D9) )
  {
    // This can be used in graphics APIs other than D3D,
    //   but it would be preferable to simply use D3DKMT
    if (d3d9ex == nullptr)
    {
      d3d9ex =
        SK_D3D9_GetTimingDevice ();
    }

    if (d3d9ex != nullptr)
    {
      UINT                             orig_latency = 3;
      d3d9ex->GetMaximumFrameLatency (&orig_latency);
      d3d9ex->SetMaximumFrameLatency (1);

      //for (size_t i = 0; i < d3d9ex->GetNumberOfSwapChains (); ++i)
      //{
        d3d9ex->WaitForVBlank (0);//static_cast <UINT> (i));
      //}
      d3d9ex->SetMaximumFrameLatency (
        config.render.framerate.pre_render_limit == -1 ?
                                          orig_latency :
        config.render.framerate.pre_render_limit
      );

      return true;
    }
  }


  return false;
}



extern bool __stdcall SK_IsGameWindowActive (void);


void
SK::Framerate::Limiter::init (double target)
{
  ms  = 1000.0 / static_cast <double> (target);
  fps =          static_cast <double> (target);

  WriteRelease64 (
    &frames, 0ULL
  );

  auto _freqQuadPart =
    SK_GetPerfFreq ().QuadPart;

  ticks_per_frame = static_cast <ULONGLONG>       (
          ( ms / 1000.00 ) * static_cast <double> (_freqQuadPart)
                                                  );


  //
  // Align the start to VBlank for minimum input latency
  //
  SK_Framerate_WaitForVBlank ();

  auto _perfQuadPart = SK_QueryPerf   ( ).QuadPart;
  auto _frames       = ReadAcquire64     (&frames);
  auto _framesDrawn  = SK_GetFramesDrawn (       );

  frames_of_fame.frames_measured.first.initClock  (_perfQuadPart);
  frames_of_fame.frames_measured.last.clock_val  = _perfQuadPart;
  frames_of_fame.frames_measured.first.initFrame  (_framesDrawn);
  frames_of_fame.frames_measured.last.frame_idx += _frames;


  WriteRelease64 ( &start, _perfQuadPart );
  WriteRelease64 ( &freq,  _freqQuadPart );
  WriteRelease64 ( &time,            0LL );
  WriteRelease64 ( &last,
                           _perfQuadPart -
         ticks_per_frame * _freqQuadPart );
  WriteRelease64 ( &next,
         ticks_per_frame * _freqQuadPart
                         + _perfQuadPart );
}


bool
SK::Framerate::Limiter::try_wait (void)
{
  if (limit_behavior != LIMIT_APPLY) {
    return false;
  }

  if (SK_IsGameWindowActive () || __target_fps_bg == 0.0f)
  {
    if (__target_fps <= 0.0f) {
      return false;
    }
  }

  LARGE_INTEGER next_;
  next_.QuadPart =
    ReadAcquire64 (&frames) * ticks_per_frame +
    ReadAcquire64 (&start );

  return
    ( SK_QueryPerf ().QuadPart < next_.QuadPart );
}


void
SK::Framerate::Limiter::wait (void)
{
  //SK_Win32_AssistStalledMessagePump (100);

  if (limit_behavior != LIMIT_APPLY) {
    return;
  }

  if (background == SK_IsGameWindowActive ())
  {
    background = (! background);
  }

  // Don't limit under certain circumstances or exiting / alt+tabbing takes
  //   longer than it should.
  if (ReadAcquire (&__SK_DLL_Ending) != 0)
    return;

  if (! background)
  {
    if (fps != __target_fps) {
         init (__target_fps);
    }
  }

  else
  {
    if (__target_fps_bg > 0.0f)
    {
      if (fps != __target_fps_bg) {
           init (__target_fps_bg);
      }
    }

    else
    {
      if (fps != __target_fps) {
           init (__target_fps);
      }
    }
  }

  if (__target_fps <= 0.0f) {
    return;
  }


  InterlockedIncrement64 (&frames);


  auto _time =
    SK_QueryPerf ().QuadPart;


  if (restart || full_restart)
  {
    if (full_restart)
    {
      init (__target_fps);
      full_restart = false;
    }

    restart        = false;
    WriteRelease64 (&frames, 0);

    auto _start =  _time  - ticks_per_frame;
    auto _next  =  _start + ticks_per_frame;

    WriteRelease64 (&start, _start);
    WriteRelease64 (&time,  _next );
    WriteRelease64 (&next,  _next );
  } else {
    WriteRelease64 (&time,  _time);
  }

  LONG64 time_  = ReadAcquire64 ( &time   ),
         start_ = ReadAcquire64 ( &start  ),
         freq_  = ReadAcquire64 ( &freq   ),
         last_  = ReadAcquire64 ( &last   ),
         next_  = ReadAcquire64 ( &frames ) * ticks_per_frame
                                            + start_;


  // Actual frametime before we forced a delay
  effective_ms =
    1000.0 * ( static_cast <double> (time_ - last_) /
               static_cast <double> (freq_)         );

  WriteRelease64 (&next, next_);

  double missed_frames,
         missing_time =
    static_cast <double> ( time_ - next_ ) /
    static_cast <double> ( ticks_per_frame );

  double edge_distance =
    modf ( missing_time, &missed_frames );

   static constexpr double dMissingTimeBoundary =    1.0;
   static constexpr double dEdgeToleranceLow    = 0.0005;
   static constexpr double dEdgeToleranceHigh   = 0.9995;

  if ( missed_frames >= dMissingTimeBoundary &&
       edge_distance >= dEdgeToleranceLow    &&
       edge_distance <= dEdgeToleranceHigh )
  {
    InterlockedAdd64 ( &frames,
         (LONG64)missed_frames );

    next_  =
      ReadAcquire64 ( &frames ) * ticks_per_frame
                                + start_;
  }

  auto
  SK_RecalcTimeToNextFrame =
  [&](void)->
    double
    {
      return
        std::max (
          ( static_cast <double> ( next_ - SK_QueryPerf ().QuadPart ) /
            static_cast <double> ( freq_ ) ),
            0.0  );
    };


  if (next_ > 0LL)
  {
    double
      to_next_in_secs =
        SK_RecalcTimeToNextFrame ();

    // busy_wait_ratio controls the ratio of scheduler-wait to busy-wait,
    //   extensive testing shows 87.5% is most reasonable on the widest
    //     selection of hardware.
    LARGE_INTEGER liDelay;
                  liDelay.QuadPart =
                    static_cast <LONGLONG> (
                      to_next_in_secs * 1000.0 *
                      (double)config.render.framerate.busy_wait_ratio
                    );


    // Create an unnamed waitable timer.
    static CHandle hLimitTimer (
      CreateWaitableTimer (nullptr, FALSE, nullptr)
    );

    // First use a kernel-waitable timer to scrub off most of the
    //   wait time without completely decimating a CPU core.
    if ( hLimitTimer != 0 && liDelay.QuadPart > 0LL)
    {
        liDelay.QuadPart =
      -(liDelay.QuadPart * 10000LL);

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

          if (to_next_in_secs <= 0.0)
          {
            break;
          }

          static constexpr
            double duS = (1000.0 * 10000.0);

          // Negative values are used to tell the Nt systemcall
          //   to use relative rather than absolute time offset.
          LARGE_INTEGER uSecs;
                        uSecs.QuadPart =
            -static_cast <LONGLONG> (duS * to_next_in_secs);


          // System Call:  NtWaitForSingleObject  [Delay = 100 ns]
          dwWait =
            SK_WaitForSingleObject_Micro ( hLimitTimer,
                                             &uSecs );

          if (dwWait != WAIT_OBJECT_0)
            dll_log->Log (L"Result of WaitForSingleObject: %x", dwWait);

          YieldProcessor ();
        }
      }
    }

    // Any remaining wait-time will degenerate into a hybrid busy-wait,
    //   this is also when VBlank synchronization is applied if user wants.
    do
    {
      DWORD dwWaitMS =
        static_cast <DWORD> (
          std::max (0.0, SK_RecalcTimeToNextFrame () * 1000.0 - 1.0)
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

      time_ =
        SK_QueryPerf ().QuadPart;
    } while (time_ <= next_);
  }

  else
  {
    dll_log->Log ( L"[FrameLimit] Framerate limiter lost time?! "
                   L"(non-monotonic clock)" );
    InterlockedAdd64 (&start, -next_);
  }

  WriteRelease64 (&time, time_);
  WriteRelease64 (&last, time_);

  SK_RunOnce (init (fps));
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
  static std::once_flag the_wuncler;
  static          std::unique_ptr <Limiter> limiter = nullptr;

  std::call_once (the_wuncler, [&](void)
  {
    SK_GetCommandProcessor ()->AddCommand (
      "SK::Framerate::ResetLimit", new skLimitResetCmd ());

    limiter =
      std::make_unique <Limiter> (config.render.framerate.target_fps);

    SK_ReleaseAssert (limiter != nullptr)
  });

  return
    limiter.get ();
}

void
SK::Framerate::Tick (bool wait, double dt, LARGE_INTEGER now)
{
  if (wait)
    SK::Framerate::GetLimiter ()->wait ();

  if (! ( frame_history.isAllocated  () &&
          frame_history2.isAllocated () ) )
  {
    // Late initialization
    Init ();
  }

  static LARGE_INTEGER _last_frame = { };

  now = SK_CurrentPerf ();
  dt  =
    static_cast <double> (now.QuadPart - _last_frame.QuadPart) /
    static_cast <double> (SK::Framerate::Stats::freq.QuadPart);

  // Statistics can be done across multiple frames, so let's
  //   do that and avoid adding extra work to the game.
  static int amortized_stats = 0;


  // Prevent inserting infinity into the dataset
  if ( std::isnormal (dt) )
  {
    if (frame_history->addSample (1000.0 * dt, now))
    {
      amortized_stats = 0;
    }
    frame_history2->addSample (
      SK::Framerate::GetLimiter ()->effective_frametime (),
        now
    );
  }

  static constexpr int _NUM_STATS = 5;

  enum class StatType {
    Mean             = 0,
    Min              = 1,
    Max              = 2,
    PercentileClass0 = 3,
    PercentileClass1 = 4,
  };

  if (amortized_stats < _NUM_STATS)
  {
    static constexpr LARGE_INTEGER
      all_samples = { 0UL, 0UL };

    SK::Framerate::Stats*
      pContainers [] =
      {
        frame_history_snapshots->mean.getPtr        (),
        frame_history_snapshots->min.getPtr         (),
        frame_history_snapshots->max.getPtr         (),
        frame_history_snapshots->percentile0.getPtr (),
        frame_history_snapshots->percentile1.getPtr ()
      };

    auto stat_idx =
      amortized_stats++;

    auto* container =
      pContainers [stat_idx];

    double sample = 0.0;

    switch (static_cast <StatType> (stat_idx))
    {
      case StatType::PercentileClass0:
      case StatType::PercentileClass1:
      {
        int idx =
          ( static_cast <StatType> (stat_idx) ==
                         StatType::PercentileClass1 ) ?
                                                    1 : 0;

        sample =
          frame_history->calcPercentile (
            SK_Framerate_GetPercentileByIdx (idx),
              all_samples
          );
      } break;

      case StatType::Mean:
      case StatType::Min:
      case StatType::Max:
      {
        using CalcSample_pfn =
          double ( SK::Framerate::Stats::* )
                    ( LARGE_INTEGER ) noexcept;

        static constexpr
          CalcSample_pfn
            FrameHistoryCalcSample_FnTbl [] =
            {
              &SK::Framerate::Stats::calcMean,
              &SK::Framerate::Stats::calcMin,
              &SK::Framerate::Stats::calcMax
            };

        auto calcSample =
          std::bind (
            FrameHistoryCalcSample_FnTbl [stat_idx],
              frame_history.getPtr (),
                std::placeholders::_1
          );

        sample =
          calcSample (all_samples);
      } break;
    }

    if (std::isnormal (sample) && now.QuadPart > 0)
    {
      container->addSample (
        sample, now
      );
    }
  }

  _last_frame = now;
};


double
SK::Framerate::Stats::calcMean (double seconds)
{
  return
    calcMean (SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcSqStdDev (double mean, double seconds)
{
  return
    calcSqStdDev (mean, SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcMin (double seconds)
{
  return
    calcMin (SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcMax (double seconds)
{
  return
    calcMax (SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcOnePercentLow (double seconds)
{
  return
    calcOnePercentLow (SK_DeltaPerf (seconds, freq.QuadPart));
}

double
SK::Framerate::Stats::calcPointOnePercentLow (double seconds)
{
  return
    calcPointOnePercentLow (SK_DeltaPerf (seconds, freq.QuadPart));
}

int
SK::Framerate::Stats::calcHitches ( double tolerance,
                                    double mean,
                                    double seconds )
{
  return
    calcHitches (tolerance, mean, SK_DeltaPerf (seconds, freq.QuadPart));
}

int
SK::Framerate::Stats::calcNumSamples (double seconds)
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
      pStats->data [idx].when = LARGE_INTEGER { 0LL, 0L };
      pStats->data [idx].val  = 0.0;
    };

  for ( auto i = 0 ; i < MAX_SAMPLES ; ++i )
  {
    _clear (mean.getPtr        (), i);
    _clear (min.getPtr         (), i);
    _clear (max.getPtr         (), i);
    _clear (percentile0.getPtr (), i);
    _clear (percentile1.getPtr (), i);
  }

  mean->samples        = 0;
  min->samples         = 0;
  max->samples         = 0;
  percentile0->samples = 0;
  percentile1->samples = 0;
}


std::vector <double>&
SK::Framerate::Stats::sortAndCacheFrametimeHistory (void) //noexcept
{
#pragma warning (push)
#pragma warning (disable: 4244)
  if (! InterlockedCompareExchange (&worker._init, 1, 0))
  {
    worker.hSignalProduce =
      SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);

    worker.hSignalConsume =
      SK_CreateEvent (nullptr, FALSE, TRUE, nullptr);

    SK_Thread_CreateEx ([](LPVOID lpUser)->DWORD
    {
      SK_Thread_SetCurrentPriority (THREAD_PRIORITY_BELOW_NORMAL);

      worker_context_s* pWorker =
        (worker_context_s *)lpUser;

      while ( WAIT_OBJECT_0 ==
                WaitForSingleObject ( pWorker->hSignalProduce,
                                        INFINITE ) )
      {
        LONG work_idx =
          ReadAcquire (&pWorker->work_idx);

        auto& kSortBuffer =
          pWorker->sorted_frame_history [work_idx];

        boost::sort::pdqsort (
          kSortBuffer.second.begin (),
          kSortBuffer.second.end   (), std::greater <> ()
        );

        kSortBuffer.first =
          SK_GetFramesDrawn ();

        InterlockedExchange (&pWorker->work_idx, work_idx ? 0 : 1);

        SetEvent (pWorker->hSignalConsume);
      }

      return 0;
    }, L"[SK] Framepacing Statistics", (LPVOID)&worker);
  }

  auto& kReadBuffer =
    worker.sorted_frame_history [
      ReadAcquire (&worker.work_idx) ? 0 : 1
    ];

  if (true)//kReadBuffer.first/*worker.ulLastFrame*/ != SK_GetFramesDrawn ())
  {
    if ( WAIT_OBJECT_0 ==
           WaitForSingleObject (worker.hSignalConsume, 0) )
    {
      LONG idx =
        ReadAcquire (&worker.work_idx);

      auto& kWriteBuffer =
        worker.sorted_frame_history [idx];
            kReadBuffer  =
        worker.sorted_frame_history [idx ? 0 : 1];

      kWriteBuffer.second.clear ();
      kWriteBuffer.first = SK_GetFramesDrawn ();

      for (const auto datum : data)
      {
        if (datum.when.QuadPart >= 0)
        {
          if (_isreal (datum.val))
          {
            kWriteBuffer.second.push_back (datum.val);
          }
        }
      }

      worker.ulLastFrame = SK_GetFramesDrawn ();

      SetEvent (worker.hSignalProduce);
    }
  }
#pragma warning (pop)

  return
    kReadBuffer.second;
}