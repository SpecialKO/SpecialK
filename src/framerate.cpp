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

#include <SpecialK/diagnostics/cpu.h>

#include <SpecialK/log.h>

#pragma comment(lib, "dwmapi.lib")

#include <concurrent_unordered_map.h>

int64_t                     SK_QpcFreq       = 0;
int64_t                     SK_QpcTicksPerMs = 0;
SK::Framerate::EventCounter SK::Framerate::events;


float __target_fps    = 0.0;
float __target_fps_bg = 0.0;

enum class SK_LimitApplicationSite {
  BeforeBufferSwap,
  DuringBufferSwap,
  AfterBufferSwap,
  DontCare,
  EndOfFrame // = 4 (Default)
};

//float fSwapWaitRatio = 0.77f;
//float fSwapWaitFract = 0.79f;

float fSwapWaitRatio = 0.15f;
float fSwapWaitFract = 0.85f;

float
SK::Framerate::Limiter::undershoot_percent = 7.5f;

class SK_FramerateLimiter_CfgProxy : public SK_IVariableListener {
  bool OnVarChange (SK_IVariable* var, void* val = nullptr)
  {
    if ((float *)var->getValuePointer () == &__target_fps)
    {
      config.render.framerate.target_fps =
        *static_cast <float *> (val);

      __target_fps = config.render.framerate.target_fps;
    }

    if ((float *)var->getValuePointer () == &__target_fps_bg)
    {
      config.render.framerate.target_fps_bg =
        *static_cast <float *> (val);

      __target_fps_bg = config.render.framerate.target_fps_bg;
    }

    return true;
  }
} __ProdigalFramerateSon;

extern NtQueryTimerResolution_pfn NtQueryTimerResolution;
extern NtSetTimerResolution_pfn   NtSetTimerResolution;
extern NtSetTimerResolution_pfn   NtSetTimerResolution_Original;

#define STATUS_SUCCESS          ((NTSTATUS)0x00000000L)
#define D3DKMT_MAX_WAITFORVERTICALBLANK_OBJECTS 8

