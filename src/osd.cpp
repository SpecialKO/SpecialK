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

#include "osd.h"

#include "RTSSSharedMemory.h"

#pragma comment (lib, "Shlwapi.lib")
#include <Shlwapi.h>

#include <atlbase.h>

#include <float.h>
#include <io.h>
#include <tchar.h>

#include "config.h"
#include "io_monitor.h"
#include "gpu_monitor.h"
#include "memory_monitor.h"

#include "core.h"
#include "framerate.h"

#include "log.h"

#include "dxgi_backend.h"
#include "d3d9_backend.h"
#include "opengl_backend.h"

#define OSD_PRINTF   if (config.osd.show)     { pszOSD += sprintf (pszOSD,
#define OSD_R_PRINTF if (config.osd.show &&\
                         config.render.show)  { pszOSD += sprintf (pszOSD,
#define OSD_M_PRINTF if (config.osd.show &&\
                         config.mem.show)     { pszOSD += sprintf (pszOSD,
#define OSD_B_PRINTF if (config.osd.show &&\
                         config.load_balance\
                         .use)                { pszOSD += sprintf (pszOSD,
#define OSD_S_PRINTF if (config.osd.show &&\
                         config.mem.show &&\
                         config.sli.show)     { pszOSD += sprintf (pszOSD,
#define OSD_C_PRINTF if (config.osd.show &&\
                         config.cpu.show)     { pszOSD += sprintf (pszOSD,
#define OSD_G_PRINTF if (config.osd.show &&\
                         config.gpu.show)     { pszOSD += sprintf (pszOSD,
#define OSD_D_PRINTF if (config.osd.show &&\
                         config.disk.show)    { pszOSD += sprintf (pszOSD,
#define OSD_P_PRINTF if (config.osd.show &&\
                         config.pagefile.show)\
                                              { pszOSD += sprintf (pszOSD,
#define OSD_I_PRINTF if (config.osd.show &&\
                         config.io.show)      { pszOSD += sprintf (pszOSD,
#define OSD_END    ); }

extern char* szOSD;

#include "nvapi.h"
extern NV_GET_CURRENT_SLI_STATE sli_state;
extern BOOL nvapi_init;

// Probably need to use a critical section to make this foolproof, we will
//   cross that bridge later though. The OSD is performance critical
bool osd_shutting_down = false;

// Initialize some things (like color, position and scale) on first use
bool osd_init          = false;

std::wstring rtss_hook_dll = L"";

BOOL
SK_ReleaseSharedMemory (LPVOID lpMemory)
{
  if (lpMemory != nullptr) {
    return UnmapViewOfFile (lpMemory);
  }

  return FALSE;
}

LPVOID
SK_GetSharedMemory (DWORD dwProcID)
{
  if (osd_shutting_down && osd_init == false)
    return nullptr;

  HANDLE hMapFile =
    OpenFileMappingA (FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

  if (hMapFile) {
    LPVOID               pMapAddr =
      MapViewOfFile (hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    // We got our pointer, now close the file... we'll clean this pointer up later
    CloseHandle (hMapFile);

    LPRTSS_SHARED_MEMORY pMem     =
      (LPRTSS_SHARED_MEMORY)pMapAddr;

    if (pMem)
    {
      if ((pMem->dwSignature == 'RTSS') && 
          (pMem->dwVersion >= 0x00020000))
      {
        // ProcID is a wild-card, just return memory without checking to see if RTSS
        //   knows about a particular process.
        if (dwProcID == 0)
          return pMapAddr;

        for (DWORD dwApp = 0; dwApp < pMem->dwAppArrSize; dwApp++)
        {
          RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY pApp =
            (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY)
              ((LPBYTE)pMem + pMem->dwAppArrOffset +
                      dwApp * pMem->dwAppEntrySize);

          if (pApp->dwProcessID == dwProcID)
          {
            // Everything is good and RTSS knows about dwProcID!
            return pMapAddr;
          }
        }
      }

      // We got a pointer, but... it was not to useable RivaTuner memory
      UnmapViewOfFile (pMapAddr);
    }
  }

  return nullptr;
}

LPVOID
SK_GetSharedMemory (void)
{
  return SK_GetSharedMemory (GetCurrentProcessId ());
}

#include "log.h"

#include <d3d9.h>

bool
SK_IsD3D9 (DWORD dwFlags = 0x00)
{
  static bool d3d9 = false;

  if ((dwFlags != 0x00) && ((dwFlags & APPFLAG_D3D9) || 
                            (dwFlags & APPFLAG_D3D9EX)))
    d3d9 = true;

  return d3d9;
}

bool
SK_IsD3D11 (DWORD dwFlags = 0x00)
{
  static bool d3d11 = false;

  if ((dwFlags != 0x00) && (dwFlags & APPFLAG_D3D11))
    d3d11 = true;

  return d3d11;
}

bool
SK_IsD3D12 (DWORD dwFlags = 0x00)
{
  static bool d3d12 = false;

  if ((dwFlags != 0x00) && (dwFlags & APPFLAG_D3D12))
    d3d12 = true;

  return d3d12;
}

bool
SK_IsOpenGL (DWORD dwFlags = 0x00)
{
  static bool ogl = false;

  if ((dwFlags != 0x00) && (dwFlags & APPFLAG_OGL))
    ogl = true;

  return ogl;
}

std::wstring
SK_GetAPINameFromOSDFlags (DWORD dwFlags)
{
  // All three are DXGI-based and probable
  if (dwFlags & APPFLAG_D3D12)
    return L"D3D12";
  if (dwFlags & APPFLAG_D3D11)
    return L"D3D11";
  if (dwFlags & APPFLAG_D3D10)
    return L"D3D10";

  if (dwFlags & APPFLAG_OGL)
    return L"OpenGL";

  if (dwFlags & APPFLAG_D3D9EX)
    return L"D3D9Ex";

  if (dwFlags & APPFLAG_D3D9) {
      extern IDirect3DDevice9*   g_pD3D9Dev;
    CComPtr <IDirect3DDevice9Ex> pExDev;

    // TODO: Add a query for this with its code written in d3d9.cpp, instead of, well querying it ;)
    if ( g_pD3D9Dev != nullptr &&
         SUCCEEDED (g_pD3D9Dev->QueryInterface (IID_PPV_ARGS (&pExDev))) ) {
      return L"D3D9Ex";
    }

    return L"D3D9";
  }

#ifdef OLDER_D3D_SUPPORT
  if (dwFlags & APPFLAG_D3D8)
    return L"D3D8";
  if (dwFlags & APPFLAG_DD)
    return L"DDRAW";
#endif

  return L"UNKNOWN";
}

enum SK_UNITS {
  Celsius    = 0,
  Fahrenheit = 1,
  B          = 2,
  KiB        = 3,
  MiB        = 4,
  GiB        = 5,
  Auto       = 32
};

std::wstring
SK_SizeToString (uint64_t size, SK_UNITS unit = Auto)
{
  wchar_t str [64];

  if (unit == Auto) {
    if      (size > (1ULL << 32ULL)) unit = GiB;
    else if (size > (1ULL << 22ULL)) unit = MiB;
    else if (size > (1ULL << 12ULL)) unit = KiB;
    else                             unit = B;
  }

  switch (unit)
  {
    case GiB:
      _swprintf (str, L"%#5llu GiB", size >> 30);
      break;
    case MiB:
      _swprintf (str, L"%#5llu MiB", size >> 20);
      break;
    case KiB:
      _swprintf (str, L"%#5llu KiB", size >> 10);
      break;
    case B:
    default:
      _swprintf (str, L"%#3llu Bytes", size);
      break;
  }

  return str;
}

std::wstring
SK_SizeToStringF (uint64_t size, int width, int precision, SK_UNITS unit = Auto)
{
  wchar_t str [64];

  if (unit == Auto) {
    if      (size > (1ULL << 32ULL)) unit = GiB;
    else if (size > (1ULL << 22ULL)) unit = MiB;
    else if (size > (1ULL << 12ULL)) unit = KiB;
    else                             unit = B;
  }

  switch (unit)
  {
  case GiB:
    _swprintf (str, L"%#*.*f GiB", width, precision,
              (float)size / (1024.0f * 1024.0f * 1024.0f));
    break;
  case MiB:
    _swprintf (str, L"%#*.*f MiB", width, precision,
              (float)size / (1024.0f * 1024.0f));
    break;
  case KiB:
    _swprintf (str, L"%#*.*f KiB", width, precision, (float)size / 1024.0f);
    break;
  case B:
  default:
    _swprintf (str, L"%#*llu Bytes", width-1-precision, size);
    break;
  }

  return str;
}

std::wstring
SK_FormatTemperature (int32_t in_temp, SK_UNITS in_unit, SK_UNITS out_unit)
{
  int32_t converted;
  wchar_t wszOut [16];

  if (in_unit == Celsius && out_unit == Fahrenheit) {
    //converted = in_temp * 2 + 30;
    converted = (int32_t)((float)(in_temp) * 9.0f/5.0f + 32.0f);
    _swprintf (wszOut, L"%#3li°F", converted);
  } else if (in_unit == Fahrenheit && out_unit == Celsius) {
    converted = (int32_t)(((float)in_temp - 32.0f) * (5.0f/9.0f));
    _swprintf (wszOut, L"%#2li°C", converted);
  } else {
    _swprintf (wszOut, L"%#2li°C", in_temp);
  }

  return wszOut;
}

#define CINTERFACE
#include <d3d9.h>
extern IDirect3DSwapChain9* g_pSwapChain9;

std::string external_osd_name;

BOOL
__stdcall
SK_DrawExternalOSD (std::string app_name, std::string text)
{
  external_osd_name = app_name;

  SK_UpdateOSD (text.c_str (), nullptr, app_name.c_str ());

  return TRUE;
}


//Stupid Hack, rewrite me... (IN PROGRESS - see isPlugin below)
static bool isArkhamKnight    = false;
static bool isTalesOfZestiria = false;
static bool isFallout4        = false;
static bool isDarkSouls3      = false;
static bool isDivinityOrigSin = false;

static bool isPlugin          = false;


// TODO: Move this into CORE or somehwere else, the "plugin" system
//        needs a proper formal design.
std::wstring plugin_name = L"";

void
__stdcall
SK_SetPluginName (std::wstring name)
{
  plugin_name = name;
  isPlugin    = true;
}

void
SK_InstallOSD (void)
{
#if 0
#ifndef _WIN64
  HMODULE hModRTSS = GetModuleHandle (L"RTSSHooks.dll");
#else
  HMODULE hModRTSS = GetModuleHandle (L"RTSSHooks64.dll");
#endif

  if (hModRTSS != nullptr) {
#if 0
    typedef int (__cdecl *UninstallRTSSHook_t)(void);

    UninstallRTSSHook_t UninstallRTSSHook = 
      (UninstallRTSSHook_t)GetProcAddress (hModRTSS, "UninstallRTSSHook");

    if (UninstallRTSSHook != nullptr) {
      UninstallRTSSHook ();
    }
#endif

    wchar_t wszRTSSHook [512] = { L'\0' };
    GetModuleFileNameW (hModRTSS, wszRTSSHook, 511);

    FreeLibrary (hModRTSS);
    hModRTSS =
      LoadLibrary      (wszRTSSHook);

#if 0
    typedef int (__cdecl *InstallRTSSHook_t)(void);

    InstallRTSSHook_t InstallRTSSHook = 
      (InstallRTSSHook_t)GetProcAddress (hModRTSS, "InstallRTSSHook");

    if (InstallRTSSHook != nullptr) {
      dll_log.LogEx (true, L" Installing RTSS Hook: ");
        InstallRTSSHook ();
      dll_log.LogEx (false, L"done!\n");
      return;
    }

    dll_log.LogEx (false, L"No Such Function\n");
#endif
  }
#endif
  SK_SetOSDScale (config.osd.scale);
  SK_SetOSDPos   (config.osd.pos_x, config.osd.pos_y);
  SK_SetOSDColor (config.osd.red, config.osd.green, config.osd.blue);
}


SK::Framerate::Stats frame_history;
SK::Framerate::Stats frame_history2;

#include "command.h"

BOOL
SK_DrawOSD (void)
{
  LARGE_INTEGER now;
  double        dt;

  SK::Framerate::Tick (dt, now);

  static bool cleared = false;

#if 0
  // Automatically free VRAM cache when it is a bit on the excessive side
  if ((process_stats.memory.page_file_bytes >> 30ULL) > 28) {
    SK_GetCommandProcessor ()->ProcessCommandLine ("VRAM.Purge 9999");
  }

  if ((process_stats.memory.page_file_bytes >> 30ULL) < 12) {
    SK_GetCommandProcessor ()->ProcessCommandLine ("VRAM.Purge 0");
  }
#endif

  if ((! config.osd.show) && cleared)
    return TRUE;

  //
  // Enough attempts to cover 15 Seconds at 240 Hz
  //
  const unsigned int MAX_RTSS_ATTEMPTS = 15 * 240;

  static unsigned int connect_attempts = 1;

  // Bail-out early when shutting down, or RTSS does not know about our process
  LPVOID pMemory = SK_GetSharedMemory ();

  if (! pMemory) {
    ++connect_attempts;

    // Test for _equality_, because we only want to print this once!
    if (connect_attempts == MAX_RTSS_ATTEMPTS) {
      dll_log.Log (L"[   RTSS   ] Exhausted Connection Attempts... disabling OSD!");
      config.osd.show  = false;
      cleared          = true;
      connect_attempts = 1; // Restart the counter so that the OSD can be turned on
                            //   if RTSS is started after the game.
    }

    return FALSE;
  }

  if (! osd_init) {
    osd_init = true;

#ifndef _WIN64
    HMODULE hModRTSS = GetModuleHandle (L"RTSSHooks.dll");
#else
    HMODULE hModRTSS = GetModuleHandle (L"RTSSHooks64.dll");
#endif

    wchar_t wszRTSSModFile [MAX_PATH];
    GetModuleFileNameW (hModRTSS, wszRTSSModFile, MAX_PATH);

    rtss_hook_dll = wszRTSSModFile;

    dll_log.LogEx ( true,
      L"[   RTSS   ] Opening Connection to RivaTuner Statistics Server... " );

    dll_log.LogEx ( false,
      L"successful after %u attempt(s)!\n", connect_attempts );

    SK_InstallOSD ();
  }

  char* pszOSD = szOSD;
  *pszOSD = '\0';

  static io_perf_t
    io_counter;

  buffer_t buffer = mem_info [0].buffer;
  int      nodes  = mem_info [buffer].nodes;

  extern std::wstring SK_VER_STR;

  if (config.time.show)
  {
    SYSTEMTIME st;
    GetLocalTime (&st);

    wchar_t time [64];
    GetTimeFormat (config.time.format,0L,&st,NULL,time,64);

    static HMODULE hModGame = GetModuleHandle (nullptr);
    static wchar_t wszGameName [MAX_PATH] = { L'\0' };

    if (wszGameName [0] == L'\0') {
      GetModuleFileName (hModGame, wszGameName, MAX_PATH);
      if (StrStrIW (wszGameName, L"BatmanAK.exe"))
        isArkhamKnight = true;
      else if (StrStrIW (wszGameName, L"Tales of Zestiria.exe"))
        isTalesOfZestiria = true;
      else if (StrStrIW (wszGameName, L"Fallout4.exe"))
        isFallout4 = true;
      else if (StrStrIW (wszGameName, L"DarkSoulsIII.exe"))
        isDarkSouls3 = true;
      else if (StrStrIW (wszGameName, L"EoCApp.exe"))
        isDivinityOrigSin = true;
    }

    else if (isFallout4) {
      OSD_PRINTF "Fallout 4 \"Works\" v 0.3.4   %ws\n\n",
                 time
      OSD_END
    } else if (isDivinityOrigSin) {
      OSD_PRINTF "Divinity: Original Sin \"Works\" v 0.0.1   %ws\n\n",
                 time
      OSD_END
    } else if (isArkhamKnight) {
      OSD_PRINTF "Batman \"Fix\" v 0.20   %ws\n\n",
                 time
      OSD_END
    } else if (isDarkSouls3) {
      OSD_PRINTF "%ws   %ws\n\n",
                 plugin_name.c_str (), time
      OSD_END
    } else if (isPlugin) {
      OSD_PRINTF "%ws   %ws\n\n",
                 plugin_name.c_str (), time
      OSD_END
    } else {
      OSD_PRINTF "Special K v %ws   %ws\n\n",
                 SK_VER_STR.c_str (), time
      OSD_END
    }
  }

  if (config.fps.show)
  {
    LPRTSS_SHARED_MEMORY pMem =
      (LPRTSS_SHARED_MEMORY)pMemory;

    if (pMem)
    {
      if ((pMem->dwSignature == 'RTSS') && 
          (pMem->dwVersion >= 0x00020000))
      {
        for (DWORD dwApp = 0; dwApp < pMem->dwAppArrSize; dwApp++)
        {
          RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY pApp =
            (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY)
              ((LPBYTE)pMem + pMem->dwAppArrOffset +
                      dwApp * pMem->dwAppEntrySize);

          //
          // Print the API Statistics and Framerate
          //
          if (pApp->dwProcessID == GetCurrentProcessId ())
          {
            // What the bloody hell?! How do we ever get a dt value near 0?
            if (dt > 0.001)
              frame_history.addSample (1000.0 * dt, now);
            else
              frame_history.addSample (INFINITY, now);

            frame_history2.addSample (SK::Framerate::GetLimiter ()->effective_frametime (), now);

            double mean    = frame_history.calcMean     ();
            double sd      = frame_history.calcSqStdDev (mean);
            double min     = frame_history.calcMin      ();
            double max     = frame_history.calcMax      ();
            int    hitches = frame_history.calcHitches  (1.2, mean);

            double effective_mean = frame_history2.calcMean  ();

            static double fps           = 0.0;
            static DWORD  last_fps_time = timeGetTime ();

            if (timeGetTime () - last_fps_time > 666) {
              fps           = 1000.0 / mean;
              last_fps_time = timeGetTime ();
            }

                                    SK_IsD3D9                 (pApp->dwFlags);
                                    SK_IsD3D11                (pApp->dwFlags);
                                    SK_IsOpenGL               (pApp->dwFlags);
            std::wstring api_name = SK_GetAPINameFromOSDFlags (pApp->dwFlags);

            if (mean != INFINITY) {
              if (SK::Framerate::GetLimiter ()->get_limit () != 0.0 && (! isTalesOfZestiria) && frame_history2.calcNumSamples () > 0) {
                OSD_PRINTF "  %-6ws :  %#4.01f FPS, %#13.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)   <%4.01f FPS / %3.2f ms>",
                  api_name.c_str (),
                    // Cast to FP to avoid integer division by zero.
                    fps,////1000.0f * (float)pApp->dwFrames / (float)(pApp->dwTime1 - pApp->dwTime0),
                      mean,
                        sqrt (sd),
                          min,
                            max,
                              hitches,
                                1000.0 / effective_mean,
                                  effective_mean
                OSD_END
#if 0
              //
              // Framerate Smoothing
              //
              double effective_fps = 1000.0 / effective_mean;

              if (effective_fps > 60.0)
                effective_fps = 60.0;

              if (effective_fps < 30.0)
                effective_fps = 30.0;

              static double        last_change = 0.0;
              static LARGE_INTEGER when        = { 0 };

              if (fabs (effective_fps - last_change) > 10.0 || (double)(now.QuadPart - when.QuadPart) / (double)SK::Framerate::Stats::freq.QuadPart > 0.25) {
                extern float target_fps;
                target_fps = effective_fps;
                //SK::Framerate::GetLimiter ()->change_limit (effective_fps);
                last_change   = effective_fps;
                when.QuadPart = now.QuadPart;
              }
#endif



              static LARGE_INTEGER last_frame;

#if 0
              if (1000.0 * dt < config.render.framerate.max_delta_time) {
                LARGE_INTEGER next;
                next.QuadPart = last_frame.QuadPart + ((double)SK_FrameStats::freq.QuadPart * (double)(config.render.framerate.max_delta_time) / 1000.0);

                LARGE_INTEGER delay;
                while (true) {
                  QueryPerformanceCounter_Original (&delay);

                  if (delay.QuadPart < next.QuadPart)
                    continue;

                  break;
                }

                QueryPerformanceCounter_Original (&last_frame);
              }
#endif
              }

              // No Effective Frametime History
              else {
                OSD_PRINTF "  %-6ws :  %#4.01f FPS, %#13.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)",
                  api_name.c_str (),
                    // Cast to FP to avoid integer division by zero.
                    fps,//1000.0f * (float)pApp->dwFrames / (float)(pApp->dwTime1 - pApp->dwTime0),
                      mean,
                        sqrt (sd),
                          min,
                            max,
                              hitches
                OSD_END
              }
            }

            // No Frametime History
            else {
              OSD_PRINTF "  %-6ws :  %#4.01f FPS, %#13.01f ms",
                api_name.c_str (),
                  // Cast to FP to avoid integer division by zero.
                  1000.0f * (float)pApp->dwFrames / (float)(pApp->dwTime1 - pApp->dwTime0),
                    pApp->dwFrameTime / 1000.0f
              OSD_END
            }

#if 0
            extern IDXGISwapChain2* g_pSwapChain2;
            if (g_pSwapChain2 != nullptr) {
              LARGE_INTEGER freq;
              QueryPerformanceFrequency (&freq);

              freq.QuadPart /= 1000ULL;

              DXGI_FRAME_STATISTICS stats;
              g_pSwapChain2->GetFrameStatistics (&stats);

              static double last_SyncQPC = (double)stats.SyncQPCTime.QuadPart / (double)freq.QuadPart;

              OSD_PRINTF ",  %04.1fms",
                (double)stats.SyncQPCTime.QuadPart / (double)freq.QuadPart - last_SyncQPC
              OSD_END

              last_SyncQPC = (double)stats.SyncQPCTime.QuadPart / (double)freq.QuadPart;
            }
#endif

            OSD_PRINTF "\n" OSD_END

#ifdef DUMP_SWAPCHAIN_INFO
            if (g_pSwapChain9 != nullptr) {
              D3DDISPLAYMODE        dmode;
              D3DPRESENT_PARAMETERS pparams;

              g_pSwapChain9->GetDisplayMode       (&dmode);
              g_pSwapChain9->GetPresentParameters (&pparams);

              OSD_PRINTF "  SWAPCH :  %#3lu Hz (%#4lux%#4lu) - Flags: 0x%04X, Refresh: %#3lu Hz, Interval: %#2lu, Effect: %lu\n",
                dmode.RefreshRate, dmode.Width, dmode.Height,
                pparams.Flags, pparams.FullScreen_RefreshRateInHz, pparams.PresentationInterval, pparams.SwapEffect
              OSD_END
            }
#endif
            break;
          }
        }
      }
    }
  }

  // Poll GPU stats...
  SK_PollGPU ();

  int afr_idx  = sli_state.currentAFRIndex,
      afr_last = sli_state.previousFrameAFRIndex,
      afr_next = sli_state.nextFrameAFRIndex;

  for (int i = 0; i < gpu_stats.num_gpus; i++) {
    OSD_G_PRINTF "  GPU%i   :            %#3lu%%",
      i, gpu_stats.gpus [i].loads_percent.gpu
    OSD_END

    if (nvapi_init && gpu_stats.gpus [i].loads_percent.vid > 0) {
      // Vector 3D (subtract 1 space)
      OSD_G_PRINTF ",  VID%i %#3lu%% ,",

      // Raster 3D
      //OSD_G_PRINTF ",  VID%i %#3lu%%  ,",
        i, gpu_stats.gpus [i].loads_percent.vid
      OSD_END
    } else {
      // Vector 3D (subtract 1 space)
      OSD_G_PRINTF ",             " OSD_END

      // Raster 3D
      //OSD_G_PRINTF ",              " OSD_END
    }

    OSD_G_PRINTF " %#4lu MHz",
          gpu_stats.gpus [i].clocks_kHz.gpu / 1000UL
    OSD_END

    if (gpu_stats.gpus [i].volts_mV.supported)
    {
      // Over (or under) voltage limit!
      if (false)//gpu_stats.gpus [i].volts_mV.over)
      {
        OSD_G_PRINTF ", %#6.1fmV (%+#6.1fmV)",
          gpu_stats.gpus [i].volts_mV.core, gpu_stats.gpus [i].volts_mV.ov
        OSD_END
      } else {
        OSD_G_PRINTF ", %#6.1fmV",
          gpu_stats.gpus [i].volts_mV.core
        OSD_END
      }
    } else {
      // Padding because no voltage reading is available
      OSD_G_PRINTF ",         "
      OSD_END
    }

    if (gpu_stats.gpus [i].fans_rpm.supported && gpu_stats.gpus [i].fans_rpm.gpu > 0)
    {
      OSD_G_PRINTF ", %#4lu RPM",
        gpu_stats.gpus [i].fans_rpm.gpu
      OSD_END
    } else {
      // Padding because no RPM reading is available
      OSD_G_PRINTF ",         "
      OSD_END
    }

    std::wstring temp = 
      SK_FormatTemperature (
        gpu_stats.gpus [i].temps_c.gpu,
          Celsius,
            config.system.prefer_fahrenheit ? Fahrenheit :
                                              Celsius );
    OSD_G_PRINTF ", (%ws)",
      temp.c_str ()
    OSD_END

    if (config.sli.show) {
      if (afr_last == i)
        OSD_G_PRINTF "@" OSD_END

      if (afr_idx == i)
        OSD_G_PRINTF "!" OSD_END

      if (afr_next == i)
        OSD_G_PRINTF "#" OSD_END
    }

    if (nvapi_init &&
        config.gpu.print_slowdown &&
        gpu_stats.gpus [i].nv_perf_state != NV_GPU_PERF_DECREASE_NONE) {
      OSD_G_PRINTF "   SLOWDOWN:" OSD_END

      if (gpu_stats.gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_AC_BATT)
        OSD_G_PRINTF " (Battery)" OSD_END
      if (gpu_stats.gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_API_TRIGGERED)
        OSD_G_PRINTF " (Driver)" OSD_END
      if (gpu_stats.gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_INSUFFICIENT_POWER)
        OSD_G_PRINTF " (Power Supply)" OSD_END
      if (gpu_stats.gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_POWER_CONTROL)
        OSD_G_PRINTF " (Power Limit)" OSD_END
      if (gpu_stats.gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_THERMAL_PROTECTION)
        OSD_G_PRINTF " (Thermal Limit)" OSD_END
    }

    OSD_G_PRINTF "\n" OSD_END
  }

  //
  // DXGI 1.4 Memory Info (VERY accurate)
  ///
  if (nodes > 0 && nodes < 4) {
    // We need to be careful here, it's not guaranteed that NvAPI adapter indices
    //   match up with DXGI 1.4 node indices... Adapter LUID may shed some light
    //     on that in the future.
    for (int i = 0; i < nodes; i++) {
      if (nvapi_init) {
        OSD_G_PRINTF "  VRAM%i  : %#5llu MiB (%#3lu%%: %#5.01lf GiB/s)",
          i,
          mem_info [buffer].local    [i].CurrentUsage >> 20ULL,
                      gpu_stats.gpus [i].loads_percent.fb,
          (double)((uint64_t)gpu_stats.gpus [i].clocks_kHz.ram * 2ULL * 1000ULL *
                   (uint64_t)gpu_stats.gpus [i].hwinfo.mem_bus_width) / 8.0 /
                     (1024.0 * 1024.0 * 1024.0) *
                    ((double)gpu_stats.gpus [i].loads_percent.fb / 100.0)
        OSD_END
      } else {
        OSD_G_PRINTF "  VRAM%i  : %#5llu MiB",
          i, mem_info [buffer].local [i].CurrentUsage >> 20ULL
        OSD_END
      }

      OSD_G_PRINTF ", %#4lu MHz",
        gpu_stats.gpus [i].clocks_kHz.ram / 1000UL
      OSD_END

      // Add memory temperature if it exists
      if (i <= gpu_stats.num_gpus && gpu_stats.gpus [i].temps_c.ram != 0) {
        std::wstring temp = 
          SK_FormatTemperature (
            gpu_stats.gpus [i].temps_c.gpu,
              Celsius,
                config.system.prefer_fahrenheit ? Fahrenheit :
                                                  Celsius );
        OSD_G_PRINTF ", (%ws)",
          temp.c_str ()
        OSD_END
      }

      OSD_G_PRINTF "\n" OSD_END
    }

    for (int i = 0; i < nodes; i++) {
      // Figure out the generation from the transfer rate...
      int pcie_gen = gpu_stats.gpus [i].hwinfo.pcie_gen;

      if (nvapi_init) {
        OSD_G_PRINTF "  SHARE%i : %#5llu MiB (%#3lu%%: %#5.02lf GiB/s), PCIe %li.0x%lu\n",
          i,
           mem_info [buffer].nonlocal [i].CurrentUsage >> 20ULL,
                       gpu_stats.gpus [i].loads_percent.bus,
                       gpu_stats.gpus [i].hwinfo.pcie_bandwidth_mb () / 1024.0 *
              ((double)gpu_stats.gpus [i].loads_percent.bus / 100.0),
                       pcie_gen,
                       gpu_stats.gpus [i].hwinfo.pcie_lanes
                       //gpu_stats.gpus [i].hwinfo.pcie_transfer_rate
        OSD_END
      } else {
        OSD_G_PRINTF "  SHARE%i : %#5llu MiB, PCIe %li.0x%lu\n",
          i,
          mem_info [buffer].nonlocal [i].CurrentUsage >> 20ULL,
          pcie_gen,
          gpu_stats.gpus [i].hwinfo.pcie_lanes
        OSD_END
      }
    }
  }

  //
  // NvAPI or ADL Memory Info (Reasonably Accurate on Windows 8.1 and older)
  //
  else {
    // We need to be careful here, it's not guaranteed that NvAPI adapter indices
    //   match up with DXGI 1.4 node indices... Adapter LUID may shed some light
    //     on that in the future.
    for (int i = 0; i < gpu_stats.num_gpus; i++) {
      if (nvapi_init) {
        OSD_G_PRINTF "  VRAM%i  : %#5llu MiB (%#3lu%%: %#5.01lf GiB/s)",
          i,
                      gpu_stats.gpus [i].memory_B.local >> 20ULL,
                      gpu_stats.gpus [i].loads_percent.fb,
          (double)((uint64_t)gpu_stats.gpus [i].clocks_kHz.ram * 2ULL * 1000ULL *
                   (uint64_t)gpu_stats.gpus [i].hwinfo.mem_bus_width) / 8.0 /
                     (1024.0 * 1024.0 * 1024.0) *
                    ((double)gpu_stats.gpus [i].loads_percent.fb / 100.0)
        OSD_END
      } else {
        OSD_G_PRINTF "  VRAM%i  : %#5llu MiB",
          i, gpu_stats.gpus [i].memory_B.local >> 20ULL
        OSD_END
      }

      OSD_G_PRINTF ", %#4lu MHz",
        gpu_stats.gpus [i].clocks_kHz.ram / 1000UL
      OSD_END

      OSD_G_PRINTF "\n" OSD_END
    }

    for (int i = 0; i < gpu_stats.num_gpus; i++) {
      // Figure out the generation from the transfer rate...
      int pcie_gen = gpu_stats.gpus [i].hwinfo.pcie_gen;

      if (nvapi_init) {
        OSD_G_PRINTF "  SHARE%i : %#5llu MiB (%#3lu%%: %#5.02lf GiB/s), PCIe %li.0x%lu\n",
          i,
                       gpu_stats.gpus [i].memory_B.nonlocal >> 20ULL,
                       gpu_stats.gpus [i].loads_percent.bus,
                       gpu_stats.gpus [i].hwinfo.pcie_bandwidth_mb () / 1024.0 *
              ((double)gpu_stats.gpus [i].loads_percent.bus / 100.0),
                       pcie_gen,
                       gpu_stats.gpus [i].hwinfo.pcie_lanes
                       //gpu_stats.gpus [i].hwinfo.pcie_transfer_rate
        OSD_END
      } else {
        OSD_G_PRINTF "  SHARE%i : %#5llu MiB, PCIe %li.0x%lu\n",
          i,
          gpu_stats.gpus [i].memory_B.nonlocal    >> 20ULL,
          pcie_gen,
          gpu_stats.gpus [i].hwinfo.pcie_lanes
        OSD_END
      }

      // Add memory temperature if it exists
      if (i <= gpu_stats.num_gpus && gpu_stats.gpus [i].temps_c.ram != 0) {
        std::wstring temp = 
          SK_FormatTemperature (
            gpu_stats.gpus [i].temps_c.gpu,
              Celsius,
                config.system.prefer_fahrenheit ? Fahrenheit :
                                                  Celsius );
        OSD_G_PRINTF ", (%ws)",
          temp.c_str ()
        OSD_END
      }
    }
  }

  //OSD_G_PRINTF "\n" OSD_END

  if (config.render.show && (SK_IsD3D9 () || SK_IsD3D11 () || SK_IsOpenGL ())) {
    if (SK_IsD3D11 ()) {
      OSD_R_PRINTF "\n%ws",
        SK::DXGI::getPipelineStatsDesc ().c_str ()
      OSD_END
    } else if (SK_IsD3D9 ()) {
      OSD_R_PRINTF "\n%ws",
        SK::D3D9::getPipelineStatsDesc ().c_str ()
      OSD_END
    } else if (SK_IsOpenGL ()) {
      OSD_R_PRINTF "\n%ws",
        SK::OpenGL::getPipelineStatsDesc ().c_str ()
      OSD_END
    }
  }

  OSD_C_PRINTF "\n  Total  : %#3llu%%  -  (Kernel: %#3llu%%   "
                 "User: %#3llu%%   Interrupt: %#3llu%%)\n",
        cpu_stats.cpus [0].percent_load, 
          cpu_stats.cpus [0].percent_kernel, 
            cpu_stats.cpus [0].percent_user, 
              cpu_stats.cpus [0].percent_interrupt
  OSD_END

  for (DWORD i = 1; i < cpu_stats.num_cpus; i++) {
    if (! config.cpu.simple) {
      OSD_C_PRINTF "  CPU%lu   : %#3llu%%  -  (Kernel: %#3llu%%   "
                   "User: %#3llu%%   Interrupt: %#3llu%%)\n",
        i-1,
          cpu_stats.cpus [i].percent_load, 
            cpu_stats.cpus [i].percent_kernel, 
              cpu_stats.cpus [i].percent_user, 
                cpu_stats.cpus [i].percent_interrupt
      OSD_END
    } else {
      OSD_C_PRINTF "  CPU%lu   : %#3llu%%\n",
        i-1,
          cpu_stats.cpus [i].percent_load
      OSD_END
    }
  }

  // Only do this if the IO data view is active
  if (config.io.show)
    SK_CountIO (io_counter, config.io.interval / 1.0e-7);

  OSD_I_PRINTF "\n  Read   :%#6.02f MiB/s - (%#6.01f IOP/s)"
               "\n  Write  :%#6.02f MiB/s - (%#6.01f IOP/s)"
               "\n  Other  :%#6.02f MiB/s - (%#6.01f IOP/s)\n",
               io_counter.read_mb_sec,  io_counter.read_iop_sec,
               io_counter.write_mb_sec, io_counter.write_iop_sec,
               io_counter.other_mb_sec, io_counter.other_iop_sec
  OSD_END

  if (nodes > 0 && nodes < 4) {
    int i = 0;

    OSD_M_PRINTF "\n"
                   "----- (DXGI 1.4): Local Memory -------"
                   "--------------------------------------\n"
    OSD_END

    while (i < nodes) {
      OSD_M_PRINTF "  %8s %i  (Reserve:  %#5llu / %#5llu MiB  - "
                   " Budget:  %#5llu / %#5llu MiB)",
                  nodes > 1 ? (nvapi_init ? "SLI Node" : "CFX Node") : "GPU",
                  i,
                  mem_info [buffer].local [i].CurrentReservation      >> 20ULL,
                  mem_info [buffer].local [i].AvailableForReservation >> 20ULL,
                  mem_info [buffer].local [i].CurrentUsage            >> 20ULL,
                  mem_info [buffer].local [i].Budget                  >> 20ULL
      OSD_END

      //
      // SLI Status Indicator
      //
      if (afr_last == i)
        OSD_S_PRINTF "@" OSD_END

      if (afr_idx == i)
        OSD_S_PRINTF "!" OSD_END

      if (afr_next == i)
        OSD_S_PRINTF "#" OSD_END

      OSD_M_PRINTF "\n" OSD_END

      i++;
    }

    i = 0;

    OSD_M_PRINTF "----- (DXGI 1.4): Non-Local Memory ---"
                 "--------------------------------------\n"
    OSD_END

    while (i < nodes) {
      if ((mem_info [buffer].nonlocal [i].CurrentUsage >> 20ULL) > 0) {
        OSD_M_PRINTF "  %8s %i  (Reserve:  %#5llu / %#5llu MiB  -  "
                     "Budget:  %#5llu / %#5llu MiB)\n",
                         nodes > 1 ? "SLI Node" : "GPU",
                         i,
                mem_info [buffer].nonlocal [i].CurrentReservation      >> 20ULL,
                mem_info [buffer].nonlocal [i].AvailableForReservation >> 20ULL,
                mem_info [buffer].nonlocal [i].CurrentUsage            >> 20ULL,
                mem_info [buffer].nonlocal [i].Budget                  >> 20ULL
        OSD_END
      }

      i++;
    }

    OSD_M_PRINTF "----- (DXGI 1.4): Miscellaneous ------"
                 "--------------------------------------\n"
    OSD_END

    int64_t headroom = mem_info [buffer].local [0].Budget -
                       mem_info [buffer].local [0].CurrentUsage;

    OSD_M_PRINTF "  Max. Resident Set:  %#5llu MiB  -"
                 "  Max. Over Budget:  %#5llu MiB\n"
                 "     Budget Changes:  %#5llu      - "
                  "      Budget Left:  %#5lli MiB\n",
                                    mem_stats [0].max_usage       >> 20ULL,
                                    mem_stats [0].max_over_budget >> 20ULL,
                                    mem_stats [0].budget_changes,
                                    headroom / 1024 / 1024
    OSD_END
  }

  OSD_M_PRINTF "\n" OSD_END

  std::wstring working_set =
    SK_SizeToString (process_stats.memory.working_set,   MiB);
  std::wstring commit =
    SK_SizeToString (process_stats.memory.private_bytes, MiB);
  std::wstring virtual_size = 
    SK_SizeToString (process_stats.memory.virtual_bytes, MiB);

  OSD_M_PRINTF "  Working Set: %ws,  Committed: %ws,  Address Space: %ws\n",
    working_set.c_str  (),
    commit.c_str       (),
    virtual_size.c_str ()
  OSD_END

  std::wstring working_set_peak =
    SK_SizeToString (process_stats.memory.working_set_peak,     MiB);
  std::wstring commit_peak =
    SK_SizeToString (process_stats.memory.page_file_bytes_peak, MiB);
  std::wstring virtual_peak = 
    SK_SizeToString (process_stats.memory.virtual_bytes_peak,   MiB);

  OSD_M_PRINTF "        *Peak: %ws,      *Peak: %ws,          *Peak: %ws\n",
    working_set_peak.c_str (),
    commit_peak.c_str      (),
    virtual_peak.c_str     ()
  OSD_END

  extern bool     SK_D3D11_need_tex_reset;
  extern uint32_t SK_D3D11_amount_to_purge;
  extern bool     SK_D3D11_cache_textures;

  if (SK_D3D11_cache_textures) {
    extern std::string SK_D3D11_SummarizeTexCache (void);

    OSD_M_PRINTF "\n%s",
      SK_D3D11_SummarizeTexCache ().c_str ()
    OSD_END
  }

  extern int gpu_prio;

  OSD_B_PRINTF "\n  GPU Priority: %+1i\n",
    gpu_prio
  OSD_END

#if 0
  bool use_mib_sec = disk_stats.num_disks > 0 ?
                       (disk_stats.disks [0].bytes_sec > (1024 * 1024 * 2)) : false;

  if (use_mib_sec) {
#endif
    for (DWORD i = 0; i < disk_stats.num_disks; i++) {
      std::wstring read_bytes_sec =
        SK_SizeToStringF (disk_stats.disks [i].read_bytes_sec, 6, 1);

      std::wstring write_bytes_sec =
        SK_SizeToStringF (disk_stats.disks [i].write_bytes_sec, 6, 1);

      if (i == 0) {
        OSD_D_PRINTF "\n  Disk %16s %#3llu%%  -  (Read %#3llu%%: %ws/s, "
                                                 "Write %#3llu%%: %ws/s)\n",
          disk_stats.disks [i].name,
            disk_stats.disks [i].percent_load,
              disk_stats.disks [i].percent_read,
                read_bytes_sec.c_str (),
                  disk_stats.disks [i].percent_write,
                    write_bytes_sec.c_str ()
        OSD_END
      }
      else {
        OSD_D_PRINTF "  Disk %-16s %#3llu%%  -  (Read %#3llu%%: %ws/s, "
                                                "Write %#3llu%%: %ws/s)\n",
          disk_stats.disks [i].name,
            disk_stats.disks [i].percent_load,
              disk_stats.disks [i].percent_read,
                read_bytes_sec.c_str (),
                  disk_stats.disks [i].percent_write,
                    write_bytes_sec.c_str ()
        OSD_END
      }
    }
#if 0
  }
  else
  {
    for (int i = 0; i < disk_stats.num_disks; i++) {
      OSD_D_PRINTF "\n  Disk %16s %#3llu%%  -  (Read: %#3llu%%   Write: %#3llu%%) - "
                                        "(Read: %#5.01f KiB   Write: %#5.01f KiB)",
        disk_stats.disks[i].name,
          disk_stats.disks[i].percent_load,
            disk_stats.disks[i].percent_read,
              disk_stats.disks[i].percent_write,
                (float)disk_stats.disks[i].read_bytes_sec / (1024.0f),
                (float)disk_stats.disks[i].write_bytes_sec / (1024.0f)
      OSD_END

      if (i == 0)
        OSD_D_PRINTF "\n" OSD_END
    }
  }
#endif

  for (DWORD i = 0; i < pagefile_stats.num_pagefiles; i++) {
      std::wstring usage =
        SK_SizeToStringF (pagefile_stats.pagefiles [i].usage, 5,2);
      std::wstring size = 
        SK_SizeToStringF (pagefile_stats.pagefiles [i].size, 5,2);
      std::wstring peak =
        SK_SizeToStringF (pagefile_stats.pagefiles [i].usage_peak, 5,2);

      OSD_P_PRINTF "\n  Pagefile %20s  %ws / %ws  (Peak: %ws)",
        pagefile_stats.pagefiles [i].name,
          usage.c_str    (),
            size.c_str   (),
              peak.c_str ()
      OSD_END
  }

  OSD_P_PRINTF "\n" OSD_END

  // Avoid unnecessary MMIO when the user has the OSD turned off
  cleared = (! config.osd.show);

  BOOL ret = SK_UpdateOSD (szOSD, pMemory);

  SK_ReleaseSharedMemory (pMemory);

  return ret;
}

BOOL
SK_UpdateOSD (LPCSTR lpText, LPVOID pMapAddr, LPCSTR lpAppName)
{
  if (lpAppName == nullptr)
    lpAppName = "Special K";

  static DWORD dwProcID =
    GetCurrentProcessId ();

  BOOL bResult = FALSE;

  if (osd_shutting_down && osd_init == false) {
    return bResult;
  }

  // If pMapAddr == nullptr, manage memory ourselves
  bool own_memory = false;
  if (pMapAddr == nullptr) {
    // When shutting down, don't care if RivaTuner knows about our process
    //   anymore ... we simply want the text gone!
    if (osd_shutting_down) {
      pMapAddr = SK_GetSharedMemory (0);
      osd_init = false;
      lpText = "\0";
    }
    else
      pMapAddr = SK_GetSharedMemory (dwProcID);
    own_memory = true;
  }

  LPRTSS_SHARED_MEMORY pMem     =
    (LPRTSS_SHARED_MEMORY)pMapAddr;

  if (pMem)
  {
    for (DWORD dwPass = 0; dwPass < 2; dwPass++)
    {
      //1st Pass: Find previously captured OSD slot
      //2nd Pass: Otherwise find the first unused OSD slot and capture it

      for (DWORD dwEntry = 1; dwEntry < pMem->dwOSDArrSize; dwEntry++)
      {
        // Allow primary OSD clients (i.e. EVGA Precision / MSI Afterburner)
        //   to use the first slot exclusively, so third party applications
        //     start scanning the slots from the second one

        RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry =
          (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)
            ((LPBYTE)pMem + pMem->dwOSDArrOffset +
                  dwEntry * pMem->dwOSDEntrySize);

        if (dwPass)
        {
          if (! strlen (pEntry->szOSDOwner))
            strcpy (pEntry->szOSDOwner, lpAppName);
        }

        if (! strcmp (pEntry->szOSDOwner, lpAppName))
        {
          if (pMem->dwVersion >= 0x00020007)
            //Use extended text slot for v2.7 and higher shared memory,
            // it allows displaying 4096 symbols instead of 256 for regular
            //  text slot
            strncpy (pEntry->szOSDEx, lpText, sizeof pEntry->szOSDEx - 1);
            //snprintf (pEntry->szOSDEx, sizeof pEntry->szOSDEx - 1, "Frame: %d\n%s", pMem->dwOSDFrame, lpText);
          else
            strncpy (pEntry->szOSD,   lpText, sizeof pEntry->szOSD   - 1);

          pMem->dwOSDFrame++;

          bResult = TRUE;
          break;
        }
      }

      if (bResult)
        break;
    }
  }

  if (own_memory)
    SK_ReleaseSharedMemory (pMapAddr);

  return bResult;
}

void
SK_ReleaseOSD (void)
{
  // Reload the RTSS DLL so we can update the text
  HMODULE hModRTSS =
    LoadLibraryW (rtss_hook_dll.c_str ());

  osd_shutting_down = true;
  osd_init          = true;

  SK_UpdateOSD   ("", nullptr, external_osd_name.c_str ());

  osd_init          = true;
  osd_shutting_down = true;

  SK_UpdateOSD   ("");

  FreeLibrary    (hModRTSS);
}


void
SK_SetOSDPos (int x, int y, LPCSTR lpAppName)
{
  if (lpAppName == nullptr)
    lpAppName = "Special K";

  // 0,0 means don't touch anything.
  if (x == 0 && y == 0)
    return;

  LPVOID pMapAddr =
    SK_GetSharedMemory ();

  LPRTSS_SHARED_MEMORY pMem     =
    (LPRTSS_SHARED_MEMORY)pMapAddr;

  if (pMem)
  {
    for (DWORD dwEntry = 1; dwEntry < pMem->dwOSDArrSize; dwEntry++)
    {
      RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry =
        (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)
        ((LPBYTE)pMem + pMem->dwOSDArrOffset +
          dwEntry * pMem->dwOSDEntrySize);

      if (! strcmp (pEntry->szOSDOwner, lpAppName))
      {
        for (DWORD dwApp = 0; dwApp < pMem->dwAppArrSize; dwApp++)
        {
          RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY pApp =
            (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY)
            ((LPBYTE)pMem + pMem->dwAppArrOffset +
              dwApp * pMem->dwAppEntrySize);

          if (pApp->dwProcessID == GetCurrentProcessId ())
          {
            config.osd.pos_x = x;
            config.osd.pos_y = y;

            pApp->dwOSDX = x;
            pApp->dwOSDX = y;

            pApp->dwFlags |= OSDFLAG_UPDATED;
            break;
          }
        }
        break;
      }
    }
  }
  SK_ReleaseSharedMemory (pMapAddr);
}

void
SK_SetOSDColor (int red, int green, int blue, LPCSTR lpAppName)
{
  if (lpAppName == nullptr)
    lpAppName = "Special K";

  LPVOID pMapAddr =
    SK_GetSharedMemory ();

  LPRTSS_SHARED_MEMORY pMem     =
    (LPRTSS_SHARED_MEMORY)pMapAddr;

  if (pMem)
  {
    for (DWORD dwEntry = 1; dwEntry < pMem->dwOSDArrSize; dwEntry++)
    {
      RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry =
        (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)
        ((LPBYTE)pMem + pMem->dwOSDArrOffset +
          dwEntry * pMem->dwOSDEntrySize);

      if (! strcmp (pEntry->szOSDOwner, "Special K"))
      {
        for (DWORD dwApp = 0; dwApp < pMem->dwAppArrSize; dwApp++)
        {
          RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY pApp =
            (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY)
            ((LPBYTE)pMem + pMem->dwAppArrOffset +
              dwApp * pMem->dwAppEntrySize);

          if (pApp->dwProcessID == GetCurrentProcessId ())
          {
            int red_   = (pApp->dwOSDColor >> 16) & 0xFF;
            int green_ = (pApp->dwOSDColor >>  8) & 0xFF;
            int blue_  = (pApp->dwOSDColor      ) & 0xFF;

            if (red >= 0 && red <= 255) {
              config.osd.red = red;
              red_ = red;
            }

            if (green >= 0 && green <= 255) {
              config.osd.green = green;
              green_ = green;
            }

            if (blue >= 0 && blue <= 255) {
              config.osd.blue = blue;
              blue_ = blue;
            }

            pApp->dwOSDColor = ((red_ << 16) & 0xff0000) | ((green_ << 8) & 0xff00) | (blue_ & 0xff);

            pApp->dwFlags |= OSDFLAG_UPDATED;
            break;
          }
        }
        break;
      }
    }
  }
  SK_ReleaseSharedMemory (pMapAddr);
}

void
SK_SetOSDScale (DWORD dwScale, bool relative, LPCSTR lpAppName)
{
  if (lpAppName == nullptr)
    lpAppName = "Special K";

  LPVOID pMapAddr =
    SK_GetSharedMemory ();

  LPRTSS_SHARED_MEMORY pMem     =
    (LPRTSS_SHARED_MEMORY)pMapAddr;

  if (pMem)
  {
    for (DWORD dwEntry = 1; dwEntry < pMem->dwOSDArrSize; dwEntry++)
    {
      RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry =
        (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)
        ((LPBYTE)pMem + pMem->dwOSDArrOffset +
          dwEntry * pMem->dwOSDEntrySize);

      if (! strcmp (pEntry->szOSDOwner, "Special K"))
      {
        for (DWORD dwApp = 0; dwApp < pMem->dwAppArrSize; dwApp++)
        {
          RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY pApp =
            (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_APP_ENTRY)
            ((LPBYTE)pMem + pMem->dwAppArrOffset +
              dwApp * pMem->dwAppEntrySize);

          if (pApp->dwProcessID == GetCurrentProcessId ())
          {
            if (! relative)
              pApp->dwOSDPixel = dwScale;

             else
               pApp->dwOSDPixel += dwScale;

            // Clamp to a sane range :)
            if (pApp->dwOSDPixel < 1)
              pApp->dwOSDPixel = 1;

            config.osd.scale = pApp->dwOSDPixel;

            pApp->dwFlags |= OSDFLAG_UPDATED;
            break;
          }
        }
        break;
      }
    }
  }
  SK_ReleaseSharedMemory (pMapAddr);
}

void
SK_ResizeOSD (int scale_incr, LPCSTR lpAppName)
{
  if (lpAppName == nullptr)
    lpAppName = "Special K";

  SK_SetOSDScale (scale_incr, true, lpAppName);
}

