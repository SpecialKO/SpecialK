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
#define DIRECTINPUT_VERSION 0x0800

#include <SpecialK/input/input.h>
#include <SpecialK/input/dinput8_backend.h>
#include <SpecialK/window.h>
#include <SpecialK/console.h>

#include <SpecialK/utility.h>
#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <dinput.h>
#include <comdef.h>

#include <stdarg.h>

#include <imgui/imgui.h>
#include <SpecialK/tls.h>

#include <SpecialK/input/xinput.h>


bool
SK_InputUtil_IsHWCursorVisible (void)
{
  CURSORINFO cursor_info        = { };
             cursor_info.cbSize = sizeof (CURSORINFO);
  
  GetCursorInfo_Original (&cursor_info);

  return (cursor_info.flags & CURSOR_SHOWING);
}


#define SK_LOG_INPUT_CALL { static int  calls  = 0;                   { SK_LOG0 ( (L"[!] > Call #%lu: %hs", calls++, __FUNCTION__), L"Input Mgr." ); } }
#define SK_LOG_FIRST_CALL { static bool called = false; if (! called) { SK_LOG0 ( (L"[!] > First Call: %34hs", __FUNCTION__), L"Input Mgr." ); called = true; } }


#define SK_HID_READ(type)  SK_HID_Backend.markRead  (type);
#define SK_HID_WRITE(type) SK_HID_Backend.markWrite (type);

#define SK_RAWINPUT_READ(type)  SK_RawInput_Backend.markRead  (type);
#define SK_RAWINPUT_WRITE(type) SK_RawInput_Backend.markWrite (type);


