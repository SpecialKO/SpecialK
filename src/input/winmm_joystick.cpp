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
//////////////////////////////////////////////////////////////
//
// WinMM (Windows 95 joystick API)
//
//////////////////////////////////////////////////////////////
joyGetPos_pfn   joyGetPos_Original   = nullptr;
joyGetPosEx_pfn joyGetPosEx_Original = nullptr;

// This caching mechanism is only needed for joyGetPosEx, because all calls
//   to joyGetPos are forwarded to joyGetPosEx
struct SK_WINMM_joyGetPosExHistory
{
  struct test_record_s {
    DWORD    dwLastTested = 0UL;
    MMRESULT lastResult   = JOYERR_NOERROR;
  };

  static constexpr auto                         _TimeBetweenTestsInMs = 666UL;
  static concurrency::concurrent_unordered_map <UINT, test_record_s> _records;

  static MMRESULT getJoyState (UINT uJoyID)
  {
    auto& record =
      _records [uJoyID];

    if (auto                  currentTimeInMs = SK_timeGetTime ();
        record.dwLastTested < currentTimeInMs - _TimeBetweenTestsInMs)
    {
      __try {
        JOYCAPSW joy_caps   = { };
        record.dwLastTested = currentTimeInMs;
        record.lastResult   =
          SK_joyGetDevCapsW (uJoyID, &joy_caps, sizeof (JOYCAPSW));

        // This API enumerates random devices that don't qualify as gamepads;
        //   if there is no D-Pad _AND_ fewer than 4 buttons, then ignore it.
        bool bInvalidController =
          (! (joy_caps.wCaps & JOYCAPS_POV4DIR)) ||
              joy_caps.wNumButtons < 4;

        if (bInvalidController)
        {
          record.dwLastTested = currentTimeInMs + _TimeBetweenTestsInMs;
          record.lastResult   = JOYERR_NOCANDO;
        }
      }

      __except (EXCEPTION_EXECUTE_HANDLER)
      {
        SK_LOGi0 (L"Swallowed a Structured Exception During joyGetPosEx!");

        record.dwLastTested = currentTimeInMs + _TimeBetweenTestsInMs;
        record.lastResult   = JOYERR_NOCANDO;
      }

      // Offset the time by a random fractional amount so that games
      //   do not check multiple failing devices in a single frame
      record.dwLastTested += std::max ( 133UL,
        static_cast <DWORD> ( ( static_cast <float> (std::rand ()) /
                                static_cast <float> (RAND_MAX) ) *
                                static_cast <float> (_TimeBetweenTestsInMs) )
                                      );
    }

    return
      record.lastResult;
  }
};

concurrency::concurrent_unordered_map <
  UINT, SK_WINMM_joyGetPosExHistory::test_record_s
>       SK_WINMM_joyGetPosExHistory::_records;


