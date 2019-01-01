#pragma once

#include <mmsystem.h>
#include <Windows.h>

volatile LONG
  __SK_KeyMessageCount = 0;

IMGUI_API
bool SK_ImGui_Visible = false;

#include <SpecialK/utility.h>
#include <SpecialK/config.h>
#include <SpecialK/thread.h>
#include <SpecialK/tls.h>

#include <SpecialK/console.h>
#include <SpecialK/window.h>



extern IMGUI_API bool SK_ImGui_Visible;
extern bool           SK_ImGui_IsMouseRelevant (void);

extern HWND SK_GetParentWindow (HWND);

extern void __stdcall SK_ImGui_DrawEULA (LPVOID reserved);
struct show_eula_s {
  bool show             = false;
  bool never_show_again = false;
} eula;


const ImWchar*
SK_ImGui_GetGlyphRangesDefaultEx (void)
{
  static const ImWchar ranges [] =
  {
    0x0020, 0x00FF, // Basic Latin + Latin Supplement
    0x0100, 0x03FF, // Latin, IPA, Greek
    0x2000, 0x206F, // General Punctuation
    0x2100, 0x21FF, // Letterlike Symbols
    0x2600, 0x26FF, // Misc. Characters
    0x2700, 0x27BF, // Dingbats
    0x207f, 0x2090, // N/A (literally, the symbols for N/A :P)
    0
  };
  return &ranges [0];
}

SK_Thread_HybridSpinlock font_lock (300);

void
SK_ImGui_LoadFonts (void)
{
  static volatile LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    ImFontConfig font_cfg = { };
    font_cfg.MergeMode    = true;

    auto LoadFont = [](const std::string& filename, float point_size, const ImWchar* glyph_range, ImFontConfig* cfg = nullptr)
    {
      char szFullPath [ MAX_PATH * 2 + 1 ] = { };

      if (GetFileAttributesA (filename.c_str ()) != INVALID_FILE_ATTRIBUTES)
         strncpy (szFullPath, filename.c_str (), MAX_PATH * 2);

      else
      {
        snprintf (szFullPath, MAX_PATH * 2 - 1, R"(%ws\%s)", SK_GetFontsDir ().c_str (), filename.c_str ());


        if (GetFileAttributesA (szFullPath) == INVALID_FILE_ATTRIBUTES)
          *szFullPath = '\0';
      }

      if (*szFullPath != '\0')
      {
        ImGuiIO& io =
          ImGui::GetIO ();

        return
          io.Fonts->AddFontFromFileTTF ( szFullPath,
                                           point_size,
                                             cfg,
                                               glyph_range );
      }

      return (ImFont *)nullptr;
    };

    if (! LoadFont (
            config.imgui.font.default_font.file,
              config.imgui.font.default_font.size,
                SK_ImGui_GetGlyphRangesDefaultEx () ) )
    {

      io.Fonts->AddFontDefault ();
    }

    LoadFont (config.imgui.font.japanese.file,  config.imgui.font.japanese.size, io.Fonts->GetGlyphRangesJapanese (), &font_cfg);
    LoadFont (config.imgui.font.chinese.file,   config.imgui.font.chinese.size,  io.Fonts->GetGlyphRangesChinese  (), &font_cfg);
    //LoadFont (config.imgui.font.korean.file,    config.imgui.font.korean.size,   io.Fonts->GetGlyphRangesKorean   (), &font_cfg);
    LoadFont (config.imgui.font.cyrillic.file,  config.imgui.font.cyrillic.size, io.Fonts->GetGlyphRangesCyrillic (), &font_cfg);

    io.Fonts->AddFontDefault ();

    InterlockedIncrement (&init);
  }

  SK_Thread_SpinUntilAtomicMin (&init, 2);
}

#include <windowsx.h>
#include <SpecialK/hooks.h>


extern float analog_sensitivity;

#include <set>
#include <SpecialK/log.h>


extern INT SK_ImGui_ActivationKeys [256];

#define SK_RAWINPUT_READ(type)  SK_RawInput_Backend.markRead  (type);
#define SK_RAWINPUT_WRITE(type) SK_RawInput_Backend.markWrite (type);

SK_Thread_HybridSpinlock raw_input_lock (13);

