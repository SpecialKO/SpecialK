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
#include <hidclass.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Input Mgr."

#include <imgui/backends/imgui_d3d11.h>
#include <SpecialK/injection/injection.h>

bool SK_WantBackgroundRender (void)
{
  return
    config.window.background_render;
}

extern "C" {
  extern LONG g_sHookedPIDs [MAX_INJECTED_PROCS];
}

DWORD SK_WGI_GamePollingThreadId = 0;

bool
SK_ImGui_WantGamepadCapture (void)
{
  auto _Return = [](BOOL bCapture) ->
  bool
  {
    // Disable Steam Input permanently here if the user wants
    if (config.input.gamepad.steam.disabled_to_game)
    {
      // Only needs to be done once, it will be restored at exit.
      SK_RunOnce (
        SK_Steam_ForceInputAppId (SPECIAL_KILLER_APPID)
      );
    }

    static BOOL        lastCapture = -1;
    if (std::exchange (lastCapture, bCapture) != bCapture)
    {
      // Suspend the Windows.Gaming.Input thread in case we did not
      //   manage to hook the necessary UWP interface APIs
      if (SK_WGI_GamePollingThreadId != 0)
      {
        SK_AutoHandle hThread__ (
          OpenThread ( THREAD_SUSPEND_RESUME,
                         FALSE,
                           SK_WGI_GamePollingThreadId )
        );

        if ((intptr_t)hThread__.m_h > 0)
        {
          static int suspensions = 0;

          if (bCapture)
          {
            if (suspensions == 0)
            { ++suspensions;
              SuspendThread (hThread__);
            }
          }

          else
          {
            if (suspensions > 0)
            { --suspensions;
              ResumeThread (hThread__);
            }
          }
        }
      }

      // Conditionally block Steam Input
      if (! config.input.gamepad.steam.disabled_to_game)
      {
        // Prefer to force an override in the Steam client itself,
        //   but fallback to forced input appid if necessary
        if (! SK::SteamAPI::SetWindowFocusState (! bCapture))
        {
          SK_Steam_ForceInputAppId ( bCapture ?
                         SPECIAL_KILLER_APPID : config.steam.appid );

          // Delayed command to set the Input AppId to 0
          if (! bCapture)
          {
            SK_SteamInput_Unfux0r ();
          }
        }
      }
    }

    if (! bCapture)
    {
      // Implicitly block input to this game if SK is currently injected
      //   into two games at once, and the other game is currently foreground.
      if (! game_window.active)
      {
        HWND hWndForeground =
          SK_GetForegroundWindow ();

        DWORD                                      dwForegroundPid = 0x0;
        GetWindowThreadProcessId (hWndForeground, &dwForegroundPid);

        for ( auto pid : g_sHookedPIDs )
        {
          if (pid == (LONG)dwForegroundPid)
          {
            bCapture = true;
          }
        }
      }
    }

    return bCapture;
  };

  bool imgui_capture =
    config.input.gamepad.disabled_to_game == SK_InputEnablement::Disabled;

  if (SK_GImDefaultContext ())
  {
    if (SK_ImGui_Active ())
    {
      if (nav_usable)
        imgui_capture = true;
    }

    if (SK_ImGui_GamepadComboDialogActive)
      imgui_capture = true;
  }

  if ((! SK_IsGameWindowActive ()) && config.input.gamepad.disabled_to_game != SK_InputEnablement::Enabled)
    imgui_capture = true;

  if ( config.input.gamepad.scepad.enhanced_ps_button &&
            (config.input.gamepad.xinput.ui_slot >= 0 && 
             config.input.gamepad.xinput.ui_slot <  4) )
  {
    extern XINPUT_STATE hid_to_xi;
    extern XINPUT_STATE
         SK_ImGui_XInputState;
    if ((SK_ImGui_XInputState.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) ||
                   (hid_to_xi.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE))
      imgui_capture = true;
  }

  return
    _Return (imgui_capture);
}

UINT
WINAPI
SendInput_Detour (
  _In_ UINT    nInputs,
  _In_ LPINPUT pInputs,
  _In_ int     cbSize
)
{
  SK_LOG_FIRST_CALL

  return
    SK_SendInput (nInputs, pInputs, cbSize);
}

