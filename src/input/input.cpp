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

#define __SK_SUBSYSTEM__ L"Input Mgr."

#include <SpecialK/command.h>

bool
SK_InputUtil_IsHWCursorVisible (void)
{
  CURSORINFO cursor_info        = { };
             cursor_info.cbSize = sizeof (CURSORINFO);

  SK_GetCursorInfo (&cursor_info);

  return
    ( cursor_info.flags & CURSOR_SHOWING );
}

#define SK_HID_READ(type)  SK_HID_Backend->markRead  (type);
#define SK_HID_WRITE(type) SK_HID_Backend->markWrite (type);

#define SK_RAWINPUT_READ(type)  SK_RawInput_Backend->markRead  (type);
#define SK_RAWINPUT_WRITE(type) SK_RawInput_Backend->markWrite (type);


//////////////////////////////////////////////////////////////
//
// HIDClass (User mode)
//
//////////////////////////////////////////////////////////////
HidD_GetPreparsedData_pfn  HidD_GetPreparsedData_Original  = nullptr;
HidD_FreePreparsedData_pfn HidD_FreePreparsedData_Original = nullptr;
HidD_GetFeature_pfn        HidD_GetFeature_Original        = nullptr;
HidP_GetData_pfn           HidP_GetData_Original           = nullptr;
HidP_GetCaps_pfn           HidP_GetCaps_Original           = nullptr;

bool
SK_HID_FilterPreparsedData (PHIDP_PREPARSED_DATA pData)
{
  bool filter = false;

        HIDP_CAPS caps;
  const NTSTATUS  stat =
          HidP_GetCaps_Original (pData, &caps);

  if ( stat           == HIDP_STATUS_SUCCESS &&
       caps.UsagePage == HID_USAGE_PAGE_GENERIC )
  {
    switch (caps.Usage)
    {
      case HID_USAGE_GENERIC_GAMEPAD:
      case HID_USAGE_GENERIC_JOYSTICK:
      {
        SK_HID_READ (sk_input_dev_type::Gamepad)

        if (SK_ImGui_WantGamepadCapture () && (! config.input.gamepad.native_ps4))
        {
          filter = true;
        }
      } break;

      case HID_USAGE_GENERIC_POINTER:
      case HID_USAGE_GENERIC_MOUSE:
      {
        SK_HID_READ (sk_input_dev_type::Mouse)

        if (SK_ImGui_WantMouseCapture ())
        {
          filter = true;
        }
      } break;

      case HID_USAGE_GENERIC_KEYBOARD:
      case HID_USAGE_GENERIC_KEYPAD:
      {
        SK_HID_READ (sk_input_dev_type::Keyboard)

        if (SK_ImGui_WantKeyboardCapture ())
        {
          filter = true;
        }
      } break;
    }
  }

  //SK_LOG0 ( ( L"HID Preparsed Data - Stat: %04x, UsagePage: %02x, Usage: %02x",
                //stat, caps.UsagePage, caps.Usage ),
              //L" HIDInput ");

  return filter;
}

PHIDP_PREPARSED_DATA* SK_HID_PreparsedDataP = nullptr;
PHIDP_PREPARSED_DATA  SK_HID_PreparsedData  = nullptr;

_Must_inspect_result_
_Success_(return==TRUE)
BOOLEAN __stdcall
HidD_GetPreparsedData_Detour (
  _In_  HANDLE                HidDeviceObject,
  _Out_ PHIDP_PREPARSED_DATA *PreparsedData )
{
  SK_LOG_FIRST_CALL

  PHIDP_PREPARSED_DATA pData = nullptr;
  BOOLEAN              bRet  =
    HidD_GetPreparsedData_Original ( HidDeviceObject,
                                       &pData );

  if (bRet && pData != nullptr)
  {
    SK_HID_PreparsedDataP = PreparsedData;
    SK_HID_PreparsedData  = pData;

    if (SK_HID_FilterPreparsedData (pData) || config.input.gamepad.disable_ps4_hid)
    {
      if (HidD_FreePreparsedData_Original (pData))
        return FALSE;

      assert (false && L"The Sky is Falling!");

      return FALSE;
    }

    *PreparsedData   =  pData;
  }

  // Can't figure out how The Witness works yet, but it will bypass input blocking
  //   on HID using a PS4 controller unless we return FALSE here.
  //return FALSE;
  return bRet;
}

SetCursor_pfn NtUserSetCursor_Original = nullptr;
GetCursor_pfn NtUserGetCursor_Original = nullptr;

GetCursorInfo_pfn       GetCursorInfo_Original       = nullptr;
NtUserGetCursorInfo_pfn NtUserGetCursorInfo_Original = nullptr;

BOOLEAN
__stdcall
HidD_FreePreparsedData_Detour (
  _In_ PHIDP_PREPARSED_DATA PreparsedData )
{
  BOOLEAN bRet =
    HidD_FreePreparsedData_Original (PreparsedData);

  if (SK_HID_PreparsedData == PreparsedData)
      SK_HID_PreparsedData = nullptr;

  return bRet;
}

BOOLEAN
_Success_ (return)
__stdcall
HidD_GetFeature_Detour ( _In_  HANDLE HidDeviceObject,
                         _Out_ PVOID  ReportBuffer,
                         _In_  ULONG  ReportBufferLength )
{
  ////SK_HID_READ (sk_input_dev_type::Gamepad)

  bool                 filter = false;
  PHIDP_PREPARSED_DATA pData  = nullptr;

  if (HidD_GetPreparsedData_Original (HidDeviceObject, &pData))
  {
    if (SK_HID_FilterPreparsedData (pData))
      filter = true;

    HidD_FreePreparsedData_Original (pData);
  }

  if (! filter)
    return HidD_GetFeature_Original ( HidDeviceObject, ReportBuffer, ReportBufferLength );

  return FALSE;
}

NTSTATUS
__stdcall
HidP_GetData_Detour (
  _In_    HIDP_REPORT_TYPE     ReportType,
  _Out_   PHIDP_DATA           DataList,
  _Inout_ PULONG               DataLength,
  _In_    PHIDP_PREPARSED_DATA PreparsedData,
  _In_    PCHAR                Report,
  _In_    ULONG                ReportLength )
{
  SK_LOG_FIRST_CALL

  NTSTATUS ret =
    HidP_GetData_Original ( ReportType, DataList,
                              DataLength, PreparsedData,
                                Report, ReportLength );


  // De we want block this I/O?
  bool filter = false;

  if ( ret == HIDP_STATUS_SUCCESS && ( ReportType == HidP_Input || ReportType == HidP_Output ))
  {
    // This will classify the data for us, so don't record this event yet.
    filter = SK_HID_FilterPreparsedData (PreparsedData);
  }



  if (filter && (DataLength != nullptr))
  {
    memset (DataList, 0, *DataLength);
           *DataLength = 0;
  }

  return ret;
}

