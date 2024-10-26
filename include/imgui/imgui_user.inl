#pragma once

#include <mmsystem.h>
#include <Windows.h>

#include <hidclass.h>
#include <SetupAPI.h>
#include <Cfgmgr32.h>

volatile LONG
  __SK_KeyMessageCount = 0;

#include <SpecialK/utility.h>
#include <SpecialK/config.h>
#include <SpecialK/thread.h>
#include <SpecialK/tls.h>

#include <SpecialK/console.h>
#include <SpecialK/window.h>

#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>
#include <SpecialK/input/sce_pad.h>
#include <SpecialK/input/xinput_hotplug.h>

#include <algorithm>


#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <SpecialK/control_panel.h>

show_eula_s eula;

const ImWchar*
SK_ImGui_GetGlyphRangesDefaultEx (void)
{
  static const ImWchar ranges [] =
  {
    0x0020, 0x00FF, // Basic Latin + Latin Supplement
    0x0100, 0x03FF, // Latin, IPA, Greek
    0x2000, 0x206F, // General Punctuation
    0x2070, 0x209F, // Superscripts and Subscripts
    0x2100, 0x21FF, // Letterlike Symbols
    0x2600, 0x26FF, // Misc. Characters
    0x2700, 0x27BF, // Dingbats
    0xc2b1, 0xc2b3, // ²
    0
  };
  return &ranges [0];
}

#include <../imgui/fa_brands_400.ttf.h>
#include <../imgui/fa_solid_900.ttf.h>

#include <../imgui/font_awesome.h>

const ImWchar*
SK_ImGui_GetGlyphRangesFontAwesome (void)
{
  static const ImWchar ranges [] =
  {
    ICON_MIN_FA, ICON_MAX_FA,
    0
  };
  return &ranges [0];
}

SK_LazyGlobal <SK_Thread_HybridSpinlock> font_lock;

#include              <filesystem>
#include              <fstream>
namespace sk_fs = std::filesystem;

ImFont* __SK_ImGui_FontConsolas;
ImFont*
SK_ImGui_GetFont_Consolas (void)
{
  return __SK_ImGui_FontConsolas;
}

void
SK_ImGui_LoadFonts (void)
{
  static volatile LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    static auto& io =
      ImGui::GetIO ();

    ImFontConfig font_cfg = { };
    font_cfg.MergeMode    = true;

    auto LoadFont = [](const std::string& filename, float point_size,
                       const ImWchar* glyph_range,  ImFontConfig* cfg = nullptr)
    {
      char szFullPath [ MAX_PATH + 2 ] = { };

      if (GetFileAttributesA (            filename.c_str ()) != INVALID_FILE_ATTRIBUTES)
         strncpy_s (szFullPath, MAX_PATH, filename.c_str (),
                               _TRUNCATE);

      else
      {
        snprintf (szFullPath, MAX_PATH, R"(%ws\%s)", SK_GetFontsDir ().c_str (), filename.c_str ());


        if (GetFileAttributesA (szFullPath) == INVALID_FILE_ATTRIBUTES)
          *szFullPath = '\0';
      }

      if (*szFullPath != '\0')
      {
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

    LoadFont (config.imgui.font.japanese.file,  config.imgui.font.japanese.size, io.Fonts->GetGlyphRangesJapanese                (), &font_cfg);
    if (config.apis.last_known != SK_RenderAPI::D3D9)
      LoadFont (config.imgui.font.chinese.file,   config.imgui.font.chinese.size,  io.Fonts->GetGlyphRangesChineseSimplifiedCommon (), &font_cfg);
    //LoadFont (config.imgui.font.korean.file,    config.imgui.font.korean.size,   io.Fonts->GetGlyphRangesKorean                (), &font_cfg);
    LoadFont (config.imgui.font.cyrillic.file,  config.imgui.font.cyrillic.size, io.Fonts->GetGlyphRangesCyrillic                (), &font_cfg);

    sk_fs::path fontDir =
      sk_fs::path (SK_GetInstallPath ()) / L"Fonts";

    std::error_code ec = { };

    if (! sk_fs::exists (            fontDir, ec))
          sk_fs::create_directories (fontDir, ec);

    static auto
      sk_fs_wb = ( std::ios_base::binary
                 | std::ios_base::out  );

    auto _UnpackFontIfNeeded =
    [&]( const char*   szFont,
         const uint8_t akData [],
         const size_t  cbSize )
    {
      if (! sk_fs::is_regular_file ( fontDir / szFont, ec )     )
                     std::ofstream ( fontDir / szFont, sk_fs_wb ).
        write ( reinterpret_cast <const char *> (akData),
                                                 cbSize);
    };

    auto      awesome_fonts = {
      std::make_tuple (
        FONT_ICON_FILE_NAME_FAS, fa_solid_900_ttf,
                     _ARRAYSIZE (fa_solid_900_ttf) ),
      std::make_tuple (
        FONT_ICON_FILE_NAME_FAB, fa_brands_400_ttf,
                     _ARRAYSIZE (fa_brands_400_ttf) )
                              };

    std::for_each (
              awesome_fonts.begin (),
              awesome_fonts.end   (),
      [&](const auto& font)
      {        _UnpackFontIfNeeded (
        std::get <0> (font),
        std::get <1> (font),
        std::get <2> (font)        );
         LoadFont (SK_WideCharToUTF8 (
                      fontDir/
        std::get <0> (font)).c_str (),
         config.imgui.font.default_font.size,
          SK_ImGui_GetGlyphRangesFontAwesome (),
                     &font_cfg);
      }
    );

    io.Fonts->AddFontDefault ();

    font_cfg           = {   };
    font_cfg.MergeMode = false;

    __SK_ImGui_FontConsolas =
      LoadFont ("Consolab.ttf", 18, SK_ImGui_GetGlyphRangesDefaultEx (), &font_cfg);

    InterlockedIncrementRelease (&init);
  }

  SK_Thread_SpinUntilAtomicMin (&init, 2);
}

#include <windowsx.h>
#include <SpecialK/hooks.h>

float analog_sensitivity = 0.00333f;
bool  nav_usable         = false;

#include <set>
#include <SpecialK/log.h>

#define SK_RAWINPUT_READ(type)  SK_RawInput_Backend->markRead   (type);
#define SK_RAWINPUT_WRITE(type) SK_RawInput_Backend->markWrite  (type);
#define SK_RAWINPUT_VIEW(type)  SK_RawInput_Backend->markViewed (type);
#define SK_RAWINPUT_HIDE(type)  SK_RawInput_Backend->markHidden (type);

SK_LazyGlobal <SK_Thread_HybridSpinlock> raw_input_lock;

INT
WINAPI
SK_ImGui_ProcessRawInput ( _In_      HRAWINPUT hRawInput,
                           _In_      UINT      uiCommand,
                           _Out_opt_ LPVOID    pData,
                           _Inout_   PUINT     pcbSize,
                           _In_      UINT      cbSizeHeader,
                                     BOOL      self,
                                     INT       precache_size = 0 )
{
  if (! hRawInput)
  {
    SetLastError (ERROR_INVALID_HANDLE);
    return ~0U;
  }

  if (cbSizeHeader != sizeof (RAWINPUTHEADER) || pcbSize == nullptr)
  {
    SetLastError (ERROR_INVALID_PARAMETER);
    return ~0U;
  }

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
  {
    SetLastError (ERROR_OUTOFMEMORY);
    return ~0U;
  }

  auto pRawCtx =
    pTLS->raw_input.getPtr ();

  if (pRawCtx == nullptr)
  {
    SetLastError (ERROR_OUTOFMEMORY);
    return ~0U;
  }

  UINT size       = 0U;
  bool from_cache = false;

  if ( pRawCtx->cached_input.hRawInput == hRawInput )
  {
    SK_LOGs1 (     L" RawInput ",
      L"Cache Hit for RawInput Handle %p (on tid=%x)",
                     hRawInput, GetCurrentThreadId () );

    switch (uiCommand)
    {
      case RID_INPUT:
        size =
          pRawCtx->cached_input.Data->header.dwSize;
        break;

      case RID_HEADER:
        size = sizeof (RAWINPUTHEADER);
        break;

      default:
        SetLastError (ERROR_INVALID_PARAMETER);
        return ~0U; 
    }

    if (pData == nullptr)
    {
      *pcbSize = size;
      return 0;
    }

    if (*pcbSize < size)
    {
      SetLastError (ERROR_INSUFFICIENT_BUFFER);
      return ~0U;
    }

    memcpy (pData, pRawCtx->cached_input.Data, size);

    from_cache = true;
  }

  if (pData == nullptr)
  {
    return
      SK_GetRawInputData ( hRawInput, uiCommand,
                   pData, pcbSize,
                           cbSizeHeader );
  }

  static auto& io =
    ImGui::GetIO ();

  static auto* pConsole =
    SK_Console::getInstance ();

  bool focus             = SK_IsGameWindowActive (    );
  bool already_processed =
    ( pRawCtx->last_input == hRawInput &&
                uiCommand == RID_INPUT );

  pRawCtx->last_input = (
    ( (! self) && uiCommand == RID_INPUT ) ?
                     hRawInput : pRawCtx->last_input );

  if (! from_cache)
  {
    pRawCtx->cached_input.Size =
      sizeof (pRawCtx->cached_input.Data);

    size =
      SK_GetRawInputData ( hRawInput, RID_INPUT,
           pRawCtx->cached_input.Data,
          &pRawCtx->cached_input.Size, cbSizeHeader );

    if (size != ~0U)
    {
      pRawCtx->cached_input.hRawInput  = hRawInput;
      pRawCtx->cached_input.SizeHeader = sizeof (RAWINPUTHEADER);
      pRawCtx->cached_input.uiCommand  = RID_INPUT;

      if (uiCommand == RID_HEADER)
        size = sizeof (RAWINPUTHEADER);

      if (pData != nullptr)
      {
        if (        *pcbSize >= size)
          memcpy (pData, pRawCtx->cached_input.Data, size);
        else
        {
          SetLastError (ERROR_INSUFFICIENT_BUFFER);
          return ~0U;
        }
      }

      SK_LOGs1 (      L" RawInput ",
        L"Cache Miss for RawInput Handle %p (on tid=%x)",
                        hRawInput, GetCurrentThreadId () );
    }
  }

  // On error, simply return immediately...
  if (size == ~0U)
    return size;

  // Input event happened while the window had focus if true, otherwise another
  //   window is currently capturing input and the most appropriate response is
  //     usually to ignore the event.
  bool foreground =
    GET_RAWINPUT_CODE_WPARAM (((RAWINPUT *)pData)->header.wParam) == RIM_INPUT;

  bool filter   = false;
  bool mouse    = false;
  bool keyboard = false;
  bool gamepad  = false;

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
            if (SK_WantBackgroundRender () && (! focus))
              filter = true;

            mouse = true;

            if ( (! already_processed)
                           && uiCommand == RID_INPUT )
            {
              if (! filter)
              {
                SK_RAWINPUT_READ (sk_input_dev_type::Mouse)
                SK_RAWINPUT_VIEW (sk_input_dev_type::Mouse)
              }
              else SK_RAWINPUT_HIDE (sk_input_dev_type::Mouse)
            }
          } break;


          case RIM_TYPEKEYBOARD:
          {
            InterlockedIncrement (&__SK_KeyMessageCount);

            USHORT VKey =
              (((RAWINPUT *)pData)->data.keyboard.VKey & 0xFF);


            // Only filter keydown message, not key releases
            if (SK_ImGui_WantKeyboardCapture ())
            {
              if (VKey & 0xF8) // Valid Keys:  8 - 255
                filter = true;
            }


            if (SK_ImGui_IsMouseRelevant ())
            {
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
            if (pConsole->isVisible () && (VKey & 0xFF) > 7)
              filter = true;


            // Block keyboard input to the game while it's in the background
            if (SK_WantBackgroundRender () && (! focus))
              filter = true;


            if (VKey & 0xF8) // Valid Keys:  8 - 255
              keyboard = true;

            if (keyboard && ( SK_ImGui_WantKeyboardCapture () ||
                                (((RAWINPUT *)pData)->data.keyboard.Message == WM_CHAR ||
                                 ((RAWINPUT *)pData)->data.keyboard.Message == WM_SYSCHAR) ))
              filter = true;


            if ( (! already_processed)
                           && uiCommand == RID_INPUT )
            {
              if (! filter)
              {
                SK_RAWINPUT_READ (sk_input_dev_type::Keyboard)
                SK_RAWINPUT_VIEW (sk_input_dev_type::Keyboard)
              }
              else SK_RAWINPUT_HIDE (sk_input_dev_type::Keyboard)
            }

        //// Leads to double-input processing, left here in case Legacy Messages are disabled and this is needed
        ////
#if 0
        ////if (!(((RAWINPUT *) pData)->data.keyboard.Flags & RI_KEY_BREAK))
        ////{
        ////  pConsole->KeyDown (VKey & 0xFF, MAXDWORD);
        ////        io.KeysDown [VKey & 0xFF] = focus;
        ////}
            if (game_window.active)
            {
              switch (((RAWINPUT *) pData)->data.keyboard.Message)
              {
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                        io.KeysDown [VKey & 0xFF] = focus;
                  pConsole->KeyDown (VKey & 0xFF, MAXDWORD);
                  break;

                case WM_KEYUP:
                case WM_SYSKEYUP:
                      io.KeysDown [VKey & 0xFF] = false;
                  pConsole->KeyUp (VKey & 0xFF, MAXDWORD);
                  break;
              }
            }
#endif
          } break;


          default:
          {
            gamepad = true;

            // TODO: Determine which controller the input is from
            if (SK_ImGui_WantGamepadCapture () || config.input.gamepad.disable_hid)
              filter = true;

            if ( (! already_processed)
                           && uiCommand == RID_INPUT )
            {
              if (! filter)
              {
                SK_RAWINPUT_READ (sk_input_dev_type::Gamepad)
                SK_RAWINPUT_VIEW (sk_input_dev_type::Gamepad)
              }
              else SK_RAWINPUT_HIDE (sk_input_dev_type::Gamepad)
            }
          } break;
        }

        return filter;
      };

  filter =
    FilterRawInput (uiCommand, (RAWINPUT *)pData, mouse, keyboard) && (! self);


  if (uiCommand == RID_INPUT)
  {
    switch (((RAWINPUT *)pData)->header.dwType)
    {
      case RIM_TYPEMOUSE:
      {
        if (foreground)
        {
          if (self)
          {
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_LEFT_BUTTON_DOWN   )
              io.MouseDown [0] = true;
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_RIGHT_BUTTON_DOWN  )
              io.MouseDown [1] = true;
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_MIDDLE_BUTTON_DOWN )
              io.MouseDown [2] = true;
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_BUTTON_4_DOWN      )
              io.MouseDown [3] = true;
            if ( ((RAWINPUT *)pData)->data.mouse.ulButtons & RI_MOUSE_BUTTON_5_DOWN      )
              io.MouseDown [4] = true;
          }

          if ( ((RAWINPUT *)pData)->data.mouse.usButtonFlags == RI_MOUSE_WHEEL && self   )
          {
            io.MouseWheel +=
            ((float)(short)((RAWINPUT *)pData)->data.mouse.usButtonData) /
             (float)WHEEL_DELTA;

          }
        }
      } break;


      case RIM_TYPEKEYBOARD:
      {
        //if (self)
        {
          USHORT VKey =
            ((RAWINPUT *)pData)->data.keyboard.VKey;

          // VKeys 0-7 aren't on the keyboard :)
          if (VKey & 0xFFF8) // Valid Keys:  8 - 65535
          {
            if (foreground)
            {
          //// Leads to double-input processing, left here in case Legacy Messages are disabled and this is needed
          ////
#if 0
          ////if (! (((RAWINPUT *) pData)->data.keyboard.Flags & RI_KEY_BREAK))
          ////{
          ////  pConsole->KeyDown (VKey & 0xFF, MAXDWORD);
          ////        io.KeysDown [VKey & 0xFF] = SK_IsGameWindowActive ();
          ////}

              switch (((RAWINPUT *) pData)->data.keyboard.Message)
              {
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                  io.KeysDown [VKey & 0xFF] = focus;
                  break;

                case WM_KEYUP:
                case WM_SYSKEYUP:
                  io.KeysDown [VKey & 0xFF] = false;
                  break;

                case WM_CHAR:
                case WM_SYSCHAR:
                  if (self)
                    io.AddInputCharacter (VKey);
                  break;
              }
#endif
            }
          }

          //else
          //  SK_ReleaseAssert ("Invalid Key Code" && (VKey & 0xFFF8));
        }
      } break;


      default:
        break;
    }
  }



  if (filter)
  {
    // Tell the game this event happened in the background, most will
    //   throw it out quick and easy.
    ((RAWINPUT *)pData)->header.wParam = RIM_INPUTSINK;

    // Supplying an invalid device will early-out SDL before it calls HID APIs to try
    //   and get an input report that we don't want it to see...
    ((RAWINPUT *)pData)->header.hDevice = nullptr;

    SK_ReleaseAssert (*pcbSize >= static_cast <UINT> (size) &&
                      *pcbSize >= sizeof (RAWINPUTHEADER));

    if (keyboard)
    {
      if (! (((RAWINPUT *)pData)->data.keyboard.Flags & RI_KEY_BREAK))
             ((RAWINPUT *)pData)->data.keyboard.VKey  = 0;

      // Fake key release
      ((RAWINPUT *)pData)->data.keyboard.Flags |= RI_KEY_BREAK;
    }

    // Block mouse input in The Witness by zeroing-out the memory; most other 
    //   games will see *pcbSize=0 and RIM_INPUTSINK and not process input...
    else
    {
      if (mouse)
        RtlZeroMemory (&((RAWINPUT *)pData)->data.mouse, *pcbSize - sizeof (RAWINPUTHEADER));
      else if (gamepad)
      {
        if (*pcbSize <= sizeof (RAWINPUTHEADER))
          RtlZeroMemory (&((RAWINPUT *)pData)->data.hid, *pcbSize);
        else
        {
          SK_ReleaseAssert (
            *pcbSize - sizeof (RAWINPUTHEADER) >= (((RAWINPUT *)pData)->data.hid.dwCount *
                                                   ((RAWINPUT *)pData)->data.hid.dwSizeHid) + 2 * sizeof (DWORD)
          );

          RtlZeroMemory (&((RAWINPUT *)pData)->data.hid,
                         (((RAWINPUT *)pData)->data.hid.dwCount *
                          ((RAWINPUT *)pData)->data.hid.dwSizeHid) + 2 * sizeof (DWORD));
        }
      }
    }

    if (precache_size == 0)
    {
      if (! keyboard)
      {
        *pcbSize = 0;
            size = ~0U;
      }
    }
  }

  return
    size;
}

