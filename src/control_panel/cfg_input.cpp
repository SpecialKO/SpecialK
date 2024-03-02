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

#include <SpecialK/control_panel/input.h>

#include <hidclass.h>

#include <imgui/font_awesome.h>

bool cursor_vis = false;

using namespace SK::ControlPanel;

extern std::vector <RAWINPUTDEVICE>
SK_RawInput_GetMice      (bool* pDifferent = nullptr);
extern std::vector <RAWINPUTDEVICE>
SK_RawInput_GetKeyboards (bool* pDifferent = nullptr);

extern int SK_ImGui_ProcessGamepadStatusBar (bool bDraw);

SK_LazyGlobal <SK::Framerate::Stats> gamepad_stats;
SK_LazyGlobal <SK::Framerate::Stats> gamepad_stats_filtered;

void SK_ImGui_UpdateCursor (void)
{
  POINT             orig_pos;
  SK_GetCursorPos (&orig_pos);

  SK_SetCursorPos (game_window.actual.window.left + (game_window.actual.window.right  - game_window.actual.window.left) / 2,
                   game_window.actual.window.top  + (game_window.actual.window.bottom - game_window.actual.window.top)  / 2);

  SK_ImGui_Cursor.update ();

  SK_SetCursorPos (orig_pos.x, orig_pos.y);
}

extern bool SK_Window_IsCursorActive   (void);
extern bool SK_Window_ActivateCursor   (bool changed      = false);
extern bool SK_Window_DeactivateCursor (bool ignore_imgui = false);

extern ImVec2& __SK_ImGui_LastWindowCenter (void);
#define SK_ImGui_LastWindowCenter  __SK_ImGui_LastWindowCenter()

void
SK_ImGui_CenterCursorAtPos (ImVec2& center = SK_ImGui_LastWindowCenter)
{
  auto& io =
    ImGui::GetIO ();

  // Ignore this if the cursor is in a different application
  POINT                 ptCursor;
  if (SK_GetCursorPos (&ptCursor))
  {
    GetWindowRect (game_window.hWnd,
                  &game_window.actual.window);
    if (PtInRect (&game_window.actual.window, ptCursor))
    {
      if (center.x == -1.0f && center.y == -1.0f)
      {
        center.x = io.DisplaySize.x / 2;
        center.y = io.DisplaySize.y / 2;
      }

      SK_ImGui_Cursor.pos.x = static_cast <LONG> (center.x);
      SK_ImGui_Cursor.pos.y = static_cast <LONG> (center.y);

      io.MousePos.x = center.x;
      io.MousePos.y = center.y;

      POINT screen_pos = SK_ImGui_Cursor.pos;

      SK_ImGui_Cursor.LocalToScreen (&screen_pos);
      SK_SetCursorPos               ( screen_pos.x,
                                      screen_pos.y );
    }
  }
}

void
SK_ImGui_CenterCursorOnWindow (void)
{
  return
    SK_ImGui_CenterCursorAtPos ();
}

static DWORD last_xinput     = 0;
static DWORD last_scepad     = 0;
static DWORD last_wgi        = 0;
static DWORD last_hid        = 0;
static DWORD last_di7        = 0;
static DWORD last_di8        = 0;
static DWORD last_steam      = 0;
static DWORD last_messagebus = 0;
static DWORD last_rawinput   = 0;
static DWORD last_winhook    = 0;
static DWORD last_winmm      = 0;
static DWORD last_win32      = 0;

static DWORD hide_xinput     = 0;
static DWORD hide_scepad     = 0;
static DWORD hide_wgi        = 0;
static DWORD hide_hid        = 0;
static DWORD hide_di7        = 0;
static DWORD hide_di8        = 0;
static DWORD hide_steam      = 0;
static DWORD hide_messagebus = 0;
static DWORD hide_rawinput   = 0;
static DWORD hide_winhook    = 0;
static DWORD hide_winmm      = 0;
static DWORD hide_win32      = 0;

bool
SK::ControlPanel::Input::Draw (void)
{
  auto& io =
    ImGui::GetIO ();

  const bool bHasPlayStation =
    (last_scepad != 0 || (! SK_HID_PlayStationControllers.empty ()));

  if (bHasPlayStation)
    ImGui::SetNextItemOpen (true, ImGuiCond_Once);

  const bool input_mgmt_open =
    ImGui::CollapsingHeader ("Input Management");

  if (config.imgui.show_input_apis)
  {
    auto perfNow =
      static_cast <uint64_t> (SK_QueryPerf ().QuadPart);

    struct { ULONG reads  [XUSER_MAX_COUNT]; } xinput     { };
    struct { ULONG reads;                    } sce_pad    { };
    struct { ULONG reads;                    } wgi        { };
    struct { ULONG reads  [XUSER_MAX_COUNT];
             BOOL  active [XUSER_MAX_COUNT]; } steam      { };
    struct { ULONG reads;                    } winmm      { };
    struct { ULONG reads;                    } messagebus { };

    struct { ULONG kbd_reads, mouse_reads;   } winhook    { };

    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } di7       { };
    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } di8       { };
    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } hid       { };
    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } raw_input { };

    struct { ULONG cursorpos,      keystate,
               keyboardstate, asynckeystate;              } win32     { };

    xinput.reads [0]        = SK_XInput_Backend->reads     [0];
    xinput.reads [1]        = SK_XInput_Backend->reads     [1];
    xinput.reads [2]        = SK_XInput_Backend->reads     [2];
    xinput.reads [3]        = SK_XInput_Backend->reads     [3];

    steam.reads  [0]        = SK_Steam_Backend->reads      [0];
    steam.active [0]        = 
      ReadULong64Acquire (&SK_Steam_Backend->viewed.gamepad_xbox       ) > (perfNow - SK_PerfFreq / 2);
    steam.reads  [1]        = SK_Steam_Backend->reads      [1];
    steam.active [1]        =
      ReadULong64Acquire (&SK_Steam_Backend->viewed.gamepad_playstation) > (perfNow - SK_PerfFreq / 2);
    steam.reads  [2]        = SK_Steam_Backend->reads      [2];
    steam.active [2]        =
      ReadULong64Acquire (&SK_Steam_Backend->viewed.gamepad_generic    ) > (perfNow - SK_PerfFreq / 2);
    steam.reads  [3]        = SK_Steam_Backend->reads      [3];
    steam.active [3]        =
      ReadULong64Acquire (&SK_Steam_Backend->viewed.gamepad_nintendo   ) > (perfNow - SK_PerfFreq / 2);

    sce_pad.reads           = SK_ScePad_Backend->reads     [2/*sk_input_dev_type::Gamepad*/];
    wgi.reads               = SK_WGI_Backend->reads        [2/*sk_input_dev_type::Gamepad*/];
    winmm.reads             = SK_WinMM_Backend->reads      [2];
    messagebus.reads        = SK_MessageBus_Backend->reads [2];

    winhook.kbd_reads       = SK_WinHook_Backend->reads    [1];
    winhook.mouse_reads     = SK_WinHook_Backend->reads    [0];

    di7.kbd_reads           = SK_DI7_Backend->reads        [1];
    di7.mouse_reads         = SK_DI7_Backend->reads        [0];
    di7.gamepad_reads       = SK_DI7_Backend->reads        [2];

    di8.kbd_reads           = SK_DI8_Backend->reads        [1];
    di8.mouse_reads         = SK_DI8_Backend->reads        [0];
    di8.gamepad_reads       = SK_DI8_Backend->reads        [2];

    hid.kbd_reads           = SK_HID_Backend->reads        [1];
    hid.mouse_reads         = SK_HID_Backend->reads        [0];
    hid.gamepad_reads       = SK_HID_Backend->reads        [2];

    raw_input.kbd_reads     = SK_RawInput_Backend->reads   [1];
    raw_input.mouse_reads   = SK_RawInput_Backend->reads   [0];
    raw_input.gamepad_reads = SK_RawInput_Backend->reads   [2];

    win32.asynckeystate     = SK_Win32_Backend->reads      [3];
    win32.keyboardstate     = SK_Win32_Backend->reads      [2];
    win32.keystate          = SK_Win32_Backend->reads      [1];
    win32.cursorpos         = SK_Win32_Backend->reads      [0];


    // Implictly hide slots before tallying read/write activity, these slots
    //   read-back neutral data and are effectively hidden
    if (config.input.gamepad.xinput.disable [0]) SK_XInput_Backend->last_frame.hidden [0] = 1;
    if (config.input.gamepad.xinput.disable [1]) SK_XInput_Backend->last_frame.hidden [1] = 1;
    if (config.input.gamepad.xinput.disable [2]) SK_XInput_Backend->last_frame.hidden [2] = 1;
    if (config.input.gamepad.xinput.disable [3]) SK_XInput_Backend->last_frame.hidden [3] = 1;