//////////////////////////////////////////////////////////////
//
// HIDClass (User mode)
//
//////////////////////////////////////////////////////////////
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
          filter = true;
      } break;

      case HID_USAGE_GENERIC_POINTER:
      case HID_USAGE_GENERIC_MOUSE:
      {
        SK_HID_READ (sk_input_dev_type::Mouse)

        if (SK_ImGui_WantMouseCapture ())
          filter = true;
      } break;

      case HID_USAGE_GENERIC_KEYBOARD:
      case HID_USAGE_GENERIC_KEYPAD:
      {
        SK_HID_READ (sk_input_dev_type::Keyboard)

        if (SK_ImGui_WantKeyboardCapture ())
          filter = true;
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

BOOLEAN
_Success_(return)
__stdcall
HidD_GetPreparsedData_Detour (
  _In_  HANDLE                HidDeviceObject,
  _Out_ PHIDP_PREPARSED_DATA *PreparsedData )
{
  SK_LOG_FIRST_CALL

  PHIDP_PREPARSED_DATA pData = nullptr;
  BOOLEAN              bRet  =
    HidD_GetPreparsedData_Original ( HidDeviceObject,
                                       &pData );

  if (bRet)
  {
    SK_HID_PreparsedDataP = PreparsedData;
    SK_HID_PreparsedData  = pData;

    if (SK_HID_FilterPreparsedData (pData) || config.input.gamepad.disable_ps4_hid)
      return FALSE;

    *PreparsedData   =  pData;
  }

  // Can't figure out how The Witness works yet, but it will bypass input blocking
  //   on HID using a PS4 controller unless we return FALSE here.
  //return FALSE;
  return bRet;
}

HidD_GetPreparsedData_pfn  HidD_GetPreparsedData_Original  = nullptr;
HidD_FreePreparsedData_pfn HidD_FreePreparsedData_Original = nullptr;
HidD_GetFeature_pfn        HidD_GetFeature_Original        = nullptr;
HidP_GetData_pfn           HidP_GetData_Original           = nullptr;
HidP_GetCaps_pfn           HidP_GetCaps_Original           = nullptr;
SetCursor_pfn              SetCursor_Original              = nullptr;

BOOLEAN
__stdcall
HidD_FreePreparsedData_Detour (
  _In_ PHIDP_PREPARSED_DATA PreparsedData )
{
  BOOLEAN bRet =
    HidD_FreePreparsedData_Original (PreparsedData);

  if (PreparsedData == SK_HID_PreparsedData)
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


  if (! filter)
    return ret;

  else {
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

  if (! InterlockedCompareExchange (&hooked, 1, 0))
  {
    SK_LOG0 ( ( L"Game uses HID, installing input hooks..." ),
                L"   Input  " );

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
      (HidP_GetCaps_pfn)GetProcAddress ( GetModuleHandle (L"HID.DLL"),
                                           "HidP_GetCaps" );

    SK_ApplyQueuedHooks ();

    if (HidP_GetData_Original == nullptr)
      InterlockedDecrement (&hooked);
  }
}

void
SK_Input_PreHookHID (void)
{
  if (! config.input.gamepad.hook_hid)
    return;

  static sk_import_test_s tests [] = { { "hid.dll", false } };

  SK_TestImports (GetModuleHandle (nullptr), tests, 1);

  if (tests [0].used)// || GetModuleHandle (L"hid.dll"))
  {
    SK_Input_HookHID ();
  }
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

GetRegisteredRawInputDevices_pfn GetRegisteredRawInputDevices_Original = nullptr;

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
      //raw_devices.push_back (RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, 0x00, NULL });
      //raw_mice.push_back    (RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, 0x00, NULL });
      //raw_overrides.mouse.legacy_messages = true;
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
        RegisterRawInputDevices_Original ( &it, 1, sizeof RAWINPUTDEVICE );
      }

      else
      {
        different  |= (it.dwFlags & RIDEV_NOLEGACY) == 0;
        it.dwFlags |= RIDEV_NOLEGACY;
        RegisterRawInputDevices_Original ( &it, 1, sizeof RAWINPUTDEVICE );
      }
    
      overrides.emplace_back (it);
    }

    if (pDifferent != nullptr)
      *pDifferent = different;

    return overrides;
  }

  else
  {
    if (pDifferent != nullptr)
       *pDifferent = false;

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

    // Aw, the game doesn't have any mice -- let's fix that.
    if (raw_keyboards.empty ())
    {
      //raw_devices.push_back   (RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, 0x00, NULL });
      //raw_keyboards.push_back (RAWINPUTDEVICE { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, 0x00, NULL });
      //raw_overrides.keyboard.legacy_messages = true;
    }

    for (auto& it : raw_keyboards)
    {
      if (raw_overrides.keyboard.legacy_messages)
      {
        different |= ((it).dwFlags & RIDEV_NOLEGACY) != 0;
        (it).dwFlags &= ~(RIDEV_NOLEGACY | RIDEV_APPKEYS);
      }

      else
      {
        different |= ((it).dwFlags & RIDEV_NOLEGACY) == 0;
        (it).dwFlags |=   RIDEV_NOLEGACY | RIDEV_APPKEYS;
      }

      overrides.emplace_back (it);
    }

    if (pDifferent != nullptr)
      *pDifferent = different;

    return overrides;
  }

  else
  {
    if (pDifferent != nullptr)
      *pDifferent = false;

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

    std::vector <RAWINPUTDEVICE> device_override;

    bool different = false;

    std::vector <RAWINPUTDEVICE> mice = SK_RawInput_GetMice (&different);

    for (auto& it : raw_keyboards) device_override.push_back (it);
    for (auto& it : raw_gamepads)  device_override.push_back (it);
    for (auto& it : mice)          device_override.push_back (it);

//    dll_log.Log (L"%lu mice are now legacy...", mice.size ());

    RegisterRawInputDevices_Original (
      device_override.data (),
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

    RegisterRawInputDevices_Original (
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

    std::vector <RAWINPUTDEVICE> device_override;

    bool different = false;

    std::vector <RAWINPUTDEVICE> keyboards = SK_RawInput_GetKeyboards (&different);

    for (auto& it : keyboards)    device_override.push_back (it);
    for (auto& it : raw_gamepads) device_override.push_back (it);
    for (auto& it : raw_mice)     device_override.push_back (it);

    RegisterRawInputDevices_Original (
      device_override.data (),
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
  {
    raw_overrides.keyboard.active = false;

    RegisterRawInputDevices_Original (
      raw_devices.data (),
        static_cast <UINT> (raw_devices.size ()),
          sizeof RAWINPUTDEVICE
    );
  }
}

std::vector <RAWINPUTDEVICE>&
SK_RawInput_GetRegisteredGamepads (void)
{
  return raw_gamepads;
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
          raw_mice.push_back       (it);
          break;

        case HID_USAGE_GENERIC_KEYBOARD:
          raw_keyboards.push_back  (it);
          break;

        case HID_USAGE_GENERIC_JOYSTICK: // Joystick
        case HID_USAGE_GENERIC_GAMEPAD:  // Gamepad
          raw_gamepads.push_back   (it);
          break;

        default:
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
    GetRegisteredRawInputDevices_Original ( pDevices,
                                              &uiNumDevices,
                                                sizeof RAWINPUTDEVICE );

  assert (ret == -1);

  SetLastError (dwLastError);

  if (uiNumDevices != 0 && ret == -1)
  {
    pDevices = new
      RAWINPUTDEVICE [uiNumDevices + 1];

    GetRegisteredRawInputDevices_Original ( pDevices,
                                              &uiNumDevices,
                                                sizeof RAWINPUTDEVICE );

    raw_devices.clear ();

    for (UINT i = 0; i < uiNumDevices; i++)
      raw_devices.push_back (pDevices [i]);

    SK_RawInput_ClassifyDevices ();

    delete [] pDevices;
  }

  return uiNumDevices;
}

UINT
WINAPI
GetRegisteredRawInputDevices_Detour (
  _Out_opt_ PRAWINPUTDEVICE pRawInputDevices,
  _Inout_   PUINT           puiNumDevices,
  _In_      UINT            cbSize )
{
  UNREFERENCED_PARAMETER (cbSize);

  SK_LOG_FIRST_CALL

  assert (cbSize == sizeof RAWINPUTDEVICE);

  // On the first call to this function, we will need to query this stuff.
  static bool init = false;

  //if (! init)
  //{
    SK_RawInput_PopulateDeviceList ();
    //init = true;
  //}


  if (*puiNumDevices < static_cast <UINT> (raw_devices.size ()))
  {
      *puiNumDevices = static_cast <UINT> (raw_devices.size ());

    SetLastError (ERROR_INSUFFICIENT_BUFFER);

    return std::numeric_limits <UINT>::max ();
  }

  int idx = 0;

  if (pRawInputDevices)
  {
    for (auto it : raw_devices)
    {
      pRawInputDevices [idx++] = it;
    }
  }

  else
  {
    idx +=
      static_cast <int> (raw_devices.size ());
  }

  return idx;
}

BOOL WINAPI RegisterRawInputDevices_Detour (
  _In_ PCRAWINPUTDEVICE pRawInputDevices,
  _In_ UINT             uiNumDevices,
  _In_ UINT             cbSize )
{
  SK_LOG_FIRST_CALL

  if (cbSize != sizeof RAWINPUTDEVICE)
  {
    dll_log.Log ( L"[ RawInput ] RegisterRawInputDevices has wrong "
                  L"structure size (%lu bytes), expected: %zu",
                    cbSize,
                      sizeof RAWINPUTDEVICE );

    return
      RegisterRawInputDevices_Original ( pRawInputDevices,
                                           uiNumDevices,
                                             cbSize );
  }

  raw_devices.clear ();

  RAWINPUTDEVICE* pDevices = nullptr;

  if (pRawInputDevices && uiNumDevices > 0)
    pDevices = new RAWINPUTDEVICE [uiNumDevices];

  // The devices that we will pass to Windows after any overrides are applied
  std::vector <RAWINPUTDEVICE> actual_device_list;

  if (pDevices != nullptr)
  {
    // We need to continue receiving window messages for the console to work
    for (unsigned int i = 0; i < uiNumDevices; i++)
    {
      pDevices [i] = pRawInputDevices [i];
      raw_devices.push_back (pDevices [i]);
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
      RegisterRawInputDevices_Original ( actual_device_list.data   (),
                       static_cast <UINT> (actual_device_list.size () ),
                                             cbSize ) :
                FALSE;

  if (pDevices)
    delete [] pDevices;

  return bRet;
}

RegisterRawInputDevices_pfn RegisterRawInputDevices_Original = nullptr;
GetRawInputData_pfn         GetRawInputData_Original         = nullptr;
GetRawInputBuffer_pfn       GetRawInputBuffer_Original       = nullptr;

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
        ZeroMemory (pData, *pcbSize);
        const int max_items = (sizeof RAWINPUT / *pcbSize);
              int count     =                            0;
            auto *pTemp     = (uint8_t *)
                                  new RAWINPUT [max_items];
        uint8_t *pInput     =                        pTemp;
           auto *pOutput    =             (uint8_t *)pData;
        UINT     cbSize     =                     *pcbSize;
                  *pcbSize  =                            0;

        int       temp_ret  =
          GetRawInputBuffer_Original ( (RAWINPUT *)pTemp, &cbSize, cbSizeHeader );

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

  return GetRawInputBuffer_Original (pData, pcbSize, cbSizeHeader);
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
GetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                        _In_      UINT      uiCommand,
                        _Out_opt_ LPVOID    pData,
                        _Inout_   PUINT     pcbSize,
                        _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  //if (SK_ImGui_WantGamepadCapture ())
    return SK_ImGui_ProcessRawInput (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader, false);

 //return GetRawInputData_Original (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
}


/////////////////////////////////////////////////
//
// ImGui Cursor Management
//
/////////////////////////////////////////////////
#include <imgui/imgui.h>
sk_imgui_cursor_s SK_ImGui_Cursor;

HCURSOR GetGameCursor (void);

__inline
bool
SK_ImGui_IsMouseRelevant (void)
{
  // SK_ImGui_Visible is the full-blown config UI;
  //   but we also have floating widgets that may capture mouse
  //     input.
  return SK_ImGui_Active () || ImGui::IsAnyWindowHovered ();
}

__inline
void
sk_imgui_cursor_s::update (void)
{
  if (GetGameCursor () != nullptr)
    SK_ImGui_Cursor.orig_img = GetGameCursor ();

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

__inline
void
sk_imgui_cursor_s::showImGuiCursor (void)
{
  showSystemCursor (false);
}

__inline
void
sk_imgui_cursor_s::LocalToScreen (LPPOINT lpPoint)
{
  LocalToClient  (lpPoint);
  ClientToScreen (game_window.hWnd, lpPoint);
}

void
sk_imgui_cursor_s::LocalToClient (LPPOINT lpPoint)
{
  RECT real_client;
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
  RECT real_client;
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

  float x = 2.0f * ((float)lpPoint->x / std::max (1.0f, in.width )) - 1.0f;
  float y = 2.0f * ((float)lpPoint->y / std::max (1.0f, in.height)) - 1.0f;
                                        // Avoid division-by-zero, this should be a signaling NAN but
                                        //   some games alter FPU behavior and will turn this into a non-continuable exception.

  lpPoint->x = (LONG)( ( x * out.width  + out.width  ) / 2.0f );
  lpPoint->y = (LONG)( ( y * out.height + out.height ) / 2.0f );
}

__inline
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

  if (ImGui::GetIO ().MouseDownDuration [0] <= 0.0f || last_cursor == nullptr)
  {
    switch (ImGui::GetMouseCursor ())
    {
      case ImGuiMouseCursor_Arrow:
        //SetCursor_Original ((last_cursor = LoadCursor (nullptr, IDC_ARROW)));
        return ((last_cursor = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_POINTER)));
        break;                          
      case ImGuiMouseCursor_TextInput:  
        return ((last_cursor = LoadCursor (nullptr, IDC_IBEAM)));
        break;                          
      case ImGuiMouseCursor_ResizeEW:
        //SetCursor_Original ((last_cursor = LoadCursor (nullptr, IDC_SIZEWE)));
        return ((last_cursor = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_HORZ)));
        break;                          
      case ImGuiMouseCursor_ResizeNWSE: 
        return ((last_cursor = LoadCursor (nullptr, IDC_SIZENWSE)));
        break;
    }
  }

  else
    return (last_cursor);

  return GetCursor ();
}

void
ImGuiCursor_Impl (void)
{
  CURSORINFO ci = { };
  ci.cbSize     = sizeof CURSORINFO;

  GetCursorInfo_Original (&ci);

  //
  // Hardware Cursor
  //
  if (config.input.ui.use_hw_cursor)
    SetCursor_Original (ImGui_DesiredCursor ());

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
      SetCursor_Original (nullptr);
      ImGui::GetIO ().MouseDrawCursor = (! SK_ImGui_Cursor.idle);
    }
  }
}

