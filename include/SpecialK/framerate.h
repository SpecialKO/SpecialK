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

#ifndef __SK__FRAMERATE_H__
#define __SK__FRAMERATE_H__

#include <boost/sort/pdqsort/pdqsort.hpp>

#include <Unknwnbase.h>
#include <Windows.h>
#include <mmsystem.h>

#include <cstdint>
#include <cmath>
#include <forward_list>

#include <SpecialK/utility.h>

template <class T, class U>
constexpr T narrow_cast(U&& u)
{
  return static_cast<T>(std::forward<U>(u));
}

static constexpr auto
  _isreal =
  [](double float_val) ->
    bool
    {
      //return
      //  std::isnormal (float_val);
      return
        (! (isinf (float_val) || isnan (float_val)));
    };

float SK_Framerate_GetPercentileByIdx (int idx);

using QueryPerformanceCounter_pfn = BOOL (WINAPI *)(_Out_ LARGE_INTEGER *lpPerformanceCount);

BOOL
WINAPI
SK_QueryPerformanceCounter (_Out_ LARGE_INTEGER *lpPerformanceCount);

LARGE_INTEGER SK_GetPerfFreq (void);
LARGE_INTEGER SK_QueryPerf   (void);

__forceinline
LARGE_INTEGER
SK_CurrentPerf (void)
 {
   LARGE_INTEGER                time;
   SK_QueryPerformanceCounter (&time);
   return                       time;
 };

static auto SK_DeltaPerf =
 [](auto delta, auto freq)->
  LARGE_INTEGER
   {
     LARGE_INTEGER time =
       SK_CurrentPerf ();

     time.QuadPart -=
       gsl::narrow_cast <LONGLONG> (
         delta * freq
       );

     return
       time;
   };

static auto SK_DeltaPerfMS =
 [](auto delta, auto freq)->
  double
   {
     return
       1000.0 * (double)(SK_DeltaPerf   (delta, freq).QuadPart) /
                (double)(SK_GetPerfFreq (           ).QuadPart);
   };


extern  __forceinline
ULONG64 __stdcall
SK_GetFramesDrawn (void);


namespace SK
{
  namespace Framerate
  {
    void Init     (void);
    void Shutdown (void);

    void Tick        ( bool           wait       =  true,
                       double         dt         =   0.0,
                       LARGE_INTEGER  now        = { 0,0 },
                       IUnknown*      pSwapChain = nullptr );

    #define MAX_SAMPLES 1000

    class Stats;

    struct DeepFrameState
    {
      using
        SK_LazyStats =
          SK_LazyGlobal <
            SK::Framerate::Stats
          >;

      SK_LazyStats mean;
      SK_LazyStats min;
      SK_LazyStats max;
      SK_LazyStats percentile0;
      SK_LazyStats percentile1;

      void reset (void);
    };

    class Limiter {
    public:
      Limiter (double target = 60.0);

      ~Limiter (void)
      {
        if (           timer_wait != 0 )
          CloseHandle (timer_wait);
      }//= default;

      void            init            (double target);
      void            wait            (void);
      bool        try_wait            (void); // No actual wait, just return
                                              //  whether a wait would have occurred.

      void        set_limit           (double target);
      double      get_limit           (void) noexcept { return fps; };

      double      effective_frametime (void);

      void        set_undershoot      (float percent) { undershoot_percent = percent; };
      float       get_undershoot      (void) {   return undershoot_percent;           };

      int32_t     suspend             (void) noexcept { return ++limit_behavior;     }
      int32_t     resume              (void) noexcept { return --limit_behavior;     }
      bool        frozen              (void) noexcept { return   limit_behavior < 0; }

      void        reset (bool full = false) noexcept {
        if (full) full_restart = true;
        else           restart = true;
      }

      struct present_stats_s {
      //DXGI_FRAME_STATISTICS sequence_start = { };
        UINT                  queue_depth    =  0;
        std::vector <UINT>    frame_ids;
      };

      struct snapshot_s {
        bool        *pRestart     = nullptr;
        bool        *pFullRestart = nullptr;
        bool        *pBackground  = nullptr;

        double       ms              = 0.0;
        double       fps             = 0.0;
        double       effective_ms    = 0.0;

        uint64_t     ticks_per_frame =   0;

        volatile
          LONG64     time  = { },
                     start = { },
                     next  = { },
                     last  = { },
                     freq  = { };

        volatile
          LONG64     frames = 0;
      } rate_shot;

