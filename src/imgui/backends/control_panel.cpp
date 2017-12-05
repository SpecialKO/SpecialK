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
#include <imgui/widgets/msgbox.h>
#include <SpecialK/render_backend.h>

#include <SpecialK/widgets/widget.h>
#include <d3d9.h>

#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/DLL_VERSION.H>
#include <SpecialK/command.h>
#include <SpecialK/console.h>
#include <SpecialK/utility.h>

#include <SpecialK/window.h>
#include <SpecialK/log.h>
#include <SpecialK/steam_api.h>

#include <SpecialK/render_backend.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/d3d9_backend.h>
#include <SpecialK/D3D9/texmgr.h>
#include <SpecialK/sound.h>

#include <SpecialK/update/version.h>
#include <SpecialK/update/network.h>
#include <SpecialK/framerate.h>

#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/input/xinput_hotplug.h>

#include <SpecialK/nvapi.h>
#include <SpecialK/ini.h>
#include <SpecialK/import.h>
#include <SpecialK/injection/injection.h>

#include <SpecialK/io_monitor.h>

#include <SpecialK/plugin/reshade.h>

#include <concurrent_queue.h>


#include <windows.h>
#include <cstdio>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <typeindex>

#include "resource.h"


extern bool
SK_Denuvo_UsedByGame (bool retest = false);

struct denuvo_file_s
{
  AppId_t      app;
  CSteamID     user;
  uint64       hash;
  std::wstring path;
  FILETIME     ft_key;
  SYSTEMTIME   st_local;
};

extern std::vector <denuvo_file_s> denuvo_files;


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

extern const wchar_t* __stdcall SK_GetBackend (void);

extern bool     __stdcall SK_FAR_IsPlugIn      (void);
extern void     __stdcall SK_FAR_ControlPanel  (void);

       bool               SK_DXGI_FullStateCache = false;

extern GetCursorInfo_pfn GetCursorInfo_Original;
       bool              cursor_vis      = false;

       bool              show_shader_mod_dlg      = false;
extern bool              SK_D3D11_ShaderModDlg (void);

       bool              show_d3d9_shader_mod_dlg = false;
extern bool              SK_D3D9_TextureModDlg (void);

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

//
// If we did this from the render thread, we would deadlock most games
//
void
SK_DeferCommand (const char* szCommand)
{
  CreateThread ( nullptr,
                   0x00,
                     [ ](LPVOID user) ->
      DWORD
        {
          CHandle hThread (GetCurrentThread ());

          std::unique_ptr <char> cmd ((char *)user);

          SK_GetCommandProcessor ()->ProcessCommandLine (
             (const char *)cmd.get ()
          );

          return 0;
        },(LPVOID)_strdup (szCommand),
      0x00,
    nullptr
  );
};

extern void SK_Steam_SetNotifyCorner (void);

extern void __stdcall SK_ImGui_DrawEULA (LPVOID reserved);
extern bool           SK_ImGui_Visible;
       bool           SK_ReShade_Visible        = false;
       bool           SK_ControlPanel_Activated = false;

extern void
__stdcall
SK_SteamAPI_SetOverlayState (bool active);

extern bool
__stdcall
SK_SteamAPI_GetOverlayState (bool real);

extern void ImGui_ToggleCursor (void);

struct show_eula_s {
  bool show;
  bool never_show_again;
} extern eula;

#include <map>
#include <unordered_set>
#include <d3d11.h>

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

#include <ImGui/imgui_internal.h>

#pragma warning (push)
#pragma warning (disable: 4706)
// Special K Extensions to ImGui
namespace SK_ImGui
{
  bool
  VerticalToggleButton (const char *text, bool *v)
  {
          ImFont        *font      = GImGui->Font;
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

    color = style.Colors [ImGuiCol_Button];

    if (*v)
      color = style.Colors [ImGuiCol_ButtonActive];

    ImGui::PushStyleColor (ImGuiCol_Button, color);
    ImGui::PushID         (text);

    ret = ImGui::Button ( "", ImVec2 (text_size.y + pad * 2,
                                      text_size.x + pad * 2) );
    ImGui::PopStyleColor ();

    while ((c = *text++))
    {
      glyph = font->FindGlyph (c);

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
        snprintf (szBatteryLevel, 127, "%hu%% Battery Charge Remaining", battery_level);

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
  ImGui::SetNextWindowFocus           (                                                               );

  ImGui::OpenPopup ("Special K Warning");


  if ( ImGui::BeginPopupModal ( "Special K Warning",
                                  nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                    ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse )
     )
  {
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
  ImGui::SetNextWindowFocus           (                                                               );

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
SK_ImGui_IsItemRightClicked (ImGuiIO& io = ImGui::GetIO ())
{
  if (ImGui::IsItemClicked (1))
    return true;

  if (ImGui::IsItemHovered ())
  {
    if (io.NavInputsDownDuration [ImGuiNavInput_PadActivate] > 0.4f)
    {
      io.NavInputsDownDuration [ImGuiNavInput_PadActivate] = 0.0f;
      return true;
    }
  }

  return false;
};

bool
SK_ImGui_IsWindowRightClicked (ImGuiIO& io)
{
  if (ImGui::IsWindowFocused () || ImGui::IsWindowHovered ())
  {
    if (ImGui::IsMouseHoveringWindow () && io.MouseClicked [1])
      return true;

    if (ImGui::IsWindowFocused () && io.MouseDoubleClicked [4])
    {
      return true;
    }
  }

  return false;
};

void
SK_ImGui_UpdateCursor (void)
{
  POINT orig_pos;
  GetCursorPos_Original (&orig_pos);
  SetCursorPos_Original (0, 0);

  SK_ImGui_Cursor.update ();

  SetCursorPos_Original (orig_pos.x, orig_pos.y);
}

ImVec2 SK_ImGui_LastWindowCenter  (-1.0f, -1.0f);
void   SK_ImGui_CenterCursorAtPos (ImVec2 center = SK_ImGui_LastWindowCenter);

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
        snprintf ( szTitle, 511, "%ws%s     (%01lu:%02lu:%02lu)###SK_MAIN_CPL",
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

        snprintf ( szTitle, 511, "%ws(%01lu:%02lu:%02lu)###SK_MAIN_CPL",
                     title.c_str (),
                       hours, mins, secs );
      }

      else
        snprintf (szTitle, 511, "%ws###SK_MAIN_CPL", title.c_str ());
    }
  }

  return szTitle;
}

#include <TlHelp32.h>

static SK_WASAPI_SessionManager sessions;
static SK_WASAPI_AudioSession*  audio_session;

bool
SK_ImGui_SelectAudioSessionDlg (void)
{
  ImGuiIO& io (ImGui::GetIO ());

         bool  changed   = false;
  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  int            sel_idx = -1;


  ImGui::SetNextWindowSizeConstraints ( ImVec2 (io.DisplaySize.x * 0.25f,
                                                io.DisplaySize.y * 0.15f),
                                        ImVec2 (io.DisplaySize.x * 0.75f,
                                                io.DisplaySize.y * 0.666f) );

  if (ImGui::BeginPopupModal ("Audio Session Selector", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                                                 ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse))
  {
    int count = 0;

    SK_WASAPI_AudioSession** pSessions =
      sessions.getActive (&count);

    float max_width =
      ImGui::CalcTextSize ("Audio Session Selector").x;

    for (int i = 0; i < count; i++)
    {
      ImVec2 size = ImGui::CalcTextSize (pSessions [i]->getName ());

      if (size.x > max_width) max_width = size.x;
    }
    ImGui::PushItemWidth (max_width * 5.0f * io.FontGlobalScale);

    ImGui::BeginChild ("SessionSelectHeader",   ImVec2 (0, 0), true,  ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_NoInputs |
                                                                      ImGuiWindowFlags_NoNavInputs);
    ImGui::BeginGroup ( );
    ImGui::Columns    (2);
    ImGui::Text       ("Task");
    ImGui::NextColumn ();
    ImGui::Text       ("Volume / Mute");
    ImGui::NextColumn ();
    ImGui::Columns    (1);

    ImGui::Separator  ();

    //if (ImGui::ListBoxHeader ("##empty", count, std::min (count + 3, 10)))
    ImGui::BeginGroup ( );
    {
      ImGui::PushStyleColor (ImGuiCol_ChildWindowBg, ImColor (0, 0, 0, 0));
      ImGui::BeginChild     ("SessionSelectData",    ImVec2  (0, 0), true,  ImGuiWindowFlags_NavFlattened);

      ImGui::BeginGroup ();//"SessionSelectData");
      ImGui::Columns    (2);

      for (int i = 0; i < count; i++)
      {
        SK_WASAPI_AudioSession* pSession =
          pSessions [i];

        //bool selected     = false;
        bool drawing_self = (pSession->getProcessId () == GetCurrentProcessId ());

        CComPtr <ISimpleAudioVolume> volume_ctl =
          pSession->getSimpleAudioVolume ();

        BOOL mute = FALSE;

        if (volume_ctl != nullptr && SUCCEEDED (volume_ctl->GetMute (&mute)))
        {
          if (drawing_self)
          {
            ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
            ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
            ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
          }
          if (ImGui::Selectable (pSession->getName (), drawing_self || pSession == audio_session))
            sel_idx = i;

          if (drawing_self)
            ImGui::PopStyleColor (3);

          ImGui::NextColumn ();

          float                         volume = 0.0f;
          volume_ctl->GetMasterVolume (&volume);

          char      szLabel [32] = { };
          snprintf (szLabel, 31, "###VolumeSlider%i", i);

          ImGui::PushStyleColor (ImGuiCol_Text,           mute ? ImColor (0.5f, 0.5f, 0.5f) : ImColor (1.0f, 1.0f, 1.0f));
          ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor::HSV ( 0.4f * volume, 0.6f, mute ? 0.2f : 0.4f));
          ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor::HSV ( 0.4f * volume, 0.7f, mute ? 0.2f : 0.4f));
          ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor::HSV ( 0.4f * volume, 0.8f, mute ? 0.2f : 0.4f));
          ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImColor::HSV ( 0.4f * volume, 0.9f,        0.6f * (mute ? 0.5f : 1.0f)));

          volume *= 100.0f;

          ImGui::PushItemWidth (ImGui::GetContentRegionAvailWidth () - 37.0f);
          {
            if (ImGui::SliderFloat (szLabel, &volume, 0.0f, 100.0f, "Volume: %03.1f%%"))
              volume_ctl->SetMasterVolume (volume / 100.0f, nullptr);
          }
          ImGui::PopItemWidth  ( );
          ImGui::PopStyleColor (5);

          ImGui::SameLine      ( );

          snprintf (szLabel, 31, "###VoumeCheckbox%i", i);

          ImGui::PushItemWidth (35.0f);
          {
            if (ImGui::Checkbox (szLabel, (bool *)&mute))
              volume_ctl->SetMute (mute, nullptr);
          }
          ImGui::PopItemWidth  (     );
          ImGui::NextColumn    (     );
        }
      }

      ImGui::Columns  (1);
      ImGui::EndGroup ( );
      ImGui::EndChild ( );

      ImGui::PopStyleColor ();

      ImGui::EndGroup ( );
      ImGui::EndGroup ( );
      ImGui::EndChild ( );
      //ImGui::ListBoxFooter ();

      if (sel_idx != -1)
      {
        audio_session = pSessions [sel_idx];

        changed  = true;
      }

      if (changed)
      {
        ImGui::CloseCurrentPopup ();
      }
    }

    ImGui::PopItemWidth ();
    ImGui::EndPopup     ();
  }

  return changed;
}

void
SK_ImGui_AdjustCursor (void)
{
  CreateThread ( nullptr, 0,
    [](LPVOID user)->
    DWORD
    {
      UNREFERENCED_PARAMETER (user);

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

  if (! ( (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9)  ) ||
          (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11) ) ) )
    return;

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

  if (ImGui::Combo ("###SubMenu_DisplayMode_Combo", &mode, modes, 2) && mode != orig_mode)
  {
    switch (mode)
    {
      case DISPLAY_MODE_WINDOWED:
      {
        rb.requestWindowedMode (force);

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
    ImGui::SetTooltip ("Games that do not natively support Fullscreen mode may require an application restart.");
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

    modes = config.window.borderless ? "Bordered\0Borderless\0Borderless Fullscreen\0\0" :
                                       "Bordered\0Borderless\0\0";

    if (ImGui::Combo ("Window Style###SubMenu_WindowBorder_Combo", &mode, modes, config.window.borderless ? 3 : 2) && mode != orig_mode)
    {
      bool borderless = config.window.borderless;

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

      if (config.window.borderless != borderless)
      {
        config.window.borderless = borderless;
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

void
SK_ImGui_SummarizeD3D9Swapchain (IDirect3DSwapChain9* pSwap9)
{
  if (pSwap9 != nullptr)
  {
    D3DPRESENT_PARAMETERS pparams = { };

    if (SUCCEEDED (pSwap9->GetPresentParameters (&pparams)))
    {
      ImGui::BeginTooltip    ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (0.95f, 0.95f, 0.45f));
      ImGui::TextUnformatted ("Framebuffer and Presentation Setup");
      ImGui::PopStyleColor   ();
      ImGui::Separator       ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (0.785f, 0.785f, 0.785f));
      ImGui::TextUnformatted ("Color:");
      ImGui::TextUnformatted ("Depth/Stencil:");
      ImGui::TextUnformatted ("Resolution:");
      ImGui::TextUnformatted ("Back Buffers:");
      if (! pparams.Windowed)
      ImGui::TextUnformatted ("Refresh Rate:");
      ImGui::TextUnformatted ("Swap Interval:");
      ImGui::TextUnformatted ("Swap Effect:");
      ImGui::TextUnformatted ("MSAA Samples:");
      if (pparams.Flags != 0)
      ImGui::TextUnformatted ("Flags:");
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();

      ImGui::SameLine        ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (1.0f, 1.0f, 1.0f));
      ImGui::Text            ("%ws",                SK_D3D9_FormatToStr (pparams.BackBufferFormat).c_str       ());
      ImGui::Text            ("%ws",                SK_D3D9_FormatToStr (pparams.AutoDepthStencilFormat).c_str ());
      ImGui::Text            ("%ux%u",                                   pparams.BackBufferWidth, pparams.BackBufferHeight);
      ImGui::Text            ("%u",                                      pparams.BackBufferCount);
      if (! pparams.Windowed)
      ImGui::Text            ("%u Hz",                                   pparams.FullScreen_RefreshRateInHz);
      if (pparams.PresentationInterval == 0)
        ImGui::Text          ("%u: VSYNC OFF",                           pparams.PresentationInterval);
      else if (pparams.PresentationInterval == 1)
        ImGui::Text          ("%u: Normal V-SYNC",                       pparams.PresentationInterval);
      else if (pparams.PresentationInterval == 2)
        ImGui::Text          ("%u: 1/2 Refresh V-SYNC",                  pparams.PresentationInterval);
      else if (pparams.PresentationInterval == 3)
        ImGui::Text          ("%u: 1/3 Refresh V-SYNC",                  pparams.PresentationInterval);
      else if (pparams.PresentationInterval == 4)
        ImGui::Text          ("%u: 1/4 Refresh V-SYNC",                  pparams.PresentationInterval);
      else
        ImGui::Text          ("%u: UNKNOWN or Invalid",                  pparams.PresentationInterval);
      ImGui::Text            ("%ws",            SK_D3D9_SwapEffectToStr (pparams.SwapEffect).c_str ());
      ImGui::Text            ("%u",                                      pparams.MultiSampleType);
      if (pparams.Flags != 0)
      ImGui::Text            ("%ws", SK_D3D9_PresentParameterFlagsToStr (pparams.Flags).c_str ()) ;
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();
      ImGui::EndTooltip      ();
    }
  }
}

void
SK_ImGui_SummarizeDXGISwapchain (IDXGISwapChain* pSwapDXGI)
{
  if (pSwapDXGI != nullptr)
  {
    DXGI_SWAP_CHAIN_DESC swap_desc = { };

    if (SUCCEEDED (pSwapDXGI->GetDesc (&swap_desc)))
    {
      SK_RenderBackend& rb =
        SK_GetCurrentRenderBackend ();

      auto* pDevCtx =
        static_cast <ID3D11DeviceContext *> (rb.d3d11.immediate_ctx);

      // This limits us to D3D11 for now, but who cares -- D3D10 sucks and D3D12 can't be drawn to yet :)
      if (! pDevCtx)
        return;

      ImGui::BeginTooltip    ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (0.95f, 0.95f, 0.45f));
      ImGui::TextUnformatted ("Framebuffer and Presentation Setup");
      ImGui::PopStyleColor   ();
      ImGui::Separator       ();

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (0.785f, 0.785f, 0.785f));
      ImGui::TextUnformatted ("Color:");
    //ImGui::TextUnformatted ("Depth/Stencil:");
      ImGui::TextUnformatted ("Resolution:");
      ImGui::TextUnformatted ("Back Buffers:");
      if ((! swap_desc.Windowed) && swap_desc.BufferDesc.Scaling          != DXGI_MODE_SCALING_UNSPECIFIED)
      ImGui::TextUnformatted ("Scaling Mode:");
      if ((! swap_desc.Windowed) && swap_desc.BufferDesc.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
      ImGui::TextUnformatted ("Scanlines:");
      if (! swap_desc.Windowed && swap_desc.BufferDesc.RefreshRate.Denominator != 0)
      ImGui::TextUnformatted ("Refresh Rate:");
      ImGui::TextUnformatted ("Swap Interval:");
      ImGui::TextUnformatted ("Swap Effect:");
      if  (swap_desc.SampleDesc.Count > 1)
      ImGui::TextUnformatted ("MSAA Samples:");
      if (swap_desc.Flags != 0)
      ImGui::TextUnformatted ("Flags:");
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();

      ImGui::SameLine        ();

      

      ImGui::BeginGroup      ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (1.0f, 1.0f, 1.0f));
      ImGui::Text            ("%ws",                SK_DXGI_FormatToStr (swap_desc.BufferDesc.Format).c_str ());
    //ImGui::Text            ("%ws",                SK_DXGI_FormatToStr (dsv_desc.Format).c_str             ());
      ImGui::Text            ("%ux%u",                                   swap_desc.BufferDesc.Width, swap_desc.BufferDesc.Height);
      ImGui::Text            ("%u",                                      std::max (1UL, swap_desc.Windowed ? swap_desc.BufferCount : swap_desc.BufferCount - 1UL));
      if ((! swap_desc.Windowed) && swap_desc.BufferDesc.Scaling          != DXGI_MODE_SCALING_UNSPECIFIED)
      ImGui::Text            ("%ws",        SK_DXGI_DescribeScalingMode (swap_desc.BufferDesc.Scaling));
      if ((! swap_desc.Windowed) && swap_desc.BufferDesc.ScanlineOrdering != DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED)
      ImGui::Text            ("%ws",      SK_DXGI_DescribeScanlineOrder (swap_desc.BufferDesc.ScanlineOrdering));
      if (! swap_desc.Windowed && swap_desc.BufferDesc.RefreshRate.Denominator != 0)
      ImGui::Text            ("%.2f Hz",                                 static_cast <float> (swap_desc.BufferDesc.RefreshRate.Numerator) /
                                                                         static_cast <float> (swap_desc.BufferDesc.RefreshRate.Denominator));
      if (rb.present_interval == 0)
        ImGui::Text          ("%u: VSYNC OFF",                           rb.present_interval);
      else if (rb.present_interval == 1)
        ImGui::Text ("%u: Normal V-SYNC", rb.present_interval);
      else if (rb.present_interval == 2)
        ImGui::Text          ("%u: 1/2 Refresh V-SYNC",                  rb.present_interval);
      else if (rb.present_interval == 3)
        ImGui::Text          ("%u: 1/3 Refresh V-SYNC",                  rb.present_interval);
      else if (rb.present_interval == 4)
        ImGui::Text          ("%u: 1/4 Refresh V-SYNC",                  rb.present_interval);
      else
        ImGui::Text          ("%u: UNKNOWN or Invalid",                  0);//pparams.PresentationInterval);
      ImGui::Text            ("%ws",            SK_DXGI_DescribeSwapEffect (swap_desc.SwapEffect));
      if  (swap_desc.SampleDesc.Count > 1)
      ImGui::Text            ("%u",                                         swap_desc.SampleDesc.Count);
      if (swap_desc.Flags != 0)
        ImGui::Text          ("%ws",        SK_DXGI_DescribeSwapChainFlags (static_cast <DXGI_SWAP_CHAIN_FLAG> (swap_desc.Flags)).c_str ());
      ImGui::PopStyleColor   ();
      ImGui::EndGroup        ();
      ImGui::EndTooltip      ();

      pDevCtx->Release ();
    }
  }
}

void
SK_ImGui_NV_DepthBoundsD3D11 (void)
{
  static bool  enable = false;
  static float fMin   = 0.0f;
  static float fMax   = 1.0f;

  bool changed = false;

  changed |= ImGui::Checkbox ("Enable Depth Bounds Test", &enable);

  if (enable)
  {
    changed |= ImGui::SliderFloat ("fMinDepth", &fMin, 0.0f, fMax);
    changed |= ImGui::SliderFloat ("fMaxDepth", &fMax, fMin, 1.0f);
  }

  if (changed)
  {
    NvAPI_D3D11_SetDepthBoundsTest ( SK_GetCurrentRenderBackend ().device,
                                       enable ? 0x1 : 0x0,
                                         fMin, fMax );
  }
}

extern float target_fps;

void
SK_ImGui_DrawGraph_FramePacing (void)
{
  ImGuiIO& io (ImGui::GetIO ());

  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  float sum = 0.0f;

  float min = FLT_MAX;
  float max = 0.0f;

  for ( auto val : SK_ImGui_Frames.getValues () )
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

  ImGui::PopStyleColor ();
}

void
SK_ImGui_DrawTexCache_Chart (void)
{
  if (config.textures.d3d11.cache)
  {
    ImGui::PushStyleColor (ImGuiCol_Border, ImColor (245,245,245));
    ImGui::Separator (   );
    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Columns   ( 3 );
      ImGui::Text    ( "          Size" );                                                                 ImGui::NextColumn ();
      ImGui::Text    ( "      Activity" );                                                                 ImGui::NextColumn ();
      ImGui::Text    ( "       Savings" );
    ImGui::Columns   ( 1 );
    ImGui::PopStyleColor
                     (   );

    ImGui::PushStyleColor
                     ( ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.0f) );

    ImGui::Separator      ( );
    ImGui::PopStyleColor  (1);

    ImGui::PushStyleColor(ImGuiCol_Border, ImColor(0.5f, 0.5f, 0.5f, 0.666f));
    ImGui::Columns   ( 3 );
      ImGui::Text    ( "%#7zu      MiB",
                                                     SK_D3D11_Textures.AggregateSize_2D >> 20ULL );     ImGui::NextColumn (); 
       ImGui::TextColored
                     (ImVec4 (0.3f, 1.0f, 0.3f, 1.0f),
                       "%#5lu      Hits",            SK_D3D11_Textures.RedundantLoads_2D.load () );     ImGui::NextColumn ();
       if (SK_D3D11_Textures.Budget != 0)
         ImGui::Text ( "Budget:  %#7zu MiB  ",       SK_D3D11_Textures.Budget / 1048576ULL );
    ImGui::Columns   ( 1 );

    ImGui::Separator (   );

    ImGui::Columns   ( 3 );
      ImGui::Text    ( "%#7zu Textures",             SK_D3D11_Textures.Entries_2D.load () );            ImGui::NextColumn ();
      ImGui::TextColored
                     ( ImVec4 (1.0f, 0.3f, 0.3f, 1.60f),
                       "%#5lu   Misses",             SK_D3D11_Textures.CacheMisses_2D.load () );        ImGui::NextColumn ();
     ImGui::Text   ( "Time:        %#7.01lf ms  ", SK_D3D11_Textures.RedundantTime_2D         );
    ImGui::Columns   ( 1 );

    ImGui::Separator (   );

    ImGui::Columns   ( 3 );  
      ImGui::Text    ( "%#6lu   Evictions",            SK_D3D11_Textures.Evicted_2D.load ()   );        ImGui::NextColumn ();
      ImGui::TextColored (ImColor::HSV (std::min ( 0.4f * (float)SK_D3D11_Textures.RedundantLoads_2D / 
                                                          (float)SK_D3D11_Textures.CacheMisses_2D, 0.4f ), 0.95f, 0.8f),
                       " %.2f  Hit/Miss",                  (double)SK_D3D11_Textures.RedundantLoads_2D / 
                                                           (double)SK_D3D11_Textures.CacheMisses_2D  ); ImGui::NextColumn ();
      ImGui::Text    ( "Driver I/O: %#7llu MiB  ",     SK_D3D11_Textures.RedundantData_2D >> 20ULL );

    ImGui::Columns   ( 1 );

    ImGui::Separator (   );

    ImGui::PopStyleColor ( );

    int size = config.textures.cache.max_size;

    ImGui::TreePush  ( "" );
    if (ImGui::SliderInt ( "Cache Size (GPU-shared memory)", &size, 256, 8192, "%.0f MiB"))
      config.textures.cache.max_size = size;
      SK_GetCommandProcessor ()->ProcessCommandFormatted ("TexCache.MaxSize %d ", config.textures.cache.max_size);
    ImGui::TreePop   (    );

    ImGui::PopStyleColor ( );
  }
}