VOID
WINAPI
mouse_event_Detour (
  _In_ DWORD     dwFlags,
  _In_ DWORD     dx,
  _In_ DWORD     dy,
  _In_ DWORD     dwData,
  _In_ ULONG_PTR dwExtraInfo
)
{
  SK_LOG_FIRST_CALL

  // TODO: Process this the right way...
  if (SK_ImGui_WantMouseCapture ())
  {
    return;
  }

  mouse_event_Original (
    dwFlags, dx, dy, dwData, dwExtraInfo
  );
}

VOID
WINAPI
SK_mouse_event (
  _In_ DWORD     dwFlags,
  _In_ DWORD     dx,
  _In_ DWORD     dy,
  _In_ DWORD     dwData,
  _In_ ULONG_PTR dwExtraInfo )
{
  ( mouse_event_Original != nullptr )                             ?
    mouse_event_Original ( dwFlags, dx, dy, dwData, dwExtraInfo ) :
    mouse_event          ( dwFlags, dx, dy, dwData, dwExtraInfo ) ;
}

bool
SK_ImGui_HandlesMessage (MSG *lpMsg, bool /*remove*/, bool /*peek*/)
{
  bool handled = false;

  if ((! lpMsg) || (lpMsg->hwnd != game_window.hWnd && lpMsg->hwnd != game_window.child) || lpMsg->message >= WM_USER)
  {
    return handled;
  }

  if (game_window.hWnd == lpMsg->hwnd)
  {
    static HWND
        hWndLast  = 0;
    if (hWndLast != lpMsg->hwnd)
    {

          __SKX_WinHook_InstallInputHooks (nullptr);
      if (__SKX_WinHook_InstallInputHooks (lpMsg->hwnd))
                                hWndLast = lpMsg->hwnd;
    }
  }

  if (config.input.cursor.manage)
  {
    // The handler may filter a specific timer message.
    if (SK_Input_DetermineMouseIdleState (lpMsg))
      handled = true;
  }

  //if (SK_IsGameWindowActive ())
  {
    switch (lpMsg->message)
    {
      case WM_MOUSELEAVE:
        if (lpMsg->hwnd == game_window.hWnd && game_window.top == 0) 
        {
          game_window.mouse.inside   = false;
          game_window.mouse.tracking = false;

          // We're no longer inside the game window, move the cursor off-screen
          ImGui::GetIO ().MousePos =
            ImVec2 (-FLT_MAX, -FLT_MAX);
        }
        break;

      case WM_CHAR:
      case WM_MENUCHAR:
      {
        if (game_window.active && (game_window.hWnd == lpMsg->hwnd || game_window.child == lpMsg->hwnd))
        {
          return
            SK_ImGui_WantTextCapture ();
        }
      } break;

      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
      {
        if (game_window.active)
        {
          if ((SK_Console::getInstance ()->KeyDown (
                        lpMsg->wParam & 0xFF,
                        lpMsg->lParam
              ) != 0 && lpMsg->message != WM_SYSKEYDOWN)
                     ||
              SK_ImGui_WantKeyboardCapture ()
            )
          {
            if (ImGui_WndProcHandler (lpMsg->hwnd,   lpMsg->message,
                                      lpMsg->wParam, lpMsg->lParam))
            {
              game_window.DefWindowProc (lpMsg->hwnd,   lpMsg->message,
                                         lpMsg->wParam, lpMsg->lParam);
              handled = true;
            }
          }
        }
      } break;

      case WM_KEYUP:
      case WM_SYSKEYUP:
      {
        if (game_window.active)
        {
          if ((SK_Console::getInstance ()->KeyUp (
                        lpMsg->wParam & 0xFF,
                        lpMsg->lParam
              ) != 0 && lpMsg->message != WM_SYSKEYUP)
                     ||
              SK_ImGui_WantKeyboardCapture ()
            )
          {
            if (ImGui_WndProcHandler (lpMsg->hwnd,   lpMsg->message,
                                      lpMsg->wParam, lpMsg->lParam))
            {
              //game_window.DefWindowProc ( lpMsg->hwnd,   lpMsg->message,
              //                            lpMsg->wParam, lpMsg->lParam );
              //handled = true;
            }
          }
        }
      } break;

      case WM_SETCURSOR:
      {
        handled =
          ( 0 != ImGui_WndProcHandler (lpMsg->hwnd,   lpMsg->message,
                                       lpMsg->wParam, lpMsg->lParam) );
      } break;


      case WM_INPUT:
      {
        if (config.input.gamepad.hook_raw_input)
        {
          bool should_handle = false;

          auto& hWnd   = lpMsg->hwnd;
          auto& uMsg   = lpMsg->message;
          auto& wParam = lpMsg->wParam;
          auto& lParam = lpMsg->lParam;
          
          bool        bWantMouseCapture    =
              SK_ImGui_WantMouseCapture    (),
                      bWantKeyboardCapture =
              SK_ImGui_WantKeyboardCapture (),
                      bWantGamepadCapture  =
              SK_ImGui_WantGamepadCapture  ();
          
          bool bWantAnyCapture = bWantMouseCapture    ||
                                 bWantKeyboardCapture ||
                                 bWantGamepadCapture;

          if (bWantAnyCapture)
          {
            bool mouse = false,
              keyboard = false,
               gamepad = false;

            SK_Input_ClassifyRawInput ((HRAWINPUT)lParam, mouse, keyboard, gamepad);
            
            if (mouse && SK_ImGui_WantMouseCapture ())
            {
              should_handle = true;
            }
            
            if (keyboard && SK_ImGui_WantKeyboardCapture ())
            {
              should_handle = true;
            }
            
            if (gamepad && SK_ImGui_WantGamepadCapture ())
            {
              should_handle = true;
            }
            
            if (should_handle)
            {
              handled =
                (  0 !=
                 ImGui_WndProcHandler (lpMsg->hwnd,   lpMsg->message,
                                       lpMsg->wParam, lpMsg->lParam));
            }
          }
          
          // Cleanup the message, we'll re-write the message to WM_NULL later
          if (handled)
          {
            IsWindowUnicode (lpMsg->hwnd) ?
              DefWindowProcW (hWnd, uMsg, wParam, lParam) :
              DefWindowProcA (hWnd, uMsg, wParam, lParam);
          }
        }
      } break;

      // Pre-Dispose These Messages (fixes The Witness)
      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_LBUTTONDBLCLK:
      case WM_MBUTTONDBLCLK:
      case WM_MBUTTONUP:
      case WM_RBUTTONDBLCLK:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
      case WM_XBUTTONDBLCLK:
      case WM_XBUTTONDOWN:
      case WM_XBUTTONUP:

      case WM_CAPTURECHANGED:
      case WM_MOUSEMOVE:
      case WM_MOUSEWHEEL:
      case WM_MOUSEHWHEEL:
      {
        handled =
          (0 != ImGui_WndProcHandler (lpMsg->hwnd,   lpMsg->message,
                                      lpMsg->wParam, lpMsg->lParam)) &&
             SK_ImGui_WantMouseCapture ();
      } break;

      case WM_WINDOWPOSCHANGING:
        break;

    }

    switch (lpMsg->message)
    {
      // TODO: Does this message have an HWND always?
      case WM_DEVICECHANGE:
      {
        handled =
          ( 0 != ImGui_WndProcHandler ( lpMsg->hwnd,   lpMsg->message,
                                        lpMsg->wParam, lpMsg->lParam ) );
      } break;

      case WM_DPICHANGED:
      {
        if (SK_GetThreadDpiAwareness () != DPI_AWARENESS_UNAWARE)
        {
          const RECT* suggested_rect =
              (RECT *)lpMsg->lParam;

          SK_LOG0 ( ( L"DPI Scaling Changed: %lu (%.0f%%)",
                        HIWORD (lpMsg->wParam),
                ((float)HIWORD (lpMsg->wParam) / (float)USER_DEFAULT_SCREEN_DPI) * 100.0f ),
                      L"Window Mgr" );

          ::SetWindowPos (
            lpMsg->hwnd, HWND_TOP,
              suggested_rect->left,                         suggested_rect->top,
              suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS
          );

          SK_Window_RepositionIfNeeded ();
        }

        ////extern void
        ////SK_Display_UpdateOutputTopology (void);
        ////SK_Display_UpdateOutputTopology (    );
      } break;

      default:
      {
      } break;
    }
  }

  ////SK_LOG1 ( ( L" -- %s Handled!", handled ? L"Was" : L"Was Not" ),
  ////            L"ImGuiTrace" );

  return handled;
}

