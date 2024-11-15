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
#include <resource.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Input Mgr."

#include <ReShade/reshade.hpp>

bool
SK_ImGui_ExemptOverlaysFromKeyboardCapture (void)
{
//static const UINT vKeyEpic    = VK_F3;
  static const UINT vKeySteam   = VK_TAB;
  static const UINT vKeyReShade = VK_HOME;
  static const UINT vKeyShift   = VK_SHIFT;

  static bool bLastTab  = false;
//static bool bLastF3   = false;
  static bool bLastHome = false;

  const bool bTab   = (sk::narrow_cast <USHORT> (SK_GetAsyncKeyState (vKeySteam  )) & 0x8000) != 0;
//const bool bF3    = (sk::narrow_cast <USHORT> (SK_GetAsyncKeyState (vKeyEpic   )) & 0x8000) != 0;
  const bool bShift = (sk::narrow_cast <USHORT> (SK_GetAsyncKeyState (vKeyShift  )) & 0x8000) != 0;
  const bool bHome  = (sk::narrow_cast <USHORT> (SK_GetAsyncKeyState (vKeyReShade)) & 0x8000) != 0;

  if ( bHome == bLastHome &&
       bTab  == bLastTab )
     //bF3   == bLastF3 )
  {
    return false;
  }

  bool bTabChanged  = (bLastTab  != bTab  );
//bool bF3Changed   = (bLastF3   != bF3   );
  bool bHomeChanged = (bLastHome != bHome );

  bLastTab  = bTab;
//bLastF3   = bF3;
  bLastHome = bHome;

  if (game_window.active && SK_ImGui_WantKeyboardCapture ())
  {
    static HMODULE hModReShadeDLL = reshade::internal::get_reshade_module_handle ();
    static bool    bHasReShadeDLL = hModReShadeDLL != nullptr;

    static
       int                         reshade_dll_tests = 0;
    if (bHasReShadeDLL == false && reshade_dll_tests < 10)
    {
      static DWORD lastTested = SK_timeGetTime ();
      if (         lastTested < SK_timeGetTime () - 2500UL)
      {
        hModReShadeDLL = reshade::internal::get_reshade_module_handle ();
        bHasReShadeDLL = hModReShadeDLL != nullptr;

        if (! bHasReShadeDLL)
          ++reshade_dll_tests;
      }
    }

    const bool
      bSteamOverlay    =  ( bShift && bTab ),
    //bEpicOverlay     =  ( bShift && bF3  ),
      bReShadeOverlay  =  ( bHome  &&
                           (bHasReShadeDLL) );

    if (bSteamOverlay /*|| bEpicOverlay*/ || bReShadeOverlay)
    {
      WriteULong64Release (
        &config.input.keyboard.temporarily_allow,
          SK_GetFramesDrawn () + 40
      );

      if (bSteamOverlay/*|| bEpicOverlay*/)
      {
        const BYTE bScancodeShift =
          (BYTE)MapVirtualKey (vKeyShift, 0);

        const DWORD dwFlagsShift =
          ( bScancodeShift & 0xE0 ) == 0  ?
                static_cast <DWORD> (0x0) :
                static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

        if (bSteamOverlay && bTabChanged)
        {
          const BYTE bScancodeSteam =
            (BYTE)MapVirtualKey (vKeySteam, 0);

          const DWORD dwFlagsSteam =
            ( bScancodeSteam & 0xE0 ) == 0  ?
                  static_cast <DWORD> (0x0) :
                  static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

          SK_keybd_event ((BYTE)vKeyShift, bScancodeShift, dwFlagsShift, 0);
          SK_keybd_event ((BYTE)vKeySteam, bScancodeSteam, dwFlagsSteam, 0);
        }

#if 0
        else if (bEpicOverlay && bF3Changed)
        {
          const BYTE bScancodeEpic =
            (BYTE)MapVirtualKey (vKeyEpic, 0);

          const DWORD dwFlagsEpic =
            ( bScancodeEpic & 0xE0 ) == 0  ?
                 static_cast <DWORD> (0x0) :
                 static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

          SK_keybd_event ((BYTE)vKeyShift, bScancodeShift, dwFlagsShift, 0);
          SK_keybd_event ((BYTE)vKeyEpic,  bScancodeEpic,  dwFlagsEpic,  0);
        }
#endif
      }

      else if (bReShadeOverlay && bHomeChanged)
      {
        const BYTE bScancodeReShade =
          (BYTE)MapVirtualKey (vKeyReShade, 0);

        const DWORD dwFlagsReShade =
          ( bScancodeReShade & 0xE0 ) == 0 ?
                 static_cast <DWORD> (0x0) :
                 static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

        SK_keybd_event ((BYTE)vKeyReShade, bScancodeReShade, dwFlagsReShade, 0);
      }

      return true;
    }
  }

  else
  {
    bLastTab  = false;
  //bLastF3   = false;
    bLastHome = false;
  }

  return false;
}

