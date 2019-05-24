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

#include <imgui/imgui.h>
#include <imgui/backends/imgui_gl3.h>
#include <imgui/backends/imgui_d3d9.h>
#include <imgui/backends/imgui_d3d11.h>
#include <imgui/backends/imgui_d3d12.h>

#include <SpecialK/control_panel.h>

#include <SpecialK/control_panel/osd.h>
#include <SpecialK/control_panel/d3d9.h>
#include <SpecialK/control_panel/d3d11.h>
//#include <SpecialK/control_panel/d3d12.h>
#include <SpecialK/control_panel/opengl.h>
#include <SpecialK/control_panel/steam.h>
#include <SpecialK/control_panel/input.h>
#include <SpecialK/control_panel/window.h>
#include <SpecialK/control_panel/sound.h>
#include <SpecialK/control_panel/plugins.h>
#include <SpecialK/control_panel/compatibility.h>

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

#include <SpecialK/nvapi.h>
#include <SpecialK/osd/text.h>

LONG imgui_staged_frames   = 0;
LONG imgui_finished_frames = 0;
BOOL imgui_staged          = FALSE;

using namespace SK::ControlPanel;

SK_RenderAPI                 SK::ControlPanel::render_api;
unsigned long                SK::ControlPanel::current_time;
uint64_t                     SK::ControlPanel::current_tick;// Perf Counter
SK::ControlPanel::font_cfg_s SK::ControlPanel::font;


bool __imgui_alpha = true;

__declspec (dllexport)
void
SK_ImGui_Toggle (void);

extern uint32_t __stdcall SK_Steam_PiratesAhoy (void);
extern uint32_t __stdcall SK_SteamAPI_AppID    (void);

extern bool     __stdcall SK_FAR_IsPlugIn      (void);
extern void     __stdcall SK_FAR_ControlPanel  (void);

       bool               SK_DXGI_FullStateCache = false;


int __SK_FramerateLimitApplicationSite = 4;


std::wstring
SK_NvAPI_GetGPUInfoStr (void);

void
LoadFileInResource ( int          name,
                     int          type,
                     DWORD*       size,
                     const char** data )
{
  HMODULE handle =
    GetModuleHandle (nullptr);

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

extern void __stdcall SK_ImGui_DrawEULA (LPVOID reserved);
       bool IMGUI_API SK_ImGui_Visible;
       bool           SK_ReShade_Visible        = false;
       bool           SK_ControlPanel_Activated = false;

extern void ImGui_ToggleCursor (void);

struct show_eula_s {
  bool show;
  bool never_show_again;
} extern eula;

#include <map>
#include <unordered_set>
#include <unordered_map>
#include <d3d11.h>

#include <ImGui/imgui_internal.h>

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


struct sk_imgui_nav_state_s {
  bool nav_usable   = false;
  bool io_NavUsable = false;
  bool io_NavActive = false;
};

////e#include <stack>
////estd::stack <sk_imgui_nav_state_s> SK_ImGui_NavStack;
////e
////evoid
////eSK_ImGui_PushNav (bool enable)
////e{
////e  ImGuiIO& io =
////e    ImGui::GetIO ();
////e
////e  SK_ImGui_NavStack.push ( { nav_usable, io.NavUsable, io.NavActive } );
////e
////e  if (enable)
////e  {
////e    nav_usable = true; io.NavUsable = true; io.NavActive = true;
////e  }
////e}
////e
////evoid
////eSK_ImGui_PopNav (void)
////e{
////e  ImGuiIO& io =
////e    ImGui::GetIO ();
////e
////e  // Underflow?
////e  if (SK_ImGui_NavStack.empty ()) {
////e    dll_log->Log (L"ImGui Nav State Underflow");
////e    return;
////e  }
////e
////e  auto& stack_top =
////e    SK_ImGui_NavStack.top ();
////e
////e  nav_usable   = stack_top.nav_usable;
////e  io.NavActive = stack_top.io_NavActive;
////e  io.NavUsable = stack_top.io_NavUsable;
////e
////e  SK_ImGui_NavStack.pop ();
////e}



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
  ImGuiIO& io =
    ImGui::GetIO ();

  static const ImVec2 vCenterPivot   (0.5f, 0.5f);
         const ImVec2 vDisplayCenter (
    io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f
  );

  ImGui::SetNextWindowPos (vDisplayCenter, cond, vCenterPivot);
}

void
SK_ImGui_ProcessWarnings (void)
{
  static SK_Warning warning =
    { u8"", L"" };

  if (! SK_ImGui_Warnings->empty ())
  {
    if (warning.message.empty ())
      SK_ImGui_Warnings->try_pop (warning);
  }

  if (warning.message.empty ())
    return;

  ImGuiIO& io =
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
    ImGui::FocusWindow (ImGui::GetCurrentWindow ());

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
    else
      ImGui::SetItemDefaultFocus ();

    ImGui::EndPopup ();
  }
}

bool SK_ImGui_WantExit = false;

void
SK_ImGui_ConfirmExit (void)
{
  auto& io = ImGui::GetIO ();

  SK_ImGui_WantExit = true;

  SK_ImGui_SetNextWindowPosCenter     (ImGuiCond_Always);
  ImGui::SetNextWindowSizeConstraints ( ImVec2 (360.0f, 40.0f),
                                          ImVec2 ( 0.925f * io.DisplaySize.x,
                                                   0.925f * io.DisplaySize.y )
                                      );

  ImGui::OpenPopup ("Confirm Forced Software Termination");
}