      snapshot_s getSnapshot (void)
      {
        rate_shot.pRestart        = &restart;
        rate_shot.pFullRestart    = &full_restart;
        rate_shot.pBackground     = &background;
        rate_shot.ms              = ms;
        rate_shot.fps             = fps;
        rate_shot.effective_ms    = effective_ms;
        rate_shot.ticks_per_frame = ticks_per_frame;

        InterlockedExchange64 (&rate_shot.time,  ReadAcquire64 (&time));
        InterlockedExchange64 (&rate_shot.start, ReadAcquire64 (&start));
        InterlockedExchange64 (&rate_shot.next,  ReadAcquire64 (&next));
        InterlockedExchange64 (&rate_shot.last,  ReadAcquire64 (&last));
        InterlockedExchange64 (&rate_shot.freq,  ReadAcquire64 (&freq));

        WriteRelease64 (&rate_shot.frames,
                   ReadAcquire64 (&frames));

        return rate_shot;
      }

      // Record keeping data pertaining to all frames SK has seen
      //   for the entire application runtime, irrespective of user-
      //     defined preferences for limiting / skipping.
      struct frame_journal_s
      {
        struct dataspread_s
        {
          struct
          {
            LONG64 frame_idx = -1;
            LONG64 clock_val =  0;

            void initFrame (LONG64 lvi) { frame_idx =
             ( frame_idx == -1 ) ? lvi  : frame_idx;};
            void initClock (LONG64 lvc) { clock_val =
             ( clock_val ==  0 ) ? lvc  : clock_val;};
          } first, last;

          LONG64
            count (void) const
            { return
               last.frame_idx -
              first.frame_idx;
            }
          LONG64
            duration (void) const
            { return
                 last.clock_val -
                first.clock_val;
            }
        } frames_measured;

        struct {
          dataspread_s span = { { -1, 0 },
                                { -1, 0 } };
        } limiter_resets;

      //LONG64 time_waited = 0; // TODO
      } frames_of_fame;

      struct amortized_stats_s {
        int            phase      =  0;
        LARGE_INTEGER _last_frame = { };
      } amortization;

      SK_LazyGlobal <Stats> frame_history;
      SK_LazyGlobal <Stats> frame_history2;
            DeepFrameState  frame_history_snapshots;

      static
      float         undershoot_percent;

      static
      double        timer_res_ms;

    private:

      bool          restart      = false;
      bool          full_restart = false;
      bool          background   = false;

      double        ms           = 0.0,
                    fps          = 0.0,
                    effective_ms = 0.0;

      ULONGLONG     ticks_per_frame     = 0ULL;
      ULONGLONG     ticks_to_undershoot = 0ULL;

      // Don't align timing perfectly on a VBlank interval, because DWM composition is
      //   asynchronous to our present queue and we want to arrive with work to submit
      //     before VBlank begins, not _after_.

      volatile
        LONG64      time   = { },
                    start  = { },
                    next   = { },
                    last   = { },
                    freq   = { };
      volatile
        LONG64      frames = 0;
      volatile
        LONG64      last_frame = 0; // Never apply a limit twice in one frame

#define LIMIT_APPLY     0
#define LIMIT_UNDERFLOW (limit_behavvior < 0)
#define LIMIT_SUSPENDED (limit_behavivor > 0)

      // 0 = Limiter runs, < 0 = Reference Counting Bug (dumbass)
      //                   > 0 = Temporarily Ignore Limits
      int32_t        limit_behavior =
                     LIMIT_APPLY;

      HANDLE         timer_wait   = 0;
      bool           lazy_init    = false;
    };

    using EventCounter = class EventCounter_V1;

    class EventCounter_V1
    {
    public:
      class SleepStats
      {
      public:
        volatile ULONG attempts   = 0UL,
                       rejections = 0UL;

        struct
        {
          volatile LONG deprived = 0ULL,
                        allowed  = 0ULL;
        } time;


        void sleep (DWORD dwMilliseconds) { InterlockedIncrement (&attempts);
                                            InterlockedAdd       (&time.allowed,  narrow_cast <ULONG> (dwMilliseconds)); }
        void wake  (DWORD dwMilliseconds) { InterlockedIncrement (&attempts);
                                            InterlockedIncrement (&rejections);
                                            InterlockedAdd       (&time.deprived, narrow_cast <ULONG> (dwMilliseconds)); }
      };

      SleepStats& getMessagePumpStats  (void) noexcept { return message_pump;  }
      SleepStats& getRenderThreadStats (void) noexcept { return render_thread; }
      SleepStats& getMicroStats        (void) noexcept { return micro_sleep;   }
      SleepStats& getMacroStats        (void) noexcept { return macro_sleep;   }

    protected:
      SleepStats message_pump, render_thread,
                 micro_sleep,  macro_sleep;
    };

    extern SK_LazyGlobal <EventCounter> events;

    static inline EventCounter* GetEvents  (void) noexcept { return events.getPtr (); }

