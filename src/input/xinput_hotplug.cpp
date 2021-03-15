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
#define __SK_SUBSYSTEM__ L"XInput_Hot"

#include <SpecialK/input/xinput_hotplug.h>


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
  void  updatePollTime (DWORD dwTime = timeGetTime ())
  {
    WriteULongRelease (&last_polled, dwTime);
  }
} static placeholders [XUSER_MAX_COUNT + 1] = {
  { 3333UL, FALSE, 0 }, {  5000UL, FALSE, 0 }, {    6666UL, FALSE, 0 },
                        { 10000UL, FALSE, 0 }, { 9999999UL, FALSE, 0 }
};

// One extra for overflow, XUSER_MAX_COUNT is never a valid index
SK_LazyGlobal <std::array <SK_XInput_PacketJournal, XUSER_MAX_COUNT + 1>> xinput_packets;


SK_XInput_PacketJournal
SK_XInput_GetPacketJournal (DWORD dwUserIndex)
{
  static auto& packets =
    xinput_packets.get ();

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  if (dwUserIndex >= XUSER_MAX_COUNT)
     return packets [XUSER_MAX_COUNT];

  return
    packets [dwUserIndex];
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
    else
      placeholders [dwUserIndex].setRefreshTime (timeGetTime ());

    return true;
  }

  placeholders [dwUserIndex].setRefreshTime (133UL);

  return false;
}

void
SK_XInput_NotifyDeviceArrival (void)
{
  return;


  static HANDLE hNotifyEvent =
    SK_CreateEvent (nullptr, FALSE, TRUE, nullptr);

  static HANDLE hReconnectThread =
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

        extern void SK_XInput_SetRefreshInterval (ULONG ulIntervalMS);
        extern void SK_XInput_Refresh (UINT iJoyID);

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
                {
                  SetEvent (hNotifyEvent);

                  SK_LOG_FIRST_CALL //( ( L"USB HID Hotplug Notify is HOT; disabling lazy controller checks." ),
                                          //L"XInput_Hot" ) );
                } break;

                case DBT_DEVICEREMOVECOMPLETE:
                {
                  SK_XInput_SetRefreshInterval (timeGetTime ());
                } break;
              }

              int idx = 0;

              for (auto& placeholder : placeholders)
              {
                if (idx++ != 0) WriteULongRelease (&placeholder.RecheckInterval, (33UL));
                else            WriteULongRelease (&placeholder.RecheckInterval, (1UL));
              }

              return 0;
            } break;
          };

          return
            DefWindowProcW (hwnd, message, wParam, lParam);
        };

        WNDCLASSEXW wnd_class   = { };
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

          HDEVNOTIFY hDevNotify =
            SK_RegisterDeviceNotification (hWndDeviceListener);

          do
          {
            auto MessagePump = [&] (void) ->
            void
            {
              MSG                 msg = { };
              while (PeekMessage (&msg, hWndDeviceListener, 0, 0, PM_REMOVE | PM_NOYIELD) > 0)
              {
                TranslateMessage (&msg);
                DispatchMessage  (&msg);
              }
            };

            if (dwWaitStatus == ArrivalEvent)
            {
              SK_XInput_Refresh (0);

              int idx = 0;

              for ( auto& placeholder : placeholders )
              {
                placeholder.updatePollTime (
                  ( timeGetTime () - placeholder.getRefreshTime () )
                );

                if (ReadAcquire (&placeholder.holding) && idx != 0)
                  SK_XInput_Refresh (idx);

                ++idx;

                if (MsgWaitForMultipleObjects (0, nullptr, FALSE, 66, QS_ALLINPUT) == WAIT_OBJECT_0)
                {
                  MessagePump ();
                }
              }

              SK_XInput_SetRefreshInterval (500UL);
//              SK_XInput_SetRefreshInterval (15000UL);
            }

            dwWaitStatus =
              MsgWaitForMultipleObjects (2, phWaitObjects, FALSE, INFINITE, QS_ALLINPUT);

            if (dwWaitStatus == (WAIT_OBJECT_0 + 2))
            {
              MessagePump ();
            }
          } while (dwWaitStatus != ShutdownEvent);

          UnregisterDeviceNotification (hDevNotify);
          DestroyWindow                (hWndDeviceListener);
          UnregisterClassW             (wnd_class.lpszClassName, wnd_class.hInstance);
        }

        else
          SK_ReleaseAssert (! L"Failed to register Window Class!");

        CloseHandle (hNotify);

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] HID Hotplug Dispatch", (LPVOID)hNotifyEvent
    );

  SetEvent (hNotifyEvent);
}