#define UPDATE_BACKEND_TIMES(backend,name,func)                                         \
  if (SK_##backend##_Backend->##func## ())                                              \
  {                                                                                     \
    last_##name = SK_##backend##_Backend->active.hidden ? last_##name   : current_time; \
    hide_##name = SK_##backend##_Backend->active.hidden ? current_time  : hide_##name;  \
  }

    UPDATE_BACKEND_TIMES (XInput,         xinput, nextFrame);
    UPDATE_BACKEND_TIMES (ScePad,         scepad, nextFrame);
    UPDATE_BACKEND_TIMES (WGI,               wgi, nextFrame);
    UPDATE_BACKEND_TIMES (MessageBus, messagebus, nextFrame);
    UPDATE_BACKEND_TIMES (Steam,           steam, nextFrame);
    UPDATE_BACKEND_TIMES (HID,               hid, nextFrame);
    UPDATE_BACKEND_TIMES (DI7,               di7, nextFrame);
    UPDATE_BACKEND_TIMES (DI8,               di8, nextFrame);
    UPDATE_BACKEND_TIMES (RawInput,     rawinput, nextFrame);
    UPDATE_BACKEND_TIMES (WinHook,       winhook, nextFrame);
    UPDATE_BACKEND_TIMES (Win32,           win32, nextFrameWin32);
    UPDATE_BACKEND_TIMES (WinMM,           winmm, nextFrame);

#define SETUP_LABEL_COLOR(name,threshold)                               \
      const DWORD input_time = std::max (last_##name##, hide_##name##); \
                                                                        \
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * ((float)current_time - \
                                                                          (float)  input_time) / (threshold)), (hide_##name## >= last_##name##) ? 0.0f : 1.0f, \
                                                                                                               (hide_##name## >= last_##name##) ? 0.6f : 0.8f).Value);

    if ( last_steam > current_time - 500UL ||
         hide_steam > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (steam, 500.0f);

      if (SK::SteamAPI::AppID () > 0)
      {
        ImGui::SameLine ( );
        ImGui::Text ("       Steam");
        ImGui::PopStyleColor ( );

        //bool is_emulating_xinput =
        //  steam_ctx.Input ()->GetControllerForGamepadIndex (0) != 0;

        if (ImGui::IsItemHovered ( ))
        {
          ImGui::BeginTooltip ( );
          {
            ImGui::BeginGroup  ( );
              if (steam.active [0]) ImGui::TextUnformatted ("Xbox");
              if (steam.active [1]) ImGui::TextUnformatted ("PlayStation");
              if (steam.active [2]) ImGui::TextUnformatted ("Generic");
              if (steam.active [3]) ImGui::TextUnformatted ("Nintendo");
            ImGui::EndGroup    ( );
            ImGui::SameLine    ( );
            ImGui::BeginGroup  ( );
              if (steam.active [0]) ImGui::Text ("%lu", steam.reads [0]);
              if (steam.active [1]) ImGui::Text ("%lu", steam.reads [1]);
              if (steam.active [2]) ImGui::Text ("%lu", steam.reads [2]);
              if (steam.active [3]) ImGui::Text ("%lu", steam.reads [3]);
            ImGui::EndGroup    ( );

          //if (is_emulating_xinput)
          //{
          //  ImGui::Separator  ();
          //  ImGui::BulletText ("Click to configure (XInput Emulation)");
          //}
          }
          ImGui::EndTooltip ( );
        }

        //if (is_emulating_xinput && ImGui::IsItemClicked ())
        //{
        //  steam_ctx.Input ()->ShowBindingPanel (
        //    steam_ctx.Input ()->GetControllerForGamepadIndex (0)
        //  );
        //}
      }
    }

    if ( last_xinput > current_time - 500UL ||
         hide_xinput > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (xinput, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       %s", SK_XInput_GetPrimaryHookName ());
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::BeginGroup   ();
        for ( int i = 0 ; i < XUSER_MAX_COUNT ; ++i )
        {
          if (xinput.reads [i] > 0)
            ImGui::Text     ("Gamepad %d     %lu", i, xinput.reads [i]);
        }
        ImGui::EndGroup     ();
        ImGui::SameLine     ();
        ImGui::BeginGroup   ();
        for ( int i = 0 ; i < XUSER_MAX_COUNT ; ++i )
        {
          if (xinput.reads [i] > 0)
          {
            ImGui::TextColored ( ImVec4 (1.f, 0.f, 0.f, 1.f),
              config.input.gamepad.xinput.disable [i] ?
                                      "  Disabled" : "" );
          }
        }
        ImGui::EndGroup     ();
        ImGui::EndTooltip   ();
      }
    }

    if ( last_wgi > current_time - 500UL ||
         hide_wgi > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (wgi, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       Windows.Gaming.Input");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ( ))
      {
        ImGui::BeginTooltip ( );
        ImGui::Text ("Gamepad     %lu", wgi.reads);
        ImGui::EndTooltip ( );
      }
    }

    if ( last_scepad > current_time - 500UL ||
         hide_scepad > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (scepad, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       PlayStation");
      ImGui::PopStyleColor  ();
    }

    if ( last_hid > current_time - 500UL ||
         hide_hid > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (hid, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       HID");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (hid.kbd_reads > 0)
          ImGui::Text       ("Keyboard    %lu", hid.kbd_reads);
        if (hid.mouse_reads > 0)
          ImGui::Text       ("Mouse       %lu", hid.mouse_reads);
        if (hid.gamepad_reads > 0)
          ImGui::Text       ("Gamepad     %lu", hid.gamepad_reads);

        ImGui::EndTooltip   ();
      }
    }

    if ( last_winmm > current_time - 500UL ||
         hide_winmm > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (winmm, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("    WinMM Joystick");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        if (winmm.reads > 0)
          ImGui::Text       ("Gamepad     %lu", winmm.reads);
        ImGui::EndTooltip   ();
      }
    }

    // MessageBus is technically third-party software polling input from within
    //  the game's process... reporting its activity is not useful to end-users
#if 0
    if (last_messagebus > current_time - 500UL || hide_messagebus > current_time - 500UL)
    {
      SETUP_LABEL_COLOR (messagebus, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       NVIDIA MessageBus");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        if (messagebus.reads > 0)
          ImGui::Text       ("Gamepad     %lu", messagebus.reads);
        ImGui::EndTooltip   ();
      }
    }
#endif

    if ( last_di7 > current_time - 500UL ||
         hide_di7 > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (di7, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       DirectInput 7");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (di7.kbd_reads > 0) {
          ImGui::Text       ("Keyboard  %lu", di7.kbd_reads);
        }
        if (di7.mouse_reads > 0) {
          ImGui::Text       ("Mouse     %lu", di7.mouse_reads);
        }
        if (di7.gamepad_reads > 0) {
          ImGui::Text       ("Gamepad   %lu", di7.gamepad_reads);
        };

        ImGui::EndTooltip   ();
      }
    }

    if ( last_di8 > current_time - 500UL ||
         hide_di8 > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (di8, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       DirectInput 8");
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

    if ( last_winhook > current_time - 10000UL ||
         hide_winhook > current_time - 10000UL )
    {
      SETUP_LABEL_COLOR (winhook, 10000.0f);

      ImGui::SameLine      ();
      ImGui::Text ("       Windows Hook");
      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        if (winhook.mouse_reads > 0)
          ImGui::Text ("Mouse      %lu", winhook.mouse_reads);
        if (winhook.kbd_reads > 0)
          ImGui::Text ("Keyboard   %lu", winhook.kbd_reads);
        ImGui::EndTooltip   ();
      }
    }

    if ( last_win32 > current_time - 10000UL ||
         hide_win32 > current_time - 10000UL )
    {
      SETUP_LABEL_COLOR (win32, 10000.0f);

      ImGui::SameLine      ();
      ImGui::Text ("       Win32");
      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::BeginGroup   ();
        if (win32.asynckeystate > 0)
          ImGui::TextUnformatted ("GetAsyncKeyState\t");
        if (win32.keystate > 0)
          ImGui::TextUnformatted ("GetKeyState\t");
        if (win32.keyboardstate > 0)
          ImGui::TextUnformatted ("GetKeyboardState\t");
        if (win32.cursorpos > 0)
          ImGui::TextUnformatted ("GetCursorPos\t");

        ImGui::EndGroup     ();
        ImGui::SameLine     ();
        ImGui::BeginGroup   ();

        if (win32.asynckeystate > 0)
          ImGui::Text ("%lu", win32.asynckeystate);
        if (win32.keystate > 0)
          ImGui::Text ("%lu", win32.keystate);
        if (win32.keyboardstate > 0)
          ImGui::Text ("%lu", win32.keyboardstate);
        if (win32.cursorpos > 0)
          ImGui::Text ("%lu", win32.cursorpos);
        ImGui::EndGroup     ();
        ImGui::EndTooltip   ();
      }
    }

    // Place Raw Input at the end, since it's the most likely to disappear
    if ( last_rawinput > current_time - 500UL ||
         hide_rawinput > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (rawinput, 500.0f);

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
      auto _CursorBoundaryWidget = [&]()
      {
        ImGui::BeginGroup             ();
        ImGui::SeparatorEx            (ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine               ();
        SK_ImGui_CursorBoundaryConfig ();
        ImGui::EndGroup               ();
      };

      ImGui::TreePush ("");
      ImGui::BeginGroup ();
      ImGui::BeginGroup ();

      bool bIdleHideChange =
      ImGui::Checkbox ( "Hide When Not Moved", &config.input.cursor.manage   );
      
      if ( bIdleHideChange )
        SK_ImGui_Cursor.force = sk_cursor_state::None;

      ImVec2 vCursorWidgetPos (0.0f, 0.0f);

      if (config.input.cursor.manage) {
        ImGui::TreePush ("");
        ImGui::Checkbox ( "Keyboard Activates",
                                          &config.input.cursor.keys_activate );
#if 1
        ImGui::Checkbox ( "Gamepad Deactivates",
                                    &config.input.cursor.gamepad_deactivates );
        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip (
            "Uses XInput or HID (PlayStation) to auto-hide the cursor "
            "on gamepad input."
          );
        }
#endif
        ImGui::TreePop  ();
        ImGui::SameLine ();

        vCursorWidgetPos =
          ImGui::GetCursorPos ();
      }

      ImGui::EndGroup   ();
      ImGui::BeginGroup ();

      if (config.input.cursor.manage)
      {
        ImGui::SameLine   ();

        float seconds =
          (float)config.input.cursor.timeout  / 1000.0f;

        const float val =
          config.input.cursor.manage ? 1.0f : 0.0f;

        ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImVec4 ( 0.3f,  0.3f,  0.3f,  val));
        ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImVec4 ( 0.6f,  0.6f,  0.6f,  val));
        ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImVec4 ( 0.9f,  0.9f,  0.9f,  val));
        ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImVec4 ( 1.0f,  1.0f,  1.0f, 1.0f));

        if ( ImGui::SliderFloat ( "Seconds Before Hiding",
                                    &seconds, 0.0f, 10.0f ) )
        {
          config.input.cursor.timeout =
            static_cast <LONG> ( seconds * 1000.0f );
        }

        ImGui::PopStyleColor (4);

        ImGui::SetCursorPos   (vCursorWidgetPos);
        _CursorBoundaryWidget ();
      }

#if 1
      if (! config.input.cursor.manage)
      {
        if (SK_ImGui_Cursor.force == sk_cursor_state::None)
        {
          if (! SK_InputUtil_IsHWCursorVisible ())
          {
            if (ImGui::Button (" Force Mouse Cursor Visible "))
            {
              SK_ImGui_Cursor.force = sk_cursor_state::Visible;
            }
          }

          else
          {
            if (ImGui::Button (" Force Mouse Cursor Hidden "))
            {
              SK_ImGui_Cursor.force = sk_cursor_state::Hidden;
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::SetTooltip (
                "May not work in some games, auto-hide (0.0 seconds) may help..."
              );
            }
          }
        }

        else
        {
          constexpr auto stop_hiding_label  = " Stop Forcing Cursor Hidden ";
          constexpr auto stop_showing_label = " Stop Forcing Cursor Visible ";

          if ( ImGui::Button ( SK_ImGui_Cursor.force ==
                                     sk_cursor_state::Hidden ?
                                           stop_hiding_label :
                                           stop_showing_label ) )
          {
            SK_ImGui_Cursor.force = sk_cursor_state::None;
          }
        }
      }
#endif
      ImGui::EndGroup ();
      ImGui::EndGroup ();

      if (! config.input.cursor.manage)
      {
        ImGui::SameLine       ();
        _CursorBoundaryWidget ();
      }

      ImGui::TreePop ();
    }

    if (bHasPlayStation)
      ImGui::SetNextItemOpen (true, ImGuiCond_Once);

    bool uncollapsed_gamepads =
      ImGui::CollapsingHeader ("Gamepad", ImGuiTreeNodeFlags_AllowOverlap);

  //SK_ImGui_ProcessGamepadStatusBar (true);

    if (uncollapsed_gamepads)
    {
      ImGui::TreePush      ("");

      ImGui::Columns        (2);
      if (config.input.gamepad.hook_xinput)
      {
        ImGui::Checkbox       ("Haptic UI Feedback", &config.input.gamepad.haptic_ui);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("Rumble when interacting with SK's control panel using a gamepad");
          ImGui::Separator       ();
          ImGui::BulletText      ("Quickly identifies when games are not receiving gamepad input because of the control panel.");
          ImGui::EndTooltip      ();
        }

        ImGui::SameLine       ();

        if (config.input.gamepad.hook_xinput && config.input.gamepad.xinput.hook_setstate)
          ImGui::Checkbox     ("Disable ALL Rumble", &config.input.gamepad.disable_rumble);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("Prevent the GAME from making use of controller vibration");
          ImGui::Separator       ();
          ImGui::BulletText      ("In some games, there is a performance penalty for rumble and it cannot be turned off in-game...");
          ImGui::EndTooltip      ();
        }
      }

      ImGui::NextColumn     ();

      if (config.input.gamepad.hook_xinput)
      {
        ImGui::Checkbox ("Rehook XInput", &config.input.gamepad.rehook_xinput); ImGui::SameLine ();

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f),
                              "Re-installs input hooks if third-party hooks are detected.");
          ImGui::Separator ();
          ImGui::BulletText ("This may improve compatibility with x360ce, but will require a game restart.");
          ImGui::EndTooltip ();
        }
      }

      ImGui::SameLine      ();
      ImGui::SeparatorEx   (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine      ();

      ImGui::TextColored   (ImVec4 (1.f, 0.f, 0.f, 1.f), ICON_FA_BAN);
      ImGui::SameLine      ();

      if (ImGui::BeginMenu ("Block Gamepad Input APIs###Input_API_Select"))
      {
        static bool _need_restart = false;

        ImGui::BeginGroup      ();
        if (ImGui::Checkbox (ICON_FA_XBOX " XInput", &config.input.gamepad.xinput.blackout_api))
        {
          SK_Win32_NotifyDeviceChange (!config.input.gamepad.xinput.blackout_api, !config.input.gamepad.disable_hid);

          _need_restart = true;
        }
        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("Consider blocking XInput in games that natively support non-Xbox controllers.");
          ImGui::Separator       ();
          ImGui::BulletText      ("Blocking XInput may prevent double-inputs if using something like DS4Windows.");
          ImGui::BulletText      ("Blocking XInput may cause the game to use native button prompts for your device.");
          ImGui::Separator       ();
          ImGui::TextUnformatted ("A game restart -may- be required after blocking XInput.");
          ImGui::EndTooltip      ();
        }

        if (ImGui::Checkbox (ICON_FA_USB " HID",    &config.input.gamepad.disable_hid))
        {
          SK_Win32_NotifyDeviceChange (!config.input.gamepad.xinput.blackout_api, !config.input.gamepad.disable_hid);

          _need_restart = true;
        }
        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("Sometimes a game supports both HID and XInput, but XInput has more features...");
          ImGui::Separator       ();
          ImGui::BulletText      ("Blocking HID may prevent double-inputs.");
          ImGui::BulletText      ("Blocking HID usually causes games to display Xbox buttons.");
          ImGui::Separator       ();
          ImGui::TextUnformatted ("A game restart -may- be required after blocking HID.");
          ImGui::EndTooltip      ();
        }