bool
SK_ImGui_WantKeyboardCapture (bool update)
{
  static std::atomic_bool capture = false;

  if (! SK_GImDefaultContext ())
  {
    capture.store (false);
    return false;
  }

  // Allow keyboard input while ReShade overlay is active
  if (SK_ReShadeAddOn_IsOverlayActive ())
  {
    config.input.mouse   .disabled_to_game = 2;
    config.input.keyboard.disabled_to_game = 2;
    capture.store (false);

    return !SK_IsGameWindowActive ();
  }
  else
  {
    config.input.mouse.       disabled_to_game =
    config.input.mouse.   org_disabled_to_game;
    config.input.keyboard.    disabled_to_game =
    config.input.keyboard.org_disabled_to_game;
  }

  // Allow keyboard input while Steam /EOS overlays are active
  if (SK::SteamAPI::GetOverlayState (true) ||
           SK::EOS::GetOverlayState (true))
  {
    capture.store (false);
    return false;
  }

  const auto framesDrawn =
    SK_GetFramesDrawn ();

  // Do not block on first frame drawn unless explicitly disabled
  if (framesDrawn < 1 && (config.input.keyboard.disabled_to_game != 1))
  {
    capture.store (false);
    return false;
  }

  static std::atomic <ULONG64> lastFrameCaptured = 0;

  if (! update)
    return capture.load () || lastFrameCaptured > framesDrawn - 2;

  bool imgui_capture =
    config.input.keyboard.disabled_to_game == SK_InputEnablement::Disabled;

  const bool bWindowActive =
     SK_IsGameWindowActive ();

  if (bWindowActive || SK_WantBackgroundRender ())
  {
    static const auto& io =
      ImGui::GetIO ();

    if ((nav_usable || io.WantCaptureKeyboard || io.WantTextInput) && (! SK_ImGuiEx_Visible))
      imgui_capture = true;                                        // Don't block keyboard input on popups, or stupid games can miss Alt+F4

    else if (SK_IsConsoleVisible ())
      imgui_capture = true;

    else
    {
      // Poke through input for a special-case
      if (ReadULong64Acquire (&config.input.keyboard.temporarily_allow) > framesDrawn - 40)
      {
        imgui_capture = false;
      }
    }
  }

  if ((! bWindowActive) && config.input.keyboard.disabled_to_game == SK_InputEnablement::DisabledInBackground)
    imgui_capture = true;

  if (             imgui_capture) lastFrameCaptured = framesDrawn;
    capture.store (imgui_capture);
  return           imgui_capture;
}

bool
SK_ImGui_WantTextCapture (void)
{
  if (! SK_GImDefaultContext ())
    return false;

  // Allow keyboard input while ReShade overlay is active
  if (SK_ReShadeAddOn_IsOverlayActive ())
    return false;

  // Allow keyboard input while Steam /EOS overlays are active
  if (SK::SteamAPI::GetOverlayState (true) ||
           SK::EOS::GetOverlayState (true))
  {
    return false;
  }

  bool imgui_capture =
    config.input.keyboard.disabled_to_game == SK_InputEnablement::Disabled;

  const bool bWindowActive =
     SK_IsGameWindowActive ();

  if (bWindowActive || SK_WantBackgroundRender ())
  {
    static const auto& io =
      ImGui::GetIO ();

    if (io.WantTextInput)
      imgui_capture = true;

    else if (bWindowActive)
    {
      // Poke through input for a special-case
      if (ReadULong64Acquire (&config.input.keyboard.temporarily_allow) > SK_GetFramesDrawn () - 40)
        imgui_capture = false;
    }
  }

  if ((! bWindowActive) && config.input.keyboard.disabled_to_game == SK_InputEnablement::DisabledInBackground)
    imgui_capture = true;

  return
    imgui_capture;
}

