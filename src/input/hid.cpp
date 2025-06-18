// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
HidD_GetAttributes_pfn         HidD_GetAttributes_Original     = nullptr;
HidP_GetData_pfn               HidP_GetData_Original           = nullptr;
HidP_GetCaps_pfn               HidP_GetCaps_Original           = nullptr;
HidP_GetUsages_pfn             HidP_GetUsages_Original         = nullptr;

HidD_GetAttributes_pfn         SK_HidD_GetAttributes           = nullptr;
HidD_GetProductString_pfn      SK_HidD_GetProductString        = nullptr;
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



#define __SK_HID_CalculateLatency

enum class SK_Input_DeviceFileType : int
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

SK_HID_DeviceFile::SK_HID_DeviceFile (HANDLE file, const wchar_t *wszPath)
{
  // Make sure our custom HID DLL is loaded before initializing devices
  if (! SK_HidD_GetAttributes)
  {
    SK_RunOnce (SK_Input_PreHookHID ());
  }

  static
    concurrency::concurrent_unordered_map <std::wstring, SK_HID_DeviceFile> known_paths;

  // This stuff can be REALLY slow when games are constantly enumerating devices,
  //   so keep a cache handy.
  if (known_paths.count (wszPath))
  {
    auto& known_path = known_paths.at (wszPath);
    hidpCaps         = known_path.hidpCaps;

    wcsncpy_s ( wszDevicePath, MAX_PATH,
                wszPath,       _TRUNCATE );

    wcsncpy_s (            wszProductName,      2047,
                known_path.wszProductName,           _TRUNCATE );
    wcsncpy_s (            wszManufacturerName, 2047,
                known_path.wszManufacturerName,      _TRUNCATE );
    wcsncpy_s (            wszSerialNumber,     2047,
                known_path.wszSerialNumber,          _TRUNCATE );

    device_type         = known_path.device_type;
    device_vid          = known_path.device_vid;
    device_pid          = known_path.device_pid;
    bDisableDevice      = known_path.bDisableDevice;

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
                wszProductName, 4093, &dwBytesRead, nullptr
              );

              SK_DeviceIoControl (
                hFile, IOCTL_HID_GET_MANUFACTURER_STRING, 0, 0,
                wszManufacturerName, 4093, &dwBytesRead, nullptr
              );

              SK_DeviceIoControl (
                hFile, IOCTL_HID_GET_SERIALNUMBER_STRING, 0, 0,
                wszSerialNumber, 4093, &dwBytesRead, nullptr
              );

              setPollingFrequency (0);

#ifdef DEBUG
              SK_ImGui_Warning (wszSerialNumber);
#endif

              HIDD_ATTRIBUTES hidAttribs      = {                      };
                              hidAttribs.Size = sizeof (HIDD_ATTRIBUTES);

              SK_HidD_GetAttributes (hFile, &hidAttribs);

              device_pid = hidAttribs.ProductID;
              device_vid = hidAttribs.VendorID;

              device_type = sk_input_dev_type::Gamepad;

              ULONG ulBuffers = 0;

              if (config.input.gamepad.hid.max_allowed_buffers > 0)
              {
                dwBytesRead = 0;

                SK_DeviceIoControl (
                  hFile, IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS,
                    nullptr, 0, &ulBuffers, sizeof (ULONG),
                      &dwBytesRead, nullptr );

                if (ulBuffers > (UINT)config.input.gamepad.hid.max_allowed_buffers)
                  setBufferCount     (config.input.gamepad.hid.max_allowed_buffers);
                setPollingFrequency  (0);
                
                dwBytesRead = 0;
                
                SK_DeviceIoControl (
                  hFile, IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS,
                    nullptr, 0, &ulBuffers, sizeof (ULONG),
                      &dwBytesRead, nullptr );
              }
            } break;

            case HID_USAGE_GENERIC_POINTER:
            case HID_USAGE_GENERIC_MOUSE:
            {
              device_type = sk_input_dev_type::Mouse;

              setPollingFrequency (0);
            } break;

            case HID_USAGE_GENERIC_KEYBOARD:
            case HID_USAGE_GENERIC_KEYPAD:
            {
              device_type = sk_input_dev_type::Keyboard;

              setPollingFrequency (0);
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
#if 0
            if (config.system.log_level > 0)
            {
              SK_DeviceIoControl (
                hFile, IOCTL_HID_GET_PRODUCT_STRING, 0, 0,
                wszProductName, 4093, &dwBytesRead, nullptr
              );

              SK_DeviceIoControl (
                hFile, IOCTL_HID_GET_MANUFACTURER_STRING, 0, 0,
                wszManufacturerName, 4093, &dwBytesRead, nullptr
              );

              SK_DeviceIoControl (
                hFile, IOCTL_HID_GET_SERIALNUMBER_STRING, 0, 0,
                wszSerialNumber, 4093, &dwBytesRead, nullptr
              );

              SK_ImGui_Warning (wszSerialNumber);

              SK_LOGi1 (
                L"Unknown HID Device Type (Product=%ws):  UsagePage=%x, Usage=%x",
                  wszProductName, hidpCaps.UsagePage, hidpCaps.Usage
              );
            }
#endif
          }
        }

        SK_ReleaseAssert ( // WTF?
          SK_HidD_FreePreparsedData (preparsed_data) != FALSE
        );

        setPollingFrequency (0);
      }
    }

    known_paths.insert ({ wszPath, *this });
  }

  setPollingFrequency (0);
  setBufferCount      (config.input.gamepad.hid.max_allowed_buffers);
}

bool
SK_HID_DeviceFile::setPollingFrequency (DWORD dwFreq)
{
  if ((intptr_t)hFile <= 0)
    return false;

  return
    SK_DeviceIoControl (
      hFile, IOCTL_HID_SET_POLL_FREQUENCY_MSEC,
        &dwFreq, sizeof (DWORD), nullptr, 0,
          nullptr, nullptr );
}

bool
SK_HID_DeviceFile::setBufferCount (DWORD dwBuffers)
{
  if (dwBuffers < 2)
  {
    SK_LOGi0 (
      L"Invalid Buffer Count (%u) passed to "
      L"SK_HID_DeviceFile::setBufferCount (...); "
      L"Using minimum supported value (2) instead.",
      dwBuffers
    );

    dwBuffers = 2;
  }

  if ((intptr_t)hFile <= 0)
    return false;

  return
    SK_DeviceIoControl (
      hFile, IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS,
        &dwBuffers, sizeof (dwBuffers), nullptr,
          0, nullptr, nullptr
    );
}

bool
SK_HID_DeviceFile::isInputAllowed (void) const
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

       concurrency::concurrent_vector        <SK_HID_PlayStationDevice>     SK_HID_PlayStationControllers;
       concurrency::concurrent_unordered_map <HANDLE, SK_HID_DeviceFile>    SK_HID_DeviceFiles;
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