typedef struct _D3DKMT_GETVERTICALBLANKEVENT {
  D3DKMT_HANDLE                  hAdapter;
  D3DKMT_HANDLE                  hDevice;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
  HANDLE                         *phEvent;
} D3DKMT_GETVERTICALBLANKEVENT;

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT2 {
  D3DKMT_HANDLE                  hAdapter;
  D3DKMT_HANDLE                  hDevice;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
  UINT                           NumObjects;
  HANDLE                         ObjectHandleArray [D3DKMT_MAX_WAITFORVERTICALBLANK_OBJECTS];
} D3DKMT_WAITFORVERTICALBLANKEVENT2;

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT {
  D3DKMT_HANDLE                  hAdapter;
  D3DKMT_HANDLE                  hDevice;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_WAITFORVERTICALBLANKEVENT;

typedef struct _D3DKMT_GETSCANLINE {
  D3DKMT_HANDLE                  hAdapter;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
  BOOLEAN                        InVerticalBlank;
  UINT                           ScanLine;
} D3DKMT_GETSCANLINE;

typedef struct _D3DKMT_SETSTABLEPOWERSTATE {
  D3DKMT_HANDLE hAdapter;
  BOOL          Enabled;
} D3DKMT_SETSTABLEPOWERSTATE;

using D3DKMTGetDWMVerticalBlankEvent_pfn   = NTSTATUS (WINAPI *)(const D3DKMT_GETVERTICALBLANKEVENT      *unnamedParam1);
using D3DKMTWaitForVerticalBlankEvent2_pfn = NTSTATUS (WINAPI *)(const D3DKMT_WAITFORVERTICALBLANKEVENT2 *unnamedParam1);
using D3DKMTWaitForVerticalBlankEvent_pfn  = NTSTATUS (WINAPI *)(const D3DKMT_WAITFORVERTICALBLANKEVENT  *unnamedParam1);
using D3DKMTGetScanLine_pfn                = NTSTATUS (WINAPI *)(D3DKMT_GETSCANLINE                      *unnamedParam1);
using D3DKMTSetStablePowerState_pfn        = NTSTATUS (WINAPI *)(const D3DKMT_SETSTABLEPOWERSTATE        *unnamedParam1);

void
SK::Framerate::Init (void)
{
  static std::once_flag the_wuncler;

  std::call_once (the_wuncler, [&](void)
  {
    SK_FPU_LogPrecision ();


    SK_ICommandProcessor* pCommandProc =
      SK_GetCommandProcessor ();


    pCommandProc->AddVariable ( "LimitSite",
            new SK_IVarStub <int> (&config.render.framerate.enforcement_policy));

    pCommandProc->AddVariable ( "WaitForVBLANK",
            new SK_IVarStub <bool> (&config.render.framerate.wait_for_vblank));


    pCommandProc->AddVariable ( "TargetFPS",
            new SK_IVarStub <float> (&__target_fps,    &__ProdigalFramerateSon));
    pCommandProc->AddVariable ( "BackgroundFPS",
            new SK_IVarStub <float> (&__target_fps_bg, &__ProdigalFramerateSon));

    pCommandProc->AddVariable ( "SwapWaitRatio",
            new SK_IVarStub <float> (&fSwapWaitRatio));

    pCommandProc->AddVariable ( "SwapWaitFract",
            new SK_IVarStub <float> (&fSwapWaitFract));

    pCommandProc->AddVariable ( "Undershoot",
            new SK_IVarStub <float> (&SK::Framerate::Limiter::undershoot_percent));

    SK_Scheduler_Init ();


    pCommandProc->AddVariable ( "MaxDeltaTime",
        new SK_IVarStub <int> (&config.render.framerate.max_delta_time));

    //if (! config.render.framerate.enable_mmcss)
    {
      if ( NtQueryTimerResolution != nullptr &&
           NtSetTimerResolution   != nullptr )
      {
        ULONG min, max, cur;
        NtQueryTimerResolution (&min, &max, &cur);

        SK_LOG0 ( ( L"Kernel resolution.: %f ms",
                      static_cast <float> (cur * 100)/1000000.0f ),
                    L"  Timing  " );

        if (NtSetTimerResolution_Original != nullptr)
            NtSetTimerResolution_Original (max, TRUE,  &cur);
        else
            NtSetTimerResolution          (max, TRUE,  &cur);

        SK_LOG0 ( ( L"New resolution....: %f ms",
                      static_cast <float> (cur * 100)/1000000.0f ),
                    L"  Timing  " );

        SK::Framerate::Limiter::timer_res_ms =
          (static_cast <double> (cur) * 100.0) / 1000000.0;
      }
    }
  });
}

void
SK::Framerate::Shutdown (void)
{
  SK_Scheduler_Shutdown ();
}


SK::Framerate::Limiter::Limiter (double target, bool tracks_game_window)
{
  effective_ms  = 0.0;
  tracks_window = tracks_game_window;

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

  if (rb.adapter.d3dkmt != 0 && rb.adapter.VidPnSourceId != 0)
  {
    SK_Thread_ScopedPriority
      thread_prio_boost (THREAD_PRIORITY_TIME_CRITICAL);

    static D3DKMTWaitForVerticalBlankEvent_pfn
           D3DKMTWaitForVerticalBlankEvent =
          (D3DKMTWaitForVerticalBlankEvent_pfn)SK_GetProcAddress (L"gdi32.dll",
          "D3DKMTWaitForVerticalBlankEvent");

    if (D3DKMTWaitForVerticalBlankEvent != nullptr)
    {
      D3DKMT_WAITFORVERTICALBLANKEVENT
             waitForVerticalBlankEvent               = { };
             waitForVerticalBlankEvent.hAdapter      = rb.adapter.d3dkmt;
             waitForVerticalBlankEvent.VidPnSourceId = rb.adapter.VidPnSourceId;

      if ( STATUS_SUCCESS ==
             D3DKMTWaitForVerticalBlankEvent (&waitForVerticalBlankEvent) )
      {
        return true;
      }
    }

    static D3DKMTGetScanLine_pfn
           D3DKMTGetScanLine =
          (D3DKMTGetScanLine_pfn)SK_GetProcAddress (L"gdi32.dll",
          "D3DKMTGetScanLine");

    D3DKMT_GETSCANLINE
      getScanLine               = { };
      getScanLine.hAdapter      = rb.adapter.d3dkmt;
      getScanLine.VidPnSourceId = rb.adapter.VidPnSourceId;

    if (D3DKMTGetScanLine != nullptr)
    {
      ////if ( STATUS_SUCCESS ==
      ////          D3DKMTGetScanLine (&getScanLine) && getScanLine.InVerticalBlank )
      ////{
      ////  return true;
      ////}

      UINT max_visible_scanline = 0u;
      UINT max_scanline         = 0u;

      bool stage_two = false;

      // Has been modified to wait for the END of VBLANK
      //
      while ( STATUS_SUCCESS ==
                D3DKMTGetScanLine (&getScanLine) )
      {
        if (! getScanLine.InVerticalBlank)
        {
          // We found the maximum scanline, now we are back at the top
          //   of the screen...
          if (max_scanline > 0 && getScanLine.ScanLine == 0)
          {
            stage_two = true;
          }

          YieldProcessor ();

          max_visible_scanline =
            std::max (max_visible_scanline, getScanLine.ScanLine);
        }

        else
        {
          if (stage_two && getScanLine.ScanLine == max_scanline - 2)
            return true;

          max_scanline =
            std::max (max_scanline, getScanLine.ScanLine);

          // Indiscriminately returning true would get us any time during VBLANK
          //
          //return true;
        }
      }
    }

    //static D3DKMTWaitForVerticalBlankEvent_pfn
    //       D3DKMTWaitForVerticalBlankEvent =
    //      (D3DKMTWaitForVerticalBlankEvent_pfn)SK_GetProcAddress (L"gdi32.dll",
    //      "D3DKMTWaitForVerticalBlankEvent");
    //
    //if (D3DKMTWaitForVerticalBlankEvent != nullptr)
    //{
    //  D3DKMT_WAITFORVERTICALBLANKEVENT
    //         waitForVerticalBlankEvent               = { };
    //         waitForVerticalBlankEvent.hAdapter      = rb.adapter.d3dkmt;
    //         waitForVerticalBlankEvent.VidPnSourceId = rb.adapter.VidPnSourceId;
    //
    //  if ( STATUS_SUCCESS ==
    //         D3DKMTWaitForVerticalBlankEvent (&waitForVerticalBlankEvent) )
    //  {
    //    return true;
    //  }
    //}
  }

  return true;

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
    // If available (Windows 8+), wait on the swapchain
    auto d3d9ex =
      rb.getDevice <IDirect3DDevice9Ex> ();

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

void
SK_Framerate_WaitForVBlank2 (void)
{
  static D3DKMTWaitForVerticalBlankEvent_pfn
         D3DKMTWaitForVerticalBlankEvent =
        (D3DKMTWaitForVerticalBlankEvent_pfn)SK_GetProcAddress (L"gdi32.dll",
        "D3DKMTWaitForVerticalBlankEvent");

  if (D3DKMTWaitForVerticalBlankEvent != nullptr)
  {
    static auto& rb =
      SK_GetCurrentRenderBackend ();

    D3DKMT_WAITFORVERTICALBLANKEVENT
           waitForVerticalBlankEvent               = { };
           waitForVerticalBlankEvent.hAdapter      = rb.adapter.d3dkmt;
           waitForVerticalBlankEvent.VidPnSourceId = rb.adapter.VidPnSourceId;

    if ( STATUS_SUCCESS ==
           D3DKMTWaitForVerticalBlankEvent (&waitForVerticalBlankEvent) )
    {
      return;
    }
  }

  SK_Framerate_WaitForVBlank ();
}

void
SK_D3DKMT_WaitForVBlank (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  // Flush batched commands before zonking this thread off
  if (rb.d3d11.immediate_ctx != nullptr)
      rb.d3d11.immediate_ctx->Flush ();

  SK_Framerate_WaitForVBlank ();

  return;

  SK_Framerate_WaitForVBlank2 ();
};

void
SK_D3DKMT_WaitForScanline0 (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  static D3DKMTGetScanLine_pfn
         D3DKMTGetScanLine =
        (D3DKMTGetScanLine_pfn)SK_GetProcAddress (L"gdi32.dll",
        "D3DKMTGetScanLine");

  D3DKMT_GETSCANLINE
         getScanLine               = { };
         getScanLine.hAdapter      = rb.adapter.d3dkmt;
         getScanLine.VidPnSourceId = rb.adapter.VidPnSourceId;

  while ( STATUS_SUCCESS ==
            D3DKMTGetScanLine (&getScanLine) )
  {
    if (getScanLine.ScanLine == 0)
      break;

    LONGLONG llNextLine = SK_QueryPerf ().QuadPart +
      static_cast <LONGLONG> (1.0 / ( static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Numerator) /
                                      static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Denominator) ) * SK_QpcFreq);

    while (SK_QueryPerf ().QuadPart < llNextLine)
      YieldProcessor ();
  }
}