void
sk_imgui_cursor_s::showSystemCursor (bool system)
{
  CURSORINFO cursor_info = { };
  cursor_info.cbSize     = sizeof (CURSORINFO);

  static HCURSOR wait_cursor =
    LoadCursor (nullptr, IDC_WAIT);

  if (SK_ImGui_Cursor.orig_img == wait_cursor)
    SK_ImGui_Cursor.orig_img = LoadCursor (nullptr, IDC_ARROW);

  if (system)
  {
    SetCursor_Original     (SK_ImGui_Cursor.orig_img);
    GetCursorInfo_Original (&cursor_info);

    if ((! SK_ImGui_IsMouseRelevant ()) || (cursor_info.flags & CURSOR_SHOWING))
      ImGui::GetIO ().MouseDrawCursor = false;

    else
      ImGuiCursor_Impl ();
  }

  else
    ImGuiCursor_Impl ();
}


void
sk_imgui_cursor_s::activateWindow (bool active)
{
  CURSORINFO ci = { };
  ci.cbSize     = sizeof ci;

  GetCursorInfo_Original (&ci);

  if (active)
  {
    if (SK_ImGui_IsMouseRelevant ())
    {
      if (SK_ImGui_WantMouseCapture ())
      {

      }

      else if (SK_ImGui_Cursor.orig_img)
        SetCursor_Original (SK_ImGui_Cursor.orig_img);
    }
  }
}




