  
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
#include <implot/implot.h>

#include <SpecialK/control_panel/compatibility.h>
#include <SpecialK/control_panel/d3d11.h>
#include <SpecialK/control_panel/d3d9.h>
//#include <SpecialK/control_panel/d3d12.h>
#include <SpecialK/control_panel/input.h>
#include <SpecialK/control_panel/opengl.h>
#include <SpecialK/control_panel/osd.h>
#include <SpecialK/control_panel/plugins.h>
#include <SpecialK/control_panel/sound.h>
#include <SpecialK/control_panel/platform.h>
#include <SpecialK/control_panel/window.h>

#include <SpecialK/storefront/epic.h>

#include <SpecialK/nvapi.h>

#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <imgui/font_awesome.h>

#include <filesystem>

LONG imgui_staged_frames   = 0;
LONG imgui_finished_frames = 0;
BOOL imgui_staged          = FALSE;

bool imgui_demo    = false;
bool imgui_debug   = false;
bool imgui_metrics = false;
bool imgui_about   = false;
bool implot_demo   = false;

extern float g_fDPIScale;

using namespace SK::ControlPanel;

SK_RenderAPI                 SK::ControlPanel::render_api;
DWORD                        SK::ControlPanel::current_time = 0;
uint64_t                     SK::ControlPanel::current_tick;// Perf Counter
SK::ControlPanel::font_cfg_s SK::ControlPanel::font;

DWORD
SK_GetCurrentMS (void) noexcept
{
  // Handle possible scenario where SK uses this before
  // the control panel is initialized / rendered
  if (SK::ControlPanel::current_time == 0)
      SK::ControlPanel::current_time = SK_timeGetTime ();

  return
    SK::ControlPanel::current_time;
}

static const auto Keybinding =
[] (SK_ConfigSerializedKeybind* binding, sk::ParameterStringW* param) ->
auto
{
  if (! (binding != nullptr && param != nullptr))
    return false;

  std::string label =
    SK_WideCharToUTF8 (binding->human_readable);

  ImGui::PushID (binding->bind_name);

  if (SK_ImGui_KeybindSelect (binding, label.c_str ()))
  {
    ImGui::OpenPopup (        binding->bind_name);
                              binding->assigning = true;
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

void
SK_Display_UpdateOutputTopology (void);

bool __imgui_alpha = true;

SK_API
void
SK_ImGui_Toggle (void);

extern bool __stdcall SK_FAR_IsPlugIn      (void);
extern void __stdcall SK_FAR_ControlPanel  (void);

std::string
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

void SK_DPI_Update (void)
{
  using  GetDpiForSystem_pfn = UINT (WINAPI *)(void);
  static
    auto GetDpiForSystem =         (GetDpiForSystem_pfn)
    SK_GetProcAddress ( L"user32", "GetDpiForSystem" );

  using  GetDpiForWindow_pfn = UINT (WINAPI *)(HWND);
  static
    auto GetDpiForWindow =         (GetDpiForWindow_pfn)
    SK_GetProcAddress ( L"user32", "GetDpiForWindow" );

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
       bool IMGUI_API SK_ImGui_Visible          = false;
       bool           SK_ImGuiEx_Visible        = false;
       bool           SK_ControlPanel_Activated = false;

extern void ImGui_ToggleCursor (void);

bool SK_ImGui_WantExit    = false;
bool SK_ImGui_WantRestart = false;

#pragma warning (push)
#pragma warning (disable: 4706)
// Special K Extensions to ImGui
namespace SK_ImGui
{

  float
  SanitizeFontGlobalScale (float scale)
  {
    return std::max (1.0f, std::min (5.0f, scale));
  }

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

    SYSTEM_POWER_STATUS  sps = { };
    SYSTEM_BATTERY_STATE sbs = { };

    const NTSTATUS ntStatus =
      CallNtPowerInformation ( SystemBatteryState,
                               nullptr, 0,
                               &sbs, sizeof (sbs) );

    if (has_battery || (SUCCEEDED (ntStatus) && sbs.Rate != 0))
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

        if (battery_level < 255 || sbs.Rate != 0) // 255 = Running on AC (not charging)
        {
          if (battery_level  > 100)
              battery_level -= 100;

          if (battery_level  > 100) // If we're still > 100, it's because running on AC.
              battery_level  = 100;

          const float battery_ratio = (float)battery_level/100.0f;

          static char szBatteryLevel [128] = { };

          if (sps.BatteryLifeTime != -1)
            snprintf (szBatteryLevel, 127, sbs.Rate != 0 ? "%hhu%% Battery Remaining\t\t[%lu Minutes, %5.1f W]" 
                                                         : "%hhu%% Battery Remaining\t\t[%lu Minutes]",
                                                                               battery_level,
                                                                    sps.BatteryLifeTime / 60,
                                                        static_cast <double> (
                                                      sk::narrow_cast <LONG> (sbs.Rate)
                                                                             )         / 1000.0);
          else if (charging)
            snprintf (szBatteryLevel, 127, sbs.Rate != 0 ? "%hhu%% Battery Charged, %5.1f W" :
                                                           "%hhu%% Battery Charged",
                                                                               battery_level,
                                                        static_cast <double> (
                                                      sk::narrow_cast <LONG> (sbs.Rate)
                                                                             )         / 1000.0);
          else
            snprintf (szBatteryLevel, 127, sbs.Rate != 0 ? "%hhu%% Battery Remaining, %5.1f W" :
                                                           "%hhu%% Battery Remaining",
                                                                               battery_level,
                                                        static_cast <double> (
                                                      sk::narrow_cast <LONG> (sbs.Rate)
                                                                             )         / 1000.0);

          float luminance =
            charging ?
                0.166f + (0.5f + (sin ((float)(current_time % 2000) / 2000.0f)) * 0.5f) / 2.0f :
                0.666f;

          ImGui::PushStyleColor (ImGuiCol_PlotHistogram,  ImColor::HSV (battery_ratio * 0.278f, 0.88f, luminance).Value);
          ImGui::PushStyleColor (ImGuiCol_Text,           ImColor (255, 255, 255).Value);
          ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor ( 0.3f,  0.3f,  0.3f, 0.7f).Value);
          ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor ( 0.6f,  0.6f,  0.6f, 0.8f).Value);
          ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor ( 0.9f,  0.9f,  0.9f, 0.9f).Value);

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
SKIF_ImGui_PushDisableState (void)
{
  // Push the states in a specific order
  ImGui::PushItemFlag   (ImGuiItemFlags_Disabled, true);
  //ImGui::PushStyleVar (ImGuiStyleVar_Alpha,     ImGui::GetStyle ().Alpha * 0.5f); // [UNUSED]
  ImGui::PushStyleColor (ImGuiCol_Text,           ImGui::GetStyleColorVec4 (ImGuiCol_TextDisabled));
  ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImLerp (ImGui::GetStyleColorVec4 (ImGuiCol_WindowBg), ImGui::GetStyleColorVec4 (ImGuiCol_TextDisabled), 0.50f));
  ImGui::PushStyleColor (ImGuiCol_CheckMark,      ImLerp (ImGui::GetStyleColorVec4 (ImGuiCol_WindowBg), ImGui::GetStyleColorVec4 (ImGuiCol_TextDisabled), 0.50f));
  ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImLerp (ImGui::GetStyleColorVec4 (ImGuiCol_WindowBg), ImGui::GetStyleColorVec4 (ImGuiCol_FrameBg),      0.25f));
}

void
SKIF_ImGui_PopDisableState (void)
{
  // Pop the states in the reverse order that we pushed them in
  ImGui::PopStyleColor ( ); // ImGuiCol_FrameBg
  ImGui::PopStyleColor ( ); // ImGuiCol_CheckMark
  ImGui::PopStyleColor ( ); // ImGuiCol_SliderGrab
  ImGui::PopStyleColor ( ); // ImGuiCol_Text
  //ImGui::PopStyleVar ( ); // ImGuiStyleVar_Alpha [UNUSED]
  ImGui::PopItemFlag   ( ); // ImGuiItemFlags_Disabled
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

  if (SK_ImGui_WantExit)
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
  SK_ImGui_Visible = true;

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
    SK_ImGuiEx_Visible = true;

    ImGui::TextColored ( ImColor::HSV (0.075f, 1.0f, 1.0f),
                           "\n\t%hs\t\n\n", SK_WideCharToUTF8 (warning.message).c_str () );

    ImGui::Separator   ();
    ImGui::TextColored ( ImColor::HSV (0.15f, 1.0f, 1.0f), "   " );
    ImGui::SameLine    ();
    ImGui::Spacing     ();
    ImGui::SameLine    ();

    if (ImGui::Button ("Okay"))
    {
      SK_ImGuiEx_Visible = false;
      SK_ReShadeAddOn_ActivateOverlay (false);

      warning.message.clear ();

      ImGui::CloseCurrentPopup ();
    }
    ImGui::EndPopup       ();
  }
}

static bool orig_nav_state           = false;
static bool dxgi_mode_switch_failure = false;

void
SK_ImGui_ConfirmExit (void)
{
  static const auto& io =
    ImGui::GetIO ();

  SK_ImGui_SetNextWindowPosCenter     (ImGuiCond_Always);
  ImGui::SetNextWindowSizeConstraints ( ImVec2 (360.0f, 40.0f),
                                          ImVec2 ( 0.925f * io.DisplaySize.x,
                                                   0.925f * io.DisplaySize.y )
                                      );

  if (! ImGui::IsPopupOpen ( SK_ImGui_WantRestart ?
                "Confirm Forced Software Restart" :
                "Confirm Forced Software Termination" ) )
  {
    SK_ImGui_WantExit = false;
    orig_nav_state    = nav_usable;

    ImGui::OpenPopup ( SK_ImGui_WantRestart ?
          "Confirm Forced Software Restart" :
          "Confirm Forced Software Termination" );
  }
}

void
SK_ImGui_ReportModeSwitchFailure (void)
{
  dxgi_mode_switch_failure = true;
}

bool  SK_ImGui_UnconfirmedDisplayChanges = false;
DWORD SK_ImGui_DisplayChangeTime         = 0;