LONG64 __SK_VBlankLatency_QPCycles;

struct qpc_interval_s {
  int64_t t0;
  int64_t tBegin;
  int64_t tEnd;

  int64_t getNextBegin (int64_t tNow)
  {
    return tEnd != 0 ?
      tNow + ((tNow - t0) / tEnd) * tBegin
                     : 0LL;
  }

  int64_t getNextEnd (int64_t tNow)
  {
    return tEnd != 0 ?
      tNow + ((tNow - t0) / tEnd) * tEnd
                     : 0LL;
  }

  bool isInside (int64_t tNow)
  {
    const auto qpcBegin = getNextBegin (tNow);
    const auto qpcEnd   = getNextEnd   (tNow);

    return
      ( tNow >= qpcBegin && tNow <= qpcEnd );
  }

  void waitForBegin (void)
  {
    const int64_t qpcNow =
      SK_QueryPerf ().QuadPart;

    const auto qpcNext =
      getNextBegin (qpcNow);

    while (SK_QueryPerf ().QuadPart < qpcNext)
      YieldProcessor ();
  }

  void waitForEnd (void)
  {
    const int64_t qpcNow =
      SK_QueryPerf ().QuadPart;

    const auto qpcNext =
      getNextEnd (qpcNow);

    while (SK_QueryPerf ().QuadPart < qpcNext)
      YieldProcessor ();
  }
} __VBlank;

void
SK::Framerate::Limiter::init (double target, bool _tracks_window)
{
  this->tracks_window =
       _tracks_window;

  SK_Thread_ScopedPriority
    thread_prio_boost (THREAD_PRIORITY_TIME_CRITICAL);

  ticks_per_frame = static_cast <ULONGLONG> (
          ( 1.0 / static_cast <double> (target) ) *
                  static_cast <double> (SK_QpcFreq)
                                            );
  LONGLONG __qpcStamp = 0LL;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto now =
    SK_QueryPerf ().QuadPart,
       next_vsync =
    rb.latency.counters.frameStats0.SyncQPCTime.QuadPart;

  if (tracks_window)
  {
    if (config.render.framerate.swapchain_wait > 0)
    {
      SK_AutoHandle hWaitHandle (SK_GetCurrentRenderBackend ().getSwapWaitHandle ());
      if ((intptr_t)hWaitHandle.m_h > 0)
      {
        SK_WaitForSingleObject (hWaitHandle, 25UL);
      }
    }

    if (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator > 0 &&
        next_vsync > now - static_cast <LONGLONG> (
                           static_cast <double> (SK_QpcFreq) /
                         ( static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator)     /
                           static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator) ) * 120.0 ) )
    {
    //SK_ImGui_Warning (SK_FormatStringW (L"VSync Freq: %f, HSync Freq: %f", ( static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator)   /
    //                                                                         static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator) ),
    //                                                                       ( static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Numerator)   /
    //                                                                         static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Denominator) )).c_str ());
      while (next_vsync < now)
      {
        next_vsync += static_cast <LONGLONG> (
            static_cast <double> (SK_QpcFreq) /
          ( static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator)     /
            static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator) ) );
      }
    }

    else
      next_vsync = now;

    next_vsync -= 2LL * static_cast <LONGLONG> (
        static_cast <double> (SK_QpcFreq) /
      ( static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Numerator)     /
        static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Denominator) ) );