bool
SK_ImGui_IsItemClicked (void)
{
  if (ImGui::IsItemClicked ())
    return true;

  if (ImGui::IsItemHovered ())
  {
    static auto& io = ImGui::GetIO ();

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
    static auto& io = ImGui::GetIO ();

    if (io.NavInputsDownDuration [ImGuiNavInput_Activate] > 0.4f)
    {
      io.NavInputsDownDuration [ImGuiNavInput_Activate] = 0.0f;
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

    static std::wstring title = L"";
                        title.clear ();

    extern std::wstring __stdcall SK_GetPluginName (void);
    extern bool         __stdcall SK_HasPlugin     (void);

    if (SK_HasPlugin () && hModTBFix == nullptr)
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
  static          HANDLE hAdjustEvent = 0;
  static volatile LONG   lInit        = 0;

  if (! InterlockedCompareExchange (&lInit, 1, 0))
  {
    hAdjustEvent =
      SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);

    SK_Thread_Create ([](LPVOID pUser)->
    DWORD
    {
      SetCurrentThreadDescription (L"[SK] Cursor Adjustment Thread");

      InterlockedIncrement (&lInit);


      HANDLE hEvent = (HANDLE)pUser;

      while (! ReadAcquire (&__SK_DLL_Ending))
      {
        if ( WaitForSingleObject ((HANDLE)pUser, INFINITE) == WAIT_OBJECT_0 )
        {
          SK_ClipCursor (nullptr);
             SK_AdjustWindow   ();       // Restore game's clip cursor behavior
        }
      }
      hAdjustEvent = INVALID_HANDLE_VALUE;
      CloseHandle (hEvent);


      SK_Thread_CloseSelf ();

      return 0;
    }, (LPVOID)hAdjustEvent);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&lInit, 2);

  if ((uintptr_t)hAdjustEvent > 0)
       SetEvent (hAdjustEvent);
}

bool reset_frame_history = true;
bool was_reset           = false;


class SK_ImGui_FrameHistory : public SK_Stat_DataHistory <float, 120>
{
public:
  void timeFrame       (double seconds)
  {
    addValue ((float)(1000.0 * seconds));
  }
};

SK_LazyGlobal <SK_ImGui_FrameHistory> SK_ImGui_Frames;


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
DisplayModeMenu (bool windowed)
{
  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  enum {
    DISPLAY_MODE_WINDOWED   = 0,
    DISPLAY_MODE_FULLSCREEN = 1
  };

  int         mode      = windowed ? DISPLAY_MODE_WINDOWED :
                                     DISPLAY_MODE_FULLSCREEN;
  int    orig_mode      = mode;
  bool       force      = windowed ? config.display.force_windowed :
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
       static_cast <int> (SK_RenderAPI::D3D11) )
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

      static DXGI_SWAP_CHAIN_DESC last_swapDesc = { };

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

      static std::string           combo_str    = "";
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
      static std::string           nv_color_combo = "";
      static int                   nv_color_idx   =  0;

      SK_ComQIPtr <IDXGISwapChain> pSwapChain (rb.swapchain);

      DXGI_SWAP_CHAIN_DESC  swapDesc = { };
      pSwapChain->GetDesc (&swapDesc);

      static int orig_item =
        current_item;

      // Re-build the set of options if resolution changes,
      //   or if the swapchain buffer format does
      if ( ! ( last_swapDesc.BufferDesc.Width                 == swapDesc.BufferDesc.Width  &&
               last_swapDesc.BufferDesc.Height                == swapDesc.BufferDesc.Height &&
               last_swapDesc.BufferDesc.Format                == swapDesc.BufferDesc.Format &&
               last_swapDesc.BufferDesc.RefreshRate.Numerator == swapDesc.BufferDesc.RefreshRate.Numerator )
         )
      {
        last_swapDesc.BufferDesc.Width  =
             swapDesc.BufferDesc.Width;
        last_swapDesc.BufferDesc.Height =
             swapDesc.BufferDesc.Height;
        last_swapDesc.BufferDesc.Format =
             swapDesc.BufferDesc.Format;
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
              std::string partial_encoding = "";
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
          pContainer->GetDisplayModeList ( swapDesc.BufferDesc.Format, 0x0,
                                           &num_modes, nullptr );

          dxgi_modes.resize (num_modes);

          if ( SUCCEEDED ( pContainer->GetDisplayModeList ( swapDesc.BufferDesc.Format, 0x0,
                                                            &num_modes, dxgi_modes.data () ) ) )
          {
            int idx = 1;

            combo_str += "No Override";
            combo_str += '\0';

            for ( auto& dxgi_mode : dxgi_modes )
            {
              if ( dxgi_mode.Format == swapDesc.BufferDesc.Format &&
                   dxgi_mode.Width  == swapDesc.BufferDesc.Width  &&
                   dxgi_mode.Height == swapDesc.BufferDesc.Height )
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

    modes = (config.window.borderless || mode == 2) ?
              "Bordered\0Borderless\0Borderless Fullscreen\0\0" :
               "Bordered\0Borderless\0\0";

    if (ImGui::Combo ("Window Style###SubMenu_WindowBorder_Combo", &mode, modes) && mode != orig_mode)
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
  }
}



#include <SpecialK/nvapi.h>

extern "C"
{
NVAPI_INTERFACE
NvAPI_D3D11_SetDepthBoundsTest ( IUnknown* pDeviceOrContext,
                                 NvU32     bEnable,
                                 float     fMinDepth,
                                 float     fMaxDepth );
};

extern float __target_fps;
extern float __target_fps_bg;

void
SK_ImGui_DrawGraph_FramePacing (void)
{
  static const auto& io =
    ImGui::GetIO ();

  const  float font_size           =
    ( ImGui::GetFont  ()->FontSize * io.FontGlobalScale );

  const  float font_size_multiline =
    ( ImGui::GetStyle ().ItemSpacing.y      +
      ImGui::GetStyle ().ItemInnerSpacing.y + font_size );


  float sum = 0.0f;

  float min = FLT_MAX;
  float max = 0.0f;

  for ( const auto& val : SK_ImGui_Frames->getValues () )
  {
    sum += val;

    if (val > max)
      max = val;

    if (val < min)
      min = val;
  }

  static       char szAvg [512] = { };
  static const bool ffx = SK_GetModuleHandle (L"UnX.dll") != nullptr;

  float& target =
    ( game_window.active || __target_fps_bg == 0.0f ) ?
            __target_fps  : __target_fps_bg;

  float target_frametime = ( target == 0.0f ) ?
                              ( 1000.0f / (ffx ? 30.0f : 60.0f) ) :
                                ( 1000.0f / fabs (target) );

  float frames = std::min ( (float)SK_ImGui_Frames->getUpdates  (),
                            (float)SK_ImGui_Frames->getCapacity () );


  if (ffx)
  {
    // Normal Gameplay: 30 FPS
    if (sum / frames > 30.0)
      target_frametime = 33.333333f;

    // Menus: 60 FPS
    else
      target_frametime = 16.666667f;
  }


  snprintf
        ( szAvg,
            511,
              u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
              u8"    Extreme frame times:     %6.3f min, %6.3f max\n\n\n\n"
              u8"Variation:  %8.5f ms        %.1f FPS  ±  %3.1f frames",
                sum / frames,
                  target_frametime,
                    min, max, (double)max - (double)min,
                      1000.0f / (sum / frames),
                        ((double)max-(double)min)/(1000.0f/(sum/frames)) );

  ImGui::PushStyleColor ( ImGuiCol_PlotLines,
                            (ImVec4&&)ImColor::HSV ( 0.31f - 0.31f *
                     std::min ( 1.0f, (max - min) / (2.0f * target_frametime) ),
                                             0.86f,
                                               0.95f ) );

  const ImVec2 border_dims (
    std::max (500.0f, ImGui::GetContentRegionAvailWidth ()),
      font_size * 7.0f
  );

  ImGui::PlotLines ( SK_ImGui_Visible ? "###ControlPanel_FramePacing" :
                                        "###Floating_FramePacing",
                       SK_ImGui_Frames->getValues     ().data (),
      static_cast <int> (frames),
                           SK_ImGui_Frames->getOffset (),
                             szAvg,
                               -.1f,
                                 2.0f * target_frametime + 0.1f,
                                   border_dims );

  //SK_RenderBackend& rb =
  //  SK_GetCurrentRenderBackend ();
  //
  //if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
  //{
  //  if (rb.gsync_state.capable)
  //  {
  //    ImGui::SameLine ();
  //    ImGui::TextColored (ImColor::HSV (0.226537f, 1.0f, 0.36f), "G-Sync: ");
  //    ImGui::SameLine ();
  //
  //    if (rb.gsync_state.active)
  //    {
  //      ImGui::TextColored (ImColor::HSV (0.226537f, 1.0f, 0.45f), "Active");
  //    }
  //
  //    else
  //    {
  //      ImGui::TextColored (ImColor::HSV (0.226537f, 0.75f, 0.27f), "Inactive");
  //    }
  //  }
  //}

  ImGui::PopStyleColor ();
}

__declspec (dllexport)
bool
SK_ImGui_ControlPanel (void)
{
  if (! imgui_staged_frames)
    return false;

  ImGuiIO& io (ImGui::GetIO ());

  if (ImGui::GetFont () == nullptr)
  {
    dll_log->Log (L"[   ImGui   ]  Fatal Error:  No Font Loaded!");
    return false;
  }


  bool windowed =
    (! SK_GetCurrentRenderBackend ().fullscreen_exclusive);

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
        //ImGui::TextWrapped ("%ws", SK_NvAPI_GetGPUInfoStr ().c_str ());
        ImGui::MenuItem    ("Display G-Sync Status",     "", &config.apis.NvAPI.gsync_status);
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
          ShellExecuteW (GetActiveWindow (), L"explore", SK_GetHostPath (), nullptr, nullptr, SW_NORMAL);
        }

#if 0
        if (SK::SteamAPI::AppID () != 0 && SK::SteamAPI::GetDataDir () != "")
        {
          if (ImGui::BeginMenu ("Browse Steam Directories"))
          {
            if (ImGui::MenuItem ("Cloud Data", ""))
            {
              ShellExecuteA ( GetActiveWindow (), "explore",
                                SK::SteamAPI::GetDataDir ().c_str (),
                                  nullptr, nullptr, SW_NORMAL );
            }

            if (ImGui::MenuItem ("Cloud Config", ""))
            {
              ShellExecuteA ( GetActiveWindow (), "explore",
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
            std::wstring (SK_GetConfigPath ()) + std::wstring (LR"(\logs)");

          ShellExecuteW ( GetActiveWindow (), L"explore", log_dir.c_str (),
                            nullptr, nullptr, SW_NORMAL );
        }


        static SK_RenderBackend& rb =
          SK_GetCurrentRenderBackend ();

        bool supports_texture_mods =
        //( static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9)  ) ||
          ( static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11) );

        if (supports_texture_mods)
        {
          extern SK_LazyGlobal <std::wstring> SK_D3D11_res_root;

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
              wchar_t      wszPath [MAX_PATH * 2] = { };
              wcscpy      (wszPath, SK_D3D11_res_root->c_str ());
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
            std::wstring (SK_GetConfigPath ()) + std::wstring (LR"(\ReShade)");

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

          ShellExecuteW ( GetActiveWindow (), L"explore",
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

        if ( (! SK::SteamAPI::AppID ())          &&
             (! config.steam.force_load_steamapi) )
        {
          ImGui::Separator ();

          if (ImGui::MenuItem ("Manually Inject SteamAPI", ""))
          {
            config.steam.force_load_steamapi = true;
            SK_Steam_KickStart  ();

            if (SK_GetFramesDrawn () > 1)
              SK_ApplyQueuedHooks ();
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


      static auto& rb =
        SK_GetCurrentRenderBackend ();

      auto HDRMenu =
      [&](void)
      {
        float imgui_nits =
          rb.ui_luminance / 1.0_Nits;

        if ( ImGui::SliderFloat ( "Special K Luminance###IMGUI_LUMINANCE",
                                   &imgui_nits,
                                    80.0f, rb.display_gamut.maxLocalY,
                                      u8"%.1f cd/m²" ) )
        {
          rb.ui_luminance =
               imgui_nits * 1.0_Nits;

          SK_SaveConfig ();
        }

#define STEAM_OVERLAY_VS_CRC32C 0xf48cf597
#define UPLAY_OVERLAY_PS_CRC32C 0x35ae281c

        static bool steam_overlay = false;
        static long first_try     = SK_GetFramesDrawn ();

        if ((! steam_overlay) && ((SK_GetFramesDrawn () - first_try) < 240))
        {      steam_overlay =
          SK_D3D11_IsShaderLoaded <ID3D11VertexShader> (
            STEAM_OVERLAY_VS_CRC32C
          );
        }

        if (steam_overlay)
        {
          float steam_nits =
            config.steam.overlay_hdr_luminance / 1.0_Nits;

          if ( ImGui::SliderFloat ( "Steam Overlay Luminance###STEAM_LUMINANCE",
                                     &steam_nits,
                                      80.0f, rb.display_gamut.maxY,
                                        u8"%.1f cd/m²" ) )
          {
            config.steam.overlay_hdr_luminance =
                                    steam_nits * 1.0_Nits;

            SK_SaveConfig ();
          }
        }

        static bool uplay_overlay = false;

        if ((! uplay_overlay) && ((SK_GetFramesDrawn () - first_try) < 240))
        {      uplay_overlay =
          SK_D3D11_IsShaderLoaded <ID3D11PixelShader> (
            UPLAY_OVERLAY_PS_CRC32C
          );
        }

        if (uplay_overlay)
        {
          float uplay_nits =
            config.uplay.overlay_luminance / 1.0_Nits;

          if ( ImGui::SliderFloat ( "uPlay Overlay Luminance###UPLAY_LUMINANCE",
                                     &uplay_nits,
                                      80.0f, rb.display_gamut.maxY,
                                        u8"%.1f cd/m²" ) )
          {
            config.uplay.overlay_luminance =
                                    uplay_nits * 1.0_Nits;

            SK_SaveConfig ();
          }
        }

        ImGui::Separator ();

        bool hdr_changed = false;

        if (config.steam.screenshots.enable_hook)
        {
          hdr_changed =
            ImGui::Checkbox ( "Keep Full-Range JPEG-XR HDR Screenshots (.JXR)",
                                &config.steam.screenshots.png_compress );
        }

        if ( ( screenshot_manager != nullptr &&
               screenshot_manager->getExternalScreenshotRepository ().files > 0 ) )
        {
          const SK_Steam_ScreenshotManager::screenshot_repository_s& repo =
            screenshot_manager->getExternalScreenshotRepository (hdr_changed);

          ImGui::BeginGroup (  );
          ImGui::TreePush   ("");
          ImGui::Text ( "%lu files using %ws",
                          repo.files,
                            SK_File_SizeToString (repo.liSize.QuadPart).c_str  ()
                      );

          ImGui::SameLine ();

          if (ImGui::Button ("Browse"))
          {
            ShellExecuteW ( GetActiveWindow (),
              L"explore",
                screenshot_manager->getExternalScreenshotPath (),
                  nullptr, nullptr,
                        SW_NORMAL
            );
          }

          ImGui::TreePop  ();
          ImGui::EndGroup ();
        }

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
          (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR) )
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



      if (ImGui::BeginMenu ("Help"))
      {
        bool selected = false;

        //ImGui::MenuItem ("Special K Documentation (Work in Progress)", "Ctrl + Shift + Nul", &selected, false);

        ImGui::TreePush ("");
        {
          if (ImGui::MenuItem (R"("Kaldaien's Mod")", "Steam Group", &selected, true))
            SK_SteamOverlay_GoToURL ("http://steamcommunity.com/groups/SpecialK_Mods", true);
        }
        ImGui::TreePop ();

        ImGui::Separator ();

        if ( ImGui::MenuItem ( "View Release Notes",
                                 SK_WideCharToUTF8 (current_branch.release.title).c_str (),
                                   &selected
                             )
           )
        {
          SK_SteamOverlay_GoToURL (
            SK_WideCharToUTF8 (current_branch.release.notes).c_str (),
              true
          );
        }

        if (ImGui::MenuItem ("About this Software...", "", &selected))
          eula.show = true;

        ImGui::Separator (  );
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
              ImGui::GetIO ().Fonts->Fonts [SK_IMGUI_FIXED_FONT]
            );
            //ImGui::BulletText   ("%lu injections since restart", count);

            std::vector <SK_InjectionRecord_s*> records (
              ReadAcquire (&SK_InjectionRecord_s::count)
            );

            ImGui::BeginGroup ();
            for ( unsigned int i = 0 ; i < records.capacity () ; i++ )
            {
              records [i] = SK_Inject_GetRecord (i);
              ImGui::BulletText ("");
            }
            ImGui::EndGroup   ();
            ImGui::SameLine   ();
            ImGui::BeginGroup ();
            for ( const auto& it : records )
            {
              ImGui::Text ( " pid %04u: ", it->process.id );
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
                  it->render.api  = SK_GetCurrentRenderBackend ().api;

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
    ImVec2 (250, 75), ImVec2 ( 0.9f * io.DisplaySize.x,
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

  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

          char szAPIName [32] = { };
    snprintf ( szAPIName, 32, "%ws",  rb.name );

    // Translation layers (D3D8->11 / DDraw->11 / D3D11On12)
    auto api_mask = static_cast <int> (rb.api);

    if ( (api_mask &  static_cast <int> (SK_RenderAPI::D3D12))      != 0x0 &&
          api_mask != static_cast <int> (SK_RenderAPI::D3D12) )
    {
      lstrcatA (szAPIName,   "On12");
    }

    else if ( (api_mask &  static_cast <int> (SK_RenderAPI::D3D11)) != 0x0 &&
               api_mask != static_cast <int> (SK_RenderAPI::D3D11) )
    {
      lstrcatA (szAPIName, u8"→11" );
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
                   (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR);
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

    snprintf ( szResolution, 63, "   %lix%li",
                                   client.right - client.left,
                                     client.bottom - client.top );

    if (windowed)
    {
      if (ImGui::MenuItem (" Window Resolution     ", szResolution))
      {
        config.window.res.override.x = client.right  - client.left;
        config.window.res.override.y = client.bottom - client.top;

        override = true;
      }
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
        config.window.res.override.x = client.right  - client.left;
        config.window.res.override.y = client.bottom - client.top;

        override = true;
      }
    }

    if (ImGui::IsItemHovered ())
    {
      if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9))
      {
        CComQIPtr <IDirect3DDevice9> pDev9 (rb.device);

        if (pDev9 != nullptr)
        {
          CComQIPtr <IDirect3DSwapChain9> pSwap9 (rb.swapchain);

          if (pSwap9 != nullptr)
          {
            SK_ImGui_SummarizeD3D9Swapchain (pSwap9);
          }
        }
      }

      else if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11))
      {
        CComQIPtr <IDXGISwapChain> pSwapDXGI (rb.swapchain);

        if (pSwapDXGI != nullptr)
        {
          SK_ImGui_SummarizeDXGISwapchain (pSwapDXGI);
        }
      }
    }

    int device_x =
      rb.windows.device.getDevCaps ().res.x;
    int device_y =
      rb.windows.device.getDevCaps ().res.y;

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


    if (override)
      SK_ImGui_AdjustCursor ();


    ImGui::Columns   ( 1 );
    ImGui::Separator (   );

    static bool has_own_scale = (hModTBFix);

    if ((! has_own_scale) && ImGui::CollapsingHeader ("UI Render Settings"))
    {
      ImGui::TreePush    ("");

      extern bool __SK_ImGui_D3D11_DrawDeferred;

      ImGui::Checkbox ( "Use D3D11 Deferred ImGui CmdList Execution",
                          &__SK_ImGui_D3D11_DrawDeferred );

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
        ImGui::SetTooltip ("Resolves flickering in some DirectDraw and Direct3D 8 / 9 games");

      ImGui::TextUnformatted ("Anti-Aliasing:  ");                                          ImGui::SameLine ();
      ImGui::Checkbox        ("Lines",             &config.imgui.render.antialias_lines);   ImGui::SameLine ();
      ImGui::Checkbox        ("Contours",          &config.imgui.render.antialias_contours);

      ImGui::TreePop     ();
    }


    SK_PlugIn_ControlPanelWidget ();


#ifdef _WIN64
    switch (SK_GetCurrentGameID ())
    {
      case SK_GAME_ID::Okami:
      {
        extern bool SK_Okami_PlugInCfg (void);
                    SK_Okami_PlugInCfg ();
      } break;

      case SK_GAME_ID::GalGun_Double_Peace:
      {
        extern bool SK_GalGun_PlugInCfg (void);
                    SK_GalGun_PlugInCfg ();
      } break;

      case SK_GAME_ID::StarOcean4:
      {
        extern bool SK_SO4_PlugInCfg (void);
                    SK_SO4_PlugInCfg ();
      } break;

      case SK_GAME_ID::LifeIsStrange_BeforeTheStorm:
      {
        extern bool SK_LSBTS_PlugInCfg (void);
                    SK_LSBTS_PlugInCfg ();
      } break;

      case SK_GAME_ID::FinalFantasyXV:
      {
        extern bool SK_FFXV_PlugInCfg (void);
                    SK_FFXV_PlugInCfg ();
      } break;

      case SK_GAME_ID::NiNoKuni2:
      {
        extern bool SK_NNK2_PlugInCfg (void);
                    SK_NNK2_PlugInCfg ();
      } break;

      case SK_GAME_ID::PillarsOfEternity2:
      {
        extern bool SK_POE2_PlugInCfg (void);
                    SK_POE2_PlugInCfg ();
      } break;

      case SK_GAME_ID::Yakuza0:
      case SK_GAME_ID::YakuzaKiwami2:
      {
        SK_Yakuza0_PlugInCfg ();
      } break;

      case SK_GAME_ID::MonsterHunterWorld:
      {
        extern bool SK_MHW_PlugInCfg (void);
                    SK_MHW_PlugInCfg ();
      } break;

      case SK_GAME_ID::DragonQuestXI:
      {
        extern bool SK_DQXI_PlugInCfg (void);
                    SK_DQXI_PlugInCfg ();
      } break;

      case SK_GAME_ID::Shenmue:
      {
        extern bool SK_SM_PlugInCfg (void);
                    SK_SM_PlugInCfg ();
      } break;

      case SK_GAME_ID::AssassinsCreed_Odyssey:
      {
        extern bool SK_ACO_PlugInCfg (void);
                    SK_ACO_PlugInCfg ();
      } break;

      case SK_GAME_ID::Tales_of_Vesperia:
      {
        extern bool SK_TVFix_PlugInCfg (void);
                    SK_TVFix_PlugInCfg ();
      } break;

      case SK_GAME_ID::Sekiro:
      {
        extern bool SK_Sekiro_PlugInCfg (void);
                    SK_Sekiro_PlugInCfg ();
      }
    };
#endif


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
              __target_fps = 60.0f;
        }

        bool bg_limit =
              ( limit &&
          ( __target_fps_bg != 0.0f ) );

        if (limit)
        {
          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip (
              "Graph color represents frame time variance, not proximity"
              " to your target FPS." );
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
                SK_GetCurrentRenderBackend ().windows.unity;

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
                                             "60.000 fps (No Preference)" )
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
                             (game_window.active || (! bg_limit)) );

        if (limit)
        {
          if (advanced && bg_limit)
          {
            _LimitSlider (
              __target_fps_bg, "###Background_FPS",
                                  "BackgroundFPS", (! game_window.active)
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
          ImGui::BeginGroup ();

          if ( ImGui::Checkbox ( "Use Multimedia Class Scheduling",
                                   &config.render.framerate.enable_mmcss ) )
          {
            SK_DWM_EnableMMCSS (config.render.framerate.enable_mmcss);
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Minimizes dropped frames when combined with FPS limiting.");
            ImGui::Separator    ();
            ImGui::BulletText   ("Keep this option enabled unless troubleshooting something");
            ImGui::EndTooltip   ();
          }

          ImGui::SameLine (); ImGui::Spacing (); ImGui::SameLine ();
                              ImGui::Spacing ();
          ImGui::SameLine (); ImGui::Spacing (); ImGui::SameLine ();

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

          ImGui::SameLine (); ImGui::Spacing (); ImGui::SameLine ();
                              ImGui::Spacing ();
          ImGui::SameLine (); ImGui::Spacing (); ImGui::SameLine ();

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

          ImGui::BeginGroup ();
          if (__target_fps > 0.0f)
          {
            //if (ImGui::SliderInt   ( "Maximum CPU Render-Ahead",
            //                           &config.render.framerate.max_render_ahead,
            //                             0, 2,
            //                         config.render.framerate.max_render_ahead != 1 ?
            //                           "%.0f Frames" : "%.0f Frame" )
            //   )
            //{
            //  SK::Framerate::GetLimiter ()->init (
            //    SK::Framerate::GetLimiter ()->get_limit ()
            //  );
            //}
            //
            //if (ImGui::IsItemHovered ())
            //{
            //  ImGui::BeginTooltip   (  );
            //  ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f));
            //  ImGui::Text           ("Controls How Many Frames CPU May Prepare In Advance");
            //  ImGui::Separator      (  );
            //  ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.75f, 0.75f, 0.75f, 1.0f));
            //  ImGui::Text           ("  Lower = Stricter Adherence to Framerate Limit + Lower Input Latency");
            //  ImGui::PopStyleColor  ( 2);
            //  ImGui::BulletText     ("0 is reasonable if your GPU consistently draws at or above the target rate");
            //  ImGui::TreePush       ("");
            //  ImGui::BulletText     ("Consider 1 or 2 to help with stuttering otherwise");
            //  ImGui::TreePop        (  );
            //  ImGui::EndTooltip     (  );
            //}
            //  ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.75f, 0.75f, 0.75f, 1.0f));
            //  ImGui::Text           ("  Lower = Stricter, but setting");
            //  ImGui::SameLine       ();

            ImGui::SliderFloat ( "Target Framerate Tolerance", &config.render.framerate.limiter_tolerance, 1.0f, 24.0f);

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip   ();
              ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.75f, 0.25f, 1.0f));
              ImGui::Text           ("Controls Framerate Smoothness\n\n");
              ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.0f));
              ImGui::Text           ("  Lower = Stricter, but setting");
              ImGui::SameLine       ();
              ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 1.0f, 0.65f, 1.0f));
              ImGui::Text           ("too low");
              ImGui::SameLine       ();
              ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.0f));
              ImGui::Text           ("will cause framerate instability...");
              ImGui::PopStyleColor (4);
              ImGui::Separator      ( );
              ImGui::Text           ("Recomputes clock phase if a single frame takes longer than <1.0 + tolerance> x <target_ms> to complete");
              ImGui::BulletText     ("Adjust this if your set limit is fighting with VSYNC (frequent frametime graph oscillations).");
              ImGui::EndTooltip     ( );
            }

            SK_ComQIPtr <ID3D11Device> pDev     (rb.device);
            SK_ComQIPtr <IDXGIDevice>  pDXGIDev (pDev);

            if (pDXGIDev != nullptr)
            {
              static INT nPrio  = -8;
              if        (nPrio == -8) pDXGIDev->GetGPUThreadPriority (&nPrio);

              if (ImGui::SliderInt ("GPU Priority", &nPrio, -7, 7))
              {
                if (SUCCEEDED (pDXGIDev->SetGPUThreadPriority ( nPrio)))
                               pDXGIDev->GetGPUThreadPriority (&nPrio);
              }
            }
          }
          ImGui::EndGroup  ();
          ImGui::Separator ();

          bool spoof =
            ( config.render.framerate.override_num_cpus != -1 );

          static SYSTEM_INFO             si = { };
          SK_RunOnce (SK_GetSystemInfo (&si));

          ImGui::BeginGroup    ();
          if ( ImGui::Checkbox ("Spoof CPU Core Count", &spoof) )
          {
            config.render.framerate.override_num_cpus =
              ( spoof ? si.dwNumberOfProcessors : -1 );
          }

          if (spoof)
          {
            ImGui::SameLine    (                            );
            ImGui::SliderInt   ( "###SPOOF_CPU_COUNT",
                                 &config.render.framerate.override_num_cpus,
                                   1, si.dwNumberOfProcessors,
                                     "Number of CPUs: %d" );
          }
          ImGui::EndGroup      ();
        }
      }
    }

    ImGui::PopItemWidth ();
  }

  SK::ControlPanel::render_api = rb.api;

  SK::ControlPanel::D3D9::Draw   ();
  SK::ControlPanel::D3D11::Draw  ();
  SK::ControlPanel::OpenGL::Draw ();

  SK::ControlPanel::Compatibility::Draw ();

  SK::ControlPanel::Input::Draw  ();
  SK::ControlPanel::Window::Draw ();
  SK::ControlPanel::Sound::Draw  ();

  SK::ControlPanel::OSD::Draw     ();

  bool open_widgets = ImGui::CollapsingHeader ("Widgets");

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
    bool gpumon        = SK_ImGui_Widgets->gpu_monitor->isVisible     ();
    bool volumecontrol = SK_ImGui_Widgets->volume_control->isVisible  ();
    bool cpumon        = SK_ImGui_Widgets->cpu_monitor->isVisible     ();
    bool pipeline      = SK_ImGui_Widgets->d3d11_pipeline->isVisible  ();
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

    if ( (int)render_api & (int)SK_RenderAPI::D3D11 )
    {
      ImGui::SameLine ();
      ImGui::Checkbox ("Texture Cache", &SK_ImGui_Widgets->texcache);

      ImGui::SameLine ();
      if (ImGui::Checkbox ("Pipeline Stats", &pipeline))
      {
        SK_ImGui_Widgets->d3d11_pipeline->setVisible (pipeline).
                                          setActive  (pipeline);
      }

      ////SK_DXGI_HDRControl* pHDRCtl =
      ////  SK_HDR_GetControl ();
      ////
      ////if (pHDRCtl->meta._AdjustmentCount > 0)
      ////{
      if (rb.isHDRCapable ())
      {
        ImGui::SameLine ();
        if (ImGui::Checkbox ("HDR Display", &hdr))
        {
          SK_ImGui_Widgets->hdr_control->setVisible (hdr).
                                         setActive  (hdr);
        }
      }
    }

    ImGui::SameLine ();

    if (ImGui::Checkbox ("Tobii Eyetracker", &tobii))
    {
      SK_ImGui_Widgets->tobii->setVisible (tobii).
                               setActive  (tobii);
    }

    ImGui::SameLine ();

    if (ImGui::Checkbox ("Threads", &threads))
    {
      SK_ImGui_Widgets->thread_profiler->setVisible (threads).
                                         setActive  (threads);
    }

    ImGui::TreePop  ();
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
  if (SK_SteamAPI_Friends ())
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

