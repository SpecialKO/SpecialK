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

#define OSD_IMP
#include <SpecialK/osd/text.h>

#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>

#include <SpecialK/nvapi.h>


SK_TextOverlayManager*
SK_TextOverlayManager::getInstance (void)
{
  SK_RunOnce (
    pSelf =
      std::make_unique <SK_TextOverlayManager> ()
  );

  return
    pSelf.get ();
}

std::unique_ptr <SK_TextOverlayManager>    SK_TextOverlayManager::pSelf = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> SK_TextOverlayManager::lock_ = nullptr;

SK_TextOverlayManager::SK_TextOverlayManager (void)
{
  lock_ =
    std::make_unique <SK_Thread_HybridSpinlock> (104858);

  need_full_reset_ = false;

  SK_ICommandProcessor* cmd =
    SK_GetCommandProcessor ();

  if (cmd != nullptr)
  {
    cmd->AddVariable ("OSD.Red",   SK_CreateVar (SK_IVariable::Int, &config.osd.red));
    cmd->AddVariable ("OSD.Green", SK_CreateVar (SK_IVariable::Int, &config.osd.green));
    cmd->AddVariable ("OSD.Blue",  SK_CreateVar (SK_IVariable::Int, &config.osd.blue));

    pos_.x = SK_CreateVar (SK_IVariable::Int,   &config.osd.pos_x, this);
    pos_.y = SK_CreateVar (SK_IVariable::Int,   &config.osd.pos_y, this);
    scale_ = SK_CreateVar (SK_IVariable::Float, &config.osd.scale, this);

    cmd->AddVariable ("OSD.PosX",  pos_.x);
    cmd->AddVariable ("OSD.PosY",  pos_.y);
    cmd->AddVariable ("OSD.Scale", scale_);

    osd_.show      = SK_CreateVar (SK_IVariable::Boolean, &config.osd.show);
    fps_.show      = SK_CreateVar (SK_IVariable::Boolean, &config.fps.show);
    gpu_.show      = SK_CreateVar (SK_IVariable::Boolean, &config.gpu.show);
    cpu_.show      = SK_CreateVar (SK_IVariable::Boolean, &config.cpu.show);
    mem_.show      = SK_CreateVar (SK_IVariable::Boolean, &config.mem.show);
    disk_.show     = SK_CreateVar (SK_IVariable::Boolean, &config.disk.show);
    io_.show       = SK_CreateVar (SK_IVariable::Boolean, &config.io.show);
    pagefile_.show = SK_CreateVar (SK_IVariable::Boolean, &config.pagefile.show);
    sli_.show      = SK_CreateVar (SK_IVariable::Boolean, &config.sli.show);

    cmd->AddVariable ("OSD.Show",                osd_.show);

    cmd->AddVariable ("OSD.FPS.Show",            fps_.show);
    cmd->AddVariable ("OSD.Shaders.Show",        SK_CreateVar (SK_IVariable::Boolean, &config.render.show));

    cmd->AddVariable ("OSD.Memory.Show",         mem_.show);
    cmd->AddVariable ("OSD.Memory.UpdateFreq",   SK_CreateVar (SK_IVariable::Float,   &config.mem.interval));

    cmd->AddVariable ("OSD.SLI.Show",            sli_.show);

    cmd->AddVariable ("OSD.CPU.Show",            cpu_.show);
    cmd->AddVariable ("OSD.CPU.Simple",          SK_CreateVar (SK_IVariable::Boolean, &config.cpu.simple));
    cmd->AddVariable ("OSD.CPU.UpdateFreq",      SK_CreateVar (SK_IVariable::Float,   &config.cpu.interval));

    cmd->AddVariable ("OSD.GPU.Show",            gpu_.show);
    cmd->AddVariable ("OSD.GPU.PrintSlowdown",   SK_CreateVar (SK_IVariable::Boolean, &config.gpu.print_slowdown));
    cmd->AddVariable ("OSD.GPU.UpdateFreq",      SK_CreateVar (SK_IVariable::Float,   &config.gpu.interval));

    cmd->AddVariable ("OSD.Disk.Show",           disk_.show);
    cmd->AddVariable ("OSD.Disk.Type",           SK_CreateVar (SK_IVariable::Int,     &config.disk.type));
    cmd->AddVariable ("OSD.Disk.UpdateFreq",     SK_CreateVar (SK_IVariable::Float,   &config.disk.interval));

    cmd->AddVariable ("OSD.Pagefile.Show",       pagefile_.show);
    cmd->AddVariable ("OSD.Pagefile.UpdateFreq", SK_CreateVar (SK_IVariable::Float,   &config.pagefile.interval));

    cmd->AddVariable ("OSD.IOPS.Show",           io_.show);
    cmd->AddVariable ("OSD.IOPS.UpdateFreq",     SK_CreateVar (SK_IVariable::Float,   &config.io.interval));
  }
}


SK_TextOverlay*
SK_TextOverlayManager::createTextOverlay (const char *szAppName)
{
  std::string app_name (szAppName);

  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
          scope_lock (*lock_);

    if (overlays_.count (app_name))
    {
      SK_TextOverlay* overlay =
        overlays_ [app_name];

      return overlay;
    }
  }

  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*lock_);


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
  const std::string app_name (szAppName);

  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*lock_);

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
  const std::string app_name (szAppName);

  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*lock_);

  if (overlays_.count (app_name))
  {
    SK_TextOverlay* overlay =
      overlays_ [app_name];

    return overlay;
  }

  return nullptr;
}

SK_TextOverlay::SK_TextOverlay (const char* szAppName)
{
  strncpy (data_.name, szAppName, 32);
  strncpy (font_.name, "Consola", 64);

  data_.text_capacity = 4096;
  data_.text          = (char *)calloc (1, data_.text_capacity);

  if (data_.text != nullptr)
  {
    *data_.text     = '\0';
     data_.text_len = 0;
  }

  else
  {
    data_.text_capacity = 0; // Failed to allocate memory
    data_.text_len      = 0;
  }

  font_.scale = 1.0f;
}