bool
SK_ImGui_WantMouseWarpFiltering (void)
{
  if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open && SK_ImGui_IsMouseRelevant       () ) ||
       ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () ) || (game_window.mouse.can_track && !game_window.mouse.inside && config.input.mouse.disabled_to_game == 2) )
  {
    return true;
  }

  return false;
}

LONG
SK_ImGui_DeltaTestMouse ( const POINTS& last_pos,
                          const LPARAM& lParam,
                          const short   threshold = 1 )
{
  bool filter_warps =
    SK_ImGui_WantMouseWarpFiltering ();

  POINT local {
    GET_X_LPARAM (lParam),
    GET_Y_LPARAM (lParam)
  };

  if ( filter_warps && ( last_pos.x != local.x ||
                         last_pos.y != local.y ) )
  {
    bool filter = false;

    // Filter out small movements / mouselook warps
    //
    //   This does create a weird deadzone in the center of the screen,
    //     but most people will not notice ;)
    //
    if ( abs (last_pos.x - local.x) < threshold &&
         abs (last_pos.y - local.y) < threshold )
    {
      filter = true;
    }

    static auto& io =
      ImGui::GetIO ();

    POINT center { static_cast <LONG> (io.DisplaySize.x / 2.0f),
                   static_cast <LONG> (io.DisplaySize.y / 2.0f) };

    SK_ImGui_Cursor.LocalToClient (&center);

    // Now test the cursor against the center of the screen
    if ( abs (center.x - local.x) <= (static_cast <float> (center.x) / (100.0f / config.input.mouse.antiwarp_deadzone)) &&
         abs (center.y - local.y) <= (static_cast <float> (center.y) / (100.0f / config.input.mouse.antiwarp_deadzone)) )
    {
      filter = true;
    }

    // Dispose Without Processing
    if (filter)
    {
      return
        SK_ImGui_IsMouseRelevant ( ) ?
                                  1  :  0;
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

static POINTS last_pos;

ImGuiContext* SK_GImDefaultContext (void);

bool
MessageProc ( const HWND&   hWnd,
              const UINT&   msg,
              const WPARAM& wParam,
              const LPARAM& lParam )
{
  static bool window_active = true;

  if (! SK_GImDefaultContext ())
    return false;

  static auto& io =
    ImGui::GetIO ();

  static auto* pConsole =
    SK_Console::getInstance ();

  auto ActivateWindow = [&](bool active = false)
  {
    std::ignore = active;
  };

  switch (msg)
  {
    // TODO: Take the bazillion different sources of input and translate them all into
    //          a standard window message format for sanity's sake during filter evaluation.
    case WM_APPCOMMAND:
    {
      if (hWnd == game_window.hWnd || IsChild (game_window.hWnd, hWnd))
      {
        switch (GET_DEVICE_LPARAM (lParam))
        {
          case FAPPCOMMAND_KEY:
          {
            OutputDebugStringW (L"WM_APPCOMMAND Keyboard Event");

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
              OutputDebugStringW (L"Removed WM_APPCOMMAND Mouse Event");
              return true;
            }

            OutputDebugStringW (L"WM_APPCOMMAND Mouse Event");

            LPARAM dwPos = static_cast <LPARAM> (GetMessagePos ());
            LONG   lRet  = SK_ImGui_DeltaTestMouse (
                                    *(const POINTS *)&last_pos,
                   dwPos                           );

            if (lRet >= 0)
            {
              OutputDebugStringW (L"Removed WM_APPCOMMAND Mouse Delta Failure");
              return true;
            }
          } break;
        }
      }
    } break;



    ////case WM_MOUSEACTIVATE:
    ////{
    ////  ActivateWindow (((HWND)wParam == hWnd));
    ////
    ////  if ((! window_active) && SK_WantBackgroundRender ())
    ////  {
    ////    if (GetFocus () == game_window.hWnd)
    ////      return true;
    ////  }
    ////} break;
    ////
    ////
    ////case WM_ACTIVATEAPP:
    ////case WM_ACTIVATE:
    ////case WM_NCACTIVATE:
    ////{
    ////  if (hWnd == game_window.hWnd || hWnd == game_window.child)
    ////  {
    ////    if ( msg == WM_NCACTIVATE ||
    ////         msg == WM_ACTIVATEAPP )
    ////    {
    ////      ActivateWindow (wParam != 0x00);
    ////    }
    ////
    ////    else if (msg == WM_ACTIVATE)
    ////    {
    ////      switch (LOWORD (wParam))
    ////      {
    ////        case WA_ACTIVE:
    ////        case WA_CLICKACTIVE:
    ////        default: // Unknown
    ////        {
    ////          ActivateWindow ((HWND)lParam != game_window.hWnd);
    ////        } break;
    ////
    ////        case WA_INACTIVE:
    ////        {
    ////          ActivateWindow ( lParam == 0 ||
    ////                     (HWND)lParam == game_window.hWnd );
    ////        } break;
    ////      }
    ////    }
    ////  }
    ////
    ////  if ((! window_active) && SK_WantBackgroundRender ())
    ////  {
    ////    if (GetFocus () == game_window.hWnd)
    ////      return true;
    ////  }
    ////} break;


    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: // Sent on receipt of the second click
      if (hWnd == game_window.hWnd || hWnd == game_window.child) io.MouseDown [0] = true;
      return true;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK: // Sent on receipt of the second click
      if (hWnd == game_window.hWnd || hWnd == game_window.child) io.MouseDown [1] = true;
      return true;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK: // Sent on receipt of the second click
      if (hWnd == game_window.hWnd || hWnd == game_window.child) io.MouseDown [2] = true;
      return true;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK: // Sent on receipt of the second click
    {
      if (hWnd == game_window.hWnd || hWnd == game_window.child)
      {
        WORD Flags =
          GET_XBUTTON_WPARAM (wParam);

        io.MouseDown [3] |= (Flags & XBUTTON1) != 0;
        io.MouseDown [4] |= (Flags & XBUTTON2) != 0;

        return true;
      }
    } break;

    // Don't care about these events for anything other than filtering;
    //   we will poll the immediate mouse state when the frame starts.
    //
    //  This effectively gives us buffered mouse input behavior, where
    //    no mouse click is ever lost (only the time that it happened).
    //
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
      if (hWnd == game_window.hWnd || hWnd == game_window.child)
      {
        return true;
      } break;


    case WM_MOUSEWHEEL:
      if (hWnd == game_window.hWnd || hWnd == game_window.child || IsChild (game_window.hWnd, hWnd))
      {
        io.MouseWheel +=
           static_cast <float> (GET_WHEEL_DELTA_WPARAM (wParam)) /
           static_cast <float> (WHEEL_DELTA);
        return true;
      } break;

    case WM_INPUTLANGCHANGE:
    {
      SK_TLS* pTLS =
           SK_TLS_Bottom ();
      if (pTLS != nullptr)
      {
        SK_ImGui_InputLanguage_s& language =
          pTLS->input_core->input_language;

        language.changed      = true;
        language.keybd_layout = nullptr;

        return false;
      }
    } break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
      if (hWnd == game_window.hWnd || hWnd == game_window.child)
      {
        InterlockedIncrement (&__SK_KeyMessageCount);

        BYTE  vkCode   = LOWORD (wParam) & 0xFF;
        BYTE  scanCode = HIWORD (lParam) & 0x7F;

        if (vkCode & 0xF8) // Valid Keys:  8 - 255
        {
          // Don't process Alt+Tab or Alt+Enter
          if ( msg == WM_SYSKEYDOWN &&
                 ( vkCode == VK_TAB ||
                   vkCode == VK_RETURN )
             )
          {
            return false;
          }

          // Just the make / break events, repeats are ignored
          if ((lParam & 0x40000000UL) == 0)
            pConsole->KeyDown (vkCode, MAXDWORD);

          if (vkCode != VK_TAB || SK_ImGui_WantTextCapture ())
          {
            SK_TLS* pTLS =
              SK_TLS_Bottom ();

            wchar_t key_str;

            static HKL
              keyboard_layout =
                SK_Input_GetKeyboardLayout ();

            if (pTLS != nullptr)
            {
              SK_ImGui_InputLanguage_s& language =
                pTLS->input_core->input_language;

              language.update ();

              keyboard_layout =
                language.keybd_layout;
            }

            if ( ToUnicodeEx ( vkCode,
                               scanCode,
                               (const BYTE *)io.KeysDown,
                              &key_str,
                               1,
                               0x04, // Win10-Specific Flag: No Keyboard State Change
                               keyboard_layout )
                     &&
                  iswprint ( key_str )
               )
            {
              return
                MessageProc ( hWnd,
                    WM_CHAR, (WPARAM)key_str,
                     lParam );
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
      }
    } break;


    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
      if (hWnd == game_window.hWnd || hWnd == game_window.child)
      {
        InterlockedIncrement (&__SK_KeyMessageCount);

        BYTE vkCode = LOWORD (wParam) & 0xFF;
        if ( vkCode & 0xF8 ) // Valid Keys:  8 - 255
        {
          // Don't process Alt+Tab or Alt+Enter
          if ( msg == WM_SYSKEYUP &&
               ( vkCode == VK_TAB ||
                 vkCode == VK_RETURN )
             )
          {
            return false;
          }

          pConsole->KeyUp (vkCode, lParam);

          return true;
        }
      }
    } break;


    case WM_MOUSEMOVE:
    {
      if ((hWnd == game_window.hWnd && (! game_window.child)) || hWnd == game_window.child)
      {
        SK_ImGui_UpdateMouseTracker ();

        POINTS     xyPos = MAKEPOINTS (lParam);
        LONG   lDeltaRet =
          SK_ImGui_DeltaTestMouse (
               std::exchange (last_pos, xyPos),
                                       lParam);

        POINT
          cursor_pos = {
            last_pos.x,
            last_pos.y
          };

        SK_ImGui_Cursor.ClientToLocal (&cursor_pos);

        if (! SK_ImGui_WantMouseCapture ())
        {
          SK_ImGui_Cursor.orig_pos =    cursor_pos;
        }

        // Return:
        //
        //   -1 if no filtering is desired
        //    0 if message should propogate, but preserve internal cursor state
        //    1 if message should be removed
        //
        if (     lDeltaRet >= 0 ) {
          return lDeltaRet;
        }

        SK_ImGui_Cursor.pos =           cursor_pos;

        io.MousePos = {
          (float)cursor_pos.x,
          (float)cursor_pos.y
        };

        if (! SK_ImGui_WantMouseCapture ())
        {
          return FALSE;
        }

        return TRUE;
      }
    } break;


    case WM_CHAR:
    {
      if ( hWnd == game_window.hWnd  ||
           hWnd == game_window.child ||
           IsChild (game_window.hWnd, hWnd) )
      {
        InterlockedIncrement (&__SK_KeyMessageCount);

        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if ((wParam & 0xff) > 7 && wParam < 0x10000)
        {
          io.AddInputCharacter ((unsigned short)(wParam & 0xFFFF));
        }

        return true;
      }
    } break;


    case WM_INPUT:
    {
      if (! config.input.gamepad.hook_raw_input)
        return false;

      bool bRet = false;

      bool mouse    = false,
           keyboard = false,
           gamepad  = false;

      UINT dwSize = 0;

      bool        bWantMouseCapture    =
          SK_ImGui_WantMouseCapture    (),
                  bWantKeyboardCapture =
          SK_ImGui_WantKeyboardCapture (),
                  bWantGamepadCapture  =
          SK_ImGui_WantGamepadCapture  () || config.input.gamepad.disable_hid;

      bool bWantAnyCapture = bWantMouseCapture    ||
                             bWantKeyboardCapture ||
                             bWantGamepadCapture;

      if (bWantAnyCapture)
      {
        dwSize =
          SK_Input_ClassifyRawInput ( (HRAWINPUT)lParam,
                                               mouse,
                                                 keyboard,
                                                   gamepad );
      }

      if (dwSize > 0)
      {
        if ( //mouse || keyboard ||
             (mouse    && bWantMouseCapture)    ||
             (keyboard && bWantKeyboardCapture) ||
             (gamepad  && bWantGamepadCapture) )
        {
          LPBYTE lpb =
            SK_TLS_Bottom ()->raw_input->allocData (dwSize);

          if (lpb != nullptr)
          {
            if ( SK_ImGui_ProcessRawInput (
                   (HRAWINPUT)lParam, RID_INPUT,
                              lpb,    &dwSize,
                 sizeof (RAWINPUTHEADER), TRUE ) > 0 )
            {
              ///wParam       =
              ///  ( ( wParam & ~0xFF )
              ///             |  RIM_INPUTSINK );

              bRet = true;
            }
          }
        }
      }

      return bRet;
    } break;
  }

  return false;
};

LRESULT
WINAPI
ImGui_WndProcHandler ( HWND   hWnd,   UINT   msg,
                       WPARAM wParam, LPARAM lParam )
{
  static auto& io =
    ImGui::GetIO ();

  if (msg == WM_TIMER)
  {
    MSG
      msg_ = { };
      msg_.message = msg;
      msg_.lParam  = lParam;
      msg_.wParam  = wParam;

    if (wParam == 0x68993)
    {
      return
        SK_Input_DetermineMouseIdleState (&msg_);
    }
  }

  if (msg == WM_SETCURSOR)
  {
  //SK_LOG0 ( (L"ImGui Witnessed WM_SETCURSOR"), L"Window Mgr" );

    if ( LOWORD (lParam) == HTCLIENT ||
         LOWORD (lParam) == HTTRANSPARENT )
    {
      static POINTS lastMouse =
        { SHORT_MAX, SHORT_MAX };

      auto messagePos  = GetMessagePos  ();
      auto messageTime = GetMessageTime ();

      POINTS mousePos =
        MAKEPOINTS (messagePos);

      bool bRawCapture =
        ImGui::GetIO ().WantCaptureMouse;

      if ( ( hWnd == game_window.hWnd ||
             hWnd == game_window.child ) && HIWORD (lParam) != WM_NULL && ( mousePos.x != lastMouse.x ||
                                                                            mousePos.y != lastMouse.y ||
                                                                            bRawCapture ) )
      {
        static LONG        lastTime = 0;
        if (std::exchange (lastTime, messageTime) != messageTime || bRawCapture)
        {
          static HCURSOR hLastClassCursor = (HCURSOR)(-1);

          if (bRawCapture || ( SK_ImGui_WantMouseCapture  () &&
                               SK_ImGui_IsAnythingHovered () ) )
          {
            if (hLastClassCursor == (HCURSOR)(-1))
                hLastClassCursor  = (HCURSOR)GetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR);

            if (config.input.ui.use_hw_cursor)
            {
              SetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR, (LONG_PTR)ImGui_DesiredCursor ());
            }

            lastMouse = mousePos;

            return TRUE;
          }

          else
          {
            if (hLastClassCursor != (HCURSOR)(-1))
                  SetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR, (LONG_PTR)std::exchange (hLastClassCursor, (HCURSOR)(-1)));

            if (config.input.cursor.manage)
            {
              MSG
                msg_ = { };
                msg_.message = msg;
                msg_.lParam  = lParam;
                msg_.wParam  = wParam;

              SK_Input_DetermineMouseIdleState (&msg_);

              if (! SK_Window_IsCursorActive ())
              {
                lastMouse = mousePos;

                SK_SetCursor (0);

                return TRUE;
              }
            }
          }

          lastMouse = mousePos;
        }
      }
    }
  }

  if (msg == WM_MOUSELEAVE)
  {
    if (hWnd == game_window.hWnd)
    {
      game_window.mouse.inside   = false;
      game_window.mouse.tracking = false;

      // We're no longer inside the game window, move the cursor off-screen
      ImGui::GetIO ().MousePos =
        ImVec2 (-FLT_MAX, -FLT_MAX);
    }
  }

  if (msg == WM_MOUSEMOVE)
  {
    if (hWnd == game_window.hWnd)
    {
      SK_ImGui_UpdateMouseTracker ();
    }
  }

  if (msg == WM_SYSCOMMAND)
  {
    //SK_LOG0 ( (L"ImGui Witnessed WM_SYSCOMMAND"), L"Window Mgr" );
    //dll_log.Log (L"WM_SYSCOMMAND (wParam=%x, lParam=%x) [HWND=%x] :: Game Window = %x", (wParam & 0xFFF0), lParam, hWnd, game_window.hWnd);

    switch (LOWORD (wParam & 0xFFF0))
    {
      case SC_RESTORE:
      case SC_SIZE:
      case SC_PREVWINDOW:
      case SC_NEXTWINDOW:
      case SC_MOVE:
      case SC_MOUSEMENU:
      case SC_DEFAULT:
      case SC_CONTEXTHELP:
      {
        return 0;
      } break;

      // Generally an application will handle this, but if it doesn't,
      //   trigger Special K's popup window,
      case SC_CLOSE:
      {
        if (hWnd == game_window.hWnd || IsChild (hWnd, game_window.hWnd))
        {
          //SK_LOG0 ( (L"ImGui Examined SysCmd (SC_CLOSE)"), L"Window Mgr" );

          SK_ImGui_WantExit |= config.input.keyboard.catch_alt_f4;

          if (SK_ImGui_Active () || config.input.keyboard.override_alt_f4)
          {
            SK_ImGui_WantExit = true;
            return 1;
          }
        }

        return 0;
      } break;

      case SC_KEYMENU:
      {
        //SK_LOG0 ( (L"ImGui Examined SysCmd (SC_KEYMENU)"), L"Window Mgr" );

        if (hWnd == game_window.hWnd || IsChild (game_window.hWnd, hWnd))
        {
          // Disable ALT application menu
          if (lParam == 0x00 || lParam == 0x20)
          {
            return 1;
          }

          else if (lParam == 0x05/*VK_F4*/) // DOES NOT USE Virtual Key Codes!
          {
            SK_ImGui_WantExit |= config.input.keyboard.catch_alt_f4;

            if (SK_ImGui_Active () || config.input.keyboard.override_alt_f4)
            {
              if (! config.input.keyboard.override_alt_f4)
                WriteULong64Release (&config.input.keyboard.temporarily_allow, SK_GetFramesDrawn () + 40);

              SK_ImGui_WantExit = true;

              if (config.input.keyboard.override_alt_f4)
                return 1;
            }
          }
        }

        return 0;
      }
      break;

      case SC_TASKLIST:
      {
        if (config.window.disable_screensaver)
          return 1;
      } break;

      default:
        return 0;
    }
  }

  if (msg == WM_DEVICECHANGE)
  {
    switch (wParam)
    {
      case DBT_DEVICEARRIVAL:
      case DBT_DEVICEREMOVECOMPLETE:
      {
        DEV_BROADCAST_HDR* pDevHdr =
          (DEV_BROADCAST_HDR *)lParam;

        if (pDevHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
        {
          bool arrival =
            (wParam == DBT_DEVICEARRIVAL);

          DEV_BROADCAST_DEVICEINTERFACE_W *pDevW =
            (DEV_BROADCAST_DEVICEINTERFACE_W *)pDevHdr;

          DEV_BROADCAST_DEVICEINTERFACE_A *pDevA =
            (DEV_BROADCAST_DEVICEINTERFACE_A *)pDevHdr;

          if (IsEqualGUID (pDevW->dbcc_classguid, GUID_DEVINTERFACE_HID) ||
              IsEqualGUID (pDevW->dbcc_classguid, GUID_XUSB_INTERFACE_CLASS))
          {
            bool xinput = IsEqualGUID (pDevW->dbcc_classguid, GUID_XUSB_INTERFACE_CLASS);

            const bool ansi = pDevA->dbcc_size == sizeof (DEV_BROADCAST_DEVICEINTERFACE_A);
            const bool wide = pDevW->dbcc_size == sizeof (DEV_BROADCAST_DEVICEINTERFACE_W);

            wchar_t wszFileName [MAX_PATH] = { };

            if (ansi)
            {
              wcsncpy_s (                         wszFileName, MAX_PATH,
                SK_UTF8ToWideChar (pDevA->dbcc_name).c_str (), _TRUNCATE );
            }

            else if (wide)
            {
              wcsncpy_s (wszFileName, MAX_PATH, pDevW->dbcc_name, _TRUNCATE);
            }

            xinput |= wcsstr (wszFileName, L"IG_") != nullptr;

            if (xinput)
            {
              if ( config.input.gamepad.xinput.placehold [0] || config.input.gamepad.xinput.placehold [1] ||
                   config.input.gamepad.xinput.placehold [2] || config.input.gamepad.xinput.placehold [3] )
              {
                if (arrival)
                {
                  SK_LOG0 ( ( L" (Input Device Connected)" ),
                              L"XInput_Hot" );
                }

                else
                {
                  SK_LOG0 ( ( L" (Input Device Disconnected)" ),
                              L"XInput_Hot" );
                }

                for ( auto slot : { 0, 1, 2, 3 } )
                {
                  if (config.input.gamepad.xinput.placehold [slot])
                  {
                    SK_XInput_Refresh        (slot);
                    SK_XInput_PollController (slot);
                  }
                }

                return true;
              }
            }
          }
        }
      } break;
    }
  }

  UNREFERENCED_PARAMETER (lParam);

  bool handled          = MessageProc (hWnd, msg, wParam, lParam);
  bool filter_raw_input = (msg == WM_INPUT && handled && config.input.gamepad.hook_raw_input);

  bool filter_warps     = SK_ImGui_WantMouseWarpFiltering ();

  UINT uMsg = msg;

  if (handled)
  {
    bool keyboard_capture =
      ( ( (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST) ||
         //uMsg == WM_HOTKEY     ||
         // *** Presumably, games do not use `WM_HOTKEY`... the Steam Overlay does
         //       => Generally we don't want to block keyboard input to Steam.
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
      if ( ( uMsg == WM_KEYDOWN ||
             uMsg == WM_KEYUP ) && SK_ImGui_WantMouseCapture () ) 
      {
        // Block Mouse Input
        mouse_capture    = true;
        keyboard_capture = false;
      }
    }


    if ( uMsg == WM_KEYDOWN ||
         uMsg == WM_SYSKEYDOWN )
    {
      // Only handle key-down if the game window is active,
      //   otherwise treat it as key release.
      io.KeysDown [wParam & 0xFF] = SK_IsGameWindowActive ();
    }

    else if ( uMsg == WM_KEYUP ||
              uMsg == WM_SYSKEYUP )
    {
      io.KeysDown [wParam & 0xFF] = false;
    }


    if (config.input.ui.capture_mouse)
    {
      mouse_capture =
        ( uMsg >= WM_MOUSEFIRST &&
          uMsg <= WM_MOUSELAST );
    }

    if ( keyboard_capture || mouse_capture || filter_raw_input )
    {
      return 1;
    }
  }

  return 0;
}

bool
_Success_(false)
SK_ImGui_FilterXInput (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  bool disable =
    config.input.gamepad.disabled_to_game == SK_InputEnablement::Disabled ||
      ( SK_ImGui_WantGamepadCapture () /* &&
        dwUserIndex == (DWORD)config.input.gamepad.xinput.ui_slot */ );

  if (disable || config.input.gamepad.xinput.disable [dwUserIndex])
  {
    RtlZeroMemory (&pState->Gamepad, sizeof (XINPUT_GAMEPAD));

    // SDL Keepalive
    if (pState->dwPacketNumber < 1)
        pState->dwPacketNumber = 1;

    return true;
  }

  return false;
}

bool
_Success_(false)
SK_ImGui_FilterXInputKeystroke (
  _In_  DWORD             dwUserIndex,
  _Out_ XINPUT_KEYSTROKE *pKeystroke )
{
  bool disable =
    config.input.gamepad.disabled_to_game == SK_InputEnablement::Disabled ||
      ( SK_ImGui_WantGamepadCapture ()   &&
        dwUserIndex == (DWORD)config.input.gamepad.xinput.ui_slot );

  if (disable)
  {
    RtlZeroMemory (&pKeystroke->Flags, sizeof (pKeystroke->Flags));

    return true;
  }

  return false;
}


struct {
  struct linear_pulse_event_s {
    float duration, strength,
          start,    end;

    float run (void) {
      auto now = static_cast <float> (SK::ControlPanel::current_time);

      return config.input.gamepad.haptic_ui ?
               std::max (0.0f, strength * ((end - now) / (end - start))) :
                         0.0f;
    }
  } PulseTitle  { 350.0f,   1.0f,  0.0f, 0.0f, },
    PulseButton {  85.0f,   0.5f,  0.0f, 0.0f, },
    PulseNav    {  60.0f, 0.015f,  0.0f, 0.0f, };
} haptic_events;


bool
SK_ImGui_HasXboxController (void)
{
  return
    SK_ImGui_ProcessGamepadStatusBar (false) > 0;
}

bool
SK_ImGui_HasPlayStationController (void)
{
  for ( const auto& controller : SK_HID_PlayStationControllers )
  {
    if (controller.bConnected)
      return true;
  }

  return false;
}

bool
SK_ImGui_HasDualSenseController (void)
{
  for ( const auto& controller : SK_HID_PlayStationControllers )
  {
    if ( controller.bConnected &&
         controller.bDualSense )
      return true;
  }

  return false;
}

bool
SK_ImGui_HasDualSenseEdgeController (void)
{
  for ( const auto& controller : SK_HID_PlayStationControllers )
  {
    if ( controller.bConnected &&
         controller.bDualSenseEdge )
      return true;
  }

  return false;
}


#include <SpecialK/steam_api.h>

bool
WINAPI
SK_ImGui_ToggleEx ( bool& toggle_ui,
                    bool& toggle_nav )
{
  //
  // Only allow one toggle per-frame, even if we wind up calling
  //   this function multiple times to translate HID to XInput...
  //
  static ULONG64              ulLastToggleFrame = 0;
  if (SK_GetFramesDrawn () != ulLastToggleFrame)
  {
    if (toggle_ui||toggle_nav)
    {
      ulLastToggleFrame = SK_GetFramesDrawn ();

      if (toggle_ui)
      {
        SK_ImGui_Toggle ();
      }

      if (toggle_nav && SK_ImGui_Active ())
        nav_usable = (! nav_usable);

      if (nav_usable)
        ImGui::SetNextWindowFocus ();

      toggle_ui  = SK_ImGui_Active ();
      toggle_nav = nav_usable;

      if (SK_ImGui_Active () && nav_usable)
      {
        haptic_events.PulseTitle.start = static_cast <float> (SK::ControlPanel::current_time);
        haptic_events.PulseTitle.end   = haptic_events.PulseTitle.start +
                                           haptic_events.PulseTitle.duration;
      }

      //ImGui::GetIO ().NavActive = nav_usable;

      // Zero-out any residual haptic data
      else
      {
        SK_XInput_ZeroHaptics (config.input.gamepad.steam.ui_slot);
        SK_XInput_ZeroHaptics (config.input.gamepad.xinput.ui_slot);
      }
    }
  }

  return SK_ImGui_Active ();
}

#include <SpecialK/input/dinput8_backend.h>
#include <SpecialK/input/steam.h>

joyGetDevCapsW_pfn _joyGetDevCapsW = nullptr;
joyGetPos_pfn      _joyGetPos      = nullptr;
joyGetPosEx_pfn    _joyGetPosEx    = nullptr;
joyGetNumDevs_pfn  _joyGetNumDevs  = nullptr;

extern IDirectInputDevice8W_GetDeviceState_pfn
       IDirectInputDevice8W_GetDeviceState_Original;

extern XINPUT_STATE  di8_to_xi;
extern XINPUT_STATE  hid_to_xi;
extern XINPUT_STATE  sce_to_xi;

UINT
WINAPI
SK_joyGetPosEx ( _In_  UINT        uJoyID,
                 _Out_ LPJOYINFOEX pji )
{
  if (_joyGetPosEx == nullptr)
  {
    SK_RunOnce (SK_Input_HookWinMM ());
  }

  return
    _joyGetPosEx == nullptr ? JOYERR_UNPLUGGED
                            :
    _joyGetPosEx (uJoyID, pji);
}

UINT
WINAPI
SK_joyGetDevCapsW ( _In_                     UINT_PTR   uJoyID,
                    _Out_writes_bytes_(cbjc) LPJOYCAPSW pjc,
                    _In_                     UINT       cbjc )
{
  if (_joyGetDevCapsW == nullptr)
  {
    SK_RunOnce (SK_Input_HookWinMM ());
  }

  return
    _joyGetDevCapsW == nullptr ? JOYERR_UNPLUGGED
                               :
    _joyGetDevCapsW (uJoyID, pjc, cbjc);
}

UINT
WINAPI
SK_joyGetNumDevs (void)
{
  if (_joyGetNumDevs == nullptr)
  {
    SK_RunOnce (SK_Input_HookWinMM ());
  }

  return
    _joyGetNumDevs == nullptr ? JOYERR_UNPLUGGED
                              :
    _joyGetNumDevs ();
}

bool
SK_XInput_ValidateStatePointer (XINPUT_STATE *pState)
{
  __try {
    pState->dwPacketNumber++;
    pState->dwPacketNumber--;
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return false;
  }

  return true;
}

#include <SpecialK/sound.h>

XINPUT_STATE SK_ImGui_XInputState = {};

bool
SK_ImGui_PollGamepad_EndFrame (XINPUT_STATE* pState)
{
  // Triggers SteamInput kill switch once per-frame
  SK_ImGui_WantGamepadCapture ();

  static auto& io =
    ImGui::GetIO ();

  bool bToggleNav = false,
       bToggleVis = false;

  // Reset Mouse / Keyboard State so that we can process all state transitions
  //   that occur during the next frame without losing any input events.
  if ( SK_IsGameWindowActive () )
  {
    bool Alt =
      (SK_GetAsyncKeyState (VK_MENU)    ) != 0;
    bool Ctrl =
      (SK_GetAsyncKeyState (VK_CONTROL) ) != 0;
    bool Shift =
      (SK_GetAsyncKeyState (VK_SHIFT)   ) != 0;

    // This code looks strange, but this function is called multiple
    //   times per-frame if games poll more than one gamepad... so
    //     we need to OR the modifier key state together and use the
    //      _instantaneous_ modifier key state for UI activation.
    struct {
      struct {
        bool last    = false;
        bool now     = false;
        bool toggled = false;
      } capslock,
        backspace, tab;

      ULONG64 last_frame = 0;
    } static keys;

    ULONG64 this_frame =
      SK_GetFramesDrawn ();

    if (keys.last_frame < this_frame)
    {
      if (keys.tab.toggled)
      {
        void
        CALLBACK
        SK_PluginKeyPress (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode);
        SK_PluginKeyPress (io.KeyCtrl, io.KeyShift, io.KeyAlt, VK_TAB);
      }

      keys.capslock.last  = keys.capslock.now;
      keys.backspace.last = keys.backspace.now;
      keys.tab.last       = keys.tab.now;

      keys.last_frame     = this_frame;

      keys.capslock.now   =
        ImGui::IsKeyPressed (ImGuiKey_CapsLock,  false);
      keys.backspace.now  =
        ImGui::IsKeyPressed (ImGuiKey_Backspace, false);
      keys.tab.now        =
        (SK_GetAsyncKeyState (VK_TAB) & 0x8000);

      keys.capslock.toggled  = false;
      keys.backspace.toggled = false;
      keys.tab.toggled       = false;

      io.KeyAlt   = false;
      io.KeyShift = false;
      io.KeyCtrl  = false;
    }

    io.KeyAlt   |= Alt;
    io.KeyShift |= Shift;
    io.KeyCtrl  |= Ctrl;

    keys.capslock.now  |=
      ImGui::IsKeyPressed (ImGuiKey_CapsLock,  false);
    keys.backspace.now |=
      ImGui::IsKeyPressed (ImGuiKey_Backspace, false);
    keys.tab.now |=
      ((SK_GetAsyncKeyState (VK_TAB) & 0x8000) == 0x8000);

    if (! keys.capslock.toggled)
    {
      bToggleNav |=
         ( SK_ImGui_Visible &&
            ((! keys.capslock.last) && keys.capslock.now) );

      keys.capslock.toggled |= bToggleNav;
    }

    if (! keys.backspace.toggled)
    {
      bToggleVis |=
         ( Ctrl  &&
           Shift &&
            ((! keys.backspace.last) && keys.backspace.now) );

      keys.backspace.toggled |= bToggleVis;
    }

    if (! keys.tab.toggled)
    {
      keys.tab.toggled |=
        ((! keys.tab.last) && keys.tab.now);
    }
  }


  bool bUseGamepad =
    SK_IsGameWindowActive () ||
      ( config.window.background_render &&
        config.input.gamepad.disabled_to_game != SK_InputEnablement::DisabledInBackground );

  // Steam may corrupt the stack, we can try to recover...
  bUseGamepad &= SK_XInput_ValidateStatePointer (pState);

  bool bRet = false;

  static XINPUT_STATE last_state = { 1, 0 };

  hid_to_xi.Gamepad = {};

  bool bHasPlayStation = false;

  if (bUseGamepad)
  {
    SK_RunOnce (SK_HID_SetupPlayStationControllers ());

    // Figure out the most recent controller with activity
    UINT64                  ullLastActiveTimestamp  = 0ULL;
    SK_HID_PlayStationDevice *pLastActiveController = nullptr;

    for ( auto& ps_controller : SK_HID_PlayStationControllers )
    {
      if (ps_controller.bConnected)
      {
        if (ps_controller.request_input_report ())
        {
          if (ps_controller.xinput.last_active >= ullLastActiveTimestamp)
          {
            ullLastActiveTimestamp = ps_controller.xinput.last_active;
            pLastActiveController  = &ps_controller;
          }

          if ( config.input.gamepad.xinput.ui_slot >= 0 &&
               config.input.gamepad.xinput.ui_slot <  4 )
          { // Use the HID data to control the UI
            bHasPlayStation = true;
          }
        }
      }
    }

    if (pLastActiveController != nullptr)
    {
      hid_to_xi = pLastActiveController->xinput.prev_report;

      if (SK_ImGui_WantGamepadCapture () || config.input.gamepad.xinput.emulate)
      {
        for ( auto& ps_controller : SK_HID_PlayStationControllers )
        {
          if (ps_controller.bConnected)// && (ps_controller.bSimpleMode == false || ps_controller.bBluetooth == false))
              ps_controller.write_output_report ();
        }
        //pLastActiveController->write_output_report ();
      }
    }

    auto& state =
        *pState;

    bool bHasRealXInputOnUISlot =
      SK_XInput_PollController (config.input.gamepad.xinput.ui_slot, &state);

    static XINPUT_STATE last_native_state = {};
    static UINT64       last_native_time  = 0;

    bool bHasNewNativeInput =                                                               // Buttons should repeat
      bHasRealXInputOnUISlot && (memcmp (&state, &last_native_state, sizeof (XINPUT_STATE)) || state.Gamepad.wButtons != 0);

    memcpy (&last_native_state, &state, sizeof (XINPUT_STATE));

    if (bHasNewNativeInput)
    {
      last_native_time = SK_QueryPerf ().QuadPart;
    }

    if (bHasPlayStation && ullLastActiveTimestamp > last_native_time)
    {
      state = hid_to_xi;
    }

    else
    {
      bHasPlayStation = false;
    }

    //extern void SK_ScePad_PaceMaker (void);
    //            SK_ScePad_PaceMaker ();

    SK_ImGui_XInputState = state;

    if ( bHasPlayStation ||
         bHasRealXInputOnUISlot )
    {
      //
      // Do not do in the background unless bg input is enabled
      //
      //   Also only allow input from the most recently active device,
      //     in case we're reading the same controller over
      //       Bluetooth and USB...
      //
      const bool bAllowSpecialButtons =
        ( config.input.gamepad.disabled_to_game == 0 ||
            SK_IsGameWindowActive () );

      static bool bChordActivated = false;

      if ( config.input.gamepad.scepad.enhanced_ps_button &&
                               bAllowSpecialButtons )
      {
        static HWND hWndLastApp =
          SK_GetForegroundWindow ();

        if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
            (     state.Gamepad.wButtons & XINPUT_GAMEPAD_X)     &&
          (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_X)))
        {
          if (SK_GetForegroundWindow () != game_window.hWnd)
          {
            hWndLastApp =
              SK_GetForegroundWindow ();

            auto show_cmd = 
              IsMinimized (game_window.hWnd) ? SW_SHOWNORMAL
                                             : SW_SHOW;

            ShowWindow                 (game_window.hWnd, show_cmd);
            SK_RealizeForegroundWindow (game_window.hWnd);
            ShowWindow                 (game_window.hWnd, show_cmd);
          }

          bChordActivated = true;
        }

        if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
            (     state.Gamepad.wButtons & XINPUT_GAMEPAD_A)     &&
          (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_A)))
        {
          SendMessageTimeout (GetDesktopWindow (), WM_SYSCOMMAND, SC_SCREENSAVE, 0, SMTO_BLOCK, INFINITE, nullptr);
        }

        if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
            (     state.Gamepad.wButtons & XINPUT_GAMEPAD_B)     &&
          (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_B)))
        {
          if (SK_GetForegroundWindow () != hWndLastApp &&
                                      0 != hWndLastApp &&
                                 IsWindow (hWndLastApp))
          {
            auto show_cmd = 
              IsMinimized (hWndLastApp) ? SW_SHOWNORMAL
                                        : SW_SHOW;

            ShowWindow                 (hWndLastApp, show_cmd);
            SK_RealizeForegroundWindow (hWndLastApp);
            ShowWindow                 (hWndLastApp, show_cmd);
          }

          bChordActivated = true;
        }

        if (bAllowSpecialButtons)
        {
#if 1
          static bool bIsVLC =
            StrStrIW (SK_GetHostApp (), L"vlc");

          if (bIsVLC && SK_IsGameWindowActive ())
          {
            if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_A) &&
              (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_A)))
            {
              const BYTE bScancode =
                (BYTE)MapVirtualKey (VK_MEDIA_PLAY_PAUSE, 0);

              const DWORD dwFlags =
                ( bScancode & 0xE0 ) == 0   ?
                  static_cast <DWORD> (0x0) :
                  static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

              SK_keybd_event (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags,                   0);
              SK_keybd_event (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
            }

            if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) &&
              (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)))
            {
              const BYTE bScancode =
                (BYTE)MapVirtualKey (VK_LEFT, 0);

              const DWORD dwFlags =
                ( bScancode & 0xE0 ) == 0   ?
                  static_cast <DWORD> (0x0) :
                  static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

              SK_keybd_event (VK_LEFT, bScancode, dwFlags,                   0);
              SK_keybd_event (VK_LEFT, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
            }

            if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) &&
              (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)))
            {
              const BYTE bScancode =
                (BYTE)MapVirtualKey (VK_RIGHT, 0);

              const DWORD dwFlags =
                ( bScancode & 0xE0 ) == 0   ?
                  static_cast <DWORD> (0x0) :
                  static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

              SK_keybd_event (VK_RIGHT, bScancode, dwFlags,                   0);
              SK_keybd_event (VK_RIGHT, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
            }

            if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
              (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)))
            {
              const BYTE bScancode =
                (BYTE)MapVirtualKey (VK_DOWN, 0);

              const DWORD dwFlags =
                ( bScancode & 0xE0 ) == 0   ?
                  static_cast <DWORD> (0x0) :
                  static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

              SK_keybd_event (VK_DOWN, bScancode, dwFlags,                   0);
              SK_keybd_event (VK_DOWN, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
            }

            if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
              (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)))
            {
              const BYTE bScancode =
                (BYTE)MapVirtualKey (VK_UP, 0);

              const DWORD dwFlags =
                ( bScancode & 0xE0 ) == 0   ?
                  static_cast <DWORD> (0x0) :
                  static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

              SK_keybd_event (VK_UP, bScancode, dwFlags,                   0);
              SK_keybd_event (VK_UP, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
            }

            if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) &&
              (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)))
            {
              const BYTE bScancode =
                (BYTE)MapVirtualKey (VK_MEDIA_PREV_TRACK, 0);

              const DWORD dwFlags =
                ( bScancode & 0xE0 ) == 0   ?
                  static_cast <DWORD> (0x0) :
                  static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

              SK_keybd_event (VK_MEDIA_PREV_TRACK, bScancode, dwFlags,                   0);
              SK_keybd_event (VK_MEDIA_PREV_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
            }

            if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) &&
              (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)))
            {
              const BYTE bScancode =
                (BYTE)MapVirtualKey (VK_MEDIA_NEXT_TRACK, 0);

              const DWORD dwFlags =
                ( bScancode & 0xE0 ) == 0   ?
                  static_cast <DWORD> (0x0) :
                  static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

              SK_keybd_event (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags,                   0);
              SK_keybd_event (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
            }
          }