void
SK_ImGui_StageNextFrame (void)
{
  static auto& humanToVirtual = humanKeyNameToVirtKeyCode.get ();
  static auto& virtualToHuman = virtKeyCodeToHumanKeyName.get ();

  if (imgui_staged)
    return;


  // Excessively long frames from things like toggling the Steam overlay
  //   must be ignored.
  static ULONG last_frame         = 0;
  bool         skip_frame_history = false;

  if (last_frame < SK_GetFramesDrawn () - 1)
  {
    skip_frame_history = true;
  }

  if (last_frame != SK_GetFramesDrawn ())
  {
    last_frame  = SK_GetFramesDrawn ();

    ImGuiIO& io = ImGui::GetIO ();

    font.size           = ImGui::GetFont () ?
                            ImGui::GetFont ()->FontSize * io.FontGlobalScale :
                             1.0f;

    font.size_multiline = SK::ControlPanel::font.size      +
                          ImGui::GetStyle ().ItemSpacing.y +
                          ImGui::GetStyle ().ItemInnerSpacing.y;
  }

  current_tick = SK_QueryPerf ().QuadPart;
  current_time = timeGetTime  ();

  bool d3d9  = false;
  bool d3d11 = false;
  bool d3d12 = false;
  bool gl    = false;

  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

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

  ImGuiIO& io (
    ImGui::GetIO ()
  );

  // TODO: Generalize this!
  //if ( SK_GetCurrentGameID () == SK_GAME_ID::MonsterHunterWorld ||
  //     SK_GetCurrentGameID () == SK_GAME_ID::DragonQuestXI )
  CComQIPtr <IDXGISwapChain> pSwapChain (
    rb.swapchain
  );
  if (pSwapChain != nullptr)
  {
    DXGI_SWAP_CHAIN_DESC  desc = {};
    pSwapChain->GetDesc (&desc);

    // scRGB
    if ( rb.isHDRCapable ()   &&
        (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR)
                              &&
        desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
    {
    }
  }

  extern volatile
               LONG __SK_ScreenShot_CapturingHUDless;
  if (ReadAcquire (&__SK_ScreenShot_CapturingHUDless))
  {
    imgui_staged = false;
    return;
  }



  //
  // Framerate history
  //
  if (! (reset_frame_history || skip_frame_history))
  {
    SK_ImGui_Frames->timeFrame (io.DeltaTime);
  }

  else if (reset_frame_history) SK_ImGui_Frames->reset ();

  reset_frame_history = false;

  extern void SK_Widget_InitFramePacing    (void);
  extern void SK_Widget_InitThreadProfiler (void);
  extern void SK_Widget_InitVolumeControl  (void);
  extern void SK_Widget_InitGPUMonitor     (void);
  extern void SK_Widget_InitTobii          (void);
  SK_RunOnce (SK_Widget_InitThreadProfiler (    ));
  SK_RunOnce (SK_Widget_InitFramePacing    (    ));
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
              SK_ImGui_Widgets->thread_profiler,
                SK_ImGui_Widgets->hdr_control,
                  SK_ImGui_Widgets->tobii
  };

  if (init_widgets)
  {
    init_widgets = false;

    for (auto& widget : widgets)
      widget->run_base ();
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
    SK_ControlPanel_Activated = true;
    keep_open                 = SK_ImGui_ControlPanel ();
  }


  for (auto& widget : widgets)
  {
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



  static DWORD dwStartTime = current_time;
  if ((current_time < dwStartTime + 1000 * config.version_banner.duration) || eula.show)
  {
    ImGui::PushStyleColor    (ImGuiCol_Text,     ImVec4 (1.f,   1.f,   1.f,   1.f));
    ImGui::PushStyleColor    (ImGuiCol_WindowBg, ImVec4 (.333f, .333f, .333f, 0.828282f));
    ImGui::SetNextWindowPos  (ImVec2 (10, 10));
    ImGui::SetNextWindowSize (ImVec2 ( ImGui::GetIO ().DisplayFramebufferScale.x - 20.0f,
                                       ImGui::GetFrameHeightWithSpacing ()   * 4.5f  ), ImGuiCond_Always );
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
      ImGui::TextUnformatted ("please see the Release Notes, under");                                ImGui::SameLine ();
    }
    else
    {
      ImGui::TextUnformatted ("  Please see the Release Notes, under");                              ImGui::SameLine ();
    }
    ImGui::TextColored       (ImColor::HSV (.52f, 1.f, 1.f),  "Help | Release Notes");               ImGui::SameLine ();
    ImGui::TextUnformatted   ("for what passes as documentation for this project.");

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
      ImGui::TextColored   (ImColor::HSV (.15, 0.9, 1.), "%s",
                            utf8_release_title.c_str());ImGui::SameLine ();
      ImGui::Text          ("from the");                ImGui::SameLine ();
      ImGui::TextColored   (ImColor::HSV (.4, 0.9, 1.), "%s",
                            utf8_branch_name.c_str ()); ImGui::SameLine ();
      ImGui::Text          ("development branch.");
      ImGui::Spacing       ();
      ImGui::Spacing       ();
      ImGui::TreePush      ("");
      ImGui::TextColored   (ImColor::HSV (.08, .85, 1.0), "%s",
                            utf8_time_checked.c_str ());ImGui::SameLine ();
      ImGui::TextColored   (ImColor (1.f, 1.f, 1.f, 1.f), u8"  ※  ");
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
                                    virtualToHuman [VK_CONTROL].c_str (),
                                    virtualToHuman [VK_SHIFT].c_str   (),
                                    virtualToHuman [VK_BACK].c_str    () );
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


      extern LONG SK_D3D11_Resampler_GetActiveJobCount  (void);
      extern LONG SK_D3D11_Resampler_GetWaitingJobCount (void);
      extern LONG SK_D3D11_Resampler_GetErrorCount      (void);

      const LONG jobs =
        ( SK_D3D11_Resampler_GetActiveJobCount  () +
          SK_D3D11_Resampler_GetWaitingJobCount () );

      static DWORD dwLastActive = 0;

      if (jobs != 0 || dwLastActive > current_time - 500)
        extra_lines++;

      if (config.textures.cache.residency_managemnt)
      {
        if (ReadAcquire (&SK_D3D11_TexCacheResidency->count.InVRAM)   > 0) extra_lines++;
        if (ReadAcquire (&SK_D3D11_TexCacheResidency->count.Shared)   > 0) extra_lines++;
        if (ReadAcquire (&SK_D3D11_TexCacheResidency->count.PagedOut) > 0) extra_lines++;
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
      static const char* szConfirm =
        " Confirm Exit? ";
      static const char* szDisclaimer =
        "\n         You will lose any unsaved game progress.      \n\n";

      ImGui::FocusWindow (ImGui::GetCurrentWindow ());
      ImGui::TextColored (ImColor::HSV (0.075f, 1.0f, 1.0f), szDisclaimer);
      ImGui::Separator   ();

      ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f),     szConfirm);

      ImGui::SameLine    (); ImGui::Spacing (); ImGui::SameLine ();

      if (ImGui::Button  ("Okay"))
      {
        SK_SelfDestruct     (   );
        SK_TerminateProcess (0x0);
        ExitProcess         (0x0);
      }

      //ImGui::PushItemWidth (ImGui::GetWindowContentRegionWidth ()*0.33f);
      //ImGui::SameLine (); ImGui::SameLine (); ImGui::PopItemWidth ();

      ImGui::SameLine    ();

      if (ImGui::Button  ("Cancel"))
      {
        SK_ImGui_WantExit  = false;
        SK_ReShade_Visible = false;
        ImGui::CloseCurrentPopup ();
      }

      ImGui::SetItemDefaultFocus ();

      ImGui::SameLine    ();
      ImGui::TextUnformatted (" ");
      ImGui::SameLine    ();

      ImGui::Checkbox    ( "Enable Alt + F4",
                             &config.input.keyboard.catch_alt_f4 );

      ImGui::EndPopup    ();
    }
  }

  if (io.WantSetMousePos)
  {
    SK_ImGui_Cursor.pos.x = static_cast <LONG> (io.MousePos.x);
    SK_ImGui_Cursor.pos.y = static_cast <LONG> (io.MousePos.y);

    POINT screen_pos = SK_ImGui_Cursor.pos;

    HCURSOR hCur = GetCursor ();

    if (hCur != nullptr)
      SK_ImGui_Cursor.orig_img = hCur;

    SK_ImGui_Cursor.LocalToScreen (&screen_pos);
    SK_SetCursorPos ( screen_pos.x,
                      screen_pos.y );

    io.WantCaptureMouse = true;

    SK_ImGui_UpdateCursor ();
  }

  imgui_staged = true;
  imgui_staged_frames++;
}