void
SK_ImGui_VolumeManager (void)
{
  ImGuiIO& io (ImGui::GetIO ());

  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  std::string app_name;

  sessions.Activate ();

  CComPtr <IAudioMeterInformation> pMeterInfo =
    sessions.getMeterInfo ();

  if (pMeterInfo == nullptr)
    audio_session = nullptr;

  if (audio_session != nullptr)
    app_name = audio_session->getName ();

  app_name += "###AudioSessionAppName";

  ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.90f, 0.68f, 0.45f));
  ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.90f, 0.72f, 0.80f));
  ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.87f, 0.78f, 0.80f));

  ImGui::Columns (2, "Amazon Sep", false);

  bool selected = true;
  if (ImGui::Selectable (app_name.c_str (), &selected))
    ImGui::OpenPopup ("Audio Session Selector");

  ImGui::PopStyleColor (3);

  if (ImGui::IsItemHovered ())
    ImGui::SetTooltip ("Click Here to Manage Another Application");

  ImGui::SetColumnOffset (1, 530 * io.FontGlobalScale);
  ImGui::NextColumn      (                           );

  // Send both keydown and keyup events because software like Amazon Music only responds to keyup
  ImGui::BeginGroup ();
  {
    ImGui::PushItemWidth (-1);
    if (ImGui::Button (u8"  <<  "))
    {
      ISteamMusic* pMusic =
        SK_SteamAPI_Music ();

      if (pMusic && pMusic->BIsEnabled ())
      {
        if (pMusic->BIsPlaying ()) pMusic->PlayPrevious ();
      }

      keybd_event_Original (VK_MEDIA_PREV_TRACK, 0, KEYEVENTF_EXTENDEDKEY,                   0);
      keybd_event_Original (VK_MEDIA_PREV_TRACK, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }

    ImGui::SameLine ();

    if (ImGui::Button (u8"  Play / Pause  "))
    {
      ISteamMusic* pMusic =
        SK_SteamAPI_Music ();

      if (pMusic && pMusic->BIsEnabled ())
      {
        if (pMusic->BIsPlaying ()) pMusic->Pause ();
        else                       pMusic->Play  ();
      }


      keybd_event_Original (VK_MEDIA_PLAY_PAUSE, 0, KEYEVENTF_EXTENDEDKEY,                   0);
      keybd_event_Original (VK_MEDIA_PLAY_PAUSE, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0); 
    }

    ImGui::SameLine ();

    if (ImGui::Button (u8"  >>  "))
    {
      ISteamMusic* pMusic =
        SK_SteamAPI_Music ();

      if (pMusic && pMusic->BIsEnabled ())
      {
        if (pMusic->BIsPlaying ()) pMusic->PlayNext ();
      }


      keybd_event_Original (VK_MEDIA_NEXT_TRACK, 0, KEYEVENTF_EXTENDEDKEY,                   0);
      keybd_event_Original (VK_MEDIA_NEXT_TRACK, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }
    ImGui::PopItemWidth ();
  }
  ImGui::EndGroup   ();
  ImGui::Columns    (1);


  bool session_changed = SK_ImGui_SelectAudioSessionDlg ();

  ImGui::TreePush ("");

  if (audio_session == nullptr)
  {
    int count;

    SK_WASAPI_AudioSession** ppSessions =
      sessions.getActive (&count);

    // Find the session for the current game and select that first...
    for (int i = 0; i < count; i++)
    {
      if (ppSessions [i]->getProcessId () == GetCurrentProcessId ())
      {
        audio_session = ppSessions [i];
        break;
      }
    }
  }

  else
  {
    CComPtr <IChannelAudioVolume> pChannelVolume =
      audio_session->getChannelAudioVolume ();
    CComPtr <ISimpleAudioVolume>  pVolume        =
      audio_session->getSimpleAudioVolume ();

    UINT channels = 0;

    if (SUCCEEDED (pMeterInfo->GetMeteringChannelCount (&channels)))
    {
      static float channel_peaks_ [32] { };

      struct
      {
        struct {
          float inst_min = FLT_MAX;  DWORD dwMinSample = 0;  float disp_min = FLT_MAX;
          float inst_max = FLT_MIN;  DWORD dwMaxSample = 0;  float disp_max = FLT_MIN;
        } vu_peaks;

        float peaks    [120] { };
        int   current_idx =   0;
      } static history [ 32] { };

      #define VUMETER_TIME 333

      static float master_vol  = -1.0f; // Master Volume
      static BOOL  master_mute =  FALSE;

      struct volume_s
      {
        float volume           =  1.0f; // Current Volume (0.0 when muted)
        float normalized       =  1.0f; // Unmuted Volume (stores volume before muting)

        bool  muted            = false;

        // Will fill-in with unique names for ImGui
        //   (all buttons say the same thing =P)
        //
        char mute_button  [14] = { };
        char slider_label [8 ] = { };
      };

      static std::map <int, volume_s> channel_volumes;

      pVolume->GetMasterVolume (&master_vol);
      pVolume->GetMute         (&master_mute);

      const char* szMuteButtonTitle = ( master_mute ? "  Unmute  ###MasterMute" :
                                                      "   Mute   ###MasterMute" );

      if (ImGui::Button (szMuteButtonTitle))
      {
        master_mute ^= TRUE;

        pVolume->SetMute (master_mute, nullptr);
      }

      ImGui::SameLine ();

      float val = master_mute ? 0.0f : 1.0f;

      ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor ( 0.3f,  0.3f,  0.3f,  val));
      ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor ( 0.6f,  0.6f,  0.6f,  val));
      ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor ( 0.9f,  0.9f,  0.9f,  val));
      ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImColor ( 1.0f,  1.0f,  1.0f, 1.0f));
      ImGui::PushStyleColor (ImGuiCol_Text,           ImColor::HSV ( 0.15f, 0.0f,
                                                                       0.5f + master_vol * 0.5f) );

      if (ImGui::SliderFloat ("      Master Volume Control  ", &master_vol, 0.0, 1.0, ""))
      {
        if (master_mute)
        {
                            master_mute = FALSE;
          pVolume->SetMute (master_mute, nullptr);
        }

        pVolume->SetMasterVolume (master_vol, nullptr);
      }

      ImGui::SameLine ();

      ImGui::TextColored ( ImColor::HSV ( 0.15f, 0.9f,
                                            0.5f + master_vol * 0.5f),
                             "(%03.1f%%)  ",
                               master_vol * 100.0f );

      ImGui::PopStyleColor (5);
      ImGui::Separator     ( );
      ImGui::Columns       (2);

      for (UINT i = 0 ; i < channels; i++)
      {
        if (SUCCEEDED (pMeterInfo->GetChannelsPeakValues (channels, channel_peaks_)))
        {
          history [i].vu_peaks.inst_min = std::min (history [i].vu_peaks.inst_min, channel_peaks_ [i]);
          history [i].vu_peaks.inst_max = std::max (history [i].vu_peaks.inst_max, channel_peaks_ [i]);

          history [i].vu_peaks.disp_min      = history [i].vu_peaks.inst_min;

          if (history [i].vu_peaks.dwMinSample < timeGetTime () - VUMETER_TIME * 3)
          {
            history [i].vu_peaks.inst_min    = channel_peaks_ [i];
            history [i].vu_peaks.dwMinSample = timeGetTime ();
          }

          history [i].vu_peaks.disp_max      = history [i].vu_peaks.inst_max;

          if (history [i].vu_peaks.dwMaxSample < timeGetTime () - VUMETER_TIME * 3)
          {
            history [i].vu_peaks.inst_max    = channel_peaks_ [i];
            history [i].vu_peaks.dwMaxSample = timeGetTime ();
          }

          history [i].peaks [history [i].current_idx] = channel_peaks_ [i];

          if (i & 0x1)
          {
            history [i].current_idx          = (history [i].current_idx - 1);

            if (history [i].current_idx < 0)
                history [i].current_idx = IM_ARRAYSIZE (history [i].peaks) - 1;
          }

          else
          {
            history [i].current_idx           = (history [i].current_idx + 1) % IM_ARRAYSIZE (history [i].peaks);
          }

          ImGui::BeginGroup ();

          const float ht = font_size * 6;

          if (channel_volumes.count (i) == 0)
          {
            session_changed = true;

            snprintf (channel_volumes [i].mute_button, 13, "  Mute  ##%lu", i);
            snprintf (channel_volumes [i].slider_label, 7, "##vol%lu",      i);
          }

          if (pChannelVolume && SUCCEEDED (pChannelVolume->GetChannelVolume (i, &channel_volumes [i].volume)))
          {
            volume_s& ch_vol =
              channel_volumes [i];

            if (session_changed)
            {
              channel_volumes [i].muted      = (channel_volumes [i].volume <= 0.001f);
              channel_volumes [i].normalized = (channel_volumes [i].volume  > 0.001f ?
                                                channel_volumes [i].volume : 1.0f);
            }

            bool  changed     = false;
            float volume_orig = ch_vol.normalized;

            ImGui::BeginGroup  ();
            ImGui::TextColored (ImVec4 (0.80f, 0.90f, 1.0f,  1.0f), "%-22s", SK_WASAPI_GetChannelName (i));
                                                                                      ImGui::SameLine ( );
            ImGui::TextColored (ImVec4 (0.7f,  0.7f,  0.7f,  1.0f), "      Volume:"); ImGui::SameLine ( );
            ImGui::TextColored (ImColor::HSV (
                                        0.25f, 0.9f,
                                          0.5f + ch_vol.volume * 0.5f),
                                                                    "%03.1f%%   ", 100.0f * ch_vol.volume);
            ImGui::EndGroup    ();

            ImGui::BeginGroup  ();

                   val        = ch_vol.muted ? 0.1f : 0.5f;
            float *pSliderVal = ch_vol.muted ? &ch_vol.normalized : &ch_vol.volume;

            ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor::HSV ( ( i + 1 ) / (float)channels, 0.5f, val));
            ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor::HSV ( ( i + 1 ) / (float)channels, 0.6f, val));
            ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor::HSV ( ( i + 1 ) / (float)channels, 0.7f, val));
            ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImColor::HSV ( ( i + 1 ) / (float)channels, 0.9f, 0.9f));

            changed =
              ImGui::VSliderFloat ( ch_vol.slider_label,
                                      ImVec2 (font_size * 1.5f, ht),
                                        pSliderVal,
                                          0.0f, 1.0f,
                                            "" );

            ImGui::PopStyleColor  (4);

            if (changed)
            {
               ch_vol.volume =
                 ch_vol.muted  ? ch_vol.normalized :
                                 ch_vol.volume;

              if ( SUCCEEDED ( pChannelVolume->SetChannelVolume ( i,
                                                                    ch_vol.volume,
                                                                      nullptr )
                             )
                 )
              {
                ch_vol.muted      = false;
                ch_vol.normalized = ch_vol.volume;
              }

              else
                ch_vol.normalized = volume_orig;
            }

            if ( SK_ImGui::VerticalToggleButton (  ch_vol.mute_button,
                                                  &ch_vol.muted )
               )
            {
              if (ch_vol.muted)
              {
                ch_vol.normalized = ch_vol.volume;
                pChannelVolume->SetChannelVolume (i, 0.0f,              nullptr);
              }
              else
                pChannelVolume->SetChannelVolume (i, ch_vol.normalized, nullptr);
            }

            ImGui::EndGroup ();
            ImGui::SameLine ();
          }

          ImGui::BeginGroup ();
          ImGui::PushStyleColor (ImGuiCol_PlotHistogram, ImColor::HSV ( ( i + 1 ) / (float)channels, 1.0f, val));
          ImGui::PlotHistogram ( "",
                                   history [i].peaks,
                                     IM_ARRAYSIZE (history [i].peaks),
                                       history [i].current_idx,
                                         "",
                                              history [i].vu_peaks.disp_min,
                                              1.0f,
                                               ImVec2 (ImGui::GetContentRegionAvailWidth (), ht) );
          ImGui::PopStyleColor  ();

          ImGui::PushStyleColor (ImGuiCol_PlotHistogram,        ImVec4 (0.9f, 0.1f, 0.1f, 0.15f));
          ImGui::ProgressBar    (history [i].vu_peaks.disp_max, ImVec2 (-1.0f, 0.0f));
          ImGui::PopStyleColor  ();

          ImGui::ProgressBar    (channel_peaks_ [i],            ImVec2 (-1.0f, 0.0f));

          ImGui::PushStyleColor (ImGuiCol_PlotHistogram,        ImVec4 (0.1f, 0.1f, 0.9f, 0.15f));
          ImGui::ProgressBar    (history [i].vu_peaks.disp_min, ImVec2 (-1.0f, 0.0f));
          ImGui::PopStyleColor  ();
          ImGui::EndGroup       ();
          ImGui::EndGroup       ();

          if (! (i % 2))
          {
            ImGui::SameLine (); ImGui::NextColumn ();
          } else {
            ImGui::Columns   ( 1 );
            ImGui::Separator (   );
            ImGui::Columns   ( 2 );
          }
        }
      }

      ImGui::Columns (1);
    }

    // Upon failure, deactivate and try to get a new session manager on the next
    //   frame.
    else
    {
      audio_session = nullptr;
      sessions.Deactivate ();
    }
  }
}

