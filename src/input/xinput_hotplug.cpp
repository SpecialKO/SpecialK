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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"HIDHotplug"

#include <SpecialK/input/xinput_hotplug.h>
#include <hidclass.h>
#include <imgui/font_awesome.h>


HWND SK_hWndDeviceListener;


struct {
  volatile DWORD RecheckInterval = 2500UL;
  volatile LONG  holding         = FALSE;
  volatile DWORD last_polled     = 0UL;

  DWORD getRefreshTime (void) { return ReadULongAcquire (&RecheckInterval); }
  DWORD lastPolled     (void) { return ReadULongAcquire (&last_polled);     }

  void  setRefreshTime (DWORD dwTime)
  {
    WriteULongRelease (&RecheckInterval, dwTime);
  }
  void  updatePollTime (DWORD dwTime = SK_timeGetTime ())
  {
    WriteULongRelease (&last_polled, dwTime);
  }
} static placeholders [XUSER_MAX_COUNT + 1] = {
  { 3333UL, FALSE, 0 }, {  5000UL, FALSE, 0 }, {    6666UL, FALSE, 0 },
                        { 10000UL, FALSE, 0 }, { 9999999UL, FALSE, 0 }
};

// One extra for overflow, XUSER_MAX_COUNT is never a valid index
std::array <SK_XInput_PacketJournal, XUSER_MAX_COUNT + 1> xinput_packets;


SK_XInput_PacketJournal
SK_XInput_GetPacketJournal (DWORD dwUserIndex) noexcept
{
  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, XUSER_MAX_COUNT - 1UL)];

  if (dwUserIndex     >=    XUSER_MAX_COUNT)
     return xinput_packets [XUSER_MAX_COUNT];

  return
    xinput_packets [dwUserIndex];
}


bool
SK_XInput_Holding (DWORD dwUserIndex)
{
  if (dwUserIndex >= XUSER_MAX_COUNT)
    return false;

  if (ReadAcquire (&placeholders [dwUserIndex].holding))
  {
    DWORD dwRecheck =
      placeholders [dwUserIndex].getRefreshTime ();

    if (dwRecheck < 6666UL)
         placeholders [dwUserIndex].setRefreshTime (dwRecheck * 2);
    else placeholders [dwUserIndex].setRefreshTime (SK_timeGetTime ());

    return true;
  }

  placeholders [dwUserIndex].setRefreshTime (133UL);

  return false;
}

std::set <HANDLE> SK_HID_DeviceArrivalEvents;
std::set <HANDLE> SK_HID_DeviceRemovalEvents;

void SK_HID_AddDeviceArrivalEvent (HANDLE hEvent)
{
  if (SK_HID_DeviceRemovalEvents.empty () &&
      SK_HID_DeviceArrivalEvents.empty ())
         SK_XInput_NotifyDeviceArrival ();

  SK_HID_DeviceArrivalEvents.emplace (hEvent);
}

void SK_HID_AddDeviceRemovalEvent (HANDLE hEvent)
{
  if (SK_HID_DeviceRemovalEvents.empty () &&
      SK_HID_DeviceArrivalEvents.empty ())
         SK_XInput_NotifyDeviceArrival ();

  SK_HID_DeviceRemovalEvents.emplace (hEvent);
}

static HANDLE SK_XInputHot_NotifyEvent       = 0;
static HANDLE SK_XInputHot_ReconnectThread   = 0;

// Wakes the dumb polling thread when its job is done
static HANDLE SK_XInputCold_DecommisionEvent = 0;

void
SK_XInput_RefreshControllers (void)
{
  for ( auto slot : { 0, 1, 2, 3 } )
  {
    SK_XInput_Refresh        (slot);
    SK_XInput_PollController (slot);
  }
};

DWORD
WINAPI
SK_XInput_GetCapabilities (_In_  DWORD                dwUserIndex,
                           _In_  DWORD                dwFlags,
                           _Out_ XINPUT_CAPABILITIES *pCapabilities);