void
SK_ImGui_ConfirmDisplaySettings (bool *pDirty_, std::string display_name_, DEVMODEW orig_mode_)
{
  static bool              *pDirty = nullptr;
  static std::string  display_name = "";
  static DEVMODEW        orig_mode = { };
  static DWORD         dwInitiated = DWORD_MAX;

  // We're initiating
  if (pDirty_ != nullptr)
  {
    pDirty       = pDirty_;
    display_name = std::move (display_name_);
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
      SK_ChangeDisplaySettingsEx (SK_UTF8ToWideChar (
                   display_name).c_str (), &orig_mode,
                                        0, 0x0, nullptr
      );

      *pDirty = true;

      dwInitiated  = DWORD_MAX;
      pDirty       = nullptr;
      orig_mode    = { };
      display_name = "";

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

  // Not possible to implement the original behavior following ImGui
  //   codebase changes... plan on removing this.
#if 0
  if (ImGui::IsItemHovered ())
  {
    //auto& io =
    //  ImGui::GetIO ();

    /// XXX: FIXME
    //if (io.NavInputs [ImGuiNavInput_Activate] == 0.0f)
    //{
    //  return true;
    //}
  }
#endif

  return false;
}

bool
SK_ImGui_IsItemRightClicked (void)
{
  if (ImGui::IsItemClicked (1))
    return true;

  if (ImGui::IsItemHovered ())
  {
    auto& io =
      ImGui::GetIO ();

    // Activate button held for >= .4 seconds -> right-click
    if (io.NavInputs [ImGuiNavInput_Activate] > 0.4f)
    {   io.NavInputs [ImGuiNavInput_Activate] = 0.0f;
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

void SK_ImGui_CenterCursorAtPos (ImVec2& center = SK_ImGui_LastWindowCenter);
void SK_ImGui_UpdateCursor      (void);

const char*
SK_ImGui_ControlPanelTitle (void)
{
  static        char szTitle [512] = { };
  const         bool bSteam        = (SK::SteamAPI::AppID () != 0x0);
  static const  bool bEpic         = StrStrIA (GetCommandLineA (), "-epicapp");

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  static bool bTBFix = SK_GetModuleHandle (L"tbfix.dll") != nullptr;

  static std::string title;
                     title.clear ();

  if (SK_HasPlugin () && (! bTBFix))
    title += SK_WideCharToUTF8 (SK_GetPluginName ());
  else
  {
    title += "Special K  (v ";
    title += SK_GetVersionStrA ();
    title += ")";
  }

  title += "  Control Panel";

  extern __time64_t __SK_DLL_AttachTime;

  __time64_t now     = 0ULL;
   _time64 (&now);

  auto       elapsed = sk::narrow_cast <uint32_t> (now - __SK_DLL_AttachTime);

  uint32_t   secs    =  elapsed % 60ULL;
  uint32_t   mins    = (elapsed / 60ULL) % 60ULL;
  uint32_t   hours   =  elapsed / 3600ULL;

  if (bSteam || bEpic || *rb.windows.focus.title != L'\0')
  {
    static std::string window_title =
      SK_WideCharToUTF8 (rb.windows.focus.title);

    // For non-Steam/Epic games, if the window title changes, then update
    //   the control panel's title...
    if (! (bSteam || bEpic))
    {
      static ULONG64     last_changed = 0;
      if (std::exchange (last_changed, rb.windows.focus.last_changed) !=
                                       rb.windows.focus.last_changed)
      {
        window_title =
          SK_WideCharToUTF8 (rb.windows.focus.title);
      }
    }

    std::string& appname = bSteam ?
       SK::SteamAPI::AppName ()   :
                           bEpic  ?
            SK::EOS::AppName ()   : window_title;

    if (! appname.empty ())
      title += "      -      ";

    if (config.platform.show_playtime)
    {
      snprintf ( szTitle, 511, "%hs%hs     (%01u:%02u:%02u)###SK_MAIN_CPL",
                   title.c_str (), appname.c_str (),
                     hours, mins, secs );
    }

    else
    {
      snprintf ( szTitle, 511, "%hs%s###SK_MAIN_CPL",
                   title.c_str (), appname.c_str () );
    }
  }

  else
  {
    if (config.platform.show_playtime)
    {
      title += "      -      ";

      snprintf ( szTitle, 511, "%hs(%01u:%02u:%02u)###SK_MAIN_CPL",
                   title.c_str (),
                     hours, mins, secs );
    }

    else
      snprintf (szTitle, 511, "%hs###SK_MAIN_CPL", title.c_str ());
  }

  return szTitle;
}

void
SK_ImGui_AdjustCursor (void)
{
  static SK_AutoHandle hAdjustEvent (
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
    (LPVOID)hAdjustEvent.m_h
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
   std::string  display_name; // (i.e. \\DISPLAY0... legacy GDI name)
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

    HMONITOR      hMonCurrent =
      MonitorFromWindow (game_window.hWnd, MONITOR_DEFAULTTONULL);

    MONITORINFOEX minfo       = { };

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
        SK_WideCharToUTF8 (minfo.szDevice);

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

  std::string display_list;
              display_list.reserve (2048);

  for ( auto& display : rb.displays )
  {
    if (display.attached)
    {
      outputs.push_back ((int)
        ((intptr_t)&display -
         (intptr_t)&rb.displays [0]) / sizeof (SK_RenderBackend_V2::output_s)
      );
    }
  }

  bool found = false;

  for ( auto output : outputs )
  {
    if (! found)
    {
      if (rb.monitor == rb.displays [output].monitor)
      {
        found = true;
      }
    }

    display_list +=
      SK_WideCharToUTF8 (
        rb.displays [output].name
      ).c_str ();

    display_list += '\0';
  }

  // Give it another go, the monitor was no-show...
  if (! found) if (! outputs.empty ()) dirty = true;

  display_list += '\0';

  ImGui::TreePush ("");

  int active_display = rb.active_display;

  // If list is empty, don't show the menu, stupid :)
  if (display_list [0] != '\0' && ImGui::Combo ("Active Monitor", &active_display, display_list.data ()))
  {
    config.display.monitor_handle  = rb.displays [active_display].monitor;

    if (config.display.save_monitor_prefs)
    {
        config.display.monitor_idx      = rb.displays [active_display].idx;
        config.display.monitor_path_ccd = rb.displays [active_display].path_name;
    }

    config.display.monitor_default = MONITOR_DEFAULTTONEAREST;

    rb.next_monitor =
      rb.displays [active_display].monitor;

    if (config.display.save_monitor_prefs)
      SK_SaveConfig ();

    if (rb.monitor != rb.next_monitor)
    {
      SK_Window_RepositionIfNeeded ();

      dirty = true;
    }
  }

  if (ImGui::Combo (   "Resolution",  &resolution.idx,
                                       resolution.string.c_str () ))
  {
    DEVMODEW
      dm_orig        = {               };
      dm_orig.dmSize = sizeof (DEVMODEW);

    if (EnumDisplaySettingsW (SK_UTF8ToWideChar (display_name).c_str (), ENUM_CURRENT_SETTINGS, &dm_orig))
    {
      if ( DISP_CHANGE_SUCCESSFUL ==
             SK_ChangeDisplaySettingsEx (SK_UTF8ToWideChar (
                 display_name).c_str (), &resolution.modes [resolution.idx],
                                      0, CDS_TEST, nullptr )
         )
      {
        if ( DISP_CHANGE_SUCCESSFUL ==
               SK_ChangeDisplaySettingsEx (SK_UTF8ToWideChar (
                   display_name).c_str (), &resolution.modes [resolution.idx],
                                        0, 0x0/*CDS_UPDATEREGISTRY*/, nullptr)
           )
        {
          if (config.display.resolution.save)
          {
            config.display.resolution.override.x = resolution.modes [resolution.idx].dmPelsWidth;
            config.display.resolution.override.y = resolution.modes [resolution.idx].dmPelsHeight;
          }

          config.display.resolution.applied = true;

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

    if (EnumDisplaySettingsW (SK_UTF8ToWideChar (display_name).c_str (), ENUM_CURRENT_SETTINGS, &dm_orig))
    {
      if ( DISP_CHANGE_SUCCESSFUL ==
             SK_ChangeDisplaySettingsEx (SK_UTF8ToWideChar (
                 display_name).c_str (), &refresh.modes [refresh.idx],
                                      0, CDS_TEST, nullptr )
         )
      {
        if ( DISP_CHANGE_SUCCESSFUL ==
               SK_ChangeDisplaySettingsEx (SK_UTF8ToWideChar (
                   display_name).c_str (), &refresh.modes [refresh.idx],
                                        0, 0/*CDS_UPDATEREGISTRY*/, nullptr)
           )
        {
          SK_ImGui_ConfirmDisplaySettings (&dirty, display_name, dm_orig);

          if (config.display.resolution.save)
          {
            config.display.refresh_rate =
              static_cast <float> (refresh.modes [refresh.idx].dmDisplayFrequency);
          }

          config.display.resolution.applied = true;
        }
      }
    }
  }

  if (ImGui::IsItemHovered () && (_maxAvailableRefresh < _maxRefresh))
      ImGui::SetTooltip ("Higher Refresh Rates are Available at a Different Resolution");

  if ( SK_API_IsDXGIBased (rb.api) || SK_API_IsGDIBased (rb.api) ||
      (SK_API_IsDirect3D9 (rb.api) && rb.fullscreen_exclusive) )
  {
    static constexpr int VSYNC_NoOverride      = 0;
    static constexpr int VSYNC_ForceOn         = 1;
    static constexpr int VSYNC_ForceOn_Half    = 2;
    static constexpr int VSYNC_ForceOn_Third   = 3;
    static constexpr int VSYNC_ForceOn_Quarter = 4;
    static constexpr int VSYNC_ForceOff        = 5;

    int idx = 0;

    if (      config.render.framerate.present_interval == SK_NoPreference)
        idx = VSYNC_NoOverride;
    else if ( config.render.framerate.present_interval ==  0)
        idx = VSYNC_ForceOff;
    else
    {
      switch (config.render.framerate.present_interval)
      {
        case 4:  idx = VSYNC_ForceOn_Quarter; break;
        case 3:  idx = VSYNC_ForceOn_Third;   break;
        case 2:  idx = VSYNC_ForceOn_Half;    break;
        case 1:
        default: idx = VSYNC_ForceOn;         break;
      }
    }

    static const char *szVSYNC = "  No Override\0  Forced ON\0"
                               "  Forced 1/2 (No VRR)\0"
                               "  Forced 1/3 (No VRR)\0"
                               "  Forced 1/4 (No VRR)\0"
                               "  Forced OFF\0\0";

    static bool changed = false;

    if (ImGui::Combo ("VSYNC", &idx, szVSYNC))
    {
      switch (idx)
      {
        case VSYNC_ForceOn:
          config.render.framerate.present_interval =  1;
          break;
        case VSYNC_ForceOn_Half:
          config.render.framerate.present_interval =  2;
          break;
        case VSYNC_ForceOn_Third:
          config.render.framerate.present_interval =  3;
          break;
        case VSYNC_ForceOn_Quarter:
          config.render.framerate.present_interval =  4;
          break;
        case VSYNC_ForceOff:
          config.render.framerate.present_interval =  0;
          config.render.dxgi.allow_tearing         =  true;
          break;
        case VSYNC_NoOverride:
        default:
          config.render.framerate.present_interval = SK_NoPreference;
          break;
      }

      static bool bWarnOnce = false;
             bool bNeedUnclamp = config.render.framerate.present_interval    > 1 &&
                                 config.render.framerate.sync_interval_clamp != SK_NoPreference;

      if ( bNeedUnclamp && std::exchange (bWarnOnce, true) == false )
      {
        SK_ImGui_Warning (
          L"Fractional VSYNC Rates Will Prevent VRR From Working\r\n\r\n\t>>"
          L"SyncIntervalClamp Has Been Disabled (required to use 1/n Refresh VSYNC)"
        );
      }

      if (bNeedUnclamp)
      {
        config.render.framerate.sync_interval_clamp = SK_NoPreference;
      }

      // Device reset is needed to change VSYNC mode in D3D9...
      if (SK_API_IsDirect3D9 (rb.api))
      {
        if (rb.fullscreen_exclusive)
        {
          extern void SK_D3D9_TriggerReset (bool);
                      SK_D3D9_TriggerReset (false);
        }

        else // User should not even see this menu in windowed...
          changed = true;
      }

      if (SK_API_IsGDIBased (rb.api))
      {
        if (config.render.framerate.present_interval != SK_NoPreference)
          SK_GL_SwapInterval (config.render.framerate.present_interval);
      }
    }

    if (ImGui::IsItemHovered () && idx != VSYNC_NoOverride)
    {
      ImGui::SetTooltip ( "NOTE: Some games perform additional limiting based"
                          " on their VSYNC setting; consider turning in-game"
                          " VSYNC -OFF-" );
    }

    if (SK_API_IsGDIBased (rb.api) && config.render.framerate.present_interval == SK_NoPreference)
    {
      ImGui::SameLine ();
      ImGui::Text     ("-%s-", SK_GL_GetSwapInterval () > 0 ?
                                                       "On" :
                                                      "Off");
    }

    if (SK_API_IsDirect3D9 (rb.api) && changed)
    {
      ImGui::BulletText ("Game Restart Required");
    }
  }

  if (SK_WASAPI_EndPointMgr->getNumRenderEndpoints (DEVICE_STATE_ACTIVE) > 0)
  {
    auto &display =
      rb.displays [rb.active_display];

    int         selection = 0;
    std::string output_list = "  No Preference";
    
    output_list += '\0';
    output_list += "  System Default";
    output_list += '\0';

    for (UINT i = 0 ; i < SK_WASAPI_EndPointMgr->getNumRenderEndpoints () ; ++i)
    {
      auto& end_point =
        SK_WASAPI_EndPointMgr->getRenderEndpoint (i);

      if (end_point.state_ != DEVICE_STATE_ACTIVE)
        continue;

      output_list += "  ";
      output_list +=
        end_point.name_.c_str ();

      output_list += '\0';

      if (StrStrIW (end_point.id_.c_str (), display.audio.paired_device))
      {
        selection = 2 + i;
      }
    }

    if (selection == 0)
    {
      if (_wcsicmp (display.audio.paired_device, L"No Preference"))
      {
        if (! _wcsicmp (display.audio.paired_device, L"System Default"))
        {
          selection = 1;
        }
      }
    }

    output_list += '\0';

    if (ImGui::Combo ("Audio Device", &selection, output_list.c_str ()))
    {
      if (selection > 1)
      {
        wcsncpy (display.audio.paired_device, SK_WASAPI_EndPointMgr->getRenderEndpoint (selection - 2).id_.c_str (), 127);
      }

      else
      {
        if (selection == 1) wcsncpy (display.audio.paired_device, L"System Default", 127);
        else                wcsncpy (display.audio.paired_device, L"No Preference",  127);

        SK_WASAPI_EndPointMgr->setPersistedDefaultAudioEndpoint (GetCurrentProcessId (), eRender, L"");
      }

      dll_ini->get_section              (L"Display.Audio").
        add_key_value (SK_FormatStringW (L"RenderDevice.%ws", display.path_name),
                                                              display.audio.paired_device);

      dll_ini->write ();

      rb.routeAudioForDisplay (&display);
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip ("While the game is running on this monitor, it will use this audio device.");
    }
  }

  if (sk::NVAPI::nv_hardware && NvAPI_Disp_GetDitherControl != nullptr)
  {
    static NV_DITHER_STATE state = NV_DITHER_STATE_DEFAULT;
    static NV_DITHER_BITS  bits  = rb.displays [rb.active_display].bpc ==  6 ? NV_DITHER_BITS_6  :
                                   rb.displays [rb.active_display].bpc ==  8 ? NV_DITHER_BITS_8  :
                                   rb.displays [rb.active_display].bpc == 10 ? NV_DITHER_BITS_10 :
                                                                               NV_DITHER_BITS_8;
    static NV_DITHER_MODE   mode = NV_DITHER_MODE_TEMPORAL;

    SK_RunOnce (dirty = true);

    if (dirty)
    {
      NV_GPU_DITHER_CONTROL_V1
        dither_ctl         = {                        };
        dither_ctl.version = NV_GPU_DITHER_CONTROL_VER1;

      if ( NVAPI_OK ==
           NvAPI_Disp_GetDitherControl (rb.displays [rb.active_display].nvapi.display_id, &dither_ctl) )
      {
        state = dither_ctl.state;
        bits  = dither_ctl.bits;
        mode  = dither_ctl.mode;
      }
    }

    auto state_orig = state;

    if ( ImGui::Combo ( "Dithering", (int *)&state,
                                      "  Default\0  Enable\0  Disable\0\0" ) )
    {
      if ( NVAPI_OK !=
           NvAPI_Disp_SetDitherControl ( rb.displays [rb.active_display].nvapi.gpu_handle,
                                         rb.displays [rb.active_display].nvapi.display_id, state, bits, mode ) )
      {
        state = state_orig;
      }
    }

    if (state == NV_DITHER_STATE_ENABLED)
    {
      ImGui::TreePush ("");

      auto bits_orig = bits;

      if ( ImGui::Combo ( "Bits", (int *)&bits,
                                  " 6\0 8\0 10\0\0" ) )
      {
        if ( NVAPI_OK !=
             NvAPI_Disp_SetDitherControl ( rb.displays [rb.active_display].nvapi.gpu_handle,
                                           rb.displays [rb.active_display].nvapi.display_id, state, bits, mode ) )
        {
          bits = bits_orig;
        }
      }

      auto mode_orig = mode;

      if ( ImGui::Combo ( "Mode", (int *)&mode,
                                  " Spatial Dynamic\0 Spatial Static\0 Spatial Dynamic 2x2\0"
                                  " Spatial Static 2x2\0 Temporal\0\0" ) )
      {
        if ( NVAPI_OK !=
             NvAPI_Disp_SetDitherControl ( rb.displays [rb.active_display].nvapi.gpu_handle,
                                           rb.displays [rb.active_display].nvapi.display_id, state, bits, mode ) )
        {
          mode = mode_orig;
        }
      }

      ImGui::TreePop ();
    }

#if 0
    NV_GPU_DITHER_CONTROL_V1
      dither_ctl         = {                        };
      dither_ctl.version = NV_GPU_DITHER_CONTROL_VER1;

    if ( NVAPI_OK ==
         NvAPI_Disp_GetDitherControl ( rb.displays [rb.active_display].nvapi.display_id,
                                      &dither_ctl ) )
    {
      ImGui::Text ( "Dithering: %s, Bits: %s, Mode: %s",
                      dither_ctl.state == NV_DITHER_STATE_DEFAULT            ?  "Default" :
                      dither_ctl.state == NV_DITHER_STATE_ENABLED            ?  "Enabled" :
                      dither_ctl.state == NV_DITHER_STATE_DISABLED           ? "Disabled" : "Unknown",
                      dither_ctl.bits  == NV_DITHER_BITS_6                   ?        "6" :
                      dither_ctl.bits  == NV_DITHER_BITS_8                   ?        "8" :
                      dither_ctl.bits  == NV_DITHER_BITS_10                  ?       "10" : "Unknown",
                      dither_ctl.mode  == NV_DITHER_MODE_SPATIAL_DYNAMIC     ? "Spatial Dynamic"     :
                      dither_ctl.mode  == NV_DITHER_MODE_SPATIAL_STATIC      ? "Spatial Static"      :
                      dither_ctl.mode  == NV_DITHER_MODE_SPATIAL_DYNAMIC_2x2 ? "Spatial Dynamic 2x2" :
                      dither_ctl.mode  == NV_DITHER_MODE_SPATIAL_STATIC_2x2  ? "Spatial Static 2x2"  :
                      dither_ctl.mode  == NV_DITHER_MODE_TEMPORAL            ? "Temporal"            : "Unknown" );
    }
#endif
  }

  if (! rb.displays [rb.active_display].primary)
  {
    // XXX: 9/9/22, Need to fix re-enumeration of displays after doing this so the drop-down menu text is correct
    //
    if (ImGui::Button ("Make Primary Display"))
    {
      struct monitor_s {
        DEVMODE dm             = { };
        wchar_t wszDevice [32] = { };
      };

      DEVMODE
          dm        = {              };
          dm.dmSize = sizeof (DEVMODE);

      POINTL lOrigin = { };

      std::vector <monitor_s> monitors;

      for ( const auto& display : rb.displays )
      {
        if (EnumDisplaySettings (display.gdi_name, ENUM_REGISTRY_SETTINGS, &dm))
        {
          monitor_s  monitor;
          wcsncpy_s (monitor.wszDevice, 32, display.gdi_name, _TRUNCATE);
                     monitor.dm = dm;

          monitors.emplace_back (monitor);
        }

        if (! _wcsicmp (display.gdi_name, rb.displays [rb.active_display].gdi_name))
        {
          lOrigin = dm.dmPosition;
        }

        dm.dmSize = sizeof (DEVMODE);
      }

      for ( const auto& monitor : monitors )
      {
        DEVMODE
          dmNew          = monitor.dm;
          dmNew.dmFields = DM_POSITION;

        // Primary monitor is always at (0,0), so we need to shift all monitors to the new origin
        dmNew.dmPosition.x -= lOrigin.x;
        dmNew.dmPosition.y -= lOrigin.y;

        const bool primary =
          ( dmNew.dmPosition.x == 0 && dmNew.dmPosition.y == 0 );

        if ( DISP_CHANGE_SUCCESSFUL !=
             SK_ChangeDisplaySettingsEx ( monitor.wszDevice, &dmNew, NULL,
                                         (primary ? CDS_SET_PRIMARY : 0x0) |
                                                    CDS_UPDATEREGISTRY     |
                                                    CDS_NORESET, NULL ) )
        {
          SK_ReleaseAssert (! L"ChangeDisplaySettingsEx == DISP_CHANGE_SUCCESSFUL");
        }
      }

      SK_ChangeDisplaySettingsEx (nullptr, nullptr, 0, 0, 0);
    }

    if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Setting a monitor as primary gives it control of DWM composition rate and lowers latency when using mismatched refresh rates");
  }

  ImVec2 vHDRPos;

  if ( rb.displays [rb.active_display].hdr.supported ||
                    rb.isHDRCapable () )
  {
    bool hdr_enable =
      rb.displays [rb.active_display].hdr.enabled;

    if (ImGui::Checkbox ("Enable HDR", &hdr_enable))
    {
      DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE
        setHdrState                     = { };
        setHdrState.header.type         = DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE;
        setHdrState.header.size         =     sizeof (DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE);
        setHdrState.header.adapterId    = rb.displays [rb.active_display].vidpn.targetInfo.adapterId;
        setHdrState.header.id           = rb.displays [rb.active_display].vidpn.targetInfo.id;

        setHdrState.enableAdvancedColor = hdr_enable;

      if ( ERROR_SUCCESS == DisplayConfigSetDeviceInfo ( (DISPLAYCONFIG_DEVICE_INFO_HEADER *)&setHdrState ) )
      {
        rb.displays [rb.active_display].hdr.enabled = hdr_enable;
      }
    }

    if (ImGui::IsItemHovered ())
    {
      if (rb.displays [rb.active_display].hdr.enabled)
      {
        ImGui::SetTooltip ( "SDR Whitepoint: %4.1f cd/m²",
                              rb.displays [rb.active_display].hdr.white_level );
      }
    }

    ImGui::SameLine ();
    ImGui::TextUnformatted (" ");
    ImGui::SameLine ();

    vHDRPos.y =
      ImGui::GetCursorPosY ();

    switch (rb.displays [rb.active_display].hdr.encoding)
    {
      case DISPLAYCONFIG_COLOR_ENCODING_RGB:
        ImGui::Text ("RGB (%lu-bpc)",         rb.displays [rb.active_display].bpc);
        break;
      case DISPLAYCONFIG_COLOR_ENCODING_YCBCR444:
        ImGui::Text ("YCbCr 4:4:4 (%lu-bpc)", rb.displays [rb.active_display].bpc);
        break;
      case DISPLAYCONFIG_COLOR_ENCODING_YCBCR422:
        ImGui::Text ("YCbCr 4:2:2 (%lu-bpc)", rb.displays [rb.active_display].bpc);
        break;
      case DISPLAYCONFIG_COLOR_ENCODING_YCBCR420:
        ImGui::Text ("YCbCr 4:2:0 (%lu-bpc)", rb.displays [rb.active_display].bpc);
        break;
      case DISPLAYCONFIG_COLOR_ENCODING_INTENSITY:
        ImGui::Text ("ICtCp (%lu-bpc)",       rb.displays [rb.active_display].bpc);
        break;
    }

    ImGui::SameLine        ();
    vHDRPos.x =
      ImGui::GetCursorPosX ();
    ImGui::Spacing         ();
  }

  static bool bDPIAware  =
      IsProcessDPIAware (),
              bImmutableDPI = false;

  void SK_Display_ForceDPIAwarenessUsingAppCompat (bool set);
  void SK_Display_SetMonitorDPIAwareness          (bool bOnlyIfWin10);
  void SK_Display_ClearDPIAwareness               (bool bOnlyIfWin10);

  bool bDPIAwareBefore =
       bDPIAware;

  ImGui::BeginGroup ();

  if (ImGui::Checkbox ("Ignore DPI Scaling",      &bDPIAware))
  {
    config.dpi.disable_scaling                  = (bDPIAware);
    config.dpi.per_monitor.aware                = (bDPIAware);
    config.dpi.per_monitor.aware_on_all_threads = (bDPIAware);
       SK_Display_ForceDPIAwarenessUsingAppCompat (bDPIAware);

    if (bDPIAware) SK_Display_SetMonitorDPIAwareness (false);
    else           SK_Display_ClearDPIAwareness      (false);

    PostMessage (game_window.hWnd, WM_DISPLAYCHANGE, 0, 0);
    //PostMessage (game_window.hWnd, WM_SIZE, 0, 0);

    bImmutableDPI =
      ( (IsProcessDPIAware () == TRUE) == bDPIAwareBefore );

    SK_SaveConfig ();
  }

  if (ImGui::IsItemHovered ())
  {
    ImGui::BeginTooltip    ();
    ImGui::TextUnformatted ( "Fixes missing in-game resolution options and "
                             "over-sized game windows");
    ImGui::TextColored     ( ImVec4 (1.f, 1.f, 0.f, 1.f),
                             "  " ICON_FA_EXCLAMATION_TRIANGLE );
    ImGui::SameLine        (                                   );
    ImGui::Text            ( " Restoring DPI scaling requires a game restart" );
    ImGui::EndTooltip      ();
  }

  if (bImmutableDPI)
  {
    ImGui::SameLine       ();
    ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
    ImGui::BulletText     ("Game Restart Required");
    ImGui::PopStyleColor  ();
  }

  if (display_list [0] != '\0')
  {
    if (ImGui::Checkbox ("Prefer Selected Monitor", &config.display.save_monitor_prefs))
    {
      if (config.display.save_monitor_prefs)
      {
        config.display.monitor_idx =
          rb.displays [rb.active_display].idx;
        config.display.monitor_path_ccd =
          rb.displays [rb.active_display].path_name;
      }

      else
      {
        // Revert to no preference
        config.display.monitor_idx      = 0;
        config.display.monitor_path_ccd = L"";
      }

      SK_SaveConfig ();
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Changes to 'Active Monitor' using this menu (not keybinds) will be remembered");
  }

  if (ImGui::Checkbox ("Remember Display Mode", &config.display.resolution.save))
  {
    if (config.display.resolution.save)
    {
      config.display.monitor_idx =
        rb.displays [rb.active_display].idx;
      config.display.monitor_path_ccd =
        rb.displays [rb.active_display].path_name;
    }

    else
    {
      // Revert to no preference
      config.display.monitor_idx      = 0;
      config.display.monitor_path_ccd = L"";

      config.display.resolution.override.x = 0;
      config.display.resolution.override.y = 0;
    }

    SK_SaveConfig ();
  }

  if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Changes to Resolution or Refresh on 'Active Monitor'"
                         " will apply to future launches of this game");

  ImGui::EndGroup    ();
  ImGui::SameLine    ();
  ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
  ImGui::SameLine    ();
  ImGui::BeginGroup  ();

  auto &display =
    rb.displays [rb.active_display];

  ImGui::Separator   ();
  ImGui::BeginGroup  ();
  ImGui::Text        ("MPO Planes: ");
  ImGui::Text        ("HW Scheduling: ");
  ImGui::Text        ("HW Flip Queue: ");
  ImGui::EndGroup    ();
  ImGui::SameLine    ();
  ImGui::BeginGroup  ();

  if (display.mpo_planes <= 1)
  {
    ImGui::TextColored ( ImVec4 (1.f, 1.f, 0.f, 1.f), "Unsupported " ICON_FA_EXCLAMATION_TRIANGLE );
  }

  else
  {
    ImGui::TextColored ( ImVec4 (0.f, 1.f, 0.f, 1.f), "%d", display.mpo_planes );
  }

  auto _PrintEnabled      = [](UINT enabled)
  {
    if (enabled != 0)
      ImGui::TextColored ( ImVec4 (0.f, 1.f, 0.f, 1.f), "On " );
    else
      ImGui::TextColored ( ImVec4 (1.f, 1.f, 0.f, 1.f), "Off " );
  };
  auto _PrintSupportState = [](UINT state)
  {
    switch (state)
    {
      default:
      case DXGK_FEATURE_SUPPORT_ALWAYS_OFF:   ImGui::Text ("(Always Off)");   break;
      case DXGK_FEATURE_SUPPORT_EXPERIMENTAL: ImGui::Text ("(Experimental)"); break;
      case DXGK_FEATURE_SUPPORT_STABLE:       ImGui::Text ("(Stable)");       break;
      case DXGK_FEATURE_SUPPORT_ALWAYS_ON:    ImGui::Text ("(Always On)");    break;
    };
  };

  auto _PrintWDDMCapability = [&](UINT Enabled, UINT SupportState)
  {
    _PrintEnabled      (Enabled);      ImGui::SameLine ();
    _PrintSupportState (SupportState);
  };

  _PrintWDDMCapability (display.wddm_caps._2_9.HwSchEnabled,
                        display.wddm_caps._2_9.HwSchSupportState);

  _PrintWDDMCapability (display.wddm_caps._3_0.HwFlipQueueEnabled,
                        display.wddm_caps._3_0.HwFlipQueueSupportState);

  ImGui::EndGroup    ();
  ImGui::EndGroup    ();

  if (ImGui::Checkbox ("Aspect Ratio Stretch", &config.display.aspect_ratio_stretch))
  {
    config.window.center =
      config.display.aspect_ratio_stretch;

    if (config.display.aspect_ratio_stretch)
    {
      config.window.borderless        = true;
      config.window.fullscreen        = false;
      config.window.background_render = true;             // Required for this to work reliably
      config.window.always_on_top     = SmartAlwaysOnTop; // Needed to fix taskbar visibility
    }

    else
    {
      config.window.res.override.x = 0;
      config.window.res.override.y = 0;
    }

    if ( config.display.aspect_ratio_stretch &&
         config.window.res.override.isZero () )
    {
      config.window.borderless = true;
      config.window.center     = true;
    }

    SK_SaveConfig ();
  }

  if (ImGui::IsItemHovered ())
  {
    ImGui::BeginTooltip ();
    ImGui::Text         ("Fills the game's monitor with a background wherever"
                         " the game's window does not cover");
    ImGui::Separator    ();
    ImGui::BulletText   ("For best results, use the game's internal Windowed"
                         " mode option (not Borderless / Borderless Fullscreen)");

    if (display.mpo_planes <= 1)
    {
      ImGui::Separator   ();
      ImGui::TextColored ( ImVec4 (1.f, 1.f, 0.f, 1.f),
                         "  " ICON_FA_EXCLAMATION_TRIANGLE );
      ImGui::SameLine    (                                 );
      ImGui::Text        ( " Multiplane Overlays are Unsupported,"
                           " performance and latency will suffer if this is used." );
    }

    ImGui::EndTooltip   ();
  }

  if (config.display.aspect_ratio_stretch)
  {
    ImGui::SameLine ();

    float fVirtualAspect =
      (float)config.window.res.override.x /
      (float)config.window.res.override.y;

    float fNativeAspect =
      (float)(display.rect.right  - display.rect.left) /
      (float)(display.rect.bottom - display.rect.top);

    struct {
      float fAspect;
      int   idx;
    } aspect_ratios [] = {
      { 1.25f, 0 }, // 5:4
      { 1.33f, 1 }, // 4:3
      { 1.5f,  2 }, // 3:2
      { 1.6f,  3 }, // 16:10
      { 1.7f,  4 }, // 16:9
      { 2.3f,  5 }  // 21:9
    };

    int iVirtualAspect = 6; // Custom
    int iNativeAspect  = 4;

    for ( auto& aspect : aspect_ratios )
    {
      if ( fVirtualAspect < aspect.fAspect + 0.04f &&
           fVirtualAspect > aspect.fAspect - 0.04f )
      {
        iVirtualAspect = aspect.idx;
      }

      if ( fNativeAspect < aspect.fAspect + 0.04f &&
           fNativeAspect > aspect.fAspect - 0.04f )
      {
        iNativeAspect = aspect.idx;
      }
    }

    if (ImGui::Combo ("", &iVirtualAspect, " 5:4\0 4:3\0 3:2\0 16:10\0 16:9\0 21:9\0 Custom\0\0"))
    {
      switch (iVirtualAspect)
      {
        case 0:
          if (iNativeAspect >= iVirtualAspect)
          {
            config.window.res.override.y = display.rect.bottom - display.rect.top;
            config.window.res.override.x = (int)(5.0f * (config.window.res.override.y / 4.0f));
          }
          else
          {
            config.window.res.override.x = display.rect.right - display.rect.left;
            config.window.res.override.y = (int)(4.0f * (config.window.res.override.x / 5.0f));
          }
          break;
        case 1:
          if (iNativeAspect >= iVirtualAspect)
          {
            config.window.res.override.y = display.rect.bottom - display.rect.top;
            config.window.res.override.x = (int)(4.0f * (config.window.res.override.y / 3.0f));
          }
          else
          {
            config.window.res.override.x = display.rect.right - display.rect.left;
            config.window.res.override.y = (int)(3.0f * (config.window.res.override.x / 4.0f));
          }
          break;
        case 2:
          if (iNativeAspect >= iVirtualAspect)
          {
            config.window.res.override.y = display.rect.bottom - display.rect.top;
            config.window.res.override.x = (int)(3.0f * (config.window.res.override.y / 2.0f));
          }
          else
          {
            config.window.res.override.x = display.rect.right - display.rect.left;
            config.window.res.override.y = (int)(2.0f * (config.window.res.override.x / 3.0f));
          }
          break;
        case 3:
          if (iNativeAspect >= iVirtualAspect)
          {
            config.window.res.override.y = display.rect.bottom - display.rect.top;
            config.window.res.override.x = (int)(16.0f * (config.window.res.override.y / 10.0f));
          }
          else
          {
            config.window.res.override.x = display.rect.right - display.rect.left;
            config.window.res.override.y = (int)(10.0f * (config.window.res.override.x / 16.0f));
          }
          break;
        case 4:
          if (iNativeAspect >= iVirtualAspect)
          {
            config.window.res.override.y = display.rect.bottom - display.rect.top;
            config.window.res.override.x = (int)(16.0f * (config.window.res.override.y / 9.0f));
          }
          else
          {
            config.window.res.override.x = display.rect.right - display.rect.left;
            config.window.res.override.y = (int)(9.0f * (config.window.res.override.x / 16.0f));
          }
          break;
        case 5:
          if (iNativeAspect >= iVirtualAspect)
          {
            config.window.res.override.y = display.rect.bottom - display.rect.top;
            config.window.res.override.x = (int)(21.0f * (config.window.res.override.y / 9.0f));
          }
          else
          {
            config.window.res.override.x = display.rect.right  - display.rect.left;
            config.window.res.override.y = (int)(9.0f * (config.window.res.override.x / 21.0f));
          }
          break;
        case 6:
          config.window.res.override.x = 0;
          config.window.res.override.y = 0;
          break;
      }

      if (! config.window.res.override.isZero ())
      {
        // Trigger the game to resize the SwapChain so we can change its aspect ratio
        //
        PostMessage ( game_window.hWnd,                 WM_SIZE,        SIZE_RESTORED,
          MAKELPARAM (config.window.res.override.x, config.window.res.override.y)
                    );
        PostMessage ( game_window.hWnd,                 WM_DISPLAYCHANGE, 32,
          MAKELPARAM (config.window.res.override.x, config.window.res.override.y)
                    );

        extern void SK_Win32_BringBackgroundWindowToTop (void);
                    SK_Win32_BringBackgroundWindowToTop ();
      }

      SK_ImGui_AdjustCursor ();
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Some engines may require a game restart to adjust to new aspect ratio.");
  }

  static std::set <SK_ConfigSerializedKeybind *>
    keybinds = {
      &config.monitors.monitor_primary_keybind,
      &config.monitors.monitor_next_keybind,
      &config.monitors.monitor_prev_keybind,
      &config.monitors.monitor_toggle_hdr
    };

  if (ImGui::BeginMenu ("Display Management Keybinds###MonitorMenu"))
  {
    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {
      ImGui::Text ( "%s:  ",
                      keybind->bind_name );
    }
    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {
      Keybinding  (   keybind, keybind->param );
    }
    ImGui::EndGroup   ();
    ImGui::EndMenu    ();
  }

  // Display
  if ( rb.displays [rb.active_display].hdr.supported ||
       rb.isHDRCapable () )
  {
    ImGui::SetCursorPos (vHDRPos);

    bool hdr =
      SK_ImGui_Widgets->hdr_control->isVisible ();

    ImGui::TextUnformatted ("\t");
    ImGui::SameLine        (    );

    if (ImGui::Button (ICON_FA_SUN " HDR Setup"))
    {
      hdr = (! hdr);

      SK_ImGui_Widgets->hdr_control->
        setVisible (hdr).
        setActive  (hdr);
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip ("Right-click HDR Calibration to assign hotkeys");
    }
  }

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

        // Trigger the game to resize the SwapChain so we can change its format and colorspace
        //
        PostMessage ( game_window.hWnd,                 WM_SIZE,        SIZE_RESTORED,
          MAKELPARAM (game_window.actual.client.right -
                      game_window.actual.client.left,   game_window.actual.client.bottom -
                                                        game_window.actual.client.top )
                    );
        PostMessage ( game_window.hWnd,                 WM_DISPLAYCHANGE, 32,
          MAKELPARAM (game_window.actual.client.right -
                      game_window.actual.client.left,   game_window.actual.client.bottom -
                                                        game_window.actual.client.top )
                    );
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
            config.render.dxgi.scaling_mode = SK_NoPreference;
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

      auto _ComputeRefresh =
        [&](const std::pair <UINT, UINT>& fractional) ->
        long double
        {
          return
            sk::narrow_cast <long double> (fractional.first)/
            sk::narrow_cast <long double> (fractional.second);
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
        current_item = SK_NoPreference;

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
                  sk::narrow_cast <UINT> (
                    std::ceil (
                        sk::narrow_cast <long double> (dxgi_mode.RefreshRate.Numerator) /
                        sk::narrow_cast <long double> (dxgi_mode.RefreshRate.Denominator)
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
                    current_item = idx;
                    break;
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
                 -1 != sk::narrow_cast <INT> (config.render.framerate.refresh_rate) )
            {
              int lvl2_idx = 1;

              for ( auto& nomnom : refresh_rates )
              {
                long double ldRefresh =
                  _ComputeRefresh (nomnom.second);

                if ( ldRefresh < sk::narrow_cast <long double> (config.render.framerate.refresh_rate + 0.333f) &&
                     ldRefresh > sk::narrow_cast <long double> (config.render.framerate.refresh_rate - 0.333f) )
                {
                  current_item = lvl2_idx;
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
            sk::narrow_cast <float> (
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
                          sk::narrow_cast <int> (
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

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip ("Your game should be set to Windowed mode in its graphics settings if you intend to override this mode.");
    }

    ImGui::Separator ();

    SK_Display_ResolutionSelectUI ();
  }
}


extern float __target_fps;
extern float __target_fps_bg;

extern void SK_ImGui_DrawGraph_FramePacing (void);
extern void SK_ImGui_DrawGraph_Latency     (bool predraw);
extern void SK_ImGui_DrawConfig_Latency    (void);

extern void SK_Framerate_EnergyControlPanel (void);

void
SK_NV_LatencyControlPanel (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! (sk::NVAPI::nv_hardware && SK_API_IsDXGIBased (rb.api)))
    return;

  ImGui::Separator ();

  bool bLatencyManagement =
    ImGui::TreeNodeEx ("NVIDIA Latency Management", ImGuiTreeNodeFlags_DefaultOpen);

  static bool     native_disabled =
    config.nvidia.reflex.disable_native;

  if ((config.nvidia.reflex.native && config.nvidia.reflex.override) || native_disabled)
  {
    ImGui::SameLine ();

           bool changed      = false;
    static bool need_restart = false;

    changed |=
      ImGui::Checkbox ("Disable This Game's Native Reflex", &config.nvidia.reflex.disable_native);

    if (changed)
    {
      need_restart = true;

      SK_SaveConfig ();
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::TextUnformatted ("Some games have -really- broken implementations of Reflex.");
      ImGui::Separator       ();
      ImGui::BulletText      ("It may be better to disable native Reflex and use SK's implementation in some cases.");
      ImGui::BulletText      ("If using SK's Latency Analysis to quantify CPU/GPU-bound state and dial-in game settings\r\n\t"
                              " for best performance, it is important to temporarily disable native Reflex.");
      ImGui::EndTooltip      ();
    }

    if (need_restart)
    {
      ImGui::SameLine       ();
      ImGui::SeparatorEx    (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine       ();
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }
  }

  if (! bLatencyManagement)
    return;

  if ((! rb.displays [rb.active_display].primary) && config.nvidia.reflex.low_latency
                                                  && config.nvidia.reflex.enable)
  {
    ImGui::SameLine    (                                 );
    ImGui::BeginGroup  (                                 );
    ImGui::TextColored ( ImVec4 (1.f, 1.f, 0.f, 1.f),
                       "  " ICON_FA_EXCLAMATION_TRIANGLE );
    ImGui::SameLine    (                                 );
    ImGui::Text        ( " Reflex Latency modes do not"
                         " work correctly on Secondary"
                         " monitors."                    );
    ImGui::EndGroup    (                                 );

    if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Use the Display menu to assign Primary monitors");
  }

  SK_ImGui_DrawConfig_Latency ();
  SK_ImGui_DrawGraph_Latency  (false);

  ImGui::TreePop ();
}

void
SK_NV_GSYNCControlPanel ()
{
  if (sk::NVAPI::nv_hardware)
  {
    static auto& rb =
      SK_GetCurrentRenderBackend ();

    SK_RunOnce (rb.gsync_state.disabled.for_app = !SK_NvAPI_GetVRREnablement ());

    if (ImGui::BeginPopup ("G-Sync Control Panel"))
    {
      ImGui::Text ("NVIDIA G-Sync Configuration");

      ImGui::TreePush ("");

      static bool bEnableFastSync =
             SK_NvAPI_GetFastSync ();

      bool bEnableGSync =
        (! rb.gsync_state.disabled.for_app);

      if (ImGui::Checkbox ("Enable G-Sync in this Game", &bEnableGSync))
      { SK_NvAPI_SetVRREnablement                        (bEnableGSync);

        rb.gsync_state.disabled.for_app =
          (! bEnableGSync);

        ImGui::CloseCurrentPopup ();
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Requires a Game Restart");

      if (ImGui::Checkbox ("Enable FastSync in this Game", &bEnableFastSync))
      {
        SK_NvAPI_SetFastSync (bEnableFastSync);

        ImGui::CloseCurrentPopup ();
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Requires a Game Restart");

      ImGui::TreePop    ();
      ImGui::EndPopup   ();
    }
  }
}

SK_API
bool
SK_ImGui_ControlPanel (void)
{
  if (! imgui_staged_frames)
    return false;

  auto& io =
    ImGui::GetIO ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_TLS *pTLS =
        SK_TLS_Bottom ();

  if (ImGui::GetFont () == nullptr)
  {
    SK_LOG0 ( ( L" Fatal Error:  No Font Loaded!" ),
                L"   ImGui   " );
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

      if (config.apis.NvAPI.enable && sk::NVAPI::nv_hardware)
      {
        //ImGui::TextWrapped ("%hs", SK_NvAPI_GetGPUInfoStr ().c_str ());
        ImGui::MenuItem    ("Display G-Sync Status",     "", &config.apis.NvAPI.gsync_status);
      }

      ImGui::MenuItem  ("Display Playtime in Title",     "", &config.platform.show_playtime);
      ImGui::MenuItem  ("Display Mac-style Menu at Top", "", &config.imgui.use_mac_style_menu);
      ImGui::Separator ();

      DisplayModeMenu (windowed);
    };


  auto SpecialK_Menu =
    [&](void)
    {
      static std::set <SK_ConfigSerializedKeybind *>
        monitor_keybinds = {
          &config.monitors.monitor_primary_keybind,
          &config.monitors.monitor_next_keybind,
          &config.monitors.monitor_prev_keybind,
          &config.monitors.monitor_toggle_hdr
      };

      // Keybinds made using a menu option must process their popups here
      for ( auto& binding : monitor_keybinds )
      {
        if (binding->assigning)
        {
          if (! ImGui::IsPopupOpen (binding->bind_name))
            ImGui::OpenPopup (      binding->bind_name);

          std::wstring     original_binding =
                                    binding->human_readable;

          SK_ImGui_KeybindDialog (  binding           );

          if (             original_binding !=
                                    binding->human_readable)
            binding->param->store ( binding->human_readable);

          if (! ImGui::IsPopupOpen (binding->bind_name))
                                    binding->assigning = false;
        }
      }


      if (ImGui::BeginMenu (ICON_FA_SAVE "  File"))
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
          ( static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9)  ) ||
          ( static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11) ) ||
          ( static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D12) );

        if (supports_texture_mods)
        {
          static std::error_code ec = { };
          static bool
          bHasTextureMods =
            std::filesystem::exists (
              std::filesystem::path (SK_D3D11_res_root.get ())
            / std::filesystem::path (LR"(inject\textures)"), ec
          );

          if (bHasTextureMods)
          {
            SK::ControlPanel::D3D11::TextureMenu ();
          }

          else
          {
            if (ImGui::MenuItem ("Initialize Texture Mods", "", nullptr))
            {
              ec              = { };
              bHasTextureMods =
                std::filesystem::create_directories (
                  std::filesystem::path (SK_D3D11_res_root.get ())
                / std::filesystem::path (LR"(inject\textures)"), ec
              );
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

        static bool bLAA =
          SK_PE32_IsLargeAddressAware ();

        if (! bLAA)
        {
          ImGui::Separator ();

          if (ImGui::Selectable ("Apply Large Address Aware Patch to Game"))
          {
            if (SK_PE32_MakeLargeAddressAwareCopy ())
              bLAA = true;
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Allows 32-bit games to use more than 2 GiB of RAM");
            ImGui::Separator    ();
            ImGui::BulletText   ("The current game is -not- Large Address Aware");
            ImGui::BulletText   ("Fully patching the game requires re-launching it");
            ImGui::EndTooltip   ();
          }
        }

        static bool wrappable = true;

        if (SK_IsInjected () && wrappable)
        {
          ImGui::Separator ();

          if (ImGui::MenuItem ("Install Wrapper DLL"))
          {
            if (SK_Inject_SwitchToRenderWrapper ())
              wrappable = false;
          }
        }

        else if (wrappable)
        {
          ImGui::Separator ();

          if (ImGui::MenuItem ("Uninstall Wrapper DLL"))
          {
            wrappable =
              SK_Inject_SwitchToGlobalInjector ();
          }
        }

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

        if ((! SK_IsInjected ()) || SK_Inject_IsHookActive ())
        {
          extern bool
              SK_CanRestartGame (void);
          if (SK_CanRestartGame () && ImGui::MenuItem ("Restart Game"))
          {
            SK_ImGui_WantRestart = true;
            SK_ImGui_WantExit    = true;
          }
        }

        // Self-explanatory, I think
        //if (ImGui::IsItemHovered ())
        //{
        //  ImGui::SetTooltip ("Exit and Automatically Restart (useful for settings that require a restart)");
        //}

        if (ImGui::MenuItem ("Exit Game", "Alt+F4"))
        {
          SK_ImGui_WantRestart = false;
          SK_ImGui_WantExit    = true;
        }

        ImGui::EndMenu  ();
      }

      if (ImGui::BeginMenu (ICON_FA_DESKTOP "  Display"))
      {
        DisplayMenu    ();
        ImGui::EndMenu ();
      }
      
      static bool changed_hdr = false;

      auto _SetOverlayLuminance =
      [&](bool bMatchSDRWhite, float& cfg_val, float fNits)
      {
        cfg_val =
          bMatchSDRWhite ?
            rb.displays [rb.active_display].hdr.white_level * 1.0_Nits
                         :                            fNits * 1.0_Nits;
      };

      auto SDRTooltip =
      [&](void)
      {
        if (ImGui::IsItemHovered ( ))
        {
          ImGui::BeginTooltip    ( );
          ImGui::TextUnformatted ("Controls Luminance of Overlays while in HDR Mode");
          ImGui::Separator       ( );

          ImGui::Spacing         ( );
          ImGui::Spacing         ( );
          ImGui::Spacing         ( );

          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.85f, .85f, .85f, 1.f));

          ImGui::TextColored     (ImVec4 (.4f, .8f, 1.f, 1.f), " " ICON_FA_MOUSE);
          ImGui::SameLine        ();
          ImGui::Text            ((const char *)u8"Right-click to match Windows SDR white level (%5.1f cd/m²)",
                                  rb.displays [rb.active_display].hdr.white_level);

          ImGui::BulletText      ("Luminance levels above 50%% of slider range are "
                                  "not recommended in HDR10");

          ImGui::Spacing         ( );
          ImGui::Spacing         ( );

          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
          ImGui::TextUnformatted ("Special K can apply Color Correction for Third-party SDR Overlays");

          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.75f, .75f, .75f, 1.f));

          ImGui::Spacing         ( );
          ImGui::Spacing         ( );

          ImGui::TreePush        ("");
          ImGui::BulletText      ("Third-party overlay sliders will only appear if SK's own "
                                  "HDR is active and the current game is D3D11");
          ImGui::BulletText      ("HDR Color Correction for third-party overlays -does- "
                                  "apply in D3D12, but it uses the values set in D3D11");

          ImGui::Spacing         ( );
          ImGui::Spacing         ( );

          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.933f, .933f, .933f, 1.f));

          ImGui::TextUnformatted ("For overlay Color Correction in native HDR games, use SK's HDR10 or scRGB Mode + "
                                  "HDR10 Native or scRGB Native Preset");
          ImGui::TreePop         ( );
          ImGui::PopStyleColor   (4);
          ImGui::EndTooltip      ( );
        }
      };

      auto HDRLuminanceSlider =
      [&]( const char * const szLabel,
                 float&        fNits,
                 float         fMaxLuminance = SK_GetCurrentRenderBackend ().display_gamut.maxLocalY )
   -> bool
      {
        float nits =
          fNits / 1.0_Nits;

        const bool changed =
          ImGui::SliderFloat ( szLabel, &nits,
                                 80.0f, fMaxLuminance,
                                   (const char *)u8"%.1f cd/m²" );
        
        const bool right_clicked =
          ImGui::IsItemClicked (ImGuiMouseButton_Right);

        if (changed || right_clicked)
        {
          _SetOverlayLuminance ( right_clicked,
                                   fNits,
                                     nits );

          SK_SaveConfig ();
        }

        SDRTooltip ();

        return
          changed || right_clicked;
      };

      auto HDRMenu =
      [&](void)
      {
        HDRLuminanceSlider (
          "Special K Luminance###IMGUI_LUMINANCE", rb.ui_luminance
        );

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
          HDRLuminanceSlider (
            "Steam Overlay Luminance###STEAM_LUMINANCE", config.platform.overlay_hdr_luminance,
                                                                  rb.display_gamut.maxAverageY
          );
        }

        if (config.rtss.present)
        {
          HDRLuminanceSlider (
            "RTSS Overlay Luminance###RTSS_LUMINANCE", config.rtss.overlay_luminance,
                                                        rb.display_gamut.maxAverageY
          );
        }

        if (config.epic.present)
        {
          HDRLuminanceSlider (
            "Epic Overlay Luminance###EPIC_LUMINANCE", config.platform.overlay_hdr_luminance,
                                                                rb.display_gamut.maxAverageY
          );
        }

        if (config.discord.present)
        {
          HDRLuminanceSlider (
            "Discord Overlay Luminance###DISCORD_LUMINANCE", config.discord.overlay_luminance,
                                                                 rb.display_gamut.maxAverageY
          );
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
          HDRLuminanceSlider (
            "uPlay Overlay Luminance###UPLAY_LUMINANCE", config.uplay.overlay_luminance,
                                                           rb.display_gamut.maxAverageY
          );
        }

        ImGui::Separator ();

        bool hdr_changed =
            ImGui::Checkbox ( "Keep Full-Range HDR Screenshots",
                                &config.screenshots.png_compress );

        if (config.screenshots.png_compress && SK_GetBitness () == SK_Bitness::SixtyFourBit)
        {
          static bool bFetchingAVIF = false;

          int selection =
            ( config.screenshots.use_avif ?
                                        1 : 0 );

          if (
            ImGui::Combo ( "HDR File Format", &selection,
                           "JPEG-XR (.jxr)\0"
                           "AVIF\t\t(.avif)\0\0" )
             )
          {
            if (selection == 1)
            {
              if (! bFetchingAVIF)
              {
                static std::filesystem::path avif_dll =
                       std::filesystem::path (SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty)) /
                    SK_RunLHIfBitness ( 64, LR"(Image Codecs\libavif\libavif_x64.dll)",
                                            LR"(Image Codecs\libavif\libavif_x86.dll)" );

                std::error_code                          ec;
                if (! std::filesystem::exists (avif_dll, ec))
                {
                  bFetchingAVIF = true;

                  SK_Network_EnqueueDownload (
                       sk_download_request_s (
                         avif_dll.wstring (),
                           R"(https://sk-data.special-k.info/addon/ImageCodecs/)"
                                     R"(libavif/libavif_x64.dll)",
                  []( const std::vector <uint8_t>&&,
                      const std::wstring_view )
                   -> bool
                      {
                        bFetchingAVIF               = false;
                        config.screenshots.use_avif = true;
                  
                        return false;
                      }
                    ), true
                  );
                }
                else
                {
                  config.screenshots.use_avif = true;
                }
              }
            }
            else
            {
              config.screenshots.use_avif = false;
            }
          }

          if (bFetchingAVIF)
          {
            ImGui::TextColored (ImVec4 (.1f,.9f,.1f,1.f), "Downloading AVIF Plug-In...");
          }

          if (config.screenshots.use_avif)
          {
            ImGui::TreePush ("");
            
            int subsampling = ( config.screenshots.avif.yuv_subsampling == 400 ? 3 :
                                config.screenshots.avif.yuv_subsampling == 420 ? 2 :
                                config.screenshots.avif.yuv_subsampling == 422 ? 1 :
                                                                               0 );

            if (ImGui::Combo ("YUV Subsampling", &subsampling, " 4:4:4\0 4:2:2\0 4:2:0\0 4:0:0 (Black & White)\0\0"))
            {
              switch (subsampling)
              {
                default:
                case 0:
                  config.screenshots.avif.yuv_subsampling = 444;
                  break;
                case 1:
                  config.screenshots.avif.yuv_subsampling = 422;
                  break;
                case 2:
                  config.screenshots.avif.yuv_subsampling = 420;
                  break;
                case 3:
                  config.screenshots.avif.yuv_subsampling = 400;
                  break;
              }

              SK_SaveConfig ();
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip ();
              ImGui::BulletText   ("Windows natively supports 10-bit and 12-bit AVIF images at 4:2:0, or 8-bit at up to 4:4:4");
              ImGui::BulletText   ("Higher quality chroma subsampled AVIF images will only render correctly in Chrome.");
              ImGui::EndTooltip   ();
            }

            int scrgb_bits = ( config.screenshots.avif.scrgb_bit_depth == 8  ? 0 :
                               config.screenshots.avif.scrgb_bit_depth == 10 ? 1 :
                                                                               2 );

            if (__SK_HDR_16BitSwap)
            {
              if (ImGui::Combo ("scRGB->PQ Bit Depth", &scrgb_bits, " 8-bit\0 10-bit\0 12-bit\0\0"))
              {
                config.screenshots.avif.scrgb_bit_depth =
                  ( scrgb_bits == 0 ? 8  :
                    scrgb_bits == 1 ? 10 :
                                      12 );

                SK_SaveConfig ();
              }
            }

            const char* szCompressionQualityFormat =
              ( config.screenshots.avif.compression_quality == 100 ? "100 (Lossless)"
                                                                   : "%d" );

            bool changed = false;

            changed |=
              ImGui::SliderInt ("Compression Quality", &config.screenshots.avif.compression_quality, 80, 100, szCompressionQualityFormat);

            if (ImGui::IsItemHovered ())
              ImGui::SetTooltip ("You can manually enter values < 80 using ctrl+click, but the results will be terrible.");
            
            changed |=
              ImGui::SliderInt ("Compression Speed",   &config.screenshots.avif.compression_speed,   0, 10);

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip    ();
              ImGui::TextUnformatted ("How long to dedicate to compressing the image for smallest file size");
              ImGui::BulletText      ("Values < 7 are VERY slow, potentially taking minutes.");
              ImGui::BulletText      ("The compression is done on a background thread, unlikely to consume excessive CPU.");
              ImGui::BulletText      ("If you set the speed too low, HDR screenshots might not finish by the time you exit.");
              ImGui::EndTooltip      ();
            }

            if (changed)
            {
              SK_SaveConfig ();
            }

            ImGui::TreePop ();
          }
        }

        if ( rb.screenshot_mgr->getRepoStats ().files > 0 )
        {
          const SK_ScreenshotManager::screenshot_repository_s& repo =
            rb.screenshot_mgr->getRepoStats (hdr_changed);

          ImGui::BeginGroup (  );
          ImGui::TreePush   ("");
          ImGui::Text ( "%u files using %ws",
                          repo.files,
                            SK_File_SizeToString (repo.liSize.QuadPart, Auto, pTLS).data ()
                      );

          ImGui::SameLine ();

          if (ImGui::Button ("Browse"))
          {
            SK_ShellExecuteW ( nullptr,
              L"explore",
                rb.screenshot_mgr->getBasePath (),
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

        // Put any bullet points on the same line as the button
        bool same_line = false;

        if ( (rb.displays [rb.active_display].hdr.supported ||
                           rb.isHDRCapable ())              &&
             (rb.displays [rb.active_display].hdr.enabled) )
        {
          if (ImGui::Button (ICON_FA_SUN " HDR Setup"))
          {
            hdr = (! hdr);

            SK_ImGui_Widgets->hdr_control->
              setVisible (hdr).
              setActive  (hdr);
          }

          same_line = true;

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Right-click HDR Calibration to assign hotkeys");
          }
        }

        static bool
            hdr_toggled = false;
        if (hdr_toggled)
        {
          if (    same_line)
            ImGui::SameLine ();

          ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
          ImGui::BulletText     ("Game Restart May Be Required");
          ImGui::PopStyleColor  ();
        }

        //
        // TODO: Put a button in this part of the control panel to control the
        //         behavior of temporary Desktop HDR

        if ((  rb.displays [rb.active_display].hdr.supported ||
                            rb.isHDRCapable () )             &&
            (! rb.displays [rb.active_display].hdr.enabled ) )
        {
          bool bEnabled = false;

          if (ImGui::Checkbox ("Enable HDR###EnableHDRUsingCPL", &bEnabled))
          {
            extern void
            SK_Display_EnableHDR (SK_RenderBackend_V2::output_s *pDisplay);
            SK_Display_EnableHDR (&rb.displays [rb.active_display]);

            hdr_toggled = true;
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip (
              "Select scRGB from the HDR Calibration "
              "tool after turning HDR on"
              "\r\n\r\n"
              "\t\t( The Display menu has additional settings )"
              "\r\n\r\n"
              "\t\t\t >> Most Games Require a Restart <<"
            );
          }

          ImGui::SameLine       ();

          ImGui::PushStyleColor (ImGuiCol_Text, ImColor (1.0f, .7f, .3f).Value);
          ImGui::BulletText     ("HDR Is Not Currently Enabled");
          ImGui::PopStyleColor  ();
        }

        else if (rb.displays [rb.active_display].hdr.enabled)
        {
          if ( ImGui::Checkbox (
                 "Enable / Disable HDR when this game Starts or Exits",
                   &config.render.dxgi.temporary_dwm_hdr )
             )
          {
            SK_SaveConfig ();
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text (
              "If SK's scRGB HDR mode is active, HDR is automatically enabled"
              " -- regardless what you set here" );

            ImGui::BulletText (
              "Automatic Enable / Disable will still turn HDR off when games exit"
            );
            ImGui::EndTooltip ();
          }
        }

        bool bDisable =
          ( config.apis.NvAPI.disable_hdr &&
            config.render.dxgi.hide_hdr_support );

        static bool bOriginal = bDisable;

        if (ImGui::Checkbox ("Disable Game's Native HDR", &bDisable))
        {
          config.apis.NvAPI.disable_hdr       = bDisable;
          config.render.dxgi.hide_hdr_support = bDisable;

          SK_SaveConfig ();
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ("Use if game does not have a user setting to turn HDR off...");
        }

        if (bOriginal != bDisable)
        {
          ImGui::BulletText ("Game Restart Required");
        }
      };

      if ( rb.displays [rb.active_display].hdr.supported ||
                        rb.isHDRCapable () )
           //&&
           //rb.isHDRActive() )//< This menu was originally shown only when HDR is
                               //    already on, but that turns out to be unintuitive
      {
        if (ImGui::BeginMenu (ICON_FA_SUN "  HDR###CalibrateFromCPL"))
        {
          HDRMenu ();

          ImGui::EndMenu ();
        }
      }

#pragma region Legacy AutoUpdate: Scheduled For Removal (!!)
#ifdef _HAS_AUTO_UPDATE_SUPPORT
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
#pragma endregion Legacy AutoUpdate: Scheduled For Removal (!!)


      if (ImGui::BeginMenu (ICON_FA_QUESTION "  Help"))
      {
        bool selected = false;

        //ImGui::MenuItem ("Special K Documentation (Work in Progress)", "Ctrl + Shift + Nul", &selected, false);

        ImGui::TreePush ("");
        {
#if 0
          if (ImGui::MenuItem (R"("Kaldaien's Mod")", "Discourse Forums", &selected, true))
            SK_SteamOverlay_GoToURL ("https://discourse.differentk.fyi/", true);
#else
          if (ImGui::MenuItem (R"("Kaldaien's Mod")", "Discord Server", &selected, true))
            SK_SteamOverlay_GoToURL ("https://discord.gg/SpecialK", true);
#endif
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
          SK_SteamOverlay_GoToURL ("https://discord.gg/specialk",
              true
          );
        }

        if (ImGui::MenuItem ("About this Software...", "Licensed Software", &selected))
          eula.show = true;

        ImGui::Separator ();

        if (ImGui::BeginMenu ("ImGui Debug"))
        {
          ImGui::SeparatorText ("ImGui Debug");

          ImGui::MenuItem  ("Demo",      "", &imgui_demo);

          ImGui::MenuItem  ("Debug Log", "", &imgui_debug);

          ImGui::MenuItem  ("Metrics",   "", &imgui_metrics);

          ImGui::MenuItem  ("About",     "", &imgui_about);

          ImGui::SeparatorText ("ImPlot Debug");

          ImGui::MenuItem  (" Demo",     "",&implot_demo);

          ImGui::EndMenu   ( );
        }

        ImGui::Separator ();

        if ( (0 < config.steam.appid && config.steam.appid <= INT32_MAX) ||
              *rb.windows.focus.title != L'\0')
        {
          if (ImGui::MenuItem ( "Check PCGamingWiki for this Game", "Third-Party Site", &selected ))
          {
            SK_SteamOverlay_GoToURL (
                ((0 < config.steam.appid && config.steam.appid <= INT32_MAX)
                  ? SK_FormatString (
                      "https://pcgamingwiki.com/api/appid.php?appid=%lu",
                                            SK::SteamAPI::AppID ())
                  : SK_FormatString (
                      "https://www.pcgamingwiki.com/w/index.php?search=%hs",
                                            SK_WideCharToUTF8 (rb.windows.focus.title).c_str())
              ).c_str (), true
            );
          }

          ImGui::Separator ();
        }

        if (sk::NVAPI::nv_hardware)
        {
          extern INT  SK_NvAPI_GetAnselEnablement (DLL_ROLE role);
          extern BOOL SK_NvAPI_EnableAnsel        (DLL_ROLE role);
          extern BOOL SK_NvAPI_DisableAnsel       (DLL_ROLE role);

          static INT enablement =
            SK_NvAPI_GetAnselEnablement (SK_GetDLLRole ());

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
         ImGui::MenuItem ( "Current Game",
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
        ImGui::EndMenu ();
      }

      auto effective_power_mode =
        SK_Power_GetCurrentEffectiveMode ();

      if (SK_IsGameWindowActive ())
      {
        if (effective_power_mode != EffectivePowerModeNone)
        {
          ImGui::SameLine    ();
          ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
          ImGui::SameLine    ();
          ImGui::BeginGroup  ();
          ImGui::Text        ("\tEffective Power Mode:\t %hs",
                              SK_Power_GetEffectiveModeStr (effective_power_mode));

          if (effective_power_mode != EffectivePowerModeGameMode)
          {
            ImGui::SameLine ();
            ImGui::Spacing  ();
            ImGui::SameLine ();
            ImGui::TextColored (ImVec4 (1.f, 1.f, 0.f, 1.f), ICON_FA_EXCLAMATION_TRIANGLE);
            ImGui::EndGroup ();

            if (ImGui::IsItemHovered ())
            {
              ImGui::SetTooltip (
                "For best performance:\r\n\t"
                "Set 'Remember this is a game' in Windows Game Bar settings,"
                " ensure that Windows Game Mode is On, and restart the game."
              );
            }
          }

          else
          {
            ImGui::SameLine ();
            ImGui::Spacing  ();
            ImGui::SameLine ();
            ImGui::TextColored (ImVec4 (0.f, 1.f, 0.f, 1.f), ICON_FA_TACHOMETER_ALT);
            ImGui::EndGroup ();
          }
        }
      }
    };

  if (config.imgui.use_mac_style_menu)
  { if (     ImGui::BeginMainMenuBar())
    {               SpecialK_Menu   ();
               ImGui::EndMainMenuBar();
  } }

  static HMODULE
         hModTBFix =
    SK_GetModuleHandle
          (L"tbfix.dll");

  static HMODULE
         hModTZFix =
    SK_GetModuleHandle
          (L"tzfix.dll");

  static float last_width  = -1.0f;
  static float last_height = -1.0f;

  bool recenter =
    ( last_width  != io.DisplaySize.x ||
      last_height != io.DisplaySize.y );

  if (recenter)
  {
    SK_ImGui_LastWindowCenter.x = -1.0f;
    SK_ImGui_LastWindowCenter.y = -1.0f;

    SK_ImGui_SetNextWindowPosCenter (
       ImGuiCond_Always
    );

    last_width  = io.DisplaySize.x;
    last_height = io.DisplaySize.y;
  }

         const char* szTitle = SK_ImGui_ControlPanelTitle ();
  static       int     title_len
             = int (
  static_cast <float>
   (ImGui::CalcTextSize (szTitle, nullptr, true).x)
          * 1.075f / io.FontGlobalScale );

  static bool first_frame = true;
  bool        open        = true;


  // Initialize the dialog using the user's scale preference
  if (first_frame)
  {
    io.    ConfigFlags |=
      ImGuiConfigFlags_NavEnableSetMousePos;

    // Range-restrict for sanity!
    config.imgui.scale = SK_ImGui::SanitizeFontGlobalScale (config.imgui.scale);

    io.FontGlobalScale =
    config.imgui.scale;

    first_frame = false;
  }

#ifdef _ProperSpacing
  const  float font_size           =
    ( ImGui::GetFont  ()->FontSize * io.FontGlobalScale );
  const  float font_size_multiline =
    ( font_size + ImGui::GetStyle ().ItemSpacing.y +
                  ImGui::GetStyle ().ItemInnerSpacing.y );
#endif

  ImGuiStyle& style =
    ImGui::GetStyle ();

  style.WindowTitleAlign =
    ImVec2 (0.5f, 0.5f);

  ImGuiStyle orig = style;

  static float       fMinX      = 0.0f;
  static float       fLastScale = 0.0f;
  if (std::exchange (fLastScale, io.FontGlobalScale)
                              != io.FontGlobalScale)
  {
    fMinX =
      title_len * io.FontGlobalScale;
  }

  ImGui::SetNextWindowSizeConstraints (
    ImVec2 (fMinX + io.FontGlobalScale * style.ItemSpacing.x * 1.5f,
                      200.0f), ImVec2 ( 0.9f * io.DisplaySize.x,
                                        0.9f * io.DisplaySize.y ) );

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
      ImGuiWindowFlags_AlwaysAutoResize       | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_AlwaysUseWindowPadding |
      ( config.imgui.use_mac_style_menu ? 0x00 :
                                          ImGuiWindowFlags_MenuBar )
  );

  style = orig;

  if (open)
  {
    if (! config.imgui.use_mac_style_menu)
    {
      if (ImGui::BeginMenuBar())
      {      SpecialK_Menu   ();
            ImGui::EndMenuBar();
      }
    }

          char szAPIName [32] = {             };
    snprintf ( szAPIName, 32, "%ws",  rb.name );

    // Translation layers (D3D8->11 / D3D8->12 / DDraw->11 / DDraw->12 / D3D11On12)
    auto api_mask = static_cast <int> (rb.api);

    bool translated_d3d9 =
      config.apis.d3d9.translated;

    if (0x0 != (api_mask &  static_cast <int> (SK_RenderAPI::D3D12)) &&
               (api_mask != static_cast <int> (SK_RenderAPI::D3D12)  || translated_d3d9))
    {
      if (translated_d3d9)
      {
        strncpy  (szAPIName, (const char *) u8"D3D9→12", 32);
      }
      else if (api_mask == static_cast <int> (SK_RenderAPI::DDrawOn12)  ||
               api_mask == static_cast <int> (SK_RenderAPI::D3D8On12)   ||
               api_mask == static_cast <int> (SK_RenderAPI::GlideOn12))
      {
        lstrcatA (szAPIName, (const char *) u8"→12");
      }
      else
      {
        lstrcatA (szAPIName, "On12");
      }
    }

    else if (0x0 != (api_mask &  static_cast <int> (SK_RenderAPI::D3D11)) &&
                    (api_mask != static_cast <int> (SK_RenderAPI::D3D11)  || translated_d3d9))
    {
      if (! translated_d3d9)lstrcatA (szAPIName, (const char *)   u8"→11");
      else                  strncpy  (szAPIName, (const char *)u8"D3D9→11", 32);
    }

    lstrcatA ( szAPIName,
                 SK_GetBitness () == 32 ? "           [ 32-bit ]" :
                                          "           [ 64-bit ]" );

    ImGui::MenuItem ("Active Render API        ",
                                  szAPIName,
                                    nullptr, false);

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

    // Flip model may be fudging with the actual format of the framebuffer
    extern bool bOriginallysRGB;

    char szResolution [128] = { };

    bool sRGB     = (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_SRGB) || bOriginallysRGB;
    bool hdr_out  =  rb.isHDRCapable ()  &&
                     rb.isHDRActive  ();
    bool override = false;

    RECT                              client = { };
    GetClientRect (game_window.hWnd, &client);

    if ( static_cast <int> (io.DisplayFramebufferScale.x) != client.right  - client.left ||
         static_cast <int> (io.DisplayFramebufferScale.y) != client.bottom - client.top  ||
         sRGB                                                                            ||
         hdr_out )
    {
      snprintf (
        szResolution, 63, "   %ux%u",
        static_cast <UINT> (io.DisplayFramebufferScale.x),
        static_cast <UINT> (io.DisplayFramebufferScale.y) );

      strcat ( szResolution,
               ( sRGB    ? "    (sRGB)"
                         : "" )      );
      strcat ( szResolution,
               ( hdr_out ? "     (HDR)"
                         : "" )      );

      if (ImGui::MenuItem (" Framebuffer Resolution",
                                       szResolution))
      {
        config.window.res.override.x =
              sk::narrow_cast <UINT> (
        io.DisplayFramebufferScale.x );

        config.window.res.override.y =
              sk::narrow_cast <UINT> (
        io.DisplayFramebufferScale.y );

        override =
          true;
      }
    }

    SK_DPI_Update ();

    if (sRGB) strcat (szResolution,     "    (sRGB)");
            snprintf (szResolution, 63, "   %ix%i",
              (int)((float)(client.right    - client.left) * g_fDPIScale),
                (int)((float)(client.bottom - client.top ) * g_fDPIScale)
                     );

    if (_fDPIScale > 100.1f)
    {
      static char szScale [32] = {                     };
      snprintf (  szScale, 31, "    %.0f%%", _fDPIScale);
      strcat ( szResolution,
                  szScale );
    }

    float refresh_rate =
      static_cast <float> (
        rb.getActiveRefreshRate ()
      );

    if (  refresh_rate != 0.0f  )
    {
      strcat ( szResolution,
           SK_FormatString (
             " @ %6.02f Hz",
              refresh_rate ).c_str ()
             );
    }


    if (windowed)
    {
      if (ImGui::MenuItem (" Window Resolution     ", szResolution))
      {
        config.window.res.override.x = (int)((float)(client.right  - client.left) * g_fDPIScale);
        config.window.res.override.y = (int)((float)(client.bottom - client.top)  * g_fDPIScale);

        override = true;
      }

    //if (_fDPIScale > 100.1f && ImGui::IsItemHovered ())
    //  ImGui::SetTooltip ("DPI Awareness:  %u DPI", _uiDPI);
    }

    else
    {
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
        SK_ComQIPtr <IDXGISwapChain>
            pSwapDXGI (rb.swapchain);
        if (pSwapDXGI != nullptr)
        {
          SK_ImGui_SummarizeDXGISwapchain (
                               pSwapDXGI  );
        }
      }
    }

    int device_x =
      (int)((float)rb.windows.device.getDevCaps ().res.x * g_fDPIScale);
    int device_y =
      (int)((float)rb.windows.device.getDevCaps ().res.y * g_fDPIScale);

    if ( (client.right - client.left) != device_x || (client.bottom - client.top) != device_y ||
         io.DisplayFramebufferScale.x != device_x || io.DisplayFramebufferScale.y != device_y )
    {
      snprintf ( szResolution, 63, "   %ix%i",
                                     device_x,
                                       device_y );

      if (ImGui::MenuItem (" Device Resolution    ", szResolution))
      {
        config.window.res.override.x = device_x;
        config.window.res.override.y = device_y;

        override =
          true;
      }
    }

    if (! config.window.res.override.isZero ())
    {
      snprintf ( szResolution, 63, "   %ux%u",
                                     config.window.res.override.x,
                                       config.window.res.override.y );

      bool selected = true;

      if (ImGui::MenuItem (" Override Resolution   ",
                                    szResolution,
                                       &selected)
         )
      {
        config.window.res.override.x = 0;
        config.window.res.override.y = 0;

        override =
          true;
      }
    }


    if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
    {
      char szGSyncStatus [128] = { };

      if (rb.gsync_state.capable)
      {
        strcat (szGSyncStatus, "    Supported + ");
        if (rb.gsync_state.active)
        {
          strcat (szGSyncStatus, "Active");
        }
        else if (! rb.gsync_state.maybe_active)
          strcat (szGSyncStatus, "Inactive");
        else
          strcat (szGSyncStatus, "Unknown");
      }

      else
      {
        if (rb.gsync_state.disabled.for_app)
          strcat (szGSyncStatus, "   Disabled in this Game");
        else
          strcat (szGSyncStatus, "   Unsupported");
      }

      ImGui::MenuItem (" G-Sync Status   ", szGSyncStatus, nullptr, true);

      if (ImGui::IsItemClicked () || SK_ImGui_IsItemRightClicked ())
      {
        ImGui::OpenPopup         ("G-Sync Control Panel");
        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiCond_Always);
      }

      if (rb.gsync_state.maybe_active)
      {
        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip (
            "The NVIDIA driver API does not report this status in OpenGL, D3D12 or Vulkan."
          );
        }
      }
    }

    if (sk::NVAPI::nv_hardware)
      SK_NV_GSYNCControlPanel ();


    if (override)
      SK_ImGui_AdjustCursor ();


    ImGui::Columns   ( 1 );
    ImGui::Separator (   );

    static bool has_own_scale = (hModTBFix != nullptr);

    if ((! has_own_scale) && ImGui::CollapsingHeader ("UI Render Settings"))
    {
      ImGui::TreePush    ("");

      if ( ImGui::SliderFloat ( "###IMGUI_SCALE", &config.imgui.scale,
                                  1.0f, 3.0f, "UI Scaling Factor %.2f" ) )
      {
        // ImGui does not perform strict parameter validation,
        //   and values out of range for this can be catastrophic.
        config.imgui.scale = SK_ImGui::SanitizeFontGlobalScale (config.imgui.scale);
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


    static auto game_id =
      SK_GetCurrentGameID ();

    // TODO: Move this into SK_PlugIn_ControlPanelWidget
    switch (game_id)
    {
      case SK_GAME_ID::GalGun_Double_Peace:
      {
        extern bool SK_GalGun_PlugInCfg (void);
                    SK_GalGun_PlugInCfg ();
      } break;

      case SK_GAME_ID::FarCry6:
      {
        extern bool SK_FarCry6_PlugInCfg (void);
                    SK_FarCry6_PlugInCfg ();
      } break;
    };


  for ( auto plugin_cfg : plugin_mgr->config_fns )
  {          plugin_cfg ();                      }


  static bool has_own_limiter    = hModTBFix;
  static bool has_own_limiter_ex = hModTZFix;

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
            __target_fps =
              static_cast <float> (SK_GetCurrentRenderBackend ().windows.device.getDevCaps ().res.refresh);
          }

          config.render.framerate.target_fps = __target_fps;
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

          //if ( ( rb.api == SK_RenderAPI::D3D11 ||
          //       rb.api == SK_RenderAPI::D3D12 ) && (! (config.render.framerate.flip_discard &&
          //                                              config.render.framerate.swapchain_wait > 0)))
          //{
          //  ImGui::Separator       ();
          //  ImGui::TextUnformatted ("Did you know ... to get the most out of SK's Framerate Limiter:");
          //  ImGui::BulletText      ("Enable Flip Model + Waitable SwapChain in D3D11/12 / SwapChain Settings");
          //}

            ImGui::EndTooltip ();
          }

          if (advanced)
          {
            if (ImGui::Checkbox ("Background", &bg_limit))
            {
              if (bg_limit) __target_fps_bg = __target_fps;
              else          __target_fps_bg =         0.0f;

              config.render.framerate.target_fps_bg = __target_fps_bg;
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
          static auto cp =
            SK_GetCommandProcessor ();

          float target_mag = fabs (target);

          ImGui::PushStyleColor ( ImGuiCol_Text,
            ( active ? ImColor (1.00f, 1.00f, 1.00f).Value
                     : ImColor (0.73f, 0.73f, 0.73f).Value ) );

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
            ImGui::BeginGroup      ( );
            ImGui::TextColored     ( ImColor::HSV (0.18f, 0.88f, 0.94f),
                                       "  Ctrl Click" );
            ImGui::TextColored     ( ImColor::HSV (0.18f, 0.88f, 0.94f),
                                       "Right Click" );
            ImGui::EndGroup        ( );
            ImGui::SameLine        ( );
            ImGui::BeginGroup      ( );
            ImGui::TextUnformatted ( " Enter an Exact Framerate" );
            ImGui::TextUnformatted ( " Select a Refresh Factor" );
            ImGui::EndGroup        ( );
            ImGui::EndTooltip      ( );
          }

          ImGui::PushID (command);

          if (SK_ImGui_IsItemRightClicked ())
          {
            ImGui::OpenPopup         ("FactoredFramerateMenu");
            ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiCond_Always);
          }

          if (ImGui::BeginPopup      ("FactoredFramerateMenu"))
          {
            static bool   bVRRBias = false;

            static auto lastRefresh = 0.0;
                   auto realRefresh =
                     SK_GetCurrentRenderBackend ().windows.device.getDevCaps ().res.refresh;

            static std::string        strFractList ("", 1024);
            static std::vector <double> dFractList;
            static int                  iFractSel  = 0;
            static auto                *pLastLabel = command;
            static auto                 itemWidth  =
              ImGui::CalcTextSize ("888.888888888").x;

            if ( ( std::exchange (pLastLabel,  command)
                                            != command ) ||
                 ( std::exchange (lastRefresh, realRefresh)
                                            != realRefresh ) )
            {
              int idx = 0;

              strFractList.clear ();
                dFractList.clear ();

              double  dRefresh = realRefresh;
              while ( dRefresh >= 12.0 )
              {
                double dBiasedRefresh =
                             dRefresh - (!bVRRBias ? 0.0f :
                            (dRefresh * dRefresh) / (60.0 * 60.0) + 0.1);

                strFractList +=
                    ( std::to_string (dBiasedRefresh) + '\0' );
                dFractList.push_back (dBiasedRefresh);

                if ( target_mag < dBiasedRefresh + 0.75 &&
                     target_mag > dBiasedRefresh - 0.75 )
                {
                  iFractSel = idx;
                }

                dRefresh *= 0.5;
                ++idx;
              }

              strFractList += "\0\0";
            }

            iFractSel =
              std::min (static_cast <int> (dFractList.size ()),
                                           iFractSel);


            extern int __SK_LatentSyncSkip;

            bool bLatentSync =
              config.render.framerate.present_interval == 0 &&
             (config.render.framerate.target_fps        > 0.0f ||
                                           __target_fps > 0.0f);

            if (ImGui::Checkbox ("Latent Sync", &bLatentSync))
            {
              double dRefresh =
                rb.getActiveRefreshRate ();

              if (! bLatentSync)
              {
                config.render.framerate.present_interval = 1;

                //if (rb.gsync_state.disabled)
                //{
                //  SK_NvAPI_SetVRREnablement (TRUE);
                //  rb.gsync_state.disabled = false;
                //}
              }

              else
              {
                if (! (rb.gsync_state.disabled.for_app))
                {
                  ///rb.gsync_state.disabled = true;
                  ///SK_NvAPI_SetVRREnablement (FALSE);

                  if (rb.gsync_state.capable)
                  {
                    SK_RunOnce (SK_ImGui_Warning (L"Latent Sync may not work correctly while G-Sync is active"));
                  }
                }

                config.render.framerate.present_interval = 0;
                __SK_LatentSyncSkip                      = 0;

                SK_GetCommandProcessor ()->ProcessCommandFormatted (
                  "TargetFPS %f", dRefresh
                );
              }
            }

            if (bLatentSync)
            {
              ImGui::TreePush ("");

              double dRefresh =
                rb.getActiveRefreshRate ();

              int iMode = 2; // 1:1
#if 0
              if (     fabs (static_cast <double> (__target_fps) / 4.0 - dRefresh) < 1.0)
              {
                __SK_LatentSyncSkip = 4;
                            iMode   = 0;
              }

              else if (fabs (static_cast <double> (__target_fps) / 2.0 - dRefresh) < 1.0)
              {
                __SK_LatentSyncSkip = 2;
                            iMode   = 1;
              }

              else
#endif
              {
                __SK_LatentSyncSkip = 0;

                if (fabs (static_cast <double> (__target_fps) * 2.0 - dRefresh) < 1.0)
                {
                  iMode = 3;
                }
                if (fabs (static_cast <double> (__target_fps) * 3.0 - dRefresh) < 1.0)
                {
                  iMode = 4;
                }
                if (fabs (static_cast <double> (__target_fps) * 4.0 - dRefresh) < 1.0)
                {
                  iMode = 5;
                }
              }

              if ( ImGui::Combo ( "Scan Mode", &iMode,
                                  "4x Refresh (Disabled)\0"
                                  "2x Refresh (Disabled)\0"
                                  "1:1 Refresh\0"
                                  "1/2 Refresh\0"
                                  "1/3 Refresh\0"
                                  "1/4 Refresh\0\0" ) )
              {
                float fTargetFPS =
                  static_cast <float> (dRefresh);

                __SK_LatentSyncSkip = 0;

                switch (iMode)
                {
#if 0
                  case 0:
                  case 1:
                  {
                    if (rb.api == SK_RenderAPI::D3D11 ||
                        rb.api == SK_RenderAPI::OpenGL)//SK_API_IsDXGIBased (rb.api) || SK_API_IsGDIBased (rb.api))
                    {
                      if (iMode == 0)
                      {
                        __SK_LatentSyncSkip = 4;
                             fTargetFPS *= 4.0f;
                      }

                      if (iMode == 1)
                      {
                        __SK_LatentSyncSkip = 2;
                             fTargetFPS *= 2.0f;
                      }
                    }
                  } break;
#endif

                  default:
                  case 2: fTargetFPS *= 1.0f; break;
                  case 3: fTargetFPS /= 2.0f; break;
                  case 4: fTargetFPS /= 3.0f; break;
                  case 5: fTargetFPS /= 4.0f; break;
                }

                SK_GetCommandProcessor ()->ProcessCommandFormatted (
                  "TargetFPS %f", fTargetFPS
                );
              }
              ImGui::TreePop  (  );
            }

            if (config.render.framerate.present_interval != 0)
            {
              ImGui::PushItemWidth (itemWidth);

              if ( ImGui::Combo ( "Refresh Rate Factors",
                               &iFractSel, strFractList.data () ) )
              {
                cp->ProcessCommandFormatted (
                           "%s %f", command, dFractList [iFractSel] );
              }

              ImGui::PopItemWidth ();

              if (ImGui::Checkbox    ("VRR Bias", &bVRRBias))
              {
                lastRefresh = 0.0f;
              }
              if (bVRRBias)
              {
                ImGui::SameLine ();
                ImGui::TextUnformatted ("\t(Reflex - 0.1 FPS)");
              }
            }
            //if (                                   bVRRBias &&
            //ImGui::SliderFloat ("Maximum Range",  &fVRRBias, 0.0f, 10.0f,
            //                    "-%.2f %%"))
            //{
            //
            //}
            extern void SK_ImGui_LatentSyncConfig (void);
                        SK_ImGui_LatentSyncConfig ();

            ImGui::EndPopup     ();
          }

          ImGui::PopID ();
        };

        static std::set <SK_ConfigSerializedKeybind *>
          timing_keybinds = {
            &config.render.framerate.latent_sync.tearline_move_up_keybind,
            &config.render.framerate.latent_sync.tearline_move_down_keybind,
            &config.render.framerate.latent_sync.timing_resync_keybind,
            &config.render.framerate.latent_sync.toggle_fcat_bars_keybind
          };

        // Keybinds made using a menu option must process their popups here
        for ( auto& binding : timing_keybinds )
        {
          if (binding->assigning)
          {
            if (! ImGui::IsPopupOpen (binding->bind_name))
              ImGui::OpenPopup (      binding->bind_name);

            std::wstring     original_binding =
                                      binding->human_readable;

            SK_ImGui_KeybindDialog (  binding           );

            if (             original_binding !=
                                      binding->human_readable)
              binding->param->store ( binding->human_readable);

            if (! ImGui::IsPopupOpen (binding->bind_name))
                                      binding->assigning = false;
          }
        }



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
            ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.3f);
            ImGui::BeginGroup ();
            
            bool bLowLatency =
                ( config.render.framerate.enforcement_policy == 2);

            enum class limiter_mode_e {
              Normal      = 0,
              LowLatency  = 1,
              LatentSync  = 2,
              Reflex      = 3
            };

            // Present Interval 0 = Latent Sync; Low Latency mode unsupported
            //
            int mode = static_cast <int>
             (config.render.framerate.present_interval == 0 ? limiter_mode_e::LatentSync
                                                            :
                                                bLowLatency ? limiter_mode_e::LowLatency
                                                            : limiter_mode_e::Normal);

            const bool bReflexSupported =
              rb.isReflexSupported ();

            if (bReflexSupported)
            {
              if (config.nvidia.reflex.use_limiter)
              {
                if (config.nvidia.reflex.enable)
                  mode = 3;
                else // Turn Reflex Limiter off if user disabled Reflex
                  config.nvidia.reflex.use_limiter = false;
              }
            }

            extern bool
                __SK_ForceDLSSGPacing;
            if (__SK_ForceDLSSGPacing)
            {
              if (mode != 0)
                  mode  = 0;
            }

            static const char *szModesWithoutReflex = "Normal\0"
                                "Low-Latency\t(VRR Optimized)\0"
                               "Latent Sync\t (VSYNC -Off-)\0\0";

            static const char *szNormalModesWithReflex =  "Normal\0"
                                  "Low-Latency\t  (VRR Optimized)\0"
                                   "Latent Sync\t   (VSYNC -Off-)\0"
                                "NVIDIA Reflex\t(DLSS-G Pacing)\0\0";

            static const char *szDLSSGOnly =
                                "NVIDIA Reflex\t(DLSS-G Pacing)\0\0";

            const char *szModeList =
                        szModesWithoutReflex;

            if (bReflexSupported)
            {
              if (! __SK_ForceDLSSGPacing)
                szModeList = szNormalModesWithReflex;
              else
                szModeList = szDLSSGOnly;
            }

            if (ImGui::Combo ("Mode", &mode, szModeList))
            {
              struct vsync_prefs_s {
                int present_interval = config.render.framerate.present_interval == 0 ? -1 :
                                       config.render.framerate.present_interval;
              } static original_vsync_settings;

              struct reflex_prefs_s {
                bool use_limiter = config.nvidia.reflex.use_limiter;
                bool low_latency = config.nvidia.reflex.low_latency;
                bool override    = config.nvidia.reflex.override;
                bool enable      = config.nvidia.reflex.enable;
                bool changed     = false;
              } static original_reflex_settings;

              switch ((limiter_mode_e)mode)
              {
                default:
                case limiter_mode_e::Normal:
                  config.render.framerate.enforcement_policy = 4;

                  if (config.render.framerate.present_interval == 0) // Turn VSYNC -on-
                      config.render.framerate.present_interval  = original_vsync_settings.present_interval;

                  original_vsync_settings.present_interval = config.render.framerate.present_interval;
                  break;

                case limiter_mode_e::LowLatency:
                  config.render.framerate.enforcement_policy = 2;

                  if (config.render.framerate.present_interval == 0) // Turn VSYNC -on-
                      config.render.framerate.present_interval  = original_vsync_settings.present_interval;

                  original_vsync_settings.present_interval = config.render.framerate.present_interval;

                  if (config.render.framerate.present_interval > 1)
                  {
                    SK_ImGui_Warning (L"VRR Does Not Work When Using 1/2, 1/3 or 1/4 rate VSYNC!");
                  }
                  break;

                case limiter_mode_e::LatentSync:
                  original_vsync_settings.present_interval   = config.render.framerate.present_interval;
                  config.render.framerate.present_interval   = 0;    // Turn VSYNC -off-
                  config.render.framerate.enforcement_policy = 4;

                  // Trigger a re-sync
                  SK_GetCommandProcessor ()->ProcessCommandFormatted ("LatentSync.ResyncRate %d", config.render.framerate.latent_sync.scanline_resync - 1);
                  SK_GetCommandProcessor ()->ProcessCommandFormatted ("LatentSync.ResyncRate %d", config.render.framerate.latent_sync.scanline_resync + 1);
                  break;

                case limiter_mode_e::Reflex:
                  if (! std::exchange (original_reflex_settings.changed, true))
                  {
                    original_reflex_settings.low_latency = config.nvidia.reflex.low_latency;
                    original_reflex_settings.override    = config.nvidia.reflex.override;
                    original_reflex_settings.enable      = config.nvidia.reflex.enable;
                  }

                  config.nvidia.reflex.use_limiter = true;
                  config.nvidia.reflex.low_latency = true;
                  config.nvidia.reflex.override    = true;
                  config.nvidia.reflex.enable      = true;

                  config.render.framerate.present_interval = original_vsync_settings.present_interval;
                  break;
              }

              if (mode != (int)limiter_mode_e::Reflex)
              {
                config.nvidia.reflex.use_limiter = false;

                if (std::exchange (original_reflex_settings.changed, false))
                {
                  config.nvidia.reflex.low_latency = original_reflex_settings.low_latency;
                  config.nvidia.reflex.override    = original_reflex_settings.override;
                  config.nvidia.reflex.enable      = original_reflex_settings.enable;
                }
              }

              _ResetLimiter ();
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip ();
              ImGui::Text         ("Latency Settings");
              ImGui::Separator    ();
              ImGui::BeginGroup   ();
              ImGui::BulletText   ("Normal Mode:\t");
              ImGui::BulletText   ("Low-Latency:\t");
              ImGui::BulletText   ("Latent Sync:\t");
              if (rb.isReflexSupported ())
              ImGui::BulletText   ("NVIDIA Reflex:\t");
              ImGui::EndGroup     ();
              ImGui::SameLine     ();
              ImGui::BeginGroup   ();
              ImGui::TextUnformatted
                                  ("Prioritizes Minimum Stutter");
              ImGui::TextUnformatted
                                  ("Ideal for VRR displays; VRR should compensate for potential stutter");
              ImGui::TextUnformatted
                                  ("Ideal for Fixed-Refresh Displays; tearing possible, but location is controlled");
              if (rb.isReflexSupported ())
                ImGui::TextUnformatted
                                  ("Ideal for DLSS3 Frame Generation; worse consistency than SK in non DLSS-G scenarios");
              ImGui::EndGroup     ();
              if (config.render.framerate.present_interval == 0)
              {
                ImGui::Separator    ();
                ImGui::TextColored  (ImVec4 (.666f, 1.f, 1.f, 1.f), ICON_FA_INFO_CIRCLE);
                ImGui::SameLine     (  );
                ImGui::PushStyleColor
                          (ImGuiCol_Text, ImVec4 (0.825f, 0.825f, 0.825f, 1.f));
                ImGui::TextUnformatted
                                    ("Right-click the Framerate Limit slider to "
                                     "configure Latent Sync");
                ImGui::PopStyleColor(  );
              }
              ImGui::EndTooltip   ();
            }

            if ( rb.api == SK_RenderAPI::D3D11 ||
                 rb.api == SK_RenderAPI::D3D12 )
            {
              if (ImGui::Checkbox ("Drop Late Frames", &config.render.framerate.drop_late_flips))
                _ResetLimiter ();

              if (ImGui::IsItemHovered ())
              {
                ImGui::SetTooltip ("Always Present Newest Frame (DXGI Flip Model)");
              }

              ImGui::SameLine    ();
              ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
              ImGui::SameLine    ();
            }

            if (sk::NVAPI::nv_hardware)
            {
              bool triggered =
                config.render.framerate.auto_low_latency.triggered;

              if (triggered)
              {
                ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImVec4 (0.1f, 1.0f, 0.1f, 1.0f));
                ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImVec4 (0.4f, 0.9f, 0.4f, 1.0f));
                ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImVec4 (0.1f, 1.0f, 0.1f, 1.0f));
              }

              if (ImGui::Checkbox ("Auto VRR Mode", &config.render.framerate.auto_low_latency.waiting))
              {
                if (config.render.framerate.auto_low_latency.triggered)
                    config.render.framerate.auto_low_latency.waiting = false;

                config.render.framerate.auto_low_latency.triggered = false;
              }

              if (triggered)
                ImGui::PopStyleColor (3);

              if (ImGui::IsItemHovered ())
              {
                ImGui::BeginTooltip    ();
                ImGui::TextUnformatted ("The Framerate Limiter Self-Optimizes When VRR is Detected");
                ImGui::Separator       ();
                ImGui::BulletText      ("Limit will be set lower than refresh to remove 1 frame of latency");
                ImGui::BulletText      ("Games will be prevented from using 1/2, 1/3 or 1/4 Refresh VSYNC");
                if (config.render.framerate.auto_low_latency.policy.ultra_low_latency)
                {
                  ImGui::BulletText    ("NVIDIA Reflex will be set to Low Latency + Boost mode");
                  ImGui::BulletText    ("Framerate limiter mode will be set to VRR Optimized");
                }
                else
                  ImGui::BulletText      ("NVIDIA Reflex will be set to Low Latency mode");
                ImGui::TextColored     (ImVec4 (1.f, 1.f, .5f, 1.f), " " ICON_FA_MOUSE);
                ImGui::SameLine        ();
                ImGui::TextUnformatted ("Right-click to configure Auto VRR behavior");
                ImGui::Separator       ();
                ImGui::TextColored     (ImVec4 (.6f, .6f, 1.f, 1.f), ICON_FA_INFO_CIRCLE);
                ImGui::SameLine        ();
                ImGui::TextUnformatted ("This option turns itself off and displays green after optimizing the framerate limiter");
                ImGui::EndTooltip      ();
              }

              ImGui::OpenPopupOnItemClick ("AutoVRRConfig");

              if (ImGui::BeginPopup ("AutoVRRConfig"))
              {
                bool vrr_changed = false;

                vrr_changed |=
                  ImGui::Checkbox ("Enable By Default", &config.render.framerate.auto_low_latency.policy.global_opt);

                if (ImGui::IsItemHovered ())
                  ImGui::SetTooltip ("Controls whether games automatically use this feature");

                vrr_changed |=
                  ImGui::Checkbox ("Ultra Low-Latency", &config.render.framerate.auto_low_latency.policy.ultra_low_latency);

                if (ImGui::IsItemHovered ())
                  ImGui::SetTooltip ("Aggressively favor low-latency even if it worsens frame pacing");

                // Turn on Auto-Low Latency after making any changes
                if (vrr_changed)
                {
                  config.render.framerate.auto_low_latency.waiting   = config.render.framerate.auto_low_latency.policy.global_opt;
                  config.render.framerate.auto_low_latency.triggered = false;
                }

                ImGui::EndPopup ();
              }
            }

            ImGui::EndGroup     ();
            ImGui::PopItemWidth ();
            ImGui::SameLine     (0.0f, 20.0f);
            ImGui::SeparatorEx  (ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine     (0.0f, 20.0f);
          }

          ImGui::BeginGroup ();

          changed |=
            ImGui::Checkbox ( "Sleepless Render Thread(s)",
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

          int min_render_prio = 3;

          switch (config.priority.minimum_render_prio)
          {
            case THREAD_PRIORITY_IDLE:          min_render_prio = 0; break;
            case THREAD_PRIORITY_LOWEST:        min_render_prio = 1; break;
            case THREAD_PRIORITY_BELOW_NORMAL:  min_render_prio = 2; break;
            case THREAD_PRIORITY_NORMAL:        min_render_prio = 3; break;
            default:
            case THREAD_PRIORITY_ABOVE_NORMAL:  min_render_prio = 4; break;
            case THREAD_PRIORITY_HIGHEST:       min_render_prio = 5; break;
            case THREAD_PRIORITY_TIME_CRITICAL: min_render_prio = 6; break;
          }

          ImGui::PushItemWidth (ImGui::GetContentRegionAvail ().x);

          if (ImGui::Combo ( "###Render Thread Priority", &min_render_prio,
                                "Render Priority:\tIdle\0"
                                "Render Priority:\tLowest\0"
                                "Render Priority:\tBelow Normal\0"
                                "Render Priority:\tNormal\0"
                                "Render Priority:\tAbove Normal\0"
                                "Render Priority:\tHighest\0"
                                "Render Priority:\tTime Critical\0\0" ))
          {
            switch (min_render_prio)
            {
              case 0:  config.priority.minimum_render_prio = THREAD_PRIORITY_IDLE;          break;
              case 1:  config.priority.minimum_render_prio = THREAD_PRIORITY_LOWEST;        break;
              case 2:  config.priority.minimum_render_prio = THREAD_PRIORITY_BELOW_NORMAL;  break;
              case 3:  config.priority.minimum_render_prio = THREAD_PRIORITY_NORMAL;        break;
              default:
              case 4:  config.priority.minimum_render_prio = THREAD_PRIORITY_ABOVE_NORMAL;  break;
              case 5:  config.priority.minimum_render_prio = THREAD_PRIORITY_HIGHEST;       break;
              case 6:  config.priority.minimum_render_prio = THREAD_PRIORITY_TIME_CRITICAL; break;
            }
          }

          ImGui::PopItemWidth ();
#if 0
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
#endif

          bool spoof =
            ( config.render.framerate.override_num_cpus != SK_NoPreference );

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

          // Only show these for debug purposes, normal users never need to see
          if (config.system.log_level > 0)
          {
            typedef enum _D3DKMT_SCHEDULINGPRIORITYCLASS {
              D3DKMT_SCHEDULINGPRIORITYCLASS_IDLE,
              D3DKMT_SCHEDULINGPRIORITYCLASS_BELOW_NORMAL,
              D3DKMT_SCHEDULINGPRIORITYCLASS_NORMAL,
              D3DKMT_SCHEDULINGPRIORITYCLASS_ABOVE_NORMAL,
              D3DKMT_SCHEDULINGPRIORITYCLASS_HIGH,
              D3DKMT_SCHEDULINGPRIORITYCLASS_REALTIME // Mortals are not authorized to use this :)
            } D3DKMT_SCHEDULINGPRIORITYCLASS;

            using D3DKMTSetProcessSchedulingPriorityClass_pfn = NTSTATUS (WINAPI*)(HANDLE, D3DKMT_SCHEDULINGPRIORITYCLASS );
            using D3DKMTGetProcessSchedulingPriorityClass_pfn = NTSTATUS (WINAPI*)(HANDLE, D3DKMT_SCHEDULINGPRIORITYCLASS*);

            static D3DKMTSetProcessSchedulingPriorityClass_pfn
                   D3DKMTSetProcessSchedulingPriorityClass =
                  (D3DKMTSetProcessSchedulingPriorityClass_pfn)SK_GetProcAddress ( L"Gdi32.dll",
                  "D3DKMTSetProcessSchedulingPriorityClass");

            static D3DKMTGetProcessSchedulingPriorityClass_pfn
                   D3DKMTGetProcessSchedulingPriorityClass =
                  (D3DKMTGetProcessSchedulingPriorityClass_pfn)SK_GetProcAddress ( L"Gdi32.dll",
                  "D3DKMTGetProcessSchedulingPriorityClass");

            D3DKMT_SCHEDULINGPRIORITYCLASS                                       sched_class;
              D3DKMTGetProcessSchedulingPriorityClass (SK_GetCurrentProcess (), &sched_class);

            static std::string
                sched_drop_down;
            if (sched_drop_down.empty ())
            {
              sched_drop_down += "Idle";
              sched_drop_down += '\0';

              sched_drop_down += "Below Normal";
              sched_drop_down += '\0';

              sched_drop_down += "Normal";
              sched_drop_down += '\0';

              sched_drop_down += "Above Normal";
              sched_drop_down += '\0';

              sched_drop_down += "High";
              sched_drop_down += '\0';
              sched_drop_down += '\0';
            }

            if (config.render.framerate.present_interval == 0)
            {
              if (ImGui::Combo ("D3DKMT Process Priority Class", (int *)&sched_class, sched_drop_down.data ()))
              {
                D3DKMTSetProcessSchedulingPriorityClass (SK_GetCurrentProcess (), sched_class);
              }
            }
          }

          SK_Framerate_EnergyControlPanel ();

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

      if (rb.isReflexSupported ())
      {
        if (ImGui::Checkbox ("Latency Analysis###ReflexLatency", &latency))
        {
          SK_ImGui_Widgets->latency->setVisible (latency).
                                     setActive  (latency);
        }
      }
    }

    if (rb.isHDRCapable () || __SK_HDR_16BitSwap || __SK_HDR_10BitSwap)
    {
      ImGui::SameLine ();
      if (ImGui::Checkbox ("HDR Display", &hdr))
      {
        SK_ImGui_Widgets->hdr_control->setVisible (hdr).
                                       setActive  (hdr);
      }
    }

    else
    { SK_ImGui_Widgets->hdr_control->setVisible (false).
                                     setActive  (false);
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

    static std::set <SK_ConfigSerializedKeybind *>
      keybinds = {
        &config.widgets.hide_all_widgets_keybind
      };

    ImGui::BeginGroup ();
    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {
      ImGui::Text
      ( "%s:  ",keybind->bind_name );
    }
    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {Keybinding(keybind,  keybind->param);}
    ImGui::EndGroup   ();
    ImGui::EndGroup   ();

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
        rb.screenshot_mgr->init ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ( "In D3D11/12 games, integrates SK's high-performance/quality screenshot capture "
                            "system, adding HDR -> LDR support and other things." );
      }

      if (config.steam.screenshots.enable_hook)
      {
        ImGui::TreePush ("");
        ImGui::Checkbox ("Show OSD in Steam Screenshots",
                                    &config.screenshots.show_osd_by_default);
        ImGui::TreePop  (  );
      }
    }

    bool png_changed = false;

    ImGui::Checkbox ("Copy Screenshots to Clipboard",    &config.screenshots.copy_to_clipboard);
    ImGui::Checkbox ("Play Sound on Screenshot Capture", &config.screenshots.play_sound       );

    png_changed =
      ImGui::Checkbox (
        rb.isHDRCapable () ? "Keep HDR Screenshots          " :
                        "Keep Lossless .PNG Screenshots",
                                    &config.screenshots.png_compress
                      );

    if (rb.isHDRCapable ())
    {
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("See the HDR Menu to configure HDR Screenshot Format and Compression Settings.");
    }

    ImGui::EndGroup ();

    static std::set <SK_ConfigSerializedKeybind *>
      keybinds = {
        &config.screenshots.sk_osd_free_keybind,
        &config.screenshots.sk_osd_insertion_keybind,
        &config.screenshots.no_3rd_party_keybind,
        &config.screenshots.clipboard_only_keybind,
      };

    // Add a HUD Free Screenshot keybind option if HUD shaders are present
    if (rb.api == SK_RenderAPI::D3D11 && keybinds.size () == 2 && ReadAcquire (&SK_D3D11_TrackingCount->Conditional) > 0)
        keybinds.emplace (&config.screenshots.game_hud_free_keybind);

    // Experimental D3D12 HUDless
    else if (rb.api == SK_RenderAPI::D3D12 && keybinds.size () == 2)
        keybinds.emplace (&config.screenshots.game_hud_free_keybind);

    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {
      ImGui::Text
      ( "%s:  ",keybind->bind_name );
    }
    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    for ( auto& keybind : keybinds )
    {Keybinding(keybind,  keybind->param);}
    ImGui::EndGroup   ();

    if ( rb.screenshot_mgr->getRepoStats ().files > 0 )
    {
      const SK_ScreenshotManager::screenshot_repository_s& repo =
        rb.screenshot_mgr->getRepoStats (png_changed);

      ImGui::BeginGroup (  );
      ImGui::Separator  (  );
      ImGui::TreePush   ("");
      ImGui::Text ( "%u files using %hs",
                      repo.files,
                        SK_WideCharToUTF8 (SK_File_SizeToString (repo.liSize.QuadPart, Auto, pTLS).data ()).c_str ()
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
            rb.screenshot_mgr->getBasePath (),
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

  SK::ControlPanel::PlugIns::Draw  ();
  SK::ControlPanel::Platform::Draw ();

  SK_ImGui::BatteryMeter ();

  if (recenter)
    SK_ImGui_CenterCursorAtPos ();

  ImVec2 pos  = ImGui::GetWindowPos  ();
  ImVec2 size = ImGui::GetWindowSize ();

  SK_ImGui_LastWindowCenter.x = ( pos.x + size.x / 2.0f );
  SK_ImGui_LastWindowCenter.y = ( pos.y + size.y / 2.0f );
  }

  ImGui::End           ();
  ImGui::PopStyleColor ();

  static bool     was_open = false;
  if ((! open) && was_open)
  {
    SK_XInput_ZeroHaptics (config.input.gamepad.steam.ui_slot ); // XXX: MAKE SEPARATE
    SK_XInput_ZeroHaptics (config.input.gamepad.xinput.ui_slot);
  }

  was_open
       = open;
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
SK_Platform_GetUserName (char* pszName, int max_len = 512)
{
  if (pszName == nullptr)
    return;

  if (*pszName != L'\0')
    return;

  if (SK::SteamAPI::AppID () != 0)
  {
    if (SK_SteamAPI_Friends () != nullptr)
    {
      auto orig_se =
        SK_SEH_ApplyTranslator (
          SK_FilteringStructuredExceptionTranslator (
            EXCEPTION_ACCESS_VIOLATION
          )
        );

      try
      {
        strncpy_s (                         pszName,  max_len,
          SK_SteamAPI_Friends ()->GetPersonaName (), _TRUNCATE
                  );
      }
      catch (const SK_SEH_IgnoredException&)
      {                                    }
      SK_SEH_RemoveTranslator (orig_se);
    }
  }

  else if (SK::EOS::UserID () != 0 && (! SK::EOS::PlayerNickname ().empty ()))
  {
    strncpy_s (pszName,                            max_len,
               SK::EOS::PlayerNickname ().data (), _TRUNCATE);
  }
}


LRESULT
CALLBACK
SK_ImGui_MouseProc (int code, WPARAM wParam, LPARAM lParam)
{
  if (code < 0 || (! SK_IsGameWindowActive ())) // We saw nothing (!!)
    return CallNextHookEx (0, code, wParam, lParam);

  auto& io =
    ImGui::GetIO ();

  MOUSEHOOKSTRUCT* mhs =
    (MOUSEHOOKSTRUCT*)lParam;

  bool bPassthrough = true;

  if (mhs->wHitTestCode == HTCLIENT || mhs->wHitTestCode == HTTRANSPARENT)
  {
    if (mhs->hwnd == game_window.hWnd || mhs->hwnd == game_window.child)
    {
      POINT                                          pt (mhs->pt);
      ScreenToClient             (game_window.hWnd, &pt);
      if (ChildWindowFromPointEx (game_window.hWnd,  pt, CWP_SKIPDISABLED) == game_window.hWnd)
      {
        bPassthrough = false;

        io.KeyCtrl  |= ((mhs->dwExtraInfo & MK_CONTROL) != 0);
        io.KeyShift |= ((mhs->dwExtraInfo & MK_SHIFT  ) != 0);
      }
    }
  }

  switch (wParam)
  {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
      if (! bPassthrough)
      {
        io.MouseDown [0] = true;

        // Only capture mouse clicks when the window is in the foreground, failure to let
        //   left-clicks passthrough would prevent activating the game window.
        if ( SK_ImGui_WantMouseCapture () && game_window.active &&
                SK_GetForegroundWindow () == game_window.hWnd )
          return 1;
      }
      break;

    case WM_LBUTTONUP:
      if (! bPassthrough)
      {
        if (SK_ImGui_WantMouseCapture ())
          return 1;
      }
      break;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
      if (! bPassthrough)
      {
        io.MouseDown [1] = true;

        if (SK_ImGui_WantMouseCapture ())
          return 1;
      }
      break;

    case WM_RBUTTONUP:
      if (! bPassthrough)
      {
        if (SK_ImGui_WantMouseCapture ())
          return 1;
      }
      break;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
      if (! bPassthrough)
      {
        io.MouseDown [2] = true;

        if (SK_ImGui_WantMouseCapture ())
          return 1;
      }
      break;

    case WM_MBUTTONUP:
      if (! bPassthrough)
      {
        if (SK_ImGui_WantMouseCapture ())
          return 1;
      }
      break;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
      if (! bPassthrough)
      {
        MOUSEHOOKSTRUCTEX* mhsx =
       (MOUSEHOOKSTRUCTEX*)lParam;

        io.MouseDown [3] |= ((HIWORD (mhsx->mouseData)) == XBUTTON1);
        io.MouseDown [4] |= ((HIWORD (mhsx->mouseData)) == XBUTTON2);

        if (SK_ImGui_WantMouseCapture ())
          return 1;
      } break;

    case WM_XBUTTONUP:
      if (! bPassthrough)
      {
        if (SK_ImGui_WantMouseCapture ())
          return 1;
      } break;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
      if (! bPassthrough)
      {
        MOUSEHOOKSTRUCTEX* mhsx =
       (MOUSEHOOKSTRUCTEX*)lParam;

        if (wParam == WM_MOUSEWHEEL)
        {
          io.MouseWheel +=
            (float)GET_WHEEL_DELTA_WPARAM (mhsx->mouseData) /
                (float)WHEEL_DELTA;
        }

        else {
          io.MouseWheelH +=
            (float)GET_WHEEL_DELTA_WPARAM (mhsx->mouseData) /
                (float)WHEEL_DELTA;
        }

        if (SK_ImGui_WantMouseCapture ())
          return 1;
      } break;

    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
      if (! bPassthrough)
      {
        if (SK_ImGui_WantMouseCapture ())
        {
          // Install a mouse tracker to get WM_MOUSELEAVE
          if (! (game_window.mouse.tracking && game_window.mouse.inside))
            SK_ImGui_UpdateMouseTracker ();

          return 1;
        }
      }

    //SK_ImGui_Cursor.last_move = current_time;
      break;
  }

  return
    CallNextHookEx (0, code, wParam, lParam);
}

LRESULT
CALLBACK
SK_ImGui_KeyboardProc (int       code, WPARAM wParam, LPARAM lParam)
{
  if (code < 0 || (! SK_IsGameWindowActive ())) // We saw nothing (!!)
    return CallNextHookEx (0, code, wParam, lParam);

  bool //wasPressed = (((DWORD)lParam) & (1UL << 30UL)) != 0UL,
          isPressed = (((DWORD)lParam) & (1UL << 31UL)) == 0UL;//,
        //isAltDown = (((DWORD)lParam) & (1UL << 29UL)) != 0UL;

  SHORT vKey =
      std::min (static_cast <SHORT> (wParam),
                static_cast <SHORT> (  511));
  auto& io =
    ImGui::GetIO ();

  if (io.KeyAlt && vKey == VK_F4 && isPressed)
  {
    if (SK_ImGui_Active () || config.input.keyboard.catch_alt_f4 || config.input.keyboard.override_alt_f4 || SK_ImGui_WantKeyboardCapture ())
        SK_ImGui_WantExit = true;

    if (SK_ImGui_Visible || SK_ImGui_WantKeyboardCapture ()) return 1;
  }

  if (SK_ImGui_WantKeyboardCapture () && (! io.WantTextInput))
  {
    if (isPressed) SK_Console::getInstance ()->KeyDown ((BYTE)(vKey & 0xFF), 0x0);
    else           SK_Console::getInstance ()->KeyUp   ((BYTE)(vKey & 0xFF), 0x0);

    return 1;
  }

  return
    CallNextHookEx (0, code, wParam, lParam);
}

void
SK_ImGui_StageNextFrame (void)
{
  auto& virtualToHuman =
    virtKeyCodeToFullyLocalizedKeyName.get ();

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
  {   last_frame  = SK_GetFramesDrawn ();

    auto& io =
      ImGui::GetIO ();

    font.size           = ImGui::GetFont () != nullptr                     ?
                          ImGui::GetFont ()->FontSize * io.FontGlobalScale :
                                                                          1.0f;

    font.size_multiline = SK::ControlPanel::font.size      +
                          ImGui::GetStyle ().ItemSpacing.y +
                          ImGui::GetStyle ().ItemInnerSpacing.y;
  }

  current_tick = static_cast <uint64_t> (SK_QueryPerf ().QuadPart);
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
  auto& io    = ImGui::GetIO    ();
  auto& style = ImGui::GetStyle ();

  static float orgWindowBg =
    style.Colors [ImGuiCol_WindowBg].w;

  if (config.imgui.render.disable_alpha)
    style.Colors [ImGuiCol_WindowBg].w = 1.0f;
  else
    style.Colors [ImGuiCol_WindowBg].w = orgWindowBg;

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

  if (config.render.framerate.latent_sync.show_fcat_bars)
  {
    extern void SK_ImGui_DrawFCAT (void);
                SK_ImGui_DrawFCAT ();
  }

  static auto widgets =
  {
    SK_ImGui_Widgets->frame_pacing,
      SK_ImGui_Widgets->volume_control,
        SK_ImGui_Widgets->gpu_monitor,
          SK_ImGui_Widgets->cpu_monitor,
            SK_ImGui_Widgets->d3d11_pipeline,
              SK_ImGui_Widgets->thread_profiler,
                SK_ImGui_Widgets->hdr_control,
                  SK_ImGui_Widgets->tobii,
                    SK_ImGui_Widgets->latency
  };

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
    SK_ControlPanel_Activated = true;
    keep_open                 = SK_ImGui_ControlPanel ();

    if (imgui_demo)    ImGui::ShowDemoWindow     ( );
    if (imgui_debug)   ImGui::ShowDebugLogWindow ( );
    if (imgui_metrics) ImGui::ShowMetricsWindow  ( );
    if (imgui_about)   ImGui::ShowAboutWindow    ( );
    if (implot_demo)   ImPlot::ShowDemoWindow    ( );
  }


  // Don't draw widgets on the first frame, stuff may not be
  //   fully initialized yet...
  if (SK_GetFramesDrawn () > 1)
  {
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
  



    if (! SK_GetStoreOverlayState (true))
    {
      SK_DrawOSD     ();
      SK_DrawConsole ();
    }
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

    static char              szName [512] = { };
    SK_Platform_GetUserName (szName);


    ImGui::TextColored     (ImColor::HSV (.11f, 1.f, 1.f),  "%hs   ",
                                                  SK_WideCharToUTF8 (SK_GetPluginName ()).c_str ()); ImGui::SameLine ();

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

    snprintf (current_ver,        127, "%hs (%i)", SK_WideCharToUTF8 (vinfo.package).c_str (), vinfo.build);
    snprintf (current_branch_str,  63, "%hs",      SK_WideCharToUTF8 (vinfo.branch).c_str  ());

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
                               R"('%hs + %hs + %hs')",
                                    SK_WideCharToUTF8 (virtualToHuman [VK_CONTROL]).c_str (),
                                    SK_WideCharToUTF8 (virtualToHuman [VK_SHIFT]).c_str   (),
                                    SK_WideCharToUTF8 (virtualToHuman [VK_BACK]).c_str    () );
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

    ImGui::SameLine (); ImGui::Spacing     ();
    ImGui::SameLine (); ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine (); ImGui::Spacing     ();
    ImGui::SameLine ();
    ImGui::TextColored  (ImVec4 (0.999f, 0.666f, 0.333f, 1.f), ICON_FA_INFO_CIRCLE);
    ImGui::SameLine ();
    ImGui::TextUnformatted ("Configure this Startup Banner in OSD Settings.");

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
          ImVec2 center =
            pWin->Rect ().GetCenter ();

          SK_ImGui_CenterCursorAtPos (center);
          ImGui::SetWindowFocus      (pWin->Name);
        }
      }

      SK_ImGuiEx_Visible = true;
    }

    else
      SK_ImGuiEx_Visible = false;
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



  if (SK_GetFramesDrawn () > 1)
    SK_ImGui_ProcessWarnings ();


  if (SK_ImGui_UnconfirmedDisplayChanges)
  {
    SK_ImGui_ConfirmDisplaySettings (nullptr, "", {});

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

        /// XXX: FIXME
#if 0
        ImGui::SetNavID (
          ImGui::GetItemID (), 0
        );
#endif

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


  auto _PerformExit = [](void)
  {
    SK_SelfDestruct     (   );
    SK_TerminateProcess (0x0);
    ExitProcess         (0x0);
  };

  if (SK_ImGui_WantExit)
  {
    // Uncomment this to always display a confirmation dialog
    //SK_ImGuiEx_Visible = true; // Make into user config option

    if (config.input.keyboard.catch_alt_f4 || SK_ImGui_Visible)
      SK_ImGui_ConfirmExit ();             // ^^ Control Panel In Use
    else // Want exit, but confirmation is disabled?
      _PerformExit ();

    SK_ImGui_WantExit = false;
  }

  if ( ImGui::BeginPopupModal ( SK_ImGui_WantRestart ?
                   "Confirm Forced Software Restart" :
                   "Confirm Forced Software Termination",
                                  nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoScrollbar      |
                                    ImGuiWindowFlags_NoScrollWithMouse )
     )
  {
    SK_ImGuiEx_Visible = true;
    nav_usable = true;

    static const char* szConfirmExit    = " Confirm Exit? ";
    static const char* szConfirmRestart = " Confirm Restart? ";
           const char* szConfirm        = SK_ImGui_WantRestart ?
                                              szConfirmRestart :
                                              szConfirmExit;
    static const char* szDisclaimer     =
      "\n         You will lose any unsaved game progress.      \n\n";

    ImGui::TextColored (ImColor::HSV (0.075f, 1.0f, 1.0f), "%hs", szDisclaimer);
    ImGui::Separator   ();

    ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f),         szConfirm);

    ImGui::SameLine    (); ImGui::Spacing (); ImGui::SameLine ();

    static bool
        center_mouse = false;
    if (center_mouse)
    {
      if ( static int frames = 0;
                    ++frames > 5 )
      {
        ImGui::GetIO ().MousePos = ImGui::GetCursorScreenPos ();
        ImGui::GetIO ().WantSetMousePos = true;

        center_mouse = false;
        frames       = 0;
      }
    }

    if (ImGui::IsWindowAppearing ())
    {
      center_mouse = true;
    }

    if (ImGui::Button  ("Okay"))
    {
      SK_ImGuiEx_Visible = false;

      if (SK_ImGui_WantRestart)
      {
        SK_RestartGame ();
      }

      else
      {
        _PerformExit ();
      }
    }
    if (ImGui::IsWindowAppearing ())
    {
      ImGui::GetIO ().NavActive  = true;
      ImGui::GetIO ().NavVisible = true;

      /// XXX: FIXME
#if 0
      ImGui::SetNavID (
        ImGui::GetItemID (), 0
      );
#endif

      GImGui->NavDisableHighlight  = false;
      GImGui->NavDisableMouseHover =  true;
    }

    //ImGui::PushItemWidth (ImGui::GetWindowContentRegionWidth ()*0.33f);
    //ImGui::SameLine (); ImGui::SameLine (); ImGui::PopItemWidth ();

    ImGui::SameLine    ();

    if (ImGui::Button  ("Cancel"))
    {
      SK_ImGui_WantExit    = false;
      SK_ImGui_WantRestart = false;
      SK_ImGuiEx_Visible   = false;
      SK_ReShadeAddOn_ActivateOverlay (false);
      nav_usable           = orig_nav_state;
      ImGui::CloseCurrentPopup ();
    }

    if (! SK_ImGui_WantRestart)
    {
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
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("If disabled, game's default Alt + F4 behavior will apply");

        if (SK_ImGui_Visible)
        {
          ImGui::Separator  ();
          ImGui::BulletText ("Alt + F4 always displays a confirmation if the control panel is visible");
        }
        ImGui::EndTooltip   ();
      }
    }

    ImGui::EndPopup       ();
  }



  if (std::exchange (dxgi_mode_switch_failure, false))
  {
    SK_ImGui_SetNextWindowPosCenter     (ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints ( ImVec2 (360.0f, 40.0f),
                                            ImVec2 ( 0.925f * io.DisplaySize.x,
                                                     0.925f * io.DisplaySize.y )
                                        );

    if (! ImGui::IsPopupOpen ("Fullscreen / Windowed Display Mode Switch Failure"))
    {
      SK_ImGui_WantExit = false;
      orig_nav_state    = nav_usable;

      ImGui::OpenPopup ("Fullscreen / Windowed Display Mode Switch Failure");
    }
  }

  if ( ImGui::BeginPopupModal (
         "Fullscreen / Windowed Display Mode Switch Failure",
           nullptr,
             ImGuiWindowFlags_AlwaysAutoResize |
             ImGuiWindowFlags_NoScrollbar      |
             ImGuiWindowFlags_NoScrollWithMouse )
     )
  {
    static bool
        shown_once = false;
    if (shown_once) ImGui::CloseCurrentPopup ();

    else
    {
      auto _ClosePopup = [](void)
      {
        shown_once           = true;
        SK_ImGui_WantExit    = false;
        SK_ImGui_WantRestart = false;
        SK_ImGuiEx_Visible   = false;
        SK_ReShadeAddOn_ActivateOverlay (false);
        nav_usable           = orig_nav_state;
        ImGui::CloseCurrentPopup ();
      };

      nav_usable = true;

      static const char* szAction      = " Reconfigure? ";
      static const char* szDescription =
        "\n         Flip Model Fullscreen Mode Switch Failed      \n\n";

      ImGui::TextColored (ImColor::HSV (0.075f, 1.0f, 1.0f), "%hs", szDescription);
      ImGui::Separator   ();
      ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f),         szAction);

      ImGui::SameLine    (); ImGui::Spacing (); ImGui::SameLine ();

      if (ImGui::Button  ("Force Windowed"))
      {
        config.display.force_windowed   =  true;
        config.display.force_fullscreen = false;

        _ClosePopup ();
      } if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Safest Option");
          ImGui::Spacing      ();
          ImGui::SameLine     ();
          ImGui::Text         ("Prefer SK's Window Mode Optimizations except for...");
          ImGui::Separator    ();
          ImGui::Text         ("Scenarios not to Force Windowed Mode:");
          ImGui::BulletText   ("Desktop and Game run at different Resolutions");
          ImGui::BulletText   ("Desktop and Game run at different Refresh Rates");
          ImGui::BulletText   ("Game is not giving you Hardware: Independent Flip");
          ImGui::EndTooltip   ();
        }

      if (ImGui::IsWindowAppearing ())
      {
        ImGui::GetIO ().MousePos    = ImGui::GetCursorScreenPos ();
        ImGui::GetIO ().MousePos.y += ImGui::GetTextLineHeight  () * 2.0f;
        ImGui::GetIO ().WantSetMousePos = true;

        ImGui::GetIO ().NavActive  = true;
        ImGui::GetIO ().NavVisible = true;

        /// XXX: FIXME
#if 0
        ImGui::SetNavID (
          ImGui::GetItemID (), 0
        );
#endif

        GImGui->NavDisableHighlight  = false;
        GImGui->NavDisableMouseHover =  true;
      }

      //ImGui::PushItemWidth (ImGui::GetWindowContentRegionWidth ()*0.33f);
      //ImGui::SameLine (); ImGui::SameLine (); ImGui::PopItemWidth ();

      ImGui::SameLine    ();

      if (ImGui::Button  ("Disable Flip Model"))
      {
        config.render.framerate.flip_discard = false;

        _ClosePopup ();
      } if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Not Recommended");
          ImGui::Spacing      ();
          ImGui::SameLine     ();
          ImGui::Text         ("Task switching performance will suffer, HDR will not work");
          ImGui::Separator    ();
          ImGui::Text         ("Last-resort, if you really -must- have fullscreen exclusive");
          ImGui::EndTooltip   ();
        }

      ImGui::SameLine    ();

      if (ImGui::Button  ("No"))
      {
        _ClosePopup ();
      } if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip (
            "Do nothing, stop showing this message (until the next game launch) "
            "and hope for the best."
          );
        }
    }

    ImGui::EndPopup       ();
  }


  if (io.WantSetMousePos && SK_IsGameWindowActive ())
  {
    POINT                 ptCursor;
    if (SK_GetCursorPos (&ptCursor))
    {
      // Ignore this if the cursor is in a different application
      GetWindowRect (game_window.hWnd,
                    &game_window.actual.window);
      if (PtInRect (&game_window.actual.window, ptCursor))
      {
        SK_ImGui_Cursor.pos.x = static_cast <LONG> (io.MousePos.x);
        SK_ImGui_Cursor.pos.y = static_cast <LONG> (io.MousePos.y);

        POINT screen_pos = SK_ImGui_Cursor.pos;

        SK_ImGui_Cursor.LocalToScreen (&screen_pos);
        SK_SetCursorPos ( screen_pos.x,
                          screen_pos.y );

        SK_ImGui_UpdateCursor ();
      }
    }
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
SK_API
DWORD
SK_ImGui_DrawFrame ( _Unreferenced_parameter_ DWORD  dwFlags,
                                              LPVOID lpUser )
{
  UNREFERENCED_PARAMETER (dwFlags);
  UNREFERENCED_PARAMETER (lpUser);

  // Optionally:  Disable SK's OSD while the Steam overlay is active
  //
  if ( config.platform.overlay_hides_sk_osd &&
       SK_GetStoreOverlayState (true) )
  {
    return 0;
  }

  if (! imgui_staged)
  {
    SK_ImGui_StageNextFrame ();
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  // Popup Windows, actually
  SK_Steam_DrawOSD ();

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

extern bool SK_IsRectTooSmall (RECT* lpRect0, RECT* lpRect1);

extern RECT SK_Input_SaveClipRect    (RECT *pSave = nullptr);
extern RECT SK_Input_RestoreClipRect (void);

SK_API
void
SK_ImGui_Toggle (void)
{
  static ULONG64 last_frame = 0;

  if (last_frame != SK_GetFramesDrawn ())
  {
    current_time  = SK_timeGetTime    ();
    last_frame    = SK_GetFramesDrawn ();
  }

  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  SK_ImGui_Visible = (! SK_ImGui_Visible);

  if (SK_ImGui_Visible)
    ImGui::SetNextWindowFocus ();

  static HMODULE hModTBFix = SK_GetModuleHandle (L"tbfix.dll");
  static HMODULE hModTZFix = SK_GetModuleHandle (L"tzfix.dll");

  if (! (config.window.confine_cursor || config.window.unconfine_cursor))
  { // Expand clip rects while SK's UI is open so the mouse works as expected :)
    if (SK_ImGui_Active ())
    {
      RECT                     rectClip = { };
      if (     GetClipCursor (&rectClip) &&
           SK_IsRectTooSmall (&rectClip, &game_window.actual.window) )
      {
         SK_Input_SaveClipRect    ();
         SK_ClipCursor            (&game_window.actual.window);
      }
      else
        SK_Input_SaveClipRect     ();
    }
    else SK_Input_RestoreClipRect ();
  }

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
    }

    else
    {
      SK_ImGui_Cursor.last_move = current_time;

      //SK_ImGui_Cursor.showImGuiCursor ();

      if (SK::ControlPanel::Platform::WarnIfUnsupported ())
        config.imgui.show_eula = true;
      else
      {
        eula.never_show_again = true;
        eula.show             = false;
      }
    }
  }

  if (SK_ImGui_Visible)
  {
    // Reuse the game's overlay activation callback (if it has one)
    if (config.platform.reuse_overlay_pause)
      SK::SteamAPI::SetOverlayState (true);

    SK_Console::getInstance ()->visible = false;

    SK_ReShadeAddOn_ActivateOverlay (false);
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
    // Reuse the game's overlay activation callback (if it has one)
    if (config.platform.reuse_overlay_pause)
      SK::SteamAPI::SetOverlayState (false);

    nav_usable = false;
  }

  //reset_frame_history = true;

  if ((! SK_ImGui_Visible) && SK_ImGui_OpenCloseCallback->fn != nullptr)
  {
    if (SK_ImGui_OpenCloseCallback->fn (SK_ImGui_OpenCloseCallback->data))
      SK_ImGui_OpenCloseCallback->fn (SK_ImGui_OpenCloseCallback->data);
  }
}




void
SK_Display_UpdateOutputTopology (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  rb.updateOutputTopology ();
}