void
SK_Input_HookHID (void)
{
  if (! config.input.gamepad.hook_hid)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    SK_LOG0 ( ( L"Game uses HID, installing input hooks..." ),
                L"  Input   " );

    SK_CreateDLLHook2 (     L"HID.DLL",
                             "HidP_GetData",
                              HidP_GetData_Detour,
     static_cast_p2p <void> (&HidP_GetData_Original) );

    SK_CreateDLLHook2 (     L"HID.DLL",
                             "HidD_GetPreparsedData",
                              HidD_GetPreparsedData_Detour,
     static_cast_p2p <void> (&HidD_GetPreparsedData_Original) );

    SK_CreateDLLHook2 (     L"HID.DLL",
                             "HidD_FreePreparsedData",
                              HidD_FreePreparsedData_Detour,
     static_cast_p2p <void> (&HidD_FreePreparsedData_Original) );

    SK_CreateDLLHook2 (     L"HID.DLL",
                             "HidD_GetFeature",
                              HidD_GetFeature_Detour,
     static_cast_p2p <void> (&HidD_GetFeature_Original) );

    HidP_GetCaps_Original =
      (HidP_GetCaps_pfn)GetProcAddress ( SK_GetModuleHandle (L"HID.DLL"),
                                           "HidP_GetCaps" );

    if (SK_GetFramesDrawn () > 1)
      SK_ApplyQueuedHooks ();

    InterlockedIncrementRelease (&hooked);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}

bool
SK_Input_PreHookHID (void)
{
  if (! config.input.gamepad.hook_hid)
    return false;

  static sk_import_test_s tests [] = { { "hid.dll", false } };

  SK_TestImports (GetModuleHandle (nullptr), tests, 1);

  if (tests [0].used)// || GetModuleHandle (L"hid.dll"))
  {
    SK_Input_HookHID ();

    return true;
  }

  return false;
}


//////////////////////////////////////////////////////////////////////////////////
//
// Raw Input
//
//////////////////////////////////////////////////////////////////////////////////
std::vector <RAWINPUTDEVICE> raw_devices;   // ALL devices, this is the list as Windows would give it to the game

std::vector <RAWINPUTDEVICE> raw_mice;      // View of only mice
std::vector <RAWINPUTDEVICE> raw_keyboards; // View of only keyboards
std::vector <RAWINPUTDEVICE> raw_gamepads;  // View of only game pads

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
    if (raw_mice.empty ())
    {
      raw_devices.push_back (
        RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC,
                         HID_USAGE_GENERIC_MOUSE, 0x00, nullptr } );
      raw_mice.push_back    (
        RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC,
                         HID_USAGE_GENERIC_MOUSE, 0x00, nullptr } );
      raw_overrides.mouse.legacy_messages = true;
    }

    for (auto& it : raw_mice)
    {
      HWND hWnd = it.hwndTarget;

      if (raw_overrides.mouse.legacy_messages)
      {
        different    |= (it.dwFlags & RIDEV_NOLEGACY) != 0;
        it.dwFlags   &= ~(RIDEV_NOLEGACY | RIDEV_APPKEYS | RIDEV_REMOVE);
        it.dwFlags   &= ~RIDEV_CAPTUREMOUSE;
        it.hwndTarget = hWnd;
        SK_RegisterRawInputDevices ( &it, 1, sizeof RAWINPUTDEVICE );
      }

      else
      {
        different  |= (it.dwFlags & RIDEV_NOLEGACY) == 0;
        it.dwFlags |= RIDEV_NOLEGACY;
        SK_RegisterRawInputDevices ( &it, 1, sizeof RAWINPUTDEVICE );
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

    return raw_mice;
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
    if (raw_keyboards.empty ())
    {
      raw_devices.push_back   (
        RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC,
                         HID_USAGE_GENERIC_KEYBOARD, 0x00, nullptr } );
      raw_keyboards.push_back (
        RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC,
                         HID_USAGE_GENERIC_KEYBOARD, 0x00, nullptr } );
      raw_overrides.keyboard.legacy_messages = true;
    }

    for (auto& it : raw_keyboards)
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

    for (auto& it : raw_keyboards) device_override.push_back (it);
    for (auto& it : raw_gamepads)  device_override.push_back (it);
    for (auto& it : mice)          device_override.push_back (it);

//    dll_log.Log (L"%lu mice are now legacy...", mice.size ());

    SK_RegisterRawInputDevices ( device_override.data (),
             static_cast <UINT> (device_override.size ()),
                   sizeof RAWINPUTDEVICE
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
      raw_devices.data (),
        static_cast <UINT> (raw_devices.size ()),
          sizeof RAWINPUTDEVICE
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

    for (auto& it : keyboards)    device_override.push_back (it);
    for (auto& it : raw_gamepads) device_override.push_back (it);
    for (auto& it : raw_mice)     device_override.push_back (it);

    SK_RegisterRawInputDevices ( device_override.data (),
             static_cast <UINT> (device_override.size ()),
                   sizeof RAWINPUTDEVICE
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

    SK_RegisterRawInputDevices ( raw_devices.data (),
             static_cast <UINT> (raw_devices.size ()),
                   sizeof RAWINPUTDEVICE
    );
  }
}

std::vector <RAWINPUTDEVICE>&
SK_RawInput_GetRegisteredGamepads (void)
{
  return
    raw_gamepads;
}


