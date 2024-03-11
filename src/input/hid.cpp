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
#include <imgui/font_awesome.h>

#include <hidclass.h>
#include <bluetoothapis.h>
#include <bthdef.h>
#include <bthioctl.h>

#pragma comment (lib, "bthprops.lib")

//
// From the Driver SDK, which we would rather not make a dependency
//   required to build Special K...    (hidport.h)
//
#define IOCTL_HID_GET_STRING                     HID_CTL_CODE(4)
#define IOCTL_HID_ACTIVATE_DEVICE                HID_CTL_CODE(7)
#define IOCTL_HID_DEACTIVATE_DEVICE              HID_CTL_CODE(8)
#define IOCTL_HID_GET_DEVICE_ATTRIBUTES          HID_CTL_CODE(9)
#define IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST HID_CTL_CODE(10)

typedef struct _HID_DEVICE_ATTRIBUTES {
  ULONG  Size;
  USHORT VendorID;
  USHORT ProductID;
  USHORT VersionNumber;
  USHORT Reserved [11];
} HID_DEVICE_ATTRIBUTES,
*PHID_DEVICE_ATTRIBUTES;

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Input Mgr."

#define SK_HID_READ(type)  SK_HID_Backend->markRead   (type);
#define SK_HID_WRITE(type) SK_HID_Backend->markWrite  (type);
#define SK_HID_VIEW(type)  SK_HID_Backend->markViewed (type);
#define SK_HID_HIDE(type)  SK_HID_Backend->markHidden (type);


//////////////////////////////////////////////////////////////
//
// HIDClass (User mode)
//
//////////////////////////////////////////////////////////////
HidD_GetPreparsedData_pfn      HidD_GetPreparsedData_Original  = nullptr;
HidD_FreePreparsedData_pfn     HidD_FreePreparsedData_Original = nullptr;
HidD_GetFeature_pfn            HidD_GetFeature_Original        = nullptr;
HidD_SetFeature_pfn            HidD_SetFeature_Original        = nullptr;
HidP_GetData_pfn               HidP_GetData_Original           = nullptr;
HidP_GetCaps_pfn               HidP_GetCaps_Original           = nullptr;
HidP_GetUsages_pfn             HidP_GetUsages_Original         = nullptr;

HidD_GetAttributes_pfn         SK_HidD_GetAttributes           = nullptr;
HidD_GetSerialNumberString_pfn SK_HidD_GetSerialNumberString   = nullptr;
HidD_GetPreparsedData_pfn      SK_HidD_GetPreparsedData        = nullptr;
HidD_FreePreparsedData_pfn     SK_HidD_FreePreparsedData       = nullptr;
HidD_GetInputReport_pfn        SK_HidD_GetInputReport          = nullptr;
HidD_GetFeature_pfn            SK_HidD_GetFeature              = nullptr;
HidD_SetFeature_pfn            SK_HidD_SetFeature              = nullptr;
HidD_FlushQueue_pfn            SK_HidD_FlushQueue              = nullptr;
HidP_GetData_pfn               SK_HidP_GetData                 = nullptr;
HidP_GetCaps_pfn               SK_HidP_GetCaps                 = nullptr;
HidP_GetButtonCaps_pfn         SK_HidP_GetButtonCaps           = nullptr;
HidP_GetValueCaps_pfn          SK_HidP_GetValueCaps            = nullptr;
HidP_GetUsages_pfn             SK_HidP_GetUsages               = nullptr;
HidP_GetUsageValue_pfn         SK_HidP_GetUsageValue           = nullptr;
HidP_GetUsageValueArray_pfn    SK_HidP_GetUsageValueArray      = nullptr;



enum class SK_Input_DeviceFileType
{
  None    = 0,
  HID     = 1,
  NVIDIA  = 2,
  Invalid = 4
};


struct SK_HID_OverlappedRequest {
  HANDLE hFile                 = INVALID_HANDLE_VALUE;
  DWORD  dwNumberOfBytesToRead = 0;
  LPVOID lpBuffer              = nullptr;
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
  sk_input_dev_type device_type                        = sk_input_dev_type::Other;
  USHORT            device_vid                         = 0x0;
  USHORT            device_pid                         = 0x0;
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
      hidpCaps            = known_paths.at (wszPath).hidpCaps;

      wcsncpy_s ( wszDevicePath, MAX_PATH,
                  wszPath,       _TRUNCATE );

      wcsncpy_s (                          wszProductName,      128,
                  known_paths.at (wszPath).wszProductName,           _TRUNCATE );
      wcsncpy_s (                          wszManufacturerName, 128,
                  known_paths.at (wszPath).wszManufacturerName,      _TRUNCATE );
      wcsncpy_s (                          wszSerialNumber,     128,
                  known_paths.at (wszPath).wszSerialNumber,          _TRUNCATE );

      device_type         = known_paths.at (wszPath).device_type;
      device_vid          = known_paths.at (wszPath).device_vid;
      device_pid          = known_paths.at (wszPath).device_pid;
      bDisableDevice      = known_paths.at (wszPath).bDisableDevice;

      hFile = file;
    }

    else
    {
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
          hFile       = file;
#endif

          DWORD dwBytesRead = 0;

          if (hidpCaps.UsagePage == HID_USAGE_PAGE_GENERIC)
          {
            switch (hidpCaps.Usage)
            {
              case HID_USAGE_GENERIC_GAMEPAD:
              case HID_USAGE_GENERIC_JOYSTICK:
              case HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER:
              {
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

#ifdef DEBUG
                SK_ImGui_Warning (wszSerialNumber);
#endif

                HIDD_ATTRIBUTES hidAttribs      = {                      };
                                hidAttribs.Size = sizeof (HIDD_ATTRIBUTES);

                SK_HidD_GetAttributes (hFile, &hidAttribs);

                device_pid = hidAttribs.ProductID;
                device_vid = hidAttribs.VendorID;

                device_type = sk_input_dev_type::Gamepad;

                LONG lImmediate = 0;

                SK_DeviceIoControl (
                  hFile, IOCTL_HID_SET_POLL_FREQUENCY_MSEC,
                    &lImmediate, sizeof (LONG), nullptr, 0, &dwBytesRead, nullptr );

                SK_ImGui_CreateNotification (
                  "HID.GamepadAttached", SK_ImGui_Toast::Info,
                  *wszManufacturerName != L'\0' ?
                    SK_FormatString ("%ws: %ws\r\n\tVID: 0x%04x | PID: 0x%04x",
                       wszManufacturerName,
                       wszProductName, device_vid,
                                       device_pid ).c_str () :
                    SK_FormatString ("Generic Driver: %ws\r\n\tVID: 0x%04x | PID: 0x%04x",
                       wszProductName, device_vid,
                                       device_pid ).c_str (),
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
              if (config.system.log_level > 0)
              {
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

                SK_ImGui_Warning (wszSerialNumber);

                SK_LOGi1 (
                  L"Unknown HID Device Type (Product=%ws):  UsagePage=%x, Usage=%x",
                    wszProductName, hidpCaps.UsagePage, hidpCaps.Usage
                );
              }
            }
          }

          SK_ReleaseAssert ( // WTF?
            SK_HidD_FreePreparsedData (preparsed_data) != FALSE
          );
        }
      }

      known_paths.insert ({ wszPath, *this });
    }
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

  int neutralizeHidInput (uint8_t report_id);
  int remapHidInput      (void);

  std::vector<BYTE> last_data_read;
  concurrency::concurrent_unordered_map <LPOVERLAPPED, SK_HID_OverlappedRequest> _overlappedRequests;
  concurrency::concurrent_unordered_map <DWORD,        std::vector <byte>>       _cachedInputReportsByReportId;
  concurrency::concurrent_unordered_map <DWORD,        std::vector <byte>>       _blockedInputReportsBySize;
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
  // Unnecessary for now, since we only use a single list of HID
  //   devices...
#ifdef SK_HAS_DIFFERENT_DEVICE_CLASSES
  // Bloom filter since most file reads -are not- for input devices
  if ( SK_Input_DeviceFiles.find (hFile) ==
       SK_Input_DeviceFiles.cend (     ) )
  {
    return
      { SK_Input_DeviceFileType::None, nullptr, true };
  }
#endif

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

#ifdef SK_HAS_DIFFERENT_DEVICE_CLASSES
  else if ( const auto nv_iter  = SK_NVIDIA_DeviceFiles.find (hFile);
                       nv_iter != SK_NVIDIA_DeviceFiles.cend (     ) )
  {
    return
      { SK_Input_DeviceFileType::NVIDIA, &nv_iter->second, true };
  }
#endif

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
  
// IOCTL defines.
#define BTH_IOCTL_BASE      0

#define BTH_CTL(id)         CTL_CODE(FILE_DEVICE_BLUETOOTH,  \
               (id),                   \
               METHOD_BUFFERED,        \
               FILE_ANY_ACCESS)
#define BTH_KERNEL_CTL(id)  CTL_CODE(FILE_DEVICE_BLUETOOTH,  \
                      (id),                   \
                      METHOD_NEITHER,         \
                      FILE_ANY_ACCESS)
#define IOCTL_BTH_GET_DEVICE_INFO           BTH_CTL(BTH_IOCTL_BASE+0x02)
#define IOCTL_BTH_DISCONNECT_DEVICE         BTH_CTL(BTH_IOCTL_BASE+0x03)