    // The identifying SwapChain is an opaque handle, we have no reason to hold a reference to this
    //   and you can cast any value you want to this pointer (e.g. an OpenGL HGLRC). Prototype uses
    //     IUnknown because it's straightforward in DXGI / D3D9 to use these as handles :)
                  bool          HasLimiter (IUnknown *pSwapChain          = nullptr);
                  Limiter*      GetLimiter (IUnknown *pSwapChain          = nullptr,
                                            bool      bCreateIfNoneExists = true   );
    class Stats {
    public:
      static LARGE_INTEGER freq;

      Stats (void) noexcept {
        QueryPerformanceFrequency (&freq);
      }

      std::vector <double>&
        sortAndCacheFrametimeHistory (void);

      struct sample_t {
        double        val  =   0.0   ;
        LARGE_INTEGER when = { 0ULL };
      } data [MAX_SAMPLES];

      struct worker_context_s
      {
        ~worker_context_s (void)
        {
          if (hSignalProduce != nullptr)
            CloseHandle (hSignalProduce);
          if (hSignalConsume != nullptr)
            CloseHandle (hSignalConsume);
        }

        HANDLE        hSignalProduce = nullptr;
        HANDLE        hSignalConsume = nullptr;
         ULONG        ulLastFrame    = 0;
        volatile LONG work_idx       = 0;
        volatile LONG _init          = 0;

        std::pair <ULONG, std::vector <double> >
                      sorted_frame_history [2];
      } worker;

      int64_t samples = 0;

      bool addSample (double sample, LARGE_INTEGER time) noexcept
      {
        data [samples % MAX_SAMPLES].val  = sample;
        data [samples % MAX_SAMPLES].when = time;

        return
          ( ( ++samples % MAX_SAMPLES ) == 0 );
      }

      double calcDataTimespan (void)
      {
        uint64_t samples_present =
          samples < MAX_SAMPLES ?
                        samples : MAX_SAMPLES;

        LARGE_INTEGER min_time;
                      min_time.QuadPart = std::numeric_limits <LONGLONG>::max ();
        LARGE_INTEGER max_time = {                   0ULL };

        for ( auto sample_idx = 0               ;
                   sample_idx < samples_present ;
                 ++sample_idx )
        {
          LARGE_INTEGER sampled_time =
            data [sample_idx].when;

          if ( sampled_time.QuadPart >
                   max_time.QuadPart && sampled_time.QuadPart < std::numeric_limits <LONGLONG>::max () )
          {
                max_time.QuadPart =
            sampled_time.QuadPart;
          }

          if ( sampled_time.QuadPart <
                   min_time.QuadPart && sampled_time.QuadPart >= 0 )
          {
                min_time.QuadPart =
            sampled_time.QuadPart;
          }
        }

        return
          static_cast <double> (max_time.QuadPart - min_time.QuadPart) /
          static_cast <double> ( SK::Framerate::Stats::freq.QuadPart );
      }

         int calcNumSamples         (double seconds  = 1.0);
      double calcMean               (double seconds  = 1.0);
      double calcSqStdDev           (double mean,
                                     double seconds  = 1.0);
      double calcMin                (double seconds  = 1.0);
      double calcMax                (double seconds  = 1.0);
         int calcHitches            (double tolerance,
                                     double mean,
                                     double seconds  = 1.0);

      double calcOnePercentLow      (double seconds  = 1.0);
      double calcPointOnePercentLow (double seconds  = 1.0);

      double calcMean               (LARGE_INTEGER start) noexcept
      {
        double mean         = 0.0;
        int    samples_used =  0 ;

        for ( const auto datum : data )
        {
          if (datum.when.QuadPart >= start.QuadPart)
          {
            if (_isreal (datum.val) && datum.val > 0.0)
            {
              ++samples_used;
              mean += datum.val;
            }
          }
        }

        return
          ( mean / static_cast <double> (samples_used) );
      }

      double calcPercentile (float percent, LARGE_INTEGER start) //noexcept
      {
        UNREFERENCED_PARAMETER (start);

        std::vector <double>&
               sampled_lows = sortAndCacheFrametimeHistory ();
        size_t samples_used = sampled_lows.size            ();

        size_t end_sample_idx =
          std::max ( (size_t)0, std::min
              ( samples_used,
                  (size_t)std::round ((float)samples_used * (percent / 100.0f))
              )
          );

        const double
            avg_ms =
              std::accumulate ( sampled_lows.begin (),
                                                  sampled_lows.begin ()  + end_sample_idx,
                                   0.0 ) / (double)(end_sample_idx);

        return
          avg_ms;
      }

      double calcOnePercentLow (LARGE_INTEGER start) //noexcept
      {
        UNREFERENCED_PARAMETER (start);

        std::vector <double>&
               sampled_lows = sortAndCacheFrametimeHistory ();
        size_t samples_used = sampled_lows.size            ();

        size_t end_sample_idx =
          (size_t)std::round ((float)samples_used / 100.0f);

        const double
            one_percent_avg_ms =
              std::accumulate ( sampled_lows.begin (),
                                sampled_lows.begin ()  + end_sample_idx,
                                        0.0 ) / (double)(end_sample_idx);

        return
          one_percent_avg_ms;
      }