UINT
WINAPI
SK_ImGui_ProcessRawInput ( _In_      HRAWINPUT hRawInput,
                           _In_      UINT      uiCommand,
                           _Out_opt_ LPVOID    pData,
                           _Inout_   PUINT     pcbSize,
                           _In_      UINT      cbSizeHeader,
                                     BOOL      self )
{
  SK_TLS *pTLS = nullptr;
    ;

  if (                     pData == nullptr ||
       (pTLS = SK_TLS_Bottom ()) == nullptr )
  {
    return
      SK_GetRawInputData ( hRawInput, uiCommand,
                               pData, pcbSize,
                                        cbSizeHeader );
  }

  auto pRawCtx =
    &pTLS->raw_input;

  bool focus = game_window.active;

  bool already_processed =
    ( pRawCtx->last_input == hRawInput &&
                uiCommand == RID_INPUT );

  pRawCtx->last_input = (
    ( (! self) && uiCommand == RID_INPUT ) ?
                     hRawInput : pRawCtx->last_input );


  ////if (self && (! already_processed))
  ////{
  ////  if (SK_ImGui_WantMouseCapture ())
  ////    SK_RawInput_EnableLegacyMouse  (true);
  ////  else
  ////    SK_RawInput_RestoreLegacyMouse ();
  ////}
  //
  //SK_RawInput_EnableLegacyMouse  (true);
  //
  //// Keep this on ALWAYS to fix Steam Overlay in Skyrim SE
  ////
  ////if (SK_ImGui_WantKeyboardCapture ())
  //  SK_RawInput_EnableLegacyKeyboard (true);
  ////else
  ////  SK_RawInput_RestoreLegacyKeyboard ();

  int size =
    SK_GetRawInputData (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

  bool filter   = false;
  bool mouse    = false;
  bool keyboard = false;

  // Input event happened while the window had focus if true, otherwise another
  //   window is currently capturing input and the most appropriate response is
  //     usually to ignore the event.
  bool foreground =
    GET_RAWINPUT_CODE_WPARAM (((RAWINPUT *)pData)->header.wParam) == RIM_INPUT;

  auto FilterRawInput =
    [&](UINT uiCommand, RAWINPUT* pData, bool& mouse, bool& keyboard) ->
     bool
      {
        bool filter = false;

        switch (pData->header.dwType)
        {
          case RIM_TYPEMOUSE:
          {
            if (  SK_ImGui_IsMouseRelevant  ())
            { if (SK_ImGui_WantMouseCapture ())
                filter = true;
            }

            // Block mouse input to the game while it's in the background
            if (config.window.background_render && (! game_window.active))
              filter = true;

            mouse = true;

            if ( ((! self) && (! already_processed))
                           && uiCommand == RID_INPUT &&
                  (! filter) )
            {
              SK_RAWINPUT_READ (sk_input_dev_type::Mouse)
            }
          } break;


          case RIM_TYPEKEYBOARD:
          {
            InterlockedIncrement (&__SK_KeyMessageCount);

            USHORT VKey =
              (((RAWINPUT *)pData)->data.keyboard.VKey & 0xFF);

            if (SK_ImGui_IsMouseRelevant ())
            {
              // Only filter keydown message, not key releases
              if (SK_ImGui_WantKeyboardCapture ())
              {
                if (VKey & 0xF8) // Valid Keys:  8 - 255
                  filter = true;
              }


              if (SK_ImGui_WantMouseCapture ())
              {
                // That's actually a mouse button...
                if (foreground && VKey < 7)
                {
                  if (((RAWINPUT *)pData)->data.keyboard.Message == WM_KEYDOWN)
                    filter = true;

                  if (((RAWINPUT *)pData)->data.keyboard.Message == WM_SYSKEYDOWN)
                    filter = true;

                  if (((RAWINPUT *)pData)->data.keyboard.Message == WM_KEYUP)
                    filter = true;

                  if (((RAWINPUT *)pData)->data.keyboard.Message == WM_SYSKEYUP)
                    filter = true;
                }
              }
            }


            // Block keyboard input to the game while the console is active
            if (SK_Console::getInstance ()->isVisible () && (VKey & 0xFF) > 7)
              filter = true;


            // Block keyboard input to the game while it's in the background
            if (config.window.background_render && (! focus/*game_window.active*/))
              filter = true;


            if (VKey & 0xF8) // Valid Keys:  8 - 255
              keyboard = true;

            if (keyboard && ( SK_ImGui_WantKeyboardCapture () || (((RAWINPUT *)pData)->data.keyboard.Message == WM_CHAR ||
                                                                  ((RAWINPUT *)pData)->data.keyboard.Message == WM_SYSCHAR) ))
              filter = true;


            if ( ((! self) && (! already_processed))
                           && uiCommand == RID_INPUT
                           && (! filter) )
            {
              SK_RAWINPUT_READ (sk_input_dev_type::Keyboard)
            }
          } break;


          default:
          {
            // TODO: Determine which controller the input is from
            if (SK_ImGui_WantGamepadCapture ())
              filter = true;

            if ( ((! self) && (! already_processed))
                           && uiCommand == RID_INPUT
                           && (! filter) )
            {
              SK_RAWINPUT_READ (sk_input_dev_type::Gamepad)
            }
          } break;
        }

        return filter;
      };


  filter =
    FilterRawInput (uiCommand, (RAWINPUT *)pData, mouse, keyboard);


  if (uiCommand == RID_INPUT /*&& SK_ImGui_Visible*/)
  {
    switch (((RAWINPUT *)pData)->header.dwType)
    {
      case RIM_TYPEMOUSE:
      {
        if (self)
        {
          if (SK_ImGui_IsMouseRelevant () && config.input.mouse.add_relative_motion)
          {
            // 99% of games don't need this, and if we use relative motion to update the cursor position that
            //   requires re-synchronizing with the desktop's logical cursor coordinates at some point because
            //     Raw Input does not include cursor acceleration, etc.
            POINT client { ((RAWINPUT *)pData)->data.mouse.lLastX,
                           ((RAWINPUT *)pData)->data.mouse.lLastY };

            ////SK_ImGui_Cursor.ClientToLocal (&client);

            SK_ImGui_Cursor.pos.x += client.x;
            SK_ImGui_Cursor.pos.y += client.y;

            ImGui::GetIO ().MousePos.x = (float)SK_ImGui_Cursor.pos.x;
            ImGui::GetIO ().MousePos.y = (float)SK_ImGui_Cursor.pos.y;

            SK_ImGui_Cursor.update ();
          }

          if (foreground)
          {
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_LEFT_BUTTON_DOWN   )
              ImGui::GetIO ().MouseDown [0] = true;
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_RIGHT_BUTTON_DOWN  )
              ImGui::GetIO ().MouseDown [1] = true;
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_MIDDLE_BUTTON_DOWN )
              ImGui::GetIO ().MouseDown [2] = true;
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_BUTTON_4_DOWN      )
              ImGui::GetIO ().MouseDown [3] = true;
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_BUTTON_5_DOWN      )
              ImGui::GetIO ().MouseDown [4] = true;
          }

          if ( ((RAWINPUT *)pData)->data.mouse.usButtonFlags == RI_MOUSE_WHEEL       )
          {
            ImGui::GetIO ().MouseWheel +=
            ((float)(short)((RAWINPUT *)pData)->data.mouse.usButtonData) /
             (float)WHEEL_DELTA;

          }
        }
      } break;


      case RIM_TYPEKEYBOARD:
      {
        if (self)
        {
          USHORT VKey =
            ((RAWINPUT *)pData)->data.keyboard.VKey;

          // VKeys 0-7 aren't on the keyboard :)
          if (VKey & 0xFFF8) // Valid Keys:  8 - 65535
          {
            if (foreground)
            {
              if (!(((RAWINPUT *) pData)->data.keyboard.Flags & RI_KEY_BREAK))
              {
                ImGui::GetIO ().KeysDown [VKey & 0xFF] = true;
              }

              switch (((RAWINPUT *) pData)->data.keyboard.Message)
              {
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                  ImGui::GetIO ().KeysDown [VKey & 0xFF] = true;
                  break;

                case WM_CHAR:
                case WM_SYSCHAR:
                  ImGui::GetIO ().AddInputCharacter (VKey);
                  break;
              }
            }
          }

          else
            SK_ReleaseAssert ("Invalid Key Code" && (VKey & 0xFFF8));
        }
      } break;


      default:
        break;
    }
  }


  if (filter || keyboard)
  {
    // Clearing all bytes above would have set the type to mouse, and some games
    //   will actually read data coming from RawInput even when the size returned is 0!
    ((RAWINPUT *)pData)->header.dwType = keyboard ? RIM_TYPEKEYBOARD      :
                                                    mouse ? RIM_TYPEMOUSE :
                                                            RIM_TYPEHID;

    if (! keyboard)
    {
      RtlZeroMemory (pData, *pcbSize);
    }

    // Tell the game this event happened in the background, most will
    //   throw it out quick and easy. Even easier if we tell it the event came
    //     from the keyboard.
    if (filter)
    {
      ((RAWINPUT *)pData)->header.wParam = RIM_INPUTSINK;
      ((RAWINPUT *)pData)->header.dwType = RIM_TYPEKEYBOARD;

      // Fake key release
      ((RAWINPUT *)pData)->data.keyboard.Flags |= RI_KEY_BREAK;

      DefRawInputProc ((RAWINPUT **)&pData, 1, sizeof (RAWINPUTHEADER));
    }

    ///// Ugh, why does everything have to be so complicated? :P
    /////
    /////   This horrible nightmare prevents games from seeing keys as stuck after
    /////     activating the ImGui overlay.
    /////
    if (keyboard)
    {
      bool release = (((RAWINPUT *)pData)->data.keyboard.Flags & RI_KEY_BREAK) != 0;
      bool erase   = SK_ImGui_WantKeyboardCapture ();
      bool invert  = false;

      USHORT VKey =
        ( (RAWINPUT *)pData )->data.keyboard.VKey & 0xFF;

      if (erase)
      {
        if ((! release) && SK_ImGui_ActivationKeys [VKey] > 0)
        {
          SK_ImGui_ActivationKeys [VKey]--;
          invert = true;
        }

        if (invert)
        {
          // We WANT the game to know about this event, don't sink it.
          ((RAWINPUT *)pData)->header.wParam = RIM_INPUT;
          ((RAWINPUT *)pData)->header.dwType = RIM_TYPEKEYBOARD;

          // Fake key release
          ((RAWINPUT *) pData)->data.keyboard.Flags |= RI_KEY_BREAK;

          if (((RAWINPUT *)pData)->data.keyboard.Message == WM_KEYDOWN)
              ((RAWINPUT *)pData)->data.keyboard.Message  = WM_KEYUP;

          else if (((RAWINPUT *)pData)->data.keyboard.Message == WM_SYSKEYDOWN)
                   ((RAWINPUT *)pData)->data.keyboard.Message  = WM_SYSKEYUP;

          else if (((RAWINPUT *)pData)->data.keyboard.Message == WM_CHAR)
                   ((RAWINPUT *)pData)->data.keyboard.Message  = WM_NULL;
        }
      }
    }

    if (! keyboard)
      *pcbSize = 0;

    size = *pcbSize;
  }

  return
    size;
}

// So that we can release them before the game thinks they're stuck :)
void
SK_Input_RememberPressedKeys (void)
{
  for (int i = 7; i < 256; i++)
  {
    SK_ImGui_ActivationKeys [i] += ImGui::GetIO ().KeysDown [i] ? 6 : 0;
  }
}

#if  0
LRESULT
WINAPI
ImGui_UniversalMouseDispatch ( UINT msg, WPARAM wParam, LPARAM lParam )
{
  switch (msg)
  {
    case WM_MOUSEMOVE:
      break;

    case WM_LBUTTONDOWN:
      break;

    case WM_LBUTTONUP:
      break;
  }

  return 0;
}
#endif