SK_TextOverlay::~SK_TextOverlay (void)
{
  SK_TextOverlayManager::getInstance ()->
                   removeTextOverlay (data_.name);

  std::free (data_.text);

  data_.text_capacity = 0;
  data_.text_len      = 0;

  data_.text          = nullptr;
}


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
#define OSD_F_PRINTF if (config.osd.show &&\
                         config.fps.show &&\
                       ! config.fps.compact)  { pszOSD += sprintf (pszOSD,
#define OSD_X_PRINTF if((config.osd.show &&\
                         config.fps.show &&\
                       ! config.fps.compact) ||\
                        (config.osd.show &&\
                         config.gpu.show)  )  { pszOSD += sprintf (pszOSD,
#define OSD_END    ); }

char szOSD [32768] = { };

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

std::wstring_view
SK_File_SizeToString (uint64_t size, SK_UNITS unit, SK_TLS* pTLS)
{
  if (unit == Auto)
  {
    if      (size > (1ULL << 32ULL)) unit = GiB;
    else if (size > (1ULL << 22ULL)) unit = MiB;
    else if (size > (1ULL << 12ULL)) unit = KiB;
    else                             unit = B;
  }

  if (pTLS == nullptr)
      pTLS = SK_TLS_Bottom ();
  if (pTLS == nullptr)
    return L"";

  std::wstring_view               pwszString (
    pTLS->scratch_memory->osd.wszFileSize.alloc (66, false), 64
  );

  size_t len = 0;

  switch (unit)
  {
    case GiB:
      len = SK_FormatStringViewW (pwszString, L"%#5llu GiB", size >> 30);
      break;
    case MiB:
      len = SK_FormatStringViewW (pwszString, L"%#5llu MiB", size >> 20);
      break;
    case KiB:
      len = SK_FormatStringViewW (pwszString, L"%#5llu KiB", size >> 10);
      break;
    case B:
    default:
      len = SK_FormatStringViewW (pwszString, L"%#3llu Bytes", size);
      break;
  }
                      ((wchar_t *)pwszString.data ())[std::min ((size_t)64, len)] = L'\0';
  return                          pwszString;
}

std::string_view
SK_File_SizeToStringA (uint64_t size, SK_UNITS unit, SK_TLS* pTLS)
{
  if (unit == Auto)
  {
    if      (size > (1ULL << 32ULL)) unit = GiB;
    else if (size > (1ULL << 22ULL)) unit = MiB;
    else if (size > (1ULL << 12ULL)) unit = KiB;
    else                             unit = B;
  }

  if (pTLS == nullptr)
      pTLS = SK_TLS_Bottom ();
  if (pTLS == nullptr)
    return "";

  std::string_view               pszString (
    pTLS->scratch_memory->osd.szFileSize.alloc (66, false), 64
  );

  size_t len = 0;

  switch (unit)
  {
    case GiB:
      len = SK_FormatStringView (pszString, "%#5llu GiB", size >> 30);
      break;
    case MiB:
      len = SK_FormatStringView (pszString, "%#5llu MiB", size >> 20);
      break;
    case KiB:
      len = SK_FormatStringView (pszString, "%#5llu KiB", size >> 10);
      break;
    case B:
    default:
      len = SK_FormatStringView (pszString, "%#3llu Bytes", size);
      break;
  }

                        ((char *)pszString.data ())[std::min ((size_t)64, len)] = '\0';
  return                         pszString;
}

std::string_view
SK_File_SizeToStringAF (uint64_t size, int width, int precision, SK_UNITS unit, SK_TLS* pTLS)
{
  if (unit == Auto)
  {
    if      (size > (1ULL << 32ULL)) unit = GiB;
    else if (size > (1ULL << 22ULL)) unit = MiB;
    else if (size > (1ULL << 12ULL)) unit = KiB;
    else                             unit = B;
  }

  if (pTLS == nullptr)
      pTLS = SK_TLS_Bottom ();
  if (pTLS == nullptr)
    return "";

  std::string_view       pszStr (
    pTLS->scratch_memory->osd.szFileSize.alloc (66, false), 64
  );

  size_t len = 0;

  switch (unit)
  {
  case GiB:
    len =
    SK_FormatStringView (pszStr, "%#*.*f GiB", width, precision,
                                     (float)size / (1024.0f * 1024.0f * 1024.0f));
    break;
  case MiB:
    len =
    SK_FormatStringView (pszStr, "%#*.*f MiB", width, precision,
                                     (float)size / (1024.0f * 1024.0f));
    break;
  case KiB:
    len =
    SK_FormatStringView (pszStr, "%#*.*f KiB", width, precision, (float)size / 1024.0f);
    break;
  case B:
  default:
    len =
    SK_FormatStringView (pszStr, "%#*llu Bytes", width-1-precision, size);
    break;
  }
                ((char *)pszStr.data ())[std::min ((size_t)64, len)] = '\0';
  return                 pszStr;
}

std::wstring_view
SK_File_SizeToStringF (uint64_t size, int width, int precision, SK_UNITS unit, SK_TLS* pTLS)
{
  if (unit == Auto)
  {
    if      (size > (1ULL << 32ULL)) unit = GiB;
    else if (size > (1ULL << 22ULL)) unit = MiB;
    else if (size > (1ULL << 12ULL)) unit = KiB;
    else                             unit = B;
  }

  if (pTLS == nullptr)
      pTLS = SK_TLS_Bottom ();
  if (pTLS == nullptr)
    return L"";

  std::wstring_view       pwszStr (
    pTLS->scratch_memory->osd.wszFileSize.alloc (66, false), 64
  );

  size_t len = 0;

  switch (unit)
  {
  case GiB:
    len =
    SK_FormatStringViewW (pwszStr, L"%#*.*f GiB", width, precision,
                                           (float)size / (1024.0f * 1024.0f * 1024.0f));
    break;
  case MiB:
    len =
    SK_FormatStringViewW (pwszStr, L"%#*.*f MiB", width, precision,
                                           (float)size / (1024.0f * 1024.0f));
    break;
  case KiB:
    len =
    SK_FormatStringViewW (pwszStr, L"%#*.*f KiB", width, precision, (float)size / 1024.0f);
    break;
  case B:
  default:
    len =
    SK_FormatStringViewW (pwszStr, L"%#*llu Bytes", width-1-precision, size);
    break;
  }
              ((wchar_t *)pwszStr.data ())[std::min ((size_t)64, len)] = L'\0';
  return                  pwszStr;
}

std::wstring_view
SK_FormatTemperature (int32_t in_temp, SK_UNITS in_unit, SK_UNITS out_unit, SK_TLS* pTLS = nullptr)
{
  int32_t converted;

  if (pTLS == nullptr)
      pTLS = SK_TLS_Bottom ();
  if (pTLS == nullptr)
    return L"";

  std::wstring_view         pwszStr (
    pTLS->scratch_memory->osd.wszTemperature.alloc (18, false), 16
  );

  size_t len = 0;

  if (in_unit == Celsius && out_unit == Fahrenheit)
  {
    //converted = in_temp * 2 + 30;
    converted = (int32_t)((float)(in_temp) * 9.0f/5.0f + 32.0f);
    len       =
      SK_FormatStringViewW (pwszStr, L"%#3liF", converted);
  }

  else if (in_unit == Fahrenheit && out_unit == Celsius)
  {
    converted = (int32_t)(((float)in_temp - 32.0f) * (5.0f/9.0f));
    len       =
      SK_FormatStringViewW (pwszStr, L"%#2liC", converted);
  }

  else
  {
    len =
      SK_FormatStringViewW (pwszStr, L"%#2liC", in_temp);
  }
                ((wchar_t *)pwszStr.data ())[std::min ((size_t)16, len)] = L'\0';
  return                    pwszStr;
}

std::string_view
SK_FormatTemperature (double in_temp, SK_UNITS in_unit, SK_UNITS out_unit, SK_TLS* pTLS = nullptr)
{
  double converted;

  if (pTLS == nullptr)
      pTLS = SK_TLS_Bottom ();
  if (pTLS == nullptr)
    return "";

  std::string_view         pszStr (
    pTLS->scratch_memory->osd.szTemperature.alloc (18, false), 16
  );

  size_t len = 0;

  if (in_unit == Celsius && out_unit == Fahrenheit)
  {
    //converted = in_temp * 2 + 30;
    converted = (in_temp * (9.0/5.0)) + 32.0;
    len       =
      SK_FormatStringView (pszStr, (const char *)u8"%#5.1f°F", converted);
  }

  else if (in_unit == Fahrenheit && out_unit == Celsius)
  {
    converted = (in_temp - 32.0) * (5.0/9.0);
    len       =
      SK_FormatStringView (pszStr, (const char *)u8"%#4.1f°C", converted);
  }

  else
  {
    len =
      SK_FormatStringView (pszStr, (const char *)u8"%#4.1f°C", in_temp);
  }
                  ((char *)pszStr.data ())[std::min ((size_t)16, len)] = '\0';
  return                   pszStr;
}

SK_LazyGlobal <std::string> external_osd_name;

BOOL
__stdcall
SK_DrawExternalOSD (std::string osd_app_name, std::string text)
{
  external_osd_name.get () =
           osd_app_name;

  SK_UpdateOSD ( text.c_str (), nullptr,
         osd_app_name.c_str () );

  return TRUE;
}

BOOL
__stdcall
SKX_DrawExternalOSD (const char* szAppName, const char* szText)
{
  external_osd_name.get () =
          szAppName;

  SK_UpdateOSD ( szText, nullptr,
                 szAppName );

  return TRUE;
}


void
__stdcall
SK_InstallOSD (void)
{
  if (InterlockedCompareExchange (&osd_init, TRUE, FALSE) == FALSE)
  {
    SK_TextOverlayManager::getInstance ()->createTextOverlay ("Special K");

    SK_SetOSDScale (config.osd.scale);
    SK_SetOSDPos   (config.osd.pos_x, config.osd.pos_y);
    SK_SetOSDColor (config.osd.red,   config.osd.green, config.osd.blue);
    ImGui::GetIO ().FontGlobalScale = SK_ImGui::SanitizeFontGlobalScale (
      config.imgui.scale);
  }
}


BOOL
__stdcall
SK_DrawOSD (void)
{
  static bool cleared = false;

  if (ReadAcquire (&osd_init) == FALSE)
    SK_InstallOSD ();

  if (! config.osd.show)
    return FALSE;

#if 0
  // Automatically free VRAM cache when it is a bit on the excessive side
  if ((process_stats.memory.page_file_bytes >> 30ULL) > 28) {
    SK_GetCommandProcessor ()->ProcessCommandLine ("VRAM.Purge 9999");
  }

  if ((process_stats.memory.page_file_bytes >> 30ULL) < 12) {
    SK_GetCommandProcessor ()->ProcessCommandLine ("VRAM.Purge 0");
  }
#endif

  ////if ((! config.osd.show) && cleared)
  ////  return TRUE;

  if (ReadAcquire (&osd_init) == FALSE)
    return FALSE;

  SK_TLS *pTLS =
        SK_TLS_Bottom ();

  ImGui::SetNextWindowSize (ImGui::GetIO ().DisplaySize, ImGuiCond_Always);
  ImGui::SetNextWindowPos  (ImVec2 (0.0f, 0.0f),         ImGuiCond_Always);

  ImGui::Begin ("###OSD", nullptr,   ImGuiWindowFlags_NoTitleBar    | ImGuiWindowFlags_NoResize           | ImGuiWindowFlags_NoMove                | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus        | ImGuiWindowFlags_NoNav      | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs );

  char* pszOSD = szOSD;
       *pszOSD = '\0';

  int         left_padding =  0;
  const char *pad_str      = "";

  if (config.title.show || config.time.show)
      left_padding = 2;

  static io_perf_t
    io_counter;

  buffer_t buffer = dxgi_mem_info [     0].buffer;
  int      nodes  = dxgi_mem_info [buffer].nodes;

  if (config.title.show)
  {
    static HMODULE hModGame = skModuleRegistry::HostApp ();
    static wchar_t wszGameName [MAX_PATH + 2] = { };

    if (wszGameName [0] == L'\0')
    {
      GetModuleFileName (hModGame, wszGameName, MAX_PATH);

      if (     StrStrIW (wszGameName, L"BatmanAK.exe"))
        plugin_mgr->isArkhamKnight    = true;
      else if (StrStrIW (wszGameName, L"Tales of Zestiria.exe"))
        plugin_mgr->isTalesOfZestiria = true;
      else if (StrStrIW (wszGameName, L"NieRAutomata"))
        plugin_mgr->isNieRAutomata    = true;
      else if (StrStrIW (wszGameName, L"DarkSoulsIII.exe"))
        plugin_mgr->isDarkSouls3      = true;
    }

    if (plugin_mgr->isArkhamKnight)
    {
      OSD_PRINTF "Batman \"Fix\" v 0.20"
      OSD_END
    }

    else if (plugin_mgr->isNieRAutomata ||
             plugin_mgr->isDarkSouls3   ||
             SK_HasPlugin ())
    {
      OSD_PRINTF "%ws", SK_GetPluginName ().c_str ()
      OSD_END
    }

    else
    {
      OSD_PRINTF "Special K v %ws", SK_GetVersionStr ()
      OSD_END
    }
  }

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
                              127 );

    OSD_PRINTF ((config.title.show) ? "   %ws" : "%ws"), time
    OSD_END
  }

  bool bTitleOrClock =
    (config.title.show || config.time.show);

  if (bTitleOrClock && (! (config.fps.show || config.fps.framenumber) || ! config.fps.compact))
  {
    OSD_PRINTF "\n\n" OSD_END
  }

  auto _DrawFrameCountIf = [&](bool predicate = true)
  {
    if (! (config.fps.framenumber && predicate))
      return;

    const auto frame_count =
      SK_GetFramesDrawn ();
    
    const int padding =
      config.fps.show
       ? ( config.fps.compact ? left_padding
                              : 0 )
       :                        left_padding;
    
    const char* szFormat =
      config.fps.compact 
       ? ( config.fps.show ? "%*hs%llu%*hs"
                           : "%*hs%llu\n\n" )
       : ( config.fps.show ? "%*hs  (Frame: %llu)"
                           : "%*hs%llu\n\n" );
    
    OSD_PRINTF (szFormat), padding, pad_str, frame_count,
             bTitleOrClock ? 0 : 2, pad_str
    OSD_END
  };

  _DrawFrameCountIf (config.fps.compact);

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  // Delay this a few frames so we do not create multiple framerate limiters
  //
  //  * Each SwapChain gets one, and some games have temporary init-only SwapChains...
  //
  auto *pLimiter = config.fps.show ?
    SK::Framerate::GetLimiter (
      rb.swapchain.p, false   )    : nullptr;

  if (pLimiter != nullptr)
  {
    auto &history =
      pLimiter->frame_history_snapshots->frame_history,
         &history2 =
      pLimiter->frame_history_snapshots->frame_history2;

    const double mean    = history.calcMean     ();
    const double sd      = history.calcSqStdDev (mean);
    const double min     = history.calcMin      ();
    const double max     = history.calcMax      ();
    const int    hitches = history.calcHitches  (1.2, mean);

    const double effective_mean = history2.calcMean ();

    static double fps           = 0.0;
    static DWORD  last_fps_time = SK_timeGetTime ();
    const  DWORD         dwTime = SK_timeGetTime ();

    if (dwTime - last_fps_time > 666)
    {
      fps           = sk::narrow_cast <double>(1000.0 / mean);
      last_fps_time = dwTime;
    }

    const bool gsync =
     ( sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status &&
       rb.gsync_state.capable &&
       rb.gsync_state.active  );

    if (fabs (mean - INFINITY) > std::numeric_limits <double>::epsilon ())
    {
      const char* format = "";

      const bool has_cpu_frametime =
             pLimiter->get_limit () > 0.0 &&
        (! plugin_mgr->isTalesOfZestiria) &&
             history2.calcNumSamples () > 0;

      if (config.fps.compact)
      {
        OSD_PRINTF ("%*hs%2.0f\n"), left_padding, pad_str, fps
        OSD_END
      }

      else {
        if (! gsync)
        {
          format =
            ( config.fps.frametime  ?
                config.fps.advanced ?
                  has_cpu_frametime ?
                    "%*hs%-7ws:  %#4.01f FPS, %#13.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)   <%4.01f FPS / %3.2f ms>" :
                    "%*hs%-7ws:  %#4.01f FPS, %#13.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)"                           :
                    "%*hs%-7ws:  %#4.01f FPS, %#13.01f ms"                                                                    :
                    "%*hs%-7ws:  %#4.01f FPS"
            );
        }

        else
        {
          format =
            ( config.fps.frametime ?
              config.fps.advanced  ?
                has_cpu_frametime  ?
                  "%*hs%-7ws:  %#4.01f FPS (G-Sync),%#5.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)   <%4.01f FPS / %3.2f ms>" :
                  "%*hs%-7ws:  %#4.01f FPS (G-Sync),%#5.01f ms (s=%3.2f,min=%3.2f,max=%3.2f,hitches=%d)"                           :
                  "%*hs%-7ws:  %#4.01f FPS (G-Sync),%#5.01f ms"                                                                    :
                  "%*hs%-7ws:  %#4.01f FPS (G-Sync)"
            );
        }

        if (has_cpu_frametime)
        {
          OSD_PRINTF format, left_padding, pad_str,
            rb.name,
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

        else
        {
          OSD_PRINTF format, left_padding, pad_str,
            rb.name,
              fps,
                mean,
                  sqrt (sd),
                    min,
                      max,
                        hitches
          OSD_END
        }
      }
    }

    // No Frametime History
    else if (! config.fps.compact)
    {
      const char* format =
        gsync ?
          ( config.fps.frametime                           ?
              "%*hs%-7ws:  %#4.01f FPS (G-Sync),%5.01f ms" :
              "%*hs%-7ws:  %#4.01f FPS (G-Sync)"             )
              :
          ( config.fps.frametime                     ?
              "%*hs%-7ws:  %#4.01f FPS, %#13.01f ms" :
              "%*hs%-7ws:  %#4.01f FPS"                );

      OSD_PRINTF format, left_padding, pad_str,
        rb.name,
          // Cast to FP to avoid integer division by zero.
          1000.0f * 0.0f / 1.0f, 0.0f
      OSD_END
    }

    _DrawFrameCountIf (! config.fps.compact);

    OSD_PRINTF "\n" OSD_END
  }

  _DrawFrameCountIf (! (config.fps.show || config.fps.compact));

  // Poll GPU stats...
  if (config.gpu.show)
    SK_PollGPU ();

  gpu_sensors_t* gpu_stats =
    SK_GPU_CurrentSensorData ();

  int afr_idx  = (gpu_stats != nullptr && gpu_stats->num_gpus > 1) ? SK_NV_sli_state->currentAFRIndex       : 0,
      afr_last = (gpu_stats != nullptr && gpu_stats->num_gpus > 1) ? SK_NV_sli_state->previousFrameAFRIndex : 0,
      afr_next = (gpu_stats != nullptr && gpu_stats->num_gpus > 1) ? SK_NV_sli_state->nextFrameAFRIndex     : 0;

  if (gpu_stats != nullptr)
  {
    for (int i = 0; i < gpu_stats->num_gpus; i++)
    {
      OSD_G_PRINTF "%*hsGPU%i   :            %#3u%%", left_padding, pad_str,
        i, gpu_stats->gpus [i].loads_percent.gpu / 1000
      OSD_END

      if (nvapi_init && gpu_stats->gpus [i].loads_percent.vid > 0)
      {
        // Vector 3D (subtract 1 space)
        //OSD_G_PRINTF ",  VID%i %#3lu%% ,",

        // Raster 3D
        OSD_G_PRINTF ",  VID%i %#3u%%  ,",
          i, gpu_stats->gpus [i].loads_percent.vid / 1000
        OSD_END
      } else {
        // Vector 3D (subtract 1 space)
        //OSD_G_PRINTF ",             " OSD_END

        // Raster 3D
        OSD_G_PRINTF ",              " OSD_END
      }

      if (gpu_stats->gpus [i].clocks_kHz.gpu >= 1000)
      {
        OSD_G_PRINTF " %#4u MHz",
              gpu_stats->gpus [i].clocks_kHz.gpu / 1000UL
        OSD_END
      }

      if (gpu_stats->gpus [i].volts_mV.supported)
      {
        // Over (or under) voltage limit!
        if (false)//gpu_stats->gpus [i].volts_mV.over)
        {
          OSD_G_PRINTF ", %#4.3fV (%+#4.3fV)",
            gpu_stats->gpus [i].volts_mV.core / 1000.0, gpu_stats->gpus [i].volts_mV.ov / 1000.0
          OSD_END
        }

        else
        {
          OSD_G_PRINTF ", %#4.3fV",
            gpu_stats->gpus [i].volts_mV.core / 1000.0
          OSD_END
        }
      }

      else
      {
        //// Padding because no voltage reading is available
        ///////OSD_G_PRINTF ",         "
        ///////OSD_END
      }

      if (gpu_stats->gpus [i].fans_rpm.supported && gpu_stats->gpus [i].fans_rpm.gpu > 0)
      {
        OSD_G_PRINTF ", %#4u RPM",
          gpu_stats->gpus [i].fans_rpm.gpu
        OSD_END
      }

      else
      {
        //// Padding because no RPM reading is available
        ///////OSD_G_PRINTF ",         "
        ///////OSD_END
      }

      if (gpu_stats->gpus [i].temps_c.gpu > 0.0f)
      {
        static std::string temp ("", 16);

        temp.assign (
          SK_FormatTemperature (
            (double)gpu_stats->gpus [i].temps_c.gpu,
              Celsius,
                config.system.prefer_fahrenheit ? Fahrenheit :
                                                  Celsius,     pTLS ).data () );

        OSD_G_PRINTF ", (%hs)",
          temp.c_str ()
        OSD_END
      }

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
          gpu_stats->gpus [i].nv_perf_state != NV_GPU_PERF_DECREASE_NONE) {
        OSD_G_PRINTF "   SLOWDOWN:" OSD_END

        if (gpu_stats->gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_AC_BATT)
          OSD_G_PRINTF " (Battery)" OSD_END
        if (gpu_stats->gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_API_TRIGGERED)
          OSD_G_PRINTF " (Driver)" OSD_END
        if (gpu_stats->gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_INSUFFICIENT_POWER)
          OSD_G_PRINTF " (Power Supply)" OSD_END
        if (gpu_stats->gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_POWER_CONTROL)
          OSD_G_PRINTF " (Power Limit)" OSD_END
        if (gpu_stats->gpus [i].nv_perf_state & NV_GPU_PERF_DECREASE_REASON_THERMAL_PROTECTION)
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
          OSD_G_PRINTF "%*hsVRAM%i  : %#5llu MiB (%#3u%%: %#5.01lf GiB/s)", left_padding, pad_str,
            i,
                                 dxgi_mem_info [buffer].local    [i].CurrentUsage            >>   20ULL,
                                                 gpu_stats->gpus [i].loads_percent.fb / 1000,
  static_cast <double> ( static_cast <uint64_t> (gpu_stats->gpus [i].clocks_kHz.ram) * 2ULL   * 1000ULL  *
                         static_cast <uint64_t> (gpu_stats->gpus [i].hwinfo.mem_bus_width) )  /     8.0  /
                                                                             (1024.0 * 1024.0 * 1024.0) *
  static_cast <double> (                         gpu_stats->gpus [i].loads_percent.fb / 1000) /   100.0
          OSD_END
        }

        else
        {
          OSD_G_PRINTF "%*hsVRAM%i  : %#5llu MiB", left_padding, pad_str,
            i, dxgi_mem_info [buffer].local [i].CurrentUsage >> 20ULL
          OSD_END
        }

        if (gpu_stats->gpus [i].clocks_kHz.ram >= 1000)
        {
          OSD_G_PRINTF ", %#4lu MHz",
            gpu_stats->gpus [i].clocks_kHz.ram / 1000UL
          OSD_END
        }

        // Add memory temperature if it exists
        if (i <= gpu_stats->num_gpus && gpu_stats->gpus [i].temps_c.ram != 0)
        {
          static std::string temp ("", 16);

          temp.assign (
            SK_FormatTemperature (
              (double)gpu_stats->gpus [i].temps_c.ram,
                Celsius,
                  config.system.prefer_fahrenheit ? Fahrenheit :
                                                    Celsius,     pTLS ).data ()
            );

          OSD_G_PRINTF ", (%hs)",
            temp.data ()
          OSD_END
        }

        OSD_G_PRINTF "\n" OSD_END
      }

      for (int i = 0; i < nodes; i++)
      {
        // Figure out the generation from the transfer rate...
        int pcie_gen = gpu_stats->gpus [i].hwinfo.pcie_gen;

        if (nvapi_init)
        {
          OSD_G_PRINTF "%*hsSHARE%i : %#5llu MiB (%#3u%%: %#5.02lf GiB/s), PCIe %i.0x%u\n", left_padding, pad_str,
            i,
        dxgi_mem_info [buffer].nonlocal [i].CurrentUsage               >>  20ULL,
                         gpu_stats->gpus [i].loads_percent.bus / 1000,
                         gpu_stats->gpus [i].hwinfo.pcie_bandwidth_mb () / 1024.0 *
   static_cast <double> (gpu_stats->gpus [i].loads_percent.bus / 1000)   /  100.0,
                         pcie_gen,
                         gpu_stats->gpus [i].hwinfo.pcie_lanes
                         //gpu_stats->gpus [i].hwinfo.pcie_transfer_rate
          OSD_END
        }

        else if (pcie_gen > 0 && gpu_stats->gpus [i].hwinfo.pcie_lanes > 0)
        {
          OSD_G_PRINTF "%*hsSHARE%i : %#5llu MiB, PCIe %i.0x%lu\n", left_padding, pad_str,
            i,
       dxgi_mem_info [buffer].nonlocal [i].CurrentUsage >> 20ULL,
            pcie_gen,
            gpu_stats->gpus [i].hwinfo.pcie_lanes
          OSD_END
        }

        else
        {
          OSD_G_PRINTF "%*hsSHARE%i : %#5llu MiB      \n", left_padding, pad_str,
            i,
       dxgi_mem_info [buffer].nonlocal [i].CurrentUsage >> 20ULL
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
      for (int i = 0; i < gpu_stats->num_gpus; i++)
      {
        if (nvapi_init)
        {
          OSD_G_PRINTF "%*hsVRAM%i  : %#5llu MiB (%#3u%%: %#5.01lf GiB/s)", left_padding, pad_str,
            i,
       dxgi_mem_info [buffer].local     [i].CurrentUsage >> 20ULL,
                        gpu_stats->gpus [i].loads_percent.fb / 1000,
  static_cast <double> ( static_cast <uint64_t> (gpu_stats->gpus [i].clocks_kHz.ram) * 2ULL   * 1000ULL  *
                         static_cast <uint64_t> (gpu_stats->gpus [i].hwinfo.mem_bus_width) )  /     8.0  /
                                                                              (1024.0 * 1024.0 * 1024.0) *
  static_cast <double> (                         gpu_stats->gpus [i].loads_percent.fb /1000)  /   100.0
          OSD_END
        }

        else
        {
          OSD_G_PRINTF "%*hsVRAM%i  : %#5llu MiB", left_padding, pad_str,
            i, gpu_stats->gpus [i].memory_B.local >> 20ULL
          OSD_END
        }

        if (gpu_stats->gpus [i].clocks_kHz.ram >= 1000)
        {
          OSD_G_PRINTF ", %#4u MHz",
            gpu_stats->gpus [i].clocks_kHz.ram / 1000UL
          OSD_END
        }

        OSD_G_PRINTF "\n" OSD_END
      }

      for (int i = 0; i < gpu_stats->num_gpus; i++)
      {
        // Figure out the generation from the transfer rate...
        int pcie_gen = gpu_stats->gpus [i].hwinfo.pcie_gen;

        if (nvapi_init)
        {
          OSD_G_PRINTF "%*hsSHARE%i : %#5llu MiB (%#3u%%: %#5.02lf GiB/s), PCIe %i.0x%u\n", left_padding, pad_str,
            i,
                         gpu_stats->gpus [i].memory_B.nonlocal          >> 20ULL,
                         gpu_stats->gpus [i].loads_percent.bus / 1000,
                         gpu_stats->gpus [i].hwinfo.pcie_bandwidth_mb () / 1024.0 *
   static_cast <double> (gpu_stats->gpus [i].loads_percent.bus / 1000)   / 100.0,
                         pcie_gen,
                         gpu_stats->gpus [i].hwinfo.pcie_lanes
                         //gpu_stats->gpus [i].hwinfo.pcie_transfer_rate
          OSD_END
        }

        else if (gpu_stats->gpus [i].hwinfo.pcie_lanes > 0 && pcie_gen > 0)
        {
          OSD_G_PRINTF "%*hsSHARE%i : %#5llu MiB, PCIe %i.0x%u\n", left_padding, pad_str,
            i,
            gpu_stats->gpus [i].memory_B.nonlocal    >> 20ULL,
            pcie_gen,
            gpu_stats->gpus [i].hwinfo.pcie_lanes
          OSD_END
        }

        else
        {
          OSD_G_PRINTF "%*hsSHARE%i : %#5llu MiB       \n", left_padding, pad_str,
            i,
            gpu_stats->gpus [i].memory_B.nonlocal    >> 20ULL
          OSD_END
        }

        // Add memory temperature if it exists
        if (gpu_stats->gpus [i].temps_c.ram != 0)
        {
          static std::string temp ("", 16);

          temp.assign (
            SK_FormatTemperature (
              (double)gpu_stats->gpus [i].temps_c.ram,
                Celsius,
                  config.system.prefer_fahrenheit ? Fahrenheit :
                                                    Celsius,     pTLS ).data ()
          );

          OSD_G_PRINTF ", (%hs)",
            temp.data ()
          OSD_END
        }
      }
    }

    OSD_X_PRINTF "\n" OSD_END
  }

  else {
    OSD_F_PRINTF "\n" OSD_END
  }

  if (config.render.show && (SK_IsD3D9 () || SK_IsD3D11 () || SK_IsOpenGL ()))
  {
    if (SK_IsD3D11 ())
    {
      OSD_R_PRINTF "%hs\n\n",
        SK::DXGI::getPipelineStatsDesc ().c_str ()
      OSD_END
    }

    else if (SK_IsD3D9 ())
    {
      OSD_R_PRINTF "%hs\n\n",
        SK::D3D9::getPipelineStatsDesc ().c_str ()
      OSD_END
    }

    else if (SK_IsOpenGL ())
    {
      OSD_R_PRINTF "%hs\n\n",
        SK::OpenGL::getPipelineStatsDesc ().c_str ()
      OSD_END
    }
  }

  static auto& cpu_stats = SK_WMI_CPUStats.get ();

  if (cpu_stats.booting)
  {
    OSD_C_PRINTF "  Starting CPU Monitor...\n\n" OSD_END
  }

  else
  {
    const auto& cpus     = cpu_stats.cpus;
    const auto& num_cpus = cpu_stats.num_cpus;

    OSD_C_PRINTF "%*hsTotal  : %#3li%%  -  (Kernel: %#3li%%   "
                   "User: %#3li%%   Interrupt: %#3li%%)\n", left_padding, pad_str,
        static_cast <long> (      cpus [64].getPercentLoad      ()),
          static_cast <long> (    cpus [64].getPercentKernel    ()),
            static_cast <long> (  cpus [64].getPercentUser      ()),
              static_cast <long> (cpus [64].getPercentInterrupt ())
    OSD_END

    static const int digits = (num_cpus / 11 > 0 ? 2 : 1);

    for (DWORD i = 0; i < num_cpus; i++)
    {
      if (! config.cpu.simple)
      {
        OSD_C_PRINTF "%*hsCPU%0*lu%-*s: %#3li%%  -  (Kernel: %#3li%%   "
                     "User: %#3li%%   Interrupt: %#3li%%)\n", left_padding, pad_str,
          digits, i, 4-digits, "",
            static_cast <long> (      cpus [i].getPercentLoad      ()),
              static_cast <long> (    cpus [i].getPercentKernel    ()),
                static_cast <long> (  cpus [i].getPercentUser      ()),
                  static_cast <long> (cpus [i].getPercentInterrupt ())
        OSD_END
      }

      else
      {
        OSD_C_PRINTF "%*hsCPU%0*lu%-*s: %#3li%%\n", left_padding, pad_str,
          digits, i, 4-digits, "",
            static_cast <long> (cpus [i].getPercentLoad ())
        OSD_END
      }
    }
    
    OSD_C_PRINTF "\n" OSD_END
  }

  // Only do this if the IO data view is active
  if (config.io.show)
    SK_CountIO (io_counter, 0);//config.io.interval / 1.0e-7);

  OSD_I_PRINTF "%*hsRead   :%#6.02f MiB/s - (%#7.01f IOP/s)\n"
               "%*hsWrite  :%#6.02f MiB/s - (%#7.01f IOP/s)\n"
             //"%*hsOther  :%#6.02f MiB/s - (%#7.01f IOP/s)\n",
               "\n",
               left_padding, pad_str,
               io_counter.read_mb_sec,  io_counter.read_iop_sec,
               left_padding, pad_str,
               io_counter.write_mb_sec, io_counter.write_iop_sec//,
               //io_counter.other_mb_sec, io_counter.other_iop_sec,
               //left_padding, pad_str
  OSD_END

  if (nodes > 0 && nodes < 4)
  {
    int i = 0;

    OSD_M_PRINTF   "----- (DXGI 1.4): Local Memory -------"
                   "--------------------------------------\n"
    OSD_END

    while (i < nodes)
    {
      OSD_M_PRINTF "%*hs%8s %i  (Reserve:  %#5llu / %#5llu MiB  - "
                   " Budget:  %#5llu / %#5llu MiB)", left_padding, pad_str,
                  nodes > 1 ? (nvapi_init ? "SLI Node" : "CFX Node") : "GPU",
                  i,
             dxgi_mem_info [buffer].local [i].CurrentReservation      >> 20ULL,
             dxgi_mem_info [buffer].local [i].AvailableForReservation >> 20ULL,
             dxgi_mem_info [buffer].local [i].CurrentUsage            >> 20ULL,
             dxgi_mem_info [buffer].local [i].Budget                  >> 20ULL
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
      if ((dxgi_mem_info [buffer].nonlocal [i].CurrentUsage >> 20ULL) > 0)
      {
        OSD_M_PRINTF "%*hs%8s %i  (Reserve:  %#5llu / %#5llu MiB  -  "
                     "Budget:  %#5llu / %#5llu MiB)\n", left_padding, pad_str,
                         nodes > 1 ? "SLI Node" : "GPU",
                         i,
           dxgi_mem_info [buffer].nonlocal [i].CurrentReservation      >> 20ULL,
           dxgi_mem_info [buffer].nonlocal [i].AvailableForReservation >> 20ULL,
           dxgi_mem_info [buffer].nonlocal [i].CurrentUsage            >> 20ULL,
           dxgi_mem_info [buffer].nonlocal [i].Budget                  >> 20ULL
        OSD_END
      }

      i++;
    }

    OSD_M_PRINTF "----- (DXGI 1.4): Miscellaneous ------"
                 "--------------------------------------\n"
    OSD_END

    int64_t headroom = dxgi_mem_info [buffer].local [0].Budget -
                       dxgi_mem_info [buffer].local [0].CurrentUsage;

    OSD_M_PRINTF "  Max. Resident Set:  %#5llu MiB  -"
                 "  Max. Over Budget:  %#5llu MiB\n"
                 "     Budget Changes:  %#5llu      - "
                  "      Budget Left:  %#5lli MiB\n",
                               dxgi_mem_stats [0].max_usage       >> 20ULL,
                               dxgi_mem_stats [0].max_over_budget >> 20ULL,
                               dxgi_mem_stats [0].budget_changes,
                                    headroom / 1024 / 1024
    OSD_END
      
    OSD_M_PRINTF "\n" OSD_END
  }

  OSD_M_PRINTF "" OSD_END
  {
    static PROCESS_MEMORY_COUNTERS_EX
           pmcx    = {           };
           pmcx.cb = sizeof (pmcx);

    static DWORD              // Throttle stuff
        lastMemoryUpdate = 0;
    if (lastMemoryUpdate < SK::ControlPanel::current_time - 125)
    {
      GetProcessMemoryInfo (
        SK_GetCurrentProcess (),
                   (PPROCESS_MEMORY_COUNTERS)
               &pmcx,
        sizeof (pmcx)
      );
    }

    static
      std::wstring working_set,
                   commit,
                   virtual_size;

    working_set.assign  (
      SK_File_SizeToString (pmcx.WorkingSetSize, MiB, pTLS).data ());
    commit.assign       (
      SK_File_SizeToString (pmcx.PrivateUsage,   MiB, pTLS).data ());
    virtual_size.assign (
      SK_File_SizeToString (pmcx.PrivateUsage           +
                            pmcx.QuotaNonPagedPoolUsage +
                            pmcx.QuotaPagedPoolUsage, MiB,
                                                      pTLS).data ());

    OSD_M_PRINTF "  Working Set: %ws,  Committed: %ws,  Address Space: %ws\n",
      working_set.c_str  (),
      commit.c_str       (),
      virtual_size.c_str ()
    OSD_END

    static
      std::wstring working_set_peak,
                   commit_peak,
                   virtual_peak;

    working_set_peak.assign (
      SK_File_SizeToString (pmcx.PeakWorkingSetSize,      MiB, pTLS).data ());
    commit_peak.assign      (
      SK_File_SizeToString (pmcx.PeakPagefileUsage,       MiB, pTLS).data ());
    virtual_peak.assign     (
      SK_File_SizeToString (pmcx.PeakPagefileUsage          +
                            pmcx.QuotaPeakNonPagedPoolUsage +
                            pmcx.QuotaPeakPagedPoolUsage, MiB, pTLS).data ());

    OSD_M_PRINTF "        *Peak: %ws,      *Peak: %ws,          *Peak: %ws\n",
      working_set_peak.c_str (),
      commit_peak.c_str      (),
      virtual_peak.c_str     ()
    OSD_END

    OSD_M_PRINTF "\n" OSD_END
  }

  extern bool SK_D3D11_cache_textures;

  if (SK_D3D11_cache_textures && SK_IsD3D11 ())
  {
    extern std::string SK_D3D11_SummarizeTexCache (void);

    OSD_M_PRINTF "%s\n",
      SK_D3D11_SummarizeTexCache ().c_str ()
    OSD_END

    OSD_M_PRINTF "\n" OSD_END
  }

  static auto& disk_stats = SK_WMI_DiskStats.get ();

  if (disk_stats.booting)
  {
    OSD_D_PRINTF "  Starting Disk Monitor...\n\n" OSD_END
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
      static std::wstring read_bytes_sec,
                          write_bytes_sec;

      read_bytes_sec.assign (
        SK_File_SizeToStringF (disk_stats.disks [i].read_bytes_sec, 6, 1, Auto, pTLS).data ());

      write_bytes_sec.assign (
        SK_File_SizeToStringF (disk_stats.disks [i].write_bytes_sec, 6, 1, Auto, pTLS).data ());

      if (i == 0)
      {
        OSD_D_PRINTF "%*hsDisk %16s %#3llu%%  -  (Read %#3llu%%: %ws/s, "
                                                 "Write %#3llu%%: %ws/s)\n", left_padding, pad_str,
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
        OSD_D_PRINTF "%*hsDisk %-16s %#3llu%%  -  (Read %#3llu%%: %ws/s, "
                                                  "Write %#3llu%%: %ws/s)\n", left_padding, pad_str,
          disk_stats.disks [i].name,
            disk_stats.disks [i].percent_load,
              disk_stats.disks [i].percent_read,
                read_bytes_sec.c_str (),
                  disk_stats.disks [i].percent_write,
                    write_bytes_sec.c_str ()
        OSD_END
      }
    }

    OSD_D_PRINTF "\n" OSD_END
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

  auto& pagefile_stats = *SK_WMI_PagefileStats;

  if (pagefile_stats.booting)
  {
    OSD_P_PRINTF "  Starting Pagefile Monitor...\n\n" OSD_END
  }

  else
  {
    static std::wstring usage,
                        size,
                        peak;

    for (DWORD i = 0; i < pagefile_stats.num_pagefiles; i++)
    {
      usage.assign (
        SK_File_SizeToStringF (pagefile_stats.pagefiles [i].usage,      5, 2, Auto, pTLS).data ());
      size.assign  (
        SK_File_SizeToStringF (pagefile_stats.pagefiles [i].size,       5, 2, Auto, pTLS).data ());
      peak.assign  (
        SK_File_SizeToStringF (pagefile_stats.pagefiles [i].usage_peak, 5, 2, Auto, pTLS).data ());

      OSD_P_PRINTF "\n%*hsPagefile %20s  %ws / %ws  (Peak: %ws)", left_padding, pad_str,
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

  BOOL bRet =
    SK_UpdateOSD (szOSD);

  ImGui::End ();

  return
    bRet;
}

BOOL
__stdcall
SK_UpdateOSD (LPCSTR lpText, LPVOID pMapAddr, LPCSTR lpAppName)
 {
  UNREFERENCED_PARAMETER (pMapAddr);

  if (! ReadAcquire (&osd_init))
    return FALSE;

  if (lpAppName == nullptr)
      lpAppName = "Special K";

  SK_TextOverlay* pOverlay =
    SK_TextOverlayManager::getInstance    ()->
                           getTextOverlay (lpAppName);

#define IMPLICIT_CREATION
#ifdef  IMPLICIT_CREATION
  if (pOverlay == nullptr)
  {
    pOverlay =
      SK_TextOverlayManager::getInstance       ()->
                             createTextOverlay (lpAppName);
  }
#endif

  if (pOverlay != nullptr)
  {
    pOverlay->update (lpText);
    return TRUE;
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
    SK_TextOverlayManager::getInstance ()->
                           getTextOverlay (lpAppName);

  if (overlay != nullptr)
  {
    const auto fX = static_cast <float> (x);
    const auto fY = static_cast <float> (y);

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

  if (overlay_mgr != nullptr)
  {
    SK_TextOverlay* overlay =
      overlay_mgr->getTextOverlay (lpAppName);

    if (overlay != nullptr)
    {
      if (relative)
        overlay->resize (fScale);
      else
        overlay->setScale (fScale);

      // TEMP HACK
      // ---------
      //
      // If the primary overlay is rescaled, rescale everything else with it...
      if (overlay == overlay_mgr->getTextOverlay ("Special K"))
      {
        static auto* cp =
          SK_GetCommandProcessor ();

        if (cp != nullptr)
        {
          cp->ProcessCommandFormatted (
            "OSD.Scale %f",
              overlay->getScale ()
          );
        }
      }
    }
  }
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
SK_TextOverlay::reset (void)
{
  transform_.x = 0.0f;
  transform_.y = 0.0f;
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
[](const char* line) noexcept
{
  int num_lines;

  for (   num_lines = 0;
    line [num_lines];
    line [num_lines] == '\n' ? num_lines++ :
                                   *line++ );

  return
    num_lines;
};

void
__stdcall
SK_OSD_GetDefaultColor (float& r, float& g, float& b) noexcept
{
  r = (238.0f / 255.0f);
  g = (250.0f / 255.0f);
  b = (  5.0f / 255.0f);
}

float
SK_TextOverlay::update (const char* szText)
{
  //
  // TODO: Add a queue of deferred updates so that this can be called
  //         whether or not ImGui is rendering when the update is needed
  //

  const size_t src_len =
    ( szText != nullptr ? strlen (szText) :
                                     0   );

  if (src_len != 0)
  {
    if (data_.text_capacity <  src_len)
    {   data_.text_capacity = (src_len + 4096) -
                              (src_len % 4096);
      std::free   (data_.text);
                   data_.text = (char *)
      std::malloc (data_.text_capacity );
    }

    data_.text_len =
      std::min (data_.text_capacity, src_len + 1);

    strncpy_s (
      data_.text,
      data_.text_len, szText,
           _TRUNCATE );
  }

  else
  {
    *data_.text     = '\0';
     data_.text_len =   0 ;
  }

  // Empty the gometry buffer and bail-out.
  if (szText == nullptr || *data_.text == '\0')
  {
           data_.extent = 0.0f;
    return data_.extent;
  }

  extern ImFont*  SK_ImGui_GetFont_Consolas (void);
  ImFont* pFont = SK_ImGui_GetFont_Consolas (    );

  if (ImGui::GetCurrentWindowRead () == nullptr || pFont == nullptr)
    return data_.extent;

  ImGui::PushFont (pFont);

  auto pImGuiWindow = ImGui::GetCurrentWindow ();
       pImGuiWindow->FontWindowScale = font_.scale;

        float baseline = 0.0f;
  const float spacing  = pImGuiWindow->CalcFontSize () + ImGui::GetStyle ().ItemSpacing.y;

  float red   = static_cast <float> (config.osd.red)   / 255.0f;
  float green = static_cast <float> (config.osd.green) / 255.0f;
  float blue  = static_cast <float> (config.osd.blue)  / 255.0f;

  if ( config.osd.red   == SK_NoPreference ||
       config.osd.green == SK_NoPreference ||
       config.osd.blue  == SK_NoPreference )
  {
    SK_OSD_GetDefaultColor (red, green, blue);
  }

  float   x, y;
  getPos (x, y);

  x += transform_.x;
  y += transform_.y;


  if (temp_.tokenizer_workingset.capacity () < data_.text_len)
      temp_.tokenizer_workingset.resize (      data_.text_len);

  auto tokenized_text =
    temp_.tokenizer_workingset.data ();

  strncpy_s ( tokenized_text, data_.text_len,
                              data_.text, _TRUNCATE );

  char token [2] = "\n";

  int   num_lines    = SK_CountLines (tokenized_text);
  char* line         = strtok_ex     (tokenized_text, token);

  bool  has_tokens   = (num_lines > 0);

  line =
    ( has_tokens ? line
                 : tokenized_text );

  // Compute the longest line so we can left-align text
  if (x < 0.0f)
  {
    float longest_line = 0.0f;

    while (line != nullptr)
    {
      // Fast-path: Skip blank lines
      if (*line != '\0')
      {
        longest_line = std::max ( longest_line,
            ImGui::GetFont ()->CalcTextSizeA   (
                 pImGuiWindow->CalcFontSize (),
                           FLT_MAX, 0.0f, line ).x
                                );
      }

      line =
        ( has_tokens ? strtok_ex (nullptr, token)
                     :            nullptr );
    }

    // Add 1.0 so that an X position of -1.0 is perfectly flush with the right
    x = ImGui::GetIO ().DisplaySize.x + x - longest_line + 1.0f;

    strncpy_s ( tokenized_text, data_.text_len,
                                data_.text, _TRUNCATE );

    // Restart tokenizing
    line =
      ( has_tokens ? strtok_ex (tokenized_text, token)
                   :            tokenized_text );
  }

  else x += 5.0f; // Fix-up for text off-screen

  static const ImVec4 shadow     (0.0f,  0.0f, 0.0f, 1.0f);
               ImVec4 foreground ( red, green, blue, 1.0f);

  if (y < 0.0f)
    y = ImGui::GetIO ().DisplaySize.y + y - ((num_lines + 1) * spacing) + 1.0f;

  auto draw_list =
    ImGui::GetWindowDrawList  ();

  while (line != nullptr)
  {
    // Fast-path: Skip blank lines
    if (*line != '\0')
    {
      // First the shadow
      //
      draw_list->AddText (
        ImGui::GetFont     (),
        ImGui::GetFontSize (), ImVec2 (x            + 1.f,
                                       y + baseline + 1.f),
                     ImColor (shadow),         line,
                              NULL, 0.0f, nullptr );
      // Then the foreground
      //
      draw_list->AddText (
        ImGui::GetFont     (),
        ImGui::GetFontSize (), ImVec2 (x, y + baseline),
                     ImColor (foreground),        line,
                                 NULL, 0.0f, nullptr );
    }

    line =
      ( has_tokens ? strtok_ex (nullptr, token)
                   :            nullptr );

    baseline += spacing;
  }

  data_.extent = baseline;

  ImGui::PopFont ();

  return
    data_.extent;
}

float
SK_TextOverlay::draw (float x, float y, bool full)
{
  transform_.x = x;
  transform_.y = y;

  if (full)
  {
    update (nullptr);
  }

  return data_.extent;
}

void
SK_TextOverlay::setScale (float scale) noexcept
{
  if (scale >= 0.1f)
    font_.scale = scale;
  else
    font_.scale = 0.1f;
}

float
SK_TextOverlay::getScale (void) noexcept
{
  return font_.scale;
}

void
SK_TextOverlay::resize (float incr) noexcept
{
  setScale (getScale () + incr);
}

void
SK_TextOverlay::move (float  x_off, float  y_off) noexcept
{
  float x,y;

  getPos (x, y);
  setPos (x + x_off, y + y_off);
}

void
SK_TextOverlay::setPos (float x,float y) noexcept
{
  // We cannot anchor the command console to the left or bottom...
  if (0 == strcmp (data_.name, "SpecialK Console"))
  {
    x = std::max (0.0f, x);
    y = std::max (0.0f, y);
  }

  pos_.x = x;
  pos_.y = y;
}

void
SK_TextOverlay::getPos (float& x, float& y) noexcept
{
  x = pos_.x;
  y = pos_.y;
}


void
SK_TextOverlayManager::queueReset (void)
{
  need_full_reset_ = true;
}

void
SK_TextOverlayManager::resetAllOverlays (void)
{
  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*lock_);

  auto it =
    overlays_.begin ();

  while (it != overlays_.cend ())
  {
    it->second->pos_.x = static_cast <float> (config.osd.pos_x);
    it->second->pos_.y = static_cast <float> (config.osd.pos_y);

    it->second->font_.scale = config.osd.scale;

    it->second->font_.primary_color =
     ( ( (config.osd.red   << 16) & 0xff0000 ) |
       ( (config.osd.green << 8)  & 0xff00   ) |
         (config.osd.blue         & 0xff     ) );

    it->second->font_.shadow_color = 0xff000000;

    it->second->reset  ();
    it->second->update (nullptr);

    ++it;
  }

  it =
    overlays_.begin ();

  while (it != overlays_.cend ())
  {
    it->second->reset  ();
    it->second->update (nullptr);

    ++it;
  }
}

float
SK_TextOverlayManager::drawAllOverlays (float x, float y, bool full)
{
  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*lock_);

  const float base_y = y;

  // Draw top-down
  if (config.osd.pos_y >= 0)
  {
    auto it =
      overlays_.cbegin ();

    // Handle situations where the OSD font is not yet loaded
    if ( it         == overlays_.cend () ||
         it->second == nullptr )
    {
      return fabs (y - base_y);
    }

    while (it != overlays_.cend ())
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
         it->second              == nullptr )
    {
      return fabs (y - base_y);
    }

    // Push the starting position up one whole line
 /////////////////y -= it->second->font_.cegui->getFontHeight (config.osd.scale);

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
  std::scoped_lock <SK_Thread_HybridSpinlock>
        scope_lock (*lock_);

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
    {
      config.osd.pos_x =
        *static_cast <signed int *> (val);
    }

    else if (var == pos_.y)
    {
      config.osd.pos_y =
        *static_cast <signed int *> (val);
    }

    else if (var == scale_)
    {
      config.osd.scale =
        *static_cast <float *> (val);
    }


    std::scoped_lock <SK_Thread_HybridSpinlock>
          scope_lock (*lock_);

      auto  it =  overlays_.cbegin ();
    while ( it != overlays_.cend   () )
    {
      if (var == pos_.x || var == pos_.y)
      {
        const auto pos_x = static_cast <float> (config.osd.pos_x);
        const auto pos_y = static_cast <float> (config.osd.pos_y);

        it->second->setPos (pos_x, pos_y);
      }

      else if (var == scale_)
        it->second->setScale (config.osd.scale);

      ++it;
    }

    return true;
  }


  if ( var == mem_.show  ||
       var == cpu_.show  ||
       var == disk_.show ||
       var == pagefile_.show )
  {
    *static_cast <bool *> (var->getValuePointer ()) =
    *static_cast <bool *> (val);

    if (*static_cast <bool *> (val))
      SK_StartPerfMonThreads ( );

    return true;
  }

  if (var == sli_.show)
  {
    if (            nvapi_init  &&
         sk::NVAPI::nv_hardware &&
         sk::NVAPI::CountSLIGPUs () > 1 )
    {
      *static_cast <bool *> (var->getValuePointer ()) =
      *static_cast <bool *> (val);

      return true;
    }

    return false;
  }

  if ( var == io_.show  ||
       var == gpu_.show ||
       var == fps_.show )
  {
    *static_cast <bool *> (var->getValuePointer ()) =
    *static_cast <bool *> (val);

    return true;
  }

  if (var == osd_.show)
  {
    *static_cast <bool *> (var->getValuePointer ()) =
    *static_cast <bool *> (val);

    if (*static_cast <bool *> (val))
      SK_InstallOSD ();

    return true;
  }

  return false;
}