keybd_event_pfn keybd_event_Original = nullptr;

void
WINAPI
keybd_event_Detour (
  _In_ BYTE       bVk,
  _In_ BYTE       bScan,
  _In_ DWORD     dwFlags,
  _In_ ULONG_PTR dwExtraInfo )
{
  SK_LOG_FIRST_CALL

  // TODO: Process this the right way...
  if (SK_ImGui_WantKeyboardCapture ())
  {
    return;
  }

  keybd_event_Original (
    bVk, bScan, dwFlags, dwExtraInfo
  );
}

struct SK_Win32_KeybdEvent {
  BYTE       bVk;
  BYTE       bScan;
  DWORD     dwFlags;
  ULONG_PTR dwExtraInfo;
};

void
WINAPI
SK_keybd_event (
  _In_ BYTE       bVk,
  _In_ BYTE       bScan,
  _In_ DWORD     dwFlags,
  _In_ ULONG_PTR dwExtraInfo )
{
  static concurrency::concurrent_queue <SK_Win32_KeybdEvent> _keybd_events;
  static HANDLE                                               hInputEvent =
    SK_CreateEvent (nullptr, FALSE, FALSE, L"[SK] Input Synthesis Requested");

  static HANDLE hKeyboardSynthesisThread =
    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      SK_Thread_SetCurrentPriority (THREAD_PRIORITY_TIME_CRITICAL);

      HANDLE hSignals [] =
        { __SK_DLL_TeardownEvent,
                     hInputEvent };

      while ( WAIT_OBJECT_0 !=
                WaitForMultipleObjects (2, hSignals, FALSE, INFINITE) )
      {
        while (! _keybd_events.empty ())
        {
          SK_Win32_KeybdEvent        data = { };
          if (_keybd_events.try_pop (data))
          {
            ( keybd_event_Original != nullptr )                       ?
              keybd_event_Original ( data.bVk,     data.bScan,
                                     data.dwFlags, data.dwExtraInfo ) :
              keybd_event          ( data.bVk,     data.bScan,
                                     data.dwFlags, data.dwExtraInfo ) ;
          }
        }
      }

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] Keyboard Input Synthesizer"
  );

  _keybd_events.push (
    { bVk, bScan, dwFlags, dwExtraInfo }
  );

  SetEvent (hInputEvent);
}

GetKeyState_pfn      GetKeyState_Original      = nullptr;
GetAsyncKeyState_pfn GetAsyncKeyState_Original = nullptr;
GetKeyboardState_pfn GetKeyboardState_Original = nullptr;

SHORT
WINAPI
SK_GetAsyncKeyState (int vKey)
{
  if (GetAsyncKeyState_Original != nullptr)
    return GetAsyncKeyState_Original (vKey);

  return
    GetAsyncKeyState (vKey);
}

// Shared by GetAsyncKeyState and GetKeyState, just pass the correct
//   function pointer and this code only has to be written once.
SHORT
WINAPI
SK_GetSharedKeyState_Impl (int vKey, GetAsyncKeyState_pfn pfnGetFunc)
{
  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  auto SK_ConsumeVKey = [&](int vKey) ->
  SHORT
  {
    SHORT sKeyState =
      pfnGetFunc (vKey);

    sKeyState &= ~(1 << 15); // High-Order Bit = 0
    sKeyState &= ~1;         // Low-Order Bit  = 0

    if (pfnGetFunc == GetAsyncKeyState_Original)
      SK_Win32_Backend->markHidden (sk_win32_func::GetAsyncKeystate);
    else if (pfnGetFunc == GetKeyState_Original)
      SK_Win32_Backend->markHidden (sk_win32_func::GetKeyState);

    return
      sKeyState;
  };

  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible ())
  {
    return
      SK_ConsumeVKey (vKey);
  }

  bool fullscreen =
    ( SK_GetFramesDrawn () && rb.isTrueFullscreen () );

  // Block keyboard input to the game while it's in the background
  if ((! fullscreen) &&
      SK_WantBackgroundRender () && (! SK_IsGameWindowActive ()))
  {
    if (pfnGetFunc == GetAsyncKeyState_Original)
    {
      return
        SK_ConsumeVKey (vKey);
    }
  }

  // Valid Keys:  8 - 255
  if ((vKey & 0xF8) != 0)
  {
    if (SK_ImGui_WantKeyboardCapture ())
    {
      return
        SK_ConsumeVKey (vKey);
    }
  }

  // 0-8 = Mouse + Unused Buttons
  else if (vKey < 8)
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
    {
      return
        SK_ConsumeVKey (vKey);
    }
  }

  if (pfnGetFunc == GetAsyncKeyState_Original)
    SK_Win32_Backend->markRead (sk_win32_func::GetAsyncKeystate);
  else if (pfnGetFunc == GetKeyState_Original)
    SK_Win32_Backend->markRead (sk_win32_func::GetKeyState);

  return
    pfnGetFunc (vKey);
}