bool
SK_ImGui_WantMouseWarpFiltering (void)
{
  extern bool
  SK_InputUtil_IsHWCursorVisible (void);

  if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open && SK_ImGui_IsMouseRelevant       () ) ||
       ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () ) )
  {
    return true;
  }

  return false;
}

LONG
SK_ImGui_DeltaTestMouse (       POINTS& last_pos,
                                DWORD   lParam,
                          const short   threshold = 1 )
{
  bool filter_warps =
    SK_ImGui_WantMouseWarpFiltering ();

  if (filter_warps)
  {
    bool filter = false;

    // Filter out small movements / mouselook warps
    //
    //   This does create a weird deadzone in the center of the screen,
    //     but most people will not notice ;)
    //
    if ( abs (last_pos.x - GET_X_LPARAM (lParam)) < threshold &&
         abs (last_pos.y - GET_Y_LPARAM (lParam)) < threshold )
    {
      filter = true;
    }

    ImGuiIO& io =
      ImGui::GetIO ();

    POINT center { static_cast <LONG> (io.DisplaySize.x / 2.0f),
                   static_cast <LONG> (io.DisplaySize.y / 2.0f) };

    SK_ImGui_Cursor.LocalToClient (&center);

    // Now test the cursor against the center of the screen
    if ( abs (center.x - GET_X_LPARAM (lParam)) <= (static_cast <float> (center.x) / (100.0f / config.input.mouse.antiwarp_deadzone)) &&
         abs (center.y - GET_Y_LPARAM (lParam)) <= (static_cast <float> (center.y) / (100.0f / config.input.mouse.antiwarp_deadzone)) )
    {
      filter = true;
    }

    POINT local { GET_X_LPARAM (lParam),
                  GET_Y_LPARAM (lParam) };

    last_pos.x = static_cast <SHORT> (local.x);
    last_pos.y = static_cast <SHORT> (local.y);

    // Dispose Without Processing
    if (filter)
    {
      return SK_ImGui_IsMouseRelevant () ? 1 : 0;
    }
  }

  return -1;
}

#include <dbt.h>

// GetKeyboardLayout (...) is somewhat slow and it is unnecessary unless
//   WM_INPUTLANGCHANGE is encountered.
void SK_ImGui_InputLanguage_s::update (void)
{
  if (changed)
  {
    keybd_layout = GetKeyboardLayout (0);
    changed      = false;
  }
}
SK_ImGui_InputLanguage_s SK_ImGui_InputLanguage;
//^^^^^^^^ This is per-thread, but we only process input on one.



LRESULT
WINAPI
ImGui_WndProcHandler ( HWND   hWnd,    UINT  msg,
                       WPARAM wParam, LPARAM lParam );

static POINTS last_pos;

bool
MessageProc ( const HWND&   hWnd,
              const UINT&   msg,
              const WPARAM& wParam,
              const LPARAM& lParam )
{
  static bool window_active = true;

  ImGuiIO& io =
    ImGui::GetIO ();

  auto ActivateWindow = [&](bool active = false)
  {
    bool changed = (active != window_active);

    if (active && changed)
      SK_Input_RememberPressedKeys ();

    if ((! active) && changed)
    {
      SecureZeroMemory (io.MouseDown, sizeof (bool) * 5  );
      SecureZeroMemory (io.KeysDown,  sizeof (bool) * 512);
    }

    window_active = active;
  };

  switch (msg)
  {
    case WM_HOTKEY:
    {
      if (SK_ImGui_WantGamepadCapture ())
      {
        return 1;
      }
    } break;

    // TODO: Take the bazillion different sources of input and translate them all into
    //          a standard window message format for sanity's sake during filter evaluation.
    case WM_APPCOMMAND:
    {
      switch (GET_DEVICE_LPARAM (lParam))
      {
        case FAPPCOMMAND_KEY:
        {
          dll_log.Log (L"WM_APPCOMMAND Keyboard Event");

          //if (SK_ImGui_WantKeyboardCapture ())
          //{
          if (window_active)
            return true;
          //}
        } break;

        case FAPPCOMMAND_MOUSE:
        {
          if (SK_ImGui_WantMouseCapture ())
          {
            dll_log.Log (L"Removed WM_APPCOMMAND Mouse Event");
            return true;
          }

          dll_log.Log (L"WM_APPCOMMAND Mouse Event");

          DWORD dwPos = GetMessagePos ();
          LONG  lRet  = SK_ImGui_DeltaTestMouse (*(POINTS *)&last_pos, dwPos);

          if (lRet >= 0)
          {
            dll_log.Log (L"Removed WM_APPCOMMAND Mouse Delta Failure");
            return true;
          }
        } break;
      }
    } break;



    case WM_MOUSEACTIVATE:
    {
      if (hWnd == game_window.hWnd)
      {
        ActivateWindow (((HWND)wParam == hWnd));
      }
    } break;


    case WM_ACTIVATEAPP:
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
    {
      if (hWnd == game_window.hWnd)
      {
        if (msg == WM_NCACTIVATE || msg == WM_ACTIVATEAPP)
        {
          ActivateWindow (wParam != 0x00);
        }

        else if (msg == WM_ACTIVATE)
        {
          switch (LOWORD (wParam))
          {
            case WA_ACTIVE:
            case WA_CLICKACTIVE:
            default: // Unknown
            {
              ActivateWindow ((HWND)lParam != game_window.hWnd);
            } break;

            case WA_INACTIVE:
            {
              ActivateWindow (lParam == 0 || (HWND)lParam == game_window.hWnd);
            } break;
          }
        }
      }
    } break;


    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: // Sent on receipt of the second click
      io.MouseDown [0] = true;
      return true;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK: // Sent on receipt of the second click
      io.MouseDown [1] = true;
      return true;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK: // Sent on receipt of the second click
      io.MouseDown [2] = true;
      return true;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK: // Sent on receipt of the second click
    {
      WORD Flags =
        GET_XBUTTON_WPARAM (wParam);

      io.MouseDown [3] |= (Flags & XBUTTON1) != 0;
      io.MouseDown [4] |= (Flags & XBUTTON2) != 0;

      return true;
    } break;

    // Don't care about these events for anything other than filtering;
    //   we will poll the immediate mouse state when the frame starts.
    //
    //  This effectively gives us buffered mouse input behavior, where
    //    no mouse click is ever lost (only the time that it happened).
    //
    case WM_LBUTTONUP:
      return true;
    case WM_RBUTTONUP:
      return true;
    case WM_MBUTTONUP:
      return true;
    case WM_XBUTTONUP:
      return true;


    case WM_MOUSEWHEEL:
      io.MouseWheel +=
        static_cast <float> (GET_WHEEL_DELTA_WPARAM (wParam)) /
        static_cast <float> (WHEEL_DELTA)                    ;
      return true;

    case WM_INPUTLANGCHANGE:
    {
      SK_ImGui_InputLanguage_s& language =
        SK_TLS_Bottom ()->input_core.input_language;

      language.changed      = true;
      language.keybd_layout = nullptr;
      return false;
    } break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
      InterlockedIncrement (&__SK_KeyMessageCount);

      BYTE  vkCode   = LOWORD (wParam) & 0xFF;
      BYTE  scanCode = HIWORD (lParam) & 0x7F;

      if (vkCode & 0xF8) // Valid Keys:  8 - 255
      {
        // Don't process Alt+Tab or Alt+Enter
        if (msg == WM_SYSKEYDOWN && ( vkCode == VK_TAB || vkCode == VK_RETURN ))
          return false;

        // Just the make / break events, repeats are ignored
        if      ((lParam & 0x40000000UL) == 0)
          SK_Console::getInstance ()->KeyDown (vkCode, MAXDWORD);

        if (vkCode != VK_TAB)
        {
          wchar_t key_str;

          SK_ImGui_InputLanguage_s& language =
            SK_TLS_Bottom ()->input_core.input_language;

          language.update ();

          if ( ToUnicodeEx ( vkCode,
                             scanCode,
                             (const BYTE *)io.KeysDown,
                            &key_str,
                             1,
                             0x04, // Win10-Specific Flag: No Keyboard State Change
                             language.keybd_layout )
                   &&
                iswprint ( key_str )
             )
          {
            ImGui_WndProcHandler (
              hWnd, WM_CHAR, (WPARAM)key_str, lParam
            );
          }
        }
      }

      // Mouse event
      //
      else if (vkCode < 7)
      {
        int remap = -1;

        // Stupid hack, but these indices are discontinuous
        //
        switch (vkCode)
        {
          case VK_LBUTTON:
            remap = 0;
            break;

          case VK_RBUTTON:
            remap = 1;
            break;

          case VK_MBUTTON:
            remap = 2;
            break;

          case VK_XBUTTON1:
            remap = 3;
            break;

          case VK_XBUTTON2:
            remap = 4;
            break;

          default:
            assert (false); // WTF?! These keys don't exist
            break;
        }

        if (remap != -1)
          io.MouseDown [remap] = true;
      }

      return true;
    } break;


    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
      InterlockedIncrement (&__SK_KeyMessageCount);

      BYTE vkCode = LOWORD (wParam) & 0xFF;

      if (vkCode & 0xF8) // Valid Keys:  8 - 255
      {
        // Don't process Alt+Tab or Alt+Enter
        if (msg == WM_SYSKEYUP && ( vkCode == VK_TAB || vkCode == VK_RETURN ))
          return false;

        SK_Console::getInstance ()->KeyUp (vkCode, lParam);

        return true;
      }
    } break;


    case WM_NCMOUSEMOVE:
    case WM_MOUSEMOVE:
    {
      LONG lDeltaRet =
        SK_ImGui_DeltaTestMouse (last_pos, (DWORD)lParam);

      // Return:
      //
      //   -1 if no filtering is desired
      //    0 if the message should be passed onto app, but internal cursor pos unchanged
      //    1 if the message should be completely eradicated
      //
      if (lDeltaRet >= 0)
      {
        if (SK_ImGui_IsMouseRelevant ())
          SK_ImGui_Cursor.update ();

        return lDeltaRet;
      }

      SHORT xPos = GET_X_LPARAM (lParam);
      SHORT yPos = GET_Y_LPARAM (lParam);

      SK_ImGui_Cursor.pos.x = xPos;
      SK_ImGui_Cursor.pos.y = yPos;

      SK_ImGui_Cursor.ClientToLocal (&SK_ImGui_Cursor.pos);

      io.MousePos.x = (float)SK_ImGui_Cursor.pos.x;
      io.MousePos.y = (float)SK_ImGui_Cursor.pos.y;

      if (! SK_ImGui_WantMouseCapture ())
      {
        if (SK_ImGui_IsMouseRelevant ())
          SK_ImGui_Cursor.update ();

        SK_ImGui_Cursor.orig_pos =
        SK_ImGui_Cursor.pos;

        return TRUE;
      }

      if (SK_ImGui_IsMouseRelevant ())
        SK_ImGui_Cursor.update ();

      return true;
    } break;


    case WM_CHAR:
    {
      InterlockedIncrement (&__SK_KeyMessageCount);

      // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
      if ((wParam & 0xff) > 7 && wParam < 0x10000)
      {
        io.AddInputCharacter ((unsigned short)(wParam & 0xFFFF));
      }

      return true;
    } break;


    case WM_INPUT:
    {
      bool bRet = false;

      extern UINT
      SK_Input_ClassifyRawInput ( HRAWINPUT lParam, bool& mouse,
                                                    bool& keyboard,
                                                    bool& gamepad );

      bool mouse    = false,
           keyboard = false,
           gamepad  = false;

      //GET_RAWINPUT_CODE_WPARAM

      UINT dwSize =
        SK_Input_ClassifyRawInput ( (HRAWINPUT)lParam,
                                             mouse,
                                               keyboard,
                                                 gamepad );

      if (dwSize > 0)
      {
        if ((mouse    && SK_ImGui_WantMouseCapture    ()) ||
            (keyboard && SK_ImGui_WantKeyboardCapture ()) ||
            (gamepad  && SK_ImGui_WantGamepadCapture  ()))
        {
          LPBYTE lpb =
            SK_TLS_Bottom ()->raw_input.allocData (dwSize);

          if ( SK_ImGui_ProcessRawInput ( (HRAWINPUT)lParam, RID_INPUT,
                                                        lpb, &dwSize,
                                                        sizeof (RAWINPUTHEADER), TRUE ) > 0 )
          {
            ///wParam       =
            ///  ( ( wParam & ~0xFF )
            ///             |  RIM_INPUTSINK );

            bRet = true;
          }
        }
      }

      if (game_window.DefWindowProc != nullptr)
      {
        game_window.DefWindowProc (
          hWnd, msg,
                wParam, lParam
        );
      }

      else
      {
        IsWindowUnicode  (hWnd) ?
          DefWindowProcW (hWnd, msg, wParam, lParam) :
          DefWindowProcA (hWnd, msg, wParam, lParam);
      }

      return bRet;
    } break;
  }

  return false;
};