__declspec (dllexport)
bool
SK_ImGui_ControlPanel (void)
{
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
        bool selected = false;

        static HMODULE hModReShade      = SK_ReShade_GetDLL ();
        static bool    bIsReShadeCustom =
                          ( hModReShade != nullptr &&

        SK_RunLHIfBitness ( 64, GetProcAddress (hModReShade, "?SK_ImGui_DrawCallback@@YAIPEAX@Z"),
                                GetProcAddress (hModReShade, "?SK_ImGui_DrawCallback@@YGIPAX@Z" ) ) );

        if (ImGui::MenuItem ( "Browse Logs", "", &selected ))
        {
          std::wstring log_dir =
            std::wstring (SK_GetConfigPath ()) + std::wstring (L"\\logs");

          ShellExecuteW (GetActiveWindow (), L"explore", log_dir.c_str (), nullptr, nullptr, SW_NORMAL);
        }

        if (bIsReShadeCustom && ImGui::MenuItem ( "Browse ReShade Assets", "", nullptr ))
        {
          std::wstring reshade_dir =
            std::wstring (SK_GetConfigPath ()) + std::wstring (L"\\ReShade");

          static bool dir_exists =
            PathIsDirectory ( std::wstring (
                                std::wstring ( SK_GetConfigPath () ) +
                                  L"\\ReShade\\Shaders\\"
                              ).c_str () );

          if (! dir_exists)
          {
            SK_CreateDirectories ( std::wstring (
                                     std::wstring ( SK_GetConfigPath () ) +
                                       L"\\ReShade\\Shaders\\"
                                   ).c_str () );
            SK_CreateDirectories ( std::wstring (
                                     std::wstring ( SK_GetConfigPath () ) +
                                       L"\\ReShade\\Textures\\"
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
              }

              if (ImGui::IsItemHovered ())
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
          ImGui::MenuItem  ("Current Branch###Menu_CurrentBranch", current_branch_str, &selected, false);

        ImGui::Separator ();

        if (vinfo.build < vinfo_latest.build)
        {
          if (ImGui::MenuItem  ("Update Now"))
            SK_UpdateSoftware (nullptr);

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip   ();
            ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.8f, 0.1f, 1.0f));
            ImGui::Text           ("DO NOT DO THIS IN FULLSCREEN EXCLUSIVE MODE");
            ImGui::PopStyleColor  ();
            ImGui::Separator      ();
            ImGui::BulletText     ("In fact you should not do this at all if you can help it");
            ImGui::BulletText     ("Restart the software and let it do the update at startup for best results.");
            ImGui::EndTooltip     ();
          }
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
    ImGui::PushStyleColor ( ImGuiCol_Text, ImColor::HSV ( (float)(timeGetTime () % 2800) / 2800.0f, 
                                            (0.5f + (sin ((float)(timeGetTime () % 500)  / 500.0f)) * 0.5f) / 2.0f,
                                             1.0f ) );
  else
    ImGui::PushStyleColor ( ImGuiCol_Text, ImColor (255, 255, 255) );
  ImGui::Begin            ( szTitle, &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                       (config.imgui.use_mac_style_menu ? 0x00: ImGuiWindowFlags_MenuBar ) );
  ImGui::PopStyleColor    ();

  style = orig;


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

    lstrcatA ( szAPIName, SK_GetBitness () == 32 ? "    [ 32-bit ]" :
                                                   "    [ 64-bit ]" );

    ImGui::MenuItem ("Active Render API        ", szAPIName);

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
      snprintf ( szResolution, 63, "   %lux%lu", 
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

    HDC hDC = GetWindowDC (game_window.hWnd);

    int res_x = GetDeviceCaps (hDC, HORZRES);
    int res_y = GetDeviceCaps (hDC, VERTRES);

    ReleaseDC (game_window.hWnd, hDC);

    if ( client.right - client.left   != res_x || client.bottom - client.top   != res_y ||
         io.DisplayFramebufferScale.x != res_x || io.DisplayFramebufferScale.y != res_y )
    {
      snprintf ( szResolution, 63, "   %ix%i",
                                     res_x,
                                       res_y );
    
      if (ImGui::MenuItem (" Device Resolution    ", szResolution))
      {
        config.window.res.override.x = res_x;
        config.window.res.override.y = res_y;

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
        strcat (szGSyncStatus, "Supported + ");
        if (rb.gsync_state.active)
          strcat (szGSyncStatus, "Active");
        else
          strcat (szGSyncStatus, "Inactive");
      }
      else
        strcat (szGSyncStatus, "Unsupported");

      ImGui::MenuItem (" G-Sync Status   ", szGSyncStatus);
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
    if (SK_GetCurrentGameID () == SK_GAME_ID::GalGun_Double_Peace)
    {
      if (ImGui::CollapsingHeader ("Gal*Gun: Double Peace", ImGuiTreeNodeFlags_DefaultOpen))
      {
        static bool emperor_has_no_clothes = false;

        ImGui::TreePush ("");

        if (ImGui::Checkbox ("The emperor of Japan has no clothes", &emperor_has_no_clothes))
        {
          const uint32_t ps_primary = 0x9b826e8a;
          const uint32_t vs_outline = 0x2e1993cf;

          if (emperor_has_no_clothes)
          {
            SK::D3D9::Shaders.vertex.blacklist.emplace (vs_outline);
            SK::D3D9::Shaders.pixel.blacklist.emplace  (ps_primary);
          }

          else
          {
            SK::D3D9::Shaders.vertex.blacklist.erase (vs_outline);
            SK::D3D9::Shaders.pixel.blacklist.erase  (ps_primary);
          }
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ( emperor_has_no_clothes ? "And neither do the girls in this game!" :
                                                       "But the prudes in this game do." );

        ImGui::TreePop ();
      }
    }

    if (SK_GetCurrentGameID () == SK_GAME_ID::LifeIsStrange_BeforeTheStorm)
    {
      if (ImGui::CollapsingHeader ("Life is Strange: Before the Storm", ImGuiTreeNodeFlags_DefaultOpen))
      {
        static bool evil          = false;
        static bool even_stranger = false;
        static bool wired         = false;

        const uint32_t vs_eyes = 0x223ccf2d;
        const uint32_t ps_face = 0xbde11248;
        const uint32_t ps_skin = 0xa79e425c;

        ImGui::TreePush ("");

        if (ImGui::Checkbox ("Life is Wired", &wired))
        {
          if (wired)
          {
            SK_D3D11_Shaders.pixel.wireframe.emplace (ps_skin);
            SK_D3D11_Shaders.pixel.wireframe.emplace (ps_face);
          }
        
          else
          {
            SK_D3D11_Shaders.pixel.wireframe.erase (ps_skin);
            SK_D3D11_Shaders.pixel.wireframe.erase (ps_face);
          }
        }
        
        if (ImGui::Checkbox ("Life is Evil", &evil))
        {
          if (evil)
          {
            SK_D3D11_Shaders.vertex.blacklist.emplace (vs_eyes);
          }
        
          else
          {
            SK_D3D11_Shaders.vertex.blacklist.erase (vs_eyes);
          }
        }
        
        if (ImGui::Checkbox ("Life is Even Stranger", &even_stranger))
        {
          if (even_stranger)
          {
            SK_D3D11_Shaders.pixel.blacklist.emplace (ps_face);
            SK_D3D11_Shaders.pixel.blacklist.emplace (ps_skin);
          }
        
          else
          {
            SK_D3D11_Shaders.pixel.blacklist.erase (ps_face);
            SK_D3D11_Shaders.pixel.blacklist.erase (ps_skin);
          }
        }

        //bool enable = evil || even_stranger || wired;
        //
        //extern void
        //SK_D3D11_EnableTracking (bool state);
        //SK_D3D11_EnableTracking (enable || show_shader_mod_dlg);

        ImGui::TreePop ();
      }
    }
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

          if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("CTRL + Click to Enter an Exact Framerate");

          ImGui::SameLine ();

          advanced = ImGui::TreeNode ("Advanced ###Advanced_FPS");

          if (advanced)
          {
            ImGui::TreePop    ();
            ImGui::BeginGroup ();

            if (target_fps > 0.0f)
            {
              ImGui::SliderFloat ( "Target Framerate Tolerance", &config.render.framerate.limiter_tolerance, 0.005f, 0.75);
           
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
                ImGui::EndTooltip     ( );
              }
            }

            changed |= ImGui::Checkbox ("Sleepless Render Thread",         &config.render.framerate.sleepless_render  );
                    if (ImGui::IsItemHovered ())
                    {
                      SK::Framerate::EventCounter::SleepStats& stats =
                        SK::Framerate::GetEvents ()->getRenderThreadStats ();

                        ImGui::SetTooltip
                                       ( "(%llu ms asleep, %llu ms awake)",
                                           /*(stats.attempts - stats.rejections), stats.attempts,*/
                                             InterlockedAdd64 (&stats.time.allowed,  0),
                                             InterlockedAdd64 (&stats.time.deprived, 0) );
                      }
                       ImGui::SameLine (                                                                              );

            changed |= ImGui::Checkbox ("Sleepless Window Thread", &config.render.framerate.sleepless_window );
                    if (ImGui::IsItemHovered ())
                    {
                      SK::Framerate::EventCounter::SleepStats& stats =
                        SK::Framerate::GetEvents ()->getMessagePumpStats ();

                        ImGui::SetTooltip
                                       ( "(%llu ms asleep, %llu ms awake)",
                                           /*(stats.attempts - stats.rejections), stats.attempts,*/
                                             InterlockedAdd64 (&stats.time.allowed,  0),
                                             InterlockedAdd64 (&stats.time.deprived, 0) );
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

    SK_RenderAPI api = rb.api;

    if ( static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D9) &&
         ImGui::CollapsingHeader ("Direct3D 9 Settings", ImGuiTreeNodeFlags_DefaultOpen) )
    {
      ImGui::TreePush ("");

      if (ImGui::Button ("  D3D9 Render Mod Tools  "))
        show_d3d9_shader_mod_dlg ^= 1;

      ImGui::SameLine ();

      ImGui::Checkbox ("Enable Texture Modding (Experimental)", &config.textures.d3d9_mod);

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("Requires a game restart.");
      }

      if (config.textures.d3d9_mod)
      {
      ImGui::TreePush ("");
      if (ImGui::CollapsingHeader ("Texture Memory Stats", ImGuiTreeNodeFlags_DefaultOpen))
      {
        extern bool __remap_textures;

        ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 15.0f);
        ImGui::TreePush     ("");

        ImGui::BeginChild  ("Texture Details", ImVec2 ( font_size           * 30,
                                                        font_size_multiline * 6.0f ),
                                                 true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

        ImGui::Columns   ( 3 );
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
          ImGui::Text    ( "          Size" );                                                                 ImGui::NextColumn ();
          ImGui::Text    ( "      Activity" );                                                                 ImGui::NextColumn ();
          ImGui::Text    ( "       Savings" );
          ImGui::PopStyleColor  ();
        ImGui::Columns   ( 1 );

        ImGui::PushStyleColor
                         ( ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.0f) );

        ImGui::Separator (   );

      //ImGui::PushFont  (ImGui::GetIO ().Fonts->Fonts [1]);
        ImGui::Columns   ( 3 );
          ImGui::Text    ( "%#6zu MiB Total",
                                                         SK::D3D9::tex_mgr.cacheSizeTotal () >> 20ULL ); ImGui::NextColumn ();

          static DWORD  dwLastVRAMUpdate   = 0UL;
                 DWORD  dwNow              = timeGetTime ();
          static size_t d3d9_tex_mem_avail = 0UL;

          if (dwLastVRAMUpdate < dwNow - 1500)
          {
            d3d9_tex_mem_avail =
              static_cast <IDirect3DDevice9 *>(SK_GetCurrentRenderBackend ().device)->GetAvailableTextureMem () / 1048576UL;
            dwLastVRAMUpdate = dwNow;
          }

          ImGui::TextColored
                         (ImVec4 (0.3f, 1.0f, 0.3f, 1.0f),
                           "%#5lu     Hits",             SK::D3D9::tex_mgr.getHitCount    ()          ); ImGui::NextColumn ();
          ImGui::Text       ( "Budget: %#7zu MiB  ",     d3d9_tex_mem_avail );
        ImGui::Columns   ( 1 );

        ImGui::Separator (   );

        ImColor  active   ( 1.0f,  1.0f,  1.0f, 1.0f);
        ImColor  inactive (0.75f, 0.75f, 0.75f, 1.0f);
        ImColor& color   = __remap_textures ? inactive : active;

        ImGui::PushStyleColor (ImGuiCol_Text, color);

        bool selected = (! __remap_textures);

        ImGui::Columns   ( 3 );
          ImGui::Selectable  ( SK_FormatString ( "%#6zu MiB Base###D3D9_BaseTextures",
                                                   SK::D3D9::tex_mgr.cacheSizeBasic () >> 20ULL ).c_str (),
                                 &selected ); ImGui::NextColumn ();

          ImGui::PopStyleColor ();

          if (SK_ImGui_IsItemClicked ())
            __remap_textures = false;
          if ((! selected) && (ImGui::IsItemHovered () || ImGui::IsItemFocused ()))
            ImGui::SetTooltip ("Click here to use the game's original textures.");

          ImGui::TextColored
                         (ImVec4 (1.0f, 0.3f, 0.3f, 1.0f),
                           "%#5lu   Misses",             SK::D3D9::tex_mgr.getMissCount   ()          );  ImGui::NextColumn ();
          ImGui::Text    ( "Time:    %#7.03lf  s  ",     SK::D3D9::tex_mgr.getTimeSaved   () / 1000.0f);
        ImGui::Columns   ( 1 );

        ImGui::Separator (   );

        color    = __remap_textures ? active : inactive;
        selected = __remap_textures;

        ImGui::PushStyleColor (ImGuiCol_Text, color);

        ImGui::Columns   ( 3 );
          ImGui::Selectable  ( SK_FormatString ( "%#6zu MiB Injected###D3D9_InjectedTextures",
                                                             SK::D3D9::tex_mgr.cacheSizeInjected () >> 20ULL ).c_str (),
                                           &selected ); ImGui::NextColumn ();

          ImGui::PopStyleColor ();

          if (SK_ImGui_IsItemClicked ())
            __remap_textures = true;
          if ((! selected) && (ImGui::IsItemHovered () || ImGui::IsItemFocused ()))
            ImGui::SetTooltip ("Click here to use custom textures.");

          ImGui::TextColored (ImColor::HSV (std::min ( 0.4f * (float)SK::D3D9::tex_mgr.getHitCount  ()   / 
                                                              (float)SK::D3D9::tex_mgr.getMissCount (), 0.4f ), 0.98f, 1.0f),
                           "%.2f  Hit/Miss",          (double)SK::D3D9::tex_mgr.getHitCount  () / 
                                                      (double)SK::D3D9::tex_mgr.getMissCount ()          ); ImGui::NextColumn ();
          ImGui::Text    ( "Driver: %#7zu MiB  ",    SK::D3D9::tex_mgr.getByteSaved          () >> 20ULL );

        ImGui::PopStyleColor
                         (   );
        ImGui::Columns   ( 1 );
        ImGui::SliderInt ("Cache Size (in MiB)###D3D9_TexCache_Size_MiB", &config.textures.cache.max_size, 256, 4096);
      //ImGui::PopFont   (   );
        ImGui::EndChild  (   );

#if 0
        if (ImGui::CollapsingHeader ("Thread Stats"))
        {
          std::vector <SK::D3D9::TexThreadStats> stats =
            SK::D3D9::tex_mgr.getThreadStats ();

          int thread_id = 0;

          for (auto&& it : stats)
          {
            ImGui::Text ("Thread #%lu  -  %6lu jobs retired, %5lu MiB loaded  -  %.6f User / %.6f Kernel / %3.1f Idle",
                         thread_id++,
                         it.jobs_retired, it.bytes_loaded >> 20UL,
                         (double)ULARGE_INTEGER
            {
              it.runtime.user.dwLowDateTime, it.runtime.user.dwHighDateTime
            }.QuadPart / 10000000.0,
                         (double)ULARGE_INTEGER
            {
              it.runtime.kernel.dwLowDateTime, it.runtime.kernel.dwHighDateTime
            }.QuadPart / 10000000.0,
                (double)ULARGE_INTEGER
              {
                it.runtime.idle.dwLowDateTime, it.runtime.idle.dwHighDateTime
              }.QuadPart / 10000000.0);
          }
        }
#endif

        ImGui::TreePop     ();
        ImGui::PopStyleVar ();
      }
      ImGui::TreePop ();
      }

      ImGui::TreePop ();
    }

    if ( static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11) &&
         ImGui::CollapsingHeader ("Direct3D 11 Settings", ImGuiTreeNodeFlags_DefaultOpen) )
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
      ImGui::TreePush       ("");

      ////ImGui::Checkbox ("Overlay Compatibility Mode", &SK_DXGI_SlowStateCache);

      ////if (ImGui::IsItemHovered ())
        ////ImGui::SetTooltip ("Increased compatibility with video capture software")

      extern BOOL SK_DXGI_SupportsTearing (void);

      bool swapchain =
        ImGui::CollapsingHeader ("SwapChain Management");

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip   ();
        ImGui::TextColored    ( ImColor (235, 235, 235),
                                "Highly Advanced Render Tweaks" );
        ImGui::Separator      ();
        ImGui::BulletText     ("Altering these settings may require manual INI edits to recover from");
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.85f, 0.1f, 0.9f));
        ImGui::BulletText     ("For best results, consult your nearest Kaldaien ;)");
        ImGui::PopStyleColor  ();
        ImGui::EndTooltip     ();
      }

      if (swapchain)
      {
        static bool flip         = config.render.framerate.flip_discard;
        static bool waitable     = config.render.framerate.swapchain_wait > 0;
        static int  buffer_count = config.render.framerate.buffer_count;

        ImGui::TreePush ("");

        ImGui::Checkbox ("Use Flip Model Presentation", &config.render.framerate.flip_discard);
        ImGui::InputInt ("Presentation Interval",       &config.render.framerate.present_interval);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();

          if (! config.render.framerate.flip_discard)
          {
            ImGui::Text       ("In Regular Presentation, this Controls V-Sync");
            ImGui::Separator  (                                               );
            ImGui::BulletText ("-1=Game Controlled,  0=Force Off,  1=Force On");
          }

          else
          {
            ImGui::Text       ("In Flip Model, this Controls Frame Queuing Rather than V-Sync)");
            ImGui::Separator  (                                                                );
            ImGui::BulletText ("Values > 1 will disable G-Sync but will produce the most "
                               "consistent frame rates possible."                              );
          }

          ImGui::EndTooltip ();
        }

        config.render.framerate.present_interval =
          std::max (-1, std::min (4, config.render.framerate.present_interval));

        ImGui::InputInt ("BackBuffer Count",       &config.render.framerate.buffer_count);

        // Clamp to [-1,oo)
        if (config.render.framerate.buffer_count < -1)
          config.render.framerate.buffer_count = -1;

        ImGui::InputInt ("Maximum Device Latency", &config.render.framerate.pre_render_limit);

        if (config.render.framerate.flip_discard)
        {
          bool waitable_ = config.render.framerate.swapchain_wait > 0;

          if (ImGui::Checkbox ("Waitable SwapChain", &waitable_))
          {
            if (! waitable_) config.render.framerate.swapchain_wait = 0;
            else             config.render.framerate.swapchain_wait = 15;
          }

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Reduces input latency, BUT makes it impossible to change resolution.");

          if (waitable) {
            ImGui::SliderInt ("Maximum Wait Period", &config.render.framerate.swapchain_wait, 1, 66);
          }

          if (SK_DXGI_SupportsTearing ())
          {
            bool tearing_pref = config.render.dxgi.allow_tearing;
            if (ImGui::Checkbox ("Enable DWM Tearing", &tearing_pref))
            {
              config.render.dxgi.allow_tearing = tearing_pref;
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip ();
              ImGui::Text         ("Enables tearing in windowed mode (Windows 10+); not particularly useful.");
              ImGui::EndTooltip   ();
            }
          }
        }

        bool changed = (flip         != config.render.framerate.flip_discard      ) ||
                       (waitable     != config.render.framerate.swapchain_wait > 0) ||
                       (buffer_count != config.render.framerate.buffer_count      );

        if (changed)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
          ImGui::BulletText     ("Game Restart Required");
          ImGui::PopStyleColor  ();
        }

        ImGui::TreePop  (  );
      }

      if (ImGui::CollapsingHeader ("Texture Management"))
      {
        static bool orig_cache = config.textures.d3d11.cache;

        ImGui::TreePush ("");
        ImGui::Checkbox ("Enable Texture Caching", &config.textures.d3d11.cache); 

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("Reduces Driver Memory Management Overhead in Games that Stream Textures");

          if (orig_cache)
          {
#if 0
            LONG contains_ = 0L;
            LONG erase_    = 0L;
            LONG index_    = 0L;

            std::pair <int, std::pair <LONG, std::pair <LONG, char*>>> busiest = { 0, { 0, { 0, "Invalid" } } };

            int idx = 0;

            for (auto it : SK_D3D11_Textures.HashMap_2D)
            {
              LONG i = ReadAcquire (&it.contention_score.index);
              LONG c = ReadAcquire (&it.contention_score.contains);
              LONG a = 0L;
              LONG e = ReadAcquire (&it.contention_score.erase);

              a = ( i + c + a + e );

              if ( idx > 0 && busiest.second.first < a )
              {
                busiest.first        = idx;
                busiest.second.first = a;

                LONG max_val =
                  std::max (i, std::max (c, e));

                if (max_val == i)
                  busiest.second.second.second = "operator []()";
                else if (max_val == c)
                  busiest.second.second.second = "contains ()";
                else if (max_val == e)
                  busiest.second.second.second = "erase ()";
                else
                  busiest.second.second.second = "unknown";

                busiest.second.second.first    = max_val;
              }

              ++idx;

              contains_ += c; erase_ += e; index_ += i;
            }

            if (busiest.first != 0)
            {
              ImGui::Separator  (                                 );
              ImGui::BeginGroup (                                 );
              ImGui::BeginGroup (                                 );
              ImGui::BulletText ( "HashMap Contains: "            );
              ImGui::BulletText ( "HashMap Erase:    "            );
              ImGui::BulletText ( "HashMap Index:    "            );
              ImGui::Text       ( ""                              );
              ImGui::BulletText ( "Most Contended:   "            );
              ImGui::EndGroup   (                                 );
              ImGui::SameLine   (                                 );
              ImGui::BeginGroup (                                 );
              ImGui::Text       ( "%li Ops", contains_            );
              ImGui::Text       ( "%li Ops", erase_               );
              ImGui::Text       ( "%li Ops", index_               );
              ImGui::Text       ( ""                              );
              ImGui::Text       ( R"(Mipmap LOD%02li (%li :: <"%s">))",
                                   busiest.first - 1,
                                     busiest.second.second.first,
                                     busiest.second.second.second );
              ImGui::EndGroup   (                                 );
              ImGui::EndGroup   (                                 );
              ImGui::SameLine   (                                 );
              ImGui::BeginGroup (                                 );
              int lod = 0;
              for ( auto& it : SK_D3D11_Textures.HashMap_2D )
              {
                ImGui::BulletText ("LOD %02lu Load Factor: ", lod++);
              }
              ImGui::EndGroup   (                                 );
              ImGui::SameLine   (                                 );
              ImGui::BeginGroup (                                 );
              for ( auto& it : SK_D3D11_Textures.HashMap_2D )
              {
                ImGui::Text     (" %.2f", it.entries.load_factor());
              }
              ImGui::EndGroup   (                                 );
            }
#endif
          }
          else
          {
            ImGui::Separator  (                                   );
            ImGui::BulletText ( "Requires Application Restart"    );
          }
          ImGui::EndTooltip   (                                   );
        }

        //ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.85f, 0.1f, 0.9f));
        //ImGui::SameLine (); ImGui::BulletText ("Requires restart");
        //ImGui::PopStyleColor  ();

        if (config.textures.d3d11.cache)
        {
          ImGui::SameLine ();
          ImGui::Spacing  ();
          ImGui::SameLine ();

          ImGui::Checkbox ("Ignore Textures Without Mipmaps", &config.textures.cache.ignore_nonmipped);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Important Compatibility Setting for Some Games (e.g. The Witcher 3)");

          ImGui::SameLine ();

          ImGui::Checkbox ("Cache Staged Texture Uploads", &config.textures.cache.allow_staging);

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Enable Texture Dumping and Injection in Unity-based Games");
            ImGui::Separator    ();
            ImGui::BulletText   ("May cause degraded performance.");
            ImGui::BulletText   ("Try to leave this off unless textures are missing from the mod tools.");
            ImGui::EndTooltip   ();
          }
        }

        ImGui::TreePop  ();

        SK_ImGui_DrawTexCache_Chart ();
      }

      static bool enable_resolution_limits = ! ( config.render.dxgi.res.min.isZero () && 
                                                 config.render.dxgi.res.max.isZero () );

      bool res_limits =
        ImGui::CollapsingHeader ("Resolution Limiting", enable_resolution_limits ? ImGuiTreeNodeFlags_DefaultOpen : 0x00);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Restrict the lowest/highest resolutions reported to a game");
        ImGui::Separator    ();
        ImGui::BulletText   ("Useful for games that compute aspect ratio based on the highest reported resolution.");
        ImGui::EndTooltip   ();
      }

      if (res_limits)
      {
        ImGui::TreePush  ("");
        ImGui::InputInt2 ("Minimum Resolution", reinterpret_cast <int *> (&config.render.dxgi.res.min.x));
        ImGui::InputInt2 ("Maximum Resolution", reinterpret_cast <int *> (&config.render.dxgi.res.max.x));
        ImGui::TreePop   ();
       }

      if (ImGui::Button (" Render Mod Tools "))
      {
        show_shader_mod_dlg = ( ! show_shader_mod_dlg );
      }

      ImGui::SameLine ();

