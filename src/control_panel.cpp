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

#include <SpecialK/control_panel.h>

#include <imgui/backends/imgui_d3d11.h>
#include <imgui/backends/imgui_d3d12.h>
#include <imgui/backends/imgui_d3d9.h>
#include <imgui/backends/imgui_gl3.h>

#include <SpecialK/control_panel/compatibility.h>
#include <SpecialK/control_panel/d3d11.h>
#include <SpecialK/control_panel/d3d9.h>
//#include <SpecialK/control_panel/d3d12.h>
#include <SpecialK/control_panel/input.h>
#include <SpecialK/control_panel/opengl.h>
#include <SpecialK/control_panel/osd.h>
#include <SpecialK/control_panel/plugins.h>
#include <SpecialK/control_panel/sound.h>
#include <SpecialK/control_panel/steam.h>
#include <SpecialK/control_panel/window.h>

#include <SpecialK/nvapi.h>

#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>

LONG imgui_staged_frames   = 0;
LONG imgui_finished_frames = 0;
BOOL imgui_staged          = FALSE;

extern float g_fDPIScale;

using namespace SK::ControlPanel;

SK_RenderAPI                 SK::ControlPanel::render_api;
DWORD                        SK::ControlPanel::current_time = 0;
uint64_t                     SK::ControlPanel::current_tick;// Perf Counter
SK::ControlPanel::font_cfg_s SK::ControlPanel::font;

DWORD
SK_GetCurrentMS (void)
{
  // Handle possible scenario where SK uses this before
  // the control panel is initialized / rendered
  if (SK::ControlPanel::current_time == 0)
      SK::ControlPanel::current_time = SK_timeGetTime ();

  return
    SK::ControlPanel::current_time;
}

void
SK_Display_UpdateOutputTopology (void);

bool __imgui_alpha = true;

__declspec (dllexport)
void
SK_ImGui_Toggle (void);

//extern uint32_t __stdcall SK_Steam_PiratesAhoy (void);
extern uint32_t __stdcall SK_SteamAPI_AppID    (void);

extern bool     __stdcall SK_FAR_IsPlugIn      (void);
extern void     __stdcall SK_FAR_ControlPanel  (void);

std::wstring
SK_NvAPI_GetGPUInfoStr (void);

void
LoadFileInResource ( int          name,
                     int          type,
                     DWORD*       size,
                     const char** data )
{
  HMODULE handle =
    SK_GetModuleHandle (nullptr);

  HRSRC rc =
   FindResource ( handle,
                    MAKEINTRESOURCE (name),
                    MAKEINTRESOURCE (type) );

  if (rc != nullptr)
  {
    HGLOBAL rcData =
      LoadResource (handle, rc);

    if (rcData != nullptr)
    {
      *size =
        SizeofResource (handle, rc);

      *data =
        static_cast <const char *>(
          LockResource (rcData)
        );
    }
  }
}

static UINT  _uiDPI     =     96;
static float _fDPIScale = 100.0f;

static auto SK_DPI_Update = [](void) ->
void
{
  using  GetDpiForSystem_pfn = UINT (WINAPI *)(void);
  static GetDpiForSystem_pfn
         GetDpiForSystem = (GetDpiForSystem_pfn)
    SK_GetProcAddress ( SK_GetModuleHandleW (L"user32"),
                                    "GetDpiForSystem" );

  using  GetDpiForWindow_pfn = UINT (WINAPI *)(HWND);
  static GetDpiForWindow_pfn
         GetDpiForWindow = (GetDpiForWindow_pfn)
    SK_GetProcAddress ( SK_GetModuleHandleW (L"user32"),
                                    "GetDpiForWindow" );

  if (GetDpiForSystem != nullptr)
  {
    extern float
      g_fDPIScale;

    UINT dpi = ( GetDpiForWindow != nullptr &&
                        IsWindow (game_window.hWnd) ) ?
                 GetDpiForWindow (game_window.hWnd)   :
                 GetDpiForSystem (                );

    if (dpi != 0)
    {
      _uiDPI     = dpi;
      _fDPIScale =
        ( (float)dpi /
          (float)USER_DEFAULT_SCREEN_DPI ) * 100.0f;
    }
  }
};

extern void __stdcall SK_ImGui_DrawEULA (LPVOID reserved);
       bool IMGUI_API SK_ImGui_Visible;
       bool           SK_ReShade_Visible        = false;
       bool           SK_ControlPanel_Activated = false;

extern void ImGui_ToggleCursor (void);

struct show_eula_s {
  bool show;
  bool never_show_again;
} extern eula;



#pragma warning (push)
#pragma warning (disable: 4706)
// Special K Extensions to ImGui
namespace SK_ImGui
{
  bool
  VerticalToggleButton (const char *text, bool *v)
  {
          ImFont        *font_     = GImGui->Font;
    const ImFontGlyph   *glyph     = nullptr;
          char           c         = 0;
          bool           ret       = false;
    const ImGuiContext&  g         = *GImGui;
    const ImGuiStyle&    style     = g.Style;
          float          pad       = style.FramePadding.x;
          ImVec4         color     = { };
    const char*          hash_mark = strstr (text, "##");
          ImVec2         text_size = ImGui::CalcTextSize     (text, hash_mark);
          ImGuiWindow*   window    = ImGui::GetCurrentWindow ();
          ImVec2         pos       =
            ImVec2 ( window->DC.CursorPos.x + pad,
                     window->DC.CursorPos.y + text_size.x + pad );

    const ImU32 text_color =
      ImGui::ColorConvertFloat4ToU32 (style.Colors [ImGuiCol_Text]);

    if (*v)
      color = style.Colors [ImGuiCol_ButtonActive];
    else
      color = style.Colors [ImGuiCol_Button];

    ImGui::PushStyleColor (ImGuiCol_Button, color);
    ImGui::PushID         (text);

    ret = ImGui::Button ( "", ImVec2 (text_size.y + pad * 2,
                                      text_size.x + pad * 2) );
    ImGui::PopStyleColor ();

    while ((c = *text++))
    {
      glyph = font_->FindGlyph (c);

      if (! glyph)
        continue;

      window->DrawList->PrimReserve (6, 4);
      window->DrawList->PrimQuadUV  (
              ImVec2   ( pos.x + glyph->Y0 , pos.y - glyph->X0 ),
              ImVec2   ( pos.x + glyph->Y0 , pos.y - glyph->X1 ),
              ImVec2   ( pos.x + glyph->Y1 , pos.y - glyph->X1 ),
              ImVec2   ( pos.x + glyph->Y1 , pos.y - glyph->X0 ),

                ImVec2 (         glyph->U0,         glyph->V0 ),
                ImVec2 (         glyph->U1,         glyph->V0 ),
                ImVec2 (         glyph->U1,         glyph->V1 ),
                ImVec2 (         glyph->U0,         glyph->V1 ),

                  text_color );

      pos.y -= glyph->AdvanceX;

      // Do not print embedded hash codes
      if (hash_mark != nullptr && text >= hash_mark)
        break;
    }

    ImGui::PopID ();

    if (ret)
      *v ^= 1UL;

    return ret;
  }
#pragma warning (pop)

  // Should return true when clicked, this is not consistent with
  //   the rest of the ImGui API.
  bool BatteryMeter (void)
  {
    static bool has_battery = true;

    SYSTEM_POWER_STATUS sps = { };

    if (has_battery)
    {
      if (GetSystemPowerStatus (&sps))
      {
        has_battery   = (sps.BatteryFlag & 128) == 0;
        bool charging = (sps.BatteryFlag & 8  ) == 8;

        uint8_t battery_level =
          sps.ACLineStatus != 1 ?
            sps.BatteryLifePercent :
              charging ? 100 + sps.BatteryLifePercent :
                         ( (sps.BatteryFlag & 2) ||
                           (sps.BatteryFlag & 4) ) ? sps.BatteryLifePercent
                                                   : 255;

        if (battery_level < 255) // 255 = Running on AC (not charging)
        {
          if (battery_level  > 100)
              battery_level -= 100;

          const float battery_ratio = (float)battery_level/100.0f;

          static char szBatteryLevel [128] = { };

          if (sps.BatteryLifeTime != -1)
            snprintf (szBatteryLevel, 127, "%hhu%% Battery Remaining\t\t[%lu Minutes]",
                      battery_level, sps.BatteryLifeTime / 60);
          else if (charging)
            snprintf (szBatteryLevel, 127, "%hhu%% Battery Charged",   battery_level);
          else
            snprintf (szBatteryLevel, 127, "%hhu%% Battery Remaining", battery_level);

          float luminance =
            charging ?
                0.166f + (0.5f + (sin ((float)(current_time % 2000) / 2000.0f)) * 0.5f) / 2.0f :
                0.666f;

          ImGui::PushStyleColor (ImGuiCol_PlotHistogram,  (unsigned int)ImColor::HSV (battery_ratio * 0.278f, 0.88f, luminance));
          ImGui::PushStyleColor (ImGuiCol_Text,           (unsigned int)ImColor (255, 255, 255));
          ImGui::PushStyleColor (ImGuiCol_FrameBg,        (unsigned int)ImColor ( 0.3f,  0.3f,  0.3f, 0.7f));
          ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, (unsigned int)ImColor ( 0.6f,  0.6f,  0.6f, 0.8f));
          ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  (unsigned int)ImColor ( 0.9f,  0.9f,  0.9f, 0.9f));

          ImGui::ProgressBar ( battery_ratio,
                                 ImVec2 (-1, 0),
                                   szBatteryLevel );

          ImGui::PopStyleColor  (5);

          return true;
        }
      }

      else
        has_battery = false;
    }

    return false;
  }
} // namespace SK_ImGui

struct SK_Warning {
  std::string  title;   // UTF-8   for ImGui
  std::wstring message; // UTF-16L for Win32 (will be converted when displayed)
};

SK_LazyGlobal <concurrency::concurrent_queue <SK_Warning>> SK_ImGui_Warnings;

void
SK_ImGui_WarningWithTitle ( const wchar_t* wszMessage,
                            const    char*  szTitle )
{
  SK_ImGui_Warnings->push (     {
      std::string  ( szTitle  ),
      std::wstring (wszMessage) }
  );
}

void
SK_ImGui_WarningWithTitle ( const wchar_t* wszMessage,
                            const wchar_t* wszTitle )
{
  SK_ImGui_WarningWithTitle (
                       wszMessage,
    SK_WideCharToUTF8 (wszTitle).c_str ()
  );
}

void
SK_ImGui_Warning (const wchar_t* wszMessage)
{
  SK_ImGui_WarningWithTitle (wszMessage, "Special K Warning");
}

void
SK_ImGui_SetNextWindowPosCenter (ImGuiCond cond)
{
  static const auto& io =
    ImGui::GetIO ();

  static const ImVec2 vCenterPivot   (0.5f, 0.5f);
         const ImVec2 vDisplayCenter (
    io.DisplaySize.x / 2.0f,
    io.DisplaySize.y / 2.0f
  );

  ImGui::SetNextWindowPos (vDisplayCenter, cond, vCenterPivot);
}

void
SK_ImGui_ProcessWarnings (void)
{
  extern bool
  SK_ImGui_IsEULAVisible (void);

  if (SK_ImGui_IsEULAVisible ())
    return;

  static SK_Warning warning =
    { "", L"" };

  if (! SK_ImGui_Warnings->empty ())
  {
    if (warning.message.empty ())
      SK_ImGui_Warnings->try_pop (warning);
  }

  if (warning.message.empty ())
    return;

  static const auto& io =
    ImGui::GetIO ();

  // Stupid hack to show ImGui windows without the control panel open
  SK_ReShade_Visible = true;

  SK_ImGui_SetNextWindowPosCenter     (ImGuiCond_Always);
  ImGui::SetNextWindowSizeConstraints ( ImVec2 (360.0f, 40.0f),
                                          ImVec2 ( 0.925f * io.DisplaySize.x,
                                                   0.925f * io.DisplaySize.y )
                                      );

  ImGui::OpenPopup (warning.title.c_str ());

  if ( ImGui::BeginPopupModal ( warning.title.c_str (),
                                  nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoScrollbar      |
                                    ImGuiWindowFlags_NoScrollWithMouse )
     )
  {
    ImGui::TextColored ( ImColor::HSV (0.075f, 1.0f, 1.0f),
                           "\n\t%ws\t\n\n", warning.message.c_str () );

    ImGui::Separator   ();
    ImGui::TextColored ( ImColor::HSV (0.15f, 1.0f, 1.0f), "   " );
    ImGui::SameLine    ();
    ImGui::Spacing     ();
    ImGui::SameLine    ();

    if (ImGui::Button ("Okay"))
    {
      SK_ReShade_Visible = false;
      warning.message.clear ();

      ImGui::CloseCurrentPopup ();
    }
    ImGui::EndPopup       ();
  }
}

bool SK_ImGui_WantExit = false;

void
SK_ImGui_ConfirmExit (void)
{
  static const auto& io =
    ImGui::GetIO ();

  SK_ImGui_WantExit = true;

  SK_ImGui_SetNextWindowPosCenter     (ImGuiCond_Always);
  ImGui::SetNextWindowSizeConstraints ( ImVec2 (360.0f, 40.0f),
                                          ImVec2 ( 0.925f * io.DisplaySize.x,
                                                   0.925f * io.DisplaySize.y )
                                      );

  ImGui::OpenPopup ("Confirm Forced Software Termination");
}

bool  SK_ImGui_UnconfirmedDisplayChanges = false;
DWORD SK_ImGui_DisplayChangeTime         = 0;