// Given a complete list of devices in raw_devices, sub-divide into category for
//   quicker device filtering.
void
SK_RawInput_ClassifyDevices (void)
{
  raw_mice.clear      ();
  raw_keyboards.clear ();
  raw_gamepads.clear  ();

  for (auto& it : raw_devices)
  {
    if ((it).usUsagePage == HID_USAGE_PAGE_GENERIC)
    {
      switch ((it).usUsage)
      {
        case HID_USAGE_GENERIC_MOUSE:
          SK_LOG1 ( (L" >> Registered Mouse    (HWND=%x, Flags=%x)",
                       it.hwndTarget, it.dwFlags ),
                     L" RawInput " );

          raw_mice.push_back       (it);
          break;

        case HID_USAGE_GENERIC_KEYBOARD:
          SK_LOG1 ( (L" >> Registered Keyboard (HWND=%x, Flags=%x)",
                       it.hwndTarget, it.dwFlags ),
                     L" RawInput " );

          raw_keyboards.push_back  (it);
          break;

        case HID_USAGE_GENERIC_JOYSTICK: // Joystick
        case HID_USAGE_GENERIC_GAMEPAD:  // Gamepad
          SK_LOG1 ( (L" >> Registered Gamepad  (HWND=%x, Flags=%x)",
                       it.hwndTarget, it.dwFlags ),
                     L" RawInput " );

          raw_gamepads.push_back   (it);
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
  raw_devices.clear   ( );
  raw_mice.clear      ( );
  raw_keyboards.clear ( );
  raw_gamepads.clear  ( );

  DWORD            dwLastError = GetLastError ();
  RAWINPUTDEVICE*  pDevices    = nullptr;
  UINT            uiNumDevices = 0;

  UINT ret =
    SK_GetRegisteredRawInputDevices ( pDevices,
                                        &uiNumDevices,
                                          sizeof RAWINPUTDEVICE );

  assert (ret == -1);

  SetLastError (dwLastError);

  if (uiNumDevices != 0 && ret == -1)
  {
    SK_TLS* pTLS =
      SK_TLS_Bottom ();

    pDevices =
      pTLS->raw_input->allocateDevices (uiNumDevices + 1);

    SK_GetRegisteredRawInputDevices ( pDevices,
                                     &uiNumDevices,
                                       sizeof RAWINPUTDEVICE );

    raw_devices.clear ();

    for (UINT i = 0; i < uiNumDevices; i++)
      raw_devices.push_back (pDevices [i]);

    SK_RawInput_ClassifyDevices ();
  }

  return
    uiNumDevices;
}

UINT
WINAPI
NtUserGetRegisteredRawInputDevices_Detour (
  _Out_opt_ PRAWINPUTDEVICE pRawInputDevices,
  _Inout_   PUINT           puiNumDevices,
  _In_      UINT            cbSize )
{
  UNREFERENCED_PARAMETER (cbSize);

  SK_LOG_FIRST_CALL

  assert (cbSize == sizeof RAWINPUTDEVICE);

  // On the first call to this function, we will need to query this stuff.
  //static bool init = false;

  //if (! init)
  //{
    SK_RawInput_PopulateDeviceList ();
    //init = true;
  //}


  if (*puiNumDevices < gsl::narrow_cast <UINT> (raw_devices.size ()))
  {   *puiNumDevices = gsl::narrow_cast <UINT> (raw_devices.size ());

    SetLastError (ERROR_INSUFFICIENT_BUFFER);

    return std::numeric_limits <UINT>::max ();
  }

  int idx = 0;

  if (pRawInputDevices != nullptr)
  {
    for (auto it : raw_devices)
    {
      pRawInputDevices [idx++] = it;
    }
  }

  else
  {
    idx +=
      gsl::narrow_cast <int> (raw_devices.size ());
  }

  return idx;
}

BOOL
WINAPI
NtUserRegisterRawInputDevices_Detour (
  _In_ PCRAWINPUTDEVICE pRawInputDevices,
  _In_ UINT             uiNumDevices,
  _In_ UINT             cbSize )
{
  SK_LOG_FIRST_CALL

  if (cbSize != sizeof RAWINPUTDEVICE)
  {
    SK_LOG0 ( ( L"RegisterRawInputDevices has wrong "
                L"structure size (%lu bytes), expected: %lu",
                  cbSize, (UINT)sizeof RAWINPUTDEVICE ),
               L" RawInput " );

    return
      SK_RegisterRawInputDevices ( pRawInputDevices,
                                  uiNumDevices,
                                    cbSize );
  }

  raw_devices.clear ();

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  std::unique_ptr <RAWINPUTDEVICE []> pLocalDevices;
  RAWINPUTDEVICE*                     pDevices = nullptr;

  if (pRawInputDevices != nullptr && uiNumDevices > 0)
  {
    pDevices = pTLS->raw_input->allocateDevices (uiNumDevices);
  }

  // The devices that we will pass to Windows after any overrides are applied
  std::vector <RAWINPUTDEVICE> actual_device_list;

  if (pDevices != nullptr)
  {
    // We need to continue receiving window messages for the console to work
    for ( size_t i = 0; i < uiNumDevices; ++i)
    {
      pDevices [i] = pRawInputDevices [i];
      raw_devices.push_back (pDevices [i]);

      if ( pDevices [i].hwndTarget != nullptr &&
           pDevices [i].hwndTarget != game_window.hWnd )
      {
        static wchar_t wszWindowClass [128] = { };
        static wchar_t wszWindowTitle [128] = { };

        RealGetWindowClassW   (pDevices [i].hwndTarget, wszWindowClass, 127);
        InternalGetWindowText (pDevices [i].hwndTarget, wszWindowTitle, 127);

        SK_LOG1 (
                  ( L"RawInput is being tracked on hWnd=%x - { (%s), '%s' }",
                      pDevices [i].hwndTarget, wszWindowClass, wszWindowTitle ),
                        __SK_SUBSYSTEM__ );
      }
    }

    SK_RawInput_ClassifyDevices ();
  }

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
// This function has never once been encountered after debugging > 100 games,
//   it is VERY likely this logic is broken since no known game uses RawInput
//     for buffered reads.
//
UINT
WINAPI
GetRawInputBuffer_Detour (_Out_opt_ PRAWINPUT pData,
                          _Inout_   PUINT     pcbSize,
                          _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  if (SK_ImGui_Active ())
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    bool filter = false;

    // Unconditional
    if (config.input.ui.capture)
      filter = true;

    // Only specific types
    if (io.WantCaptureKeyboard || io.WantCaptureMouse)
      filter = true;

    if (filter)
    {
      if (pData != nullptr)
      {
        RtlZeroMemory (pData, *pcbSize);

        const int max_items = (sizeof RAWINPUT / *pcbSize);
              int count     =                            0;
            auto *pTemp     =
                                  new RAWINPUT [max_items];
        uint8_t *pInput     =             (uint8_t *)pTemp;
           auto *pOutput    =             (uint8_t *)pData;
        UINT     cbSize     =                     *pcbSize;
                  *pcbSize  =                            0;

        int       temp_ret  =
          GetRawInputBuffer_Original ( pTemp, &cbSize, cbSizeHeader );

        for (int i = 0; i < temp_ret; i++)
        {
          auto* pItem = (RAWINPUT *)pInput;

          bool  remove = false;
          int  advance = pItem->header.dwSize;

          switch (pItem->header.dwType)
          {
            case RIM_TYPEKEYBOARD:
              SK_RAWINPUT_READ (sk_input_dev_type::Keyboard)
              if (SK_ImGui_WantKeyboardCapture ())
                remove = true;
              break;

            case RIM_TYPEMOUSE:
              SK_RAWINPUT_READ (sk_input_dev_type::Mouse)
              if (SK_ImGui_WantMouseCapture ())
                remove = true;
              break;

            default:
              SK_RAWINPUT_READ (sk_input_dev_type::Gamepad)
              if (SK_ImGui_WantGamepadCapture ())
                remove = true;
              break;
          }

          if (config.input.ui.capture)
            remove = true;

          if (! remove)
          {
            memcpy (pOutput, pItem, pItem->header.dwSize);
             pOutput += advance;
                        ++count;
            *pcbSize += advance;
          }

          pInput += advance;
        }

        delete [] pTemp;

        return count;
      }
    }
  }

  return
    GetRawInputBuffer_Original (pData, pcbSize, cbSizeHeader);
}

UINT
WINAPI
SK_ImGui_ProcessRawInput (_In_      HRAWINPUT hRawInput,
                          _In_      UINT      uiCommand,
                          _Out_opt_ LPVOID    pData,
                          _Inout_   PUINT     pcbSize,
                          _In_      UINT      cbSizeHeader,
                                    BOOL      self);

UINT
WINAPI
NtUserGetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                              _In_      UINT      uiCommand,
                              _Out_opt_ LPVOID    pData,
                              _Inout_   PUINT     pcbSize,
                              _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  return
    SK_ImGui_ProcessRawInput (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader, FALSE);
}


/////////////////////////////////////////////////
//
// ImGui Cursor Management
//
/////////////////////////////////////////////////
#include <imgui/imgui.h>
sk_imgui_cursor_s SK_ImGui_Cursor;

HCURSOR GetGameCursor (void);

bool
SK_ImGui_IsMouseRelevant (void)
{
  // SK_ImGui_Visible is the full-blown config UI;
  //   but we also have floating widgets that may capture mouse
  //     input.
  return config.input.mouse.disabled_to_game || SK_ImGui_Active () ||
         ImGui::IsWindowHovered (ImGuiHoveredFlags_AnyWindow);
      // ^^^ These are our floating widgets
}

void
sk_imgui_cursor_s::update (void)
{
  HCURSOR hCur =
    GetGameCursor ();

  if (hCur != nullptr)
    SK_ImGui_Cursor.orig_img = hCur;

  if (SK_ImGui_IsMouseRelevant ())
  {
    if (ImGui::GetIO ().WantCaptureMouse || SK_ImGui_Cursor.orig_img == nullptr)
      SK_ImGui_Cursor.showSystemCursor (false);
    else
      SK_ImGui_Cursor.showSystemCursor (true);
  }

  else
    SK_ImGui_Cursor.showSystemCursor ();
}

//__inline
void
sk_imgui_cursor_s::showImGuiCursor (void)
{
  showSystemCursor (false);
}

void
sk_imgui_cursor_s::LocalToScreen (LPPOINT lpPoint)
{
  LocalToClient  (lpPoint);
  ClientToScreen (game_window.hWnd, lpPoint);
}

extern ImGuiContext* SK_GImDefaultContext (void);

void
sk_imgui_cursor_s::LocalToClient (LPPOINT lpPoint)
{
  if (! SK_GImDefaultContext ())
    return;

  RECT                              real_client = { };
  GetClientRect (game_window.hWnd, &real_client);

  ImVec2 local_dims =
    ImGui::GetIO ().DisplayFramebufferScale;

  struct {
    float width  = 1.0f,
          height = 1.0f;
  } in, out;

  in.width   = local_dims.x;
  in.height  = local_dims.y;

  out.width  = (float)(real_client.right  - real_client.left);
  out.height = (float)(real_client.bottom - real_client.top);

  float x = 2.0f * ((float)lpPoint->x / std::max (1.0f, in.width )) - 1.0f;
  float y = 2.0f * ((float)lpPoint->y / std::max (1.0f, in.height)) - 1.0f;

  lpPoint->x = (LONG)( ( x * out.width  + out.width  ) / 2.0f );
  lpPoint->y = (LONG)( ( y * out.height + out.height ) / 2.0f );
}

void
sk_imgui_cursor_s::ClientToLocal    (LPPOINT lpPoint)
{
  if (! SK_GImDefaultContext ())
    return;

  RECT                              real_client = { };
  GetClientRect (game_window.hWnd, &real_client);

  const ImVec2 local_dims =
    ImGui::GetIO ().DisplayFramebufferScale;

  struct {
    float width  = 1.0f,
          height = 1.0f;
  } in, out;

  out.width  = local_dims.x;
  out.height = local_dims.y;

  in.width   = (float)(real_client.right  - real_client.left);
  in.height  = (float)(real_client.bottom - real_client.top);

  float x =     2.0f * ((float)lpPoint->x /
            std::max (1.0f, in.width )) - 1.0f;
  float y =     2.0f * ((float)lpPoint->y /
            std::max (1.0f, in.height)) - 1.0f;
            // Avoid division-by-zero, this should be a signaling NAN but
            //   some games alter FPU behavior and will turn this into a non-continuable exception.

  lpPoint->x = (LONG)( ( x * out.width  + out.width  ) / 2.0f );
  lpPoint->y = (LONG)( ( y * out.height + out.height ) / 2.0f );
}

//__inline
void
sk_imgui_cursor_s::ScreenToLocal (LPPOINT lpPoint)
{
  ScreenToClient (game_window.hWnd, lpPoint);
  ClientToLocal  (lpPoint);
}


#include <resource.h>

HCURSOR
ImGui_DesiredCursor (void)
{
  static HCURSOR last_cursor = nullptr;

  static const std::map <UINT, HCURSOR>
    __cursor_cache =
    {
      { ImGuiMouseCursor_Arrow,
          LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_POINTER) },
      { ImGuiMouseCursor_TextInput,
          LoadCursor (nullptr,               IDC_IBEAM)          },
      { ImGuiMouseCursor_ResizeEW,
          LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_HORZ)    },
      { ImGuiMouseCursor_ResizeNWSE,
          LoadCursor (nullptr,               IDC_SIZENWSE)       }
    };


  if (ImGui::GetIO ().MouseDownDuration [0] <= 0.0f || last_cursor == nullptr)
  {
    const auto&
        it  = __cursor_cache.find (ImGui::GetMouseCursor ());
    if (it != __cursor_cache.cend ())
    {
      last_cursor =
             it->second;
      return it->second;
    }
  }

  else if (SK_ImGui_IsMouseRelevant ())
    return (last_cursor);

  return
    GetGameCursor ();
}