#if 0
      if (ImGui::Button (" Re-Hook SwapChain Present "))
      {
        extern void
        SK_DXGI_HookPresent (IDXGISwapChain* pSwapChain, bool rehook = false);

        SK_DXGI_HookPresent ((IDXGISwapChain *)SK_GetCurrentRenderBackend ().swapchain, true);
      }

      ImGui::SameLine ();
#endif

      bool advanced =
        ImGui::TreeNode ("Advanced (Debug)###Advanced_NVD3D11");

      if (advanced)
      {
        ImGui::TreePop               ();
        ImGui::Separator             ();

        ImGui::Checkbox ("Enhanced (64-bit) Depth+Stencil Buffer", &config.render.dxgi.enhanced_depth);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Requires application restart");

        if (sk::NVAPI::nv_hardware)
        {
          ImGui::SameLine              ();
          SK_ImGui_NV_DepthBoundsD3D11 ();
        }
      }

      ImGui::TreePop       ( );
      ImGui::PopStyleColor (3);
    }

    if ( ImGui::CollapsingHeader ("Compatibility Settings") )
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));
      ImGui::TreePush       ("");

      if (ImGui::CollapsingHeader ("Third-Party Software"))
      {
        ImGui::TreePush ("");
        ImGui::Checkbox     ("Disable GeForce Experience and NVIDIA Shield", &config.compatibility.disable_nv_bloat);
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("May improve software compatibility, but disables ShadowPlay, couch co-op and various Shield-related functionality.");
        ImGui::TreePop  ();
      }

      if (ImGui::CollapsingHeader ("Render Backends", SK_IsInjected () ? ImGuiTreeNodeFlags_DefaultOpen : 0))
      {
        ImGui::TreePush ("");

        auto EnableActiveAPI =
        [ ](SK_RenderAPI api)
        {
          switch (api)
          {
            case SK_RenderAPI::D3D9Ex:
              config.apis.d3d9ex.hook     = true;
            case SK_RenderAPI::D3D9:
              config.apis.d3d9.hook       = true;
              break;

#ifdef _WIN64
            case SK_RenderAPI::D3D12:
              config.apis.dxgi.d3d12.hook = true;
#endif
            case SK_RenderAPI::D3D11:
              config.apis.dxgi.d3d11.hook = true;
              break;

#ifndef _WIN64
            case SK_RenderAPI::DDrawOn11:
              config.apis.ddraw.hook       = true;
              config.apis.dxgi.d3d11.hook  = true;
              break;

            case SK_RenderAPI::D3D8On11:
              config.apis.d3d8.hook        = true;
              config.apis.dxgi.d3d11.hook  = true;
              break;
#endif

            case SK_RenderAPI::OpenGL:
              config.apis.OpenGL.hook     = true;
              break;

#ifdef _WIN64
            case SK_RenderAPI::Vulkan:
              config.apis.Vulkan.hook     = true;
              break;
#endif
          }
        };

        using Tooltip_pfn = void (*)(void);

        auto ImGui_CheckboxEx =
        [ ]( const char* szName, bool* pVar,
                                 bool  enabled = true,
             Tooltip_pfn tooltip_disabled      = nullptr )
        {
          if (enabled)
          {
            ImGui::Checkbox (szName, pVar);
          }

          else
          {
            ImGui::TreePush     ("");
            ImGui::TextDisabled (szName);

            if (tooltip_disabled != nullptr)
              tooltip_disabled ();

            ImGui::TreePop      (  );

            *pVar = false;
          }
        };

#ifdef _WIN64
        const int num_lines = 4; // Basic set of APIs
#else
        const int num_lines = 5; // + DirectDraw / Direct3D 8
#endif

        ImGui::PushStyleVar                                                                          (ImGuiStyleVar_ChildWindowRounding, 10.0f);
        ImGui::BeginChild ("", ImVec2 (font_size * 39, font_size_multiline * num_lines * 1.1f), true, ImGuiWindowFlags_NavFlattened);

        ImGui::Columns    ( 2 );

        ImGui_CheckboxEx ("Direct3D 9",   &config.apis.d3d9.hook);
        ImGui_CheckboxEx ("Direct3D 9Ex", &config.apis.d3d9ex.hook, config.apis.d3d9.hook);

        ImGui::NextColumn (   );

        ImGui_CheckboxEx ("Direct3D 11",  &config.apis.dxgi.d3d11.hook);

#ifdef _WIN64
        ImGui_CheckboxEx ("Direct3D 12",  &config.apis.dxgi.d3d12.hook, config.apis.dxgi.d3d11.hook);
#endif

        ImGui::Columns    ( 1 );
        ImGui::Separator  (   );

#ifndef _WIN64
        ImGui::Columns    ( 2 );

        static bool has_dgvoodoo2 =
          GetFileAttributesA (
            SK_FormatString ( "%ws\\PlugIns\\ThirdParty\\dgVoodoo\\d3dimm.dll",
                                std::wstring ( SK_GetDocumentsDir () + L"\\My Mods\\SpecialK" ).c_str ()
                            ).c_str ()
          ) != INVALID_FILE_ATTRIBUTES;

        // Leaks memory, but who cares? :P
        static const char* dgvoodoo2_install_path =
          _strdup (
            SK_FormatString ( "%ws\\PlugIns\\ThirdParty\\dgVoodoo",
                    std::wstring ( SK_GetDocumentsDir () + L"\\My Mods\\SpecialK" ).c_str ()
                ).c_str ()
          );

        auto Tooltip_dgVoodoo2 = []
        {
          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
              ImGui::TextColored (ImColor (235, 235, 235), "Requires Third-Party Plug-In:");
              ImGui::SameLine    ();
              ImGui::TextColored (ImColor (255, 255, 0),   "dgVoodoo2");
              ImGui::Separator   ();
              ImGui::BulletText  ("Please install this to: '%s'", dgvoodoo2_install_path);
            ImGui::EndTooltip   ();
          }
        };

        ImGui_CheckboxEx ("Direct3D 8", &config.apis.d3d8.hook,   has_dgvoodoo2, Tooltip_dgVoodoo2);

        ImGui::NextColumn (  );

        ImGui_CheckboxEx ("Direct Draw", &config.apis.ddraw.hook, has_dgvoodoo2, Tooltip_dgVoodoo2);

        ImGui::Columns    ( 1 );
        ImGui::Separator  (   );
#endif

        ImGui::Columns    ( 2 );
                          
        ImGui::Checkbox   ("OpenGL ", &config.apis.OpenGL.hook); ImGui::SameLine ();
#ifdef _WIN64
        ImGui::Checkbox   ("Vulkan ", &config.apis.Vulkan.hook);
#endif

        ImGui::NextColumn (  );

        if (ImGui::Button (" Disable All But the Active API "))
        {
          config.apis.d3d9ex.hook     = false; config.apis.d3d9.hook       = false;
          config.apis.dxgi.d3d11.hook = false;
          config.apis.OpenGL.hook     = false;
#ifdef _WIN64
          config.apis.dxgi.d3d12.hook = false; config.apis.Vulkan.hook     = false;
#else
          config.apis.d3d8.hook       = false; config.apis.ddraw.hook      = false;
#endif
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Application start time and negative interactions with third-party software can be reduced by turning off APIs that are not needed...");

        ImGui::Columns    ( 1 );

        ImGui::EndChild   (  );
        ImGui::PopStyleVar ( );

        EnableActiveAPI   (api);
        ImGui::TreePop    ();
      }

      if (ImGui::CollapsingHeader ("Hardware Monitoring"))
      {
        ImGui::TreePush ("");
        ImGui::Checkbox ("NvAPI  ", &config.apis.NvAPI.enable);
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("NVIDIA's hardware monitoring API, needed for the GPU stats on the OSD. Turn off only if your driver is buggy.");

        ImGui::SameLine ();
        ImGui::Checkbox ("ADL   ",   &config.apis.ADL.enable);
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("AMD's hardware monitoring API, needed for the GPU stats on the OSD. Turn off only if your driver is buggy.");
        ImGui::TreePop  ();
      }

      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

      if (ImGui::CollapsingHeader ("Debugging"))
      {
        ImGui::TreePush   ("");
        ImGui::BeginGroup (  );
        ImGui::Checkbox   ("Enable Crash Handler",          &config.system.handle_crashes);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Play Metal Gear Solid Alert Sound and Log Crashes in logs/crash.log");

        ImGui::Checkbox  ("ReHook LoadLibrary",             &config.compatibility.rehook_loadlibrary);

        if (ImGui::IsItemHovered ())
{
          ImGui::BeginTooltip ();
          ImGui::Text         ("Keep LoadLibrary Hook at Front of Hook Chain");
          ImGui::Separator    ();
          ImGui::BulletText   ("Improves Debug Log Accuracy");
          ImGui::BulletText   ("Third-Party Software May Deadlock Game at Startup if Enabled");
          ImGui::EndTooltip   ();
        }

        ImGui::SliderInt ("Log Level",                      &config.system.log_level, 0, 5);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Controls Debug Log Verbosity; Higher = Bigger/Slower Logs");

        ImGui::Checkbox  ("Log Game Output",                &config.system.game_output);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Log any Game Text Output to logs/game_output.log");

        if (ImGui::Checkbox  ("Print Debug Output to Console",  &config.system.display_debug_out))
        {
          if (config.system.display_debug_out)
          {
            if (! SK::Diagnostics::Debugger::CloseConsole ()) config.system.display_debug_out = true;
          }

          else
          {
            SK::Diagnostics::Debugger::SpawnConsole ();
          }
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Spawns Debug Console at Startup for Debug Text from Third-Party Software");

        ImGui::Checkbox  ("Trace LoadLibrary",              &config.system.trace_load_library);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Monitor DLL Load Activity");
          ImGui::Separator    ();
          ImGui::BulletText   ("Required for Render API Auto-Detection in Global Injector");
          ImGui::EndTooltip   ();
        }

        ImGui::Checkbox  ("Strict DLL Loader Compliance",   &config.system.strict_compliance);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    (  );
          ImGui::TextUnformatted ("Prevent Loading DLLs Simultaneously Across Multiple Threads");
          ImGui::Separator       (  );
          ImGui::BulletText      ("Eliminates Race Conditions During DLL Startup");
          ImGui::BulletText      ("Unsafe for a LOT of Improperly Designed Third-Party Software\n");
          ImGui::TreePush        ("");
          ImGui::TextUnformatted ("");
          ImGui::BeginGroup      (  );
          ImGui::TextUnformatted ("PROPER DLL DESIGN:  ");
          ImGui::EndGroup        (  );
          ImGui::SameLine        (  );
          ImGui::BeginGroup      (  );
          ImGui::TextUnformatted ("Never Call LoadLibrary (...) from DllMain (...)'s Thread !!!");
          ImGui::TextUnformatted ("Never Wait on a Synchronization Object from DllMain (...) !!");
          ImGui::EndGroup        (  );
          ImGui::TreePop         (  );
          ImGui::EndTooltip      (  );
        }

        ImGui::EndGroup    ( );
        ImGui::SameLine    ( );
        ImGui::BeginGroup  ( );
        auto DescribeRect = [](LPRECT rect, const char* szType, const char* szName)
        {
          ImGui::Text (szType);
          ImGui::NextColumn ();
          ImGui::Text (szName);
          ImGui::NextColumn ();
          ImGui::Text ( "| (%4li,%4li) / %4lix%li |  ",
                            rect->left, rect->top,
                              rect->right-rect->left, rect->bottom - rect->top );
          ImGui::NextColumn ();
        };

        ImGui::Columns (3);

        DescribeRect (&game_window.actual.window, "Window", "Actual" );
        DescribeRect (&game_window.actual.client, "Client", "Actual" );

        ImGui::Columns   (1);
        ImGui::Separator ( );
        ImGui::Columns   (3);

        DescribeRect (&game_window.game.window,   "Window", "Game"   );
        DescribeRect (&game_window.game.client,   "Client", "Game"   );

        ImGui::Columns   (1);
        ImGui::Separator ( );
        ImGui::Columns   (3);

        RECT window;
        GetClientRect (game_window.hWnd, &client);
        GetWindowRect (game_window.hWnd, &window);

        DescribeRect  (&window,   "Window", "GetWindowRect"   );
        DescribeRect  (&client,   "Client", "GetClientRect"   );
                           
        ImGui::Columns     (1);
        ImGui::Separator   ( );
        ImGui::EndGroup    ( );

        ImGui::Text        ( "ImGui Cursor State: %lu (%lu,%lu) { %lu, %lu }",
                               SK_ImGui_Cursor.visible, SK_ImGui_Cursor.pos.x,
                                                        SK_ImGui_Cursor.pos.y,
                                 SK_ImGui_Cursor.orig_pos.x, SK_ImGui_Cursor.orig_pos.y );
        ImGui::SameLine    ( );
        ImGui::Text        (" {%s :: Last Update: %lu}",
                              SK_ImGui_Cursor.idle ? "Idle" :
                                                     "Not Idle",
                                SK_ImGui_Cursor.last_move);
        ImGui::TreePop      ( );
      }

      ImGui::PopStyleColor  (3);

      ImGui::TreePop        ( );
      ImGui::PopStyleColor  (3);
    }

    bool input_mgmt_open = ImGui::CollapsingHeader ("Input Management");

    if (config.imgui.show_input_apis)
    {
      static DWORD last_xinput   = 0;
      static DWORD last_hid      = 0;
      static DWORD last_di8      = 0;
      static DWORD last_steam    = 0;
      static DWORD last_rawinput = 0;

      struct { ULONG reads; } xinput { };
      struct { ULONG reads; } steam  { };

      struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } di8       { };
      struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } hid       { };
      struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } raw_input { };

      xinput.reads            = SK_XInput_Backend.reads   [2];

      di8.kbd_reads           = SK_DI8_Backend.reads      [1];
      di8.mouse_reads         = SK_DI8_Backend.reads      [0];
      di8.gamepad_reads       = SK_DI8_Backend.reads      [2];

      hid.kbd_reads           = SK_HID_Backend.reads      [1];
      hid.mouse_reads         = SK_HID_Backend.reads      [0];
      hid.gamepad_reads       = SK_HID_Backend.reads      [2];

      raw_input.kbd_reads     = SK_RawInput_Backend.reads [1];
      raw_input.mouse_reads   = SK_RawInput_Backend.reads [0];
      raw_input.gamepad_reads = SK_RawInput_Backend.reads [2];

      steam.reads             = SK_Steam_Backend.reads    [2];


      if (SK_XInput_Backend.nextFrame ())
        last_xinput = timeGetTime ();

      if (SK_Steam_Backend.nextFrame ())
        last_steam = timeGetTime ();

      if (SK_HID_Backend.nextFrame ())
        last_hid = timeGetTime ();

      if (SK_DI8_Backend.nextFrame ())
        last_di8 = timeGetTime ();

      if (SK_RawInput_Backend.nextFrame ())
        last_rawinput = timeGetTime ();


      if (last_steam > timeGetTime ( ) - 500UL)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - ( 0.4f * ( timeGetTime ( ) - last_steam ) / 500.0f ), 1.0f, 0.8f));
        ImGui::SameLine ( );
        ImGui::Text ("       Steam");
        ImGui::PopStyleColor ( );

        if (ImGui::IsItemHovered ( ))
        {
          ImGui::BeginTooltip ( );
          ImGui::Text ("Gamepad     %lu", steam.reads);
          ImGui::EndTooltip ( );
        }
      }

      if (last_xinput > timeGetTime () - 500UL)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (timeGetTime () - last_xinput) / 500.0f), 1.0f, 0.8f));
        ImGui::SameLine       ();
        ImGui::Text           ("       XInput");
        ImGui::PopStyleColor  ();

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Gamepad     %lu", xinput.reads);
          ImGui::EndTooltip   ();
        }
      }

      if (last_hid > timeGetTime () - 500UL)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (timeGetTime () - last_hid) / 500.0f), 1.0f, 0.8f));
        ImGui::SameLine       ();
        ImGui::Text           ("       HID");
        ImGui::PopStyleColor  ();

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();

          if (hid.kbd_reads > 0)
            ImGui::Text       ("Keyboard      %lu", hid.kbd_reads);
          if (hid.mouse_reads > 0)
            ImGui::Text         ("Mouse       %lu", hid.mouse_reads);
          if (hid.gamepad_reads > 0)
            ImGui::Text         ("Gamepad     %lu", hid.gamepad_reads);

          ImGui::EndTooltip   ();
        }
      }

      if (last_di8 > timeGetTime () - 500UL)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (timeGetTime () - last_di8) / 500.0f), 1.0f, 0.8f));
        ImGui::SameLine       ();
        ImGui::Text           ("       Direct Input");
        ImGui::PopStyleColor  ();

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();

          if (di8.kbd_reads > 0) {
            ImGui::Text       ("Keyboard  %lu", di8.kbd_reads);
          }
          if (di8.mouse_reads > 0) {
            ImGui::Text       ("Mouse     %lu", di8.mouse_reads);
          }
          if (di8.gamepad_reads > 0) {
            ImGui::Text       ("Gamepad   %lu", di8.gamepad_reads);
          };

          ImGui::EndTooltip   ();
        }
      }

      if (last_rawinput > timeGetTime () - 500UL)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (timeGetTime () - last_rawinput) / 500.0f), 1.0f, 0.8f));
        ImGui::SameLine       ();
        ImGui::Text           ("       Raw Input");
        ImGui::PopStyleColor  ();

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();

          if (raw_input.kbd_reads > 0) {
            ImGui::Text       ("Keyboard   %lu", raw_input.kbd_reads);
          }
          if (raw_input.mouse_reads > 0) {
            ImGui::Text       ("Mouse      %lu", raw_input.mouse_reads);
          }
          if (raw_input.gamepad_reads > 0) {
            ImGui::Text       ("Gamepad    %lu", raw_input.gamepad_reads);
          }

          ImGui::EndTooltip   ();
        }
      }
    }

    if (input_mgmt_open)
    {

      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
      ImGui::TreePush       ("");

      if (ImGui::CollapsingHeader ("Mouse Cursor"))
      {
        ImGui::TreePush ("");
        ImGui::BeginGroup ();
        ImGui::Checkbox   ( "Hide When Not Moved", &config.input.cursor.manage        );

        if (config.input.cursor.manage) {
          ImGui::TreePush ("");
          ImGui::Checkbox ( "or Key Pressed",
                                                   &config.input.cursor.keys_activate );
          ImGui::TreePop  ();
        }
 
        ImGui::EndGroup   ();
        ImGui::SameLine   ();

        float seconds = 
          (float)config.input.cursor.timeout  / 1000.0f;

        float val = config.input.cursor.manage ? 1.0f : 0.0f;

        ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor ( 0.3f,  0.3f,  0.3f,  val));
        ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor ( 0.6f,  0.6f,  0.6f,  val));
        ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor ( 0.9f,  0.9f,  0.9f,  val));
        ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImColor ( 1.0f,  1.0f,  1.0f, 1.0f));

        if ( ImGui::SliderFloat ( "Seconds Before Hiding",
                                    &seconds, 0.0f, 30.0f ) )
        {
          config.input.cursor.timeout = static_cast <LONG> (( seconds * 1000.0f ));
        }

        ImGui::PopStyleColor (4);

        if (! cursor_vis)
        {
          if (ImGui::Button (" Force Mouse Cursor Visible ")) {
            while (ShowCursor (TRUE) < 0)
              ;

            cursor_vis = true;
          }
        }

        else
        {
          if (ImGui::Button (" Force Mouse Cursor Hidden "))
          {
            while (ShowCursor (FALSE) >= -1)
              ;

            cursor_vis = false;
          }
        }

        ImGui::TreePop ();
      }

      if (ImGui::CollapsingHeader ("Gamepad"))
      {
        ImGui::TreePush      ("");

        ImGui::Columns        (2);
        ImGui::Checkbox       ("Haptic UI Feedback", &config.input.gamepad.haptic_ui);

        ImGui::SameLine       ();

        ImGui::Checkbox       ("Disable ALL Rumble", &config.input.gamepad.xinput.disable_rumble);

        ImGui::NextColumn     ();

        ImGui::Checkbox       ("Rehook XInput", &config.input.gamepad.rehook_xinput); ImGui::SameLine ();

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip  ();
            ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Re-installs input hooks if third-party hooks are detected.");
            ImGui::Separator   ();
            ImGui::BulletText  ("This may improve compatibility with x360ce, but will require a game restart.");
          ImGui::EndTooltip    ();
        }

        ImGui::Checkbox       ("Disable PS4 HID Input", &config.input.gamepad.disable_ps4_hid);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip  ();
            ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Prevents double input processing in games that support XInput and native PS4.");
            ImGui::Separator   ();
            ImGui::BulletText  ("This option requires restarting the game.");
          ImGui::EndTooltip    ();
        }

        ImGui::NextColumn ( );
        ImGui::Columns    (1);

        ImGui::Separator ();

        bool connected [4];
        connected [0] = SK_XInput_PollController (0);
        connected [1] = SK_XInput_PollController (1);
        connected [2] = SK_XInput_PollController (2);
        connected [3] = SK_XInput_PollController (3);

        if ( connected [0] || connected [1] ||
             connected [2] || connected [3] )
        {
          ImGui::Text("UI Controlled By:  "); ImGui::SameLine();

          if (connected [0]) {
            ImGui::RadioButton ("XInput Controller 0##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 0);
            if (connected [1] || connected [2] || connected [3]) ImGui::SameLine ();
          }

          if (connected [1]) {
            ImGui::RadioButton ("XInput Controller 1##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 1);
            if (connected [2] || connected [3]) ImGui::SameLine ();
          }

          if (connected [2]) {
            ImGui::RadioButton ("XInput Controller 2##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 2);
            if (connected [3]) ImGui::SameLine ();
          }

          if (connected [3])
            ImGui::RadioButton ("XInput Controller 3##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 3);

          ImGui::SameLine    ();
          ImGui::RadioButton ("Nothing", (int *)&config.input.gamepad.xinput.ui_slot, 4);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Config menu will only respond to keyboard/mouse input.");
        }

        ImGui::Text ("XInput Placeholders");

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip  ();
            ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Substitute Real Controllers With Virtual Ones Until Connected.");
            ImGui::Separator   ();
            ImGui::BulletText  ("Useful for games like God Eater 2 that do not support hot-plugging in a sane way.");
            ImGui::BulletText  ("Also reduces performance problems games cause themselves by trying to poll controllers that are not connected.");
          ImGui::EndTooltip    ();
        }

        ImGui::SameLine();

        auto XInputPlaceholderCheckbox = [](const char* szName, DWORD dwIndex)
        {
          ImGui::Checkbox (szName, &config.input.gamepad.xinput.placehold [dwIndex]);

          SK_XInput_PacketJournal journal =
            SK_XInput_GetPacketJournal (dwIndex);

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ( );
             ImGui::TextColored (ImColor (255, 255, 255), "Hardware Packet Sequencing" );
             ImGui::TextColored (ImColor (160, 160, 160), "(Last: %lu | Now: %lu)",
                                    journal.sequence.last, journal.sequence.current );
             ImGui::Separator   ( );
             ImGui::Columns     (2, nullptr, 0);
             ImGui::TextColored (ImColor (255, 165, 0), "Virtual Packets..."); ImGui::NextColumn ();
             ImGui::Text        ("%+07lu", journal.packet_count.virt);        ImGui::NextColumn ();
             ImGui::TextColored (ImColor (127, 255, 0), "Real Packets...");    ImGui::NextColumn ();
             ImGui::Text        ("%+07lu", journal.packet_count.real);
             ImGui::Columns     (1);
            ImGui::EndTooltip   ( );
          }
        };

        XInputPlaceholderCheckbox ("Slot 0", 0); ImGui::SameLine ();
        XInputPlaceholderCheckbox ("Slot 1", 1); ImGui::SameLine ();
        XInputPlaceholderCheckbox ("Slot 2", 2); ImGui::SameLine ();
        XInputPlaceholderCheckbox ("Slot 3", 3);

// TODO
#if 0
        ImGui::Separator ();

extern float SK_ImGui_PulseTitle_Duration;
extern float SK_ImGui_PulseTitle_Strength;

extern float SK_ImGui_PulseButton_Duration;
extern float SK_ImGui_PulseButton_Strength;

extern float SK_ImGui_PulseNav_Duration;
extern float SK_ImGui_PulseNav_Strength;

        ImGui::SliderFloat ("NavPulseStrength", &SK_ImGui_PulseNav_Strength, 0.0f, 2.0f);
        ImGui::SliderFloat ("NavPulseDuration", &SK_ImGui_PulseNav_Duration, 0.0f, 1000.0f);

        ImGui::SliderFloat ("ButtonPulseStrength", &SK_ImGui_PulseButton_Strength, 0.0f, 2.0f);
        ImGui::SliderFloat ("ButtonPulseDuration", &SK_ImGui_PulseButton_Duration, 0.0f, 1000.0f);

        ImGui::SliderFloat ("TitlePulseStrength", &SK_ImGui_PulseTitle_Strength, 0.0f, 2.0f);
        ImGui::SliderFloat ("TitlePulseDuration", &SK_ImGui_PulseTitle_Duration, 0.0f, 1000.0f);
#endif

        auto GamepadDebug = [](UINT idx) ->
        void
        {
          JOYINFOEX joy_ex   { };
          JOYCAPSA  joy_caps { };

          joy_ex.dwSize  = sizeof JOYINFOEX;
          joy_ex.dwFlags = JOY_RETURNALL      | JOY_RETURNPOVCTS |
                           JOY_RETURNCENTERED | JOY_USEDEADZONE;

          if (joyGetPosEx    (idx, &joy_ex)                    != JOYERR_NOERROR)
            return;

          if (joyGetDevCapsA (idx, &joy_caps, sizeof JOYCAPSA) != JOYERR_NOERROR || joy_caps.wCaps == 0)
            return;

          std::stringstream buttons;

          for ( unsigned int i = 0, j = 0; i < joy_caps.wMaxButtons; i++ )
          {
            if (joy_ex.dwButtons & (1 << i))
            {
              if (j != 0)
                buttons << ", ";

              buttons << "Button " << std::to_string (i);

              ++j;
            }
          }

          ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
          ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
          ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

          bool expanded = ImGui::CollapsingHeader (SK_FormatString ("%s###JOYSTICK_DEBUG_%lu", joy_caps.szPname, idx).c_str ());

          ImGui::Combo    ("Gamepad Type", &config.input.gamepad.predefined_layout, "PlayStation 4\0Steam\0\0", 2);

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("This setting is only used if XInput or DirectInput are not working.");
          }

          ImGui::SameLine ();

          ImGui::Checkbox    ("Favor DirectInput over XInput", &config.input.gamepad.native_ps4);

          if (expanded)
          {
            ImGui::TreePush        (       ""       );

            ImGui::TextUnformatted (buttons.str ().c_str ());

            float angle =
              static_cast <float> (joy_ex.dwPOV) / 100.0f;

            if (joy_ex.dwPOV != JOY_POVCENTERED)
              ImGui::Text (u8" D-Pad:  %4.1f°", angle);
            else
              ImGui::Text (  " D-Pad:  Centered");

            struct axis_s {
              const char* label;
              float       min, max;
              float       now;
            }
              axes [6] = { { "X-Axis", static_cast <float> (joy_caps.wXmin),
                                       static_cast <float> (joy_caps.wXmax),
                                       static_cast <float> (joy_ex.dwXpos) },

                            { "Y-Axis", static_cast <float> (joy_caps.wYmin), 
                                        static_cast <float> (joy_caps.wYmax),
                                        static_cast <float> (joy_ex.dwYpos) },

                            { "Z-Axis", static_cast <float> (joy_caps.wZmin),
                                        static_cast <float> (joy_caps.wZmax),
                                        static_cast <float> (joy_ex.dwZpos) },

                            { "R-Axis", static_cast <float> (joy_caps.wRmin),
                                        static_cast <float> (joy_caps.wRmax),
                                        static_cast <float> (joy_ex.dwRpos) },

                            { "U-Axis", static_cast <float> (joy_caps.wUmin),
                                        static_cast <float> (joy_caps.wUmax),
                                        static_cast <float> (joy_ex.dwUpos) },

                            { "V-Axis", static_cast <float> (joy_caps.wVmin),
                                        static_cast <float> (joy_caps.wVmax),
                                        static_cast <float> (joy_ex.dwVpos) } };

            for (UINT axis = 0; axis < joy_caps.wMaxAxes; axis++)
            {
              auto  range  = static_cast <float>  (axes [axis].max - axes [axis].min);
              float center = static_cast <float> ((axes [axis].max + axes [axis].min)) / 2.0f;
              float rpos   = 0.5f;

              if (static_cast <float> (axes [axis].now) < center)
                rpos = center - (center - axes [axis].now);
              else
                rpos = static_cast <float> (axes [axis].now - axes [axis].min);

              ImGui::ProgressBar ( rpos / range,
                                     ImVec2 (-1, 0),
                                       SK_FormatString ( "%s [ %.0f, { %.0f, %.0f } ]",
                                                           axes [axis].label, axes [axis].now,
                                                           axes [axis].min,   axes [axis].max ).c_str () );
            }

            ImGui::TreePop     ( );
          }
          ImGui::PopStyleColor (3);
        };

        ImGui::Separator ();

        GamepadDebug (JOYSTICKID1);
        GamepadDebug (JOYSTICKID2);

        ImGui::TreePop       ( );
      }

      if (ImGui::CollapsingHeader ("Low-Level Mouse Settings", ImGuiTreeNodeFlags_DefaultOpen))
      {
        static bool  deadzone_hovered = false;
               float float_thresh     = std::max (1.0f, std::min (100.0f, config.input.mouse.antiwarp_deadzone));

        ImVec2 deadzone_pos    = ImGui::GetIO ().DisplaySize;
               deadzone_pos.x /= 2.0f;
               deadzone_pos.y /= 2.0f;
        ImVec2 deadzone_size ( ImGui::GetIO ().DisplaySize.x * float_thresh / 200.0f,
                               ImGui::GetIO ().DisplaySize.y * float_thresh / 200.0f );

        ImVec2 xy0 ( deadzone_pos.x - deadzone_size.x,
                     deadzone_pos.y - deadzone_size.y );
        ImVec2 xy1 ( deadzone_pos.x + deadzone_size.x,
                     deadzone_pos.y + deadzone_size.y );

        if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open ||
               SK_ImGui_Cursor.prefs.no_warp.visible )  && 
             ( deadzone_hovered || ImGui::IsMouseHoveringRect ( xy0, xy1, false ) ) )
        {
          ImVec4 col = ImColor::HSV ( 0.18f, 
                                          std::min (1.0f, 0.85f + (sin ((float)(timeGetTime () % 400) / 400.0f))),
                                                               (float)(0.66f + (timeGetTime () % 830) / 830.0f ) );
          const ImU32 col32 =
            ImColor (col);

          ImDrawList* draw_list =
            ImGui::GetWindowDrawList ();

          draw_list->PushClipRectFullScreen (                                     );
          draw_list->AddRect                ( xy0, xy1, col32, 32.0f, 0xF, 3.333f );
          draw_list->PopClipRect            (                                     );
        }

        ImGui::TreePush      ("");

        ImGui::BeginGroup    ();
        ImGui::Text          ("Mouse Problems?");
        ImGui::TreePush      ("");

