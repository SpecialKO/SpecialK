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

extern void SK_XInput_SetRefreshInterval   (ULONG ulIntervalMS);
extern void SK_XInput_Refresh              (UINT iJoyID);

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

void
SK_XInput_NotifyDeviceArrival (void)
{
  SK_RunOnce (
  {
    SK_XInputHot_NotifyEvent =
              SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

    SK_XInputHot_ReconnectThread =
    SK_Thread_CreateEx ([](LPVOID user)->
      DWORD
      {
        HANDLE hNotify =
          (HANDLE)user;

        HANDLE phWaitObjects [2] = {
          hNotify, __SK_DLL_TeardownEvent
        };

        static constexpr DWORD ArrivalEvent  = ( WAIT_OBJECT_0     );
        static constexpr DWORD ShutdownEvent = ( WAIT_OBJECT_0 + 1 );

        auto SK_HID_DeviceNotifyProc =
      [] (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
      -> LRESULT
        {
          switch (message)
          {
            case WM_DEVICECHANGE:
            {
              switch (wParam)
              {
                case DBT_DEVICEARRIVAL:
                case DBT_DEVICEREMOVECOMPLETE:
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
                      SK_LOG0 ( ( L" Device %s:\t%s",  arrival ? L"Arrival"
                                                               : L"Removal",
                                                       pDev->dbcc_name ),
                                  __SK_SUBSYSTEM__ );

                      bool playstation = false;
                      bool xinput      = IsEqualGUID (pDev->dbcc_classguid, GUID_XUSB_INTERFACE_CLASS);

                      wchar_t    wszFileName [MAX_PATH];
                      wcsncpy_s (wszFileName, MAX_PATH, pDev->dbcc_name, _TRUNCATE);

                      playstation |= wcsstr (wszFileName, L"VID_054C") != nullptr;
                      xinput      |= wcsstr (wszFileName, L"IG_")      != nullptr;

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

                              controller.hDeviceFile =
                                SK_CreateFileW ( wszFileName, FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                                              FILE_SHARE_READ   | FILE_SHARE_WRITE,
                                                                nullptr, OPEN_EXISTING, 0, nullptr );

                              if (controller.hDeviceFile != INVALID_HANDLE_VALUE)
                              {
                                if (SK_HidD_GetPreparsedData (controller.hDeviceFile, &controller.pPreparsedData))
                                {
                                  controller.bConnected = true;
/*
                                  SK_ImGui_CreateNotification (
                                    "HID.GamepadReetached", SK_ImGui_Toast::Info,
                                    SK_FormatString (
                                      "%ws" ICON_FA_GAMEPAD "\tHas Risen From The Dead!",
                                       controller.wszDevicePath ).c_str (),
                                            "HID Compliant Zombie Gamepad is Back, and Hungers for Brains!",
                                                                       16666, SK_ImGui_Toast::UseDuration |
                                                                              SK_ImGui_Toast::ShowCaption |
                                                                              SK_ImGui_Toast::ShowTitle   |
                                                                              SK_ImGui_Toast::ShowNewest
                                  );
*/

                                  //SK_ImGui_Warning (L"PlayStation Controller Reconnected");

                                  has_existing = true;
                                }
                              }
                              break;
                            }
                          }

                          if (! has_existing)
                          {
                            SK_HID_PlayStationDevice controller;

                            wcsncpy_s (controller.wszDevicePath, MAX_PATH,
                                                  wszFileName,   _TRUNCATE);

                            controller.hDeviceFile =
                              SK_CreateFileW ( wszFileName, FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                                                            FILE_SHARE_READ   | FILE_SHARE_WRITE,
                                                              nullptr, OPEN_EXISTING, 0, nullptr );

                            if (controller.hDeviceFile != nullptr)
                            {
                              controller.bConnected = true;
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

                              if (SK_HidD_GetPreparsedData (controller.hDeviceFile, &controller.pPreparsedData))
                              {
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

                              //SK_ImGui_Warning (L"PlayStation Controller Connected");
                            }
                          }
                        }
                      }

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

                        else if (playstation)
                        {
                          for ( auto& controller : SK_HID_PlayStationControllers )
                          {
                            if (! _wcsicmp (controller.wszDevicePath, wszFileName))
                            {
                              controller.bConnected = false;

/*
                              SK_ImGui_CreateNotification (
                                "HID.GamepadReetached", SK_ImGui_Toast::Info,
                                SK_FormatString (
                                  "%ws" ICON_FA_GAMEPAD "\tHas Left the Building!",
                                   controller.wszDevicePath ).c_str (),
                                        "HID Compliant Gamepad is MIssing In Action!",
                                                  16666, SK_ImGui_Toast::UseDuration |
                                                         SK_ImGui_Toast::ShowCaption |
                                                         SK_ImGui_Toast::ShowTitle   |
                                                         SK_ImGui_Toast::ShowNewest
                              );
*/

                              if (                          controller.hDeviceFile != nullptr)
                                CloseHandle (std::exchange (controller.hDeviceFile,   nullptr));

                              if (controller.pPreparsedData != nullptr)
                                  SK_HidD_FreePreparsedData (
                                    std::exchange (controller.pPreparsedData, nullptr)
                                  );

                              //SK_ImGui_Warning (L"PlayStation Controller Disconnected");

                              break;
                            }
                          }
                        }
                      }
                    }
                  }
                } break;

                default: // Don't care
                  break;
              }

              return 0;
            } break;

            default: // Don't care
              break;
          };

          return
            DefWindowProcW (hwnd, message, wParam, lParam);
        };

        WNDCLASSEXW
          wnd_class               = {                          };
          wnd_class.hInstance     = SK_GetModuleHandle (nullptr);
          wnd_class.lpszClassName = L"SK_HID_Listener";
          wnd_class.lpfnWndProc   = SK_HID_DeviceNotifyProc;
          wnd_class.cbSize        = sizeof (WNDCLASSEXW);

        if (RegisterClassEx (&wnd_class))
        {
          DWORD dwWaitStatus = WAIT_OBJECT_0;

          HWND hWndDeviceListener =
            (HWND)CreateWindowEx ( 0, L"SK_HID_Listener",    NULL, 0,
                                   0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );

          // It's technically unnecessary to register this, but not a bad idea
          HDEVNOTIFY hDevNotify =
            SK_RegisterDeviceNotification (hWndDeviceListener);

          do
          {
            auto MessagePump = [&] (void) ->
            void
            {
              MSG                      msg = { };
              while (SK_PeekMessageW (&msg, hWndDeviceListener, 0, 0, PM_REMOVE | PM_NOYIELD) > 0)
              {
                SK_TranslateMessage (&msg);
                SK_DispatchMessageW (&msg);
              }
            };

            dwWaitStatus =
              MsgWaitForMultipleObjects (2, phWaitObjects, FALSE, INFINITE, QS_ALLINPUT);

            // Event is created in signaled state to queue a refresh in case of
            //   late inject
            if (dwWaitStatus == ArrivalEvent)
            {
              static ULONGLONG ullLastFrame = 0;

              // Do at most once per-frame, then pick up residuals next frame
              if ( std::exchange (ullLastFrame, SK_GetFramesDrawn ()) <
                                                SK_GetFramesDrawn () )
              {
                SK_XInput_RefreshControllers (                        );
                ResetEvent                   (SK_XInputHot_NotifyEvent);
              }

              else
              {
                dwWaitStatus =
                  MsgWaitForMultipleObjects (0, nullptr, FALSE, 3UL, QS_ALLINPUT);

                if (std::exchange (dwWaitStatus, WAIT_OBJECT_0 + 2) !=
                                                 WAIT_OBJECT_0)
                                   dwWaitStatus  = ArrivalEvent;
              }
            }

            if (dwWaitStatus == (WAIT_OBJECT_0 + 2))
            {
              MessagePump ();
            }
          } while (dwWaitStatus != ShutdownEvent);

          UnregisterDeviceNotification (hDevNotify);
          DestroyWindow                (hWndDeviceListener);
          UnregisterClassW             (wnd_class.lpszClassName, wnd_class.hInstance);
        }

        else if (config.system.log_level > 0)
          SK_ReleaseAssert (! L"Failed to register Window Class!");

        SK_CloseHandle (SK_XInputHot_NotifyEvent);
                        SK_XInputHot_NotifyEvent = 0;

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] HID Hotplug Dispatch", (LPVOID)SK_XInputHot_NotifyEvent
    );
  });
}