LRESULT
WINAPI
SK_HID_DeviceNotifyProc (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  if (           Msg == WM_DEVICECHANGE   &&
      (void *)lParam != nullptr           &&
             (wParam == DBT_DEVICEARRIVAL ||
              wParam == DBT_DEVICEREMOVECOMPLETE))
  {
    DEV_BROADCAST_HDR* pDevHdr =
      (DEV_BROADCAST_HDR *)lParam;

    if (pDevHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
    {
      const bool arrival =
        (wParam == DBT_DEVICEARRIVAL);

      SK_ReleaseAssert (
        pDevHdr->dbch_size >= sizeof (DEV_BROADCAST_DEVICEINTERFACE_W)
      );

      DEV_BROADCAST_DEVICEINTERFACE_W *pDev =
        (DEV_BROADCAST_DEVICEINTERFACE_W *)pDevHdr;

      if (IsEqualGUID (pDev->dbcc_classguid, GUID_DEVINTERFACE_HID) ||
          IsEqualGUID (pDev->dbcc_classguid, GUID_XUSB_INTERFACE_CLASS))
      {
        bool playstation = false;
        bool xinput      = IsEqualGUID (pDev->dbcc_classguid, GUID_XUSB_INTERFACE_CLASS);

        wchar_t    wszFileName [MAX_PATH];
        wcsncpy_s (wszFileName, MAX_PATH, pDev->dbcc_name, _TRUNCATE);

        SK_AutoHandle hDeviceFile (
          SK_CreateFileW ( wszFileName, FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                        FILE_SHARE_READ   | FILE_SHARE_WRITE,
                                          nullptr, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH  |
                                                                  FILE_ATTRIBUTE_TEMPORARY |
                                                                  FILE_FLAG_OVERLAPPED, nullptr )
                                );

        HIDD_ATTRIBUTES hidAttribs      = {                      };
                        hidAttribs.Size = sizeof (HIDD_ATTRIBUTES);

        wchar_t wszDeviceName [128] = L"Unknown";

        if (hDeviceFile.isValid ())
        {
          // If user disabled HID, this will be nullptr
          if (SK_HidD_GetProductString != nullptr)
          {
            SK_HidD_GetProductString (hDeviceFile.m_h, wszDeviceName, 254);
            SK_HidD_GetAttributes    (hDeviceFile.m_h, &hidAttribs);

            playstation |= ( hidAttribs.VendorID == SK_HID_VID_SONY );
          }
        }

        else
        {
          // On device removal, all we can do is go by the filename...
          playstation |= wcsstr (wszFileName, L"054c") != nullptr;
        }

        SK_LOG0 ( ( L" Device %s:\t%32ws (%s)", arrival ? L"Arrival"
                                                        : L"Removal",
                                              wszDeviceName,
                                              pDev->dbcc_name ),
                    __SK_SUBSYSTEM__ );

        xinput |= wcsstr (wszFileName, L"IG_") != nullptr;

        //
        // Device Arrival
        //
        if (arrival)
        {
          for (  auto event : SK_HID_DeviceArrivalEvents  )
            SetEvent (event);

          // XInput devices contain IG_...
          if ( xinput &&
               wcsstr (pDev->dbcc_name, LR"(\kbd)") == nullptr )
               // Ignore XInputGetKeystroke
          {
            XINPUT_CAPABILITIES caps = { };

            // Determine all connected XInput controllers and only
            //   refresh those that need it...
            for ( int i = 0 ; i < XUSER_MAX_COUNT ; ++i )
            {
              if ( ERROR_SUCCESS ==
                     SK_XInput_GetCapabilities (i, XINPUT_DEVTYPE_GAMEPAD, &caps) )
              {
                SK_XInput_Refresh        (i);
                SK_XInput_PollController (i);

                if ((intptr_t)SK_XInputCold_DecommisionEvent > 0)
                    SetEvent (SK_XInputCold_DecommisionEvent);
              }
            }
          }

          else if (playstation)
          {
            bool has_existing = false;

            for ( auto& controller : SK_HID_PlayStationControllers )
            {
              if (! _wcsicmp (controller.wszDevicePath, wszFileName))
              {
                // We missed a device removal event if this is true
                SK_ReleaseAssert (controller.bConnected == false);

                controller.hDeviceFile = hDeviceFile.Detach ();

                                                            // If user disabled HID, this will be nullptr
                if (controller.hDeviceFile != INVALID_HANDLE_VALUE &&
                      SK_HidD_GetPreparsedData != nullptr)
                { if (SK_HidD_GetPreparsedData (controller.hDeviceFile, &controller.pPreparsedData))
                  {
                    controller.bConnected = true;
                    controller.bBluetooth =  //Bluetooth_Base_UUID
                      StrStrIW (wszFileName, L"{00001124-0000-1000-8000-00805f9b34fb}");

                    controller.reset_device ();

                    controller.setBufferCount      (config.input.gamepad.hid.max_allowed_buffers);
                    controller.setPollingFrequency (0);

                    if (config.system.log_level > 0)
                      SK_ImGui_Warning (L"PlayStation Controller Reconnected");

                    has_existing = true;

                    if ((! controller.bBluetooth) || (! config.input.gamepad.bt_input_only))
                      controller.write_output_report ();

                    if (        controller.hReconnectEvent != nullptr)
                      SetEvent (controller.hReconnectEvent);
                  }
                }
                break;
              }
            }

            if (! has_existing)
            {
              SK_HID_PlayStationDevice controller (hDeviceFile.Detach ());

              controller.pid = hidAttribs.ProductID;
              controller.vid = hidAttribs.VendorID;

              controller.bBluetooth =
                StrStrIW (
                  controller.wszDevicePath, //Bluetooth_Base_UUID
                          L"{00001124-0000-1000-8000-00805f9b34fb}"
                );

              controller.bDualSense =
                (controller.pid == SK_HID_PID_DUALSENSE_EDGE) ||
                (controller.pid == SK_HID_PID_DUALSENSE);

              controller.bDualSenseEdge =
                controller.pid == SK_HID_PID_DUALSENSE_EDGE;

              controller.bDualShock4 =
                (controller.pid == SK_HID_PID_DUALSHOCK4)      ||
                (controller.pid == SK_HID_PID_DUALSHOCK4_REV2) ||
                (controller.pid == 0x0BA0); // Dongle

              controller.bDualShock3 =
                (controller.pid == SK_HID_PID_DUALSHOCK3);

              if (! (controller.bDualSense || controller.bDualShock4 || controller.bDualShock3))
              {
                if (controller.vid == SK_HID_VID_SONY)
                {
                  SK_LOGi0 (L"SONY Controller with Unknown PID ignored: %ws", wszFileName);
                }

                return
                  DefWindowProcW (hWnd, Msg, wParam, lParam);
              }

              wcsncpy_s (controller.wszDevicePath, MAX_PATH,
                                    wszFileName,   _TRUNCATE);

              if (controller.hDeviceFile != INVALID_HANDLE_VALUE)
              {
                controller.setBufferCount      (config.input.gamepad.hid.max_allowed_buffers);
                controller.setPollingFrequency (0);

                if (SK_HidD_GetPreparsedData != nullptr &&
                    SK_HidD_GetPreparsedData (controller.hDeviceFile, &controller.pPreparsedData))
                {
                  HIDP_CAPS                                      caps = { };
                    SK_HidP_GetCaps (controller.pPreparsedData, &caps);

                  controller.input_report.resize   (caps.InputReportByteLength);
                  controller.output_report.resize  (caps.OutputReportByteLength);
                  controller.feature_report.resize (caps.FeatureReportByteLength);

                  controller.initialize_serial ();

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

                    return
                      DefWindowProcW (hWnd, Msg, wParam, lParam);
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
                }

                controller.bConnected = true;
                controller.reset_device ();

                auto iter =
                  SK_HID_PlayStationControllers.push_back (controller);

                iter->write_output_report ();

                if (config.system.log_level > 0)
                  SK_ImGui_Warning (L"PlayStation Controller Connected");
              }
            }
          }
        }


        //
        // Device Removal
        //
        else
        {
          for (  auto event : SK_HID_DeviceRemovalEvents  )
            SetEvent (event);

          if ( xinput &&
               wcsstr (pDev->dbcc_name, LR"(\kbd)") == nullptr )
               // Ignore XInputGetKeystroke
          {
            // We really have no idea what controller this is, so refresh them all
            SetEvent (SK_XInputHot_NotifyEvent);

            if ((intptr_t)SK_XInputCold_DecommisionEvent > 0)
                SetEvent (SK_XInputCold_DecommisionEvent);
          }

          else// if (playstation)
          {
            for ( auto& controller : SK_HID_PlayStationControllers )
            {
              if (! _wcsicmp (controller.wszDevicePath, wszFileName))
              {
                controller.bConnected = false;
                controller.reset_device ();
                controller.reset_rgb  = false;

                if (                (intptr_t)controller.hDeviceFile > 0)
                  CloseHandle (std::exchange (controller.hDeviceFile,  nullptr));

                if (controller.pPreparsedData != nullptr &&
                    SK_HidD_FreePreparsedData != nullptr)
                    SK_HidD_FreePreparsedData (
                      std::exchange (controller.pPreparsedData, nullptr)
                    );

                if (        controller.hDisconnectEvent != nullptr)
                  SetEvent (controller.hDisconnectEvent);

                if (config.system.log_level > 0)
                  SK_ImGui_Warning (L"PlayStation Controller Disconnected");

                break;
              }
            }
          }
        }
      }

      return 1;
    }
  }

  return
    DefWindowProcW (hWnd, Msg, wParam, lParam);
};

DWORD
WINAPI
SK_HID_Hotplug_Dispatch (LPVOID user)
{
  HANDLE hNotify =
    (HANDLE)user;

  HANDLE phWaitObjects [2] = {
    hNotify, __SK_DLL_TeardownEvent
  };

  static constexpr DWORD ArrivalEvent  = ( WAIT_OBJECT_0     );
  static constexpr DWORD ShutdownEvent = ( WAIT_OBJECT_0 + 1 );

  std::wstring wnd_class_name =
    SK_FormatStringW (
      L"SK_HID_Listener_pid%x",
        GetCurrentProcessId () );

  WNDCLASSEXW
    wnd_class               = {                          };
    wnd_class.hInstance     = SK_GetModuleHandle (nullptr);
    wnd_class.lpszClassName = wnd_class_name.c_str ();
    wnd_class.lpfnWndProc   = SK_HID_DeviceNotifyProc;
    wnd_class.cbSize        = sizeof (WNDCLASSEXW);

  if (RegisterClassEx (&wnd_class))
  {
    DWORD dwWaitStatus = WAIT_OBJECT_0;

    SK_hWndDeviceListener =
      (HWND)CreateWindowEx ( 0, wnd_class_name.c_str (),     NULL, 0,
                             0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );

    // It's technically unnecessary to register this, but not a bad idea
    HDEVNOTIFY hDevNotify =
      SK_RegisterDeviceNotification (SK_hWndDeviceListener);

    do
    {
      if (hDevNotify != 0) {
        dwWaitStatus =
          MsgWaitForMultipleObjects (2, phWaitObjects, FALSE, INFINITE, QS_ALLINPUT);
      } else {
        dwWaitStatus = ArrivalEvent;
      }

      // Event is created in signaled state to queue a refresh in case of
      //   late inject
      if (dwWaitStatus == ArrivalEvent)
      {
        static ULONGLONG last_frame = 0ULL;
        auto             this_frame = SK_GetFramesDrawn ();

        // Do at most once per-frame, then pick up residuals next frame
        if (                       0 == this_frame ||
             std::exchange (last_frame, this_frame) <
                                        this_frame )
        {
          SK_XInput_RefreshControllers (                        );
          ResetEvent                   (SK_XInputHot_NotifyEvent);
        }

        else
        {
          dwWaitStatus =
            MsgWaitForMultipleObjects (0, nullptr, FALSE, 4UL, QS_ALLINPUT);
        }
      }

      if (dwWaitStatus != ShutdownEvent)
      {
        MSG                      msg = { };
        while (SK_PeekMessageW (&msg, SK_hWndDeviceListener, 0, 0, PM_REMOVE | PM_NOYIELD) > 0)
        {
          SK_TranslateMessage (&msg);
          SK_DispatchMessageW (&msg);
        }
      }
    } while (hDevNotify != 0 &&
           dwWaitStatus != ShutdownEvent);

    UnregisterDeviceNotification (hDevNotify);
    DestroyWindow                (SK_hWndDeviceListener);
    UnregisterClassW             (wnd_class.lpszClassName, wnd_class.hInstance);
  }

  else if (config.system.log_level > 0)
    SK_ReleaseAssert (! L"Failed to register Window Class!");

  if (                             SK_XInputHot_NotifyEvent!=nullptr)
    SK_CloseHandle (std::exchange (SK_XInputHot_NotifyEvent, nullptr));

  SK_Thread_CloseSelf ();

  return 0;
}

void
SK_XInput_NotifyDeviceArrival (void)
{
  SK_RunOnce (
  {
    SK_XInputHot_NotifyEvent =
              SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

    SK_XInputHot_ReconnectThread =
      SK_Thread_CreateEx ( SK_HID_Hotplug_Dispatch,
                       L"[SK] HID Hotplug Dispatch",
                 (LPVOID)SK_XInputHot_NotifyEvent );
  });
}


DWORD
WINAPI
SK_XInput_Polling_Thread (LPVOID user)
{
  if (! user)
  {
    SK_LOGi0 (L"Invalid user parameters in XInput Polling Thread Init.");
    SK_Thread_CloseSelf ();
    return 0;
  }

  SK_AutoHandle& hHotplugUnawareXInputRefresh =
    *(SK_AutoHandle *)user;

  SK_XInputCold_DecommisionEvent =
                  SK_CreateEvent (nullptr, TRUE, FALSE, nullptr);
  
  SetThreadPriority      ( SK_GetCurrentThread (),
    THREAD_PRIORITY_BELOW_NORMAL );
  SetThreadPriorityBoost ( SK_GetCurrentThread (),
    TRUE                         );
  
  HANDLE hWaitEvents [] = {
    hHotplugUnawareXInputRefresh.m_h, SK_XInputCold_DecommisionEvent,
                                              __SK_DLL_TeardownEvent
  };
  
  DWORD  dwWait =  WAIT_OBJECT_0;
  while (dwWait == WAIT_OBJECT_0)
  {
    dwWait =
      MsgWaitForMultipleObjects ( 3, hWaitEvents, FALSE,
                                        INFINITE, QS_ALLINPUT );
  
    XINPUT_STATE                  xstate = { };
    SK_XInput_PollController (0, &xstate);
    SK_XInput_PollController (1, &xstate);
    SK_XInput_PollController (2, &xstate);
    SK_XInput_PollController (3, &xstate);
  
    ResetEvent (hHotplugUnawareXInputRefresh.m_h);
  } while ( dwWait == WAIT_OBJECT_0 ); // Events #1 and #2 end this thread
  
  if (                (intptr_t)SK_XInputCold_DecommisionEvent > 0)
  { CloseHandle (std::exchange (SK_XInputCold_DecommisionEvent, (HANDLE)0));
  
    // One notification is enough to stop periodically testing
    //   slots and use event-based logic instead
    SK_XInput_SetRefreshInterval (SK_timeGetTime ());
  }
  
  hHotplugUnawareXInputRefresh.Close ();
  
  SK_Thread_CloseSelf ();
  
  return 0;
}

void
SK_XInput_DeferredStatusChecks (void)
{
  static SK_AutoHandle
    hHotplugUnawareXInputRefresh (
      SK_CreateEvent (nullptr, TRUE, FALSE, nullptr)
    );

  static HANDLE
    hXInputPollingThread =
      SK_Thread_CreateEx ( SK_XInput_Polling_Thread,
                       L"[SK] XInput Polling Thread",
                     &hHotplugUnawareXInputRefresh );

  static ULONG64 ullLastFrameChecked =
    SK_GetFramesDrawn ();

  const auto frames_drawn = 
    SK_GetFramesDrawn ();

  // Always refresh at the beginning of a frame rather than the end,
  //   a failure event may cause a lengthy delay, missing VBLANK.
  if (ullLastFrameChecked < frames_drawn - 30 &&
      (intptr_t)hHotplugUnawareXInputRefresh.m_h > 0)
  { SetEvent (  hHotplugUnawareXInputRefresh);
      ullLastFrameChecked = frames_drawn;
  }
}


void
SK_XInput_StopHolding (DWORD dwUserIndex)
{
  WriteRelease (&placeholders [dwUserIndex].holding, FALSE);
}

// Throttles polling while no actual device is connected, because
//   XInput polling failures are performance nightmares.
DWORD
SK_XInput_PlaceHold ( DWORD         dwRet,
                      DWORD         dwUserIndex,
                      XINPUT_STATE *pState )
{
  if (dwUserIndex >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
  if (pState      == nullptr)         return (DWORD)ERROR_INVALID_PARAMETER;

  const bool was_holding =
    ReadAcquire (&placeholders [dwUserIndex].holding);

  if ( dwRet != ERROR_SUCCESS &&
       (config.input.gamepad.xinput.placehold [dwUserIndex]) )
  {
    if (! ReadAcquire (&placeholders [dwUserIndex].holding))
    {                   placeholders [dwUserIndex].updatePollTime ();
         WriteRelease (&placeholders [dwUserIndex].holding, TRUE);
    }

    else
    {
      if (                     placeholders [dwUserIndex].lastPolled     () <
           SK_timeGetTime () - placeholders [dwUserIndex].getRefreshTime () )
      {
        // Re-check the next time this controller is polled
        WriteRelease (&placeholders [dwUserIndex].holding, FALSE);
      }
    }

    dwRet = ERROR_SUCCESS;

    auto& journal =
      xinput_packets [dwUserIndex];

    if (! was_holding)
    {
      journal.packet_count.virt++;
      journal.sequence.current++;
    }

    RtlZeroMemory (
      &pState->Gamepad, sizeof (XINPUT_GAMEPAD)
    );

    if (! was_holding)
    {
      journal.sequence.current =
        std::max   ( journal.sequence.current,
          std::max ( journal.packet_count.real,
                     journal.sequence.last ) );
    }

    pState->dwPacketNumber = journal.sequence.current;
                             journal.sequence.last = 0;
  }

  return dwRet;
}

DWORD
SK_XInput_PlaceHoldEx ( DWORD            dwRet,
                        DWORD            dwUserIndex,
                        XINPUT_STATE_EX *pState )
{
  return SK_XInput_PlaceHold ( dwRet, dwUserIndex, (XINPUT_STATE *)pState );
}

DWORD
SK_XInput_PlaceHoldCaps ( DWORD                dwRet,
                          DWORD                dwUserIndex,
                          DWORD                dwFlags,
                          XINPUT_CAPABILITIES *pCapabilities )
{
  UNREFERENCED_PARAMETER (dwFlags);

  if (dwUserIndex   >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
  if (pCapabilities == nullptr)         return (DWORD)E_POINTER;

  if ( dwRet != ERROR_SUCCESS &&
       (config.input.gamepad.xinput.placehold  [dwUserIndex]) )
  {
    if (! ReadAcquire (&placeholders [dwUserIndex].holding))
    {                   placeholders [dwUserIndex].updatePollTime ();
         WriteRelease (&placeholders [dwUserIndex].holding, TRUE);
    }

    else
    {
      if (                     placeholders [dwUserIndex].lastPolled     () <
           SK_timeGetTime () - placeholders [dwUserIndex].getRefreshTime () )
      {
        // Re-check the next time this controller is polled
        WriteRelease (&placeholders [dwUserIndex].holding, FALSE);
      }
    }

    dwRet = ERROR_SUCCESS;

    RtlZeroMemory (pCapabilities, sizeof XINPUT_CAPABILITIES);

    pCapabilities->Type    = XINPUT_DEVTYPE_GAMEPAD;
    pCapabilities->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;
    pCapabilities->Flags   = XINPUT_CAPS_FFB_SUPPORTED;
  }

  return dwRet;
}

DWORD
SK_XInput_PlaceHoldBattery ( DWORD                       dwRet,
                             DWORD                       dwUserIndex,
                             BYTE                        devType,
                             XINPUT_BATTERY_INFORMATION *pBatteryInformation )
{
  UNREFERENCED_PARAMETER (devType);

  if (dwUserIndex         >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
  if (pBatteryInformation == nullptr)         return (DWORD)E_POINTER;

  if ( dwRet != ERROR_SUCCESS &&
       (config.input.gamepad.xinput.placehold [dwUserIndex]) )
  {
    if (! ReadAcquire (&placeholders [dwUserIndex].holding))
    {                   placeholders [dwUserIndex].updatePollTime ();
         WriteRelease (&placeholders [dwUserIndex].holding, TRUE);
    }

    else
    {
      if (                     placeholders [dwUserIndex].lastPolled     () <
           SK_timeGetTime () - placeholders [dwUserIndex].getRefreshTime () )
      {
        // Re-check the next time this controller is polled
        WriteRelease (&placeholders [dwUserIndex].holding, FALSE);
      }
    }

    dwRet = ERROR_SUCCESS;

    pBatteryInformation->BatteryType  = BATTERY_TYPE_WIRED;
    pBatteryInformation->BatteryLevel = BATTERY_LEVEL_FULL;
  }

  return dwRet;
}

DWORD
SK_XInput_PlaceHoldSet ( DWORD             dwRet,
                         DWORD             dwUserIndex,
                         XINPUT_VIBRATION *pVibration )
{
  if (dwUserIndex >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
  if (pVibration  == nullptr)         return (DWORD)E_POINTER;

  if ( dwRet != ERROR_SUCCESS &&
       (config.input.gamepad.xinput.placehold  [dwUserIndex]) )
  {
    if (! ReadAcquire (&placeholders [dwUserIndex].holding))
    {                   placeholders [dwUserIndex].updatePollTime ();
         WriteRelease (&placeholders [dwUserIndex].holding, TRUE);
    }

    else
    {
      if (                     placeholders [dwUserIndex].lastPolled     () <
           SK_timeGetTime () - placeholders [dwUserIndex].getRefreshTime () )
      {         WriteRelease (&placeholders [dwUserIndex].holding, FALSE);
      }
    }

    dwRet = ERROR_SUCCESS;

    // Leave these whatever the game thinks/wants, it doesn't matter ;)
    //pVibration->wLeftMotorSpeed  = 0;
    //pVibration->wRightMotorSpeed = 0;
  }

  return dwRet;
}


// Make sure that virtual and physical controllers
//   always return monotonic results
void
SK_XInput_PacketJournalize (DWORD dwRet, DWORD dwUserIndex, XINPUT_STATE *pState)
{
  if (dwUserIndex >= XUSER_MAX_COUNT) return;
  if (pState      == nullptr)         return;

  if (dwRet == ERROR_SUCCESS)
  {
    auto& journal =
      xinput_packets [dwUserIndex];

    if ( journal.sequence.last != pState->dwPacketNumber )
    {
      journal.sequence.last = pState->dwPacketNumber;
      journal.packet_count.real++;
      journal.sequence.current++;

      SK_XInput_Backend->markViewed ((sk_input_dev_type)(1 << dwUserIndex));

      journal.sequence.current =
        std::max   ( journal.sequence.current,
          std::max ( journal.packet_count.real,
                     journal.sequence.last ) );

      pState->dwPacketNumber =
        journal.sequence.current;
    }
  }

  // Failure will be handled by placeholders if need be.
}





static constexpr GUID GUID_Zero =
  { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

using RegisterDeviceNotification_pfn = HDEVNOTIFY (WINAPI *)(
  _In_ HANDLE hRecipient,
  _In_ LPVOID NotificationFilter,
  _In_ DWORD  Flags
);

RegisterDeviceNotification_pfn RegisterDeviceNotificationW_Original = nullptr;

struct SK_Win32_DeviceNotificationInstance
{
  union {
    SERVICE_STATUS_HANDLE hServiceStatus;
    HWND                  hWnd;
  };

  DWORD dwFlags;
  bool  bUnicode;
};

#include <hidclass.h>

SK_LazyGlobal <concurrency::concurrent_vector <SK_Win32_DeviceNotificationInstance>> SK_Win32_RegisteredDevNotifications;

// These declarations look weird, but they're just allocating enough storage
//   for the base struct + a 260 character string...
static DEV_BROADCAST_DEVICEINTERFACE_W dbcc_xbox_w [16][20] = { };
static DEV_BROADCAST_DEVICEINTERFACE_W dbcc_hid_w  [16][20] = { };
static DEV_BROADCAST_DEVICEINTERFACE_A dbcc_xbox_a [16][10] = { };
static DEV_BROADCAST_DEVICEINTERFACE_A dbcc_hid_a  [16][10] = { };

SK_LazyGlobal <concurrency::concurrent_unordered_set <HWND>> SK_Win32_NotifiedWindows;

void
SK_Win32_NotifyHWND_W (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (hWnd == nullptr || ! IsWindow (hWnd))
    return;

  DWORD  dwProcId   = 0x0,
         dwThreadId =
  SK_GetWindowThreadProcessId (hWnd, &dwProcId);

  if (dwProcId != GetCurrentProcessId ())
    return;

  static UINT64 ulLastFrameNotified = 0;

  int spins = 0;

  if (dwThreadId != GetCurrentThreadId ())
  {
    while (SK_GetFramesDrawn () == ulLastFrameNotified)
    {
      SK_SleepEx (1ul, FALSE);

      if (++spins > 1)
        break;
    }
  }

  WNDPROC wndProc =
    (WNDPROC)SK_GetWindowLongPtrW (hWnd, GWLP_WNDPROC);

  __try {
    CallWindowProcW (wndProc, hWnd, uMsg, wParam, lParam);
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ||
             GetExceptionCode () == EXCEPTION_BREAKPOINT       ?
                                    EXCEPTION_EXECUTE_HANDLER  : EXCEPTION_CONTINUE_SEARCH )
  {
    SendMessageTimeoutW (hWnd, uMsg, wParam, lParam, SMTO_ABORTIFHUNG | SMTO_BLOCK, 100UL, nullptr);
  }

  ulLastFrameNotified = SK_GetFramesDrawn ();
}

void
SK_Win32_NotifyHWND_A (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (hWnd == nullptr || ! IsWindow (hWnd))
    return;

  DWORD  dwProcId   = 0x0,
         dwThreadId =
  SK_GetWindowThreadProcessId (hWnd, &dwProcId);

  if (dwProcId != GetCurrentProcessId ())
    return;

  static UINT64 ulLastFrameNotified = 0;

  if (dwThreadId != GetCurrentThreadId ())
  {
    int spins = 0;

    while (SK_GetFramesDrawn () == ulLastFrameNotified)
    {
      SK_SleepEx (1ul, FALSE);

      if (++spins > 1)
        break;
    }
  }

  WNDPROC wndProc =
    (WNDPROC)SK_GetWindowLongPtrA (hWnd, GWLP_WNDPROC);

  __try {
    CallWindowProcA (wndProc, hWnd, uMsg, wParam, lParam);
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ||
             GetExceptionCode () == EXCEPTION_BREAKPOINT       ?
                                    EXCEPTION_EXECUTE_HANDLER  : EXCEPTION_CONTINUE_SEARCH )
  {
    SendMessageTimeoutA (hWnd, uMsg, wParam, lParam, SMTO_ABORTIFHUNG | SMTO_BLOCK, 100UL, nullptr);
  }

  ulLastFrameNotified = SK_GetFramesDrawn ();
}

void
SK_Win32_NotifyHWND (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (IsWindowUnicode (hWnd))
    return SK_Win32_NotifyHWND_W (hWnd, uMsg, wParam, lParam);

  return
    SK_Win32_NotifyHWND_A (hWnd, uMsg, wParam, lParam);
}

void
SK_Win32_NotifyDeviceChange (bool add_xusb, bool add_hid)
{
  static std::mutex                          _lock;
  std::scoped_lock <std::mutex> scoped_lock (_lock);

  SK_Win32_NotifiedWindows->clear ();

#define IDT_SDL_DEVICE_CHANGE_TIMER_1 1200
#define IDT_SDL_DEVICE_CHANGE_TIMER_2 1201

//Device={EC87F1E3-C13B-4100-B5F7-8B84D54260CB}
//Size=164, Device Type=5 :: Name=\\?\USB#VID_045E&PID_028E#01#{ec87f1e3-c13b-4100-b5f7-8b84d54260cb}
//Device={4D1E55B2-F16F-11CF-88CB-001111000030}
//Size=206, Device Type=5 :: Name=\\?\HID#VID_045E&PID_028E&IG_00#3&1524d57f&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}

  SK_RunOnce (
  {
    for ( auto& xbox_w : dbcc_xbox_w )
    {
      xbox_w->dbcc_size       = sizeof (xbox_w);
      xbox_w->dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
      xbox_w->dbcc_classguid  = GUID_XUSB_INTERFACE_CLASS;
    }

    for ( auto& hid_w : dbcc_hid_w )
    {
      hid_w->dbcc_size        = sizeof (hid_w);
      hid_w->dbcc_devicetype  = DBT_DEVTYP_DEVICEINTERFACE;
      hid_w->dbcc_classguid   = GUID_DEVINTERFACE_HID;
    }

    for ( auto& xbox_a : dbcc_xbox_a )
    {
      xbox_a->dbcc_size       = sizeof (xbox_a);
      xbox_a->dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
      xbox_a->dbcc_classguid  = GUID_XUSB_INTERFACE_CLASS;
    }

    for ( auto& hid_a : dbcc_hid_a )
    {
      hid_a->dbcc_size        = sizeof (hid_a);
      hid_a->dbcc_devicetype  = DBT_DEVTYP_DEVICEINTERFACE;
      hid_a->dbcc_classguid   = GUID_DEVINTERFACE_HID;
    }
  });

  // Static so that it can be used in the EnumWindows lambda;
  //   this entire function is guarded by a lock, so this is safe
  static int idx = 0;

  idx = 0;

  for (auto& controller : SK_HID_PlayStationControllers )
  {
    if (idx > 15)
    {
      SK_ReleaseAssert (idx <= 15);

      return;
    }

    wcsncpy_s (dbcc_xbox_w [idx]->dbcc_name, MAX_PATH, controller.wszDevicePath, _TRUNCATE);
    wcsncpy_s (dbcc_hid_w  [idx]->dbcc_name, MAX_PATH, controller.wszDevicePath, _TRUNCATE);

    dbcc_xbox_w [idx]->dbcc_size = sizeof (DEV_BROADCAST_DEVICEINTERFACE_W) + (DWORD)wcslen (dbcc_xbox_w [idx]->dbcc_name) * 2;
    dbcc_hid_w  [idx]->dbcc_size = sizeof (DEV_BROADCAST_DEVICEINTERFACE_W) + (DWORD)wcslen (dbcc_hid_w  [idx]->dbcc_name) * 2;

    strncpy (dbcc_xbox_a [idx]->dbcc_name, SK_WideCharToUTF8 (controller.wszDevicePath).c_str (), MAX_PATH);
    strncpy (dbcc_hid_a  [idx]->dbcc_name, SK_WideCharToUTF8 (controller.wszDevicePath).c_str (), MAX_PATH);

    dbcc_xbox_a [idx]->dbcc_size = sizeof (DEV_BROADCAST_DEVICEINTERFACE_A) + (DWORD)strlen (dbcc_xbox_a [idx]->dbcc_name);
    dbcc_hid_a  [idx]->dbcc_size = sizeof (DEV_BROADCAST_DEVICEINTERFACE_A) + (DWORD)strlen (dbcc_hid_a  [idx]->dbcc_name);

    for ( auto& notify : SK_Win32_RegisteredDevNotifications.get () )
    {
      if (SK_Win32_NotifiedWindows->count (notify.hWnd) != 0)
        continue;

      if (! (notify.dwFlags & DEVICE_NOTIFY_SERVICE_HANDLE))
      {
        if (notify.bUnicode)
        {
          SK_Win32_NotifyHWND_W (notify.hWnd, WM_DEVICECHANGE, add_xusb ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, (LPARAM)&dbcc_xbox_w [idx]);
          SK_Win32_NotifyHWND_W (notify.hWnd, WM_DEVICECHANGE, add_hid  ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, (LPARAM)&dbcc_hid_w  [idx]);
        
          SK_Win32_NotifiedWindows->insert (notify.hWnd);
        }

        else
        {
          SK_Win32_NotifyHWND_A (notify.hWnd, WM_DEVICECHANGE, add_xusb ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, (LPARAM)&dbcc_xbox_a [idx]);
          SK_Win32_NotifyHWND_A (notify.hWnd, WM_DEVICECHANGE, add_hid  ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, (LPARAM)&dbcc_hid_a  [idx]);
        
          SK_Win32_NotifiedWindows->insert (notify.hWnd);
        }
      }
    }

    static bool _add_xusb, _add_hid;

    _add_xusb = add_xusb;
    _add_hid  = add_hid;

    EnumWindows ([](HWND hWnd, LPARAM)->BOOL
    {
      if (hWnd == SK_hWndDeviceListener)
        return TRUE;

      if (SK_Win32_NotifiedWindows->count (hWnd) != 0)
        return TRUE;

      DWORD                               dwPid = 0x0;
      SK_GetWindowThreadProcessId (hWnd, &dwPid);

      if (dwPid != GetProcessId (SK_GetCurrentProcess ()))
        return TRUE;

      if (SK_GetCurrentRenderBackend ().windows.unity)
      {
        if (SK_GetWindowLongPtrW (hWnd, GWL_STYLE) & WS_CHILD)
          return TRUE;
      }

      else if (hWnd != game_window.hWnd)
        return TRUE;

      if (IsWindowUnicode (hWnd))
      {
        SK_Win32_NotifiedWindows->insert (hWnd);

        SK_Win32_NotifyHWND_W (hWnd, WM_DEVICECHANGE, _add_xusb ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, (LPARAM)&dbcc_xbox_w [idx]);
        SK_Win32_NotifyHWND_W (hWnd, WM_DEVICECHANGE, _add_hid  ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, (LPARAM)&dbcc_hid_w  [idx]);
      }
      else
      {
        SK_Win32_NotifiedWindows->insert (hWnd);

        SK_Win32_NotifyHWND_A (hWnd, WM_DEVICECHANGE, _add_xusb ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, (LPARAM)&dbcc_xbox_a [idx]);
        SK_Win32_NotifyHWND_A (hWnd, WM_DEVICECHANGE, _add_hid  ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, (LPARAM)&dbcc_hid_a  [idx]);
      }

      return TRUE;
    }, (LPARAM)nullptr);

    SK_Win32_NotifiedWindows->clear ();

    ++idx;
  }
}

HDEVNOTIFY
WINAPI
RegisterDeviceNotificationW_Detour (
  _In_ HANDLE hRecipient,
  _In_ LPVOID NotificationFilter,
  _In_ DWORD  Flags )
{
  SK_LOG_FIRST_CALL

  const auto* pNotifyFilter =
    static_cast <DEV_BROADCAST_DEVICEINTERFACE_W *> (NotificationFilter);

  if (pNotifyFilter->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE && (! (Flags & DEVICE_NOTIFY_SERVICE_HANDLE)))
  {
#if 0
    OLECHAR wszGUID [128] = { };

    HRESULT hr =
      StringFromGUID2 (pNotifyFilter->dbcc_classguid, wszGUID, 127);

    SK_LOG0 ( ( L"@ Game registered device notification for GUID: '%s'", wszGUID ),
                L"Input Mgr." );
#endif

    // Fix for Watch_Dogs 2 and possibly other games
    if (IsEqualGUID (pNotifyFilter->dbcc_classguid, GUID_Zero))
    {
      Flags |= DEVICE_NOTIFY_ALL_INTERFACE_CLASSES;

      SK_LOG1 ( (L" >> Fixing Zero GUID used in call to RegisterDeviceNotificationW (...)"),
                 __SK_SUBSYSTEM__ );
    }
  }

  ////if ( config.input.gamepad.xinput.placehold [0] || config.input.gamepad.xinput.placehold [1] ||
  ////     config.input.gamepad.xinput.placehold [2] || config.input.gamepad.xinput.placehold [3] )
  ////  return nullptr;

  auto hDevNotify =
    RegisterDeviceNotificationW_Original (hRecipient, NotificationFilter, Flags);

  if (hDevNotify != nullptr)
  {
    SK_Win32_RegisteredDevNotifications->push_back ({ (SERVICE_STATUS_HANDLE)hRecipient, Flags, true });
  }

  return hDevNotify;
}

RegisterDeviceNotification_pfn RegisterDeviceNotificationA_Original = nullptr;

HDEVNOTIFY
WINAPI
RegisterDeviceNotificationA_Detour (
  _In_ HANDLE hRecipient,
  _In_ LPVOID NotificationFilter,
  _In_ DWORD  Flags )
{
  SK_LOG_FIRST_CALL

  const auto* pNotifyFilter =
    static_cast <DEV_BROADCAST_DEVICEINTERFACE_A *> (NotificationFilter);

  if (pNotifyFilter->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE && (! (Flags & DEVICE_NOTIFY_SERVICE_HANDLE)))
  {
#if 0
    OLECHAR wszGUID [128] = { };

    HRESULT hr =
      StringFromGUID2 (pNotifyFilter->dbcc_classguid, wszGUID, 127);

    SK_LOG0 ( ( L"@ Game registered device notification for GUID: '%s'", wszGUID ),
                L"Input Mgr." );
#endif

    // Fix for Watch_Dogs 2 and possibly other games
    if (IsEqualGUID (pNotifyFilter->dbcc_classguid, GUID_Zero))
    {
      Flags |= DEVICE_NOTIFY_ALL_INTERFACE_CLASSES;

      SK_LOG1 ( (L" >> Fixing Zero GUID used in call to RegisterDeviceNotificationA (...)"),
                 __SK_SUBSYSTEM__ );
    }
  }

  ////if ( config.input.gamepad.xinput.placehold [0] || config.input.gamepad.xinput.placehold [1] ||
  ////     config.input.gamepad.xinput.placehold [2] || config.input.gamepad.xinput.placehold [3] )
  ////  return nullptr;

  auto hDevNotify =
    RegisterDeviceNotificationA_Original (hRecipient, NotificationFilter, Flags);

  if (hDevNotify != nullptr)
  {
    SK_Win32_RegisteredDevNotifications->push_back ({ (SERVICE_STATUS_HANDLE)hRecipient, Flags, false });
  }

  return hDevNotify;
}


void
SK_XInput_InitHotPlugHooks (void)
{
// According to the DLL Export Table, ...A and ...W are the same freaking function :)
  SK_CreateDLLHook2 (       L"user32",
                             "RegisterDeviceNotificationW",
                              RegisterDeviceNotificationW_Detour,
     static_cast_p2p <void> (&RegisterDeviceNotificationW_Original) );

  // If they are identical, then do not install two hooks
  if ( SK_GetProcAddress (L"user32", "RegisterDeviceNotificationA") !=
       SK_GetProcAddress (L"user32", "RegisterDeviceNotificationW") )
  {
    SK_CreateDLLHook2 (       L"user32",
                               "RegisterDeviceNotificationA",
                                RegisterDeviceNotificationA_Detour,
       static_cast_p2p <void> (&RegisterDeviceNotificationA_Original) );
  }

  SK_ApplyQueuedHooks ();
}


HDEVNOTIFY
WINAPI
SK_RegisterDeviceNotification (_In_ HANDLE hRecipient)
{
  const bool bUnicode =
    IsWindowUnicode ((HWND)hRecipient);

  if (bUnicode)
  {
    DEV_BROADCAST_DEVICEINTERFACE_W
    NotificationFilter                 = { };
    NotificationFilter.dbcc_size       = sizeof (DEV_BROADCAST_DEVICEINTERFACE_W);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid  = GUID_DEVINTERFACE_HID;

    if (RegisterDeviceNotificationW_Original != nullptr)
    {
      return
        RegisterDeviceNotificationW_Original (hRecipient, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
    }

    else
    {
      return
        RegisterDeviceNotificationW (hRecipient, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
    }
  }

  else
  {
    DEV_BROADCAST_DEVICEINTERFACE_A
    NotificationFilter                 = { };
    NotificationFilter.dbcc_size       = sizeof (DEV_BROADCAST_DEVICEINTERFACE_A);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid  = GUID_DEVINTERFACE_HID;

    if (RegisterDeviceNotificationA_Original != nullptr)
    {
      return
        RegisterDeviceNotificationA_Original (hRecipient, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
    }

    else
    {
      return
        RegisterDeviceNotificationA (hRecipient, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
    }
  }
}