HCURSOR game_cursor = nullptr;

bool
SK_ImGui_WantKeyboardCapture (void)
{
  bool imgui_capture = false;

  //if (SK_ImGui_Visible)
  //{
    ImGuiIO& io =
      ImGui::GetIO ();

    if (nav_usable || io.WantCaptureKeyboard || io.WantTextInput)
      imgui_capture = true;
  //}

  return imgui_capture;
}

bool
SK_ImGui_WantTextCapture (void)
{
  bool imgui_capture = false;

  //if (SK_ImGui_Visible)
  //{
    ImGuiIO& io =
      ImGui::GetIO ();

    if (io.WantTextInput)
      imgui_capture = true;
  //}

  return imgui_capture;
}

bool
SK_ImGui_WantGamepadCapture (void)
{
  bool imgui_capture = false;

  if (SK_ImGui_Active ())
  {
    if (nav_usable)
      imgui_capture = true;
  }

#ifdef _WIN64
  // Stupid hack, breaking whatever abstraction this horrible mess passes for
  extern bool __FAR_Freelook;
  if (__FAR_Freelook)
    imgui_capture = true;
#endif

  extern bool SK_ImGui_GamepadComboDialogActive;

  if (SK_ImGui_GamepadComboDialogActive)
    imgui_capture = true;

  return imgui_capture;
}

