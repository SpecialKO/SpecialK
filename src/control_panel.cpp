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

#include <imgui/imgui.h>
#include <imgui/backends/imgui_gl3.h>
#include <imgui/backends/imgui_d3d9.h>
#include <imgui/backends/imgui_d3d11.h>

#include <SpecialK/render/backend.h>

#include <SpecialK/widgets/widget.h>

#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/DLL_VERSION.H>
#include <SpecialK/command.h>
#include <SpecialK/console.h>
#include <SpecialK/utility.h>

#include <SpecialK/window.h>
#include <SpecialK/log.h>
#include <SpecialK/steam_api.h>

#include <SpecialK/update/version.h>
#include <SpecialK/update/network.h>
#include <SpecialK/framerate.h>

#include <SpecialK/nvapi.h>
#include <SpecialK/ini.h>
#include <SpecialK/import.h>
#include <SpecialK/thread.h>

#include <SpecialK/input/xinput.h>
#include <SpecialK/injection/injection.h>

#include <SpecialK/performance/io_monitor.h>
#include <SpecialK/plugin/reshade.h>

#include <concurrent_queue.h>

#include <windows.h>
#include <cstdio>

#include "resource.h"

#include <SpecialK/control_panel.h>

#include <SpecialK/control_panel/osd.h>
#include <SpecialK/control_panel/d3d9.h>
#include <SpecialK/control_panel/d3d11.h>
#include <SpecialK/control_panel/opengl.h>
#include <SpecialK/control_panel/steam.h>
#include <SpecialK/control_panel/input.h>
#include <SpecialK/control_panel/window.h>
#include <SpecialK/control_panel/sound.h>
#include <SpecialK/control_panel/plugins.h>
#include <SpecialK/control_panel/compatibility.h>

LONG imgui_staged_frames   = 0;
LONG imgui_finished_frames = 0;
BOOL imgui_staged          = FALSE;

using namespace SK::ControlPanel;

SK_RenderAPI                 SK::ControlPanel::render_api;
unsigned long                SK::ControlPanel::current_time;
SK::ControlPanel::font_cfg_s SK::ControlPanel::font;


bool __imgui_alpha = true;

__declspec (dllexport)
void
SK_ImGui_Toggle (void);

using SetCursorPos_pfn = BOOL (WINAPI *)
(
  _In_ int X,
  _In_ int Y
);

extern SetCursorPos_pfn  SetCursorPos_Original;

extern uint32_t __stdcall SK_Steam_PiratesAhoy (void);
extern uint32_t __stdcall SK_SteamAPI_AppID    (void);

extern bool     __stdcall SK_FAR_IsPlugIn      (void);
extern void     __stdcall SK_FAR_ControlPanel  (void);

       bool               SK_DXGI_FullStateCache = false;

extern GetCursorInfo_pfn GetCursorInfo_Original;

std::wstring
SK_NvAPI_GetGPUInfoStr (void);

void
LoadFileInResource ( int          name,
                     int          type,
                     DWORD&       size,
                     const char*& data )
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
      size =
        SizeofResource (handle, rc);
    
      data =
        static_cast <const char *>(
          LockResource (rcData)
        );
    }
  } 
}

extern void __stdcall SK_ImGui_DrawEULA (LPVOID reserved);
extern bool           SK_ImGui_Visible;
       bool           SK_ReShade_Visible        = false;
       bool           SK_ControlPanel_Activated = false;

extern void ImGui_ToggleCursor (void);

struct show_eula_s {
  bool show;
  bool never_show_again;
} extern eula;

#include <map>
#include <unordered_set>
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
    const ImFont::Glyph *glyph     = nullptr;
          char           c         = 0;
          bool           ret       = false;
    const ImGuiContext&  g         = *GImGui;
    const ImGuiStyle&    style     = g.Style;
          float          pad       = style.FramePadding.x;
          ImVec4         color     = { };
    const char*          hash_mark = strstr (text, "##");
          ImVec2         text_size = ImGui::CalcTextSize     (text, hash_mark);
          ImGuiWindow*   window    = ImGui::GetCurrentWindow ();
          ImVec2         pos       = ImVec2 ( window->DC.CursorPos.x + pad,
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

      pos.y -= glyph->XAdvance;

      // Do not print embedded hash codes
      if (hash_mark != nullptr && text >= hash_mark)
        break;
    }

    ImGui::PopID ();

    if (ret)
      *v ^= 1;

    return ret;
  }
#pragma warning (pop)

  // Should return true when clicked, this is not consistent with
  //   the rest of the ImGui API.
  bool BatteryMeter (void)
  {
    if (! SK::SteamAPI::AppID ())
      return false;

    ISteamUtils* utils =
      SK_SteamAPI_Utils ();

    if (utils)
    {
      const uint8_t battery_level =
        utils->GetCurrentBatteryPower ();
      
      if (battery_level != 255) // 255 = Running on AC
      {
        const float battery_ratio = (float)battery_level/100.0f;

        static char szBatteryLevel [128] = { };
        snprintf (szBatteryLevel, 127, "%hhu%% Battery Charge Remaining", battery_level);

        ImGui::PushStyleColor (ImGuiCol_PlotHistogram,  ImColor::HSV (battery_ratio * 0.278f, 0.88f, 0.666f));
        ImGui::PushStyleColor (ImGuiCol_Text,           ImColor (255, 255, 255));
        ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor ( 0.3f,  0.3f,  0.3f, 0.7f));
        ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor ( 0.6f,  0.6f,  0.6f, 0.8f));
        ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor ( 0.9f,  0.9f,  0.9f, 0.9f));

        ImGui::ProgressBar ( battery_ratio,
                               ImVec2 (-1, 0),
                                 szBatteryLevel );

        ImGui::PopStyleColor  (5);

        return true;
      }
    }

    return false;
  }
} // namespace SK_ImGui


struct sk_imgui_nav_state_s {
  bool nav_usable   = false;
  bool io_NavUsable = false;
  bool io_NavActive = false;
};

#include <stack>
std::stack <sk_imgui_nav_state_s> SK_ImGui_NavStack;

void
SK_ImGui_PushNav (bool enable)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  SK_ImGui_NavStack.push ( { nav_usable, io.NavUsable, io.NavActive } );

  if (enable)
  {
    nav_usable = true; io.NavUsable = true; io.NavActive = true;
  }
}

void
SK_ImGui_PopNav (void)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  // Underflow?
  if (SK_ImGui_NavStack.empty ()) {
    dll_log.Log (L"ImGui Nav State Underflow");
    return;
  }

  nav_usable   = SK_ImGui_NavStack.top ().nav_usable;
  io.NavActive = SK_ImGui_NavStack.top ().io_NavActive;
  io.NavUsable = SK_ImGui_NavStack.top ().io_NavUsable;
  
  SK_ImGui_NavStack.pop ();
}



