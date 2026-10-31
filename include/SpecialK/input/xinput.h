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
#include <joystickapi.h>

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

#define XINPUT_GETSTATE_ORDINAL           MAKEINTRESOURCEA (002)
#define XINPUT_SETSTATE_ORDINAL           MAKEINTRESOURCEA (003)
#define XINPUT_GETCAPABILITIES_ORDINAL    MAKEINTRESOURCEA (004)
#define XINPUT_ENABLE_ORDINAL             MAKEINTRESOURCEA (005)
#define XINPUT_GETSTATEEX_ORDINAL         MAKEINTRESOURCEA (100)
#define XINPUT_POWEROFF_ORDINAL           MAKEINTRESOURCEA (103)
#define XINPUT_GETCAPABILITIES_EX_ORDINAL MAKEINTRESOURCEA (108)


#define XINPUT_DEVTYPE_GAMEPAD             0x01
#define XINPUT_DEVSUBTYPE_UNKNOWN          0x00
#define XINPUT_DEVSUBTYPE_GAMEPAD          0x01
#define XINPUT_DEVSUBTYPE_WHEEL            0x02
#define XINPUT_DEVSUBTYPE_ARCADE_STICK     0x03
#define XINPUT_DEVSUBTYPE_FLIGHT_STICK     0x04
#define XINPUT_DEVSUBTYPE_DANCE_PAD        0x05
#define XINPUT_DEVSUBTYPE_GUITAR           0x06
#define XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE 0x07
#define XINPUT_DEVSUBTYPE_DRUM_KIT         0x08
#define XINPUT_DEVSUBTYPE_GUITAR_BASS      0x0B
#define XINPUT_DEVSUBTYPE_ARCADE_PAD       0x13

#define XINPUT_FLAG_GAMEPAD           0x01

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
#define XINPUT_CAPS_WIRELESS          0x0002
#define XINPUT_CAPS_VOICE_SUPPORTED   0x0004
#define XINPUT_CAPS_PMD_SUPPORTED     0x0008
#define XINPUT_CAPS_NO_NAVIGATION     0x0010

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

typedef struct _XINPUT_KEYSTROKE {
  WORD  VirtualKey;
  WCHAR Unicode;
  WORD  Flags;
  BYTE  UserIndex;
  BYTE  HidCode;
} XINPUT_KEYSTROKE, *PXINPUT_KEYSTROKE;

typedef struct _XINPUT_CAPABILITIES_EX {
  XINPUT_CAPABILITIES Capabilities;
  WORD                VendorId;
  WORD                ProductId;
  WORD                ProductVersion;
  WORD                unk1;
  DWORD               unk2;
} XINPUT_CAPABILITIES_EX, *PXINPUT_CAPABILITIES_EX;


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