void
ImGuiCursor_Impl (void)
{
  CURSORINFO ci = { };
  ci.cbSize     = sizeof CURSORINFO;

  SK_GetCursorInfo (&ci);

  HCURSOR desired =
    ( config.input.ui.use_hw_cursor ?
             ImGui_DesiredCursor () : nullptr );

  //
  // Hardware Cursor
  //
  if (config.input.ui.use_hw_cursor && ci.hCursor != desired)
  {
    SK_SetCursor (desired);
                       ci.cbSize = sizeof (CURSORINFO);
    SK_GetCursorInfo (&ci);
  }

  if ( config.input.ui.use_hw_cursor && (ci.flags & CURSOR_SHOWING) )
  {
    ImGui::GetIO ().MouseDrawCursor = false;
  }

  //
  // Software
  //
  else
  {
    if (SK_ImGui_IsMouseRelevant ())
    {
      if (ci.hCursor != desired)
          SK_SetCursor (desired);

      ImGui::GetIO ().MouseDrawCursor = (! SK_ImGui_Cursor.idle);
    }
  }
}

static HCURSOR wait_cursor  = nullptr;
static HCURSOR arrow_cursor = nullptr;

void
sk_imgui_cursor_s::showSystemCursor (bool system)
{
  if (wait_cursor == nullptr)
    wait_cursor = LoadCursor (nullptr, IDC_WAIT);

  if (arrow_cursor == nullptr)
    arrow_cursor = LoadCursor (nullptr, IDC_ARROW);

  CURSORINFO cursor_info = { };
  cursor_info.cbSize     = sizeof (CURSORINFO);

  if (SK_ImGui_Cursor.orig_img == wait_cursor)
      SK_ImGui_Cursor.orig_img = arrow_cursor;

  if (system)
  {
    //if (refs_added == 0) { ShowCursor (TRUE); ++refs_added; }

    SK_GetCursorInfo (&cursor_info);

    if (cursor_info.hCursor != SK_ImGui_Cursor.orig_img)
    { SK_SetCursor            (SK_ImGui_Cursor.orig_img);

                         // The previous call to GetCursorInfo is known to
                         //   zero-out the size field for some reason...
                         cursor_info.cbSize = sizeof (CURSORINFO);
      SK_GetCursorInfo (&cursor_info);
    }

    if ((! SK_ImGui_IsMouseRelevant ()) || (cursor_info.flags & CURSOR_SHOWING))
      ImGui::GetIO ().MouseDrawCursor = false;

    else
      ImGuiCursor_Impl ();
  }

  else
  {
    //if (refs_added == 1) { --refs_added; ShowCursor (FALSE); }
    ImGuiCursor_Impl ();
  }
}


void
sk_imgui_cursor_s::activateWindow (bool active)
{
  CURSORINFO ci = { };
  ci.cbSize     = sizeof ci;

  SK_GetCursorInfo (&ci);

  if (active)
  {
    if (SK_ImGui_IsMouseRelevant ())
    {
      if (SK_ImGui_WantMouseCapture ())
      {
        SK_SetCursor (ImGui_DesiredCursor ());
      }

      else if (SK_ImGui_Cursor.orig_img)
      {
        SK_SetCursor (SK_ImGui_Cursor.orig_img);
      }
    }
  }
}




HWND WINAPI SK_GetForegroundWindow (void);

bool
SK_ImGui_WantKeyboardCapture (void)
{
  if (! SK_GImDefaultContext ())
    return false;

  bool imgui_capture = false;

  ImGuiIO& io =
    ImGui::GetIO ();

  if (nav_usable || io.WantCaptureKeyboard || io.WantTextInput)
    imgui_capture = true;

  if (config.input.keyboard.disabled_to_game)
    imgui_capture = true;

  return
    imgui_capture;
}