// Throttles polling while no actual device is connected, because
//   XInput polling failures are performance nightmares.
DWORD
SK_XInput_PlaceHold ( DWORD         dwRet,
                      DWORD         dwUserIndex,
                      XINPUT_STATE *pState )
{
  if (dwUserIndex >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
  if (pState      == nullptr)         return (DWORD)E_POINTER;

  bool was_holding = ReadAcquire (&placeholders [dwUserIndex].holding);

  if ( dwRet != ERROR_SUCCESS &&
       config.input.gamepad.xinput.placehold [dwUserIndex] )
  {
    if (! ReadAcquire (&placeholders [dwUserIndex].holding))
    {
      placeholders [dwUserIndex].updatePollTime ();

      WriteRelease (&placeholders [dwUserIndex].holding, TRUE);
    }

    else
    {
      if ( placeholders [dwUserIndex].lastPolled () <
           timeGetTime () - placeholders [dwUserIndex].getRefreshTime () )
      {
        // Re-check the next time this controller is polled
        WriteRelease (&placeholders [dwUserIndex].holding, FALSE);
      }
    }

    dwRet = ERROR_SUCCESS;

    static auto& packets =
      xinput_packets.get ();
           auto& journal =
      packets [dwUserIndex];

    if (! was_holding)
    {
      journal.packet_count.virt++;
      journal.sequence.current++;
    }

    SecureZeroMemory (&pState->Gamepad, sizeof XINPUT_GAMEPAD);

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
       config.input.gamepad.xinput.placehold [dwUserIndex] )
  {
    if (! ReadAcquire (&placeholders [dwUserIndex].holding))
    {
      placeholders [dwUserIndex].updatePollTime ();

      WriteRelease (&placeholders [dwUserIndex].holding, TRUE);
    }

    else
    {
      if ( placeholders [dwUserIndex].lastPolled () <
           timeGetTime () - placeholders [dwUserIndex].getRefreshTime () )
      {
        // Re-check the next time this controller is polled
        WriteRelease (&placeholders [dwUserIndex].holding, FALSE);
      }
    }

    dwRet = ERROR_SUCCESS;

    SecureZeroMemory (pCapabilities, sizeof XINPUT_CAPABILITIES);

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
       config.input.gamepad.xinput.placehold [dwUserIndex] )
  {
    if (! ReadAcquire (&placeholders [dwUserIndex].holding)) {
      placeholders [dwUserIndex].updatePollTime ();

      WriteRelease (&placeholders [dwUserIndex].holding, TRUE);
    }

    else
    {
      if ( placeholders [dwUserIndex].lastPolled () <
           timeGetTime () - placeholders [dwUserIndex].getRefreshTime () )
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
       config.input.gamepad.xinput.placehold [dwUserIndex] )
  {
    if (! ReadAcquire (&placeholders [dwUserIndex].holding))
    {
      placeholders [dwUserIndex].updatePollTime ();

      WriteRelease (&placeholders [dwUserIndex].holding, TRUE);
    }

    else
    {
      if ( placeholders [dwUserIndex].lastPolled () <
           timeGetTime () - placeholders [dwUserIndex].getRefreshTime () )
      {
        WriteRelease (&placeholders [dwUserIndex].holding, FALSE);
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
  static auto& packets =
    xinput_packets.get ();

  if (dwUserIndex >= XUSER_MAX_COUNT) return;
  if (pState      == nullptr)         return;

  if (dwRet == ERROR_SUCCESS)
  {
    auto& journal =
      packets [dwUserIndex];

    if ( journal.sequence.last != pState->dwPacketNumber )
    {
      journal.sequence.last = pState->dwPacketNumber;
      journal.packet_count.real++;
      journal.sequence.current++;

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

HDEVNOTIFY
WINAPI
RegisterDeviceNotificationW_Detour (
  _In_ HANDLE hRecipient,
  _In_ LPVOID NotificationFilter,
  _In_ DWORD  Flags )
{
  SK_LOG_FIRST_CALL

  auto* pNotifyFilter =
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
                 L"XInput_Hot" );
    }
  }

  if ( config.input.gamepad.xinput.placehold [0] || config.input.gamepad.xinput.placehold [1] ||
       config.input.gamepad.xinput.placehold [2] || config.input.gamepad.xinput.placehold [3] )
    return nullptr;

  return
    RegisterDeviceNotificationW_Original (hRecipient, NotificationFilter, Flags);
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

  auto* pNotifyFilter =
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
                 L"XInput_Hot" );
    }
  }

  if ( config.input.gamepad.xinput.placehold [0] || config.input.gamepad.xinput.placehold [1] ||
       config.input.gamepad.xinput.placehold [2] || config.input.gamepad.xinput.placehold [3] )
    return nullptr;

  return
    RegisterDeviceNotificationA_Original (hRecipient, NotificationFilter, Flags);
}


void
SK_XInput_InitHotPlugHooks (void)
{
// According to the DLL Export Table, ...A and ...W are the same freaking function :)
  SK_CreateDLLHook3 (       L"user32",
                             "RegisterDeviceNotificationW",
                              RegisterDeviceNotificationW_Detour,
     static_cast_p2p <void> (&RegisterDeviceNotificationW_Original) );

  SK_CreateDLLHook3 (       L"user32",
                             "RegisterDeviceNotificationA",
                              RegisterDeviceNotificationA_Detour,
     static_cast_p2p <void> (&RegisterDeviceNotificationA_Original) );
}


HDEVNOTIFY
WINAPI
SK_RegisterDeviceNotification (_In_ HANDLE hRecipient)
{
  GUID GUID_DEVINTERFACE_HID =
    { 0x4D1E55B2L, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

  bool bUnicode =
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