SHORT
WINAPI
GetAsyncKeyState_Detour (_In_ int vKey)
{
  SK_LOG_FIRST_CALL

  return
    SK_GetSharedKeyState_Impl (
      vKey,
        GetAsyncKeyState_Original
    );
}

SHORT
WINAPI
SK_GetKeyState (_In_ int nVirtKey)
{
  if (GetKeyState_Original != nullptr)
    return GetKeyState_Original (nVirtKey);

  return GetKeyState (nVirtKey);
}

SHORT
WINAPI
GetKeyState_Detour (_In_ int vKey)
{
  SK_LOG_FIRST_CALL

  return
    SK_GetSharedKeyState_Impl (
      vKey,
        GetKeyState_Original
    );
}

//typedef BOOL (WINAPI *SetKeyboardState_pfn)(PBYTE lpKeyState); // TODO

BOOL
WINAPI
SK_GetKeyboardState (PBYTE lpKeyState)
{
  if (GetKeyboardState_Original != nullptr)
    return GetKeyboardState_Original (lpKeyState);

  return
    GetKeyboardState (lpKeyState);
}

BOOL
WINAPI
GetKeyboardState_Detour (PBYTE lpKeyState)
{
  SK_LOG_FIRST_CALL

  BOOL bRet =
    SK_GetKeyboardState (lpKeyState);

  if (bRet)
  {
    bool capture_mouse    = SK_ImGui_WantMouseCapture    ();
    bool capture_keyboard = SK_ImGui_WantKeyboardCapture ();

    // All-at-once
    if (capture_mouse && capture_keyboard)
    {
      RtlZeroMemory (lpKeyState, 255);
    }

    else
    {
      if (capture_keyboard)
      {
        RtlZeroMemory (&lpKeyState [7], 247);
      }

      // Some games use this API for mouse buttons, for reasons that are beyond me...
      if (capture_mouse)
      {
        RtlZeroMemory (lpKeyState, 7);
      }
    }

    if (! (capture_keyboard && capture_mouse))
      SK_Win32_Backend->markRead (sk_win32_func::GetKeyboardState);
    else
      SK_Win32_Backend->markHidden (sk_win32_func::GetKeyboardState);
  }

  return bRet;
}


typedef HKL (WINAPI *GetKeyboardLayout_pfn)(_In_ DWORD idThread);
                     GetKeyboardLayout_pfn
                     GetKeyboardLayout_Original = nullptr;

SK_LazyGlobal <concurrency::concurrent_unordered_map <DWORD, HKL>> CachedHKL;

void
SK_Win32_InvalidateHKLCache (void)
{
  CachedHKL->clear ();
}

HKL
WINAPI
GetKeyboardLayout_Detour (_In_ DWORD idThread)
{
  const auto& ret =
    CachedHKL->find (idThread);

  if (ret != CachedHKL->cend ())
    return ret->second;


  HKL current_hkl =
    GetKeyboardLayout_Original (idThread);

  CachedHKL.get ()[idThread] = current_hkl; //-V108

  return current_hkl;
}


SK_LazyGlobal <std::unordered_map <char, char>> SK_Keyboard_LH_Arrows;