XINPUT_STATE     hid_to_xi { };
volatile ULONG64 hid_to_xi_time = 0;

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

BOOLEAN
WINAPI
HidD_GetAttributes_Detour (_In_  HANDLE           HidDeviceObject,
                           _Out_ PHIDD_ATTRIBUTES Attributes)
{
  SK_LOG_FIRST_CALL

  BOOLEAN ret =
    HidD_GetAttributes_Original (HidDeviceObject, Attributes);

  if (ret)
  {
    if (Attributes->VendorID == SK_HID_VID_SONY)
    {
      if (config.input.gamepad.scepad.show_ds4_v1_as_v2 == SK_Enabled)
      {
        if (Attributes->ProductID == SK_HID_PID_DUALSHOCK4)
        {   Attributes->ProductID  = SK_HID_PID_DUALSHOCK4_REV2;
            SK_RunOnce (SK_LOGi0 (L"Identifying DualShock 4 controller as DualShock 4 v2 to game."));
        }
      }

      if (config.input.gamepad.scepad.hide_ds4_v2_pid == SK_Enabled)
      {
        if (Attributes->ProductID == SK_HID_PID_DUALSHOCK4_REV2)
        {   Attributes->ProductID  = SK_HID_PID_DUALSHOCK4;
            SK_RunOnce (SK_LOGi0 (L"Identifying DualShock 4 v2 controller as DualShock 4 to game."));
        }

        if (Attributes->ProductID == SK_HID_PID_DUALSHOCK4_DONGLE)
        {   Attributes->ProductID  = SK_HID_PID_DUALSHOCK4;
            SK_RunOnce (SK_LOGi0 (L"Identifying DualShock 4 (via Dongle) controller as DualShock 4 to game."));
        }
      }

      if (config.input.gamepad.scepad.hide_ds_edge_pid == SK_Enabled)
      {
        if (Attributes->ProductID == SK_HID_PID_DUALSENSE_EDGE)
        {   Attributes->ProductID  = SK_HID_PID_DUALSENSE;
            SK_RunOnce (SK_LOGi0 (L"Identifying DualSense Edge controller as DualSense to game."));
        }
      }
    }
  }

  return ret;
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
WriteFile_pfn             WriteFile_Original             = nullptr;
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
WriteFile_Detour (HANDLE       hFile,
                  LPCVOID      lpBuffer,
                  DWORD        nNumberOfBytesToWrite,
                  LPDWORD      lpNumberOfBytesWritten,
                  LPOVERLAPPED lpOverlapped)
{
  SK_LOG_FIRST_CALL

  for ( auto callback : plugin_mgr->write_file_fns )
             callback (hFile);

  // Fast path, we only care about HID Input Reports
  if (nNumberOfBytesToWrite <= 31 || nNumberOfBytesToWrite >= 4096)
  {
    return
      WriteFile_Original (
            hFile, lpBuffer, nNumberOfBytesToWrite,
              lpNumberOfBytesWritten, lpOverlapped
      );
  }

  const auto &[ dev_file_type, dev_ptr, dev_allowed ] =
    SK_Input_GetDeviceFileAndState (hFile);

  if (dev_ptr != nullptr)
  {
    switch (dev_file_type)
    {
      case SK_Input_DeviceFileType::HID:
      {
        if (config.input.gamepad.disable_hid)
        {
          SetLastError (ERROR_DEVICE_NOT_CONNECTED);
          return FALSE;
        }

        if (! dev_allowed)
        {
          memset ((void *)lpBuffer, 0, nNumberOfBytesToWrite);
        }

        else
        {
          auto hid_file =
            (SK_HID_DeviceFile *)(dev_ptr);

          BYTE report_id = 0;

          if (lpBuffer != nullptr)
          {
            report_id = ((uint8_t *)(lpBuffer))[0];
          }

          hid_file->filterHidOutput (report_id, nNumberOfBytesToWrite, (void *)lpBuffer);

          // Do not report this, some engines seem to open temporary devices to write
          //   controller state to and then close them.
#if 0
          if ( hid_file->bytes_read    == 0 &&
               hid_file->bytes_written == 0 )
          {
            SK_ImGui_CreateNotification (
              "HID.GamepadAttached", SK_ImGui_Toast::Info,
              *hid_file->wszManufacturerName != L'\0' ?
                SK_FormatString ("%ws: %ws\r\n\tVID: 0x%04x | PID: 0x%04x",
                   hid_file->wszManufacturerName,
                   hid_file->wszProductName, hid_file->device_vid,
                                             hid_file->device_pid ).c_str () :
                SK_FormatString ("Generic Driver: %ws\r\n\tVID: 0x%04x | PID: 0x%04x",
                   hid_file->wszProductName, hid_file->device_vid,
                                             hid_file->device_pid ).c_str (),
              "Native (HID class) Gamepad Protocol In Use By Game", 10000
            );
          }
#endif

          hid_file->bytes_written += nNumberOfBytesToWrite;
        }
      } break;

      default:
      {
        SK_RunOnce (
          SK_LOGi1 (L"Unexpected Device Type (%d) [%ws:%d]", dev_file_type,
                                                  __FILEW__, __LINE__) );
      } break;
    }
  }

  return
    WriteFile_Original (
      hFile, lpBuffer, nNumberOfBytesToWrite,
        lpNumberOfBytesWritten, lpOverlapped );
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

  for ( auto callback : plugin_mgr->read_file_fns )
             callback (hFile);

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

  if (dev_ptr != nullptr)
  {
    switch (dev_file_type)
    {
      case SK_Input_DeviceFileType::HID:
      {
        if (config.input.gamepad.scepad.unlimit_polling_rate)
        {
          config.input.gamepad.scepad.pollig_thread_tid =
            SK_GetCurrentThreadId ();
        }

        if (config.input.gamepad.disable_hid)
        {
          SetLastError (ERROR_DEVICE_NOT_CONNECTED);
          return FALSE;
        }

        auto hid_file =
          (SK_HID_DeviceFile *)(dev_ptr);

        BYTE report_id = 0;

        if (lpBuffer != nullptr)
        {
          report_id = ((uint8_t *)(lpBuffer))[0];

          if (! report_id)
          {     report_id = sk::narrow_cast <BYTE> (nNumberOfBytesToRead);
          }
        }

        void* pBuffer = lpBuffer;

        if (! dev_allowed)
        {
          if (nNumberOfBytesToRead > 0)
          {
            if (        hid_file->_blockedInputReportsBySize [nNumberOfBytesToRead].size () < nNumberOfBytesToRead)
                        hid_file->_blockedInputReportsBySize [nNumberOfBytesToRead].resize (nNumberOfBytesToRead);
              pBuffer = hid_file->_blockedInputReportsBySize [nNumberOfBytesToRead].data ();

            if (report_id > 0 && hid_file->_cachedInputReportsByReportId [report_id].size () >= nNumberOfBytesToRead)
            {
              if (hid_file->neutralizeHidInput (report_id, nNumberOfBytesToRead))
              {
                if (                   lpBuffer != nullptr)
                  SK_UNTRUSTED_memcpy (lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (), nNumberOfBytesToRead);
              }
            }

            else if (report_id > 0 && hid_file->canNeutralizeInput (report_id, nNumberOfBytesToRead))
            {
              if (
                ReadFile_Original (
                  hFile, lpBuffer, nNumberOfBytesToRead,
                    lpNumberOfBytesRead, lpOverlapped
                )
              )
              {
                hid_file->neutralizeHidInput (report_id, nNumberOfBytesToRead, lpBuffer);
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
          size_t nNumberOfBytesRead = lpNumberOfBytesRead != nullptr ?
                             (size_t)*lpNumberOfBytesRead : (size_t)nNumberOfBytesToRead;

          if ( hid_file->bytes_read    == 0 /* &&
               hid_file->bytes_written == 0  */ )
          {
            SK_COMPAT_ApplyHIDAttachFixUps ();

            const bool harmful =
              config.input.gamepad.xinput.emulate || SK_XInput_PollController (0);

            if (config.input.gamepad.hid.always_show_attach || harmful)
            {
              auto format_str = (! harmful) ?
                    "%ws: %ws\r\n\tVID: 0x%04x | PID: 0x%04x" :
                    "%ws: %ws\r\n\r\n\tXInput emulation (i.e. Steam Input, DS4Windows, etc.) software"
                                "\r\n\tmay prevent the game from using advanced input features.\r\n";

            SK_ImGui_CreateNotification (
              "HID.GamepadAttached", SK_ImGui_Toast::Info,
              *hid_file->wszManufacturerName != L'\0' ?
                SK_FormatString (format_str,
                  hid_file->wszManufacturerName,
                  hid_file->wszProductName, hid_file->device_vid,
                                            hid_file->device_pid ).c_str () :
                SK_FormatString ("Generic Driver: %ws\r\n\tVID: 0x%04x | PID: 0x%04x",
                  hid_file->wszProductName, hid_file->device_vid,
                                            hid_file->device_pid ).c_str (),
              "Native (HID class) Gamepad Protocol In Use By Game", 10000
            );
          }
          }

          hid_file->bytes_read += nNumberOfBytesToRead;

          if (lpBuffer != nullptr)
          {
            report_id = ((uint8_t *)(lpBuffer))[0];

            if (! report_id)
            {
              report_id = lpNumberOfBytesRead != nullptr ?
                (uint8_t)*lpNumberOfBytesRead : (BYTE)nNumberOfBytesToRead;
            }

            hid_file->filterHidInput (report_id, (DWORD)nNumberOfBytesRead, lpBuffer);
          }

          auto& cached_report =
            hid_file->_cachedInputReportsByReportId [report_id];

          if ((! lpOverlapped) && dev_allowed)
          {
            if (cached_report.size () < nNumberOfBytesRead && nNumberOfBytesRead > 0)
                cached_report.resize (  nNumberOfBytesRead);

            if (                                          lpBuffer != nullptr)
              SK_UNTRUSTED_memcpy (cached_report.data (), lpBuffer, nNumberOfBytesRead);
          }

          bool bDeviceAllowed = dev_allowed;

          if (report_id > 0 && lpOverlapped != nullptr)
          {
            ///if (hid_file->_cachedInputReportsByReportId [report_id].size () < nNumberOfBytesRead)
            ///{   hid_file->_cachedInputReportsByReportId [report_id].resize (  nNumberOfBytesRead);
            ///}
            ///
            ///if (((uint8_t *)lpBuffer)[0] != 0)
            ///{
            ///  hid_file->_cachedInputReportsByReportId [report_id].data ()[0] = ((uint8_t *)lpBuffer)[0];
            ///}
            ///
            ///if (hid_file->neutralizeHidInput (report_id, nNumberOfBytesRead))
            ///{
            ///  SK_UNTRUSTED_memcpy (
            ///    lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (),
            ///        nNumberOfBytesToRead );
            ///}
          }

          if (! bDeviceAllowed)
          {
            SK_ReleaseAssert (lpBuffer != nullptr);

            SK_HID_HIDE (hid_file->device_type);

            if (lpOverlapped == nullptr) // lpNumberOfBytesRead MUST be non-null
            {
              if (report_id > 0 && cached_report.size () >= nNumberOfBytesToRead)
              {
                // Give the game old data, from before we started blocking stuff...
                int num_bytes_read =
                  hid_file->neutralizeHidInput (report_id, nNumberOfBytesToRead);
              
                if (num_bytes_read > 0)
                {
                  if (lpNumberOfBytesRead != nullptr)
                     *lpNumberOfBytesRead = num_bytes_read;
                }

                else
                {
                  num_bytes_read = lpNumberOfBytesRead != nullptr ?
                             (int)*lpNumberOfBytesRead : num_bytes_read;
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

              if (report_id > 0 && cached_report.size () >= nNumberOfBytesToRead)
              {
                // Give the game old data, from before we started blocking stuff...
                int num_bytes_read =
                  hid_file->neutralizeHidInput (report_id, nNumberOfBytesToRead);

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

            if (lpOverlapped == nullptr && lpBuffer != nullptr)
            {
              report_id = ((uint8_t *)(lpBuffer))[0];
            }

            DWORD dwBytesRead = 0;

            if (lpNumberOfBytesRead != nullptr)
               dwBytesRead = *lpNumberOfBytesRead;
            else
               dwBytesRead = nNumberOfBytesToRead;

            if (lpOverlapped == nullptr && report_id > 0 && lpBuffer != nullptr)
            {
              if (hid_file->_cachedInputReportsByReportId [report_id].size () <= dwBytesRead)
                  hid_file->_cachedInputReportsByReportId [report_id].resize (   dwBytesRead);

              if (dwBytesRead > 0)
              {
                hid_file->_cachedInputReportsByReportId [report_id].data ()[0] = report_id;

                //if (hid_file->neutralizeHidInput (report_id, nNumberOfBytesToRead))
                {
                  SK_UNTRUSTED_memcpy (
                    hid_file->_cachedInputReportsByReportId [report_id].data (), lpBuffer,
                        dwBytesRead );

                  hid_file->neutralizeHidInput (report_id, dwBytesRead);
                }
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

      default:
      {
        SK_RunOnce (
          SK_LOGi1 (L"Unexpected Device Type (%d) [%ws:%d]", dev_file_type,
                                                  __FILEW__, __LINE__) );
      } break;
    }
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

      uint8_t report_id = ((uint8_t *)(lpBuffer))[0];
      hid_file->filterHidInput (report_id, nNumberOfBytesToRead, lpBuffer);

      if ( hid_file->bytes_read    == 0 /* &&
           hid_file->bytes_written == 0  */ )
      {
        SK_COMPAT_ApplyHIDAttachFixUps ();

        const bool harmful =
          config.input.gamepad.xinput.emulate || SK_XInput_PollController (0);

        if (config.input.gamepad.hid.always_show_attach || harmful)
        {
          auto format_str = (! harmful) ?
                "%ws: %ws\r\n\tVID: 0x%04x | PID: 0x%04x" :
                "%ws: %ws\r\n\r\n\tXInput emulation (i.e. Steam Input, DS4Windows, etc.) software"
                            "\r\n\tmay prevent the game from using advanced input features.\r\n";

          SK_ImGui_CreateNotification (
            "HID.GamepadAttached", SK_ImGui_Toast::Info,
            *hid_file->wszManufacturerName != L'\0' ?
              SK_FormatString (format_str,
                hid_file->wszManufacturerName,
                hid_file->wszProductName, hid_file->device_vid,
                                          hid_file->device_pid ).c_str () :
              SK_FormatString ("Generic Driver: %ws\r\n\tVID: 0x%04x | PID: 0x%04x",
                hid_file->wszProductName, hid_file->device_vid,
                                          hid_file->device_pid ).c_str (),
            "Native (HID class) Gamepad Protocol In Use By Game", 10000
          );
        }
      }

      hid_file->bytes_read += nNumberOfBytesToRead;

      SK_HID_READ (hid_file->device_type);
      SK_HID_VIEW (hid_file->device_type);
    } break;

    case SK_Input_DeviceFileType::NVIDIA:
    {
      SK_MessageBus_Backend->markRead (2);
    } break;

    default:
    {
      SK_RunOnce (
        SK_LOGi1 (L"Unexpected Device Type (%d) [%ws:%d]", dev_file_type,
                                                __FILEW__, __LINE__) );
    } break;
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

  len =
    std::min (len, sk::narrow_cast <int> (wcslen (wszPattern)));

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

  len =
    std::min (len, sk::narrow_cast <int> (strlen (szString)));

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

  // hTemplateFile is unused for OPEN_EXISTING,
  //   remove it so that it does not trigger Application Verifier.
  if (dwCreationDisposition == OPEN_EXISTING)
      hTemplateFile = 0;

  if (lpFileName == nullptr)
  {
    SetLastError (ERROR_NO_SUCH_DEVICE);
  
    return INVALID_HANDLE_VALUE;
  }

  HANDLE hRet =
    CreateFileA_Original (
      lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
          dwFlagsAndAttributes, hTemplateFile );

  const bool bSuccess = LONG_PTR (hRet) > 0;

  if (bSuccess)
  {
    for ( auto callback : plugin_mgr->open_file_a_fns )
    {
      callback (lpFileName, hRet);
    }
  }

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

        auto& dev_file =
          SK_HID_DeviceFiles [hRet];

        for (auto& overlapped_request : dev_file._overlappedRequests)
        {
          overlapped_request.second.dwNumberOfBytesToRead = 0;
          overlapped_request.second.hFile                 = INVALID_HANDLE_VALUE;
          overlapped_request.second.lpBuffer              = nullptr;
        }

        dev_file = hid_file;
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

  if (lpFileName == nullptr)
  {
    SetLastError (ERROR_NO_SUCH_DEVICE);
  
    return INVALID_HANDLE_VALUE;
  }

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

        if (nullptr != hRet)
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

        auto& dev_file =
          SK_HID_DeviceFiles [hRet];

        for (auto& overlapped_request : dev_file._overlappedRequests)
        {
          overlapped_request.second.dwNumberOfBytesToRead = 0;
          overlapped_request.second.hFile                 = INVALID_HANDLE_VALUE;
          overlapped_request.second.lpBuffer              = nullptr;
        }

        dev_file = hid_file;
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

  // hTemplateFile is unused for OPEN_EXISTING,
  //   remove it so that it does not trigger Application Verifier.
  if (dwCreationDisposition == OPEN_EXISTING)
      hTemplateFile = 0;

  if (lpFileName == nullptr)
  {
    SetLastError (ERROR_NO_SUCH_DEVICE);
  
    return INVALID_HANDLE_VALUE;
  }

  HANDLE hRet =
    CreateFileW_Original (
      lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
          dwFlagsAndAttributes, hTemplateFile );

  const bool bSuccess = (LONG_PTR)hRet > 0;

  if (bSuccess)
  {
    for ( auto callback : plugin_mgr->open_file_w_fns )
    {
      callback (lpFileName, hRet);
    }
  }

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

        if (nullptr != hRet)
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

        auto& dev_file =
          SK_HID_DeviceFiles [hRet];

        for (auto& overlapped_request : dev_file._overlappedRequests)
        {
          overlapped_request.second.dwNumberOfBytesToRead = 0;
          overlapped_request.second.hFile                 = INVALID_HANDLE_VALUE;
          overlapped_request.second.lpBuffer              = nullptr;
        }

        dev_file = hid_file;
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

  if (    dev_ptr != nullptr && lpNumberOfBytesTransferred != nullptr)
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

        uint8_t report_id = overlapped_request.lpBuffer != nullptr ?
                ((uint8_t *)overlapped_request.lpBuffer) [0]       : 0x0;

        bRet =
          GetOverlappedResultEx_Original (
            hFile, lpOverlapped, lpNumberOfBytesTransferred, dwMilliseconds, bWait
          );

        DWORD dwSize = *lpNumberOfBytesTransferred;

        if (overlapped_request.lpBuffer != nullptr)
        {
          bool neutralized = false;

          if (dwSize != 0 && hid_file->_cachedInputReportsByReportId [report_id].size () >= dwSize)
          {
            if (hid_file->neutralizeHidInput (report_id, dwSize))
            {
              neutralized = true;
            }

            SK_UNTRUSTED_memcpy (
              overlapped_request.lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (), dwSize
            );
          }

          if (! neutralized)
          {
            *lpNumberOfBytesTransferred = 0;
          }
        }

        return bRet;
      }

      bRet =
        GetOverlappedResultEx_Original (
          hFile, lpOverlapped, lpNumberOfBytesTransferred, dwMilliseconds, bWait
        );

      if (bRet != FALSE)
      {
        uint8_t report_id = overlapped_request.lpBuffer != nullptr ?
                ((uint8_t *)overlapped_request.lpBuffer)[0]        : 0x0;

        if (dev_allowed)
        {
          if (*lpNumberOfBytesTransferred != 0)
          {
            if (overlapped_request.lpBuffer != nullptr)
              hid_file->filterHidInput (report_id, *lpNumberOfBytesTransferred, overlapped_request.lpBuffer);

            auto& cached_report =
              hid_file->_cachedInputReportsByReportId [report_id];

            if (cached_report.size () < *lpNumberOfBytesTransferred && *lpNumberOfBytesTransferred > 0)
                cached_report.resize (  *lpNumberOfBytesTransferred);

            if (overlapped_request.lpBuffer != nullptr && cached_report.size () >= *lpNumberOfBytesTransferred)
            {
              if (((uint8_t *)overlapped_request.lpBuffer)[0] == 0x0 ||
                  ((uint8_t *)overlapped_request.lpBuffer)[0] == report_id)
              {
                SK_UNTRUSTED_memcpy (cached_report.data (), overlapped_request.lpBuffer, *lpNumberOfBytesTransferred);
              }
            }
          }

          SK_HID_READ (hid_file->device_type);
          SK_HID_VIEW (hid_file->device_type);
          // We did the bulk of this processing in ReadFile_Detour (...)
          //   nothing to be done here for now.
        }

        else
        {
          DWORD dwSize =
            *lpNumberOfBytesTransferred;

          if (overlapped_request.lpBuffer != nullptr)
          {
            bool neutralized = false;

            if (dwSize != 0 && hid_file->_cachedInputReportsByReportId [report_id].size () >= dwSize)
            {
              if (hid_file->neutralizeHidInput (report_id, dwSize))
              {
                neutralized = true;
              }

              SK_UNTRUSTED_memcpy (
                overlapped_request.lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (), dwSize
              );
            }

            if (! neutralized)
            {
              dwSize = 0;
            }
          }

          *lpNumberOfBytesTransferred = dwSize;
        }
      }

      return bRet;
    } break;

    case SK_Input_DeviceFileType::NVIDIA:
    {
      SK_MessageBus_Backend->markRead (2);
    } break;

    default:
    {
      SK_RunOnce (
        SK_LOGi1 (L"Unexpected Device Type (%d) [%ws:%d]", dev_file_type,
                                                __FILEW__, __LINE__) );
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

  if (    dev_ptr != nullptr && lpNumberOfBytesTransferred != nullptr)
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

        uint8_t report_id = overlapped_request.lpBuffer != nullptr ?
                ((uint8_t *)overlapped_request.lpBuffer) [0]       : 0x0;

        bRet =
          GetOverlappedResult_Original (
            hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait
          );

        DWORD dwSize = *lpNumberOfBytesTransferred;

        if (overlapped_request.lpBuffer != nullptr)
        {
          bool neutralized = false;

          if (dwSize != 0 && hid_file->_cachedInputReportsByReportId [report_id].size () >= dwSize)
          {
            if (hid_file->neutralizeHidInput (report_id, dwSize))
            {
              neutralized = true;
            }

            SK_UNTRUSTED_memcpy (
              overlapped_request.lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (), dwSize
            );
          }

          if (! neutralized)
          {
            *lpNumberOfBytesTransferred = 0;
          }
        }

        return bRet;
      }

      bRet =
        GetOverlappedResult_Original (
          hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait
        );

      if (bRet != FALSE)
      {
        uint8_t report_id = overlapped_request.lpBuffer != nullptr ?
                ((uint8_t *)overlapped_request.lpBuffer)[0]        : 0x0;

        if (dev_allowed)
        {
          if (*lpNumberOfBytesTransferred != 0)
          {
            if (overlapped_request.lpBuffer != nullptr)
              hid_file->filterHidInput (report_id, *lpNumberOfBytesTransferred, overlapped_request.lpBuffer);

            auto& cached_report =
              hid_file->_cachedInputReportsByReportId [report_id];

            if (cached_report.size () < *lpNumberOfBytesTransferred && *lpNumberOfBytesTransferred > 0)
                cached_report.resize (  *lpNumberOfBytesTransferred);

            if (overlapped_request.lpBuffer != nullptr && cached_report.size () >= *lpNumberOfBytesTransferred)
            {
              if (((uint8_t *)overlapped_request.lpBuffer)[0] == 0x0 ||
                  ((uint8_t *)overlapped_request.lpBuffer)[0] == report_id)
              {
                SK_UNTRUSTED_memcpy (cached_report.data (), overlapped_request.lpBuffer, *lpNumberOfBytesTransferred);
              }
            }
          }

          SK_HID_READ (hid_file->device_type);
          SK_HID_VIEW (hid_file->device_type);
          // We did the bulk of this processing in ReadFile_Detour (...)
          //   nothing to be done here for now.
        }

        else
        {
          DWORD dwSize = 
            *lpNumberOfBytesTransferred;

          if (overlapped_request.lpBuffer != nullptr)
          {
            bool neutralized = false;

            if (dwSize != 0 && hid_file->_cachedInputReportsByReportId [report_id].size () >= dwSize)
            {
              if (hid_file->neutralizeHidInput (report_id, dwSize))
              {
                neutralized = true;
              }

              SK_UNTRUSTED_memcpy (
                overlapped_request.lpBuffer, hid_file->_cachedInputReportsByReportId [report_id].data (), dwSize
              );
            }

            if (! neutralized)
            {
              dwSize = 0;
            }
          }

          *lpNumberOfBytesTransferred = dwSize;
        }
      }

      return bRet;
    } break;

    case SK_Input_DeviceFileType::NVIDIA:
    {
      SK_MessageBus_Backend->markRead (2);
    } break;

    default:
    {
      SK_RunOnce (
        SK_LOGi1 (L"Unexpected Device Type (%d) [%ws:%d]", dev_file_type,
                                                __FILEW__, __LINE__) );
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
    L"DeviceIoControl %p (Type: %x, Function: %u", hDevice,
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
  static bool _once = false;
  if (        _once == false && SK_GetCallingDLL () != SK_GetDLL ())
  {           _once = true;
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
  static bool _once = false;
  if (        _once == false && SK_GetCallingDLL () != SK_GetDLL ())
  {           _once = true;
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
  static bool _once = false;
  if (        _once == false && SK_GetCallingDLL () != SK_GetDLL ())
  {           _once = true;
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
  static bool _once = false;
  if (        _once == false && SK_GetCallingDLL () != SK_GetDLL ())
  {           _once = true;
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
  static bool _once = false;
  if (        _once == false && SK_GetCallingDLL () != SK_GetDLL ())
  {           _once = true;
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
  static bool _once = false;
  if (        _once == false && SK_GetCallingDLL () != SK_GetDLL ())
  {           _once = true;
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
  static bool _once = false;
  if (        _once == false && SK_GetCallingDLL () != SK_GetDLL ())
  {           _once = true;
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
  static bool _once = false;
  if (        _once == false && SK_GetCallingDLL () != SK_GetDLL ())
  {           _once = true;
    SK_LOG_FIRST_CALL
  }

  return
    SK_SetupDiDestroyDeviceInfoList (DeviceInfoSet);
}

#pragma pack (push,8)
#define SystemHandleInformationSize 0x400000

enum SKIF_PROCESS_INFORMATION_CLASS
{
    ProcessBasicInformation                     = 0x00, //  0
    ProcessDebugPort                            = 0x07, //  7
    ProcessWow64Information                     = 0x1A, // 26
    ProcessImageFileName                        = 0x1B, // 27
    ProcessBreakOnTermination                   = 0x1D, // 29
    ProcessSubsystemInformation                 = 0x4B  // 75
};

enum SYSTEM_INFORMATION_CLASS
{ SystemBasicInformation                = 0,
  SystemPerformanceInformation          = 2,
  SystemTimeOfDayInformation            = 3,
  SystemProcessInformation              = 5,
  SystemProcessorPerformanceInformation = 8,
  SystemHandleInformation               = 16, // 0x10
  SystemInterruptInformation            = 23,
  SystemExceptionInformation            = 33,
  SystemRegistryQuotaInformation        = 37,
  SystemLookasideInformation            = 45,
  SystemExtendedHandleInformation       = 64, // 0x40
  SystemCodeIntegrityInformation        = 103,
  SystemPolicyInformation               = 134,
};

typedef enum _OBJECT_INFORMATION_CLASS {
  ObjectBasicInformation,
  ObjectNameInformation,
  ObjectTypeInformation,
  ObjectTypesInformation,
  ObjectHandleFlagInformation,
  ObjectSessionInformation,
} OBJECT_INFORMATION_CLASS;

using NtQueryInformationProcess_pfn =
  NTSTATUS (NTAPI *)(
       IN  HANDLE                    Handle,
       IN  SKIF_PROCESS_INFORMATION_CLASS ProcessInformationClass, // PROCESSINFOCLASS 
       OUT PVOID                     ProcessInformation,
       IN  ULONG                     ProcessInformationLength,
       OUT PULONG                    ReturnLength OPTIONAL
  );

typedef struct _SK_SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
  USHORT UniqueProcessId;
  USHORT CreatorBackTraceIndex;
  UCHAR  ObjectTypeIndex;
  UCHAR  HandleAttributes;
  USHORT HandleValue;
  PVOID  Object;
  ULONG  GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO,
*PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SK_SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX
{
  PVOID       Object;
  union
  {
    ULONG_PTR UniqueProcessId;

    struct
    {
#ifdef _WIN64
      DWORD   ProcessId;
      DWORD   ThreadId; // ?? ( What are the upper-bits for anyway ? )
#else
      WORD    ProcessId;
      WORD    ThreadId; // ?? ( What are the upper-bits for anyway ? )
#endif
    };
  };

  union
  {
    ULONG_PTR HandleValue;
    HANDLE    Handle;
  };

  ULONG       GrantedAccess;
  USHORT      CreatorBackTraceIndex;
  USHORT      ObjectTypeIndex;
  ULONG       HandleAttributes;
  ULONG       Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX,
*PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

typedef struct _SK_SYSTEM_HANDLE_INFORMATION
{ ULONG                          NumberOfHandles;
  SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles     [1];
} SYSTEM_HANDLE_INFORMATION,
 *PSYSTEM_HANDLE_INFORMATION;

typedef struct _SK_SYSTEM_HANDLE_INFORMATION_EX
{ ULONG_PTR                         NumberOfHandles;
  ULONG_PTR                         Reserved;
  SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles     [1];
} SYSTEM_HANDLE_INFORMATION_EX,
*PSYSTEM_HANDLE_INFORMATION_EX;

typedef struct _SK_UNICODE_STRING
{ USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING,
*PUNICODE_STRING;

// MIT: https://github.com/antonioCoco/ConPtyShell/blob/master/ConPtyShell.cs
typedef struct _OBJECT_TYPE_INFORMATION_V2 {
  UNICODE_STRING  TypeName;
  ULONG           TotalNumberOfObjects;
  ULONG           TotalNumberOfHandles;
  ULONG           TotalPagedPoolUsage;
  ULONG           TotalNonPagedPoolUsage;
  ULONG           TotalNamePoolUsage;
  ULONG           TotalHandleTableUsage;
  ULONG           HighWaterNumberOfObjects;
  ULONG           HighWaterNumberOfHandles;
  ULONG           HighWaterPagedPoolUsage;
  ULONG           HighWaterNonPagedPoolUsage;
  ULONG           HighWaterNamePoolUsage;
  ULONG           HighWaterHandleTableUsage;
  ULONG           InvalidAttributes;
  GENERIC_MAPPING GenericMapping;
  ULONG           ValidAccessMask;
  BOOLEAN         SecurityRequired;
  BOOLEAN         MaintainHandleCount;
  UCHAR           TypeIndex;    // Added in V2
  CHAR            ReservedByte; // Added in V2
  ULONG           PoolType;
  ULONG           DefaultPagedPoolCharge;
  ULONG           DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION_V2,
*POBJECT_TYPE_INFORMATION_V2;

typedef struct _SK_OBJECT_NAME_INFORMATION
{ UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION,
*POBJECT_NAME_INFORMATION;

using NtQueryObject_pfn =
  NTSTATUS (NTAPI *)(
       IN  HANDLE                   Handle       OPTIONAL,
       IN  OBJECT_INFORMATION_CLASS ObjectInformationClass,
       OUT PVOID                    ObjectInformation,
       IN  ULONG                    ObjectInformationLength,
       OUT PULONG                   ReturnLength OPTIONAL
  );

using NtQuerySystemInformation_pfn =
  NTSTATUS (NTAPI *)(
        IN  SYSTEM_INFORMATION_CLASS SystemInformationClass,
        OUT PVOID                    SystemInformation,
        IN  ULONG                    SystemInformationLength,
        OUT PULONG                   ReturnLength OPTIONAL
  );

using NtQueryObject_pfn =
  NTSTATUS (NTAPI *)(
       IN  HANDLE                   Handle       OPTIONAL,
       IN  OBJECT_INFORMATION_CLASS ObjectInformationClass,
       OUT PVOID                    ObjectInformation,
       IN  ULONG                    ObjectInformationLength,
       OUT PULONG                   ReturnLength OPTIONAL
  );

using NtDuplicateObject_pfn =
  NTSTATUS (NTAPI *)(
        IN  HANDLE      SourceProcessHandle,
        IN  HANDLE      SourceHandle,
        OUT HANDLE      TargetProcessHandle,
        OUT PHANDLE     TargetHandle,
        IN  ACCESS_MASK DesiredAccess OPTIONAL,
        IN  ULONG       Attributes    OPTIONAL,
        IN  ULONG       Options       OPTIONAL
  );

// CC BY-SA 3.0: https://stackoverflow.com/a/39104745
typedef struct _OBJECT_TYPES_INFORMATION {
  LONG NumberOfTypes;
} OBJECT_TYPES_INFORMATION,
*POBJECT_TYPES_INFORMATION;

// CC BY-SA 3.0: https://stackoverflow.com/a/39104745
//               https://jadro-windows.cz/download/ntqueryobject.zip
#define ALIGN_DOWN(Length, Type)       (            (ULONG)(Length) & ~(sizeof(Type) - 1))
#define ALIGN_UP(Length, Type)         (ALIGN_DOWN(((ULONG)(Length) +   sizeof(Type) - 1), Type))

// CC BY-SA 3.0: https://stackoverflow.com/a/39104745
//               https://jadro-windows.cz/download/ntqueryobject.zip
// Modified to use POBJECT_TYPE_INFORMATION_V2 instead of POBJECT_TYPE_INFORMATION
USHORT GetTypeIndexByName (std::wstring TypeName)
{
  static NtQueryObject_pfn
         NtQueryObject =
        (NtQueryObject_pfn)SK_GetProcAddress (L"NtDll",
        "NtQueryObject");

  POBJECT_TYPE_INFORMATION_V2  TypeInfo = NULL;
  POBJECT_TYPES_INFORMATION   TypesInfo = NULL;
  USHORT   ret                          = USHRT_MAX;
  NTSTATUS ntStatTypesInfo;
  ULONG    BufferLength = SystemHandleInformationSize;

  auto* pTLS =
      SK_TLS_Bottom ();

  auto& types_info_buffer =
    pTLS->local_scratch->query->NtInfo;

  do
  {
    TypesInfo = (POBJECT_TYPES_INFORMATION)types_info_buffer.alloc (BufferLength);

    ntStatTypesInfo =
      NtQueryObject (NULL, ObjectTypesInformation, TypesInfo, BufferLength, &BufferLength);
  } while (ntStatTypesInfo == STATUS_INFO_LENGTH_MISMATCH && BufferLength != 0);

  if (NT_SUCCESS (ntStatTypesInfo) && TypesInfo->NumberOfTypes > 0)
  {
    // Align to first element of the array
    TypeInfo = (POBJECT_TYPE_INFORMATION_V2)((PCHAR)TypesInfo + ALIGN_UP(sizeof(*TypesInfo), ULONG_PTR));
    for (int i = 0; i < TypesInfo->NumberOfTypes; i++)
    {
      //USHORT     _TypeIndex = i + 2;               // OBJECT_TYPE_INFORMATION requires adding 2 to get the proper type index
      USHORT       _TypeIndex = TypeInfo->TypeIndex; // OBJECT_TYPE_INFORMATION_V2 includes it in the struct
      std::wstring _TypeName  = std::wstring(TypeInfo->TypeName.Buffer, TypeInfo->TypeName.Length / sizeof(WCHAR));

      if (TypeName == _TypeName)
      {
        ret = _TypeIndex;
        break;
      }

      // Align to the next element of the array
      TypeInfo = (POBJECT_TYPE_INFORMATION_V2)((PCHAR)(TypeInfo + 1) + ALIGN_UP(TypeInfo->TypeName.MaximumLength, ULONG_PTR));
    }
  }

  return ret;
}
#pragma pack (pop)

void
SK_Input_EnumOpenHIDFiles (void)
{
  static HANDLE hDeviceEnumThread =
  SK_Thread_CreateEx ([](LPVOID)->DWORD
  {
    auto* pTLS =
      SK_TLS_Bottom ();

    static NtQuerySystemInformation_pfn
           NtQuerySystemInformation =
          (NtQuerySystemInformation_pfn)SK_GetProcAddress (L"NtDll",
          "NtQuerySystemInformation");

    static NtDuplicateObject_pfn
           NtDuplicateObject =
          (NtDuplicateObject_pfn)SK_GetProcAddress (L"NtDll",
          "NtDuplicateObject");

    static NtQueryObject_pfn
           NtQueryObject =
          (NtQueryObject_pfn)SK_GetProcAddress (L"NtDll",
          "NtQueryObject");

    static USHORT FileIndex = GetTypeIndexByName (L"File");

    WaitForInit ();

    SK_PerfEvent_Begin (L"SK_Input_EnumOpenHIDFiles");
    
    auto& handle_info_buffer =
      pTLS->local_scratch->query->NtInfo;

    NTSTATUS                        ns           = STATUS_INFO_LENGTH_MISMATCH;
    DWORD                         dwReadBytes    =     0UL;
    PSYSTEM_HANDLE_INFORMATION_EX  pHandleInfoEx = nullptr;

    for ( DWORD dSize   =  SystemHandleInformationSize ;
                dSize  != 0 && !pHandleInfoEx          ;
                dSize <<= 1 )
    {
      handle_info_buffer.alloc (static_cast <size_t> (dSize));

      pHandleInfoEx =
        (PSYSTEM_HANDLE_INFORMATION_EX)handle_info_buffer.data;

      ns =
        NtQuerySystemInformation (
          SystemExtendedHandleInformation,
                                 handle_info_buffer.data,
            static_cast <ULONG> (handle_info_buffer.len),
            &dwReadBytes );

      if (! NT_SUCCESS (ns))
      {
        pHandleInfoEx = nullptr;
        dwReadBytes   = 0;
        
        if (ns != STATUS_INFO_LENGTH_MISMATCH)
          break;
      }
    }
    
    if (NT_SUCCESS (ns) && dwReadBytes != 0 && pHandleInfoEx != nullptr)
    {
      const DWORD dwPidOfMe =
        GetCurrentProcessId ();

      PROCESSENTRY32W
      FindProcessByName (const wchar_t* wszName);

      DWORD dwSteamClientPid = config.input.gamepad.steam.disabled_to_game ?
        FindProcessByName (L"steam.exe").th32ProcessID                     : 0x0;

      SK_AutoHandle hSteamProcess (
        dwSteamClientPid != 0 ?
          OpenProcess ( PROCESS_DUP_HANDLE |
                        PROCESS_QUERY_INFORMATION,
                          FALSE,
                            dwSteamClientPid )
                              :
          INVALID_HANDLE_VALUE
      );

      // Go through all handles of the system
      for ( unsigned int i = 0;
                         i < pHandleInfoEx->NumberOfHandles;
                         i++ )
      {
        // We only want files
        if (pHandleInfoEx->Handles [i].ObjectTypeIndex != FileIndex)
          continue;

        // Skip handles not belong to the game or Steam
        if (pHandleInfoEx->Handles [i].ProcessId       != dwPidOfMe &&
            pHandleInfoEx->Handles [i].ProcessId       != dwSteamClientPid)
          continue;
    
        HANDLE file =
          pHandleInfoEx->Handles [i].Handle;

        HIDD_ATTRIBUTES hidAttribs      = {                      };
                        hidAttribs.Size = sizeof (HIDD_ATTRIBUTES);

        if (pHandleInfoEx->Handles [i].ProcessId == dwSteamClientPid)
        {
          if (config.input.gamepad.steam.disabled_to_game && hSteamProcess.isValid ())
          {
            if (DuplicateHandle ( hSteamProcess,     file,
                              GetCurrentProcess (), &file, 0, FALSE, DUPLICATE_SAME_ACCESS ))
            {
              SK_AutoHandle hRemoteFile (file);
              if (SK_HidD_GetAttributes (file, &hidAttribs))
              {
                if (hidAttribs.VendorID == SK_HID_VID_SONY)
                {
                  SK_LOGi0 (
                    L"The Steam Client has a HID Device File Open ( vid=%04x, pid=%04x )",
                      hidAttribs.VendorID, hidAttribs.ProductID
                  );

                  HANDLE hSayonaraSteamInput;

                  if ( DuplicateHandle ( hSteamProcess,    pHandleInfoEx->Handles [i].Handle,
                                     GetCurrentProcess (), &hSayonaraSteamInput, 0, FALSE, DUPLICATE_CLOSE_SOURCE ))
                  {    CloseHandle                         (hSayonaraSteamInput);
                    SK_LOGi0 (L" * Not anymore, you evil bastard!");
                  }
                }
              }
            }
          }
        }

        if (pHandleInfoEx->Handles [i].ProcessId != dwPidOfMe)
          continue;

#if 0
        std::wstring handle_name = L"";

        ULONG                 _ObjectNameLen ( MAX_PATH );
        std::vector <uint8_t> pObjectName;

        for ( NTSTATUS ntStat  = STATUS_INFO_LENGTH_MISMATCH ;
                       ntStat == STATUS_INFO_LENGTH_MISMATCH ; )
        {
          pObjectName.resize (
              _ObjectNameLen );

          ntStat =
            NtQueryObject ( file,
                    ObjectNameInformation,
                  pObjectName.data (),
                  _ObjectNameLen,
                  &_ObjectNameLen
            );

          if (NT_SUCCESS (ntStat))
          {
             POBJECT_NAME_INFORMATION _pni =
            (POBJECT_NAME_INFORMATION) pObjectName.data ();

            handle_name = _pni != nullptr ?
                          _pni->Name.Length > 0 ?
                          _pni->Name.Buffer     : L""
                                                : L"";
          }
        }

        SK_LOGi0 (L"File Name=%ws", handle_name.c_str ());
#endif

        if (SK_HidD_GetAttributes (file, &hidAttribs))
        {
          PHIDP_PREPARSED_DATA                 preparsed_data = nullptr;
          if (SK_HidD_GetPreparsedData (file, &preparsed_data))
          {
            HIDP_CAPS                                                    hidpCaps;
            if (HIDP_STATUS_SUCCESS == SK_HidP_GetCaps (preparsed_data, &hidpCaps))
            {
              if (hidpCaps.UsagePage == HID_USAGE_PAGE_GENERIC)
              {
                switch (hidpCaps.Usage)
                {
                  case HID_USAGE_GENERIC_GAMEPAD:
                  case HID_USAGE_GENERIC_JOYSTICK:
                  case HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER:
                  {
                    if (SK_Input_DeviceFiles.insert (file).second)
                    {
                      SK_HID_DeviceFile hid_file (file, L"");

                      if (hid_file.device_type == sk_input_dev_type::Gamepad)
                      {
                        SK_LOGi0 (
                          L"Registering Already Open HID Device File: ( vid=%04x, pid=%04x )",
                            hid_file.device_vid, hid_file.device_pid
                        );

                        auto& dev_file =
                          SK_HID_DeviceFiles [file];

                        for (auto& overlapped_request : dev_file._overlappedRequests)
                        {
                          overlapped_request.second.dwNumberOfBytesToRead = 0;
                          overlapped_request.second.hFile                 = INVALID_HANDLE_VALUE;
                          overlapped_request.second.lpBuffer              = nullptr;
                        }

                        dev_file = hid_file;
                      }
                    }
                  } break;
                }
              }
            }

            SK_HidD_FreePreparsedData (preparsed_data);
          }
        }
      }
    }

    SK_PerfEvent_End (L"SK_Input_EnumOpenHIDFiles");

    SK_Thread_CloseSelf ();

    return 0;
  }, L"[SK] Existing HID Device Enumerator");
}

void
SK_Input_HookHID (void)
{
  if (! config.input.gamepad.hook_hid)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    SK_PROFILE_FIRST_CALL

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
                             "HidD_GetAttributes",
                              HidD_GetAttributes_Detour,
     static_cast_p2p <void> (&HidD_GetAttributes_Original) );

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

    //SK_CreateDLLHook2 (      L"kernel32.dll",
    //                          "DeviceIoControl",
    //                           DeviceIoControl_Detour,
    //  static_cast_p2p <void> (&DeviceIoControl_Original) );

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

#define _HOOK_WRITE_FILE
#ifdef  _HOOK_WRITE_FILE
    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "WriteFile",
                               WriteFile_Detour,
      static_cast_p2p <void> (&WriteFile_Original) );
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

    SK_ApplyQueuedHooks ();

    InterlockedIncrementRelease (&hooked);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}

bool
SK_Input_PreHookHID (void)
{
  bool ret = true;

  // Only do this once, and make all other threads trying to do it wait
  //
  static volatile LONG             _init  =  0;
  if (InterlockedCompareExchange (&_init, 1, 0) == 0)
  {
    SK_PROFILE_FIRST_CALL

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

    SK_HidD_GetProductString =
      (HidD_GetProductString_pfn)SK_GetProcAddress (hModHID,
      "HidD_GetProductString");

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

    ret = false;

    if (config.input.gamepad.hook_hid)
    {
      static sk_import_test_s tests [] = {
        { "hid.dll", false }
      };

      SK_TestImports (
        SK_GetModuleHandle (nullptr), tests, 1
      );

      if (tests [0].used || SK_GetModuleHandle (L"hid.dll"))
      {
        SK_Input_HookHID ();

        ret = true;
      }

      SK_Input_EnumOpenHIDFiles ();
    }

    InterlockedIncrement (&_init);
  }

  else
  {
    SK_Thread_SpinUntilAtomicMin (&_init, 2);
  }

  return ret;
}

SK_HID_PlayStationDevice*
SK_HID_GetActivePlayStationDevice (bool return_null_if_xbox_is_active) noexcept
{
  if (return_null_if_xbox_is_active && SK_Input_IsXboxControllerActive ())
    return nullptr;

  SK_HID_PlayStationDevice *pNewestInputDevice = nullptr;

  for ( auto& ps_controller : SK_HID_PlayStationControllers )
  {
    if (ps_controller.bConnected)
    {
      if (pNewestInputDevice == nullptr || ps_controller.xinput.isNewer (pNewestInputDevice->xinput))
      {
        pNewestInputDevice = &ps_controller;
      }
    }
  }

  return pNewestInputDevice;
}