void
SK_ImGui_ConfirmDisplaySettings (bool *pDirty_, std::wstring display_name_, DEVMODEW orig_mode_)
{
  static bool               *pDirty = nullptr;
  static std::wstring  display_name = L"";
  static DEVMODEW         orig_mode = { };
  static DWORD          dwInitiated = DWORD_MAX;

  // We're initiating
  if (pDirty_ != nullptr)
  {
    pDirty       = pDirty_;
    display_name = display_name_;
    orig_mode    = orig_mode_;
    dwInitiated  = SK_timeGetTime ();

    if (config.display.confirm_mode_changes)
    {
      SK_ImGui_UnconfirmedDisplayChanges = true;
    } SK_ImGui_DisplayChangeTime         = dwInitiated;

         *pDirty = true;
  }

  if (SK_ImGui_UnconfirmedDisplayChanges)
  {
    if (SK_ImGui_DisplayChangeTime < SK_timeGetTime () - 15000)
    {
      SK_ChangeDisplaySettingsEx (
                   display_name.c_str (), &orig_mode,
                                       0, CDS_UPDATEREGISTRY, nullptr
      );

      *pDirty = true;

      dwInitiated  = DWORD_MAX;
      pDirty       = nullptr;
      orig_mode    = { };
      display_name = L"";

      SK_ImGui_UnconfirmedDisplayChanges = false;

      return;
    }

    static const auto& io =
      ImGui::GetIO ();

    SK_ImGui_SetNextWindowPosCenter     (ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints ( ImVec2 (360.0f, 40.0f),
                                            ImVec2 ( 0.925f * io.DisplaySize.x,
                                                     0.925f * io.DisplaySize.y )
                                        );

    ImGui::OpenPopup ("Confirm Display Setting Changes");
  }
}

bool
SK_ImGui_IsItemClicked (void)
{
  if (ImGui::IsItemClicked ())
    return true;

  if (ImGui::IsItemHovered ())
  {
    static auto& io =
      ImGui::GetIO ();

    if (io.NavInputsDownDuration [ImGuiNavInput_Activate] == 0.0f)
    {
      return true;
    }
  }

  return false;
}

bool
SK_ImGui_IsItemRightClicked (void)
{
  if (ImGui::IsItemClicked (1))
    return true;

  if (ImGui::IsItemHovered ())
  {
    static auto& io =
      ImGui::GetIO ();

    if (io.NavInputsDownDuration [ImGuiNavInput_Activate] > 0.4f)
    {   io.NavInputsDownDuration [ImGuiNavInput_Activate] = 0.0f;
      return true;
    }
  }

  return false;
};

bool
SK_ImGui_IsWindowRightClicked (const ImGuiIO& io)
{
  if (ImGui::IsWindowFocused () || ImGui::IsWindowHovered ())
  {
    if (ImGui::IsWindowHovered (ImGuiHoveredFlags_AnyWindow) && io.MouseClicked [1])
      return true;

    if (ImGui::IsWindowFocused () && io.MouseDoubleClicked [4])
    {
      return true;
    }
  }

  return false;
};


ImVec2&
__SK_ImGui_LastWindowCenter (void)
{
  static ImVec2 vec2 (-1.0f, -1.0f);
  return        vec2;
}

#define SK_ImGui_LastWindowCenter  __SK_ImGui_LastWindowCenter()

void SK_ImGui_CenterCursorAtPos (ImVec2 center = SK_ImGui_LastWindowCenter);
void SK_ImGui_UpdateCursor      (void);

const char*
SK_ImGui_ControlPanelTitle (void)
{
  static char szTitle [512] = { };
  const  bool steam         = (SK::SteamAPI::AppID () != 0x0);

  {
    // TEMP HACK
    static HMODULE hModTBFix = SK_GetModuleHandle (L"tbfix.dll");

    static std::wstring title;
                        title.clear ();

  //extern std::wstring __stdcall SK_GetPluginName (void);
  //extern bool         __stdcall SK_HasPlugin     (void);

    if (SK_HasPlugin () && hModTBFix == (HMODULE)nullptr)
      title += SK_GetPluginName ();
    else
    {
      title += L"Special K  (v ";
      title += SK_GetVersionStrW ();
      title += L")";
    }

    title += L"  Control Panel";

    extern __time64_t __SK_DLL_AttachTime;

    __time64_t now     = 0ULL;
     _time64 (&now);

    auto       elapsed = gsl::narrow_cast <uint32_t> (now - __SK_DLL_AttachTime);

    uint32_t   secs    =  elapsed % 60ULL;
    uint32_t   mins    = (elapsed / 60ULL) % 60ULL;
    uint32_t   hours   =  elapsed / 3600ULL;

    if (steam)
    {
      std::string appname =
        SK::SteamAPI::AppName ();

      if (appname.length ())
        title += L"      -      ";

      if (config.steam.show_playtime)
      {
        snprintf ( szTitle, 511, "%ws%s     (%01u:%02u:%02u)###SK_MAIN_CPL",
                     title.c_str (), appname.c_str (),
                       hours, mins, secs );
      }

      else
      {
        snprintf ( szTitle, 511, "%ws%s###SK_MAIN_CPL",
                     title.c_str (), appname.c_str () );
      }
    }

    else
    {
      if (config.steam.show_playtime)
      {
        title += L"      -      ";

        snprintf ( szTitle, 511, "%ws(%01u:%02u:%02u)###SK_MAIN_CPL",
                     title.c_str (),
                       hours, mins, secs );
      }

      else
        snprintf (szTitle, 511, "%ws###SK_MAIN_CPL", title.c_str ());
    }
  }

  return szTitle;
}

void
SK_ImGui_AdjustCursor (void)
{
  static CHandle hAdjustEvent (
    SK_CreateEvent (nullptr, TRUE, FALSE, nullptr)
  );

  SetEvent (hAdjustEvent.m_h);

  static HANDLE
    hThread = SK_Thread_CreateEx (
    [](LPVOID pUser)-> DWORD
    {
      HANDLE hSignals [] = {
        __SK_DLL_TeardownEvent,
             (HANDLE)pUser
      };

      DWORD dwWaitStatus =
              WAIT_ABANDONED;

      while (dwWaitStatus != WAIT_OBJECT_0)
      {
        dwWaitStatus =
          WaitForMultipleObjects ( 2, hSignals, FALSE, INFINITE );

        if (dwWaitStatus == (WAIT_OBJECT_0 + 1))
        {
          ResetEvent (hSignals [1]);

          //SK_ClipCursor (nullptr);
          //   SK_AdjustWindow   ();       // Restore game's clip cursor behavior

          SK_Window_RepositionIfNeeded ();

          //extern void SK_AdjustClipRect (void);
          //            SK_AdjustClipRect ();
        }
      };

      hAdjustEvent.Close ();

      SK_Thread_CloseSelf ();

      return 0;
    },
    L"[SK] Cursor Adjustment Thread",
    (LPVOID)hAdjustEvent
  );
}

bool reset_frame_history = true;

class SK_ImGui_FrameHistory : public SK_Stat_DataHistory <float, 120>
{
public:
  void timeFrame       (double seconds)
  {
    addValue ((float)(1000.0 * seconds));
  }
};

extern SK_LazyGlobal <SK_ImGui_FrameHistory> SK_ImGui_Frames;


#pragma optimize( "", off )
__declspec (noinline)
IMGUI_API
void
__stdcall
SK_PlugIn_ControlPanelWidget (void)
{
  // Give a function body to prevent dead code elimination
  int x = rand ();
    ++x;
}
#pragma optimize( "", on )

void
SK_Display_ResolutionSelectUI (bool bMarkDirty = false)
{
  static bool dirty = true;

  if (bMarkDirty)
  {
    dirty = true;
    return;
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  struct list_s {
    int                    idx    =  0 ;
    std::vector <DEVMODEW> modes  = { };
    std::string            string = " ";

    void clear (void) {
      idx = 0;
      modes.clear  ();
      string.clear ();
    }
  };

  static
   std::wstring display_name; // (i.e. \\DISPLAY0... legacy GDI name)
  static list_s      refresh,
                  resolution;

  static DWORD
    _maxWidth  = 0,
    _maxHeight = 0,
   _maxRefresh = 0,  _maxPixels           = 0,
                     _maxAvailablePixels  = 0,
                     _maxAvailableRefresh = 0;

  if (dirty)
  {
    _maxWidth   = 0,
    _maxHeight  = 0,  _maxPixels           = 0,
    _maxRefresh = 0;  _maxAvailablePixels  = 0,
                      _maxAvailableRefresh = 0;

    POINT
     ptCenter = {
       game_window.actual.window.left   +
     ( game_window.actual.window.right  - game_window.actual.window.left ) / 2,
       game_window.actual.window.top    +
     ( game_window.actual.window.bottom - game_window.actual.window.top  ) / 2
                };

    MONITORINFOEX minfo       = { };
    HMONITOR      hMonCurrent =
      MonitorFromPoint (ptCenter, MONITOR_DEFAULTTONULL);

    if (hMonCurrent != nullptr)
    {
      struct
      { DWORD width, height;
      } native {  0, 0    };

      for ( auto& display : rb.displays )
      {
        if (display.monitor == hMonCurrent)
        {
          native.width  = display.native.width;
          native.height = display.native.height;
          break;
        }
      }

         refresh.clear ();
      resolution.clear ();

      std::set <std::string> resolutions,
                               refreshes;

      std::set <std::pair <DWORD, DWORD>> used_resolutions_;
      std::set <DWORD>                    used_refreshes_;

      minfo.cbSize =
        sizeof (MONITORINFOEX);

      GetMonitorInfo (hMonCurrent, &minfo);

      display_name =
        minfo.szDevice;

      DEVMODEW dm_now  = { },
               dm_enum = { };

       dm_now.dmSize = sizeof (DEVMODEW);
      dm_enum.dmSize = sizeof (DEVMODEW);

      if ( EnumDisplaySettingsW (   minfo.szDevice, ENUM_CURRENT_SETTINGS,
                                                         &dm_now ) )
      { for (                                  auto idx = 0        ;
              EnumDisplaySettingsW (minfo.szDevice, idx, &dm_enum) ;
                                                  ++idx )
        {
          dm_enum.dmSize =
            sizeof (DEVMODEW);

          if (dm_enum.dmBitsPerPel == dm_now.dmBitsPerPel)
          {
            _maxWidth   = std::max (_maxWidth, dm_enum.dmPelsWidth);
            _maxHeight  = std::max (_maxWidth, dm_enum.dmPelsHeight);
            _maxPixels  = std::max (           dm_enum.dmPelsWidth * dm_enum.dmPelsHeight,
            _maxPixels             );
            _maxRefresh = std::max (           dm_enum.dmDisplayFrequency,
            _maxRefresh            );
          }

          if ( dm_enum.dmBitsPerPel == dm_now.dmBitsPerPel &&
               dm_enum.dmPelsWidth  == dm_now.dmPelsWidth  &&
               dm_enum.dmPelsHeight == dm_now.dmPelsHeight )
          {
            _maxAvailableRefresh = std::max ( dm_enum.dmDisplayFrequency,
            _maxAvailableRefresh );

            dm_enum.dmFields =
              DM_DISPLAYFREQUENCY;

            if (used_refreshes_.emplace (dm_enum.dmDisplayFrequency).second)
            {
              refresh.modes.
                emplace_back (dm_enum);
            }
          }
        }

        std::sort ( std::begin (refresh.modes),
                    std::end   (refresh.modes), []( const DEVMODEW& a,
                                                    const DEVMODEW& b )
                    { return a.dmDisplayFrequency  >
                             b.dmDisplayFrequency; } );

        for (                                  auto idx = 0        ;
              EnumDisplaySettingsW (minfo.szDevice, idx, &dm_enum) ;
                                                  ++idx )
        {
          dm_enum.dmSize =
            sizeof (DEVMODEW);

          if ( dm_now.dmBitsPerPel       ==  dm_enum.dmBitsPerPel  &&
               dm_now.dmDisplayFrequency ==  dm_enum.dmDisplayFrequency )
          { _maxAvailablePixels = std::max ( dm_enum.dmPelsWidth * dm_enum.dmPelsHeight,
            _maxAvailablePixels );           dm_enum.dmFields    =
              ( DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY );

            if (used_resolutions_.emplace (std::make_pair (dm_enum.dmPelsWidth, dm_enum.dmPelsHeight)).second)
            {
              resolution.modes.
                emplace_back (dm_enum);
            }
          }
        }

        std::sort ( std::begin (resolution.modes),
                    std::end   (resolution.modes), []( const DEVMODEW& a,
                                                       const DEVMODEW& b )
                    { return ( a.dmPelsWidth * a.dmPelsHeight )  >
                             ( b.dmPelsWidth * b.dmPelsHeight ); } );
      }

      int idx = 0;

      for ( auto& it : refresh.modes )
      {
        std::string mode_str =
          SK_FormatString ( "  %u Hz",
                   it.dmDisplayFrequency );

        if (refreshes.emplace (mode_str).second)
        {
          if ( dm_now.dmDisplayFrequency
                == it.dmDisplayFrequency )
          {
            refresh.idx = idx;
          }

          refresh.string.append (mode_str);
          refresh.string += '\0';

          ++idx;
        }
      }

      idx = 0;

      for ( auto& it : resolution.modes )
      {
        std::string mode_str =
          SK_FormatString ( "%ux%u",
               it.dmPelsWidth,
               it.dmPelsHeight );

        if (resolutions.emplace (mode_str).second)
        {
          if ( it.dmPelsWidth  == dm_now.dmPelsWidth &&
               it.dmPelsHeight == dm_now.dmPelsHeight )
          {
            resolution.idx = idx;
          }

          if ( it.dmPelsWidth  == native.width &&
               it.dmPelsHeight == native.height )
          {  resolution.string += "* "; } else
             resolution.string += "  ";

          resolution.string.append (mode_str);
          resolution.string += '\0';

          ++idx;
        }
      }

      resolution.string += '\0';
      refresh.string    += '\0';

      dirty = false;
    }
  }

  std::vector <int> outputs;

  char display_list [4096] = { };
  int  active_idx          =  0;

  for ( auto& display : rb.displays )
  {
    if (display.attached)
      outputs.push_back (display.idx);
  }

  char* pszDispList =
       display_list;

  bool found = false;

  for ( auto output : outputs )
  {
    StrCatA ( pszDispList,
      SK_WideCharToUTF8 (
        rb.displays [output].name
      ).c_str ()
    );

    pszDispList +=
      wcsnlen (rb.displays [output].name, 128);

    (*pszDispList++) = '\0';

    if (! _wcsnicmp ( rb.displays [output].name,
                                rb.display_name, 128 )
       ) { active_idx = output; found = true; }
  }

  // Give it another go, the monitor was no-show...
  if (! found) dirty = true;

  (*pszDispList++) = '\0';

  //ImGui::TextColored ( ImColor (0.75f, 0.75f, 0.75f), "Desktop Settings" );
  //ImGui::SameLine              ( 0.0f, 30.0f);
  ImGui::TreePush ();

  if (ImGui::Combo ("Active Monitor", &active_idx, display_list))
  {
    //auto* cp =
    //  SK_GetCommandProcessor ();

    config.window.res.override.x = 0,
    config.window.res.override.y = 0;
    config.window.fullscreen     = false;
    config.window.borderless     = false;
    config.window.center         = false;

    config.display.monitor_handle = rb.displays [active_idx].monitor;
    config.display.monitor_idx    = rb.displays [active_idx].idx;

    config.window.res.override.x = (rb.displays [active_idx].rect.right  - rb.displays [active_idx].rect.left),
    config.window.res.override.y = (rb.displays [active_idx].rect.bottom - rb.displays [active_idx].rect.top);

    SK_Window_RepositionIfNeeded ();

    //cp->ProcessCommandFormatted ("Window.OverrideRes %ux%u",

    config.window.borderless = true;
    config.window.center     = true;
    config.window.fullscreen = true;

    SK_Window_RepositionIfNeeded ();

    config.window.res.override.x = 0;
    config.window.res.override.y = 0;

    SK_SetCursorPos
    ( rb.displays [active_idx].rect.left   +
     (rb.displays [active_idx].rect.right  - rb.displays [active_idx].rect.left) / 2,
      rb.displays [active_idx].rect.top    +
     (rb.displays [active_idx].rect.bottom - rb.displays [active_idx].rect.top)  / 2 );

    dirty = true;
  }

  if (ImGui::Combo (   "Resolution",  &resolution.idx,
                                       resolution.string.c_str () ))
  {
    DEVMODEW
      dm_orig        = {               };
      dm_orig.dmSize = sizeof (DEVMODEW);

    if (EnumDisplaySettingsW (display_name.c_str (), ENUM_CURRENT_SETTINGS, &dm_orig))
    {
      if ( DISP_CHANGE_SUCCESSFUL ==
             SK_ChangeDisplaySettingsEx (
                 display_name.c_str (), &resolution.modes [resolution.idx],
                                     0, CDS_TEST, nullptr )
         )
      {
        if ( DISP_CHANGE_SUCCESSFUL ==
               SK_ChangeDisplaySettingsEx (
                   display_name.c_str (), &resolution.modes [resolution.idx],
                                       0, CDS_UPDATEREGISTRY, nullptr )
           )
        {
          SK_ImGui_ConfirmDisplaySettings (&dirty, display_name, dm_orig);
        }
      }
    }
  }

  if (ImGui::IsItemHovered () && (_maxAvailablePixels < _maxPixels))
      ImGui::SetTooltip ("Higher Resolutions are Available by selecting a Different Refresh Rate");


  if (ImGui::Combo ( "Refresh Rate",  &refresh.idx,
                                       refresh.string.c_str () ))
  {
    DEVMODEW
      dm_orig        = {               };
      dm_orig.dmSize = sizeof (DEVMODEW);

    if (EnumDisplaySettingsW (display_name.c_str (), ENUM_CURRENT_SETTINGS, &dm_orig))
    {
      if ( DISP_CHANGE_SUCCESSFUL ==
             SK_ChangeDisplaySettingsEx (
                 display_name.c_str (), &refresh.modes [refresh.idx],
                                     0, CDS_TEST, nullptr )
         )
      {
        if ( DISP_CHANGE_SUCCESSFUL ==
               SK_ChangeDisplaySettingsEx (
                   display_name.c_str (), &refresh.modes [refresh.idx],
                                       0, CDS_UPDATEREGISTRY, nullptr )
           )
        {
          SK_ImGui_ConfirmDisplaySettings (&dirty, display_name, dm_orig);
        }
      }
    }
  }

  if (ImGui::IsItemHovered () && (_maxAvailableRefresh < _maxRefresh))
      ImGui::SetTooltip ("Higher Refresh Rates are Available at a Different Resolution");

  ImGui::TreePop ();
}

void
DisplayModeMenu (bool windowed)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  enum {
    DISPLAY_MODE_WINDOWED   = 0,
    DISPLAY_MODE_FULLSCREEN = 1
  };

  int         mode  = windowed ? DISPLAY_MODE_WINDOWED :
                                 DISPLAY_MODE_FULLSCREEN;
  int    orig_mode  = mode;
  bool       force  = windowed ? config.display.force_windowed :
                                 config.display.force_fullscreen;

  const char* modes = "Windowed Mode\0Fullscreen Mode\0\0";


  // Engaging fullscreen mode in OpenGL is more complicated than I want to
  //   deal with right now, involving ChangeDisplaySettings (...)
  bool     can_go_full = false;

  if (( (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9)  ) ||
        (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11) ) ))
  {
    // ATTN Unity:  Use asynchronous window messages!
    can_go_full = !( rb.windows.unity && (
                             (int)rb.api & (int)SK_RenderAPI::D3D11) );
  }


  if ( static_cast <int> (rb.api) &
       static_cast <int> (SK_RenderAPI::D3D11) )
  {
    if (rb.srgb_stripped)
    {
      int srgb_mode      = std::max (0, config.render.dxgi.srgb_behavior + 1);
      int srgb_orig_mode =              config.render.dxgi.srgb_behavior + 1;

      if (ImGui::Combo ( "sRGB Bypass###SubMenu_DisplaySRGB_Combo", &srgb_mode,
                         "Passthrough\0Strip\0Apply\0\0",            3)    &&
                                                        srgb_mode != srgb_orig_mode)
      {
        config.render.dxgi.srgb_behavior = srgb_mode - 1;

        SK_SaveConfig ();
      }
    }
  }


  if (can_go_full)
  {
    if (ImGui::Combo ("###SubMenu_DisplayMode_Combo", &mode, modes, 2) && mode != orig_mode)
    {
      switch (mode)
      {
        case DISPLAY_MODE_WINDOWED:
        {
          rb.requestWindowedMode (force);

          config.window.borderless = true;

          bool borderless = config.window.borderless;
          bool fullscreen = config.window.fullscreen;

          if (borderless)
            SK_DeferCommand ("Window.Borderless 1");
          else
          {
            config.window.borderless = true;
            SK_DeferCommand ("Window.Borderless 0");
          }

          if (borderless && fullscreen)
          {
            SK_ImGui_AdjustCursor ();
          }
        } break;

        default:
        case DISPLAY_MODE_FULLSCREEN:
        {
          rb.requestFullscreenMode (force);
        } break;
      }
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip ("This may lockup non-compliant graphics engines;"
                         " save your game first!");
    }

    ImGui::SameLine ();

    if (ImGui::Checkbox ("Force Override", &force))
    {
      switch (mode)
      {
        case DISPLAY_MODE_WINDOWED:
          config.display.force_fullscreen = false;
          config.display.force_windowed   = force;
          break;

        default:
        case DISPLAY_MODE_FULLSCREEN:
          config.display.force_fullscreen = force;
          config.display.force_windowed   = false;
          break;
      }
    }
  }


  if ( static_cast <int> (rb.api) &
       static_cast <int> (SK_RenderAPI::D3D11) ||
       static_cast <int> (rb.api) &
       static_cast <int> (SK_RenderAPI::D3D12) )
  {
    if (mode == DISPLAY_MODE_FULLSCREEN)
    {
      mode      = std::min (std::max (config.render.dxgi.scaling_mode + 1, 0), 3);
      orig_mode = mode;

      modes = "Application Preference\0Unspecified\0Centered\0Stretched\0\0";

      if (ImGui::Combo ("Scaling Mode###SubMenu_DisplayScaling_Combo", &mode, modes, 4) && mode != orig_mode)
      {
        switch (mode)
        {
          default:
          case 0:
            config.render.dxgi.scaling_mode = -1;
            break;

          case 1:
            config.render.dxgi.scaling_mode = 0;
            break;
          case 2:
            config.render.dxgi.scaling_mode = 1;
            break;
          case 3:
            config.render.dxgi.scaling_mode = 2;
            break;
        }

        rb.requestFullscreenMode (force);
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ( );
        ImGui::Text         ( "Override Scaling Mode" );
        ImGui::Separator    ( );
        ImGui::BulletText   ( "Set to Unspecified to CORRECTLY run "
                              "Fullscreen Display Native Resolution" );
        ImGui::EndTooltip   ( );
      }

      static DXGI_SWAP_CHAIN_DESC
                    last_swapDesc = { };

      static auto& io =
        ImGui::GetIO ();

      auto _ComputeRefresh =
        [&](const std::pair <UINT, UINT>& fractional) ->
        long double
        {
          return
            gsl::narrow_cast <long double> (fractional.first)/
            gsl::narrow_cast <long double> (fractional.second);
        };

      static std::string           combo_str;
      static UINT                  num_modes    =  0;
      static int                   current_item = -1;
      static std::vector
            <   DXGI_MODE_DESC   > dxgi_modes;
      static std::map   < UINT,
              std::pair < UINT,
                          UINT > > refresh_rates;

      static std::vector
        <    std::pair < NV_COLOR_FORMAT,
                         NV_BPC         >
        >                          nv_color_encodings;
      static std::string           nv_color_combo;
      static int                   nv_color_idx   =  0;

      SK_ComQIPtr <IDXGISwapChain>  pSwapChain  (rb.swapchain);
      SK_ComQIPtr <IDXGISwapChain3> pSwapChain3 (rb.swapchain);

      DXGI_SWAP_CHAIN_DESC    swapDesc  = { };
      pSwapChain->GetDesc   (&swapDesc);
      DXGI_SWAP_CHAIN_DESC1   swapDesc1 = { };
      pSwapChain3->GetDesc1 (&swapDesc1);

      static int orig_item =
        current_item;

      // Re-build the set of options if resolution changes,
      //   or if the swapchain buffer format does
      if ( ! ( last_swapDesc.BufferDesc.Width                   == swapDesc1.Width                           &&
               last_swapDesc.BufferDesc.Height                  == swapDesc1.Height                          &&
               last_swapDesc.BufferDesc.Format                  == swapDesc1.Format                          &&
               last_swapDesc.BufferDesc.RefreshRate.Numerator   == swapDesc.BufferDesc.RefreshRate.Numerator &&
               last_swapDesc.BufferDesc.RefreshRate.Denominator == swapDesc.BufferDesc.RefreshRate.Denominator )
         )
      {
        last_swapDesc.BufferDesc.Width  =
             swapDesc.BufferDesc.Width;
        last_swapDesc.BufferDesc.Height =
             swapDesc.BufferDesc.Height;
        last_swapDesc.BufferDesc.Format =
                       swapDesc1.Format;
        last_swapDesc.BufferDesc.RefreshRate.Numerator =
             swapDesc.BufferDesc.RefreshRate.Numerator;
        last_swapDesc.BufferDesc.RefreshRate.Denominator =
             swapDesc.BufferDesc.RefreshRate.Denominator;

        num_modes            = 0;
        combo_str.clear     ( );
        refresh_rates.clear ( );
        dxgi_modes.resize   (0);

        nv_color_combo.clear      ( );
        nv_color_encodings.resize (0);

        if ( rb.hdr_capable && rb.scanout.nvapi_hdr.active )
        {
          nv_color_encodings.emplace_back (
            std::make_pair (NV_COLOR_FORMAT_AUTO,   NV_BPC_DEFAULT)
          );
          nv_color_encodings.emplace_back (
            std::make_pair (NV_COLOR_FORMAT_RGB,    NV_BPC_8)
          );
          nv_color_encodings.emplace_back (
            std::make_pair (NV_COLOR_FORMAT_YUV444, NV_BPC_8)
          );
          nv_color_encodings.emplace_back (
            std::make_pair (NV_COLOR_FORMAT_YUV422, NV_BPC_8)
          );

          if (rb.scanout.nvapi_hdr.color_support_hdr.supports_10b_12b_444 & 0x1)
          {
            nv_color_encodings.emplace_back (
              std::make_pair (NV_COLOR_FORMAT_RGB, NV_BPC_10)
            );
            nv_color_encodings.emplace_back (
              std::make_pair (NV_COLOR_FORMAT_YUV444, NV_BPC_10)
            );
          }

          if ( rb.scanout.nvapi_hdr.color_support_hdr.supports_YUV422_12bit )
          {
            nv_color_encodings.emplace_back (
              std::make_pair (NV_COLOR_FORMAT_YUV422, NV_BPC_12)
            );
          }

          if (rb.scanout.nvapi_hdr.color_support_hdr.supports_10b_12b_444 & 0x2)
          {
            nv_color_encodings.emplace_back (
              std::make_pair (NV_COLOR_FORMAT_YUV444, NV_BPC_12)
            );
            nv_color_encodings.emplace_back (
              std::make_pair (NV_COLOR_FORMAT_RGB,    NV_BPC_12)
            );
          }
        }

        else if (sk::NVAPI::nv_hardware)
        {
#define _ValidateColorCombo(fmt,bpc) \
               (NVAPI_OK == SK_NvAPI_CheckColorSupport_SDR (0, (bpc), (fmt)))

          nv_color_encodings.emplace_back (std::make_pair (NV_COLOR_FORMAT_AUTO, NV_BPC_DEFAULT));

          static const
            std::pair <NV_COLOR_FORMAT, NV_BPC>
              combos_to_test [] =
              {
                { NV_COLOR_FORMAT_RGB,    NV_BPC_6  },
                { NV_COLOR_FORMAT_YUV420, NV_BPC_6  },
                { NV_COLOR_FORMAT_YUV422, NV_BPC_6  },
                { NV_COLOR_FORMAT_YUV444, NV_BPC_6  },

                { NV_COLOR_FORMAT_RGB,    NV_BPC_8  },
                { NV_COLOR_FORMAT_YUV420, NV_BPC_8  },
                { NV_COLOR_FORMAT_YUV422, NV_BPC_8  },
                { NV_COLOR_FORMAT_YUV444, NV_BPC_8  },

                { NV_COLOR_FORMAT_RGB,    NV_BPC_10 },
                { NV_COLOR_FORMAT_YUV420, NV_BPC_10 },
                { NV_COLOR_FORMAT_YUV422, NV_BPC_10 },
                { NV_COLOR_FORMAT_YUV444, NV_BPC_10 },

                { NV_COLOR_FORMAT_RGB,    NV_BPC_12 },
                { NV_COLOR_FORMAT_YUV420, NV_BPC_12 },
                { NV_COLOR_FORMAT_YUV422, NV_BPC_12 },
                { NV_COLOR_FORMAT_YUV444, NV_BPC_12 },

                { NV_COLOR_FORMAT_RGB,    NV_BPC_16 },
                { NV_COLOR_FORMAT_YUV420, NV_BPC_16 },
                { NV_COLOR_FORMAT_YUV422, NV_BPC_16 },
                { NV_COLOR_FORMAT_YUV444, NV_BPC_16 }
              };

          NvAPI_Status
          SK_NvAPI_CheckColorSupport_SDR (
            NvU32                displayId,
            NV_BPC               bpcTest,
            NV_COLOR_FORMAT      formatTest      = NV_COLOR_FORMAT_AUTO,
            NV_DYNAMIC_RANGE     rangeTest       = NV_DYNAMIC_RANGE_AUTO,
            NV_COLOR_COLORIMETRY colorimetryTest = NV_COLOR_COLORIMETRY_AUTO );

          for ( auto& test_set : combos_to_test )
          {
            if (_ValidateColorCombo (test_set.first, test_set.second))
            {
              nv_color_encodings.emplace_back (
                               std::make_pair ( test_set.first, test_set.second ) );
            }
          }
        }

        auto _MakeColorEncodingComboList = [&](void) ->
        int
        {
          int match   = 0;//-1;
          int idx_cnt = 0;

          for ( auto& encoding : nv_color_encodings )
          {
            if ( encoding.first == NV_COLOR_FORMAT_AUTO && encoding.second == NV_BPC_DEFAULT )
            {
              nv_color_combo += "Driver Default\0";
                             ++ idx_cnt;
            }

            else
            {
              std::string partial_encoding;
              // Requires 2 valid components or we have no mode.
              int         validated_encode =  2;
              int         matching_encode  =  0;

              switch (encoding.second)
              {
                case    NV_BPC_6:  partial_encoding += " 6-bit"; break;
                case    NV_BPC_8:  partial_encoding += " 8-bit"; break;
                case    NV_BPC_10: partial_encoding += "10-bit"; break;
                case    NV_BPC_12: partial_encoding += "12-bit"; break;
                case    NV_BPC_16: partial_encoding += "16-bit"; break;
                default:           validated_encode --;   break;
              }

              switch (encoding.first)
              {
                case    NV_COLOR_FORMAT_RGB:
                  partial_encoding += " RGB\0";         break;
                case    NV_COLOR_FORMAT_YUV420:
                  partial_encoding += " YCbCr 4:2:0\0"; break;
                case    NV_COLOR_FORMAT_YUV422:
                  partial_encoding += " YCbCr 4:2:2\0"; break;
                case    NV_COLOR_FORMAT_YUV444:
                  partial_encoding += " YCbCr 4:4:4\0"; break;
                default:
                  validated_encode --;                  break;
              }

              matching_encode +=
                ( rb.scanout.nvapi_hdr.color_format == encoding.first );
              matching_encode +=
                ( rb.scanout.nvapi_hdr.bpc          == encoding.second );

              if (validated_encode == 2)
              {
                nv_color_combo += partial_encoding;

                if ( matching_encode == 2 )
                     match = idx_cnt;

                ++ idx_cnt;
              }
            }

            nv_color_combo += '\0';
          }

          return match;
        };

        nv_color_idx =
          _MakeColorEncodingComboList ();

        // -1 == Don't Care
        current_item = -1;

        SK_ComPtr <IDXGIOutput> pContainer;

        if (SUCCEEDED (pSwapChain->GetContainingOutput (&pContainer)))
        {
          pContainer->GetDisplayModeList ( swapDesc1.Format, 0x0,
                                           &num_modes, nullptr );

          dxgi_modes.resize (num_modes);

          if ( SUCCEEDED ( pContainer->GetDisplayModeList ( swapDesc1.Format, 0x0,
                                                            &num_modes, dxgi_modes.data () ) ) )
          {
            int idx = 1;

            combo_str += "No Override";
            combo_str += '\0';

            for ( auto& dxgi_mode : dxgi_modes )
            {
              if ( dxgi_mode.Format == swapDesc1.Format &&
                   dxgi_mode.Width  == swapDesc1.Width  &&
                   dxgi_mode.Height == swapDesc1.Height )
              {
                ///dll_log->Log ( L" ( %lux%lu -<+| %ws |+>- ) @ %f Hz",
                ///                             mode.Width,
                ///                             mode.Height,
                ///        SK_DXGI_FormatToStr (mode.Format).c_str (),
                ///              ( (long double)mode.RefreshRate.Numerator /
                ///                (long double)mode.RefreshRate.Denominator )
                ///             );

                UINT integer_refresh =
                  gsl::narrow_cast <UINT> (
                    std::ceil (
                        gsl::narrow_cast <long double> (dxgi_mode.RefreshRate.Numerator) /
                        gsl::narrow_cast <long double> (dxgi_mode.RefreshRate.Denominator)
                                          )
                    );

                if ( ! refresh_rates.count (integer_refresh) )
                {      refresh_rates       [integer_refresh] = std::make_pair (
                                                                 dxgi_mode.RefreshRate.Numerator,
                                                                   dxgi_mode.RefreshRate.Denominator );

                  // Exact Match -- Hurray!
                  if ( config.render.framerate.rescan_.Numerator == dxgi_mode.RefreshRate.Numerator  &&
                       config.render.framerate.rescan_.Denom     == dxgi_mode.RefreshRate.Denominator )
                  {
                    current_item = idx-2;
                  }

                  ++idx;
                }
              }
            }

            for ( auto& unique_refresh : refresh_rates )
            {
              combo_str +=
                SK_FormatString ("%6.02f Hz", _ComputeRefresh (unique_refresh.second)) + '\0';
            }

            // No Exact Match, but we can probably find something close...
            if ( -1 == current_item &&
                 -1 != gsl::narrow_cast <INT> (config.render.framerate.refresh_rate) )
            {
              int lvl2_idx = 1;

              for ( auto& nomnom : refresh_rates )
              {
                long double ldRefresh =
                  _ComputeRefresh (nomnom.second);

                if ( ldRefresh < gsl::narrow_cast <long double> (config.render.framerate.refresh_rate + 0.333f) &&
                     ldRefresh > gsl::narrow_cast <long double> (config.render.framerate.refresh_rate - 0.333f) )
                {
                  current_item = lvl2_idx-2;
                  break;
                }

                ++lvl2_idx;
              }
            }

            combo_str += '\0';
          }
        }
      }

      int     rel_item =
        ( current_item == -1 ? 0 : current_item );

      bool refresh_change =
        ImGui::Combo ( "Refresh Rate###SubMenu_RefreshRate_Combo",
                         &rel_item, combo_str.c_str () );

      if (refresh_change)
      {
        current_item =
          ( rel_item == 0 ? -1 : rel_item );

        if (current_item > 0)
        {
          const auto& map_entry = [&](int idx) ->
          const auto&
          {
            for ( const auto& entry : refresh_rates )
            {
              if (idx-- == 0) return entry;
            }

            return
              *refresh_rates.crbegin ();
          };

          auto& refresh =
            map_entry (current_item - 1);

          config.render.framerate.rescan_.Numerator = refresh.second.first;
          config.render.framerate.rescan_.Denom     = refresh.second.second;
          config.render.framerate.refresh_rate      =
            gsl::narrow_cast <float> (
              _ComputeRefresh (refresh.second)
            );
          config.render.framerate.rescan_ratio      =
            SK_FormatStringW ( L"%lu/%lu", config.render.framerate.rescan_.Numerator,
                                           config.render.framerate.rescan_.Denom );

          // Now comes the magic, try to apply the refresh rate immediately and not crash...
          rb.requestFullscreenMode ();
        }

        // Don't Care = No Override --> Do Nothing on-screen, but store "No Override" in cfg
        else
        {
          config.render.framerate.refresh_rate      =     -1.0f;
          config.render.framerate.rescan_ratio      =   L"-1/1";
          config.render.framerate.rescan_.Numerator = UINT (-1);
          config.render.framerate.rescan_.Denom     = UINT ( 1);
        }

        orig_item = current_item;

        SK_SaveConfig ();
      }

      if (! nv_color_encodings.empty ())
      {
        if ( ImGui::Combo ( "Color Encoding###SubMenu_HDREncode_Combo",
                              &nv_color_idx,
                               nv_color_combo.c_str    (),
                          gsl::narrow_cast <int> (
                               nv_color_encodings.size ()
                                                 )
                           )
           )
        {
          if ( rb.hdr_capable   &&
               rb.driver_based_hdr )
          {
            if ( rb.scanout.nvapi_hdr.setColorEncoding_HDR (
                   nv_color_encodings [nv_color_idx].first,
                   nv_color_encodings [nv_color_idx].second ) )
            { }
          }

          else
          {
            if ( rb.scanout.nvapi_hdr.setColorEncoding_SDR (
                   nv_color_encodings [nv_color_idx].first,
                   nv_color_encodings [nv_color_idx].second ) )
            { }
          }

          // Trigger a re-enumeration of available modes.
          last_swapDesc.BufferDesc.Width = 0;
        }
      }
    }
  }

  if (! rb.fullscreen_exclusive)
  {
    mode      = config.window.borderless ? config.window.fullscreen ? 2 : 1 : 0;
    orig_mode = mode;

    bool window_is_borderless =
      (! SK_Window_HasBorder (game_window.hWnd));

    // No user-defined override, so let's figure out the game's current state
    if (mode == 0)
    {
      // Borderless
      if (window_is_borderless)
      {
        if (! SK_Window_IsFullscreen (game_window.hWnd))
          mode = 1; // Just Borderless, not fullscreen
        else
          mode = 2; // Fullscreen Borderless
      }
    }

    modes = (config.window.borderless || mode == 2)     ?
      "Bordered\0Borderless\0Borderless Fullscreen\0\0" :
      "Bordered\0Borderless\0\0";

    if (ImGui::Combo ("Window Style###SubMenu_WindowBorder_Combo", &mode, modes) &&
                                                                    mode != orig_mode)
    {
      switch (mode)
      {
        case 0:
          config.window.borderless = false;
          break;

        case 2:
          config.window.borderless = true;
          config.window.fullscreen = true;
          break;

        case 1:
          config.window.borderless = true;
          config.window.fullscreen = false;
          break;
      }

      SK_ImGui_AdjustCursor ();

      if (config.window.borderless != window_is_borderless)
      {
        config.window.borderless = window_is_borderless;
        config.window.fullscreen = false;

        SK_DeferCommand ("Window.Borderless toggle");
      }
    }

    ImGui::Separator ();

    SK_Display_ResolutionSelectUI ();
  }
}