extern IMGUI_API void ImGui_ImplGL3_RenderDrawData (ImDrawData* draw_data);
extern IMGUI_API void ImGui_ImplDX9_RenderDrawData (ImDrawData* draw_data);
extern IMGUI_API void ImGui_ImplDX11_RenderDrawData(ImDrawData* draw_data);
//#ifdef _WIN64
//extern         void ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data);
//#endif

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

  bool d3d9  = false;
  bool d3d11 = false;
  bool d3d12 = false;
  bool gl    = false;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.api == SK_RenderAPI::OpenGL)
  {
    gl = true;

    ImGui::Render ();
    ImGui_ImplGL3_RenderDrawData (ImGui::GetDrawData ());
  }

  else if ( ( static_cast <int> (rb.api) &
              static_cast <int> (SK_RenderAPI::D3D9) ) != 0 )
  {
    d3d9 = true;

    CComQIPtr <IDirect3DDevice9> pDev (
      rb.device
    );

    if ( SUCCEEDED (
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
    d3d11 = true;

    ImGui::Render ();
    ImGui_ImplDX11_RenderDrawData (ImGui::GetDrawData ());
  }

  else if ( ( static_cast <int> (rb.api) &
              static_cast <int> (SK_RenderAPI::D3D12) ) != 0 )
  {
    d3d12 = true;

    ImGui::Render ();
  }

  else
  {
    SK_LOG0 ( (L"No Render API"), L"Overlay" );
    return 0x0;
  }

  if (! keep_open)
    SK_ImGui_Toggle ();


  POINT             orig_pos;
  SK_GetCursorPos (&orig_pos);

//SK_ImGui_Cursor.update ();

  //// The only reason this might be used is to reset the window's
  ////   class cursor. SK_ImGui_Cursor.update (...) doesn't reposition
  ////                   the cursor.
#define FIXME
#ifndef FIXME
  SK_SetCursorPos (orig_pos.x, orig_pos.y);
#endif

  imgui_finished_frames++;


  imgui_staged = false;

  return 0;
}

__declspec (dllexport)
void
SK_ImGui_Toggle (void)
{
  // XXX: HACK for Monster Hunter: World
  game_window.active = true;

  static ImGuiIO& io (
    ImGui::GetIO ()
  );

  static ULONG last_frame = 0;

  if (last_frame != SK_GetFramesDrawn ())
  {
    current_time  = timeGetTime       ();
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
    io.WantCaptureKeyboard = (! SK_ImGui_Visible);
    io.WantCaptureMouse    = (! SK_ImGui_Visible);


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



#if 0
    modes = "Bordered\0Borderless\0Borderless Fullscreen\0\0";

    if (ImGui::Combo ("Window Style###SubMenu_WindowBorder_Combo", &mode, modes) && mode != orig_mode)
    {
      switch (mode)
      {
        case 0:
          config.window.borderless = false;
          config.window.fullscreen = false;
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

      bool toggle_border     = (config.window.borderless != window_is_borderless);
      bool toggle_fullscreen = (config.window.fullscreen != SK_Window_IsFullscreen (game_window.hWnd));

      extern void
      SK_DeferCommands (const char** szCommands, int count);

      if (toggle_border)     config.window.borderless = window_is_borderless;
      if (toggle_fullscreen) config.window.fullscreen = SK_Window_IsFullscreen (game_window.hWnd);

      SK_ImGui_AdjustCursor ();

      if (toggle_border && toggle_fullscreen)
      {
        if (config.window.fullscreen)
        {
          const char* cmds [2] = { "Window.Fullscreen toggle",
                                   "Window.Borderless toggle" };

          SK_DeferCommands ( cmds, 2 );
        }

        else
        {
          const char* cmds [2] = { "Window.Borderless toggle",
                                   "Window.Fullscreen toggle" };

          SK_DeferCommands ( cmds, 2 );
        }
      }

      else
      {
        if (toggle_border)     { SK_DeferCommand ("Window.Borderless toggle"); }
        if (toggle_fullscreen) { SK_DeferCommand ("Window.Fullscreen toggle"); }
      }
#endif