bool
SK_ImGui_WantMouseCapture (void)
{
  bool imgui_capture = false;

  if (SK_ImGui_IsMouseRelevant ())
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    if (config.input.ui.capture_mouse || io.WantCaptureMouse/* || io.WantTextInput*/)
      imgui_capture = true;

    if (config.input.ui.capture_hidden && (! SK_InputUtil_IsHWCursorVisible ()))
      imgui_capture = true;
  }

  return imgui_capture;
}

HCURSOR GetGameCursor (void)
{
  static HCURSOR sk_imgui_arrow = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_POINTER);
  static HCURSOR sk_imgui_horz  = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_HORZ);
  static HCURSOR sk_imgui_ibeam = LoadCursor (nullptr, IDC_IBEAM);
  static HCURSOR sys_arrow      = LoadCursor (nullptr, IDC_ARROW);
  static HCURSOR sys_wait       = LoadCursor (nullptr, IDC_WAIT);

  static HCURSOR hCurLast = nullptr;
         HCURSOR hCur     = GetCursor ();

  if ( hCur != sk_imgui_horz && hCur != sk_imgui_arrow && hCur != sk_imgui_ibeam &&
       hCur != sys_arrow     && hCur != sys_wait )
    hCurLast = hCur;

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
    GetCursorPos_Original         (&SK_ImGui_Cursor.pos);
    SK_ImGui_Cursor.ScreenToLocal (&SK_ImGui_Cursor.pos);

    ImGui::GetIO ().WantCaptureMouse = true;
  }

  else
  {
    if (SK_ImGui_Cursor.orig_img == nullptr)
      SK_ImGui_Cursor.orig_img = GetGameCursor ();

    if (SK_ImGui_WantMouseCapture ())
    {
      //ClipCursor_Original   (&SK_ImGui_Cursor.clip_rect);

      POINT screen = SK_ImGui_Cursor.orig_pos;
      SK_ImGui_Cursor.LocalToScreen (&screen);
      SetCursorPos_Original ( screen.x,
                              screen.y );
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
SetCursor_Detour (
  _In_opt_ HCURSOR hCursor )
{
  SK_LOG_FIRST_CALL

  SK_ImGui_Cursor.orig_img = hCursor;

  if (! SK_ImGui_IsMouseRelevant ())
    return SetCursor_Original (hCursor);
  else if (! (ImGui::GetIO ().WantCaptureMouse || hCursor == nullptr))
    return SetCursor_Original (hCursor);

  return GetGameCursor ();
}

BOOL
WINAPI
GetCursorInfo_Detour (PCURSORINFO pci)
{
  SK_LOG_FIRST_CALL

  POINT pt  = pci->ptScreenPos;
  BOOL  ret = GetCursorInfo_Original (pci);
        pci->ptScreenPos = pt;

  pci->hCursor =
    SK_ImGui_Cursor.orig_img;


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


  return GetCursorInfo_Original (pci);
}

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  SK_LOG_FIRST_CALL


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


  return GetCursorPos_Original (lpPoint);
}

BOOL
WINAPI
SetCursorPos_Detour (_In_ int x, _In_ int y)
{
  SK_LOG_FIRST_CALL

  // Game WANTED to change its position, so remember that.
  SK_ImGui_Cursor.orig_pos.x = x;
  SK_ImGui_Cursor.orig_pos.y = y;

  SK_ImGui_Cursor.ScreenToLocal (&SK_ImGui_Cursor.orig_pos);

  // Don't let the game continue moving the cursor while
  //   Alt+Tabbed out
  if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) &&
      config.window.background_render && (! game_window.active))
    return TRUE;

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
    return SetCursorPos_Original (x, y);
  }

  return TRUE;
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

  // TODO: Process this the right way...

  if (SK_ImGui_Active ())
  {
    return 0;
  }

  return SendInput_Original (nInputs, pInputs, cbSize);
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