extern float __target_fps;
extern float __target_fps_bg;

extern void SK_ImGui_DrawGraph_FramePacing (void);
extern void SK_ImGui_DrawGraph_Latency     (void);
extern void SK_ImGui_DrawConfig_Latency    (void);

void
SK_NV_LatencyControlPanel (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (sk::NVAPI::nv_hardware && ( rb.api == SK_RenderAPI::D3D11 ||
                                  rb.api == SK_RenderAPI::D3D12 ))
  {
    ImGui::Separator  ();
    ImGui::Text       ("NVIDIA Driver Black Magic");
    ImGui::TreePush   ();

    SK_ImGui_DrawConfig_Latency ();
    SK_ImGui_DrawGraph_Latency  ();

    ImGui::TreePop    ();
  }
}


__declspec (dllexport)
bool
SK_ImGui_ControlPanel (void)
{
  if (! imgui_staged_frames)
    return false;

  static auto& io =
    ImGui::GetIO ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (ImGui::GetFont () == nullptr)
  {
    dll_log->Log (L"[   ImGui   ]  Fatal Error:  No Font Loaded!");
    return false;
  }


  bool windowed =
    (! rb.fullscreen_exclusive);

  auto DisplayMenu =
    [&](void)
    {
      //if (ImGui::MenuItem ("Force-Inject Steam Overlay", "", nullptr))
        //SK_Steam_LoadOverlayEarly ();

      //ImGui::Separator ();
      bool nav_moves_mouse =
        io.ConfigFlags & ImGuiConfigFlags_NavEnableSetMousePos;

      if (
        ImGui::MenuItem ("Virtual Gamepad/Keyboard Cursor", "", &nav_moves_mouse)
      )
      {
        io.ConfigFlags = nav_moves_mouse ?
          ( io.ConfigFlags |  ImGuiConfigFlags_NavEnableSetMousePos ) :
          ( io.ConfigFlags & ~ImGuiConfigFlags_NavEnableSetMousePos );
      }

      ImGui::MenuItem ("Display Active Input APIs",       "", &config.imgui.show_input_apis);


      // TEMP HACK: NvAPI does not support G-Sync Status in D3D12
      if (rb.api != SK_RenderAPI::D3D12)
      {
        if (config.apis.NvAPI.enable && sk::NVAPI::nv_hardware)
        {
          //ImGui::TextWrapped ("%ws", SK_NvAPI_GetGPUInfoStr ().c_str ());
          ImGui::MenuItem    ("Display G-Sync Status",     "", &config.apis.NvAPI.gsync_status);
        }
      }

      ImGui::MenuItem  ("Display Playtime in Title",     "", &config.steam.show_playtime);
      ImGui::MenuItem  ("Display Mac-style Menu at Top", "", &config.imgui.use_mac_style_menu);
      ImGui::Separator ();

      DisplayModeMenu (windowed);
    };


  auto SpecialK_Menu =
    [&](void)
    {
      if (ImGui::BeginMenu ("File"))
      {
        static HMODULE hModReShade      = SK_ReShade_GetDLL ();
        static bool    bIsReShadeCustom =
                          ( hModReShade != nullptr &&

          SK_RunLHIfBitness ( 64, GetProcAddress (hModReShade, "?SK_ImGui_DrawCallback@@YAIPEAX@Z"),
                                  GetProcAddress (hModReShade, "?SK_ImGui_DrawCallback@@YGIPAX@Z" ) ) );

        if (ImGui::MenuItem ( "Browse Game Directory", "" ))
        {
          SK_ShellExecuteW ( nullptr, L"explore", SK_GetHostPath (), nullptr, nullptr, SW_NORMAL);
        }

#if 0
        if (SK::SteamAPI::AppID () != 0 && SK::SteamAPI::GetDataDir () != "")
        {
          if (ImGui::BeginMenu ("Browse Steam Directories"))
          {
            if (ImGui::MenuItem ("Cloud Data", ""))
            {
              SK_ShellExecuteA ( nullptr, "explore",
                                   SK::SteamAPI::GetDataDir ().c_str (),
                                     nullptr, nullptr, SW_NORMAL );
            }

            if (ImGui::MenuItem ("Cloud Config", ""))
            {
              SK_ShellExecuteA ( nullptr, "explore",
                                   SK::SteamAPI::GetConfigDir ().c_str (),
                                     nullptr, nullptr, SW_NORMAL );
            }

            ImGui::EndMenu ();
          }
        }
#endif

        ImGui::Separator ();

        if (ImGui::MenuItem ( "Browse Special K Logs", "" ))
        {
          std::wstring log_dir =
            std::wstring (SK_GetConfigPath ()) + LR"(\logs)";

          SK_ShellExecuteW ( nullptr, L"explore", log_dir.c_str (),
                               nullptr, nullptr, SW_NORMAL );
        }

        bool supports_texture_mods =
        //( static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9)  ) ||
          ( static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11) );

        if (supports_texture_mods)
        {
          static bool bHasTextureMods =
            ( INVALID_FILE_ATTRIBUTES !=
                GetFileAttributesW (SK_D3D11_res_root->c_str ()) );

          if (bHasTextureMods)
          {
            SK::ControlPanel::D3D11::TextureMenu ();
          }

          else
          {
            if (ImGui::MenuItem ("Initialize Texture Mods", "", nullptr))
            {
              wchar_t      wszPath [MAX_PATH + 2] = { };
              wcsncpy_s   (wszPath, MAX_PATH,
                 SK_D3D11_res_root->c_str (), _TRUNCATE);
              PathAppendW (wszPath, LR"(inject\textures\)");

              SK_CreateDirectories (wszPath);

              bHasTextureMods =
                ( INVALID_FILE_ATTRIBUTES !=
                    GetFileAttributesW (SK_D3D11_res_root->c_str ()) );
            }
          }
        }

        if (bIsReShadeCustom && ImGui::MenuItem ( "Browse ReShade Assets", "", nullptr ))
        {
          std::wstring reshade_dir =
            std::wstring (SK_GetConfigPath ()) + LR"(\ReShade)";

          static bool dir_exists =
            PathIsDirectory ( std::wstring (
                                std::wstring ( SK_GetConfigPath () ) +
                                  LR"(\ReShade\Shaders\)"
                              ).c_str () );

          if (! dir_exists)
          {
            SK_CreateDirectories ( std::wstring (
                                     std::wstring ( SK_GetConfigPath () ) +
                                       LR"(\ReShade\Shaders\)"
                                   ).c_str () );
            SK_CreateDirectories ( std::wstring (
                                     std::wstring ( SK_GetConfigPath () ) +
                                       LR"(\ReShade\Textures\)"
                                   ).c_str () );

            dir_exists = true;
          }

          SK_ShellExecuteW ( nullptr, L"explore",
                               reshade_dir.c_str (), nullptr, nullptr,
                                 SW_NORMAL );
        }

        static bool wrappable = true;

        if (SK_IsInjected () && wrappable)
        {
          ImGui::Separator ();

          if (ImGui::MenuItem ("Install Wrapper DLL for this game"))
          {
            if (SK_Inject_SwitchToRenderWrapper ())
              wrappable = false;
          }
        }

        else if (wrappable)
        {
          ImGui::Separator ();

          if (ImGui::MenuItem ("Uninstall Wrapper DLL for this game"))
          {
            wrappable =
              SK_Inject_SwitchToGlobalInjector ();
          }
        }

        ////////if ( (! SK::SteamAPI::AppID ())          &&
        ////////     (! config.steam.force_load_steamapi) )
        ////////{
        ////////  ImGui::Separator ();
        ////////
        ////////  if (ImGui::MenuItem ("Manually Inject SteamAPI", ""))
        ////////  {
        ////////    config.steam.force_load_steamapi = true;
        ////////    SK_Steam_KickStart  ();
        ////////
        ////////    if (SK_GetFramesDrawn () > 1)
        ////////      SK_ApplyQueuedHooks ();
        ////////  }
        ////////}

        ImGui::Separator ();
        //if (ImGui::MenuItem ("Unload Special K", "EXPERIMENTAL"))
        //{
        //  bool
        //  __stdcall
        //  SK_ShutdownCore (const wchar_t* backend);
        //  extern const wchar_t* __SK_BootedCore;
        //
        //  SK_ShutdownCore (__SK_BootedCore);
        //}

        if (ImGui::MenuItem ("Exit Game", "Alt+F4"))
        {
          SK_ImGui_WantExit = true;
        }

        ImGui::EndMenu  ();
      }

      if (ImGui::BeginMenu ("Display"))
      {
        DisplayMenu    ();
        ImGui::EndMenu ();
      }

      auto HDRMenu =
      [&](void)
      {
        float imgui_nits =
          rb.ui_luminance / 1.0_Nits;

        if ( ImGui::SliderFloat ( "Special K Luminance###IMGUI_LUMINANCE",
                                   &imgui_nits,
                                    80.0f, rb.display_gamut.maxLocalY,
                                      (const char *)u8"%.1f cd/m²" ) )
        {
          rb.ui_luminance =
               imgui_nits * 1.0_Nits;

          SK_SaveConfig ();
        }

#define STEAM_OVERLAY_VS_CRC32C 0xf48cf597
#define UPLAY_OVERLAY_PS_CRC32C 0x35ae281c

        static bool    steam_overlay = false;
        static ULONG64 first_try     = SK_GetFramesDrawn ();

        auto pDevice =
          rb.getDevice <ID3D11Device> ();

        if ((! steam_overlay) && ((SK_GetFramesDrawn () - first_try) < 240))
        {      steam_overlay =
          SK_D3D11_IsShaderLoaded <ID3D11VertexShader> (pDevice,
            STEAM_OVERLAY_VS_CRC32C
          );
        }

        if (steam_overlay)
        {
          float steam_nits =
            config.steam.overlay_hdr_luminance / 1.0_Nits;

          if ( ImGui::SliderFloat ( "Steam Overlay Luminance###STEAM_LUMINANCE",
                                     &steam_nits,
                                      80.0f, rb.display_gamut.maxAverageY,
                                        (const char *)u8"%.1f cd/m²" ) )
          {
            config.steam.overlay_hdr_luminance =
                                    steam_nits * 1.0_Nits;

            SK_SaveConfig ();
          }
        }

        if (config.rtss.present)
        {
          float rtss_nits =
            config.rtss.overlay_luminance / 1.0_Nits;

          if ( ImGui::SliderFloat ( "RTSS Overlay Luminance###RTSS_LUMINANCE",
                                     &rtss_nits,
                                      80.0f, rb.display_gamut.maxAverageY,
                                        (const char *)u8"%.1f cd/m²" ) )
          {
            config.rtss.overlay_luminance =
                                    rtss_nits * 1.0_Nits;

            SK_SaveConfig ();
          }
        }

        if (config.discord.present)
        {
          float discord_nits =
            config.discord.overlay_luminance / 1.0_Nits;

          if ( ImGui::SliderFloat ( "Discord Overlay Luminance###DISCORD_LUMINANCE",
                                     &discord_nits,
                                      80.0f, rb.display_gamut.maxAverageY,
                                        (const char *)u8"%.1f cd/m²" ) )
          {
            config.discord.overlay_luminance =
                                    discord_nits * 1.0_Nits;

            SK_SaveConfig ();
          }
        }

        static bool uplay_overlay = false;

        if ((! uplay_overlay) && ((SK_GetFramesDrawn () - first_try) < 240))
        {      uplay_overlay =
          SK_D3D11_IsShaderLoaded <ID3D11PixelShader> (pDevice,
            UPLAY_OVERLAY_PS_CRC32C
          );
        }

        if (uplay_overlay)
        {
          float uplay_nits =
            config.uplay.overlay_luminance / 1.0_Nits;

          if ( ImGui::SliderFloat ( "uPlay Overlay Luminance###UPLAY_LUMINANCE",
                                     &uplay_nits,
                                      80.0f, rb.display_gamut.maxAverageY,
                                        (const char *)u8"%.1f cd/m²" ) )
          {
            config.uplay.overlay_luminance =
                                    uplay_nits * 1.0_Nits;

            SK_SaveConfig ();
          }
        }

        ImGui::Separator ();

        bool hdr_changed =
            ImGui::Checkbox ( "Keep Full-Range JPEG-XR HDR Screenshots (.JXR)",
                                &config.screenshots.png_compress );

        if ( rb.screenshot_mgr.getRepoStats ().files > 0 )
        {
          const SK_ScreenshotManager::screenshot_repository_s& repo =
            rb.screenshot_mgr.getRepoStats (hdr_changed);

          ImGui::BeginGroup (  );
          ImGui::TreePush   ("");
          ImGui::Text ( "%lu files using %ws",
                          repo.files,
                            SK_File_SizeToString (repo.liSize.QuadPart).c_str  ()
                      );

          ImGui::SameLine ();

          if (ImGui::Button ("Browse"))
          {
            SK_ShellExecuteW ( nullptr,
              L"explore",
                rb.screenshot_mgr.getBasePath (),
                  nullptr, nullptr,
                        SW_NORMAL
            );
          }

          ImGui::TreePop  ();
          ImGui::EndGroup ();
        }

        if (hdr_changed)
          SK_SaveConfig ();

        ImGui::Separator ();

        bool hdr =
          SK_ImGui_Widgets->hdr_control->isVisible ();

        if (ImGui::Checkbox ("HDR Widget", &hdr))
        {
          SK_ImGui_Widgets->hdr_control->setVisible (hdr).setActive (hdr);
        }

        ImGui::SameLine ();

        ImGui::TextColored ( ImColor::HSV (0.07f, 0.8f, .9f),
                               "For advanced users; experimental" );
      };

      if ( rb.isHDRCapable ()  &&
           rb.isHDRActive  () )
      {
        if (ImGui::BeginMenu ("HDR"))
        {
          HDRMenu ();
          ImGui::EndMenu ();
        }
      }



      auto PopulateBranches = [](auto branches) ->
        std::map <std::string, SK_BranchInfo>
        {
          std::map <std::string, SK_BranchInfo> details;

          for ( auto& it : branches )
          {
            details.emplace ( it,
              SK_Version_GetLatestBranchInfo (nullptr, it.c_str ())
            );
          }

          return details;
        };

      static std::vector <std::string>                branches       =
        SK_Version_GetAvailableBranches (nullptr);
      static std::map    <std::string, SK_BranchInfo> branch_details =
                       PopulateBranches (branches);

      static SK_VersionInfo vinfo =
        SK_Version_GetLocalInfo (nullptr);

      static char current_ver        [128] = { };
      static char current_branch_str [ 64] = { };

      snprintf ( current_ver,        127, "%ws (%li)",
                   vinfo.package.c_str (), vinfo.build );
      snprintf ( current_branch_str,  63, "%ws",
                   vinfo.branch.c_str  () );

      static SK_VersionInfo vinfo_latest =
        SK_Version_GetLatestInfo (nullptr);

      static SK_BranchInfo_V1 current_branch =
        SK_Version_GetLatestBranchInfo (
          nullptr, SK_WideCharToUTF8 (vinfo.branch).c_str ()
        );

#if 0
      bool updatable =
        ( SK_GetPluginName ().find (L"Special K") == std::wstring::npos ||
          SK_IsInjected    () );

      if (ImGui::BeginMenu ("Update"))
      {
        bool selected = false;

        ImGui::MenuItem  ( "Current Version###Menu_CurrentVersion",
                             current_ver, &selected, false );

        if (updatable && branches.size () > 1)
        {
          static
            char    szCurrentBranchMenu [128] = { };
          sprintf ( szCurrentBranchMenu, "Current Branch:  (%s)"
                                         "###SelectBranchMenu",
                      current_branch_str );

          if (ImGui::BeginMenu (szCurrentBranchMenu))
          {
            for ( auto& it : branches )
            {
              selected = ( SK_UTF8ToWideChar (it)._Equal (
                             current_branch.release.vinfo.branch )
                         );

              static std::string branch_desc;
                                 branch_desc =
                SK_WideCharToUTF8 (branch_details [it].general.description);

              if ( ImGui::MenuItem ( it.c_str (), branch_desc.c_str (),
                                                              &selected ) )
              {
                SK_Version_SwitchBranches (nullptr, it.c_str ());

                // Re-fetch the version info and then go to town updating stuff ;)
                SK_FetchVersionInfo1 (nullptr, true);

                branches       = SK_Version_GetAvailableBranches (nullptr);
                vinfo          = SK_Version_GetLocalInfo         (nullptr);
                vinfo_latest   = SK_Version_GetLatestInfo        (nullptr);
                current_branch = SK_Version_GetLatestBranchInfo  (nullptr,
                         SK_WideCharToUTF8 (vinfo_latest.branch).c_str ());
                branch_details = PopulateBranches (branches);

                // !!! Handle the case where the number of branches changes after we fetch the repo
                break;
              }

              else if (ImGui::IsItemHovered ())
              {
                static std::wstring title;
                                    title =
                branch_details [it].release.title;

                ImGui::BeginTooltip ();
                ImGui::Text         ("%ws", title.c_str ());
                //ImGui::Separator    ();
                //ImGui::BulletText   ("Build: %li", branch_details [it].release.vinfo.build);
                ImGui::EndTooltip   ();
              }
            }

            ImGui::Separator ();

            ImGui::TreePush       ("");
            ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.125f, 0.9f, 0.75f));
            ImGui::Text           ("Most of my projects have branches that pre-date this menu...");
            ImGui::BulletText     ("Changing branches here may be a one-way trip :)");
            ImGui::PopStyleColor  ();
            ImGui::TreePop        ();

            ImGui::EndMenu ();
          }
        }

        else
        {

        }

        ImGui::MenuItem  ( "Current Branch###Menu_CurrentBranch",
                             current_branch_str, &selected, false );

        ImGui::Separator ();

        if (vinfo.build < vinfo_latest.build)
        {
          if (ImGui::MenuItem  ("Update Now"))
            SK_UpdateSoftware (nullptr);

          ImGui::Separator ();
        }

        static std::string utf8_time_checked;
                           utf8_time_checked =
          SK_WideCharToUTF8 (SK_Version_GetLastCheckTime_WStr ());

        snprintf        ( current_ver, 127, "%ws (%li)",
                            vinfo_latest.package.c_str (),
                            vinfo_latest.build );
        ImGui::MenuItem ( "Latest Version###Menu_LatestVersion",
                            current_ver, &selected, false );
        ImGui::MenuItem ( "Last Checked###Menu_LastUpdateCheck",
                          utf8_time_checked.c_str (), &selected, false );

        enum {
          SixHours    = 0,
          TwelveHours = 1,
          OneDay      = 2,
          OneWeek     = 3,
          Never       = 4
        };

        const ULONGLONG _Hour = 36000000000ULL;

        auto GetFrequencyPreset = [&] (void) -> int {
          uint64_t freq = SK_Version_GetUpdateFrequency (nullptr);

          if (freq == 0 || freq == MAXULONGLONG)
            return Never;

          if (freq <= (6 * _Hour))
            return SixHours;

          if (freq <= (12 * _Hour))
            return TwelveHours;

          if (freq <= (24 * _Hour))
            return OneDay;

          if (freq <= (24 * 7 * _Hour))
            return OneWeek;

          return Never;
        };

        static int sel = GetFrequencyPreset ();

        if (updatable)
        {
          ImGui::Text     ("Check for Updates");
          ImGui::TreePush ("");

          if ( ImGui::Combo ( "###UpdateCheckFreq", &sel,
                                "Once every 6 hours\0"
                                "Once every 12 hours\0"
                                "Once per-day\0"
                                "Once per-week\0"
                                "Never (disable)\0\0" ) )
          {
            switch (sel)
            {
              default:
              case SixHours:
                SK_Version_SetUpdateFrequency (nullptr,      6 * _Hour);
                break;
              case TwelveHours:
                SK_Version_SetUpdateFrequency (nullptr,     12 * _Hour);
                break;
              case OneDay:
                SK_Version_SetUpdateFrequency (nullptr,     24 * _Hour);
                break;
              case OneWeek:
                SK_Version_SetUpdateFrequency (nullptr, 7 * 24 * _Hour);
                break;
              case Never:
                SK_Version_SetUpdateFrequency (nullptr,              0);
                break;
            }
          }

          ImGui::TreePop ( );

          if (vinfo.build >= vinfo_latest.build)
          {
            if (ImGui::MenuItem  (" >> Check Now"))
            {
              SK_FetchVersionInfo1 (nullptr, true);
              branches       = SK_Version_GetAvailableBranches (nullptr);
              vinfo          = SK_Version_GetLocalInfo         (nullptr);
              vinfo_latest   = SK_Version_GetLatestInfo        (nullptr);
              branch_details = PopulateBranches                (branches);

              if (vinfo.build < vinfo_latest.build)
                SK_Version_ForceUpdateNextLaunch (nullptr);
            }
          }
        }

        ImGui::EndMenu ();
      }

      SK::ControlPanel::Steam::DrawMenu ();