bool
SK_ImGui_WantTextCapture (void)
{
  if (! SK_GImDefaultContext ())
    return false;

  bool imgui_capture = false;

  ImGuiIO& io =
    ImGui::GetIO ();

  if (io.WantTextInput && game_window.active)
    imgui_capture = true;

  if (config.input.keyboard.disabled_to_game)
    imgui_capture = true;

  return imgui_capture;
}

bool
SK_ImGui_WantGamepadCapture (void)
{
  if (! SK_GImDefaultContext ())
    return false;

  bool imgui_capture = false;

  if (SK_ImGui_Active ())
  {
    if (nav_usable)
      imgui_capture = true;
  }

#ifdef _M_AMD64
  // Stupid hack, breaking whatever abstraction this horrible mess passes for
  extern bool __FAR_Freelook;
  if (__FAR_Freelook)
    imgui_capture = true;
#endif

  extern bool SK_ImGui_GamepadComboDialogActive;

  if (SK_ImGui_GamepadComboDialogActive)
    imgui_capture = true;

  if (config.input.gamepad.disabled_to_game)
    imgui_capture = true;

  return imgui_capture;
}


static const DWORD REASON_DISABLED = 0x4;

bool
SK_ImGui_WantMouseCaptureEx (DWORD dwReasonMask)
{
  if (! SK_GImDefaultContext ())
    return false;

  bool imgui_capture = false;

  if (SK_ImGui_IsMouseRelevant ())
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    if (config.input.ui.capture_mouse || io.WantCaptureMouse)
      imgui_capture = true;

    else if ((dwReasonMask & REASON_DISABLED) && config.input.mouse.disabled_to_game)
      imgui_capture = true;

    else if (config.input.ui.capture_hidden && (! SK_InputUtil_IsHWCursorVisible ()))
      imgui_capture = true;
  }

  return imgui_capture;
}



bool
SK_ImGui_WantMouseCapture (void)
{
  return
    SK_ImGui_WantMouseCaptureEx (0xFFFF);
}



HCURSOR GetGameCursor (void)
{
  static HCURSOR sk_imgui_arrow = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_POINTER);
  static HCURSOR sk_imgui_horz  = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_HORZ);
  static HCURSOR sk_imgui_ibeam = LoadCursor (nullptr, IDC_IBEAM);
  static HCURSOR sys_arrow      = LoadCursor (nullptr, IDC_ARROW);
  static HCURSOR sys_wait       = LoadCursor (nullptr, IDC_WAIT);

  static HCURSOR hCurLast       = nullptr;
  /// FIXME
         HCURSOR hCur           = SK_ImGui_Cursor.orig_img;//GetCursor ();

  if ( hCur != sk_imgui_horz && hCur != sk_imgui_arrow && hCur != sk_imgui_ibeam &&
       hCur != sys_arrow     && hCur != sys_wait )
  {
    hCurLast = hCur;
  }

  return hCurLast;
}

void
ImGui_ToggleCursor (void)
{
  if (! SK_ImGui_Cursor.visible)
  {
    if (SK_ImGui_Cursor.orig_img == nullptr)
      SK_ImGui_Cursor.orig_img = GetGameCursor ();

    //GetClipCursor         (&SK_ImGui_Cursor.clip_rect);

    SK_ImGui_CenterCursorOnWindow ();

    // Save original cursor position
    SK_GetCursorPos               (&SK_ImGui_Cursor.pos);
    SK_ImGui_Cursor.ScreenToLocal (&SK_ImGui_Cursor.pos);

    ImGui::GetIO ().WantCaptureMouse = true;
  }

  else
  {
    if (SK_ImGui_Cursor.orig_img == nullptr)
    {   SK_ImGui_Cursor.orig_img =
          GetGameCursor ();      }

    if (SK_ImGui_WantMouseCapture ())
    {
      //ClipCursor_Original   (&SK_ImGui_Cursor.clip_rect);

      static POINT last_pos =
        SK_ImGui_Cursor.orig_pos;

      if ( 0 !=
            memcmp (&last_pos, &SK_ImGui_Cursor.orig_pos, sizeof (POINT)) )
      {
        POINT screen = SK_ImGui_Cursor.orig_pos;
        SK_ImGui_Cursor.LocalToScreen (&screen);
        SK_SetCursorPos ( screen.x,
                          screen.y );
      }
    }

    ImGui::GetIO ().WantCaptureMouse = false;
  }

  SK_ImGui_Cursor.visible = (! SK_ImGui_Cursor.visible);
  SK_ImGui_Cursor.update ();
}



using GetMouseMovePointsEx_pfn = int (WINAPI *)(
  _In_  UINT             cbSize,
  _In_  LPMOUSEMOVEPOINT lppt,
  _Out_ LPMOUSEMOVEPOINT lpptBuf,
  _In_  int              nBufPoints,
  _In_  DWORD            resolution
);

GetMouseMovePointsEx_pfn GetMouseMovePointsEx_Original = nullptr;

int
WINAPI
GetMouseMovePointsEx_Detour(
  _In_  UINT             cbSize,
  _In_  LPMOUSEMOVEPOINT lppt,
  _Out_ LPMOUSEMOVEPOINT lpptBuf,
  _In_  int              nBufPoints,
  _In_  DWORD            resolution )
{
  SK_LOG_FIRST_CALL

  if (SK_ImGui_IsMouseRelevant ())
  {
    bool implicit_capture = false;

    // Depending on warp prefs, we may not allow the game to know about mouse movement
    //   (even if ImGui doesn't want mouse capture)
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open && SK_ImGui_IsMouseRelevant ()       ) ||
         ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
    {
      implicit_capture = true;
    }

    if (SK_ImGui_WantMouseCapture () || implicit_capture)
    {
      *lpptBuf = *lppt;

      return 0;
    }
  }

  return
    GetMouseMovePointsEx_Original (cbSize, lppt, lpptBuf, nBufPoints, resolution);
}

HCURSOR
WINAPI
SK_SetCursor (
  _In_opt_ HCURSOR hCursor)
{
  HCURSOR orig =
         SK_ImGui_Cursor.img;

  if (NtUserSetCursor_Original != nullptr )
  {   NtUserSetCursor_Original (  hCursor );
      SK_ImGui_Cursor.img       = hCursor; }
  else
  {   SK_ImGui_Cursor.img       = hCursor ;
            SetCursor            (hCursor); }

  return
    orig;
}

HCURSOR
WINAPI
NtUserSetCursor_Detour (
  _In_opt_ HCURSOR hCursor )
{
  SK_LOG_FIRST_CALL

  SK_ImGui_Cursor.orig_img = hCursor;

  if (SK_ImGui_WantMouseCapture ())
  {
    SK_ImGui_Cursor.img =
      ImGui_DesiredCursor ();

    NtUserSetCursor_Original (SK_ImGui_Cursor.img);

    return
      SK_ImGui_Cursor.img;
  }

  return
    NtUserSetCursor_Original (hCursor);
}

HCURSOR
WINAPI
NtUserGetCursor_Detour (VOID)
{
  SK_LOG_FIRST_CALL

  //return
  //  NtUserGetCursor_Original ();

  static auto& io =
    ImGui::GetIO ();

  if (! (SK_ImGui_IsMouseRelevant () ||
                io.WantCaptureMouse)   )
  {
    SK_ImGui_Cursor.img =
      NtUserGetCursor_Original ();
  }

  return
    SK_ImGui_Cursor.img;
}