void SK_Bluetooth_SetupPowerOff (void)
{
  class SK_Bluetooth_PowerListener : public SK_IVariableListener
  {
  public:
    SK_Bluetooth_PowerListener (void)
    {
      SK_GetCommandProcessor ()->AddVariable (
        "Input.Gamepad.PowerOff", SK_CreateVar (SK_IVariable::Boolean, &state, this)
      );
    }

    bool OnVarChange (SK_IVariable* var, void* val = nullptr)
    {
      std::ignore = val;

      if (var->getValuePointer () == &state)
      {
        std::scoped_lock <std::mutex> _(mutex);

        // For proper operation, this should always be serialized onto a single
        //   thread; in other words, use SK_DeferCommand (...) to change this.
        for ( auto& device : SK_HID_PlayStationControllers )
        {
          if (device.bBluetooth && device.bConnected)
          { 
            // The serial number is actually the Bluetooth MAC address; handy...
            wchar_t wszSerialNumber [32] = { };
 
            DWORD dwBytesReturned = 0;
 
            SK_DeviceIoControl (
              device.hDeviceFile, IOCTL_HID_GET_SERIALNUMBER_STRING, 0, 0,
                                                                     wszSerialNumber, 64,
                                                                    &dwBytesReturned, nullptr );

            SK_LOGi0 ( L"Attempting to disconnect Bluetooth MAC Address: %ws",
                         wszSerialNumber );

            ULONGLONG                           ullHWAddr = 0x0;
            swscanf (wszSerialNumber, L"%llx", &ullHWAddr);

            BLUETOOTH_FIND_RADIO_PARAMS findParams =
              { .dwSize = sizeof (BLUETOOTH_FIND_RADIO_PARAMS) };

            HANDLE hBtRadio    = INVALID_HANDLE_VALUE;
            HANDLE hFindRadios =
              BluetoothFindFirstRadio (&findParams, &hBtRadio);

            BOOL   success  = FALSE;
            while (success == FALSE && hBtRadio != 0)
            {
              success =
                SK_DeviceIoControl (
                  hBtRadio, IOCTL_BTH_DISCONNECT_DEVICE, &ullHWAddr, 8,
                                                         nullptr,    0,
                                       &dwBytesReturned, nullptr );

              CloseHandle (hBtRadio);

              if (! success)
                if (! BluetoothFindNextRadio (hFindRadios, &hBtRadio))
                  hBtRadio = 0;
            }

            BluetoothFindRadioClose (hFindRadios);
          }
        }
      }

      return true;
    }

    std::mutex mutex;
    bool       state;
  } static power;
}

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

        HANDLE hDeviceFile (
            SK_CreateFileW ( wszFileName, FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                          FILE_SHARE_READ   | FILE_SHARE_WRITE,
                                            nullptr, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH  |
                                                                    FILE_ATTRIBUTE_TEMPORARY |
                                                                    FILE_FLAG_OVERLAPPED, nullptr )
                           );

        HIDD_ATTRIBUTES hidAttribs      = {                      };
                        hidAttribs.Size = sizeof (HIDD_ATTRIBUTES);

        SK_HidD_GetAttributes (hDeviceFile, &hidAttribs);
  
        bool bSONY = 
          hidAttribs.VendorID == 0x54c ||
          hidAttribs.VendorID == 0x2054c;
          // The 0x2054c VID comes from Bluetooth service discovery.
          //
          //   * It should never actually show up through a call to
          //       HidD_GetAttributes (...); only in UNC paths.

        if (bSONY)
        {
          SK_HID_PlayStationDevice controller;

          controller.pid = hidAttribs.ProductID;
          controller.vid = hidAttribs.VendorID;

          controller.bBluetooth =
            StrStrIW (
              wszFileName, //Bluetooth_Base_UUID
                           L"{00001124-0000-1000-8000-00805f9b34fb}"
            );

          controller.bDualSense =
            (controller.pid == 0x0DF2) ||
            (controller.pid == 0x0CE6);

          controller.bDualShock4 =
            (controller.pid == 0x05C4) ||
            (controller.pid == 0x09CC) ||
            (controller.pid == 0x0BA0);

          controller.bDualShock3 =
            (controller.pid == 0x0268);

          if (! (controller.bDualSense || controller.bDualShock4 || controller.bDualShock3))
          {
            SK_LOGi0 (L"SONY Controller with Unknown PID ignored: %ws", wszFileName);
            continue;
          }
  
          wcsncpy_s (controller.wszDevicePath, MAX_PATH,
                                wszFileName,   _TRUNCATE);

          controller.hDeviceFile =
                     hDeviceFile;
  
          if (controller.hDeviceFile != INVALID_HANDLE_VALUE)
          {
            if (! SK_HidD_GetPreparsedData (controller.hDeviceFile, &controller.pPreparsedData))
            	continue;

#ifdef DEBUG
            if (controller.bBluetooth)
              SK_ImGui_Warning (L"Bluetooth");
#endif

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

            if (num_caps > 2)
            {
              SK_LOGi0 (
                L"PlayStation Controller has too many button sets (%d);"
                L" will ignore Device=%ws", num_caps, wszFileName
              );

              continue;
            }

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
  
            SK_HID_PlayStationControllers.push_back (controller);
          }
        }
      }
    }
  
    SK_SetupDiDestroyDeviceInfoList (hid_device_set);
  }
}



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
    if (SK_HidD_GetPreparsedData (HidDeviceObject, &pData))
    {
      if (SK_HID_FilterPreparsedData (pData))
        filter = true;

      SK_HidD_FreePreparsedData (pData);
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

CreateFileW_pfn                     SK_CreateFileW = nullptr;
CreateFile2_pfn                     SK_CreateFile2 = nullptr;
ReadFile_pfn                           SK_ReadFile = nullptr;
WriteFile_pfn                         SK_WriteFile = nullptr;
CancelIoEx_pfn                       SK_CancelIoEx = nullptr;
GetOverlappedResult_pfn     SK_GetOverlappedResult = nullptr;
GetOverlappedResultEx_pfn SK_GetOverlappedResultEx = nullptr;

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

bool
SK_UNTRUSTED_memcpy (void* dst, void* src, size_t size)
{
  __try {
    memcpy (dst, src, size);

    return true;
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return false;
  }
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

  // Fast path, we only care about HID Input Reports
  if (nNumberOfBytesToRead <= 31 || nNumberOfBytesToRead >= 4096)
  {
    return
      ReadFile_Original (
            hFile, lpBuffer, nNumberOfBytesToRead,
              lpNumberOfBytesRead, lpOverlapped
      );
  }

  const auto &[ dev_file_type, dev_ptr, dev_allowed ] =
    SK_Input_GetDeviceFileAndState (hFile);

  switch (dev_file_type)
  {
    case SK_Input_DeviceFileType::HID:
    {
      if (config.input.gamepad.disable_hid)
      {
        SetLastError (ERROR_DEVICE_NOT_CONNECTED);
        return FALSE;
      }

      auto hid_file =
        (SK_HID_DeviceFile *)dev_ptr;

      BYTE report_id = 0;

      if (lpBuffer != nullptr)
      {
        report_id = ((uint8_t *)lpBuffer)[0];
      }

      void* pBuffer = lpBuffer;

      if (! dev_allowed)
      {
        if (nNumberOfBytesToRead > 0)
        {
          if (        hid_file->_blockedInputReportsBySize [nNumberOfBytesToRead].size () < nNumberOfBytesToRead)
                      hid_file->_blockedInputReportsBySize [nNumberOfBytesToRead].resize (nNumberOfBytesToRead);
            pBuffer = hid_file->_blockedInputReportsBySize [nNumberOfBytesToRead].data ();

          if (hid_file->_cachedInputReportsByReportId [report_id].size () >= nNumberOfBytesToRead)
          {
            if (hid_file->neutralizeHidInput (report_id))
            {
              if (                   lpBuffer != nullptr)
                SK_UNTRUSTED_memcpy (lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (), nNumberOfBytesToRead);
            }
          }
        }
      }

      BOOL bRet =
        ReadFile_Original (
          hFile, pBuffer, nNumberOfBytesToRead,
            lpNumberOfBytesRead, lpOverlapped
        );

      if (lpOverlapped != nullptr && bRet)
      {
        auto& overlapped = hid_file->_overlappedRequests [lpOverlapped];

        overlapped.dwNumberOfBytesToRead = nNumberOfBytesToRead;
        overlapped.hFile                 = hFile;
        overlapped.lpBuffer              = lpBuffer;
      }

#if 0
      SK_LOGi0 (
        L"ReadFile (%ws, %p, %d, %p, {overlapped: %p} => %hs",
          hid_file->wszDevicePath, lpBuffer, nNumberOfBytesToRead,
              lpNumberOfBytesRead, lpOverlapped, bRet ? "Success" : "Fail"
      );
      SK_LOGi0 ("Report ID: %x", report_id);
#endif

      if (bRet)
      {
        auto& cached_report =
          hid_file->_cachedInputReportsByReportId [report_id];

        if ((! lpOverlapped) && dev_allowed)
        {
          if (                   cached_report.size   ()          < nNumberOfBytesToRead)
                                 cached_report.resize (             nNumberOfBytesToRead);
            SK_UNTRUSTED_memcpy (cached_report.data   (), lpBuffer, nNumberOfBytesToRead);
        }

        bool bDeviceAllowed = dev_allowed;

        if (cached_report.size () < nNumberOfBytesToRead && lpOverlapped != nullptr && SK_GetFramesDrawn () > 15)
        {
          hid_file->_cachedInputReportsByReportId [report_id].resize (nNumberOfBytesToRead);
          hid_file->_cachedInputReportsByReportId [report_id].data ()[0] = ((uint8_t *)lpBuffer) [0];

          if (hid_file->neutralizeHidInput (report_id))
          {
            SK_UNTRUSTED_memcpy (
              lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (),
                  nNumberOfBytesToRead );
          }
        }

        if (! bDeviceAllowed)
        {
          SK_ReleaseAssert (lpBuffer != nullptr);

          SK_HID_HIDE (hid_file->device_type);

          if (lpOverlapped == nullptr) // lpNumberOfBytesRead MUST be non-null
          {
            if (cached_report.size () >= nNumberOfBytesToRead)
            {
              // Give the game old data, from before we started blocking stuff...
              int num_bytes_read =
                hid_file->neutralizeHidInput (report_id);
            
              //if (num_bytes_read == 0)
              //{
              //  num_bytes_read = nNumberOfBytesToRead;
              //}
            
              if (num_bytes_read > 0)
              {
                if (lpNumberOfBytesRead != nullptr)
                   *lpNumberOfBytesRead = num_bytes_read;
              }
            
              if (lpBuffer != nullptr && num_bytes_read > 0)
              {
                SK_UNTRUSTED_memcpy (
                  lpBuffer, cached_report.data (),
                 std::min ((int)nNumberOfBytesToRead,
                                      num_bytes_read)
                );
              }
            }
          }

          else
          {
            SK_RunOnce (
              SK_LOGi0 (L"ReadFile HID IO Cancelled")
            );

            if (cached_report.size () >= nNumberOfBytesToRead)
            {
              // Give the game old data, from before we started blocking stuff...
              int num_bytes_read =
                hid_file->neutralizeHidInput (report_id);

              //if (num_bytes_read == 0)
              //{
              //  num_bytes_read = nNumberOfBytesToRead;
              //}

              if (num_bytes_read > 0)
              {
                if (lpNumberOfBytesRead != nullptr)
                   *lpNumberOfBytesRead = num_bytes_read;
              }

              if (lpBuffer != nullptr && num_bytes_read > 0)
              {
                SK_UNTRUSTED_memcpy (
                  lpBuffer, cached_report.data (),
                 std::min ((int)nNumberOfBytesToRead,
                                      num_bytes_read)
                );
              }
            }
          }

          return bRet;
        }

        else
        {
          SK_HID_READ (hid_file->device_type);

          if ( lpOverlapped == nullptr && 
                 hid_file->_cachedInputReportsByReportId [report_id].size () < nNumberOfBytesToRead )
          {
            hid_file->_cachedInputReportsByReportId [report_id].resize (nNumberOfBytesToRead);
            hid_file->_cachedInputReportsByReportId [report_id].data ()[0] = report_id;

            //if (hid_file->neutralizeHidInput (report_id))
            {
              SK_UNTRUSTED_memcpy (
                hid_file->_cachedInputReportsByReportId [report_id].data (), lpBuffer,
                    nNumberOfBytesToRead );
            }
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

  if (nNumberOfBytesToRead <= 31 || nNumberOfBytesToRead >= 4096)
  {
    return
      bRet;
  }

  const auto &[ dev_file_type, dev_ptr, dev_allowed ] =
    SK_Input_GetDeviceFileAndState (hFile);

  switch (dev_file_type)
  {
    case SK_Input_DeviceFileType::HID:
    {
      if (config.input.gamepad.disable_hid)
      {
        SetLastError (ERROR_DEVICE_NOT_CONNECTED);
        return FALSE;
      }

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
      if (config.input.gamepad.disable_hid)
      {
        SetLastError (ERROR_NO_SUCH_DEVICE);

        CloseHandle (hRet);

        return INVALID_HANDLE_VALUE;
      }

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

        for (auto& overlapped_request : dev_file._overlappedRequests)
        {
          overlapped_request.second.dwNumberOfBytesToRead = 0;
          overlapped_request.second.hFile                 = INVALID_HANDLE_VALUE;
          overlapped_request.second.lpBuffer              = nullptr;
        }

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
      if (config.input.gamepad.disable_hid)
      {
        SetLastError (ERROR_NO_SUCH_DEVICE);

        CloseHandle (hRet);

        return INVALID_HANDLE_VALUE;
      }

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

        for (auto& overlapped_request : dev_file._overlappedRequests)
        {
          overlapped_request.second.dwNumberOfBytesToRead = 0;
          overlapped_request.second.lpBuffer              = nullptr;
        }

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
      if (config.input.gamepad.disable_hid)
      {
        SetLastError (ERROR_NO_SUCH_DEVICE);

        CloseHandle (hRet);

        return INVALID_HANDLE_VALUE;
      }

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

        for (auto& overlapped_request : dev_file._overlappedRequests)
        {
          overlapped_request.second.dwNumberOfBytesToRead = 0;
          overlapped_request.second.hFile                 = INVALID_HANDLE_VALUE;
          overlapped_request.second.lpBuffer              = nullptr;
        }

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
      if (config.input.gamepad.disable_hid)
      {
        SetLastError (ERROR_DEVICE_NOT_CONNECTED);

        CancelIoEx (hFile, lpOverlapped);

        return FALSE;
      }

      auto hid_file =
        (SK_HID_DeviceFile *)dev_ptr;

      BOOL bRet = TRUE;

      auto& overlapped_request =
        hid_file->_overlappedRequests [lpOverlapped];

      if (! dev_allowed)
      {
        SK_HID_HIDE (hid_file->device_type);

        SK_RunOnce (
          SK_LOGi0 (L"GetOverlappedResult HID IO Cancelled")
        );

        DWORD dwSize = 
          overlapped_request.dwNumberOfBytesToRead;

        uint8_t report_id = overlapped_request.lpBuffer != nullptr ?
                ((uint8_t *)overlapped_request.lpBuffer) [0]       : 0x0;

        if (dwSize != 0 && hid_file->_cachedInputReportsByReportId [report_id].size () >= dwSize)
        {
          if (hid_file->neutralizeHidInput (report_id))
          {
            SK_UNTRUSTED_memcpy (
              overlapped_request.lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (), dwSize
            );
          }
        }
      }

      bRet =
        GetOverlappedResultEx_Original (
          hFile, lpOverlapped, lpNumberOfBytesTransferred, dwMilliseconds, bWait
        );

      if (bRet != FALSE)
      {
        int report_id = overlapped_request.lpBuffer != nullptr ?
            ((uint8_t *)overlapped_request.lpBuffer)[0]        : 0x0;

        if (dev_allowed)
        {
          if (overlapped_request.dwNumberOfBytesToRead != 0)
          {
            auto& cached_report =
              hid_file->_cachedInputReportsByReportId [report_id];

            if (                     cached_report.size () < overlapped_request.dwNumberOfBytesToRead)
            {
              if (((uint8_t *)overlapped_request.lpBuffer)[0] == 0x0 || ((uint8_t *)overlapped_request.lpBuffer)[0] == report_id)
              {
                                     cached_report.resize (                              overlapped_request.dwNumberOfBytesToRead);
                SK_UNTRUSTED_memcpy (cached_report.data (), overlapped_request.lpBuffer, overlapped_request.dwNumberOfBytesToRead);
              }
            }
          }

          SK_HID_READ (hid_file->device_type);
          SK_HID_VIEW (hid_file->device_type);
          // We did the bulk of this processing in ReadFile_Detour (...)
          //   nothing to be done here for now.
        }

        overlapped_request.dwNumberOfBytesToRead = 0;
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
      if (config.input.gamepad.disable_hid)
      {
        SetLastError (ERROR_DEVICE_NOT_CONNECTED);

        CancelIoEx (hFile, lpOverlapped);

        return FALSE;
      }

      auto hid_file =
        (SK_HID_DeviceFile *)dev_ptr;

      BOOL bRet = TRUE;

      auto& overlapped_request =
        hid_file->_overlappedRequests [lpOverlapped];

      if (! dev_allowed)
      {
        SK_HID_HIDE (hid_file->device_type);

        SK_RunOnce (
          SK_LOGi0 (L"GetOverlappedResult HID IO Cancelled")
        );

        DWORD dwSize = 
          overlapped_request.dwNumberOfBytesToRead;

        uint8_t report_id = overlapped_request.lpBuffer != nullptr ?
                ((uint8_t *)overlapped_request.lpBuffer) [0]       : 0x0;

        if (dwSize != 0 && hid_file->_cachedInputReportsByReportId [report_id].size () >= dwSize)
        {
          if (hid_file->neutralizeHidInput (report_id))
          {
            SK_UNTRUSTED_memcpy (
              overlapped_request.lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (), dwSize
            );
          }
        }
      }

      bRet =
        GetOverlappedResult_Original (
          hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait
        );

      if (bRet != FALSE)
      {
        int report_id = overlapped_request.lpBuffer != nullptr ?
            ((uint8_t *)overlapped_request.lpBuffer)[0]        : 0x0;

        if (dev_allowed)
        {
          if (overlapped_request.dwNumberOfBytesToRead != 0)
          {
            auto& cached_report =
              hid_file->_cachedInputReportsByReportId [report_id];

            if (                     cached_report.size () < overlapped_request.dwNumberOfBytesToRead)
            {
              if (((uint8_t *)overlapped_request.lpBuffer)[0] == 0x0 || ((uint8_t *)overlapped_request.lpBuffer)[0] == report_id)
              {
                                     cached_report.resize (                              overlapped_request.dwNumberOfBytesToRead);
                SK_UNTRUSTED_memcpy (cached_report.data (), overlapped_request.lpBuffer, overlapped_request.dwNumberOfBytesToRead);
              }
            }
          }

          SK_HID_READ (hid_file->device_type);
          SK_HID_VIEW (hid_file->device_type);
          // We did the bulk of this processing in ReadFile_Detour (...)
          //   nothing to be done here for now.
        }

        overlapped_request.dwNumberOfBytesToRead = 0;
      }

      return bRet;
    } break;

    case SK_Input_DeviceFileType::NVIDIA:
    {
      SK_MessageBus_Backend->markRead (2);
    } break;
  }

  return
    GetOverlappedResult_Original (
      hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait
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
    if (! SK_GetModuleHandle (L"hid.dll"))
             SK_LoadLibraryW (L"hid.dll");

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

    SK_CreateFile2           = CreateFile2_Original;
    SK_CreateFileW           = CreateFileW_Original;
    SK_ReadFile              = ReadFile_Original;
    SK_GetOverlappedResult   = GetOverlappedResult_Original;
    SK_GetOverlappedResultEx = GetOverlappedResultEx_Original;

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
                               
  SK_HidD_GetAttributes =
    (HidD_GetAttributes_pfn)SK_GetProcAddress (hModHID,
    "HidD_GetAttributes");

  SK_HidD_GetSerialNumberString =
    (HidD_GetSerialNumberString_pfn)SK_GetProcAddress (hModHID,
    "HidD_GetSerialNumberString");

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

  SK_GetOverlappedResultEx =
    (GetOverlappedResultEx_pfn)SK_GetProcAddress (hModKernel32,
    "GetOverlappedResultEx");

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

#pragma pack(push,1)
template <int N> struct BTCRC {
  uint8_t  Buff [N-4];
  uint32_t CRC;
};
#pragma pack(pop)

#pragma pack(push,1)
template <int N> struct BTAudio {
  uint16_t FrameNumber;
  uint8_t  AudioTarget; // 0x02 speaker?, 0x24 headset?, 0x03 mic?
  uint8_t  SBCData [N-3];
};
#pragma pack(pop)

// Derived from (2024-02-07):
// 
//   https://controllers.fandom.com/wiki/Sony_DualSense#Likely_Interface
//
#pragma pack(push,1)
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
#pragma pack(pop)

#pragma pack(push,1)
struct SK_HID_DualSense_BtReport_0x31
{
  union {
    BTCRC <78> CRC;
    struct {
      uint8_t  ReportID;      // 0x31
      uint8_t  UNK1      : 1; // -+
      uint8_t  EnableHID : 1; //  | - these 3 bits seem to act as an enum
      uint8_t  UNK2      : 1; // -+
      uint8_t  UNK3      : 1;
      uint8_t  SeqNo     : 4; // increment for every write // we have no proof of this, need to see some PS5 captures
      SK_HID_DualSense_SetStateData
               State;
    } Data;
  };
};
#pragma pack(pop)

#pragma pack(push,1)
struct SK_HID_DualSense_GetSimpleStateDataBt // 9
{
/*0  */ uint8_t   LeftStickX;
/*1  */ uint8_t   LeftStickY;
/*2  */ uint8_t   RightStickX;
/*3  */ uint8_t   RightStickY;
/*4.0*/ Direction DPad           : 4;
/*4.4*/ uint8_t   ButtonSquare   : 1;
/*4.5*/ uint8_t   ButtonCross    : 1;
/*4.6*/ uint8_t   ButtonCircle   : 1;
/*4.7*/ uint8_t   ButtonTriangle : 1;
/*5.0*/ uint8_t   ButtonL1       : 1;
/*5.1*/ uint8_t   ButtonR1       : 1;
/*5.2*/ uint8_t   ButtonL2       : 1;
/*5.3*/ uint8_t   ButtonR2       : 1;
/*5.4*/ uint8_t   ButtonShare    : 1;
/*5.5*/ uint8_t   ButtonOptions  : 1;
/*5.6*/ uint8_t   ButtonL3       : 1;
/*5.7*/ uint8_t   ButtonR3       : 1;
/*6.1*/ uint8_t   ButtonHome     : 1;
/*6.2*/ uint8_t   ButtonPad      : 1;
/*6.3*/ uint8_t   Counter        : 6;
/*7  */ uint8_t   TriggerLeft;
/*8  */ uint8_t   TriggerRight;
// anything beyond this point, if set, is invalid junk data that was not cleared
};
#pragma pack(pop)

#pragma pack(push,1)
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
/* 9.3*/ uint8_t    UNK1                : 1; // appears unused
/* 9.4*/ uint8_t    ButtonLeftFunction  : 1; // DualSense Edge
/* 9.5*/ uint8_t    ButtonRightFunction : 1; // DualSense Edge
/* 9.6*/ uint8_t    ButtonLeftPaddle    : 1; // DualSense Edge
/* 9.7*/ uint8_t    ButtonRightPaddle   : 1; // DualSense Edge
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
#pragma pack(pop)

#pragma pack(push,1)
struct SK_HID_DualShock4_SetStateData // 47
{
  uint8_t EnableRumbleUpdate        : 1;
  uint8_t EnableLedUpdate           : 1;
  uint8_t EnableLedBlink            : 1;
  uint8_t EnableExtWrite            : 1;
  uint8_t EnableVolumeLeftUpdate    : 1;
  uint8_t EnableVolumeRightUpdate   : 1;
  uint8_t EnableVolumeMicUpdate     : 1;
  uint8_t EnableVolumeSpeakerUpdate : 1;
  uint8_t UNK_RESET1                : 1; // unknown reset, both set high by Remote Play
  uint8_t UNK_RESET2                : 1; // unknown reset, both set high by Remote Play
  uint8_t UNK1                      : 1;
  uint8_t UNK2                      : 1;
  uint8_t UNK3                      : 1;
  uint8_t UNKPad                    : 3;
  uint8_t Empty1;
  uint8_t RumbleRight;              // weak
  uint8_t RumbleLeft;               // strong
  uint8_t LedRed;
  uint8_t LedGreen;
  uint8_t LedBlue;
  uint8_t LedFlashOnPeriod;
  uint8_t LedFlashOffPeriod;
  uint8_t ExtDataSend [8];               // sent to I2C EXT port, stored in 8x8 byte block
  uint8_t VolumeLeft;                    // 0x00 - 0x4F inclusive
  uint8_t VolumeRight;                   // 0x00 - 0x4F inclusive
  uint8_t VolumeMic;                     // 0x00, 0x01 - 0x40 inclusive (0x00 is special behavior)
  uint8_t VolumeSpeaker;                 // 0x00 - 0x4F
  uint8_t UNK_AUDIO1                : 7; // clamped to 1-64 inclusive, appears to be set to 5 for audio
  uint8_t UNK_AUDIO2                : 1; // unknown, appears to be set to 1 for audio
  uint8_t Pad [8];
};
#pragma pack(pop)

#pragma pack(push,1)
struct SK_HID_DualShock4_SetStateDataBt // 74
{
  uint8_t EnableRumbleUpdate        : 1;
  uint8_t EnableLedUpdate           : 1;
  uint8_t EnableLedBlink            : 1;
  uint8_t EnableExtWrite            : 1;
  uint8_t EnableVolumeLeftUpdate    : 1;
  uint8_t EnableVolumeRightUpdate   : 1;
  uint8_t EnableVolumeMicUpdate     : 1;
  uint8_t EnableVolumeSpeakerUpdate : 1;
  uint8_t UNK_RESET1                : 1; // unknown reset, both set high by Remote Play
  uint8_t UNK_RESET2                : 1; // unknown reset, both set high by Remote Play
  uint8_t UNK1                      : 1;
  uint8_t UNK2                      : 1;
  uint8_t UNK3                      : 1;
  uint8_t UNKPad                    : 3;
  uint8_t Empty1;
  uint8_t RumbleRight;              // Weak
  uint8_t RumbleLeft;               // Strong
  uint8_t LedRed;
  uint8_t LedGreen;
  uint8_t LedBlue;
  uint8_t LedFlashOnPeriod;
  uint8_t LedFlashOffPeriod;
  uint8_t ExtDataSend [8];               // sent to I2C EXT port, stored in 8x8 byte block
  uint8_t VolumeLeft;                    // 0x00 - 0x4F inclusive
  uint8_t VolumeRight;                   // 0x00 - 0x4F inclusive
  uint8_t VolumeMic;                     // 0x00, 0x01 - 0x40 inclusive (0x00 is special behavior)
  uint8_t VolumeSpeaker;                 // 0x00 - 0x4F
  uint8_t UNK_AUDIO1                : 7; // clamped to 1-64 inclusive, appears to be set to 5 for audio
  uint8_t UNK_AUDIO2                : 1; // unknown, appears to be set to 1 for audio
  uint8_t Pad [52];
};
#pragma pack(pop)

#pragma pack(push,1)
struct SK_HID_DualShock4_ReportOut11 // 78
{
  union {
    BTCRC <78> CRC;
    struct {
      uint8_t ReportID;        // 0x11
      uint8_t PollingRate : 6; // note 0 appears to be clamped to 1
      uint8_t EnableCRC   : 1;
      uint8_t EnableHID   : 1;
      uint8_t EnableMic   : 3; // somehow enables mic, appears to be 3 bit flags
      uint8_t UnkA4       : 1;
      uint8_t UnkB1       : 1;
      uint8_t UnkB2       : 1; // seems to always be 1
      uint8_t UnkB3       : 1;
      uint8_t EnableAudio : 1;
      union {
        SK_HID_DualShock4_SetStateDataBt State;
        BTAudio <75> Audio;
      };
    } Data;
  };
};
#pragma pack(pop)

#pragma pack(push,1)
struct SK_HID_DualShock4_BasicGetStateData {
/*0  */ uint8_t   LeftStickX;
/*1  */ uint8_t   LeftStickY;
/*2  */ uint8_t   RightStickX;
/*3  */ uint8_t   RightStickY;
/*4.0*/ Direction DPad           : 4;
/*4.4*/ uint8_t   ButtonSquare   : 1;
/*4.5*/ uint8_t   ButtonCross    : 1;
/*4.6*/ uint8_t   ButtonCircle   : 1;
/*4.7*/ uint8_t   ButtonTriangle : 1;
/*5.0*/ uint8_t   ButtonL1       : 1;
/*5.1*/ uint8_t   ButtonR1       : 1;
/*5.2*/ uint8_t   ButtonL2       : 1;
/*5.3*/ uint8_t   ButtonR2       : 1;
/*5.4*/ uint8_t   ButtonShare    : 1;
/*5.5*/ uint8_t   ButtonOptions  : 1;
/*5.6*/ uint8_t   ButtonL3       : 1;
/*5.7*/ uint8_t   ButtonR3       : 1;
/*6.0*/ uint8_t   ButtonHome     : 1;
/*6.1*/ uint8_t   ButtonPad      : 1;
/*6.2*/ uint8_t   Counter        : 6; // always 0 on USB, counts up with some skips on BT
/*7  */ uint8_t   TriggerLeft;
/*8  */ uint8_t   TriggerRight;
};
#pragma pack(pop)

#pragma pack(push,1)
struct SK_HID_DualShock4_GetStateData : SK_HID_DualShock4_BasicGetStateData {
/* 9  */ uint16_t Timestamp; // in 5.33us units?
/*11  */ uint8_t  Temperature;
/*12  */ int16_t  AngularVelocityX;
/*14  */ int16_t  AngularVelocityZ;
/*16  */ int16_t  AngularVelocityY;
/*18  */ int16_t  AccelerometerX;
/*20  */ int16_t  AccelerometerY;
/*22  */ int16_t  AccelerometerZ;
/*24  */ uint8_t  ExtData [5]; // range can be set by EXT device
/*29  */ uint8_t  PowerPercent      : 4; // 0x00-0x0A or 0x01-0x0B if plugged int
/*29.4*/ uint8_t  PluggedPowerCable : 1;
/*29.5*/ uint8_t  PluggedHeadphones : 1;
/*29.6*/ uint8_t  PluggedMic        : 1;
/*29,7*/ uint8_t  PluggedExt        : 1;
/*30.0*/ uint8_t  UnkExt1           : 1; // ExtCapableOfExtraData?
/*30.1*/ uint8_t  UnkExt2           : 1; // ExtHasExtraData?
/*30.2*/ uint8_t  NotConnected      : 1; // Used by dongle to indicate no controller
/*30.3*/ uint8_t  Unk1              : 5;
/*31  */ uint8_t  Unk2; // unused?
/*32  */ uint8_t  TouchCount;
};
#pragma pack(pop)

#pragma pack(push,1)
struct USBGetStateData : SK_HID_DualShock4_GetStateData {
  TouchData TouchData [3];
  uint8_t   Pad       [3];
};
#pragma pack(pop)

#pragma pack(push,1)
struct BTGetStateData : SK_HID_DualShock4_GetStateData {
  TouchData TouchData [4];
  uint8_t Pad         [6];
};
#pragma pack(pop)

void
SK_HID_PlayStationDevice::setVibration (
  USHORT left,
  USHORT right,
  USHORT max_val )
{
  if (max_val == 0)
  {
    _vibration.max_val =
      std::max ( { _vibration.max_val, left, right } );

    if (_vibration.max_val > 255)
      max_val = 65535;
    else
      max_val = 255;
  }

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

        pDevice->input_report [0] = pDevice->bBluetooth ? 49
                                                        : pDevice->button_report_id;

        if (pDevice->bDualShock4)
          pDevice->input_report [0] = pDevice->bBluetooth ? 0x11 :
                                      pDevice->button_report_id;

        bool bHasBluetooth = false;

        for ( auto& ps_controller : SK_HID_PlayStationControllers )
        {
          if ( ps_controller.bConnected &&
               ps_controller.bBluetooth )
          {
            bHasBluetooth = true;
          }
        }


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
               dwLastErr != ERROR_NOT_READY      &&
               dwLastErr != ERROR_DEVICE_NOT_CONNECTED )
          {
            _com_error err (
              _com_error::WCodeToHRESULT (
                sk::narrow_cast <WORD> (dwLastErr)
              )
            );

            SK_LOGs0 (
              L"InputBkEnd",
              L"SK_ReadFile ({%ws}, InputReport) failed, Error=%d [%ws]",
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
          SK_CancelIoEx (pDevice->hDeviceFile, &async_input_request);

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
            DWORD dwLastErr =
              GetLastError ();

            // Stop polling on the first failure, if the device comes back to life, we'll probably
            //   get a device arrival notification.
            if (GetLastError () != NOERROR)
            {
              // We have an event that will be signalled when the device reconnects...
              if (dwLastErr != ERROR_DEVICE_NOT_CONNECTED)
              {
                _com_error err (dwLastErr);

                SK_LOGs0 (
                  L"InputBkEnd",
                  L"SK_GetOverlappedResult ({%ws}, InputReport) failed, Error=%d [%ws]",
                    pDevice->wszDevicePath, dwLastErr,
                      err.ErrorMessage ()
                );
              }

              continue;
            }
          }

          if (dwBytesTransferred != 0)
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

            ////if (SK_ImGui_WantGamepadCapture () || config.input.gamepad.xinput.emulate)
            ////{
            ////  //static DWORD dwLastFlush = 0;
            ////  //if (         dwLastFlush < SK::ControlPanel::current_time - 16)
            ////  //{            dwLastFlush = SK::ControlPanel::current_time;
            ////    pDevice->write_output_report ();
            ////  //}
            ////}

            ULONG num_usages =
              static_cast <ULONG> (pDevice->button_usages.size ());

            pDevice->xinput.report.Gamepad = { };

            if (dwBytesTransferred != 78 && (! (pDevice->bDualShock4 && pDevice->bBluetooth)))
            {
              if ( HIDP_STATUS_SUCCESS ==
                SK_HidP_GetUsages ( HidP_Input, pDevice->buttons [0].UsagePage, 0,
                                                pDevice->button_usages.data (),
                                                                &num_usages,
                                                pDevice->pPreparsedData,
                                         (PCHAR)pDevice->input_report.data  (),
                           static_cast <ULONG> (pDevice->input_report.size  ()) )
                 )
              {
                for ( UINT i = 0; i < num_usages; ++i )
                {
                  pDevice->buttons [
                    pDevice->button_usages [i] -
                    pDevice->buttons       [0].Usage
                  ].state = true;
                }
              }

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

              // Report 0x1, USB Mode
              if (dwBytesTransferred == 64)
              {
                SK_HID_DualSense_GetStateData* pData = 
                  (SK_HID_DualSense_GetStateData *)&(pDevice->input_report.data ()[1]);

                pDevice->battery.state =
                  (SK_HID_PlayStationDevice::PowerState)((((BYTE *)pData)[52] & 0xF0) >> 4);

                const float batteryPercent =
                  ( std::min (((((BYTE *)pData)[52] & 0xF) /*- ((((BYTE *)pData)[53] & 0x10) ? 1 : 0)*/) * 10.0f + 5.0f, 100.0f) );

                if (pDevice->battery.state == Discharging || 
                    pDevice->battery.state == Charging    ||
                    pDevice->battery.state == Complete)
                {
                  pDevice->battery.percentage = batteryPercent;
                }

                if (pDevice->buttons.size () < 19)
                    pDevice->buttons.resize (  19);

                pDevice->buttons [15].state = pData->ButtonLeftFunction  != 0;
                pDevice->buttons [16].state = pData->ButtonRightFunction != 0;
                pDevice->buttons [17].state = pData->ButtonLeftPaddle    != 0;
                pDevice->buttons [18].state = pData->ButtonRightPaddle   != 0;
              }
            }

            else if (! pDevice->bDualShock4)
            {
              pDevice->bBluetooth = true;
                    bHasBluetooth = true;
                    
              SK_RunOnce (SK_Bluetooth_SetupPowerOff ());

              if (! config.input.gamepad.bt_input_only)
                pDevice->write_output_report ();

              if (pDevice->buttons.size () < 19)
                  pDevice->buttons.resize (  19);

              BYTE* pInputRaw =
                (BYTE *)pDevice->input_report.data ();

              SK_HID_DualSense_GetStateData *pData =
                (SK_HID_DualSense_GetStateData *)&pInputRaw [2];

              bool bSimple = false;

              // We're in simplified mode...
              if (pInputRaw [0] == 0x1)
              {
                bSimple = true;
              }

              if (! bSimple)
              {
                if (config.input.gamepad.bt_input_only)
                {
                  SK_ImGui_CreateNotification ( "HID.Bluetooth.ModeChange", SK_ImGui_Toast::Warning,
                    "Reconnect them to Activate DualShock 3 Compatibility Mode",
                    "Special K has Disconnected all Bluetooth Controllers!",
                    //"Bluetooth PlayStation Controller(s) Running in DualShock 4 / DualSense Mode!",
                        15000UL, SK_ImGui_Toast::UseDuration | SK_ImGui_Toast::ShowTitle |
                                 SK_ImGui_Toast::ShowCaption | SK_ImGui_Toast::ShowNewest );

                  SK_DeferCommand ("Input.Gamepad.PowerOff 1");
                }

                pDevice->xinput.report.Gamepad.sThumbLX =
                  static_cast <SHORT> (32767.0 * static_cast <double> (static_cast <LONG> (pData->LeftStickX) - 128) / 128.0);

                pDevice->xinput.report.Gamepad.sThumbLY =
                  static_cast <SHORT> (32767.0 * static_cast <double> (128 - static_cast <LONG> (pData->LeftStickY)) / 128.0);

                pDevice->xinput.report.Gamepad.sThumbRX =
                  static_cast <SHORT> (32767.0 * static_cast <double> (static_cast <LONG> (pData->RightStickX) - 128) / 128.0);

                pDevice->xinput.report.Gamepad.sThumbRY =
                  static_cast <SHORT> (32767.0 * static_cast <double> (128 - static_cast <LONG> (pData->RightStickY)) / 128.0);
                
                pDevice->xinput.report.Gamepad.bLeftTrigger =
                  static_cast <BYTE> (static_cast <BYTE> (pData->TriggerLeft));

                pDevice->xinput.report.Gamepad.bRightTrigger =
                  static_cast <BYTE> (static_cast <BYTE> (pData->TriggerRight));

                switch ((int)pData->DPad)
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

                pDevice->buttons [ 0].state = pData->ButtonSquare   != 0;
                pDevice->buttons [ 1].state = pData->ButtonCross    != 0;
                pDevice->buttons [ 2].state = pData->ButtonCircle   != 0;
                pDevice->buttons [ 3].state = pData->ButtonTriangle != 0;
                pDevice->buttons [ 4].state = pData->ButtonL1       != 0;
                pDevice->buttons [ 5].state = pData->ButtonR1       != 0;
                pDevice->buttons [ 6].state = pData->ButtonL2       != 0;
                pDevice->buttons [ 7].state = pData->ButtonR2       != 0;

                pDevice->buttons [ 8].state = pData->ButtonCreate   != 0;
                pDevice->buttons [ 9].state = pData->ButtonOptions  != 0;

                pDevice->buttons [10].state = pData->ButtonL3       != 0;
                pDevice->buttons [11].state = pData->ButtonR3       != 0;

                // Do not write buttons the HID API does not list
                if (pDevice->buttons.size () >= 13)
                  pDevice->buttons [12].state = pData->ButtonHome   != 0;
                if (pDevice->buttons.size () >= 14)
                  pDevice->buttons [13].state = pData->ButtonPad    != 0;
                if (pDevice->buttons.size () >= 15)
                {
                  pDevice->buttons [14].state = pData->ButtonMute   != 0;
                  pDevice->buttons [15].state = pData->ButtonLeftFunction  != 0;
                  pDevice->buttons [16].state = pData->ButtonRightFunction != 0;
                  pDevice->buttons [17].state = pData->ButtonLeftPaddle    != 0;
                  pDevice->buttons [18].state = pData->ButtonRightPaddle   != 0;
                }

                pDevice->sensor_timestamp = pData->SensorTimestamp;

                if (pDevice->sensor_timestamp < 10200000)
                    pDevice->reset_rgb = false;
              }

              else
              {
                SK_HID_DualSense_GetSimpleStateDataBt *pSimpleData =
                  (SK_HID_DualSense_GetSimpleStateDataBt *)&pInputRaw [1];

                pDevice->xinput.report.Gamepad.sThumbLX =
                  static_cast <SHORT> (32767.0 * static_cast <double> (static_cast <LONG> (pSimpleData->LeftStickX) - 128) / 128.0);

                pDevice->xinput.report.Gamepad.sThumbLY =
                  static_cast <SHORT> (32767.0 * static_cast <double> (128 - static_cast <LONG> (pSimpleData->LeftStickY)) / 128.0);

                pDevice->xinput.report.Gamepad.sThumbRX =
                  static_cast <SHORT> (32767.0 * static_cast <double> (static_cast <LONG> (pSimpleData->RightStickX) - 128) / 128.0);

                pDevice->xinput.report.Gamepad.sThumbRY =
                  static_cast <SHORT> (32767.0 * static_cast <double> (128 - static_cast <LONG> (pSimpleData->RightStickY)) / 128.0);
                
                pDevice->xinput.report.Gamepad.bLeftTrigger =
                  static_cast <BYTE> (static_cast <BYTE> (pSimpleData->TriggerLeft));

                pDevice->xinput.report.Gamepad.bRightTrigger =
                  static_cast <BYTE> (static_cast <BYTE> (pSimpleData->TriggerRight));

                switch ((int)pSimpleData->DPad)
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

                pDevice->buttons [ 0].state = pSimpleData->ButtonSquare   != 0;
                pDevice->buttons [ 1].state = pSimpleData->ButtonCross    != 0;
                pDevice->buttons [ 2].state = pSimpleData->ButtonCircle   != 0;
                pDevice->buttons [ 3].state = pSimpleData->ButtonTriangle != 0;
                pDevice->buttons [ 4].state = pSimpleData->ButtonL1       != 0;
                pDevice->buttons [ 5].state = pSimpleData->ButtonR1       != 0;
                pDevice->buttons [ 6].state = pSimpleData->ButtonL2       != 0;
                pDevice->buttons [ 7].state = pSimpleData->ButtonR2       != 0;

                pDevice->buttons [ 8].state = pSimpleData->ButtonShare    != 0;
                pDevice->buttons [ 9].state = pSimpleData->ButtonOptions  != 0;

                pDevice->buttons [10].state = pSimpleData->ButtonL3       != 0;
                pDevice->buttons [11].state = pSimpleData->ButtonR3       != 0;

                // Do not write buttons the HID API does not list
                if (pDevice->buttons.size () >= 13)
                  pDevice->buttons [12].state = pSimpleData->ButtonHome   != 0;
                if (pDevice->buttons.size () >= 14)
                  pDevice->buttons [13].state = pSimpleData->ButtonPad    != 0;
                if (pDevice->buttons.size () >= 15)
                  pDevice->buttons [14].state = false; // No mute button
              }

              pDevice->battery.state =
                (SK_HID_PlayStationDevice::PowerState)((((BYTE *)pData)[52] & 0xF0) >> 4);

              const float batteryPercent =
                ( std::min (((((BYTE *)pData)[52] & 0xF) /*- ((((BYTE *)pData)[53] & 0x10) ? 1 : 0)*/) * 10.0f + 5.0f, 100.0f) );

              if (pDevice->battery.state == Discharging || 
                  pDevice->battery.state == Charging    ||
                  pDevice->battery.state == Complete)
              {
                pDevice->battery.percentage = batteryPercent;
              }

              switch (pDevice->battery.state)
              {
                case Charging:
                case Discharging:
                {
                  if (pDevice->battery.percentage >= 30.0f)
                    SK_ImGui_DismissNotification ("DualSense.BatteryCharge");
                  else
                    SK_ImGui_CreateNotificationEx (
                      "DualSense.BatteryCharge", SK_ImGui_Toast::Info, nullptr, nullptr,
                         INFINITE, SK_ImGui_Toast::UseDuration  |
                                   SK_ImGui_Toast::ShowNewest   | 
                                   SK_ImGui_Toast::DoNotSaveINI |
                                   SK_ImGui_Toast::Unsilencable,
                      [](void *pUserData)->bool
                      {
                        SK_HID_PlayStationDevice::battery_s *pBatteryState =
                          (SK_HID_PlayStationDevice::battery_s *)pUserData;

                        static const char* szBatteryLevels [] = {
                          ICON_FA_BATTERY_EMPTY,
                          ICON_FA_BATTERY_QUARTER,
                          ICON_FA_BATTERY_HALF,
                          ICON_FA_BATTERY_THREE_QUARTERS,
                          ICON_FA_BATTERY_FULL
                        };

                        static ImColor battery_colors [] = {
                          ImColor::HSV (0.0f, 1.0f, 1.0f), ImColor::HSV (0.1f, 1.0f, 1.0f),
                          ImColor::HSV (0.2f, 1.0f, 1.0f), ImColor::HSV (0.3f, 1.0f, 1.0f),
                          ImColor::HSV (0.4f, 1.0f, 1.0f)
                        };

                        const int batteryLevel =
                          (pBatteryState->percentage > 70.0f) ? 4 :
                          (pBatteryState->percentage > 50.0f) ? 3 :
                          (pBatteryState->percentage > 30.0f) ? 2 :
                          (pBatteryState->percentage > 10.0f) ? 1 : 0;

                        // Battery charge is high enough, don't bother showing this...
                        if (pBatteryState->percentage > 30.0f)
                          return true;

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

                        ImGui::BeginGroup ();
                        ImGui::TextColored ( batteryColor, "%hs",
                            szBatteryLevels [batteryLevel]
                        );

                        ImGui::SameLine ();

                             if (pBatteryState->state == Discharging)
                          ImGui::Text ("%3.0f%% " ICON_FA_ARROW_DOWN, pBatteryState->percentage);
                        else if (pBatteryState->state == Charging)
                          ImGui::Text ("%3.0f%% " ICON_FA_ARROW_UP,   pBatteryState->percentage);
                        else
                          ImGui::Text ("%3.0f%% ",                    pBatteryState->percentage);

                        ImGui::EndGroup ();

                        if (ImGui::IsItemClicked ())
                          SK_ImGui_DismissNotification ("DualSense.BatteryCharge");

                        return false;
                      }, &pDevice->battery
                    );
                } break;
                default:
                  break;
              }
            }

            else if (pDevice->bDualShock4)
            {
                pDevice->bBluetooth = true;
                      bHasBluetooth = true;
                    
              SK_RunOnce (SK_Bluetooth_SetupPowerOff ());

              if (! config.input.gamepad.bt_input_only)
                pDevice->write_output_report ();

              if (pDevice->buttons.size () < 14)
                  pDevice->buttons.resize (  14);

              BYTE* pInputRaw =
                (BYTE *)pDevice->input_report.data ();

              SK_HID_DualShock4_GetStateData *pData =
                (SK_HID_DualShock4_GetStateData *)&pInputRaw [3];

              pDevice->xinput.report.Gamepad.sThumbLX =
                static_cast <SHORT> (32767.0 * static_cast <double> (static_cast <LONG> (pData->LeftStickX) - 128) / 128.0);

              pDevice->xinput.report.Gamepad.sThumbLY =
                static_cast <SHORT> (32767.0 * static_cast <double> (128 - static_cast <LONG> (pData->LeftStickY)) / 128.0);

              pDevice->xinput.report.Gamepad.sThumbRX =
                static_cast <SHORT> (32767.0 * static_cast <double> (static_cast <LONG> (pData->RightStickX) - 128) / 128.0);

              pDevice->xinput.report.Gamepad.sThumbRY =
                static_cast <SHORT> (32767.0 * static_cast <double> (128 - static_cast <LONG> (pData->RightStickY)) / 128.0);
              
              pDevice->xinput.report.Gamepad.bLeftTrigger =
                static_cast <BYTE> (static_cast <BYTE> (pData->TriggerLeft));

              pDevice->xinput.report.Gamepad.bRightTrigger =
                static_cast <BYTE> (static_cast <BYTE> (pData->TriggerRight));

              switch ((int)pData->DPad)
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

              pDevice->buttons [ 0].state = pData->ButtonSquare   != 0;
              pDevice->buttons [ 1].state = pData->ButtonCross    != 0;
              pDevice->buttons [ 2].state = pData->ButtonCircle   != 0;
              pDevice->buttons [ 3].state = pData->ButtonTriangle != 0;
              pDevice->buttons [ 4].state = pData->ButtonL1       != 0;
              pDevice->buttons [ 5].state = pData->ButtonR1       != 0;
              pDevice->buttons [ 6].state = pData->ButtonL2       != 0;
              pDevice->buttons [ 7].state = pData->ButtonR2       != 0;

              pDevice->buttons [ 8].state = pData->ButtonShare    != 0;
              pDevice->buttons [ 9].state = pData->ButtonOptions  != 0;

              pDevice->buttons [10].state = pData->ButtonL3       != 0;
              pDevice->buttons [11].state = pData->ButtonR3       != 0;

              // Do not write buttons the HID API does not list
              if (pDevice->buttons.size () >= 13)
                pDevice->buttons [12].state = pData->ButtonHome   != 0;
              if (pDevice->buttons.size () >= 14)
                pDevice->buttons [13].state = pData->ButtonPad    != 0;

              pDevice->sensor_timestamp = pData->Timestamp;

              //if (pDevice->sensor_timestamp < 10200000)
              //    pDevice->reset_rgb = false;

              pDevice->battery.state =
                (SK_HID_PlayStationDevice::PowerState)pData->PluggedPowerCable ?
                                SK_HID_PlayStationDevice::PowerState::Charging :
                             SK_HID_PlayStationDevice::PowerState::Discharging;

              const float batteryPercent =
                (float)(pData->PluggedPowerCable ? (pData->PowerPercent & 0xF) - 1
                                                 : (pData->PowerPercent & 0xF)) * 10.0f;

              pDevice->battery.percentage = batteryPercent;

              switch (pDevice->battery.state)
              {
                case Charging:
                case Discharging:
                {
                  if (pDevice->battery.percentage >= 30.0f)
                    SK_ImGui_DismissNotification ("DualShock.BatteryCharge");
                  else
                    SK_ImGui_CreateNotificationEx (
                      "DualShock.BatteryCharge", SK_ImGui_Toast::Info, nullptr, nullptr,
                         INFINITE, SK_ImGui_Toast::UseDuration  |
                                   SK_ImGui_Toast::ShowNewest   | 
                                   SK_ImGui_Toast::DoNotSaveINI |
                                   SK_ImGui_Toast::Unsilencable,
                      [](void *pUserData)->bool
                      {
                        SK_HID_PlayStationDevice::battery_s *pBatteryState =
                          (SK_HID_PlayStationDevice::battery_s *)pUserData;

                        static const char* szBatteryLevels [] = {
                          ICON_FA_BATTERY_EMPTY,
                          ICON_FA_BATTERY_QUARTER,
                          ICON_FA_BATTERY_HALF,
                          ICON_FA_BATTERY_THREE_QUARTERS,
                          ICON_FA_BATTERY_FULL
                        };

                        static ImColor battery_colors [] = {
                          ImColor::HSV (0.0f, 1.0f, 1.0f), ImColor::HSV (0.1f, 1.0f, 1.0f),
                          ImColor::HSV (0.2f, 1.0f, 1.0f), ImColor::HSV (0.3f, 1.0f, 1.0f),
                          ImColor::HSV (0.4f, 1.0f, 1.0f)
                        };

                        const int batteryLevel =
                          (pBatteryState->percentage > 70.0f) ? 4 :
                          (pBatteryState->percentage > 50.0f) ? 3 :
                          (pBatteryState->percentage > 30.0f) ? 2 :
                          (pBatteryState->percentage > 10.0f) ? 1 : 0;

                        // Battery charge is high enough, don't bother showing this...
                        if (pBatteryState->percentage > 30.0f)
                          return true;

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

                        ImGui::BeginGroup ();
                        ImGui::TextColored ( batteryColor, "%hs",
                            szBatteryLevels [batteryLevel]
                        );

                        ImGui::SameLine ();

                             if (pBatteryState->state == Discharging)
                          ImGui::Text ("%3.0f%% " ICON_FA_ARROW_DOWN, pBatteryState->percentage);
                        else if (pBatteryState->state == Charging)
                          ImGui::Text ("%3.0f%% " ICON_FA_ARROW_UP,   pBatteryState->percentage);
                        else
                          ImGui::Text ("%3.0f%% ",                    pBatteryState->percentage);

                        ImGui::EndGroup ();

                        if (ImGui::IsItemClicked ())
                          SK_ImGui_DismissNotification ("DualShock.BatteryCharge");

                        return false;
                      }, &pDevice->battery
                    );
                } break;
                default:
                  break;
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
            }

            if (pDevice->buttons.size () >= 13 &&
                pDevice->buttons [12].state) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_GUIDE;

            bool bIsInputActive = false;

            // Button states should repeat
            if (pDevice->xinput.report.Gamepad.wButtons != 0x0)
              bIsInputActive = true;

            if (abs (pDevice->xinput.report.Gamepad.sThumbLX) > (XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * 2)  ||
                abs (pDevice->xinput.report.Gamepad.sThumbLY) > (XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * 2)  ||
                abs (pDevice->xinput.report.Gamepad.sThumbRX) > (XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE * 2) ||
                abs (pDevice->xinput.report.Gamepad.sThumbRY) > (XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE * 2) ||
                     pDevice->xinput.report.Gamepad.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
                     pDevice->xinput.report.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            {
              bIsInputActive = true;
            }

            bool bIsDeviceMostRecentlyActive = true;

            for ( auto& ps_controller : SK_HID_PlayStationControllers )
            {
              if (ps_controller.xinput.last_active > pDevice->xinput.last_active)
              {
                bIsDeviceMostRecentlyActive = false;
                break;
              }
            }

            //
            // Do not do in the background unless bg input is enabled
            //
            //   Also only allow input from the most recently active device,
            //     in case we're reading the same controller over
            //       Bluetooth and USB...
            //
            const bool bAllowSpecialButtons =
              ( config.input.gamepad.disabled_to_game == 0 ||
                  SK_IsGameWindowActive () ) && bIsDeviceMostRecentlyActive;

            if ( config.input.gamepad.scepad.enhanced_ps_button &&
                                 pDevice->buttons.size () >= 13 &&
                      (config.input.gamepad.xinput.ui_slot >= 0 && 
                       config.input.gamepad.xinput.ui_slot <  4) &&
                                          bIsDeviceMostRecentlyActive )
            {
              static HWND hWndLastApp =
                SK_GetForegroundWindow ();

              if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
                  (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_X)     &&
                (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_X)))
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

                pDevice->chord_activated = true;
              }

              if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
                  (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_B)     &&
                (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_B)))
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

                pDevice->chord_activated = true;
              }

              if (bAllowSpecialButtons)
              {
#if 0
                static bool bIsVLC =
                  StrStrIW (SK_GetHostApp (), L"vlc");

                if (bIsVLC)
                {
                  if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_A) &&
                    (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_A)))
                  {
                    BYTE bScancode =
                      (BYTE)MapVirtualKey (VK_MEDIA_PLAY_PAUSE, 0);

                    DWORD dwFlags =
                      ( bScancode & 0xE0 ) == 0   ?
                        static_cast <DWORD> (0x0) :
                        static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

                    keybd_event_Original (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags,                   0);
                    keybd_event_Original (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
                  }

                  if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) &&
                    (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)))
                  {
                    BYTE bScancode =
                     (BYTE)MapVirtualKey (VK_MEDIA_PREV_TRACK, 0);

                    DWORD dwFlags =
                      ( bScancode & 0xE0 ) == 0   ?
                        static_cast <DWORD> (0x0) :
                        static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

                    keybd_event_Original (VK_MEDIA_PREV_TRACK, bScancode, dwFlags,                   0);
                    keybd_event_Original (VK_MEDIA_PREV_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
                  }

                  if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) &&
                    (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)))
                  {
                    BYTE bScancode =
                     (BYTE)MapVirtualKey (VK_MEDIA_NEXT_TRACK, 0);

                    DWORD dwFlags =
                      ( bScancode & 0xE0 ) == 0   ?
                        static_cast <DWORD> (0x0) :
                        static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

                    keybd_event_Original (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags,                   0);
                    keybd_event_Original (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);
                  }
                }
#endif



                if ( pDevice->buttons [12].state &&
                   (!pDevice->buttons [12].last_state))
                {
                  pDevice->chord_activated = false;
                }

                if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
                    (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_Y)     &&
                  (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_Y)))
                {
                  SK_DeferCommand ("Input.Gamepad.PowerOff 1");
                  pDevice->chord_activated = true;
                }

                if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE) &&
                    (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)  &&
                  (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)))
                {
                  SK_SteamAPI_TakeScreenshot ();
                  pDevice->chord_activated = true;
                }

                if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)      &&
                    (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) &&
                  (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)))
                {
                  BYTE bScancode =
                    (BYTE)MapVirtualKey (VK_MEDIA_PLAY_PAUSE, 0);

                  DWORD dwFlags =
                    ( bScancode & 0xE0 ) == 0   ?
                      static_cast <DWORD> (0x0) :
                      static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

                  keybd_event_Original (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags,                   0);
                  keybd_event_Original (VK_MEDIA_PLAY_PAUSE, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);

                  pDevice->chord_activated = true;
                }

                if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)         &&
                    (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) &&
                  (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)))
                {
                  BYTE bScancode =
                   (BYTE)MapVirtualKey (VK_MEDIA_PREV_TRACK, 0);

                  DWORD dwFlags =
                    ( bScancode & 0xE0 ) == 0   ?
                      static_cast <DWORD> (0x0) :
                      static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

                  keybd_event_Original (VK_MEDIA_PREV_TRACK, bScancode, dwFlags,                   0);
                  keybd_event_Original (VK_MEDIA_PREV_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);

                  pDevice->chord_activated = true;
                }

                if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)          &&
                    (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) &&
                  (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)))
                {
                  BYTE bScancode =
                   (BYTE)MapVirtualKey (VK_MEDIA_NEXT_TRACK, 0);

                  DWORD dwFlags =
                    ( bScancode & 0xE0 ) == 0   ?
                      static_cast <DWORD> (0x0) :
                      static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY);

                  keybd_event_Original (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags,                   0);
                  keybd_event_Original (VK_MEDIA_NEXT_TRACK, bScancode, dwFlags | KEYEVENTF_KEYUP, 0);

                  pDevice->chord_activated = true;
                }

                if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)      &&
                    (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) &&
                  (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)))
                {
                  SK_GetCommandProcessor ()->
                    ProcessCommandLine ("HDR.Luminance += 0.125");

                  pDevice->chord_activated = true;
                }

                if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)     &&
                    (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) &&
                  (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)))
                {
                  SK_GetCommandProcessor ()->
                    ProcessCommandLine ("HDR.Luminance -= 0.125");

                  pDevice->chord_activated = true;
                }

                if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)   &&
                    (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
                  (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)))
                {
                  SK_GetCommandProcessor ()->
                    ProcessCommandLine ("Sound.Volume += 10.0");

                  pDevice->chord_activated = true;
                }

                if ((pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE)     &&
                    (pDevice->xinput.     report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
                  (!(pDevice->xinput.prev_report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)))
                {
                  SK_GetCommandProcessor ()->
                    ProcessCommandLine ("Sound.Volume -= 10.0");

                  pDevice->chord_activated = true;
                }

                if ( (! pDevice->buttons [12].state)     &&
                        pDevice->buttons [12].last_state &&
                     (! pDevice->chord_activated) )
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
            }

            if ( pDevice->buttons.size () >= 15 &&
                 pDevice->bDualSense            && config.input.gamepad.scepad.mute_applies_to_game && bAllowSpecialButtons )
            {
              if (   pDevice->buttons [14].state &&
                  (! pDevice->buttons [14].last_state) )
              {
                SK_SetGameMute (! SK_IsGameMuted ());
              }
            }
            
            //if ( pDevice->buttons.size () >= 16 && 
            //     pDevice->bDualSense            && bAllowSpecialButtons )
            //{
            //  if (pDevice->buttons [15].state && 
            //    (!pDevice->buttons [15].last_state))
            //  {
            //    SK_SteamAPI_TakeScreenshot ();
            //  }
            //}

            if ( memcmp ( &pDevice->xinput.prev_report.Gamepad,
                          &pDevice->xinput.     report.Gamepad, sizeof (XINPUT_GAMEPAD)) )
            {
              pDevice->xinput.report.dwPacketNumber++;
              pDevice->xinput.prev_report = pDevice->xinput.report;
            }

            if (bIsInputActive)
            {
              pDevice->xinput.last_active = SK_QueryPerf ().QuadPart;

              // Give Bluetooth devices a +666 ms advantage
              if (pDevice->bBluetooth)
                  pDevice->xinput.last_active += (2 * (SK_PerfFreq / 3));
            }

            for ( auto& button : pDevice->buttons )
            {
              button.last_state =
                std::exchange (button.state, false);
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
  static std::mutex s_output_mutex;

  if (! bConnected)
    return false;

  if (bDualSense)
  {
    if (! bBluetooth)
    {
      SK_ReleaseAssert (
        output_report.size () >= sizeof (SK_HID_DualSense_SetStateData) + 1
      );

      if (output_report.size () <= sizeof (SK_HID_DualSense_SetStateData))
        return false;
    }

    if (hOutputEnqueued == nullptr)
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

        // Will be signaled when DLL teardown is processed and any vibration is cleared to 0.
        bool bQuit = false;

        DWORD  dwWaitState  = WAIT_FAILED;
        while (dwWaitState != WAIT_OBJECT_0)
        {
          if (bQuit)
            break;

          dwWaitState =
            WaitForMultipleObjects ( _countof (hEvents),
                                               hEvents, FALSE, INFINITE );

          if (dwWaitState == WAIT_OBJECT_0)
          {
            pDevice->setVibration (0, 0);
            bQuit     = true;
            bFinished = true;
          }

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

          if (! bFinished)
          {
            continue;
          }

          if (pDevice->bBluetooth)
          {
            // Prevent changing the protocol of Bluetooth-paired PlayStation controllers
            while (config.input.gamepad.bt_input_only)
            {
              if ( WAIT_OBJECT_0 ==
                     WaitForSingleObject (__SK_DLL_TeardownEvent, 1000UL) )
              {
                SK_Thread_CloseSelf ();

                return 0;
              }
            }
          }

          ZeroMemory ( pDevice->output_report.data (),
                       pDevice->output_report.size () );

          // Report Type
          pDevice->output_report [0] =
            pDevice->bBluetooth ? 0x31 :
                                  0x02;

          if (! pDevice->bBluetooth)
          {
            BYTE* pOutputRaw =
              (BYTE *)pDevice->output_report.data ();

            SK_HID_DualSense_SetStateData* output =
              (SK_HID_DualSense_SetStateData *)&pOutputRaw [1];

            output->EnableRumbleEmulation    = true;
            output->UseRumbleNotHaptics      = true;
            output->AllowMuteLight           = true;

            if (config.input.gamepad.scepad.led_color_r    >= 0 ||
                config.input.gamepad.scepad.led_color_g    >= 0 ||
                config.input.gamepad.scepad.led_color_b    >= 0 ||
                config.input.gamepad.scepad.led_brightness >= 0)
            {
              output->AllowLedColor          = true;
            }

            output->AllowHapticLowPassFilter = true;
            output->AllowMotorPowerLevel     = false;

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

            output->TouchPowerSave      = config.input.gamepad.scepad.power_save_mode;
            output->MotionPowerSave     = config.input.gamepad.scepad.power_save_mode;
            output->AudioPowerSave      = config.input.gamepad.scepad.power_save_mode;
            output->HapticLowPassFilter = true;

            output->RumbleMotorPowerReduction = 0x0;
              //config.input.gamepad.scepad.rumble_power_level == 100.0f ? 0
              //                                                         :
              //static_cast <uint8_t> ((100.0f - config.input.gamepad.scepad.rumble_power_level) / 12.5f);

            if (config.input.gamepad.scepad.led_brightness == 3)
            {
              output->AllowLightBrightnessChange = 1;
              output->LightBrightness            = (LightBrightness)3;
            }

            else if (config.input.gamepad.scepad.led_brightness >= 0)
            {
              output->AllowLightBrightnessChange = 1;
              output->LightBrightness            =
                (LightBrightness)std::clamp (config.input.gamepad.scepad.led_brightness, 0, 2);
            }

            if (config.input.gamepad.scepad.led_color_r >= 0 || 
                config.input.gamepad.scepad.led_color_g >= 0 ||
                config.input.gamepad.scepad.led_color_b >= 0)
            {
              output->LedRed   = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_r, 0, 255));
              output->LedGreen = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_g, 0, 255));
              output->LedBlue  = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_b, 0, 255));

              if (output->AllowLightBrightnessChange)
              {
                switch (output->LightBrightness)
                {
                  case Mid:
                    output->LedRed   /= 2;
                    output->LedGreen /= 2;
                    output->LedBlue  /= 2;
                    break;
                  case Dim:
                    output->LedRed   /= 4;
                    output->LedGreen /= 4;
                    output->LedBlue  /= 4;
                    break;
                  case NoLightAction3:
                    output->LedRed   /= 8;
                    output->LedGreen /= 8;
                    output->LedBlue  /= 8;
                    output->LightBrightness = Dim;
                    break;
                }
              }
            }

            else
            {
              output->LedRed   = pDevice->_color.r;
              output->LedGreen = pDevice->_color.g;
              output->LedBlue  = pDevice->_color.b;
            }
          }

          else if (pDevice->bBluetooth)
          {
            BYTE bt_data [79] = { };

            struct Header {
              uint8_t Head;
              uint8_t ReportID;
              uint8_t UNK1      : 1; // -+
              uint8_t EnableHID : 1; //  | - these 3 bits seem to act as an enum
              uint8_t UNK2      : 1; // -+
              uint8_t UNK3      : 1;
              uint8_t SeqNo     : 4; // increment for every write // we have no proof of this, need to see some PS5 captures
            } static header = { 0xA2, 0x31, 0x0, 0x1, 0x0, 0x0, 0x0 };

            header.SeqNo++;

            memcpy (bt_data, &header, sizeof (header));

            SK_HID_DualSense_SetStateData* output =
           (SK_HID_DualSense_SetStateData *)&bt_data [3];

            output->EnableRumbleEmulation = true;
            output->UseRumbleNotHaptics   = true;
            output->AllowMuteLight        = true;

            if (config.input.gamepad.scepad.led_color_r    >= 0 || 
                config.input.gamepad.scepad.led_color_g    >= 0 ||
                config.input.gamepad.scepad.led_color_b    >= 0 ||
                config.input.gamepad.scepad.led_brightness >= 0)
            {
              output->AllowLedColor       = true;
            }

            output->AllowPlayerIndicators    = config.input.gamepad.xinput.debug;
            output->AllowHapticLowPassFilter = true;
            output->AllowMotorPowerLevel     = false;
            
            // Firmware reqs
            output->
               EnableImprovedRumbleEmulation = true;

            output->RumbleEmulationRight =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.right)
              );
            output->RumbleEmulationLeft  =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.left)
              );

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

            output->HostTimestamp       = SK::ControlPanel::current_time;
            output->HapticLowPassFilter = true;

            if (config.input.gamepad.xinput.debug)
            {
              static int counter = 0;

              output->PlayerLight1    = (counter & 0x01) != 0;
              output->PlayerLight2    = (counter & 0x02) != 0;
              output->PlayerLight3    = (counter & 0x04) != 0;
              output->PlayerLight4    = (counter & 0x08) != 0;
              output->PlayerLight5    = (counter & 0x10) != 0;
              output->PlayerLightFade = 1;

              ++counter;

              if (counter > 32)
                  counter = 0;
            }

            bt_data [46] &= ~(1 << 7);
            bt_data [46] &= ~(1 << 8);

            if (config.input.gamepad.xinput.debug)
            {
              output->AllowLedColor = true;

              auto color_val =
                ImColor::HSV (
                  (float)               (SK::ControlPanel::current_time % 2800)/ 2800.f,
                    (.5f + (sin ((float)(SK::ControlPanel::current_time % 500) / 500.f)) * .5f) / 2.f,
                       1.f   ).Value;

              output->LedRed   = (uint8_t)(color_val.x * 255.0f);
              output->LedGreen = (uint8_t)(color_val.y * 255.0f);
              output->LedBlue  = (uint8_t)(color_val.z * 255.0f);
            }

            else
            {
              if (config.input.gamepad.scepad.led_brightness == 3)
              {
                if (pDevice->sensor_timestamp >= 10200000 && (! pDevice->reset_rgb))
                {
                  pDevice->reset_rgb  = true;
                  output->ResetLights = true;
                }

                output->AllowLightBrightnessChange = 1;
                output->LightBrightness            = (LightBrightness)3;
              }

              else if (config.input.gamepad.scepad.led_brightness >= 0)
              {
                output->AllowLightBrightnessChange = 1;
                output->LightBrightness            =
                  (LightBrightness)std::clamp (config.input.gamepad.scepad.led_brightness, 0, 2);

                if (pDevice->sensor_timestamp >= 10200000 && (! pDevice->reset_rgb))
                {
                  pDevice->reset_rgb  = true;
                  output->ResetLights = true;
                }
              }

              if (config.input.gamepad.scepad.led_color_r >= 0 || 
                  config.input.gamepad.scepad.led_color_g >= 0 ||
                  config.input.gamepad.scepad.led_color_b >= 0)
              {
                if (pDevice->sensor_timestamp >= 10200000 && (! pDevice->reset_rgb))
                {
                  pDevice->reset_rgb  = true;
                  output->ResetLights = true;
                }

                output->LedRed   = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_r, 0, 255));
                output->LedGreen = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_g, 0, 255));
                output->LedBlue  = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_b, 0, 255));

                if (output->AllowLightBrightnessChange)
                {
                  switch (output->LightBrightness)
                  {
                    case Mid:
                      output->LedRed   /= 2;
                      output->LedGreen /= 2;
                      output->LedBlue  /= 2;
                      break;
                    case Dim:
                      output->LedRed   /= 4;
                      output->LedGreen /= 4;
                      output->LedBlue  /= 4;
                      break;
                    case NoLightAction3:
                      output->LedRed   /= 8;
                      output->LedGreen /= 8;
                      output->LedBlue  /= 8;
                      output->LightBrightness = Dim;
                      break;
                  }
                }
              }

              else
              {
                output->LedRed   = pDevice->_color.r;
                output->LedGreen = pDevice->_color.g;
                output->LedBlue  = pDevice->_color.b;
              }
            }

            uint32_t crc =
             crc32 (0x0, bt_data, 75);

            memcpy (                                &bt_data [75], &crc, 4);
            memcpy (pDevice->output_report.data (), &bt_data [ 1],      78);
          }

          async_output_request        = { };
          async_output_request.hEvent = pDevice->hOutputFinished;

          BOOL bWriteAsync = FALSE;

          {
            std::scoped_lock write_lock (s_output_mutex);

            bWriteAsync =
            SK_WriteFile ( pDevice->hDeviceFile, pDevice->output_report.data (),
                            static_cast <DWORD> (pDevice->bBluetooth ? 78 :
                                                 pDevice->output_report.size ()), nullptr, &async_output_request );
          }

          if (! bWriteAsync)
          {
            DWORD dwLastErr =
                 GetLastError ();

            // Invalid Handle and Device Not Connected get a 250 ms cooldown
            if ( dwLastErr != NOERROR              &&
                 dwLastErr != ERROR_IO_PENDING     &&
                 dwLastErr != ERROR_INVALID_HANDLE &&
                 dwLastErr != ERROR_NO_SUCH_DEVICE &&
                 dwLastErr != ERROR_NOT_READY      &&
                 dwLastErr != ERROR_DEVICE_NOT_CONNECTED )
            {
              _com_error err (dwLastErr);

              SK_LOGs0 (
                L"InputBkEnd",
                L"SK_WriteFile ({%ws}, OutputReport) failed, Error=%d [%ws]",
                  pDevice->wszDevicePath, dwLastErr,
                    err.ErrorMessage ()
              );
            }

            if (dwLastErr != ERROR_IO_PENDING && dwLastErr != NOERROR)
            {
              if (dwLastErr != ERROR_INVALID_HANDLE)
                SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

              SK_SleepEx (125UL, TRUE); // Prevent runaway CPU usage on failure

              SetEvent (pDevice->hOutputFinished);
              continue;
            }

            else
            {
              DWORD                                                                         dwBytesRead = 0x0;
              if (! SK_GetOverlappedResultEx (pDevice->hDeviceFile, &async_output_request, &dwBytesRead, 100UL, FALSE))
              {
                dwLastErr = GetLastError ();

                if (dwLastErr != ERROR_IO_PENDING && dwLastErr != NOERROR)
                {
                  if (dwLastErr != ERROR_IO_INCOMPLETE)
                  {
                    if (dwLastErr != ERROR_NOT_READY      &&
                        dwLastErr != WAIT_TIMEOUT         &&
                        dwLastErr != ERROR_GEN_FAILURE    &&   // Happens during power-off
                        dwLastErr != ERROR_OPERATION_ABORTED ) // Happens during power-off
                    {
                      _com_error err (dwLastErr);

                      SK_LOGs0 (
                        L"InputBkEnd",
                        L"SK_GetOverlappedResultEx ({%ws}, OutputReport) failed, Error=%d [%ws]",
                          pDevice->wszDevicePath, dwLastErr,
                            err.ErrorMessage ()
                      );
                    }

                    if (dwLastErr != ERROR_INVALID_HANDLE && dwLastErr != NOERROR)
                      SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

                    if (dwLastErr == ERROR_GEN_FAILURE || dwLastErr == ERROR_OPERATION_ABORTED)
                      SK_SleepEx (150UL, TRUE);
                  }

                  SetEvent (pDevice->hOutputFinished);
                  continue;
                }
              }
            }
          }

          pDevice->ulLastFrameOutput =
                   SK_GetFramesDrawn ();

          pDevice->dwLastTimeOutput =
            SK::ControlPanel::current_time;

          bEnqueued = false;
          bFinished = false;
        }

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] HID Output Report Producer", this);
    }

    else
      SetEvent (hOutputEnqueued);

    return true;
  }

  else if (bDualShock4)
  {
    if (! bBluetooth)
    {
      if (output_report.size () <= sizeof (SK_HID_DualShock4_SetStateData))
      {
        output_report.resize (sizeof (SK_HID_DualShock4_SetStateData) + 1);
      }

      SK_ReleaseAssert (
        output_report.size () >= sizeof (SK_HID_DualShock4_SetStateData) + 1
      );

      if (output_report.size () <= sizeof (SK_HID_DualShock4_SetStateData))
        return false;
    }

    if (hOutputEnqueued == nullptr)
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

        // Will be signaled when DLL teardown is processed and any vibration is cleared to 0.
        bool bQuit     = false;

        DWORD  dwWaitState  = WAIT_FAILED;
        while (dwWaitState != WAIT_OBJECT_0)
        {
          if (bQuit)
            break;

          dwWaitState =
            WaitForMultipleObjects ( _countof (hEvents),
                                               hEvents, FALSE, INFINITE );

          if (dwWaitState == WAIT_OBJECT_0)
          {
            pDevice->setVibration (0, 0);
            bQuit     = true;
            bFinished = true;
          }

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

          if (! bFinished)
          {
            continue;
          }

          ZeroMemory ( pDevice->output_report.data (),
                       pDevice->output_report.size () );

          // Report Type
          pDevice->output_report [0] =
            pDevice->bBluetooth ? 0x11 : 0x05;

          if (! pDevice->bBluetooth)
          {
            BYTE* pOutputRaw =
              (BYTE *)pDevice->output_report.data ();

            SK_HID_DualShock4_SetStateData* output =
              (SK_HID_DualShock4_SetStateData *)&pOutputRaw [1];

            output->EnableRumbleUpdate = true;

            if (config.input.gamepad.scepad.led_color_r    >= 0 ||
                config.input.gamepad.scepad.led_color_g    >= 0 ||
                config.input.gamepad.scepad.led_color_b    >= 0 ||
                config.input.gamepad.scepad.led_brightness >= 0)
            {
              output->EnableLedUpdate  = true;
            }

#if 1
            if ( pDevice->_vibration.last_set >= SK::ControlPanel::current_time -
                 pDevice->_vibration.MAX_TTL_IN_MSECS )
            {
#endif
              output->RumbleRight =
                sk::narrow_cast <BYTE> (
                  ReadULongAcquire (&pDevice->_vibration.right)
                );
              output->RumbleLeft  =
                sk::narrow_cast <BYTE> (
                  ReadULongAcquire (&pDevice->_vibration.left)
                );
#if 1
            }

            else
            {
              output->RumbleRight = 0;
              output->RumbleLeft  = 0;

              WriteULongRelease (&pDevice->_vibration.left,  0);
              WriteULongRelease (&pDevice->_vibration.right, 0);
            }

            //if (std::exchange (pDevice->_vibration.last_left,  output->RumbleEmulationLeft ) != output->RumbleEmulationLeft  || output->RumbleEmulationLeft  == 0)
            //  pDevice->_vibration.last_output = pDevice->_vibration.last_set;
            //if (std::exchange (pDevice->_vibration.last_right, output->RumbleEmulationRight) != output->RumbleEmulationRight || output->RumbleEmulationRight == 0)
            //  pDevice->_vibration.last_output = pDevice->_vibration.last_set;
#endif

              //config.input.gamepad.scepad.rumble_power_level == 100.0f ? 0
              //                                                         :
              //static_cast <uint8_t> ((100.0f - config.input.gamepad.scepad.rumble_power_level) / 12.5f);

            if (config.input.gamepad.scepad.led_color_r >= 0 || 
                config.input.gamepad.scepad.led_color_g >= 0 ||
                config.input.gamepad.scepad.led_color_b >= 0)
            {
              output->LedRed   = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_r, 0, 255));
              output->LedGreen = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_g, 0, 255));
              output->LedBlue  = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_b, 0, 255));

              if (config.input.gamepad.scepad.led_brightness >= 0)
              {
                switch (config.input.gamepad.scepad.led_brightness)
                {
                  // Full-bright
                  default:
                    break;
                  case 1:
                    output->LedRed   /= 2;
                    output->LedGreen /= 2;
                    output->LedBlue  /= 2;
                    break;
                  case 2:
                    output->LedRed   /= 4;
                    output->LedGreen /= 4;
                    output->LedBlue  /= 4;
                    break;
                  case 3:
                    output->LedRed   /= 8;
                    output->LedGreen /= 8;
                    output->LedBlue  /= 8;
                    break;
                }
              }
            }

            else
            {
              output->LedRed   = pDevice->_color.r;
              output->LedGreen = pDevice->_color.g;
              output->LedBlue  = pDevice->_color.b;
            }
          }

          else if (pDevice->bBluetooth)
          {
            BYTE bt_data [83] = { };

            struct Header {
              uint8_t Head;
              uint8_t ReportID;
              uint8_t PollingRate : 6; // note 0 appears to be clamped to 1
              uint8_t EnableCRC   : 1;
              uint8_t EnableHID   : 1;
              uint8_t EnableMic   : 3; // somehow enables mic, appears to be 3 bit flags
              uint8_t UnkA4       : 1;
              uint8_t UnkB1       : 1;
              uint8_t UnkB2       : 1; // seems to always be 1
              uint8_t UnkB3       : 1;
              uint8_t EnableAudio : 1;
            } static header = { 0xA2, 0x11, 1, 1, 1, 0, 0, 0, 1, 0, 0 };

            memcpy (bt_data, &header, sizeof (header));

            BYTE* pOutputRaw =
              (BYTE *)&bt_data [4];

            SK_HID_DualShock4_SetStateDataBt* output =
              (SK_HID_DualShock4_SetStateDataBt *)&pOutputRaw [0];

            output->EnableRumbleUpdate = true;

            if (config.input.gamepad.scepad.led_color_r    >= 0 ||
                config.input.gamepad.scepad.led_color_g    >= 0 ||
                config.input.gamepad.scepad.led_color_b    >= 0 ||
                config.input.gamepad.scepad.led_brightness >= 0)
            {
              output->EnableLedUpdate  = true;
            }

#if 1
            if ( pDevice->_vibration.last_set >= SK::ControlPanel::current_time -
                 pDevice->_vibration.MAX_TTL_IN_MSECS )
            {
#endif
              output->RumbleRight =
                sk::narrow_cast <BYTE> (
                  ReadULongAcquire (&pDevice->_vibration.right)
                );
              output->RumbleLeft  =
                sk::narrow_cast <BYTE> (
                  ReadULongAcquire (&pDevice->_vibration.left)
                );
#if 1
            }

            else
            {
              output->RumbleRight = 0;
              output->RumbleLeft  = 0;

              WriteULongRelease (&pDevice->_vibration.left,  0);
              WriteULongRelease (&pDevice->_vibration.right, 0);
            }

            //if (std::exchange (pDevice->_vibration.last_left,  output->RumbleEmulationLeft ) != output->RumbleEmulationLeft  || output->RumbleEmulationLeft  == 0)
            //  pDevice->_vibration.last_output = pDevice->_vibration.last_set;
            //if (std::exchange (pDevice->_vibration.last_right, output->RumbleEmulationRight) != output->RumbleEmulationRight || output->RumbleEmulationRight == 0)
            //  pDevice->_vibration.last_output = pDevice->_vibration.last_set;
#endif

              //config.input.gamepad.scepad.rumble_power_level == 100.0f ? 0
              //                                                         :
              //static_cast <uint8_t> ((100.0f - config.input.gamepad.scepad.rumble_power_level) / 12.5f);

            if (config.input.gamepad.scepad.led_color_r >= 0 || 
                config.input.gamepad.scepad.led_color_g >= 0 ||
                config.input.gamepad.scepad.led_color_b >= 0)
            {
              output->LedRed   = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_r, 0, 255));
              output->LedGreen = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_g, 0, 255));
              output->LedBlue  = sk::narrow_cast <uint8_t> (std::clamp (config.input.gamepad.scepad.led_color_b, 0, 255));

              if (config.input.gamepad.scepad.led_brightness >= 0)
              {
                switch (config.input.gamepad.scepad.led_brightness)
                {
                  // Full-bright
                  default:
                    break;
                  case 1:
                    output->LedRed   /= 2;
                    output->LedGreen /= 2;
                    output->LedBlue  /= 2;
                    break;
                  case 2:
                    output->LedRed   /= 4;
                    output->LedGreen /= 4;
                    output->LedBlue  /= 4;
                    break;
                  case 3:
                    output->LedRed   /= 8;
                    output->LedGreen /= 8;
                    output->LedBlue  /= 8;
                    break;
                }
              }
            }

            else
            {
              output->LedRed   = pDevice->_color.r;
              output->LedGreen = pDevice->_color.g;
              output->LedBlue  = pDevice->_color.b;
            }

            uint32_t crc =
             crc32 (0x0, bt_data, 75);

            memcpy (                                &bt_data [75], &crc, 4);
            memcpy (pDevice->output_report.data (), &bt_data [ 1],      78);
          }

          async_output_request        = { };
          async_output_request.hEvent = pDevice->hOutputFinished;

          BOOL bWriteAsync = FALSE;

          {
            std::scoped_lock write_lock (s_output_mutex);

            bWriteAsync =
            SK_WriteFile ( pDevice->hDeviceFile, pDevice->output_report.data (),
                            static_cast <DWORD> (pDevice->bBluetooth ? 78 :
                                                                       32), nullptr, &async_output_request );
          }

          if (! bWriteAsync)
          {
            DWORD dwLastErr =
                 GetLastError ();

            // Invalid Handle and Device Not Connected get a 250 ms cooldown
            if ( dwLastErr != NOERROR                 &&
                 dwLastErr != ERROR_GEN_FAILURE       && // Happens during power-off
                 dwLastErr != ERROR_IO_PENDING        &&
                 dwLastErr != ERROR_INVALID_HANDLE    &&
                 dwLastErr != ERROR_NO_SUCH_DEVICE    &&
                 dwLastErr != ERROR_NOT_READY         &&
                 dwLastErr != ERROR_OPERATION_ABORTED && // Happens during power-off
                 dwLastErr != ERROR_DEVICE_NOT_CONNECTED )
            {
              _com_error err (dwLastErr);

              SK_LOGs0 (
                L"InputBkEnd",
                L"SK_WriteFile ({%ws}, OutputReport) failed, Error=%d [%ws]",
                  pDevice->wszDevicePath, dwLastErr,
                    err.ErrorMessage ()
              );
            }

            if (dwLastErr != ERROR_IO_PENDING && dwLastErr != NOERROR)
            {
              if (dwLastErr != ERROR_INVALID_HANDLE)
                SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

              SK_SleepEx (125UL, TRUE); // Prevent runaway CPU usage on failure

              SetEvent (pDevice->hOutputFinished);
              continue;
            }

            else
            {
              DWORD                                                                         dwBytesRead = 0x0;
              if (! SK_GetOverlappedResultEx (pDevice->hDeviceFile, &async_output_request, &dwBytesRead, 100UL, FALSE))
              {
                dwLastErr = GetLastError ();

                if (dwLastErr != ERROR_IO_PENDING && dwLastErr != NOERROR)
                {
                  if (dwLastErr != ERROR_IO_INCOMPLETE)
                  {
                    if (dwLastErr != ERROR_NOT_READY      &&
                        dwLastErr != WAIT_TIMEOUT         &&
                        dwLastErr != ERROR_GEN_FAILURE    &&   // Happens during power-off
                        dwLastErr != ERROR_OPERATION_ABORTED ) // Happens during power-off
                    {
                      _com_error err (dwLastErr);

                      SK_LOGs0 (
                        L"InputBkEnd",
                        L"SK_GetOverlappedResultEx ({%ws}, OutputReport) failed, Error=%d [%ws]",
                          pDevice->wszDevicePath, dwLastErr,
                            err.ErrorMessage ()
                      );
                    }

                    if (dwLastErr != ERROR_INVALID_HANDLE && dwLastErr != NOERROR)
                      SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

                    if (dwLastErr == ERROR_GEN_FAILURE || dwLastErr == ERROR_OPERATION_ABORTED)
                      SK_SleepEx (150UL, TRUE);
                  }

                  SetEvent (pDevice->hOutputFinished);
                  continue;
                }
              }
            }
          }

          pDevice->ulLastFrameOutput =
                   SK_GetFramesDrawn ();

          pDevice->dwLastTimeOutput =
            SK::ControlPanel::current_time;

          bEnqueued = false;
          bFinished = false;
        }

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