#if 0
        int  input_backend = 1;
        bool changed       = false;

        changed |=
          ImGui::RadioButton ("Win32",     &input_backend, 0); ImGui::SameLine ();
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Temporarily Disabled (intended for compatibility only)");
        changed |=
          ImGui::RadioButton ("Raw Input", &input_backend, 1);
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("More Reliable (currently the only supported input API)");
#endif
        bool non_relative = (! config.input.mouse.add_relative_motion);

        ImGui::Checkbox ("Fix Jittery Mouse (in menus)", &non_relative);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Disable RawInput Mouse Delta Processing");
          ImGui::Separator    ();
          ImGui::BulletText   ("In games that ONLY use DirectInput / RawInput for mouse, this may make the config menu unusable.");
          ImGui::EndTooltip   ();
        }

        config.input.mouse.add_relative_motion = (! non_relative);

        ImGui::Checkbox ("Fix Synaptics Scroll", &config.input.mouse.fix_synaptics);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Generate Missing DirectInput / RawInput / HID Events for Touchpad Scroll");
          ImGui::Separator    ();
          ImGui::BulletText   ("Synaptics touchpads only generate Win32 API messages and scroll events go unnoticed by most games.");
          ImGui::BulletText   ("Enabling this will attempt to fix missing input APIs for the Synaptics driver.");
          ImGui::EndTooltip   ();
        }

        ImGui::TreePop       ();
        ImGui::EndGroup      ();

        ImGui::SameLine ();

        ImGui::BeginGroup    ();
        ImGui::Text          ("Mouse Input Capture");
        ImGui::TreePush      ("");

        ImGui::BeginGroup    (  );

        if (ImGui::Checkbox ("Block Mouse", &config.input.ui.capture_mouse))
        {
          SK_ImGui_UpdateCursor ();
          //SK_ImGui_AdjustCursor ();
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip  ();
            ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Prevent Game from Detecting Mouse Input while this UI is Visible");
            ImGui::Separator   ();
            ImGui::BulletText  ("May help with mouselook in some games");
            //ImGui::BulletText  ("Implicitly enabled if running at a non-native Fullscreen resolution");
          ImGui::EndTooltip    ();
        }

        ImGui::SameLine ();

        if (ImGui::Checkbox ("Use Hardware Cursor", &config.input.ui.use_hw_cursor))
        {
          SK_ImGui_UpdateCursor ();
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip  ();
            ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Reduce Input Latency -- (Trade Cursor Lag for UI Lag)");
            ImGui::Separator   ();
            ImGui::BulletText  ("You will experience several frames of lag while dragging UI windows around.");
            ImGui::BulletText  ("Most Games use Hardware Cursors; turning this on will reduce visible cursor trails.");
            ImGui::BulletText  ("Automatically switches to Software when the game is not using Hardware.");
          ImGui::EndTooltip    ();
        }

        ImGui::Checkbox ("Block Input When No Cursor is Visible", &config.input.ui.capture_hidden);  ImGui::SameLine ();

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Generally prevents mouselook if you move your cursor away from the config UI");

        ImGui::EndGroup   (  );

        ImGui::TreePush   ("");
        ImGui::SameLine   (  );

        ImGui::BeginGroup (  );

        ImGui::Checkbox ("No Warp (cursor visible)",              &SK_ImGui_Cursor.prefs.no_warp.visible);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip  ();
            ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Do Not Alllow Game to Move Cursor to Center of Screen");
            ImGui::Separator   ();
            ImGui::BulletText  ("Any time the cursor is visible");
            ImGui::BulletText  ("Fixes buggy games like Mass Effect Andromeda");
          ImGui::EndTooltip    ();
        }

        ImGui::Checkbox ("No Warp (UI open)",                     &SK_ImGui_Cursor.prefs.no_warp.ui_open);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip  ();
            ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Do Not Alllow Game to Move Cursor to Center of Screen");
            ImGui::Separator   ();
            ImGui::BulletText  ("Any time the UI is visible");
            ImGui::BulletText  ("May be needed if Mouselook is fighting you tooth and nail.");
          ImGui::EndTooltip    ();
        }

        ImGui::EndGroup       ( );

        if ( SK_ImGui_Cursor.prefs.no_warp.ui_open ||
             SK_ImGui_Cursor.prefs.no_warp.visible ) 
        {
          if ( ImGui::SliderFloat ( "Anti-Warp Deadzone##CursorDeadzone",
                                      &float_thresh, 1.0f, 100.0f, "%4.2f%% of Screen" ) )
          {
            if (float_thresh <= 1.0f)
              float_thresh = 1.0f;

            config.input.mouse.antiwarp_deadzone = float_thresh;
          }

          if ( ImGui::IsItemHovered () || ImGui::IsItemFocused () ||
               ImGui::IsItemClicked () || ImGui::IsItemActive  () )
          {
            deadzone_hovered = true;
          }

          else
          {
            deadzone_hovered = false;
          }
        }

        ImGui::TreePop        ( );
        ImGui::TreePop        ( );

        ImGui::EndGroup       ( );

#if 0
        extern bool SK_DInput8_BlockWindowsKey (bool block);
        extern bool SK_DInput8_HasKeyboard     (void);

        if (SK_DInput8_HasKeyboard ())
        {
          if (ImGui::Checkbox ("Block Windows Key", &config.input.keyboard.block_windows_key))
          {
            config.input.keyboard.block_windows_key = SK_DInput8_BlockWindowsKey (config.input.keyboard.block_windows_key);
          }
        }