BOOL
WINAPI
SK_GetCursorInfo (PCURSORINFO pci)
{
  if ( pci != nullptr )
  {
    if (pci->cbSize == 0)
    {
      // Fix-Up:
      //
      //    It is Undocumented, but Windows sometimes
      //      zeros-out the cbSize field such that two
      //        successive uses of the same data structure
      //          will fail because cbSize does not match.
      //
      pci->cbSize =
        sizeof (CURSORINFO);
      // Should be the same size in modern software.
    }

    if (NtUserGetCursorInfo_Original != nullptr)
      return NtUserGetCursorInfo_Original (pci);

    else if (GetCursorInfo_Original != nullptr)
      return GetCursorInfo_Original (pci);

    return
      GetCursorInfo (pci);
  }

  return FALSE;
}

BOOL
WINAPI
NtUserGetCursorInfo_Detour (PCURSORINFO pci)
{
  SK_LOG_FIRST_CALL

  if (config.input.mouse.disabled_to_game && SK_GetCallingDLL () == GetModuleHandle (nullptr))
  {
    return FALSE;
  }

  POINT   pt        = pci->ptScreenPos;
  BOOL    ret       = SK_GetCursorInfo (pci);

  struct state_backup
  {
    explicit state_backup (PCURSORINFO pci) :
      hCursor     (pci->hCursor),
      ptScreenPos (pci->ptScreenPos) { };

    HCURSOR hCursor;
    POINT   ptScreenPos;
  } actual (pci);

  pci->ptScreenPos = pt;
  pci->hCursor     =
    SK_ImGui_Cursor.orig_img;


  if (SK_ImGui_IsMouseRelevant ())
    pci->hCursor = ImGui_DesiredCursor ();


  if (ret && SK_ImGui_IsMouseRelevant ())
  {
    bool implicit_capture = false;

    // Depending on warp prefs, we may not allow the game to know about mouse movement
    //   (even if ImGui doesn't want mouse capture)
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open && SK_ImGui_IsMouseRelevant       () ) ||
         ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
      implicit_capture = true;

    if (SK_ImGui_WantMouseCapture () || implicit_capture)
    {
      POINT client =
        SK_ImGui_Cursor.orig_pos;

      SK_ImGui_Cursor.LocalToScreen (&client);

      pci->ptScreenPos.x = client.x;
      pci->ptScreenPos.y = client.y;
    }

    else
    {
      POINT client =
        SK_ImGui_Cursor.pos;

      SK_ImGui_Cursor.LocalToScreen (&client);

      pci->ptScreenPos.x = client.x;
      pci->ptScreenPos.y = client.y;
    }

    return TRUE;
  }


  pci->hCursor     = SK_ImGui_IsMouseRelevant () ? ImGui_DesiredCursor () :
                     actual.hCursor;
  pci->ptScreenPos = actual.ptScreenPos;

  return ret;
}

BOOL
WINAPI
GetCursorInfo_Detour (PCURSORINFO pci)
{
  SK_LOG_FIRST_CALL

  return
    NtUserGetCursorInfo_Detour (pci);
}

float SK_SO4_MouseScale = 2.467f;

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  SK_LOG_FIRST_CALL

  if (config.input.mouse.disabled_to_game && SK_GetCallingDLL () == GetModuleHandle (nullptr))
  {
    return FALSE;
  }

  if (config.window.background_render && (! game_window.active))
  {
    *lpPoint = SK_ImGui_Cursor.orig_pos;

    return TRUE;
  }


  if (SK_ImGui_IsMouseRelevant ())
  {
    bool implicit_capture = false;

    // Depending on warp prefs, we may not allow the game to know about mouse movement
    //   (even if ImGui doesn't want mouse capture)
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open && SK_ImGui_IsMouseRelevant       () ) ||
         ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
      implicit_capture = true;

    if (SK_ImGui_WantMouseCapture () || implicit_capture)
    {
      POINT client =
        SK_ImGui_Cursor.orig_pos;

      SK_ImGui_Cursor.LocalToScreen (&client);

      lpPoint->x = client.x;
      lpPoint->y = client.y;
    }

    else
    {
      POINT client =
        SK_ImGui_Cursor.pos;

      SK_ImGui_Cursor.LocalToScreen (&client);

      lpPoint->x = client.x;
      lpPoint->y = client.y;
    }

    return TRUE;
  }


  BOOL bRet =
    SK_GetCursorPos (lpPoint);

  if (bRet)
  {
    SK_ImGui_Cursor.orig_pos = *lpPoint;
    SK_ImGui_Cursor.pos      = *lpPoint;

    SK_ImGui_Cursor.ScreenToLocal (&SK_ImGui_Cursor.orig_pos);
    SK_ImGui_Cursor.ScreenToLocal (&SK_ImGui_Cursor.pos);
  }



  if (SK_GetCurrentGameID () == SK_GAME_ID::StarOcean4 && GetActiveWindow () == SK_GetGameWindow () && SK_GetCallingDLL () == GetModuleHandle (nullptr))
  {
    const auto& io =
      ImGui::GetIO ();

    static ULONG last_frame = SK_GetFramesDrawn ();
    static POINT last_pos   = *lpPoint;

    POINT new_pos = *lpPoint;

    new_pos.x = (LONG)(io.DisplayFramebufferScale.x * 0.5f);
    new_pos.y = (LONG)(io.DisplayFramebufferScale.y * 0.5f);

    float ndc_x = 2.0f * (((float)(lpPoint->x - last_pos.x) + new_pos.x) / io.DisplayFramebufferScale.x) - 1.0f;
    float ndc_y = 2.0f * (((float)(lpPoint->y - last_pos.y) + new_pos.y) / io.DisplayFramebufferScale.y) - 1.0f;

    bool new_frame = false;

    if (last_frame != SK_GetFramesDrawn ())
    {
      last_pos   = *lpPoint;
      last_frame = SK_GetFramesDrawn ();
      new_frame  = true;
    }

    lpPoint->x = (LONG)((ndc_x * SK_SO4_MouseScale * io.DisplayFramebufferScale.x + io.DisplayFramebufferScale.x) / 2.0f);
    lpPoint->y = (LONG)((ndc_y * SK_SO4_MouseScale * io.DisplayFramebufferScale.y + io.DisplayFramebufferScale.y) / 2.0f);

    static int calls = 0;

    POINT get_pos; // 4 pixel cushion to avoid showing the cursor
    if (new_frame && ( (calls++ % 8) == 0 || ( SK_GetCursorPos (&get_pos) && (get_pos.x <= 4 || get_pos.x >= (io.DisplayFramebufferScale.x - 4) ||
                                                                              get_pos.y <= 4 || get_pos.y >= (io.DisplayFramebufferScale.y - 4)) ) ))
    {
      last_pos = new_pos;
      SK_SetCursorPos (new_pos.x, new_pos.y);
      lpPoint->x = new_pos.x;
      lpPoint->y = new_pos.y;
    }
  }


  return bRet;
}

