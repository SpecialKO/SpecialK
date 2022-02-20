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

#ifndef __SK__SCEPAD_H__
#define __SK__SCEPAD_H__

#include <stdint.h>
#include <inttypes.h>

using SK_SceUserID = int;

enum class
SK_ScePadDataOffset
{
  PAD_BUTTON_SHARE       = 0x000000001UL, // "Create"
  PAD_BUTTON_L3          = 0x000000002UL,
  PAD_BUTTON_R3          = 0x000000004UL,
  PAD_BUTTON_OPTIONS     = 0x000000008UL,
  PAD_BUTTON_UP          = 0x000000010UL,
  PAD_BUTTON_RIGHT       = 0x000000020UL,
  PAD_BUTTON_DOWN        = 0x000000040UL,
  PAD_BUTTON_LEFT        = 0x000000080UL,
  PAD_BUTTON_L2          = 0x000000100UL,
  PAD_BUTTON_R2          = 0x000000200UL,
  PAD_BUTTON_L1          = 0x000000400UL,
  PAD_BUTTON_R1          = 0x000000800UL,
  PAD_BUTTON_TRIANGLE    = 0x000001000UL,
  PAD_BUTTON_PLAYSTATION = 0x000010000UL,
  PAD_BUTTON_CIRCLE      = 0x000002000UL,
  PAD_BUTTON_CROSS       = 0x000004000UL,
  PAD_BUTTON_SQUARE      = 0x000008000UL,
  PAD_BUTTON_TOUCH_PAD   = 0x000100000UL,
  PAD_BUTTON_MUTE        = 0x000200000UL,
} SK_ScePadDataOffset;

#define SK_SCEPAD_BUTTON_START PAD_BUTTON_OPTIONS

#define SK_SCEPAD_PORT_TYPE_STANDARD           0
#define SK_SCEPAD_PORT_TYPE_SPECIAL            2
#define SK_SCEPAD_PORT_TYPE_REMOTE_CONTROL    16

#define SK_SCEPAD_MAX_DATA_NUM                64
#define SK_SCEPAD_MAX_TOUCH_NUM                2
#define SK_SCEPAD_MAX_DEVICE_UNIQUE_DATA_SIZE 12

struct SK_ScePadOpenParam {
  uint8_t reserve [8];
};

struct SK_ScePadAnalogStick {
  uint8_t x, y;
};

struct SK_ScePadAnalogButtons {
  uint8_t l2, r2;
  uint8_t padding [2];
};

struct SK_ScePadTouch {
  uint16_t x, y;
  uint8_t    id;
  uint8_t reserve [3];
};

struct SK_ScePadTouchData {
  uint8_t        touchNum;
  uint8_t        reserve [                      3];
  uint32_t       reserve1;
  SK_ScePadTouch touch   [SK_SCEPAD_MAX_TOUCH_NUM];
};

struct SK_ScePadExtensionUnitData {
  uint32_t extensionUnitId;
  uint8_t reserve     [ 1];
  uint8_t dataLength;
  uint8_t data        [10];
};

struct SK_SceFQuaternion
{
	float x, y, z, w;
};

struct SK_SceFVector3
{
	float x, y, z;
};

struct SK_ScePadData
{
  uint32_t                   buttons;
  SK_ScePadAnalogStick       leftStick;
  SK_ScePadAnalogStick       rightStick;
  SK_ScePadAnalogButtons     analogButtons;
  SK_SceFQuaternion          orientation;
  SK_SceFVector3             acceleration;
  SK_SceFVector3             angularVelocity;
  SK_ScePadTouchData         touchData;
  bool                       connected; // sizeof(bool) == 1 ( Intended? )
  uint64_t                   timestamp;
  SK_ScePadExtensionUnitData eud;
  uint8_t                    connectedCount;
  uint8_t                    reserve [2];
  uint8_t                    deviceUniqueDataLen;
  uint8_t                    deviceUniqueData
            [SK_SCEPAD_MAX_DEVICE_UNIQUE_DATA_SIZE];
};

struct SK_ScePadVibrationParam {
  uint8_t bigMotor;   // 0 - stop 1~255:rotate
  uint8_t smallMotor; // 0 - stop 1~255:rotate
};

struct SK_ScePadColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t reserve [1];
};

int SK_ScePadInit (void);

// userID = UserID which opens pad
// type = (PAD_PORT_TYPE_STANDARD|PAD_PORT_TYPE_SPECIAL|PAD_PORT_TYPE_REMOTE_CONTROL)
// index = 0

int SK_ScePadGetHandle (SK_SceUserID userID, int type,
                                             int index);
int SK_ScePadOpen      (SK_SceUserID userID, int type,
                                             int index, SK_ScePadOpenParam *inputParam);
int SK_ScePadClose     (int handle);

// PadData - Controller Data Param
// count = PAD_MAX_DATA_NUM (HISTORY)

int SK_ScePadSetParticularMode               (bool mode);
int SK_ScePadRead                            (int handle, SK_ScePadData *iData, int count);
int SK_ScePadReadState                       (int handle, SK_ScePadData *iData);
int SK_ScePadResetOrientation                (int handle);
int SK_ScePadSetAngularVelocityDeadbandState (int handle, bool enable);
int SK_ScePadSetMotionSensorState            (int handle, bool enable);
int SK_ScePadSetTiltCorrectionState          (int handle, bool enable);
int SK_ScePadSetVibration                    (int handle, SK_ScePadVibrationParam *param);

int SK_ScePadPadResetLightBar                (int handle);
int SK_ScePadSetLightBar                     (int handle, SK_ScePadColor *param);

using scePadInit_pfn                            = int (*)(void);
using scePadGetHandle_pfn                       = int (*)(SK_SceUserID userID, int type, int index);
using scePadOpen_pfn                            = int (*)(SK_SceUserID userID, int type, int index,
                                                          SK_ScePadOpenParam *inputParam);
using scePadClose_pfn                           = int (*)(int handle);
using scePadSetParticularMode_pfn               = int (*)(bool);

using scePadRead_pfn                            = int (*)(int handle, SK_ScePadData *iData, int count);
using scePadReadState_pfn                       = int (*)(int handle, SK_ScePadData *iData);
using scePadResetOrientation_pfn                = int (*)(int handle);
using scePadSetAngularVelocityDeadbandState_pfn = int (*)(int handle, bool enable);
using scePadSetMotionSensorState_pfn            = int (*)(int handle, bool enable);
using scePadSetTiltCorrectionState_pfn          = int (*)(int handle, bool enable);
using scePadSetVibration_pfn                    = int (*)(int handle, SK_ScePadVibrationParam *param);

using scePadResetLightBar_pfn                   = int (*)(int handle);
using scePadSetLightBar_pfn                     = int (*)(int handle, SK_ScePadColor *param);

#endif /* __SK__SCEPAD_H__ */