#endif

        ImGui::TreePop        ();
      }

      bool devices = ImGui::CollapsingHeader ("Enable / Disable Devices");

      if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("The primary use-case for these options is preventing a game from changing input icons.");

      if (devices)
      {
        ImGui::TreePush ("");
        ImGui::Checkbox ("Disable Mouse Input to Game",    &config.input.mouse.disabled_to_game);
        ImGui::SameLine ();
        ImGui::Checkbox ("Disable Keyboard Input to Game", &config.input.keyboard.disabled_to_game);
        ImGui::TreePop  ();
      }

      ImGui::TreePop       ( );
      ImGui::PopStyleColor (3);
    }

    if ( ImGui::CollapsingHeader ("Window Management") )
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));
      ImGui::TreePush       ("");

      if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) && ImGui::CollapsingHeader ("Style and Position", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TreePush ("");

        bool borderless        = config.window.borderless;
        bool center            = config.window.center;
        bool fullscreen        = config.window.fullscreen;

        if ( ImGui::Checkbox ( "Borderless  ", &borderless ) )
        {
            //config.window.fullscreen = false;

          if (! config.window.fullscreen)
            SK_DeferCommand ("Window.Borderless toggle");
        }

        if (ImGui::IsItemHovered ()) 
        {
          if (! config.window.fullscreen)
          {
            if (borderless)
              ImGui::SetTooltip ("Add/Restore Window Borders");
            else
              ImGui::SetTooltip ("Remove Window Borders");
          }

          else
            ImGui::SetTooltip ("Cannot be Changed while Fullscreen is Checked");
        }

        if (borderless)
        {
          ImGui::SameLine ();

          if ( ImGui::Checkbox ( "Fullscreen (Borderless Upscale)", &fullscreen ) )
          {
            config.window.fullscreen = fullscreen;
            SK_ImGui_AdjustCursor ();
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Stretch Borderless Window to Fill Monitor");
            ImGui::Separator    ();
            ImGui::BulletText   ("Framebuffer Resolution Unchanged (GPU-Side Upscaling)");
            ImGui::BulletText   ("Upscaling to Match Desktop Resolution Adds AT LEAST 1 Frame of Input Latency!");
            ImGui::EndTooltip   ();
          }
        }

        if (! config.window.fullscreen)
        {
          ImGui::InputInt2 ("Override Resolution", (int *)&config.window.res.override.x);

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Set if Game's Window Resolution is Reported Wrong");
            ImGui::Separator    ();
            ImGui::BulletText   ("0x0 = Disable");
            ImGui::BulletText   ("Applied the Next Time a Style/Position Setting is Changed");
            ImGui::EndTooltip   ();
          }
        }

        if (! (config.window.borderless && config.window.fullscreen))
        {
          if ( ImGui::Checkbox ( "Center", &center ) ) {
            config.window.center = center;
            SK_ImGui_AdjustCursor ();
            //SK_DeferCommand ("Window.Center toggle");
          }

          if (ImGui::IsItemHovered ()) {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Keep the Render Window Centered at All Times");
            ImGui::Separator    ();
            ImGui::BulletText   ( "The Drag-Lock feature cannot be used while Window "
                                  "Centering is turned on." );
            ImGui::EndTooltip   ();
          }

          if (! config.window.center)
          {
            ImGui::TreePush    ("");
            ImGui::TextColored (ImVec4 (1.0f, 1.0f, 0.0f, 1.0f), "\nPress Ctrl + Shift + ScrollLock to Toggle Drag-Lock Mode");
            ImGui::BulletText  ("Useful for Positioning Borderless Windows.");
            ImGui::Text        ("");
            ImGui::TreePop     ();
          }

          bool pixel_perfect = ( config.window.offset.x.percent == 0.0 &&
                                 config.window.offset.y.percent == 0.0 );

          if (ImGui::Checkbox ("Pixel-Aligned Placement", &pixel_perfect))
          {
            if (pixel_perfect) {
              config.window.offset.x.absolute = 0;
              config.window.offset.y.absolute = 0;
              config.window.offset.x.percent  = 0.0f;
              config.window.offset.y.percent  = 0.0f;
            }

            else {
              config.window.offset.x.percent  = 0.000001f;
              config.window.offset.y.percent  = 0.000001f;
              config.window.offset.x.absolute = 0;
              config.window.offset.y.absolute = 0;
            }
          }

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ( "Pixel-Aligned Placement behaves inconsistently "
                                "when Desktop Resolution changes");

          if (! config.window.center)
          {
            ImGui::SameLine ();

            ImGui::Checkbox   ("Remember Dragged Position", &config.window.persistent_drag);

            if (ImGui::IsItemHovered ())
              ImGui::SetTooltip ( "Store the location of windows moved using "
                                  "Drag-Lock and apply at game startup" );
          }

          bool moved = false;

          HMONITOR hMonitor =
            MonitorFromWindow ( game_window.hWnd,
                                  MONITOR_DEFAULTTONEAREST );

          MONITORINFO mi  = { };
          mi.cbSize       = sizeof (mi);
          GetMonitorInfo (hMonitor, &mi);

          if (pixel_perfect)
          {
            int  x_pos        = std::abs (config.window.offset.x.absolute);
            int  y_pos        = std::abs (config.window.offset.y.absolute);

            bool right_align  = config.window.offset.x.absolute < 0;
            bool bottom_align = config.window.offset.y.absolute < 0;

            int  extent_x     = (mi.rcMonitor.right  - mi.rcMonitor.left) / 2 + 1;
            int  extent_y     = (mi.rcMonitor.bottom - mi.rcMonitor.top)  / 2 + 1;

            if (config.window.center) {
              extent_x /= 2;
              extent_y /= 2;
            }

            // Do NOT Apply Immediately or the Window Will Oscillate While
            //   Adjusting the Slider
            static bool queue_move = false;

            moved  = ImGui::SliderInt ("X Offset##WindowPix",       &x_pos, 0, extent_x, "%.0f pixels"); ImGui::SameLine ();
            moved |= ImGui::Checkbox  ("Right-aligned##WindowPix",  &right_align);
            moved |= ImGui::SliderInt ("Y Offset##WindowPix",       &y_pos, 0, extent_y, "%.0f pixels"); ImGui::SameLine ();
            moved |= ImGui::Checkbox  ("Bottom-aligned##WindowPix", &bottom_align);

            if (moved)
              queue_move = true;

            // We need to set pixel offset to 1 to do what the user expects
            //   these values to do... 0 = NO OFFSET, but the slider may move
            //     from right-to-left skipping 1.
            static bool reset_x_to_zero = false;
            static bool reset_y_to_zero = false;

            if (moved)
            {
              config.window.offset.x.absolute = x_pos * (right_align  ? -1 : 1);
              config.window.offset.y.absolute = y_pos * (bottom_align ? -1 : 1);

              if (right_align && config.window.offset.x.absolute >= 0)
                config.window.offset.x.absolute = -1;

              if (bottom_align && config.window.offset.y.absolute >= 0)
                config.window.offset.y.absolute = -1;

              if (config.window.offset.x.absolute == 0)
              {
                config.window.offset.x.absolute = 1;
                reset_x_to_zero = true;
              }

              if (config.window.offset.y.absolute == 0)
              {
                config.window.offset.y.absolute = 1;
                reset_y_to_zero = true;
              }
            }

            if (queue_move && (! ImGui::IsMouseDown (0)))
            {
              queue_move = false;

              SK_ImGui_AdjustCursor ();

              if (reset_x_to_zero) config.window.offset.x.absolute = 0;
              if (reset_y_to_zero) config.window.offset.y.absolute = 0;

              if (reset_x_to_zero || reset_y_to_zero)
                SK_ImGui_AdjustCursor ();

              reset_x_to_zero = false; reset_y_to_zero = false;
            }
          }

          else
          {
            float x_pos = std::abs (config.window.offset.x.percent);
            float y_pos = std::abs (config.window.offset.y.percent);

            x_pos *= 100.0f;
            y_pos *= 100.0f;

            bool right_align  = config.window.offset.x.percent < 0.0f;
            bool bottom_align = config.window.offset.y.percent < 0.0f;

            float extent_x = 50.05f;
            float extent_y = 50.05f;

            if (config.window.center) {
              extent_x /= 2.0f;
              extent_y /= 2.0f;
            }

            // Do NOT Apply Immediately or the Window Will Oscillate While
            //   Adjusting the Slider
            static bool queue_move = false;

            moved  = ImGui::SliderFloat ("X Offset##WindowRel",       &x_pos, 0, extent_x, "%.3f %%"); ImGui::SameLine ();
            moved |= ImGui::Checkbox    ("Right-aligned##WindowRel",  &right_align);
            moved |= ImGui::SliderFloat ("Y Offset##WindowRel",       &y_pos, 0, extent_y, "%.3f %%"); ImGui::SameLine ();
            moved |= ImGui::Checkbox    ("Bottom-aligned##WindowRel", &bottom_align);

            // We need to set pixel offset to 1 to do what the user expects
            //   these values to do... 0 = NO OFFSET, but the slider may move
            //     from right-to-left skipping 1.
            static bool reset_x_to_zero = false;
            static bool reset_y_to_zero = false;

            if (moved)
              queue_move = true;

            if (moved)
            {
              x_pos /= 100.0f;
              y_pos /= 100.0f;

              config.window.offset.x.percent = x_pos * (right_align  ? -1.0f : 1.0f);
              config.window.offset.y.percent = y_pos * (bottom_align ? -1.0f : 1.0f);

              if (right_align && config.window.offset.x.percent >= 0.0f)
                config.window.offset.x.percent = -0.01f;

              if (bottom_align && config.window.offset.y.percent >= 0.0f)
                config.window.offset.y.percent = -0.01f;

              if ( config.window.offset.x.percent <  0.000001f &&
                   config.window.offset.x.percent > -0.000001f )
              {
                config.window.offset.x.absolute = 1;
                reset_x_to_zero = true;
              }

              if ( config.window.offset.y.percent <  0.000001f &&
                   config.window.offset.y.percent > -0.000001f )
              {
                config.window.offset.y.absolute = 1;
                reset_y_to_zero = true;
              }
            }

            if (queue_move && (! ImGui::IsMouseDown (0)))
            {
              queue_move = false;

              SK_ImGui_AdjustCursor ();

              if (reset_x_to_zero) config.window.offset.x.absolute = 0;
              if (reset_y_to_zero) config.window.offset.y.absolute = 0;

              if (reset_x_to_zero || reset_y_to_zero)
                SK_ImGui_AdjustCursor ();

              reset_x_to_zero = false; reset_y_to_zero = false;
            }
          }
        }

        ImGui::Text     ("Window Layering");
        ImGui::TreePush ("");

        bool changed = false;

        changed |= ImGui::RadioButton ("No Preference",         &config.window.always_on_top, -1); ImGui::SameLine ();
        changed |= ImGui::RadioButton ("Prevent Always-On-Top", &config.window.always_on_top,  0); ImGui::SameLine ();
        changed |= ImGui::RadioButton ("Force Always-On-Top",   &config.window.always_on_top,  1);

        if (changed)
        {
          if (config.window.always_on_top == 1)
            SK_DeferCommand ("Window.TopMost true");
          else if (config.window.always_on_top == 0)
            SK_DeferCommand ("Window.TopMost false");
        }

        ImGui::TreePop ();
        ImGui::TreePop ();
      }

      if (ImGui::CollapsingHeader ("Input/Output Behavior", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TreePush ("");

        bool background_render = config.window.background_render;
        bool background_mute   = config.window.background_mute;

        ImGui::Text     ("Background Behavior");
        ImGui::TreePush ("");

        if ( ImGui::Checkbox ( "Mute Game ", &background_mute ) )
          SK_DeferCommand    ("Window.BackgroundMute toggle");
        
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Mute the Game when Another Window has Input Focus");

        if (! SK_GetCurrentRenderBackend ().fullscreen_exclusive)
        {
          ImGui::SameLine ();

          if ( ImGui::Checkbox ( "Continue Rendering", &background_render ) )
            SK_DeferCommand    ("Window.BackgroundRender toggle");

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Block Application Switch Notifications to the Game");
            ImGui::Separator    ();
            ImGui::BulletText   ("Most Games will Continue Rendering");
            ImGui::BulletText   ("Disables a Game's Built-in Mute-on-Alt+Tab Functionality");
            ImGui::BulletText   ("Keyboard/Mouse Input is Blocked, but not Gamepad Input");
            ImGui::EndTooltip   ();
          }
        }

        ImGui::TreePop ();

        ImGui::Text     ("Cursor Boundaries");
        ImGui::TreePush ("");
        
        int  ovr     = 0;
        bool changed = false;

        if (config.window.confine_cursor)
          ovr = 1;
        if (config.window.unconfine_cursor)
          ovr = 2;

        changed |= ImGui::RadioButton ("Normal Game Behavior", &ovr, 0); ImGui::SameLine ();
        changed |= ImGui::RadioButton ("Keep Inside Window",   &ovr, 1); ImGui::SameLine ();

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Prevents Mouse Cursor from Leaving the Game Window");
          ImGui::Separator    ();
          ImGui::BulletText   ("This window-lock will be disengaged when you press Alt + Tab");
          ImGui::EndTooltip   ();
        }

        changed |= ImGui::RadioButton ("Unrestrict Cursor",    &ovr, 2);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Prevent Game from Restricting Cursor to Window");

        if (changed)
        {
          switch (ovr)
          {
            case 0:
              config.window.confine_cursor   = 0;
              config.window.unconfine_cursor = 0;
              break;
            case 1:
              config.window.confine_cursor   = 1;
              config.window.unconfine_cursor = 0;
              break;
            case 2:
              config.window.confine_cursor   = 0;
              config.window.unconfine_cursor = 1;
              break;
          }

          SK_ImGui_AdjustCursor ();
        }

        ImGui::TreePop ();

        ImGui::Checkbox ("Disable Screensaver", &config.window.disable_screensaver);

        ImGui::TreePop ();
      }

      ImGui::TreePop       ( );
      ImGui::PopStyleColor (3);
    }

    if (ImGui::CollapsingHeader ("Volume Management"))
    {
      SK_ImGui_VolumeManager ();

      ImGui::TreePop ();
    }

    if (ImGui::CollapsingHeader ("On Screen Display (OSD)"))
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
      ImGui::TreePush       ("");

      ImGui::Checkbox ("Display OSD", &config.osd.show);

      if (config.osd.show && ImGui::CollapsingHeader ("Basic Monitoring", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TreePush ("");

        ImGui::Checkbox ("Title/Clock ",   &config.time.show); ImGui::SameLine ();
        ImGui::Checkbox ("Framerate",      &config.fps.show);

        ImGui::Checkbox ("GPU Stats",      &config.gpu.show);

        if (config.gpu.show)
        {
          ImGui::TreePush ("");

          int temp_unit = config.system.prefer_fahrenheit ? 1 : 0;

          ImGui::RadioButton (" Celsius ",    &temp_unit, 0); ImGui::SameLine ();
          ImGui::RadioButton (" Fahrenheit ", &temp_unit, 1); ImGui::SameLine ();
          ImGui::Checkbox    (" Print Slowdown", &config.gpu.print_slowdown);

          config.system.prefer_fahrenheit = temp_unit == 1 ? true : false;

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("On NVIDIA GPUs, this will print driver throttling details (e.g. power saving)");

          ImGui::TreePop  ();
        }

        ImGui::TreePop ();
      }

      if (config.osd.show && ImGui::CollapsingHeader ("Extended Monitoring"))
      {
        ImGui::TreePush     ("");

        ImGui::PushStyleVar (ImGuiStyleVar_WindowRounding, 16.0f);
        ImGui::BeginChild   ("WMI Monitors", ImVec2 (font_size * 50.0f,font_size_multiline * 6.05f), true, ImGuiWindowFlags_NavFlattened);

        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.785f, 0.0784f, 1.0f));
        ImGui::TextWrapped    ("These functions spawn a WMI monitoring service and may take several seconds to start.");
        ImGui::PopStyleColor  ();

        extern void
        __stdcall
        SK_StartPerfMonThreads (void);

        bool spawn = false;

        ImGui::Separator ( );
        ImGui::Columns   (3);

        spawn |= ImGui::Checkbox ("CPU Stats",         &config.cpu.show);

        if (config.cpu.show)
        {
          ImGui::TreePush ("");
          ImGui::Checkbox (" Simplified View",         &config.cpu.simple);
          ImGui::TreePop  (  );
        }

        spawn |= ImGui::Checkbox ("Memory Stats",      &config.mem.show);

        ImGui::NextColumn (  );

        spawn |= ImGui::Checkbox ("General I/O Stats", &config.io.show);
        spawn |= ImGui::Checkbox ("Pagefile Stats",    &config.pagefile.show);

        ImGui::NextColumn (  );

        spawn |= ImGui::Checkbox ("Disk Stats",        &config.disk.show);

        if (config.disk.show)
        {
          ImGui::TreePush ("");
          bool hovered;

          ImGui::RadioButton (" Logical Disks",  &config.disk.type, 1);
          hovered = ImGui::IsItemHovered ();
          ImGui::RadioButton (" Physical Disks", &config.disk.type, 0);
          hovered |= ImGui::IsItemHovered ();
          ImGui::TreePop  ();

          if (hovered)
            ImGui::SetTooltip ("Requires Application Restart");
        }

        else { ImGui::NewLine (); ImGui::NewLine (); }

        if (spawn)
          SK_StartPerfMonThreads ();

        ImGui::Columns     (1);
        ImGui::Separator   ( );

        ImGui::Columns     (3, "", false);
        ImGui::NextColumn  ();
        ImGui::NextColumn  ();
        ImGui::Checkbox    ("Remember These Settings", &config.osd.remember_state);
        ImGui::Columns     (1);

        ImGui::EndChild    ();
        ImGui::PopStyleVar ();

        ImGui::TreePop     ();
      }

      if (config.osd.show && ImGui::CollapsingHeader ("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TreePush ("");

        float color [3] = { static_cast <float> (config.osd.red)   / 255.0f,
                            static_cast <float> (config.osd.green) / 255.0f,
                            static_cast <float> (config.osd.blue)  / 255.0f };

        float default_r, default_g, default_b;

        extern void __stdcall SK_OSD_GetDefaultColor ( float& r,  float& g,  float& b);
                              SK_OSD_GetDefaultColor (default_r, default_g, default_b);

        if (config.osd.red   == MAXDWORD) color [0] = default_r;
        if (config.osd.green == MAXDWORD) color [1] = default_g;
        if (config.osd.blue  == MAXDWORD) color [2] = default_b;

        if (ImGui::ColorEdit3 ("OSD Color", color))
        {
          color [0] = std::max (std::min (color [0], 1.0f), 0.0f);
          color [1] = std::max (std::min (color [1], 1.0f), 0.0f);
          color [2] = std::max (std::min (color [2], 1.0f), 0.0f);

          config.osd.red   = static_cast <int>(color [0] * 255);
          config.osd.green = static_cast <int>(color [1] * 255);
          config.osd.blue  = static_cast <int>(color [2] * 255);

          if ( color [0] >= default_r - 0.001f &&
               color [0] <= default_r + 0.001f    ) config.osd.red   = MAXDWORD;
          if ( color [1] >= default_g - 0.001f && 
               color [1] <= default_g + 0.001f    ) config.osd.green = MAXDWORD;
          if ( color [2] >= default_b - 0.001f &&
               color [2] <= default_b + 0.001f    ) config.osd.blue  = MAXDWORD;
        }

        if (ImGui::SliderFloat ("OSD Scale", &config.osd.scale, 0.5f, 10.0f))
        {
          extern
          void
          __stdcall
          SK_SetOSDScale ( float  fScale,
                           bool   relative,
                           LPCSTR lpAppName );

          SK_SetOSDScale (config.osd.scale, false, nullptr);
        }

        int x_pos = std::abs (config.osd.pos_x);
        int y_pos = std::abs (config.osd.pos_y);

        bool right_align  = config.osd.pos_x < 0;
        bool bottom_align = config.osd.pos_y < 0;

        bool
        moved  = ImGui::SliderInt ("X Origin##OSD",       &x_pos, 1, static_cast <int> (io.DisplaySize.x)); ImGui::SameLine ();
        moved |= ImGui::Checkbox  ("Right-aligned##OSD",  &right_align);
        moved |= ImGui::SliderInt ("Y Origin##OSD",       &y_pos, 1, static_cast <int> (io.DisplaySize.y)); ImGui::SameLine ();
        moved |= ImGui::Checkbox  ("Bottom-aligned##OSD", &bottom_align);

        if (moved)
        {
          extern void
            __stdcall
          SK_SetOSDPos (int x, int y, LPCSTR lpAppName);

          config.osd.pos_x = x_pos * (right_align  ? -1 : 1);
          config.osd.pos_y = y_pos * (bottom_align ? -1 : 1);

          if (right_align  && config.osd.pos_x >= 0)
            config.osd.pos_x = -1;

          if (bottom_align && config.osd.pos_y >= 0)
            config.osd.pos_y = -1;

          SK_SetOSDPos (config.osd.pos_x, config.osd.pos_y, nullptr);
        }

        ImGui::TreePop ();
      }

      ImGui::TreePop       ( );
      ImGui::PopStyleColor (3);
    }

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

      if ( (int)api & (int)SK_RenderAPI::D3D11 )
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

    if (ImGui::CollapsingHeader ("Plug-Ins"))
    {
      ImGui::TreePush ("");

      extern iSK_INI*       dll_ini;

      wchar_t imp_path_reshade    [MAX_PATH + 2] = { };
      wchar_t imp_name_reshade    [64]           = { };

      wchar_t imp_path_reshade_ex [MAX_PATH + 2] = { };
      wchar_t imp_name_reshade_ex [64]           = { };

      wchar_t* wszShimFormat =
        L"%s\\PlugIns\\Unofficial\\ReShade\\ReShade%u.dll";

#ifdef _WIN64
      wcscat   (imp_name_reshade, L"Import.ReShade64");
      wsprintf (imp_path_reshade, L"%s\\PlugIns\\ThirdParty\\ReShade\\ReShade64.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());

      wcscat   (imp_name_reshade_ex, L"Import.ReShade64_Custom");

      if (SK_IsInjected ())
      {
        wsprintf (imp_path_reshade_ex, L"%s\\PlugIns\\Unofficial\\ReShade\\ReShade64.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());
      }

      else
      {
        wsprintf ( imp_path_reshade_ex, wszShimFormat,
                     std::wstring (SK_GetDocumentsDir        (                ) +
                                               L"\\My Mods\\SpecialK").c_str (  ),
                                   SK_GetBitness             (                ) );
      }
#else
      wcscat   (imp_name_reshade, L"Import.ReShade32");
      wsprintf (imp_path_reshade, L"%s\\PlugIns\\ThirdParty\\ReShade\\ReShade32.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());

      wcscat   (imp_name_reshade_ex, L"Import.ReShade32_Custom");

      if (SK_IsInjected ())
      {
        wsprintf (imp_path_reshade_ex, L"%s\\PlugIns\\Unofficial\\ReShade\\ReShade32.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());
      }

      else
      {
        wsprintf ( imp_path_reshade_ex, wszShimFormat,
                     std::wstring (SK_GetDocumentsDir        (                ) +
                                               L"\\My Mods\\SpecialK").c_str (  ),
                                   SK_GetBitness             (                ) );
      }
#endif
      bool reshade_official   = dll_ini->contains_section (imp_name_reshade);
      bool reshade_unofficial = dll_ini->contains_section (imp_name_reshade_ex);

      static int order    = 0;
      static int order_ex = 1;

      auto PlugInSelector = [&](iSK_INI* ini, std::string name, auto path, auto import_name, bool& enable, int& order, auto default_order = 1) ->
      bool
      {
        std::string hash_name  = name + "##PlugIn";
        std::string hash_load  = "Load Order##";
                    hash_load += name;

        bool changed =
          ImGui::Checkbox (hash_name.c_str (), &enable);

        if (ImGui::IsItemHovered ())
        {
          if (GetFileAttributesW (path) == INVALID_FILE_ATTRIBUTES)
            ImGui::SetTooltip ("Please install %s to %ws", name.c_str (), path);
        }

        if (ini->contains_section (import_name))
        {
          if (ini->get_section (import_name).get_value (L"When") == L"Early")
            order = 0;
          else
            order = 1;
        }
        else
          order = default_order;

        ImGui::SameLine ();

        changed |=
          ImGui::Combo (hash_load.c_str (), &order, "Early\0Plug-In\0\0");

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Plug-In Load Order is Suggested by Default.");
          ImGui::Separator    ();
          ImGui::BulletText   ("If a plug-in does not show up or the game crashes, try loading it early.");
          ImGui::BulletText   ("Early plug-ins handle rendering before Special K; ReShade will apply its effects to Special K's UI if loaded early.");
          ImGui::EndTooltip   ();
        }

        return changed;
      };

      auto SavePlugInPreference = [](iSK_INI* ini, bool enable, auto import_name, auto role, auto order, auto path)
      {
        if (! enable)
        {
          ini->remove_section (import_name);
          ini->write          (ini->get_filename ());
        }

        else if (GetFileAttributesW (path) != INVALID_FILE_ATTRIBUTES)
        {
          wchar_t wszImportRecord [4096];

          _swprintf ( wszImportRecord, L"[%s]\n"
#ifdef _WIN64
                                       L"Architecture=x64\n"
#else
                                       L"Architecture=Win32\n"
#endif
                                       L"Role=%s\n"
                                       L"When=%s\n"
                                       L"Filename=%s\n\n",
                                         import_name,
                                           role,
                                             order == 0 ? L"Early" :
                                                          L"PlugIn",
                                               path );

          ini->import (wszImportRecord);
          ini->write  (ini->get_filename ());
        }
      };

      bool changed = false;

      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

      if (ImGui::CollapsingHeader ("Third-Party"))
      {
        ImGui::TreePush    ("");
        changed |= 
            PlugInSelector (dll_ini, "ReShade (Official)", imp_path_reshade, imp_name_reshade, reshade_official, order, 1);
        ImGui::TreePop     (  );
      }
      ImGui::PopStyleColor ( 3);

      if (SK_IsInjected () || StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dxgi.dll")     ||
                              StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"d3d9.dll")     ||
                              StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"OpenGL32.dll") ||
                              StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dinput8.dll"))
      {
        ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
        ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
        ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

        bool unofficial = ImGui::CollapsingHeader ("Unofficial");

        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.247f, .95f, .98f));
        ImGui::SameLine       ();
        ImGui::Text           ("           Customized for Special K");
        ImGui::PopStyleColor  ();

        if (unofficial)
        {
          ImGui::TreePush    ("");
          changed |=
              PlugInSelector (dll_ini, "ReShade (Custom)", imp_path_reshade_ex, imp_name_reshade_ex, reshade_unofficial, order_ex, 1);
          ImGui::TreePop     (  );
        }
        ImGui::PopStyleColor ( 3);
      }

      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.15f, 0.95f, 0.98f));
      ImGui::TextWrapped    ("If you run into problems with a Plug-In, pressing and holding Ctrl + Shift at game startup can disable them.");
      ImGui::PopStyleColor  ();


      if (changed)
      {
        if (reshade_unofficial)
          reshade_official = false;

        if (reshade_official)
          reshade_unofficial = false;

        SavePlugInPreference (dll_ini, reshade_official,   imp_name_reshade,    L"ThirdParty", order,    imp_path_reshade   );
        SavePlugInPreference (dll_ini, reshade_unofficial, imp_name_reshade_ex, L"Unofficial", order_ex, imp_path_reshade_ex);
      }

      ImGui::TreePop ();
    }

    if (SK::SteamAPI::AppID () != 0)
    {
      if ( ImGui::CollapsingHeader ("Steam Enhancements", ImGuiTreeNodeFlags_CollapsingHeader | 
                                                          ImGuiTreeNodeFlags_DefaultOpen ) )
      {
        ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
        ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
        ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));
        ImGui::TreePush       ("");

        if (SK_SteamAPI_GetNumPossibleAchievements () > 0)
        {
          static char szProgress [128] = { };

          float  ratio            = SK::SteamAPI::PercentOfAchievementsUnlocked ();
          size_t num_achievements = SK_SteamAPI_GetNumPossibleAchievements      ();

          snprintf ( szProgress, 127, "%.2f%% of Achievements Unlocked (%u/%u)",
                       ratio * 100.0f, static_cast <uint32_t> ((ratio * static_cast <float> (num_achievements))),
                                       static_cast <uint32_t> (                              num_achievements) );

          ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImColor (0.90f, 0.72f, 0.07f, 0.80f) ); 
          ImGui::ProgressBar    ( ratio,
                                    ImVec2 (-1, 0),
                                      szProgress );
          ImGui::PopStyleColor  ();

          int friends =
            SK_SteamAPI_GetNumFriends ();

          if (friends && ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip   ();

            static int num_records = 0;

            auto max_lines = static_cast <int> ((io.DisplaySize.y * 0.725f) / (font_size_multiline * 0.9f));
            int  cur_line  = 0;
               num_records = 0;

            ImGui::BeginGroup     ();

            ImGui::PushStyleColor ( ImGuiCol_Text,          ImColor (255, 255, 255)              ); 
            ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImColor (0.90f, 0.72f, 0.07f, 0.80f) ); 

            for (int i = 0; i < (int)((float)friends * SK_SteamAPI_FriendStatPercentage ()); i++)
            {
              size_t      len     = 0;
              const char* szName  = SK_SteamAPI_GetFriendName (i, &len);

              float       percent =
                SK_SteamAPI_GetUnlockedPercentForFriend (i);

              if (percent > 0.0f)
              {
                ImGui::ProgressBar    ( percent, ImVec2 (io.DisplaySize.x * 0.0816f, 0.0f) );
                ImGui::SameLine       ( );
                ImGui::PushStyleColor (ImGuiCol_Text, ImColor (.81f, 0.81f, 0.81f));
                ImGui::Text           (szName);
                ImGui::PopStyleColor  (1);

                ++num_records;

                if (cur_line >= max_lines)
                {
                  ImGui::EndGroup     ( );
                  ImGui::SameLine     ( );
                  ImGui::BeginGroup   ( );
                  cur_line = 0;
                }

                else
                  ++cur_line;
              }
            }

            ImGui::PopStyleColor  (2);
            ImGui::EndGroup       ( );
            ImGui::EndTooltip     ( );
          }

          if (ImGui::CollapsingHeader ("Achievements") )
          {
            ImGui::TreePush ("");
            ImGui::BeginGroup ();
            extern void
            SK_UnlockSteamAchievement (uint32_t idx);

            if (ImGui::Button (" Test Unlock "))
              SK_UnlockSteamAchievement (0);

            if (ImGui::IsItemHovered ())
              ImGui::SetTooltip ("Perform a FAKE unlock so that you can tune your preferences.");

            ImGui::SameLine ();

            ImGui::Checkbox ("Play Sound ", &config.steam.achievements.play_sound);

            if (config.steam.achievements.play_sound) {
              ImGui::SameLine ();
              
              int i = 0;
              
              if (! _wcsicmp (config.steam.achievements.sound_file.c_str (), L"xbox"))
                i = 1;
              else
                i = 0;
              
              if (ImGui::Combo ("", &i, "PlayStation Network\0Xbox Live\0\0", 2))
              {
                if (i == 0) config.steam.achievements.sound_file = L"psn";
                       else config.steam.achievements.sound_file = L"xbox";
              
                extern void
                SK_SteamAPI_LoadUnlockSound (const wchar_t* wszUnlockSound);
              
                SK_SteamAPI_LoadUnlockSound (config.steam.achievements.sound_file.c_str ());
              }
            }

            ImGui::EndGroup ();
            ImGui::SameLine ();

            ImGui::Checkbox ("Take Screenshot", &config.steam.achievements.take_screenshot);

            ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
            ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
            ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

            bool uncollapsed = ImGui::CollapsingHeader ("Enhanced Popup");

            ImGui::SameLine (); ImGui::Checkbox        ("   Fetch Friend Unlock Stats", &config.steam.achievements.pull_friend_stats);

            if (uncollapsed)
            {
              ImGui::TreePush ("");

              int  mode    = (config.steam.achievements.popup.show + config.steam.achievements.popup.animate);
              bool changed = false;

              ImGui::Text          ("Draw Mode:");                              ImGui::SameLine ();

              changed |=
                ImGui::RadioButton ("Disabled ##AchievementPopup",   &mode, 0); ImGui::SameLine ();
              changed |=
                ImGui::RadioButton ("Stationary ##AchievementPopup", &mode, 1); ImGui::SameLine ();
              ImGui::BeginGroup    ( );
              changed |=
                ImGui::RadioButton ("Animated ##AchievementPopup",   &mode, 2);

                ImGui::SameLine    ( );
                ImGui::Combo       ( "##PopupLoc",         &config.steam.achievements.popup.origin,
                                             "Top-Left\0"
                                             "Top-Right\0"
                                             "Bottom-Left\0"
                                             "Bottom-Right\0\0" );

              if ( changed )
              {
                config.steam.achievements.popup.show    = (mode > 0);
                config.steam.achievements.popup.animate = (mode > 1);

                // Make sure the duration gets set non-zero when this changes
                if (config.steam.achievements.popup.show)
                {
                  if ( config.steam.achievements.popup.duration == 0 )
                    config.steam.achievements.popup.duration = 6666UL;
                }
              }

              if (config.steam.achievements.popup.show)
              {
                ImGui::BeginGroup ( );
                ImGui::TreePush   ("");
                ImGui::Text       ("Duration:"); ImGui::SameLine ();

                float duration =
                  std::max ( 1.0f, ( (float)config.steam.achievements.popup.duration / 1000.0f ) );

                if ( ImGui::SliderFloat ( "##PopupDuration", &duration, 1.0f, 30.0f, "%.2f Seconds" ) )
                {
                  config.steam.achievements.popup.duration =
                    static_cast <LONG> ( duration * 1000.0f );
                }
                ImGui::TreePop   ( );
                ImGui::EndGroup  ( );
              }
              ImGui::EndGroup    ( );
              
              //ImGui::SliderFloat ("Inset Percentage",    &config.steam.achievements.popup.inset, 0.0f, 1.0f, "%.3f%%", 0.01f);
              ImGui::TreePop     ( );
            }

            ImGui::TreePop       ( );
            ImGui::PopStyleColor (3);
          }
        }

        ISteamUtils* utils =
          SK_SteamAPI_Utils ();

        if (utils != nullptr && utils->IsOverlayEnabled () && ImGui::CollapsingHeader ("Overlay Notifications"))
        {
          ImGui::TreePush  ("");

          if (ImGui::Combo ( " ", &config.steam.notify_corner,
                                    "Top-Left\0"
                                    "Top-Right\0"
                                    "Bottom-Left\0"
                                    "Bottom-Right\0"
                                    "(Let Game Decide)\0\0" ))
          {
            SK_Steam_SetNotifyCorner ();
          }

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Applies Only to Traditional Overlay (not Big Picture)");

          ImGui::TreePop ();
        }

        if (SK_Denuvo_UsedByGame ())
        {
          ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.00f, 0.00f, 0.00f, 1.00f));
          ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.00f, 0.00f, 0.00f, 1.00f));
          ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.00f, 0.00f, 0.00f, 1.00f));
          ImGui::PushStyleColor (ImGuiCol_Text,          ImColor::HSV (0.15f, 1.0f, 1.0f));

          if (ImGui::CollapsingHeader ("Denuvo"))
          {
            ImGui::PopStyleColor (4);

            ImGui::TreePush ("");

            size_t idx = 0;

            ImGui::BeginGroup ();
            for ( auto it : denuvo_files )
            {
              size_t found =
                it.path.find_last_of (L'\\');

              ImGui::BeginGroup      ();
              ImGui::Text            ( "Key %lu:",
                                             idx++ );
              ImGui::TextUnformatted ( "First Activated:" );
              ImGui::EndGroup        ();

              ImGui::SameLine        ();

              ImGui::BeginGroup      ();
              ImGui::Text            ( "%ws", &it.path.c_str ()[found + 1] );

              if (denuvo_files.size () > idx)
              {
                ImGui::SameLine      ();
                ImGui::TextColored   (ImColor::HSV (0.08f, 1.f, 1.f), " [ Expired ]");
              }

              ImGui::Text            ( "%02d/%02d/%d  %02d:%02d",
                                         it.st_local.wMonth, it.st_local.wDay, it.st_local.wYear,
                                         it.st_local.wHour,  it.st_local.wMinute );

              ImGui::EndGroup        ();
            }
            ImGui::EndGroup          ();

            ImGui::SameLine          ();

            ImGui::BeginGroup        ();
            for ( auto it : denuvo_files )
            {
              size_t found =
                it.path.find_last_of (L'\\');

              ImGui::PushID (_wtol (&it.path.c_str ()[found + 1]));

              if (ImGui::Button      ("  Delete Me  "))
              {
                DeleteFileW (it.path.c_str ());

                SK_Denuvo_UsedByGame (true); // Re-Test
              }

              if (ImGui::IsItemHovered ())
              {
                ImGui::BeginTooltip  ();
                ImGui::Text          ("Force Denuvo to Re-Activate the next time the Game Starts");
                ImGui::Separator     ();
                ImGui::BulletText    ("Useful if you plan to go offline for an extended period.");
                ImGui::BulletText    (" >> RESTART the game immediately after doing this to re-activate <<");
                ImGui::EndTooltip    ();
              }

              ImGui::TextUnformatted ( "" );

              ImGui::PopID           ();
            }
            ImGui::EndGroup          ();

            ImGui::TreePop  ();
          }

          else
          {
            ImGui::PopStyleColor   (4);
          }
        }

        ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
        ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
        ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

        if (ImGui::CollapsingHeader ("Compatibility"))
        {
          ImGui::TreePush ("");
          ImGui::Checkbox (" Bypass Online DRM Checks  ",          &config.steam.spoof_BLoggedOn);

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::TextColored  (ImColor::HSV (0.159f, 1.0f, 1.0f), "Fixes pesky games that use SteamAPI to deny Offline mode");
            ImGui::Separator    ();
            ImGui::BulletText   ("This is a much larger problem than you would believe.");
            ImGui::BulletText   ("This also fixes some games that crash when Steam disconnects (unrelated to DRM).");
            ImGui::EndTooltip   ();
          }

          ImGui::Checkbox (" Load Steam Overlay Early  ",          &config.steam.preload_overlay);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Can make the Steam Overlay work in situations it otherwise would not.");

          ImGui::SameLine ();

          ImGui::Checkbox (" Load Steam Client DLL Early  ",       &config.steam.preload_client);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("May prevent some Steam DRM-based games from hanging at startup.");

          ImGui::Checkbox (" Disable User Stats Receipt Callback", &config.steam.block_stat_callback);

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Text         ("Fix for Games that Panic when Flooded with Achievement Data");
            ImGui::Separator    ();
            ImGui::BulletText   ("These Games may shutdown SteamAPI when Special K fetches Friend Achievements");
            ImGui::BulletText   ("If SteamAPI Frame Counter is STUCK, turn this option ON and restart the Game");
            ImGui::EndTooltip   ();
          }

          ImGui::TreePop  ();
        }

        ImGui::PopStyleColor (3);

        bool valid = (! config.steam.silent);

        valid = valid && (! SK_Steam_PiratesAhoy ());

        if (valid)
        {
          bool publisher_is_stupid = false;

          if (config.steam.spoof_BLoggedOn)
          {
            auto status =
              static_cast <int> (SK_SteamUser_BLoggedOn ());

            if (status & static_cast <int> (SK_SteamUser_LoggedOn_e::Spoofing))
            {
              publisher_is_stupid = true;

              ImGui::PushStyleColor (ImGuiCol_TextDisabled,  ImColor::HSV (0.074f, 1.f, 1.f));
              ImGui::MenuItem       ("This game's publisher may consider you a pirate! :(", "", nullptr, false);
              ImGui::PopStyleColor  ();
            }
          }

          if (! publisher_is_stupid)
            ImGui::MenuItem ("I am not a pirate!", "", &valid, false);
        }

        else
        {
          ImGui::MenuItem (u8"I am probably a pirate moron™", "", &valid, false);
          {
            // Delete the CPY config, and push user back onto legitimate install,
            //   to prevent repeated pirate detection.
            //
            //  If stupid user presses this button for any other reason, that is
            //    their own problem. Crackers should make a better attempt to
            //      randomize their easily detectable hackjobs.
            if (GetFileAttributes (L"CPY.ini") != INVALID_FILE_ATTRIBUTES)
            {
              DeleteFileW (L"CPY.ini");
              
              MoveFileW   (L"steam_api64.dll",   L"CPY.ini");
              MoveFileW   (L"steamclient64.dll", L"steam_api64.dll");
              MoveFileW   (L"CPY.ini",           L"steamclient64.dll");
            }
          }
        }

        ImGui::PopStyleColor (3);
        ImGui::TreePop       ( );
      }

      SK_ImGui::BatteryMeter ();

      ImGui::Columns    ( 1 );
      ImGui::Separator  (   );

      if (SK::SteamAPI::GetNumPlayers () > 1)
      {
        ImGui::Columns    ( 2, "SteamSep", true );

        static char szNumber       [16] = { };
        static char szPrettyNumber [32] = { };

        const NUMBERFMTA fmt = { 0, 0, 3, ".", ",", 0 };

        snprintf (szNumber, 15, "%li", SK::SteamAPI::GetNumPlayers ());

        GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
                             0x00,
                               szNumber, &fmt,
                                 szPrettyNumber, 32 );

        ImGui::Text       (" %s Players in-Game on Steam  ", szPrettyNumber);
        ImGui::NextColumn (   );
      }

      extern volatile LONGLONG SK_SteamAPI_CallbackRunCount;

      ImGui::Bullet     ();   ImGui::SameLine ();

      bool pause =
        SK_SteamAPI_GetOverlayState (false);

      if (ImGui::Selectable ( "SteamAPI Frame", &pause ))
        SK_SteamAPI_SetOverlayState (pause);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip   (       );
        ImGui::Text           ( "In"  );                 ImGui::SameLine ();
        ImGui::PushStyleColor ( ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f) ); 
        ImGui::Text           ( "Steam Overlay Aware");  ImGui::SameLine ();
        ImGui::PopStyleColor  (       );
        ImGui::Text           ( "software, click to toggle the game's overlay pause mode." );
        ImGui::EndTooltip     (       );
      }

      ImGui::SameLine ();
      ImGui::Text     ( ": %10llu  ", InterlockedAdd64 (&SK_SteamAPI_CallbackRunCount, 0) );
      ImGui::Columns(1, nullptr, false);

      // If fetching stats and online...
      if ((! SK_SteamAPI_FriendStatsFinished ()) && SK_SteamAPI_GetNumPlayers () > 1)
      {
        float ratio   = SK_SteamAPI_FriendStatPercentage ();
        int   friends = SK_SteamAPI_GetNumFriends        ();

        static char szLabel [512] = { };

        snprintf ( szLabel, 511,
                     "Fetching Achievements... %.2f%% (%u/%u) : %s",
                       100.0f * ratio,
static_cast <uint32_t> (
                        ratio * static_cast <float>    (friends)
                       ),
                                static_cast <uint32_t> (friends),
                      SK_SteamAPI_GetFriendName (
static_cast <uint32_t> (
                        ratio * static_cast <float>    (friends))
                      )
                 );
        
        ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImColor (0.90f, 0.72f, 0.07f, 0.80f) ); 
        ImGui::PushStyleColor ( ImGuiCol_Text,          ImColor (255, 255, 255)              ); 
        ImGui::ProgressBar (
          SK_SteamAPI_FriendStatPercentage (),
            ImVec2 (-1, 0), szLabel );
        ImGui::PopStyleColor  (2);
      }
    }

  ImVec2 pos  = ImGui::GetWindowPos  ();
  ImVec2 size = ImGui::GetWindowSize ();

  SK_ImGui_LastWindowCenter.x = pos.x + size.x / 2.0f;
  SK_ImGui_LastWindowCenter.y = pos.y + size.y / 2.0f;

  if (recenter)
    SK_ImGui_CenterCursorAtPos ();


  if (show_test_window)
    ImGui::ShowTestWindow (&show_test_window);

  if (show_shader_mod_dlg)
    show_shader_mod_dlg = SK_D3D11_ShaderModDlg ();

  if (show_d3d9_shader_mod_dlg)
    show_d3d9_shader_mod_dlg = SK_D3D9_TextureModDlg ();

  ImGui::End   ();

  if (! open)
    SK_XInput_ZeroHaptics (config.input.gamepad.xinput.ui_slot);

  return open;
}