BOOL
WINAPI
SetCursorPos_Detour (_In_ int x, _In_ int y)
{
  SK_LOG_FIRST_CALL

  if (config.input.mouse.disabled_to_game && SK_GetCallingDLL () == GetModuleHandle (nullptr))
  {
    return FALSE;
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  // Game WANTED to change its position, so remember that.
  SK_ImGui_Cursor.orig_pos.x = x;
  SK_ImGui_Cursor.orig_pos.y = y;

  SK_ImGui_Cursor.ScreenToLocal (&SK_ImGui_Cursor.orig_pos);

  // Don't let the game continue moving the cursor while
  //   Alt+Tabbed out
  if ((! rb.fullscreen_exclusive) &&
      config.window.background_render && (! game_window.active))
  {
    return TRUE;
  }

  // Prevent Mouse Look while Drag Locked
  if (config.window.drag_lock)
    return TRUE;

  if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open && SK_ImGui_IsMouseRelevant       () ) ||
       ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
  {
    //game_mouselook = SK_GetFramesDrawn ();
  }

  else if (! SK_ImGui_WantMouseCapture ())
  {
    return
      SK_SetCursorPos (x, y);
  }

  return
    TRUE;
}

UINT
WINAPI
NtUserSendInput_Detour (
  _In_ UINT    nInputs,
  _In_ LPINPUT pInputs,
  _In_ int     cbSize
)
{
  SK_LOG_FIRST_CALL

  // TODO: Process this the right way...

  if (SK_ImGui_Active ())
  {
    return 0;
  }

  return
    SK_SendInput (nInputs, pInputs, cbSize);
}

keybd_event_pfn keybd_event_Original = nullptr;

void
WINAPI
keybd_event_Detour (
    _In_ BYTE bVk,
    _In_ BYTE bScan,
    _In_ DWORD dwFlags,
    _In_ ULONG_PTR dwExtraInfo
)
{
  SK_LOG_FIRST_CALL

// TODO: Process this the right way...

  if (SK_ImGui_Active ())
  {
    return;
  }

  keybd_event_Original (bVk, bScan, dwFlags, dwExtraInfo);
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

  if (SK_ImGui_IsMouseRelevant ())
  {
    return;
  }

  mouse_event_Original (dwFlags, dx, dy, dwData, dwExtraInfo);
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

SHORT
WINAPI
NtUserGetAsyncKeyState_Detour (_In_ int vKey)
{
  SK_LOG_FIRST_CALL

  static auto& rb =
    SK_GetCurrentRenderBackend ();

#define SK_ConsumeVKey(vKey) { SK_GetAsyncKeyState(vKey); return 0; }

  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible ())
    SK_ConsumeVKey (vKey);

  bool fullscreen =
    ( SK_GetFramesDrawn () && rb.fullscreen_exclusive );

  // Block keyboard input to the game while it's in the background
  if ((! fullscreen) &&
      config.window.background_render && (! game_window.active))
    SK_ConsumeVKey (vKey);

  if (vKey & 0xF8) // Valid Keys:  8 - 255
  {
    if (SK_ImGui_WantKeyboardCapture ())
      SK_ConsumeVKey (vKey);
  }

  else if (vKey < 8)
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
      SK_ConsumeVKey (vKey);
  }

  if (config.input.keyboard.disabled_to_game && SK_GetCallingDLL () == GetModuleHandle (nullptr))
  {
    return 0;
  }

  return
    SK_GetAsyncKeyState (vKey);
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
NtUserGetKeyState_Detour (_In_ int nVirtKey)
{
  SK_LOG_FIRST_CALL

  static auto& rb =
    SK_GetCurrentRenderBackend ();

#define SK_ConsumeVirtKey(nVirtKey) { SK_GetKeyState (nVirtKey); return 0; }

  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible ())
    SK_ConsumeVirtKey (nVirtKey);

  bool fullscreen =
    ( SK_GetFramesDrawn () && rb.fullscreen_exclusive );

  // Block keyboard input to the game while it's in the background
  if ((! fullscreen) &&
      config.window.background_render && (! game_window.active))
    SK_ConsumeVirtKey (nVirtKey);

  if (nVirtKey & 0xF8) // Valid Keys:  8 - 255
  {
    if (SK_ImGui_WantKeyboardCapture ())
      SK_ConsumeVirtKey (nVirtKey);
  }

  else if (nVirtKey < 8)
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
      SK_ConsumeVirtKey (nVirtKey);
  }

  if (config.input.keyboard.disabled_to_game && SK_GetCallingDLL () == GetModuleHandle (nullptr))
  {
    return 0;
  }

  return SK_GetKeyState (nVirtKey);
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
NtUserGetKeyboardState_Detour (PBYTE lpKeyState)
{
  SK_LOG_FIRST_CALL

  BOOL bRet =
    SK_GetKeyboardState (lpKeyState);

  if (bRet)
  {
    if (SK_ImGui_WantKeyboardCapture ())
      RtlZeroMemory (&lpKeyState [7], 247);

    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
      RtlZeroMemory (lpKeyState, 7);
  }

  if (config.input.keyboard.disabled_to_game && SK_GetCallingDLL () == GetModuleHandle (nullptr))
  {
    return FALSE;
  }

  return bRet;
}

#include <dbt.h>

LRESULT
WINAPI
ImGui_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern BOOL
SK_IsChild (HWND hWndParent, HWND hWnd);

bool
SK_ImGui_HandlesMessage (MSG *lpMsg, bool /*remove*/, bool /*peek*/)
{
  // Many GL games mysteriously violate this with no ill-effect...
  //
  ///assert ( lpMsg->hwnd == 0 ||
  ///         lpMsg->hwnd == game_window.hWnd );

  bool handled = false;

  if ((! lpMsg) || ( lpMsg->hwnd != game_window.hWnd &&
                         nullptr != game_window.hWnd &&
                         nullptr != lpMsg->hwnd ))
  {
    return handled;
  }

  ////SK_LOG1 ( ( L"Handle Message? %04x, HWND=%04x", lpMsg->message, lpMsg->hwnd ),
  ////            L"ImGuiTrace" );
  ////
  ////if (           game_window.hWnd != nullptr     &&
  ////               game_window.hWnd != lpMsg->hwnd &&
  ////    false            ==
  ////   SK_IsChild (game_window.hWnd,   lpMsg->hwnd)
  ////   )
  ////{
  ////  return false;
  ////}

  switch (lpMsg->message)
  {
    case WM_CHAR:
    case WM_MENUCHAR:
    {
      return
        SK_ImGui_WantTextCapture ();
    } break;


    // Fix for Melody's Escape, which attempts to remove these messages!
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
      handled  =
        ( ImGui_WndProcHandler  ( lpMsg->hwnd,   lpMsg->message,
                                  lpMsg->wParam, lpMsg->lParam ) != 0 )
          && SK_ImGui_WantKeyboardCapture ();
    } break;

    case WM_SETCURSOR:
    {
      if (SK_ImGui_WantMouseCapture ())
      {
        if (lpMsg->hwnd == game_window.hWnd &&
                           game_window.hWnd != nullptr)
        {
          handled = true;
        }
      }
    } break;

    // TODO: Does this message have an HWND always?
    case WM_DEVICECHANGE:
    {
      handled =
        ( 0 != ImGui_WndProcHandler (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam) );
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
    case WM_NCMOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    {
      handled  =
        ( 0 != ImGui_WndProcHandler ( lpMsg->hwnd,   lpMsg->message,
                                      lpMsg->wParam, lpMsg->lParam ) )
          && SK_ImGui_WantMouseCapture ();
    } break;

    case WM_INPUT:
    {
      if ( RIM_INPUTSINK ==
             GET_RAWINPUT_CODE_WPARAM (lpMsg->wParam) )
      {
        handled = true;

        DefWindowProcW ( lpMsg->hwnd,   lpMsg->message,
                         lpMsg->wParam, lpMsg->lParam );
      }

      else
      {
        handled =
           ( 0 != ImGui_WndProcHandler ( lpMsg->hwnd,   lpMsg->message,
                                         lpMsg->wParam, lpMsg->lParam  ) );
      }
    } break;

    default:
    {
    } break;
  }

  ////SK_LOG1 ( ( L" -- %s Handled!", handled ? L"Was" : L"Was Not" ),
  ////            L"ImGuiTrace" );

  return handled;
}