#endif


      if (ImGui::BeginMenu ("Help"))
      {
        bool selected = false;

        //ImGui::MenuItem ("Special K Documentation (Work in Progress)", "Ctrl + Shift + Nul", &selected, false);

        ImGui::TreePush ("");
        {
          if (ImGui::MenuItem (R"("Kaldaien's Mod")", "Discourse Forums", &selected, true))
            SK_SteamOverlay_GoToURL ("https://discourse.differentk.fyi/", true);
        }
        ImGui::TreePop ();

        ImGui::Separator ();

        if (ImGui::BeginMenu ("Support Us##Donate"))
        {
          if (ImGui::MenuItem ( "Recurring Donation + Perks", "Become a Patron", &selected ))
          {
            SK_SteamOverlay_GoToURL (
                "https://www.patreon.com/bePatron?u=33423623", true
            );
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::TextColored  (ImColor::HSV (.963f, .789f, .989f), "Patreon Benefits");
            ImGui::Separator    ();
            ImGui::BulletText   ("The names of mid-tier or higher patrons are permanently displayed in SKIF");
            ImGui::BulletText   ("Access to special Discord channels and prioritized support");
            ImGui::Spacing      ();
            ImGui::TextColored  (ImColor::HSV (.432f, .789f, .999f), "Donations Pay For");
            ImGui::Separator    ();
            ImGui::BeginGroup   ();
            ImGui::BulletText   ("Web Services:  ");
            ImGui::BulletText   ("Code Services: ");
            ImGui::BulletText   ("Game Testing:  ");
            ImGui::EndGroup     ();
            ImGui::SameLine     ();
            ImGui::BeginGroup   ();
            ImGui::Text         ("Wiki, Discourse, GitLab, CDN storage, Dedicated E-Mail Server");
            ImGui::Text         ("Code signing, disassemblers, debuggers, static analysis...");
            ImGui::Text         ("Games often have to be purchased for compatibility testing...");
            ImGui::EndGroup     ();
            ImGui::Separator    ();
            ImGui::EndTooltip   ();
          }

          if (ImGui::MenuItem ( "One-Time Donation", "Donate with PayPal", &selected ))
          {
            SK_SteamOverlay_GoToURL (
                "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=8A7FSUFJ6KB2U", true
            );
          }

          ImGui::EndMenu        ();
        }

        if (ImGui::MenuItem ( "Documentation", "Official Wiki", &selected ))
        {
          SK_SteamOverlay_GoToURL (
              "https://wiki.special-k.info", true
          );
        }


        if ( ImGui::MenuItem ( "Release History",
                                 "Downloads",
                                   &selected
                             )
           )
        {
          SK_SteamOverlay_GoToURL ("https://discord.com/invite/ER4EDBJPTa",
              true
          );
        }

        if (ImGui::MenuItem ("About this Software...", "Licensed Software", &selected))
          eula.show = true;

        ImGui::Separator ();

        if (SK_SteamAPI_AppID () != 0x0)
        {
          if (ImGui::MenuItem ( "Check PCGamingWiki for this Game", "Third-Party Site", &selected ))
          {
            SK_SteamOverlay_GoToURL (
              SK_FormatString (
                "http://pcgamingwiki.com/api/appid.php?appid=%lu",
                                           SK_SteamAPI_AppID ()
              ).c_str (), true
            );
          }
        }

        if (sk::NVAPI::nv_hardware)
        {
          extern INT  SK_NvAPI_GetAnselEnablement (DLL_ROLE role);
          extern BOOL SK_NvAPI_EnableAnsel        (DLL_ROLE role);
          extern BOOL SK_NvAPI_DisableAnsel       (DLL_ROLE role);

          static INT enablement =
            SK_NvAPI_GetAnselEnablement (SK_GetDLLRole ());

#ifndef _WIN64
          static HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi.dll");
#else
          static HMODULE hLib = SK_Modules->LoadLibraryLL (L"nvapi64.dll");
#endif
#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
          typedef void* (*NvAPI_QueryInterface_pfn)(unsigned int offset);
          typedef NvAPI_Status(__cdecl *NvAPI_RestartDisplayDriver_pfn)(void);
          static NvAPI_QueryInterface_pfn          NvAPI_QueryInterface       =
            (NvAPI_QueryInterface_pfn)SK_GetProcAddress (hLib, "nvapi_QueryInterface");
          static NvAPI_RestartDisplayDriver_pfn NvAPI_RestartDisplayDriver = NvAPI_QueryInterface == nullptr ?
                                                                                                     nullptr :
            (NvAPI_RestartDisplayDriver_pfn)NvAPI_QueryInterface (__NvAPI_RestartDisplayDriver);

          if (NvAPI_RestartDisplayDriver != nullptr)
          {
            if (ImGui::MenuItem ("Restart NVIDIA Display Driver", "No Reboot Required", nullptr))
              NvAPI_RestartDisplayDriver ();

            if (ImGui::IsItemHovered ())
              ImGui::SetTooltip ("NVIDIA display buggers can be restarted on-the-fly; may take a few seconds.");
          }

          if (enablement >= 0)
          {
            switch (enablement)
            {
              default:
              case 0:
                if (ImGui::MenuItem ("Enable Ansel for this Game", "", nullptr))
                {
                  SK_NvAPI_EnableAnsel (SK_GetDLLRole ());
                  enablement =
                    SK_NvAPI_GetAnselEnablement (SK_GetDLLRole ());
                } break;

              case 1:
                if (ImGui::MenuItem ("Disable Ansel for this Game", "", nullptr))
                {
                  SK_NvAPI_DisableAnsel (SK_GetDLLRole ());
                  enablement =
                    SK_NvAPI_GetAnselEnablement (SK_GetDLLRole ());
                } break;
            }

            ImGui::Separator ();
          }
        }

        ImGui::TreePush  ("");

        if (SK_IsInjected ())
        {
          ImGui::MenuItem ( "Special K Bootstrapper",
                            SK_FormatString (
                              "Global Injector  %s",
                                  SK_GetVersionStrA ()
                              ).c_str (), ""
                          );

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ( );
            ImGui::Text         ( "%lu-Bit Injection History",
                                    SK_GetBitness () );
            ImGui::Separator    ( );

            SK_ImGui_AutoFont fixed_font (
              io.Fonts->Fonts [SK_IMGUI_FIXED_FONT]
            );
            //ImGui::BulletText   ("%lu injections since restart", count);

            boost::container::static_vector <
              SK_InjectionRecord_s*, MAX_INJECTED_PROC_HISTORY
            > records;

            ImGui::BeginGroup ();
            for ( unsigned int i = 0 ; i < records.capacity () ; i++ )
            {
              SK_InjectionRecord_s *pRecord =
                SK_Inject_GetRecordByIdx (i);

              if (pRecord->process.id != 0)
              {
                records.push_back (pRecord);

                ImGui::BulletText ("");
              }
            }
            ImGui::EndGroup   ();
            ImGui::SameLine   ();
            ImGui::BeginGroup ();
            for ( const auto& it : records )
            {
              ImGui::Text ( " pid %04lu: ", it->process.id );
            }
            ImGui::EndGroup   ();
            ImGui::SameLine   ();
            ImGui::BeginGroup ();
            for ( const auto& it : records )
            {
              ImGui::Text ( " [%32ws] ",
                                      it->process.name );
            }
            ImGui::EndGroup   ();
            ImGui::SameLine   ();
            ImGui::BeginGroup ();
            for ( const auto& it : records )
            {
              if (it->process.id == GetCurrentProcessId ())
                  it->render.api  = rb.api;

              ImGui::Text ( " - %-12ws",
                                    SK_Render_GetAPIName (it->render.api) );
            }
            ImGui::EndGroup   ();
            ImGui::SameLine   ();
            ImGui::BeginGroup ();
            for ( const auto& it : records )
            {
              enum status_e {
                Finished,
                Running,
                Crashed
              } app_stat = Finished;

              if (it->process.id == GetCurrentProcessId ())
              {
                app_stat = Running;
              }

              else
              {
                if (it->process.crashed)
                  app_stat = Crashed;
              }

              if (app_stat == Crashed)
              {
                ImGui::Text (" { Crashed%s}",
                                 ( it->render.frames == 0 ? " at start? " :
                                                            " " ) );

              }

              else
              {
                ImGui::Text ("");
              }
            }
            ImGui::EndGroup   ();

            //ImGui::SameLine   ();
            //ImGui::BeginGroup ();
            //for (int i = 0; i < count; i++)
            //{
            //  SK_InjectionRecord_s* record =
            //    SK_Inject_GetRecord (i);
            //
            //  ImGui::Text ( " { %llu Frames }", record->process.id == GetCurrentProcessId () ?
            //                                      SK_GetFramesDrawn () :
            //                                      record->render.frames );
            //}
            //ImGui::EndGroup  ();

           fixed_font.Detach ();
           ImGui::EndTooltip ();
         }
       }

       else
       {
         ImGui::MenuItem ( "Special K Bootstrapper",
                             SK_FormatString (
                               "%ws API Wrapper  %s",
                                 SK_GetBackend   (),
                               SK_GetVersionStrA ()
                             ).c_str (), ""
                         );
       }

       static auto& host_executable =
           imports->host_executable;

       if (host_executable.product_desc.length () > 4)
       {
         ImGui::MenuItem   ( "Current Game",
                               SK_WideCharToUTF8 (
                                 host_executable.product_desc
                               ).c_str ()
                           );
       }

       ImGui::TreePop   ();
       ImGui::Separator ();

       ImGui::TreePush ("");
       ImGui::TreePush ("");

       for ( auto& import : imports->imports )
       {
         if (import.filename != nullptr)
         {
           if (import.role->get_value () != L"PlugIn")
           {
             ImGui::MenuItem (
               SK_FormatString ( "Third-Party Plug-In:  (%ws)",
                                   import.name.c_str () ).c_str (),
               SK_WideCharToUTF8 (
                 import.product_desc
               ).c_str ()
             );
           }

           else
           {
             ImGui::MenuItem (
               SK_FormatString ( "Official Plug-In:  (%ws)",
                                   import.name.c_str () ).c_str (),
               SK_WideCharToUTF8 (
                 import.product_desc
               ).c_str ()
             );
           }
         }
       }

#ifndef _WIN64
        if (imports->dgvoodoo_d3d8 != nullptr)
        {
          ImGui::MenuItem ( SK_WideCharToUTF8 (imports->dgvoodoo_d3d8->name).c_str         (),
                            SK_WideCharToUTF8 (imports->dgvoodoo_d3d8->product_desc).c_str () );
        }

        if (imports->dgvoodoo_ddraw != nullptr)
        {
          ImGui::MenuItem ( SK_WideCharToUTF8 (imports->dgvoodoo_ddraw->name).c_str         (),
                            SK_WideCharToUTF8 (imports->dgvoodoo_ddraw->product_desc).c_str () );
        }

        if (imports->dgvoodoo_d3dimm != nullptr)
        {
          ImGui::MenuItem ( SK_WideCharToUTF8 (imports->dgvoodoo_d3dimm->name).c_str         (),
                            SK_WideCharToUTF8 (imports->dgvoodoo_d3dimm->product_desc).c_str () );
        }
#endif

        ImGui::TreePop ();
        ImGui::TreePop ();

        ImGui::EndMenu  ();
      }
    };


  if (config.imgui.use_mac_style_menu && ImGui::BeginMainMenuBar ())
  {
    SpecialK_Menu ();

    ImGui::EndMainMenuBar ();
  }



  static HMODULE hModTBFix = SK_GetModuleHandle (L"tbfix.dll");
  static HMODULE hModTZFix = SK_GetModuleHandle (L"tzfix.dll");

  static float last_width  = -1;
  static float last_height = -1;

  bool recenter = ( last_width  != io.DisplaySize.x ||
                    last_height != io.DisplaySize.y );

  if (recenter)
  {
    SK_ImGui_LastWindowCenter.x = -1.0f;
    SK_ImGui_LastWindowCenter.y = -1.0f;

    SK_ImGui_SetNextWindowPosCenter (ImGuiCond_Always);
    last_width = io.DisplaySize.x; last_height = io.DisplaySize.y;
  }


  ImGui::SetNextWindowSizeConstraints (
    ImVec2 (250, 150), ImVec2 ( 0.9f * io.DisplaySize.x,
                                0.9f * io.DisplaySize.y )
  );


  const char* szTitle     = SK_ImGui_ControlPanelTitle ();
  static int  title_len   =
    int (static_cast <float> (ImGui::CalcTextSize (szTitle).x) * 1.075f);
  static bool first_frame = true;
  bool        open        = true;


  // Initialize the dialog using the user's scale preference
  if (first_frame)
  {
    io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableSetMousePos;

    // Range-restrict for sanity!
    config.imgui.scale =
      std::max (1.0f, std::min (5.0f, config.imgui.scale));

    io.FontGlobalScale = config.imgui.scale;
    first_frame        = false;
  }

  const  float font_size           =
    ( ImGui::GetFont  ()->FontSize * io.FontGlobalScale );
  const  float font_size_multiline =
    ( font_size + ImGui::GetStyle ().ItemSpacing.y +
                  ImGui::GetStyle ().ItemInnerSpacing.y );

  ImGuiStyle& style =
    ImGui::GetStyle ();

  style.WindowTitleAlign =
    ImVec2 (0.5f, 0.5f);

  ImGuiStyle orig = style;

  style.WindowMinSize.x = title_len * 1.075f * io.FontGlobalScale;
  style.WindowMinSize.y = 200;

  if (nav_usable)
  {
    ImGui::PushStyleColor (
      ImGuiCol_Text,
        (ImVec4&&)ImColor::HSV (
          (float)               (current_time % 2800)/ 2800.f,
            (.5f + (sin ((float)(current_time % 500) / 500.f)) * .5f) / 2.f,
               1.f )
    );
  }
  else
    ImGui::PushStyleColor ( ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f) );

  ImGui::Begin (
    szTitle, &open,
      ImGuiWindowFlags_AlwaysAutoResize |
      ( config.imgui.use_mac_style_menu ? 0x00 :
                                          ImGuiWindowFlags_MenuBar )
  );

  style = orig;

  if (open)
  {
  if (! config.imgui.use_mac_style_menu)
  {
    if (ImGui::BeginMenuBar ())
    {
      SpecialK_Menu         ();
      ImGui::EndMenuBar     ();
    }
  }

          char szAPIName [32] = { };
    snprintf ( szAPIName, 32, "%ws",  rb.name );

    // Translation layers (D3D8->11 / DDraw->11 / D3D11On12)
    auto api_mask = static_cast <int> (rb.api);

    bool translated_d3d9 =
      config.apis.d3d9.translated;

    if ( (api_mask &  static_cast <int> (SK_RenderAPI::D3D12))      != 0x0 &&
          api_mask != static_cast <int> (SK_RenderAPI::D3D12) )
    {
      lstrcatA (szAPIName,   "On12");
    }

    else if ( (api_mask &  static_cast <int> (SK_RenderAPI::D3D11)) != 0x0 &&
              (api_mask != static_cast <int> (SK_RenderAPI::D3D11) || translated_d3d9) )
    {
      if (! translated_d3d9)
        lstrcatA  (szAPIName, (const char *)    u8"→11");
      else
        lstrcpynA (szAPIName, (const char *)u8"D3D9→11", 32);
    }

    lstrcatA ( szAPIName, SK_GetBitness () == 32 ? "           [ 32-bit ]" :
                                                   "           [ 64-bit ]" );

    ImGui::MenuItem ("Active Render API        ", szAPIName, nullptr, false);

    if (SK_ImGui_IsItemRightClicked ())
    {
      ImGui::OpenPopup         ("RenderSubMenu");
      ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiCond_Always);
    }

    if (ImGui::BeginPopup      ("RenderSubMenu"))
    {
      DisplayMenu     ();
      ImGui::EndPopup ();
    }

    ImGui::Separator ();

    char szResolution [128] = { };

    bool sRGB     = rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_SRGB;
    bool hdr_out  = rb.isHDRCapable ()  &&
                    rb.isHDRActive  ();
    bool override = false;

    RECT client;
    GetClientRect (game_window.hWnd, &client);

    if ( static_cast <int> (io.DisplayFramebufferScale.x) != client.right  - client.left ||
         static_cast <int> (io.DisplayFramebufferScale.y) != client.bottom - client.top  ||
         sRGB                                                                            ||
         hdr_out )
    {
      snprintf ( szResolution, 63, "   %ux%u",
                   static_cast <UINT> (io.DisplayFramebufferScale.x),
                   static_cast <UINT> (io.DisplayFramebufferScale.y) );

      if (sRGB)
        strcat (szResolution, "    (sRGB)");

      if (hdr_out)
        strcat (szResolution, "     (HDR)");

      if (ImGui::MenuItem (" Framebuffer Resolution", szResolution))
      {
        config.window.res.override.x =
          static_cast <UINT> (io.DisplayFramebufferScale.x);
        config.window.res.override.y =
          static_cast <UINT> (io.DisplayFramebufferScale.y);

        override = true;
      }
    }

    SK_DPI_Update ();

    if (sRGB)
      strcat (szResolution, "    (sRGB)");

    snprintf ( szResolution, 63, "   %lix%li",
                                   (int)((float)(client.right - client.left)   * g_fDPIScale),
                                     (int)((float)(client.bottom - client.top) * g_fDPIScale) );

    if (_fDPIScale > 100.1f)
    {
      static char
                szScale [32] = { };
      snprintf (szScale, 31, "    %.0f%%", _fDPIScale);
      strcat (szResolution, szScale);
    }

    if (windowed)
    {
      if (ImGui::MenuItem (" Window Resolution     ", szResolution))
      {
        config.window.res.override.x = (int)((float)(client.right  - client.left) * g_fDPIScale);
        config.window.res.override.y = (int)((float)(client.bottom - client.top)  * g_fDPIScale);

        override = true;
      }

      if (_fDPIScale > 100.1f && ImGui::IsItemHovered ())
        ImGui::SetTooltip ("DPI Awareness:  %u DPI", _uiDPI);
    }

    else
    {
      float refresh_rate =
        rb.getActiveRefreshRate ();

      if (refresh_rate != 0.0f)
      {
        strcat ( szResolution,
                   SK_FormatString (" @ %6.02f Hz", refresh_rate).c_str ()
               );
      }

      if (ImGui::MenuItem (" Fullscreen Resolution", szResolution))
      {
        config.window.res.override.x = (int)((float)(client.right  - client.left) * g_fDPIScale);
        config.window.res.override.y = (int)((float)(client.bottom - client.top)  * g_fDPIScale);

        override = true;
      }
    }

    if (ImGui::IsItemHovered ())
    {
      if ( static_cast <int> (rb.api) &
           static_cast <int> (SK_RenderAPI::D3D9) )
      {
        auto pDev9 =
          rb.getDevice <IDirect3DDevice9> ();

        if (pDev9 != nullptr)
        {
          SK_ComQIPtr <IDirect3DSwapChain9> pSwap9 (rb.swapchain);

          if (pSwap9 != nullptr)
          {
            SK_ImGui_SummarizeD3D9Swapchain (pSwap9);
          }
        }
      }

      else if ( (static_cast <int> (rb.api) &
                 static_cast <int> (SK_RenderAPI::D3D11)) ||
                (static_cast <int> (rb.api) &
                 static_cast <int> (SK_RenderAPI::D3D12)) )
      {
        SK_ComQIPtr <IDXGISwapChain> pSwapDXGI (rb.swapchain);

        if (pSwapDXGI != nullptr)
        {
          SK_ImGui_SummarizeDXGISwapchain (pSwapDXGI);
        }
      }
    }

    int device_x =
      (int)((float)rb.windows.device.getDevCaps ().res.x * g_fDPIScale);
    int device_y =
      (int)((float)rb.windows.device.getDevCaps ().res.y * g_fDPIScale);

    if ( client.right - client.left   != device_x || client.bottom - client.top   != device_y ||
         io.DisplayFramebufferScale.x != device_x || io.DisplayFramebufferScale.y != device_y )
    {
      snprintf ( szResolution, 63, "   %ix%i",
                                     device_x,
                                       device_y );

      if (ImGui::MenuItem (" Device Resolution    ", szResolution))
      {
        config.window.res.override.x = device_x;
        config.window.res.override.y = device_y;

        override = true;
      }
    }

    if (! config.window.res.override.isZero ())
    {
      snprintf ( szResolution, 63, "   %lux%lu",
                                     config.window.res.override.x,
                                       config.window.res.override.y );

      bool selected = true;
      if (ImGui::MenuItem (" Override Resolution   ", szResolution, &selected))
      {
        config.window.res.override.x = 0;
        config.window.res.override.y = 0;

        override = true;
      }
    }


    // TEMP HACK: NvAPI does not support G-Sync Status in D3D12
    if (rb.api != SK_RenderAPI::D3D12)
    {
      if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
      {
        char szGSyncStatus [128] = { };

        if (rb.gsync_state.capable)
        {
          strcat (szGSyncStatus, "    Supported + ");
          if (rb.gsync_state.active)
          {
            strcat (szGSyncStatus, "Active");

            // Opt-in to Auto-Low Latency the first time this is seen
            if (config.render.framerate.auto_low_latency) {
                config.render.framerate.enforcement_policy = 2;
                config.render.framerate.auto_low_latency   = false;
            }
          }
          else
            strcat (szGSyncStatus, "Inactive");
        }

        else
        {
          strcat ( szGSyncStatus, rb.api == SK_RenderAPI::OpenGL ?
                                    " Unknown in GL" : "   Unsupported" );
        }

        ImGui::MenuItem (" G-Sync Status   ", szGSyncStatus, nullptr, false);

        if (rb.api == SK_RenderAPI::OpenGL && ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip (
            "The NVIDIA driver API does not report this status in OpenGL."
          );
        }
      }
    }


    if (override)
      SK_ImGui_AdjustCursor ();


    ImGui::Columns   ( 1 );
    ImGui::Separator (   );

    static bool has_own_scale = (hModTBFix);

    if ((! has_own_scale) && ImGui::CollapsingHeader ("UI Render Settings"))
    {
      ImGui::TreePush    ("");

      if ( ImGui::SliderFloat ( "###IMGUI_SCALE", &config.imgui.scale,
                                  1.0f, 3.0f, "UI Scaling Factor %.2f" ) )
      {
        // ImGui does not perform strict parameter validation,
        //   and values out of range for this can be catastrophic.
        config.imgui.scale =
          std::max (1.0f, std::min (5.0f, config.imgui.scale));
        io.FontGlobalScale = config.imgui.scale;
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ( "Optimal UI layout requires 1.0; other scales "
                            "may not display as intended." );
      }

      ImGui::SameLine        ();
      ImGui::Checkbox        ("Disable Transparency", &config.imgui.render.disable_alpha);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Resolves UI flicker in frame-doubled games");

      ImGui::TextUnformatted ("Anti-Aliasing:  ");                                          ImGui::SameLine ();
      ImGui::Checkbox        ("Lines",             &config.imgui.render.antialias_lines);   ImGui::SameLine ();
      ImGui::Checkbox        ("Contours",          &config.imgui.render.antialias_contours);

      ImGui::TreePop     ();
    }


    SK_PlugIn_ControlPanelWidget ();


    switch (SK_GetCurrentGameID ())
    {
      case SK_GAME_ID::GalGun_Double_Peace:
      {
        extern bool SK_GalGun_PlugInCfg (void);
                    SK_GalGun_PlugInCfg ();
      } break;
    };


  for ( auto plugin_cfg : plugin_mgr->config_fns )
  {          plugin_cfg ();                      }


  static bool has_own_limiter    = (hModTBFix);
  static bool has_own_limiter_ex = (hModTZFix);

  if (! has_own_limiter)
  {
    ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

    if ( ImGui::CollapsingHeader ("Framerate Limiter", ImGuiTreeNodeFlags_CollapsingHeader |
                                                       ImGuiTreeNodeFlags_DefaultOpen ) )
    {
      SK_ImGui_DrawGraph_FramePacing ();

      if (! has_own_limiter_ex)
      {
        static bool advanced = false;
               bool changed  = false;

        // Don't apply this number if it's < 10; that does very undesirable things
        float target_orig = __target_fps;

        bool limit = (__target_fps > 0.0f);

            ImGui::BeginGroup ();
        if (ImGui::Checkbox ("Framerate Limit", &limit))
        {
          if (__target_fps != 0.0f) // Negative zero... it exists and we don't want it.
              __target_fps = -__target_fps;

          if (__target_fps == 0.0f)
          {
            DWM_TIMING_INFO dwmTiming        = {                      };
                            dwmTiming.cbSize = sizeof (DWM_TIMING_INFO);

            if ( SUCCEEDED ( SK_DWM_GetCompositionTimingInfo (&dwmTiming) ) )
            {
              SK_FPU_ControlWord fpu_cw_orig =
                SK_FPU_SetPrecision (_PC_64);

              __target_fps =
                static_cast <float> (
                  static_cast <double> (dwmTiming.rateRefresh.uiNumerator) /
                  static_cast <double> (dwmTiming.rateRefresh.uiDenominator)
                );
                //        static_cast <float> (
                //1.0 / ( static_cast <double> (dwmTiming.qpcRefreshPeriod) /
                //        static_cast <double> (SK_GetPerfFreq ().QuadPart) )
                //                            );

              SK_FPU_SetControlWord (_MCW_PC, &fpu_cw_orig);
            }

            else
              __target_fps = 60.0f;
          }
        }

        bool bg_limit =
              ( limit &&
          ( __target_fps_bg != 0.0f ) );

        if (limit)
        {
          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::TextUnformatted (
              "Graph color represents frame time variance, not proximity"
              " to your target FPS."
            );

            if ( ( rb.api == SK_RenderAPI::D3D11 ||
                   rb.api == SK_RenderAPI::D3D12 ) && (! (config.render.framerate.flip_discard &&
                                                          config.render.framerate.swapchain_wait > 0)))
            {
              ImGui::Separator       ();
              ImGui::TextUnformatted ("Did you know ... to get the most out of SK's Framerate Limiter:");
              ImGui::BulletText      ("Enable Flip Model + Waitable SwapChain in D3D11/12 / SwapChain Settings");
            }

            ImGui::EndTooltip ();
          }

          if (advanced)
          {
            if (ImGui::Checkbox ("Background", &bg_limit))
            {
              if (bg_limit) __target_fps_bg = __target_fps;
              else          __target_fps_bg =         0.0f;
            }

            if (ImGui::IsItemHovered ())
            {
              static bool unity =
                rb.windows.unity;

              ImGui::BeginTooltip ();
              ImGui::Text (
                "Optional secondary limit applies when the game is running"
                " in the background." );

              if (unity)
              {
                ImGui::Separator   (    );
                ImGui::Spacing     (    );
                ImGui::BulletText  ( "This is a Unity engine game and "
                                     "requires special attention." );
                ImGui::Spacing     (    );
                ImGui::TreePush    ( "" );
                ImGui::TextColored ( ImColor (.62f, .62f, .62f),
                                     "\tRefer to the Following Setting:" );
                ImGui::Spacing     (    );
                ImGui::TreePush    ( "" );
                ImGui::TextColored ( ImColor (1.f, 1.f, 1.f),
                                       "\tWindow Management > Input/Output"
                                       " Behavior > Continue Rendering" );
                ImGui::TreePop     (    );
                ImGui::TreePop     (    );
              }

              ImGui::EndTooltip ();
            }
          }
        }

        auto _ResetLimiter = [&](void) -> void
        {
          auto *pLimiter =
            SK::Framerate::GetLimiter (rb.swapchain.p, false);

          if (pLimiter != nullptr)
              pLimiter->reset (true);
        };

        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();

        auto _LimitSlider =
          [&](       float& target,
                const char*  label,
                const char* command,
                      bool  active ) -> void
        {
          float target_mag = fabs (target);

          ImGui::PushStyleColor ( ImGuiCol_Text,
            ( active ? ImColor (1.00f, 1.00f, 1.00f)
                     : ImColor (0.73f, 0.73f, 0.73f) ) );

          if ( ImGui::DragFloat ( label, &target_mag,
                                      1.0f, 24.0f, 166.0f, target > 0 ?
                          ( active ? "%6.3f fps  (Limit Engaged)" :
                                     "%6.3f fps  (~Window State)" )
                                                                  :
                                                           target < 0 ?
                                             "%6.3f fps  (Graphing Only)"
                                                                  :
                                             "VSYNC Rate (No Preference)" )
             )
          {
            static auto cp =
              SK_GetCommandProcessor ();

            target =
              ( ( target < 0.0f ) ? (-1.0f * target_mag) :
                                             target_mag    );

            if (target > 10.0f || target == 0.0f)
              cp->ProcessCommandFormatted ("%s %f", command, target);
            else if (target < 0.0f)
            {
              float graph_target = target;
              cp->ProcessCommandFormatted ("%s 0.0", command);
                    target = graph_target;
            }
            else
              target = target_orig;
          }
          ImGui::PopStyleColor ();

          if (ImGui::IsItemHovered ( ))
          {
            ImGui::BeginTooltip    ( );
            ImGui::TextColored     ( ImColor::HSV (0.18f, 0.88f, 0.94f),
                                       "Ctrl + Click" );
            ImGui::SameLine        ( );
            ImGui::TextUnformatted ( "to Enter an Exact Framerate" );
            ImGui::EndTooltip      ( );
          }
        };

        _LimitSlider ( __target_fps, "###FPS_TargetOrLimit",
                                            "TargetFPS",
                        (SK_IsGameWindowActive () || (! bg_limit)) );

        if (limit)
        {
          if (advanced && bg_limit)
          {
            _LimitSlider (
              __target_fps_bg, "###Background_FPS",
                                  "BackgroundFPS", (! SK_IsGameWindowActive ())
            );
          }
        }

        ImGui::EndGroup ();
        ImGui::SameLine ();

        advanced =
          ImGui::TreeNode ("Advanced ###Advanced_FPS");

        if (advanced)
        {
          ImGui::TreePop    ();
          ImGui::Separator  ();
          if (__target_fps > 0.0f)
          {
            ImGui::BeginGroup ();

            static bool bLowLatency =
                ( config.render.framerate.enforcement_policy == 2);

            if (ImGui::Checkbox ("Low Latency Mode", &bLowLatency))
            {
              if (bLowLatency) config.render.framerate.enforcement_policy = 2;
              else             config.render.framerate.enforcement_policy = 4;

              _ResetLimiter ();
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip ();
              ImGui::Text         ("Reduces Input Latency");
              ImGui::Separator    ();
              ImGui::BulletText   ("This mode is ideal for users with G-Sync / VRR displays");
              ImGui::BulletText   ("Latency reduction can be quite profound, but comes with potential minor stutter");
              ImGui::Separator    ();
              ImGui::BulletText   ("The default mode has some latency benefits, but was designed to fix stutter on fixed-refresh displays");
              ImGui::EndTooltip   ();
            }

            //ImGui::SliderInt ("Limit Enforcement Site", &__SK_FramerateLimitApplicationSite, 0, 4);

            if ( rb.api == SK_RenderAPI::D3D11 ||
                 rb.api == SK_RenderAPI::D3D12 )
            {
              if (ImGui::Checkbox ("Drop Late Frames", &config.render.framerate.drop_late_flips))
                _ResetLimiter ();

              if (ImGui::IsItemHovered ())
              {
                ImGui::SetTooltip ("Always Present Newest Frame (DXGI Flip Model)");
              }
            }

            ImGui::EndGroup   ();
            ImGui::SameLine   (0.0f, 20.0f);
          }

          ImGui::BeginGroup ();

          if (rb.api != SK_RenderAPI::D3D12)
          {
            changed |=
              ImGui::Checkbox ( "Sleepless Render Thread",
                                  &config.render.framerate.sleepless_render );

            if (ImGui::IsItemHovered ())
            {
              SK::Framerate::EventCounter::SleepStats& stats =
                SK::Framerate::GetEvents ()->getRenderThreadStats ();

              ImGui::SetTooltip
                             ( "(%li ms asleep, %li ms awake)",
                                 /*(stats.attempts - stats.rejections), stats.attempts,*/
                                   ReadAcquire (&stats.time.allowed),
                                   ReadAcquire (&stats.time.deprived) );
            }
          }

          changed |=
            ImGui::Checkbox ( "Sleepless Window Thread",
                                &config.render.framerate.sleepless_window );

          if (ImGui::IsItemHovered ())
          {
            SK::Framerate::EventCounter::SleepStats& stats =
              SK::Framerate::GetEvents ()->getMessagePumpStats ();

            ImGui::SetTooltip
                           ( "(%li ms asleep, %li ms awake)",
                               /*(stats.attempts - stats.rejections), stats.attempts,*/
                                 ReadAcquire (&stats.time.allowed),
                                 ReadAcquire (&stats.time.deprived) );
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   (0.0f, 20.0f);
          ImGui::BeginGroup ();
          if ( ImGui::Checkbox ( "Use Multimedia Class Scheduling",
                                   &config.render.framerate.enable_mmcss ) )
          {
            SK_DWM_EnableMMCSS ((BOOL)config.render.framerate.enable_mmcss);
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Minimizes dropped frames when combined with FPS limiting.");
            ImGui::Separator    ();
            ImGui::BulletText   ("Keep this option enabled unless troubleshooting something");
            ImGui::EndTooltip   ();
          }

          bool spoof =
            ( config.render.framerate.override_num_cpus != -1 );

          static SYSTEM_INFO             si = { };
          SK_RunOnce (SK_GetSystemInfo (&si));

          if ( ImGui::Checkbox ("Spoof CPU Core Count", &spoof) )
          {
            config.render.framerate.override_num_cpus =
              ( spoof ? si.dwNumberOfProcessors : -1 );
          }

          if (ImGui::IsItemHovered ())
              ImGui::SetTooltip ("Useful in Unity games -- set lower than actual to fix negative performance scaling.");

          ImGui::EndGroup ();

          if (spoof)
          {
            ImGui::BeginGroup ();
            ImGui::SliderInt  ( "###SPOOF_CPU_COUNT",
                                &config.render.framerate.override_num_cpus,
                                  1, si.dwNumberOfProcessors,
                                    "Number of CPUs: %d" );
            ImGui::EndGroup   ();
          }

          SK_NV_LatencyControlPanel ();
        }
      }
    }

    ImGui::PopItemWidth ();
  }

  using namespace SK::ControlPanel;

  render_api = rb.api;

  // These stubs only draw if the operating environment
  //   includes their respective APIs.
  //
  // * Don't bother conditionally calling these.
  //
           D3D9::Draw ();
          D3D11::Draw ();
         OpenGL::Draw ();

  Compatibility::Draw ();

          Input::Draw ();
         Window::Draw ();
          Sound::Draw ();

            OSD::Draw ();

  const bool open_widgets =
    ImGui::CollapsingHeader ("Widgets");

  if (ImGui::IsItemHovered ( ))
  {
    ImGui::BeginTooltip ();
    ImGui::Text         ("Advanced Graphical Extensions to the OSD");
    ImGui::Separator    ();
    ImGui::BulletText   ("Widgets are Graphical Representations of Performance Data");
    ImGui::BulletText   ("Right-click a Widget to Access its Config Menu");
    ImGui::EndTooltip   ();
  }

  if (open_widgets)
  {
    bool framepacing   = SK_ImGui_Widgets->frame_pacing->isVisible    ();
    bool latency       = SK_ImGui_Widgets->latency->isVisible         ();
    bool gpumon        = SK_ImGui_Widgets->gpu_monitor->isVisible     ();
    bool volumecontrol = SK_ImGui_Widgets->volume_control->isVisible  ();
    bool cpumon        = SK_ImGui_Widgets->cpu_monitor->isVisible     ();
    bool pipeline11    = SK_ImGui_Widgets->d3d11_pipeline->isVisible  ();
    bool pipeline12    = SK_ImGui_Widgets->d3d12_pipeline->isVisible  ();
    bool threads       = SK_ImGui_Widgets->thread_profiler->isVisible ();
    bool hdr           = SK_ImGui_Widgets->hdr_control->isVisible     ();
    bool tobii         = SK_ImGui_Widgets->tobii->isVisible           ();

    ImGui::TreePush ("");

    if (ImGui::Checkbox ("Framepacing", &framepacing))
    {
      SK_ImGui_Widgets->frame_pacing->setVisible (framepacing).
                                      setActive  (framepacing);
    }
    ImGui::SameLine ();

    if (ImGui::Checkbox ("CPU",         &cpumon))
    {
      SK_ImGui_Widgets->cpu_monitor->setVisible (cpumon);
    }
    ImGui::SameLine ();

    if (ImGui::Checkbox ("GPU",         &gpumon))
    {
      SK_ImGui_Widgets->gpu_monitor->setVisible (gpumon);//.setActive (gpumon);
    }
    ImGui::SameLine ();

    //ImGui::Checkbox ("Memory",       &SK_ImGui_Widgets->memory);
    //ImGui::SameLine ();
    //ImGui::Checkbox ("Disk",         &SK_ImGui_Widgets->disk);
    //ImGui::SameLine ();

    if (ImGui::Checkbox ("Volume Control", &volumecontrol))
    {
      SK_ImGui_Widgets->volume_control->setVisible (volumecontrol).
                                        setActive  (volumecontrol);
    }

    if ( (int)render_api & (int)SK_RenderAPI::D3D11 ||
         (int)render_api & (int)SK_RenderAPI::D3D12 )
    {
      if ((int)render_api & (int)SK_RenderAPI::D3D11)
      {
        ImGui::SameLine ();
        ImGui::Checkbox ("Texture Cache", &SK_ImGui_Widgets->texcache);
      }

      ImGui::SameLine ();

      if ((int)render_api & (int)SK_RenderAPI::D3D11)
      {
        if (ImGui::Checkbox ("Pipeline Stats###Pipeline11", &pipeline11))
        {
          SK_ImGui_Widgets->d3d11_pipeline->setVisible (pipeline11).
                                            setActive  (pipeline11);
        }

        ImGui::SameLine ();
      }

      else
      {
        if (ImGui::Checkbox ("Pipeline Stats###Pipeline12", &pipeline12))
        {
          SK_ImGui_Widgets->d3d12_pipeline->setVisible (pipeline12).
                                            setActive  (pipeline12);
        }

        ImGui::SameLine ();
      }

      if (sk::NVAPI::nv_hardware)
      {
        if (ImGui::Checkbox ("Latency Analysis###ReflexLatency", &latency))
        {
          SK_ImGui_Widgets->latency->setVisible (latency).
                                     setActive  (latency);
        }
      }
    }

    if (rb.isHDRCapable () || __SK_HDR_16BitSwap)
    {
      ImGui::SameLine ();
      if (ImGui::Checkbox ("HDR Display", &hdr))
      {
        SK_ImGui_Widgets->hdr_control->setVisible (hdr).
                                       setActive  (hdr);
      }
    }

#if 0
    ImGui::SameLine ();

    if (ImGui::Checkbox ("Tobii Eyetracker", &tobii))
    {
      SK_ImGui_Widgets->tobii->setVisible (tobii).
                               setActive  (tobii);
    }
#else
    UNREFERENCED_PARAMETER (tobii);
#endif

    ImGui::SameLine ();

    if (ImGui::Checkbox ("Threads", &threads))
    {
      SK_ImGui_Widgets->thread_profiler->setVisible (threads).
                                         setActive  (threads);
    }

    ImGui::TreePop  ();
  }

  if (ImGui::CollapsingHeader ("Screenshots"))
  {
    ImGui::TreePush ("");

    ImGui::BeginGroup ();

    if (SK::SteamAPI::AppID () > 0 && SK::SteamAPI::GetCallbacksRun ())
    {
      if (ImGui::Checkbox ("Enable Steam Screenshot Hook", &config.steam.screenshots.enable_hook))
      {
        rb.screenshot_mgr.init ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ( "In D3D11/12 games, integrates SK's high-performance/quality screenshot capture "
                            "system, adding HDR -> LDR support and other things." );
      }

      if (config.steam.screenshots.enable_hook)
      {
        ImGui::TreePush ("");
        ImGui::Checkbox ("Show OSD in Steam Screenshots", &config.screenshots.show_osd_by_default);
        ImGui::TreePop  (  );
      }
    }

    bool png_changed = false;

    ImGui::Checkbox ("Copy Screenshots to Clipboard",        &config.screenshots.copy_to_clipboard);
    ImGui::Checkbox ("Play Sound on Screenshot Capture",     &config.screenshots.play_sound);

    if (rb.api != SK_RenderAPI::D3D12)
    {
      png_changed =
        ImGui::Checkbox ( rb.isHDRCapable () ? "Keep HDR .JXR Screenshots     " :
                                               "Keep Lossless .PNG Screenshots",
                                                               &config.screenshots.png_compress       );
    } else { config.screenshots.png_compress = true; }

    ImGui::EndGroup ();

    const auto Keybinding =
    [] (SK_Keybind* binding, sk::ParameterStringW* param) ->
    auto
    {
      if (! (binding != nullptr && param != nullptr))
        return false;

      std::string label =
        SK_WideCharToUTF8 (binding->human_readable);

      ImGui::PushID (binding->bind_name);

      if (ImGui::Selectable (label.c_str (), false))
      {
        ImGui::OpenPopup (binding->bind_name);
      }

      std::wstring original_binding = binding->human_readable;

      SK_ImGui_KeybindDialog (binding);

      ImGui::PopID ();

      if (original_binding != binding->human_readable)
      {
        param->store (binding->human_readable);

        return true;
      }

      return false;
    };

    static std::set <SK_ConfigSerializedKeybind *>
      keybinds = {
        &config.screenshots.game_hud_free_keybind,
        &config.screenshots.sk_osd_free_keybind,
        &config.screenshots.sk_osd_insertion_keybind
      };

    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {
      ImGui::Text          ( "%s:  ",
                               keybind->bind_name );
    }
    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {
      Keybinding ( keybind, keybind->param );
    }
    ImGui::EndGroup   ();

    if ( rb.screenshot_mgr.getRepoStats ().files > 0 )
    {
      const SK_ScreenshotManager::screenshot_repository_s& repo =
        rb.screenshot_mgr.getRepoStats (png_changed);

      ImGui::BeginGroup (  );
      ImGui::Separator  (  );
      ImGui::TreePush   ("");
      ImGui::Text ( "%u files using %ws",
                      repo.files,
                        SK_File_SizeToString (repo.liSize.QuadPart).c_str  ()
                  );

      if (SK::SteamAPI::AppID () > 0 && SK::SteamAPI::GetCallbacksRun () && ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ( rb.isHDRCapable () ?
                              "Steam does not support HDR Screenshots, so SK maintains its own storage for .JXR Screenshots" :
                              "Steam does not support .PNG Screenshots, so SK maintains its own storage for Lossless Screenshots." );
      }

      ImGui::SameLine ();

      if (ImGui::Button ("Browse"))
      {
        SK_ShellExecuteW ( nullptr,
          L"explore",
            rb.screenshot_mgr.getBasePath (),
              nullptr, nullptr,
                    SW_NORMAL
        );
      }

      ImGui::TreePop  ();
      ImGui::EndGroup ();
    }
    ImGui::EndGroup   ();
    ImGui::TreePop    ();
  }

  SK::ControlPanel::PlugIns::Draw ();
  SK::ControlPanel::Steam::Draw   ();

  SK_ImGui::BatteryMeter ();

  ImVec2 pos  = ImGui::GetWindowPos  ();
  ImVec2 size = ImGui::GetWindowSize ();

  SK_ImGui_LastWindowCenter.x = ( pos.x + size.x / 2.0f );
  SK_ImGui_LastWindowCenter.y = ( pos.y + size.y / 2.0f );

  if (recenter)
    SK_ImGui_CenterCursorAtPos ();
  }

  ImGui::End           ();
  ImGui::PopStyleColor ();

  static bool was_open = false;

  if ((! open) && was_open)
  {
    SK_XInput_ZeroHaptics (config.input.gamepad.steam.ui_slot); // XXX: MAKE SEPARATE
    SK_XInput_ZeroHaptics (config.input.gamepad.xinput.ui_slot);
  }

  was_open = open;

  return open;
}