int SK_HID_DeviceFile::neutralizeHidInput (uint8_t report_id)
{
  switch (device_vid)
  {
    case SK_HID_VID_SONY:
    {
      switch (device_pid)
      {
        case SK_HID_PID_DUALSHOCK4:
        case SK_HID_PID_DUALSHOCK4_REV2:
        {
          if (report_id == 1 && _cachedInputReportsByReportId [report_id].size () >= 34)
          {
            if (_cachedInputReportsByReportId [report_id].data ()[0] == report_id)
            {
              SK_HID_DualShock4_GetStateData* pData =
                (SK_HID_DualShock4_GetStateData *)&(_cachedInputReportsByReportId [report_id].data ()[1]);

              // The analog axes
              memset (
                &pData->LeftStickX,  128,   4);
              memset (
                &pData->TriggerLeft,   0,   2);
                 pData->Counter++;
                 pData->DPad               = Neutral;
                 pData->ButtonSquare       = 0;
                 pData->ButtonCross        = 0;
                 pData->ButtonCircle       = 0;
                 pData->ButtonTriangle     = 0;
                 pData->ButtonL1           = 0;
                 pData->ButtonR1           = 0;
                 pData->ButtonL2           = 0;
                 pData->ButtonR2           = 0;
                 pData->ButtonShare        = 0;
                 pData->ButtonOptions      = 0;
                 pData->ButtonL3           = 0;
                 pData->ButtonR3           = 0;
                 pData->ButtonHome         = 0;
                 pData->ButtonPad          = 0;
              memset (
                &pData->AngularVelocityX, 0, 12);

              return 34;
            }
          }
        } break;
        case SK_HID_PID_DUALSENSE_EDGE:
        case SK_HID_PID_DUALSENSE:
        {
          if (  _cachedInputReportsByReportId [report_id].size () >= 64)
          {
            if (_cachedInputReportsByReportId [report_id].data ()[0] == 1)
            {
              SK_HID_DualSense_GetStateData* pData =
                (SK_HID_DualSense_GetStateData *)&(_cachedInputReportsByReportId [report_id].data ()[1]);

              // The analog axes
              memset (
                &pData->LeftStickX,  128,   4);
              memset (
                &pData->TriggerLeft,   0,   2);
                 pData->SeqNo++;
                 pData->DPad               = Neutral;
                 pData->ButtonSquare       = 0;
                 pData->ButtonCross        = 0;
                 pData->ButtonCircle       = 0;
                 pData->ButtonTriangle     = 0;
                 pData->ButtonL1           = 0;
                 pData->ButtonR1           = 0;
                 pData->ButtonL2           = 0;
                 pData->ButtonR2           = 0;
                 pData->ButtonCreate       = 0;
                 pData->ButtonOptions      = 0;
                 pData->ButtonL3           = 0;
                 pData->ButtonR3           = 0;
                 pData->ButtonHome         = 0;
                 pData->ButtonPad          = 0;
                 pData->ButtonMute         = 0;
                 pData->UNK1               = 0;
                 pData->ButtonLeftFunction = 0;
                 pData->ButtonRightFunction= 0;
                 pData->ButtonLeftPaddle   = 0;
                 pData->ButtonRightPaddle  = 0;
                 pData->UNK2               = 0;
              memset (
                &pData->AngularVelocityX, 0, 12);

              return 64;
            }

            else if (_cachedInputReportsByReportId [report_id].data ()[0] == 49)
            {
              BYTE* pInputRaw =
                (BYTE *)_cachedInputReportsByReportId [report_id].data ();

              SK_HID_DualSense_GetStateData *pData =
                (SK_HID_DualSense_GetStateData *)&pInputRaw [2];

              bool bSimple = false;

              // We're in simplified mode...
              if (pInputRaw [0] == 0x1)
              {
                bSimple = true;
              }

              if (! bSimple)
              {
                // The analog axes
                memset (
                  &pData->LeftStickX,  128,   4);
                memset (
                  &pData->TriggerLeft,   0,   2);
                   pData->SeqNo++;
                   pData->DPad               = Neutral;
                   pData->ButtonSquare       = 0;
                   pData->ButtonCross        = 0;
                   pData->ButtonCircle       = 0;
                   pData->ButtonTriangle     = 0;
                   pData->ButtonL1           = 0;
                   pData->ButtonR1           = 0;
                   pData->ButtonL2           = 0;
                   pData->ButtonR2           = 0;
                   pData->ButtonCreate       = 0;
                   pData->ButtonOptions      = 0;
                   pData->ButtonL3           = 0;
                   pData->ButtonR3           = 0;
                   pData->ButtonHome         = 0;
                   pData->ButtonPad          = 0;
                   pData->ButtonMute         = 0;
                   pData->UNK1               = 0;
                   pData->ButtonLeftFunction = 0;
                   pData->ButtonRightFunction= 0;
                   pData->ButtonLeftPaddle   = 0;
                   pData->ButtonRightPaddle  = 0;
                   pData->UNK2               = 0;
                memset (
                  &pData->AngularVelocityX, 0, 12);
              }

              else
              {
                SK_HID_DualSense_GetSimpleStateDataBt *pSimpleData =
                  (SK_HID_DualSense_GetSimpleStateDataBt *)&pInputRaw [1];

                // The analog axes
                memset (
                  &pSimpleData->LeftStickX,  128,   4);
                memset (
                  &pSimpleData->TriggerLeft,   0,   2);
                   pSimpleData->Counter++;
                   pSimpleData->DPad               = Neutral;
                   pSimpleData->ButtonSquare       = 0;
                   pSimpleData->ButtonCross        = 0;
                   pSimpleData->ButtonCircle       = 0;
                   pSimpleData->ButtonTriangle     = 0;
                   pSimpleData->ButtonL1           = 0;
                   pSimpleData->ButtonR1           = 0;
                   pSimpleData->ButtonL2           = 0;
                   pSimpleData->ButtonR2           = 0;
                   pSimpleData->ButtonShare        = 0;
                   pSimpleData->ButtonOptions      = 0;
                   pSimpleData->ButtonL3           = 0;
                   pSimpleData->ButtonR3           = 0;
                   pSimpleData->ButtonHome         = 0;
                   pSimpleData->ButtonPad          = 0;
              }

              return 64;
            }
          }
        } break;
      }
    } break;
  }

  return 0;
}