// Parts of the Win32 API that are safe to hook from DLL Main
void SK_Input_PreInit (void)
{
  bool bHasWin32u =
      ( SK_GetProcAddress (L"win32u", "NtUserGetCursorInfo") != nullptr );

  //if (SK_GetProcAddress (L"Win32U", "NtUserGetRawInputData") != nullptr)
  //{
  //  SK_CreateDLLHook2 (       L"Win32U",
  //                      "NtUserGetRawInputData",
  //                       NtUserGetRawInputData_Detour,
  //    static_cast_p2p <void> (&GetRawInputData_Original) );
  //}
  //
  //else
  {
    SK_CreateDLLHook2 (       L"user32",
                               "GetRawInputData",
                          NtUserGetRawInputData_Detour,
       static_cast_p2p <void> (&GetRawInputData_Original) );
  }

#if 0
  SK_CreateUser32Hook (      "NtUserGetAsyncKeyState",
                              NtUserGetAsyncKeyState_Detour,
     static_cast_p2p <void> (&GetAsyncKeyState_Original) );
#else
  SK_CreateDLLHook2 (       L"user32",
                                   "GetAsyncKeyState",
                              NtUserGetAsyncKeyState_Detour,
     static_cast_p2p <void> (&GetAsyncKeyState_Original) );
#endif

  // Trails of Cold Steel doesn't work right if we hook NtUserGetKeyState;
  //   it uses keyboard state for mouse buttons.
#if 0
  SK_CreateUser32Hook (      "NtUserGetKeyState",
                              NtUserGetKeyState_Detour,
     static_cast_p2p <void> (&GetKeyState_Original) );
#else
  SK_CreateDLLHook2 (       L"user32",
                             "GetKeyState",
                              NtUserGetKeyState_Detour,
     static_cast_p2p <void> (&GetKeyState_Original) );
#endif

  //SK_CreateUser32Hook (      "NtUserGetKeyboardState",
  SK_CreateDLLHook2 (       L"user32",
                                   "GetKeyboardState",
                             //"NtUserGetKeyboardState",
                              NtUserGetKeyboardState_Detour,
     static_cast_p2p <void> (&GetKeyboardState_Original) );


  SK_CreateDLLHook2 (       L"user32",
                             "GetCursorPos",
                              GetCursorPos_Detour,
     static_cast_p2p <void> (&GetCursorPos_Original) );


  if (bHasWin32u)
  {
    SK_CreateDLLHook2 (      L"win32u",
                              "NtUserGetCursorInfo",
                               NtUserGetCursorInfo_Detour,
      static_cast_p2p <void> (&NtUserGetCursorInfo_Original) );
  }

  SK_CreateDLLHook2 (      L"user32",
                            "GetCursorInfo",
                             GetCursorInfo_Detour,
    static_cast_p2p <void> (&GetCursorInfo_Original) );

  SK_CreateDLLHook2 (       L"user32",
                             "GetMouseMovePointsEx",
                              GetMouseMovePointsEx_Detour,
     static_cast_p2p <void> (&GetMouseMovePointsEx_Original) );


  SK_CreateDLLHook2 (       L"user32",
                                   "SetCursor",
//SK_CreateUser32Hook (      "NtUserSetCursor",
                              NtUserSetCursor_Detour,
     static_cast_p2p <void> (&NtUserSetCursor_Original) );

  SK_CreateDLLHook2 (       L"user32",
                                   "GetCursor",
//SK_CreateUser32Hook (      "NtUserGetCursor",
                              NtUserGetCursor_Detour,
     static_cast_p2p <void> (&NtUserGetCursor_Original) );


  SK_CreateDLLHook2 (       L"user32",
                             "SetCursorPos",
                              SetCursorPos_Detour,
     static_cast_p2p <void> (&SetCursorPos_Original) );


  SK_CreateDLLHook2 (     L"user32",
                           "SendInput",
                      NtUserSendInput_Detour,
   static_cast_p2p <void> (&SendInput_Original) );

  //SK_CreateUser32Hook (      "NtUserSendInput",
  //                            NtUserSendInput_Detour,
  //   static_cast_p2p <void> (&SendInput_Original) );


  SK_CreateDLLHook2 (       L"user32",
                             "mouse_event",
                              mouse_event_Detour,
     static_cast_p2p <void> (&mouse_event_Original) );

  SK_CreateDLLHook2 (       L"user32",
                             "keybd_event",
                              keybd_event_Detour,
     static_cast_p2p <void> (&keybd_event_Original) );


  //SK_CreateUser32Hook (      "NtUserRegisterRawInputDevices",
  SK_CreateDLLHook2 (       L"user32",
                                   "RegisterRawInputDevices",
                              NtUserRegisterRawInputDevices_Detour,
     static_cast_p2p <void> (&RegisterRawInputDevices_Original) );

  //SK_CreateUser32Hook (      "NtUserGetRegisteredRawInputDevices",
  SK_CreateDLLHook2 (       L"user32",
                                   "GetRegisteredRawInputDevices",
                              NtUserGetRegisteredRawInputDevices_Detour,
     static_cast_p2p <void> (&GetRegisteredRawInputDevices_Original) );


  SK_CreateDLLHook2 (       L"user32",
                             "GetRawInputBuffer",
                              GetRawInputBuffer_Detour,
     static_cast_p2p <void> (&GetRawInputBuffer_Original) );


  if (config.input.gamepad.hook_xinput)
    SK_XInput_InitHotPlugHooks ( );
}




#include <concurrent_unordered_map.h>

typedef HKL (WINAPI *GetKeyboardLayout_pfn)(
  _In_ DWORD idThread
);

GetKeyboardLayout_pfn GetKeyboardLayout_Original = nullptr;

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
SK_Input_Init (void)
{
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

  auto CreateInputVar_Bool = [](auto name, auto config_var)
  {
    SK_GetCommandProcessor ()->AddVariable (
      name,
        SK_CreateVar ( SK_IVariable::Boolean,
                         config_var
                     )
    );
  };

  auto CreateInputVar_Int = [](auto name, auto config_var)
  {
    SK_GetCommandProcessor ()->AddVariable (
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

  CreateInputVar_Int  ("Input.Steam.UIController",     &config.input.gamepad.steam.ui_slot);
  CreateInputVar_Int  ("Input.XInput.UIController",    &config.input.gamepad.xinput.ui_slot);

  SK_Input_PreHookHID    ();
  SK_Input_PreHookDI8    ();
  SK_Input_PreHookXInput ();
}



SK_LazyGlobal <sk_input_api_context_s> SK_XInput_Backend;
SK_LazyGlobal <sk_input_api_context_s> SK_HID_Backend;
SK_LazyGlobal <sk_input_api_context_s> SK_RawInput_Backend;


bool SK_ImGui_InputLanguage_s::changed      = true; // ^^^^ Default = true
HKL  SK_ImGui_InputLanguage_s::keybd_layout;