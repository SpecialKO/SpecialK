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
#include <imgui/font_awesome.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Input Mgr."

#define SK_HID_READ(type)  SK_HID_Backend->markRead   (type);
#define SK_HID_WRITE(type) SK_HID_Backend->markWrite  (type);
#define SK_HID_VIEW(type)  SK_HID_Backend->markViewed (type);
#define SK_HID_HIDE(type)  SK_HID_Backend->markHidden (type);

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

              SK_ImGui_CreateNotification (
                "HID.GamepadAttached", SK_ImGui_Toast::Info,
                SK_FormatString ("%ws\t%ws",
                   wszManufacturerName,
                   wszProductName ).c_str (),
                "HID Compliant Gamepad Connected", 10000
              );
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

XINPUT_STATE hid_to_xi { };
  
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
            SK_CreateFileW ( wszFileName, FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                          FILE_SHARE_READ   | FILE_SHARE_WRITE,
                                            nullptr, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH  |
                                                                    FILE_ATTRIBUTE_TEMPORARY |
                                                                    FILE_FLAG_OVERLAPPED, nullptr );
  
          if (controller.hDeviceFile != INVALID_HANDLE_VALUE)
          {
            if (! SK_HidD_GetPreparsedData (controller.hDeviceFile, &controller.pPreparsedData))
            	continue;

            HIDP_CAPS                                      caps = { };
              SK_HidP_GetCaps (controller.pPreparsedData, &caps);

            controller.input_report.resize  (caps.InputReportByteLength);
            controller.output_report.resize (caps.OutputReportByteLength);

            std::vector <HIDP_BUTTON_CAPS>
              buttonCapsArray;
              buttonCapsArray.resize (caps.NumberInputButtonCaps);

            std::vector <HIDP_VALUE_CAPS>
              valueCapsArray;
              valueCapsArray.resize (caps.NumberInputValueCaps);

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

                // ???
                else
                {
                  // No idea what a third set of buttons would be...
                  SK_ReleaseAssert (num_caps <= 2);
                }
              }

              USHORT value_caps_count =
                sk::narrow_cast <USHORT> (valueCapsArray.size ());

              if ( HIDP_STATUS_SUCCESS ==
                     SK_HidP_GetValueCaps ( HidP_Input, valueCapsArray.data (),
                                                       &value_caps_count,
                                                        controller.pPreparsedData ) )
              {
                controller.value_caps.resize (value_caps_count);

                for ( int idx = 0; idx < value_caps_count; ++idx )
                {
                  controller.value_caps [idx] = valueCapsArray [idx];
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

            controller.bDualShock4 =
              StrStrIW (wszFileName, L"PID_05C4") != nullptr ||
              StrStrIW (wszFileName, L"PID_09CC") != nullptr ||
              StrStrIW (wszFileName, L"PID_0BA0") != nullptr;

            controller.bDualShock3 =
              StrStrIW (wszFileName, L"PID_0268") != nullptr;
  
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
HidD_GetPreparsedData_pfn   HidD_GetPreparsedData_Original  = nullptr;
HidD_FreePreparsedData_pfn  HidD_FreePreparsedData_Original = nullptr;
HidD_GetFeature_pfn         HidD_GetFeature_Original        = nullptr;
HidD_SetFeature_pfn         HidD_SetFeature_Original        = nullptr;
HidP_GetData_pfn            HidP_GetData_Original           = nullptr;
HidP_GetCaps_pfn            HidP_GetCaps_Original           = nullptr;
HidP_GetUsages_pfn          HidP_GetUsages_Original         = nullptr;

HidD_GetPreparsedData_pfn   SK_HidD_GetPreparsedData        = nullptr;
HidD_FreePreparsedData_pfn  SK_HidD_FreePreparsedData       = nullptr;
HidD_GetInputReport_pfn     SK_HidD_GetInputReport          = nullptr;
HidD_GetFeature_pfn         SK_HidD_GetFeature              = nullptr;
HidD_SetFeature_pfn         SK_HidD_SetFeature              = nullptr;
HidD_FlushQueue_pfn         SK_HidD_FlushQueue              = nullptr;
HidP_GetData_pfn            SK_HidP_GetData                 = nullptr;
HidP_GetCaps_pfn            SK_HidP_GetCaps                 = nullptr;
HidP_GetButtonCaps_pfn      SK_HidP_GetButtonCaps           = nullptr;
HidP_GetValueCaps_pfn       SK_HidP_GetValueCaps            = nullptr;
HidP_GetUsages_pfn          SK_HidP_GetUsages               = nullptr;
HidP_GetUsageValue_pfn      SK_HidP_GetUsageValue           = nullptr;
HidP_GetUsageValueArray_pfn SK_HidP_GetUsageValueArray      = nullptr;

bool
SK_HID_FilterPreparsedData (PHIDP_PREPARSED_DATA pData)
{
  bool filter = false;

        HIDP_CAPS caps;
  const NTSTATUS  stat =
          SK_HidP_GetCaps (pData, &caps);

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

    if ( config.input.gamepad.disable_hid  ||
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
HidD_SetFeature_Detour ( _In_ HANDLE HidDeviceObject,
                         _In_ PVOID  ReportBuffer,
                         _In_ ULONG  ReportBufferLength )
{
  SK_LOG_FIRST_CALL

#if 0
  SK_ImGui_CreateNotification (
    "HidD_SetFeature.Trace", SK_ImGui_Toast::Info,
    SK_FormatString ( "Device: %p, *pReportBuffer=%p, ReportLength=%d bytes",
                        HidDeviceObject, ReportBuffer, ReportBufferLength ).c_str (),
    "HidD_SetFeature (...) called!", 1000UL,
      SK_ImGui_Toast::UseDuration | SK_ImGui_Toast::ShowCaption |
      SK_ImGui_Toast::ShowTitle   | SK_ImGui_Toast::ShowNewest );
#endif

  return
    HidD_SetFeature_Original ( HidDeviceObject,
                               ReportBuffer,
                               ReportBufferLength );
}

BOOLEAN
_Success_ (return)
__stdcall
HidD_GetFeature_Detour ( _In_  HANDLE HidDeviceObject,
                         _Out_ PVOID  ReportBuffer,
                         _In_  ULONG  ReportBufferLength )
{
  SK_LOG_FIRST_CALL

  ////SK_HID_READ (sk_input_dev_type::Gamepad)

  bool                 filter = false;
  PHIDP_PREPARSED_DATA pData  = nullptr;

  if (SK_ImGui_WantGamepadCapture ())
  {
    if (HidD_GetPreparsedData_Original (HidDeviceObject, &pData))
    {
      if (SK_HID_FilterPreparsedData (pData))
        filter = true;

      HidD_FreePreparsedData_Original (pData);
    }
  }

  BOOLEAN bRet =
    HidD_GetFeature_Original (
      HidDeviceObject, ReportBuffer,
                       ReportBufferLength
    );

  if (filter)
  {
    ZeroMemory (ReportBuffer, ReportBufferLength);
  }

  return bRet;
}

NTSTATUS
__stdcall
HidP_GetUsages_Detour (
  _In_                                        HIDP_REPORT_TYPE     ReportType,
  _In_                                        USAGE                UsagePage,
  _In_opt_                                    USHORT               LinkCollection,
  _Out_writes_to_(*UsageLength, *UsageLength) PUSAGE               UsageList,
  _Inout_                                     PULONG               UsageLength,
  _In_                                        PHIDP_PREPARSED_DATA PreparsedData,
  _Out_writes_bytes_(ReportLength)            PCHAR                Report,
  _In_                                        ULONG                ReportLength
)
{
  SK_LOG_FIRST_CALL

  NTSTATUS ret =
    HidP_GetUsages_Original ( ReportType, UsagePage,
                                LinkCollection, UsageList,
                                  UsageLength, PreparsedData,
                                    Report, ReportLength );

  // De we want block this I/O?
  bool filter = false;

  if (          ret == HIDP_STATUS_SUCCESS &&
       ( ReportType == HidP_Input          ||
         ReportType == HidP_Output         ||
         ReportType == HidP_Feature ) )
  {
    if (SK_ImGui_WantGamepadCapture ())
    {
      // This will classify the data for us, so don't record this event yet.
      filter =
        SK_HID_FilterPreparsedData (PreparsedData);
    }
  }

  if (filter)
  {
    SK_ReleaseAssert (UsageLength != nullptr);

    ZeroMemory (UsageList, *UsageLength);
                           *UsageLength = 0;
  }

  return ret;
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

CreateFileW_pfn                 SK_CreateFileW = nullptr;
CreateFile2_pfn                 SK_CreateFile2 = nullptr;
ReadFile_pfn                       SK_ReadFile = nullptr;
WriteFile_pfn                     SK_WriteFile = nullptr;
CancelIoEx_pfn                   SK_CancelIoEx = nullptr;
GetOverlappedResult_pfn SK_GetOverlappedResult = nullptr;

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

    if (config.input.gamepad.disable_hid)
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

      HidP_GetCaps_Original =
        (HidP_GetCaps_pfn)SK_GetProcAddress ( SK_GetModuleHandle (L"HID.DLL"),
                                              "HidP_GetCaps" );
    }

    SK_CreateDLLHook2 (     L"HID.DLL",
                             "HidD_GetFeature",
                              HidD_GetFeature_Detour,
     static_cast_p2p <void> (&HidD_GetFeature_Original) );

    SK_CreateDLLHook2 (     L"HID.DLL",
                             "HidD_SetFeature",
                              HidD_SetFeature_Detour,
     static_cast_p2p <void> (&HidD_SetFeature_Original) );

    SK_CreateDLLHook2 (     L"HID.DLL",
                             "HidP_GetUsages",
                              HidP_GetUsages_Detour,
     static_cast_p2p <void> (&HidP_GetUsages_Original) );

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

#define _HOOK_READ_FILE
#ifdef  _HOOK_READ_FILE
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

  SK_HidD_SetFeature =
    (HidD_SetFeature_pfn)SK_GetProcAddress (hModHID,
    "HidD_SetFeature");

  SK_HidP_GetData =
    (HidP_GetData_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetData");

  SK_HidP_GetCaps =
    (HidP_GetCaps_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetCaps");

  SK_HidP_GetButtonCaps =
    (HidP_GetButtonCaps_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetButtonCaps");

  SK_HidP_GetValueCaps =
    (HidP_GetValueCaps_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetValueCaps");

  SK_HidP_GetUsages =
    (HidP_GetUsages_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetUsages");

  SK_HidP_GetUsageValue =
    (HidP_GetUsageValue_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetUsageValue");

  SK_HidP_GetUsageValueArray =
    (HidP_GetUsageValueArray_pfn)SK_GetProcAddress (hModHID,
    "HidP_GetUsageValueArray");

  SK_HidD_GetInputReport =
    (HidD_GetInputReport_pfn)SK_GetProcAddress (hModHID,
    "HidD_GetInputReport");

  SK_HidD_FlushQueue =
    (HidD_FlushQueue_pfn)SK_GetProcAddress (hModHID,
    "HidD_FlushQueue");

  SK_CreateFile2 =
    (CreateFile2_pfn)SK_GetProcAddress (hModKernel32,
    "CreateFile2");

  SK_CreateFileW =
    (CreateFileW_pfn)SK_GetProcAddress (hModKernel32,
    "CreateFileW");

  SK_ReadFile =
    (ReadFile_pfn)SK_GetProcAddress (hModKernel32,
    "ReadFile");

  SK_WriteFile =
    (WriteFile_pfn)SK_GetProcAddress (hModKernel32,
    "WriteFile");

  SK_CancelIoEx =
    (CancelIoEx_pfn)SK_GetProcAddress (hModKernel32,
    "CancelIoEx");

  SK_GetOverlappedResult =
    (GetOverlappedResult_pfn)SK_GetProcAddress (hModKernel32,
    "GetOverlappedResult");

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


enum Direction : uint8_t {
  North     = 0,
  NorthEast,
  East,
  SouthEast,
  South,
  SouthWest,
  West,
  NorthWest,
  Neutral   = 8
};

enum PowerState : uint8_t {
  Discharging         = 0x00, // Use PowerPercent
  Charging            = 0x01, // Use PowerPercent
  Complete            = 0x02, // PowerPercent not valid? assume 100%?
  AbnormalVoltage     = 0x0A, // PowerPercent not valid?
  AbnormalTemperature = 0x0B, // PowerPercent not valid?
  ChargingError       = 0x0F  // PowerPercent not valid?
};

enum MuteLight : uint8_t {
  Off       = 0,
  On,
  Breathing,
  DoNothing, // literally nothing, this input is ignored,
             // though it might be a faster blink in other versions
  NoMuteAction4,
  NoMuteAction5,
  NoMuteAction6,
  NoMuteAction7 = 7
};

enum LightBrightness : uint8_t {
  Bright    = 0,
  Mid,
  Dim,
  NoLightAction3,
  NoLightAction4,
  NoLightAction5,
  NoLightAction6,
  NoLightAction7 = 7
};

enum LightFadeAnimation : uint8_t {
  Nothing = 0,
  FadeIn, // from black to blue
  FadeOut // from blue to black
};

struct TouchFingerData { // 4
/*0.0*/ uint32_t Index       : 7;
/*0.7*/ uint32_t NotTouching : 1;
/*1.0*/ uint32_t FingerX     : 12;
/*2.4*/ uint32_t FingerY     : 12;
};

struct TouchData { // 9
/*0*/ TouchFingerData Finger [2];
/*8*/ uint8_t         Timestamp;
};

template <int N> struct BTCRC {
  uint8_t  Buff [N-4];
  uint32_t CRC;
};

// Derived from (2024-02-07):
// 
//   https://controllers.fandom.com/wiki/Sony_DualSense#Likely_Interface
//
struct SK_HID_DualSense_SetStateData // 47
{
/* 0.0*/ uint8_t EnableRumbleEmulation         : 1; // Suggest halving rumble strength
/* 0.1*/ uint8_t UseRumbleNotHaptics           : 1; // 
/*    */                                       
/* 0.2*/ uint8_t AllowRightTriggerFFB          : 1; // Enable setting RightTriggerFFB
/* 0.3*/ uint8_t AllowLeftTriggerFFB           : 1; // Enable setting LeftTriggerFFB
/*    */                                       
/* 0.4*/ uint8_t AllowHeadphoneVolume          : 1; // Enable setting VolumeHeadphones
/* 0.5*/ uint8_t AllowSpeakerVolume            : 1; // Enable setting VolumeSpeaker
/* 0.6*/ uint8_t AllowMicVolume                : 1; // Enable setting VolumeMic
/*    */                                       
/* 0.7*/ uint8_t AllowAudioControl             : 1; // Enable setting AudioControl section
/* 1.0*/ uint8_t AllowMuteLight                : 1; // Enable setting MuteLightMode
/* 1.1*/ uint8_t AllowAudioMute                : 1; // Enable setting MuteControl section
/*    */                                       
/* 1.2*/ uint8_t AllowLedColor                 : 1; // Enable RGB LED section
/*    */                                       
/* 1.3*/ uint8_t ResetLights                   : 1; // Release the LEDs from Wireless firmware control
/*    */                                            // When in wireless mode this must be signaled to control LEDs
/*    */                                            // This cannot be applied during the BT pair animation.
/*    */                                            // SDL2 waits until the SensorTimestamp value is >= 10200000
/*    */                                            // before pulsing this bit once.
/*    */
/* 1.4*/ uint8_t AllowPlayerIndicators         : 1; // Enable setting PlayerIndicators section
/* 1.5*/ uint8_t AllowHapticLowPassFilter      : 1; // Enable HapticLowPassFilter
/* 1.6*/ uint8_t AllowMotorPowerLevel          : 1; // MotorPowerLevel reductions for trigger/haptic
/* 1.7*/ uint8_t AllowAudioControl2            : 1; // Enable setting AudioControl2 section
/*    */                                       
/* 2  */ uint8_t RumbleEmulationRight;              // emulates the light weight
/* 3  */ uint8_t RumbleEmulationLeft;               // emulated the heavy weight
/*    */
/* 4  */ uint8_t VolumeHeadphones;                  // max 0x7f
/* 5  */ uint8_t VolumeSpeaker;                     // PS5 appears to only use the range 0x3d-0x64
/* 6  */ uint8_t VolumeMic;                         // not linear, seems to max at 64, 0 is not fully muted
/*    */
/*    */ // AudioControl
/* 7.0*/ uint8_t MicSelect                     : 2; // 0 Auto
/*    */                                            // 1 Internal Only
/*    */                                            // 2 External Only
/*    */                                            // 3 Unclear, sets external mic flag but might use internal mic, do test
/* 7.2*/ uint8_t EchoCancelEnable              : 1;
/* 7.3*/ uint8_t NoiseCancelEnable             : 1;
/* 7.4*/ uint8_t OutputPathSelect              : 2; // 0 L_R_X
/*    */                                            // 1 L_L_X
/*    */                                            // 2 L_L_R
/*    */                                            // 3 X_X_R
/* 7.6*/ uint8_t InputPathSelect               : 2; // 0 CHAT_ASR
/*    */                                            // 1 CHAT_CHAT
/*    */                                            // 2 ASR_ASR
/*    */                                            // 3 Does Nothing, invalid
/*    */
/* 8  */ MuteLight MuteLightMode;
/*    */
/*    */ // MuteControl
/* 9.0*/ uint8_t TouchPowerSave                : 1;
/* 9.1*/ uint8_t MotionPowerSave               : 1;
/* 9.2*/ uint8_t HapticPowerSave               : 1; // AKA BulletPowerSave
/* 9.3*/ uint8_t AudioPowerSave                : 1;
/* 9.4*/ uint8_t MicMute                       : 1;
/* 9.5*/ uint8_t SpeakerMute                   : 1;
/* 9.6*/ uint8_t HeadphoneMute                 : 1;
/* 9.7*/ uint8_t HapticMute                    : 1; // AKA BulletMute
/*    */
/*10  */ uint8_t  RightTriggerFFB [11];
/*21  */ uint8_t  LeftTriggerFFB  [11];
/*32  */ uint32_t HostTimestamp;                    // mirrored into report read
/*    */
/*    */ // MotorPowerLevel
/*36.0*/ uint8_t TriggerMotorPowerReduction    : 4; // 0x0-0x7 (no 0x8?) Applied in 12.5% reductions
/*36.4*/ uint8_t RumbleMotorPowerReduction     : 4; // 0x0-0x7 (no 0x8?) Applied in 12.5% reductions
/*    */
/*    */ // AudioControl2
/*37.0*/ uint8_t SpeakerCompPreGain            : 3; // additional speaker volume boost
/*37.3*/ uint8_t BeamformingEnable             : 1; // Probably for MIC given there's 2, might be more bits, can't find what it does
/*37.4*/ uint8_t UnkAudioControl2              : 4; // some of these bits might apply to the above
/*    */
/*38.0*/ uint8_t AllowLightBrightnessChange    : 1; // LED_BRIHTNESS_CONTROL
/*38.1*/ uint8_t AllowColorLightFadeAnimation  : 1; // LIGHTBAR_SETUP_CONTROL
/*38.2*/ uint8_t EnableImprovedRumbleEmulation : 1; // Use instead of EnableRumbleEmulation
                                                    // requires FW >= 0x0224
                                                    // No need to halve rumble strength
/*38.3*/ uint8_t UNKBITC                       : 5; // unused
/*    */
/*39.0*/ uint8_t HapticLowPassFilter           : 1;
/*39.1*/ uint8_t UNKBIT                        : 7;
/*    */
/*40  */ uint8_t UNKBYTE;                           // previous notes suggested this was HLPF, was probably off by 1
/*    */
/*41  */ LightFadeAnimation LightFadeAnimation;
/*42  */ LightBrightness    LightBrightness;
/*    */
/*    */ // PlayerIndicators
/*    */ // These bits control the white LEDs under the touch pad.
/*    */ // Note the reduction in functionality for later revisions.
/*    */ // Generation 0x03 - Full Functionality
/*    */ // Generation 0x04 - Mirrored Only
/*    */ // Suggested detection: (HardwareInfo & 0x00FFFF00) == 0X00000400
/*    */ //
/*    */ // Layout used by PS5:
/*    */ // 0x04 - -x- -  Player 1
/*    */ // 0x06 - x-x -  Player 2
/*    */ // 0x15 x -x- x  Player 3
/*    */ // 0x1B x x-x x  Player 4
/*    */ // 0x1F x xxx x  Player 5* (Unconfirmed)
/*    */ //
/*    */ //                                         // HW 0x03 // HW 0x04
/*43.0*/ uint8_t PlayerLight1                  : 1; // x --- - // x --- x
/*43.1*/ uint8_t PlayerLight2                  : 1; // - x-- - // - x-x -
/*43.2*/ uint8_t PlayerLight3                  : 1; // - -x- - // - -x- -
/*43.3*/ uint8_t PlayerLight4                  : 1; // - --x - // - x-x -
/*43.4*/ uint8_t PlayerLight5                  : 1; // - --- x // x --- x
/*43.5*/ uint8_t PlayerLightFade               : 1; // if low player lights fade in, if high player lights instantly change
/*43.6*/ uint8_t PlayerLightUNK                : 2;
/*    */
/*    */ // RGB LED
/*44  */ uint8_t LedRed;
/*45  */ uint8_t LedGreen;
/*46  */ uint8_t LedBlue;
// Structure ends here though on BT there is padding and a CRC, see ReportOut31
};

struct SK_HID_DualSense_GetStateData // 63
{
/* 0  */ uint8_t    LeftStickX;
/* 1  */ uint8_t    LeftStickY;
/* 2  */ uint8_t    RightStickX;
/* 3  */ uint8_t    RightStickY;
/* 4  */ uint8_t    TriggerLeft;
/* 5  */ uint8_t    TriggerRight;
/* 6  */ uint8_t    SeqNo;                   // always 0x01 on BT
/* 7.0*/ Direction  DPad                : 4;
/* 7.4*/ uint8_t    ButtonSquare        : 1;
/* 7.5*/ uint8_t    ButtonCross         : 1;
/* 7.6*/ uint8_t    ButtonCircle        : 1;
/* 7.7*/ uint8_t    ButtonTriangle      : 1;
/* 8.0*/ uint8_t    ButtonL1            : 1;
/* 8.1*/ uint8_t    ButtonR1            : 1;
/* 8.2*/ uint8_t    ButtonL2            : 1;
/* 8.3*/ uint8_t    ButtonR2            : 1;
/* 8.4*/ uint8_t    ButtonCreate        : 1;
/* 8.5*/ uint8_t    ButtonOptions       : 1;
/* 8.6*/ uint8_t    ButtonL3            : 1;
/* 8.7*/ uint8_t    ButtonR3            : 1;
/* 9.0*/ uint8_t    ButtonHome          : 1;
/* 9.1*/ uint8_t    ButtonPad           : 1;
/* 9.2*/ uint8_t    ButtonMute          : 1;
/* 9.3*/ uint8_t    ButtonLeftFunction  : 1; // DualSense Edge
/* 9.4*/ uint8_t    ButtonRightFunction : 1; // DualSense Edge
/* 9.5*/ uint8_t    ButtonLeftPaddle    : 1; // DualSense Edge
/* 9.6*/ uint8_t    ButtonRightPaddle   : 1; // DualSense Edge
/* 9.7*/ uint8_t    UNK1                : 1; // appears unused
/*10  */ uint8_t    UNK2;                    // appears unused
/*11  */ uint32_t   UNK_COUNTER;             // Linux driver calls this reserved, tools leak calls the 2 high bytes "random"
/*15  */ int16_t    AngularVelocityX;
/*17  */ int16_t    AngularVelocityZ;
/*19  */ int16_t    AngularVelocityY;
/*21  */ int16_t    AccelerometerX;
/*23  */ int16_t    AccelerometerY;
/*25  */ int16_t    AccelerometerZ;
/*27  */ uint32_t   SensorTimestamp;
/*31  */ int8_t     Temperature;                  // reserved2 in Linux driver
/*32  */ TouchData  TouchData;
/*41.0*/ uint8_t    TriggerRightStopLocation : 4; // trigger stop can be a range from 0 to 9 (F/9.0 for Apple interface)
/*41.4*/ uint8_t    TriggerRightStatus       : 4;
/*42.0*/ uint8_t    TriggerLeftStopLocation  : 4;
/*42.4*/ uint8_t    TriggerLeftStatus        : 4; // 0 feedbackNoLoad
                                                  // 1 feedbackLoadApplied
                                                  // 0 weaponReady
                                                  // 1 weaponFiring
                                                  // 2 weaponFired
                                                  // 0 vibrationNotVibrating
                                                  // 1 vibrationIsVibrating
/*43  */ uint32_t   HostTimestamp;                // mirrors data from report write
/*47.0*/ uint8_t    TriggerRightEffect       : 4; // Active trigger effect, previously we thought this was status max
/*47.4*/ uint8_t    TriggerLeftEffect        : 4; // 0 for reset and all other effects
                                                  // 1 for feedback effect
                                                  // 2 for weapon effect
                                                  // 3 for vibration
/*48  */ uint32_t   DeviceTimeStamp;         
/*52.0*/ uint8_t    PowerPercent             : 4; // 0x00-0x0A
/*52.4*/ PowerState PowerState               : 4;
/*53.0*/ uint8_t    PluggedHeadphones        : 1;
/*53.1*/ uint8_t    PluggedMic               : 1;
/*53.2*/ uint8_t    MicMuted                 : 1; // Mic muted by powersave/mute command
/*53.3*/ uint8_t    PluggedUsbData           : 1;
/*53.4*/ uint8_t    PluggedUsbPower          : 1;
/*53.5*/ uint8_t    PluggedUnk1              : 3;
/*54.0*/ uint8_t    PluggedExternalMic       : 1; // Is external mic active (automatic in mic auto mode)
/*54.1*/ uint8_t    HapticLowPassFilter      : 1; // Is the Haptic Low-Pass-Filter active?
/*54.2*/ uint8_t    PluggedUnk3              : 6;
/*55  */ uint8_t    AesCmac [8];
};

void
SK_HID_PlayStationDevice::setVibration (
  USHORT left,
  USHORT right,
  USHORT max_val )
{
  WriteULongRelease (&_vibration.left,
      static_cast  <ULONG> (255.0 *
        std::clamp (
          (static_cast <double> (left)/
           static_cast <double> (max_val)), 0.0, 1.0)));
  
  WriteULongRelease (&_vibration.right,
      static_cast  <ULONG> (255.0 *
        std::clamp (
          (static_cast <double> (right)/
           static_cast <double> (max_val)), 0.0, 1.0)));
  
  _vibration.last_set = SK::ControlPanel::current_time;
}

bool
SK_HID_PlayStationDevice::request_input_report (void)
{
  if (! bConnected)
    return false;

  if (hInputEvent == nullptr)
  {   hInputEvent =
        SK_CreateEvent ( nullptr, TRUE, TRUE,
          SK_FormatStringW (L"[SK] HID Input Report %p", hDeviceFile).c_str ());

    if (hDisconnectEvent == nullptr)
        hDisconnectEvent =
          SK_CreateEvent ( nullptr, FALSE, FALSE,
            SK_FormatStringW (L"[SK] HID IO Suspend %p", hDeviceFile).c_str ());

    if (hReconnectEvent == nullptr)
        hReconnectEvent =
          SK_CreateEvent ( nullptr, FALSE, FALSE,
            SK_FormatStringW (L"[SK] HID IO Resume %p", hDeviceFile).c_str ());

    SK_Thread_CreateEx ([](LPVOID pUser)->DWORD
    {
      SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);

      SK_HID_PlayStationDevice* pDevice =
        (SK_HID_PlayStationDevice *)pUser;

      HANDLE hOperationalEvents [] = {
        __SK_DLL_TeardownEvent,
        pDevice->hInputEvent,
        pDevice->hDisconnectEvent,
        pDevice->hReconnectEvent
      };

      OVERLAPPED async_input_request = { };

      DWORD  dwWaitState  = WAIT_FAILED;
      while (dwWaitState != WAIT_OBJECT_0)
      {
        async_input_request        = { };
        async_input_request.hEvent = pDevice->hInputEvent;

        ZeroMemory ( pDevice->input_report.data (),
                     pDevice->input_report.size () );

        pDevice->input_report [0] = pDevice->button_report_id;

        BOOL bReadAsync =
          SK_ReadFile ( pDevice->hDeviceFile, pDevice->input_report.data (),
                         static_cast <DWORD> (pDevice->input_report.size ()), nullptr, &async_input_request );

        if (! bReadAsync)
        {
          DWORD dwLastErr =
               GetLastError ();

          // Invalid Handle and Device Not Connected get a 250 ms cooldown
          if ( dwLastErr != NOERROR              &&
               dwLastErr != ERROR_IO_PENDING     &&
               dwLastErr != ERROR_INVALID_HANDLE &&
               dwLastErr != ERROR_NO_SUCH_DEVICE &&
               dwLastErr != ERROR_DEVICE_NOT_CONNECTED )
          {
            _com_error err (
              _com_error::WCodeToHRESULT (
                sk::narrow_cast <WORD> (dwLastErr)
              )
            );

            SK_LOGs0 (
              L"InputBkEnd",
              L"SK_ReadFile ({%ws}) failed, Error=%d [%ws]",
                pDevice->wszDevicePath, dwLastErr,
                  err.ErrorMessage ()
            );
          }

          if (dwLastErr != ERROR_IO_PENDING)
          {
            if (dwLastErr != ERROR_INVALID_HANDLE)
              SK_CancelIoEx (pDevice->hDeviceFile, &async_input_request);

            SK_SleepEx (250UL, TRUE); // Prevent runaway CPU usage on failure

            continue;
          }
        }

        dwWaitState =
          WaitForMultipleObjects ( _countof (hOperationalEvents),
                                             hOperationalEvents, FALSE, INFINITE );

        // Disconnect
        if (dwWaitState == (WAIT_OBJECT_0 + 2))
        {
          SK_ReleaseAssert (
            SK_CancelIoEx (pDevice->hDeviceFile, &async_input_request)
          );

          continue;
        }

        // Input Report Waiting
        if (dwWaitState == (WAIT_OBJECT_0 + 1))
        {
          DWORD dwBytesTransferred = 0;

          if (! SK_GetOverlappedResult (
                  pDevice->hDeviceFile, &async_input_request,
                    &dwBytesTransferred, TRUE ) )
          {
            // Stop polling on the first failure, if the device comes back to life, we'll probably
            //   get a device arrival notification.
            continue;
          }

          else
          {
            if (pDevice->pPreparsedData == nullptr)
            {
              SK_ReleaseAssert (pDevice->pPreparsedData != nullptr);
              continue;
            }

            bool clear_haptics = false;

            if (pDevice->_vibration.last_set != 0 &&
                pDevice->_vibration.last_set < SK::ControlPanel::current_time - pDevice->_vibration.MAX_TTL_IN_MSECS)
            {
              pDevice->_vibration.last_set = 0;
              clear_haptics = true;
            }

            if (SK_ImGui_WantGamepadCapture () || config.input.gamepad.xinput.emulate)
            {
              //static DWORD dwLastFlush = 0;
              //if (         dwLastFlush < SK::ControlPanel::current_time - 16)
              //{            dwLastFlush = SK::ControlPanel::current_time;
                pDevice->write_output_report ();
              //}
            }

            ULONG num_usages =
              static_cast <ULONG> (pDevice->button_usages.size ());

            if ( HIDP_STATUS_SUCCESS ==
              SK_HidP_GetUsages ( HidP_Input, pDevice->buttons [0].UsagePage, 0,
                                              pDevice->button_usages.data (),
                                                              &num_usages,
                                              pDevice->pPreparsedData,
                                       (PCHAR)pDevice->input_report.data  (),
                         static_cast <ULONG> (pDevice->input_report.size  ()) )
               )
            {
              for ( auto& button : pDevice->buttons )
              {
                button.last_state =
                  std::exchange (button.state, false);
              }

              for ( UINT i = 0; i < num_usages; ++i )
              {
                pDevice->buttons [
                  pDevice->button_usages [i] -
                  pDevice->buttons       [0].Usage
                ].state = true;
              }
            }

            pDevice->xinput.report.Gamepad = { };

            for ( UINT i = 0 ; i < pDevice->value_caps.size () ; ++i )
            {
              ULONG value;

              if ( HIDP_STATUS_SUCCESS ==
                SK_HidP_GetUsageValue ( HidP_Input, pDevice->value_caps [i].UsagePage,   0,
                                                    pDevice->value_caps [i].Range.UsageMin,
                                                                                    &value,
                                                    pDevice->pPreparsedData,
                                             (PCHAR)pDevice->input_report.data  (),
                               static_cast <ULONG> (pDevice->input_report.size  ()) ) )
              {
                switch (pDevice->value_caps [i].Range.UsageMin)
                {
                  case 0x30: // X-axis
                    pDevice->xinput.report.Gamepad.sThumbLX =
                      static_cast <SHORT> (32767.0 * static_cast <double> (static_cast <LONG> (value) - 128) / 128.0);
                      break;

                  case 0x31: // Y-axis
                    pDevice->xinput.report.Gamepad.sThumbLY =
                      static_cast <SHORT> (32767.0 * static_cast <double> (128 - static_cast <LONG> (value)) / 128.0);
                      break;

                  case 0x32: // Z-axis
                    pDevice->xinput.report.Gamepad.sThumbRX =
                      static_cast <SHORT> (32767.0 * static_cast <double> (static_cast <LONG> (value) - 128) / 128.0);
                      break;

                  case 0x33: // Rotate-X
                    pDevice->xinput.report.Gamepad.bLeftTrigger =
                      static_cast <BYTE> (static_cast <BYTE> (value));
                    break;

                  case 0x34: // Rotate-Y
                    pDevice->xinput.report.Gamepad.bRightTrigger =
                      static_cast <BYTE> (static_cast <BYTE> (value));
                    break;

                  case 0x35: // Rotate-Z
                    pDevice->xinput.report.Gamepad.sThumbRY =
                      static_cast <SHORT> (32767.0 * static_cast <double> (128 - static_cast <LONG> (value)) / 128.0);
#if 0
                    if (value != 0)
                    {
                      SK_ImGui_CreateNotification (
                        "HID.Debug.Axes", SK_ImGui_Toast::Info,
                          std::to_string (value).c_str (),
                        SK_FormatString ( "HID Axial State [%d, %d]",
                                          ps_controller.value_caps [i].PhysicalMin,
                                          ps_controller.value_caps [i].PhysicalMax
                                        ).c_str (),
                            1000UL, SK_ImGui_Toast::UseDuration |
                                    SK_ImGui_Toast::ShowCaption |
                                    SK_ImGui_Toast::ShowTitle   |
                                    SK_ImGui_Toast::ShowNewest );
                    }
#endif
                      break;

                  case 0x39: // Hat Switch
                  {
                    switch (value)
                    {
                      case 0: pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;    break;
                      case 1: pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
                              pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT; break;
                      case 2: pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT; break;
                      case 3: pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
                              pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;  break;
                      case 4: pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;  break;
                      case 5: pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
                              pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;  break;
                      case 6: pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;  break;
                      case 7: pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
                              pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;    break;                            
                      case 8:
                      default:
                        // Centered value, do nothing
                        break;
                    }
#if 0
                    if (value != 8)
                    {
                      SK_ImGui_CreateNotification (
                        "HID.Debug.HatSwitch", SK_ImGui_Toast::Info,
                          std::to_string (value).c_str (), "HID D-Pad State",
                            1000UL, SK_ImGui_Toast::UseDuration |
                                    SK_ImGui_Toast::ShowCaption |
                                    SK_ImGui_Toast::ShowTitle   |
                                    SK_ImGui_Toast::ShowNewest );
                    }
#endif
                  }
                }
              }
            }

            if ( config.input.gamepad.scepad.enhanced_ps_button &&
                                 pDevice->buttons.size () >= 12 &&
                      (config.input.gamepad.xinput.ui_slot >= 0 && 
                       config.input.gamepad.xinput.ui_slot <  4) )
            {
              if (   pDevice->buttons [12].state &&
                  (! pDevice->buttons [12].last_state) )
              {
                bool bToggleVis = false;
                bool bToggleNav = false;

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

            if (pDevice->bDualSense && config.input.gamepad.scepad.mute_applies_to_game)
            {
              if (   pDevice->buttons [14].state &&
                  (! pDevice->buttons [14].last_state) )
              {
                SK_SetGameMute (! SK_IsGameMuted ());
              }
            }

            if ( pDevice->bDualSense ||
                 pDevice->bDualShock4 )
            {
              if (pDevice->buttons [ 0].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_X;
              if (pDevice->buttons [ 1].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_A;
              if (pDevice->buttons [ 2].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_B;
              if (pDevice->buttons [ 3].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_Y;

              if (pDevice->buttons [ 4].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
              if (pDevice->buttons [ 5].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
              if (pDevice->buttons [ 6].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;
              if (pDevice->buttons [ 7].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;

              if (pDevice->buttons [ 8].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;
              if (pDevice->buttons [ 9].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_START;

              if (pDevice->buttons [10].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
              if (pDevice->buttons [11].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
              if (pDevice->buttons [12].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_GUIDE;
            }

            // Dual Shock 3
            else if (pDevice->bDualShock3)
            {
              if (pDevice->buttons [ 0].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_Y;
              if (pDevice->buttons [ 1].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_B;
              if (pDevice->buttons [ 2].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_A;
              if (pDevice->buttons [ 3].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_X;

              if (pDevice->buttons [ 4].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;
              if (pDevice->buttons [ 5].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;
              if (pDevice->buttons [ 6].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
              if (pDevice->buttons [ 7].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

              if (pDevice->buttons [ 8].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_START;
              if (pDevice->buttons [ 9].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;

              if (pDevice->buttons [10].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
              if (pDevice->buttons [11].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
              if (pDevice->buttons [12].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_GUIDE;
            }

            //pDevice->bHasPlayStation = true;

            if ( memcmp ( &pDevice->xinput.prev_report.Gamepad,
                          &pDevice->xinput.     report.Gamepad, sizeof (XINPUT_GAMEPAD)) )
            {
              pDevice->xinput.report.dwPacketNumber++;
              pDevice->xinput.prev_report = pDevice->xinput.report;
            }

            ResetEvent (pDevice->hInputEvent);
          }
        }
      }

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] HID Input Report Consumer", this);
  }

  return true;
}

bool
SK_HID_PlayStationDevice::write_output_report (void)
{
  if (! bConnected)
    return false;

  if (bDualSense)
  {
    SK_ReleaseAssert (
      output_report.size () >= sizeof (SK_HID_DualSense_SetStateData)
    );

    if (output_report.size () < sizeof (SK_HID_DualSense_SetStateData))
      return false;

    if (hOutputEnqueued== nullptr)
    {
      hOutputEnqueued =
        SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

      hOutputFinished =
        SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

      SK_Thread_CreateEx ([](LPVOID pData)->DWORD
      {
        SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);

        SK_HID_PlayStationDevice* pDevice =
          (SK_HID_PlayStationDevice *)pData;

        HANDLE hEvents [] = {
          __SK_DLL_TeardownEvent,
           pDevice->hOutputFinished,
           pDevice->hOutputEnqueued
        };

        OVERLAPPED async_output_request = { };

        bool bEnqueued = false;
        bool bFinished = false;

        DWORD  dwWaitState  = WAIT_FAILED;
        while (dwWaitState != WAIT_OBJECT_0)
        {
          dwWaitState =
            WaitForMultipleObjects ( _countof (hEvents),
                                               hEvents, FALSE, INFINITE );

          if (dwWaitState == WAIT_OBJECT_0)
            break;

          if (dwWaitState == (WAIT_OBJECT_0 + 2))
          {
            ResetEvent (pDevice->hOutputEnqueued);
            bEnqueued = true;
          }

          if (dwWaitState == (WAIT_OBJECT_0 + 1))
          {
            ResetEvent (pDevice->hOutputFinished);
            bFinished = true;
          }

          if (! (bEnqueued && bFinished))
          {
            continue;
          }

          ZeroMemory ( pDevice->output_report.data (),
                       pDevice->output_report.size () );

          // Report Type
          pDevice->output_report [0] = 0x02;

          BYTE* pOutputRaw =
            (BYTE *)pDevice->output_report.data ();

          SK_HID_DualSense_SetStateData* output =
            (SK_HID_DualSense_SetStateData *)&pOutputRaw [1];

          output->EnableRumbleEmulation    = true;
          output->UseRumbleNotHaptics      = true;
          output->AllowMuteLight           = true;

          if (false)
          {
            // SK's not really interested in this...
            output->AllowLedColor          = true;
          }

          output->AllowHapticLowPassFilter = true;
          output->AllowMotorPowerLevel     = true;

          // Firmware reqs
          output->
             EnableImprovedRumbleEmulation = true;

#if 1
          if ( pDevice->_vibration.last_set >= SK::ControlPanel::current_time -
               pDevice->_vibration.MAX_TTL_IN_MSECS )
          {
#endif
            output->RumbleEmulationRight =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.right)
              );
            output->RumbleEmulationLeft  =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.left)
              );
#if 1
          }

          else
          {
            output->RumbleEmulationRight = 0;
            output->RumbleEmulationLeft  = 0;

            WriteULongRelease (&pDevice->_vibration.left,  0);
            WriteULongRelease (&pDevice->_vibration.right, 0);
          }

          if (std::exchange (pDevice->_vibration.last_left,  output->RumbleEmulationLeft ) != output->RumbleEmulationLeft  || output->RumbleEmulationLeft  == 0)
            pDevice->_vibration.last_output = pDevice->_vibration.last_set;
          if (std::exchange (pDevice->_vibration.last_right, output->RumbleEmulationRight) != output->RumbleEmulationRight || output->RumbleEmulationRight == 0)
            pDevice->_vibration.last_output = pDevice->_vibration.last_set;
#endif

          static bool       bMuted     = SK_IsGameMuted ();
          static DWORD dwLastMuteCheck = SK_timeGetTime ();

          if (dwLastMuteCheck < SK::ControlPanel::current_time - 750UL)
          {   dwLastMuteCheck = SK::ControlPanel::current_time;
                   bMuted     = SK_IsGameMuted ();
                   // This API is rather expensive
          }

          output->MuteLightMode =
                 bMuted               ?
               (! game_window.active) ? Breathing
                                      : On
                                      : Off;

          output->TouchPowerSave      = true;
          output->MotionPowerSave     = true;
          output->AudioPowerSave      = true;
          output->HapticLowPassFilter = true;

          output->RumbleMotorPowerReduction =
            config.input.gamepad.scepad.rumble_power_level == 100.0f ? 0
                                                                     :
            static_cast <uint8_t> ((100.0f - config.input.gamepad.scepad.rumble_power_level) / 12.5f);

  //      pOutputRaw [ 9] = 0;
  //      pOutputRaw [39] = 2;
	//      pOutputRaw [42] = 2;

	        pOutputRaw [45] = pDevice->_color.r;
	        pOutputRaw [46] = pDevice->_color.g;
	        pOutputRaw [47] = pDevice->_color.b;

          async_output_request        = { };
          async_output_request.hEvent = pDevice->hOutputFinished;

          BOOL bWriteAsync =
            SK_WriteFile ( pDevice->hDeviceFile, pDevice->output_report.data (),
                            static_cast <DWORD> (pDevice->output_report.size ()), nullptr, &async_output_request );

          if (! bWriteAsync)
          {
            DWORD dwLastErr =
                 GetLastError ();

            // Invalid Handle and Device Not Connected get a 250 ms cooldown
            if ( dwLastErr != NOERROR              &&
                 dwLastErr != ERROR_IO_PENDING     &&
                 dwLastErr != ERROR_INVALID_HANDLE &&
                 dwLastErr != ERROR_NO_SUCH_DEVICE &&
                 dwLastErr != ERROR_DEVICE_NOT_CONNECTED )
            {
              _com_error err (
                _com_error::WCodeToHRESULT (
                  sk::narrow_cast <WORD> (dwLastErr)
                )
              );

              SK_LOGs0 (
                L"InputBkEnd",
                L"SK_WriteFile ({%ws}) failed, Error=%d [%ws]",
                  pDevice->wszDevicePath, dwLastErr,
                    err.ErrorMessage ()
              );
            }

            if (dwLastErr != ERROR_IO_PENDING)
            {
              if (dwLastErr != ERROR_INVALID_HANDLE)
                SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

              SK_SleepEx (250UL, TRUE); // Prevent runaway CPU usage on failure

              SetEvent (pDevice->hOutputFinished);
              continue;
            }

            else
            {
              WaitForSingleObject (pDevice->hOutputFinished, INFINITE);
              continue;
            }
          }

          pDevice->ulLastFrameOutput =
                   SK_GetFramesDrawn ();

          pDevice->dwLastTimeOutput =
            SK::ControlPanel::current_time;

          bEnqueued = false;
          bFinished = false;
        } while (true);

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] HID Output Report Producer", this);
    }

    else
      SetEvent (hOutputEnqueued);

    return true;
  }

  return false;
}

#if 0
bool
SK_HID_PlayStationDevice::write_output_report (void)
{
  if (! bConnected)
    return false;

  if (bDualSense)
  {
    SK_ReleaseAssert (
      output_report.size () >= sizeof (SK_HID_DualSense_SetStateData)
    );

    if (output_report.size () < sizeof (SK_HID_DualSense_SetStateData))
      return false;

    if (hOutputEnqueued == nullptr)
    {
      hOutputEnqueued =
        SK_CreateEvent (nullptr, TRUE, FALSE, nullptr);
      hOutputFinished =
        SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

      SK_Thread_CreateEx ([](LPVOID pData)->DWORD
      {
        SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);

        SK_HID_PlayStationDevice* pDevice =
          (SK_HID_PlayStationDevice *)pData;

        HANDLE hEvents [] = {
          __SK_DLL_TeardownEvent,
           pDevice->hOutputEnqueued,
           pDevice->hOutputFinished
        };

        OVERLAPPED async_output_request = { };

        DWORD  dwWaitState  = WAIT_FAILED;
        while (dwWaitState != WAIT_OBJECT_0)
        {
          async_output_request        = { };
          async_output_request.hEvent = pDevice->hOutputFinished;

          dwWaitState =
            WaitForMultipleObjects ( _countof (hEvents),
                                               hEvents, FALSE, INFINITE );

          // Application Exiting
          if (dwWaitState == WAIT_OBJECT_0)
            break;

          //if (dwWaitState == (WAIT_OBJECT_0 + 1))
          //{
          //  while ( pDevice->_vibration.last_set == pDevice->_vibration.last_output &&
          //         (pDevice->ulLastFrameOutput == SK_GetFramesDrawn () ||
          //          pDevice->dwLastTimeOutput > SK::ControlPanel::current_time - 500) )
          //  {
          //    ResetEvent (pDevice->hOutputEnqueued);
          //
          //    DWORD dwInnerWait =
          //      WaitForMultipleObjects (_countof (hEvents), hEvents, FALSE, 33UL);
          //
          //    if (dwInnerWait == WAIT_OBJECT_0)
          //    {   dwWaitState  = WAIT_OBJECT_0;
          //      break;
          //    }
          //  }
          //}

          //if (dwWaitState != (WAIT_OBJECT_0 + 1))
          //{
          //  ResetEvent (pDevice->hOutputEvent);
          //  continue;
          //}

          BYTE* pOutputRaw =
            (BYTE *)pDevice->output_report.data ();

          SK_HID_DualSense_SetStateData* output =
            (SK_HID_DualSense_SetStateData *)&pOutputRaw [1];

          // Report Type
          pOutputRaw [0] = 0x02;

          output->EnableRumbleEmulation    = true;
          output->UseRumbleNotHaptics      = true;
          output->AllowMuteLight           = true;

          if (false)
          {
            // SK's not really interested in this...
            output->AllowLedColor          = true;
          }

          output->AllowHapticLowPassFilter = true;
          output->AllowMotorPowerLevel     = true;

          // Firmware reqs
          output->
             EnableImprovedRumbleEmulation = true;

#if 1
          if ( pDevice->_vibration.last_set >= SK::ControlPanel::current_time -
               pDevice->_vibration.MAX_TTL_IN_MSECS )
          {
#endif
            output->RumbleEmulationRight =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.right)
              );
            output->RumbleEmulationLeft  =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.left)
              );
#if 1
          }

          else
          {
            output->RumbleEmulationRight = 0;
            output->RumbleEmulationLeft  = 0;

            WriteULongRelease (&pDevice->_vibration.left,  0);
            WriteULongRelease (&pDevice->_vibration.right, 0);
          }

          if (std::exchange (pDevice->_vibration.last_left,  output->RumbleEmulationLeft ) != output->RumbleEmulationLeft  || output->RumbleEmulationLeft  == 0)
            pDevice->_vibration.last_output = pDevice->_vibration.last_set;
          if (std::exchange (pDevice->_vibration.last_right, output->RumbleEmulationRight) != output->RumbleEmulationRight || output->RumbleEmulationRight == 0)
            pDevice->_vibration.last_output = pDevice->_vibration.last_set;
#endif

          static bool       bMuted     = SK_IsGameMuted ();
          static DWORD dwLastMuteCheck = SK_timeGetTime ();

          if (dwLastMuteCheck < SK::ControlPanel::current_time - 750UL)
          {   dwLastMuteCheck = SK::ControlPanel::current_time;
                   bMuted     = SK_IsGameMuted ();
                   // This API is rather expensive
          }

          output->MuteLightMode =
                 bMuted               ?
               (! game_window.active) ? Breathing
                                      : On
                                      : Off;

          output->TouchPowerSave      = true;
          output->MotionPowerSave     = true;
          output->AudioPowerSave      = true;
          output->HapticLowPassFilter = true;

          output->RumbleMotorPowerReduction =
            config.input.gamepad.scepad.rumble_power_level == 100.0f ? 0
                                                                     :
            static_cast <uint8_t> ((100.0f - config.input.gamepad.scepad.rumble_power_level) / 12.5f);

  //      pOutputRaw [ 9] = 0;
  //      pOutputRaw [39] = 2;
	//      pOutputRaw [42] = 2;

	        pOutputRaw [45] = pDevice->_color.r;
	        pOutputRaw [46] = pDevice->_color.g;
	        pOutputRaw [47] = pDevice->_color.b;

          // Queued Output Report Finished
          if (dwWaitState == (WAIT_OBJECT_0 + 2))
          {
            ResetEvent (pDevice->hOutputFinished);

            BOOL  bRet =
              SK_WriteFile ( pDevice->hDeviceFile, pOutputRaw,
                  static_cast <DWORD> ( pDevice->output_report.size () ),
                            nullptr, &async_output_request );

            if (! bRet)
            {
              DWORD dwLastErr =
                   GetLastError ();

              if ( dwLastErr != NOERROR              &&
                   dwLastErr != ERROR_IO_PENDING     &&
                   dwLastErr != ERROR_INVALID_HANDLE &&
                   dwLastErr != ERROR_DEVICE_NOT_CONNECTED )
              {
                SK_ImGui_CreateNotification (
                  "XInput.Emulation.DebugWrite", SK_ImGui_Toast::Error,
                    SK_FormatString ("SK_WriteFile (...) failed with code=%d", GetLastError ()).c_str (),
                                     "HID Debug", INFINITE, SK_ImGui_Toast::UseDuration |
                                                            SK_ImGui_Toast::ShowCaption |
                                                            SK_ImGui_Toast::ShowTitle   |
                                                            SK_ImGui_Toast::ShowNewest );
              }

              if (dwLastErr != ERROR_IO_PENDING)
              {
                if (dwLastErr != ERROR_INVALID_HANDLE)
                  SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

                SK_SleepEx (250UL, TRUE); // Prevent runaway CPU usage on failure
              }

              // Try again the next iteration...
              SetEvent (pDevice->hOutputFinished);

              continue;
            }

            pDevice->ulLastFrameOutput =
                     SK_GetFramesDrawn ();

            pDevice->dwLastTimeOutput =
              SK::ControlPanel::current_time;

            ResetEvent (pDevice->hOutputEnqueued);
          }

#if 0
          if (ReadULongAcquire (&pDevice->_vibration.left ) != 0 ||
              ReadULongAcquire (&pDevice->_vibration.right) != 0)
          {
            SK_ImGui_CreateNotification (
              "XInput.Emulation.Debug", SK_ImGui_Toast::Info,
                SK_FormatString ("Left Motor: %d\r\nRight Motor:%d\r\n",
                                 ReadULongAcquire (&pDevice->_vibration.left ),
                                 ReadULongAcquire (&pDevice->_vibration.right)).c_str (),
                                 "Vibration Debug", 15000, SK_ImGui_Toast::UseDuration |
                                                           SK_ImGui_Toast::ShowCaption |
                                                           SK_ImGui_Toast::ShowTitle   |
                                                           SK_ImGui_Toast::ShowNewest );
          }
#endif
        }

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] HID Output Report Thread", this);
    }

    else
      SetEvent (hOutputEnqueued);

    return true;
  }

  return false;
}
#endif