LRESULT
WINAPI
ImGui_WndProcHandler ( HWND   hWnd,    UINT  msg,
                       WPARAM wParam, LPARAM lParam )
{
  // Handle this message, but don't remove it.
  if (msg == WM_DISPLAYCHANGE)
  {
    SK_LOG0 ( (L"Handling WM_DISPLAYCHANGE"), L"Window Mgr");

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    if ( ((int)rb.api & (int)SK_RenderAPI::D3D11) ||
         ((int)rb.api & (int)SK_RenderAPI::D3D12 ))
    {
      extern void
      SK_DXGI_UpdateSwapChain (IDXGISwapChain*);

      CComQIPtr <IDXGISwapChain> pSwap (rb.swapchain);

      if (pSwap != nullptr)
      {
        SK_DXGI_UpdateSwapChain (pSwap);
      }
    }
  }



  if (msg == WM_SETCURSOR)
  {
    if ( SK_ImGui_IsMouseRelevant  () ||
         SK_ImGui_WantMouseCapture ()    )
    {
      SK_ImGui_Cursor.update ();
      return TRUE;
    }
  }


  extern bool
  SK_ImGui_WantExit;

  if (msg == WM_SYSCOMMAND)
  {
    //dll_log.Log (L"WM_SYSCOMMAND (wParam=%x, lParam=%x) [HWND=%x] :: Game Window = %x", (wParam & 0xFFF0), lParam, hWnd, game_window.hWnd);

    switch (LOWORD (wParam & 0xFFF0))
    {
      case SC_RESTORE:
      case SC_SIZE:
      case SC_PREVWINDOW:
      case SC_NEXTWINDOW:
      case SC_MOVE:
      case SC_MOUSEMENU:
      case SC_MINIMIZE:
      case SC_MAXIMIZE:
      case SC_DEFAULT:
      case SC_CONTEXTHELP:
      {
        return
          DefWindowProcW (hWnd, msg, wParam, lParam);
      } break;

      // Generally an application will handle this, but if it doesn't,
      //   trigger Special K's popup window,
      case SC_CLOSE:
        if (game_window.hWnd == hWnd)
        {
          SK_ImGui_WantExit |= config.input.keyboard.catch_alt_f4;

          if (config.input.keyboard.catch_alt_f4)
            return 0;
        } break;

      case SC_KEYMENU:
        if (game_window.hWnd == hWnd)
        {
          // Disable ALT application menu
          if (lParam == 0x00 || lParam == 0x20)
          {
            return 0;
          }

          else if (lParam == 0x05/*VK_F4*/) // DOES NOT USE Virtual Key Codes!
          {
            SK_ImGui_WantExit |= config.input.keyboard.catch_alt_f4;

            if (config.input.keyboard.catch_alt_f4)
              return 0;
          }
          
          else
          {
            return
              DefWindowProcW (hWnd, msg, wParam, lParam);
          }
        }
        break;

      case SC_SCREENSAVE:
      case SC_MONITORPOWER:
        if (config.window.disable_screensaver)
          return 0;
        break;
    }
  }



  if (msg == WM_DEVICECHANGE)
  {
    switch (wParam)
    {
      case DBT_DEVICEARRIVAL:
      {
        auto *pHdr = reinterpret_cast <DEV_BROADCAST_HDR *> (lParam);

        if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
        {
          if ( config.input.gamepad.xinput.placehold [0] || config.input.gamepad.xinput.placehold [1] ||
               config.input.gamepad.xinput.placehold [2] || config.input.gamepad.xinput.placehold [3] )
          {
            dll_log.Log (L"[XInput_Hot]  (Input Device Connected)");
            return true;
          }
        }
      } break;

      case DBT_DEVICEQUERYREMOVE:
      case DBT_DEVICEREMOVEPENDING:
      case DBT_DEVICEREMOVECOMPLETE:
      {
        auto *pHdr = reinterpret_cast <DEV_BROADCAST_HDR *> (lParam);

        if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
        {
          if ( config.input.gamepad.xinput.placehold [0] || config.input.gamepad.xinput.placehold [1] ||
               config.input.gamepad.xinput.placehold [2] || config.input.gamepad.xinput.placehold [3] )
          {
            dll_log.Log (L"[XInput_Hot]  (Input Device Disconnected)");
            return true;
          }
        }
      } break;
    }
  }

  UNREFERENCED_PARAMETER (lParam);

  bool handled          = MessageProc (hWnd, msg, wParam, lParam);
  bool filter_raw_input = (msg == WM_INPUT && handled);


  bool filter_warps = SK_ImGui_WantMouseWarpFiltering ();

  UINT uMsg = msg;

  if (/*SK_ImGui_Visible &&*/ handled)
  {
    bool keyboard_capture =
      ( ( (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST) ||
           uMsg == WM_HOTKEY     ||
         ( uMsg == WM_APPCOMMAND && GET_DEVICE_LPARAM (lParam)  == FAPPCOMMAND_KEY   ) ) &&
          SK_ImGui_WantKeyboardCapture () );

    bool mouse_capture =
      ( ( ( uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST ) ||
          ( uMsg == WM_APPCOMMAND && GET_DEVICE_LPARAM (lParam) == FAPPCOMMAND_MOUSE ) ) &&

          ( SK_ImGui_WantMouseCapture () ||
            (filter_warps && uMsg == WM_MOUSEMOVE) )
      );

    if ((wParam & 0xFF) < 7)
    {
      // Some games use Virtual Key Codes 1-6 (mouse button 0-4)
      //   instead of WM_LBUTTONDOWN, etc.
      if ( ( SK_ImGui_WantMouseCapture () && uMsg == WM_KEYDOWN ) ||
           ( SK_ImGui_WantMouseCapture () && uMsg == WM_KEYUP   ) )
      {
        // Block Mouse Input
        mouse_capture    = true;
        keyboard_capture = false;
      }
    }

    if (config.input.ui.capture_mouse)
    {
      mouse_capture = (uMsg >= WM_MOUSEFIRST  && uMsg <= WM_MOUSELAST);
    }

    if ( keyboard_capture || mouse_capture || filter_raw_input )
    {
      /////if (uMsg == WM_INPUT && filter_raw_input)
      /////{
      /////  if (GET_RAWINPUT_CODE_WPARAM (wParam) == RIM_INPUT)
      /////  {
      /////    bool bUnicode =
      /////      IsWindowUnicode (hWnd);
      /////
      /////    ( bUnicode ? DefWindowProcW (hWnd, uMsg, lParam, wParam) :
      /////                 DefWindowProcA (hWnd, uMsg, lParam, wParam) );
      /////
      /////    return 1;
      /////  }
      /////
      /////  return 0;
      /////}

      return 1;
    }
  }

  return 0;
}


