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

//////////////////////////////////////////////////////////////////////////////////
//
// Raw Input
//
//////////////////////////////////////////////////////////////////////////////////
//#define __MANAGE_RAW_INPUT_REGISTRATION
#define __HOOK_BUFFERED_RAW_INPUT
SK_LazyGlobal <std::mutex>                   raw_device_view;
SK_LazyGlobal <std::vector <RAWINPUTDEVICE>> raw_devices;   // ALL devices, this is the list as Windows would give it to the game

SK_LazyGlobal <std::vector <RAWINPUTDEVICE>> raw_mice;      // View of only mice
SK_LazyGlobal <std::vector <RAWINPUTDEVICE>> raw_keyboards; // View of only keyboards
SK_LazyGlobal <std::vector <RAWINPUTDEVICE>> raw_gamepads;  // View of only game pads

#define SK_RAWINPUT_READ(type)  SK_RawInput_Backend->markRead   (type);
#define SK_RAWINPUT_WRITE(type) SK_RawInput_Backend->markWrite  (type);
#define SK_RAWINPUT_VIEW(type)  SK_RawInput_Backend->markViewed (type);
#define SK_RAWINPUT_HIDE(type)  SK_RawInput_Backend->markHidden (type);

struct
{
  struct
  {
    bool active          = false;
    bool legacy_messages = false;
  } mouse,
    keyboard;

} raw_overrides;

GetRegisteredRawInputDevices_pfn
GetRegisteredRawInputDevices_Original = nullptr;

// Returns all mice, in their override state (if applicable)
std::vector <RAWINPUTDEVICE>
SK_RawInput_GetMice (bool* pDifferent = nullptr)
{
  if (raw_overrides.mouse.active)
  {
    bool different = false;

    std::vector <RAWINPUTDEVICE> overrides;

    // Aw, the game doesn't have any mice -- let's fix that.
    if (raw_mice->empty ())
    {
      raw_devices->emplace_back (
        RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC,
                         HID_USAGE_GENERIC_MOUSE, 0x00, nullptr } );
      raw_mice->emplace_back    (
        RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC,
                         HID_USAGE_GENERIC_MOUSE, 0x00, nullptr } );
      raw_overrides.mouse.legacy_messages = true;
    }

    for (auto& it : raw_mice.get ())
    {
      if (raw_overrides.mouse.legacy_messages)
      {
        different    |= (it.dwFlags & RIDEV_NOLEGACY) != 0;
        it.dwFlags   &= ~(RIDEV_NOLEGACY | RIDEV_APPKEYS | RIDEV_REMOVE);
        it.dwFlags   &= ~RIDEV_CAPTUREMOUSE;
        SK_RegisterRawInputDevices ( &it, 1, sizeof (RAWINPUTDEVICE) );
      }

      else
      {
        different  |= (it.dwFlags & RIDEV_NOLEGACY) == 0;
        it.dwFlags |= RIDEV_NOLEGACY;
        SK_RegisterRawInputDevices ( &it, 1, sizeof (RAWINPUTDEVICE) );
      }

      overrides.emplace_back (it);
    }

    if (pDifferent != nullptr)
       *pDifferent  = different;

    return overrides;
  }

  else
  {
    if (pDifferent != nullptr)
       *pDifferent  = false;

    return raw_mice.get ();
  }
}

// Returns all keyboards, in their override state (if applicable)
std::vector <RAWINPUTDEVICE>
SK_RawInput_GetKeyboards (bool* pDifferent = nullptr)
{
  if (raw_overrides.keyboard.active)
  {
    bool different = false;

    std::vector <RAWINPUTDEVICE> overrides;

    // Aw, the game doesn't have any keyboards -- let's fix that.
    if (raw_keyboards->empty ())
    {
      raw_devices->push_back   (
        RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC,
                         HID_USAGE_GENERIC_KEYBOARD, 0x00, nullptr } );
      raw_keyboards->push_back (
        RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC,
                         HID_USAGE_GENERIC_KEYBOARD, 0x00, nullptr } );
      raw_overrides.keyboard.legacy_messages = true;
    }

    for (auto& it : raw_keyboards.get ())
    {
      if (raw_overrides.keyboard.legacy_messages)
      {
        different    |=    ((it).dwFlags & RIDEV_NOLEGACY) != 0;
        (it).dwFlags &= ~(RIDEV_NOLEGACY | RIDEV_APPKEYS);
      }

      else
      {
        different    |=    ((it).dwFlags & RIDEV_NOLEGACY) == 0;
        (it).dwFlags |=   RIDEV_NOLEGACY | RIDEV_APPKEYS;
      }

      overrides.emplace_back (it);
    }

    if (pDifferent != nullptr)
       *pDifferent  = different;

    return overrides;
  }

  else
  {
    if (pDifferent != nullptr)
       *pDifferent  = false;

    //(it).dwFlags &= ~(RIDEV_NOLEGACY | RIDEV_APPKEYS);

    return raw_keyboards;
  }
}