concurrency::concurrent_queue <std::wstring> SK_ImGui_Warnings;

void
SK_ImGui_Warning (const wchar_t* wszMessage)
{
  SK_ImGui_Warnings.push (wszMessage);
}

void
SK_ImGui_ProcessWarnings (void)
{
  static std::wstring warning_msg = L"";

  if (! SK_ImGui_Warnings.empty ())
  {
    if (warning_msg.empty ())
      SK_ImGui_Warnings.try_pop (warning_msg);
  }

  if (warning_msg.empty ())
    return;

  ImGuiIO& io = ImGui::GetIO ();

  // Stupid hack to show ImGui windows without the control panel open
  SK_ReShade_Visible = true;

  ImGui::SetNextWindowPosCenter       (ImGuiSetCond_Always);
  ImGui::SetNextWindowSizeConstraints ( ImVec2 (360.0f, 40.0f), ImVec2 ( 0.925f * io.DisplaySize.x,
                                                                         0.925f * io.DisplaySize.y ) );

  ImGui::OpenPopup ("Special K Warning");


  if ( ImGui::BeginPopupModal ( "Special K Warning",
                                  nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                    ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse )
     )
  {
    ImGui::FocusWindow (ImGui::GetCurrentWindow ());

    ImGui::TextColored ( ImColor::HSV (0.075f, 1.0f, 1.0f), "\n         %ws         \n\n", warning_msg.c_str ());

    ImGui::Separator ();

    ImGui::TextColored ( ImColor::HSV (0.15f, 1.0f, 1.0f), "   " );

    ImGui::SameLine ();

    ImGui::Spacing (); ImGui::SameLine ();

    if (ImGui::Button ("Okay"))
    {
      SK_ReShade_Visible = false;
      warning_msg.clear ();

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
  ImGuiIO& io = ImGui::GetIO ();

  SK_ImGui_WantExit = true;

  ImGui::SetNextWindowPosCenter       (ImGuiSetCond_Always);
  ImGui::SetNextWindowSizeConstraints ( ImVec2 (360.0f, 40.0f), ImVec2 ( 0.925f * io.DisplaySize.x,
                                                                         0.925f * io.DisplaySize.y ) );

  ImGui::OpenPopup ("Confirm Forced Software Termination");
}

bool
SK_ImGui_IsItemClicked (void)
{
  if (ImGui::IsItemClicked ())
    return true;

  if (ImGui::IsItemHovered ())
  {
    if (ImGui::GetIO ().NavInputsDownDuration [ImGuiNavInput_PadActivate] == 0.0f)
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
    ImGuiIO& io = ImGui::GetIO ();

    if (io.NavInputsDownDuration [ImGuiNavInput_PadActivate] > 0.4f)
    {
      io.NavInputsDownDuration [ImGuiNavInput_PadActivate] = 0.0f;
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
    if (ImGui::IsAnyWindowHovered () && io.MouseClicked [1])
      return true;

    if (ImGui::IsWindowFocused () && io.MouseDoubleClicked [4])
    {
      return true;
    }
  }

  return false;
};


ImVec2 SK_ImGui_LastWindowCenter  (-1.0f, -1.0f);

void SK_ImGui_CenterCursorAtPos (ImVec2 center = SK_ImGui_LastWindowCenter);
void SK_ImGui_UpdateCursor      (void);

const char*
SK_ImGui_ControlPanelTitle (void)
{
  static char szTitle [512] = { };
  const  bool steam         = (SK::SteamAPI::AppID () != 0x0);

  extern volatile LONGLONG SK_SteamAPI_CallbackRunCount;

  {
    // TEMP HACK
    static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");

    std::wstring title = L"";

    extern std::wstring __stdcall SK_GetPluginName (void);
    extern bool         __stdcall SK_HasPlugin     (void);

    if (SK_HasPlugin () && hModTBFix == nullptr)
      title += SK_GetPluginName ();
    else
      title += L"Special K  (v " SK_VERSION_STR_W L")";

    title += L"  Control Panel";

    extern __time64_t __SK_DLL_AttachTime;

    __time64_t now     = 0ULL;
     _time64 (&now);

    auto       elapsed = static_cast <uint32_t> (now - __SK_DLL_AttachTime);

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
  CreateThread ( nullptr, 0,
    [](LPVOID)->
    DWORD
    {
      SetCurrentThreadDescription (L"[SK] Cursor Adjustment Thread");

      ClipCursor_Original (nullptr);
        SK_AdjustWindow   ();        // Restore game's clip cursor behavior

      CloseHandle (GetCurrentThread ());

      return 0;
    }, nullptr, 0x00, nullptr );
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

SK_ImGui_FrameHistory SK_ImGui_Frames;


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
  SK_RenderBackend& rb =
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
          if (can_go_full)
            config.display.force_fullscreen = force;
          config.display.force_windowed     = false;
          break;
      }
    }
  }


  if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11))
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
        ImGui::BeginTooltip ();
        ImGui::Text         ("Override Scaling Mode");
        ImGui::Separator    ();
        ImGui::BulletText   ("Set to Unspecified to CORRECTLY run Fullscreen Display Native Resolution");
        ImGui::EndTooltip   ();
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

extern float target_fps;

