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

#include <cstdint>
#include <cinttypes>

using SK_SceUserID    = int32_t;
using SK_ScePadHandle = int32_t;
using SK_ScePadResult = int;

// Shizam, a scoped enum only half the pain in the ass enum class is (!!)
class SK_ScePadButtonBitmap
{
public:
  using enum_type_t =
           uint32_t;

  SK_ScePadButtonBitmap (void)
  {
    _value = None;
  }

  SK_ScePadButtonBitmap (enum_type_t init) :
                             _value (init) { };

  enum
  {
    None        = 0x000000000UL,
    Share       = 0x000000001UL, // Requires SK_ScePadSetParticularMode
    L3          = 0x000000002UL,
    R3          = 0x000000004UL,
    Options     = 0x000000008UL,
    Up          = 0x000000010UL,
    Right       = 0x000000020UL,
    Down        = 0x000000040UL,
    Left        = 0x000000080UL,
    L2          = 0x000000100UL,
    R2          = 0x000000200UL,
    L1          = 0x000000400UL,
    R1          = 0x000000800UL,
    Triangle    = 0x000001000UL,
    PlayStation = 0x000010000UL, // Requires SK_ScePadSetParticularMode
    Circle      = 0x000002000UL,
    Cross       = 0x000004000UL,
    Square      = 0x000008000UL,
    TouchPad    = 0x000100000UL,
    Mute        = 0x000200000UL, // Requires SK_ScePadSetParticularMode
    All         = static_cast <enum_type_t> (UINT_MAX)
  };

  operator enum_type_t (void) const noexcept
  {
    return _value;
  }

  bool isDown (enum_type_t mask) const noexcept
  {
    if (mask != SK_ScePadButtonBitmap::None)
      return ( mask == (_value &  mask) );
    else
      return            _value == mask;
  }

  void setState (enum_type_t mask, bool set) noexcept
  {
    if (set) _value |=  mask;
    else     _value &= ~mask;
  }

  void clearButtons (enum_type_t mask) noexcept { setState (mask, false); }
  void setButtons   (enum_type_t mask) noexcept { setState (mask, true);  }

private:
  enum_type_t _value;
};

enum class SK_ScePadDeviceClass
{
  Invalid       = -1,
  Standard      =  0,
  Guitar        =  1,
  Drum          =  2,
  DJ_Turntable  =  3,
  DanceMat      =  4,
  Navigation    =  5,
  SteeringWheel =  6,
  Stick         =  7,
  FlightStick   =  8,
  Gun           =  9
};

static constexpr int SK_SCEPAD_PORT_TYPE_STANDARD          =  0;
static constexpr int SK_SCEPAD_PORT_TYPE_SPECIAL           =  2;
static constexpr int SK_SCEPAD_PORT_TYPE_REMOTE_CONTROL    = 16;

static constexpr int SK_SCEPAD_MAX_DATA_NUM                = 64;
static constexpr int SK_SCEPAD_MAX_TOUCH_NUM               =  2;
static constexpr int SK_SCEPAD_MAX_DEVICE_UNIQUE_DATA_SIZE = 12;

static constexpr int SK_SCE_ERROR_OK               = 0x00000000;
static constexpr int SK_SCE_ERROR_NOT_SUPPORTED    = 0x80000004;
static constexpr int SK_SCE_ERROR_ALREADY          = 0x80000020;
static constexpr int SK_SCE_ERROR_BUSY             = 0x80000021;
static constexpr int SK_SCE_ERROR_OUT_OF_MEMORY    = 0x80000022;
static constexpr int SK_SCE_ERROR_PRIV_REQUIRED    = 0x80000023;
static constexpr int SK_SCE_ERROR_NOT_FOUND        = 0x80000025;
static constexpr int SK_SCE_ERROR_ILLEGAL_CONTEXT  = 0x80000030;
static constexpr int SK_SCE_ERROR_CPUDI            = 0x80000031;
static constexpr int SK_SCE_ERROR_SEMAPHORE        = 0x80000041;
static constexpr int SK_SCE_ERROR_INVALID_ID       = 0x80000100;
static constexpr int SK_SCE_ERROR_INVALID_NAME     = 0x80000101;
static constexpr int SK_SCE_ERROR_INVALID_INDEX    = 0x80000102;
static constexpr int SK_SCE_ERROR_INVALID_POINTER  = 0x80000103;
static constexpr int SK_SCE_ERROR_INVALID_SIZE     = 0x80000104;
static constexpr int SK_SCE_ERROR_INVALID_FLAG     = 0x80000105;
static constexpr int SK_SCE_ERROR_INVALID_COMMAND  = 0x80000106;
static constexpr int SK_SCE_ERROR_INVALID_MODE     = 0x80000107;
static constexpr int SK_SCE_ERROR_INVALID_FORMAT   = 0x80000108;
static constexpr int SK_SCE_ERROR_INVALID_VALUE    = 0x800001FE;
static constexpr int SK_SCE_ERROR_INVALID_ARGUMENT = 0x800001FF;
static constexpr int SK_SCE_ERROR_NOENT            = 0x80000202;
static constexpr int SK_SCE_ERROR_BAD_FILE         = 0x80000209;
static constexpr int SK_SCE_ERROR_ACCESS_ERROR     = 0x8000020D;
static constexpr int SK_SCE_ERROR_EXIST            = 0x80000211;
static constexpr int SK_SCE_ERROR_INVAL            = 0x80000216;
static constexpr int SK_SCE_ERROR_MFILE            = 0x80000218;
static constexpr int SK_SCE_ERROR_NOSPC            = 0x8000021C;
static constexpr int SK_SCE_ERROR_DFUNC            = 0x800002FF;