////if (config.render.framerate.enforcement_policy != 2)
////  SK_D3DKMT_WaitForVBlank ();

  //static D3DKMTGetScanLine_pfn
  //       D3DKMTGetScanLine =
  //      (D3DKMTGetScanLine_pfn)SK_GetProcAddress (L"gdi32.dll",
  //      "D3DKMTGetScanLine");
  //
  //if (D3DKMTGetScanLine != nullptr)
  //{
  //  D3DKMT_GETSCANLINE
  //    getScanLine               = { };
  //    getScanLine.hAdapter      = rb.adapter.d3dkmt;
  //    getScanLine.VidPnSourceId = rb.adapter.VidPnSourceId;
  //
  //  __VBlank.tBegin = 0;
  //  __VBlank.tEnd   = 0;
  //
  //  while ( STATUS_SUCCESS ==
  //            D3DKMTGetScanLine (&getScanLine) )
  //  {
  //    if (getScanLine.ScanLine == 0)
  //    {
  //      __VBlank.t0 = SK_QueryPerf ().QuadPart;
  //
  //      break;
  //    }
  //  }
  //
  //  while ( STATUS_SUCCESS ==
  //            D3DKMTGetScanLine (&getScanLine) )
  //  {
  //    if (getScanLine.InVerticalBlank)
  //    {
  //      if (__VBlank.tBegin == 0)
  //      {
  //        __VBlank.tBegin = SK_QueryPerf ().QuadPart -__VBlank.t0;
  //      }
  //    }
  //
  //    else
  //    {
  //      if (__VBlank.tEnd == 0 && __VBlank.tBegin != 0)
  //      {
  //        __VBlank.tEnd = SK_QueryPerf ().QuadPart -__VBlank.t0;
  //      }
  //    }
  //
  //    if (__VBlank.tEnd != 0 && __VBlank.tBegin != 0)
  //      break;
  //  }
  //
  //  dll_log->Log (L"Blanking: %f ms, Visible Time: %f ms", static_cast <double> (__VBlank.tEnd - __VBlank.tBegin) / SK_QpcTicksPerMs,
  //                                                         static_cast <double> (                __VBlank.tBegin) / SK_QpcTicksPerMs);
  }
  //
  ////SK_D3DKMT_WaitForScanline0 ();
  //
  //__qpcStamp =
  //  SK_QueryPerf ().QuadPart;
  //

  __qpcStamp = next_vsync;

  ms  = 1000.0 / static_cast <double> (target);
  fps =          static_cast <double> (target);

  WriteRelease64 (
    &frames, 0ULL
  );

  auto _frames      = ReadAcquire64     (&frames);
  auto _framesDrawn = SK_GetFramesDrawn (       );

  frames_of_fame.frames_measured.first.initClock  (__qpcStamp);
  frames_of_fame.frames_measured.last.clock_val  = __qpcStamp;
  frames_of_fame.frames_measured.first.initFrame  (_framesDrawn);
  frames_of_fame.frames_measured.last.frame_idx += _frames;

#if 0
  DWM_TIMING_INFO dwmTiming        = {                      };
                  dwmTiming.cbSize = sizeof (DWM_TIMING_INFO);

  if ( tracks_window && SUCCEEDED ( SK_DWM_GetCompositionTimingInfo (&dwmTiming) ) )
  {
    LONGLONG llCompositionLatency =
        (LONGLONG)dwmTiming.qpcCompose -
        (LONGLONG)dwmTiming.qpcVBlank;

    //static double uS =
    //  static_cast <double> ( SK_GetPerfFreq ().QuadPart ) / ( 1000.0 * 1000.0 );

    //SK_LOG0 ( ( L"Compose: %llu, VBlank: %llu, RefreshPeriod: %f uS...  CompositionLatency: %lli ticks",
    //              dwmTiming.qpcCompose,  dwmTiming.qpcVBlank,
    //               static_cast <double> (dwmTiming.qpcRefreshPeriod) * uS,
    //                         llCompositionLatency ), L"  DWM    ");

    //__qpcStamp =
    //  dwmTiming.qpcVBlank;

    //if (config.render.framerate.enforcement_policy == 2)
    //  _perfQuadPart += (llCompositionLatency + ticks_to_undershoot);
    //else
    ///__qpcStamp -= (llCompositionLatency + ticks_to_undershoot);
    __qpcStamp -= ticks_to_undershoot;

    __SK_VBlankLatency_QPCycles =
      __qpcStamp - dwmTiming.qpcVBlank;
  }