using SK_ImGui_DrawCallback_pfn      = UINT (__stdcall *)(void *user);
using SK_ImGui_OpenCloseCallback_pfn = bool (__stdcall *)(void *user);

struct SK_ImGui_DrawCallback_s
{
  SK_ImGui_DrawCallback_pfn fn   = nullptr;
  void*                     data = nullptr;
};

struct SK_ImGui_OpenCloseCallback_s
{
  SK_ImGui_OpenCloseCallback_pfn fn   = nullptr;
  void*                          data = nullptr;
};

SK_LazyGlobal <SK_ImGui_DrawCallback_s>      SK_ImGui_DrawCallback;
SK_LazyGlobal <SK_ImGui_OpenCloseCallback_s> SK_ImGui_OpenCloseCallback;

__declspec (noinline)
IMGUI_API
void
SK_ImGui_InstallDrawCallback (SK_ImGui_DrawCallback_pfn fn, void* user)
{
  SK_ImGui_DrawCallback->fn   = fn;
  SK_ImGui_DrawCallback->data = user;
}

IMGUI_API
void
SK_ImGui_InstallOpenCloseCallback (SK_ImGui_OpenCloseCallback_pfn fn, void* user)
{
  SK_ImGui_OpenCloseCallback->fn   = fn;
  SK_ImGui_OpenCloseCallback->data = user;
}