#if 0
        if (ImGui::Checkbox (ICON_FA_WINDOWS " WinMM", &config.input.gamepad.disable_winmm))
        {
          SK_Win32_NotifyDeviceChange (!config.input.gamepad.xinput.blackout_api, !config.input.gamepad.disable_hid);

          _need_restart = true;

          config.utility.save_async ();
        }

        bool disable_dinput = 
          config.input.gamepad.dinput.block_enum_devices || 
          config.input.gamepad.dinput.blackout_gamepads;

        if (ImGui::Checkbox (ICON_FA_WINDOWS " DirectInput", &disable_dinput))
        {
          config.input.gamepad.dinput.blackout_gamepads  = disable_dinput;
          config.input.gamepad.dinput.block_enum_devices = disable_dinput;

          SK_Win32_NotifyDeviceChange (!config.input.gamepad.xinput.blackout_api, !config.input.gamepad.disable_hid);

          _need_restart = true;

          config.utility.save_async ();
        }
#endif

        if (ImGui::Checkbox (ICON_FA_STEAM " Steam", &config.input.gamepad.steam.disabled_to_game))
        {
          _need_restart = true;
        }
        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip (
            "Work In Progress:   "
            "You probably will not get gamepad input at all if Steam Input is active and blocked."
          );
        }
        ImGui::EndGroup   ();

        if (_need_restart)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
          ImGui::BulletText     ("Game Restart May Be Required");
          ImGui::PopStyleColor  ();
        }
        ImGui::EndMenu    ();
      }

      ImGui::NextColumn ( );
      ImGui::Columns    (1);

      ImGui::Separator ();

      bool connected [XUSER_MAX_COUNT] = {
        SK_XInput_WasLastPollSuccessful (0), SK_XInput_WasLastPollSuccessful (1),
        SK_XInput_WasLastPollSuccessful (2), SK_XInput_WasLastPollSuccessful (3)
      };

      if (config.input.gamepad.hook_xinput)
      {
        ImGui::Columns (2);

        ImGui::Text ("UI Controller:\t"); ImGui::SameLine ();

        int *ui_slot =
          (int *)&config.input.gamepad.xinput.ui_slot;

        if (config.input.gamepad.xinput.ui_slot != 4)
          ImGui::RadioButton (
            SK_FormatString ("Auto (XInput " ICON_FA_GAMEPAD " %d)##XInputSlot",
                              config.input.gamepad.xinput.ui_slot).c_str (), ui_slot, *ui_slot);
        else
          ImGui::RadioButton (R"(Auto##XInputSlot)",                         ui_slot, 0);

        ImGui::SameLine    ();
        ImGui::RadioButton ("Nothing##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 4);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Config menu will only respond to keyboard/mouse input");

        ImGui::NextColumn ( );

        if (config.input.gamepad.xinput.ui_slot >= 0 && config.input.gamepad.xinput.ui_slot < 4)
        {
          ImGui::Checkbox ("Dynamic XInput " ICON_FA_GAMEPAD " 0", &config.input.gamepad.xinput.auto_slot_assign);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Automatically reassign slot 0 in response to gamepad input");
        }

        else
        {
          config.input.gamepad.xinput.auto_slot_assign = false;
        }

        ImGui::NextColumn ( );
        ImGui::Columns    (2);

        if (! config.input.gamepad.xinput.blackout_api)
        {
          ImGui::Text     ("XInput Slots:\t");
          ImGui::SameLine ();

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::TextColored  (ImVec4 (1.f, 1.f, 1.f, 1.f),
                                 "Substitute (or Disable) XInput Controllers With Virtual Ones Until A Real One Is Connected");
            ImGui::Separator    ();
            ImGui::BulletText   ("Placeholding (checked) a slot is useful for games that do not normally support hot-plugging");
            ImGui::BulletText   ("Disabling (red button) a slot is useful if Steam Input and DS4Windows are both emulating XInput");
            ImGui::Separator    ();
            ImGui::BulletText   ("Both may improve performance in games that poll disconnected controllers");
            ImGui::EndTooltip   ();
          }

          auto XInputPlaceholderCheckbox = [](const char* szName, DWORD dwIndex)
          {
            bool disable_slot =
              config.input.gamepad.xinput.disable [dwIndex];

            bool placehold_slot =
              config.input.gamepad.xinput.placehold [dwIndex];

            int state = 0;

            if      (  disable_slot) state = 2;
            else if (placehold_slot) state = 1;
            else                     state = 0;

            if (disable_slot)
            {
              ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImVec4 (1.0f, 0.1f, 0.1f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImVec4 (0.9f, 0.4f, 0.4f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImVec4 (1.0f, 0.1f, 0.1f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_CheckMark,      ImVec4 (0.0f, 0.0f, 0.0f, 1.0f));
            }

            else if (SK_ImGui_WantGamepadCapture ())
            {
              ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImVec4 (1.0f, 1.0f, 0.1f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImVec4 (0.9f, 0.9f, 0.4f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImVec4 (1.0f, 1.0f, 0.1f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_CheckMark,      ImVec4 (0.0f, 0.0f, 0.0f, 1.0f));
            }

            if (ImGui::Checkbox (szName, &placehold_slot))
            {
              state = (++state % 3);

              switch (state)
              {
                case 0:
                  config.input.gamepad.xinput.placehold [dwIndex] = false;
                  config.input.gamepad.xinput.disable   [dwIndex] = false;
                  break;
                case 1:
                  config.input.gamepad.xinput.placehold [dwIndex] =  true;
                  config.input.gamepad.xinput.disable   [dwIndex] = false;
                  break;
                case 2:
                  config.input.gamepad.xinput.placehold [dwIndex] = false;
                  config.input.gamepad.xinput.disable   [dwIndex] =  true;
                  break;
              }

              if (state != 2)
              {
                SK_Win32_NotifyDeviceChange ();
              }
            }

            if (disable_slot || SK_ImGui_WantGamepadCapture ())
            {
              ImGui::PopStyleColor (4);
            }

            if (ImGui::IsItemHovered ())
            {
              const SK_XInput_PacketJournal journal =
                  SK_XInput_GetPacketJournal (dwIndex);

              ImGui::BeginTooltip  ( );

              ImGui::BeginGroup    ( );
              ImGui::TextUnformatted ("Device State: ");
              ImGui::TextUnformatted ("Placeholding: ");
              ImGui::EndGroup      ( );

              ImGui::SameLine      ( );

              ImGui::BeginGroup    ( );
              if (config.input.gamepad.xinput.disable [dwIndex] || config.input.gamepad.xinput.blackout_api)
                ImGui::TextColored (ImVec4 (1.0f, 0.1f, 0.1f, 1.0f), "Disabled");
              else if (SK_ImGui_WantGamepadCapture ())
                ImGui::TextColored (ImVec4 (1.0f, 1.0f, 0.1f, 1.0f), "Blocked");
              else
                ImGui::TextColored (ImVec4 (0.1f, 1.0f, 0.1f, 1.0f), "Enabled");

              if (config.input.gamepad.xinput.placehold [dwIndex])
              {
                if (SK_XInput_Holding (dwIndex))
                  ImGui::TextColored (ImVec4 ( 0.1f,  1.0f,  0.1f, 1.0f), "Enabled and Active");
                else
                  ImGui::TextColored (ImVec4 (0.75f, 0.75f, 0.75f, 1.0f), "Enabled");
              }

              else
                  ImGui::TextColored (ImVec4 ( 0.5f,  0.5f,  0.5f, 1.0f), "N/A");
              ImGui::EndGroup      ( );
              
              ImGui::Separator     ( );

              if (config.input.gamepad.xinput.placehold [dwIndex] && journal.packet_count.virt > 0)
              {
                ImGui::TextColored (ImColor (255, 255, 255), "Hardware Packet Sequencing");
                ImGui::TextColored (ImColor (160, 160, 160), "(Last: %lu | Now: %lu)",
                                    journal.sequence.last, journal.sequence.current);
                ImGui::Separator   ( );
                ImGui::Columns     (2, nullptr, false);
                ImGui::TextColored (ImColor (255, 165, 0), "Virtual Packets..."); ImGui::NextColumn ();
                ImGui::Text        ("%+07li", journal.packet_count.virt);         ImGui::NextColumn ();
                ImGui::TextColored (ImColor (127, 255, 0), "Real Packets...");    ImGui::NextColumn ();
                ImGui::Text        ("%+07li", journal.packet_count.real);
                ImGui::Columns     (1);
              }

              else
              {
                ImGui::BulletText ("Inputs Processed:\t%d", journal.packet_count.real);
              }

              ImGui::EndTooltip  ( );
            }
          };

          XInputPlaceholderCheckbox (ICON_FA_GAMEPAD " 0", 0); ImGui::SameLine ();
          XInputPlaceholderCheckbox (ICON_FA_GAMEPAD " 1", 1); ImGui::SameLine ();
          XInputPlaceholderCheckbox (ICON_FA_GAMEPAD " 2", 2); ImGui::SameLine ();
          XInputPlaceholderCheckbox (ICON_FA_GAMEPAD " 3", 3);
        }

        ImGui::NextColumn ( );

        ImGui::ItemSize ( ImVec2 (0.0f, 0.0f),
          ImGui::GetStyle ( ).FramePadding.y );

        SK_ImGui_ProcessGamepadStatusBar (true);

        ImGui::NextColumn ( );
        ImGui::Columns    (1);

        ////ImGui::BeginGroup ();
        ////ImGui::Text       (" Slot Redistribution ");
        ////auto slots =
        ////  (int *)config.input.gamepad.xinput.assignment;
        ////ImGui::SameLine   ();
        ////ImGui::InputInt4  ("###Slot Remapping", slots);
        ////ImGui::EndGroup   ();

        ImGui::Separator ( );
      }

      if (bHasPlayStation)
      {
        ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
        ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
        ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

        if (ImGui::CollapsingHeader ("PlayStation  (DualShock 3/4 / DualSense)", ImGuiTreeNodeFlags_DefaultOpen))
        {
          ImGui::TreePush ("");

          static HMODULE hModScePad =
            SK_GetModuleHandle (L"libscepad.dll");

          if (hModScePad)
          {
            ImGui::Checkbox ("Hook libScePad", &config.input.gamepad.hook_scepad);

            if (ImGui::IsItemHovered ())
                ImGui::SetTooltip ("SONY's native input API; unlocks additional settings in games that use it");

            if (config.input.gamepad.hook_scepad && last_scepad != 0)
            {
              ImGui::SameLine   (0.0f, 30);

              ImGui::BeginGroup ();
              ImGui::Checkbox   ("Disable Touchpad",             &config.input.gamepad.scepad.disable_touch);
              ImGui::Checkbox   ("Use Share as Touchpad Click",  &config.input.gamepad.scepad.share_clicks_touch);
              ImGui::EndGroup   ();

              ImGui::SameLine   ();
            }

            else
              ImGui::SameLine   (0.0f, 30);
          }

          bool bBluetooth  = false;
          bool bDualSense  = false;
          bool bDualShock4 = false;

          for ( auto& ps_controller : SK_HID_PlayStationControllers )
          {
            if (! ps_controller.bConnected)
              continue;

            if (ps_controller.bBluetooth)
            {
              bBluetooth = true;
            }

            if (ps_controller.bDualSense)
            {
              bDualSense = true;
            }

            if (ps_controller.bDualShock4)
            {
              bDualShock4 = true;
            }
          }

          ImGui::BeginGroup ();

          if (bDualSense)
            ImGui::Checkbox ("Apply Mute Button to -Game-",                           &config.input.gamepad.scepad.mute_applies_to_game);
          ImGui::Checkbox   ("Toggle Control Panel using  (" ICON_FA_PLAYSTATION ")", &config.input.gamepad.scepad.enhanced_ps_button);

          if (ImGui::IsItemHovered ())
          {
            if (config.input.gamepad.xinput.ui_slot > 3)
              ImGui::SetTooltip ("Will not work while \"UI Controller\" is set to 'Nothing'");
            else
              ImGui::SetTooltip ("Exit \"Exclusive Input Mode\" by Holding Share / Select or Pressing Caps Lock");
          }

          ImGui::EndGroup   ();

          if (config.input.gamepad.hook_xinput)
          {
            ImGui::SameLine    ();
            ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine    ();

            ImGui::BeginGroup  ();
            if (ImGui::Checkbox("XInput Mode", &config.input.gamepad.xinput.emulate))
            {
              if (config.input.gamepad.xinput.emulate)
              {
                SK_Win32_NotifyDeviceChange (true, false);

                if (config.input.gamepad.xinput.blackout_api)
                {
                  SK_ImGui_WarningWithTitle (
                    L"XInput was being blocked to the game; it must be unblocked"
                    L" for XInput mode to work.\r\n\r\n\t"
                    L"* A game restart may be required",
                      L"XInput Has Been Unblocked"
                  );

                  config.input.gamepad.xinput.blackout_api = false;
                }

                config.input.gamepad.xinput.placehold [0] = true;
              }

              else
              {
                SK_Win32_NotifyDeviceChange (false, true);
              }

              config.utility.save_async ();
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip    ();
              ImGui::TextUnformatted ("Translates HID to XInput for PlayStation controllers");
              ImGui::Separator       ();
              ImGui::BulletText      ("Fully supports DualSense and DualShock 4 (USB and Bluetooth).");
              ImGui::BulletText      ("Limited support for DualShock 3.");
              ImGui::Separator       ();
              ImGui::BulletText      ("Does not support more than one controller.");
              ImGui::BulletText      ("May require a game restart.");
              ImGui::EndTooltip      ();
            }

            if (config.input.gamepad.xinput.emulate)
            {
              ImGui::TreePush ("");
              ImGui::Checkbox ("Debug Mode", &config.input.gamepad.xinput.debug);
              ImGui::TreePop  (  );
            }

            ImGui::EndGroup ();
          }

          SK_HID_PlayStationDevice* pBatteryDevice   = nullptr;
          SK_HID_PlayStationDevice* pBluetoothDevice = nullptr;

          for ( auto& ps_controller : SK_HID_PlayStationControllers )
          {
            if (ps_controller.bConnected)
            {
              if (ps_controller.bBluetooth)
              {
                pBluetoothDevice = &ps_controller;
              }

              if (ps_controller.battery.state != SK_HID_PlayStationDevice::PowerState::ChargingError)
              {
                pBatteryDevice = &ps_controller;
              }
            }
          }

          if (pBatteryDevice != nullptr)
          {
            ImGui::SameLine    ();
            ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine    ();

            ImGui::BeginGroup  ();

            switch (pBatteryDevice->battery.state)
            {
              default:
              case SK_HID_PlayStationDevice::Charging:
              case SK_HID_PlayStationDevice::Discharging:
              {
                SK_HID_PlayStationDevice::battery_s *pBatteryState =
                  (SK_HID_PlayStationDevice::battery_s *)&pBatteryDevice->battery;

                static const char* szBatteryLevels [] = {
                  ICON_FA_BATTERY_EMPTY,
                  ICON_FA_BATTERY_QUARTER,
                  ICON_FA_BATTERY_HALF,
                  ICON_FA_BATTERY_THREE_QUARTERS,
                  ICON_FA_BATTERY_FULL
                };

                static ImColor battery_colors [] = {
                  ImColor::HSV (0.0f, 1.0f, 1.0f), ImColor::HSV (0.1f, 1.0f, 1.0f),
                  ImColor::HSV (0.2f, 1.0f, 1.0f), ImColor::HSV (0.3f, 1.0f, 1.0f),
                  ImColor::HSV (0.4f, 1.0f, 1.0f)
                };

                const int batteryLevel =
                  (pBatteryState->percentage > 70.0f) ? 4 :
                  (pBatteryState->percentage > 50.0f) ? 3 :
                  (pBatteryState->percentage > 30.0f) ? 2 :
                  (pBatteryState->percentage > 10.0f) ? 1 : 0;

                auto batteryColor =
                  battery_colors [batteryLevel];

                if (batteryLevel <= 1)
                {
                  batteryColor.Value.w =
                    static_cast <float> (
                      0.5 + 0.4 * std::cos (3.14159265359 *
                        (static_cast <double> (SK::ControlPanel::current_time % 2250) / 1125.0))
                    );
                }

                ImVec2 vButtonOrigin =
                  ImGui::GetCursorPos ();

                ImGui::BeginGroup ();
                ImGui::TextColored ( batteryColor, "%hs",
                    szBatteryLevels [batteryLevel]
                );

                ImGui::SameLine ();

                     if (pBatteryState->state == SK_HID_PlayStationDevice::Discharging)
                  ImGui::Text ("%3.0f%% " ICON_FA_ARROW_DOWN, pBatteryState->percentage);
                else if (pBatteryState->state == SK_HID_PlayStationDevice::Charging)
                  ImGui::Text ("%3.0f%% " ICON_FA_ARROW_UP,   pBatteryState->percentage);
                else
                  ImGui::Text ("%3.0f%% ",                    pBatteryState->percentage);

                ImGui::EndGroup ();

                ImVec2 vButtonEnd =
                  vButtonOrigin + ImGui::GetItemRectSize ();

                ImGui::SetCursorPos                                       (vButtonOrigin);
                ImGui::InvisibleButton ("###GamepadPowerOff", vButtonEnd - vButtonOrigin);

                if (pBluetoothDevice != nullptr)
                {
                  if (SK_ImGui_IsItemClicked ())
                  {
                    ImGui::ClearActiveID   ( );
                    SK_DeferCommand ("Input.Gamepad.PowerOff 1");
                  }

                  else if (SK_ImGui_IsItemRightClicked ())
                  {
                    ImGui::ClearActiveID ( );
                    ImGui::OpenPopup     ("BluetoothCompatMenu");
                  }

                  else if (ImGui::IsItemHovered ())
                  {
                    ImGui::BeginTooltip    ( );
                    ImGui::BeginGroup      ( );
                    ImGui::TextColored     ( ImColor::HSV (0.18f, 0.88f, 0.94f),
                                               " Left-Click" );
                    ImGui::TextColored     ( ImColor::HSV (0.18f, 0.88f, 0.94f),
                                               "Right-Click" );
                    ImGui::EndGroup        ( );
                    ImGui::SameLine        ( );
                    ImGui::BeginGroup      ( );
                    ImGui::TextUnformatted ( " Power-off Gamepad" );
                    ImGui::TextUnformatted ( " Bluetooth Config" );
                    ImGui::EndGroup        ( );
                    ImGui::EndTooltip      ( );
                  }

                  if (ImGui::BeginPopup ("BluetoothCompatMenu"))
                  {
                    if (ImGui::Checkbox ("Bluetooth Compatibility Mode",
                        &config.input.gamepad.bt_input_only))
                    {
                      if (config.input.gamepad.bt_input_only)
                      {
                        SK_DeferCommand ("Input.Gamepad.PowerOff 1");
                      }

                      config.utility.save_async ();

                      ImGui::ClearActiveID     ( );
                      ImGui::CloseCurrentPopup ( );
                    }

                    if (ImGui::IsItemHovered ())
                    {
                      if (! config.input.gamepad.bt_input_only)
                        ImGui::SetTooltip (
                          "Power-off Bluetooth Controllers and use DualShock 3 Compatibility Mode when "
                          "Powered-on"
                        );
                      else
                        ImGui::SetTooltip (
                          "Enable Enhanced DualShock 4 / DualSense Features in Bluetooth Mode "
                          "(may break input in native DirectInput/HID games)"
                        );
                    }
                    ImGui::EndPopup     ();
                  }
                }
              } break;
            }

            if (bDualSense)
            {
              if (ImGui::Checkbox ("Power Saving Mode", &config.input.gamepad.scepad.power_save_mode))
              {
                config.utility.save_async ();
              }
            }

            ImGui::EndGroup   ();
          }

          if (bDualSense || bDualShock4)
          {
            bool bOverrideRGB =
              config.input.gamepad.scepad.led_color_r    != -1 ||
              config.input.gamepad.scepad.led_color_g    != -1 ||
              config.input.gamepad.scepad.led_color_b    != -1 ||
              config.input.gamepad.scepad.led_brightness != -1;

            ImGui::BeginGroup ();
            if (ImGui::Checkbox ("Override RGB", &bOverrideRGB))
            {
              if (! bOverrideRGB)
              {
                config.input.gamepad.scepad.led_color_r    = -1;
                config.input.gamepad.scepad.led_color_g    = -1;
                config.input.gamepad.scepad.led_color_b    = -1;
                config.input.gamepad.scepad.led_brightness = -1;
              }

              else
              {
                config.input.gamepad.scepad.led_color_r    = 255;
                config.input.gamepad.scepad.led_color_g    = 255;
                config.input.gamepad.scepad.led_color_b    = 255;
                config.input.gamepad.scepad.led_brightness =   0;
              }
              config.utility.save_async ();
            }

            if (bOverrideRGB)
            {
              ImGui::SameLine ();

              float color [3] = { (float)config.input.gamepad.scepad.led_color_r / 255.0f,
                                  (float)config.input.gamepad.scepad.led_color_g / 255.0f,
                                  (float)config.input.gamepad.scepad.led_color_b / 255.0f };
              if (ImGui::ColorEdit3 ("###PlayStation_RGB", color))
              {
                config.input.gamepad.scepad.led_color_r = (int)(color [0] * 255.0f);
                config.input.gamepad.scepad.led_color_g = (int)(color [1] * 255.0f);
                config.input.gamepad.scepad.led_color_b = (int)(color [2] * 255.0f);
                config.utility.save_async ();
              }

              ImGui::SameLine ();

              int brightness = 3 - config.input.gamepad.scepad.led_brightness;

              const char* szLabel = brightness == 0 ? "Very Dim" :
                                    brightness == 1 ? "Dim"      :
                                    brightness == 2 ? "Mid"      :
                                                      "Bright";

              if (ImGui::SliderInt ("Brightness", &brightness, 0, 3, szLabel))
              {
                config.input.gamepad.scepad.led_brightness = 3 - brightness;
                config.utility.save_async ();
              }
            }
            ImGui::EndGroup   ();
          }

#if 0
          if ( bDualSense &&
               ImGui::SliderFloat ( "Rumble Motor Power Level",
                                      &config.input.gamepad.scepad.rumble_power_level,
                                        12.5f, 100.0f ) )
          {
            if ( config.input.gamepad.scepad.rumble_power_level > 93.75f)
                 config.input.gamepad.scepad.rumble_power_level = 100.0f;
            else config.input.gamepad.scepad.rumble_power_level -=
          fmodf (config.input.gamepad.scepad.rumble_power_level, 12.5f);

            for ( auto& ps_controller : SK_HID_PlayStationControllers )
            {
              ps_controller.write_output_report ();
            }
          }
#endif
          ImGui::TreePop  (  );
        }

        ImGui::PopStyleColor (3);
      }
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

      static LARGE_INTEGER
        liLastPoll [16] = { { }, { }, { }, { }, { }, { }, { }, { },
                            { }, { }, { }, { }, { }, { }, { }, { } };
      static UINT
        uiLastErr  [16] = { JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR,
                            JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR,
                            JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR,
                            JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR };

      auto GamepadDebug = [&](UINT idx) ->
      void
      {
        //// Only 2 joysticks (possibly fewer if the driver's b0rked)
        //if ( idx >= SK_joyGetNumDevs () )
        //{
        //  return;
        //}

        // Throttle polling on errors => once every 750 ms
        //
        //   Proper solution involves watching for a device notification,
        //     but I don't want to bother with that right now :)
        if (                  uiLastErr [idx] != JOYERR_NOERROR &&
             SK_DeltaPerfMS (liLastPoll [idx].QuadPart, 1) < ( idx == 0 ?  6666.6 :
                                                                          12121.2 ) )
        {
          return;
        }

        JOYINFOEX joy_ex   { };
        JOYCAPSW  joy_caps { };

        joy_ex.dwSize  = sizeof (JOYINFOEX);
        joy_ex.dwFlags = JOY_RETURNALL      | JOY_RETURNPOVCTS |
                         JOY_RETURNCENTERED | JOY_USEDEADZONE;

        uiLastErr        [idx] =
       SK_joyGetDevCapsW (idx, &joy_caps, sizeof (JOYCAPSW));
              liLastPoll [idx] = SK_QueryPerf ();
        if (   uiLastErr [idx] != JOYERR_NOERROR || joy_caps.wCaps == 0)
        {
          if (joy_caps.wCaps == 0)
            uiLastErr [idx] = JOYERR_NOCANDO;

          return;
        }

        uiLastErr     [idx] =
       SK_joyGetPosEx (idx, &joy_ex);
           liLastPoll [idx] = SK_QueryPerf ();
        if (uiLastErr [idx] != JOYERR_NOERROR)
          return;

        ImGui::PushID (idx);

        std::stringstream buttons;

        const unsigned int max_buttons =
          std::min (16U, joy_caps.wMaxButtons);

        for ( unsigned int i = 0,
                           j = 0                                    ;
                           i < max_buttons ;
                         ++i )
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

        bool expanded = ImGui::CollapsingHeader (SK_FormatString ("%ws##JOYSTICK_DEBUG", joy_caps.szPname).c_str ());

        ImGui::Combo    ("Gamepad Type", &config.input.gamepad.predefined_layout, "PlayStation 4\0Steam\0\0", 2);

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ("This setting is only used if XInput or DirectInput are not working.");
        }

        ImGui::SameLine ();

        ImGui::Checkbox     ("Use DirectInput instead of XInput", &config.input.gamepad.native_ps4);

        if (expanded)
        {
          ImGui::TreePush        (       ""       );

          ImGui::TextUnformatted (buttons.str ().c_str ());

          const float angle =
            static_cast <float> (joy_ex.dwPOV) / 100.0f;

          if (joy_ex.dwPOV != JOY_POVCENTERED)
            ImGui::Text ((const char *)u8" D-Pad:  %4.1f°", angle);
          else
            ImGui::Text (                " D-Pad:  Centered");

          struct axis_s {
            const char* label;
            float       min, max;
            float       now;
          }
            const axes [6] = { { "X-Axis", static_cast <float> (joy_caps.wXmin),
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

          const UINT max_axes =
            std::min (6U, joy_caps.wMaxAxes);

          for ( UINT axis = 0         ;
                     axis <  max_axes ;
                   ++axis )
          {
            auto  const range  = static_cast <float>  (axes [axis].max - axes [axis].min);
            float const center = static_cast <float> ((axes [axis].max + axes [axis].min)) / 2.0f;
            float       rpos   = 0.5f;

            if (static_cast <float> (axes [axis].now) < center)
              rpos = center - (center - axes [axis].now);
            else
              rpos = static_cast <float> (axes [axis].now - axes [axis].min);

            ImGui::ProgressBar ( rpos / range,
                                   ImVec2 (-1.0f, 0.0f),
                                     SK_FormatString ( "%s [ %.0f, { %.0f, %.0f } ]",
                                                         axes [axis].label, axes [axis].now,
                                                         axes [axis].min,   axes [axis].max ).c_str () );
          }

          ImGui::TreePop     ( );
        }
        ImGui::PopStyleColor (3);
        ImGui::PopID         ( );
      };

#if 0
      static DWORD dwLastCheck = current_time;
      static UINT  dwLastCount = SK_joyGetNumDevs ();

      const DWORD _CHECK_INTERVAL = 1500UL;

      UINT count =
        ( dwLastCheck < (current_tick - _CHECK_INTERVAL) ) ?
                  SK_joyGetNumDevs () : dwLastCount;

      if (dwLastCheck < (current_tick - _CHECK_INTERVAL))
          dwLastCount = count;

      if (  count > 0) { GamepadDebug (JOYSTICKID1);
        if (count > 1) {
          for ( UINT i = 0 ; i < count ; ++i )
            GamepadDebug (i);
        }
      }
#endif

      if (config.input.gamepad.hook_xinput)
      {
        static bool started = false;

        if (! config.input.gamepad.xinput.emulate)
        {
          static bool   init       = false;
          static HANDLE hStartStop =
            SK_CreateEvent (nullptr, TRUE, FALSE, nullptr);

          if (ImGui::Button (started ? "Stop XInput Latency Test" :
                                       "Start XInput Latency Test"))
          {
            if (! started) { started = true;  SetEvent   (hStartStop); }
            else           { started = false; ResetEvent (hStartStop); }

            if (! init)
            {     init = true;
              SK_Thread_CreateEx ([](LPVOID) -> DWORD
              {
                XINPUT_STATE states [2] = { };
                ULONGLONG    times  [2] = { };
                ULONGLONG    times_ [2] = { };
                int                  i  =  0;

                do
                {
                  SK_WaitForSingleObject (hStartStop, INFINITE);

                  if (SK_XInput_PollController (static_cast <INT> (config.input.gamepad.xinput.ui_slot), &states [i % 2]))
                  {
                    XINPUT_STATE& old = states [(i + 1) % 2];
                    XINPUT_STATE& now = states [ i++    % 2];

                    if (old.dwPacketNumber != now.dwPacketNumber)
                    {
                      LARGE_INTEGER nowTime = SK_QueryPerf ();

                      if (memcmp (&old.Gamepad, &now.Gamepad, sizeof (XINPUT_GAMEPAD)))
                      {
                        ULONGLONG oldTime = times_ [0];
                                            times_ [0] = times_ [1];
                                            times_ [1] = nowTime.QuadPart;

                        gamepad_stats_filtered->addSample ( 1000.0 *
                          static_cast <double> (times_ [0] - oldTime) /
                          static_cast <double> (SK_PerfFreq),
                            nowTime
                        );
                      }

                      ULONGLONG oldTime = times [0];
                                          times [0] = times [1];
                                          times [1] = nowTime.QuadPart;

                      gamepad_stats->addSample ( 1000.0 *
                        static_cast <double> (times [0] - oldTime) /
                        static_cast <double> (SK_PerfFreq),
                          nowTime
                      );
                    }
                  }
                } while (0 == ReadAcquire (&__SK_DLL_Ending));

                SK_Thread_CloseSelf ();

                return 0;
              }, L"[SK] XInput Latency Tester", (LPVOID)hStartStop);
            }
          }

          static double high_min = std::numeric_limits <double>::max (),
                        high_max,
                        avg;

          static double high_min_f = std::numeric_limits <double>::max (),
                        high_max_f,
                        avg_f;

          ImGui::SameLine    ( );
          ImGui::BeginGroup  ( );

          if (started)
          {
            ImGui::BeginGroup( );
            ImGui::Text      ( "%lu Raw Samples - (Min | Max | Mean) - %4.2f ms | %4.2f ms | %4.2f ms",
                                 gamepad_stats->calcNumSamples (),
                                 gamepad_stats->calcMin        (),
                                 gamepad_stats->calcMax        (),
                                 gamepad_stats->calcMean       () );

            ImGui::Text      ( "%lu Validated Samples - (Min | Max | Mean) - %4.2f ms | %4.2f ms | %4.2f ms",
                                 gamepad_stats_filtered->calcNumSamples (),
                                 gamepad_stats_filtered->calcMin        (),
                                 gamepad_stats_filtered->calcMax        (),
                                 gamepad_stats_filtered->calcMean       () );
            ImGui::EndGroup  ( );

            high_min_f = std::min (gamepad_stats_filtered->calcMin (), high_min_f);
            high_min   = std::min (gamepad_stats->calcMin          (), high_min  );
          }

          ImGui::BeginGroup  ( );
          if (high_min   < 250.0)
            ImGui::Text      ( "Minimum Latency: %4.2f ms", high_min );
          if (high_min_f < 250.0)
            ImGui::Text      ( "Minimum Latency: %4.2f ms (Validation Applied)", high_min_f );
          ImGui::EndGroup    ( );
          ImGui::EndGroup    ( );
        }
      }
      ImGui::TreePop       ( );
    }

    if (ImGui::CollapsingHeader ("Low-Level Mouse Settings"))//, ImGuiTreeNodeFlags_DefaultOpen))
    {
      static bool  deadzone_hovered = false;
             float float_thresh     = std::max (1.0f, std::min (100.0f, config.input.mouse.antiwarp_deadzone));

      ImVec2 deadzone_pos    = io.DisplaySize;
             deadzone_pos.x /= 2.0f;
             deadzone_pos.y /= 2.0f;
      const ImVec2 deadzone_size ( io.DisplaySize.x * float_thresh / 200.0f,
                                   io.DisplaySize.y * float_thresh / 200.0f );

      const ImVec2 xy0 ( deadzone_pos.x - deadzone_size.x,
                         deadzone_pos.y - deadzone_size.y );
      const ImVec2 xy1 ( deadzone_pos.x + deadzone_size.x,
                         deadzone_pos.y + deadzone_size.y );

      if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open ||
             SK_ImGui_Cursor.prefs.no_warp.visible )  &&
           ( deadzone_hovered || ImGui::IsMouseHoveringRect ( xy0, xy1, false ) ) )
      {
        const ImVec4 col = ImColor::HSV ( 0.18f,
                                std::min (1.0f, 0.85f + (sin ((float)(current_time % 400) / 400.0f))),
                                                     (float)(0.66f + (current_time % 830) / 830.0f ) );
        const ImU32 col32 =
          ImColor (col);

        ImDrawList* draw_list =
          ImGui::GetWindowDrawList ();

        draw_list->PushClipRectFullScreen (                                     );
        draw_list->AddRect                ( xy0, xy1, col32, 32.0f, 0xF, 3.333f );
        draw_list->PopClipRect            (                                     );
      }

      ImGui::TreePush      ("");

      ImGui::BeginGroup    (  );
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
#if 0
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
#endif

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

      ImGui::SameLine      ();
      ImGui::SeparatorEx   (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine      ();

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

    const bool devices =
      ImGui::CollapsingHeader ("Enable / Disable Devices", ImGuiTreeNodeFlags_DefaultOpen);

    if (devices)
    {
      ImGui::TreePush     ("");
      ImGui::BeginGroup   (  );
      ImGui::BeginGroup   (  );
      ImGui::Combo        ("Mouse Input", &config.input.mouse.disabled_to_game,
                           "Enabled\0Disabled (Always)\0Disabled (in Background)\0\0");
      ImGui::Combo        ("Keyboard Input", &config.input.keyboard.disabled_to_game,
                           "Enabled\0Disabled (Always)\0Disabled (in Background)\0\0");

      if (ImGui::IsItemHovered () && config.input.keyboard.disabled_to_game == SK_InputEnablement::DisabledInBackground)
        ImGui::SetTooltip ("Most games block keyboard input in the background to begin with...");

      if (/*config.input.gamepad.hook_dinput7        ||*/ config.input.gamepad.hook_dinput8   ||
            config.input.gamepad.hook_hid            ||   config.input.gamepad.hook_xinput    ||
            config.steam.appid != 0                  ||   config.input.gamepad.hook_raw_input ||
            config.input.gamepad.hook_windows_gaming ||   config.input.gamepad.hook_winmm )
      {
        bool changed =
          ImGui::Combo ("Gamepad Input", &config.input.gamepad.disabled_to_game,
                        "Enabled\0Disabled (Always)\0Disabled (in Background)\0\0");

        if (changed)
        {
          SK_Steam_ProcessWindowActivation (game_window.active);
        }
      }
      ImGui::EndGroup     (  );
      ImGui::SameLine     (  );
      ImGui::SeparatorEx  (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine     (  );
      ImGui::BeginGroup   (  );
      ImGui::PushStyleColor
                          (ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.f));
      ImGui::PushItemWidth(
        ImGui::GetFont ()->CalcTextSizeA (
          1.0f, FLT_MAX, 0.0f, "If \"Continue Rendering\" and Gamepad Input are Enabled, TTT"
        ).x
      );
      ImGui::TextWrapped  ("If \"Continue Rendering\" and Gamepad Input are Enabled, "
                           "you can continue playing this game using your Gamepad(s) "
                           "while other applications use your Keyboard & Mouse.");
      ImGui::PopItemWidth (  );
      ImGui::PopStyleColor(  );
      ImGui::EndGroup     (  );
      ImGui::Separator    (  );
      ImGui::EndGroup     (  );
      ImGui::BeginGroup   (  );
      ImGui::TextColored  (ImVec4 (.666f, 1.f, 1.f, 1.f), ICON_FA_INFO_CIRCLE);
      ImGui::SameLine     (  );
      ImGui::PushStyleColor
                          (ImGuiCol_Text, ImVec4 (0.825f, 0.825f, 0.825f, 1.f));
      ImGui::TextUnformatted
                          ("These settings work best in conjunction with "
                           "\"Continue Rendering\" (refer to Window Management)");
      ImGui::PopStyleColor(  );
      ImGui::EndGroup     (  );
#if 0
      ImGui::Separator ();

      static std::vector <RAWINPUTDEVICE> keyboards_ = SK_RawInput_GetKeyboards ();

      if (! keyboards_.empty ())
      {
        static bool toggle =
          config.input.keyboard.disabled_to_game;

        if (toggle != config.input.keyboard.disabled_to_game)
        {
          toggle =
            config.input.keyboard.disabled_to_game;

          if (! toggle)
          {
            SK_RegisterRawInputDevices (&keyboards_ [0], 1, sizeof RAWINPUTDEVICE);
          }

          else
          {
            RAWINPUTDEVICE alt_x =
              keyboards_ [0];

            alt_x.usUsage     = HID_USAGE_GENERIC_KEYBOARD;
            alt_x.usUsagePage = HID_USAGE_PAGE_GENERIC;
            alt_x.dwFlags     = RIDEV_NOLEGACY | /*RIDEV_INPUTSINK |*/ RIDEV_NOHOTKEYS;

            SK_RegisterRawInputDevices (&alt_x, 1, sizeof RAWINPUTDEVICE);
          }
        }
      }

      auto _EnumDeviceFlags =
        [](            RAWINPUTDEVICE& device,
            std::vector <std::string>& flags,
            std::vector <std::string>& descs  ) ->
        void
        {
          const DWORD& dwFlags =
                device.dwFlags;

          if (dwFlags & RIDEV_REMOVE)
          {
            flags.emplace_back ("RIDEV_REMOVE");
            descs.emplace_back ("Ignoring Device Class");
          }
          if (dwFlags & RIDEV_EXCLUDE)
          {
            flags.emplace_back ("RIDEV_EXCLUDE");
            descs.emplace_back ("");
          }
          if (dwFlags & RIDEV_PAGEONLY)
          {
            flags.emplace_back ("RIDEV_PAGEONLY");
            descs.emplace_back ("");
          }
          if (dwFlags & RIDEV_NOLEGACY)
          {
            flags.emplace_back ("RIDEV_NOLEGACY");
            descs.emplace_back ("Disabling Win32 Device Messages");
          }
          if (dwFlags & RIDEV_INPUTSINK)
          {
            flags.emplace_back ("RIDEV_INPUTSINK");
            descs.emplace_back ("RawInput Delivers Data to non-Foreground Windows");
          }
          if (dwFlags & RIDEV_CAPTUREMOUSE)
          {
            flags.emplace_back ("RIDEV_CAPTUREMOUSE");
            descs.emplace_back ("Click-to-Activate Disabled");
          }
          if (dwFlags & RIDEV_NOHOTKEYS)
          {
            flags.emplace_back ("RIDEV_NOHOTKEYS");
            descs.emplace_back ("Ignoring Most System-defined Alt+... Hotkeys");
          }
          if (dwFlags & RIDEV_APPKEYS)
          {
            flags.emplace_back ("RIDEV_APPKEYS");
            descs.emplace_back ("Application-key Shortcuts are Enabled");
          }
          if (dwFlags & RIDEV_EXINPUTSINK)
          {
            flags.emplace_back ("RIDEV_EXINPUTSINK");
            descs.emplace_back ("Receive RawInput Data if Foreground App is Not using RawInput");
          }
          if (dwFlags & RIDEV_DEVNOTIFY)
          {
            flags.emplace_back ("RIDEV_DEVNOTIFY");
            descs.emplace_back ("Receive RawInput Data if Foreground App is Not using RawInput");
          }
        };

      std::vector <RAWINPUTDEVICE> mice      = SK_RawInput_GetMice      ();
      std::vector <RAWINPUTDEVICE> keyboards = SK_RawInput_GetKeyboards ();

      std::vector <std::vector <std::string>> mouse_flags;
                                              mouse_flags.reserve (mice.size ());
      std::vector <std::vector <std::string>> mouse_descs;
                                              mouse_descs.reserve (mice.size ());
      std::vector <std::vector <std::string>> keyboard_flags;
                                              keyboard_flags.reserve (keyboards.size ());
      std::vector <std::vector <std::string>> keyboard_descs;
                                              keyboard_descs.reserve (keyboards.size ());

      ImGui::Text     ("Raw Input Setup");
      ImGui::TreePush ("");

      ImGui::BeginGroup (           );
      if (mice.size ())
      {
        ImGui::Text       ("Mice"     );
        ImGui::Separator  (           );

        int idx = 0;
        for (          auto mouse  : mice )                {
          _EnumDeviceFlags ( mouse, mouse_flags [  idx],
                                    mouse_descs [idx++] ); }

        ImGui::BeginGroup   ();
        for (  auto flag_array    : mouse_flags )
        { for (auto flag          : flag_array  )
          { ImGui::TextUnformatted (flag.c_str ());
        } } ImGui::EndGroup ();
        ImGui::SameLine     ();
        ImGui::BeginGroup   ();
        for (  auto desc_array    : mouse_descs )
        { for (auto desc          : desc_array  )
          { ImGui::TextUnformatted (desc.c_str ());
        } } ImGui::EndGroup ();
        ImGui::EndGroup     (                    );
      }

      if (keyboards.size ())
      {
        ImGui::SameLine     (                    );
        ImGui::BeginGroup   (                    );
        ImGui::Text         ("Keyboards"         );
        ImGui::Separator    (                    );

        int idx = 0;
        for (          auto  keyboard : keyboards )               {
          _EnumDeviceFlags ( keyboard,  keyboard_flags [  idx],
                                        keyboard_descs [idx++] ); }

        ImGui::BeginGroup     ();
        for (  auto flag_array  :   keyboard_flags )
        { for (auto flag        :   flag_array     )
          { ImGui::TextUnformatted (flag.c_str ());
        } } ImGui::EndGroup   ();
        ImGui::SameLine       ();
        ImGui::BeginGroup     ();
        for (  auto desc_array  : keyboard_descs )
        { for (auto desc        : desc_array     )
          { ImGui::TextUnformatted (desc.c_str ());
        } } ImGui::EndGroup   ();
      }
      ImGui::EndGroup ();
#endif
      ImGui::TreePop  ();
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);

    return true;
  }

  return false;
}


bool
SK_ImGui_KeybindSelect (SK_Keybind* keybind, const char* szLabel)
{
  (void)keybind;

  bool ret = false;

  ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.667f, 0.667f, 0.667f, 1.0f));
  ImGui::PushItemWidth  (ImGui::GetContentRegionAvail ().x);

  ret =
    ImGui::Selectable (szLabel, false);

  ImGui::PopItemWidth  ();
  ImGui::PopStyleColor ();

  return ret;
}

