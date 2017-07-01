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

#include <SpecialK/framerate.h>
#include <SpecialK/render_backend.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>

#include <d3d9.h>
#include <d3d11.h>
#include <atlbase.h>

SK::Framerate::EventCounter SK::Framerate::events;

LPVOID pfnQueryPerformanceCounter = nullptr;
LPVOID pfnSleep                   = nullptr;

Sleep_pfn                   Sleep_Original                   = nullptr;
QueryPerformanceCounter_pfn QueryPerformanceCounter_Original = nullptr;

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

#include <SpecialK/utility.h>

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
  bool bIsCallerGame = (SK_GetCallingDLL () == GetModuleHandle (nullptr));

  if (bIsCallerGame)
  {
    BOOL bGUIThread    = IsGUIThread (FALSE);
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

        //if (dwMilliseconds <= 1)
        YieldProcessor ();

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

        //if (dwMilliseconds <= 1)
        YieldProcessor ();

        return;
      }

      SK::Framerate::events.getMessagePumpStats ().sleep (dwMilliseconds);
    }
  }

  //if (config.framerate.yield_processor && dwMilliseconds == 0)
  if (dwMilliseconds == 0)
  {
    YieldProcessor ();
    return;
  }

#define DUAL_USE_MAX_DELTA
#ifdef DUAL_USE_MAX_DELTA
  // TODO: Stop this nonsense and make an actual parameter for this...
  //         (min sleep?)
  if (dwMilliseconds >= (DWORD)config.render.framerate.max_delta_time)
  {
    //if (dwMilliseconds == 0)
      //return YieldProcessor ();
#endif

    Sleep_Original (dwMilliseconds);
#ifdef DUAL_USE_MAX_DELTA
  }
#endif
}

extern volatile LONG SK_BypassResult;

BOOL
WINAPI
QueryPerformanceCounter_Detour (_Out_ LARGE_INTEGER *lpPerformanceCount)
{
  return QueryPerformanceCounter_Original (lpPerformanceCount);
}

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

  pCommandProc->AddVariable ( "WaitForVBLANK",
          new SK_IVarStub <bool> (&config.render.framerate.wait_for_vblank));
  pCommandProc->AddVariable ( "MaxDeltaTime",
          new SK_IVarStub <int> (&config.render.framerate.max_delta_time));

  pCommandProc->AddVariable ( "LimiterTolerance",
          new SK_IVarStub <float> (&config.render.framerate.limiter_tolerance));
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
  SK_DisableHook (pfnSleep);
  SK_DisableHook (pfnQueryPerformanceCounter);
}

SK::Framerate::Limiter::Limiter (double target)
{
  init (target);
}