char SK_KeyMap_LeftHand_Arrow (char key)
{
  if ( SK_Keyboard_LH_Arrows->cend (   ) !=
       SK_Keyboard_LH_Arrows->find (key)  )
  {
    return
      SK_Keyboard_LH_Arrows.get ()[key];
  }

  return '\0';
}


void
SK_Input_InitKeyboard (void)
{
  static bool        run_once = false;
  if (std::exchange (run_once, true))
    return;

  SK_ImGui_InputLanguage_s::keybd_layout =
    GetKeyboardLayout (0);
  
  bool azerty = false;
  bool qwertz = false;
  bool qwerty = false;
  
  static const auto test_chars = {
    'W', 'Q', 'Y'
  };
  
  static const std::multimap <char, UINT> known_sig_parts = {
    { 'W', 0x11 }, { 'W', 0x2c },
    { 'Q', 0x10 }, { 'Q', 0x1e },
    { 'Y', 0x15 }, { 'Y', 0x2c }
  };
  
  UINT layout_sig [] =
  { 0x0, 0x0, 0x0 };
  
  int idx = 0;
  
  for ( auto& ch : test_chars )
  {
    bool matched = false;
    UINT scode   =
      MapVirtualKeyEx (ch, MAPVK_VK_TO_VSC, GetKeyboardLayout (SK_Thread_GetCurrentId ()));
  
    if (known_sig_parts.find (ch) != known_sig_parts.cend ())
    {
      auto range =
        known_sig_parts.equal_range (ch);
  
      for_each (range.first, range.second,
        [&](const std::unordered_multimap <char, UINT>::value_type & cmd_pair)
        {
          if (cmd_pair.second == scode)
          {
            matched = true;
          }
        }
      );
    }
  
    if (! matched)
    {
      SK_LOG0 ( ( L"Unexpected Keyboard Layout -- Scancode 0x%x maps to Virtual Key '%c'",
                    scode, ch ),
                  L"Key Layout" );
    }
  
    layout_sig [idx++] = scode;
  }
  
  azerty  = ( layout_sig [0] == 0x2c && layout_sig [1] == 0x1e );
  qwertz  = ( layout_sig [0] == 0x11 && layout_sig [1] == 0x10 &&
              layout_sig [2] == 0x2c ); // No special treatment needed, just don't call it "unknown"
  qwerty  = ( layout_sig [0] == 0x11 && layout_sig [1] == 0x10 &&
              layout_sig [2] == 0x15 );
  
  if (! azerty) SK_Keyboard_LH_Arrows ['W'] = 'W'; else SK_Keyboard_LH_Arrows ['W'] = 'Z';
  if (! azerty) SK_Keyboard_LH_Arrows ['A'] = 'A'; else SK_Keyboard_LH_Arrows ['A'] = 'Q';
  
  SK_Keyboard_LH_Arrows ['S'] = 'S';
  SK_Keyboard_LH_Arrows ['D'] = 'D';
  
  std::wstring layout_desc = L"Unexpected";
  
  if      (azerty) layout_desc = L"AZERTY";
  else if (qwerty) layout_desc = L"QWERTY";
  else if (qwertz) layout_desc = L"QWERTZ";
  
  SK_LOG0 ( ( L" -( Keyboard Class:  %s  *  [ W=%x, Q=%x, Y=%x ] )-",
                layout_desc.c_str (), layout_sig [0], layout_sig [1], layout_sig [2] ),
              L"Key Layout" );
}

void
SK_Input_PreHookKeyboard (void)
{
  SK_RunOnce (
  {
    SK_CreateDLLHook2 (      L"user32",
                              "GetAsyncKeyState",
                               GetAsyncKeyState_Detour,
      static_cast_p2p <void> (&GetAsyncKeyState_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "GetKeyState",
                               GetKeyState_Detour,
      static_cast_p2p <void> (&GetKeyState_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "GetKeyboardState",
                               GetKeyboardState_Detour,
      static_cast_p2p <void> (&GetKeyboardState_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "keybd_event",
                               keybd_event_Detour,
      static_cast_p2p <void> (&keybd_event_Original) );
  });
}


bool SK_ImGui_InputLanguage_s::changed      = true; // ^^^^ Default = true
HKL  SK_ImGui_InputLanguage_s::keybd_layout;