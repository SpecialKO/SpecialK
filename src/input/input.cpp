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

bool SK_WantBackgroundRender (void)
{
  return
    config.window.background_render;
}

bool
SK_InputUtil_IsHWCursorVisible (void)
{
  if (SK_GetCursor () != nullptr)
  {
    CURSORINFO cursor_info        = { };
               cursor_info.cbSize = sizeof (CURSORINFO);

    SK_GetCursorInfo (&cursor_info);

    return
      ( (cursor_info.flags & CURSOR_SHOWING) && (cursor_info.hCursor != nullptr) );
  }

  return false;
}

#define SK_HID_READ(type)  SK_HID_Backend->markRead   (type);
#define SK_HID_WRITE(type) SK_HID_Backend->markWrite  (type);
#define SK_HID_VIEW(type)  SK_HID_Backend->markViewed (type);
#define SK_HID_HIDE(type)  SK_HID_Backend->markHidden (type);

#define SK_RAWINPUT_READ(type)  SK_RawInput_Backend->markRead   (type);
#define SK_RAWINPUT_WRITE(type) SK_RawInput_Backend->markWrite  (type);
#define SK_RAWINPUT_VIEW(type)  SK_RawInput_Backend->markViewed (type);
#define SK_RAWINPUT_HIDE(type)  SK_RawInput_Backend->markHidden (type);

enum class SK_Input_DeviceFileType
{
  None    = 0,
  HID     = 1,
  NVIDIA  = 2,
  Invalid = 4
};

struct SK_NVIDIA_DeviceFile {
  SK_NVIDIA_DeviceFile (void) = default;
  SK_NVIDIA_DeviceFile (HANDLE file, const wchar_t *wszPath) : hFile (file)
  {
    wcsncpy_s ( wszDevicePath, MAX_PATH,
                      wszPath, _TRUNCATE );
  };

  HANDLE  hFile                        = INVALID_HANDLE_VALUE;
  wchar_t wszDevicePath [MAX_PATH + 2] = { };
  bool    bDisableDevice               = FALSE;
};

struct SK_HID_DeviceFile {
  HIDP_CAPS         hidpCaps                           = { };
  wchar_t           wszProductName      [128]          = { };
  wchar_t           wszManufacturerName [128]          = { };
  wchar_t           wszSerialNumber     [128]          = { };
  wchar_t           wszDevicePath       [MAX_PATH + 2] = { };
  std::vector<BYTE> last_data_read;
  sk_input_dev_type device_type                        = sk_input_dev_type::Other;
  BOOL              bDisableDevice                     = FALSE;
  HANDLE            hFile                              = INVALID_HANDLE_VALUE;

  SK_HID_DeviceFile (void) = default;

  SK_HID_DeviceFile (HANDLE file, const wchar_t *wszPath)
  {
    static
      concurrency::concurrent_unordered_map <std::wstring, SK_HID_DeviceFile> known_paths;

    // This stuff can be REALLY slow when games are constantly enumerating devices,
    //   so keep a cache handy.
    if (known_paths.count (wszPath))
    {
      memcpy (this, &known_paths.at (wszPath), sizeof (SK_HID_DeviceFile));
      return;
    }

    wchar_t *lpFileName = nullptr;

    if (wszPath != nullptr)
    {
      wcsncpy_s ( wszDevicePath, MAX_PATH,
                  wszPath,       _TRUNCATE );
    }

    PHIDP_PREPARSED_DATA                 preparsed_data = nullptr;
    if (SK_HidD_GetPreparsedData (file, &preparsed_data))
    {
      if (HIDP_STATUS_SUCCESS == SK_HidP_GetCaps (preparsed_data, &hidpCaps))
      {
#if 0
        if (! DuplicateHandle (
                GetCurrentProcess (), file,
                GetCurrentProcess (), &hFile,
                  0x0, FALSE, DUPLICATE_SAME_ACCESS ) )
        {
          SK_LOGi0 (L"Failed to duplicate handle for HID device: %ws!", lpFileName);
          return;
        }
#else
        std::ignore = lpFileName;
        hFile       = file;
#endif

        DWORD dwBytesRead = 0;

        SK_DeviceIoControl (
          hFile, IOCTL_HID_GET_PRODUCT_STRING, 0, 0,
          wszProductName, 128, &dwBytesRead, nullptr
        );

        SK_DeviceIoControl (
          hFile, IOCTL_HID_GET_MANUFACTURER_STRING, 0, 0,
          wszManufacturerName, 128, &dwBytesRead, nullptr
        );

        SK_DeviceIoControl (
          hFile, IOCTL_HID_GET_SERIALNUMBER_STRING, 0, 0,
          wszSerialNumber, 128, &dwBytesRead, nullptr
        );

        if (hidpCaps.UsagePage == HID_USAGE_PAGE_GENERIC)
        {
          switch (hidpCaps.Usage)
          {
            case HID_USAGE_GENERIC_GAMEPAD:
            case HID_USAGE_GENERIC_JOYSTICK:
            case HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER:
            {
              device_type = sk_input_dev_type::Gamepad;
            } break;

            case HID_USAGE_GENERIC_POINTER:
            case HID_USAGE_GENERIC_MOUSE:
            {
              device_type = sk_input_dev_type::Mouse;
            } break;

            case HID_USAGE_GENERIC_KEYBOARD:
            case HID_USAGE_GENERIC_KEYPAD:
            {
              device_type = sk_input_dev_type::Keyboard;
            } break;
          }
        }

        if (device_type == sk_input_dev_type::Other)
        {
          if (hidpCaps.UsagePage == HID_USAGE_PAGE_KEYBOARD)
          {
            device_type = sk_input_dev_type::Keyboard;
          }

          else if (hidpCaps.UsagePage == HID_USAGE_PAGE_VR    ||
                   hidpCaps.UsagePage == HID_USAGE_PAGE_SPORT ||
                   hidpCaps.UsagePage == HID_USAGE_PAGE_GAME  ||
                   hidpCaps.UsagePage == HID_USAGE_PAGE_ARCADE)
          {
            device_type = sk_input_dev_type::Gamepad;
          }

          else
          {
            SK_LOGi1 (
              L"Unknown HID Device Type (Product=%ws):  UsagePage=%x, Usage=%x",
                wszProductName, hidpCaps.UsagePage, hidpCaps.Usage
            );
          }
        }

        SK_ReleaseAssert ( // WTF?
          SK_HidD_FreePreparsedData (preparsed_data) != FALSE
        );
      }
    }

    known_paths.insert ({ wszPath, *this });
  }

  bool setPollingFrequency (DWORD dwFreq)
  {
    std::ignore = dwFreq;

    return false;
  }

  bool isInputAllowed (void) const
  {
    if (bDisableDevice)
      return false;

    switch (device_type)
    {
      case sk_input_dev_type::Mouse:
        return (! SK_ImGui_WantMouseCapture ());
      case sk_input_dev_type::Keyboard:
        return (! SK_ImGui_WantKeyboardCapture ());
      case sk_input_dev_type::Gamepad:
        return (! SK_ImGui_WantGamepadCapture ());
      default: // No idea what this is, ignore it...
        break;
    }

    return true;
  }
};

       concurrency::concurrent_vector        <SK_HID_PlayStationDevice>     SK_HID_PlayStationControllers;
static concurrency::concurrent_unordered_map <HANDLE, SK_HID_DeviceFile>    SK_HID_DeviceFiles;
static concurrency::concurrent_unordered_map <HANDLE, SK_NVIDIA_DeviceFile> SK_NVIDIA_DeviceFiles;

// Faster check for known device files, the type can be checked after determining whether SK
//   knows about this device...
static concurrency::concurrent_unordered_set <HANDLE>                      SK_Input_DeviceFiles;

std::tuple <SK_Input_DeviceFileType, void*, bool>
SK_Input_GetDeviceFileAndState (HANDLE hFile)
{
  // Bloom filter since most file reads -are not- for input devices
  if ( SK_Input_DeviceFiles.find (hFile) ==
       SK_Input_DeviceFiles.cend (     ) )
  {
    return
      { SK_Input_DeviceFileType::None, nullptr, true };
  }

  //
  // Figure out device type
  //
  if ( const auto hid_iter  = SK_HID_DeviceFiles.find (hFile);
                  hid_iter != SK_HID_DeviceFiles.cend (     ) )
  {
    auto& hid_device =
          hid_iter->second;

    if (hid_device.device_type != sk_input_dev_type::Other)
    {
      return
        { SK_Input_DeviceFileType::HID, &hid_device, hid_device.isInputAllowed () };
    }
  }

  else if ( const auto nv_iter  = SK_NVIDIA_DeviceFiles.find (hFile);
                       nv_iter != SK_NVIDIA_DeviceFiles.cend (     ) )
  {
    return
      { SK_Input_DeviceFileType::NVIDIA, &nv_iter->second, true };
  }

  return
    { SK_Input_DeviceFileType::Invalid, nullptr, true };
}

SetupDiGetClassDevsW_pfn             SK_SetupDiGetClassDevsW             = nullptr;
SetupDiGetClassDevsExW_pfn           SK_SetupDiGetClassDevsExW           = nullptr;
SetupDiGetClassDevsA_pfn             SK_SetupDiGetClassDevsA             = nullptr;
SetupDiGetClassDevsExA_pfn           SK_SetupDiGetClassDevsExA           = nullptr;
SetupDiEnumDeviceInfo_pfn            SK_SetupDiEnumDeviceInfo            = nullptr;
SetupDiEnumDeviceInterfaces_pfn      SK_SetupDiEnumDeviceInterfaces      = nullptr;
SetupDiGetDeviceInterfaceDetailW_pfn SK_SetupDiGetDeviceInterfaceDetailW = nullptr;
SetupDiGetDeviceInterfaceDetailA_pfn SK_SetupDiGetDeviceInterfaceDetailA = nullptr;
SetupDiDestroyDeviceInfoList_pfn     SK_SetupDiDestroyDeviceInfoList     = nullptr;
  
void SK_HID_SetupPlayStationControllers (void)
{
  HDEVINFO hid_device_set = 
    SK_SetupDiGetClassDevsW (&GUID_DEVINTERFACE_HID, nullptr, nullptr, DIGCF_DEVICEINTERFACE |
                                                                       DIGCF_PRESENT);
  
  if (hid_device_set != INVALID_HANDLE_VALUE)
  {
    SP_DEVINFO_DATA devInfoData = {
      .cbSize = sizeof (SP_DEVINFO_DATA)
    };
  
    SP_DEVICE_INTERFACE_DATA devInterfaceData = {
      .cbSize = sizeof (SP_DEVICE_INTERFACE_DATA)
    };
  
    for (                                     DWORD dwDevIdx = 0            ;
          SK_SetupDiEnumDeviceInfo (hid_device_set, dwDevIdx, &devInfoData) ;
                                                  ++dwDevIdx )
    {
      devInfoData.cbSize      = sizeof (SP_DEVINFO_DATA);
      devInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);
  
      if (! SK_SetupDiEnumDeviceInterfaces ( hid_device_set, nullptr, &GUID_DEVINTERFACE_HID,
                                                   dwDevIdx, &devInterfaceData) )
      {
        continue;
      }
  
      static wchar_t devInterfaceDetailData [MAX_PATH + 2];
  
      ULONG ulMinimumSize = 0;
  
      SK_SetupDiGetDeviceInterfaceDetailW (
        hid_device_set, &devInterfaceData, nullptr,
          0, &ulMinimumSize, nullptr );
  
      if (GetLastError () != ERROR_INSUFFICIENT_BUFFER)
        continue;
  
      if (ulMinimumSize > sizeof (wchar_t) * (MAX_PATH + 2))
        continue;
  
      SP_DEVICE_INTERFACE_DETAIL_DATA *pDevInterfaceDetailData =
        (SP_DEVICE_INTERFACE_DETAIL_DATA *)devInterfaceDetailData;
  
      pDevInterfaceDetailData->cbSize =
        sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
  
      if ( SK_SetupDiGetDeviceInterfaceDetailW (
             hid_device_set, &devInterfaceData, pDevInterfaceDetailData,
               ulMinimumSize, &ulMinimumSize, nullptr ) )
      {
        wchar_t *wszFileName =
          pDevInterfaceDetailData->DevicePath;
  
        if (StrStrIW (wszFileName, L"VID_054c"))
        {
          SK_HID_PlayStationDevice controller;
  
          wcsncpy_s (controller.wszDevicePath, MAX_PATH,
                                wszFileName,   _TRUNCATE);
  
          controller.hDeviceFile =
            SK_CreateFile2 ( wszFileName, FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                          FILE_SHARE_READ   | FILE_SHARE_WRITE,
                                            OPEN_EXISTING, nullptr );
  
          if (controller.hDeviceFile != nullptr)
          {
            std::vector <HIDP_VALUE_CAPS> value_caps;

            if (! SK_HidD_GetPreparsedData (controller.hDeviceFile, &controller.pPreparsedData))
            	continue;

            HIDP_CAPS                                      caps = { };
              SK_HidP_GetCaps (controller.pPreparsedData, &caps);

            controller.input_report.resize (caps.InputReportByteLength);

            std::vector <HIDP_BUTTON_CAPS>
              buttonCapsArray;
              buttonCapsArray.resize (caps.NumberInputButtonCaps);

            USHORT num_caps =
              caps.NumberInputButtonCaps;

            if ( HIDP_STATUS_SUCCESS ==
              SK_HidP_GetButtonCaps ( HidP_Input,
                                        buttonCapsArray.data (), &num_caps,
                                          controller.pPreparsedData ) )
            {
              for (UINT i = 0 ; i < num_caps ; ++i)
              {
                // Face Buttons
                if (buttonCapsArray [i].IsRange)
                {
                  controller.button_report_id =
                    buttonCapsArray [i].ReportID;
                  controller.button_usage_min =
                    buttonCapsArray [i].Range.UsageMin;
                  controller.button_usage_max =
                    buttonCapsArray [i].Range.UsageMax;

                  controller.buttons.resize (
                    static_cast <size_t> (
                      controller.button_usage_max -
                      controller.button_usage_min + 1
                    )
                  );
                }

                // D-Pad
                else
                {
                  // No idea what a third set of buttons would be...
                  SK_ReleaseAssert (num_caps <= 2);

                  controller.dpad_report_id =
                    buttonCapsArray [i].ReportID;
                  controller.dpad.Usage =
                    buttonCapsArray [i].NotRange.Usage;
                  controller.dpad.Usage =
                    buttonCapsArray [i].UsagePage;
                }
              }

              // We need a contiguous array to read-back the set buttons,
              //   rather than allocating it dynamically, do it once and reuse.
              controller.button_usages.resize (controller.buttons.size ());

              USAGE idx = 0;

              for ( auto& button : controller.buttons )
              {
                button.UsagePage = buttonCapsArray [0].UsagePage;
                button.Usage     = controller.button_usage_min + idx++;
                button.state     = false;
              }
            }

            controller.bConnected = true;
            controller.bDualSense =
              StrStrIW (wszFileName, L"PID_0DF2") != nullptr ||
              StrStrIW (wszFileName, L"PID_0CE6") != nullptr;
  
            SK_HID_PlayStationControllers.push_back (controller);
          }
        }
      }
    }
  
    SK_SetupDiDestroyDeviceInfoList (hid_device_set);
  }
}

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