// Temporarily override game's preferences for input device window message generation
bool
SK_RawInput_EnableLegacyMouse (bool enable)
{
  if (! raw_overrides.mouse.active)
  {
    raw_overrides.mouse.active          = true;
    raw_overrides.mouse.legacy_messages = enable;

    // XXX: In the one in a million chance that a game supports multiple mice ...
    //        Special K doesn't distinguish one from the other :P
    //

    bool different = false;

    std::vector <RAWINPUTDEVICE> device_override;
    std::vector <RAWINPUTDEVICE> mice =
      SK_RawInput_GetMice (&different);

    for (auto& it : raw_keyboards.get ()) device_override.push_back (it);
    for (auto& it : raw_gamepads. get ()) device_override.push_back (it);
    for (auto& it : mice)                 device_override.push_back (it);

//    dll_log.Log (L"%lu mice are now legacy...", mice.size ());

    SK_RegisterRawInputDevices ( device_override.data (),
             static_cast <UINT> (device_override.size ()),
                   sizeof (RAWINPUTDEVICE)
    );

    return different;
  }

  return false;
}

// Restore the game's original setting
void
SK_RawInput_RestoreLegacyMouse (void)
{
  if (raw_overrides.mouse.active)
  {
    raw_overrides.mouse.active = false;

    SK_RegisterRawInputDevices (
      raw_devices->data (),
        static_cast <UINT> (raw_devices->size ()),
          sizeof (RAWINPUTDEVICE)
    );
  }
}

// Temporarily override game's preferences for input device window message generation
bool
SK_RawInput_EnableLegacyKeyboard (bool enable)
{
  if (! raw_overrides.keyboard.active)
  {
    raw_overrides.keyboard.active          = true;
    raw_overrides.keyboard.legacy_messages = enable;

    bool different = false;

    std::vector <RAWINPUTDEVICE> device_override;
    std::vector <RAWINPUTDEVICE> keyboards =
      SK_RawInput_GetKeyboards (&different);

    for (auto& it : keyboards)           device_override.push_back (it);
    for (auto& it : raw_gamepads.get ()) device_override.push_back (it);
    for (auto& it : raw_mice.    get ()) device_override.push_back (it);

    SK_RegisterRawInputDevices ( device_override.data (),
             static_cast <UINT> (device_override.size ()),
                   sizeof (RAWINPUTDEVICE)
    );

    return different;
  }

  return false;
}

// Restore the game's original setting
void
SK_RawInput_RestoreLegacyKeyboard (void)
{
  if (raw_overrides.keyboard.active)
  {   raw_overrides.keyboard.active = false;

    SK_RegisterRawInputDevices ( raw_devices->data (),
             static_cast <UINT> (raw_devices->size ()),
                   sizeof (RAWINPUTDEVICE)
    );
  }
}

std::vector <RAWINPUTDEVICE>&
SK_RawInput_GetRegisteredGamepads (void)
{
  return
    raw_gamepads.get ();
}


// Given a complete list of devices in raw_devices, sub-divide into category for
//   quicker device filtering.
void
SK_RawInput_ClassifyDevices (void)
{
  raw_mice->clear      ();
  raw_keyboards->clear ();
  raw_gamepads->clear  ();

  for (auto& it : raw_devices.get ())
  {
    if ((it).usUsagePage == HID_USAGE_PAGE_GENERIC)
    {
      switch ((it).usUsage)
      {
        case HID_USAGE_GENERIC_MOUSE:
          SK_LOG1 ( (L" >> Registered Mouse    (HWND=%p, Flags=%x)",
                       it.hwndTarget, it.dwFlags ),
                     L" RawInput " );

          raw_mice->push_back       (it);
          break;

        case HID_USAGE_GENERIC_KEYBOARD:
          SK_LOG1 ( (L" >> Registered Keyboard (HWND=%p, Flags=%x)",
                       it.hwndTarget, it.dwFlags ),
                     L" RawInput " );

          raw_keyboards->push_back  (it);
          break;

        case HID_USAGE_GENERIC_JOYSTICK: // Joystick
        case HID_USAGE_GENERIC_GAMEPAD:  // Gamepad
          SK_LOG1 ( (L" >> Registered Gamepad  (HWND=%p, Flags=%x)",
                       it.hwndTarget, it.dwFlags ),
                     L" RawInput " );

          raw_gamepads->push_back   (it);
          break;

        default:
          SK_LOG0 ( (L" >> Registered Unknown  (Usage=%x)", it.usUsage),
                     L" RawInput " );

          // UH OH, what the heck is this device?
          break;
      }
    }
  }
}