#endif

          // Guide button was just pressed, enter chord processing mode.
          if ( (     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
             (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)))
          {
            bChordActivated = false;
          }

          if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
              (     state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)     &&
            (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)))
          {
            SK_DeferCommand ("Input.Gamepad.PowerOff 1");
            bChordActivated = true;
          }

          if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
              (     state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)  &&
            (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)))
          {
            SK_SteamAPI_TakeScreenshot ();
            bChordActivated = true;
          }

          if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)      &&
              (     state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) &&
            (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)))
          {
            const BYTE bScancode =
              (BYTE)MapVirtualKey (VK_MEDIA_PLAY_PAUSE, 0);

            const DWORD dwFlags =
              ( bScancode & 0xE0 ) == 0   ?
                static_cast <DWORD> (0x0) :
                static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

            SK_keybd_event (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags,                   0);
            SK_keybd_event (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);

            bChordActivated = true;
          }

          if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)         &&
              (     state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) &&
            (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)))
          {
            const BYTE bScancode =
              (BYTE)MapVirtualKey (VK_MEDIA_PREV_TRACK, 0);

            const DWORD dwFlags =
              ( bScancode & 0xE0 ) == 0   ?
                static_cast <DWORD> (0x0) :
                static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

            SK_keybd_event (VK_MEDIA_PREV_TRACK, bScancode, dwFlags,                   0);
            SK_keybd_event (VK_MEDIA_PREV_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);

            bChordActivated = true;
          }

          if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)          &&
              (     state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) &&
            (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)))
          {
            const BYTE bScancode =
              (BYTE)MapVirtualKey (VK_MEDIA_NEXT_TRACK, 0);

            const DWORD dwFlags =
              ( bScancode & 0xE0 ) == 0   ?
                static_cast <DWORD> (0x0) :
                static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

            SK_keybd_event (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags,                   0);
            SK_keybd_event (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);

            bChordActivated = true;
          }

          if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)      &&
              (     state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) &&
            (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)))
          {
            SK_GetCommandProcessor ()->
              ProcessCommandLine ("HDR.Luminance += 0.125");

            bChordActivated = true;
          }

          if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)     &&
              (     state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) &&
            (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)))
          {
            SK_GetCommandProcessor ()->
              ProcessCommandLine ("HDR.Luminance -= 0.125");

            bChordActivated = true;
          }

          if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)   &&
              (     state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
            (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)))
          {
            SK_GetCommandProcessor ()->
              ProcessCommandLine ("Sound.Volume += 10.0");

            bChordActivated = true;
          }

          if ((     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)     &&
              (     state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
            (!(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)))
          {
            SK_GetCommandProcessor ()->
              ProcessCommandLine ("Sound.Volume -= 10.0");

            bChordActivated = true;
          }

          if (!(     state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
             ( (last_state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE))&&
             (! bChordActivated) )
          {
            //bool bToggleVis = false;
            //bool bToggleNav = false;

            if (SK_ImGui_Active ())
            {
              bToggleVis |= true;
              bToggleNav |= true;
            }

            else
            {
              bToggleNav |= (! nav_usable);
              bToggleVis |= true;
            }

            bool
            WINAPI
            SK_ImGui_ToggleEx ( bool& toggle_ui,
                                bool& toggle_nav );

            if (                 bToggleVis||bToggleNav)
              SK_ImGui_ToggleEx (bToggleVis, bToggleNav);
          }
        }
      }

      bRet = true;

      if ( state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK  &&
           state.Gamepad.wButtons & XINPUT_GAMEPAD_START &&
           last_state.dwPacketNumber != state.dwPacketNumber )
      {
        if (! ( last_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK &&
                last_state.Gamepad.wButtons & XINPUT_GAMEPAD_START ) )
        {
          // Additional condition for Final Fantasy X so as not to interfere with soft reset
          if (! ( state.Gamepad.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
                  state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) )
          {
            bToggleNav |= (! nav_usable);
            bToggleVis |= true;
          }
        }
      }