#include <algorithm>

float analog_sensitivity = 333.33f;

#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>

bool  nav_usable       = false;

extern bool SK_ImGui_Visible;

bool
_Success_(false)
SK_ImGui_FilterXInput (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  bool disable =
    config.input.gamepad.disabled_to_game ||
      ( SK_ImGui_WantGamepadCapture ()    &&
        dwUserIndex == (DWORD)config.input.gamepad.xinput.ui_slot );

  if (disable)
  {
    ZeroMemory (&pState->Gamepad, sizeof XINPUT_GAMEPAD);

    // SDL Keepalive
    pState->dwPacketNumber =
      std::max (1UL, pState->dwPacketNumber);

    return true;
  }

  return false;
}


struct {
  struct linear_pulse_event_s {
    float duration, strength,
          start,    end;

    float run (void) {
      auto now = static_cast <float> (timeGetTime ());

      return config.input.gamepad.haptic_ui ?
               std::max (0.0f, strength * ((end - now) / (end - start))) :
                         0.0f;
    }
  } PulseTitle  { 350.0f,   1.0f,  0.0f, 0.0f, },
    PulseButton {  85.0f,   0.5f,  0.0f, 0.0f, },
    PulseNav    {  60.0f, 0.015f,  0.0f, 0.0f, };
} haptic_events;


bool
WINAPI
SK_XInput_PulseController ( INT   iJoyID,
                            float fStrengthLeft,
                            float fStrengthRight );
extern void
WINAPI
SK_XInput_ZeroHaptics ( INT iJoyID );


#include <SpecialK/steam_api.h>

extern void SK_ImGui_Toggle (void);

bool
WINAPI
SK_ImGui_ToggleEx (bool& toggle_ui, bool& toggle_nav)
{
  // XXX: HACK for Monster Hunter: World
  game_window.active = true;

  if (toggle_ui)
    SK_ImGui_Toggle ();

  if (toggle_nav && SK_ImGui_Active ())
    nav_usable = (! nav_usable);

  //if (nav_usable)
    ImGui::SetNextWindowFocus ();

  toggle_ui  = SK_ImGui_Active ();
  toggle_nav = nav_usable;

  if (SK_ImGui_Active () && nav_usable)
  {
    SK_Input_RememberPressedKeys ();

    haptic_events.PulseTitle.start = static_cast <float> (timeGetTime ());
    haptic_events.PulseTitle.end   = haptic_events.PulseTitle.start +
                                       haptic_events.PulseTitle.duration;
  }

  //ImGui::GetIO ().NavActive = nav_usable;

  // Zero-out any residual haptic data
  if (! SK_ImGui_Active ())
  {
    SK_XInput_ZeroHaptics (config.input.gamepad.steam.ui_slot);
    SK_XInput_ZeroHaptics (config.input.gamepad.xinput.ui_slot);
  }

  return SK_ImGui_Active ();
}

#include <SpecialK/input/dinput8_backend.h>
#include <SpecialK/input/steam.h>

extern IDirectInputDevice8W_GetDeviceState_pfn
        IDirectInputDevice8W_GetDeviceState_Original;

extern XINPUT_STATE  di8_to_xi;
extern XINPUT_STATE  joy_to_xi;