UINT
SK_RawInput_PopulateDeviceList (void)
{
  raw_devices->clear   ( );
  raw_mice->clear      ( );
  raw_keyboards->clear ( );
  raw_gamepads->clear  ( );

  DWORD            dwLastError = GetLastError ();
  RAWINPUTDEVICE*  pDevices    = nullptr;
  UINT            uiNumDevices = 512;

  UINT ret =
    SK_GetRegisteredRawInputDevices ( nullptr,
                                        &uiNumDevices,
                                          sizeof (RAWINPUTDEVICE) );

  assert (ret == -1);

  SK_SetLastError (dwLastError);

  if (uiNumDevices != 0 && sk::narrow_cast <int> (ret) == -1 && pDevices != nullptr)
  {
    SK_TLS* pTLS =
      SK_TLS_Bottom ();

    pDevices =
      pTLS->raw_input->allocateDevices (uiNumDevices + 1ull);

    SK_GetRegisteredRawInputDevices ( pDevices,
                                     &uiNumDevices,
                                       sizeof (RAWINPUTDEVICE) );

    raw_devices->clear ();
    raw_devices->reserve (uiNumDevices);

    for (UINT i = 0; i < uiNumDevices; i++)
      raw_devices->push_back (pDevices [i]);

    SK_RawInput_ClassifyDevices ();
  }

  return
    uiNumDevices;
}

UINT
WINAPI
GetRegisteredRawInputDevices_Detour (
  _Out_opt_ PRAWINPUTDEVICE pRawInputDevices,
  _Inout_   PUINT           puiNumDevices,
  _In_      UINT            cbSize )
{
  SK_LOG_FIRST_CALL

  static std::mutex getDevicesMutex;

  std::scoped_lock <std::mutex>
          get_lock (getDevicesMutex);

  std::scoped_lock <std::mutex>
         view_lock (raw_device_view);

  // OOPS?!
  if ( cbSize != sizeof (RAWINPUTDEVICE) || puiNumDevices == nullptr ||
                     (pRawInputDevices &&  *puiNumDevices == 0) )
  {
    SetLastError (ERROR_INVALID_PARAMETER);

    return ~0U;
  }

  // On the first call to this function, we will need to query this stuff.
  static bool        init         = false;
  if (std::exchange (init, true) == false)
  {
    SK_RawInput_PopulateDeviceList ();
  }

  if (*puiNumDevices < sk::narrow_cast <UINT> (raw_devices->size ()))
  {   *puiNumDevices = sk::narrow_cast <UINT> (raw_devices->size ());

    SetLastError (ERROR_INSUFFICIENT_BUFFER);

    return ~0U;
  }

  unsigned int idx      = 0;
  unsigned int num_devs = sk::narrow_cast <UINT> (raw_devices->size ());

  if (pRawInputDevices != nullptr)
  {
    for (idx = 0 ; idx < num_devs ; ++idx)
    {
      pRawInputDevices [idx] = raw_devices.get ()[idx];
    }
  }

  return num_devs;
}