HidD_GetPreparsedData_pfn  SK_HidD_GetPreparsedData  = nullptr;
HidD_FreePreparsedData_pfn SK_HidD_FreePreparsedData = nullptr;
HidD_GetInputReport_pfn    SK_HidD_GetInputReport    = nullptr;
HidD_GetFeature_pfn        SK_HidD_GetFeature        = nullptr;
HidP_GetData_pfn           SK_HidP_GetData           = nullptr;
HidP_GetCaps_pfn           SK_HidP_GetCaps           = nullptr;
HidP_GetButtonCaps_pfn     SK_HidP_GetButtonCaps     = nullptr;
HidP_GetUsages_pfn         SK_HidP_GetUsages         = nullptr;

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
      case HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER:
      {
        if (SK_ImGui_WantGamepadCapture () && (! config.input.gamepad.native_ps4))
        {
          filter = true;

          SK_HID_HIDE (sk_input_dev_type::Gamepad);
        }

        else
        {
          SK_HID_READ (sk_input_dev_type::Gamepad);
          SK_HID_VIEW (sk_input_dev_type::Gamepad);
        }
      } break;

      case HID_USAGE_GENERIC_POINTER:
      case HID_USAGE_GENERIC_MOUSE:
      {
        if (SK_ImGui_WantMouseCapture ())
        {
          filter = true;

          SK_HID_HIDE (sk_input_dev_type::Mouse);
        }

        else
        {
          SK_HID_READ (sk_input_dev_type::Mouse);
          SK_HID_VIEW (sk_input_dev_type::Mouse);
        }
      } break;

      case HID_USAGE_GENERIC_KEYBOARD:
      case HID_USAGE_GENERIC_KEYPAD:
      {
        if (SK_ImGui_WantKeyboardCapture ())
        {
          filter = true;

          SK_HID_HIDE (sk_input_dev_type::Keyboard);
        }

        else
        {
          SK_HID_READ (sk_input_dev_type::Keyboard);
          SK_HID_VIEW (sk_input_dev_type::Keyboard);
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
  const BOOLEAN              bRet  =
    HidD_GetPreparsedData_Original ( HidDeviceObject,
                                       &pData );

  if (bRet && pData != nullptr)
  {
    SK_HID_PreparsedDataP = PreparsedData;
    SK_HID_PreparsedData  = pData;

    if ( config.input.gamepad.disable_ps4_hid  ||
                 SK_HID_FilterPreparsedData (pData))
    {
      if (! HidD_FreePreparsedData_Original (pData))
      {
        SK_ReleaseAssert (false && L"The Sky is Falling!");
      }

      return FALSE;
    }

    if (PreparsedData != nullptr)
       *PreparsedData = pData;
  }

  return
    bRet;
}

ShowCursor_pfn ShowCursor_Original = nullptr;

int
WINAPI
SK_ShowCursor (BOOL bShow)
{
  return (ShowCursor_Original != nullptr) ?
          ShowCursor_Original (bShow)     :
          ShowCursor          (bShow);
}

int
WINAPI
ShowCursor_Detour (BOOL bShow)
{
  CURSORINFO
    cursor_info        = { };
    cursor_info.cbSize = sizeof (CURSORINFO);

  SK_GetCursorInfo (&cursor_info);

  const bool bIsShowing =
    (cursor_info.flags & CURSOR_SHOWING);

  const bool bIsCapturing =
    (SK_ImGui_WantMouseCapture () && SK_ImGui_IsAnythingHovered ());

  const bool bCanHide =
    ((config.input.cursor.manage == false || config.input.cursor.timeout == 0 || SK_Window_IsCursorActive () == false) &&
                ((! bIsCapturing) || config.input.ui.use_hw_cursor == false)) || SK_ImGui_Cursor.force == sk_cursor_state::Hidden;
  const bool bCanShow =
    ((config.input.cursor.manage == false ||(config.input.cursor.timeout != 0 && SK_Window_IsCursorActive () == true)) &&
                ((! bIsCapturing) || config.input.ui.use_hw_cursor == true )) || SK_ImGui_Cursor.force == sk_cursor_state::Visible;


  static int expected_val = 0;

  SK_RunOnce (expected_val = ShowCursor_Original (FALSE) + 1;
                             ShowCursor_Original (TRUE));

  static constexpr int  _MAX_TRIES = 8;

  int ret = 0;

  ret = bShow ? ++expected_val
              : --expected_val;


  if (     SK_ImGui_Cursor.force == sk_cursor_state::Visible)
    bShow = true;
  else if (SK_ImGui_Cursor.force == sk_cursor_state::Hidden )
    bShow = false;


  if (config.input.cursor.manage && SK_ImGui_Cursor.force == sk_cursor_state::None)
  {
    if (bIsCapturing)
    {
      bShow = config.input.ui.use_hw_cursor;
    }

    if (bIsShowing)
    {
      if (bShow)
      {
        return ret;
      }

      else
      {
        if (! bCanHide)
        {
          return ret;
        }

        ShowCursor_Original (FALSE);

        return ret;
      }
    }

    else
    {
      if (! bShow)
      {
        return ret;
      }

      else
      {
        if (! bCanShow)
          return ret;

        ShowCursor_Original (TRUE);

        return ret;
      }
    }
  }


  else if (bIsCapturing || SK_ImGui_Cursor.force != sk_cursor_state::None)
  {
    if (SK_ImGui_Cursor.force == sk_cursor_state::None)
      bShow = config.input.ui.use_hw_cursor;

    if (bShow)
    {
      if (! (cursor_info.flags & CURSOR_SHOWING))
      {
        for ( int i = 0 ; i < _MAX_TRIES ; ++i )
        {
          if (ShowCursor_Original (TRUE) >= 0)
            break;
        }
      }

      return ret;
    }
  
    else
    {
      if (cursor_info.flags & CURSOR_SHOWING)
      {
        for ( int i = 0 ; i < _MAX_TRIES ; ++i )
        {
          if (ShowCursor_Original (FALSE) < 0)
            break;
        }
      }

      return ret;
    }
  }


  for ( int x = 0 ; x < 512 ; ++x )
  {
    int real_val =
      ShowCursor_Original (bShow);

    if (     real_val < ret) bShow = true;
    else if (real_val > ret) bShow = false;
    else break;
  }


  return
    ret;
}

SetCursor_pfn SetCursor_Original = nullptr;
GetCursor_pfn GetCursor_Original = nullptr;

GetCursorInfo_pfn       GetCursorInfo_Original       = nullptr;
NtUserGetCursorInfo_pfn NtUserGetCursorInfo_Original = nullptr;

BOOLEAN
__stdcall
HidD_FreePreparsedData_Detour (
  _In_ PHIDP_PREPARSED_DATA PreparsedData ) noexcept
{
  BOOLEAN bRet =
    HidD_FreePreparsedData_Original (
                               PreparsedData );
  if ( SK_HID_PreparsedData == PreparsedData )
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
  {
    return
      HidD_GetFeature_Original (
        HidDeviceObject, ReportBuffer,
                         ReportBufferLength
      );
  }

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

  if (          ret == HIDP_STATUS_SUCCESS &&
       ( ReportType == HidP_Input          ||
         ReportType == HidP_Output ) )
  {
    // This will classify the data for us, so don't record this event yet.
    filter =
      SK_HID_FilterPreparsedData (PreparsedData);
  }


  if (filter && (DataLength != nullptr))
  {
    using  HidP_MaxDataListLength_pfn = ULONG (WINAPI *)(HIDP_REPORT_TYPE, PHIDP_PREPARSED_DATA);
    static HidP_MaxDataListLength_pfn
        SK_HidP_MaxDataListLength =
          (HidP_MaxDataListLength_pfn)SK_GetProcAddress (L"hid.dll",
          "HidP_MaxDataListLength");

    if ( SK_HidP_MaxDataListLength != nullptr &&
         SK_HidP_MaxDataListLength (ReportType, PreparsedData) != 0 )
    {
      if (    DataList != nullptr)
      memset (DataList, 0, sizeof (HIDP_DATA) * *DataLength);
                                                *DataLength = 0;
    }
  }

  return ret;
}

OpenFileMappingW_pfn      OpenFileMappingW_Original      = nullptr;
CreateFileMappingW_pfn    CreateFileMappingW_Original    = nullptr;
ReadFile_pfn              ReadFile_Original              = nullptr;
ReadFileEx_pfn            ReadFileEx_Original            = nullptr;
GetOverlappedResult_pfn   GetOverlappedResult_Original   = nullptr;
GetOverlappedResultEx_pfn GetOverlappedResultEx_Original = nullptr;
DeviceIoControl_pfn       DeviceIoControl_Original       = nullptr;

static CreateFileA_pfn CreateFileA_Original = nullptr;
static CreateFileW_pfn CreateFileW_Original = nullptr;
static CreateFile2_pfn CreateFile2_Original = nullptr;

CreateFile2_pfn              SK_CreateFile2 = nullptr;
ReadFile_pfn                    SK_ReadFile = nullptr;

#if 0
NTSTATUS
WINAPI
SK_HidP_GetCaps (_In_  PHIDP_PREPARSED_DATA PreparsedData,
                 _Out_ PHIDP_CAPS           Capabilities)
{
  static HidP_GetCaps_pfn _HidP_GetCaps = nullptr;

  if (HidP_GetCaps_Original != nullptr)
    return HidP_GetCaps_Original (PreparsedData, Capabilities);

  else
  {
    if (_HidP_GetCaps == nullptr)
    {
      _HidP_GetCaps =
        (HidP_GetCaps_pfn)SK_GetProcAddress (L"hid.dll",
        "HidP_GetCaps");
    }
  }

  if (_HidP_GetCaps != nullptr)
  {
    return
      _HidP_GetCaps (PreparsedData, Capabilities);
  }

  return HIDP_STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
WINAPI
SK_HidD_GetPreparsedData (_In_  HANDLE                HidDeviceObject,
                          _Out_ PHIDP_PREPARSED_DATA *PreparsedData)
{
  static HidD_GetPreparsedData_pfn _HidD_GetPreparsedData = nullptr;

  if (HidD_GetPreparsedData_Original != nullptr)
    return HidD_GetPreparsedData_Original (HidDeviceObject, PreparsedData);

  else
  {
    if (_HidD_GetPreparsedData == nullptr)
    {
      _HidD_GetPreparsedData =
        (HidD_GetPreparsedData_pfn)SK_GetProcAddress (L"hid.dll",
        "HidD_GetPreparsedData");
    }
  }

  if (_HidD_GetPreparsedData != nullptr)
  {
    return
      _HidD_GetPreparsedData (HidDeviceObject, PreparsedData);
  }

  return FALSE;
}

BOOLEAN
WINAPI
SK_HidD_FreePreparsedData (_In_ PHIDP_PREPARSED_DATA PreparsedData)
{
  static HidD_FreePreparsedData_pfn _HidD_FreePreparsedData = nullptr;

  if (HidD_FreePreparsedData_Original != nullptr)
    return HidD_FreePreparsedData_Original (PreparsedData);

  else
  {
    if (_HidD_FreePreparsedData == nullptr)
    {
      _HidD_FreePreparsedData =
        (HidD_FreePreparsedData_pfn)SK_GetProcAddress (L"hid.dll",
        "HidD_FreePreparsedData");
    }
  }

  if (_HidD_FreePreparsedData != nullptr)
  {
    return
      _HidD_FreePreparsedData (PreparsedData);
  }

  return FALSE;
}
#endif

BOOL
WINAPI
SK_DeviceIoControl (HANDLE       hDevice,
                    DWORD        dwIoControlCode,
                    LPVOID       lpInBuffer,
                    DWORD        nInBufferSize,
                    LPVOID       lpOutBuffer,
                    DWORD        nOutBufferSize,
                    LPDWORD      lpBytesReturned,
                    LPOVERLAPPED lpOverlapped)
{
  static DeviceIoControl_pfn _DeviceIoControl = nullptr;

  if (DeviceIoControl_Original != nullptr)
  {
    return DeviceIoControl_Original (
      hDevice, dwIoControlCode, lpInBuffer, nInBufferSize,
        lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped
    );
  }

  else
  {
    if (_DeviceIoControl == nullptr)
    {
      _DeviceIoControl =
        (DeviceIoControl_pfn)SK_GetProcAddress (L"kernel32.dll",
        "DeviceIoControl");
    }
  }

  if (_DeviceIoControl != nullptr)
  {
    return _DeviceIoControl (
      hDevice, dwIoControlCode, lpInBuffer, nInBufferSize,
        lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped
    );
  }

  return FALSE;
}

static
HANDLE
WINAPI
OpenFileMappingW_Detour (DWORD   dwDesiredAccess,
                         BOOL    bInheritHandle,
                         LPCWSTR lpName)
{
  SK_LOG_FIRST_CALL

  SK_LOGi0 (L"OpenFileMappingW (%ws)", lpName);

  return
    OpenFileMappingW_Original (
      dwDesiredAccess, bInheritHandle, lpName
    );
}

static
HANDLE
WINAPI
CreateFileMappingW_Detour (HANDLE                hFile,
                           LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
                           DWORD                 flProtect,
                           DWORD                 dwMaximumSizeHigh,
                           DWORD                 dwMaximumSizeLow,
                           LPCWSTR               lpName)
{
  SK_LOG_FIRST_CALL

  SK_LOGi0 (L"CreateFileMappingW (%ws)", lpName);

  return
    CreateFileMappingW_Original (
      hFile, lpFileMappingAttributes, flProtect,
        dwMaximumSizeHigh, dwMaximumSizeLow, lpName
    );
}

static
BOOL
WINAPI
ReadFile_Detour (HANDLE       hFile,
                 LPVOID       lpBuffer,
                 DWORD        nNumberOfBytesToRead,
                 LPDWORD      lpNumberOfBytesRead,
                 LPOVERLAPPED lpOverlapped)
{
  SK_LOG_FIRST_CALL

  const auto &[ dev_file_type, dev_ptr, dev_allowed ] =
    SK_Input_GetDeviceFileAndState (hFile);

  switch (dev_file_type)
  {
    case SK_Input_DeviceFileType::HID:
    {
      auto hid_file =
        (SK_HID_DeviceFile *)dev_ptr;

      auto pTlsBackedBuffer =
        SK_TLS_Bottom ()->scratch_memory->log.
                             formatted_output.alloc (nNumberOfBytesToRead);

      auto pBuffer =
        // Overlapped reads should pass their original pointer to ReadFile,
        //   unless we intend to block them (and cancel said IO request).
        ((lpBuffer != nullptr && lpOverlapped == nullptr) || (! dev_allowed)) ?
                                                             pTlsBackedBuffer : lpBuffer;

      BOOL bRet =
        ReadFile_Original (
          hFile, pBuffer, nNumberOfBytesToRead,
            lpNumberOfBytesRead, lpOverlapped
        );

      if (bRet)
      {
        if (! dev_allowed)
        {
          SK_ReleaseAssert (lpBuffer != nullptr);

          if (lpOverlapped == nullptr || CancelIo (hFile))
          {
            SK_HID_HIDE (hid_file->device_type);

            if (lpOverlapped == nullptr) // lpNumberOfBytesRead MUST be non-null
            {
              if (hid_file->last_data_read.size () >= *lpNumberOfBytesRead)
              {
                // Give the game old data, from before we started blocking stuff...
                memcpy (lpBuffer, hid_file->last_data_read.data (), *lpNumberOfBytesRead);
              }
            }

            else
            {
              SK_RunOnce (
                SK_LOGi0 (L"ReadFile HID IO Cancelled")
              );
            }

            return TRUE;
          }
        }

        else
        {
          SK_HID_READ (hid_file->device_type);

          if (lpNumberOfBytesRead != nullptr && lpBuffer != nullptr && lpOverlapped == nullptr)
          {
            if (hid_file->last_data_read.size () < nNumberOfBytesToRead)
                hid_file->last_data_read.resize (  nNumberOfBytesToRead * 2);

            memcpy (hid_file->last_data_read.data (), pTlsBackedBuffer, *lpNumberOfBytesRead);
            memcpy (lpBuffer,                         pTlsBackedBuffer, *lpNumberOfBytesRead);
          }

          SK_HID_VIEW (hid_file->device_type);
        }
      }

      return bRet;
    } break;

    case SK_Input_DeviceFileType::NVIDIA:
    {
      SK_MessageBus_Backend->markRead (2);
    } break;
  }

  return
    ReadFile_Original (
      hFile, lpBuffer, nNumberOfBytesToRead,
        lpNumberOfBytesRead, lpOverlapped );
}

static
BOOL
WINAPI
ReadFileEx_Detour (HANDLE                          hFile,
                   LPVOID                          lpBuffer,
                   DWORD                           nNumberOfBytesToRead,
                   LPOVERLAPPED                    lpOverlapped,
                   LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
  SK_LOG_FIRST_CALL

  BOOL bRet =
    ReadFileEx_Original (
      hFile, lpBuffer, nNumberOfBytesToRead,
        lpOverlapped, lpCompletionRoutine
    );

  // Early-out
  if (! bRet)
    return bRet;

  const auto &[ dev_file_type, dev_ptr, dev_allowed ] =
    SK_Input_GetDeviceFileAndState (hFile);

  switch (dev_file_type)
  {
    case SK_Input_DeviceFileType::HID:
    {
      auto hid_file =
        (SK_HID_DeviceFile *)dev_ptr;

      if (! dev_allowed)
      {
        if (CancelIo (hFile))
        {
          SK_HID_HIDE (hid_file->device_type);

          SK_RunOnce (
            SK_LOGi0 (L"ReadFileEx HID IO Cancelled")
          );

          return TRUE;
        }
      }

      SK_HID_READ (hid_file->device_type);
      SK_HID_VIEW (hid_file->device_type);
    } break;

    case SK_Input_DeviceFileType::NVIDIA:
    {
      SK_MessageBus_Backend->markRead (2);
    }
  }

  return bRet;
}

bool
SK_StrSupW (const wchar_t *wszString, const wchar_t *wszPattern, int len = -1)
{
  if ( nullptr == wszString || wszPattern == nullptr )
    return        wszString == wszPattern;

  if (len == -1)
      len = sk::narrow_cast <int> (wcslen (wszPattern));

  return
    0 == StrCmpNIW (wszString, wszPattern, len);
}

bool
SK_StrSupA (const char *szString, const char *szPattern, int len = -1)
{
  if ( nullptr == szString || szPattern == nullptr )
    return        szString == szPattern;

  if (len == -1)
      len = sk::narrow_cast <int> (strlen (szPattern));

  return
    0 == StrCmpNIA (szString, szPattern, len);
}

static
HANDLE
WINAPI
CreateFileA_Detour (LPCSTR                lpFileName,
                    DWORD                 dwDesiredAccess,
                    DWORD                 dwShareMode,
                    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                    DWORD                 dwCreationDisposition,
                    DWORD                 dwFlagsAndAttributes,
                    HANDLE                hTemplateFile)
{
  SK_LOG_FIRST_CALL

  HANDLE hRet =
    CreateFileA_Original (
      lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
          dwFlagsAndAttributes, hTemplateFile );

  const bool bSuccess = (LONG_PTR)hRet > 0;

  // Examine all UNC paths closely, some of these files are
  //   input devices in disguise...
  if ( bSuccess && SK_StrSupA (lpFileName, R"(\\)", 2) )
  {
    // Log UNC paths we don't expect for debugging
    static const bool bTraceAllUNC =
      ( config.system.log_level > 0 || config.file_io.trace_reads );

    std::wstring widefilename =
       SK_UTF8ToWideChar (lpFileName);

    const auto lpWideFileName =
                 widefilename.c_str ();

    if (SK_StrSupA (lpFileName, R"(\\?\hid)", 7))
    {
      bool bSkipExistingFile = false;

      if ( auto hid_file  = SK_HID_DeviceFiles.find (hRet);
                hid_file != SK_HID_DeviceFiles.end  (    ) && StrCmpIW (lpWideFileName,
                hid_file->second.wszDevicePath) == 0 )
      {
        bSkipExistingFile = true;
      }

      if (! bSkipExistingFile)
      {
        SK_HID_DeviceFile hid_file (hRet, lpWideFileName);

        if (hid_file.device_type != sk_input_dev_type::Other)
        {
          SK_Input_DeviceFiles.insert (hRet);
        }

        auto&& dev_file =
          SK_HID_DeviceFiles [hRet];

        dev_file.last_data_read.clear ();
        dev_file = std::move (hid_file);
      }
    }

    else if (SK_StrSupA (lpFileName, R"(\\.\pipe)", 8))
    {
#if 0 // Add option to disable MessageBus? It seems harmless
      if (config.input.gamepad.steam.disabled_to_game)
      {
        // If we can't close this, something's gone wrong...
        SK_ReleaseAssert (
          SK_CloseHandle (hRet)
        );

        return
          INVALID_HANDLE_VALUE;
      }
#endif

      SK_Input_DeviceFiles.insert (  hRet                );
      SK_NVIDIA_DeviceFiles         [hRet] =
               SK_NVIDIA_DeviceFile (hRet, lpWideFileName);
    }

    else if (bTraceAllUNC && SK_StrSupA (lpFileName, R"(\\)", 2))
    {
      SK_LOGi0 (L"CreateFileA (%hs)", lpFileName);
    }
  }

  return hRet;
}

static
HANDLE
WINAPI
CreateFile2_Detour (
  _In_     LPCWSTR                           lpFileName,
  _In_     DWORD                             dwDesiredAccess,
  _In_     DWORD                             dwShareMode,
  _In_     DWORD                             dwCreationDisposition,
  _In_opt_ LPCREATEFILE2_EXTENDED_PARAMETERS pCreateExParams )
{
  SK_LOG_FIRST_CALL

  HANDLE hRet =
    CreateFile2_Original (
      lpFileName, dwDesiredAccess, dwShareMode,
        dwCreationDisposition, pCreateExParams );

  const bool bSuccess = (LONG_PTR)hRet > 0;

  // Examine all UNC paths closely, some of these files are
  //   input devices in disguise...
  if ( bSuccess && SK_StrSupW (lpFileName, LR"(\\)", 2) )
  {
    // Log UNC paths we don't expect for debugging
    static const bool bTraceAllUNC =
      ( config.system.log_level > 0 || config.file_io.trace_reads );

    if (SK_StrSupW (lpFileName, LR"(\\?\hid)", 7))
    {
      bool bSkipExistingFile = false;

      if ( auto hid_file  = SK_HID_DeviceFiles.find (hRet);
                hid_file != SK_HID_DeviceFiles.end  (    ) && StrCmpIW (lpFileName,
                hid_file->second.wszDevicePath) == 0 )
      {
        bSkipExistingFile = true;
      }

      if (! bSkipExistingFile)
      {
        SK_HID_DeviceFile hid_file (hRet, lpFileName);

        if (hid_file.device_type != sk_input_dev_type::Other)
        {
          SK_Input_DeviceFiles.insert (hRet);
        }

        auto&& dev_file =
          SK_HID_DeviceFiles [hRet];

        dev_file.last_data_read.clear ();
        dev_file = std::move (hid_file);
      }
    }

    else if (SK_StrSupW (lpFileName, LR"(\\.\pipe)", 8))
    {
#if 0 // Add option to disable MessageBus? It seems harmless
      if (config.input.gamepad.steam.disabled_to_game)
      {
        // If we can't close this, something's gone wrong...
        SK_ReleaseAssert (
          SK_CloseHandle (hRet)
        );

        return
          INVALID_HANDLE_VALUE;
      }
#endif

      SK_Input_DeviceFiles.insert (  hRet            );
      SK_NVIDIA_DeviceFiles         [hRet] =
               SK_NVIDIA_DeviceFile (hRet, lpFileName);
    }

    else if (bTraceAllUNC && SK_StrSupW (lpFileName, LR"(\\)", 2))
    {
      SK_LOGi0 (L"CreateFile2 (%ws)", lpFileName);
    }
  }

  return hRet;
}

static
HANDLE
WINAPI
CreateFileW_Detour ( LPCWSTR               lpFileName,
                     DWORD                 dwDesiredAccess,
                     DWORD                 dwShareMode,
                     LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     DWORD                 dwCreationDisposition,
                     DWORD                 dwFlagsAndAttributes,
                     HANDLE                hTemplateFile )
{
  SK_LOG_FIRST_CALL

  HANDLE hRet =
    CreateFileW_Original (
      lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
          dwFlagsAndAttributes, hTemplateFile );

  const bool bSuccess = (LONG_PTR)hRet > 0;

  // Examine all UNC paths closely, some of these files are
  //   input devices in disguise...
  if ( bSuccess && SK_StrSupW (lpFileName, LR"(\\)", 2) )
  {
    // Log UNC paths we don't expect for debugging
    static const bool bTraceAllUNC =
      ( config.system.log_level > 0 || config.file_io.trace_reads );

    if (SK_StrSupW (lpFileName, LR"(\\?\hid)", 7))
    {
      bool bSkipExistingFile = false;
      
      if ( auto hid_file  = SK_HID_DeviceFiles.find (hRet);
                hid_file != SK_HID_DeviceFiles.end  (    ) && StrCmpIW (lpFileName,
                hid_file->second.wszDevicePath) == 0 )
      {
        bSkipExistingFile = true;
      }

      if (! bSkipExistingFile)
      {
        SK_HID_DeviceFile hid_file (hRet, lpFileName);

        if (hid_file.device_type != sk_input_dev_type::Other)
        {
          SK_Input_DeviceFiles.insert (hRet);
        }

        auto&& dev_file =
          SK_HID_DeviceFiles [hRet];

        dev_file.last_data_read.clear ();
        dev_file = std::move (hid_file);
      }
    }

    else if (SK_StrSupW (lpFileName, LR"(\\.\pipe)", 8))
    {
#if 0 // Add option to disable MessageBus? It seems harmless
      if (config.input.gamepad.steam.disabled_to_game)
      {
        // If we can't close this, something's gone wrong...
        SK_ReleaseAssert (
          SK_CloseHandle (hRet)
        );

        return
          INVALID_HANDLE_VALUE;
      }
#endif

      SK_Input_DeviceFiles.insert (  hRet            );
      SK_NVIDIA_DeviceFiles         [hRet] =
               SK_NVIDIA_DeviceFile (hRet, lpFileName);
    }

    else if (bTraceAllUNC && SK_StrSupW (lpFileName, LR"(\\)", 2))
    {
      SK_LOGi0 (L"CreateFileW (%ws)", lpFileName);
    }
  }

  return hRet;
}

// This is the most common way that games that manually open USB HID
//   device files actually read their input (usually the non-Ex variant).
BOOL
WINAPI
GetOverlappedResultEx_Detour (HANDLE       hFile,
                              LPOVERLAPPED lpOverlapped,
                              LPDWORD      lpNumberOfBytesTransferred,
                              DWORD        dwMilliseconds,
                              BOOL         bWait)
{
  SK_LOG_FIRST_CALL

  const auto &[ dev_file_type, dev_ptr, dev_allowed ] =
    SK_Input_GetDeviceFileAndState (hFile);

  switch (dev_file_type)
  {
    case SK_Input_DeviceFileType::HID:
    {
      auto hid_file =
        (SK_HID_DeviceFile *)dev_ptr;

      BOOL bRet = TRUE;

      if (! dev_allowed)
      {
        if (CancelIo (hFile))
        {
          SK_HID_HIDE (hid_file->device_type);

          SK_RunOnce (
            SK_LOGi0 (L"GetOverlappedResultEx HID IO Cancelled")
          );
        }
      }

      else
      {
        bRet =
          GetOverlappedResultEx_Original (
            hFile, lpOverlapped, lpNumberOfBytesTransferred, dwMilliseconds, bWait
          );
      }

      if (bRet != FALSE)
      {
        if (dev_allowed)
        {
          SK_HID_READ (hid_file->device_type);
          SK_HID_VIEW (hid_file->device_type);
          // We did the bulk of this processing in ReadFile_Detour (...)
          //   nothing to be done here for now.
        }
      }

      return bRet;
    } break;

    case SK_Input_DeviceFileType::NVIDIA:
    {
      SK_MessageBus_Backend->markRead (2);
    } break;
  }

  return
    GetOverlappedResultEx_Original (
      hFile, lpOverlapped, lpNumberOfBytesTransferred, dwMilliseconds, bWait
    );
}

BOOL
WINAPI
GetOverlappedResult_Detour (HANDLE       hFile,
                            LPOVERLAPPED lpOverlapped,
                            LPDWORD      lpNumberOfBytesTransferred,
                            BOOL         bWait)
{
  SK_LOG_FIRST_CALL

  const auto &[ dev_file_type, dev_ptr, dev_allowed ] =
    SK_Input_GetDeviceFileAndState (hFile);

  switch (dev_file_type)
  {
    case SK_Input_DeviceFileType::HID:
    {
      auto hid_file =
        (SK_HID_DeviceFile *)dev_ptr;

      BOOL bRet = TRUE;

      if (! dev_allowed)
      {
        if (CancelIo (hFile))
        {
          SK_HID_HIDE (hid_file->device_type);

          SK_RunOnce (
            SK_LOGi0 (L"GetOverlappedResult HID IO Cancelled")
          );
        }
      }

      else
        bRet =
          GetOverlappedResult_Original (
            hFile, lpOverlapped, lpNumberOfBytesTransferred,
              bWait
          );

      if (bRet != FALSE)
      {
        if (dev_allowed)
        {
          SK_HID_READ (hid_file->device_type);
          SK_HID_VIEW (hid_file->device_type);
          // We did the bulk of this processing in ReadFile_Detour (...)
          //   nothing to be done here for now.
        }
      }

      return bRet;
    }

    case SK_Input_DeviceFileType::NVIDIA:
    {
      SK_MessageBus_Backend->markRead (2);
    } break;
  }

  return
    GetOverlappedResult_Original (
      hFile, lpOverlapped, lpNumberOfBytesTransferred,
        bWait
    );
}

BOOL
WINAPI
DeviceIoControl_Detour (HANDLE       hDevice,
                        DWORD        dwIoControlCode,
                        LPVOID       lpInBuffer,
                        DWORD        nInBufferSize,
                        LPVOID       lpOutBuffer,
                        DWORD        nOutBufferSize,
                        LPDWORD      lpBytesReturned,
                        LPOVERLAPPED lpOverlapped)
{
  SK_LOG_FIRST_CALL

  const DWORD dwDeviceType =
    DEVICE_TYPE_FROM_CTL_CODE (dwIoControlCode),
        dwFunctionNum =       (dwIoControlCode >> 2) & 0xFFF;

  SK_LOGi2 (
    L"DeviceIoControl %p (Type: %x, Function: %d)", hDevice,
      dwDeviceType, dwFunctionNum
  );

  return
    DeviceIoControl_Original (
      hDevice, dwIoControlCode, lpInBuffer, nInBufferSize,
        lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped
    );
}

//
// We don't actually call any of these, the hooks will be routed
//   through a copy of SetupAPI.dll that SK maintains, instead of
//     one that Valve can render non-functional.
//
static SetupDiGetClassDevsW_pfn
       SetupDiGetClassDevsW_Original             = nullptr;
static SetupDiGetClassDevsA_pfn
       SetupDiGetClassDevsA_Original             = nullptr;
static SetupDiGetClassDevsExW_pfn
       SetupDiGetClassDevsExW_Original           = nullptr;
static SetupDiGetClassDevsExA_pfn
       SetupDiGetClassDevsExA_Original           = nullptr;
static SetupDiEnumDeviceInterfaces_pfn
       SetupDiEnumDeviceInterfaces_Original      = nullptr;
static SetupDiGetDeviceInterfaceDetailW_pfn
       SetupDiGetDeviceInterfaceDetailW_Original = nullptr;
static SetupDiGetDeviceInterfaceDetailA_pfn
       SetupDiGetDeviceInterfaceDetailA_Original = nullptr;
static SetupDiDestroyDeviceInfoList_pfn
       SetupDiDestroyDeviceInfoList_Original     = nullptr;

HDEVINFO
WINAPI
SetupDiGetClassDevsW_Detour (
  _In_opt_ const GUID   *ClassGuid,
  _In_opt_       PCWSTR  Enumerator,
  _In_opt_       HWND    hwndParent,
  _In_           DWORD   Flags )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    SK_LOG_FIRST_CALL
  }

  return
    SK_SetupDiGetClassDevsW (ClassGuid, Enumerator, hwndParent, Flags);
}