      double calcPointOnePercentLow (LARGE_INTEGER start) //noexcept
      {
        UNREFERENCED_PARAMETER (start);

        std::vector <double>&
               sampled_lows = sortAndCacheFrametimeHistory ();
        size_t samples_used = sampled_lows.size            ();

        size_t end_sample_idx =
          (size_t)std::round ((float)samples_used / 1000.0f);

        const double
            point_one_percent_avg_ms =
              std::accumulate ( sampled_lows.begin (),
                                sampled_lows.begin ()  + end_sample_idx,
                                        0.0 ) / (double)(end_sample_idx);

        return
          point_one_percent_avg_ms;
      }

      double calcSqStdDev (double mean, LARGE_INTEGER start) noexcept
      {
        double sd2          = 0.0;
        int    samples_used = 0;

        for ( const auto datum : data )
        {
          if (datum.when.QuadPart >= start.QuadPart)
          {
            if (_isreal (datum.val) && datum.val > 0.0)
            {
              sd2 += (datum.val - mean) *
                     (datum.val - mean);
              samples_used++;
            }
          }
        }

        return sd2 / static_cast <double> (samples_used);
      }

      double calcMin (LARGE_INTEGER start) noexcept
      {
        double min = INFINITY;

        for ( const auto datum : data )
        {
          if (datum.when.QuadPart >= start.QuadPart)
          {
            if (_isreal (datum.val) && datum.val > 0.0)
            {
              if (datum.val < min)
                min = datum.val;
            }
          }
        }

        return min;
      }

      double calcMax (LARGE_INTEGER start) noexcept
      {
        double max = -INFINITY;

        for ( const auto datum : data )
        {
          if (datum.when.QuadPart >= start.QuadPart)
          {
            if (_isreal (datum.val) && datum.val > 0.0)
            {
              if (datum.val > max)
                max = datum.val;
            }
          }
        }

        return max;
      }

      int calcHitches (double tolerance, double mean, LARGE_INTEGER start) noexcept
      {
        int  hitches   = 0;
        bool last_late = false;

        for ( const auto datum : data )
        {
          if (datum.when.QuadPart >= start.QuadPart)
          {
            if (datum.val > tolerance * mean)
            {
              if (! last_late)
                hitches++;
              last_late = true;
            }

            else
            {
              last_late = false;
            }
          }
        }

        return hitches;
      }

      int calcNumSamples (LARGE_INTEGER start) noexcept
      {
        int samples_used = 0;

        for ( const auto datum : data )
        {
          if (datum.when.QuadPart >= start.QuadPart)
          {
            //if (_isreal (datum.val) && datum.val > 0.0)
            //{
              samples_used++;
            //}
          }
        }

        return samples_used;
      }
    };
  };
};

using  Sleep_pfn = void (WINAPI *)(DWORD dwMilliseconds);
extern Sleep_pfn
       Sleep_Original;

using  SleepEx_pfn = DWORD (WINAPI *)(DWORD dwMilliseconds,
                                      BOOL  bAlertable);
extern SleepEx_pfn
       SleepEx_Original;

extern int __SK_FramerateLimitApplicationSite;



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

typedef NTSTATUS (WINAPI *NtDelayExecution_pfn)(
  IN  BOOLEAN        Alertable,
  IN  PLARGE_INTEGER DelayInterval
);


struct IDXGIOutput;

using WaitForVBlank_pfn = HRESULT (STDMETHODCALLTYPE *)(
  IDXGIOutput *This
);

typedef
NTSTATUS (NTAPI *NtWaitForSingleObject_pfn)(
  IN HANDLE         Handle,
  IN BOOLEAN        Alertable,
  IN PLARGE_INTEGER Timeout    // Microseconds
);


typedef enum _OBJECT_WAIT_TYPE {
  WaitAllObject,
  WaitAnyObject
} OBJECT_WAIT_TYPE,
*POBJECT_WAIT_TYPE;

typedef
NTSTATUS (NTAPI *NtWaitForMultipleObjects_pfn)(
  IN ULONG                ObjectCount,
  IN PHANDLE              ObjectsArray,
  IN OBJECT_WAIT_TYPE     WaitType,
  IN BOOLEAN              Alertable,
  IN PLARGE_INTEGER       TimeOut OPTIONAL );


extern void SK_Scheduler_Init     (void);
extern void SK_Scheduler_Shutdown (void);


#include <dwmapi.h>

extern HRESULT WINAPI
SK_DWM_GetCompositionTimingInfo (DWM_TIMING_INFO *pTimingInfo);

extern volatile LONG64 lD3DKMTPresentCalls;


#endif /* __SK__FRAMERATE_H__ */