BOOL
WINAPI
RegisterRawInputDevices_Detour (
  _In_ PCRAWINPUTDEVICE pRawInputDevices,
  _In_ UINT             uiNumDevices,
  _In_ UINT             cbSize )
{
  SK_LOG_FIRST_CALL

  if (cbSize != sizeof (RAWINPUTDEVICE))
  {
    SK_LOG0 ( ( L"RegisterRawInputDevices has wrong "
                L"structure size (%lu bytes), expected: %lu",
                  cbSize, (UINT)sizeof (RAWINPUTDEVICE) ),
               L" RawInput " );

    return
      SK_RegisterRawInputDevices ( pRawInputDevices,
                                     uiNumDevices,
                                       cbSize );
  }

#ifdef __MANAGE_RAW_INPUT_REGISTRATION
  static std::mutex registerDevicesMutex;

  std::scoped_lock <std::mutex>
     register_lock (registerDevicesMutex);

  std::scoped_lock <std::mutex>
         view_lock (raw_device_view);

  raw_devices->clear ();
#endif

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  std::unique_ptr <RAWINPUTDEVICE []> pLocalDevices;
  RAWINPUTDEVICE*                     pDevices = nullptr;

  if (pRawInputDevices != nullptr && uiNumDevices > 0 && pTLS != nullptr)
  {
    pDevices =
      pTLS->raw_input->allocateDevices (uiNumDevices);
  }

#ifdef __MANAGE_RAW_INPUT_REGISTRATION
  // The devices that we will pass to Windows after any overrides are applied
  std::vector <RAWINPUTDEVICE> actual_device_list;
#endif

  if (pDevices != nullptr)
  {
    // We need to continue receiving window messages for the console to work
    for (     size_t i = 0            ;
                     i < uiNumDevices ;
                   ++i )
    {      pDevices [i] =
   pRawInputDevices [i];

    bool match_any_in_page = 
      (pDevices [i].dwFlags & RIDEV_PAGEONLY) && pDevices [i].usUsage == 0;

#define _ALLOW_BACKGROUND_GAMEPAD
#define _ALLOW_LEGACY_MOUSE
#define _ALLOW_LEGACY_KEYBOARD

#ifdef _ALLOW_BACKGROUND_GAMEPAD
      if ( pDevices [i].hwndTarget  !=   0                                 &&
           pDevices [i].usUsagePage == HID_USAGE_PAGE_GENERIC              &&
         ( match_any_in_page                                               ||
           pDevices [i].usUsage     == HID_USAGE_GENERIC_JOYSTICK          ||
           pDevices [i].usUsage     == HID_USAGE_GENERIC_GAMEPAD           ||
           pDevices [i].usUsage     == HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER ) )
      {
        if (! match_any_in_page)
          pDevices [i].dwFlags      |= RIDEV_INPUTSINK;
      }
#endif

#ifdef _ALLOW_LEGACY_KEYBOARD
      if (pDevices [i].usUsagePage ==  HID_USAGE_PAGE_GENERIC &&
         (pDevices [i].usUsage     ==  HID_USAGE_GENERIC_KEYBOARD ||
          match_any_in_page))
      {
        if (! match_any_in_page) // This only works for Mouse and Keyboard Usages
          pDevices [i].dwFlags     &= ~RIDEV_NOLEGACY;

        if ((pDevices [i].dwFlags & RIDEV_APPKEYS) &&
          (!(pDevices [i].dwFlags & RIDEV_NOLEGACY)))
        {
          SK_LOGi0 (
            L"Game tried to register a device with RIDEV_APPKEYS flagged, but "
            L"this requires RIDEV_NOLEGACY and SK has removed that flag..."
          );

          pDevices [i].dwFlags &= ~RIDEV_APPKEYS;
        }

        if (pDevices [i].dwFlags & RIDEV_NOHOTKEYS)
        {
          SK_LOGi0 (
            L"Game has registered a keyboard with the RIDEV_NOHOTKEYS flag..."
          );
        }
      }
#endif

#ifdef _ALLOW_LEGACY_MOUSE
      if (pDevices [i].usUsagePage ==  HID_USAGE_PAGE_GENERIC &&
         (pDevices [i].usUsage     ==  HID_USAGE_GENERIC_MOUSE ||
           match_any_in_page))
      {
        // It is unnecessary for SK to enable legacy mouse APIs, because
        //   SK will fallback to a low-level mouse hook.
        //
        //  The only time legacy mouse messages might need to be enabled
        //    is for third-party overlays.
        if (! match_any_in_page) // This only works for Mouse and Keyboard Usages
          pDevices [i].dwFlags &= ~RIDEV_NOLEGACY;

        if ((pDevices [i].dwFlags & RIDEV_CAPTUREMOUSE) &&
          (!(pDevices [i].dwFlags & RIDEV_NOLEGACY)))
        {
          SK_LOGi0 (
            L"Game tried to register a device with RIDEV_CAPTUREMOUSE flagged, but "
            L"this requires RIDEV_NOLEGACY and SK has removed that flag..."
          );

          pDevices [i].dwFlags &= ~RIDEV_CAPTUREMOUSE;
        }
      }
#endif

#ifdef __MANAGE_RAW_INPUT_REGISTRATION
      raw_devices->push_back (pDevices [i]);
#endif

      if ( pDevices [i].hwndTarget != nullptr          &&
           pDevices [i].hwndTarget != game_window.hWnd &&
  IsChild (pDevices [i].hwndTarget,   game_window.hWnd) == false )
      {
        static wchar_t wszWindowClass [128] = { };
        static wchar_t wszWindowTitle [128] = { };

        RealGetWindowClassW   (pDevices [i].hwndTarget, wszWindowClass, 127);
        InternalGetWindowText (pDevices [i].hwndTarget, wszWindowTitle, 127);

        SK_LOG1 (
                  ( L"RawInput is being tracked on hWnd=%p - { (%s), '%s' }",
                      pDevices [i].hwndTarget, wszWindowClass, wszWindowTitle ),
                        __SK_SUBSYSTEM__ );
      }
    }

#ifdef __MANAGE_RAW_INPUT_REGISTRATION
    SK_RawInput_ClassifyDevices ();
#endif
  }

#ifdef __MANAGE_RAW_INPUT_REGISTRATION
  std::vector <RAWINPUTDEVICE> override_keyboards = SK_RawInput_GetKeyboards          ();
  std::vector <RAWINPUTDEVICE> override_mice      = SK_RawInput_GetMice               ();
  std::vector <RAWINPUTDEVICE> override_gamepads  = SK_RawInput_GetRegisteredGamepads ();

  for (auto& it : override_keyboards) actual_device_list.push_back (it);
  for (auto& it : override_mice)      actual_device_list.push_back (it);
  for (auto& it : override_gamepads)  actual_device_list.push_back (it);

  BOOL bRet =
    pDevices != nullptr ?
      SK_RegisterRawInputDevices ( actual_device_list.data   (),
               static_cast <UINT> (actual_device_list.size () ),
                                     cbSize ) :
                FALSE;
#else
  BOOL bRet =
    SK_RegisterRawInputDevices ( pDevices,
                                   uiNumDevices,
                                     cbSize );

  if (! bRet)
  {
    SK_LOGi0 (
      L"Game's call to RegisterRawInputDevices (...) failed with ErrorCode=%d",
      GetLastError ()
    );
  }
#endif

  return bRet;
}