#if 0
      if ( config.input.gamepad.scepad.enhanced_ps_button     &&
           last_state.dwPacketNumber != state.dwPacketNumber  &&
                state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE &&
            ( config.input.gamepad.xinput.ui_slot >= 0 &&
              config.input.gamepad.xinput.ui_slot <  4 ) )
      {
        if (! ( last_state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE ) )
        {
          bToggleNav |= (! nav_usable);
          bToggleVis |= true;
        }
      }
#endif

      static constexpr DWORD LONG_PRESS  = 400UL;
      static           DWORD dwLastPress = MAXDWORD;

      if ( (     state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) &&
           (last_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) )
      {
        if (dwLastPress < SK::ControlPanel::current_time - LONG_PRESS)
        {
          if (SK_ImGui_Active ())
          {
            bToggleVis |= false; // nop
            bToggleNav |= true;
          }

          dwLastPress = MAXDWORD;
        }
      }

      else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
        dwLastPress = SK::ControlPanel::current_time;

      else
        dwLastPress = MAXDWORD;
    }

    else
      RtlZeroMemory (&state.Gamepad, sizeof (XINPUT_GAMEPAD));


    if (                 bToggleVis||bToggleNav)
      SK_ImGui_ToggleEx (bToggleVis, bToggleNav);
  }

  static bool last_haptic = false;

  if (bUseGamepad && SK_ImGui_Active () && config.input.gamepad.haptic_ui && nav_usable)
  {
    ImGuiContext& g =
      *GImGui;

    static ImGuiID nav_id = 0;

    if (g.NavId != nav_id)
    {
      if (haptic_events.PulseNav.end > static_cast <float> (SK::ControlPanel::current_time))
      {
        haptic_events.PulseNav.end   += haptic_events.PulseNav.duration;
        haptic_events.PulseNav.start += haptic_events.PulseNav.duration;
      }

      else
      {
        haptic_events.PulseNav.start = static_cast <float> (SK::ControlPanel::current_time);
        haptic_events.PulseNav.end   = haptic_events.PulseNav.start +
                                         haptic_events.PulseNav.duration;
      }
    }

    if (g.ActiveIdIsJustActivated)
    {
      haptic_events.PulseButton.start = static_cast <float> (SK::ControlPanel::current_time);
      haptic_events.PulseButton.end   = haptic_events.PulseButton.start +
                                          haptic_events.PulseButton.duration;
    }

    SK_XInput_PulseController ( config.input.gamepad.xinput.ui_slot,
                                  haptic_events.PulseTitle.run  () +
                                  haptic_events.PulseButton.run () +
                  std::min (0.4f, haptic_events.PulseNav.run ()),
                                    haptic_events.PulseTitle.run  () +
                                    haptic_events.PulseButton.run () +
                    std::min (0.4f, haptic_events.PulseNav.run    ()) );

    nav_id = g.NavId;

    last_haptic = true;
  }

  else if (std::exchange (last_haptic, false))
  {
    // Clear haptics on the first frame after they're no longer relevant
    SK_XInput_PulseController (
      config.input.gamepad.xinput.ui_slot, 0.0f, 0.0f
    );
  }

  if (bUseGamepad)
    last_state = *pState;

  return bRet;
}


