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

#include <SpecialK/render/dxgi/dxgi_swapchain.h>



#include <SpecialK/log.h>

#pragma comment(lib, "dwmapi.lib")

LARGE_INTEGER
SK_QueryPerf ()
{
  return
    SK_CurrentPerf ();
}


LARGE_INTEGER                               SK::Framerate::Stats::freq = {};
SK_LazyGlobal <SK::Framerate::EventCounter> SK::Framerate::events;


float __target_fps    = 0.0;
float __target_fps_bg = 0.0;

enum class SK_LimitApplicationSite {
  BeforeBufferSwap,
  DuringBufferSwap,
  AfterBufferSwap,
  DontCare,
  EndOfFrame // = 4 (Default)
};

//float fSwapWaitRatio = 0.998877f;
//float fSwapWaitFract = 0.998877f;

//float fSwapWaitRatio = 0.77f;
//float fSwapWaitFract = 0.79f;

float fSwapWaitRatio = 0.745f;
float fSwapWaitFract = 0.745f;

float
SK::Framerate::Limiter::undershoot_percent = 10.0f;

void
SK::Framerate::Init (void)
{
  static std::once_flag the_wuncler;

  std::call_once (the_wuncler, [&](void)
  {
    SK_ICommandProcessor* pCommandProc =
      SK_GetCommandProcessor ();


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

    pCommandProc->AddVariable ( "SwapWaitRatio",
            new SK_IVarStub <float> (&fSwapWaitRatio));

    pCommandProc->AddVariable ( "SwapWaitFract",
            new SK_IVarStub <float> (&fSwapWaitFract));

    pCommandProc->AddVariable ( "Undershoot",
            new SK_IVarStub <float> (&SK::Framerate::Limiter::undershoot_percent));

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
  auto& rb =
    SK_GetCurrentRenderBackend ();


  SK_Thread_ScopedPriority
     thread_prio_boost (THREAD_PRIORITY_TIME_CRITICAL);

  // If available (Windows 8+), wait on the swapchain
  auto d3d9ex =
    rb.getDevice <IDirect3DDevice9Ex> ();

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
      DwmFlush ();

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

  else
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
      DwmFlush ();

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
/////bool
/////SK_Framerate_WaitForVBlank (void)
/////{
/////  void SK_D3DKMT_WaitForVBlank (void);
/////       SK_D3DKMT_WaitForVBlank ();
/////
/////  return true;
/////}
/////
/////
/////
extern bool __stdcall SK_IsGameWindowActive (void);
/////
/////
/////typedef UINT D3DDDI_VIDEO_PRESENT_SOURCE_ID;
/////typedef UINT D3DKMT_HANDLE;
/////
/////struct D3DKMT_WAITFORVERTICALBLANKEVENT
/////{
/////	D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
/////	D3DKMT_HANDLE                   hDevice;       // in: device handle [Optional]
/////	D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
/////};
/////
/////typedef struct D3DKMT_GETVERTICALBLANKEVENT
/////{
/////  D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
/////  D3DKMT_HANDLE                   hDevice;       // in: device handle [Optional]
/////  D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
/////  HANDLE*                         phEvent;
/////} D3DKMT_GETVERTICALBLANKEVENT;
/////
/////typedef struct D3DKMT_OPENADAPTERFROMHDC
/////{
/////	HDC                             hDc;            // in:  DC that maps to a single display
/////	D3DKMT_HANDLE                   hAdapter;       // out: adapter handle
/////	LUID                            AdapterLuid;    // out: adapter LUID
/////	D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // out: VidPN source ID for that particular display
/////} D3DKMT_OPENADAPTERFROMHDC;
/////
/////typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_OPENADAPTERFROMHDC)       (_Inout_              D3DKMT_OPENADAPTERFROMHDC*);
/////typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_WAITFORVERTICALBLANKEVENT)(   _In_ CONST struct D3DKMT_WAITFORVERTICALBLANKEVENT*);
/////typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_GETDWMVERTICALBLANKEVENT) (   _In_ CONST        D3DKMT_GETVERTICALBLANKEVENT*);
/////
/////SK_AutoHandle __SK_D3DKMT_DWM_VBlank (INVALID_HANDLE_VALUE);
/////
/////NTSTATUS
/////SK_D3DKMT_InitWaitForDWMVBlank (void)
/////{
/////  if (__SK_D3DKMT_DWM_VBlank.m_h != INVALID_HANDLE_VALUE)
/////    return S_OK;
/////
/////  D3DKMT_OPENADAPTERFROMHDC oa     = {          };
/////	                          oa.hDc = GetDC (NULL);
/////
/////  static PFND3DKMT_OPENADAPTERFROMHDC
/////             D3DKMTOpenAdapterFromHdc =
/////    reinterpret_cast <PFND3DKMT_OPENADAPTERFROMHDC> (
/////        SK_GetProcAddress (
/////          SK_LoadLibraryW ( L"gdi32.dll" ),
/////            "D3DKMTOpenAdapterFromHdc"
/////                          )
/////    );
/////
/////	NTSTATUS result =
/////    D3DKMTOpenAdapterFromHdc (&oa);
/////
/////  if (SUCCEEDED (result))
/////  {
/////    static PFND3DKMT_WAITFORVERTICALBLANKEVENT
/////              D3DKMTWaitForVerticalBlankEvent =
/////      reinterpret_cast <PFND3DKMT_WAITFORVERTICALBLANKEVENT> (
/////        SK_GetProcAddress (
/////          SK_LoadLibraryW ( L"gdi32.dll" ),
/////            "D3DKMTWaitForVerticalBlankEvent"
/////                          )
/////      );
/////
/////    static PFND3DKMT_GETDWMVERTICALBLANKEVENT
/////              D3DKMTGetDWMVerticalBlankEvent =
/////      reinterpret_cast <PFND3DKMT_GETDWMVERTICALBLANKEVENT> (
/////        SK_GetProcAddress (
/////          SK_LoadLibraryW ( L"gdi32.dll" ),
/////            "D3DKMTGetDWMVerticalBlankEvent"
/////                          )
/////      );
/////
/////
/////    D3DKMT_GETVERTICALBLANKEVENT
/////    get_event = { };
/////
/////    get_event.hAdapter      = oa.hAdapter;
/////    get_event.hDevice       =           0;
/////    get_event.VidPnSourceId = oa.VidPnSourceId;
/////    get_event.phEvent       = &__SK_D3DKMT_DWM_VBlank.m_h;
/////
/////    D3DKMT_WAITFORVERTICALBLANKEVENT
/////      _event               = {         };
/////      _event.hAdapter      = oa.hAdapter;
/////      _event.hDevice       =           0;
/////      _event.VidPnSourceId = oa.VidPnSourceId;
/////
/////    D3DKMTWaitForVerticalBlankEvent (&_event);
/////
/////    DWM_TIMING_INFO dwmTiming        = {                      };
/////                    dwmTiming.cbSize = sizeof (DWM_TIMING_INFO);
/////
/////    ////if ( SUCCEEDED ( SK_DWM_GetCompositionTimingInfo (&dwmTiming) ) )
/////    ////{
/////    ////  while ( SK_QueryPerf ().QuadPart < dwmTiming.qpcVBlank )
/////    ////    ;
/////    ////
/////    ////  return S_OK;
/////    ////}
/////
/////    result = D3DKMTGetDWMVerticalBlankEvent != nullptr   ?
/////             D3DKMTGetDWMVerticalBlankEvent (&get_event) : E_NOTIMPL;
/////
/////    //if (SUCCEEDED (result))
/////      //__SK_D3DKMT_DWM_VBlank.m_h = hEvent;
/////  }
/////
/////  ReleaseDC (NULL, oa.hDc);
/////
/////	return result;
/////}

void
SK_D3DKMT_WaitForVBlank (void)
{
  SK_Framerate_WaitForVBlank ();
  //SK_D3DKMT_InitWaitForDWMVBlank ();
  //
  //if (__SK_D3DKMT_DWM_VBlank.m_h != INVALID_HANDLE_VALUE)
  //  WaitForSingleObject (__SK_D3DKMT_DWM_VBlank, INFINITE);
};

LONG64 __SK_VBlankLatency_QPCycles;

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
  if ( ms >    0.0f &&
       ms <= 200.0f )
  {
    // ^^^ Anything lower than a 5 FPS limit is probably user error.

    SK_AutoHandle hWaitHandle (SK_GetCurrentRenderBackend ().getSwapWaitHandle ());
    if ((intptr_t)hWaitHandle.m_h > 0 )
    {
      SK_WaitForSingleObject (
                  hWaitHandle,
        std::max ( 48UL, 3 * static_cast <DWORD> (ms) )
      );
    }
  }


  auto _perfQuadPart = SK_QueryPerf   ( ).QuadPart;
  auto _frames       = ReadAcquire64     (&frames);
  auto _framesDrawn  = SK_GetFramesDrawn (       );

  frames_of_fame.frames_measured.first.initClock  (_perfQuadPart);
  frames_of_fame.frames_measured.last.clock_val  = _perfQuadPart;
  frames_of_fame.frames_measured.first.initFrame  (_framesDrawn);
  frames_of_fame.frames_measured.last.frame_idx += _frames;


  DWM_TIMING_INFO dwmTiming        = {                      };
                  dwmTiming.cbSize = sizeof (DWM_TIMING_INFO);

  if ( SUCCEEDED ( SK_DWM_GetCompositionTimingInfo (&dwmTiming) ) )
  {
    ticks_to_undershoot =
      static_cast <ULONGLONG> (
        static_cast < double> (ticks_per_frame) * 0.01 * undershoot_percent
                              );

    LONGLONG llCompositionLatency =
        (LONGLONG)dwmTiming.qpcCompose -
        (LONGLONG)dwmTiming.qpcVBlank;

    ///static double uS =
    ///  static_cast <double> ( SK_GetPerfFreq ().QuadPart ) / ( 1000.0 * 1000.0 );

    ///SK_LOG0 ( ( L"Compose: %llu, VBlank: %llu, RefreshPeriod: %f uS...  CompositionLatency: %lli ticks",
    ///              dwmTiming.qpcCompose,  dwmTiming.qpcVBlank,
    ///               static_cast <double> (dwmTiming.qpcRefreshPeriod) * uS,
    ///                         llCompositionLatency ), L"  DWM    ");

    _perfQuadPart               =
      dwmTiming.qpcVBlank - dwmTiming.qpcRefreshPeriod -
                                  llCompositionLatency -
                                   ticks_to_undershoot;

    __SK_VBlankLatency_QPCycles =
      _perfQuadPart - dwmTiming.qpcVBlank;
  }


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


  // Two limits applied on the same frame would cause problems, don't allow it.
  if (ReadAcquire64 (&last_frame) == static_cast <LONG64> (SK_GetFramesDrawn ())) return;
     WriteRelease64 (&last_frame,                          SK_GetFramesDrawn ());


  InterlockedIncrement64 (&frames);


  auto _time =
    SK_QueryPerf ().QuadPart;


  bool normal = true;

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

    normal = false;
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

  if (normal)
  {
    double missed_frames,
           missing_time =
      static_cast <double> ( time_ - next_ ) /
      static_cast <double> ( ticks_per_frame );

    double edge_distance =
      modf ( missing_time, &missed_frames );

     static constexpr double dMissingTimeBoundary = 1.0;
     static constexpr double dEdgeToleranceLow    = 0.15;
     static constexpr double dEdgeToleranceHigh   = 0.95;

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


  auto& rb =
    SK_GetCurrentRenderBackend ();


  if (next_ > 0LL)
  {
    if (rb.api == SK_RenderAPI::D3D11)
    {
      // Flush the queue before waiting, otherwise we could be asking the
      //   driver to evaluate commands after it should have presented a
      //     finished frame.
      SK_ComQIPtr <ID3D11DeviceContext> pDevCtx (
        rb.d3d11.immediate_ctx
      );

      if (pDevCtx != nullptr)
          pDevCtx->Flush ();
    }

    // For a Waitable chain to be effective, 100% busy-wait must not
    //   be allowed.
    if (config.render.framerate.swapchain_wait > 0)
    {
      config.render.framerate.busy_wait_ratio = std::max (
        config.render.framerate.busy_wait_ratio, 0.01f
      );
    }

    // Create an unnamed waitable timer.
    if (timer_wait == nullptr)
    {
      timer_wait =
        CreateWaitableTimer (nullptr, FALSE, nullptr);
    }

    static constexpr
      double duS = (1000.0 * 10000.0);

    double
      to_next_in_secs =
        SK_RecalcTimeToNextFrame ();

    // First use a kernel-waitable timer to scrub off most of the
    //   wait time without completely decimating a CPU core.
    if ( timer_wait != 0 && ( to_next_in_secs * config.render.framerate.busy_wait_ratio > 0.1 ||
                                                config.render.framerate.swapchain_wait  > 0 ) )
    {
      SK_AutoHandle hWaitHandle (rb.getSwapWaitHandle ());
      if ((intptr_t)hWaitHandle.m_h > 0)
      {
        if ( to_next_in_secs > ((ms / 1000.0) * fSwapWaitRatio) &&
             to_next_in_secs < 0.25f )
          // Sanity check to prevent an otherwise unbounded wait.
        {
          LARGE_INTEGER uSecs;
                        uSecs.QuadPart =
            -static_cast <LONGLONG> ( duS *
                                        to_next_in_secs *
                                        ( fSwapWaitFract ) );

          SK_WaitForSingleObject_Micro ( hWaitHandle,
                                           &uSecs );
        }

        to_next_in_secs =
          SK_RecalcTimeToNextFrame ();

      }

      // busy_wait_ratio controls the ratio of scheduler-wait to busy-wait,
      //   extensive testing shows 87.5% is most reasonable on the widest
      //     selection of hardware.
      LARGE_INTEGER liDelay;
                    liDelay.QuadPart =
                      static_cast <LONGLONG> (
                        to_next_in_secs * 1000.0 *
                        (double)config.render.framerate.busy_wait_ratio
                      );

        liDelay.QuadPart =
      -(liDelay.QuadPart * 10000LL);

      // Light-weight and high-precision -- but it's not a good idea to
      //   spend more than ~90% of our projected wait time in here or
      //     we will miss deadlines frequently.
      if ( SetWaitableTimer ( timer_wait, &liDelay,
                                0, nullptr, nullptr, TRUE ) )
      {
        DWORD  dwWait  = WAIT_FAILED;
        while (dwWait != WAIT_OBJECT_0)
        {
          if (to_next_in_secs <= 0.0)
          {
            break;
          }
          // Negative values are used to tell the Nt systemcall
          //   to use relative rather than absolute time offset.
          LARGE_INTEGER uSecs;
                        uSecs.QuadPart =
            -static_cast <LONGLONG> (duS * to_next_in_secs);

          // System Call:  NtWaitForSingleObject  [Delay = 100 ns]
          dwWait =
            SK_WaitForSingleObject_Micro ( timer_wait,
                                             &uSecs );

          if ( dwWait != WAIT_OBJECT_0 &&
               dwWait != WAIT_TIMEOUT )
          {
            dll_log->Log (L"Result of WaitForSingleObject: %x", dwWait);
          }

          to_next_in_secs =
            std::max (0.25, SK_RecalcTimeToNextFrame ());

          if (to_next_in_secs > 0.0001)
            YieldProcessor ();
        }
      }
    }


    // Any remaining wait-time will degenerate into a hybrid busy-wait,
    //   this is also when VBlank synchronization is applied if user wants.
    if ( config.render.framerate.wait_for_vblank )
    {
      DWM_TIMING_INFO dwmTiming        = {                      };
                      dwmTiming.cbSize = sizeof (DWM_TIMING_INFO);

      BOOL bNextVBlankIsShortTerm = TRUE;

      if ( SUCCEEDED ( SK_DWM_GetCompositionTimingInfo (&dwmTiming) ) )
      {
        if ( next_ < static_cast <LONG64> (dwmTiming.qpcVBlank) )
             bNextVBlankIsShortTerm = FALSE;
        if ( bNextVBlankIsShortTerm )
        {
          SK_Framerate_WaitForVBlank ();
        }
      }
    }


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

      time_ =
        SK_QueryPerf ().QuadPart;
    } while (time_ < next_);
  }

  else
  {
    dll_log->Log ( L"[FrameLimit] Framerate limiter lost time?! "
                   L"(non-monotonic clock)" );
    InterlockedAdd64 (&start, -next_);
  }

  WriteRelease64 (&time, time_);
  WriteRelease64 (&last, time_);

  if (! lazy_init)
  {
    lazy_init = true;
         init (fps);
  }
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
SK_FramerateLimit_Factory ( IUnknown *pSwapChain_,
                            bool      bCreate = true )
{
  // Prefer to reference SwapChains we wrap by their wrapped pointer
  SK_ComQIPtr <IDXGISwapChain> pSwapChain (pSwapChain_);
  SK_ComPtr   <IDXGISwapChain> pSwapChainUnwrapped;

  UINT _size =
        sizeof (LPVOID);

  IUnknown* pUnwrapped = nullptr;

  if ( pSwapChain_  != nullptr &&
       pSwapChain.p != nullptr &&
       SUCCEEDED (
         pSwapChain->GetPrivateData (
           IID_IUnwrappedDXGISwapChain, &_size,
              &pUnwrapped
                 )                  )
     )
  {
             pSwapChainUnwrapped.p =
    (IDXGISwapChain *)pUnwrapped;

             pSwapChain            =
             pSwapChainUnwrapped.p;
  }

  else
    pSwapChain = pSwapChain_;

  static concurrency::concurrent_unordered_map < IUnknown *,
      std::unique_ptr <SK::Framerate::Limiter> > limiters_;

  SK_RunOnce (
    SK_GetCommandProcessor ()->AddCommand (
      "SK::Framerate::ResetLimit", new skLimitResetCmd ()
    )
  );

  if (! limiters_.count (pSwapChain.p))
  {
    if (bCreate)
    {
      limiters_ [pSwapChain.p] =
        std::make_unique <SK::Framerate::Limiter> (
          config.render.framerate.target_fps
        );

      SK_LOG0 ( ( L" Framerate Limiter Created to Track SwapChain (%ph)",
                                                       pSwapChain.p
                ), L"FrameLimit"
              );
    }

    else
      return nullptr;
  }

  return
    limiters_.at (pSwapChain).get ();
}

bool
SK::Framerate::HasLimiter (IUnknown *pSwapChain)
{
  return
    nullptr != SK_FramerateLimit_Factory
               ( pSwapChain,     false );
}

SK::Framerate::Limiter*
SK::Framerate::GetLimiter ( IUnknown *pSwapChain,
                            bool      bCreateIfNoneExists )
{
  return
    SK_FramerateLimit_Factory ( pSwapChain,
                                bCreateIfNoneExists );
}

void
SK::Framerate::Tick ( bool          wait,
                      double        dt,
                      LARGE_INTEGER now,
                      IUnknown*     swapchain )
{
  auto* pLimiter =
    SK::Framerate::GetLimiter (swapchain);

  if (wait)
    pLimiter->wait ();

  if (! ( pLimiter->frame_history.isAllocated  () &&
          pLimiter->frame_history2.isAllocated () ) )
  {
    // Late initialization
    Init ();
  }


  now = SK_CurrentPerf ();
  dt  =
    static_cast <double> (now.QuadPart -
                  pLimiter->amortization._last_frame.QuadPart) /
    static_cast <double> (SK::Framerate::Stats::freq.QuadPart);


  // Prevent inserting infinity into the dataset
  if ( std::isnormal (dt) )
  {
    if (pLimiter->frame_history->addSample (1000.0 * dt, now))
    {
      pLimiter->amortization.phase = 0;
    }
    pLimiter->frame_history2->addSample (
      pLimiter->effective_frametime (),
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

  if (pLimiter->amortization.phase < _NUM_STATS)
  {
    static constexpr LARGE_INTEGER
      all_samples = { 0UL, 0UL };

    SK::Framerate::Stats*
      pContainers [] =
      {
        pLimiter->frame_history_snapshots.mean.getPtr        (),
        pLimiter->frame_history_snapshots.min.getPtr         (),
        pLimiter->frame_history_snapshots.max.getPtr         (),
        pLimiter->frame_history_snapshots.percentile0.getPtr (),
        pLimiter->frame_history_snapshots.percentile1.getPtr ()
      };

    auto stat_idx =
      pLimiter->amortization.phase++;

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
          pLimiter->frame_history->calcPercentile (
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
              pLimiter->frame_history.getPtr (),
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

  pLimiter->amortization._last_frame = now;
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