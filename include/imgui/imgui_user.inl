#pragma once

#include <mmsystem.h>
#include <Windows.h>

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

extern bool           SK_ImGui_IsMouseRelevant (void);

extern HWND           SK_GetParentWindow    (HWND);
extern bool __stdcall SK_IsGameWindowActive (void);

extern void __stdcall SK_ImGui_DrawEULA (LPVOID reserved);
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


extern float analog_sensitivity;

#include <set>
#include <SpecialK/log.h>

extern bool SK_WantBackgroundRender (void);

#define SK_RAWINPUT_READ(type)  SK_RawInput_Backend->markRead  (type);
#define SK_RAWINPUT_WRITE(type) SK_RawInput_Backend->markWrite (type);

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
        if (*pcbSize >= size)
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

            if ( ((! self) && (! already_processed))
                           && uiCommand == RID_INPUT &&
                  (! filter) )
            {
              SK_RAWINPUT_READ (sk_input_dev_type::Mouse)

              SK_RawInput_Backend->viewed.mouse = SK_QueryPerf ().QuadPart;
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


            if ( ((! self) && (! already_processed))
                           && uiCommand == RID_INPUT
                           && (! filter) )
            {
              SK_RAWINPUT_READ (sk_input_dev_type::Keyboard)

              SK_RawInput_Backend->viewed.keyboard = SK_QueryPerf ().QuadPart;
            }

        //// Leads to double-input processing, left here in case Legacy Messages are disabled and this is needed
        ////
        ////if (!(((RAWINPUT *) pData)->data.keyboard.Flags & RI_KEY_BREAK))
        ////{
        ////  pConsole->KeyDown (VKey & 0xFF, MAXDWORD);
        ////        io.KeysDown [VKey & 0xFF] = focus;
        ////}

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

              SK_RawInput_Backend->viewed.gamepad = SK_QueryPerf ().QuadPart;
            }
          } break;
        }

        return filter;
      };

  filter = (! self) &&
    FilterRawInput (uiCommand, (RAWINPUT *)pData, mouse, keyboard);


  if (uiCommand == RID_INPUT /*&& SK_ImGui_Visible*/)
  {
    switch (((RAWINPUT *)pData)->header.dwType)
    {
      case RIM_TYPEMOUSE:
      {
        if (foreground)
        {
          if (SK_ImGui_IsMouseRelevant () && config.input.mouse.add_relative_motion)
          {
            ///////////////////////// 99% of games don't need this, and if we use relative motion to update the cursor position that
            /////////////////////////   requires re-synchronizing with the desktop's logical cursor coordinates at some point because
            /////////////////////////     Raw Input does not include cursor acceleration, etc.
            ///////////////////////POINT client { ((RAWINPUT *)pData)->data.mouse.lLastX,
            ///////////////////////               ((RAWINPUT *)pData)->data.mouse.lLastY };
            ///////////////////////
            ///////////////////////////SK_ImGui_Cursor.ClientToLocal (&client);
            ///////////////////////
            ///////////////////////SK_ImGui_Cursor.pos.x += client.x;
            ///////////////////////SK_ImGui_Cursor.pos.y += client.y;
            ///////////////////////
            ///////////////////////io.MousePos.x = (float)SK_ImGui_Cursor.pos.x;
            ///////////////////////io.MousePos.y = (float)SK_ImGui_Cursor.pos.y;
          }

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
      RtlZeroMemory (&((RAWINPUT *)pData)->data.mouse, *pcbSize - sizeof (RAWINPUTHEADER));
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
       ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () ) )
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



extern UINT
SK_Input_ClassifyRawInput ( HRAWINPUT lParam, bool& mouse,
                                              bool& keyboard,
                                              bool& gamepad );

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
          SK_ImGui_WantGamepadCapture  ();

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

extern bool SK_Input_DetermineMouseIdleState (MSG * lpMsg);
extern bool SK_Window_IsCursorActive         (void);

LRESULT
WINAPI
ImGui_WndProcHandler ( HWND   hWnd,    UINT  msg,
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

      if ( ( hWnd == game_window.hWnd ||
             hWnd == game_window.child ) && HIWORD (lParam) == WM_MOUSEMOVE && ( mousePos.x != lastMouse.x ||
                                                                                 mousePos.y != lastMouse.y ) )
      {
        static LONG        lastTime = 0;
        if (std::exchange (lastTime, messageTime) != messageTime)
        {
          static HCURSOR hLastClassCursor = (HCURSOR)(-1);

          if ( SK_ImGui_WantMouseCapture () &&
                  ImGui::IsWindowHovered (ImGuiHoveredFlags_AnyWindow) )
          {
            if (hLastClassCursor == (HCURSOR)(-1))
                hLastClassCursor  = (HCURSOR)GetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR);

            HCURSOR ImGui_DesiredCursor (void);

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
              extern bool SK_Input_DetermineMouseIdleState (MSG * lpMsg);

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

  extern bool
  SK_ImGui_WantExit;

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
              SK_ImGui_WantExit = true;
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

      case SC_SCREENSAVE:
      case SC_MONITORPOWER:
        //SK_LOG0 ( ( L"ImGui ImGui Examined SysCmd (SC_SCREENSAVE) or (SC_MONITORPOWER)" ),
        //            L"Window Mgr" );
        if (config.window.disable_screensaver)
        {
          if (lParam != -1) // -1 == Monitor Power On, we do not want to block that!
            return 1;
        }

        if (SK_IsGameWindowActive ())
        {
          if (LOWORD (wParam & 0xFFF0) == SC_MONITORPOWER)
          {
            if (lParam != -1) // No power saving when active window, that's silly
              return 1;
          }
        }

        if (lParam == -1) // Pass it through, always
          DefWindowProcW (hWnd, msg, wParam, lParam);

        return 0;

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

          static constexpr GUID GUID_DEVINTERFACE_HID =
            { 0x4D1E55B2L, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

          static constexpr GUID GUID_XUSB_INTERFACE_CLASS =
            { 0xEC87F1E3L, 0xC13B, 0x4100, { 0xB5, 0xF7, 0x8B, 0x84, 0xD5, 0x42, 0x60, 0xCB } };

          if (IsEqualGUID (pDevW->dbcc_classguid, GUID_DEVINTERFACE_HID) ||
              IsEqualGUID (pDevW->dbcc_classguid, GUID_XUSB_INTERFACE_CLASS))
          {
            bool xinput = IsEqualGUID (pDevW->dbcc_classguid, GUID_XUSB_INTERFACE_CLASS);

            if (     pDevW->dbcc_size == sizeof (DEV_BROADCAST_DEVICEINTERFACE_W))
              xinput |= wcsstr (pDevW->dbcc_name, L"IG_") != nullptr;
            else if (pDevA->dbcc_size == sizeof (DEV_BROADCAST_DEVICEINTERFACE_A))
              xinput |= strstr (pDevA->dbcc_name,  "IG_") != nullptr;

            if (xinput)
            {
              if ( config.input.gamepad.xinput.placehold [0] || config.input.gamepad.xinput.placehold [1] ||
                   config.input.gamepad.xinput.placehold [2] || config.input.gamepad.xinput.placehold [3] )
              {
                extern void SK_XInput_Refresh (UINT iJoyID);

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


float analog_sensitivity = 0.00333f;
bool  nav_usable         = false;

bool
_Success_(false)
SK_ImGui_FilterXInput (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  bool disable =
    config.input.gamepad.disabled_to_game == SK_InputEnablement::Disabled ||
      ( SK_ImGui_WantGamepadCapture ()   &&
        dwUserIndex == (DWORD)config.input.gamepad.xinput.ui_slot );

  if (disable)
  {
    RtlSecureZeroMemory (&pState->Gamepad, sizeof (XINPUT_GAMEPAD));

    // SDL Keepalive
    pState->dwPacketNumber =
      std::max (1UL, pState->dwPacketNumber);

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
    RtlSecureZeroMemory (&pKeystroke->Flags, sizeof (pKeystroke->Flags));

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
SK_ImGui_ToggleEx ( bool& toggle_ui,
                    bool& toggle_nav )
{
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

  return SK_ImGui_Active ();
}

#include <SpecialK/input/dinput8_backend.h>
#include <SpecialK/input/steam.h>

extern IDirectInputDevice8W_GetDeviceState_pfn
        IDirectInputDevice8W_GetDeviceState_Original;

extern XINPUT_STATE  di8_to_xi;
extern XINPUT_STATE  joy_to_xi;
extern XINPUT_STATE  sce_to_xi;

using joyGetNumDevs_pfn  = UINT (WINAPI *)(void);
using joyGetPosEx_pfn    = UINT (WINAPI *)(UINT,LPJOYINFOEX);
using joyGetDevCapsW_pfn = UINT (WINAPI *)(UINT_PTR,LPJOYCAPSW,UINT);

UINT
WINAPI
SK_joyGetPosEx ( _In_  UINT        uJoyID,
                 _Out_ LPJOYINFOEX pji )
{
  static HMODULE hModWinMM =
    LoadLibraryEx ( L"winmm.dll", nullptr,
                      LOAD_LIBRARY_SEARCH_SYSTEM32 );

  static  joyGetPosEx_pfn
         _joyGetPosEx =
         (joyGetPosEx_pfn)SK_GetProcAddress (hModWinMM,
         "joyGetPosEx"                      );

  return
    _joyGetPosEx (uJoyID, pji);
}

UINT
WINAPI
SK_joyGetDevCapsW ( _In_                     UINT_PTR   uJoyID,
                    _Out_writes_bytes_(cbjc) LPJOYCAPSW pjc,
                    _In_                     UINT       cbjc )
{
  static HMODULE hModWinMM =
    LoadLibraryEx ( L"winmm.dll", nullptr,
                      LOAD_LIBRARY_SEARCH_SYSTEM32 );

  static  joyGetDevCapsW_pfn
         _joyGetDevCapsW =
         (joyGetDevCapsW_pfn)SK_GetProcAddress (hModWinMM,
         "joyGetDevCapsW"                      );

  return
    _joyGetDevCapsW (uJoyID, pjc, cbjc);
}

UINT
WINAPI
SK_joyGetNumDevs (void)
{
  static HMODULE hModWinMM =
    LoadLibraryEx ( L"winmm.dll", nullptr,
                      LOAD_LIBRARY_SEARCH_SYSTEM32 );

  static  joyGetNumDevs_pfn
         _joyGetNumDevs =
         (joyGetNumDevs_pfn)SK_GetProcAddress (hModWinMM,
         "joyGetNumDevs"                      );

  return
    _joyGetNumDevs ();
}

bool
SK_ImGui_PollGamepad_EndFrame (XINPUT_STATE& state)
{
  // Triggers SteamInput kill switch once per-frame
  SK_ImGui_WantGamepadCapture ();

  static auto& io =
    ImGui::GetIO ();

  bool bToggleNav = false,
       bToggleVis = false;

  extern bool __stdcall
       SK_IsGameWindowActive (void);

  // Reset Mouse / Keyboard State so that we can process all state transitions
  //   that occur during the next frame without losing any input events.
  if ( SK_IsGameWindowActive(    ) )
  {
    bool Alt =
      (SK_GetAsyncKeyState (VK_MENU)    ) != 0;
    bool Ctrl =
      (SK_GetAsyncKeyState (VK_CONTROL) ) != 0;
    bool Shift =
      (SK_GetAsyncKeyState (VK_SHIFT)   ) != 0;

    io.KeyAlt   = Alt;
    io.KeyShift = Shift;
    io.KeyCtrl  = Ctrl;

    bToggleNav |=
       ( SK_ImGui_Visible                 &&
         io.KeysDown         [VK_CAPITAL] &&
         io.KeysDownDuration [VK_CAPITAL] == 0.0f );

    bToggleVis |=
       ( io.KeyCtrl                    &&
         io.KeyShift                   &&
         io.KeysDown         [VK_BACK] &&
         io.KeysDownDuration [VK_BACK] == 0.0f );
  }

  bool bUseGamepad =
    SK_IsGameWindowActive () ||
      ( config.window.background_render &&
        config.input.gamepad.disabled_to_game != SK_InputEnablement::DisabledInBackground );

  bool bRet = false;

  static XINPUT_STATE last_state = { 1, 0 };

  if (bUseGamepad)
  {
    const bool api_bridge =
      config.input.gamepad.native_ps4;

#ifdef SK_STEAM_CONTROLLER_SUPPORT
    api_bridge |= ( ControllerPresent (config.input.gamepad.steam.ui_slot) );
#endif

    if (api_bridge)
    {
      // Translate DirectInput to XInput, because I'm not writing multiple controller codepaths
      //   for no good reason.
      JOYINFOEX joy_ex   { };
      JOYCAPSW  joy_caps { };
    
      joy_ex.dwSize  = sizeof (JOYINFOEX);
      joy_ex.dwFlags = JOY_RETURNALL      | JOY_RETURNPOVCTS |
                       JOY_RETURNCENTERED | JOY_USEDEADZONE;
    
      SK_joyGetPosEx    (JOYSTICKID1, &joy_ex);
      SK_joyGetDevCapsW (JOYSTICKID1, &joy_caps, sizeof (JOYCAPSW));
    
      SK_JOY_TranslateToXInput (&joy_ex, &joy_caps);
    }

#if 1
    state = joy_to_xi;
#else
    state = di8_to_xi;
#endif

#ifdef SK_STEAM_CONTROLLER_SUPPORT
    if (ControllerPresent (config.input.gamepad.steam.ui_slot))
    {
      state =
        *steam_input [config.input.gamepad.steam.ui_slot].to_xi;
    }
#endif


    //extern void SK_ScePad_PaceMaker (void);
    //            SK_ScePad_PaceMaker ();


    if ( api_bridge ||
         SK_XInput_PollController (config.input.gamepad.xinput.ui_slot, &state) )
    {
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
      RtlSecureZeroMemory (&state.Gamepad, sizeof (XINPUT_GAMEPAD));



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

#ifdef SK_STEAM_CONTROLLER_SUPPORT
    if (! ControllerPresent (config.input.gamepad.steam.ui_slot))
    {
      SK_XInput_PulseController ( config.input.gamepad.steam.ui_slot,
                                    haptic_events.PulseTitle.run  () +
                                    haptic_events.PulseButton.run () +
                    std::min (0.4f, haptic_events.PulseNav.run ()),
                                      haptic_events.PulseTitle.run  () +
                                      haptic_events.PulseButton.run () +
                      std::min (0.4f, haptic_events.PulseNav.run    ()) );
    }

    else
#endif
    {
      SK_XInput_PulseController ( config.input.gamepad.xinput.ui_slot,
                                    haptic_events.PulseTitle.run  () +
                                    haptic_events.PulseButton.run () +
                    std::min (0.4f, haptic_events.PulseNav.run ()),
                                      haptic_events.PulseTitle.run  () +
                                      haptic_events.PulseButton.run () +
                      std::min (0.4f, haptic_events.PulseNav.run    ()) );
    }

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

  last_state = state;

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

  for ( float& NavInput : io.NavInputs )
    NavInput = 0.0f;

  if (SK_ImGui_PollGamepad_EndFrame (state))
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
    //MAP_BUTTON (ImGuiNavInput_TweakSlow,  XINPUT_GAMEPAD_LEFT_SHOULDER);  // L1 / LB
    //MAP_BUTTON (ImGuiNavInput_TweakFast,  XINPUT_GAMEPAD_RIGHT_SHOULDER); // R1 / RB
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
        if ( (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0 &&
        (last_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) == 0)
        {
          io.NavInputs [ImGuiNavInput_Cancel] = 1.0f;
        }

        // Text Input                                   // e.g. Triangle button
        if ( (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0 &&
        (last_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) == 0)
        {
          io.NavInputs [ImGuiNavInput_Input] = 1.0f;
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

        io.NavInputs [ImGuiNavInput_TweakSlow] +=
          static_cast <float> (
            SK_Threshold ( state.Gamepad.bLeftTrigger,
                           XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ) /
                ( 255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD     );

        io.NavInputs [ImGuiNavInput_TweakFast] +=
          static_cast <float> (
            SK_Threshold ( state.Gamepad.bRightTrigger,
                           XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ) /
                ( 255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD     );

        //
        // [ANALOG INPUTS]
        //

        // Press Button, Tweak Value                    // e.g. Circle button
        if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0)
          io.NavInputs [ImGuiNavInput_Activate] = 1.0f;

        // Access Menu, Focus, Move, Resize             // e.g. Square button
        if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0)
          io.NavInputs [ImGuiNavInput_Menu] = 1.0f;

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
      io.NavInputs  [ImGuiNavInput_Activate] = 1.0f;

    if (io.KeysDown [VK_ESCAPE])
      io.NavInputs  [ImGuiNavInput_Cancel]   = 1.0f;
  }

  else
    io.NavActive = false;


  if (io.NavActive)
  {
    io.NavInputs [ImGuiNavInput_FocusPrev] +=
      (io.KeyCtrl && io.KeyShift && io.KeysDown [VK_TAB] &&
                            io.KeysDownDuration [VK_TAB] == 0.0f)  ? 1.0f : 0.0f;

    io.NavInputs [ImGuiNavInput_FocusNext] +=
      (io.KeyCtrl                && io.KeysDown [VK_TAB] &&
                            io.KeysDownDuration [VK_TAB] == 0.0f)  ? 1.0f : 0.0f;
  }


  if (io.NavInputs [ImGuiNavInput_Activate] != 0.0f)
    io.MouseDown [4] = true;
  else
    io.MouseDown [4] = false;


  static ULONG64 last_toggle = 0ULL;

  if ( ( io.NavInputs             [ImGuiNavInput_TweakSlow] != 0.0f &&
         io.NavInputs             [ImGuiNavInput_TweakFast] != 0.0f )   &&
       ( io.NavInputsDownDuration [ImGuiNavInput_TweakSlow] == 0.0f ||
         io.NavInputsDownDuration [ImGuiNavInput_TweakFast] == 0.0f )      )
  {
    if (last_toggle < SK_GetFramesDrawn () - 1)
    {
      SK_ImGui_Widgets->hide_all = (! SK_ImGui_Widgets->hide_all);
      last_toggle                =    SK_GetFramesDrawn ();
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

  if (! ItemAdd (total_bb, (ImGuiID)nullptr, &frame_bb))
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
                  GetStyleColorVec4 (ImGuiCol_FrameBg),
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
  extern bool __SK_EnableSetCursor;
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
    g_TicksPerSecond = SK_PerfFreq;
    g_Time           = current_time;

    delta = 0.0;

    io.KeyMap [ImGuiKey_Tab]        = VK_TAB;
    io.KeyMap [ImGuiKey_LeftArrow]  = VK_LEFT;
    io.KeyMap [ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap [ImGuiKey_UpArrow]    = VK_UP;
    io.KeyMap [ImGuiKey_DownArrow]  = VK_DOWN;
    io.KeyMap [ImGuiKey_PageUp]     = VK_PRIOR;
    io.KeyMap [ImGuiKey_PageDown]   = VK_NEXT;
    io.KeyMap [ImGuiKey_Home]       = VK_HOME;
    io.KeyMap [ImGuiKey_End]        = VK_END;
    io.KeyMap [ImGuiKey_Insert]     = VK_INSERT;
    io.KeyMap [ImGuiKey_Delete]     = VK_DELETE;
    io.KeyMap [ImGuiKey_Backspace]  = VK_BACK;
    io.KeyMap [ImGuiKey_Space]      = VK_SPACE;
    io.KeyMap [ImGuiKey_Enter]      = VK_RETURN;
    io.KeyMap [ImGuiKey_Escape]     = VK_ESCAPE;
    io.KeyMap [ImGuiKey_A]          = 'A';
    io.KeyMap [ImGuiKey_C]          = 'C';
    io.KeyMap [ImGuiKey_V]          = 'V';
    io.KeyMap [ImGuiKey_X]          = 'X';
    io.KeyMap [ImGuiKey_Y]          = 'Y';
    io.KeyMap [ImGuiKey_Z]          = 'Z';
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


  if (game_window.mouse.inside)
  {
    SK_ImGui_Cursor.ScreenToLocal (&cursor_pos);

    if ( cursor_pos.x != last_x ||
         cursor_pos.y != last_y )
    {
      if ( abs (SK_ImGui_Cursor.pos.x - cursor_pos.x) > 3 ||
           abs (SK_ImGui_Cursor.pos.y - cursor_pos.y) > 3 )
      {
#define SK_LOG_ONCE_N(lvl,expr,src) { static bool _once = false; if ((! std::exchange (_once, true))) SK_LOG##lvl (expr,src); }
#define SK_LOG_ONCE(expr,src) \
          SK_LOG_ONCE_N(0,expr,src);

        SK_LOG_ONCE ( ( L"Mouse input appears to be inconsistent..." ),
                        L"Win32Input" );

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
  else
    SK_ImGui_Cursor.last_move = SK::ControlPanel::current_time;

  bool bFocused =
    SK_IsGameWindowActive  (),//SK_IsGameWindowFocused (),
       bActive  =
    SK_IsGameWindowActive  ();

  if (bActive || game_window.mouse.inside)
  { if (new_input && bActive) for ( UINT                 i = 7 ; i < 255 ; ++i )
                                    io.KeysDown [i] = ((SK_GetAsyncKeyState (i) & 0x8000) != 0x0);
    io.MouseDown [0] = ((SK_GetAsyncKeyState (VK_LBUTTON) ) < 0);
    io.MouseDown [1] = ((SK_GetAsyncKeyState (VK_RBUTTON) ) < 0);
    io.MouseDown [2] = ((SK_GetAsyncKeyState (VK_MBUTTON) ) < 0);
    io.MouseDown [3] = ((SK_GetAsyncKeyState (VK_XBUTTON1)) < 0);
    io.MouseDown [4] = ((SK_GetAsyncKeyState (VK_XBUTTON2)) < 0);
  } else { RtlSecureZeroMemory (&io.KeysDown [7], sizeof (bool) * 248);
           RtlSecureZeroMemory ( io.MouseDown,    sizeof (bool) * 5); }

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
    SK_ImGui_WantMouseCaptureEx (0x0);

  if ( abs (last_x - SK_ImGui_Cursor.pos.x) > 3 ||
       abs (last_y - SK_ImGui_Cursor.pos.y) > 3 ||
          bWantMouseCaptureForUI )
  {
    SK_ImGui_Cursor.last_move = SK::ControlPanel::current_time;
                    last_x    = SK_ImGui_Cursor.pos.x;
                    last_y    = SK_ImGui_Cursor.pos.y;
  }


  if ((bWantMouseCaptureForUI || SK_ImGui_Active ()) && SK_ImGui_Cursor.last_move > SK::ControlPanel::current_time - 500)
    SK_ImGui_Cursor.idle = false;

  else
    SK_ImGui_Cursor.idle = ( (! SK_ImGui_WantMouseCapture ()) || config.input.mouse.disabled_to_game == SK_InputEnablement::Disabled );
                                                              // Disabled to game is a form of capture,
                                                              //   but it is exempt from idle cursor logic

  const bool bManageCursor =
    (config.input.cursor.manage || SK_ImGui_Cursor.force != sk_cursor_state::None);

  if (bManageCursor && game_window.mouse.inside)
  {
    if (SK_Window_IsCursorActive () && SK_ImGui_Cursor.force != sk_cursor_state::Hidden)
    {
      if (! SK_InputUtil_IsHWCursorVisible ())
      {
        int recursion = 8;

        if ( 0 != SK_GetSystemMetrics (SM_MOUSEPRESENT) )
          while ( recursion > 0 && ShowCursor (TRUE) < 0 ) --recursion;
      }
    }

    else if (config.input.cursor.manage || SK_ImGui_Cursor.force == sk_cursor_state::Hidden)
    {
      if (SK_InputUtil_IsHWCursorVisible ())
      {
        int recursion = 8;

        if ( 0 != SK_GetSystemMetrics (SM_MOUSEPRESENT) )
          while ( recursion > 0 && ShowCursor (FALSE) > -1 ) --recursion;

        SK_SetCursor (nullptr);
      }
    }
  }


  if (! SK_ImGui_Cursor.idle)
  {
    if (SK_ImGui_WantMouseCapture ())
    {
      extern HCURSOR ImGui_DesiredCursor (void);
      SK_SetCursor  (ImGui_DesiredCursor ());
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

  if (bFocused)
  {
    if (config.input.keyboard.catch_alt_f4)
    {
      if ( io.KeyAlt                     &&
           io.KeysDown         [VK_F4  ] &&
       ( ( io.KeysDownDuration [VK_MENU] > 0.0f ) ^
         ( io.KeysDownDuration [VK_F4  ] > 0.0f ) ) )
      {
        extern bool SK_ImGui_WantExit;
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
          game_window.active = true;

          SetWindowPos ( game_window.hWnd, SK_GetForegroundWindow (),
                           0, 0,
                           0, 0,
                             SWP_NOMOVE | SWP_NOSIZE |
                             SWP_ASYNCWINDOWPOS );
        }
      }
    }
  }
}

bool
SK_ImGui_IsEULAVisible (void)
{
  return eula.show;
}