SK_API
void
__stdcall
SK_ImGui_KeybindDialog (SK_Keybind* keybind)
{
  if (! keybind)
    return;

  auto& io =
    ImGui::GetIO ();

  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  ImGui::SetNextWindowSizeConstraints ( ImVec2   (font_size *  9.0f, font_size * 3.0f),
                                          ImVec2 (font_size * 30.0f, font_size * 6.0f) );

  if (ImGui::BeginPopupModal (keybind->bind_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize |
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

      if (ImGui::IsKeyPressed (ImGui_ImplWin32_VirtualKeyToImGuiKey (i), false))
        break;
    }

    bool bEscape =
      ImGui::IsKeyPressed (ImGuiKey_Escape, false);

    if (i != 256)
    {
      ImGui::CloseCurrentPopup ();

      keybind->vKey = bEscape ? 0 :
           static_cast <SHORT> (i);
    }

    keybind->ctrl  = bEscape ? false : io.KeyCtrl;
    keybind->shift = bEscape ? false : io.KeyShift;
    keybind->alt   = bEscape ? false : io.KeyAlt;

    keybind->update ();

    ImGui::TextColored (ImVec4 (0.6f, 0.6f, 0.6f,1.f),
                        "Press ESC To Clear Keybind");
    ImGui::Separator   (                            );

    ImGui::Text        ("Binding:  %hs", SK_WideCharToUTF8 (keybind->human_readable).c_str ());

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

SK_API
INT
__stdcall
SK_ImGui_GamepadComboDialog0 (SK_GamepadCombo_V0* combo)
{
  if (! combo)
    return 0;

  auto& io =
    ImGui::GetIO ();

  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  ImGui::SetNextWindowSizeConstraints ( ImVec2 (font_size *  9, font_size * 3 ),
                                        ImVec2 (font_size * 30, font_size * 6));

  if ( ImGui::BeginPopupModal (
        combo->combo_name.c_str (), nullptr,
         ImGuiWindowFlags_AlwaysAutoResize |
         ImGuiWindowFlags_NoCollapse       | ImGuiWindowFlags_NoSavedSettings
       )
     )
  {
    SK_ImGui_GamepadComboDialogActive = true;
    nav_usable                        = false;
    io.NavVisible                     = false;
    io.NavActive                      = false;

    io.WantCaptureKeyboard = true;

    static WORD                last_buttons     = 0;
    static DWORD               last_change      = 0;
    static BYTE                last_trigger_l   = 0;
    static BYTE                last_trigger_r   = 0;
    static SK_GamepadCombo_V0* last_combo       = nullptr;
           XINPUT_STATE        state            = { };
    static std::wstring        unparsed;

    if (SK_XInput_PollController (0, &state))
    {
      if (last_combo != combo)
      {
        unparsed.clear ();
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

        unparsed.clear ();

        while (! buttons.empty ())
        {
          unparsed.append (buttons.front ());
                           buttons.pop   ();

          if (! buttons.empty ())
            unparsed.append (L"+");
        }

        last_buttons = state.Gamepad.wButtons;
        last_change  = SK::ControlPanel::current_time;
      }

      else if ( last_change >  0 &&
                last_change < (SK::ControlPanel::current_time - 1000UL) )
      {
        combo->unparsed = unparsed;

        for (int i = 0; i < 16; i++)
        {
          io.NavInputs [i] = 0.1f;
        }

        SK_ImGui_GamepadComboDialogActive = false;
        nav_usable                        = true;
        io.NavVisible                     = true;
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

    ImGui::Text ("Binding:  %hs", SK_WideCharToUTF8 (unparsed).c_str ());

    ImGui::EndPopup ();
  }

  return 0;
}

void
SK_ImGui_CursorBoundaryConfig (void)
{
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
}