#endif

  WriteRelease64 ( &start, __qpcStamp );
  WriteRelease64 ( &freq,  SK_QpcFreq );
  WriteRelease64 ( &time,         0LL );
  WriteRelease64 ( &last,  __qpcStamp - ticks_per_frame );
  WriteRelease64 ( &next,  __qpcStamp + ticks_per_frame );
}


bool
SK::Framerate::Limiter::try_wait (void)
{
  if (limit_behavior != LIMIT_APPLY) {
    return false;
  }

  if (tracks_window)
  {
    if (SK_IsGameWindowActive () || __target_fps_bg == 0.0f)
    {
      if (__target_fps <= 0.0f) {
        return false;
      }
    }
  }

  LARGE_INTEGER next_;
  next_.QuadPart =
    ReadAcquire64 (&frames) * ticks_per_frame +
    ReadAcquire64 (&start );

  return
    ( SK_QueryPerf ().QuadPart < next_.QuadPart );
}


//#define _RESTORE_TIMER_RES

void
SK::Framerate::Limiter::wait (void)
{
  if (limit_behavior != LIMIT_APPLY) {
    return;
  }

  if (tracks_window && background == SK_IsGameWindowActive ())
  {
    background = (! background);
  }

  // Don't limit under certain circumstances or exiting / alt+tabbing takes
  //   longer than it should.
  if (ReadAcquire (&__SK_DLL_Ending) != 0)
    return;


  ULONG origTimerRes = 0,
         maxTimerRes = 0,
         minTimerRes = 0;

  if (NtQueryTimerResolution != nullptr)
  {
    NtQueryTimerResolution ( &minTimerRes,
                             &maxTimerRes,
                             &origTimerRes );

    if (origTimerRes != maxTimerRes)
    {
      static int
        adjustments = 0;

      ULONG                                              now;
      NtSetTimerResolution_Original (maxTimerRes, TRUE, &now);

      SK::Framerate::Limiter::timer_res_ms =
          (static_cast <double> (now) * 100.0) / 1000000.0;

      SK_LOG1 ( ( L"Timer Resolution Adjustment #%lu", ++adjustments ),
                  L"FrameLimit" );

    }
  }


  SK_FPU_ControlWord fpu_cw_orig =
    SK_FPU_SetPrecision (_PC_64);


  if (! background)
  {
    if (fps != __target_fps) {
         init (__target_fps);
    }
  }

  else if (tracks_window)
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

  if (tracks_window && __target_fps <= 0.0f)
  {
    if (NtSetTimerResolution_Original != nullptr &&
                                       maxTimerRes    !=   origTimerRes)
        NtSetTimerResolution_Original (maxTimerRes, TRUE, &origTimerRes);

    SK_FPU_SetControlWord (_MCW_PC, &fpu_cw_orig);

    return;
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_Thread_SetCurrentPriority (THREAD_PRIORITY_TIME_CRITICAL);

  DWORD_PTR dwPtrAffinityMask =
    SetThreadAffinityMask (
      SK_GetCurrentThread (), DWORD_PTR_MAX
    );


  auto threadId    = GetCurrentThreadId ();
  auto framesDrawn = SK_GetFramesDrawn  ();

  // Two limits applied on the same frame would cause problems, don't allow it.
  if (_frame_shame.count (threadId) &&
      _frame_shame       [threadId] == framesDrawn) return;
  else
      _frame_shame       [threadId]  = framesDrawn;

  InterlockedIncrement64 (&frames);

  auto _time =
    SK_QueryPerf ().QuadPart;


  bool normal = true;

  if (restart || full_restart)
  {
    if (full_restart || config.render.framerate.present_interval == 0)
    {
      init (__target_fps, tracks_window);
      full_restart = false;
    }

    restart        = false;
    WriteRelease64 (&frames, 0);

    auto _start =  _time;//- ticks_per_frame;
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

  double _freq =
    static_cast <double> (freq_);

  // Actual frametime before we forced a delay
  effective_ms =
    1000.0 * ( static_cast <double> (time_ - last_)
                                      /  _freq    );

  WriteRelease64 (&next, next_);

  if (normal)
  {
    double missed_frames,
           missing_time =
      static_cast <double> ( time_ - next_ ) /
      static_cast <double> ( ticks_per_frame );

    double edge_distance =
      modf ( missing_time, &missed_frames );

     static constexpr double dMissingTimeBoundary = 0.999999999999;
     static constexpr double dEdgeToleranceLow    = 0.00000001;
     static constexpr double dEdgeToleranceHigh   = 0.99999999;

    if ( missed_frames >= dMissingTimeBoundary &&
         edge_distance >= dEdgeToleranceLow    &&
         edge_distance <= dEdgeToleranceHigh )
    {
      SK_LOG1 ( ( L"Frame Skipping (%f frames) :: Edge Distance=%f",
                    missed_frames, edge_distance ), __SK_SUBSYSTEM__ );

      InterlockedAdd64 ( &frames,
           (LONG64)missed_frames );

      next_  =
        ReadAcquire64 ( &frames ) * ticks_per_frame
                                  + start_;

      if (tracks_window)
        InterlockedAdd (&SK_RenderBackend::flip_skip, 1);
      else
        full_restart = true;
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
                                       _freq ),
            0.0  );
    };


  static LONGLONG llNextVBLankBegin = 0;
  static LONGLONG llNextVBLankEnd   = 0;

  bool       bScanLineReSync    = false;
  static int iScanlineSyncFrame = 0;
        bool bScanlineLevelSync = false;

  if (config.render.framerate.present_interval == 0 && config.render.framerate.enforcement_policy != 2)
  {
    if (SK_QueryPerf ().QuadPart > next_)
      bScanlineLevelSync = true;
  }


  if (next_ <= 0LL)
      next_ = time_ + ticks_per_frame;

  if (next_ > 0LL)
  {
    // Flush batched commands before zonking this thread off
    if (tracks_window && rb.d3d11.immediate_ctx != nullptr)
      rb.d3d11.immediate_ctx->Flush ();

  // Create an unnamed waitable timer.
    if (timer_wait == nullptr)
    {
      timer_wait =
        CreateWaitableTimer (nullptr, FALSE, nullptr);
    }

    constexpr
      double duS = (1000.0 * 10000.0);

    double
      to_next_in_secs = 0.0;


    if (config.render.framerate.present_interval == 0)
    {
      // Low-Latency Mode waits for VBLANK elsewhere
      if (config.render.framerate.enforcement_policy != 2)
      {
        //__VBlank.waitForBegin ();
        extern void SK_D3DKMT_WaitForVBlank (void);
                    SK_D3DKMT_WaitForVBlank ();

        ////SK_D3DKMT_WaitForScanline0 ();
      }
    }


    to_next_in_secs =
      SK_RecalcTimeToNextFrame ();

    // First use a kernel-waitable timer to scrub off most of the
    //   wait time without completely gobbling up a CPU core.
    if ( timer_wait != 0 && to_next_in_secs * 1000.0 >= timer_res_ms * 1.5/*&& config.render.framerate.present_interval != 0*//* && to_next_in_secs * 1000.0 >= timer_res_ms * 2.875*/ )
    {
      // Schedule the wait period just shy of the timer resolution determined
      //   by NtQueryTimerResolution (...). Excess wait time will be handled by
      //     spinning, because the OS scheduler is not accurate enough.
      LARGE_INTEGER liDelay;
                    liDelay.QuadPart =
                      std::min (
                        static_cast <LONGLONG> (
                          to_next_in_secs * 1000.0 - 0.5 * timer_res_ms
                                               ),
                        static_cast <LONGLONG> (
                          to_next_in_secs * 1000.0 * 0.9
                                               )
                      );

        liDelay.QuadPart =
      -(liDelay.QuadPart * 10000LL);

      // Check if the next frame is sooner than waitable timer resolution before
      //   rescheduling this thread.
      if ( SetWaitableTimer ( timer_wait, &liDelay,
                              0, nullptr, nullptr, TRUE ) )
      {
        SK_AutoHandle hSwapWait (0);

        // The ideal thing to wait on is the SwapChain, since it is what we are
        //   ultimately trying to throttle :)
        if (tracks_window && config.render.framerate.swapchain_wait > 0)
        {
          hSwapWait.Attach (rb.getSwapWaitHandle ());
        }

        HANDLE hWaitObjs [2];
        int    iWaitObjs = 0;

        DWORD  dwWait  = WAIT_FAILED;
        while (dwWait != WAIT_OBJECT_0)
        {
          if (         (intptr_t)hSwapWait.m_h > 0)
            hWaitObjs [iWaitObjs++] = hSwapWait.m_h;

          to_next_in_secs =
            std::max (0.0, SK_RecalcTimeToNextFrame ());

          if (to_next_in_secs * 1000.0 > timer_res_ms)
            hWaitObjs [iWaitObjs++] = timer_wait;

          if (iWaitObjs == 0)
            break;

          dwWait = iWaitObjs < 2  &&
                   hWaitObjs  [0] != hSwapWait.m_h ?
            SK_WaitForSingleObject_Micro ( timer_wait, &liDelay )
           : WaitForMultipleObjects      ( iWaitObjs,
                                           hWaitObjs,
                                             TRUE,
                                               static_cast <DWORD> (to_next_in_secs * 1000.0) );

          if ( dwWait != WAIT_OBJECT_0     &&
               dwWait != WAIT_OBJECT_0 + 1 &&
               dwWait != WAIT_TIMEOUT )
          {
            dll_log->Log (L"Result of WaitForSingleObject: %x", dwWait);
          }

          if ((intptr_t)hSwapWait.m_h > 0)
                        hSwapWait.Close ();

          if (dwWait == WAIT_TIMEOUT)
            break;

          break;
        }
      }
    }


    // Any remaining wait-time will degenerate into a hybrid busy-wait,
    //   this is also when VBlank synchronization is applied if user wants.
    if ( tracks_window && config.render.framerate.wait_for_vblank )
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

    static D3DKMTGetScanLine_pfn
           D3DKMTGetScanLine =
          (D3DKMTGetScanLine_pfn)SK_GetProcAddress (L"gdi32.dll",
          "D3DKMTGetScanLine");

    bool bEarlySync = false;

    static UINT uiLastScanLine = 0;
           UINT uiScanLine     = 0;


    static int iTry = 0;

    if ((++iTry % 90) == 0)
      bScanlineLevelSync = true;

    else if (time_ < next_)
      bScanlineLevelSync = true;

    do
    {
      DWORD dwWaitMS =
        static_cast <DWORD> (
          std::max (0.0, SK_RecalcTimeToNextFrame () * 1000.0)
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

#if 1
      if (time_ <= next_ && D3DKMTGetScanLine != nullptr && config.render.framerate.present_interval == 0 && (! bEarlySync))
      {
        if (bScanlineLevelSync)
        {
          //static D3DKMTWaitForVerticalBlankEvent_pfn
          //       D3DKMTWaitForVerticalBlankEvent =
          //      (D3DKMTWaitForVerticalBlankEvent_pfn)SK_GetProcAddress (L"gdi32.dll",
          //      "D3DKMTWaitForVerticalBlankEvent");
          //
          //if (D3DKMTWaitForVerticalBlankEvent != nullptr)
          //{
          //  D3DKMT_WAITFORVERTICALBLANKEVENT
          //         waitForVerticalBlankEvent               = { };
          //         waitForVerticalBlankEvent.hAdapter      = rb.adapter.d3dkmt;
          //         waitForVerticalBlankEvent.VidPnSourceId = rb.adapter.VidPnSourceId;
          //
          //  // ( STATUS_SUCCESS ==
          //            D3DKMTWaitForVerticalBlankEvent (&waitForVerticalBlankEvent);
          //
            //LONGLONG
            //  llNextLine =
            //    SK_QueryPerf ().QuadPart + 1.0 / ( static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Numerator) /
            //                                       static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Denominator) ) * SK_QpcFreq * (rb.displays [rb.active_display].signal.timing.total_size.cy - rb.displays [rb.active_display].signal.timing.active_size.cy);
            //
            //while (SK_QueryPerf ().QuadPart < llNextLine)
            //  YieldProcessor ();
            //
            //static int iTry = 0;
            //
            //if ((++iTry % 240) == 0)
            //  bScanLineReSync = true;
          //}

          if (true)
          //if (time_ <= llNextVBLankBegin || time_ >= llNextVBLankEnd)
          {
            D3DKMT_GETSCANLINE
              getScanLine               = { };
              getScanLine.hAdapter      = rb.adapter.d3dkmt;
              getScanLine.VidPnSourceId = rb.adapter.VidPnSourceId;

            while ( STATUS_SUCCESS ==
                      D3DKMTGetScanLine (&getScanLine) )
            {
              //
              // At present, we are not particularly picky where in VBLANK
              //   the raster line is when we release the limiter; but,
              //     the end-game is to sync versus the leading edge of
              //       blanking and slip into Quick Frame Transport
              //
              if (! getScanLine.InVerticalBlank)
              {
                if (getScanLine.ScanLine == 0)
                {
                  break;
                ////  llNextVBLankBegin =
                ////    SK_QueryPerf ().QuadPart + 1.0 / ( static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Numerator) /
                ////                                       static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Denominator) ) * SK_QpcFreq * (rb.displays [rb.active_display].signal.timing.active_size.cy);
                ////  llNextVBLankEnd   =
                ////    SK_QueryPerf ().QuadPart + 1.0 / ( static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Numerator) /
                ////                                       static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Denominator) ) * SK_QpcFreq * (rb.displays [rb.active_display].signal.timing.total_size.cy);
                }

                else
                {
                  LONGLONG llNextLine =
                    SK_QueryPerf ().QuadPart + static_cast <LONGLONG> (
                                       1.0 / ( static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Numerator) /
                                               static_cast <double> (rb.displays [rb.active_display].signal.timing.hsync_freq.Denominator) ) * SK_QpcFreq * 0.5 );

                  while (SK_QueryPerf ().QuadPart < llNextLine)
                    YieldProcessor ();
                }
              }

              else
              {
                if (getScanLine.InVerticalBlank)
                {
                  uiScanLine = getScanLine.ScanLine;
                }
              }

              time_ =
                SK_QueryPerf ().QuadPart;

              if (time_ >= next_)
              {
                if (getScanLine.ScanLine == 0 || getScanLine.InVerticalBlank)
                  break;
                else if (! getScanLine.InVerticalBlank)
                  bScanLineReSync = true;
              }
            }
          }

          //else
          //{
          //  llNextVBLankBegin +=
          //    1.0 / ( static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator) /
          //            static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator) ) * SK_QpcFreq;
          //
          //  llNextVBLankEnd   +=
          //    1.0 / ( static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator) /
          //            static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator) ) * SK_QpcFreq;
          //}
        }
      }
#endif
    } while (time_ < next_);

  //if (uiLastScanLine != uiScanLine)
  //{
  //  dll_log->Log (L"Drift: %i scanlines", (int)uiLastScanLine - (int)uiScanLine);
  //  uiLastScanLine = uiScanLine;
  //}
  }

  else
  {
    SK_LOG0 ( ( L"Framerate limiter lost time?! (non-monotonic clock)" ),
                L"FrameLimit" );
    InterlockedAdd64 (&start, -next_);
  }

  WriteRelease64 (&time, time_);
  WriteRelease64 (&last, time_);

  if (bScanLineReSync && config.render.framerate.enforcement_policy != 2)
  {
    WriteRelease64 (&frames, 0);

    ///LONG64 llVBlank =
    ///  __VBlank.getNextBegin (SK_QueryPerf ().QuadPart);
    ///       llVBlank -= ticks_per_frame;

    WriteRelease64 ( &start, time_ );
    WriteRelease64 ( &time,    0LL );
    WriteRelease64 ( &last,  time_ - ticks_per_frame );
    WriteRelease64 ( &next,  time_ + ticks_per_frame );
  }

  if (! lazy_init)
  {
    lazy_init = true;
         init (fps, tracks_window);
  }

#ifdef _RESTORE_TIMER_RES
  if (origTimerRes != maxTimerRes)
  {
    ULONG                                                   dontCare;
    if (NtSetTimerResolution_Original != nullptr)
        NtSetTimerResolution_Original (origTimerRes, TRUE, &dontCare);
  }
#endif

  SK_FPU_SetControlWord (_MCW_PC, &fpu_cw_orig);

  SetThreadAffinityMask (SK_GetCurrentThread (), dwPtrAffinityMask);
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
  SK_ComPtr   <IDXGISwapChain> pUnwrap;

  UINT size = sizeof (LPVOID);

  if (pSwapChain.p != nullptr)
      pSwapChain.p->GetPrivateData (IID_IUnwrappedDXGISwapChain, &size, (void *)&pUnwrap.p);

  if ( pUnwrap != nullptr &&
       pUnwrap != pSwapChain_ )
     pSwapChain_ = pUnwrap;

  static concurrency::concurrent_unordered_map < IUnknown *,
      std::unique_ptr <SK::Framerate::Limiter> > limiters_;

  SK_RunOnce (
    SK_GetCommandProcessor ()->AddCommand (
      "SK::Framerate::ResetLimit", new skLimitResetCmd ()
    )
  );

  if (! limiters_.count (pSwapChain_))
  {
    if (bCreate)
    {
      limiters_ [pSwapChain_] =
        std::make_unique <SK::Framerate::Limiter> (
          config.render.framerate.target_fps
        );

      SK_LOG0 ( ( L" Framerate Limiter Created to Track SwapChain (%ph)",
                                                       pSwapChain_
                ), L"FrameLimit"
              );
    }

    else
      return nullptr;
  }

  return
    limiters_.at (pSwapChain_).get ();
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

class SK_ImGui_FrameHistory : public SK_Stat_DataHistory <float, 120>
{
public:
  void timeFrame       (double seconds)
  {
    addValue ((float)(1000.0 * seconds));
  }
};

extern SK_LazyGlobal <SK_ImGui_FrameHistory> SK_ImGui_Frames;
extern bool                                  reset_frame_history;

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


  if (now.QuadPart == 0)
      now = SK_CurrentPerf ();

  if (dt + 0.0000001 <= 0.0000001)
      dt =
    static_cast <double> (now.QuadPart -
                  pLimiter->amortization._last_frame.QuadPart) /
    static_cast <double> (SK_QpcFreq);


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

    static ULONG64 last_frame         = 0;
    bool           skip_frame_history = false;

    if (last_frame < SK_GetFramesDrawn () - 1)
    {
      skip_frame_history = true;
    }

    if (last_frame != SK_GetFramesDrawn ())
    {   last_frame  = SK_GetFramesDrawn ();

      if (! (reset_frame_history || skip_frame_history))
      {
        SK_ImGui_Frames->timeFrame (dt);
      }

      else if (reset_frame_history) SK_ImGui_Frames->reset ();
    }
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
                    ( LARGE_INTEGER );

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
    calcMean (SK_DeltaPerf (seconds, SK_QpcFreq));
}

double
SK::Framerate::Stats::calcSqStdDev (double mean, double seconds)
{
  return
    calcSqStdDev (mean, SK_DeltaPerf (seconds, SK_QpcFreq));
}

double
SK::Framerate::Stats::calcMin (double seconds)
{
  return
    calcMin (SK_DeltaPerf (seconds, SK_QpcFreq));
}

double
SK::Framerate::Stats::calcMax (double seconds)
{
  return
    calcMax (SK_DeltaPerf (seconds, SK_QpcFreq));
}

double
SK::Framerate::Stats::calcOnePercentLow (double seconds)
{
  return
    calcOnePercentLow (SK_DeltaPerf (seconds, SK_QpcFreq));
}

double
SK::Framerate::Stats::calcPointOnePercentLow (double seconds)
{
  return
    calcPointOnePercentLow (SK_DeltaPerf (seconds, SK_QpcFreq));
}

int
SK::Framerate::Stats::calcHitches ( double tolerance,
                                    double mean,
                                    double seconds )
{
  return
    calcHitches (tolerance, mean, SK_DeltaPerf (seconds, SK_QpcFreq));
}

int
SK::Framerate::Stats::calcNumSamples (double seconds)
{
  return
    calcNumSamples (SK_DeltaPerf (seconds, SK_QpcFreq));
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
                SK_WaitForSingleObject ( pWorker->hSignalProduce,
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
           SK_WaitForSingleObject (worker.hSignalConsume, 0) )
    {
      LONG idx =
        ReadAcquire (&worker.work_idx);

      auto& kWriteBuffer =
        worker.sorted_frame_history [idx];
            kReadBuffer  =
        worker.sorted_frame_history [idx ? 0 : 1];

      kWriteBuffer.second.clear ();
      kWriteBuffer.first = SK_GetFramesDrawn ();

      for (const auto& datum : data)
      {
        if (datum.when.QuadPart >= 0)
        {
          if (_disreal (datum.val))
          {
            kWriteBuffer.second.emplace_back (datum.val);
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

double SK::Framerate::Limiter::timer_res_ms = 0.0;