#if 0
#define __NvAPI_GetPhysicalGPUFromDisplay                 0x1890E8DA
#define __NvAPI_GetPhysicalGPUFromGPUID                   0x5380AD1A
#define __NvAPI_GetGPUIDfromPhysicalGPU                   0x6533EA3E

#define __NvAPI_GetInfoFrameStatePvt                      0x7FC17574
#define __NvAPI_GPU_GetMemoryInfo                         0x07F9B368

#define __NvAPI_LoadMicrocode                             0x3119F36E
#define __NvAPI_GetLoadedMicrocodePrograms                0x919B3136
#define __NvAPI_GetDisplayDriverBuildTitle                0x7562E947
#define __NvAPI_GetDisplayDriverCompileType               0x988AEA78
#define __NvAPI_GetDisplayDriverSecurityLevel             0x9D772BBA
#define __NvAPI_AccessDisplayDriverRegistry               0xF5579360
#define __NvAPI_GetDisplayDriverRegistryPath              0x0E24CEEE
#define __NvAPI_GetUnAttachedDisplayDriverRegistryPath    0x633252D8
#define __NvAPI_GPU_GetRawFuseData                        0xE0B1DCE9
#define __NvAPI_GPU_GetFoundry                            0x5D857A00
#define __NvAPI_GPU_GetVPECount                           0xD8CBF37B

#define __NvAPI_GPU_GetTargetID                           0x35B5FD2F

#define __NvAPI_GPU_GetShortName                          0xD988F0F3

#define __NvAPI_GPU_GetVbiosMxmVersion                    0xE1D5DABA
#define __NvAPI_GPU_GetVbiosImage                         0xFC13EE11
#define __NvAPI_GPU_GetMXMBlock                           0xB7AB19B9

#define __NvAPI_GPU_SetCurrentPCIEWidth                   0x3F28E1B9
#define __NvAPI_GPU_SetCurrentPCIESpeed                   0x3BD32008
#define __NvAPI_GPU_GetPCIEInfo                           0xE3795199
#define __NvAPI_GPU_ClearPCIELinkErrorInfo                0x8456FF3D
#define __NvAPI_GPU_ClearPCIELinkAERInfo                  0x521566BB
#define __NvAPI_GPU_GetFrameBufferCalibrationLockFailures 0x524B9773
#define __NvAPI_GPU_SetDisplayUnderflowMode               0x387B2E41
#define __NvAPI_GPU_GetDisplayUnderflowStatus             0xED9E8057