MMRESULT
WINAPI
joyGetPosEx_Detour (_In_  UINT        uJoyID,
                    _Out_ LPJOYINFOEX pjiUINT)
{
  SK_LOG_FIRST_CALL

  if (pjiUINT == nullptr)
    return MMSYSERR_INVALPARAM;

  if (config.input.gamepad.disable_winmm)
    return JOYERR_UNPLUGGED;

  // Keep a cache of previous poll requests, because this has very high
  //   failure overhead and could wreck performance when gamepads are absent.
  if (auto lastKnownState = SK_WINMM_joyGetPosExHistory::getJoyState (uJoyID);
           lastKnownState != JOYERR_NOERROR)
    return lastKnownState;

  JOYINFOEX joyInfo =
    { .dwSize  = sizeof (JOYINFOEX),
      .dwFlags = pjiUINT->dwFlags };

  auto result =
    joyGetPosEx_Original (uJoyID, &joyInfo);

  // Early-out whenever possible
  //
  //  * Steam's hook on joyGetDevCaps is a performance nightmare!
  //
  if (result != JOYERR_NOERROR)
  {
    // Initialize the output with the minimal set of neutral values,
    //   in case the game's not actually checking the return value.
    *pjiUINT = { .dwSize  = pjiUINT->dwSize,
                 .dwFlags = pjiUINT->dwFlags,
                 .dwPOV   = JOY_POVCENTERED };

    SK_LOGi4 (L"joyGetPosEx Poll Failure for Joystick %d, Ret=%x", uJoyID, result);

    return result;
  }

  static concurrency::concurrent_unordered_set <UINT>
    warned_devs;

  JOYCAPSW joy_caps = { };

  if (! warned_devs.count (uJoyID))
        SK_joyGetDevCapsW (uJoyID, &joy_caps, sizeof (JOYCAPSW));

  // This API enumerates random devices that don't qualify as gamepads;
  //   if there is no D-Pad _AND_ fewer than 4 buttons, then ignore it.
  bool bInvalidController =
    (! (joy_caps.wCaps & JOYCAPS_POV4DIR)) ||
        joy_caps.wNumButtons < 4;

  // Failure to ignore bInvalidController status would cause many games
  //   to treat the D-Pad as if it were stuck in the UP position.

  if (bInvalidController || SK_ImGui_WantGamepadCapture ())
  {
    pjiUINT->dwPOV          = JOY_POVCENTERED;
    pjiUINT->dwXpos         = (joy_caps.wXmax - joy_caps.wXmin) / 2;
    pjiUINT->dwYpos         = (joy_caps.wYmax - joy_caps.wYmin) / 2;
    pjiUINT->dwZpos         = (joy_caps.wZmax - joy_caps.wZmin) / 2;
    pjiUINT->dwRpos         = (joy_caps.wRmax - joy_caps.wRmin) / 2;
    pjiUINT->dwUpos         = (joy_caps.wUmax - joy_caps.wUmin) / 2;
    pjiUINT->dwVpos         = (joy_caps.wVmax - joy_caps.wVmin) / 2;
    pjiUINT->dwButtons      = 0;
    pjiUINT->dwButtonNumber = 0;

    if (bInvalidController)
    {
      if (warned_devs.insert (uJoyID).second)
      {
        SK_LOGi0 (
          L"Ignoring attempt to poll WinMM Joystick #%d (%ws [%ws]) because"
          L" it lacks characteristics of game input devices.", uJoyID,
            joy_caps.szPname, joy_caps.szRegKey
        );
      }

      return JOYERR_UNPLUGGED;
    }

    SK_WinMM_Backend->markHidden (sk_input_dev_type::Gamepad);

    return JOYERR_NOERROR;
  }

  // Forward the data we polled
  *pjiUINT = joyInfo;

  if (result == JOYERR_NOERROR)
    SK_WinMM_Backend->markRead (sk_input_dev_type::Gamepad);

  return result;
}

MMRESULT
WINAPI
joyGetPos_Detour (_In_  UINT      uJoyID,
                  _Out_ LPJOYINFO pjiUINT)
{
  SK_LOG_FIRST_CALL

  JOYINFOEX jex =
     { .dwSize  = sizeof (JOYINFOEX),
       .dwFlags = JOY_RETURNX | JOY_RETURNY |
                  JOY_RETURNZ | JOY_RETURNBUTTONS };

  if (pjiUINT == nullptr || uJoyID > 15)
    return JOYERR_PARMS;

  if (config.input.gamepad.disable_winmm)
    return JOYERR_UNPLUGGED;

  MMRESULT result =
    joyGetPosEx_Detour (uJoyID, &jex);

  if (result == JOYERR_NOERROR)
  {
    pjiUINT->wButtons = jex.dwButtons;
    pjiUINT->wXpos    = jex.dwXpos;
    pjiUINT->wYpos    = jex.dwYpos;
    pjiUINT->wZpos    = jex.dwZpos;
  }

  return result;
}