static bool keep_open;

void
SK_Steam_GetUserName (char* pszName, int max_len = 512)
{
  if (SK_SteamAPI_Friends () != nullptr)
  {
    auto orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator (
        EXCEPTION_ACCESS_VIOLATION
      )
    );
    try {
      strncpy_s (pszName, max_len, SK_SteamAPI_Friends ()->GetPersonaName (), _TRUNCATE);
    }

    catch (const SK_SEH_IgnoredException&)
    { }
    SK_SEH_RemoveTranslator (orig_se);
  }
}


LRESULT
CALLBACK
SK_ImGui_MouseProc (int code, WPARAM wParam, LPARAM lParam)
{
  if (code < 0)
    return CallNextHookEx (0, code, wParam, lParam);

  if (SK_ImGui_Visible)
  {
    auto& io =
      ImGui::GetIO ();

    MOUSEHOOKSTRUCT* mhs =
      (MOUSEHOOKSTRUCT*)lParam;

    if (mhs->hwnd == game_window.hWnd)
    {
      switch (wParam)
      {
        case WM_MOUSEMOVE:
          if (PtInRect (&game_window.actual.window, mhs->pt))
          {
            SK_ImGui_Cursor.pos = mhs->pt;
            SK_ImGui_Cursor.ScreenToLocal (
                                  &SK_ImGui_Cursor.pos);
            io.MousePos.x = (float)SK_ImGui_Cursor.pos.x;
            io.MousePos.y = (float)SK_ImGui_Cursor.pos.y;
          }
          break;

        // Does not work correctly
        ///case WM_MOUSEWHEEL:
        ///  io.MouseWheel +=
        ///    static_cast <float> (GET_WHEEL_DELTA_WPARAM (mhs->dwExtraInfo)) /
        ///    static_cast <float> (WHEEL_DELTA);
        ///  break;


        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
          io.MouseClicked [0] = true;
          break;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
          io.MouseClicked [1] = true;
          break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
          io.MouseClicked [2] = true;
          break;

        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        {
          ////WORD Flags =
          ////  GET_XBUTTON_WPARAM (mhs->dwExtraInfo);
          ////
          ////io.MouseDown [3] |= (Flags & XBUTTON1) != 0;
          ////io.MouseDown [4] |= (Flags & XBUTTON2) != 0;
        } break;
      }

      if (SK_ImGui_WantMouseCapture () && (wParam != WM_MOUSEWHEEL))
        return 1;
    }
  }

  return
    CallNextHookEx (0, code, wParam, lParam);
}

LRESULT
CALLBACK
SK_ImGui_KeyboardProc (int code, WPARAM wParam, LPARAM lParam)
{
  if (code < 0)
    return CallNextHookEx (0, code, wParam, lParam);

  if (SK_ImGui_Visible)
  {
    auto& io =
      ImGui::GetIO ();

  //bool wasPressed =
  //    ( lParam & (1 << 30) ) != 0;
    bool isPressed =
        ( lParam & (1 << 31) ) == 0;
  //bool isAltDown =
  //    ( lParam & (1 << 29) ) != 0;

    io.KeysDown [wParam] = isPressed;

    if (SK_ImGui_WantKeyboardCapture () && (! io.WantTextInput))
      return 1;
  }

  return
    CallNextHookEx (0, code, wParam, lParam);
}