GetKeyState_pfn             GetKeyState_Original             = nullptr;
GetAsyncKeyState_pfn        GetAsyncKeyState_Original        = nullptr;
GetKeyboardState_pfn        GetKeyboardState_Original        = nullptr;

SHORT
WINAPI
GetAsyncKeyState_Detour (_In_ int vKey)
{
  SK_LOG_FIRST_CALL

#define SK_ConsumeVKey(vKey) { GetAsyncKeyState_Original(vKey); return 0; }

  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible ())
    SK_ConsumeVKey (vKey);

  // Block keyboard input to the game while it's in the background
  if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) &&
      config.window.background_render && (! game_window.active))
    SK_ConsumeVKey (vKey);

  if (vKey & 0xF8) // Valid Keys:  8 - 255
  {
    if (SK_ImGui_WantKeyboardCapture ())
      SK_ConsumeVKey (vKey);
  }

  else
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
      SK_ConsumeVKey (vKey);
  }

  return GetAsyncKeyState_Original (vKey);
}

SHORT
WINAPI
GetKeyState_Detour (_In_ int nVirtKey)
{
  SK_LOG_FIRST_CALL

#define SK_ConsumeVirtKey(nVirtKey) { GetKeyState_Original(nVirtKey); return 0; }

  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible ())
    SK_ConsumeVirtKey (nVirtKey);

  // Block keyboard input to the game while it's in the background
  if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) &&
      config.window.background_render && (! game_window.active))
    SK_ConsumeVirtKey (nVirtKey);

  if (nVirtKey & 0xF8) // Valid Keys:  8 - 255
  {
    if (SK_ImGui_WantKeyboardCapture ())
      SK_ConsumeVirtKey (nVirtKey);
  }

  else
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
      SK_ConsumeVirtKey (nVirtKey);
  }

  return GetKeyState_Original (nVirtKey);
}

//typedef BOOL (WINAPI *SetKeyboardState_pfn)(PBYTE lpKeyState); // TODO

BOOL
WINAPI
GetKeyboardState_Detour (PBYTE lpKeyState)
{
  SK_LOG_FIRST_CALL

  if (SK_ImGui_WantKeyboardCapture ())
  {
    memset (&lpKeyState [7], 0, 248);
    return TRUE;
  }

  BOOL bRet =
    GetKeyboardState_Original (lpKeyState);

  if (bRet)
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
      memset (lpKeyState, 0, 7);
  }

  return bRet;
}