void SK_XInput_DeferredStatusChecks (void)
{
  static SK_AutoHandle hHotplugUnawareXInputRefresh (
    SK_CreateEvent (nullptr, TRUE, FALSE, nullptr)
  );

  SK_RunOnce (
    SK_Thread_CreateEx ([](LPVOID) -> DWORD
    {
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
    }, L"[SK] XInput Polling Thread")
  );

  // Always refresh at the beginning of a frame rather than the end,
  //   a failure event may cause a lengthy delay, missing VBLANK.
  if ((intptr_t)hHotplugUnawareXInputRefresh.m_h > 0)
    SetEvent (  hHotplugUnawareXInputRefresh);
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

void
SK_Win32_NotifyDeviceChange (void)
{
#define IDT_SDL_DEVICE_CHANGE_TIMER_1 1200
#define IDT_SDL_DEVICE_CHANGE_TIMER_2 1201

  static DEV_BROADCAST_DEVICEINTERFACE_W     dbcc_xbox = { };
         dbcc_xbox.dbcc_size       = sizeof (dbcc_xbox);
         dbcc_xbox.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
         dbcc_xbox.dbcc_classguid  = GUID_XUSB_INTERFACE_CLASS;

  static DEV_BROADCAST_DEVICEINTERFACE_W    dbcc_hid = { };
         dbcc_hid.dbcc_size       = sizeof (dbcc_hid);
         dbcc_hid.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
         dbcc_hid.dbcc_classguid  = GUID_DEVINTERFACE_HID;

  for ( auto& notify : SK_Win32_RegisteredDevNotifications.get () )
  {
    if (! (notify.dwFlags & DEVICE_NOTIFY_SERVICE_HANDLE))
    {
      if (notify.bUnicode)
      {
        PostMessageW (
          notify.hWnd, WM_DEVICECHANGE, DBT_DEVICEARRIVAL, (LPARAM)&dbcc_xbox);
        PostMessageW (
          notify.hWnd, WM_DEVICECHANGE, DBT_DEVICEARRIVAL, (LPARAM)&dbcc_hid);
      }

      else
      {
        PostMessageA (
          notify.hWnd, WM_DEVICECHANGE, DBT_DEVICEARRIVAL, (LPARAM)&dbcc_xbox);
        PostMessageA (
          notify.hWnd, WM_DEVICECHANGE, DBT_DEVICEARRIVAL, (LPARAM)&dbcc_hid);
      }
    }
  }

  // After handling all registered notifications, send to the game's top-level window
  PostMessageW (game_window.hWnd, WM_DEVICECHANGE, DBT_DEVICEARRIVAL, (LPARAM)&dbcc_xbox);
  PostMessageW (game_window.hWnd, WM_DEVICECHANGE, DBT_DEVICEARRIVAL, (LPARAM)&dbcc_hid);
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