#include <SpecialK/core.h>
#include <SpecialK/widgets/widget.h>

#define SK_Threshold(x,y) (x) > (y) ? ( (x) - (y) ) : 0

void
SK_ImGui_PollGamepad (void)
{
  static auto& io =
    ImGui::GetIO ();

         XINPUT_STATE state    = {      };
  static XINPUT_STATE last_state { 1, 0 };

  float                          LastNavInputs [16];
  std::copy_n (io.NavInputs, 16, LastNavInputs);

  for ( float& NavInput : io.NavInputs )
  {
    NavInput = 0.0f;
  }

  if (SK_ImGui_PollGamepad_EndFrame (&state))
  {
    //auto& gamepad = state.Gamepad;

    ////XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    //#define MAP_BUTTON(NAV_NO,BUTTON_ENUM)    \
    //{ io.NavInputs   [(NAV_NO)] =             \
    //  ( gamepad.wButtons  &  (BUTTON_ENUM)) ? \
    //                 1.0f : 0.0f;           }
    //#define MAP_ANALOG(NAV_NO,VALUE,V0,V1)                    \
    //{ float vn = (float)((VALUE) - V0)  /  (float)(V1 - V0);  \
    //  if (  vn > 1.0f                )             vn = 1.0f; \
    //  if (  vn > 0.0f && io.NavInputs [(NAV_NO)] < vn)        \
    //                     io.NavInputs [(NAV_NO)] = vn; }
//  //-------------------------------------------------------------------------------------
    //MAP_BUTTON (ImGuiNavInput_Activate,   XINPUT_GAMEPAD_A);              // Cross    / A
    //MAP_BUTTON (ImGuiNavInput_Cancel,     XINPUT_GAMEPAD_B);              // Circle   / B
    //MAP_BUTTON (ImGuiNavInput_Menu,       XINPUT_GAMEPAD_X);              // Square   / X
    //MAP_BUTTON (ImGuiNavInput_Input,      XINPUT_GAMEPAD_Y);              // Triangle / Y
    //MAP_BUTTON (ImGuiNavInput_DpadLeft,   XINPUT_GAMEPAD_DPAD_LEFT);      // D-Pad Left
    //MAP_BUTTON (ImGuiNavInput_DpadRight,  XINPUT_GAMEPAD_DPAD_RIGHT);     // D-Pad Right
    //MAP_BUTTON (ImGuiNavInput_DpadUp,     XINPUT_GAMEPAD_DPAD_UP);        // D-Pad Up
    //MAP_BUTTON (ImGuiNavInput_DpadDown,   XINPUT_GAMEPAD_DPAD_DOWN);      // D-Pad Down
    //MAP_BUTTON (ImGuiNavInput_FocusPrev,  XINPUT_GAMEPAD_LEFT_SHOULDER);  // L1 / LB
    //MAP_BUTTON (ImGuiNavInput_FocusNext,  XINPUT_GAMEPAD_RIGHT_SHOULDER); // R1 / RB
    //MAP_BUTTON (ImGuiNavInput_TweakSlow,  XINPUT_GAMEPAD_LEFT_TRIGGER);   // L2 / LT
    //MAP_BUTTON (ImGuiNavInput_TweakFast,  XINPUT_GAMEPAD_RIGHT_TRIGGER);  // R2 / RT
//  ---------------------------------------------------------------------==================
    //MAP_ANALOG (ImGuiNavInput_LStickLeft, gamepad.sThumbLX, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
    //MAP_ANALOG (ImGuiNavInput_LStickRight,gamepad.sThumbLX, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
    //MAP_ANALOG (ImGuiNavInput_LStickUp,   gamepad.sThumbLY, +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
    //MAP_ANALOG (ImGuiNavInput_LStickDown, gamepad.sThumbLY, -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32767);

    if (nav_usable)
    {
      // Non-repeating buttons
      if (last_state.dwPacketNumber <= state.dwPacketNumber)
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

        // Close Menu/PopUp/Child, Clear Selection      // e.g. Cross button
        if ( (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0) //&&
        //(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) == 0)
        {
          io.NavInputs [ImGuiNavInput_Cancel] = LastNavInputs [ImGuiNavInput_Cancel] + io.DeltaTime;
        }

        // Text Input                                   // e.g. Triangle button
        if ( (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0) //&&
        //(last_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) == 0)
        {
          io.NavInputs [ImGuiNavInput_Input] = LastNavInputs [ImGuiNavInput_Input] + io.DeltaTime;
        }

        io.NavInputs [ImGuiNavInput_LStickDown]    = 0.0f;
        io.NavInputs [ImGuiNavInput_LStickUp]      = 0.0f;

        if (uLY > 0.0f)
          io.NavInputs [ImGuiNavInput_LStickUp]   += uLY * analog_sensitivity;

        else if (uLY < 0.0f)
          io.NavInputs [ImGuiNavInput_LStickDown] -= uLY * analog_sensitivity;


        io.NavInputs [ImGuiNavInput_LStickLeft]    = 0.0f;
        io.NavInputs [ImGuiNavInput_LStickRight]   = 0.0f;

        if (uLX > 0.0f)
          io.NavInputs [ImGuiNavInput_LStickRight] += uLX * analog_sensitivity;

        else if (uLX < 0.0f)
          io.NavInputs [ImGuiNavInput_LStickLeft]  -= uLX * analog_sensitivity;


        // Next Window (with PadMenu held)              // e.g. L-trigger
        io.NavInputs [ImGuiNavInput_FocusPrev]   +=
          static_cast <float> (
            (     state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0 &&
            (last_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) == 0
          );

        // Prev Window (with PadMenu held)              // e.g. R-trigger
        io.NavInputs [ImGuiNavInput_FocusNext]   +=
          static_cast <float> (
            (     state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0 &&
            (last_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) == 0
          );

        io.NavInputs [ImGuiNavInput_TweakSlow]   +=
          static_cast <float> (
            SK_Threshold ( state.Gamepad.bLeftTrigger,
                           XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ) /
                ( 255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD     );

        io.NavInputs [ImGuiNavInput_TweakFast]   +=
          static_cast <float> (
            SK_Threshold ( state.Gamepad.bRightTrigger,
                           XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ) /
                ( 255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD     );

        //
        // [ANALOG INPUTS]
        //

        // Press Button, Tweak Value                    // e.g. Circle button
        if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0)
          io.NavInputs [ImGuiNavInput_Activate] = LastNavInputs [ImGuiNavInput_Activate] + io.DeltaTime;

        // Access Menu, Focus, Move, Resize             // e.g. Square button
        if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0)
          io.NavInputs [ImGuiNavInput_Menu] = LastNavInputs [ImGuiNavInput_Menu] + io.DeltaTime;

        // Move Up, Resize Window (with PadMenu held)   // e.g. D-pad up/down/left/right
        io.NavInputs [ImGuiNavInput_DpadUp]    +=  0.001f *
          static_cast <float> (
            (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)    != 0
          );

        // Move Down
        io.NavInputs [ImGuiNavInput_DpadDown]  +=  0.001f *
          static_cast <float> (
            (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  != 0
          );

        // Move Left
        io.NavInputs [ImGuiNavInput_DpadLeft]  +=  0.001f *
          static_cast <float> (
            (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)  != 0
          );

        // Move Right
        io.NavInputs [ImGuiNavInput_DpadRight] +=  0.001f *
          static_cast <float> (
            (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0
          );
      }
    }

    last_state = state;
  }


  if (! nav_usable)
    io.NavActive = false;


  // Don't cycle window elements when Alt+Tabbing
  if (io.KeyAlt || (! io.NavActive)) io.KeysDown [VK_TAB] = false;
  
  //
  // Also don't process tab unless navigation is active, or the control panel
  //   will activate the framerate limiter input and block keyboard to the game...
  //


  //
  // Same basic idea as above, only for keyboard
  //
  //   The primary difference between gamepad and keyboard is the lack of a left
  //     analog stick to differentiate D-Pad behavior from analog stick movement.
  //
  //   For keyboard, we alias both to the same task and also consider WASD to be
  //     identical to Up/Down/Left/Right.
  //
  if (io.NavVisible && nav_usable)
  {
    if (io.KeyCtrl)
      io.NavInputs [ImGuiNavInput_Menu] = 1.0f;

    if (! io.WantTextInput)
    {
      const bool up    = ( io.KeysDown [SK_KeyMap_LeftHand_Arrow ('W')] || io.KeysDown [VK_UP   ] );
      const bool down  = ( io.KeysDown [SK_KeyMap_LeftHand_Arrow ('S')] || io.KeysDown [VK_DOWN ] );
      const bool left  = ( io.KeysDown [SK_KeyMap_LeftHand_Arrow ('A')] || io.KeysDown [VK_LEFT ] );
      const bool right = ( io.KeysDown [SK_KeyMap_LeftHand_Arrow ('D')] || io.KeysDown [VK_RIGHT] );

      const float inv_analog     =
          ( 1.0f / analog_sensitivity );

      constexpr float analog_epsilon = 0.001f;

      //io.NavInputs [ImGuiNavInput_LStickUp]   = 0.0f;
      //io.NavInputs [ImGuiNavInput_LStickDown] = 0.0f;
      //
      //io.NavInputs [ImGuiNavInput_LStickLeft]  = 0.0f;
      //io.NavInputs [ImGuiNavInput_LStickRight] = 0.0f;
      //
      //io.NavInputs [ImGuiNavInput_DpadUp]    = 0.0f;
      //io.NavInputs [ImGuiNavInput_DpadDown]  = 0.0f;
      //io.NavInputs [ImGuiNavInput_DpadLeft]  = 0.0f;
      //io.NavInputs [ImGuiNavInput_DpadRight] = 0.0f;

      if (io.KeyCtrl)
      {
        io.NavInputs [ImGuiNavInput_LStickUp]    +=
          (io.KeysDown [VK_PRIOR] ? inv_analog : 0.0f);
        io.NavInputs [ImGuiNavInput_LStickDown]  +=
          (io.KeysDown [VK_NEXT ] ? inv_analog : 0.0f);
        io.NavInputs [ImGuiNavInput_LStickLeft]  +=
          (io.KeysDown [VK_HOME ] ? inv_analog : 0.0f);
        io.NavInputs [ImGuiNavInput_LStickRight] +=
          (io.KeysDown [VK_END  ] ? inv_analog : 0.0f);

        io.NavInputs [ImGuiNavInput_LStickUp]    +=
          (up    ? analog_epsilon : 0.0f);
        io.NavInputs [ImGuiNavInput_LStickDown]  +=
          (down  ? analog_epsilon : 0.0f);
        io.NavInputs [ImGuiNavInput_LStickLeft]  +=
          (left  ? analog_epsilon : 0.0f);
        io.NavInputs [ImGuiNavInput_LStickRight] +=
          (right ? analog_epsilon : 0.0f);
      }

      if (! io.KeyCtrl)
      {
        io.NavInputs [ImGuiNavInput_DpadUp]          +=
          (up    ? analog_epsilon : 0.0f);
        io.NavInputs [ImGuiNavInput_DpadDown]        +=
          (down  ? analog_epsilon : 0.0f);
        io.NavInputs [ImGuiNavInput_DpadLeft]        +=
          (left  ? analog_epsilon : 0.0f);
        io.NavInputs [ImGuiNavInput_DpadRight]       +=
          (right ? analog_epsilon : 0.0f);
      }
    }

    if (io.KeysDown [VK_RETURN])
      io.NavInputs  [ImGuiNavInput_Activate] = LastNavInputs [ImGuiNavInput_Activate] + io.DeltaTime;

    if (io.KeysDown [VK_ESCAPE])
      io.NavInputs  [ImGuiNavInput_Cancel]   = LastNavInputs [ImGuiNavInput_Cancel] + io.DeltaTime;
  }

  else
    io.NavActive = false;


  if (io.NavActive)
  {
    io.NavInputs [ImGuiNavInput_FocusPrev] +=
      (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed (ImGuiKey_Tab, false))
                                                                ? 1.0f : 0.0f;
    io.NavInputs [ImGuiNavInput_FocusNext] +=
      (io.KeyCtrl && ImGui::IsKeyPressed (ImGuiKey_Tab, false)) ? 1.0f : 0.0f;
  }


  if (io.NavInputs [ImGuiNavInput_Activate] != 0.0f)
    io.MouseDown [4] = true;
  else
    io.MouseDown [4] = false;


  if ( io.NavInputs [ImGuiNavInput_TweakSlow] != 0.0f &&
       io.NavInputs [ImGuiNavInput_TweakFast] != 0.0f &&
       ( LastNavInputs [ImGuiNavInput_TweakSlow] == 0.0f ||
         LastNavInputs [ImGuiNavInput_TweakFast] == 0.0f )
     )
  {
    SK_ImGui_Widgets->hide_all = (! SK_ImGui_Widgets->hide_all);
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

        ImGuiContext& g     = *GImGui;
  const ImGuiStyle&   style = g.Style;
  const ImGuiID       id    = window->GetID (label);

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

  BeginChildFrame (
    id, total_bb.GetSize (),
      ImGuiWindowFlags_NoNav        | ImGuiWindowFlags_NoInputs     |
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground
  );

  ItemSize (total_bb, style.FramePadding.y);

  if (! ItemAdd (total_bb, (ImGuiID)0, &frame_bb))
  {
    EndChildFrame ();
    return;
  }

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
                  ImColor (GetStyleColorVec4 (ImGuiCol_FrameBg)),
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

      const ImColor col_base =
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

  EndChildFrame ();
}

struct SK_ImGuiPlotArrayGetterData
{
  const float* Values;
        int    Stride;

  SK_ImGuiPlotArrayGetterData (const float* values, int stride)
  {
    Values = values;
    Stride = stride;
  }
};

static float Plot_ArrayGetter (void* data, int idx)
{
  SK_ImGuiPlotArrayGetterData* plot_data =
    (SK_ImGuiPlotArrayGetterData *)data;

  const float v =
    *(const float *)(const void *)((const unsigned char *)plot_data->Values
                  + (size_t)idx * plot_data->Stride);

  return v;
}

void
ImGui::PlotLinesC ( const char*  label,         const float* values,
                          int    values_count,        int    values_offset,
                    const char*  overlay_text,        float  scale_min,
                                                      float  scale_max,
                          ImVec2 graph_size,          int    stride,
                          float  min_color_val,       float  max_color_val,
                          float  avg,                 bool   inverse )
{
  // Disable frame rounding or it will produce artifacts on graphs
  ImGui::PushStyleVar (ImGuiStyleVar_FrameRounding, 0.0f);

  SK_ImGuiPlotArrayGetterData data =
    SK_ImGuiPlotArrayGetterData (values, stride);

  PlotCEx ( ImGuiPlotType_Lines, label,        &Plot_ArrayGetter,
              reinterpret_cast <void *>(&data), values_count,
                                                values_offset,
                overlay_text,   scale_min, scale_max,
                  graph_size,
                    min_color_val, max_color_val,
                      avg,
                        inverse
          );

  ImGui::PopStyleVar ();
}


#include <SpecialK/control_panel.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>

static int64_t g_Time           = { };
static int64_t g_TicksPerSecond = { };

// Fallback mechanism if we do not have a working window message pump to
//   receive WM_MOUSEMOVE and WM_MOUSELEAVE messages on
void
SK_ImGui_FallbackTrackMouseEvent (POINT& cursor_pos)
{
  if (PtInRect (&game_window.actual.window,
                         cursor_pos))
  {
    struct {
      HWND  hWndTop;
      POINT cursor_pos;
    } static last;
  
    HWND hWndTop = last.hWndTop;
  
    if (cursor_pos.x != last.cursor_pos.x ||
        cursor_pos.y != last.cursor_pos.y)
    {
      if (SK_GetForegroundWindow () != game_window.hWnd)
      {
        last.hWndTop =
          WindowFromPoint (cursor_pos);
      }
      else
        last.hWndTop    = game_window.hWnd;
        last.cursor_pos =  cursor_pos;
    }
  
    hWndTop =
      last.hWndTop;

    game_window.mouse.inside =
      ( hWndTop == game_window.hWnd );
  
    if ( hWndTop != game_window.hWnd )
    {
      game_window.mouse.inside =
        IsChild (game_window.hWnd, hWndTop);
  
      if (! game_window.mouse.inside)
      {
        POINT                     client_pos = cursor_pos;
        ScreenToClient (hWndTop, &client_pos);
  
        HWND hWndChild =
          ChildWindowFromPointEx (
                        hWndTop,  client_pos, CWP_SKIPINVISIBLE |
                                              CWP_SKIPTRANSPARENT
                                 );
  
        if (hWndChild == game_window.hWnd)
        {
          game_window.mouse.inside = true;
        }
      }
    }
  }

  else
    game_window.mouse.inside = false;


  if (! game_window.mouse.inside)
    ImGui::GetIO ().MousePos = ImVec2 (-FLT_MAX, -FLT_MAX);
}


void
SK_ImGui_User_NewFrame (void)
{
  SK_HID_ProcessGamepadButtonBindings ();

  __SK_EnableSetCursor = true;

  static auto& io =
    ImGui::GetIO ();

  // Setup time step
  auto current_time =
    SK_QueryPerf ().QuadPart;

  double delta =
    static_cast <double> (                       current_time -
                          std::exchange (g_Time, current_time)
                         );

  static bool        first = true;
  if (std::exchange (first, false))
  {
    io.BackendUsingLegacyKeyArrays     = (ImS8)1;
    io.BackendUsingLegacyNavInputArray = true;

    g_TicksPerSecond = SK_PerfFreq;
    g_Time           = current_time;

    delta = 0.0;

    io.KeyMap [ImGuiKey_Tab]            = VK_TAB;
    io.KeyMap [ImGuiKey_LeftArrow]      = VK_LEFT;
    io.KeyMap [ImGuiKey_RightArrow]     = VK_RIGHT;
    io.KeyMap [ImGuiKey_UpArrow]        = VK_UP;
    io.KeyMap [ImGuiKey_DownArrow]      = VK_DOWN;
    io.KeyMap [ImGuiKey_PageUp]         = VK_PRIOR;
    io.KeyMap [ImGuiKey_PageDown]       = VK_NEXT;
    io.KeyMap [ImGuiKey_Home]           = VK_HOME;
    io.KeyMap [ImGuiKey_End]            = VK_END;
    io.KeyMap [ImGuiKey_Insert]         = VK_INSERT;
    io.KeyMap [ImGuiKey_Delete]         = VK_DELETE;
    io.KeyMap [ImGuiKey_Backspace]      = VK_BACK;
    io.KeyMap [ImGuiKey_Space]          = VK_SPACE;
    io.KeyMap [ImGuiKey_Enter]          = VK_RETURN;
    io.KeyMap [ImGuiKey_Escape]         = VK_ESCAPE;
    io.KeyMap [ImGuiKey_ScrollLock]     = VK_SCROLL;
    io.KeyMap [ImGuiKey_PrintScreen]    = VK_PRINT;
    io.KeyMap [ImGuiKey_Pause]          = VK_PAUSE;
    io.KeyMap [ImGuiKey_CapsLock]       = VK_CAPITAL;
    io.KeyMap [ImGuiKey_NumLock]        = VK_NUMLOCK;
    io.KeyMap [ImGuiKey_Comma]          = VK_OEM_COMMA;
    io.KeyMap [ImGuiKey_Minus]          = VK_OEM_MINUS;
    io.KeyMap [ImGuiKey_Period]         = VK_OEM_PERIOD;
    io.KeyMap [ImGuiKey_Equal]          = VK_OEM_PLUS;
    io.KeyMap [ImGuiKey_Semicolon]      = VK_OEM_1;
    io.KeyMap [ImGuiKey_Slash]          = VK_OEM_2;
    io.KeyMap [ImGuiKey_GraveAccent]    = VK_OEM_3;
    io.KeyMap [ImGuiKey_LeftBracket]    = VK_OEM_4;
    io.KeyMap [ImGuiKey_Backslash]      = VK_OEM_5;
    io.KeyMap [ImGuiKey_RightBracket]   = VK_OEM_6;
    io.KeyMap [ImGuiKey_Apostrophe]     = VK_OEM_7;
    io.KeyMap [ImGuiKey_KeypadDivide]   = VK_DIVIDE;
    io.KeyMap [ImGuiKey_KeypadMultiply] = VK_MULTIPLY;
    io.KeyMap [ImGuiKey_KeypadSubtract] = VK_SUBTRACT;
    io.KeyMap [ImGuiKey_KeypadDecimal]  = VK_DECIMAL;
    io.KeyMap [ImGuiKey_KeypadAdd]      = VK_ADD;
    io.KeyMap [ImGuiKey_KeypadEqual]    = VK_OEM_PLUS;
    io.KeyMap [ImGuiKey_Keypad0]        = VK_NUMPAD0;
    io.KeyMap [ImGuiKey_Keypad1]        = VK_NUMPAD1;
    io.KeyMap [ImGuiKey_Keypad2]        = VK_NUMPAD2;
    io.KeyMap [ImGuiKey_Keypad3]        = VK_NUMPAD3;
    io.KeyMap [ImGuiKey_Keypad4]        = VK_NUMPAD4;
    io.KeyMap [ImGuiKey_Keypad5]        = VK_NUMPAD5;
    io.KeyMap [ImGuiKey_Keypad6]        = VK_NUMPAD6;
    io.KeyMap [ImGuiKey_Keypad7]        = VK_NUMPAD7;
    io.KeyMap [ImGuiKey_Keypad8]        = VK_NUMPAD8;
    io.KeyMap [ImGuiKey_Keypad9]        = VK_NUMPAD9;
    io.KeyMap [ImGuiKey_F1]             = VK_F1;
    io.KeyMap [ImGuiKey_F2]             = VK_F2;
    io.KeyMap [ImGuiKey_F3]             = VK_F3;
    io.KeyMap [ImGuiKey_F4]             = VK_F4;
    io.KeyMap [ImGuiKey_F5]             = VK_F5;
    io.KeyMap [ImGuiKey_F6]             = VK_F6;
    io.KeyMap [ImGuiKey_F7]             = VK_F7;
    io.KeyMap [ImGuiKey_F8]             = VK_F8;
    io.KeyMap [ImGuiKey_F9]             = VK_F9;
    io.KeyMap [ImGuiKey_F10]            = VK_F10;
    io.KeyMap [ImGuiKey_F11]            = VK_F11;
    io.KeyMap [ImGuiKey_F12]            = VK_F12;
    io.KeyMap [ImGuiKey_F13]            = VK_F13;
    io.KeyMap [ImGuiKey_F14]            = VK_F14;
    io.KeyMap [ImGuiKey_F15]            = VK_F15;
    io.KeyMap [ImGuiKey_F16]            = VK_F16;
    io.KeyMap [ImGuiKey_F17]            = VK_F17;
    io.KeyMap [ImGuiKey_F18]            = VK_F18;
    io.KeyMap [ImGuiKey_F19]            = VK_F19;
    io.KeyMap [ImGuiKey_F20]            = VK_F20;
    io.KeyMap [ImGuiKey_F21]            = VK_F21;
    io.KeyMap [ImGuiKey_F22]            = VK_F22;
    io.KeyMap [ImGuiKey_F23]            = VK_F23;
    io.KeyMap [ImGuiKey_F24]            = VK_F24;
    io.KeyMap [ImGuiKey_0]              = '0';
    io.KeyMap [ImGuiKey_1]              = '1';
    io.KeyMap [ImGuiKey_2]              = '2';
    io.KeyMap [ImGuiKey_3]              = '3';
    io.KeyMap [ImGuiKey_4]              = '4';
    io.KeyMap [ImGuiKey_5]              = '5';
    io.KeyMap [ImGuiKey_6]              = '6';
    io.KeyMap [ImGuiKey_7]              = '7';
    io.KeyMap [ImGuiKey_8]              = '8';
    io.KeyMap [ImGuiKey_9]              = '9';
    io.KeyMap [ImGuiKey_A]              = 'A';
    io.KeyMap [ImGuiKey_B]              = 'B';
    io.KeyMap [ImGuiKey_C]              = 'C';
    io.KeyMap [ImGuiKey_D]              = 'D';
    io.KeyMap [ImGuiKey_E]              = 'E';
    io.KeyMap [ImGuiKey_F]              = 'F';
    io.KeyMap [ImGuiKey_G]              = 'G';
    io.KeyMap [ImGuiKey_H]              = 'H';
    io.KeyMap [ImGuiKey_I]              = 'I';
    io.KeyMap [ImGuiKey_J]              = 'J';
    io.KeyMap [ImGuiKey_K]              = 'K';
    io.KeyMap [ImGuiKey_L]              = 'L';
    io.KeyMap [ImGuiKey_M]              = 'M';
    io.KeyMap [ImGuiKey_N]              = 'N';
    io.KeyMap [ImGuiKey_O]              = 'O';
    io.KeyMap [ImGuiKey_P]              = 'P';
    io.KeyMap [ImGuiKey_Q]              = 'Q';
    io.KeyMap [ImGuiKey_R]              = 'R';
    io.KeyMap [ImGuiKey_S]              = 'S';
    io.KeyMap [ImGuiKey_T]              = 'T';
    io.KeyMap [ImGuiKey_U]              = 'U';
    io.KeyMap [ImGuiKey_V]              = 'V';
    io.KeyMap [ImGuiKey_W]              = 'W';
    io.KeyMap [ImGuiKey_X]              = 'X';
    io.KeyMap [ImGuiKey_Y]              = 'Y';
    io.KeyMap [ImGuiKey_Z]              = 'Z';
    io.KeyMap [ImGuiKey_LeftShift]      = VK_LSHIFT;
    io.KeyMap [ImGuiKey_RightShift]     = VK_RSHIFT;
    io.KeyMap [ImGuiKey_LeftAlt]        = VK_LMENU;
    io.KeyMap [ImGuiKey_RightAlt]       = VK_RMENU;
    io.KeyMap [ImGuiKey_LeftCtrl]       = VK_LCONTROL;
    io.KeyMap [ImGuiKey_RightCtrl]      = VK_RCONTROL;
    io.KeyMap [ImGuiKey_LeftSuper]      = VK_LWIN;
    io.KeyMap [ImGuiKey_RightSuper]     = VK_RWIN;
    io.KeyMap [ImGuiKey_Menu]           = VK_APPS;
  }

  static auto ticks_per_sec =
    static_cast <double> (g_TicksPerSecond);

  io.DeltaTime =
    (delta <= 0.0) ?
              0.0f :
    std::min ( 0.333f,
    std::max ( 0.0f, static_cast <float> ( delta / ticks_per_sec ) ) );

  if ( io.DisplaySize.x <= 0.0f ||
       io.DisplaySize.y <= 0.0f )
  {
    io.DisplaySize.x = 128.0f;
    io.DisplaySize.y = 128.0f;
  }

  ImGuiContext& g = *GImGui;

  g.Style.AntiAliasedLines = config.imgui.render.antialias_lines;
  g.Style.AntiAliasedFill  = config.imgui.render.antialias_contours;


  //
  // Idle Cursor Detection  (when UI is visible, but mouse does not require capture)
  //
  //          Remove the cursor after a brief timeout period (500 ms),
  //            it will come back if moved ;)
  //
  static int last_x = SK_ImGui_Cursor.pos.x;
  static int last_y = SK_ImGui_Cursor.pos.y;


  static            LASTINPUTINFO
    lii = { sizeof (LASTINPUTINFO), 1 };
                    LASTINPUTINFO
    cii = { sizeof (LASTINPUTINFO), 2 };

  bool new_input =
    ( !GetLastInputInfo (&cii) ||
          std::exchange ( lii.dwTime,
                          cii.dwTime ) != cii.dwTime );

  // Hacky solution for missed messages causing no change in cursor pos
  //
  POINT             cursor_pos = { };
  SK_GetCursorPos (&cursor_pos);


  if ((! game_window.mouse.can_track) || game_window.mouse.last_move_msg < SK::ControlPanel::current_time - 500UL) // No Hover / Leave Tracking
  {
    // While inside, we get WM_MOUSEMOVE, while outside we get ... nothing.
    if (new_input || game_window.mouse.inside)
    {
      SK_ImGui_FallbackTrackMouseEvent (cursor_pos);
    }
  }

  // Check if the mouse is within the actual render area (device HWND)
  //   in order to stop processing mouse clicks in things like emulators
  //     with multiple windows and drop-down menus that may overlap SK's
  //       control panel...
  const auto& windows =
    SK_GetCurrentRenderBackend ().windows;

  const HWND
    hWndDevice = windows.device.hwnd,
    hWndFocus  = windows.focus. hwnd,
    hWndGame   =    game_window.hWnd,
    hWndMouse0 =
       hWndDevice != 0 && IsWindow (hWndDevice) ? hWndDevice : nullptr,
    hWndMouse1 =
       hWndFocus  != 0 && IsWindow (hWndFocus) &&
                      GetTopWindow (hWndFocus) == hWndDevice ? hWndFocus 
                                                             : hWndGame != hWndFocus ?
                                                               hWndGame              : nullptr;

  if ( game_window.mouse.inside &&
        ( SK_GetForegroundWindow ()    == hWndMouse0   ||
          WindowFromPoint (cursor_pos) == hWndMouse0 ) ||
        ( SK_GetForegroundWindow ()    == hWndMouse1   ||
          WindowFromPoint (cursor_pos) == hWndMouse1 ) )
  {
    SK_ImGui_Cursor.ScreenToLocal (&cursor_pos);

    if ( cursor_pos.x != last_x ||
         cursor_pos.y != last_y )
    {
      if ( abs (SK_ImGui_Cursor.pos.x - cursor_pos.x) > 3 ||
           abs (SK_ImGui_Cursor.pos.y - cursor_pos.y) > 3 )
      {
        SK_ImGui_Cursor.pos = cursor_pos;
      }
    }

    if ( SK_ImGui_Cursor.pos.x != last_x ||
         SK_ImGui_Cursor.pos.y != last_y )
    {
      io.MousePos.x = static_cast <float> (SK_ImGui_Cursor.pos.x);
      io.MousePos.y = static_cast <float> (SK_ImGui_Cursor.pos.y);
    }
  }

  // Cursor stops being treated as idle when it's not in the game window :)
  else {
    io.MousePos               = ImVec2 (-FLT_MAX, -FLT_MAX);
    SK_ImGui_Cursor.last_move = SK::ControlPanel::current_time;
    game_window.mouse.inside  = false;
  }

  const bool bActive =
    SK_IsGameWindowActive ();

  if (bActive && new_input)
  {
    for (UINT i = 7 ; i < 255 ; ++i)
    {
      io.KeysDown [i] =
        ((SK_GetAsyncKeyState (i) & 0x8000) != 0x0);
    }
  }

  if (! bActive)
    RtlZeroMemory (&io.KeysDown [7], sizeof (bool) * 248);

  if (game_window.mouse.inside)
  {
    io.MouseDown [0] = ((SK_GetAsyncKeyState (VK_LBUTTON) ) & 0x8000) != 0x0;
    io.MouseDown [1] = ((SK_GetAsyncKeyState (VK_RBUTTON) ) & 0x8000) != 0x0;
    io.MouseDown [2] = ((SK_GetAsyncKeyState (VK_MBUTTON) ) & 0x8000) != 0x0;
    io.MouseDown [3] = ((SK_GetAsyncKeyState (VK_XBUTTON1)) & 0x8000) != 0x0;
    io.MouseDown [4] = ((SK_GetAsyncKeyState (VK_XBUTTON2)) & 0x8000) != 0x0;
  }

  SK_ImGui_PollGamepad ();

  // Read keyboard modifiers inputs
  io.KeyCtrl   = (io.KeysDown [VK_CONTROL]) != 0;
  io.KeyShift  = (io.KeysDown [VK_SHIFT])   != 0;
  io.KeyAlt    = (io.KeysDown [VK_MENU])    != 0;

  io.KeySuper  = false;

  if (nav_usable)
  {
    io.ConfigFlags  |= ( ImGuiConfigFlags_NavEnableKeyboard |
                         ImGuiConfigFlags_NavEnableGamepad  );
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
  }

  else
  {
    io.ConfigFlags  &= ~ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags  &= ~ImGuiConfigFlags_NavEnableGamepad;
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
  }

  
  // Mouse input should be swallowed because it interacts with ImGui;
  //   regular mouse capture includes swallowing input for "Disabled to Game".
  bool bWantMouseCaptureForUI =
    SK_ImGui_WantMouseCaptureEx (0x0) && (SK_ImGui_IsAnythingHovered () || ImGui::IsPopupOpen (nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel));
    //SK_ImGui_WantMouseCaptureEx (0x0);

  if ( abs (last_x - SK_ImGui_Cursor.pos.x) > 3 ||
       abs (last_y - SK_ImGui_Cursor.pos.y) > 3 ||
          bWantMouseCaptureForUI )
  {
    SK_ImGui_Cursor.last_move = SK::ControlPanel::current_time;
                    last_x    = SK_ImGui_Cursor.pos.x;
                    last_y    = SK_ImGui_Cursor.pos.y;
  }


  if (bWantMouseCaptureForUI || (SK_ImGui_Active () && SK_ImGui_Cursor.last_move > SK::ControlPanel::current_time - 500))
    SK_ImGui_Cursor.idle = false;

  else
    SK_ImGui_Cursor.idle = ( (! bWantMouseCaptureForUI) || config.input.mouse.disabled_to_game == SK_InputEnablement::Disabled );
                                                        // Disabled to game is a form of capture,
                                                        //   but it is exempt from idle cursor logic

  if (config.input.cursor.manage && config.input.cursor.gamepad_deactivates && SK_Window_IsCursorActive ())
  {
    extern XINPUT_STATE
              hid_to_xi;
    static XINPUT_STATE
              hid_state = { };

    DWORD dwLastPacket =
      hid_state.dwPacketNumber;

    hid_state.dwPacketNumber =
    hid_to_xi.dwPacketNumber;

    if (hid_to_xi.dwPacketNumber > dwLastPacket)
    {
      if ( memcmp ( &hid_to_xi.Gamepad.wButtons,
                    &hid_state.Gamepad.wButtons, sizeof (WORD) ) ||
                abs (hid_to_xi.Gamepad.sThumbLX      - hid_state.Gamepad.sThumbLX     ) > 9000 ||
                abs (hid_to_xi.Gamepad.sThumbLY      - hid_state.Gamepad.sThumbLY     ) > 9000 ||
                abs (hid_to_xi.Gamepad.sThumbRX      - hid_state.Gamepad.sThumbRX     ) > 9000 ||
                abs (hid_to_xi.Gamepad.sThumbRY      - hid_state.Gamepad.sThumbRY     ) > 9000 ||
                abs (hid_to_xi.Gamepad.bLeftTrigger  - hid_state.Gamepad.bLeftTrigger ) > 40   ||
                abs (hid_to_xi.Gamepad.bRightTrigger - hid_state.Gamepad.bRightTrigger) > 40 )
      {
        if (! bWantMouseCaptureForUI)
        {
          SK_ImGui_Cursor.idle      = true;
          SK_ImGui_Cursor.last_move = SK::ControlPanel::current_time - 500;
          SK_Window_DeactivateCursor (true);
        }
      }

      hid_state.Gamepad = hid_to_xi.Gamepad;
    }

    else
    {
      static XINPUT_STATE
                 xi_state_last = { };
             XINPUT_STATE
                 xi_state      = { };

      dwLastPacket =
        xi_state_last.dwPacketNumber;

      bool bPolled =
        SK_XInput_PollController (0, &xi_state);

      if (bPolled && xi_state.dwPacketNumber > dwLastPacket)
      {
        if ( memcmp ( &xi_state.     Gamepad.wButtons,
                      &xi_state_last.Gamepad.wButtons, sizeof (WORD) ) ||
                  abs (xi_state.Gamepad.sThumbLX      - xi_state_last.Gamepad.sThumbLX     ) > 9000 ||
                  abs (xi_state.Gamepad.sThumbLY      - xi_state_last.Gamepad.sThumbLY     ) > 9000 ||
                  abs (xi_state.Gamepad.sThumbRX      - xi_state_last.Gamepad.sThumbRX     ) > 9000 ||
                  abs (xi_state.Gamepad.sThumbRY      - xi_state_last.Gamepad.sThumbRY     ) > 9000 ||
                  abs (xi_state.Gamepad.bLeftTrigger  - xi_state_last.Gamepad.bLeftTrigger ) > 40   ||
                  abs (xi_state.Gamepad.bRightTrigger - xi_state_last.Gamepad.bRightTrigger) > 40 )
        {
          if (! bWantMouseCaptureForUI)
          {
            SK_ImGui_Cursor.idle      = true;
            SK_ImGui_Cursor.last_move = SK::ControlPanel::current_time - 500;
            SK_Window_DeactivateCursor (true);
          }
        }

        xi_state_last = xi_state;
      }
    }
  }


  const bool bManageCursor =
    (config.input.cursor.manage || SK_ImGui_Cursor.force != sk_cursor_state::None);

  if (bManageCursor && game_window.mouse.inside)
  {
    if (SK_Window_IsCursorActive () && SK_ImGui_Cursor.force != sk_cursor_state::Hidden)
    {
      if (! SK_InputUtil_IsHWCursorVisible ())
      {
        if ( 0 != SK_GetSystemMetrics (SM_MOUSEPRESENT) )
        {
          SK_SendMsgShowCursor (TRUE);
      }
        }
    }

    else if (config.input.cursor.manage || SK_ImGui_Cursor.force == sk_cursor_state::Hidden)
    {
      if (SK_InputUtil_IsHWCursorVisible ())
      {
        if ( 0 != SK_GetSystemMetrics (SM_MOUSEPRESENT) )
        {
          SK_SendMsgShowCursor (FALSE);
        }

        SK_SendMsgSetCursor (nullptr);
      }
    }
  }


  if (! SK_ImGui_Cursor.idle)
  {
    if (SK_ImGui_WantMouseCapture () && SK_ImGui_IsAnythingHovered ())
    {
      SK_SendMsgSetCursor (ImGui_DesiredCursor ());
    }

    io.MouseDrawCursor =
      (! SK_InputUtil_IsHWCursorVisible ());
  }

  else
    io.MouseDrawCursor = false;

  
  if (bActive)
  {
    if (SK_ImGui_Active () || SK_ImGui_WantMouseCapture ())
      SK_ClipCursor (config.window.confine_cursor ?
                      &game_window.actual.window  : nullptr);
    else if (config.window.unconfine_cursor)
      SK_ClipCursor (nullptr);
    else if (config.window.confine_cursor)
      SK_ClipCursor (&game_window.actual.window);
    else
      SK_ClipCursor (&game_window.cursor_clip);
  }


  ImGui::NewFrame ();


  __SK_EnableSetCursor = false;

  if (bActive && new_input)
  {
#if 0
    for (UINT i = 511 ; i < ImGuiKey_COUNT ; ++i)
    {
      void CALLBACK SK_PluginKeyPress   (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode);
      int           SK_HandleConsoleKey (bool keyDown, BYTE vkCode, LPARAM lParam);

      ImGuiKey       key = (ImGuiKey)i;
      if (io.KeyMap [key] != 0)
      {
        auto key_data =
          ImGui::GetKeyData (key);

        if (key_data->DownDuration !=
            key_data->DownDurationPrev)
        {
          bool newly_pressed = ImGui::IsKeyPressed (key, false);

          if (newly_pressed)
          {
            //SK_PluginKeyPress (io.KeyCtrl, io.KeyShift, io.KeyAlt, (BYTE)io.KeyMap [i]);
          }
        }
      }
    }
#endif

    if (config.input.keyboard.catch_alt_f4)
    {
      if ( ImGui::IsKeyPressed (ImGuiKey_F4,   false) &&
           ImGui::IsKeyPressed (ImGuiKey_Menu, false) &&
         ( ImGui::GetKeyData   (ImGuiKey_F4  )->DownDuration == 0.0f ||
           ImGui::GetKeyData   (ImGuiKey_Menu)->DownDuration == 0.0f ) )
      {
        WriteULong64Release (
          &config.input.keyboard.temporarily_allow,
            SK_GetFramesDrawn () + 40
        );

        SK_ImGui_WantExit = true;
      }
    }
  }

  //if (eula.show)
    SK_ImGui_DrawEULA (&eula);

  if (new_input)
  {
    last_x = SK_ImGui_Cursor.pos.x;
    last_y = SK_ImGui_Cursor.pos.y;

    // Certain features (i.e. Render in Background) would swallow mouse events
    //   involved in window activation, so we need to activate the window.
    if (game_window.mouse.inside && io.MouseDown [0] && SK_ImGui_WantMouseCapture ())
    {
      if (! game_window.active)
      {
        POINT             ptCursor = { };
        SK_GetCursorPos (&ptCursor);

        if ( game_window.hWnd ==
               WindowFromPoint ( cursor_pos ) )
        {
        //game_window.active = true;

          SetWindowPos ( game_window.hWnd, SK_GetForegroundWindow (),
                           0, 0,
                           0, 0,
                             SWP_NOMOVE | SWP_NOSIZE |
                             SWP_ASYNCWINDOWPOS );
        }
      }
    }
  }

  SK_ImGui_ExemptOverlaysFromKeyboardCapture ();

  // Warn on low gamepad battery
  SK_Battery_UpdateRemainingPowerForAllDevices ();
}

bool
SK_ImGui_IsEULAVisible (void)
{
  return eula.show;
}