struct SK_ScePadTouchPadInformation
{
  float pixelDensity;

  struct
  {
  	uint16_t x;
  	uint16_t y;
  } resolution;
};

struct SK_ScePadStickInformation
{
  uint8_t deadZoneLeft;
  uint8_t deadZoneRight;
};

struct SK_ScePadControllerInformation
{
  SK_ScePadTouchPadInformation touchPadInfo;
  SK_ScePadStickInformation    stickInfo;
  uint8_t                      connectionType;
  uint8_t                      connectedCount;
  bool                         connected;
  SK_ScePadDeviceClass         deviceClass;
  uint8_t                      reserve [8];
};

struct SK_ScePadOpenParam
{
  uint8_t reserve [8];
};

struct SK_ScePadAnalogStick
{
  uint8_t x, y;
};

struct SK_ScePadAnalogButtons
{
  uint8_t l2, r2;
  uint8_t padding [2];
};

struct SK_ScePadTouch
{
  uint16_t x, y;
  uint8_t    id;
  uint8_t reserve [3];
};

struct SK_ScePadTouchData
{
  uint8_t        touchNum;
  uint8_t        reserve [                      3];
  uint32_t       reserve1;
  SK_ScePadTouch touch   [SK_SCEPAD_MAX_TOUCH_NUM];
};

struct SK_ScePadExtensionUnitData
{
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
  SK_ScePadButtonBitmap      buttonMask;
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

struct SK_ScePadVibrationParam
{
  uint8_t bigMotor;   // 0 - stop 1~255:rotate
  uint8_t smallMotor; // 0 - stop 1~255:rotate
};

struct SK_ScePadColor
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t reserve [1];
};

SK_ScePadResult SK_ScePadInit      (void);
SK_ScePadHandle SK_ScePadGetHandle (SK_SceUserID userID, int type,
                                                         int index);
SK_ScePadResult SK_ScePadOpen      (SK_SceUserID userID, int type,
                                                         int index, SK_ScePadOpenParam *inputParam);
SK_ScePadResult SK_ScePadClose     (SK_ScePadHandle handle);

SK_ScePadResult SK_ScePadSetParticularMode               (bool mode);
SK_ScePadResult SK_ScePadRead                            (SK_ScePadHandle handle, SK_ScePadData *iData, int count);
SK_ScePadResult SK_ScePadReadState                       (SK_ScePadHandle handle, SK_ScePadData *iData);
SK_ScePadResult SK_ScePadResetOrientation                (SK_ScePadHandle handle);
SK_ScePadResult SK_ScePadSetAngularVelocityDeadbandState (SK_ScePadHandle handle, bool enable);
SK_ScePadResult SK_ScePadSetMotionSensorState            (SK_ScePadHandle handle, bool enable);
SK_ScePadResult SK_ScePadSetTiltCorrectionState          (SK_ScePadHandle handle, bool enable);
SK_ScePadResult SK_ScePadSetVibration                    (SK_ScePadHandle handle, SK_ScePadVibrationParam *param);

SK_ScePadResult SK_ScePadPadResetLightBar                (SK_ScePadHandle handle);
SK_ScePadResult SK_ScePadSetLightBar                     (SK_ScePadHandle handle, SK_ScePadColor *param);

using scePadInit_pfn                            = SK_ScePadResult (*)(void);
using scePadGetHandle_pfn                       = SK_ScePadHandle (*)(SK_SceUserID userID, int type, int index);
using scePadOpen_pfn                            = SK_ScePadResult (*)(SK_SceUserID userID, int type, int index,
                                                                      SK_ScePadOpenParam *inputParam);
using scePadClose_pfn                           = SK_ScePadResult (*)(SK_ScePadHandle handle);
using scePadSetParticularMode_pfn               = SK_ScePadResult (*)(bool);

using scePadRead_pfn                            = SK_ScePadResult (*)(SK_ScePadHandle handle, SK_ScePadData *iData, int count);
using scePadReadState_pfn                       = SK_ScePadResult (*)(SK_ScePadHandle handle, SK_ScePadData *iData);
using scePadResetOrientation_pfn                = SK_ScePadResult (*)(SK_ScePadHandle handle);
using scePadSetAngularVelocityDeadbandState_pfn = SK_ScePadResult (*)(SK_ScePadHandle handle, bool enable);
using scePadSetMotionSensorState_pfn            = SK_ScePadResult (*)(SK_ScePadHandle handle, bool enable);
using scePadSetTiltCorrectionState_pfn          = SK_ScePadResult (*)(SK_ScePadHandle handle, bool enable);
using scePadSetVibration_pfn                    = SK_ScePadResult (*)(SK_ScePadHandle handle, SK_ScePadVibrationParam *param);

using scePadResetLightBar_pfn                   = SK_ScePadResult (*)(SK_ScePadHandle handle);
using scePadSetLightBar_pfn                     = SK_ScePadResult (*)(SK_ScePadHandle handle, SK_ScePadColor *param);

#endif /* __SK__SCEPAD_H__ */