void
SK_ImGui_DrawGraph_FramePacing (void)
{
  const ImGuiIO& io (ImGui::GetIO ());

  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  float sum = 0.0f;

  float min = FLT_MAX;
  float max = 0.0f;

  for ( const auto& val : SK_ImGui_Frames.getValues () )
  {
    sum += val;

    if (val > max)
      max = val;

    if (val < min)
      min = val;
  }

  static       char szAvg [512] = { };
  static const bool ffx = GetModuleHandle (L"UnX.dll") != nullptr;

  float target_frametime = ( target_fps == 0.0f ) ?
                              ( 1000.0f / (ffx ? 30.0f : 60.0f) ) :
                                ( 1000.0f / fabs (target_fps) );

  float frames = std::min ( (float)SK_ImGui_Frames.getUpdates  (),
                            (float)SK_ImGui_Frames.getCapacity () );


  if (ffx)
  {
    // Normal Gameplay: 30 FPS
    if (sum / frames > 30.0)
      target_frametime = 33.333333f;

    // Menus: 60 FPS
    else 
      target_frametime = 16.666667f;
  }


  sprintf_s
        ( szAvg,
            512,
              u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
              u8"    Extreme frame times:     %6.3f min, %6.3f max\n\n\n\n"
              u8"Variation:  %8.5f ms        %.1f FPS  ±  %3.1f frames",
                sum / frames,
                  target_frametime,
                    min, max, max - min,
                      1000.0f / (sum / frames), (max - min) / (1000.0f / (sum / frames)) );

  ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                            ImColor::HSV ( 0.31f - 0.31f *
                     std::min ( 1.0f, (max - min) / (2.0f * target_frametime) ),
                                             0.86f,
                                               0.95f ) );

  ImGui::PlotLines ( SK_ImGui_Visible ? "###ControlPanel_FramePacing" : "###Floating_FramePacing",
                       SK_ImGui_Frames.getValues     ().data (),
      static_cast <int> (frames),
                           SK_ImGui_Frames.getOffset (),
                             szAvg,
                               -.1f,
                                 2.0f * target_frametime + 0.1f,
                                   ImVec2 (
                                     std::max (500.0f, ImGui::GetContentRegionAvailWidth ()), font_size * 7.0f) );

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
    dll_log.Log (L"[   ImGui   ]  Fatal Error:  No Font Loaded!");
    return false;
  }


  bool windowed = (! SK_GetCurrentRenderBackend ().fullscreen_exclusive);

  auto DisplayMenu =
    [&](void)
    {
      //if (ImGui::MenuItem ("Force-Inject Steam Overlay", "", nullptr))
        //SK_Steam_LoadOverlayEarly ();

      //ImGui::Separator ();
      ImGui::MenuItem ("Virtual Gamepad/Keyboard Cursor", "", &io.NavMovesMouse);
      ImGui::MenuItem ("Display Active Input APIs",       "", &config.imgui.show_input_apis);


      if (config.apis.NvAPI.enable && sk::NVAPI::nv_hardware)
      {
        //ImGui::TextWrapped ("%ws", SK_NvAPI_GetGPUInfoStr ().c_str ());
        ImGui::MenuItem    ("Display G-Sync Status", "", &config.apis.NvAPI.gsync_status);
      }

      ImGui::MenuItem  ("Display Playtime in Title",     "", &config.steam.show_playtime);
      ImGui::MenuItem  ("Display Mac-style Menu at Top", "", &config.imgui.use_mac_style_menu);
      ImGui::Separator ();

      DisplayModeMenu (windowed);

      //ImGui::MenuItem ("ImGui Demo",              "", &show_test_window);
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
              ShellExecuteA (GetActiveWindow (), "explore", SK::SteamAPI::GetDataDir ().c_str (), nullptr, nullptr, SW_NORMAL);
            }

            if (ImGui::MenuItem ("Cloud Config", ""))
            {
              ShellExecuteA (GetActiveWindow (), "explore", SK::SteamAPI::GetConfigDir ().c_str (), nullptr, nullptr, SW_NORMAL);
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

          ShellExecuteW (GetActiveWindow (), L"explore", log_dir.c_str (), nullptr, nullptr, SW_NORMAL);
        }


        SK_RenderBackend& rb = SK_GetCurrentRenderBackend ();

        bool supports_texture_mods = 
        //( static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9)  ) ||
          ( static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11) );

        if (supports_texture_mods)
        {
          extern std::wstring SK_D3D11_res_root;

          static bool bHasTextureMods =
                       ( GetFileAttributesW (SK_D3D11_res_root.c_str ()) != INVALID_FILE_ATTRIBUTES );

          if (bHasTextureMods)
          {
            if (ImGui::BeginMenu ("Browse Texture Assets"))
            {
              extern uint64_t injectable_texture_bytes;

              wchar_t wszPath [MAX_PATH * 2] = { };

              if (ImGui::MenuItem ("Injectable Textures", SK_FormatString ("%ws", SK_File_SizeToString (injectable_texture_bytes).c_str ()).c_str (), nullptr))
              {
                wcscpy      (wszPath, SK_D3D11_res_root.c_str ());
                PathAppendW (wszPath, LR"(inject\textures)");

                ShellExecuteW (GetActiveWindow (), L"explore", wszPath, nullptr, nullptr, SW_NORMAL);
              }

              extern std::unordered_set <uint32_t> dumped_textures;
              extern uint64_t                      dumped_texture_bytes;

              if ((! dumped_textures.empty ()) && ImGui::MenuItem ("Dumped Textures", SK_FormatString ("%ws", SK_File_SizeToString (dumped_texture_bytes).c_str ()).c_str (), nullptr))
              {
                wcscpy      (wszPath, SK_D3D11_res_root.c_str ());
                PathAppendW (wszPath, LR"(dump\textures)");
                PathAppendW (wszPath, SK_GetHostApp ());

                ShellExecuteW (GetActiveWindow (), L"explore", wszPath, nullptr, nullptr, SW_NORMAL);
              }

              ImGui::EndMenu ();
            }
          }

          else
          {
            if (ImGui::MenuItem ("Initialize Texture Mods", "", nullptr))
            {
              wchar_t      wszPath [MAX_PATH * 2] = { };
              wcscpy      (wszPath, SK_D3D11_res_root.c_str ());
              PathAppendW (wszPath, LR"(inject\textures\)");

              SK_CreateDirectories (wszPath);

              bHasTextureMods =
                ( GetFileAttributesW (SK_D3D11_res_root.c_str ()) != INVALID_FILE_ATTRIBUTES );
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

          ShellExecuteW (GetActiveWindow (), L"explore", reshade_dir.c_str (), nullptr, nullptr, SW_NORMAL);
        }

        ImGui::Separator ();

        static bool wrappable = true;

        if (SK_IsInjected () && wrappable)
        {
          if (ImGui::MenuItem ("Install Wrapper DLL for this game"))
          {
            if (SK_Inject_SwitchToRenderWrapper ())
              wrappable = false;
          }
        }

        else
        {
          if (ImGui::MenuItem ("Uninstall Wrapper DLL for this game"))
          {
            wrappable = 
              SK_Inject_SwitchToGlobalInjector ();
          }
        }

        if (SK::SteamAPI::AppID () == 0 && (! config.steam.force_load_steamapi))
        {
          ImGui::Separator ();

          if (ImGui::MenuItem ("Manually Inject SteamAPI", ""))
          {
            config.steam.force_load_steamapi = true;
            SK_Steam_KickStart  ();
            SK_ApplyQueuedHooks ();
          }
        }

        ImGui::Separator ();

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

      if (ImGui::BeginMenu ("Update"))
      {
        bool selected = false;
            ImGui::MenuItem  ("Current Version###Menu_CurrentVersion", current_ver, &selected, false);

        if (branches.size () > 1)
        {
          char szCurrentBranchMenu [128] = { };
          sprintf (szCurrentBranchMenu, "Current Branch:  (%s)###SelectBranchMenu", current_branch_str);

          if (ImGui::BeginMenu (szCurrentBranchMenu, branches.size () > 1))
          {
            for ( auto& it : branches )
            {
              selected = ( SK_UTF8ToWideChar (it) == current_branch.release.vinfo.branch );

              if (ImGui::MenuItem (it.c_str (), SK_WideCharToUTF8 (branch_details [it].general.description).c_str (), &selected))
              {
                SK_Version_SwitchBranches (nullptr, it.c_str ());

                // Re-fetch the version info and then go to town updating stuff ;)
                SK_FetchVersionInfo1 (nullptr, true);

                branches       = SK_Version_GetAvailableBranches (nullptr);
                vinfo          = SK_Version_GetLocalInfo         (nullptr);
                vinfo_latest   = SK_Version_GetLatestInfo        (nullptr);
                current_branch = SK_Version_GetLatestBranchInfo  (nullptr, SK_WideCharToUTF8 (vinfo_latest.branch).c_str ());
                branch_details = PopulateBranches (branches);

                // !!! Handle the case where the number of branches changes after we fetch the repo
                break;
              }

              else if (ImGui::IsItemHovered ())
              {
                ImGui::BeginTooltip ();
                ImGui::Text         ("%ws", branch_details [it].release.title.c_str ());
                //ImGui::Separator    ();
                //ImGui::BulletText   ("Build: %li", branch_details [it].release.vinfo.build);
                ImGui::EndTooltip   ();
              }
            }

            ImGui::Separator ();

            ImGui::TreePush       ("");
            ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.125f, 0.9f, 0.75f));
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
          ImGui::MenuItem  ("Current Branch###Menu_CurrentBranch", current_branch_str, &selected, false);

        ImGui::Separator ();

        if (vinfo.build < vinfo_latest.build)
        {
          if (ImGui::MenuItem  ("Update Now"))
            SK_UpdateSoftware (nullptr);

          ImGui::Separator ();
        }

        snprintf        (current_ver, 127, "%ws (%li)", vinfo_latest.package.c_str (), vinfo_latest.build);
        ImGui::MenuItem ("Latest Version###Menu_LatestVersion",   current_ver,                                                      &selected, false);
        ImGui::MenuItem ("Last Checked###Menu_LastUpdateCheck",   SK_WideCharToUTF8 (SK_Version_GetLastCheckTime_WStr ()).c_str (), &selected, false);

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
            case SixHours:    SK_Version_SetUpdateFrequency (nullptr,      6 * _Hour); break;
            case TwelveHours: SK_Version_SetUpdateFrequency (nullptr,     12 * _Hour); break;
            case OneDay:      SK_Version_SetUpdateFrequency (nullptr,     24 * _Hour); break;
            case OneWeek:     SK_Version_SetUpdateFrequency (nullptr, 7 * 24 * _Hour); break;
            case Never:       SK_Version_SetUpdateFrequency (nullptr,              0); break;
          }
        }

        ImGui::TreePop ();

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
          SK_SteamOverlay_GoToURL (SK_WideCharToUTF8 (current_branch.release.notes).c_str (), true);
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
                                  SK_VERSION_STR_A
                              ).c_str (), ""
                          );

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("%lu-Bit Injection History", sizeof (uintptr_t) == 4 ? 32 :
                                                                                        64);
            ImGui::Separator    ();

            int count = InterlockedAdd (&SK_InjectionRecord_s::count, 0UL);
            ImGui::BulletText   ("%lu injections since restart", count);


            ImGui::BeginGroup ();
            for (int i = 0; i < count; i++)
            {
              SK_InjectionRecord_s* record =
                SK_Inject_GetRecord (i);
            
              ImGui::Text ( " pid %04x:  ", record->process.id );
            }
            ImGui::EndGroup   ();
            ImGui::SameLine   ();
            ImGui::BeginGroup ();
            for (int i = 0; i < count; i++)
            {
              SK_InjectionRecord_s* record =
                SK_Inject_GetRecord (i);
            
              ImGui::Text ( " [%ws]  ", record->process.name );
            }
            ImGui::EndGroup   ();

            ImGui::Separator  ();

            wchar_t* bouncy [8] = { g_LastBouncedModule0, g_LastBouncedModule1,
                                    g_LastBouncedModule2, g_LastBouncedModule3,
                                    g_LastBouncedModule4, g_LastBouncedModule5,
                                    g_LastBouncedModule6, g_LastBouncedModule7 };

            for (int i = 0; i < 8; i++)
            {
              ImGui::Text     ("Bounced Executable - %ws", bouncy [i]);
            }
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
           ImGui::EndTooltip ();
         }
       }
       else
         ImGui::MenuItem ( "Special K Bootstrapper",
                             SK_FormatString (
                               "%ws API Wrapper  %s",
                                 SK_GetBackend (), SK_VERSION_STR_A
                             ).c_str (), ""
                         );

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

       for ( auto& import : imports )
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
        if (dgvoodoo_d3d8 != nullptr)
        {
          ImGui::MenuItem ( SK_WideCharToUTF8 (dgvoodoo_d3d8->name).c_str         (),
                            SK_WideCharToUTF8 (dgvoodoo_d3d8->product_desc).c_str () );
        }

        if (dgvoodoo_ddraw != nullptr)
        {
          ImGui::MenuItem ( SK_WideCharToUTF8 (dgvoodoo_ddraw->name).c_str         (),
                            SK_WideCharToUTF8 (dgvoodoo_ddraw->product_desc).c_str () );
        }

        if (dgvoodoo_d3dimm != nullptr)
        {
          ImGui::MenuItem ( SK_WideCharToUTF8 (dgvoodoo_d3dimm->name).c_str         (),
                            SK_WideCharToUTF8 (dgvoodoo_d3dimm->product_desc).c_str () );
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



  static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");
  static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");

  static bool show_test_window = false;

  static float last_width  = -1;
  static float last_height = -1;

  bool recenter = ( last_width  != io.DisplaySize.x ||
                    last_height != io.DisplaySize.y );

  if (recenter)
  {
    SK_ImGui_LastWindowCenter.x = -1.0f;
    SK_ImGui_LastWindowCenter.y = -1.0f;

    ImGui::SetNextWindowPosCenter       (ImGuiSetCond_Always);
    last_width = io.DisplaySize.x; last_height = io.DisplaySize.y;
  }


  ImGui::SetNextWindowSizeConstraints (ImVec2 (250, 75), ImVec2 ( 0.9f * io.DisplaySize.x,
                                                                  0.9f * io.DisplaySize.y ) );


  const char* szTitle     = SK_ImGui_ControlPanelTitle ();
  static int  title_len   = int (static_cast <float> (ImGui::CalcTextSize (szTitle).x) * 1.075f);
  static bool first_frame = true;
  bool        open        = true;


  // Initialize the dialog using the user's scale preference
  if (first_frame)
  {
    io.NavMovesMouse = true;

    // Range-restrict for sanity!
    config.imgui.scale = std::max (1.0f, std::min (5.0f, config.imgui.scale));
  
    io.FontGlobalScale = config.imgui.scale;
    first_frame        = false;
  }

  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  ImGuiStyle& style =
    ImGui::GetStyle ();

  style.WindowTitleAlign =
    ImVec2 (0.5f, 0.5f);

  ImGuiStyle orig = style;

  style.WindowMinSize.x = title_len * 1.075f * io.FontGlobalScale;
  style.WindowMinSize.y = 200;

  extern bool nav_usable;

  if (nav_usable)
    ImGui::PushStyleColor ( ImGuiCol_Text, ImColor::HSV ( (float)(current_time % 2800) / 2800.0f, 
                                            (0.5f + (sin ((float)(current_time % 500)  / 500.0f)) * 0.5f) / 2.0f,
                                             1.0f ) );
  else
    ImGui::PushStyleColor ( ImGuiCol_Text, ImColor (255, 255, 255) );

  ImGui::Begin            ( szTitle, &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                       (config.imgui.use_mac_style_menu ? 0x00: ImGuiWindowFlags_MenuBar ) );

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

  SK_RenderBackend& rb =
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
      ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
    }

    if (ImGui::BeginPopup      ("RenderSubMenu"))
    {
      DisplayMenu     ();
      ImGui::EndPopup ();
    }

    ImGui::Separator ();

    char szResolution [128] = { };

    bool sRGB     = rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_SRGB;
    bool override = false;

    RECT client;
    GetClientRect (game_window.hWnd, &client);

    if ( static_cast <int> (io.DisplayFramebufferScale.x) != client.right  - client.left ||
         static_cast <int> (io.DisplayFramebufferScale.y) != client.bottom - client.top  ||
         sRGB )
    {
      snprintf ( szResolution, 63, "   %ux%u", 
                   static_cast <UINT> (io.DisplayFramebufferScale.x),
                   static_cast <UINT> (io.DisplayFramebufferScale.y) );

      if (sRGB)
        strcat (szResolution, "    (sRGB)");

      if (ImGui::MenuItem (" Framebuffer Resolution", szResolution))
      {
        config.window.res.override.x = static_cast <UINT> (io.DisplayFramebufferScale.x);
        config.window.res.override.y = static_cast <UINT> (io.DisplayFramebufferScale.y);

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
        snprintf ( szResolution, 63, "%s @ %4.1f Hz",
                                       szResolution, refresh_rate );
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
          strcat (szGSyncStatus, "Active");
        else
          strcat (szGSyncStatus, "Inactive");
      }
      else
      {
        strcat ( szGSyncStatus, rb.api == SK_RenderAPI::OpenGL ? " Unknown in GL" :
                                                                 "   Unsupported" );
      }

      ImGui::MenuItem (" G-Sync Status   ", szGSyncStatus, nullptr, false);

      if (rb.api == SK_RenderAPI::OpenGL && ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("The NVIDIA driver API does not report this status in OpenGL.");
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

      if (ImGui::SliderFloat ("###IMGUI_SCALE", &config.imgui.scale, 1.0f, 3.0f, "UI Scaling Factor %.2f"))
      {
        // ImGui does not perform strict parameter validation, and values out of range for this can be catastrophic.
        config.imgui.scale = std::max (1.0f, std::min (5.0f, config.imgui.scale));
        io.FontGlobalScale = config.imgui.scale;
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Optimal UI layout requires 1.0; other scales may not display as intended.");

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
        float target_orig = target_fps;

        bool limit = (target_fps > 0.0f);

        if (ImGui::Checkbox ("Framerate Limit", &limit))
        {
          if (target_fps != 0.0f) // Negative zero... it exists and we don't want it.
            target_fps = -target_fps;

          if (target_fps == 0.0f)
            target_fps = 60.0f;
        }

        if (limit && ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Graph color represents frame time variance, not proximity to your target FPS");

        ImGui::SameLine ();

        float target_mag = fabs (target_fps);

        if (ImGui::DragFloat ( "###FPS_TargetOrLimit", &target_mag,
                                 1.0f, 24.0f, 166.0f, target_fps > 0 ? "%6.3f fps  (Limit Engaged)" :
                                                      target_fps < 0 ? "%6.3f fps  (Graphing Only)" :
                                                                       "60.000 fps (No Preference)" ) )
        {
          target_fps = target_fps < 0.0f ? (-1.0f * target_mag) :
                                                    target_mag;

          if (target_fps > 10.0f || target_fps == 0.0f)
            SK_GetCommandProcessor ()->ProcessCommandFormatted ("TargetFPS %f", target_fps);
          else if (target_fps < 0.0f)
          {
            float graph_target = target_fps;
            SK_GetCommandProcessor ()->ProcessCommandLine      ("TargetFPS 0.0");
                  target_fps   = graph_target;
          }
          else
            target_fps = target_orig;
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextColored     (ImColor::HSV (0.18f, 0.88f, 0.94f), "Ctrl + Click");
          ImGui::SameLine        ();
          ImGui::TextUnformatted ("to Enter an Exact Framerate");
          ImGui::EndTooltip      ();
        }

        ImGui::SameLine ();

        advanced = ImGui::TreeNode ("Advanced ###Advanced_FPS");

        if (advanced)
        {
          ImGui::TreePop    ();
          ImGui::BeginGroup ();

          if (target_fps > 0.0f)
          {
            ImGui::SliderFloat ( "Target Framerate Tolerance", &config.render.framerate.limiter_tolerance, 0.925f, 4.0f);
         
            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip   ();
              ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f));
              ImGui::Text           ("Controls Framerate Smoothness\n\n");
              ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.75f, 0.75f, 0.75f, 1.0f));
              ImGui::Text           ("  Lower = Stricter, but setting");
              ImGui::SameLine       ();
              ImGui::PushStyleColor (ImGuiCol_Text, ImColor (0.95f, 1.0f, 0.65f, 1.0f));
              ImGui::Text           ("too low");
              ImGui::SameLine       ();
              ImGui::PushStyleColor (ImGuiCol_Text, ImColor(0.75f, 0.75f, 0.75f, 1.0f));
              ImGui::Text           ("will cause framerate instability...");
              ImGui::PopStyleColor  (4);
              ImGui::Separator      ( );
              ImGui::Text           ("Recomputes clock phase if a single frame takes longer than <1.0 + tolerance> x <target_ms> to complete");
              ImGui::BulletText     ("Adjust this if your set limit is fighting with VSYNC (frequent frametime graph oscillations).");
              ImGui::EndTooltip     ( );
            }
          }

          changed |= ImGui::Checkbox ("Sleepless Render Thread",         &config.render.framerate.sleepless_render  );
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
                     ImGui::SameLine (                                                                              );

          changed |= ImGui::Checkbox ("Sleepless Window Thread", &config.render.framerate.sleepless_window );
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

          ImGui::Separator ();

          if (target_fps > 0.0f)
          {
            ImGui::Checkbox      ("Busy-Wait Limiter",        &config.render.framerate.busy_wait_limiter);

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip    ();
              ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.11f, 0.97f, 0.96f));
              ImGui::TextUnformatted ("Spin the render thread up to 100% load for improved timing consistency");
              ImGui::Separator       ();
              ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.26f, 0.86f, 0.84f));
              ImGui::BulletText      ("Accurate timing is also possible using sleep-wait;");
              ImGui::SameLine        ();
              ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.5f, 1.0f, 1.0f));
              ImGui::TextUnformatted ("busy's ideal use-case is bypassing a game's built-in limiter.");
              ImGui::PopStyleColor   (3);
              ImGui::TreePush        ("");
              ImGui::BulletText      ("This CPU core will spend all of its time waiting for the next frame to start.");
              ImGui::BulletText      ("Improves smoothness on 4+ core systems, but not advised for low-end systems.");
              ImGui::BulletText      ("Negatively impacts battery life on mobile devices.");
              ImGui::TreePop         ();
              ImGui::EndTooltip      ();
            }

            ImGui::SameLine      ();

            ImGui::Checkbox      ("Reduce Input Latency",      &config.render.framerate.min_input_latency);

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip    ();
              ImGui::TextUnformatted ("Optimization for games that process input, rendering and window events in the same thread");
              ImGui::Separator       ();
              ImGui::BulletText      ("Attempts to keep the event loop running while waiting for the next frame.");
              ImGui::BulletText      ("A small number of games will react negatively to this option and gamepad input may worsen performance.");
              ImGui::EndTooltip      ();
            }

            if (! config.render.framerate.busy_wait_limiter)
            {
              float sleep_duration = 
                ( 10.0f / target_fps ) * config.render.framerate.max_sleep_percent;

              ImGui::SameLine    (                                                                     );
              ImGui::Checkbox    ( "Sleep Once, then Spin",        &config.render.framerate.yield_once );
              if (ImGui::IsItemHovered ())
              {
                ImGui::SetTooltip ( "Sleep (offer CPU resources to other ready-to-run tasks) once "
                                    "for %4.1f ms, then begin busy-waiting", sleep_duration );
              }

              ImGui::Separator   (                                                                     );
              ImGui::SliderFloat ( "Burn CPU Cycles for Better Timing",
                                     &config.render.framerate.sleep_deadline, 0.00f,
                                       1000.0f / target_fps, "Once within %4.1f ms of the next frame"  );
              ImGui::SliderFloat ( "Sleep Duration", &config.render.framerate.max_sleep_percent, 0.01f,
                                   99.9f, SK_FormatString ( "%4.1f ms", sleep_duration ).c_str ()      );
            }
          }

          ImGui::EndGroup ();
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
    bool framepacing   = SK_ImGui_Widgets.frame_pacing->isVisible   ();
    bool gpumon        = SK_ImGui_Widgets.gpu_monitor->isVisible    ();
    bool volumecontrol = SK_ImGui_Widgets.volume_control->isVisible ();
    bool cpumon        = SK_ImGui_Widgets.cpu_monitor->isVisible    ();
    bool pipeline      = SK_ImGui_Widgets.d3d11_pipeline->isVisible ();

    ImGui::TreePush ("");

    if (ImGui::Checkbox ("Framepacing", &framepacing))
    {
      SK_ImGui_Widgets.frame_pacing->setVisible (framepacing).setActive (framepacing);
    }
    ImGui::SameLine ();

    if (ImGui::Checkbox ("CPU",         &cpumon))
    {
      SK_ImGui_Widgets.cpu_monitor->setVisible (cpumon);
    }
    ImGui::SameLine ();

    if (ImGui::Checkbox ("GPU",         &gpumon))
    {
      SK_ImGui_Widgets.gpu_monitor->setVisible (gpumon);//.setActive (gpumon);
    }
    ImGui::SameLine ();

    //ImGui::Checkbox ("Memory",       &SK_ImGui_Widgets.memory);
    //ImGui::SameLine ();
    //ImGui::Checkbox ("Disk",         &SK_ImGui_Widgets.disk);
    //ImGui::SameLine ();

    if (ImGui::Checkbox ("Volume Control", &volumecontrol))
    {
      SK_ImGui_Widgets.volume_control->setVisible (volumecontrol).setActive (volumecontrol);
    }

    if ( (int)render_api & (int)SK_RenderAPI::D3D11 )
    {
      ImGui::SameLine ();
      ImGui::Checkbox ("Texture Cache", &SK_ImGui_Widgets.texcache);

      ImGui::SameLine ();
      if (ImGui::Checkbox ("Pipeline Stats", &pipeline))
      {
        SK_ImGui_Widgets.d3d11_pipeline->setVisible (pipeline).setActive (pipeline);
      }
    }
    ImGui::TreePop  (  );
  }

  SK::ControlPanel::PlugIns::Draw ();
  SK::ControlPanel::Steam::Draw   ();

  ImVec2 pos  = ImGui::GetWindowPos  ();
  ImVec2 size = ImGui::GetWindowSize ();

  SK_ImGui_LastWindowCenter.x = pos.x + size.x / 2.0f;
  SK_ImGui_LastWindowCenter.y = pos.y + size.y / 2.0f;

  if (recenter)
    SK_ImGui_CenterCursorAtPos ();


  if (show_test_window)
    ImGui::ShowTestWindow (&show_test_window);
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