void
SK_ImGui_StageNextFrame (void)
{
  static auto& humanToVirtual = humanKeyNameToVirtKeyCode.get ();
  static auto& virtualToHuman = virtKeyCodeToHumanKeyName.get ();

  if (imgui_staged)
    return;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  // Excessively long frames from things like toggling the Steam overlay
  //   must be ignored.
  static ULONG64 last_frame         = 0;
  bool           skip_frame_history = false;

  if (last_frame < SK_GetFramesDrawn () - 1)
  {
    skip_frame_history = true;
  }

  if (last_frame != SK_GetFramesDrawn ())
  {
    last_frame  = SK_GetFramesDrawn ();

    static auto& io =
      ImGui::GetIO ();

    font.size           = ImGui::GetFont () != nullptr                     ?
                          ImGui::GetFont ()->FontSize * io.FontGlobalScale :
                                                                          1.0f;

    font.size_multiline = SK::ControlPanel::font.size      +
                          ImGui::GetStyle ().ItemSpacing.y +
                          ImGui::GetStyle ().ItemInnerSpacing.y;
  }

  current_tick = SK_QueryPerf   ().QuadPart;
  current_time = SK_timeGetTime ();

  bool d3d9  = false;
  bool d3d11 = false;
  bool d3d12 = false;
  bool gl    = false;

  if (rb.api == SK_RenderAPI::OpenGL)
  {
    gl = true;

    ImGui_ImplGL3_NewFrame ();
  }

  else if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9))
  {
    d3d9 = true;

    ImGui_ImplDX9_NewFrame ();
  }

  else if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11))
  {
    d3d11 = true;

    ImGui_ImplDX11_NewFrame ();
  }

  else if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D12))
  {
    d3d12 = true;

    ImGui_ImplDX12_NewFrame ();
  }

  else
  {
    SK_LOG0 ( (L"No Render API"), L"Overlay" );
    return;
  }

  static auto& io =
    ImGui::GetIO ();

  ///SK_ComQIPtr <IDXGISwapChain> pSwapChain (rb.swapchain);
  ///
  ///if (pSwapChain != nullptr)
  ///{
  ///  DXGI_SWAP_CHAIN_DESC  desc = {};
  ///  pSwapChain->GetDesc (&desc);
  ///
  ///  // scRGB
  ///  if ( rb.isHDRCapable ()   &&
  ///      (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR)
  ///                            &&
  ///      desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
  ///  {
  ///  }
  ///}

  extern volatile
               LONG __SK_ScreenShot_CapturingHUDless;
  if (ReadAcquire (&__SK_ScreenShot_CapturingHUDless))
  {
    imgui_staged = false;
    return;
  }



  //////
  ////// Framerate history
  //////
  ////if (! (reset_frame_history || skip_frame_history))
  ////{
  ////  SK_ImGui_Frames->timeFrame (io.DeltaTime);
  ////}
  ////
  ////else if (reset_frame_history) SK_ImGui_Frames->reset ();

  reset_frame_history = false;

  extern void SK_Widget_InitLatency        (void);
  extern void SK_Widget_InitFramePacing    (void);
  extern void SK_Widget_InitThreadProfiler (void);
  extern void SK_Widget_InitVolumeControl  (void);
  extern void SK_Widget_InitGPUMonitor     (void);
  extern void SK_Widget_InitTobii          (void);
  SK_RunOnce (SK_Widget_InitThreadProfiler (    ));
  SK_RunOnce (SK_Widget_InitFramePacing    (    ));
  SK_RunOnce (SK_Widget_InitLatency        (    ));
  SK_RunOnce (SK_Widget_InitVolumeControl  (    ));
  SK_RunOnce (SK_Widget_InitTobii          (    ));
  SK_RunOnce (SK_Widget_InitGPUMonitor     (    ));

  static bool init_widgets = true;
  static auto widgets =
  {
    SK_ImGui_Widgets->frame_pacing,
      SK_ImGui_Widgets->volume_control,
        SK_ImGui_Widgets->gpu_monitor,
          SK_ImGui_Widgets->cpu_monitor,
            SK_ImGui_Widgets->d3d11_pipeline,
              SK_ImGui_Widgets->d3d12_pipeline,
                SK_ImGui_Widgets->thread_profiler,
                  SK_ImGui_Widgets->hdr_control,
                    SK_ImGui_Widgets->tobii,
                      SK_ImGui_Widgets->latency
  };

  if (init_widgets)
  {
    init_widgets = false;

    for (auto& widget : widgets)
    {
      if (widget != nullptr)
          widget->run_base ();
    }
  }


  // Default action is to draw the Special K Control panel,
  //   but if you hook this function you can do anything you
  //     want...
  //
  //    To use the Special K Control Panel from a plug-in,
  //      import SK_ImGui_ControlPanel and call that somewhere
  //        from your hook.

  keep_open = true;


  if (SK_ImGui_Visible)
  {
    using  SetWindowsHookEx_pfn = HHOOK (WINAPI*)(int, HOOKPROC, HINSTANCE, DWORD);
  //extern SetWindowsHookEx_pfn SetWindowsHookExA_Original;
    extern SetWindowsHookEx_pfn SetWindowsHookExW_Original;

  //SK_RunOnce (IsGUIThread (TRUE));

    SK_RunOnce (
      SetWindowsHookExW_Original (
        WH_KEYBOARD, SK_ImGui_KeyboardProc,
          skModuleRegistry::Self (),
              GetCurrentThreadId ()
                                 )
    );

    SK_RunOnce (
      SetWindowsHookExW_Original (
        WH_MOUSE,    SK_ImGui_MouseProc,
          skModuleRegistry::Self (),
              GetCurrentThreadId ()
                                 )
    );

    SK_ControlPanel_Activated = true;
    keep_open                 = SK_ImGui_ControlPanel ();
  }


  for (auto& widget : widgets)
  {
    if (widget == nullptr)
      continue;

    if ( widget == SK_ImGui_Widgets->latency     ||
         widget == SK_ImGui_Widgets->hdr_control ||
         widget == SK_ImGui_Widgets->d3d11_pipeline )
    {
      if ( rb.api != SK_RenderAPI::D3D11 &&
           rb.api != SK_RenderAPI::D3D12 ) continue;

      if (widget == SK_ImGui_Widgets->latency && (! sk::NVAPI::nv_hardware))
        continue;
    }

    if (widget->isActive ())
        widget->run_base ();

    else if (widget == SK_ImGui_Widgets->hdr_control ||
             widget == SK_ImGui_Widgets->thread_profiler)
    {
      widget->setActive (true);
    }

    extern bool SK_Tobii_IsCursorVisible (void);

    if ( widget->isVisible () ||
         // Tobii widget needs to be drawn to show its cursor
         ( widget == SK_ImGui_Widgets->tobii &&
           SK_Tobii_IsCursorVisible ()         )
       )
    {
      widget->draw_base ();
    }
  }



  if (! SK::SteamAPI::GetOverlayState (true))
  {
    SK_DrawOSD     ();
    SK_DrawConsole ();
  }




  static DWORD dwStartTime = current_time;
  if ((current_time < dwStartTime + 1000 * config.version_banner.duration) || eula.show)
  {
    ImGui::PushStyleColor    (ImGuiCol_Text,     ImVec4 (1.f,   1.f,   1.f,   1.f));
    ImGui::PushStyleColor    (ImGuiCol_WindowBg, ImVec4 (.333f, .333f, .333f, 0.828282f));
    ImGui::SetNextWindowPos  (ImVec2 (10, 10));
    ImGui::SetNextWindowSize (ImVec2 ( io.DisplayFramebufferScale.x        - 20.0f,
                                       ImGui::GetFrameHeightWithSpacing () * 4.5f  ), ImGuiCond_Always );
    ImGui::Begin             ( "Splash Screen##SpecialK",
                                 nullptr,
                                   ImGuiWindowFlags_NoTitleBar      |
                                   ImGuiWindowFlags_NoScrollbar     |
                                   ImGuiWindowFlags_NoMove          |
                                   ImGuiWindowFlags_NoResize        |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoInputs        |
                                   ImGuiWindowFlags_NoFocusOnAppearing );

    static char szName [512] = { };
    SK_Steam_GetUserName (szName);


    ImGui::TextColored     (ImColor::HSV (.11f, 1.f, 1.f),  "%ws   ", SK_GetPluginName ().c_str ()); ImGui::SameLine ();

    if (*szName != '\0')
    {
      ImGui::Text            ("  Hello");                                                            ImGui::SameLine ();
      ImGui::TextColored     (ImColor::HSV (0.075f, 1.0f, 1.0f), "%s", szName);                      ImGui::SameLine ();
      ImGui::TextUnformatted ("please see the Discord Release Channel, under");                      ImGui::SameLine ();
    }
    else
    {
      ImGui::TextUnformatted ("  Please see the Discord Release Channel, under");                    ImGui::SameLine ();
    }
    ImGui::TextColored       (ImColor::HSV (.52f, 1.f, 1.f),  "Help | Releases");                    ImGui::SameLine ();
    ImGui::TextUnformatted   ("for beta / stable updates to this project.");

    ImGui::Spacing ();
    ImGui::Spacing ();

    auto PopulateBranches = [](auto branches) ->
      std::map <std::string, SK_BranchInfo>
      {
        std::map <std::string, SK_BranchInfo> details;

        for ( auto& it : branches )
          details.emplace (it, SK_Version_GetLatestBranchInfo (nullptr, it.c_str ()));

        return details;
      };

    static std::vector <std::string>                branches       = SK_Version_GetAvailableBranches (nullptr);
    static std::map    <std::string, SK_BranchInfo> branch_details = PopulateBranches (branches);

    static SK_VersionInfo vinfo =
      SK_Version_GetLocalInfo (nullptr);

    char current_ver        [128] = { };
    char current_branch_str [ 64] = { };

    snprintf (current_ver,        127, "%ws (%li)", vinfo.package.c_str (), vinfo.build);
    snprintf (current_branch_str,  63, "%ws",       vinfo.branch.c_str  ());

    static SK_VersionInfo vinfo_latest =
      SK_Version_GetLatestInfo (nullptr);

    static SK_BranchInfo_V1 current_branch =
      SK_Version_GetLatestBranchInfo (nullptr, SK_WideCharToUTF8 (vinfo.branch).c_str ());

    static std::string utf8_release_title;
    static std::string utf8_release_description;
    static std::string utf8_branch_name;
    static std::string utf8_time_checked;

    if (! current_branch.release.description.empty ())
    {
      ImGui::BeginChildFrame (
        ImGui::GetID ("###SpecialK_SplashScreenFrame"),
          ImVec2 ( -1.0f,
                    ImGui::GetFrameHeightWithSpacing () * 2.1f ),
            ImGuiWindowFlags_AlwaysAutoResize
      );

      std::chrono::time_point <std::chrono::system_clock> now =
        std::chrono::system_clock::now ();

      ImColor version_color =
        ImColor::HSV (
          static_cast <float> (
            std::chrono::duration_cast <std::chrono::milliseconds> (
              now.time_since_epoch ()
            ).count () % 5000
          ) / 5000.0f, 0.275f, 1.f
        );

      if (utf8_release_title.empty ())
      {
        utf8_release_title =
          SK_WideCharToUTF8 (current_branch.release.title);
      }

      if (utf8_release_description.empty ())
      {
        utf8_release_description =
          SK_WideCharToUTF8 (current_branch.release.description);
      }

      if (utf8_branch_name.empty ())
      {
        utf8_branch_name =
          SK_WideCharToUTF8 (current_branch.release.vinfo.branch);
      }

      if (utf8_time_checked.empty ())
      {
        utf8_time_checked =
          SK_WideCharToUTF8 (SK_Version_GetLastCheckTime_WStr ());
      }

      ImGui::Text          ("You are currently using"); ImGui::SameLine ();
      ImGui::TextColored   (ImColor::HSV (.15f,.9f,1.f), "%s",
                            SK_GetVersionStrA ()); ImGui::SameLine ();
                            //utf8_release_title.c_str());ImGui::SameLine ();
    //ImGui::Text          ("from the");                ImGui::SameLine ();
    //ImGui::TextColored   (ImColor::HSV (.4f,.9f,1.f), "%s",
    //                      utf8_branch_name.c_str ()); ImGui::SameLine ();
    //ImGui::Text          ("development branch.");
      ImGui::Spacing       ();
      ImGui::Spacing       ();
      ImGui::TreePush      ("");
      ImGui::TextColored   (ImColor::HSV (.08f,.85f,1.f), "%s",
                            utf8_time_checked.c_str ());ImGui::SameLine ();
      ImGui::TextColored   (ImColor (1.f, 1.f, 1.f, 1.f), (const char *)u8"  ※  ");
                                                        ImGui::SameLine ();
      ImGui::TextColored   (version_color,               "%s",
                            utf8_release_description.c_str ());
      ImGui::TreePop       ();

      ImGui::EndChildFrame ();
    }

    else
    {
      ImGui::Text          ("");
      ImGui::Text          ("");
    }

    ImGui::Spacing         ();

    ImGui::TextUnformatted ("Press");                   ImGui::SameLine ();
    ImGui::TextColored     ( ImColor::HSV (.16f, 1.f, 1.f),
                               R"('%ws + %ws + %ws')",
                                    virtualToHuman [VK_CONTROL],
                                    virtualToHuman [VK_SHIFT],
                                    virtualToHuman [VK_BACK] );
                                                        ImGui::SameLine ();
    ImGui::TextUnformatted (", ");                      ImGui::SameLine ();
    ImGui::TextColored     ( ImColor::HSV (.16f, 1.f, 1.f),
                               R"('Select + Start' (PlayStation))" );
                                                        ImGui::SameLine ();
    ImGui::TextUnformatted ("or ");                     ImGui::SameLine ();
    ImGui::TextColored     ( ImColor::HSV (.16f, 1.f, 1.f),
                               R"('Back + Start' (Xbox))" );
                                                        ImGui::SameLine ();

    ImGui::TextUnformatted (  "to open Special K's configuration menu. " );

    ImGui::End             ( );
    ImGui::PopStyleColor   (2);
  }



  if (SK_ImGui_DrawCallback->fn != nullptr)
  {
    if (SK_ImGui_DrawCallback->fn (SK_ImGui_DrawCallback->data) > 0)
    {
      if (! SK_ImGui_Active ())
      {
        const ImGuiWindow* pWin =
          ImGui::FindWindowByName (
            "ReShade 3.0.8 by crosire; modified for Special K by Kaldaien"
            "###ReShade_Main"
          );

        if (pWin)
        {
          SK_ImGui_CenterCursorAtPos (pWin->Rect ().GetCenter ());
          ImGui::SetWindowFocus      (pWin->Name);
        }
      }

      SK_ReShade_Visible = true;
    }

    else
      SK_ReShade_Visible = false;
  }

  if (d3d11)
  {
    if (SK_ImGui_Widgets->texcache)
    {
      static bool move = true;
      if (move)
      {
        ImGui::SetNextWindowPos (
          ImVec2 ( (io.DisplaySize.x + font.size * 35) / 2.0f,
                                                         0.0f),
            ImGuiCond_Always
        );
        move = false;
      }

      float extra_lines = 0.0f;

      const LONG jobs =
        ( SK_D3D11_Resampler_GetActiveJobCount  () +
          SK_D3D11_Resampler_GetWaitingJobCount () );

      static DWORD dwLastActive = 0;

      if (jobs != 0 || dwLastActive > current_time - 500)
        extra_lines++;

      if (config.textures.cache.residency_managemnt)
      {
        auto* residency =
          SK_D3D11_TexCacheResidency.getPtr ();

        if (ReadAcquire (&residency->count.InVRAM)   > 0) extra_lines++;
        if (ReadAcquire (&residency->count.Shared)   > 0) extra_lines++;
        if (ReadAcquire (&residency->count.PagedOut) > 0) extra_lines++;
      }

      ImGui::SetNextWindowSize  (
        ImVec2 ( font.size * 35,
                 font.size_multiline * (7.25f + extra_lines)
               ), ImGuiCond_Always
      );

      ImGui::Begin ("###Widget_TexCacheD3D11",
        nullptr, ImGuiWindowFlags_NoTitleBar         | ImGuiWindowFlags_NoResize         |
                 ImGuiWindowFlags_NoScrollbar        | ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus );


      if (jobs != 0 || dwLastActive > current_time - 500)
      {
        if (jobs > 0)
          dwLastActive = current_time;

        if (jobs > 0)
          ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.4f - (0.4f * (SK_D3D11_Resampler_GetActiveJobCount ()) / (float)jobs), 0.15f, 1.0f));
        else
          ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.4f - (0.4f * (current_time - dwLastActive) / 500.0f), 1.0f, 0.8f));

        ImGui::SameLine       ();
        if (SK_D3D11_Resampler_GetErrorCount ())
          ImGui::Text         ("       Textures ReSampling (%li / %li) { Error Count: %li }",
                                 SK_D3D11_Resampler_GetActiveJobCount (),
                                   jobs,
                                 SK_D3D11_Resampler_GetErrorCount     ()
                              );
        else
          ImGui::Text         ("       Textures ReSampling (%li / %li)",
                                 SK_D3D11_Resampler_GetActiveJobCount (),
                                   jobs );
        ImGui::PopStyleColor  ();
      }


      SK_ImGui_DrawTexCache_Chart ( );

      if (ImGui::IsMouseClicked   (1) && ImGui::IsWindowHovered ())
        move = true;
      ImGui::End                  ( );
    }
  }



  SK_ImGui_ProcessWarnings ();


  if (SK_ImGui_UnconfirmedDisplayChanges)
  {
    SK_ImGui_ConfirmDisplaySettings (nullptr, L"", {});

    if ( ImGui::BeginPopupModal ( "Confirm Display Setting Changes",
                                    nullptr,
                                      ImGuiWindowFlags_AlwaysAutoResize |
                                      ImGuiWindowFlags_NoScrollbar      |
                                      ImGuiWindowFlags_NoScrollWithMouse )
       )
    {
      ImGui::TextColored ( ImColor::HSV (0.075f, 1.0f, 1.0f), "\n         Display Settings Will Revert in %4.1f Seconds...\n\n",
                                                15.0f - ( (float)SK_timeGetTime () - (float)SK_ImGui_DisplayChangeTime ) / 1000.0f );
      ImGui::Separator   ();

      ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f),     " Keep Changes?");

      ImGui::SameLine    (); ImGui::Spacing (); ImGui::SameLine ();

      if (ImGui::Button  ("Yes"))
      {
        SK_ImGui_UnconfirmedDisplayChanges = false;
      }

      if (ImGui::IsWindowAppearing ())
      {
        ImGui::GetIO ().NavActive  = true;
        ImGui::GetIO ().NavVisible = true;

        ImGui::SetNavID (
          ImGui::GetItemID (), 0
        );

        GImGui->NavDisableHighlight  = false;
        GImGui->NavDisableMouseHover =  true;
      }

      ImGui::SameLine    ();

      if (ImGui::Button  ("No"))
      {
        SK_ImGui_UnconfirmedDisplayChanges = true;
        SK_ImGui_DisplayChangeTime         =    0;

        ImGui::CloseCurrentPopup ();
      }

      ImGui::SameLine (); ImGui::Spacing (); ImGui::SameLine ();

      ImGui::Checkbox ( "Enable Confirmation",
                          &config.display.confirm_mode_changes );

      if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("If disabled, resolution changes will apply immediately with no confirmation.");

      ImGui::EndPopup ();
    }
  }


  if (SK_ImGui_WantExit)
  { SK_ReShade_Visible = true;

    SK_ImGui_ConfirmExit ();

    if ( ImGui::BeginPopupModal ( "Confirm Forced Software Termination",
                                    nullptr,
                                      ImGuiWindowFlags_AlwaysAutoResize |
                                      ImGuiWindowFlags_NoScrollbar      |
                                      ImGuiWindowFlags_NoScrollWithMouse )
       )
    {
      nav_usable = true;

      static const char* szConfirm =
        " Confirm Exit? ";
      static const char* szDisclaimer =
        "\n         You will lose any unsaved game progress.      \n\n";

      ImGui::TextColored (ImColor::HSV (0.075f, 1.0f, 1.0f), "%hs", szDisclaimer);
      ImGui::Separator   ();

      ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f),         szConfirm);

      ImGui::SameLine    (); ImGui::Spacing (); ImGui::SameLine ();


      if (ImGui::IsWindowAppearing ())
      {
        ImGui::GetIO ().MousePos        = ImGui::GetCursorScreenPos ();
        ImGui::GetIO ().WantSetMousePos = true;
      }

      if (ImGui::Button  ("Okay"))
      {
        SK_SelfDestruct     (   );
        SK_TerminateProcess (0x0);
        ExitProcess         (0x0);
      }
      if (ImGui::IsWindowAppearing ())
      {
        ImGui::GetIO ().NavActive  = true;
        ImGui::GetIO ().NavVisible = true;

        ImGui::SetNavID (
          ImGui::GetItemID (), 0
        );

        GImGui->NavDisableHighlight  = false;
        GImGui->NavDisableMouseHover =  true;
      }

      //ImGui::PushItemWidth (ImGui::GetWindowContentRegionWidth ()*0.33f);
      //ImGui::SameLine (); ImGui::SameLine (); ImGui::PopItemWidth ();

      ImGui::SameLine    ();

      if (ImGui::Button  ("Cancel"))
      {
        SK_ImGui_WantExit  = false;
        SK_ReShade_Visible = false;
        nav_usable         = false;
        ImGui::CloseCurrentPopup ();
      }

      ImGui::SameLine    ();
      ImGui::TextUnformatted (" ");
      ImGui::SameLine    ();

      if (ImGui::Checkbox ( "Enable Alt + F4",
                              &config.input.keyboard.catch_alt_f4 ))
      {
        // If user turns off here, then also turn off keyboard hook bypass
        if (! config.input.keyboard.catch_alt_f4)
              config.input.keyboard.override_alt_f4 = false;
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("If disabled, game's default Alt+F4 behavior will apply");

      ImGui::EndPopup       ();
    }
  }

  if (io.WantSetMousePos)
  {
    SK_ImGui_Cursor.pos.x = static_cast <LONG> (io.MousePos.x);
    SK_ImGui_Cursor.pos.y = static_cast <LONG> (io.MousePos.y);

    POINT screen_pos = SK_ImGui_Cursor.pos;

    SK_ImGui_Cursor.LocalToScreen (&screen_pos);
    SK_SetCursorPos ( screen_pos.x,
                      screen_pos.y );

    SK_ImGui_UpdateCursor ();
  }

  imgui_staged = true;
  imgui_staged_frames++;
}


extern IMGUI_API    void ImGui_ImplGL3_RenderDrawData  (ImDrawData* draw_data);
extern IMGUI_API    void ImGui_ImplDX9_RenderDrawData  (ImDrawData* draw_data);
extern IMGUI_API    void ImGui_ImplDX11_RenderDrawData (ImDrawData* draw_data);
extern/*IMGUI_API*/ void ImGui_ImplDX12_RenderDrawData (ImDrawData* draw_data, ID3D12GraphicsCommandList* ctx);

//
// Hook this to override Special K's GUI
//
__declspec (dllexport)
DWORD
SK_ImGui_DrawFrame ( _Unreferenced_parameter_ DWORD  dwFlags,
                                              LPVOID lpUser )
{
  UNREFERENCED_PARAMETER (dwFlags);
  UNREFERENCED_PARAMETER (lpUser);

  // Optionally:  Disable SK's OSD while the Steam overlay is active
  //
  if (  config.steam.overlay_hides_sk_osd &&
        SK::SteamAPI::GetOverlayState (true) )
  {
    return 0;
  }

  if (! imgui_staged)
  {
    SK_ImGui_StageNextFrame ();
  }

  auto& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.api == SK_RenderAPI::OpenGL)
  {
    ImGui::Render ();
    ImGui_ImplGL3_RenderDrawData (ImGui::GetDrawData ());
  }

  else if ( ( static_cast <int> (rb.api) &
              static_cast <int> (SK_RenderAPI::D3D9) ) != 0 )
  {
    auto pDev =
          rb.getDevice <IDirect3DDevice9> ();

    if ( pDev != nullptr && SUCCEEDED (
           pDev->BeginScene ()
         )
       )
    {
      ImGui::Render  ();
      ImGui_ImplDX9_RenderDrawData (ImGui::GetDrawData ());
      pDev->EndScene ();
    }
  }

  else if ( ( static_cast <int> (rb.api) &
              static_cast <int> (SK_RenderAPI::D3D11) ) != 0 )
  {
    ImGui::Render ();
    ImGui_ImplDX11_RenderDrawData (ImGui::GetDrawData ());
  }

  else if ( ( static_cast <int> (rb.api) &
              static_cast <int> (SK_RenderAPI::D3D12) ) != 0 )
  {
    if (_d3d12_rbk->frames_.size () > 0)
    {
      int swapIdx = 0;

      if (_d3d12_rbk->_pSwapChain != nullptr)
          swapIdx = _d3d12_rbk->_pSwapChain->GetCurrentBackBufferIndex ();

      SK_ReleaseAssert (
        swapIdx < static_cast <int> (_d3d12_rbk->frames_.size ())
      );

      void ImGui_ImplDX12_RenderDrawData ( ImDrawData*,
                         SK_D3D12_RenderCtx::FrameCtx* );

                                      ImGui::Render      ();
      ImGui_ImplDX12_RenderDrawData ( ImGui::GetDrawData (),
                                        &_d3d12_rbk->frames_ [swapIdx] );
    }
  }

  else
  {
    SK_LOG0 ( (L"No Render API"), L"Overlay" );
    return 0x0;
  }

  if (! keep_open)
    SK_ImGui_Toggle ();


  imgui_finished_frames++;


  imgui_staged = false;

  return 0;
}