IDirect3DDevice9Ex*
SK_D3D9_GetTimingDevice (void)
{
  if (! config.render.framerate.wait_for_vblank)
    return nullptr;

  static IDirect3DDevice9Ex* pTimingDevice = (IDirect3DDevice9Ex *)-1;

  if (pTimingDevice == (IDirect3DDevice9Ex *)-1)
  {
    CComPtr <IDirect3D9Ex> pD3D9Ex = nullptr;

    typedef HRESULT
      (STDMETHODCALLTYPE *Direct3DCreate9ExPROC)(UINT           SDKVersion,
                                                 IDirect3D9Ex** d3d9ex);

    extern Direct3DCreate9ExPROC Direct3DCreate9Ex_Import;

    HRESULT hr = (config.apis.d3d9ex.hook) ?
      Direct3DCreate9Ex_Import (D3D_SDK_VERSION, &pD3D9Ex)
                                    :
                               E_NOINTERFACE;

    HWND hwnd = 0;

    IDirect3DDevice9Ex* pDev9Ex = nullptr;

    if (SUCCEEDED (hr))
    {
      hwnd = 
      CreateWindowW (L"STATIC", L"Dummy D3D9 Window",
                       WS_POPUP | WS_MINIMIZEBOX,
                         CW_USEDEFAULT, CW_USEDEFAULT,
                           800, 600, 0,
                             nullptr, nullptr, 0x00 );

      D3DPRESENT_PARAMETERS pparams = { };
      
      pparams.SwapEffect       = D3DSWAPEFFECT_FLIPEX;
      pparams.BackBufferFormat = D3DFMT_UNKNOWN;
      pparams.hDeviceWindow    = hwnd;
      pparams.Windowed         = TRUE;
      
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

  SK_RenderAPI api = SK_GetCurrentRenderBackend ().api;

  if (      api ==      SK_RenderAPI::D3D10 ||
       (int)api &  (int)SK_RenderAPI::D3D11 ||
       (int)api &  (int)SK_RenderAPI::D3D12 )
  {
    if (SK_GetCurrentRenderBackend ().swapchain != nullptr)
    {
      if (SUCCEEDED (SK_GetCurrentRenderBackend ().swapchain->QueryInterface <IDXGISwapChain> (&dxgi_swap)))
      {
        if (SUCCEEDED (dxgi_swap->GetContainingOutput (&dxgi_output)))
        {
          dxgi_output->WaitForVBlank ();
        }
      }
    }
  }

  else if ((int)api & (int)SK_RenderAPI::D3D9)
  {
    if (SK_GetCurrentRenderBackend ().device != nullptr)
    {
      SK_GetCurrentRenderBackend ().device->QueryInterface ( IID_PPV_ARGS (&d3d9ex) );

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
  }

  QueryPerformanceCounter_Original (&start);

  next.QuadPart = 0ULL;
  time.QuadPart = 0ULL;
  last.QuadPart = 0ULL;

  last.QuadPart = (LONGLONG)(start.QuadPart - (ms / 1000.0) * freq.QuadPart);
  next.QuadPart = (LONGLONG)(start.QuadPart + (ms / 1000.0) * freq.QuadPart);
}

bool
SK::Framerate::Limiter::try_wait (void)
{
  if (target_fps == 0)
    return false;

  LARGE_INTEGER next_;

  next_.QuadPart =
    static_cast <LONGLONG> (
      ( start.QuadPart + (double)(frames+1) * (ms / 1000.0) * (double)freq.QuadPart )
    );

  QueryPerformanceCounter_Original (&time);

  if (time.QuadPart < next_.QuadPart)
    return true;

  return false;
}

void
SK::Framerate::Limiter::wait (void)
{
  static bool restart      = false;
  static bool full_restart = false;

  if (fps != target_fps)
    init (target_fps);

  if (target_fps == 0)
    return;

  frames++;

  QueryPerformanceCounter_Original (&time);

  // Actual frametime before we forced a delay
  effective_ms = 1000.0 * ((double)(time.QuadPart - last.QuadPart) / (double)freq.QuadPart);

  if ((double)(time.QuadPart - next.QuadPart) / (double)freq.QuadPart / (ms / 1000.0) > (config.render.framerate.limiter_tolerance * fps)) {
    //dll_log.Log ( L" * Frame ran long (%3.01fx expected) - restarting"
                  //L" limiter...",
            //(double)(time.QuadPart - next.QuadPart) / (double)freq.QuadPart / (ms / 1000.0) / fps );
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
    start.QuadPart = static_cast <LONGLONG> ((double)time.QuadPart + (ms / 1000.0) * (double)freq.QuadPart);
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
      ( ((double)start.QuadPart + (double)frames * (ms / 1000.0) * (double)freq.QuadPart) )
    );

  if (next.QuadPart > 0ULL)
  {
    // If available (Windows 7+), wait on the swapchain
    CComPtr <IDirect3DDevice9Ex> d3d9ex = nullptr;

    // D3D10/11/12
    CComPtr <IDXGISwapChain> dxgi_swap   = nullptr;
    CComPtr <IDXGIOutput>    dxgi_output = nullptr;

    if (config.render.framerate.wait_for_vblank)
    {
      SK_RenderAPI api = SK_GetCurrentRenderBackend ().api;

      if (      api ==      SK_RenderAPI::D3D10 ||
           (int)api &  (int)SK_RenderAPI::D3D11 ||
           (int)api &  (int)SK_RenderAPI::D3D12 )
      {
        if (SK_GetCurrentRenderBackend ().swapchain != nullptr)
        {
          if (SUCCEEDED (SK_GetCurrentRenderBackend ().swapchain->QueryInterface <IDXGISwapChain> (&dxgi_swap)))
          {
            dxgi_swap->GetContainingOutput (&dxgi_output);
          }
        }
      }

      else if ((int)api & (int)SK_RenderAPI::D3D9)
      {
        if (SK_GetCurrentRenderBackend ().device != nullptr)
        {
          if (FAILED (SK_GetCurrentRenderBackend ().device->QueryInterface ( IID_PPV_ARGS (&d3d9ex) )))
          {
            d3d9ex = SK_D3D9_GetTimingDevice ();
          }
        }
      }
    }

    const float target_ms = target_fps / 1000.0f;

    while (time.QuadPart < next.QuadPart)
    {
#if 0
      if ((double)(next.QuadPart - time.QuadPart) > (0.0166667 * (double)freq.QuadPart))
        Sleep_Original (10);
#else
      if (config.render.framerate.wait_for_vblank && (double)(next.QuadPart - time.QuadPart) > (0.001 * (1000.0 / target_fps) * (double)freq.QuadPart) * 0.555)
      {
        if (d3d9ex != nullptr)
          d3d9ex->WaitForVBlank (0);

        else if (dxgi_output != nullptr)
          dxgi_output->WaitForVBlank ();
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