void
SK_Input_HookWinMM (void)
{
  if (! config.input.gamepad.hook_winmm)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    // Not sure why we're calling this if winmm wasn't even loaded yet...
    if (! GetModuleHandleW (L"winmm.dll"))
           SK_LoadLibraryW (L"winmm.dll");

    SK_LOG0 ( ( L"Game uses WinMM, installing input hooks..." ),
                L"  Input   " );

    // This is layered on top of DirectInput, which is layered on top of
    //   HID... let's get this out of the way.
    if (! GetModuleHandleW (L"HID.dll"))
           SK_LoadLibraryW (L"HID.dll");

    SK_Input_HookHID ();

    // NOTE: API forwards calls to joyGetPosEx, do not call one from the other.
    SK_CreateDLLHook2 (     L"winmm.DLL",
                             "joyGetPos",
                              joyGetPos_Detour,
     static_cast_p2p <void> (&joyGetPos_Original) );

    SK_CreateDLLHook2 (     L"winmm.DLL",
                             "joyGetPosEx",
                              joyGetPosEx_Detour,
     static_cast_p2p <void> (&joyGetPosEx_Original) );

    extern joyGetDevCapsW_pfn _joyGetDevCapsW;
    extern joyGetPos_pfn      _joyGetPos;
    extern joyGetPosEx_pfn    _joyGetPosEx;
    extern joyGetNumDevs_pfn  _joyGetNumDevs;

    if (! config.input.gamepad.allow_steam_winmm)
    {
      static std::filesystem::path path_to_winmm_base =
            (std::filesystem::path (SK_GetInstallPath ()) /
                                 LR"(Drivers\WinMM)"),
                                       winmm_name =
                    SK_RunLHIfBitness (64, L"WinMM_SK64.dll",
                                           L"WinMM_SK32.dll"),
                               path_to_winmm = 
                               path_to_winmm_base /
                                       winmm_name;

      static const auto *pSystemDirectory =
        SK_GetSystemDirectory ();

      std::filesystem::path
        path_to_system_winmm =
          (std::filesystem::path (pSystemDirectory) / L"winmm.dll");

      std::error_code ec =
        std::error_code ();

      if (std::filesystem::exists (path_to_system_winmm, ec))
      {
        if ( (! std::filesystem::exists ( path_to_winmm,         ec))||
             (! SK_Assert_SameDLLVersion (path_to_winmm.       c_str (),
                                          path_to_system_winmm.c_str ()) ) )
        { SK_CreateDirectories           (path_to_winmm.       c_str ());

          if (   std::filesystem::exists (path_to_system_winmm,                ec))
          { std::filesystem::remove      (                      path_to_winmm, ec);
            std::filesystem::copy_file   (path_to_system_winmm, path_to_winmm, ec);
          }
        }
      }

      HMODULE hModWinMM =
        SK_LoadLibraryW (path_to_winmm.c_str ());

      _joyGetDevCapsW =
      (joyGetDevCapsW_pfn)SK_GetProcAddress (hModWinMM, "joyGetDevCapsW");

      _joyGetPos =
      (joyGetPos_pfn)SK_GetProcAddress      (hModWinMM, "joyGetPos");

      _joyGetNumDevs =
      (joyGetNumDevs_pfn)SK_GetProcAddress  (hModWinMM, "joyGetNumDevs");

      _joyGetPosEx =
      (joyGetPosEx_pfn)SK_GetProcAddress    (hModWinMM, "joyGetPosEx");
    }

    else
    {
      _joyGetDevCapsW =
      (joyGetDevCapsW_pfn)SK_GetProcAddress (L"winmm.dll", "joyGetDevCapsW");

      _joyGetPos =
      (joyGetPos_pfn)joyGetPos_Original;

      _joyGetNumDevs =
      (joyGetNumDevs_pfn)SK_GetProcAddress  (L"winmm.dll", "joyGetNumDevs");

      _joyGetPosEx =
      (joyGetPosEx_pfn)joyGetPosEx_Original;
    }

    if (ReadAcquire (&__SK_Init) > 0) SK_ApplyQueuedHooks ();

    InterlockedIncrementRelease (&hooked);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}

bool
SK_Input_PreHookWinMM (void)
{
  if (! config.input.gamepad.hook_hid)
    return false;

  static
    sk_import_test_s tests [] = {
      { "winmm.dll", false }
    };

  SK_TestImports (
    SK_GetModuleHandle (nullptr), tests, 1
  );

  if (tests [0].used || SK_GetModuleHandle (L"winmm.dll"))
  {
    SK_Input_HookWinMM ();

    return true;
  }

  return false;
}