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

#define OSD_IMP
#include <SpecialK/osd/text.h>

#include <SpecialK/render_backend.h>
#include <SpecialK/command.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>

#include <CEGUI/CEGUI.h>
#include <CEGUI/Rect.h>
#include <CEGUI/Renderer.h>

#include <map>


SK_TextOverlayManager*
SK_TextOverlayManager::getInstance (void)
{
  return ( pSelf == nullptr ? (pSelf = new SK_TextOverlayManager ()) :
                               pSelf );
}

SK_TextOverlayManager* SK_TextOverlayManager::pSelf = nullptr;
CRITICAL_SECTION       SK_TextOverlayManager::cs_   = {     };

SK_TextOverlayManager::SK_TextOverlayManager (void)
{
  InitializeCriticalSectionAndSpinCount (&cs_, 104858);

  gui_ctx_         = nullptr;
  need_full_reset_ = false;

  SK_ICommandProcessor* cmd =
    SK_GetCommandProcessor ();

  cmd->AddVariable ("OSD.Red",   SK_CreateVar (SK_IVariable::Int, &config.osd.red));
  cmd->AddVariable ("OSD.Green", SK_CreateVar (SK_IVariable::Int, &config.osd.green));
  cmd->AddVariable ("OSD.Blue",  SK_CreateVar (SK_IVariable::Int, &config.osd.blue));

  pos_.x = SK_CreateVar (SK_IVariable::Int,   &config.osd.pos_x, this);
  pos_.y = SK_CreateVar (SK_IVariable::Int,   &config.osd.pos_y, this);
  scale_ = SK_CreateVar (SK_IVariable::Float, &config.osd.scale, this);

  cmd->AddVariable ("OSD.PosX",  pos_.x);
  cmd->AddVariable ("OSD.PosY",  pos_.y);
  cmd->AddVariable ("OSD.Scale", scale_);
}


SK_TextOverlay* 
SK_TextOverlayManager::createTextOverlay (const char *szAppName)
{
  SK_AutoCriticalSection auto_crit (&cs_);

  std::string app_name (szAppName);

  if (overlays_.count (app_name))
  {
    SK_TextOverlay* overlay =
      overlays_ [app_name];

    return overlay;
  }

  auto* overlay =
    new SK_TextOverlay (szAppName);

  overlay->setPos   ( static_cast <float> (config.osd.pos_x),
                        static_cast <float> (config.osd.pos_y) );
  overlay->setScale ( config.osd.scale );

  overlays_ [app_name] = overlay;

  return overlay;
}

bool
SK_TextOverlayManager::removeTextOverlay (const char* szAppName)
{
  SK_AutoCriticalSection auto_crit (&cs_);

  std::string app_name (szAppName);

  if (overlays_.count (app_name))
  {
    overlays_.erase   (app_name);

    return true;
  }

  return false;
}

SK_TextOverlay*
SK_TextOverlayManager::getTextOverlay (const char* szAppName)
{
  SK_AutoCriticalSection auto_crit (&cs_);

  std::string app_name (szAppName);

  if (overlays_.count (app_name))
  {
    SK_TextOverlay* overlay = overlays_ [app_name];

    return overlay;
  }

  return nullptr;
}

SK_TextOverlay::SK_TextOverlay (const char* szAppName)
{
  strncpy (data_.name, szAppName,          32);
  strncpy (font_.name, "Consolas-12.font", 64);

  data_.text_len = 32768;
  data_.text     = (char *)calloc (1, data_.text_len);

  if (data_.text != nullptr)
    *data_.text  = '\0';
  else
    data_.text_len = 0; // Failed to allocate memory

  geometry_   = nullptr;
  renderer_   = nullptr;
  font_.cegui = nullptr;
  font_.scale = 1.0f;
}

SK_TextOverlay::~SK_TextOverlay (void)
{
  SK_TextOverlayManager::getInstance ()->removeTextOverlay (data_.name);

  if (data_.text != nullptr)
  {
    free ((void *)data_.text);
    data_.text_len = 0;
    data_.text     = nullptr;
  }

  if (geometry_ != nullptr)
  {
    if (renderer_ != nullptr)
    {
      renderer_->destroyGeometryBuffer (*geometry_);
      geometry_ = nullptr;
    }
  }
}

#pragma comment (lib, "Shlwapi.lib")
#include <Shlwapi.h>

#include <atlbase.h>

#include <float.h>
#include <io.h>
#include <tchar.h>

#include <SpecialK/io_monitor.h>
#include <SpecialK/gpu_monitor.h>
#include <SpecialK/memory_monitor.h>

#include <SpecialK/core.h>
#include <SpecialK/framerate.h>

#include <SpecialK/log.h>

#include <SpecialK/dxgi_backend.h>
#include <SpecialK/d3d9_backend.h>
#include <SpecialK/opengl_backend.h>

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

char szOSD [32768] = { };

#include <SpecialK/nvapi.h>
extern NV_GET_CURRENT_SLI_STATE sli_state;
extern BOOL nvapi_init;

// Probably need to use a critical section to make this foolproof, we will
//   cross that bridge later though. The OSD is performance critical
bool osd_shutting_down = false;

// Initialize some things (like color, position and scale) on first use
volatile LONG osd_init = FALSE;