HDEVINFO
WINAPI
SetupDiGetClassDevsA_Detour (
  _In_opt_ const GUID  *ClassGuid,
  _In_opt_       PCSTR  Enumerator,
  _In_opt_       HWND   hwndParent,
  _In_           DWORD  Flags )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    SK_LOG_FIRST_CALL
  }

  return
    SK_SetupDiGetClassDevsA (ClassGuid, Enumerator, hwndParent, Flags);
}

HDEVINFO
WINAPI
SetupDiGetClassDevsExW_Detour (
  _In_opt_ CONST GUID    *ClassGuid,
  _In_opt_       PCWSTR   Enumerator,
  _In_opt_       HWND     hwndParent,
  _In_           DWORD    Flags,
  _In_opt_       HDEVINFO DeviceInfoSet,
  _In_opt_       PCWSTR   MachineName,
  _Reserved_     PVOID    Reserved )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    SK_LOG_FIRST_CALL
  }

  return
    SK_SetupDiGetClassDevsExW (
      ClassGuid, Enumerator, hwndParent, Flags,
        DeviceInfoSet, MachineName, Reserved );
}

HDEVINFO
WINAPI
SetupDiGetClassDevsExA_Detour (
  _In_opt_ CONST GUID    *ClassGuid,
  _In_opt_       PCSTR    Enumerator,
  _In_opt_       HWND     hwndParent,
  _In_           DWORD    Flags,
  _In_opt_       HDEVINFO DeviceInfoSet,
  _In_opt_       PCSTR    MachineName,
  _Reserved_     PVOID    Reserved )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    SK_LOG_FIRST_CALL
  }

  return
    SK_SetupDiGetClassDevsExA (
      ClassGuid, Enumerator, hwndParent, Flags,
        DeviceInfoSet, MachineName, Reserved );
}

