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

#ifndef __SK__XINPUT_H__
#define __SK__XINPUT_H__

#include <Windows.h>

#define XUSER_MAX_COUNT               4
#define XUSER_INDEX_ANY               0x000000FF

#define XINPUT_GAMEPAD_DPAD_UP        0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN      0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT      0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT     0x0008

#define XINPUT_GAMEPAD_START          0x0010
#define XINPUT_GAMEPAD_BACK           0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB     0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB    0x0080

#define XINPUT_GAMEPAD_LEFT_SHOULDER  0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XINPUT_GAMEPAD_GUIDE          0x0400  // XInputEx

#define XINPUT_GAMEPAD_LEFT_TRIGGER   0x10000
#define XINPUT_GAMEPAD_RIGHT_TRIGGER  0x20000

#define XINPUT_GAMEPAD_A              0x1000
#define XINPUT_GAMEPAD_B              0x2000
#define XINPUT_GAMEPAD_X              0x4000
#define XINPUT_GAMEPAD_Y              0x8000

#define XINPUT_GETSTATEEX_ORDINAL MAKEINTRESOURCEA (100)

#define XINPUT_DEVTYPE_GAMEPAD        0x01
#define XINPUT_DEVSUBTYPE_GAMEPAD     0x01

#define BATTERY_DEVTYPE_GAMEPAD       0x00
#define BATTERY_DEVTYPE_HEADSET       0x01

#define BATTERY_TYPE_DISCONNECTED     0x00
#define BATTERY_TYPE_WIRED            0x01
#define BATTERY_TYPE_ALKALINE         0x02
#define BATTERY_TYPE_NIMH             0x03
#define BATTERY_TYPE_UNKNOWN          0xFF

#define BATTERY_LEVEL_EMPTY           0x00
#define BATTERY_LEVEL_LOW             0x01
#define BATTERY_LEVEL_MEDIUM          0x02
#define BATTERY_LEVEL_FULL            0x03

#define XINPUT_CAPS_FFB_SUPPORTED     0x0001

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30


typedef struct _XINPUT_GAMEPAD {
  WORD  wButtons;
  BYTE  bLeftTrigger;
  BYTE  bRightTrigger;
  SHORT sThumbLX;
  SHORT sThumbLY;
  SHORT sThumbRX;
  SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_GAMEPAD_EX {
  WORD  wButtons;
  BYTE  bLeftTrigger;
  BYTE  bRightTrigger;
  SHORT sThumbLX;
  SHORT sThumbLY;
  SHORT sThumbRX;
  SHORT sThumbRY;
  DWORD dwUnknown;
} XINPUT_GAMEPAD_EX, *PXINPUT_GAMEPAD_EX;



typedef struct _XINPUT_STATE {
  DWORD          dwPacketNumber;
  XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef struct _XINPUT_STATE_EX {
  DWORD             dwPacketNumber;
  XINPUT_GAMEPAD_EX Gamepad;
} XINPUT_STATE_EX, *PXINPUT_STATE_EX;


typedef struct _XINPUT_VIBRATION {
  WORD wLeftMotorSpeed;
  WORD wRightMotorSpeed;
} XINPUT_VIBRATION, *PXINPUT_VIBRATION;

typedef struct _XINPUT_CAPABILITIES {
  BYTE             Type;
  BYTE             SubType;
  WORD             Flags;
  XINPUT_GAMEPAD   Gamepad;
  XINPUT_VIBRATION Vibration;
} XINPUT_CAPABILITIES, *PXINPUT_CAPABILITIES;

typedef struct _XINPUT_BATTERY_INFORMATION {
  BYTE BatteryType;
  BYTE BatteryLevel;
} XINPUT_BATTERY_INFORMATION, *PXINPUT_BATTERY_INFORMATION;



using XInputGetState_pfn        = DWORD (WINAPI *)(
  _In_  DWORD        dwUserIndex,
  _Out_ XINPUT_STATE *pState
);

using XInputGetStateEx_pfn      = DWORD (WINAPI *)(
  _In_  DWORD            dwUserIndex,
  _Out_ XINPUT_STATE_EX *pState
);

using XInputGetCapabilities_pfn = DWORD (WINAPI *)(
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities
);

using XInputSetState_pfn        = DWORD (WINAPI *)(
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration
);

using XInputGetBatteryInformation_pfn = DWORD (WINAPI *)(
  _In_  DWORD                       dwUserIndex,
  _In_  BYTE                        devType,
  _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation
);

using XInputEnable_pfn = void (WINAPI *)(
  _In_ BOOL enable
);



bool SK_XInput_PollController  ( INT           iJoyID, 
                                 XINPUT_STATE* pState = nullptr );

bool SK_XInput_PulseController ( INT           iJoyID,
                                 float         fStrengthLeft,
                                 float         fStrengthRight   );

void SK_XInput_ZeroHaptics     ( INT           iJoyID           );


XINPUT_STATE
SK_JOY_TranslateToXInput (JOYINFOEX* pJoy, const JOYCAPSW* pCaps);


#endif /* __SK__XINPUT_H__ */