#define __NvAPI_GPU_GetBarInfo                            0xE4B701E3

#define __NvAPI_GPU_GetPSFloorSweepStatus                 0xDEE047AB
#define __NvAPI_GPU_GetVSFloorSweepStatus                 0xD4F3944C
#define __NvAPI_GPU_GetSerialNumber                       0x14B83A5F
#define __NvAPI_GPU_GetManufacturingInfo                  0xA4218928

#define __NvAPI_GPU_GetRamConfigStrap                     0x51CCDB2A
#define __NvAPI_GPU_GetRamBusWidth                        0x7975C581

#define __NvAPI_GPU_GetRamBankCount                       0x17073A3C
#define __NvAPI_GPU_GetArchInfo                           0xD8265D24
#define __NvAPI_GPU_GetExtendedMinorRevision              0x25F17421
#define __NvAPI_GPU_GetSampleType                         0x32E1D697
#define __NvAPI_GPU_GetHardwareQualType                   0xF91E777B
#define __NvAPI_GPU_GetAllClocks                          0x1BD69F49
#define __NvAPI_GPU_SetClocks                             0x6F151055
#define __NvAPI_GPU_SetPerfHybridMode                     0x7BC207F8
#define __NvAPI_GPU_GetPerfHybridMode                     0x5D7CCAEB
#define __NvAPI_GPU_GetHybridControllerInfo               0xD26B8A58
#define __NvAPI_GetHybridMode                             0x0E23B68C1

#define __NvAPI_RestartDisplayDriver                      0xB4B26B65
#define __NvAPI_GPU_GetAllGpusOnSameBoard                 0x4DB019E6

#define __NvAPI_SetTopologyDisplayGPU                     0xF409D5E5
#define __NvAPI_GetTopologyDisplayGPU                     0x813D89A8
#define __NvAPI_SYS_GetSliApprovalCookie                  0xB539A26E

#define __NvAPI_CreateUnAttachedDisplayFromDisplay        0xA0C72EE4
#define __NvAPI_GetDriverModel                            0x25EEB2C4
#define __NvAPI_GPU_CudaEnumComputeCapableGpus            0x5786CC6E
#define __NvAPI_GPU_PhysxSetState                         0x4071B85E
#define __NvAPI_GPU_PhysxQueryRecommendedState            0x7A4174F4
#define __NvAPI_GPU_GetDeepIdleState                      0x1AAD16B4
#define __NvAPI_GPU_SetDeepIdleState                      0x568A2292

#define __NvAPI_GetScalingCaps                            0x8E875CF9
#define __NvAPI_GPU_GetThermalTable                       0xC729203C
#define __NvAPI_GPU_GetHybridControllerInfo               0xD26B8A58
#define __NvAPI_SYS_SetPostOutput                         0xD3A092B1

std::wstring
ErrorMessage ( _NvAPI_Status err,
               const char*   args,
               UINT          line_no,
               const char*   function_name,
               const char*   file_name )
{
  char szError [256];

  NvAPI_GetErrorMessage (err, szError);

  wchar_t wszFormattedError [1024] = { };

  swprintf ( wszFormattedError, 1024,
              L"Line %u of %hs (in %hs (...)):\n"
              L"------------------------\n\n"
              L"NvAPI_%hs\n\n\t>> %hs <<",
               line_no,
                file_name,
                 function_name,
                  args,
                   szError );

  return wszFormattedError;
}


std::wstring
SK_NvAPI_GetGPUInfoStr (void)
{
  return L"";

  static wchar_t adapters [4096] = { };

  if (*adapters != L'\0')
    return adapters;

  if (sk::NVAPI::CountPhysicalGPUs ())
  {
    typedef NvU32 NvGPUID;

    typedef void*        (__cdecl *NvAPI_QueryInterface_pfn)                                   (unsigned int offset);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetRamType_pfn)            (NvPhysicalGpuHandle handle, NvU32* memtype);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetShaderPipeCount_pfn)    (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetShaderSubPipeCount_pfn) (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetFBWidthAndLocation_pfn) (NvPhysicalGpuHandle handle, NvU32* width, NvU32* loc);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetPartitionCount_pfn)     (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetTotalSMCount_pfn)       (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetTotalSPCount_pfn)       (NvPhysicalGpuHandle handle, NvU32* count);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetTotalTPCCount_pfn)      (NvPhysicalGpuHandle handle, NvU32* count);

    typedef NvAPI_Status (__cdecl *NvAPI_RestartDisplayDriver_pfn)      (void);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetSerialNumber_pfn)       (NvPhysicalGpuHandle handle,   NvU32* num);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetManufacturingInfo_pfn)  (NvPhysicalGpuHandle handle,    void* data);
    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetFoundry_pfn)            (NvPhysicalGpuHandle handle,    void* data);
    typedef NvAPI_Status (__cdecl *NvAPI_GetDriverModel_pfn)            (NvPhysicalGpuHandle handle,   NvU32* data);
    typedef NvAPI_Status (__cdecl *NvAPI_GetGPUIDFromPhysicalGPU_pfn)   (NvPhysicalGpuHandle handle, NvGPUID* gpuid);

    typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetShortName_pfn)          (NvPhysicalGpuHandle handle, NvAPI_ShortString str);
    typedef NvAPI_Status (__cdecl *NvAPI_GetHybridMode_pfn)             (NvPhysicalGpuHandle handle, NvU32*            mode);

#ifdef _WIN64
    HMODULE hLib = LoadLibrary (L"nvapi64.dll");
#else
    HMODULE hLib = LoadLibrary (L"nvapi.dll");
#endif
    static NvAPI_QueryInterface_pfn            NvAPI_QueryInterface            = (NvAPI_QueryInterface_pfn)GetProcAddress (hLib, "nvapi_QueryInterface");

    static NvAPI_GPU_GetRamType_pfn            NvAPI_GPU_GetRamType            = (NvAPI_GPU_GetRamType_pfn)NvAPI_QueryInterface            (0x57F7CAAC);
    static NvAPI_GPU_GetShaderPipeCount_pfn    NvAPI_GPU_GetShaderPipeCount    = (NvAPI_GPU_GetShaderPipeCount_pfn)NvAPI_QueryInterface    (0x63E2F56F);
    static NvAPI_GPU_GetShaderSubPipeCount_pfn NvAPI_GPU_GetShaderSubPipeCount = (NvAPI_GPU_GetShaderSubPipeCount_pfn)NvAPI_QueryInterface (0x0BE17923);
    static NvAPI_GPU_GetFBWidthAndLocation_pfn NvAPI_GPU_GetFBWidthAndLocation = (NvAPI_GPU_GetFBWidthAndLocation_pfn)NvAPI_QueryInterface (0x11104158);
    static NvAPI_GPU_GetPartitionCount_pfn     NvAPI_GPU_GetPartitionCount     = (NvAPI_GPU_GetPartitionCount_pfn)NvAPI_QueryInterface     (0x86F05D7A);
    static NvAPI_RestartDisplayDriver_pfn      NvAPI_RestartDisplayDriver      = (NvAPI_RestartDisplayDriver_pfn)NvAPI_QueryInterface      (__NvAPI_RestartDisplayDriver);
    static NvAPI_GPU_GetSerialNumber_pfn       NvAPI_GPU_GetSerialNumber       = (NvAPI_GPU_GetSerialNumber_pfn)NvAPI_QueryInterface       (__NvAPI_GPU_GetSerialNumber);
    static NvAPI_GPU_GetManufacturingInfo_pfn  NvAPI_GPU_GetManufacturingInfo  = (NvAPI_GPU_GetManufacturingInfo_pfn)NvAPI_QueryInterface  (__NvAPI_GPU_GetManufacturingInfo);
    static NvAPI_GPU_GetFoundry_pfn            NvAPI_GPU_GetFoundry            = (NvAPI_GPU_GetFoundry_pfn)NvAPI_QueryInterface            (__NvAPI_GPU_GetFoundry);
    static NvAPI_GetDriverModel_pfn            NvAPI_GetDriverModel            = (NvAPI_GetDriverModel_pfn)NvAPI_QueryInterface            (__NvAPI_GetDriverModel);
    static NvAPI_GPU_GetShortName_pfn          NvAPI_GPU_GetShortName          = (NvAPI_GPU_GetShortName_pfn)NvAPI_QueryInterface          (__NvAPI_GPU_GetShortName);

    static NvAPI_GPU_GetTotalSMCount_pfn       NvAPI_GPU_GetTotalSMCount       = (NvAPI_GPU_GetTotalSMCount_pfn)NvAPI_QueryInterface       (0x0AE5FBCFE);// 0x329D77CD);// 0x0AE5FBCFE);
    static NvAPI_GPU_GetTotalSPCount_pfn       NvAPI_GPU_GetTotalSPCount       = (NvAPI_GPU_GetTotalSPCount_pfn)NvAPI_QueryInterface       (0xE4B701E3);// 0xE0B1DCE9);// 0x0B6D62591);
    static NvAPI_GPU_GetTotalTPCCount_pfn      NvAPI_GPU_GetTotalTPCCount      = (NvAPI_GPU_GetTotalTPCCount_pfn)NvAPI_QueryInterface      (0x4E2F76A8);
    //static NvAPI_GPU_GetTotalTPCCount_pfn NvAPI_GPU_GetTotalTPCCount = (NvAPI_GPU_GetTotalTPCCount_pfn)NvAPI_QueryInterface (0xD8265D24);// 0x4E2F76A8);// __NvAPI_GPU_Get
    static NvAPI_GetGPUIDFromPhysicalGPU_pfn   NvAPI_GetGPUIDFromPhysicalGPU   = (NvAPI_GetGPUIDFromPhysicalGPU_pfn)NvAPI_QueryInterface   (__NvAPI_GetGPUIDfromPhysicalGPU);
    static NvAPI_GetHybridMode_pfn             NvAPI_GetHybridMode             = (NvAPI_GetHybridMode_pfn)NvAPI_QueryInterface             (__NvAPI_GetHybridMode);
  }

  return adapters;
}
#endif


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



//
// Hook this to override Special K's GUI
//
__declspec (dllexport)
DWORD
SK_ImGui_DrawFrame ( _Unreferenced_parameter_ DWORD  dwFlags, 
                                              LPVOID lpUser )
{
//  static bool skip;
//
//  if (dwFlags == 0)
//    skip = true;
//
//  if (skip)
//    return 0;
//
//  if (dwFlags == 1)
//    skip = false;

  UNREFERENCED_PARAMETER (dwFlags);
  UNREFERENCED_PARAMETER (lpUser);

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
    return 0x00;

  ImGuiIO& io (ImGui::GetIO ());


  //
  // Framerate history
  //
  if (! reset_frame_history)
  {
    SK_ImGui_Frames.timeFrame (io.DeltaTime);
  }

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

  bool keep_open = true;


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


  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  if (d3d11)
  {
    if (SK_ImGui_Widgets.texcache)
    {
      static bool move = true;
      if (move)
      {
        ImGui::SetNextWindowPos (ImVec2 ((io.DisplaySize.x + font_size * 35) / 2.0f, 0.0f), ImGuiSetCond_Always);
        move = false;
      }

      ImGui::SetNextWindowSize  (ImVec2 (font_size * 35, font_size_multiline * 6.25f),      ImGuiSetCond_Always);
      
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
  {
    SK_ReShade_Visible = true;

    SK_ImGui_ConfirmExit ();

    if ( ImGui::BeginPopupModal ( "Confirm Forced Software Termination",
                                    nullptr,
                                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                      ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse )
       )
    {
      ImGui::TextColored ( ImColor::HSV (0.075f, 1.0f, 1.0f), "\n         You will lose any unsaved game progress.         \n\n");

      ImGui::Separator ();

      ImGui::TextColored ( ImColor::HSV (0.15f, 1.0f, 1.0f), " Confirm Exit? " );

      ImGui::SameLine ();

      ImGui::Spacing (); ImGui::SameLine ();

      if (ImGui::Button ("Okay"))
        ExitProcess (0x00);

      //ImGui::PushItemWidth (ImGui::GetWindowContentRegionWidth () * 0.33f); ImGui::SameLine (); ImGui::SameLine (); ImGui::PopItemWidth ();

      ImGui::SameLine ();

      if (ImGui::Button ("Cancel"))
      {
        SK_ImGui_WantExit  = false;
        SK_ReShade_Visible = false;
        ImGui::CloseCurrentPopup ();
      }

      ImGui::SetItemDefaultFocus ();

      ImGui::SameLine ();
      ImGui::TextUnformatted (" ");
      ImGui::SameLine ();

      ImGui::Checkbox ("Enable Alt + F4", &config.input.keyboard.catch_alt_f4);

      ImGui::EndPopup ();
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


  if (d3d9)
  {
    extern LPDIRECT3DDEVICE9 g_pd3dDevice;

    if ( SUCCEEDED (
           g_pd3dDevice->BeginScene ()
         )
       )
    {
      ImGui::Render          ();
      g_pd3dDevice->EndScene ();
    }
  }

  else if (d3d11)
    ImGui::Render ();

  else if (gl)
    ImGui::Render ();

  if (! keep_open)
    SK_ImGui_Toggle ();


  POINT                    orig_pos;
  GetCursorPos_Original  (&orig_pos);

  SK_ImGui_Cursor.update ();

  SetCursorPos_Original  (orig_pos.x, orig_pos.y);

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

  static DWORD dwLastTime = 0x00;

  bool d3d11 = false;
  bool d3d9  = false;
  bool gl    = false;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9) )  d3d9  = true;
  if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11))  d3d11 = true;
  if (                   rb.api ==                    SK_RenderAPI::OpenGL)  gl    = true;

  auto EnableEULAIfPirate = [](void) ->
  bool
  {
    bool pirate = ( SK_SteamAPI_AppID    () != 0 && 
                    SK_Steam_PiratesAhoy () != 0x0 );
    if (pirate)
    {
      if (dwLastTime < timeGetTime () - 1000)
      {
        dwLastTime             = timeGetTime ();

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

    static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");
    static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");

    //if (hModTZFix == nullptr)
    {
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

      // TBFix
      else
        EnableEULAIfPirate ();
    }

    // TZFix
    if (hModTZFix)
      EnableEULAIfPirate ();

    if (SK_ImGui_Visible)
      SK_Console::getInstance ()->visible = false;


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
      extern bool nav_usable;
      nav_usable = false;
    }

    ImGui::SetNextWindowFocus ();

    reset_frame_history = true;
  }

  if ((! SK_ImGui_Visible) && SK_ImGui_OpenCloseCallback.fn != nullptr)
  {
    if (SK_ImGui_OpenCloseCallback.fn (SK_ImGui_OpenCloseCallback.data))
      SK_ImGui_OpenCloseCallback.fn (SK_ImGui_OpenCloseCallback.data);
  }
}

void
SK_ImGui_CenterCursorAtPos (ImVec2 center)
{
  ImGuiIO& io (ImGui::GetIO ());

  SK_ImGui_Cursor.pos.x = static_cast <LONG> (center.x);
  SK_ImGui_Cursor.pos.y = static_cast <LONG> (center.y);
  
  io.MousePos.x = center.x;
  io.MousePos.y = center.y;

  POINT screen_pos = SK_ImGui_Cursor.pos;

  if (GetCursor () != nullptr)
    SK_ImGui_Cursor.orig_img = GetCursor ();

  SK_ImGui_Cursor.LocalToScreen (&screen_pos);
  SetCursorPos_Original         ( screen_pos.x,
                                  screen_pos.y );

  io.WantCaptureMouse = true;

  SK_ImGui_UpdateCursor ();
}

void
SK_ImGui_CenterCursorOnWindow (void)
{
  return SK_ImGui_CenterCursorAtPos ();
}



__declspec (dllexport)
void
__stdcall
SK_ImGui_KeybindDialog (SK_Keybind* keybind)
{
  ImGuiIO& io (ImGui::GetIO ());

  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  ImGui::SetNextWindowSizeConstraints ( ImVec2   (font_size *  9, font_size * 3),
                                          ImVec2 (font_size * 30, font_size * 6) );

  if (ImGui::BeginPopupModal (keybind->bind_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                                           ImGuiWindowFlags_NoCollapse       | ImGuiWindowFlags_NoSavedSettings))
  {
    io.WantCaptureKeyboard = true;

    int i = 0;

    for (i = 0x08; i < 256; i++)
    {
      if ( i == VK_LCONTROL || i == VK_RCONTROL || i == VK_CONTROL ||
           i == VK_LSHIFT   || i == VK_RSHIFT   || i == VK_SHIFT   ||
           i == VK_LMENU    || i == VK_RMENU    || i == VK_MENU )
        continue;

      if ( io.KeysDownDuration [i] == 0.0 )
        break;
    }

    if (i != 256)
    {
      keybind->vKey = (SHORT)i;
      ImGui::CloseCurrentPopup ();
    }

    keybind->ctrl  = io.KeyCtrl;
    keybind->shift = io.KeyShift;
    keybind->alt   = io.KeyAlt;

    keybind->update ();

    ImGui::Text ("Binding:  %ws", keybind->human_readable.c_str ());

    ImGui::EndPopup ();
  }
}

struct SK_GamepadCombo_V0 {
  const wchar_t** button_names     = nullptr;
  std::string     combo_name       =  "";
  std::wstring    unparsed         = L"";
  int             buttons          = 0;
};

bool SK_ImGui_GamepadComboDialogActive = false;

__declspec (dllexport)
INT
__stdcall
SK_ImGui_GamepadComboDialog0 (SK_GamepadCombo_V0* combo)
{
  ImGuiIO& io (ImGui::GetIO ());

  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  ImGui::SetNextWindowSizeConstraints ( ImVec2   (font_size *  9, font_size * 3),
                                          ImVec2 (font_size * 30, font_size * 6) );

  if (ImGui::BeginPopupModal (combo->combo_name.c_str (), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                                                   ImGuiWindowFlags_NoCollapse       | ImGuiWindowFlags_NoSavedSettings))
  {
    SK_ImGui_GamepadComboDialogActive = true;
    nav_usable                        = false;
    io.NavUsable                      = false;
    io.NavActive                      = false;

    io.WantCaptureKeyboard = true;

    static WORD                last_buttons     = 0;
    static DWORD               last_change      = 0;
    static BYTE                last_trigger_l   = 0;
    static BYTE                last_trigger_r   = 0;
    static SK_GamepadCombo_V0* last_combo       = nullptr;
           XINPUT_STATE        state            = { };
    static std::wstring        unparsed         = L"";

    if (SK_XInput_PollController (0, &state))
    {
      if (last_combo != combo)
      {
        unparsed       = L"";
        last_combo     = combo;
        last_change    = 0;
        last_buttons   = state.Gamepad.wButtons;
        last_trigger_l = state.Gamepad.bLeftTrigger;
        last_trigger_r = state.Gamepad.bRightTrigger;
      }

      if ( last_buttons != state.Gamepad.wButtons || ( ( state.Gamepad.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD && last_trigger_l < XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ||
                                                       ( state.Gamepad.bLeftTrigger  < XINPUT_GAMEPAD_TRIGGER_THRESHOLD && last_trigger_l > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) )  ||
                                                     ( ( state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD && last_trigger_r < XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ||
                                                       ( state.Gamepad.bRightTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD && last_trigger_r > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ) )
      {
        last_trigger_l = state.Gamepad.bLeftTrigger;
        last_trigger_r = state.Gamepad.bRightTrigger;

        std::queue <const wchar_t*> buttons;

        if (state.Gamepad.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
          buttons.push (combo->button_names [16]);

        if (state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
          buttons.push (combo->button_names [17]);

        for (int i = 0; i < 16; i++)
        {
          if (state.Gamepad.wButtons & ( 1 << i ))
          {
            buttons.push (combo->button_names [i]);
          }
        }

        unparsed = L"";

        while (! buttons.empty ())
        {
          unparsed += buttons.front ();
                      buttons.pop   ();

          if (! buttons.empty ())
            unparsed += L"+";
        }

        last_buttons = state.Gamepad.wButtons;
        last_change  = timeGetTime ();
      }

      else if (last_change > 0 && last_change < timeGetTime () - 1000UL)
      {
        combo->unparsed = unparsed;

        for (int i = 0; i < 16; i++)
        {
          io.NavInputsDownDuration     [i] = 0.1f;
          io.NavInputsDownDurationPrev [i] = 0.1f;
        }

        SK_ImGui_GamepadComboDialogActive = false;
        nav_usable                        = true;
        io.NavUsable                      = true;
        io.NavActive                      = true;
        last_combo                        = nullptr;
        ImGui::CloseCurrentPopup ();
        ImGui::EndPopup          ();
        return 1;
      }
    }

    if (io.KeysDown [VK_ESCAPE])
    {
      last_combo                          = nullptr;
      ImGui::CloseCurrentPopup ();
      ImGui::EndPopup          ();
      return -1;
    }

    ImGui::Text ("Binding:  %ws", unparsed.c_str ());

    ImGui::EndPopup ();
  }

  return 0;
}