BOOL
__stdcall
SK_ReleaseSharedMemory (LPVOID lpMemory)
{
  UNREFERENCED_PARAMETER (lpMemory);

  return FALSE;
}

LPVOID
__stdcall
SK_GetSharedMemory (DWORD dwProcID)
{
  UNREFERENCED_PARAMETER (dwProcID);

  return nullptr;
}

LPVOID
__stdcall
SK_GetSharedMemory (void)
{
  return SK_GetSharedMemory (GetCurrentProcessId ());
}

#include <SpecialK/log.h>
#include <d3d9.h>

bool
__stdcall
SK_IsD3D8 (void)
{
  return ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D8) != 0x0;
}

bool
__stdcall
SK_IsDDraw (void)
{
  return ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::DDraw) != 0x0;
}

bool
__stdcall
SK_IsD3D9 (void)
{
  return ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9) != 0x0;
}

bool
__stdcall
SK_IsD3D11 (void)
{
  return ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D11) != 0x0;
}

bool
__stdcall
SK_IsPureD3D11 (void)
{
  return (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D11);
}

bool
SK_IsD3D12 (void)
{
  return ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D12) != 0x0;
}

bool
SK_IsPureD3D12 (void)
{
  return (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D12);
}

bool
SK_IsOpenGL (void)
{
  return (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::OpenGL);
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
  wchar_t str [64] = { };

  if (unit == Auto)
  {
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
  wchar_t str [64] = { };

  if (unit == Auto)
  {
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
  wchar_t wszOut [16] = { };

  if (in_unit == Celsius && out_unit == Fahrenheit)
  {
    //converted = in_temp * 2 + 30;
    converted = (int32_t)((float)(in_temp) * 9.0f/5.0f + 32.0f);
    _swprintf (wszOut, L"%#3li°F", converted);
  }

  else if (in_unit == Fahrenheit && out_unit == Celsius)
  {
    converted = (int32_t)(((float)in_temp - 32.0f) * (5.0f/9.0f));
    _swprintf (wszOut, L"%#2li°C", converted);
  }

  else
  {
    _swprintf (wszOut, L"%#2li°C", in_temp);
  }

  return wszOut;
}

std::string external_osd_name;

BOOL
__stdcall
SK_DrawExternalOSD (std::string app_name, std::string text)
{
  external_osd_name = app_name;

  SK_UpdateOSD (text.c_str (), nullptr, app_name.c_str ());

  return TRUE;
}

BOOL
__stdcall
SKX_DrawExternalOSD (const char* szAppName, const char* szText)
{
  external_osd_name = szAppName;

  SK_UpdateOSD (szText, nullptr, szAppName);

  return TRUE;
}


#include <SpecialK/plugin/plugin_mgr.h>

// This is a terrible design, but I don't care.
extern void
SK_CEGUI_QueueResetD3D11 (void);

extern void
SK_CEGUI_QueueResetD3D9  (void);

void
__stdcall
SK_InstallOSD (void)
{
  SK_CEGUI_QueueResetD3D9  ();
  SK_CEGUI_QueueResetD3D11 ();

  if (! InterlockedCompareExchange (&osd_init, TRUE, FALSE))
  {
    SK_TextOverlayManager::getInstance ()->createTextOverlay ("Special K");

    SK_SetOSDScale (config.osd.scale);
    SK_SetOSDPos   (config.osd.pos_x, config.osd.pos_y);
    SK_SetOSDColor (config.osd.red,   config.osd.green, config.osd.blue);
  }
}


SK::Framerate::Stats frame_history;
SK::Framerate::Stats frame_history2;

#include <SpecialK/command.h>

#include <SpecialK/diagnostics/debug_utils.h>

BOOL
__stdcall
SK_DrawOSD (void)
{
  LARGE_INTEGER now;
  double        dt;

  //
  // This really does not belong here -- this is where the framerate limiter
  //   is implemented, it is only here because it also computes frametime
  //     statistics.
  //
  SK::Framerate::Tick (dt, now);

  static bool cleared = false;

  if ((! InterlockedExchangeAdd (&osd_init, 0)))
    SK_InstallOSD ();

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

  if (! InterlockedExchangeAdd (&osd_init, 0))
    return FALSE;

  char* pszOSD = szOSD;
  *pszOSD = '\0';

  static io_perf_t
    io_counter;

  buffer_t buffer = mem_info [0].buffer;
  int      nodes  = mem_info [buffer].nodes;

  if (config.time.show)
  {
    SYSTEMTIME     st;
    GetLocalTime (&st);

    wchar_t time [128] = { };

    GetTimeFormat ( config.time.format,
                      0L,
                        &st,
                          nullptr,
                            time,
                              128 );

    static HMODULE hModGame = GetModuleHandle (nullptr);
    static wchar_t wszGameName [MAX_PATH] = { };

    if (wszGameName [0] == L'\0')
    {
      GetModuleFileName (hModGame, wszGameName, MAX_PATH);

      if (StrStrIW (wszGameName, L"BatmanAK.exe"))
        isArkhamKnight = true;
      else if (StrStrIW (wszGameName, L"Tales of Zestiria.exe"))
        isTalesOfZestiria = true;
      else if (StrStrIW (wszGameName, L"Fallout4.exe"))
        isFallout4 = true;
      else if (StrStrIW (wszGameName, L"NieRAutomata"))
        isNieRAutomata = true;
      else if (StrStrIW (wszGameName, L"DarkSoulsIII.exe"))
        isDarkSouls3 = true;
      else if (StrStrIW (wszGameName, L"EoCApp.exe"))
        isDivinityOrigSin = true;
    }

    if (isFallout4)
    {
      OSD_PRINTF "Fallout 4 \"Works\" v 0.3.5   %ws\n\n",
                 time
      OSD_END
    }

    else if (isNieRAutomata)
    {
      OSD_PRINTF "%ws   %ws\n\n",
                 SK_GetPluginName ().c_str (), time
      OSD_END
    }

    else if (isDivinityOrigSin)
    {
      OSD_PRINTF "Divinity: Original Sin \"Works\" v 0.0.1   %ws\n\n",
                 time
      OSD_END
    }

    else if (isArkhamKnight)
    {
      OSD_PRINTF "Batman \"Fix\" v 0.20   %ws\n\n",
                 time
      OSD_END
    }

    else if (isDarkSouls3)
    {
      OSD_PRINTF "%ws   %ws\n\n",
                 SK_GetPluginName ().c_str (), time
      OSD_END
    }

    else if (SK_HasPlugin  ())
    {
      OSD_PRINTF "%ws   %ws\n\n",
                 SK_GetPluginName ().c_str (), time
      OSD_END
    }

    else
    {
      OSD_PRINTF "Special K v %ws   %ws\n\n",
                 SK_GetVersionStr (), time
      OSD_END
    }
  }

  if (config.fps.show)
  {
    // What the bloody hell?! How do we ever get a dt value near 0?
    if (dt > 0.0001)
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

    const DWORD dwTime = timeGetTime ();

    if (dwTime - last_fps_time > 666)
    {
      fps           = 1000.0 / mean;
      last_fps_time = dwTime;
    }


    bool gsync = false;

    if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
    {
      if (SK_GetCurrentRenderBackend   ().gsync_state.capable)
      {
        if (SK_GetCurrentRenderBackend ().gsync_state.active)
        {
          gsync = true;
        }
      }
    }


    if (mean != INFINITY)
    {
      if (SK::Framerate::GetLimiter ()->get_limit () != 0.0 && (! isTalesOfZestiria) && frame_history2.calcNumSamples () > 0)
      {
        const char* format = "  %-7ws:  %#4.01f FPS, %#13.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)   <%4.01f FPS / %3.2f ms>";

        if (gsync)
          format = "  %-7ws:  %#4.01f FPS (G-Sync),%#5.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)   <%4.01f FPS / %3.2f ms>";

        OSD_PRINTF format,
          SK_GetCurrentRenderBackend ().name,
            // Cast to FP to avoid integer division by zero.
            fps,
              mean,
                sqrt (sd),
                  min,
                    max,
                      hitches,
                        1000.0 / effective_mean,
                          effective_mean
        OSD_END
      }

      // No Effective Frametime History
      else
      {
        const char* format = "  %-7ws:  %#4.01f FPS, %#13.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)";

        if (gsync)
          format = "  %-7ws:  %#4.01f FPS (G-Sync),%#5.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)";

        OSD_PRINTF format,
          SK_GetCurrentRenderBackend ().name,
            // Cast to FP to avoid integer division by zero.
            fps,
              mean,
                sqrt (sd),
                  min,
                    max,
                      hitches
        OSD_END
      }
    }

    // No Frametime History
    else
    {
      const char* format = "  %-7ws:  %#4.01f FPS, %#13.01f ms";

      if (gsync)
        format = "  %-7ws:  %#4.01f FPS (G-Sync),%5.01f ms";

      OSD_PRINTF format,
        SK_GetCurrentRenderBackend ().name,
          // Cast to FP to avoid integer division by zero.
          1000.0f * 0.0f / 1.0f, 0.0f
      OSD_END
    }

    OSD_PRINTF "\n" OSD_END
  }

  // Poll GPU stats...
  if (config.gpu.show)
    SK_PollGPU ();

  int afr_idx  = sli_state.currentAFRIndex,
      afr_last = sli_state.previousFrameAFRIndex,
      afr_next = sli_state.nextFrameAFRIndex;

  for (int i = 0; i < gpu_stats.num_gpus; i++)
  {
    OSD_G_PRINTF "  GPU%i   :            %#3lu%%",
      i, gpu_stats.gpus [i].loads_percent.gpu
    OSD_END

    if (nvapi_init && gpu_stats.gpus [i].loads_percent.vid > 0)
    {
      // Vector 3D (subtract 1 space)
      //OSD_G_PRINTF ",  VID%i %#3lu%% ,",

      // Raster 3D
      OSD_G_PRINTF ",  VID%i %#3lu%%  ,",
        i, gpu_stats.gpus [i].loads_percent.vid
      OSD_END
    } else {
      // Vector 3D (subtract 1 space)
      //OSD_G_PRINTF ",             " OSD_END

      // Raster 3D
      OSD_G_PRINTF ",              " OSD_END
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
      }

      else
      {
        OSD_G_PRINTF ", %#6.1fmV",
          gpu_stats.gpus [i].volts_mV.core
        OSD_END
      }
    }

    else
    {
      // Padding because no voltage reading is available
      OSD_G_PRINTF ",         "
      OSD_END
    }

    if (gpu_stats.gpus [i].fans_rpm.supported && gpu_stats.gpus [i].fans_rpm.gpu > 0)
    {
      OSD_G_PRINTF ", %#4lu RPM",
        gpu_stats.gpus [i].fans_rpm.gpu
      OSD_END
    }

    else
    {
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

    if (config.sli.show)
    {
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
  if (nodes > 0 && nodes < 4)
  {
    // We need to be careful here, it's not guaranteed that NvAPI adapter indices
    //   match up with DXGI 1.4 node indices... Adapter LUID may shed some light
    //     on that in the future.
    for (int i = 0; i < nodes; i++)
    {
      if (nvapi_init)
      {
        OSD_G_PRINTF "  VRAM%i  : %#5llu MiB (%#3lu%%: %#5.01lf GiB/s)",
          i,
                                   mem_info [buffer].local    [i].CurrentUsage            >>   20ULL,
                                               gpu_stats.gpus [i].loads_percent.fb,
static_cast <double> ( static_cast <uint64_t> (gpu_stats.gpus [i].clocks_kHz.ram) * 2ULL   * 1000ULL  *
                       static_cast <uint64_t> (gpu_stats.gpus [i].hwinfo.mem_bus_width) )  /     8.0  /
                                                                           (1024.0 * 1024.0 * 1024.0) *
static_cast <double> (                         gpu_stats.gpus [i].loads_percent.fb      )  /   100.0
        OSD_END
      }

      else
      {
        OSD_G_PRINTF "  VRAM%i  : %#5llu MiB",
          i, mem_info [buffer].local [i].CurrentUsage >> 20ULL
        OSD_END
      }

      OSD_G_PRINTF ", %#4lu MHz",
        gpu_stats.gpus [i].clocks_kHz.ram / 1000UL
      OSD_END

      // Add memory temperature if it exists
      if (i <= gpu_stats.num_gpus && gpu_stats.gpus [i].temps_c.ram != 0)
      {
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

    for (int i = 0; i < nodes; i++)
    {
      // Figure out the generation from the transfer rate...
      int pcie_gen = gpu_stats.gpus [i].hwinfo.pcie_gen;

      if (nvapi_init)
      {
        OSD_G_PRINTF "  SHARE%i : %#5llu MiB (%#3lu%%: %#5.02lf GiB/s), PCIe %i.0x%lu\n",
          i,
           mem_info [buffer].nonlocal [i].CurrentUsage               >>  20ULL,
                       gpu_stats.gpus [i].loads_percent.bus,
                       gpu_stats.gpus [i].hwinfo.pcie_bandwidth_mb () / 1024.0 *
 static_cast <double> (gpu_stats.gpus [i].loads_percent.bus)          /  100.0,
                       pcie_gen,
                       gpu_stats.gpus [i].hwinfo.pcie_lanes
                       //gpu_stats.gpus [i].hwinfo.pcie_transfer_rate
        OSD_END
      }

      else
      {
        OSD_G_PRINTF "  SHARE%i : %#5llu MiB, PCIe %i.0x%lu\n",
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
  else
  {
    // We need to be careful here, it's not guaranteed that NvAPI adapter indices
    //   match up with DXGI 1.4 node indices... Adapter LUID may shed some light
    //     on that in the future.
    for (int i = 0; i < gpu_stats.num_gpus; i++)
    {
      if (nvapi_init)
      {
        OSD_G_PRINTF "  VRAM%i  : %#5llu MiB (%#3lu%%: %#5.01lf GiB/s)",
          i,
          mem_info [buffer].local    [i].CurrentUsage >> 20ULL,
                      gpu_stats.gpus [i].loads_percent.fb,
static_cast <double> ( static_cast <uint64_t> (gpu_stats.gpus [i].clocks_kHz.ram) * 2ULL   * 1000ULL  *
                       static_cast <uint64_t> (gpu_stats.gpus [i].hwinfo.mem_bus_width) )  /     8.0  /
                                                                           (1024.0 * 1024.0 * 1024.0) *
static_cast <double> (                         gpu_stats.gpus [i].loads_percent.fb      )  /   100.0
        OSD_END
      }

      else
      {
        OSD_G_PRINTF "  VRAM%i  : %#5llu MiB",
          i, gpu_stats.gpus [i].memory_B.local >> 20ULL
        OSD_END
      }

      OSD_G_PRINTF ", %#4lu MHz",
        gpu_stats.gpus [i].clocks_kHz.ram / 1000UL
      OSD_END

      OSD_G_PRINTF "\n" OSD_END
    }

    for (int i = 0; i < gpu_stats.num_gpus; i++)
    {
      // Figure out the generation from the transfer rate...
      int pcie_gen = gpu_stats.gpus [i].hwinfo.pcie_gen;

      if (nvapi_init)
      {
        OSD_G_PRINTF "  SHARE%i : %#5llu MiB (%#3lu%%: %#5.02lf GiB/s), PCIe %i.0x%lu\n",
          i,
                       gpu_stats.gpus [i].memory_B.nonlocal          >> 20ULL,
                       gpu_stats.gpus [i].loads_percent.bus,
                       gpu_stats.gpus [i].hwinfo.pcie_bandwidth_mb () / 1024.0 *
 static_cast <double> (gpu_stats.gpus [i].loads_percent.bus)          / 100.0,
                       pcie_gen,
                       gpu_stats.gpus [i].hwinfo.pcie_lanes
                       //gpu_stats.gpus [i].hwinfo.pcie_transfer_rate
        OSD_END
      }

      else
      {
        OSD_G_PRINTF "  SHARE%i : %#5llu MiB, PCIe %i.0x%lu\n",
          i,
          gpu_stats.gpus [i].memory_B.nonlocal    >> 20ULL,
          pcie_gen,
          gpu_stats.gpus [i].hwinfo.pcie_lanes
        OSD_END
      }

      // Add memory temperature if it exists
      if (i <= gpu_stats.num_gpus && gpu_stats.gpus [i].temps_c.ram != 0)
      {
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

  if (config.render.show && (SK_IsD3D9 () || SK_IsD3D11 () || SK_IsOpenGL ()))
  {
    if (SK_IsD3D11 ())
    {
      OSD_R_PRINTF "\n%ws",
        SK::DXGI::getPipelineStatsDesc ().c_str ()
      OSD_END
    }

    else if (SK_IsD3D9 ())
    {
      OSD_R_PRINTF "\n%ws",
        SK::D3D9::getPipelineStatsDesc ().c_str ()
      OSD_END
    } 

    else if (SK_IsOpenGL ())
    {
      OSD_R_PRINTF "\n%ws",
        SK::OpenGL::getPipelineStatsDesc ().c_str ()
      OSD_END
    }
  }

  if (cpu_stats.booting)
  {
    OSD_C_PRINTF "\n  Starting CPU Monitor...\n" OSD_END
  }

  else
  {
    OSD_C_PRINTF "\n  Total  : %#3llu%%  -  (Kernel: %#3llu%%   "
                   "User: %#3llu%%   Interrupt: %#3llu%%)\n",
          cpu_stats.cpus [0].percent_load, 
            cpu_stats.cpus [0].percent_kernel, 
              cpu_stats.cpus [0].percent_user, 
                cpu_stats.cpus [0].percent_interrupt
    OSD_END

    for (DWORD i = 1; i < cpu_stats.num_cpus; i++)
    {
      if (! config.cpu.simple)
      {
        OSD_C_PRINTF "  CPU%lu   : %#3llu%%  -  (Kernel: %#3llu%%   "
                     "User: %#3llu%%   Interrupt: %#3llu%%)\n",
          i-1,
            cpu_stats.cpus [i].percent_load, 
              cpu_stats.cpus [i].percent_kernel, 
                cpu_stats.cpus [i].percent_user, 
                  cpu_stats.cpus [i].percent_interrupt
        OSD_END
      }

      else
      {
        OSD_C_PRINTF "  CPU%lu   : %#3llu%%\n",
          i-1,
            cpu_stats.cpus [i].percent_load
        OSD_END
      }
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

  if (nodes > 0 && nodes < 4)
  {
    int i = 0;

    OSD_M_PRINTF "\n"
                   "----- (DXGI 1.4): Local Memory -------"
                   "--------------------------------------\n"
    OSD_END

    while (i < nodes)
    {
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

    while (i < nodes)
    {
      if ((mem_info [buffer].nonlocal [i].CurrentUsage >> 20ULL) > 0)
      {
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

  if (process_stats.booting)
  {
    OSD_M_PRINTF "  Starting Memory Monitor...\n" OSD_END
  }

  else
  {
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
  }

  extern bool     SK_D3D11_need_tex_reset;
  extern uint32_t SK_D3D11_amount_to_purge;
  extern bool     SK_D3D11_cache_textures;

  if (SK_D3D11_cache_textures && SK_IsD3D11 ())
  {
    extern std::string SK_D3D11_SummarizeTexCache (void);

    OSD_M_PRINTF "\n%s\n",
      SK_D3D11_SummarizeTexCache ().c_str ()
    OSD_END
  }

  extern int gpu_prio;

  if (disk_stats.booting)
  {
    OSD_D_PRINTF "\n  Starting Disk Monitor...\n" OSD_END
  }

  else
  {
#if 0
    bool use_mib_sec = disk_stats.num_disks > 0 ?
                         (disk_stats.disks [0].bytes_sec > (1024 * 1024 * 2)) : false;

    if (use_mib_sec) {
#endif
    for (DWORD i = 0; i < disk_stats.num_disks; i++)
    {
      std::wstring read_bytes_sec =
        SK_SizeToStringF (disk_stats.disks [i].read_bytes_sec, 6, 1);

      std::wstring write_bytes_sec =
        SK_SizeToStringF (disk_stats.disks [i].write_bytes_sec, 6, 1);

      if (i == 0)
      {
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

      else
      {
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

  if (pagefile_stats.booting)
  {
    OSD_P_PRINTF "\n  Starting Pagefile Monitor...\n" OSD_END
  }

  else
  {
    for (DWORD i = 0; i < pagefile_stats.num_pagefiles; i++)
    {
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
  }

  // Avoid unnecessary MMIO when the user has the OSD turned off
  cleared = (! config.osd.show);

  BOOL ret = SK_UpdateOSD (szOSD);

  //game_debug.Log (L"%hs", szOSD);

  return ret;
}

BOOL
__stdcall
SK_UpdateOSD (LPCSTR lpText, LPVOID pMapAddr, LPCSTR lpAppName)
{
  UNREFERENCED_PARAMETER (pMapAddr);

  if (! InterlockedExchangeAdd (&osd_init, 0))
    return FALSE;

  if (lpAppName == nullptr)
    lpAppName = "Special K";

  try
  {
    SK_TextOverlay* pOverlay =
      SK_TextOverlayManager::getInstance ()->getTextOverlay (lpAppName);

#define IMPLICIT_CREATION
#ifdef IMPLICIT_CREATION
    if (pOverlay == nullptr) 
    {
      pOverlay =
        SK_TextOverlayManager::getInstance ()->createTextOverlay (lpAppName);
    }
#endif

    if (pOverlay != nullptr)
    {
      pOverlay->update (lpText);
      return TRUE;
    }
  }

  catch (...)
  {
  }

  return FALSE;
}

void
__stdcall
SK_ReleaseOSD (void)
{
  // Made obsolete by removal of RTSS ... may repurpose this in the future
  osd_shutting_down = true;
}


void
__stdcall
SK_SetOSDPos (int x, int y, LPCSTR lpAppName)
{
  if (lpAppName == nullptr)
    lpAppName = "Special K";

  // 0,0 means don't touch anything.
  if (x == 0 && y == 0)
    return;

  SK_TextOverlay* overlay =
    SK_TextOverlayManager::getInstance ()->getTextOverlay (lpAppName);

  if (overlay != nullptr)
  {
    auto fX = static_cast <float> (x);
    auto fY = static_cast <float> (y);

    overlay->setPos (fX, fY);
  }
}

void
__stdcall
SK_SetOSDColor ( int    red,
                 int    green,
                 int    blue,
                 LPCSTR lpAppName )
{
#if 0
  if (lpAppName == nullptr)
    lpAppName = "Special K";
#endif

  UNREFERENCED_PARAMETER (red);
  UNREFERENCED_PARAMETER (green);
  UNREFERENCED_PARAMETER (blue);
  UNREFERENCED_PARAMETER (lpAppName);
}

void
__stdcall 
SK_SetOSDScale ( float  fScale,
                 bool   relative,
                 LPCSTR lpAppName )
{
  if (lpAppName == nullptr)
    lpAppName = "Special K";

  SK_TextOverlayManager* overlay_mgr =
    SK_TextOverlayManager::getInstance ();

  SK_TextOverlay* overlay =
    overlay_mgr->getTextOverlay (lpAppName);

  if (overlay != nullptr)
  {
    if (relative)
      overlay->resize (fScale);
    else
      overlay->setScale (fScale);
  }

  // TEMP HACK
  // ---------
  //
  // If the primary overlay is rescaled, rescale everything else with it...
  if (overlay == overlay_mgr->getTextOverlay ("Special K"))
    SK_GetCommandProcessor ()->ProcessCommandFormatted (
      "OSD.Scale %f",
        overlay->getScale ()
    );
}

void
__stdcall 
SK_ResizeOSD (float scale_incr, LPCSTR lpAppName)
{
  if (lpAppName == nullptr)
    lpAppName = "Special K";

  SK_SetOSDScale (scale_incr, true, lpAppName);
}

void
SK_TextOverlay::reset (CEGUI::Renderer* pRenderer)
{
  font_.cegui = nullptr;
  geometry_   = nullptr;
  renderer_   = pRenderer;

  if (pRenderer != nullptr)
  {
    try
    {
      font_.cegui =
        &CEGUI::FontManager::getDllSingleton ().createFromFile (font_.name);

      if (font_.cegui != nullptr && pRenderer->getDisplaySize () != CEGUI::Sizef (0.0f, 0.0f))
        font_.cegui->setNativeResolution (pRenderer->getDisplaySize ());

      const CEGUI::Rectf scrn (
        CEGUI::Vector2f (0.0f, 0.0f),
          pRenderer->getDisplaySize ()
      );

      //dll_log.Log (L"%fx%f", pRenderer->getDisplaySize ().d_width, pRenderer->getDisplaySize ().d_height);

      geometry_ = &pRenderer->createGeometryBuffer (    );

      if (geometry_)
        geometry_->setClippingRegion               (scrn);
    }

    catch (...)
    {
    }
  }
}

// Not re-entrant, but that should not matter.
//
//  Original function courtesy of paxdiablo
char*
strtok_ex (char* str, char* seps)
{
  static char *tpos,
              *tkn,
              *pos = nullptr;
  static char savech;

  if (str != nullptr)
  {
    pos    = str;
    savech = 'x';
  }

  else
  {
    if (pos == nullptr)
      return nullptr;

    while (*pos != '\0')
      pos++;

    *pos++ = savech;
  }

  if (savech == '\0')
    return nullptr;

  tpos = pos;

  while (*tpos != '\0')
  {
    tkn = strchr (seps, *tpos);

    if (tkn != nullptr)
      break;

    tpos++;
  }

   savech = *tpos;
  *tpos   =  '\0';

  return pos;
}

auto SK_CountLines =
[](const char* line)
{
  int num_lines;

  for ( num_lines = 0;
          line [num_lines];
            line [num_lines] == '\n' ?  num_lines++ :
                                       *line++ );

  return num_lines;
};

void
__stdcall
SK_OSD_GetDefaultColor (float& r, float& g, float& b)
{
  r = (238.0f / 255.0f);
  g = (250.0f / 255.0f);
  b = (  5.0f / 255.0f);
}

float
SK_TextOverlay::update (const char* szText)
{
  size_t len = szText != nullptr ? strlen (szText) : 0;

  if (szText != nullptr) {
    *data_.text = '\0';
    strncat (data_.text, szText, std::min (len + 1, data_.text_len));
  }

  if (font_.cegui && geometry_)
  {
    // Empty the gometry buffer and bail-out.
    if (szText == nullptr || (! strlen (data_.text)))
    {
      geometry_->reset ();

      data_.extent = 0.0f;
      return data_.extent;
    }

    float baseline = 0.0f;
    float spacing  = font_.cegui->getLineSpacing () * font_.scale;

    float red   = static_cast <float> (config.osd.red)   / 255.0f;
    float green = static_cast <float> (config.osd.green) / 255.0f;
    float blue  = static_cast <float> (config.osd.blue)  / 255.0f;

    if (config.osd.red == -1 || config.osd.green == -1 || config.osd.blue == -1)
    {
      SK_OSD_GetDefaultColor (red, green, blue);
    }

    float   x, y;
    getPos (x, y);

    static __declspec (thread) char text [32768] = { };

    *text = '\0';
    strncat (text, data_.text, 32766);

    int   num_lines    = SK_CountLines (text);
    char* line         = strtok_ex     (text, "\n");

    bool  has_tokens   = (num_lines > 0);

    if (! has_tokens)
      line = text;

    float longest_line = 0.0f;

    // Compute the longest line so we can left-align text
    if (x < 0.0f)
    {
      while (line != nullptr)
      {
        // Fast-path: Skip blank lines
        if (*line != '\0')
        {
          float extent =
            font_.cegui->getTextExtent (line, font_.scale);

          longest_line = std::max (extent, longest_line);
        }

        if (has_tokens)
          line = strtok_ex (nullptr, "\n");
        else
          line = nullptr;
      }

      // Add 1.0 so that an X position of -1.0 is perfectly flush with the right
      x = renderer_->getDisplaySize ().d_width + x - longest_line + 1.0f;

      strncpy (text, data_.text, 32766);

      // Restart tokenizing
      if (! has_tokens)
        line = text;
      else
        line = strtok_ex (text, "\n");
    }

    CEGUI::System::getDllSingleton ().getDefaultGUIContext ().removeGeometryBuffer (CEGUI::RQ_UNDERLAY, *geometry_);
    geometry_->reset ();

    CEGUI::ColourRect foreground (
      CEGUI::Colour ( red,
                        green,
                          blue )
    );

    CEGUI::ColourRect shadow (
      CEGUI::Colour ( 0.0f, 0.0f, 0.0f )
    );

    if (y < 0.0f)
      y = renderer_->getDisplaySize ().d_height + y - (num_lines * spacing) + 1.0f;

    while (line != nullptr)
    {
      // Fast-path: Skip blank lines
      if (*line != '\0')
      {
        CEGUI::String cegui_line (line);

        try {
        // First the shadow
        //
        font_.cegui->drawText (
          *geometry_,
            cegui_line,
              CEGUI::Vector2f (x + 1.0f, y + baseline + 1.0f),
                nullptr,
                  shadow,
                    0.0f,
                      font_.scale,
                      font_.scale );

        // Then the foreground
        //
        font_.cegui->drawText (
          *geometry_,
            cegui_line,
              CEGUI::Vector2f (x, y + baseline),
                nullptr,
                  foreground,
                    0.0f,
                      font_.scale,
                      font_.scale );
        } catch (...) { };
      }

      baseline += spacing;

      if (has_tokens)
        line = strtok_ex (nullptr, "\n");
      else
        line = nullptr;
    }

    CEGUI::System::getDllSingleton ().getDefaultGUIContext ().addGeometryBuffer    (CEGUI::RQ_UNDERLAY, *geometry_);

    data_.extent = baseline;
    return data_.extent;
  }

  data_.extent = 0.0f;
  return data_.extent;
}

float
SK_TextOverlay::draw (float x, float y, bool full)
{
  if (geometry_)
  {
    geometry_->setTranslation (CEGUI::Vector3f (x, y, 0.0f));

    if (full)
    {
      geometry_->draw ();
      update          (nullptr);
    }

    return data_.extent;
  }

  return 0.0f;
}

void
SK_TextOverlay::setScale (float scale)
{
  if (scale >= 0.1f)
    font_.scale = scale;
  else
    font_.scale = 0.1f;
}

float
SK_TextOverlay::getScale (void)
{
  return font_.scale;
}

void
SK_TextOverlay::resize (float incr)
{
  setScale (getScale () + incr);
}

void
SK_TextOverlay::move (float  x_off, float  y_off)
{
  float x,y;

  getPos (x, y);
  setPos (x + x_off, y + y_off);
}

void
SK_TextOverlay::setPos (float x,float y)
{
  // We cannot anchor the command console to the left or bottom...
  if (! strcmp (data_.name, "SpecialK Console"))
  {
    x = std::min (0.0f, x);
    y = std::min (0.0f, y);
  }

  pos_.x = x;
  pos_.y = y;
}

void
SK_TextOverlay::getPos (float& x, float& y)
{
  x = pos_.x;
  y = pos_.y;
}


void
SK_TextOverlayManager::queueReset (CEGUI::Renderer* renderer)
{
  UNREFERENCED_PARAMETER (renderer);

  need_full_reset_ = true;
}

void
SK_TextOverlayManager::resetAllOverlays (CEGUI::Renderer* renderer)
{
  if (renderer != nullptr)
  {
    gui_ctx_ =
      &CEGUI::System::getDllSingleton ().getDefaultGUIContext ();
  }

  SK_AutoCriticalSection auto_crit (&cs_);

  auto it =
    overlays_.begin ();

  while (it != overlays_.end ())
  {
    if (renderer == nullptr)
    {
      it->second->pos_.x = static_cast <float> (config.osd.pos_x);
      it->second->pos_.y = static_cast <float> (config.osd.pos_y);

      it->second->font_.scale = config.osd.scale;

      it->second->font_.primary_color =
       ( ( (config.osd.red   << 16) & 0xff0000 ) | 
         ( (config.osd.green << 8)  & 0xff00   ) |
           (config.osd.blue         & 0xff     ) );

      it->second->font_.shadow_color = 0xff000000;
    }

    if (it->second->geometry_ != nullptr)
      gui_ctx_->removeGeometryBuffer (CEGUI::RQ_UNDERLAY, *it->second->geometry_);

    it->second->reset (renderer);

    if (it->second->geometry_ != nullptr)
    {
        it->second->update        (nullptr);
      gui_ctx_->addGeometryBuffer (CEGUI::RQ_UNDERLAY, *it->second->geometry_);
    }

    ++it;
  }

  if (renderer == nullptr)
  {
    it =
      overlays_.begin ();

    while (it != overlays_.end ())
    {
      if (it->second->geometry_ != nullptr)
        gui_ctx_->removeGeometryBuffer (CEGUI::RQ_UNDERLAY, *it->second->geometry_);

      it->second->reset (renderer);

      if (it->second->geometry_ != nullptr)
      {
          it->second->update        (nullptr);
        gui_ctx_->addGeometryBuffer (CEGUI::RQ_UNDERLAY, *it->second->geometry_);
      }

      ++it;
    }
  }
}

float
SK_TextOverlayManager::drawAllOverlays (float x, float y, bool full)
{
  // This is a terrible design, but I don't care.
  extern void
  SK_CEGUI_QueueResetD3D11 (void);

  extern void
  SK_CEGUI_QueueResetD3D9  (void);

  SK_AutoCriticalSection auto_crit (&cs_);

  float base_y = y;

  // Draw top-down
  if (config.osd.pos_y >= 0)
  {
    auto it =
      overlays_.begin ();

    // Handle situations where the OSD font is not yet loaded
    if ( it                      == overlays_.end () ||
         it->second              == nullptr          ||
         it->second->font_.cegui == nullptr )
    {
      SK_CEGUI_QueueResetD3D11 ();
      SK_CEGUI_QueueResetD3D9  ();
      return fabs (y - base_y);
    }

    while (it != overlays_.end ())
    {
      y += it->second->draw (x, y, full);

      ++it;
    }
  }

  // Draw from the bottom up
  else
  {
    auto it =
      overlays_.rbegin ();

    // Handle situations where the OSD font is not yet loaded
    if ( it                      == overlays_.rend () ||
         it->second              == nullptr           ||
         it->second->font_.cegui == nullptr )
    {
      SK_CEGUI_QueueResetD3D11 ();
      SK_CEGUI_QueueResetD3D9  ();
      return fabs (y - base_y);
    }

    // Push the starting position up one whole line
    y -= it->second->font_.cegui->getFontHeight (config.osd.scale);

    while (it != overlays_.rend ())
    {
      y -= it->second->draw (x, y, full);

      ++it;
    }
  }

  // Height of all rendered overlays
  return fabs (y - base_y);
}

void
SK_TextOverlayManager::destroyAllOverlays (void)
{
  SK_AutoCriticalSection auto_crit (&cs_);

  auto it =
    overlays_.begin ();

  while (it != overlays_.end ())
  {
    delete (it++)->second;
  }

  overlays_.clear ();
}

bool
SK_TextOverlayManager::OnVarChange (SK_IVariable* var, void* val)
{
  if (var == pos_.x || var == pos_.y || var == scale_)
  {
    if (var == pos_.x)
      config.osd.pos_x = *static_cast <signed int *> (val);

    else if (var == pos_.y)
      config.osd.pos_y = *static_cast <signed int *> (val);

    else if (var == scale_)
      config.osd.scale = *static_cast <float *> (val);


    SK_AutoCriticalSection auto_crit (&cs_);

      auto  it =  overlays_.begin ();
    while ( it != overlays_.end   () )
    {
      auto pos_x = static_cast <float> (config.osd.pos_x);
      auto pos_y = static_cast <float> (config.osd.pos_y);

      if (var == pos_.x || var == pos_.y)
        it->second->setPos (pos_x, pos_y);

      else if (var == scale_)
        it->second->setScale (config.osd.scale);

      ++it;
    }

    return true;
  }

  return false;
}