using  SetThreadExecutionState_pfn = EXECUTION_STATE (WINAPI *)(EXECUTION_STATE esFlags);
static SetThreadExecutionState_pfn
       SetThreadExecutionState_Original = nullptr;

EXECUTION_STATE
WINAPI
SetThreadExecutionState_Detour (EXECUTION_STATE esFlags)
{
  SK_LOG_FIRST_CALL

  // SK has smarter control over this stuff, prevent games from using this...
  //   reset any continuous state so we can micromanage screensaver activation
  if (config.window.disable_screensaver || config.window.fullscreen_no_saver)
  {
    //SetThreadExecutionState_Original (0x0);

    return 0x0;//esFlags;
  }

  return
    SetThreadExecutionState_Original (esFlags);
}

// Parts of the Win32 API that are safe to hook from DLL Main
void SK_Input_PreInit (void)
{
  SK_Input_PreHookWinHook  ();
  SK_Input_PreHookCursor   ();
  SK_Input_PreHookKeyboard ();

  //SK_CreateDLLHook2 (       L"user32",
  //                           "GetMessagePos",
  //                            GetMessagePos_Detour,
  //   static_cast_p2p <void> (&GetMessagePos_Original) );
  //
  //

  SK_CreateDLLHook2 (     L"user32",
                           "SendInput",
                            SendInput_Detour,
   static_cast_p2p <void> (&SendInput_Original) );

  SK_CreateDLLHook2 (       L"user32",
                             "mouse_event",
                              mouse_event_Detour,
     static_cast_p2p <void> (&mouse_event_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "SetThreadExecutionState",
                             SetThreadExecutionState_Detour,
    static_cast_p2p <void> (&SetThreadExecutionState_Original) );

  if (config.input.gamepad.hook_raw_input)
    SK_Input_HookRawInput ();

  if (config.input.gamepad.hook_windows_gaming)
    SK_Input_HookWGI ();

  if (config.input.gamepad.hook_xinput)
    SK_XInput_InitHotPlugHooks ( );

  if (config.input.gamepad.hook_scepad)
  {
    if (SK_IsModuleLoaded (L"libScePad.dll"))
      SK_Input_HookScePad ();
  }
}

void
SK_Input_Init (void)
{
  // -- Async Init = OFF option may invoke this twice
  //SK_ReleaseAssert (std::exchange (once, true) == false);

  static bool        once = false;
  if (std::exchange (once, true))
    return;

  SK_Input_InitKeyboard ();

  auto *cp =
    SK_Render_InitializeSharedCVars ();

  auto CreateInputVar_Bool = [&](auto name, auto config_var)
  {
    cp->AddVariable  (
      name,
        SK_CreateVar ( SK_IVariable::Boolean,
                         config_var
                     )
    );
  };

  auto CreateInputVar_Int = [&](auto name, auto config_var)
  {
    cp->AddVariable  (
      name,
        SK_CreateVar ( SK_IVariable::Int,
                         config_var
                     )
    );
  };

  CreateInputVar_Bool ("Input.Keyboard.DisableToGame", &config.input.keyboard.disabled_to_game);
  CreateInputVar_Bool ("Input.Mouse.DisableToGame",    &config.input.mouse.disabled_to_game);
  CreateInputVar_Bool ("Input.Gamepad.DisableToGame",  &config.input.gamepad.disabled_to_game);
  CreateInputVar_Bool ("Input.Gamepad.DisableRumble",  &config.input.gamepad.disable_rumble);
  CreateInputVar_Bool ("Input.XInput.HideAllDevices",  &config.input.gamepad.xinput.blackout_api);
  CreateInputVar_Bool ("Input.XInput.EnableEmulation", &config.input.gamepad.xinput.emulate);

  CreateInputVar_Int  ("Input.Steam.UIController",     &config.input.gamepad.steam.ui_slot);
  CreateInputVar_Int  ("Input.XInput.UIController",    &config.input.gamepad.xinput.ui_slot);

  SK_Input_PreHookHID    ();
  SK_Input_PreHookDI8    ();
  SK_Input_PreHookXInput ();
  SK_Input_PreHookScePad ();
  SK_Input_PreHookWinMM  ();
}


void
SK_Input_SetLatencyMarker (void) noexcept
{
  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

                  DWORD64 ulFramesDrawn =
                      SK_GetFramesDrawn ( );
  static volatile DWORD64  ulLastFrame  = 0;
  if (ReadULong64Acquire (&ulLastFrame) < ulFramesDrawn)
  {
    rb.setLatencyMarkerNV (INPUT_SAMPLE);

    WriteULong64Release (&ulLastFrame, ulFramesDrawn);
  }
}


// SK doesn't use SDL, but many games crash on exit due to polling
//   joystick after XInput is unloaded... so we'll just terminate
//     the thread manually so it doesn't crash.
void
SK_SDL_ShutdownInput (void)
{
  auto tidSDL =
    SK_Thread_FindByName (L"SDL_joystick");

  if (tidSDL != 0)
  {
    SK_AutoHandle hThread (
      OpenThread (THREAD_ALL_ACCESS, FALSE, tidSDL)
    );

    if (hThread.isValid ())
    {
      SuspendThread      (hThread);
      SK_TerminateThread (hThread, 0xDEADC0DE);
    }
  }
}

#include <imgui/font_awesome.h>

extern bool
_ShouldRecheckStatus (INT iJoyID);

int
SK_ImGui_ProcessGamepadStatusBar (bool bDraw)
{
  int attached_pads = 0;

  static const char* szBatteryLevels [] = {
    ICON_FA_BATTERY_EMPTY,
    ICON_FA_BATTERY_QUARTER,
    ICON_FA_BATTERY_HALF,
    ICON_FA_BATTERY_FULL
  };

  static constexpr
    std::array <const char*, 4> szGamepadSymbols {
      "\t" ICON_FA_GAMEPAD " 0",//\xe2\x82\x80" /*(0)*/,
      "\t" ICON_FA_GAMEPAD " 1",//\xe2\x82\x81" /*(1)*/,
      "\t" ICON_FA_GAMEPAD " 2",//\xe2\x82\x82" /*(2)*/,
      "\t" ICON_FA_GAMEPAD " 3" //\xe2\x82\x83" /*(3)*/
    };

  static ImColor battery_colors [] = {
    ImColor::HSV (0.0f, 1.0f, 1.0f), ImColor::HSV (0.1f, 1.0f, 1.0f),
    ImColor::HSV (0.2f, 1.0f, 1.0f), ImColor::HSV (0.4f, 1.0f, 1.0f)
  };

  struct gamepad_cache_s
  {
    DWORD   slot          = 0;
    BOOL    attached      = FALSE;
    ULONG64 checked_frame = INFINITE;

    struct battery_s
    {
      DWORD                      last_checked =   0  ;
      XINPUT_BATTERY_INFORMATION battery_info = {   };
      bool                       draining     = false;
      bool                       wired        = false;
    } battery;
  } static
      gamepads [4] =
        { { 0 }, { 1 },
          { 2 }, { 3 } };

  static auto constexpr
    _BatteryPollingIntervalMs = 10000UL;

  auto BatteryStateTTL =
    SK::ControlPanel::current_time - _BatteryPollingIntervalMs;

  auto current_frame =
    SK_GetFramesDrawn ();

  if (bDraw) ImGui::BeginGroup ();

  for ( auto& gamepad : gamepads )
  {
    auto& battery =
      gamepad.battery;

    if ( current_frame !=
           std::exchange ( gamepad.checked_frame,
                                   current_frame ) )
    {
      gamepad.attached =
        SK_XInput_WasLastPollSuccessful (gamepad.slot);
    }

    if (battery.last_checked < BatteryStateTTL || (! gamepad.attached))
    {   battery.draining = false;
        battery.wired    = false;

      if (gamepad.attached)
      {
        if ( ERROR_SUCCESS ==
               SK_XInput_GetBatteryInformation
               (  gamepad.slot,
                          BATTERY_DEVTYPE_GAMEPAD,
                 &battery.battery_info ) )
        { switch (battery.battery_info.BatteryType)
          {
            case BATTERY_TYPE_ALKALINE:
            case BATTERY_TYPE_UNKNOWN: // Lithium Ion ?
            case BATTERY_TYPE_NIMH:
              battery.draining = true;
              break;

            case BATTERY_TYPE_WIRED:
              battery.wired = true;
              break;

            case BATTERY_TYPE_DISCONNECTED:
            default:
              // WTF? Success on a disconnected controller?
              break;
          }

          if (battery.wired || battery.draining)
              battery.last_checked = SK::ControlPanel::current_time;
          else
              battery.last_checked = BatteryStateTTL + 1000; // Retry in 1 second
        }
      }

      if (battery.draining && gamepad.attached && battery.battery_info.BatteryLevel <= BATTERY_LEVEL_LOW)
      {
        std::string label =
          SK_FormatString ("XInput_Gamepad%d.Battery_Low", gamepad.slot);

        SK_ImGui_CreateNotification (
          label.c_str (), SK_ImGui_Toast::Warning,
            SK_FormatString ("Gamepad %d's Battery Level Is Critically Low", gamepad.slot).c_str (),
            nullptr, 10000UL, SK_ImGui_Toast::UseDuration |
                              SK_ImGui_Toast::ShowCaption |
                              SK_ImGui_Toast::ShowOnce
        );
      }
    }

    ImVec4 gamepad_color =
      ImVec4 (0.5f, 0.5f, 0.5f, 1.0f);

    float fInputAge =
      SK_XInput_Backend->getInputAge ((sk_input_dev_type)(1 << gamepad.slot));

    if (fInputAge < 2.0f)
    {
      gamepad_color =
        ImVec4 ( 0.5f + 0.25f * (2.0f - fInputAge),
                 0.5f + 0.25f * (2.0f - fInputAge),
                 0.5f + 0.25f * (2.0f - fInputAge), 1.0f );
    }

    if (gamepad.attached)
    {
      ++attached_pads;

      if (bDraw)
      {
        ImGui::BeginGroup  ();
        ImGui::TextColored (gamepad_color, szGamepadSymbols [gamepad.slot]);
        ImGui::SameLine    ();

        if (battery.draining)
        {
          auto batteryLevel =
            std::min (battery.battery_info.BatteryLevel, 3ui8);

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

          ImGui::TextColored ( batteryColor, "%hs",
              szBatteryLevels [batteryLevel]
          );
        }

        else
        {
          ImGui::TextColored ( gamepad_color,
              battery.wired ? ICON_FA_USB
                            : ICON_FA_QUESTION_CIRCLE
          );
        }

        ImGui::EndGroup ();

        if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Click to turn off (if supported)");

        if (ImGui::IsItemClicked ())
        {
          SK_XInput_PowerOff (gamepad.slot);
        }

        ImGui::SameLine ();
      }
    }
  }

  if (bDraw)
  {
    ImGui::Spacing  ();
    ImGui::EndGroup ();
  }

  return
    attached_pads;
}


SK_LazyGlobal <sk_input_api_context_s> SK_XInput_Backend;
SK_LazyGlobal <sk_input_api_context_s> SK_ScePad_Backend;
SK_LazyGlobal <sk_input_api_context_s> SK_WGI_Backend;
SK_LazyGlobal <sk_input_api_context_s> SK_HID_Backend;
SK_LazyGlobal <sk_input_api_context_s> SK_RawInput_Backend;
SK_LazyGlobal <sk_input_api_context_s> SK_MessageBus_Backend; // NVIDIA stuff

SK_LazyGlobal <sk_input_api_context_s> SK_Win32_Backend;
SK_LazyGlobal <sk_input_api_context_s> SK_WinHook_Backend;
SK_LazyGlobal <sk_input_api_context_s> SK_WinMM_Backend;