struct
{
  SK_ImGui_DrawCallback_pfn fn   = nullptr;
  void*                     data = nullptr;
} SK_ImGui_DrawCallback;

struct
{
  SK_ImGui_OpenCloseCallback_pfn fn   = nullptr;
  void*                          data = nullptr;
} SK_ImGui_OpenCloseCallback;

__declspec (noinline)
IMGUI_API
void
SK_ImGui_InstallDrawCallback (SK_ImGui_DrawCallback_pfn fn, void* user)
{
  SK_ImGui_DrawCallback.fn   = fn;
  SK_ImGui_DrawCallback.data = user;
}

IMGUI_API
void
SK_ImGui_InstallOpenCloseCallback (SK_ImGui_OpenCloseCallback_pfn fn, void* user)
{
  SK_ImGui_OpenCloseCallback.fn   = fn;
  SK_ImGui_OpenCloseCallback.data = user;
}



#include <SpecialK/osd/text.h>

static bool keep_open;

void
SK_ImGui_StageNextFrame (void)
{
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

  current_time = timeGetTime ();

  bool d3d9  = false;
  bool d3d11 = false;
  bool gl    = false;

  SK_RenderBackend& rb =
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

  else
  {
    SK_LOG0 ( (L"No Render API"), L"Overlay" );
    return;
  }

  ImGuiIO& io (ImGui::GetIO ());


  //
  // Framerate history
  //
  if (! (reset_frame_history || skip_frame_history))
  {
    SK_ImGui_Frames.timeFrame (io.DeltaTime);
  }

  else if (reset_frame_history) SK_ImGui_Frames.reset ();

  reset_frame_history = false;


  static bool init_widgets = true;

  static std::array <SK_Widget *, 5> widgets
  {
    SK_ImGui_Widgets.frame_pacing,
      SK_ImGui_Widgets.volume_control,
        SK_ImGui_Widgets.gpu_monitor,
          SK_ImGui_Widgets.cpu_monitor,
            SK_ImGui_Widgets.d3d11_pipeline
  };

  if (init_widgets)
  {
    for (auto& widget : widgets)
      widget->run_base ();

    init_widgets = false;
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
    keep_open = SK_ImGui_ControlPanel ();
  }


  for (auto& widget : widgets)
  {
    if (widget->isActive ())
      widget->run_base ();

    if (widget->isVisible ())
      widget->draw_base ();
  }



  static DWORD dwStartTime = current_time;
  if ((current_time < dwStartTime + 1000 * config.version_banner.duration) || eula.show)
  {
    ImGui::PushStyleColor    (ImGuiCol_Text,     ImVec4 (1.f,   1.f,   1.f,   1.f));
    ImGui::PushStyleColor    (ImGuiCol_WindowBg, ImVec4 (.333f, .333f, .333f, 0.828282f));
    ImGui::SetNextWindowPos  (ImVec2 (10, 10));
    ImGui::SetNextWindowSize (ImVec2 ( ImGui::GetIO ().DisplayFramebufferScale.x - 20.0f,
                                       ImGui::GetItemsLineHeightWithSpacing ()   * 4.5f  ), ImGuiSetCond_Always );
    ImGui::Begin             ( "Splash Screen##SpecialK",
                                 nullptr, ImVec2 (), -1,
                                   ImGuiWindowFlags_NoTitleBar      |
                                   ImGuiWindowFlags_NoScrollbar     |
                                   ImGuiWindowFlags_NoMove          |
                                   ImGuiWindowFlags_NoResize        |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoInputs        |
                                   ImGuiWindowFlags_NoFocusOnAppearing );

    extern std::wstring
    __stdcall
    SK_GetPluginName (void);

    char szName [512] = { };

    if (SK_SteamAPI_Friends ())
    {
      strcpy (szName, SK_SteamAPI_Friends ()->GetPersonaName ());
    }

    ImGui::TextColored     (ImColor::HSV (.11f, 1.f, 1.f),  "%ws   ", SK_GetPluginName ().c_str ()); ImGui::SameLine ();

    if (*szName != '\0')
    {
      ImGui::Text            ("  Hello");                                                            ImGui::SameLine ();
      ImGui::TextColored     (ImColor::HSV (0.075f, 1.0f, 1.0f), "%s", szName);                      ImGui::SameLine ();
      ImGui::TextUnformatted ("please see the Release Notes, under");                                ImGui::SameLine ();
    }
    else
      ImGui::TextUnformatted ("  Please see the Release Notes, under");                              ImGui::SameLine ();
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

    if (! current_branch.release.description.empty ())
    {
      ImGui::BeginChildFrame ( ImGui::GetID ("###SpecialK_SplashScreenFrame"),
                               ImVec2 (-1,ImGui::GetItemsLineHeightWithSpacing () * 2.1f),
                               ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize );

      std::chrono::time_point <std::chrono::system_clock> now =
        std::chrono::system_clock::now ();

      ImColor version_color =
        ImColor::HSV (static_cast <float> (
                         std::chrono::duration_cast <std::chrono::milliseconds> (
                           now.time_since_epoch ()
                         ).count () % 3333
                       ) / 3333.3f,
                       0.25f, 1.f
                     );

      ImGui::Text        (" You are currently running");                      ImGui::SameLine ();
      ImGui::TextColored (ImColor::HSV (.15, 0.9, 1.), "%ws",
                    current_branch.release.title.c_str () );                  ImGui::SameLine ();
      ImGui::Text        ("from the");                                        ImGui::SameLine ();
      ImGui::TextColored (ImColor::HSV (.4, 0.9, 1.), "%ws",
                    current_branch.release.vinfo.branch.c_str () );           ImGui::SameLine ();
      ImGui::Text        ("development branch.");
      ImGui::Spacing     ();
      ImGui::Spacing     ();
      ImGui::TreePush    ("");
      ImGui::TextColored (ImColor::HSV (.08, .75, 0.6), "%ws",
                          SK_Version_GetLastCheckTime_WStr ().c_str ()); ImGui::SameLine ();
      ImGui::TextColored (version_color,               "%ws",
                           current_branch.release.description.c_str ()
                         );
      ImGui::TreePop     ();
      
      ImGui::EndChildFrame ();
    }

    else
    {
      ImGui::Text          ("");
      ImGui::Text          ("");
    }

    ImGui::Spacing ();

    ImGui::TextUnformatted (                                  "Press");                            ImGui::SameLine ();
    ImGui::TextColored     ( ImColor::HSV (.16f, 1.f, 1.f),  R"('%s%s%s')",
                                            "Ctrl + ",
                                            "Shift + ",
                                            "Backspace" );                                         ImGui::SameLine ();
    ImGui::TextUnformatted (                                  ", ");                               ImGui::SameLine ();
    ImGui::TextColored     ( ImColor::HSV (.16f, 1.f, 1.f),  R"('Select + Start' (PlayStation))"); ImGui::SameLine ();
    ImGui::TextUnformatted (                                  "or ");                              ImGui::SameLine ();
    ImGui::TextColored     ( ImColor::HSV (.16f, 1.f, 1.f),  R"('Back + Start' (Xbox))");          ImGui::SameLine ();
    
    ImGui::TextUnformatted (                                  "to open Special K's "
                                                              "configuration menu. ");
    
    ImGui::End             ( );
    ImGui::PopStyleColor   (2);
  }



  if (SK_ImGui_DrawCallback.fn != nullptr)
  {
    if (SK_ImGui_DrawCallback.fn (SK_ImGui_DrawCallback.data) > 0)
    {
      if (! SK_ImGui_Active ())
      {
        ImGuiWindow* pWin =
          ImGui::FindWindowByName ("ReShade 3.0.8 by crosire; modified for Special K by Kaldaien###ReShade_Main");

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
    if (SK_ImGui_Widgets.texcache)
    {
      static bool move = true;
      if (move)
      {
        ImGui::SetNextWindowPos (ImVec2 ((io.DisplaySize.x + font.size * 35) / 2.0f, 0.0f), ImGuiSetCond_Always);
        move = false;
      }

      ImGui::SetNextWindowSize  (ImVec2 (font.size * 35, font.size_multiline * 6.25f),      ImGuiSetCond_Always);
      
      ImGui::Begin            ("###Widget_TexCacheD3D11", nullptr, ImGuiWindowFlags_NoTitleBar         | ImGuiWindowFlags_NoResize         |
                                                                   ImGuiWindowFlags_NoScrollbar        | ImGuiWindowFlags_AlwaysAutoResize |
                                                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus );

      SK_ImGui_DrawTexCache_Chart ();

      if (ImGui::IsMouseClicked (1) && ImGui::IsWindowHovered ())
        move = true;

      ImGui::End              ();
    }
  }



  SK_ImGui_ProcessWarnings ();


  if (SK_ImGui_WantExit)
  { SK_ReShade_Visible = true;

    SK_ImGui_ConfirmExit ();

    if ( ImGui::BeginPopupModal ( "Confirm Forced Software Termination",
                                    nullptr,
                                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                      ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse )
       )
    {
      ImGui::FocusWindow (ImGui::GetCurrentWindow ());
      ImGui::TextColored (ImColor::HSV (0.075f, 1.0f, 1.0f), "\n         You will lose any unsaved game progress.         \n\n");
      ImGui::Separator   ();

      ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f), " Confirm Exit? " );

      ImGui::SameLine    (); ImGui::Spacing (); ImGui::SameLine ();

      if (ImGui::Button  ("Okay"))
        ExitProcess (0x00);

      //ImGui::PushItemWidth (ImGui::GetWindowContentRegionWidth () * 0.33f); ImGui::SameLine (); ImGui::SameLine (); ImGui::PopItemWidth ();

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

      ImGui::Checkbox    ("Enable Alt + F4", &config.input.keyboard.catch_alt_f4);

      ImGui::EndPopup    ();
    }
  }



  if (io.WantMoveMouse)
  {
    SK_ImGui_Cursor.pos.x = static_cast <LONG> (io.MousePos.x);
    SK_ImGui_Cursor.pos.y = static_cast <LONG> (io.MousePos.y);

    POINT screen_pos = SK_ImGui_Cursor.pos;

    if (GetCursor () != nullptr)
      SK_ImGui_Cursor.orig_img = GetCursor ();

    SK_ImGui_Cursor.LocalToScreen (&screen_pos);
    SetCursorPos_Original ( screen_pos.x,
                            screen_pos.y );

    io.WantCaptureMouse = true;

    SK_ImGui_UpdateCursor ();
  }

  imgui_staged = true;
  imgui_staged_frames++;
}

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
  bool gl    = false;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.api == SK_RenderAPI::OpenGL)
  {
    gl = true;

    ImGui::Render ();
  }

  else if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9))
  {
    d3d9 = true;

    CComQIPtr <IDirect3DDevice9> pDev =
      SK_GetCurrentRenderBackend ().device;

    if ( SUCCEEDED (
           pDev->BeginScene ()
         )
       )
    {
      ImGui::Render  ();
      pDev->EndScene ();
    }
  }

  else if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11))
  {
    d3d11 = true;

    ImGui::Render ();
  }

  else
  {
    SK_LOG0 ( (L"No Render API"), L"Overlay" );
    return 0x0;
  }

  if (! keep_open)
    SK_ImGui_Toggle ();


  POINT                    orig_pos;
  GetCursorPos_Original  (&orig_pos);

  SK_ImGui_Cursor.update ();

  SetCursorPos_Original  (orig_pos.x, orig_pos.y);

  imgui_finished_frames++;


  imgui_staged = false;

  return 0;
}