int SK_HID_DeviceFile::remapHidInput (void)
{
  switch (device_vid)
  {
    case SK_HID_VID_SONY:
    {
      switch (device_pid)
      {
        case SK_HID_PID_DUALSENSE_EDGE:
        case SK_HID_PID_DUALSENSE:
        {
          if (last_data_read.size () >= 64)
          {
            if (last_data_read [0] == 0x1)
            {
              SK_HID_DualSense_GetStateData* pData =
                (SK_HID_DualSense_GetStateData *)&(last_data_read [1]);

              // The analog axes
                 pData->DPad               = Neutral;
                 pData->ButtonSquare       = 0;
                 pData->ButtonCross        = 0;
                 pData->ButtonCircle       = 0;
                 pData->ButtonTriangle     = 0;
                 pData->ButtonL1           = 0;
                 pData->ButtonR1           = 0;
                 pData->ButtonL2           = 0;
                 pData->ButtonR2           = 0;
                 pData->ButtonCreate       = 0;
                 pData->ButtonOptions      = 0;
                 pData->ButtonL3           = 0;
                 pData->ButtonR3           = 0;
                 pData->ButtonHome         = 0;

                 bool create = 
                   pData->ButtonCreate;
                 bool pad = 
                   pData->ButtonPad;
                 
                 pData->ButtonPad    = create;
                 pData->ButtonCreate = pad;
                 pData->ButtonMute         = 0;
                 pData->UNK1               = 0;
                 pData->ButtonLeftFunction = 0;
                 pData->ButtonRightFunction= 0;
                 pData->ButtonLeftPaddle   = 0;
                 pData->ButtonRightPaddle  = 0;
                 pData->UNK2               = 0;
              memset (
                &pData->AngularVelocityX, 0, 12);

              return 64;
            }
            //else if (last_data_read [0] == 0x31)
            //{
            //  SK_HID_DualSense_GetStateData* pData =
            //    (SK_HID_DualSense_GetStateData *)&(last_data_read [2]);
            //
            //  // The analog axes
            //  memset (
            //    &pData->LeftStickX,  128,   4);
            //  memset (
            //    &pData->TriggerLeft,   0,   2);
            //     pData->SeqNo++;
            //     pData->DPad               = Neutral;
            //     pData->ButtonSquare       = 0;
            //     pData->ButtonCross        = 0;
            //     pData->ButtonCircle       = 0;
            //     pData->ButtonTriangle     = 0;
            //     pData->ButtonL1           = 0;
            //     pData->ButtonR1           = 0;
            //     pData->ButtonL2           = 0;
            //     pData->ButtonR2           = 0;
            //     pData->ButtonCreate       = 0;
            //     pData->ButtonOptions      = 0;
            //     pData->ButtonL3           = 0;
            //     pData->ButtonR3           = 0;
            //     pData->ButtonHome         = 0;
            //     pData->ButtonPad          = 0;
            //     pData->ButtonMute         = 0;
            //     pData->UNK1               = 0;
            //     pData->ButtonLeftFunction = 0;
            //     pData->ButtonRightFunction= 0;
            //     pData->ButtonLeftPaddle   = 0;
            //     pData->ButtonRightPaddle  = 0;
            //     pData->UNK2               = 0;
            //  memset (
            //    &pData->AngularVelocityX, 0, 12);
            //
            //  return 78;
            //}
          }
        } break;
      }
    } break;
  }

  return 0;
}