using XInputGetCapabilitiesEx_pfn = DWORD (WINAPI *)(
  _In_  DWORD dwReserved,
  _In_  DWORD dwUserIndex,
  _In_  DWORD dwFlags,
  _Out_ XINPUT_CAPABILITIES_EX *pCapabilitiesEx
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

using XInputPowerOff_pfn = DWORD (WINAPI *)(
  _In_ DWORD dwUserIndex
);

#define XINPUT_KEYSTROKE_KEYDOWN 0x0001
#define XINPUT_KEYSTROKE_KEYUP   0x0002
#define XINPUT_KEYSTROKE_REPEAT  0x0004

using XInputGetKeystroke_pfn = DWORD (WINAPI *)(
  DWORD             dwUserIndex,
  DWORD             dwReserved,
  PXINPUT_KEYSTROKE pKeystroke
);




XINPUT_STATE WINAPI SK_JOY_TranslateToXInput  (JOYINFOEX* pJoy, const JOYCAPSW* pCaps);

DWORD WINAPI SK_XInput_GetBatteryInformation  (_In_  DWORD                       dwUserIndex,
                                               _In_  BYTE                        devType,
                                               _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation);

DWORD WINAPI SK_XInput_PowerOff               (_In_  DWORD                       dwUserIndex);
bool         SK_XInput_Enable                 ( BOOL bEnable = TRUE );
bool WINAPI  SK_XInput_WasLastPollSuccessful  ( INT iJoyID );
bool WINAPI  SK_XInput_PollController         ( INT           iJoyID,
                                               XINPUT_STATE* pState = nullptr );
bool WINAPI  SK_XInput_PulseController        ( INT           iJoyID,
                                               float         fStrengthLeft,
                                               float         fStrengthRight   );
void WINAPI  SK_XInput_ZeroHaptics            ( INT           iJoyID          );

void         SK_XInput_SetRefreshInterval     (ULONG ulIntervalMS);
void         SK_XInput_Refresh                (UINT iJoyID);

const char*  SK_XInput_GetPrimaryHookName     (void);
void         SK_XInput_DeferredStatusChecks   (void);
void         SK_XInput_TalesOfAriseButtonSwap (XINPUT_STATE *pState);
FARPROC      SK_XInput_GetProcAddress         (HMODULE hModule, PCSTR lpFuncName, LPCVOID pCaller);

bool
_Success_(false)
SK_ImGui_FilterXInput (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState );

bool
_Success_(false)
SK_ImGui_FilterXInputKeystroke (
  _In_  DWORD             dwUserIndex,
  _Out_ XINPUT_KEYSTROKE *pKeystroke );

void
SK_XInput_ApplyDeadzone (XINPUT_STATE* state);

enum SK_StickCurveType
{
  SK_StickCurve_Linear  = 0,
  SK_StickCurve_Power   = 1,
  SK_StickCurve_Expo    = 2,
  SK_StickCurve_Sigmoid = 3
};

enum SK_Stick
{
  SK_Stick_Left  = 0,
  SK_Stick_Right = 1
};

// Valid ranges for the Input.XInput stick-shaping settings. The INI loader,
//   the control panel sliders and SK_XInput_GetStickShaping all clamp to them.
constexpr int   SK_StickDeadzone_Max   = 16384; // raw units
constexpr float SK_StickPower_Min      =  0.25f;
constexpr float SK_StickPower_Max      =  4.00f;
constexpr float SK_StickSigmoidK_Min   =  1.00f;
constexpr float SK_StickSigmoidK_Max   = 12.00f;
constexpr float SK_StickSigmoidMid_Min =  0.15f;
constexpr float SK_StickSigmoidMid_Max =  0.85f;

// One stick's shaping settings, normalized. Built by SK_XInput_GetStickShaping.
struct SK_StickShaping
{
  float             input_deadzone;       // Controller deadzone c (input floor), 0..1
  float             deadzone_elimination; // Game deadzone D (output floor),      0..1
  SK_StickCurveType curve;
  float             power;                // Power exponent k
  float             expo;                 // Expo amount e       (0..1)
  float             sig_k;                // Sigmoid steepness
  float             sig_mid;              // Sigmoid midpoint
  float             sig_w;                // Sigmoid strength w  (0..1)
};

// Pure: reshape a normalized magnitude u (0..1) -> shaped magnitude (0..1).
//   Endpoints are pinned: 0 at u <= 0, 1 at u >= 1.
float
SK_XInput_ShapeStickMagnitude (float u, const SK_StickShaping& shaping);

// Pure: full per-stick pipeline. u normalized 0..1; returns 0..1.
//   controller deadzone c (input floor) -> remap (c..1) to 0..1 -> curve ->
//   game deadzone D (output floor). output == D exactly at u == c for every
//   curve, so the input where movement first registers is curve-independent.
//   Both the input path and the control panel preview call this.
float
SK_XInput_ShapeStickOutput (float u, const SK_StickShaping& shaping);

// Shaping settings for one stick, read from config and clamped to the valid ranges.
SK_StickShaping
SK_XInput_GetStickShaping (SK_Stick stick);

// Clamp the Input.XInput stick-shaping config values in place.
void
SK_XInput_SanitizeStickShapingConfig (void);

// Unified stick pipeline in the normalized float domain [-1, 1].
//   Shapes both sticks per the Input.XInput curve/deadzone config.
void
SK_XInput_ShapeSticks (float& lx, float& ly, float& rx, float& ry);

// INI spelling of a curve type; unrecognized strings map to Linear.
SK_StickCurveType
SK_XInput_StickCurveTypeFromString (const wchar_t* str);

const wchar_t*
SK_XInput_StickCurveTypeToString (SK_StickCurveType type);

void
SK_XInput_ApplyRemapping (XINPUT_STATE* state);


#endif /* __SK__XINPUT_H__ */