void
SK_ImGui_PollGamepad_EndFrame (void)
{
  ImGuiIO& io (ImGui::GetIO ());

  extern bool __stdcall SK_IsGameWindowActive (void);

  // Reset Mouse / Keyboard State so that we can process all state transitions
  //   that occur during the next frame without losing any input events.
  if ( SK_IsGameWindowActive () /*&& GetFocus        () == game_window.hWnd &&
                                   GetActiveWindow () == game_window.hWnd */)
  {
    static LONG  last_key_msg_cnt = 0;

    io.MouseDown [0] = (SK_GetAsyncKeyState (VK_LBUTTON)  & 0x8000) != 0;
    io.MouseDown [1] = (SK_GetAsyncKeyState (VK_RBUTTON)  & 0x8000) != 0;
    io.MouseDown [2] = (SK_GetAsyncKeyState (VK_MBUTTON)  & 0x8000) != 0;
    io.MouseDown [3] = (SK_GetAsyncKeyState (VK_XBUTTON1) & 0x8000) != 0;
    io.MouseDown [4] = (SK_GetAsyncKeyState (VK_XBUTTON2) & 0x8000) != 0;

    if ( io.MouseDown [0] ||
         io.MouseDown [1]  ) SK_ImGui_Cursor.update ();

    if (last_key_msg_cnt != ReadAcquire (&__SK_KeyMessageCount))
    {
      last_key_msg_cnt =
        ReadAcquire (&__SK_KeyMessageCount);

      // This stupid hack prevents the Steam overlay from making the software
      //   think tab is stuck down.
      io.KeysDown [0x08]  = (SK_GetAsyncKeyState ( 0x08 ) & 0x8000) != 0;
      io.KeysDown [0x09]  = (SK_GetAsyncKeyState ( 0x09 ) & 0x8000) != 0;
      io.KeysDown [0x0C]  = (SK_GetAsyncKeyState ( 0x0C ) & 0x8000) != 0;
      io.KeysDown [0x0D]  = (SK_GetAsyncKeyState ( 0x0D ) & 0x8000) != 0;
      for (int i = 0x10; i < 0x16; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0x17; i < 0x1A; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0x1B; i < 0x3A; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0x41; i < 0x5E; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0x5F; i < 0x88; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0x90; i < 0x97; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0xA0; i < 0xB8; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0xBA; i < 0xC1; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0xDB; i < 0xE0; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0xE1; i < 0xE8; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
      for (int i = 0xE9; i < 0xFF; i++)
        io.KeysDown [i]  = (SK_GetAsyncKeyState (  i ) & 0x8000) != 0;
    }

    // Don't cycle window elements when Alt+Tabbing
    if (io.KeyAlt) io.KeysDown [VK_TAB] = false;

    if (config.input.keyboard.catch_alt_f4)
    {
      if ( io.KeyAlt && io.KeysDown [VK_F4] && ( ( io.KeysDownDuration [VK_MENU] > 0 ) ^
              ( io.KeysDownDuration [VK_F4] > 0 ) ))
      {
        extern bool SK_ImGui_WantExit;
                    SK_ImGui_WantExit = true;
      }
    }
  }

  else
  {
    SecureZeroMemory (io.KeysDown,  sizeof (bool) * 512);
    SecureZeroMemory (io.MouseDown, sizeof (bool) * 5);
  }

         XINPUT_STATE state      = {      };
  static XINPUT_STATE last_state = { 1, 0 };

  bool api_bridge =
    config.input.gamepad.native_ps4 || ( ControllerPresent (config.input.gamepad.steam.ui_slot) );

  if (api_bridge)
  {
    // Translate DirectInput to XInput, because I'm not writing multiple controller codepaths
    //   for no good reason.
    JOYINFOEX joy_ex   { };
    JOYCAPSW  joy_caps { };

    joy_ex.dwSize  = sizeof JOYINFOEX;
    joy_ex.dwFlags = JOY_RETURNALL      | JOY_RETURNPOVCTS |
                     JOY_RETURNCENTERED | JOY_USEDEADZONE;

    joyGetPosEx    (JOYSTICKID1, &joy_ex);
    joyGetDevCapsW (JOYSTICKID1, &joy_caps, sizeof JOYCAPSW);

    SK_JOY_TranslateToXInput (&joy_ex, &joy_caps);
  }

#if 1
  state = joy_to_xi;
#else
  state = di8_to_xi;
#endif

  if (ControllerPresent (config.input.gamepad.steam.ui_slot))
  {
    state =
      *steam_input [config.input.gamepad.steam.ui_slot].to_xi;
  }

  if ( api_bridge ||
       SK_XInput_PollController (config.input.gamepad.xinput.ui_slot, &state) )
  {
    if ( state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK  &&
         state.Gamepad.wButtons & XINPUT_GAMEPAD_START &&
         last_state.dwPacketNumber <= state.dwPacketNumber )
    {
      if (! ( last_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK &&
              last_state.Gamepad.wButtons & XINPUT_GAMEPAD_START ) )
      {
        bool toggle = true,
             nav    = (! nav_usable);

        // Additional condition for Final Fantasy X so as not to interfere with soft reset
        if (! ( state.Gamepad.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
                state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) )
        {
          SK_ImGui_ToggleEx (toggle, nav);
        }
      }
    }

     const DWORD LONG_PRESS  = 400UL;
    static DWORD dwLastPress = MAXDWORD;

    if (last_state.dwPacketNumber <= state.dwPacketNumber)
    {
      if ( (     state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) &&
           (last_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) )
      {
        if (dwLastPress < timeGetTime () - LONG_PRESS)
        {
          bool toggle_vis = (! SK_ImGui_Active ());
          bool toggle_nav =    true;

          if (SK_ImGui_Active ())
            SK_ImGui_ToggleEx (toggle_vis, toggle_nav);

          dwLastPress = MAXDWORD;
        }
      }

      else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
        dwLastPress = timeGetTime ();

      else
        dwLastPress = MAXDWORD;
    }
  }

  else
    SecureZeroMemory (&state.Gamepad, sizeof XINPUT_GAMEPAD);


  if (SK_ImGui_Active () && config.input.gamepad.haptic_ui)
  {
    ImGuiContext& g =
      *GImGui;

    static ImGuiID nav_id = 0;

    if (g.NavId != nav_id)
    {
      if (haptic_events.PulseNav.end > static_cast <float> (timeGetTime ()))
      {
        haptic_events.PulseNav.end   += haptic_events.PulseNav.duration;
        haptic_events.PulseNav.start += haptic_events.PulseNav.duration;
      }

      else
      {
        haptic_events.PulseNav.start = static_cast <float> (timeGetTime ());
        haptic_events.PulseNav.end   = haptic_events.PulseNav.start +
                                         haptic_events.PulseNav.duration;
      }
    }

    if (g.ActiveIdIsJustActivated)
    {
      haptic_events.PulseButton.start = static_cast <float> (timeGetTime ());
      haptic_events.PulseButton.end   = haptic_events.PulseButton.start +
                                          haptic_events.PulseButton.duration;
    }

    if (! ControllerPresent (config.input.gamepad.steam.ui_slot))
    {
      SK_XInput_PulseController ( config.input.gamepad.xinput.ui_slot,
                                    haptic_events.PulseTitle.run  () +
                                    haptic_events.PulseButton.run () +
                    std::min (0.4f, haptic_events.PulseNav.run ()),
                                      haptic_events.PulseTitle.run  () +
                                      haptic_events.PulseButton.run () +
                      std::min (0.4f, haptic_events.PulseNav.run    ()) );
    }

    else
    {
      SK_XInput_PulseController ( config.input.gamepad.steam.ui_slot,
                                    haptic_events.PulseTitle.run  () +
                                    haptic_events.PulseButton.run () +
                    std::min (0.4f, haptic_events.PulseNav.run ()),
                                      haptic_events.PulseTitle.run  () +
                                      haptic_events.PulseButton.run () +
                      std::min (0.4f, haptic_events.PulseNav.run    ()) );
    }

    nav_id = g.NavId;
  }

  last_state = state;
}


#include <SpecialK/core.h>
#include <SpecialK/widgets/widget.h>

#define SK_Threshold(x,y) (x) > (y) ? ( (x) - (y) ) : 0

void
SK_ImGui_PollGamepad (void)
{
  ImGuiIO& io =
    ImGui::GetIO ();

         XINPUT_STATE state    = {      };
  static XINPUT_STATE last_state { 1, 0 };

  for ( float& NavInput : io.NavInputs )
    NavInput = 0.0f;

  bool api_bridge =
    config.input.gamepad.native_ps4 || ControllerPresent (config.input.gamepad.steam.ui_slot);

#if 1
  state = joy_to_xi;
#else
  state = di8_to_xi;
#endif

  if (ControllerPresent (config.input.gamepad.steam.ui_slot))
  {
    state =
      *steam_input [config.input.gamepad.steam.ui_slot].to_xi;
  }

  if ( ( api_bridge ||
         SK_XInput_PollController ( config.input.gamepad.xinput.ui_slot,
                                      &state
                                  )
       )                                &&
       last_state.dwPacketNumber <= state.dwPacketNumber
     )
  {
    last_state = state;

    if (nav_usable)
    {
      float LX   = state.Gamepad.sThumbLX;
      float LY   = state.Gamepad.sThumbLY;

      float norm = sqrt ( LX*LX + LY*LY );

      //float nLX  = LX / norm;
      //float nLY  = LY / norm;

      float unit = 1.0f;

      if (norm > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
      {
        norm = std::min (norm, 32767.0f) - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
        unit =           norm/(32767.0f  - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
      }

      else
      {
        norm = 0.0f;
        unit = 0.0f;
      }

      float uLX = (LX / 32767.0f) * unit;
      float uLY = (LY / 32767.0f) * unit;

      // Press Button, Tweak Value                    // e.g. Circle button
      io.NavInputs [ImGuiNavInput_PadActivate] +=
        static_cast <float> ((state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);

      // Close Menu/PopUp/Child, Clear Selection      // e.g. Cross button
      io.NavInputs [ImGuiNavInput_PadCancel]   +=
        static_cast <float> ((state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);

      // Text Input                                   // e.g. Triangle button
      io.NavInputs [ImGuiNavInput_PadInput]    +=
        static_cast <float> ((state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);

      // Access Menu, Focus, Move, Resize             // e.g. Square button
      io.NavInputs [ImGuiNavInput_PadMenu]     +=
        static_cast <float> ((state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);

      //
      // [ANALOG INPUTS]
      //
      // Move Up, Resize Window (with PadMenu held)   // e.g. D-pad up/down/left/right
      io.NavInputs [ImGuiNavInput_PadUp]    +=  0.001f *
        static_cast <float> (
          (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)    != 0
        );

      // Move Down
      io.NavInputs [ImGuiNavInput_PadDown]  +=  0.001f *
        static_cast <float> (
          (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  != 0
        );

      // Move Left
      io.NavInputs [ImGuiNavInput_PadLeft]  +=  0.001f *
        static_cast <float> (
          (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)  != 0
        );

      // Move Right
      io.NavInputs [ImGuiNavInput_PadRight] +=  0.001f *
        static_cast <float> (
          (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0
        );


      io.NavInputs [ImGuiNavInput_PadScrollDown]    = 0.0f;
      io.NavInputs [ImGuiNavInput_PadScrollUp]      = 0.0f;

      if (uLY > 0.0f)
        io.NavInputs [ImGuiNavInput_PadScrollUp]   += uLY / analog_sensitivity;

      else if (uLY < 0.0f)
        io.NavInputs [ImGuiNavInput_PadScrollDown] -= uLY / analog_sensitivity;


      io.NavInputs [ImGuiNavInput_PadScrollLeft]    = 0.0f;
      io.NavInputs [ImGuiNavInput_PadScrollRight]   = 0.0f;

      if (uLX > 0.0f)
        io.NavInputs [ImGuiNavInput_PadScrollRight] += uLX / analog_sensitivity;

      else if (uLX < 0.0f)
        io.NavInputs [ImGuiNavInput_PadScrollLeft]  -= uLX / analog_sensitivity;


      // Next Window (with PadMenu held)              // e.g. L-trigger
      io.NavInputs [ImGuiNavInput_PadFocusPrev]   +=
        static_cast <float> (
          (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0
        );

      // Prev Window (with PadMenu held)              // e.g. R-trigger
      io.NavInputs [ImGuiNavInput_PadFocusNext]   +=
        static_cast <float> (
          (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0
        );

      io.NavInputs [ImGuiNavInput_PadTweakSlow] +=
        static_cast <float> (
          SK_Threshold ( state.Gamepad.bLeftTrigger,
                         XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ) /
              ( 255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD     );

      io.NavInputs [ImGuiNavInput_PadTweakFast] +=
        static_cast <float> (
          SK_Threshold ( state.Gamepad.bRightTrigger,
                         XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ) /
              ( 255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD     );
    }
  }


  if ( io.KeysDown         [VK_CAPITAL] &&
       io.KeysDownDuration [VK_CAPITAL] == 0.0f )
  {
    bool visible = false,
         nav     = true;

    SK_ImGui_ToggleEx (visible, nav);
  }


  if (! nav_usable)
    io.NavActive = false;

  //
  // Same basic idea as above, only for keyboard
  //
  //   The primary difference between gamepad and keyboard is the lack of a left
  //     analog stick to differentiate D-Pad behavior from analog stick movement.
  //
  //   For keyboard, we alias both to the same task and also consider WASD to be
  //     identical to Up/Down/Left/Right.
  //
  if (io.NavUsable && nav_usable)
  {
    io.NavInputs [ImGuiNavInput_PadMenu] += (io.KeyCtrl) ? 1.0f : 0.0f;

    if (! io.WantTextInput)
    {
      const bool up    = ( io.KeysDown ['W'] || io.KeysDown [VK_UP   ] );
      const bool down  = ( io.KeysDown ['S'] || io.KeysDown [VK_DOWN ] );
      const bool left  = ( io.KeysDown ['A'] || io.KeysDown [VK_LEFT ] );
      const bool right = ( io.KeysDown ['D'] || io.KeysDown [VK_RIGHT] );

      const float inv_analog     =
          ( 1.0f / analog_sensitivity );

      const float analog_epsilon = 0.001f;

      io.NavInputs [ImGuiNavInput_PadScrollUp]    +=
        (io.KeysDown [VK_PRIOR] ? inv_analog : 0.0f);
      io.NavInputs [ImGuiNavInput_PadScrollDown]  +=
        (io.KeysDown [VK_NEXT ] ? inv_analog : 0.0f);
      io.NavInputs [ImGuiNavInput_PadScrollLeft]  +=
        (io.KeysDown [VK_HOME ] ? inv_analog : 0.0f);
      io.NavInputs [ImGuiNavInput_PadScrollRight] +=
        (io.KeysDown [VK_END  ] ? inv_analog : 0.0f);

      io.NavInputs [ImGuiNavInput_PadScrollUp]    +=
        (up    ? inv_analog : 0.0f);
      io.NavInputs [ImGuiNavInput_PadScrollDown]  +=
        (down  ? inv_analog : 0.0f);
      io.NavInputs [ImGuiNavInput_PadScrollLeft]  +=
        (left  ? inv_analog : 0.0f);
      io.NavInputs [ImGuiNavInput_PadScrollRight] +=
        (right ? inv_analog : 0.0f);

      io.NavInputs [ImGuiNavInput_PadUp]          +=
        (up    ? analog_epsilon : 0.0f);
      io.NavInputs [ImGuiNavInput_PadDown]        +=
        (down  ? analog_epsilon : 0.0f);
      io.NavInputs [ImGuiNavInput_PadLeft]        +=
        (left  ? analog_epsilon : 0.0f);
      io.NavInputs [ImGuiNavInput_PadRight]       +=
        (right ? analog_epsilon : 0.0f);
    }

    io.NavInputs  [ImGuiNavInput_PadActivate]     +=
      io.KeysDown [VK_RETURN]  ?  1.0f : 0.0f;
    io.NavInputs  [ImGuiNavInput_PadCancel]       +=
      io.KeysDown [VK_ESCAPE]  ?  1.0f : 0.0f;
  }

  else
    io.NavActive = false;

  io.NavInputs [ImGuiNavInput_PadFocusPrev] +=
    (io.KeyCtrl && io.KeyShift && io.KeysDown [VK_TAB] &&
                          io.KeysDownDuration [VK_TAB] == 0.0f)  ? 1.0f : 0.0f;

  io.NavInputs [ImGuiNavInput_PadFocusNext] +=
    (io.KeyCtrl                && io.KeysDown [VK_TAB] &&
                          io.KeysDownDuration [VK_TAB] == 0.0f)  ? 1.0f : 0.0f;



  if (io.NavInputs [ImGuiNavInput_PadActivate] != 0.0f)
    io.MouseDown [4] = true;
  else
    io.MouseDown [4] = false;

  static DWORD last_toggle = 0UL;

  if ( ( io.NavInputs             [ImGuiNavInput_PadTweakSlow] != 0.0f &&
         io.NavInputs             [ImGuiNavInput_PadTweakFast] != 0.0f )   &&
       ( io.NavInputsDownDuration [ImGuiNavInput_PadTweakSlow] == 0.0f ||
         io.NavInputsDownDuration [ImGuiNavInput_PadTweakFast] == 0.0f )      )
  {
    if (last_toggle < SK_GetFramesDrawn () - 1)
    {
      SK_ImGui_Widgets.hide_all = (! SK_ImGui_Widgets.hide_all);
      last_toggle               =    SK_GetFramesDrawn ();
    }
  }
}





void
ImGui::PlotCEx ( ImGuiPlotType,                               const char* label,
                 float (*values_getter)(void* data, int idx),       void* data,
                 int     values_count,  int   values_offset,  const char* overlay_text,
                 float   scale_min,     float scale_max,           ImVec2 graph_size,
                 float   min_color,     float max_color,   float,   bool  inverse )
{
  ImGuiWindow* window =
    GetCurrentWindow ();

  if (window->SkipItems)
    return;

  ImGuiContext&     g     = *GImGui;
  const ImGuiStyle& style = g.Style;

  const ImVec2 label_size =
    CalcTextSize (label, nullptr, true);

  if (graph_size.x == 0.0f)
      graph_size.x = CalcItemWidth ();
  if (graph_size.y == 0.0f)
      graph_size.y = label_size.y + (style.FramePadding.y * 2.0f);

  const ImRect frame_bb ( window->DC.CursorPos,
                          window->DC.CursorPos + ImVec2 ( graph_size.x,
                                                          graph_size.y ) );
  const ImRect inner_bb ( frame_bb.Min + style.FramePadding,
                          frame_bb.Max - style.FramePadding );
  const ImRect total_bb ( frame_bb.Min,
                          frame_bb.Max + ImVec2 ( ( (label_size.x > 0.0f)
                                                      ?
                                        style.ItemInnerSpacing.x + label_size.x
                                                      :
                                                     0.0f ),
                                                  0.0f
                                                )
                        );

  ItemSize (total_bb, style.FramePadding.y);

  if (! ItemAdd (total_bb, nullptr, &frame_bb))
    return;

  // Determine scale from values if not specified
  if ( scale_min == std::numeric_limits <float>::max () ||
       scale_max == std::numeric_limits <float>::max () )
  {
    float v_min = std::numeric_limits <float>::max ();
    float v_max = std::numeric_limits <float>::min ();

    for (int i = 0; i < values_count; i++)
    {
      const float v =
        values_getter (data, i);

      v_min = ImMin (v_min, v);
      v_max = ImMax (v_max, v);
    }

    if (scale_min == std::numeric_limits <float>::max ())
        scale_min = v_min;
    if (scale_max == std::numeric_limits <float>::max ())
        scale_max = v_max;
  }

  RenderFrame ( frame_bb.Min,
                frame_bb.Max,
                  GetColorU32 (ImGuiCol_FrameBg),
                    true,
                      style.FrameRounding );

  if (values_count > 0)
  {
    int    res_w      = ImMin ( static_cast <int> ( graph_size.x ),
                                                    values_count ) - 1;
    int    item_count = values_count - 1;

    const float
           t_step     = 1.0f / static_cast <float> (res_w);

    float  t0         = 0.0f;
    float  v0         = values_getter (
                          data,
                            (0 + values_offset) % values_count
                        );

    // Point in the normalized space of our target rectangle
    ImVec2 tp0 ( t0, 1.0f - ImSaturate ( (v0        - scale_min) /
                                         (scale_max - scale_min) ) );

    for (int n = 0; n < res_w; n++)
    {
      const float  t1      = t0 + t_step;

      const auto   v1_idx  = static_cast <int> (t0 * item_count + 0.5f);
      IM_ASSERT(   v1_idx >= 0 && v1_idx < values_count);

      const float  v1      =
        values_getter (data, (v1_idx + values_offset + 1) % values_count);

      const ImVec2 tp1 ( t1, 1.0f - ImSaturate ( (v1        - scale_min) /
                                                 (scale_max - scale_min) ) );

      auto _ComputeColor = [&](float v) -> float
      {
        float color =
          ImSaturate ( ( v         - min_color ) /
                       ( max_color - min_color ) );

        return inverse ? 1.0f - color : color;
      };

      const ImU32 col_base =
        ImColor::HSV (
          0.31f - 0.31f * ImLerp ( _ComputeColor (v0),
                                   _ComputeColor (v1),
                                                  tp1.y ),
          0.86f,
          0.95f
        );

      // NB: Draw calls are merged together by the DrawList system. Still,
      //     we should render our batch are lower level to save a bit of CPU.
      ImVec2 pos0 = ImLerp (inner_bb.Min, inner_bb.Max, tp0);
      ImVec2 pos1 = ImLerp (inner_bb.Min, inner_bb.Max, tp1);

      window->DrawList->AddLine (pos0, pos1, col_base);

      v0  = v1;
      t0  = t1;
      tp0 = tp1;
    }
  }

  // Text overlay
  if (overlay_text)
  {
    RenderTextClipped ( ImVec2 ( frame_bb.Min.x,
                                 frame_bb.Min.y + style.FramePadding.y ),
                          frame_bb.Max,
                            overlay_text,
                              nullptr, nullptr,
                                ImVec2 (0.5f, 0.0f) );
  }

  if (label_size.x > 0.0f)
  {
    RenderText ( ImVec2 ( frame_bb.Max.x + style.ItemInnerSpacing.x,
                          inner_bb.Min.y ),
                   label );
  }
}

// Note-to-Future-Self:
// --------------------
//  Dear Dumbass,
//
//   With this many arguments, stop passing them on the stack and use a structure.
//
void
ImGui::PlotLinesC ( const char*  label,         const float* values,
                          int    values_count,        int    values_offset,
                    const char*  overlay_text,        float  scale_min,
                                                      float  scale_max,
                          ImVec2 graph_size,          int    stride,
                          float  min_color_val,       float  max_color_val,
                          float  avg,                 bool   inverse )
{
  ImGuiPlotArrayGetterData data =
    ImGuiPlotArrayGetterData (values, stride);

  PlotCEx ( ImGuiPlotType_Lines, label,        &Plot_ArrayGetter,
              reinterpret_cast <void *>(&data), values_count,
                                                values_offset,
                overlay_text,   scale_min, scale_max,
                  graph_size,
                    min_color_val, max_color_val,
                      avg,
                        inverse
          );
}


#include <imgui/imgui_internal.h>
#include <SpecialK/control_panel.h>

#include <SpecialK/render/dxgi/dxgi_backend.h>

void
SK_ImGui_User_NewFrame (void)
{
  if ( ImGui::GetIO ().DisplaySize.x <= 0.0f ||
       ImGui::GetIO ().DisplaySize.y <= 0.0f )
  {
    ImGui::GetIO ().DisplaySize.x = 128.0f;
    ImGui::GetIO ().DisplaySize.y = 128.0f;
  }

  ImGuiContext& g = *GImGui;

  g.Style.AntiAliasedLines  = config.imgui.render.antialias_lines;
  g.Style.AntiAliasedShapes = config.imgui.render.antialias_contours;

  ImGui::NewFrame ();


  //
  // Idle Cursor Detection  (when UI is visible, but mouse does not require capture)
  //
  //          Remove the cursor after a brief timeout period (500 ms), it will come back if moved ;)
  //
  static int last_x, last_y;

  if (last_x != SK_ImGui_Cursor.pos.x || last_y != SK_ImGui_Cursor.pos.y || SK_ImGui_WantMouseCaptureEx (0x0))
    SK_ImGui_Cursor.last_move = SK::ControlPanel::current_time;

  last_x = SK_ImGui_Cursor.pos.x; last_y = SK_ImGui_Cursor.pos.y;

  bool was_idle = SK_ImGui_Cursor.idle;

  if (SK_ImGui_IsMouseRelevant () && SK_ImGui_Cursor.last_move < SK::ControlPanel::current_time - 500)
    SK_ImGui_Cursor.idle = true;

  else
    SK_ImGui_Cursor.idle = false;

  if (was_idle != SK_ImGui_Cursor.idle)
    SK_ImGui_Cursor.update ();


  if (eula.show)
    SK_ImGui_DrawEULA (&eula);
}