#include <Windowsx.h>
#include <dbt.h>

LRESULT
WINAPI
ImGui_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool
SK_ImGui_HandlesMessage (LPMSG lpMsg, bool, bool)
{
  assert (lpMsg->hwnd == game_window.hWnd);

  bool handled = false;

  switch (lpMsg->message)
  {
    case WM_SYSCOMMAND:
      if ((lpMsg->wParam & 0xfff0) == SC_KEYMENU && lpMsg->lParam == 0) // Disable ALT application menu
        handled = true;
      break;

    case WM_CHAR:
    case WM_MENUCHAR:
      handled = SK_ImGui_WantTextCapture ();
      break;

    // Fix for Melody's Escape, which attempts to remove these messages!
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
      if (ImGui_WndProcHandler (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam))
        handled = true;
    } break;

    case WM_SETCURSOR:
    {
      if (lpMsg->hwnd == game_window.hWnd && game_window.hWnd != nullptr)
        SK_ImGui_Cursor.update ();
    } break;

    // TODO: Does this message have an HWND always?
    case WM_DEVICECHANGE:
    {
      handled =
        ImGui_WndProcHandler (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
    } break;

    // Pre-Dispose These Mesages (fixes The Witness)
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
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
        ( ImGui_WndProcHandler (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam) != 0 ) &&
       SK_ImGui_WantMouseCapture ();
    } break;

    case WM_INPUT:
    {
      handled = (ImGui_WndProcHandler (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam) != 0);
    } break;

    default:
    {
      ImGui_WndProcHandler (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
    } break;
  }

  return handled;
}


#include <SpecialK/input/xinput_hotplug.h>

void SK_Input_Init (void);


// Parts of the Win32 API that are safe to hook from DLL Main
void SK_Input_PreInit (void)
{
  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetRawInputData",
                              GetRawInputData_Detour,
     static_cast_p2p <void> (&GetRawInputData_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetAsyncKeyState",
                              GetAsyncKeyState_Detour,
     static_cast_p2p <void> (&GetAsyncKeyState_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetKeyState",
                              GetKeyState_Detour,
     static_cast_p2p <void> (&GetKeyState_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetKeyboardState",
                              GetKeyboardState_Detour,
     static_cast_p2p <void> (&GetKeyboardState_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetCursorPos",
                              GetCursorPos_Detour,
     static_cast_p2p <void> (&GetCursorPos_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetCursorInfo",
                              GetCursorInfo_Detour,
     static_cast_p2p <void> (&GetCursorInfo_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetMouseMovePointsEx",
                              GetMouseMovePointsEx_Detour,
     static_cast_p2p <void> (&GetMouseMovePointsEx_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "SetCursor",
                              SetCursor_Detour,
     static_cast_p2p <void> (&SetCursor_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "SetCursorPos",
                              SetCursorPos_Detour,
     static_cast_p2p <void> (&SetCursorPos_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "SendInput",
                              SendInput_Detour,
     static_cast_p2p <void> (&SendInput_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "mouse_event",
                              mouse_event_Detour,
     static_cast_p2p <void> (&mouse_event_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "keybd_event",
                              keybd_event_Detour,
     static_cast_p2p <void> (&keybd_event_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "RegisterRawInputDevices",
                              RegisterRawInputDevices_Detour,
     static_cast_p2p <void> (&RegisterRawInputDevices_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetRegisteredRawInputDevices",
                              GetRegisteredRawInputDevices_Detour,
     static_cast_p2p <void> (&GetRegisteredRawInputDevices_Original) );

#if 0
  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetRawInputBuffer",
                              GetRawInputBuffer_Detour,
     static_cast_p2p <void> (&GetRawInputBuffer_Original) );
#endif

  if (config.input.gamepad.hook_xinput)
    SK_XInput_InitHotPlugHooks ();

  SK_ApplyQueuedHooks ();
}


void
SK_Input_Init (void)
{
  SK_Input_PreHookHID    ();
  SK_Input_PreHookDI8    ();
  SK_Input_PreHookXInput ();
}



sk_input_api_context_s SK_XInput_Backend;
sk_input_api_context_s SK_HID_Backend;
sk_input_api_context_s SK_RawInput_Backend;