RegisterRawInputDevices_pfn RegisterRawInputDevices_Original = nullptr;
GetRawInputData_pfn         GetRawInputData_Original         = nullptr;
GetRawInputBuffer_pfn       GetRawInputBuffer_Original       = nullptr;


UINT
WINAPI
SK_GetRawInputData (HRAWINPUT hRawInput,
                    UINT      uiCommand,
                    LPVOID    pData,
                    PUINT     pcbSize,
                    UINT      cbSizeHeader)
{
  if (GetRawInputData_Original != nullptr)
  {
    return
      GetRawInputData_Original (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
  }

  return
    GetRawInputData (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
}

UINT
WINAPI
SK_GetRegisteredRawInputDevices (PRAWINPUTDEVICE pRawInputDevices,
                                 PUINT           puiNumDevices,
                                 UINT            cbSize)
{
  if (GetRegisteredRawInputDevices_Original != nullptr)
    return GetRegisteredRawInputDevices_Original (pRawInputDevices, puiNumDevices, cbSize);

  return GetRegisteredRawInputDevices (pRawInputDevices, puiNumDevices, cbSize);
}

BOOL
WINAPI
SK_RegisterRawInputDevices (PCRAWINPUTDEVICE pRawInputDevices,
                            UINT             uiNumDevices,
                            UINT             cbSize)
{
  if (RegisterRawInputDevices_Original != nullptr)
    return RegisterRawInputDevices_Original (pRawInputDevices, uiNumDevices, cbSize);

  return RegisterRawInputDevices (pRawInputDevices, uiNumDevices, cbSize);
}

//
// Halo: Infinite was the first game ever encountered using Buffered RawInput,
//   so this has only been tested working in that game.
//
UINT
WINAPI
GetRawInputBuffer_Detour (_Out_opt_ PRAWINPUT pData,
                          _Inout_   PUINT     pcbSize,
                          _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  // Game wants to know size to allocate, let it pass-through
  if (pData == nullptr)
  {
    return
      GetRawInputBuffer_Original (pData, pcbSize, cbSizeHeader);
  }

  static ImGuiIO& io =
    ImGui::GetIO ();

  bool filter = false;

  // Unconditional
  if (config.input.ui.capture)
    filter = true;

  using QWORD = __int64;

  if (pData != nullptr)
  {
    const int max_items = (((size_t)*pcbSize * 16) / sizeof (RAWINPUT));
          int count     =                              0;
        auto *pTemp     =
          (RAWINPUT *)SK_TLS_Bottom ()->raw_input->allocData ((size_t)*pcbSize * 16);
    RAWINPUT *pInput    =                          pTemp;
    RAWINPUT *pOutput   =                          pData;
    UINT     cbSize     =                       *pcbSize;
              *pcbSize  =                              0;

    int       temp_ret  =
      GetRawInputBuffer_Original ( pTemp, &cbSize, cbSizeHeader );

    // Common usage involves calling this with a wrong sized buffer, then calling it again...
    //   early-out if it returns -1.
    if (sk::narrow_cast <INT> (temp_ret) < 0 || max_items == 0)
      return temp_ret;

    auto* pItem =
          pInput;

    // Sanity check required array storage even though TLS will
    //   allocate more than enough.
    SK_ReleaseAssert (temp_ret < max_items);

    for (int i = 0; i < temp_ret; i++)
    {
      bool remove = false;

      switch (pItem->header.dwType)
      {
        case RIM_TYPEKEYBOARD:
        {
          SK_RAWINPUT_READ (sk_input_dev_type::Keyboard)
          if (filter || SK_ImGui_WantKeyboardCapture ())
          {
            SK_RAWINPUT_HIDE (sk_input_dev_type::Keyboard);
            remove = true;
          }
          else
            SK_RAWINPUT_VIEW (sk_input_dev_type::Keyboard);

        //// Leads to double-input processing, left here in case Legacy Messages are disabled and this is needed
        ////
#if 0
          USHORT VKey =
            (((RAWINPUT *)pData)->data.keyboard.VKey & 0xFF);

          static auto* pConsole =
            SK_Console::getInstance ();

        //if (!(((RAWINPUT *) pItem)->data.keyboard.Flags & RI_KEY_BREAK))
        //{
        //  pConsole->KeyDown (VKey & 0xFF, MAXDWORD);
        //        io.KeysDown [VKey & 0xFF] = SK_IsGameWindowActive ();
        //}
          if (game_window.active)
          {
            switch (((RAWINPUT *) pData)->data.keyboard.Message)
            {
              case WM_KEYDOWN:
              case WM_SYSKEYDOWN:
                      io.KeysDown [VKey & 0xFF] = SK_IsGameWindowActive ();
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

        case RIM_TYPEMOUSE:
          SK_RAWINPUT_READ (sk_input_dev_type::Mouse)
          if (filter || SK_ImGui_WantMouseCapture ())
          {
            SK_RAWINPUT_HIDE (sk_input_dev_type::Mouse);
            remove = true;
          }
          else
            SK_RAWINPUT_VIEW (sk_input_dev_type::Mouse);
          break;

        default:
          SK_RAWINPUT_READ (sk_input_dev_type::Gamepad)
          if (filter || SK_ImGui_WantGamepadCapture () || config.input.gamepad.disable_hid)
          {
            SK_RAWINPUT_HIDE (sk_input_dev_type::Gamepad);
            remove = true;
          }
          else
            SK_RAWINPUT_VIEW (sk_input_dev_type::Gamepad);
          break;
      }

      if (config.input.ui.capture)
        remove = true;

      // If item is not removed, append it to the buffer of RAWINPUT
      //   packets we are allowing the game to see
      if (! remove)
      {
        memcpy (pOutput, pItem, pItem->header.dwSize);
                pOutput = NEXTRAWINPUTBLOCK (pOutput);

        ++count;
      }

      else
      {
        bool keyboard = pItem->header.dwType == RIM_TYPEKEYBOARD;
        bool mouse    = pItem->header.dwType == RIM_TYPEMOUSE;

        // Clearing all bytes above would have set the type to mouse, and some games
        //   will actually read data coming from RawInput even when the size returned is 0!
        pItem->header.dwType = keyboard ? RIM_TYPEKEYBOARD :
                                  mouse ? RIM_TYPEMOUSE    :
                                          RIM_TYPEHID;

        // Supplying an invalid device will early-out SDL before it calls HID APIs to try
        //   and get an input report that we don't want it to see...
        pItem->header.hDevice = nullptr;
  
        //SK_ReleaseAssert (*pcbSize >= static_cast <UINT> (size) &&
        //                  *pcbSize >= sizeof (RAWINPUTHEADER));
  
        pItem->header.wParam = RIM_INPUTSINK;
    
        if (keyboard)
        {
          if (! (pItem->data.keyboard.Flags & RI_KEY_BREAK))
                 pItem->data.keyboard.VKey  = 0;
    
          // Fake key release
          pItem->data.keyboard.Flags |= RI_KEY_BREAK;
        }
    
        // Block mouse input in The Witness by zeroing-out the memory; most other 
        //   games will see *pcbSize=0 and RIM_INPUTSINK and not process input...
        else
        {
          RtlZeroMemory ( &pItem->data.mouse,
                           pItem->header.dwSize - sizeof (RAWINPUTHEADER) );
        }

        memcpy (pOutput, pItem, pItem->header.dwSize);
                pOutput = NEXTRAWINPUTBLOCK (pOutput);

        ++count;
      }

      pItem = NEXTRAWINPUTBLOCK (pItem);
    }

    *pcbSize =
      (UINT)((uintptr_t)pOutput - (uintptr_t)pData);

    return count;
  }

  return
    GetRawInputBuffer_Original (pData, pcbSize, cbSizeHeader);
}

INT
WINAPI
SK_ImGui_ProcessRawInput (_In_      HRAWINPUT hRawInput,
                          _In_      UINT      uiCommand,
                          _Out_opt_ LPVOID    pData,
                          _Inout_   PUINT     pcbSize,
                          _In_      UINT      cbSizeHeader,
                                    BOOL      self,
                                    INT       precache_size = 0);

UINT
WINAPI
GetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                        _In_      UINT      uiCommand,
                        _Out_opt_ LPVOID    pData,
                        _Inout_   PUINT     pcbSize,
                        _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  auto GetRawInputDataImpl = [&](void) ->
  UINT
  {
    return 
      (UINT)SK_ImGui_ProcessRawInput ( hRawInput, uiCommand,
                                           pData, pcbSize,
                                    cbSizeHeader, FALSE );
  };

#if 1
  return
    GetRawInputDataImpl ();

  // The optimization below breaks a game called Run 8, it is not possible to
  //   debug the game without buying it and the performance increase in Unreal
  //     is negligible in most games.
#else
  // Something we don't know how to process
  if ((uiCommand & (RID_INPUT | RID_HEADER)) == 0)
    return ~0U;

  //
  // Optimization for Unreal Engine games; it calls GetRawInputData (...) twice
  // 
  //   * First call has nullptr for pData and ridiculous size, second call has
  //       an actual pData pointer.
  // 
  //  >> The problem with this approach is BOTH calls have the same overhead!
  //       
  //   We will pre-read the input and hand that to Unreal Engine on its second
  //     call rather than going through the full API.
  //
  if (pcbSize != nullptr && cbSizeHeader == sizeof (RAWINPUTHEADER))
  {
    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    if (! pTLS)
    {
      return
        GetRawInputDataImpl ();
    }

    auto& lastRawInput =
      pTLS->raw_input->cached_input;

    if (lastRawInput.hRawInput != hRawInput && (pData == nullptr || uiCommand == RID_HEADER))
    {
      SK_LOGi3 ( L"RawInput Cache Miss [GetSize or Header]: %p (tid=%x)",
                  hRawInput, GetCurrentThreadId () );

      lastRawInput.hRawInput  = hRawInput;
      lastRawInput.uiCommand  = RID_INPUT;
      lastRawInput.SizeHeader = cbSizeHeader;
      lastRawInput.Size       = sizeof (lastRawInput.Data);

      INT size =
        SK_ImGui_ProcessRawInput (
          lastRawInput.hRawInput, lastRawInput.uiCommand,
          lastRawInput.Data,     &lastRawInput.Size,
                                  lastRawInput.SizeHeader,
                                                    FALSE, -1 );

      if (size >= 0)
      {
        lastRawInput.Size = size;

        if (pData == nullptr)
        {
          SK_ReleaseAssert ((uiCommand & RID_HEADER) == 0);

          *pcbSize =
            lastRawInput.Size;

          return 0;
        }

        else if (uiCommand & RID_HEADER)
        {
          if (*pcbSize < cbSizeHeader)
            return ~0U;

          std::memcpy (pData, lastRawInput.Data, cbSizeHeader);

          return
            cbSizeHeader;
        }
      }

      return size;
    }

    else if (hRawInput == lastRawInput.hRawInput)
    {
      SK_LOGi3 (L"RawInput Cache Hit: %p", lastRawInput.hRawInput);

      UINT uiRequiredSize =
        ( uiCommand == RID_HEADER ? cbSizeHeader
                                  : lastRawInput.Size );

      if (*pcbSize < uiRequiredSize)
        return ~0U;

      if (pData != nullptr)
        std::memcpy (pData, lastRawInput.Data, uiRequiredSize);

      return
        uiRequiredSize;
    }

    else
    {
      lastRawInput.hRawInput  = hRawInput;
      lastRawInput.uiCommand  = RID_INPUT;
      lastRawInput.SizeHeader = cbSizeHeader;
      lastRawInput.Size       = sizeof (lastRawInput.Data);

      INT size =
        SK_ImGui_ProcessRawInput (
          lastRawInput.hRawInput, lastRawInput.uiCommand,
          lastRawInput.Data,     &lastRawInput.Size,
                                  lastRawInput.SizeHeader,
                                                    FALSE, -2 );

      if (size >= 0)
      {
        lastRawInput.Size = size;

        UINT uiRequiredSize =
          ( uiCommand == RID_HEADER ? cbSizeHeader
                                    : lastRawInput.Size );

        SK_LOGi3 ( L"RawInput Cache Miss [GetData]: %p (tid=%x)",
                    hRawInput, GetCurrentThreadId () );

        if (*pcbSize < uiRequiredSize)
          return ~0U;

        if (pData != nullptr)
          std::memcpy (pData, lastRawInput.Data, uiRequiredSize);

        return
          uiRequiredSize;
      }

      return size;
    }
  }

  return
    GetRawInputDataImpl ();
#endif
}

UINT
SK_Input_ClassifyRawInput ( HRAWINPUT lParam,
                            bool&     mouse,
                            bool&     keyboard,
                            bool&     gamepad )
{
        RAWINPUT data = { };
        UINT     size = sizeof (RAWINPUT);
  const UINT     ret  =
    SK_ImGui_ProcessRawInput ( (HRAWINPUT)lParam,
                                 RID_INPUT,
                                   &data, &size,
                                     sizeof (RAWINPUTHEADER), TRUE );

  if (ret == size)
  {
    switch (data.header.dwType)
    {
      case RIM_TYPEMOUSE:
        mouse = true;
        break;

      case RIM_TYPEKEYBOARD:
        // TODO: Post-process this; the RawInput API also duplicates the legacy mouse buttons as keys.
        //
        //         We need to treat events reflecting those keys as MOUSE events.
        keyboard = true;
        break;

      default:
        gamepad = true;
        break;
    }

    return
      data.header.dwSize;
  }

  return 0;
}

void
SK_Input_HookRawInput (void)
{
//#define InputHook(dll_name,function)                                 \
//  SK_CreateDLLHook2 ( L#dll_name, #function, ##function##_Detour,    \
//                     static_cast_p2p <void> (&##function##_Original) );


#ifdef  __MANAGE_RAW_INPUT_REGISTRATION
  SK_RunOnce (
  {
    if (config.input.gamepad.hook_raw_input)
    {
      SK_CreateDLLHook2 (       L"user32",
                                 "GetRegisteredRawInputDevices",
                                  GetRegisteredRawInputDevices_Detour,
         static_cast_p2p <void> (&GetRegisteredRawInputDevices_Original) );
    }
  });
#endif

  SK_RunOnce (
  {
    if (config.input.gamepad.hook_raw_input)
    {
      // Even if not managing Raw Input devices, we need to prevent games
      //   from disabling legacy Win32 input messages for SK's overlay to
      //     work properly with buffered keyboard input.
      SK_CreateDLLHook2 (       L"user32",
                                 "RegisterRawInputDevices",
                                  RegisterRawInputDevices_Detour,
         static_cast_p2p <void> (&RegisterRawInputDevices_Original) );

      SK_CreateDLLHook2 (       L"user32",
                                 "GetRawInputData",
                                  GetRawInputData_Detour,
         static_cast_p2p <void> (&GetRawInputData_Original) );
    }
  });

#ifdef __HOOK_BUFFERED_RAW_INPUT
  SK_RunOnce (
  {
    if (config.input.gamepad.hook_raw_input)
    {
      SK_CreateDLLHook2 (      L"user32",
                                "GetRawInputBuffer",
                                 GetRawInputBuffer_Detour,
        static_cast_p2p <void> (&GetRawInputBuffer_Original) );
    }
  });
#endif
}