BYTE __imgui_keybd_state [512] = { };

// Keys down when the UI started capturing input,
//   the first captured key release event will be
//     sent to the game and the state of that key reset.
//
//  This prevents the game from thinking a key is stuck.
INT SK_ImGui_ActivationKeys [256] = { };


__declspec (dllexport)
void
SK_ImGui_Toggle (void)
{
  ImGuiIO& io (ImGui::GetIO ());

  static ULONG last_frame = 0;

  if (last_frame != SK_GetFramesDrawn ())
  {
    current_time  = timeGetTime       ();
    last_frame    = SK_GetFramesDrawn ();
  }

  static DWORD dwLastTime   = 0x00;

  bool d3d11 = false;
  bool d3d9  = false;
  bool gl    = false;

  SK_RenderBackend& rb =
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

  if (d3d9 || d3d11 || gl)
  {
    if (SK_ImGui_Active ())
      GetKeyboardState_Original (__imgui_keybd_state);

    SK_ImGui_Visible = (! SK_ImGui_Visible);

    if (SK_ImGui_Visible)
      ImGui::SetNextWindowFocus ();


    static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");
    static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");

    // Turns the hardware cursor on/off as needed
    ImGui_ToggleCursor ();

    // Most games
    if (! hModTBFix)
    {
      // Transition: (Visible -> Invisible)
      if (! SK_ImGui_Visible)
      {
        //SK_ImGui_Cursor.showSystemCursor ();
      }

      else
      {
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

      extern bool nav_usable;
      nav_usable = false;
    }

  //reset_frame_history = true;
  }

  if ((! SK_ImGui_Visible) && SK_ImGui_OpenCloseCallback.fn != nullptr)
  {
    if (SK_ImGui_OpenCloseCallback.fn (SK_ImGui_OpenCloseCallback.data))
      SK_ImGui_OpenCloseCallback.fn (SK_ImGui_OpenCloseCallback.data);
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
          const char* cmds [2] = { "Window.Fullscreen toggle", "Window.Borderless toggle" };
          SK_DeferCommands ( cmds, 2 );
        }

        else
        {
          const char* cmds [2] = { "Window.Borderless toggle", "Window.Fullscreen toggle" };
          SK_DeferCommands ( cmds, 2 );
        }
      }

      else
      {
        if (toggle_border)     { SK_DeferCommand ("Window.Borderless toggle"); }
        if (toggle_fullscreen) { SK_DeferCommand ("Window.Fullscreen toggle"); }
      }
#endif