__declspec (dllexport)
void
SK_ImGui_Toggle (void)
{
  static auto& io =
    ImGui::GetIO ();

  static ULONG64 last_frame = 0;

  if (last_frame != SK_GetFramesDrawn ())
  {
    current_time  = SK_timeGetTime    ();
    last_frame    = SK_GetFramesDrawn ();
  }

  static DWORD dwLastTime   = 0x00;

  bool d3d11 = false;
  bool d3d12 = false;
  bool d3d9  = false;
  bool gl    = false;

  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9) )  d3d9  = true;
  if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11))  d3d11 = true;
  if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D12))  d3d12 = true;
  if (                   rb.api ==                    SK_RenderAPI::OpenGL)  gl    = true;

  auto EnableEULAIfPirate = [&](void) ->
  bool
  {
    bool pirate = ( SK_SteamAPI_AppID    () != 0 &&
                    SK_Steam_PiratesAhoy () != 0x0 );
    if (pirate)
    {
      if (dwLastTime < current_time - 1000)
      {
        dwLastTime             = current_time;

        eula.show              = true;
        eula.never_show_again  = false;

        return true;
      }
    }

    return false;
  };

  if (d3d9 || d3d11 || d3d12 || gl)
  {
    SK_ImGui_Visible = (! SK_ImGui_Visible);

    if (SK_ImGui_Visible)
      ImGui::SetNextWindowFocus ();

    static HMODULE hModTBFix = SK_GetModuleHandle (L"tbfix.dll");
    static HMODULE hModTZFix = SK_GetModuleHandle (L"tzfix.dll");

    // Turns the hardware cursor on/off as needed
    ImGui_ToggleCursor ();

    // Most games
    if (! hModTBFix)
    {
      // Transition: (Visible -> Invisible)
      if (! SK_ImGui_Visible)
      {
        // Set the last update time really far in the past to hurry along
        //   idle cursor detection to hide the mouse cursor after closing
        //     the control panel.
        SK_ImGui_Cursor.last_move = 0;

        //SK_ImGui_Cursor.showSystemCursor ();
      }

      else
      {
        SK_ImGui_Cursor.last_move = current_time;

        //SK_ImGui_Cursor.showImGuiCursor ();

        if (EnableEULAIfPirate ())
          config.imgui.show_eula = true;

        static bool first = true;

        if (first)
        {
          eula.never_show_again = true;
          eula.show             = config.imgui.show_eula;
          first                 = false;
        }
      }
    }

    if (SK_ImGui_Visible)
    {
      // Reuse the game's overlay activation callback (if it hase one)
      if (config.steam.reuse_overlay_pause)
        SK::SteamAPI::SetOverlayState (true);

      SK_Console::getInstance ()->visible = false;
    }


    // Request the number of players in-game whenever the
    //   config UI is toggled ON.
    if (SK::SteamAPI::AppID () != 0 && SK_ImGui_Visible)
        SK::SteamAPI::UpdateNumPlayers ();




    const wchar_t* config_name = SK_GetBackend ();

    if (SK_IsInjected ())
      config_name = L"SpecialK";

    SK_SaveConfig (config_name);


    // Immediately stop capturing keyboard/mouse events,
    //   this is the only way to preserve cursor visibility
    //     in some games (i.e. Tales of Berseria)
////io.WantCaptureKeyboard = (! SK_ImGui_Visible);
////io.WantCaptureMouse    = (! SK_ImGui_Visible);


    // Clear navigation focus on window close
    if (! SK_ImGui_Visible)
    {
      // Reuse the game's overlay activation callback (if it hase one)
      if (config.steam.reuse_overlay_pause)
        SK::SteamAPI::SetOverlayState (false);

      nav_usable = false;
    }

  //reset_frame_history = true;
  }

  if ((! SK_ImGui_Visible) && SK_ImGui_OpenCloseCallback->fn != nullptr)
  {
    if (SK_ImGui_OpenCloseCallback->fn (SK_ImGui_OpenCloseCallback->data))
      SK_ImGui_OpenCloseCallback->fn (SK_ImGui_OpenCloseCallback->data);
  }
}




void
SK_RBkEnd_UpdateMonitorName ( SK_RenderBackend_V2::output_s& display,
                              DXGI_OUTPUT_DESC&              outDesc );
void
SK_DXGI_UpdateColorSpace    ( IDXGISwapChain3*   This,
                              DXGI_OUTPUT_DESC1* outDesc = nullptr );

const wchar_t*
DXGIColorSpaceToStr (DXGI_COLOR_SPACE_TYPE space) noexcept;

void
SK_Display_UpdateOutputTopology (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if ( rb.swapchain != nullptr &&
       rb.device    != nullptr )
  {
    rb.updateOutputTopology ();

    return;
  }

  static
    CreateDXGIFactory1_pfn
   _CreateDXGIFactory1 ( CreateDXGIFactory1_Import != nullptr ?
                         CreateDXGIFactory1_Import            :
    CreateDXGIFactory1 );

  static SK_ComPtr <IDXGIFactory1> pFactory1;
         SK_ComPtr <IDXGIAdapter > pAdapter;

  if (   pFactory1 != nullptr &&
      (! pFactory1->IsCurrent ()))
         pFactory1.Release    ();

  auto                        _GetAdapter
= [&] (SK_ComPtr <IDXGIAdapter>& pAdapter)
->bool
  {
    try
    {
      ThrowIfFailed (
         (nullptr == pFactory1.p) ?
           _CreateDXGIFactory1  (
             IID_IDXGIFactory1,
           (void **)&pFactory1.p) :     S_OK );
      ThrowIfFailed (pFactory1->EnumAdapters
                             ( 0, &pAdapter ));

      return true;
    }

    catch (const SK_ComException& e)
    {
      SK_LOG0 ( (  L"%ws -> %hs",
                   __FUNCTIONW__, e.what ()
                ), L"MonitorTop" );

      return false;
    }
  };

  if (_GetAdapter (pAdapter))
  {
    LUID luidAdapter = { };

    /////SK_ComPtr <ID3D12Device>                             pDevice12; // D3D12
    /////if (SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&pDevice12.p))))
    /////{
    /////  // Yep, now for the fun part
    /////  luidAdapter =
    /////    pDevice12->GetAdapterLuid ();
    /////}

    // D3D11 or something else
    {
      DXGI_ADAPTER_DESC   adapterDesc = { };
      pAdapter->GetDesc (&adapterDesc);

      luidAdapter =
        adapterDesc.AdapterLuid;
    }

    static constexpr
      size_t display_size = sizeof (SK_RenderBackend_V2::output_s),
         num_displays     = sizeof ( rb.displays) /
                                     display_size;

    bool display_changed [num_displays] = { };

    for ( auto idx = 0 ; idx < num_displays ; ++idx )
    {
      RtlSecureZeroMemory (
        &rb.displays [idx], display_size
      );
    }

    UINT                             idx = 0;
    SK_ComPtr <IDXGIOutput>                pOutput;

    while ( DXGI_ERROR_NOT_FOUND !=
              pAdapter->EnumOutputs (idx, &pOutput.p) )
    {
      auto& display =
        rb.displays [idx];

      if (pOutput.p != nullptr)
      {
        DXGI_OUTPUT_DESC1 outDesc1 = { };
        DXGI_OUTPUT_DESC  outDesc  = { };

        if (SUCCEEDED (pOutput->GetDesc (&outDesc)))
        {
          display.idx      = idx;
          display.monitor  = outDesc.Monitor;
          display.rect     = outDesc.DesktopCoordinates;
          display.attached = outDesc.AttachedToDesktop;


          if (sk::NVAPI::nv_hardware != false)
          {
            NvPhysicalGpuHandle nvGpuHandles [NVAPI_MAX_PHYSICAL_GPUS] = { };
            NvU32               nvGpuCount                             =   0;
            NvDisplayHandle     nvDisplayHandle;
            NvU32               nvOutputId  = std::numeric_limits <NvU32>::max ();
            NvU32               nvDisplayId = std::numeric_limits <NvU32>::max ();

            if ( NVAPI_OK ==
                   NvAPI_GetAssociatedNvidiaDisplayHandle (
                     SK_FormatString (R"(%ws)", outDesc.DeviceName).c_str (),
                       &nvDisplayHandle
                   ) &&
                 NVAPI_OK ==
                   NvAPI_GetAssociatedDisplayOutputId (nvDisplayHandle, &nvOutputId) &&
                 NVAPI_OK ==
                   NvAPI_GetPhysicalGPUsFromDisplay   (nvDisplayHandle, nvGpuHandles,
                                                                       &nvGpuCount)  &&
                 NVAPI_OK ==
                   NvAPI_SYS_GetDisplayIdFromGpuAndOutputId (
                     nvGpuHandles [0], nvOutputId,
                                      &nvDisplayId
                   )
               )
            {
            //display.nvapi.display_handle = nvDisplayHandle;
            //display.nvapi.gpu_handle     = nvGpuHandles [0];
            //display.nvapi.output_id      = nvOutputId;
              display.nvapi.display_id     = nvDisplayId;
            }
          }

          wcsncpy_s ( display.dxgi_name,  32,
                      outDesc.DeviceName, _TRUNCATE );

          SK_RBkEnd_UpdateMonitorName (display, outDesc);

          MONITORINFO
            minfo        = {                  };
            minfo.cbSize = sizeof (MONITORINFO);

          GetMonitorInfoA (display.monitor, &minfo);

          display.primary =
            ( minfo.dwFlags & MONITORINFOF_PRIMARY );

          SK_ComQIPtr <IDXGIOutput6>
              pOutput6 (pOutput);
          if (pOutput6.p != nullptr)
          {
            if (SUCCEEDED (pOutput6->GetDesc1 (&outDesc1)))
            {
              if (outDesc1.MinLuminance > outDesc1.MaxFullFrameLuminance)
                std::swap (outDesc1.MinLuminance, outDesc1.MaxFullFrameLuminance);

              static bool once = false;

              if (! std::exchange (once, true))
              {
                SK_LOG0 ( (L" --- Working around DXGI bug, swapping invalid min / avg luminance levels"),
                           L"   DXGI   "
                        );
              }

              display.bpc               = outDesc1.BitsPerColor;
              display.gamut.minY        = outDesc1.MinLuminance;
              display.gamut.maxY        = outDesc1.MaxLuminance;
              display.gamut.maxLocalY   = outDesc1.MaxLuminance;
              display.gamut.maxAverageY = outDesc1.MaxFullFrameLuminance;
              display.gamut.xr          = outDesc1.RedPrimary   [0];
              display.gamut.yr          = outDesc1.RedPrimary   [1];
              display.gamut.xb          = outDesc1.BluePrimary  [0];
              display.gamut.yb          = outDesc1.BluePrimary  [1];
              display.gamut.xg          = outDesc1.GreenPrimary [0];
              display.gamut.yg          = outDesc1.GreenPrimary [1];
              display.gamut.Xw          = outDesc1.WhitePoint   [0];
              display.gamut.Yw          = outDesc1.WhitePoint   [1];
              display.gamut.Zw          = 1.0f - display.gamut.Xw - display.gamut.Yw;
              display.colorspace        = outDesc1.ColorSpace;

              display.hdr               = outDesc1.ColorSpace ==
                DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            }
          }
        }
      }

      rb.stale_display_info = false;

      pOutput.Release ();

      idx++;
    }

    idx = 0;

    while ( DXGI_ERROR_NOT_FOUND !=
              pAdapter->EnumOutputs (idx, &pOutput.p) )
    {
      auto& display =
        rb.displays [idx];

      if (pOutput.p != nullptr)
      {
        auto old_crc =
         rb.display_crc [idx];

         rb.display_crc [idx] =
                 crc32c ( 0,
        &display,
         display_size   );

        display_changed [idx] =
          old_crc != rb.display_crc [idx];

        SK_ComQIPtr <IDXGISwapChain> pChain (rb.swapchain.p);

        DXGI_SWAP_CHAIN_DESC swapDesc         = { };
        RECT                 rectOutputWindow = { };

        if (pChain.p != nullptr)
        {
          pChain->GetDesc (&swapDesc);
          GetWindowRect   ( swapDesc.OutputWindow, &rectOutputWindow);
        }

        auto pContainer =
          rb.getContainingOutput (rectOutputWindow);

        if (pContainer != nullptr)
        {
          if (pContainer->monitor == display.monitor)
          {
            wcsncpy_s ( rb.display_name,    128,
                        display.name, _TRUNCATE );

            // Late init
            if (rb.monitor != display.monitor)
                rb.monitor  = display.monitor;

            rb.display_gamut.xr = display.gamut.xr;
            rb.display_gamut.yr = display.gamut.yr;
            rb.display_gamut.xg = display.gamut.xg;
            rb.display_gamut.yg = display.gamut.yg;
            rb.display_gamut.xb = display.gamut.xb;
            rb.display_gamut.yb = display.gamut.yb;
            rb.display_gamut.Xw = display.gamut.Xw;
            rb.display_gamut.Yw = display.gamut.Yw;
            rb.display_gamut.Zw = 1.0f - display.gamut.Xw - display.gamut.Yw;

            rb.display_gamut.minY        = display.gamut.minY;
            rb.display_gamut.maxY        = display.gamut.maxY;
            rb.display_gamut.maxLocalY   = display.gamut.maxY;
            rb.display_gamut.maxAverageY = display.gamut.maxAverageY;

            if (display.attached)
            {
              rb.scanout.dwm_colorspace =
                display.colorspace;
            }

            SK_ComQIPtr <IDXGISwapChain3>
                   pSwap3 (rb.swapchain.p);
            if (   pSwap3)
            {
              // Windows tends to cache this stuff, we're going to build our own with
              //   more up-to-date values instead.
              DXGI_OUTPUT_DESC1 uncachedOutDesc;
              uncachedOutDesc.BitsPerColor = pContainer->bpc;
              uncachedOutDesc.ColorSpace   = pContainer->colorspace;

              SK_DXGI_UpdateColorSpace (pSwap3.p, &uncachedOutDesc);

              if ((! rb.isHDRCapable ()) && __SK_HDR_16BitSwap)
              {
                SK_RunOnce (
                  SK_ImGui_WarningWithTitle (
                    L"ERROR: Special K HDR Applied to a non-HDR Display\r\n\r\n"
                    L"\t\t>> Please Disable SK HDR or set the Windows Desktop to use HDR",
                                             L"HDR is Unsupported by the Active Display" )
                );
              }
            }
          }
        }

        if (display_changed [idx])
        {
          dll_log->LogEx ( true,
            L"[Output Dev]\n"
            L"  +------------------+---------------------\n"
            L"  | EDID Device Name |  %ws\n"
            L"  | DXGI Device Name |  %ws (HMONITOR: %x)\n"
            L"  | Desktop Display. |  %ws%ws\n"
            L"  | Bits Per Color.. |  %u\n"
            L"  | Color Space..... |  %s\n"
            L"  | Red Primary..... |  %f, %f\n"
            L"  | Green Primary... |  %f, %f\n"
            L"  | Blue Primary.... |  %f, %f\n"
            L"  | White Point..... |  %f, %f\n"
            L"  | Min Luminance... |  %f\n"
            L"  | Max Luminance... |  %f\n"
            L"  |  \"  FullFrame... |  %f\n"
            L"  +------------------+---------------------\n",
              display.name,
              display.dxgi_name, display.monitor,
              display.attached ? L"Yes"                : L"No",
              display.primary  ? L" (Primary Display)" : L"",
                          display.bpc,
              DXGIColorSpaceToStr (display.colorspace),
              display.gamut.xr,    display.gamut.yr,
              display.gamut.xg,    display.gamut.yg,
              display.gamut.xb,    display.gamut.yb,
              display.gamut.Xw,    display.gamut.Yw,
              display.gamut.minY,  display.gamut.maxY,
              display.gamut.maxAverageY
          );
        }

        pOutput.Release ();
      }

      idx++;
    }

    bool any_changed =
      std::count ( std::begin (display_changed),
                   std::end   (display_changed), true ) > 0;

    if (any_changed)
    {
      for ( UINT i = 0 ; i < idx; ++i )
      {
        SK_LOG0 ( ( L"%s Monitor %i: [ %ix%i | (%5i,%#5i) ] \"%ws\" :: %s",
                      rb.displays [i].primary ? L"*" : L" ",
                      rb.displays [i].idx,
                      rb.displays [i].rect.right  - rb.displays [i].rect.left,
                      rb.displays [i].rect.bottom - rb.displays [i].rect.top,
                      rb.displays [i].rect.left,    rb.displays [i].rect.top,
                      rb.displays [i].name,
                      rb.displays [i].hdr ? L"HDR" : L"SDR" ),
                    L"   DXGI   " );
      }
    }

    SK_Display_ResolutionSelectUI (true);
  }
}

///void
///SK_Display_GetMonitorName ( HMONITOR hMon,
///                            wchar_t* wszName )
///{
///  static auto& rb =
///    SK_GetCurrentRenderBackend ();
///
///  if (*display.name == L'\0')
///  {
///    std::string edid_name;
///
///    UINT devIdx = display.idx;
///
///    wsprintf (display.name, L"UNKNOWN");
///
///    bool nvSuppliedEDID = false;
///
///    // This is known to return EDIDs with checksums that don't match expected,
///    //   there's not much benefit to getting EDID this way, so use the registry instead.
///#if 1
///    if (sk::NVAPI::nv_hardware != false)
///    {
///      NvPhysicalGpuHandle nvGpuHandles [NVAPI_MAX_PHYSICAL_GPUS] = { };
///      NvU32               nvGpuCount                             =   0;
///      NvDisplayHandle     nvDisplayHandle;
///      NvU32               nvOutputId  = std::numeric_limits <NvU32>::max ();
///      NvU32               nvDisplayId = std::numeric_limits <NvU32>::max ();
///
///      if ( NVAPI_OK ==
///             NvAPI_GetAssociatedNvidiaDisplayHandle (
///               SK_FormatString (R"(%ws)", outDesc.DeviceName).c_str (),
///                 &nvDisplayHandle
///             ) &&
///           NVAPI_OK ==
///             NvAPI_GetAssociatedDisplayOutputId (nvDisplayHandle, &nvOutputId) &&
///           NVAPI_OK ==
///             NvAPI_GetPhysicalGPUsFromDisplay   (nvDisplayHandle, nvGpuHandles,
///                                                                 &nvGpuCount)  &&
///           NVAPI_OK ==
///             NvAPI_SYS_GetDisplayIdFromGpuAndOutputId (
///               nvGpuHandles [0], nvOutputId,
///                                &nvDisplayId
///             )
///         )
///      {
///        NV_EDID edid = {         };
///        edid.version = NV_EDID_VER;
///
///        if ( NVAPI_OK ==
///               NvAPI_GPU_GetEDID (
///                 nvGpuHandles [0],
///                 nvDisplayId, &edid
///               )
///           )
///        {
///          edid_name =
///            rb.parseEDIDForName ( edid.EDID_Data, edid.sizeofEDID );
///
///          auto nativeRes =
///            rb.parseEDIDForNativeRes ( edid.EDID_Data, edid.sizeofEDID );
///
///          display.native.width  = nativeRes.x;
///          display.native.height = nativeRes.y;
///
///          if (! edid_name.empty ())
///          {
///            nvSuppliedEDID = true;
///          }
///        }
///      }
///    }
///#endif
///
///    *display.name = L'\0';
///
///     //@TODO - ELSE: Test support for various HDR colorspaces.
///
///    DISPLAY_DEVICEW        disp_desc = { };
///    disp_desc.cb = sizeof (disp_desc);
///
///    if (EnumDisplayDevices ( outDesc.DeviceName, 0,
///                               &disp_desc, EDD_GET_DEVICE_INTERFACE_NAME ))
///    {
///      if (! nvSuppliedEDID)
///      {
///        HDEVINFO devInfo =
///          SetupDiGetClassDevsEx ( &GUID_CLASS_MONITOR,
///                                    nullptr, nullptr,
///                                      DIGCF_PRESENT,
///                                        nullptr, nullptr, nullptr );
///
///        if ((! nvSuppliedEDID) && devInfo != nullptr)
///        {
///          wchar_t   wszDevName [ 64] = { };
///          wchar_t   wszDevInst [128] = { };
///          wchar_t* pwszTok           = nullptr;
///
///          swscanf (disp_desc.DeviceID, LR"(\\?\DISPLAY#%63ws)", wszDevName);
///
///          pwszTok =
///            StrStrIW (wszDevName, L"#");
///
///          if (pwszTok != nullptr)
///          {
///            *pwszTok = L'\0';
///            wcsncpy_s ( wszDevInst,  128,
///                        pwszTok + 1, _TRUNCATE );
///
///            pwszTok =
///              StrStrIW (wszDevInst, L"#");
///
///            if (pwszTok != nullptr)
///               *pwszTok  = L'\0';
///
///
///            uint8_t EDID_Data [256] = { };
///            DWORD   edid_size       =  sizeof (EDID_Data);
///
///
///            DWORD   dwType = REG_NONE;
///            LRESULT lStat  =
///              RegGetValueW ( HKEY_LOCAL_MACHINE,
///                SK_FormatStringW ( LR"(SYSTEM\CurrentControlSet\Enum\DISPLAY\)"
///                                   LR"(%ws\%ws\Device Parameters)",
///                                     wszDevName, wszDevInst ).c_str (),
///                              L"EDID",
///                                RRF_RT_REG_BINARY, &dwType,
///                                  EDID_Data, &edid_size );
///
///            if (ERROR_SUCCESS == lStat)
///            {
///              edid_name =
///                rb.parseEDIDForName ( EDID_Data, edid_size );
///
///              auto nativeRes =
///                rb.parseEDIDForNativeRes ( EDID_Data, edid_size );
///
///              display.native.width  = nativeRes.x;
///              display.native.height = nativeRes.y;
///            }
///
///            //DISPLAY#MEIA296#5&2dafe0a1&3&UID41221#
///          }
///        }
///      }
///
///      if (EnumDisplayDevices (outDesc.DeviceName, 0, &disp_desc, 0))
///      {
///        if (SK_GetCurrentRenderBackend ().display_crc [display.idx] == 0)
///        {
///          dll_log->Log ( L"[Output Dev] DeviceName: %ws, devIdx: %lu, DeviceID: %ws",
///                           disp_desc.DeviceName, devIdx,
///                             disp_desc.DeviceID );
///        }
///
///        wsprintf ( display.name, edid_name.empty () ?
///                                    LR"(%ws (%ws))" :
///                                      L"%hs",
///                     edid_name.empty () ?
///                       disp_desc.DeviceString :
///            (WCHAR *)edid_name.c_str (),
///                         disp_desc.DeviceName );
///      }
///    }
///  }
///}

///BOOL
///CALLBACK
///SK_Display_MonitorEnumProc ( HMONITOR hMonitor,
///                             HDC      hDC,
///                             LPRECT   lpRect,
///                             LPARAM   lParam )
///{
///  SK_Monitor_s monitor = {      };
///     monitor.hMonitor  = hMonitor;
///     monitor.rcRect    =  *lpRect;
///
///  SK_Monitors->push_back (
///    monitor
///  );
///
///  return TRUE;
///}
///
///void
///SK_Display_UpdateMonitors (void)
///{
///  SK_Monitors->clear ();
///
///  if (
///    EnumDisplayMonitors ( nullptr, nullptr,
///    SK_Display_MonitorEnumProc, (LPARAM)&SK_Monitors
///                        )
///     )
///  {
///    // ...
///  }
///}