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
#define _CRT_SECURE_NO_WARNINGS
#define DIRECTINPUT_VERSION 0x0800

#define NOMINMAX

#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>
#include <SpecialK/input/xinput_hotplug.h>
#include <SpecialK/config.h>
#include <SpecialK/log.h>

#include <algorithm>
#include <dbt.h>


#undef  SK_LOG_FIRST_CALL
#define SK_LOG_FIRST_CALL { static bool called = false; if (! called) { SK_LOG0 ( (L"[!] > First Call: %34hs", __FUNCTION__), L"XInput_Hot" ); called = true; } }

struct {
  static const DWORD RecheckInterval = 1000UL;

  bool  holding     = false;
  DWORD last_polled = 0;
} static placeholders [XUSER_MAX_COUNT + 1];

// One extra for overflow, XUSER_MAX_COUNT is never a valid index
static SK_XInput_PacketJournal packets [XUSER_MAX_COUNT + 1];


SK_XInput_PacketJournal
SK_XInput_GetPacketJournal (DWORD dwUserIndex)
{
  if (dwUserIndex >= XUSER_MAX_COUNT)
    return packets [XUSER_MAX_COUNT];

  return packets [dwUserIndex];
}


bool
SK_XInput_Holding (DWORD dwUserIndex)
{
  if (dwUserIndex >= XUSER_MAX_COUNT)
    return false;

  return placeholders [dwUserIndex].holding;
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

  bool was_holding = placeholders [dwUserIndex].holding;

  if ( dwRet != ERROR_SUCCESS &&
       config.input.gamepad.xinput.placehold [dwUserIndex] )
  {
    if (! placeholders [dwUserIndex].holding)
    {
      placeholders [dwUserIndex].last_polled = timeGetTime ();

      placeholders [dwUserIndex].holding =
        true;//(! SK_XInput_PollController (dwUserIndex, pState));
    }

    else
    {
      if ( placeholders [dwUserIndex].last_polled <
           timeGetTime () - placeholders [dwUserIndex].RecheckInterval )
      {
        // Re-check the next time this controller is polled
        placeholders [dwUserIndex].holding = false;
      }
    }

    dwRet = ERROR_SUCCESS;

    if (! was_holding)
    {
      packets [dwUserIndex].packet_count.virt++;
      packets [dwUserIndex].sequence.current++;
    }

    ZeroMemory (&pState->Gamepad, sizeof XINPUT_GAMEPAD);

    if (! was_holding)
    {
      packets [dwUserIndex].sequence.current =
        std::max   ( packets [dwUserIndex].sequence.current,
          std::max ( packets [dwUserIndex].packet_count.real,
                     packets [dwUserIndex].sequence.last ) );
    }

    pState->dwPacketNumber = packets [dwUserIndex].sequence.current;
                             packets [dwUserIndex].sequence.last = 0;
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
    if (! placeholders [dwUserIndex].holding)
    {
      placeholders [dwUserIndex].last_polled = timeGetTime ();

      placeholders [dwUserIndex].holding =
        true;//(! SK_XInput_PollController (dwUserIndex));
    }

    else
    {
      if ( placeholders [dwUserIndex].last_polled <
           timeGetTime () - placeholders [dwUserIndex].RecheckInterval )
      {
        // Re-check the next time this controller is polled
        placeholders [dwUserIndex].holding = false;
      }
    }

    dwRet = ERROR_SUCCESS;

    ZeroMemory (pCapabilities, sizeof XINPUT_CAPABILITIES);

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
    if (! placeholders [dwUserIndex].holding) {
      placeholders [dwUserIndex].last_polled = timeGetTime ();

      placeholders [dwUserIndex].holding =
        true;//(! SK_XInput_PollController (dwUserIndex));
    }

    else
    {
      if ( placeholders [dwUserIndex].last_polled <
           timeGetTime () - placeholders [dwUserIndex].RecheckInterval )
      {
        // Re-check the next time this controller is polled
        placeholders [dwUserIndex].holding = false;
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
    if (! placeholders [dwUserIndex].holding)
    {
      placeholders [dwUserIndex].last_polled = timeGetTime ();

      placeholders [dwUserIndex].holding = true;
        //(! SK_XInput_PollController (dwUserIndex));
    }

    else
    {
      if ( placeholders [dwUserIndex].last_polled <
           timeGetTime () - placeholders [dwUserIndex].RecheckInterval )
      {
        placeholders [dwUserIndex].holding = false;
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
    if ( packets [dwUserIndex].sequence.last !=
         pState->dwPacketNumber )
    {
      packets [dwUserIndex].sequence.last =
        pState->dwPacketNumber;
      packets [dwUserIndex].packet_count.real++;
      packets [dwUserIndex].sequence.current++;

      packets [dwUserIndex].sequence.current =
        std::max   ( packets [dwUserIndex].sequence.current,
          std::max ( packets [dwUserIndex].packet_count.real,
                     packets [dwUserIndex].sequence.last ) );

      pState->dwPacketNumber = packets [dwUserIndex].sequence.current;
    }
  }

  // Failure will be handled by placeholders if need be.
}





static GUID GUID_Zero;

typedef HDEVNOTIFY (WINAPI *RegisterDeviceNotification_pfn)(
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

  DEV_BROADCAST_DEVICEINTERFACE_W* pNotifyFilter = 
    (DEV_BROADCAST_DEVICEINTERFACE_W *)NotificationFilter;

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

  return RegisterDeviceNotificationW_Original (hRecipient, NotificationFilter, Flags);
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

  DEV_BROADCAST_DEVICEINTERFACE_A* pNotifyFilter = 
    (DEV_BROADCAST_DEVICEINTERFACE_A *)NotificationFilter;

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

  return RegisterDeviceNotificationA_Original (hRecipient, NotificationFilter, Flags);
}

#include <SpecialK/hooks.h>

void
SK_XInput_InitHotPlugHooks (void)
{
  CLSIDFromString (L"{00000000-0000-0000-0000-000000000000}", &GUID_Zero);

// According to the DLL Export Table, ...A and ...W are the same freaking function :)
  SK_CreateDLLHook3 (       L"user32.dll",
                             "RegisterDeviceNotificationW",
                              RegisterDeviceNotificationW_Detour,
reinterpret_cast <LPVOID *> (&RegisterDeviceNotificationW_Original) );

  SK_CreateDLLHook3 (       L"user32.dll",
                             "RegisterDeviceNotificationA",
                              RegisterDeviceNotificationA_Detour,
reinterpret_cast <LPVOID *> (&RegisterDeviceNotificationA_Original) );
}