BOOL
WINAPI
SetupDiEnumDeviceInterfaces_Detour (
  _In_       HDEVINFO                  DeviceInfoSet,
  _In_opt_   PSP_DEVINFO_DATA          DeviceInfoData,
  _In_ CONST GUID                     *InterfaceClassGuid,
  _In_       DWORD                     MemberIndex,
  _Out_      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    SK_LOG_FIRST_CALL
  }

  return
    SK_SetupDiEnumDeviceInterfaces ( DeviceInfoSet, DeviceInfoData,
                                       InterfaceClassGuid, MemberIndex,
                                         DeviceInterfaceData );
}

BOOL
WINAPI
SetupDiGetDeviceInterfaceDetailW_Detour (
  _In_      HDEVINFO                           DeviceInfoSet,
  _In_      PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
  _Out_writes_bytes_to_opt_(DeviceInterfaceDetailDataSize, *RequiredSize)
            PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,
  _In_      DWORD                              DeviceInterfaceDetailDataSize,
  _Out_opt_ _Out_range_(>=, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W))
            PDWORD                             RequiredSize,
  _Out_opt_ PSP_DEVINFO_DATA                   DeviceInfoData )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    SK_LOG_FIRST_CALL
  }

  return
    SK_SetupDiGetDeviceInterfaceDetailW ( DeviceInfoSet,
                                          DeviceInterfaceData,
                                          DeviceInterfaceDetailData,
                                          DeviceInterfaceDetailDataSize,
                                                           RequiredSize,
                                                           DeviceInfoData );
}

BOOL
WINAPI
SetupDiGetDeviceInterfaceDetailA_Detour (
  _In_      HDEVINFO                           DeviceInfoSet,
  _In_      PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
  _Out_writes_bytes_to_opt_(DeviceInterfaceDetailDataSize, *RequiredSize)
            PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,
  _In_      DWORD                              DeviceInterfaceDetailDataSize,
  _Out_opt_ _Out_range_(>=, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A))
            PDWORD                             RequiredSize,
  _Out_opt_ PSP_DEVINFO_DATA                   DeviceInfoData )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    SK_LOG_FIRST_CALL
  }

  return
    SK_SetupDiGetDeviceInterfaceDetailA ( DeviceInfoSet,
                                          DeviceInterfaceData,
                                          DeviceInterfaceDetailData,
                                          DeviceInterfaceDetailDataSize,
                                                           RequiredSize,
                                                           DeviceInfoData );
}

BOOL
WINAPI
SetupDiDestroyDeviceInfoList_Detour (
  _In_ HDEVINFO DeviceInfoSet )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    SK_LOG_FIRST_CALL
  }

  return
    SK_SetupDiDestroyDeviceInfoList (DeviceInfoSet);
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

    if (config.input.gamepad.disable_ps4_hid)
    {
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
        (HidP_GetCaps_pfn)SK_GetProcAddress ( SK_GetModuleHandle (L"HID.DLL"),
                                              "HidP_GetCaps" );
    }

    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "CreateFileA",
                               CreateFileA_Detour,
      static_cast_p2p <void> (&CreateFileA_Original) );

    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "CreateFileW",
                               CreateFileW_Detour,
      static_cast_p2p <void> (&CreateFileW_Original) );

    // This was added in Windows 8, be mindful...
    //   it might not exist on WINE or in compat mode, etc.
    if (SK_GetProcAddress (L"kernel32.dll", "CreateFile2"))
    {
      SK_CreateDLLHook2 (      L"kernel32.dll",
                                "CreateFile2",
                                 CreateFile2_Detour,
        static_cast_p2p <void> (&CreateFile2_Original) );
    }

    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "DeviceIoControl",
                               DeviceIoControl_Detour,
      static_cast_p2p <void> (&DeviceIoControl_Original) );

#if 1
    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "ReadFile",
                               ReadFile_Detour,
      static_cast_p2p <void> (&ReadFile_Original) );

    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "ReadFileEx",
                               ReadFileEx_Detour,
      static_cast_p2p <void> (&ReadFileEx_Original) );
#endif

    // Hooked and then forwarded to the GetOverlappedResultEx hook
    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "GetOverlappedResult",
                               GetOverlappedResult_Detour,
      static_cast_p2p <void> (&GetOverlappedResult_Original) );

    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "GetOverlappedResultEx",
                               GetOverlappedResultEx_Detour,
      static_cast_p2p <void> (&GetOverlappedResultEx_Original) );

    if (ReadAcquire (&__SK_Init) > 0) SK_ApplyQueuedHooks ();

    InterlockedIncrementRelease (&hooked);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}

bool
SK_Input_PreHookHID (void)
{
  static std::filesystem::path path_to_driver_base =
        (std::filesystem::path (SK_GetInstallPath ()) /
                               LR"(Drivers\HID)"),
                                     driver_name =
                  SK_RunLHIfBitness (64, L"HID_SK64.dll",
                                         L"HID_SK32.dll"),
                             path_to_driver = 
                             path_to_driver_base /
                                     driver_name;

  static std::filesystem::path path_to_setupapi_base =
        (std::filesystem::path (SK_GetInstallPath ()) /
                               LR"(Drivers\SetupAPI)"),
                                     setupapi_name =
                  SK_RunLHIfBitness (64, L"SetupAPI_SK64.dll",
                                         L"SetupAPI_SK32.dll"),
                             path_to_setupapi = 
                             path_to_setupapi_base /
                                     setupapi_name;

  static std::filesystem::path path_to_kernel_base =
        (std::filesystem::path (SK_GetInstallPath ()) /
                               LR"(Drivers\Kernel32)"),
                                     kernel_name =
                  SK_RunLHIfBitness (64, L"Kernel32_SK64.dll",
                                         L"Kernel32_SK32.dll"),
                             path_to_kernel = 
                             path_to_kernel_base /
                                     kernel_name;

  static const auto *pSystemDirectory =
    SK_GetSystemDirectory ();

  std::filesystem::path
    path_to_system_hid =
      (std::filesystem::path (pSystemDirectory) / L"hid.dll");

  std::filesystem::path
    path_to_system_kernel =
      (std::filesystem::path (pSystemDirectory) / L"kernel32.dll");

  std::filesystem::path
    path_to_system_setupapi =
      (std::filesystem::path (pSystemDirectory) / L"SetupAPI.dll");

  std::error_code ec =
    std::error_code ();

  if (std::filesystem::exists (path_to_system_hid, ec))
  {
    if ( (! std::filesystem::exists ( path_to_driver,      ec))||
         (! SK_Assert_SameDLLVersion (path_to_driver.    c_str (),
                                      path_to_system_hid.c_str ()) ) )
    { SK_CreateDirectories           (path_to_driver.c_str ());

      if (   std::filesystem::exists (path_to_system_hid,                 ec))
      { std::filesystem::remove      (                    path_to_driver, ec);
        std::filesystem::copy_file   (path_to_system_hid, path_to_driver, ec);
      }
    }
  }

  if (std::filesystem::exists (path_to_system_kernel, ec))
  {
    if ( (! std::filesystem::exists ( path_to_kernel,         ec))||
         (! SK_Assert_SameDLLVersion (path_to_kernel.       c_str (),
                                      path_to_system_kernel.c_str ()) ) )
    { SK_CreateDirectories           (path_to_kernel.c_str ());

      if (   std::filesystem::exists (path_to_system_kernel,                 ec))
      { std::filesystem::remove      (                       path_to_kernel, ec);
        std::filesystem::copy_file   (path_to_system_kernel, path_to_kernel, ec);
      }
    }
  }

  if (std::filesystem::exists (path_to_system_setupapi, ec))
  {
    if ( (! std::filesystem::exists ( path_to_setupapi,         ec))||
         (! SK_Assert_SameDLLVersion (path_to_setupapi.       c_str (),
                                      path_to_system_setupapi.c_str ()) ) )
    { SK_CreateDirectories           (path_to_setupapi.c_str ());

      if (   std::filesystem::exists (path_to_system_setupapi,                   ec))
      { std::filesystem::remove      (                         path_to_setupapi, ec);
        std::filesystem::copy_file   (path_to_system_setupapi, path_to_setupapi, ec);
      }
    }
  }

  HMODULE hModHID =
    SK_LoadLibraryW (path_to_driver.c_str ());

  HMODULE hModKernel32 =
    SK_LoadLibraryW (path_to_kernel.c_str ());

  HMODULE hModSetupAPI =
    SK_LoadLibraryW (path_to_setupapi.c_str ());

  if (! (hModHID && hModKernel32 && hModSetupAPI))
  {
    SK_LOGi0 (L"Missing required HID DLLs (!!)");
  }
                               
  SK_HidD_GetPreparsedData =
    (HidD_GetPreparsedData_pfn)SK_GetProcAddress (hModHID,
    "HidD_GetPreparsedData");

  SK_HidD_FreePreparsedData =
    (HidD_FreePreparsedData_pfn)SK_GetProcAddress (hModHID,
    "HidD_FreePreparsedData");

  SK_HidD_GetFeature =
    (HidD_GetFeature_pfn)SK_GetProcAddress (hModHID,
    "HidD_GetFeature");

  SK_HidP_GetData =
    (HidP_GetData_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetData");

  SK_HidP_GetCaps =
    (HidP_GetCaps_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetCaps");

  SK_HidP_GetButtonCaps =
    (HidP_GetButtonCaps_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetButtonCaps");

  SK_HidP_GetUsages =
    (HidP_GetUsages_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetUsages");

  SK_HidD_GetInputReport =
    (HidD_GetInputReport_pfn)SK_GetProcAddress (hModHID,
    "HidD_GetInputReport");

  SK_CreateFile2 =
    (CreateFile2_pfn)SK_GetProcAddress (hModKernel32,
    "CreateFile2");

  SK_ReadFile =
    (ReadFile_pfn)SK_GetProcAddress (hModKernel32,
    "ReadFile");

  SK_SetupDiGetClassDevsW =
    (SetupDiGetClassDevsW_pfn)SK_GetProcAddress (hModSetupAPI,
    "SetupDiGetClassDevsW");

  SK_SetupDiGetClassDevsA =
    (SetupDiGetClassDevsA_pfn)SK_GetProcAddress (hModSetupAPI,
    "SetupDiGetClassDevsA");

  SK_SetupDiGetClassDevsExW =
    (SetupDiGetClassDevsExW_pfn)SK_GetProcAddress (hModSetupAPI,
    "SetupDiGetClassDevsExW");

  SK_SetupDiGetClassDevsExA =
    (SetupDiGetClassDevsExA_pfn)SK_GetProcAddress (hModSetupAPI,
    "SetupDiGetClassDevsExA");

  SK_SetupDiEnumDeviceInfo =
    (SetupDiEnumDeviceInfo_pfn)SK_GetProcAddress (hModSetupAPI,
    "SetupDiEnumDeviceInfo");

  SK_SetupDiEnumDeviceInterfaces =
    (SetupDiEnumDeviceInterfaces_pfn)SK_GetProcAddress (hModSetupAPI,
    "SetupDiEnumDeviceInterfaces");

  SK_SetupDiGetDeviceInterfaceDetailW =
    (SetupDiGetDeviceInterfaceDetailW_pfn)SK_GetProcAddress (hModSetupAPI,
    "SetupDiGetDeviceInterfaceDetailW");

  SK_SetupDiGetDeviceInterfaceDetailA =
    (SetupDiGetDeviceInterfaceDetailA_pfn)SK_GetProcAddress (hModSetupAPI,
    "SetupDiGetDeviceInterfaceDetailA");

  SK_SetupDiDestroyDeviceInfoList =
    (SetupDiDestroyDeviceInfoList_pfn)SK_GetProcAddress (hModSetupAPI,
    "SetupDiDestroyDeviceInfoList");

  if (config.input.gamepad.steam.disabled_to_game)
  {
    // These causes periodic hitches (thanks Valve), so hook them and
    //   keep Valve's dirty hands off of them.
    SK_RunOnce (
      SK_CreateDLLHook2 (L"SetupAPI.dll", "SetupDiGetClassDevsW",
                                           SetupDiGetClassDevsW_Detour,
                  static_cast_p2p <void> (&SetupDiGetClassDevsW_Original));

      SK_CreateDLLHook2 (L"SetupAPI.dll", "SetupDiGetClassDevsA",
                                           SetupDiGetClassDevsA_Detour,
                  static_cast_p2p <void> (&SetupDiGetClassDevsA_Original));

      SK_CreateDLLHook2 (L"SetupAPI.dll", "SetupDiGetClassDevsExW",
                                           SetupDiGetClassDevsExW_Detour,
                  static_cast_p2p <void> (&SetupDiGetClassDevsExW_Original));

      SK_CreateDLLHook2 (L"SetupAPI.dll", "SetupDiGetClassDevsExA",
                                           SetupDiGetClassDevsExA_Detour,
                  static_cast_p2p <void> (&SetupDiGetClassDevsExA_Original));

      SK_CreateDLLHook2 (L"SetupAPI.dll", "SetupDiEnumDeviceInterfaces",
                                           SetupDiEnumDeviceInterfaces_Detour,
                  static_cast_p2p <void> (&SetupDiEnumDeviceInterfaces_Original));

      SK_CreateDLLHook2 (L"SetupAPI.dll", "SetupDiGetDeviceInterfaceDetailW",
                                           SetupDiGetDeviceInterfaceDetailW_Detour,
                  static_cast_p2p <void> (&SetupDiGetDeviceInterfaceDetailW_Original));

      SK_CreateDLLHook2 (L"SetupAPI.dll", "SetupDiGetDeviceInterfaceDetailA",
                                           SetupDiGetDeviceInterfaceDetailA_Detour,
                  static_cast_p2p <void> (&SetupDiGetDeviceInterfaceDetailA_Original));

      SK_CreateDLLHook2 (L"SetupAPI.dll", "SetupDiDestroyDeviceInfoList",
                                           SetupDiDestroyDeviceInfoList_Detour,
                  static_cast_p2p <void> (&SetupDiDestroyDeviceInfoList_Original));
    );
  }

  if (! config.input.gamepad.hook_hid)
    return false;

  static
    sk_import_test_s tests [] = {
      { "hid.dll", false }
    };

  SK_TestImports (
    SK_GetModuleHandle (nullptr), tests, 1
  );

  if (tests [0].used || SK_GetModuleHandle (L"hid.dll"))
  {
    SK_Input_HookHID ();

    return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////
//
// WinMM (Windows 95 joystick API)
//
//////////////////////////////////////////////////////////////
joyGetPos_pfn   joyGetPos_Original   = nullptr;
joyGetPosEx_pfn joyGetPosEx_Original = nullptr;

MMRESULT
WINAPI
joyGetPos_Detour (_In_  UINT      uJoyID,
                  _Out_ LPJOYINFO pjiUINT)
{
  SK_LOG_FIRST_CALL

  JOYINFO                                    joyInfo = { };
  auto result = joyGetPos_Original (uJoyID, &joyInfo);

  if (! SK_ImGui_WantGamepadCapture ())
  {
    // Pass-through the polled data
    *pjiUINT = joyInfo;

    if (result == JOYERR_NOERROR)
      SK_WinMM_Backend->markRead (sk_input_dev_type::Gamepad);
  }

  else if (result == JOYERR_NOERROR)
    SK_WinMM_Backend->markHidden (sk_input_dev_type::Gamepad);

  return result;
}

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
      JOYINFOEX joyInfo =
        { .dwSize  = sizeof (JOYINFOEX),
          .dwFlags = JOY_RETURNRAWDATA };

      record.lastResult =
        joyGetPosEx_Original (uJoyID, &joyInfo);
      record.dwLastTested = currentTimeInMs;

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

void
SK_Input_HookWinMM (void)
{
  if (! config.input.gamepad.hook_winmm)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    SK_LOG0 ( ( L"Game uses WinMM, installing input hooks..." ),
                L"  Input   " );

    // NOTE: API forwards calls to joyGetPosEx, do not call one from the other.
    SK_CreateDLLHook2 (     L"winmm.DLL",
                             "joyGetPos",
                              joyGetPos_Detour,
     static_cast_p2p <void> (&joyGetPos_Original) );

    SK_CreateDLLHook2 (     L"winmm.DLL",
                             "joyGetPosEx",
                              joyGetPosEx_Detour,
     static_cast_p2p <void> (&joyGetPosEx_Original) );

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

  if (uiNumDevices != 0 && ret == -1 && pDevices != nullptr)
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
NtUserGetRegisteredRawInputDevices_Detour (
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
NtUserRegisterRawInputDevices_Detour (
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

#define _ALLOW_BACKGROUND_GAMEPAD
#define _ALLOW_LEGACY_KEYBOARD

#ifdef _ALLOW_BACKGROUND_GAMEPAD
      if ( pDevices [i].hwndTarget  !=   0                                 &&
           pDevices [i].usUsagePage == HID_USAGE_PAGE_GENERIC              &&
         ( pDevices [i].usUsage     == HID_USAGE_GENERIC_JOYSTICK          ||
           pDevices [i].usUsage     == HID_USAGE_GENERIC_GAMEPAD           ||
           pDevices [i].usUsage     == HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER ) )
      {
        pDevices [i].dwFlags        |= RIDEV_INPUTSINK;
      }
#endif

#ifdef _ALLOW_LEGACY_KEYBOARD
      if (pDevices [i].usUsagePage ==  HID_USAGE_PAGE_GENERIC &&
          pDevices [i].usUsage     ==  HID_USAGE_GENERIC_KEYBOARD)
      {   pDevices [i].dwFlags     &= ~RIDEV_NOLEGACY;
        /*pDevices [i].dwFlags     |=  RIDEV_NOHOTKEYS;*/ }
#endif

      pDevices [i].dwFlags &= ~RIDEV_NOLEGACY;
      pDevices [i].dwFlags &= ~RIDEV_CAPTUREMOUSE;

#ifdef _ALLOW_LEGACY_MOUSE
      if (pDevices [i].usUsagePage ==  HID_USAGE_PAGE_GENERIC &&
          pDevices [i].usUsage     ==  HID_USAGE_GENERIC_MOUSE)
      {if(pDevices [i].dwFlags     &   RIDEV_NOLEGACY)
          pDevices [i].dwFlags     &= ~RIDEV_CAPTUREMOUSE; }
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


          USHORT VKey =
            (((RAWINPUT *)pData)->data.keyboard.VKey & 0xFF);

          static auto* pConsole =
            SK_Console::getInstance ();

        //if (!(((RAWINPUT *) pItem)->data.keyboard.Flags & RI_KEY_BREAK))
        //{
        //  pConsole->KeyDown (VKey & 0xFF, MAXDWORD);
        //        io.KeysDown [VKey & 0xFF] = SK_IsGameWindowActive ();
        //}

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
          if (filter || SK_ImGui_WantGamepadCapture ())
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
NtUserGetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
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


/////////////////////////////////////////////////
//
// ImGui Cursor Management
//
/////////////////////////////////////////////////
sk_imgui_cursor_s SK_ImGui_Cursor;

HCURSOR GetGameCursor (void);

bool
SK_ImGui_IsAnythingHovered (void)
{
  return
    ImGui::IsAnyItemHovered () ||
    ImGui::IsWindowHovered  (
               ImGuiHoveredFlags_AnyWindow                    |
               ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
               ImGuiHoveredFlags_AllowWhenBlockedByPopup
                            );
}

bool
SK_ImGui_IsMouseRelevantEx (void)
{
  bool relevant =
    config.input.mouse.disabled_to_game || SK_ImGui_Active ();

  if (! relevant)
  {
    // SK_ImGui_Active () returns true for the full-blown config UI;
    //   we also have floating widgets that may capture mouse input.
    relevant =
      SK_ImGui_IsAnythingHovered ();
  }

  return relevant;
}

bool
SK_ImGui_IsMouseRelevant (void)
{
  return SK_ImGui_IsMouseRelevantEx ();
}

void
sk_imgui_cursor_s::update (void)
{
}

//__inline
void
sk_imgui_cursor_s::showImGuiCursor (void)
{
}

void
sk_imgui_cursor_s::LocalToScreen (LPPOINT lpPoint)
{
  LocalToClient  (lpPoint);
  ClientToScreen (game_window.child != 0 ? game_window.child : game_window.hWnd, lpPoint);
}

extern ImGuiContext* SK_GImDefaultContext (void);

void
sk_imgui_cursor_s::LocalToClient (LPPOINT lpPoint)
{
  if (! SK_GImDefaultContext ())
    return;

  static const auto& io =
    ImGui::GetIO ();

  RECT                                                                           real_client = { };
  GetClientRect (game_window.child != 0 ? game_window.child : game_window.hWnd, &real_client);

  ImVec2 local_dims =
    io.DisplayFramebufferScale;

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

  static const auto& io =
    ImGui::GetIO ();

  RECT                                                                           real_client = { };
  GetClientRect (game_window.child != 0 ? game_window.child : game_window.hWnd, &real_client);

  const ImVec2 local_dims =
    io.DisplayFramebufferScale;

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
  ScreenToClient (game_window.child != 0 ? game_window.child : game_window.hWnd, lpPoint);
  ClientToLocal  (lpPoint);
}


#include <resource.h>

HCURSOR
ImGui_DesiredCursor (void)
{
  if (! config.input.ui.use_hw_cursor)
    return 0;

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

  extern bool __SK_EnableSetCursor;
  if (        __SK_EnableSetCursor)
  {
    const auto&
        it  = __cursor_cache.find (static_cast <unsigned int> (ImGui::GetMouseCursor ()));
    if (it != __cursor_cache.cend ())
    {
      last_cursor =
             it->second;
      return it->second;
    }

    else
      return last_cursor;
  }

  return
    GetGameCursor ();
}

void
ImGuiCursor_Impl (void)
{
  static auto& io =
    ImGui::GetIO ();

  //
  // Hardware Cursor
  //
  if (config.input.ui.use_hw_cursor)
  {
    io.MouseDrawCursor =
      ( (! SK_ImGui_Cursor.idle) && SK_ImGui_IsMouseRelevant () && (! SK_InputUtil_IsHWCursorVisible ()));
  }

  //
  // Software
  //
  else
  {
    if (SK_ImGui_IsMouseRelevant ())
    {
      SK_SetCursor (0);
    }

    io.MouseDrawCursor = (! SK_ImGui_Cursor.idle) && (! SK_InputUtil_IsHWCursorVisible ());
  }
}

static HCURSOR wait_cursor  = nullptr;
static HCURSOR arrow_cursor = nullptr;

void
sk_imgui_cursor_s::showSystemCursor (bool system)
{
  // Refactoring; function pending removal
  UNREFERENCED_PARAMETER (system);

  static auto& io =
    ImGui::GetIO ();

  if (wait_cursor == nullptr)
    wait_cursor = LoadCursor (nullptr, IDC_WAIT);

  if (arrow_cursor == nullptr)
    arrow_cursor = LoadCursor (nullptr, IDC_ARROW);

  ImGuiCursor_Impl ();
}


void
sk_imgui_cursor_s::activateWindow (bool active)
{
  if (active && config.input.ui.use_hw_cursor)
  {
    if (SK_ImGui_IsAnythingHovered ())//SK_ImGui_IsMouseRelevant ())
    {
      if (SK_ImGui_WantMouseCapture ())
      {
        SK_SetCursor (ImGui_DesiredCursor ());
      }
    }
  }
}



extern BOOL
__stdcall
SK_IsConsoleVisible (void);

bool
SK_ImGui_WantKeyboardCapture (void)
{
  // Allow keyboard input while Steam overlay is active
  if (SK::SteamAPI::GetOverlayState (true))
    return false;

  // Allow keyboard input while ReShade overlay is active
  if (SK_ReShadeAddOn_IsOverlayActive ())
    return false;

  if (! SK_GImDefaultContext ())
    return false;

  bool imgui_capture =
    config.input.keyboard.disabled_to_game == SK_InputEnablement::Disabled;

  static const auto& io =
    ImGui::GetIO ();

  if (SK_IsGameWindowActive () || SK_WantBackgroundRender ())
  {
    if ((nav_usable || io.WantCaptureKeyboard || io.WantTextInput) && (! SK_ImGuiEx_Visible))
      imgui_capture = true;                                        // Don't block keyboard input on popups, or stupid games can miss Alt+F4

    if (SK_IsConsoleVisible ())
      imgui_capture = true;
  }

  if ((! SK_IsGameWindowActive ()) && config.input.keyboard.disabled_to_game == SK_InputEnablement::DisabledInBackground)
    imgui_capture = true;

  return
    imgui_capture;
}

bool
SK_ImGui_WantTextCapture (void)
{
  // Allow keyboard input while Steam overlay is active
  if (SK::SteamAPI::GetOverlayState (true))
    return false;

  // Allow keyboard input while ReShade overlay is active
  if (SK_ReShadeAddOn_IsOverlayActive ())
    return false;

  if (! SK_GImDefaultContext ())
    return false;

  bool imgui_capture =
    config.input.keyboard.disabled_to_game == SK_InputEnablement::Disabled;

  static const auto& io =
    ImGui::GetIO ();

  if (SK_IsGameWindowActive () || SK_WantBackgroundRender ())
  {
    if (io.WantTextInput)
      imgui_capture = true;
  }

  if ((! SK_IsGameWindowActive ()) && config.input.keyboard.disabled_to_game == SK_InputEnablement::DisabledInBackground)
    imgui_capture = true;

  return
    imgui_capture;
}

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

    // Conditionally block Steam Input
    else
    {
      static BOOL        lastCapture = -1;
      if (std::exchange (lastCapture, bCapture) != bCapture)
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

    extern bool
        SK_ImGui_GamepadComboDialogActive;
    if (SK_ImGui_GamepadComboDialogActive)
      imgui_capture = true;
  }

  if ((! SK_IsGameWindowActive ()) && config.input.gamepad.disabled_to_game != SK_InputEnablement::Enabled)
    imgui_capture = true;

  return
    _Return (imgui_capture);
}


static constexpr const DWORD REASON_DISABLED = 0x4;

bool
SK_ImGui_WantMouseCaptureEx (DWORD dwReasonMask)
{
  // Allow mouse input while Steam overlay is active
  if (SK::SteamAPI::GetOverlayState (true))
    return false;

  // Allow mouse input while ReShade overlay is active
  if (SK_ReShadeAddOn_IsOverlayActive ())
    return false;

  if (! SK_GImDefaultContext ())
    return false;

  bool imgui_capture = false;

  if (SK_ImGui_IsMouseRelevantEx ())
  {
    static const auto& io =
      ImGui::GetIO ();

    if (io.WantCaptureMouse || (config.input.ui.capture_mouse && SK_ImGui_Active ()))
      imgui_capture = true;

    else if ((dwReasonMask & REASON_DISABLED) && config.input.mouse.disabled_to_game == SK_InputEnablement::Disabled)
      imgui_capture = true;

    else if (config.input.ui.capture_hidden && (! SK_InputUtil_IsHWCursorVisible ()))
      imgui_capture = true;
  }

  if ((! SK_IsGameWindowActive ()) && config.input.mouse.disabled_to_game == SK_InputEnablement::DisabledInBackground)
    imgui_capture = true;

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
  return SK_GetCursor ();

  static HCURSOR sk_imgui_arrow = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_POINTER);
  static HCURSOR sk_imgui_horz  = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_HORZ);
  static HCURSOR sk_imgui_ibeam = LoadCursor (nullptr, IDC_IBEAM);
  static HCURSOR sys_arrow      = LoadCursor (nullptr, IDC_ARROW);
  static HCURSOR sys_wait       = LoadCursor (nullptr, IDC_WAIT);

  static HCURSOR hCurLast       = nullptr;
         HCURSOR hCur           = SK_GetCursor ();

  if ( hCur != sk_imgui_horz && hCur != sk_imgui_arrow && hCur != sk_imgui_ibeam &&
       hCur != sys_arrow     && hCur != sys_wait )
  {
    hCurLast = hCur;
  }

  return hCurLast;
}

bool
__stdcall
SK_IsGameWindowActive (void)
{
  extern HWND SK_Win32_BackgroundHWND;

  bool bActive =
    game_window.active;

  if (! bActive)
  {
    HWND hWndForeground = SK_GetForegroundWindow ();

    bActive =
      (   game_window.hWnd        == hWndForeground ||
        ( SK_Win32_BackgroundHWND == hWndForeground &&
          SK_Win32_BackgroundHWND != 0 ) );
  }

#if 0
  if (! bActive)
  {
    static auto &rb =
      SK_GetCurrentRenderBackend ();

    if ( rb.windows.device.hwnd   != 0 &&
         rb.windows.device.hwnd   != game_window.hWnd &&
         rb.windows.device.parent != 0 )
    {
      SK_ReleaseAssert (game_window.hWnd == rb.windows.focus.hwnd);

      HWND hWndForeground =
        SK_GetForegroundWindow ();

      bActive =
        hWndForeground == rb.windows.device.hwnd ||
        hWndForeground == rb.windows.device.parent;

      if (bActive && (! game_window.active))
      {
        SetWindowPos ( hWndForeground, game_window.hWnd,
                         0, 0,
                         0, 0,
                           SWP_NOMOVE | SWP_NOSIZE |
                           SWP_ASYNCWINDOWPOS );
      }
    }
  }
#endif

  // Background Window (Aspect Ratio Stretch) is Foreground...
  //   we don't want that, make the GAME the foreground.
  if (bActive && (! game_window.active))
  {
    // This only activates the window if performed on the same thread as the
    //   game's window, so don't do this if called from a different thread.
    if ( SK_GetForegroundWindow () == SK_Win32_BackgroundHWND &&
                                 0 != SK_Win32_BackgroundHWND &&
             GetCurrentThreadId () == GetWindowThreadProcessId (game_window.hWnd, nullptr) )
    {
      game_window.active = true;

      BringWindowToTop    (game_window.hWnd);
      SetWindowPos        ( SK_Win32_BackgroundHWND, game_window.hWnd,
                                  0, 0,
                                  0, 0,
                                    SWP_NOMOVE     | SWP_NOSIZE |
                             SWP_NOACTIVATE );
      SetForegroundWindow (game_window.hWnd);
      SetFocus            (game_window.hWnd);
    }
  }

  return bActive;
}

bool
__stdcall
SK_IsGameWindowFocused (void)
{
  auto
    hWndAtCenter = [&](void)
 -> HWND
    {
      return
        WindowFromPoint (
                  POINT {                     game_window.actual.window.left +
          (game_window.actual.window.right  - game_window.actual.window.left) / 2,
                                              game_window.actual.window.top  +
          (game_window.actual.window.bottom - game_window.actual.window.top)  / 2
                        }
                        );
    };

  return (
    SK_IsGameWindowActive () && (SK_GetFocus () == game_window.hWnd ||
                                hWndAtCenter () == game_window.hWnd )/*|| SK_GetForegroundWindow () == game_window.hWnd*/
  );
}

void
ImGui_ToggleCursor (void)
{
  static auto& io =
    ImGui::GetIO ();

  if (! SK_ImGui_Cursor.visible)
  {
    POINT                           pos = { };
    SK_GetCursorPos               (&pos);
    SK_ImGui_Cursor.ScreenToLocal (&pos);

    // Save original cursor position
    SK_ImGui_Cursor.orig_pos =      pos;
    SK_ImGui_Cursor.idle     =    false;

    io.WantCaptureMouse      =     true;

    SK_ImGui_CenterCursorOnWindow ();
  }

  else
  {
    SK_ImGui_Cursor.idle = true;

    if (SK_ImGui_WantMouseCapture ())
    {
      POINT                            screen =
       SK_ImGui_Cursor.orig_pos;
       SK_ImGui_Cursor.LocalToScreen (&screen);
       SK_SetCursorPos               ( screen.x,
                                       screen.y );
    }
  }

  SK_ImGui_Cursor.visible = (! SK_ImGui_Cursor.visible);
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
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
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
SK_GetCursor (VOID)
{
  if (GetCursor_Original != nullptr)
    return GetCursor_Original ();

  return GetCursor ();
}

bool __SK_EnableSetCursor = false;

HCURSOR
WINAPI
SK_SetCursor (
  _In_opt_ HCURSOR hCursor)
{
  HCURSOR hCursorRet = SK_GetCursor ();

  if (__SK_EnableSetCursor)
  {
    if (SetCursor_Original       != nullptr )
    {   SetCursor_Original       (  hCursor );
        SK_ImGui_Cursor.real_img  = hCursor; }
    else
    {   SK_ImGui_Cursor.real_img  = hCursor ;
        SetCursor                  (hCursor); }
  }

  return hCursorRet;
}

HCURSOR
WINAPI
SetCursor_Detour (
  _In_opt_ HCURSOR hCursor )
{
  SK_LOG_FIRST_CALL

  if ((SK_ImGui_WantMouseCapture () && SK_ImGui_IsAnythingHovered ()) || ImGui::GetIO ().WantCaptureMouse)
  {
    if (! config.input.ui.use_hw_cursor)
      return 0;

    hCursor =
      ImGui_DesiredCursor ();
  }

  return
    SetCursor_Original (hCursor);
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
      GetCursor_Original ();
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

  POINT pt  = pci->ptScreenPos;
  BOOL  ret = SK_GetCursorInfo (pci);

  struct state_backup
  {
    explicit state_backup (PCURSORINFO pci) :
      hCursor     (pci->hCursor),
      ptScreenPos (pci->ptScreenPos) { };

    HCURSOR hCursor;
    POINT   ptScreenPos;
  } actual (pci);

  pci->ptScreenPos = pt;


  bool bMouseRelevant =
    SK_ImGui_IsMouseRelevant ();

  if (bMouseRelevant)
    pci->hCursor = ImGui_DesiredCursor ();


  if (ret && bMouseRelevant)
  {
    bool implicit_capture = false;

    // Depending on warp prefs, we may not allow the game to know about mouse movement
    //   (even if ImGui doesn't want mouse capture)
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
         ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
      implicit_capture = true;

    if (SK_ImGui_WantMouseCapture () || implicit_capture)
    {
      pci->ptScreenPos = SK_ImGui_Cursor.orig_pos;
    }

    else
    {
      pci->ptScreenPos = SK_ImGui_Cursor.pos;
    }

    SK_ImGui_Cursor.LocalToScreen (&pci->ptScreenPos);

    return TRUE;
  }

  pci->hCursor     =
             SK_ImGui_IsMouseRelevant () ?
                ImGui_DesiredCursor   () :
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

  if (SK_WantBackgroundRender () && (! SK_IsGameWindowActive ()))
  {
    SK_Win32_Backend->markHidden (sk_win32_func::GetCursorPos);

    *lpPoint = SK_ImGui_Cursor.orig_pos;

    SK_ImGui_Cursor.LocalToScreen (lpPoint);

    return TRUE;
  }


  if (SK_ImGui_IsMouseRelevant ())
  {
    bool implicit_capture = false;

    // Depending on warp prefs, we may not allow the game to know about mouse movement
    //   (even if ImGui doesn't want mouse capture)
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
         ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
    {
      implicit_capture = true;
    }

    if (SK_ImGui_WantMouseCapture () || implicit_capture)
    {
      SK_Win32_Backend->markHidden (sk_win32_func::GetCursorPos);

      *lpPoint = SK_ImGui_Cursor.orig_pos;

      SK_ImGui_Cursor.LocalToScreen (lpPoint);

      return TRUE;
    }
  }


  BOOL bRet =
    SK_GetCursorPos (lpPoint);

  if (bRet)
  {
    SK_Win32_Backend->markRead (sk_win32_func::GetCursorPos);

    auto pos = *lpPoint;

    SK_ImGui_Cursor.ScreenToLocal (&pos);
    SK_ImGui_Cursor.pos           = pos;
  }



  static bool bStarOcean4 =
    (SK_GetCurrentGameID () == SK_GAME_ID::StarOcean4);

  if (bStarOcean4 && GetActiveWindow () == SK_GetGameWindow ())
  {
    static const auto& io =
      ImGui::GetIO ();

    static ULONG64 last_frame = SK_GetFramesDrawn ();
    static POINT   last_pos   = *lpPoint;

    POINT new_pos = *lpPoint;

    new_pos.x = (LONG)(io.DisplayFramebufferScale.x * 0.5f);
    new_pos.y = (LONG)(io.DisplayFramebufferScale.y * 0.5f);

    float ndc_x = 2.0f * (((float)(lpPoint->x - (float)last_pos.x) + (float)new_pos.x) / io.DisplayFramebufferScale.x) - 1.0f;
    float ndc_y = 2.0f * (((float)(lpPoint->y - (float)last_pos.y) + (float)new_pos.y) / io.DisplayFramebufferScale.y) - 1.0f;

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
    if (new_frame && ( (calls++ % 8) == 0 || ( SK_GetCursorPos (&get_pos) && (get_pos.x <= 4L || (float)get_pos.x >= (io.DisplayFramebufferScale.x - 4.0f) ||
                                                                              get_pos.y <= 4L || (float)get_pos.y >= (io.DisplayFramebufferScale.y - 4.0f)) ) ))
    {
      last_pos =       new_pos;
      SK_SetCursorPos (new_pos.x, new_pos.y);
      *lpPoint     = { new_pos.x, new_pos.y };
    }
  }


  return bRet;
}

BOOL
WINAPI
GetPhysicalCursorPos_Detour (LPPOINT lpPoint)
{
  SK_LOG_FIRST_CALL

  POINT                     pt;
  if (GetCursorPos_Detour (&pt))
  {
    if (LogicalToPhysicalPoint (0, &pt))
    {
      *lpPoint = pt;

      return TRUE;
    }
  }

  return FALSE;
}

DWORD
WINAPI
GetMessagePos_Detour (void)
{
  SK_LOG_FIRST_CALL

  static DWORD dwLastPos =
           GetMessagePos_Original ();

  bool implicit_capture = false;

  // Depending on warp prefs, we may not allow the game to know about mouse movement
  //   (even if ImGui doesn't want mouse capture)
  if ( SK_ImGui_IsMouseRelevant () && ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
                                      ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
  {
    implicit_capture = true;
  }

  // TODO: Use the message time to determine whether to fake this or not
  if (SK_ImGui_WantMouseCapture () || implicit_capture)
  {
    return
      dwLastPos;
  }

  dwLastPos =
    GetMessagePos_Original ();

  return
    dwLastPos;
}

BOOL
WINAPI
SetCursorPos_Detour (_In_ int x, _In_ int y)
{
  SK_LOG_FIRST_CALL

  // Game WANTED to change its position, so remember that.
  POINT                           pt { x, y };
  SK_ImGui_Cursor.ScreenToLocal (&pt);
  SK_ImGui_Cursor.orig_pos =      pt;

  // Don't let the game continue moving the cursor while
  //   Alt+Tabbed out
  if (SK_WantBackgroundRender () && (! SK_IsGameWindowActive ()))
  {
    return TRUE;
  }

  // Prevent Mouse Look while Drag Locked
  if (config.window.drag_lock)
    return TRUE;

  if ( SK_ImGui_IsMouseRelevant () && ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
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

BOOL
WINAPI
SetPhysicalCursorPos_Detour (_In_ int x, _In_ int y)
{
  SK_LOG_FIRST_CALL

  POINT                             pt = { x, y };
  if (! PhysicalToLogicalPoint (0, &pt))
    return FALSE;

  return
    SetCursorPos_Detour (x, y);
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

  return
    SK_SendInput (nInputs, pInputs, cbSize);
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

void
WINAPI
SK_keybd_event (
  _In_ BYTE       bVk,
  _In_ BYTE       bScan,
  _In_ DWORD     dwFlags,
  _In_ ULONG_PTR dwExtraInfo )
{
  ( keybd_event_Original != nullptr )                         ?
    keybd_event_Original ( bVk, bScan, dwFlags, dwExtraInfo ) :
    keybd_event          ( bVk, bScan, dwFlags, dwExtraInfo ) ;
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
  auto& rb =
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
NtUserGetAsyncKeyState_Detour (_In_ int vKey)
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
NtUserGetKeyState_Detour (_In_ int vKey)
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
NtUserGetKeyboardState_Detour (PBYTE lpKeyState)
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


LRESULT
WINAPI
ImGui_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  struct SK_MouseTimer {
    POINTS  pos      = {     }; // POINT (Short) - Not POINT plural ;)
    DWORD   sampled  =     0UL;
    bool    cursor   =    true;

    int     init     =   FALSE;
    UINT    timer_id = 0x68993;
    HCURSOR
        class_cursor =       0;
  } static last_mouse;

  //extern bool IsControllerPluggedIn (INT iJoyID);

bool
SK_Window_IsCursorActive (void)
{
  return
    last_mouse.cursor;
}

bool
SK_Window_ActivateCursor (bool changed = false)
{
  const bool was_active = last_mouse.cursor;

  if (! was_active)
  {
    if ((! SK_IsSteamOverlayActive ()))
    {
      if (config.input.ui.use_hw_cursor)
      {
        // This would start a war with the Epic Overlay if we didn't add an escape
        int recursion = 4;

        if ( 0 != SK_GetSystemMetrics (SM_MOUSEPRESENT) )
          while ( recursion > 0 && SK_ShowCursor (TRUE) < 0 ) --recursion;

        // Deliberately call SetCursor's _hooked_ function, so we can determine whether to
        //   activate the window using the game's cursor or our override
        SetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR, (LONG_PTR)last_mouse.class_cursor);
        SK_SetCursor                                               (last_mouse.class_cursor);
      }

      else
      {
        // Deliberately call SetCursor's _hooked_ function, so we can determine whether to
        //   activate the window using the game's cursor or our override
        SetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR, (LONG_PTR)0);
        SK_SetCursor                                               (0);
      }

      last_mouse.cursor = true;
    }
  }

  if (changed && (! SK_IsSteamOverlayActive ()))
    last_mouse.sampled = SK::ControlPanel::current_time;

  return (last_mouse.cursor != was_active);
};

bool
SK_Window_DeactivateCursor (bool ignore_imgui = false)
{
  if (! ignore_imgui)
  {
    if ( SK_ImGui_WantMouseCaptureEx (0xFFFFFFFF & ~REASON_DISABLED) ||
         (! last_mouse.cursor) )
    {
      last_mouse.sampled = SK::ControlPanel::current_time;

      return false;
    }
  }

  bool was_active = last_mouse.cursor;

  if (ignore_imgui || last_mouse.sampled < SK::ControlPanel::current_time - config.input.cursor.timeout)
  {
    if ((! SK_IsSteamOverlayActive ()))
    {
      if (was_active)
      {
        HCURSOR cursor = //SK_GetCursor ();
          (HCURSOR)GetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR);

        if (cursor != 0)
          last_mouse.class_cursor = cursor;
      }

      SetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR, 0);
      SK_SetCursor     (0);

      int recursion = 4;

      if ( 0 != SK_GetSystemMetrics (SM_MOUSEPRESENT) )
        while ( recursion > 0 && SK_ShowCursor (FALSE) > -1 ) --recursion;

      last_mouse.cursor  = false;
      last_mouse.sampled = SK::ControlPanel::current_time;
    }
  }

  return (last_mouse.cursor != was_active);
};

void
SK_ImGui_UpdateMouseTracker (void)
{
  bool bWasInside =
    game_window.mouse.inside;

  game_window.mouse.last_move_msg = SK::ControlPanel::current_time;
  game_window.mouse.inside        = true;

  if (game_window.hWnd == 0 || game_window.top != 0)
    return;
  
  if ((! (bWasInside && game_window.mouse.tracking && game_window.mouse.can_track)) && game_window.hWnd != 0)
  {
    TRACKMOUSEEVENT tme = { .cbSize      = sizeof (TRACKMOUSEEVENT),
                            .dwFlags     = TME_LEAVE,
                            .hwndTrack   = game_window.hWnd,
                            .dwHoverTime = HOVER_DEFAULT };
  
    if (TrackMouseEvent (&tme))
    {
      game_window.mouse.can_track = true;
      game_window.mouse.tracking  = true;
    }
  }
}

bool
SK_Input_DetermineMouseIdleState (MSG* lpMsg)
{
  static DWORD dwLastSampled = DWORD_MAX;

  static HWND hWndTimer  = 0;
  if (        hWndTimer != game_window.hWnd )
  {
    if (IsWindow (hWndTimer))
      KillTimer ( hWndTimer, sk::narrow_cast <UINT_PTR> (last_mouse.timer_id) );

    UINT _timeout =           config.input.cursor.timeout != 0 ?
      sk::narrow_cast <UINT> (config.input.cursor.timeout) / 2 :
                                                          250UL;

    // This was being installed on the wrong HWND, consider a dedicated timer
    //   in the future.
    SetTimer ( game_window.hWnd,
                 sk::narrow_cast <UINT_PTR> (last_mouse.timer_id),
                   _timeout, nullptr );

    hWndTimer = game_window.hWnd;
  }


  bool activation_event =
    ( lpMsg->message == WM_MOUSEMOVE /*|| lpMsg->message == WM_SETCURSOR*/) && (!SK_IsSteamOverlayActive ());

  bool bCapturingMouse =
    SK_ImGui_WantMouseCaptureEx (0xFFFFFFFF & ~REASON_DISABLED);

  // Don't blindly accept that WM_MOUSEMOVE actually means the mouse moved...
  if (! bCapturingMouse)
  {
    if (activation_event)
    {
      static constexpr const short threshold = 3;

      // Filter out small movements
      if ( abs (last_mouse.pos.x - SK_ImGui_Cursor.pos.x) < threshold &&
           abs (last_mouse.pos.y - SK_ImGui_Cursor.pos.y) < threshold )
      {
        activation_event = false;
      }
    }
  }

  if (config.input.cursor.keys_activate)
    activation_event |= ( lpMsg->message >= WM_KEYFIRST &&
                          lpMsg->message <= WM_KEYLAST );

  // If timeout is 0, just hide the thing indefinitely
  if (activation_event)
  {
    if ( (config.input.cursor.manage && config.input.cursor.timeout != 0) ||
            bCapturingMouse )
    {
      SK_Window_ActivateCursor (true);
    }
  }

  else if ( lpMsg->message == WM_TIMER            &&
            lpMsg->wParam  == last_mouse.timer_id &&
            (! SK_IsSteamOverlayActive ()) )
  {
    // If for some reason we get the event multiple times in a frame,
    //   use the results of the first test only
    if (std::exchange (dwLastSampled, SK::ControlPanel::current_time) !=
                                      SK::ControlPanel::current_time)
    {
      if (! bCapturingMouse)
      {
        if (config.input.cursor.manage)
        {
          static constexpr const short threshold = 3;

          if (config.input.cursor.timeout == 0)
          {
            SK_Window_DeactivateCursor ();
          }

          // Filter out small movements
          else if (abs (last_mouse.pos.x - SK_ImGui_Cursor.pos.x) < threshold &&
                   abs (last_mouse.pos.y - SK_ImGui_Cursor.pos.y) < threshold)
          {
            SK_Window_DeactivateCursor ();
          }
        }
      }

      else
        SK_Window_ActivateCursor (true);

      // Record the cursor pos (at time of timer fire)
      last_mouse.pos =
      { (SHORT)SK_ImGui_Cursor.pos.x,
        (SHORT)SK_ImGui_Cursor.pos.y };
    }

    return true;
  }

  return false;
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
      extern bool __SKX_WinHook_InstallInputHooks (HWND hWnd);
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
      } break;

      case WM_KEYUP:
      case WM_SYSKEYUP:
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


using SetWindowsHookEx_pfn = HHOOK (WINAPI*)(int, HOOKPROC, HINSTANCE, DWORD);
      SetWindowsHookEx_pfn SetWindowsHookExA_Original = nullptr;
      SetWindowsHookEx_pfn SetWindowsHookExW_Original = nullptr;

using UnhookWindowsHookEx_pfn = BOOL (WINAPI *)(HHOOK);
      UnhookWindowsHookEx_pfn UnhookWindowsHookEx_Original = nullptr;


class SK_Win32_WindowHookManager {
public:
std::map <
  DWORD, HOOKPROC > _RealMouseProcs;
         HOOKPROC   _RealMouseProc    = nullptr;
std::map <
  DWORD, HHOOK >    _RealMouseHooks;
         HHOOK      _RealMouseHook    = nullptr;

std::map <
  DWORD, HOOKPROC > _RealKeyboardProcs;
         HOOKPROC   _RealKeyboardProc = nullptr;
std::map <
  DWORD, HHOOK >    _RealKeyboardHooks;
         HHOOK      _RealKeyboardHook = nullptr;
} __hooks;

static POINTS last_pos;

LRESULT
CALLBACK
SK_Proxy_MouseProc   (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  if (nCode >= 0)
  {
    switch (wParam)
    {
      case WM_MOUSEMOVE:
      case WM_LBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDOWN:
      case WM_MBUTTONDBLCLK:
      case WM_XBUTTONDOWN:
      case WM_XBUTTONDBLCLK:
      {
        MOUSEHOOKSTRUCT *mhs =
          (MOUSEHOOKSTRUCT *)lParam;

        static auto& io =
          ImGui::GetIO ();

        io.KeyCtrl  |= ((mhs->dwExtraInfo & MK_CONTROL) != 0);
        io.KeyShift |= ((mhs->dwExtraInfo & MK_SHIFT  ) != 0);

        switch (wParam)
        {
          case WM_MOUSEMOVE:
          {
            // No TrackMouseEvent available, gotta do this manually
            if (! game_window.mouse.can_track)
            {
              POINT                                          pt (mhs->pt);
              ScreenToClient             (game_window.child != nullptr ?
                                          game_window.child            :
                                          game_window.hWnd, &pt);
              if (ChildWindowFromPointEx (game_window.child != nullptr ?
                                          game_window.child            :
                                          game_window.hWnd,  pt, CWP_SKIPDISABLED) == (game_window.child != nullptr ?
                                                                                       game_window.child            :
                                                                                       game_window.hWnd))
              {
                SK_ImGui_Cursor.ClientToLocal (&pt);
                SK_ImGui_Cursor.pos =           pt;

                io.MousePos.x = (float)SK_ImGui_Cursor.pos.x;
                io.MousePos.y = (float)SK_ImGui_Cursor.pos.y;
              }

              else
                io.MousePos = ImVec2 (-FLT_MAX, -FLT_MAX);
            }

            // Install a mouse tracker to get WM_MOUSELEAVE
            if (! (game_window.mouse.tracking && game_window.mouse.inside))
            {
              if (SK_ImGui_WantMouseCapture ())
              {
                SK_ImGui_UpdateMouseTracker ();
              }
            }
          } break;

          case WM_LBUTTONDOWN:
          case WM_LBUTTONDBLCLK:
            io.MouseDown [0] = true;
            break;

          case WM_RBUTTONDOWN:
          case WM_RBUTTONDBLCLK:
            io.MouseDown [1] = true;
            break;

          case WM_MBUTTONDOWN:
          case WM_MBUTTONDBLCLK:
            io.MouseDown [2] = true;
            break;

          case WM_XBUTTONDOWN:
          case WM_XBUTTONDBLCLK:
          {
            MOUSEHOOKSTRUCTEX* mhsx =
              (MOUSEHOOKSTRUCTEX*)lParam;

            if ((HIWORD (mhsx->mouseData)) == XBUTTON1) io.MouseDown [3] = true;
            if ((HIWORD (mhsx->mouseData)) == XBUTTON2) io.MouseDown [4] = true;
          } break;
        }
      } break;
    }

    if (SK_ImGui_WantMouseCapture ())
    {
      switch (wParam)
      {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
          SK_WinHook_Backend->markHidden (sk_input_dev_type::Mouse);

          return
            CallNextHookEx (
                nullptr, nCode,
                 wParam, lParam );
      }
    }

    else
    {
      // Game uses a mouse hook for input that the Steam overlay cannot block
      if (SK_GetStoreOverlayState (true))
      {
        SK_WinHook_Backend->markHidden (sk_input_dev_type::Mouse);

        return
          CallNextHookEx (0, nCode, wParam, lParam);
      }

      SK_WinHook_Backend->markRead (sk_input_dev_type::Mouse);

      DWORD dwTid =
        GetCurrentThreadId ();

      using MouseProc =
        LRESULT (CALLBACK *)(int,WPARAM,LPARAM);

      return
        ((MouseProc)__hooks._RealMouseProcs.count (dwTid) ?
                    __hooks._RealMouseProcs.at    (dwTid) :
                    __hooks._RealMouseProc)( nCode, wParam,
                                                    lParam );
    }
  }

  return
    CallNextHookEx (
        nullptr, nCode,
         wParam, lParam );
}

LRESULT
CALLBACK
SK_Proxy_LLMouseProc   (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  if (nCode >= 0)
  {
    switch (wParam)
    {
      case WM_MOUSEMOVE:
      case WM_LBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDOWN:
      case WM_MBUTTONDBLCLK:
      case WM_XBUTTONDOWN:
      case WM_XBUTTONDBLCLK:
      {
        MSLLHOOKSTRUCT *mhs =
          (MSLLHOOKSTRUCT *)lParam;

        static auto& io =
          ImGui::GetIO ();

        io.KeyCtrl  |= ((mhs->dwExtraInfo & MK_CONTROL) != 0);
        io.KeyShift |= ((mhs->dwExtraInfo & MK_SHIFT  ) != 0);

        switch (wParam)
        {
          case WM_LBUTTONDOWN:
          case WM_LBUTTONDBLCLK:
            io.MouseDown [0] = true;
            break;

          case WM_RBUTTONDOWN:
          case WM_RBUTTONDBLCLK:
            io.MouseDown [1] = true;
            break;

          case WM_MBUTTONDOWN:
          case WM_MBUTTONDBLCLK:
            io.MouseDown [2] = true;
            break;

          case WM_XBUTTONDOWN:
          case WM_XBUTTONDBLCLK:
            if ((HIWORD (mhs->mouseData)) == XBUTTON1) io.MouseDown [3] = true;
            if ((HIWORD (mhs->mouseData)) == XBUTTON2) io.MouseDown [4] = true;
            break;
        }
      } break;
    }

    if (SK_ImGui_WantMouseCapture ())
    {
      switch (wParam)
      {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
          SK_WinHook_Backend->markHidden (sk_input_dev_type::Mouse);

          return
            CallNextHookEx (
                nullptr, nCode,
                 wParam, lParam );
      }
    }

    else
    {
      // Game uses a mouse hook for input that the Steam overlay cannot block
      if (SK_GetStoreOverlayState (true))
      {
        SK_WinHook_Backend->markHidden (sk_input_dev_type::Mouse);

        return
          CallNextHookEx (0, nCode, wParam, lParam);
      }

      SK_WinHook_Backend->markRead (sk_input_dev_type::Mouse);

      DWORD dwTid =
        GetCurrentThreadId ();

      using MouseProc =
        LRESULT (CALLBACK *)(int,WPARAM,LPARAM);

      return
        ((MouseProc)__hooks._RealMouseProcs.count (dwTid) ?
                    __hooks._RealMouseProcs.at    (dwTid) :
                    __hooks._RealMouseProc)( nCode, wParam,
                                                    lParam );
    }
  }

  return
    CallNextHookEx (
        nullptr, nCode,
         wParam, lParam );
}


LRESULT
CALLBACK
SK_Proxy_KeyboardProc (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam  )
{
  if (nCode >= 0)
  {
    using KeyboardProc =
      LRESULT (CALLBACK *)(int,WPARAM,LPARAM);

    bool wasPressed = (((DWORD)lParam) & (1UL << 30UL)) != 0UL,
          isPressed = (((DWORD)lParam) & (1UL << 31UL)) == 0UL,
          isAltDown = (((DWORD)lParam) & (1UL << 29UL)) != 0UL;

    SHORT vKey =
      static_cast <SHORT> (wParam);

    if ( config.input.keyboard.override_alt_f4 &&
            config.input.keyboard.catch_alt_f4 )
    {
      if (SK_IsGameWindowFocused () && vKey == VK_F4 && isAltDown && isPressed && (! wasPressed))
      {
        extern bool SK_ImGui_WantExit;
                    SK_ImGui_WantExit = true;
        return 1;
      }
    }

    if (SK_IsGameWindowActive () || (! isPressed))
      ImGui::GetIO ().KeysDown [vKey] = isPressed;

    if (SK_ImGui_WantKeyboardCapture ())
    {
      SK_WinHook_Backend->markHidden (sk_input_dev_type::Keyboard);

      return
        CallNextHookEx (
            nullptr, nCode,
             wParam, lParam );
    }

    else
    {
      // Game uses a keyboard hook for input that the Steam overlay cannot block
      if (SK_GetStoreOverlayState (true) || SK_Console::getInstance ()->isVisible ())
      {
        SK_WinHook_Backend->markHidden (sk_input_dev_type::Keyboard);

        return
          CallNextHookEx (0, nCode, wParam, lParam);
      }

      DWORD dwTid =
        GetCurrentThreadId ();

      SK_WinHook_Backend->markRead (sk_input_dev_type::Keyboard);

      return
        ((KeyboardProc)__hooks._RealKeyboardProcs.count (dwTid) ?
                       __hooks._RealKeyboardProcs.at    (dwTid) :
                       __hooks._RealKeyboardProc)( nCode, wParam,
                                                          lParam );
    }
  }

  return
    CallNextHookEx (
        nullptr, nCode,
         wParam, lParam );
}

LRESULT
CALLBACK
SK_Proxy_LLKeyboardProc (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam  )
{
  if (nCode == HC_ACTION)
  {
    using KeyboardProc =
      LRESULT (CALLBACK *)(int,WPARAM,LPARAM);

    KBDLLHOOKSTRUCT *pHookData =
      (KBDLLHOOKSTRUCT *)lParam;

    bool wasPressed = false,
          isPressed = (wParam == WM_KEYDOWN    || wParam == WM_SYSKEYDOWN),
          isAltDown = (wParam == WM_SYSKEYDOWN || wParam == WM_SYSKEYUP);

    wasPressed = isPressed ^ (bool)((pHookData->flags & 0x7) != 0);

    SHORT vKey =
      static_cast <SHORT> (wParam);

    if ( config.input.keyboard.override_alt_f4 &&
            config.input.keyboard.catch_alt_f4 )
    {
      if (SK_IsGameWindowFocused () && vKey == VK_F4 && isAltDown && isPressed && (! wasPressed))
      {
        extern bool SK_ImGui_WantExit;
                    SK_ImGui_WantExit = true;
        return 1;
      }
    }

    if (SK_IsGameWindowActive () || (! isPressed))
      ImGui::GetIO ().KeysDown [vKey] = isPressed;

    if (SK_ImGui_WantKeyboardCapture ())
    {
      SK_WinHook_Backend->markHidden (sk_input_dev_type::Keyboard);

      return
        CallNextHookEx (
            nullptr, nCode,
             wParam, lParam );
    }

    else
    {
      // Game uses a keyboard hook for input that the Steam overlay cannot block
      if (SK_GetStoreOverlayState (true) || SK_Console::getInstance ()->isVisible ())
      {
        SK_WinHook_Backend->markHidden (sk_input_dev_type::Keyboard);

        return
          CallNextHookEx (0, nCode, wParam, lParam);
      }

      DWORD dwTid =
        GetCurrentThreadId ();

      SK_WinHook_Backend->markRead (sk_input_dev_type::Keyboard);

      return
        ((KeyboardProc)__hooks._RealKeyboardProcs.count (dwTid) ?
                       __hooks._RealKeyboardProcs.at    (dwTid) :
                       __hooks._RealKeyboardProc)( nCode, wParam,
                                                          lParam );
    }
  }

  return
    CallNextHookEx (
        nullptr, nCode,
         wParam, lParam );
}

BOOL
WINAPI
UnhookWindowsHookEx_Detour ( _In_ HHOOK hhk )
{
  for ( auto& hook : __hooks._RealMouseHooks )
  {
    if (hook.second == hhk)
    {
      __hooks._RealMouseHooks.erase (hook.first);
      __hooks._RealMouseProcs.erase (hook.first);

      return
        UnhookWindowsHookEx_Original (hhk);
    }
  }

  if (hhk == __hooks._RealMouseHook)
  {
    __hooks._RealMouseProc = nullptr;
    __hooks._RealMouseHook = nullptr;

    return
      UnhookWindowsHookEx_Original (hhk);
  }

  for ( auto& hook : __hooks._RealKeyboardHooks )
  {
    if (hook.second == hhk)
    {
      __hooks._RealKeyboardHooks.erase (hook.first);
      __hooks._RealKeyboardProcs.erase (hook.first);

      return
        UnhookWindowsHookEx_Original (hhk);
    }
  }

  if (hhk == __hooks._RealKeyboardHook)
  {
    __hooks._RealKeyboardProc = nullptr;
    __hooks._RealKeyboardHook = nullptr;

    return
      UnhookWindowsHookEx_Original (hhk);
  }

  for ( auto& hook : __hooks._RealKeyboardHooks )
  {
    if (hook.second == hhk)
    {
      __hooks._RealKeyboardProcs.erase (hook.first);

      return
        UnhookWindowsHookEx_Original (hhk);
    }
  }

  return
    UnhookWindowsHookEx_Original (hhk);
}

HHOOK
WINAPI
SetWindowsHookExW_Detour (
  int       idHook,
  HOOKPROC  lpfn,
  HINSTANCE hmod,
  DWORD     dwThreadId )
{
  wchar_t                   wszHookMod [MAX_PATH] = { };
  GetModuleFileNameW (hmod, wszHookMod, MAX_PATH);

  switch (idHook)
  {
    case WH_KEYBOARD:
    case WH_KEYBOARD_LL:
    {
      SK_LOG0 ( ( L" <Unicode>: Game module ( %ws ) uses a%wsKeyboard Hook...",
                       wszHookMod,
                        idHook == WH_KEYBOARD_LL ?
                                   L" Low-Level " : L" " ),
                                           L"Input Hook" );

      // Game seems to be using keyboard hooks instead of a normal Window Proc;
      //   that makes life more complicated for SK/ImGui... but we got this!
      if (idHook == WH_KEYBOARD || idHook == WH_KEYBOARD_LL)
      {
        bool install = false;

        if (dwThreadId != 0)
        {
          if (! __hooks._RealKeyboardProcs.count (dwThreadId))
          {     __hooks._RealKeyboardProcs       [dwThreadId] = lpfn;
                                                      install = true;
          }
        }

        else if (__hooks._RealKeyboardProc == nullptr)
        {        __hooks._RealKeyboardProc = lpfn;
                                   install = true;
        }

        if (install)
          lpfn = (idHook == WH_KEYBOARD ? SK_Proxy_KeyboardProc
                                        : SK_Proxy_LLKeyboardProc);
      }
    } break;

    case WH_MOUSE:
    case WH_MOUSE_LL:
    {
      SK_LOG0 ( ( L" <Unicode>: Game module ( %ws ) uses a%wsMouse Hook...",
                 wszHookMod,
                  idHook == WH_MOUSE_LL    ?
                            L" Low-Level " : L" " ),
                                    L"Input Hook" );

      // Game seems to be using mouse hooks instead of a normal Window Proc;
      //   that makes life more complicated for SK/ImGui... but we got this!
      if (idHook == WH_MOUSE || idHook == WH_MOUSE_LL)
      {
        bool install = false;

        if (dwThreadId != 0)
        {
          if (! __hooks._RealMouseProcs.count (dwThreadId))
          {     __hooks._RealMouseProcs       [dwThreadId] = lpfn;
                                                   install = true;
          }
        }

        else if (__hooks._RealMouseProc == nullptr)
        {        __hooks._RealMouseProc = lpfn;
                                install = true;
        }

        if (install)
          lpfn = (idHook == WH_MOUSE ? SK_Proxy_MouseProc
                                     : SK_Proxy_LLMouseProc);
      }
    } break;
  }

  return
    SetWindowsHookExW_Original (
      idHook, lpfn,
              hmod, dwThreadId
    );
}

HHOOK
WINAPI
SetWindowsHookExA_Detour (
  int       idHook,
  HOOKPROC  lpfn,
  HINSTANCE hmod,
  DWORD     dwThreadId )
{
  wchar_t                   wszHookMod [MAX_PATH] = { };
  GetModuleFileNameW (hmod, wszHookMod, MAX_PATH);

  switch (idHook)
  {
    case WH_KEYBOARD:
    case WH_KEYBOARD_LL:
    {
      SK_LOG0 ( ( L" <ANSI>: Game module ( %ws ) uses a%wsKeyboard Hook...",
                       wszHookMod,
                        idHook == WH_KEYBOARD_LL ?
                                   L" Low-Level " : L" " ),
                                           L"Input Hook" );

      // Game seems to be using keyboard hooks instead of a normal Window Proc;
      //   that makes life more complicated for SK/ImGui... but we got this!
      if (idHook == WH_KEYBOARD || idHook == WH_KEYBOARD_LL)
      {
        bool install = false;

        if (dwThreadId != 0)
        {
          if (! __hooks._RealKeyboardProcs.count (dwThreadId))
          {     __hooks._RealKeyboardProcs       [dwThreadId] = lpfn;
                                                      install = true;
          }
        }

        else if (__hooks._RealKeyboardProc == nullptr)
        {        __hooks._RealKeyboardProc = lpfn;
                                   install = true;
        }

        if (install)
          lpfn = (idHook == WH_KEYBOARD ? SK_Proxy_KeyboardProc
                                        : SK_Proxy_LLKeyboardProc);
      }
    } break;

    case WH_MOUSE:
    case WH_MOUSE_LL:
    {
      SK_LOG0 ( ( L" <ANSI>: Game module ( %ws ) uses a%wsMouse Hook...",
                 wszHookMod,
                  idHook == WH_MOUSE_LL    ?
                            L" Low-Level " : L" " ),
                                    L"Input Hook" );

      // Game seems to be using mouse hooks instead of a normal Window Proc;
      //   that makes life more complicated for SK/ImGui... but we got this!
      if (idHook == WH_MOUSE || idHook == WH_MOUSE_LL)
      {
        bool install = false;

        if (dwThreadId != 0)
        {
          if (! __hooks._RealMouseProcs.count (dwThreadId))
          {     __hooks._RealMouseProcs       [dwThreadId] = lpfn;
                                                   install = true;
          }
        }

        else if (__hooks._RealMouseProc == nullptr)
        {        __hooks._RealMouseProc = lpfn;
                                install = true;
        }

        if (install)
          lpfn = (idHook == WH_MOUSE ? SK_Proxy_MouseProc
                                     : SK_Proxy_LLMouseProc);
      }
    } break;
  }

  return
    SetWindowsHookExA_Original (
      idHook, lpfn,
              hmod, dwThreadId
    );
}


// Parts of the Win32 API that are safe to hook from DLL Main
void SK_Input_PreInit (void)
{
  SK_CreateDLLHook2 (      L"User32",
                            "SetWindowsHookExA",
                             SetWindowsHookExA_Detour,
    static_cast_p2p <void> (&SetWindowsHookExA_Original) );

  SK_CreateDLLHook2 (      L"User32",
                            "SetWindowsHookExW",
                             SetWindowsHookExW_Detour,
    static_cast_p2p <void> (&SetWindowsHookExW_Original) );

  SK_CreateDLLHook2 (      L"User32",
                            "UnhookWindowsHookEx",
                             UnhookWindowsHookEx_Detour,
    static_cast_p2p <void> (&UnhookWindowsHookEx_Original) );

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

  //SK_CreateUser32Hook ("NtUserGetKeyboardState",
  SK_CreateDLLHook2 (       L"user32",
                             "GetKeyboardState",
                       //"NtUserGetKeyboardState",
                        NtUserGetKeyboardState_Detour,
     static_cast_p2p <void> (&GetKeyboardState_Original) );


  SK_CreateDLLHook2 (       L"user32",
                             "GetCursorPos",
                              GetCursorPos_Detour,
     static_cast_p2p <void> (&GetCursorPos_Original) );


  // Win 8.1 and newer aliases these
  if (SK_GetProcAddress (L"user32", "GetPhysicalCursorPos") !=
      SK_GetProcAddress (L"user32", "GetCursorPos"))
  {
    SK_CreateDLLHook2 (       L"user32",
                               "GetPhysicalCursorPos",
                                GetPhysicalCursorPos_Detour,
       static_cast_p2p <void> (&GetPhysicalCursorPos_Original) );
  }

  //////////////////////////SK_CreateDLLHook2 (       L"user32",
  //////////////////////////                           "GetMessagePos",
  //////////////////////////                            GetMessagePos_Detour,
  //////////////////////////   static_cast_p2p <void> (&GetMessagePos_Original) );
  //////////////////////////
  //////////////////////////
  //////////////////////////  SK_CreateDLLHook2 (      L"user32",
  //////////////////////////                            "GetCursorInfo",
  //////////////////////////                             GetCursorInfo_Detour,
  //////////////////////////    static_cast_p2p <void> (&GetCursorInfo_Original) );
  //////////////////////////
  //////////////////////////SK_CreateDLLHook2 (       L"user32",
  //////////////////////////                           "GetMouseMovePointsEx",
  //////////////////////////                            GetMouseMovePointsEx_Detour,
  //////////////////////////   static_cast_p2p <void> (&GetMouseMovePointsEx_Original) );

  SK_CreateDLLHook2 (       L"user32",
                             "SetCursor",
                              SetCursor_Detour,
     static_cast_p2p <void> (&SetCursor_Original) );

  SK_CreateDLLHook2 (       L"user32",
                             "ShowCursor",
                              ShowCursor_Detour,
     static_cast_p2p <void> (&ShowCursor_Original) );

  ////////////////////////////SK_CreateDLLHook2 (       L"user32",
  ////////////////////////////                                 "GetCursor",
  ///////////////////////////SK_CreateUser32Hook (      "NtUserGetCursor",
  ////////////////////////////                           NtUserGetCursor_Detour,
  ////////////////////////////  static_cast_p2p <void> (&NtUserGetCursor_Original) );

  SK_CreateDLLHook2 (       L"user32",
                             "SetCursorPos",
                              SetCursorPos_Detour,
     static_cast_p2p <void> (&SetCursorPos_Original) );

  // Win 8.1 and newer aliases these
  if (SK_GetProcAddress (L"user32", "SetPhysicalCursorPos") !=
      SK_GetProcAddress (L"user32", "SetCursorPos"))
  {
    SK_CreateDLLHook2 (       L"user32",
                               "SetPhysicalCursorPos",
                                SetPhysicalCursorPos_Detour,
       static_cast_p2p <void> (&SetPhysicalCursorPos_Original) );
  }


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


  if (config.input.gamepad.hook_raw_input)
  {
    SK_CreateDLLHook2 (       L"user32",
                               "RegisterRawInputDevices",
                          NtUserRegisterRawInputDevices_Detour,
       static_cast_p2p <void> (&RegisterRawInputDevices_Original) );

#ifdef  __MANAGE_RAW_INPUT_REGISTRATION
    SK_CreateDLLHook2 (       L"user32",
                               "GetRegisteredRawInputDevices",
                          NtUserGetRegisteredRawInputDevices_Detour,
       static_cast_p2p <void> (&GetRegisteredRawInputDevices_Original) );
#endif

#if 1
    SK_CreateDLLHook2 (       L"user32",
                               "GetRawInputData",
                          NtUserGetRawInputData_Detour,
       static_cast_p2p <void> (&GetRawInputData_Original) );

#ifdef __HOOK_BUFFERED_RAW_INPUT
    SK_CreateDLLHook2 (       L"user32",
                               "GetRawInputBuffer",
                                GetRawInputBuffer_Detour,
       static_cast_p2p <void> (&GetRawInputBuffer_Original) );
#endif
  }
#endif

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
SK_Input_Init (void)
{
  // -- Async Init = OFF option may invoke this twice
  //SK_ReleaseAssert (std::exchange (once, true) == false);

  static bool        once = false;
  if (std::exchange (once, true))
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
  static auto& rb =
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
SK_ImGui_DrawGamepadStatusBar (void)
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

  ImGui::BeginGroup ();

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

      ++attached_pads;
    }
  }

  ImGui::Spacing  ();
  ImGui::EndGroup ();

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

bool SK_ImGui_InputLanguage_s::changed      = true; // ^^^^ Default = true
HKL  SK_ImGui_InputLanguage_s::keybd_layout;