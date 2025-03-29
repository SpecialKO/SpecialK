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
#include <imgui/font_awesome.h>

#include <hidclass.h>
#include <bluetoothapis.h>
#include <bthdef.h>
#include <bthioctl.h>

#pragma comment (lib, "bthprops.lib")

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Input Mgr."

//
// From the Driver SDK, which we would rather not make a dependency
//   required to build Special K...    (hidport.h)
//
#define IOCTL_HID_GET_STRING                     HID_CTL_CODE(4)
#define IOCTL_HID_ACTIVATE_DEVICE                HID_CTL_CODE(7)
#define IOCTL_HID_DEACTIVATE_DEVICE              HID_CTL_CODE(8)
#define IOCTL_HID_GET_DEVICE_ATTRIBUTES          HID_CTL_CODE(9)
#define IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST HID_CTL_CODE(10)

#pragma pack(push,1)
struct SK_HID_DualSense_GetHWAddr // 0x09 : USB
{
  uint8_t ClientMAC [6]; // Right-to-Left
  uint8_t Hard08;
  uint8_t Hard25;
  uint8_t Hard00;
  uint8_t HostMAC   [6]; // Right to Left
  uint8_t Padding   [3]; // Size according to Linux driver
};
#pragma pack(pop)

// Sometimes device change notifications outright fail, and our first and only
//   indication that the device was removed is one of these error codes...
bool SK_HID_IsDisconnectedError (DWORD dwError)
{
  return dwError == ERROR_INVALID_HANDLE ||
         dwError == ERROR_NO_SUCH_DEVICE ||
         dwError == ERROR_DEVICE_NOT_CONNECTED;
}

SK_HID_PlayStationDevice::SK_HID_PlayStationDevice (HANDLE file)
{
  DWORD dwBytesRead = 0;

  hDeviceFile = file;

  SK_DeviceIoControl (
    hDeviceFile, IOCTL_HID_GET_PRODUCT_STRING, 0, 0,
    wszProduct, 128, &dwBytesRead, nullptr
  );

  setPollingFrequency (0);
  setBufferCount      (config.input.gamepad.hid.max_allowed_buffers);

  latency.pollrate   = std::make_shared <SK::Framerate::Stats>     (         );
  xinput.lock_report = std::make_shared <SK_Thread_HybridSpinlock> (/*0x400*/);
}

SK_HID_PlayStationDevice::~SK_HID_PlayStationDevice (void)
{
}

void SK_HID_FlushPlayStationForceFeedback (void)
{
  for (auto& ps_controller : SK_HID_PlayStationControllers)
  {
    if (  ps_controller.bConnected &&
        ( ps_controller.bDualSense ||
          ps_controller.bDualSenseEdge ) )
    {
      WriteRelease (
        &ps_controller.bNeedOutput, TRUE
      ); ps_controller.reset_force_feedback ();
         ps_controller.write_output_report  ();
    }
  }
}

void SK_HID_SetupPlayStationControllers (void)
{
  // Only do this once, and make all other threads trying to do it wait
  //
  static volatile LONG             _init  =  0;
  if (InterlockedCompareExchange (&_init, 1, 0) == 0)
  {
    auto cmd_proc =
      SK_GetCommandProcessor ();

    static auto dualsense_impulse_str_l = std::make_unique <SK_IVarStub <float>> (&config.input.gamepad.dualsense.trigger_strength_l);
    static auto dualsense_resist_str_l  = std::make_unique <SK_IVarStub <float>> (&config.input.gamepad.dualsense.resist_strength_l);
    static auto dualsense_resist_pos_l  = std::make_unique <SK_IVarStub <float>> (&config.input.gamepad.dualsense.resist_start_l);

    static auto dualsense_impulse_str_r = std::make_unique <SK_IVarStub <float>> (&config.input.gamepad.dualsense.trigger_strength_r);
    static auto dualsense_resist_str_r  = std::make_unique <SK_IVarStub <float>> (&config.input.gamepad.dualsense.resist_strength_r);
    static auto dualsense_resist_pos_r  = std::make_unique <SK_IVarStub <float>> (&config.input.gamepad.dualsense.resist_start_r);

    cmd_proc->AddVariable ("Input.Gamepad.DualSense.ImpulseStrL", &dualsense_impulse_str_l->setRange ( 0.0f, 255.0f));
    cmd_proc->AddVariable ("Input.Gamepad.DualSense.ResistStrL",  &dualsense_resist_str_l-> setRange (-1.0f,   1.0f));
    cmd_proc->AddVariable ("Input.Gamepad.DualSense.ResistPosL",  &dualsense_resist_pos_l-> setRange (-1.0f,   1.0f));

    cmd_proc->AddVariable ("Input.Gamepad.DualSense.ImpulseStrR", &dualsense_impulse_str_r->setRange ( 0.0f, 255.0f));
    cmd_proc->AddVariable ("Input.Gamepad.DualSense.ResistStrR",  &dualsense_resist_str_r-> setRange (-1.0f,   1.0f));
    cmd_proc->AddVariable ("Input.Gamepad.DualSense.ResistPosR",  &dualsense_resist_pos_r-> setRange (-1.0f,   1.0f));

    SK_Input_PreHookHID ();

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
    
          const bool bSONY = 
            hidAttribs.VendorID == SK_HID_VID_SONY;

            // (*) 0x2054c - (Too large for 16-bit VID);

            // The 0x2054c VID comes from Bluetooth service discovery.
            //
            //   * It should never actually show up through a call to
            //       HidD_GetAttributes (...); only in UNC paths.

          if (bSONY)
          {
            SK_HID_PlayStationDevice controller (hDeviceFile);

            controller.pid = hidAttribs.ProductID;
            controller.vid = hidAttribs.VendorID;

            controller.bBluetooth =
              StrStrIW (
                wszFileName, //Bluetooth_Base_UUID
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
              (controller.pid == SK_HID_PID_DUALSHOCK4_DONGLE);

            controller.bDualShock3 =
              (controller.pid == SK_HID_PID_DUALSHOCK3);

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
                  L"PlayStation Controller has too many button sets (%u);"
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
                      static_cast <size_t> (controller.button_usage_max) -
                      static_cast <size_t> (controller.button_usage_min) + 1
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
                    controller.value_caps [idx] =
                           valueCapsArray [idx];
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

              controller.bConnected         = true;
              controller.output.last_crc32c = 0;
    
              const auto iter =
                SK_HID_PlayStationControllers.push_back (controller);

              iter->setPollingFrequency (0);
              iter->setBufferCount      (config.input.gamepad.hid.max_allowed_buffers);

              iter->write_output_report ();
            }
          }
        }
      }
    
      SK_SetupDiDestroyDeviceInfoList (hid_device_set);
    }

    InterlockedIncrement (&_init);
  }

  else
  {
    SK_Thread_SpinUntilAtomicMin (&_init, 2);
  }
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

  void setTriggerEffectL (playstation_trigger_effect effect, float param0 = -1.0f, float param1 = -1.0f, float param2 = -1.0f);
  void setTriggerEffectR (playstation_trigger_effect effect, float param0 = -1.0f, float param1 = -1.0f, float param2 = -1.0f);
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
struct SK_HID_DualShock4_GetStateDataUSB : SK_HID_DualShock4_GetStateData {
  TouchData TouchData [3];
  uint8_t   Pad       [3];
};
#pragma pack(pop)

#pragma pack(push,1)
struct SK_HID_DualShock4_GetStateDataBt : SK_HID_DualShock4_GetStateData {
  TouchData TouchData [4];
  uint8_t Pad         [6];
};
#pragma pack(pop)

void
SK_HID_PlayStationDevice::setVibration (
  USHORT low_freq,
  USHORT high_freq,
  USHORT left_trigger,
  USHORT right_trigger,
  USHORT max_val )
{
  if (left_trigger + right_trigger != 0)
  {
    _vibration.trigger.used = true;
  }

  if (max_val == 0)
  {
    const auto last_max = ReadULongAcquire (&_vibration.max_val);

    InterlockedCompareExchange (
      &_vibration.max_val,
        std::max ( { last_max,
                     static_cast <DWORD> (low_freq),
                     static_cast <DWORD> (high_freq),
                     static_cast <DWORD> (left_trigger),
                     static_cast <DWORD> (right_trigger),} ),
                     last_max
    );

    if (last_max > 255)
      max_val = 65535;
    else
      max_val = 255;
  }

  WriteULongRelease (&_vibration.left,
    std::min (255UL,
      static_cast <ULONG> (
        std::clamp (static_cast <double> (low_freq)/
                    static_cast <double> (max_val), 0.0, 1.0) * 256.0)));
  
  WriteULongRelease (&_vibration.right,
    std::min (255UL,
      static_cast <ULONG> (
        std::clamp (static_cast <double> (high_freq)/
                    static_cast <double> (max_val), 0.0, 1.0) * 256.0)));

  WriteULongRelease (&_vibration.trigger.left,
    std::min (255UL,
      static_cast <ULONG> (
        std::clamp (static_cast <double> (left_trigger)/
                    static_cast <double> (max_val), 0.0, 1.0) * 256.0)));
  
  WriteULongRelease (&_vibration.trigger.right,
    std::min (255UL,
      static_cast <ULONG> (
        std::clamp (static_cast <double> (right_trigger)/
                    static_cast <double> (max_val), 0.0, 1.0) * 256.0)));

  WriteULongRelease (&_vibration.last_set, SK::ControlPanel::current_time);
  WriteRelease      (&bNeedOutput, TRUE);
}

void
SK_HID_PlayStationDevice::setVibration (
  USHORT left,
  USHORT right,
  USHORT max_val )
{
  if (max_val == 0)
  {
    const auto last_max = ReadULongAcquire (&_vibration.max_val);

    InterlockedCompareExchange (
      &_vibration.max_val,
        std::max ( { last_max,
                     static_cast <DWORD> (left),
                     static_cast <DWORD> (right) } ),
                     last_max
    );

    if (last_max > 255)
      max_val = 65535;
    else
      max_val = 255;
  }

  WriteULongRelease (&_vibration.left,
    std::min (255UL,
      static_cast <ULONG> (
        std::clamp (static_cast <double> (left)/
                    static_cast <double> (max_val), 0.0, 1.0) * 256.0)));
  
  WriteULongRelease (&_vibration.right,
    std::min (255UL,
      static_cast <ULONG> (
        std::clamp (static_cast <double> (right)/
                    static_cast <double> (max_val), 0.0, 1.0) * 256.0)));

  WriteULongRelease (&_vibration.trigger.left,  0);
  WriteULongRelease (&_vibration.trigger.right, 0);

  WriteULongRelease (&_vibration.last_set, SK::ControlPanel::current_time);
  WriteRelease      (&bNeedOutput, TRUE);
}

bool
SK_HID_PlayStationDevice::setPollingFrequency (DWORD dwFreq)
{
  if ((intptr_t)hDeviceFile <= 0)
    return false;

  return
    SK_DeviceIoControl (
      hDeviceFile, IOCTL_HID_SET_POLL_FREQUENCY_MSEC,
        &dwFreq, sizeof (DWORD), nullptr, 0,
          nullptr, nullptr );
}

bool
SK_HID_PlayStationDevice::setBufferCount (DWORD dwBuffers)
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

  if ((intptr_t)hDeviceFile <= 0)
    return false;

  return
    SK_DeviceIoControl (
      hDeviceFile, IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS,
        &dwBuffers, sizeof (dwBuffers), nullptr,
          0, nullptr, nullptr
    );
}

struct cfg_binding_map_s {
  std::wstring* wszValName   = nullptr;
  UINT           uiVirtKey   = 0;
  UINT           uiButtonIdx = 0;
  bool           lastFrame   = false;
  bool           thisFrame   = false;
  UINT64         frameNum    = 0;
} static binding_map [5] = {
    { &config.input.gamepad.scepad.touch_click,  0, 13 },
    // Mute (14, on DualSense) is already reserved...
    { &config.input.gamepad.scepad.left_fn,      0, 15 },
    { &config.input.gamepad.scepad.right_fn,     0, 16 },
    { &config.input.gamepad.scepad.left_paddle,  0, 17 },
    { &config.input.gamepad.scepad.right_paddle, 0, 18 } };

std::wstring*
SK_HID_GetGamepadButtonBinding (UINT idx) noexcept
{
  for ( auto& binding : binding_map )
  {
    if (binding.uiButtonIdx == idx)
    {
      return binding.wszValName;
    }
  }

  return nullptr;
}

void
SK_HID_AssignGamepadButtonBinding (UINT idx, const wchar_t* wszKeyName, UINT vKey)
{
  for ( auto& binding : binding_map )
  {
    if (binding.uiButtonIdx == idx)
    {
      // Release the button if it had a binding already
      if (binding.lastFrame)
      {   binding.thisFrame = false;
        SK_HID_ProcessGamepadButtonBindings ();
      }

      if (vKey == 0 || wszKeyName == nullptr)
           *binding.wszValName = L"<Not Bound>";
      else *binding.wszValName = wszKeyName;

      binding.uiVirtKey   = vKey;
      binding.lastFrame   = false;
      binding.thisFrame   = false;
      binding.frameNum    = 0;

      break;
    }
  }
}

void
SK_HID_ProcessGamepadButtonBindings (void)
{
  const auto frames_drawn =
    SK_GetFramesDrawn ();

  for ( auto& binding : binding_map )
  {
    UINT VirtualKey  = binding.uiVirtKey;
    if ( VirtualKey != 0 ) // A user-configured binding exists
    {
      const bool bPressed  = (binding.thisFrame && (! binding.lastFrame));
      const bool bReleased = (binding.lastFrame && (! binding.thisFrame));

      if (bPressed || bReleased)
      {
        const BYTE bScancode =
          (BYTE)MapVirtualKey (VirtualKey, 0);

        const DWORD dwFlags =
          ( ( bScancode & 0xE0 ) == 0   ?
              static_cast <DWORD> (0x0) :
              static_cast <DWORD> (KEYEVENTF_EXTENDEDKEY) ) |
                     ( bReleased ? KEYEVENTF_KEYUP
                                 : 0x0 );

        WriteULong64Release (
          &config.input.keyboard.temporarily_allow,
            frames_drawn + 40
        );

        SK_keybd_event (sk::narrow_cast <BYTE> (VirtualKey),
                                                  bScancode, dwFlags, 0);

        binding.lastFrame = binding.thisFrame;
      }

      SK_ReleaseAssert ( binding.frameNum == 0 ||
                         binding.frameNum <= frames_drawn );

      binding.frameNum = frames_drawn;
    }
  }
}

bool
SK_HID_PlayStationDevice::request_input_report (void)
{
  if (ReadAcquire (&__SK_DLL_Ending))
  {
    return false;
  }

  if (! bConnected)
    return false;

  // If user is overriding RGB, we need to write an output report for every input report
  if ( config.input.gamepad.scepad.led_color_r    >= 0 ||
       config.input.gamepad.scepad.led_color_g    >= 0 ||
       config.input.gamepad.scepad.led_color_b    >= 0 ||
       config.input.gamepad.scepad.led_brightness >= 0 )
  {
    WriteRelease (&bNeedOutput, TRUE);
  }

  InterlockedIncrement (&input_requests);

  if (hInputEvent == nullptr)
  {  std::scoped_lock _initlock(*xinput.lock_report);
     hInputEvent =
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
      SK_SetThreadIOPriority       (SK_GetCurrentThread(),3);

      auto *pDevice =
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
        if (ReadAcquire (&__SK_DLL_Ending))
          break;

        if (ReadAcquire (&pDevice->bNeedOutput))
          pDevice->write_output_report ();

        async_input_request        = { };
        async_input_request.hEvent = pDevice->hInputEvent;

        ZeroMemory ( pDevice->input_report.data (),
                     pDevice->input_report.size () );

        pDevice->input_report [0]   = pDevice->bBluetooth ? 49
                                                          : pDevice->button_report_id;

        if (pDevice->bDualShock4)
          pDevice->input_report [0] = pDevice->bBluetooth ? 0x11
                                                          : pDevice->button_report_id;

        bool bHasBluetooth = false;

        for ( auto& ps_controller : SK_HID_PlayStationControllers )
        {
          if ( ps_controller.bConnected &&
               ps_controller.bBluetooth )
          {
            bHasBluetooth = true;
          }
        }

        if (! pDevice->bConnected)
        {
          HANDLE ReconnectEvents [] = {
            __SK_DLL_TeardownEvent,
            pDevice->hReconnectEvent
          };

          DWORD dwWait =
            WaitForMultipleObjects (2, ReconnectEvents, FALSE, 333UL);

          if (dwWait != WAIT_OBJECT_0 && dwWait != (WAIT_OBJECT_0 + 1))
          {
            continue;
          }

          // DLL is ending
          if (dwWait == WAIT_OBJECT_0)
            break;
        }

        BOOL bReadAsync =
          SK_ReadFile ( pDevice->hDeviceFile, pDevice->input_report.data (),
                     sk::narrow_cast <DWORD> (pDevice->input_report.size ()), nullptr, &async_input_request );

        if (! bReadAsync)
        {
          DWORD dwLastErr =
               GetLastError ();

          // Invalid Handle and Device Not Connected get a 100 ms cooldown
          if ( dwLastErr != NOERROR                 &&
               dwLastErr != ERROR_IO_PENDING        &&
               dwLastErr != ERROR_INVALID_HANDLE    &&
               dwLastErr != ERROR_NO_SUCH_DEVICE    &&
               dwLastErr != ERROR_NOT_READY         &&
               dwLastErr != ERROR_INVALID_PARAMETER &&
               dwLastErr != ERROR_DEVICE_NOT_CONNECTED )
          {
            _com_error err (
              _com_error::WCodeToHRESULT (
                sk::narrow_cast <WORD> (dwLastErr)
              )
            );

            SK_LOGs0 (
              L"InputBkEnd",
              L"SK_ReadFile ({%ws}, InputReport) failed, Error=%u [%ws]",
                pDevice->wszDevicePath, dwLastErr,
                  err.ErrorMessage ()
            );
          }

          if (SK_HID_IsDisconnectedError (dwLastErr))
          {
            pDevice->bConnected = false;
          }

          if (! ReadAcquire (&__SK_DLL_Ending))
          {
            if ( dwLastErr != NOERROR &&
                 dwLastErr != ERROR_IO_PENDING )
            {
              if ( dwLastErr != ERROR_INVALID_HANDLE &&
                   dwLastErr != ERROR_NO_SUCH_DEVICE )
                SK_CancelIoEx (pDevice->hDeviceFile, &async_input_request);

              SK_SleepEx (333UL, TRUE); // Prevent runaway CPU usage on failure

              continue;
            }
          }

          else
          {
            break;
          }
        }

        dwWaitState =
          WaitForMultipleObjects ( _countof (hOperationalEvents),
                                             hOperationalEvents, FALSE, INFINITE );

        if (dwWaitState == (WAIT_OBJECT_0))
        {
          // Clear rumble, then exit
          pDevice->write_output_report ();
          break;
        }

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
            const DWORD dwLastErr =
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
                  L"SK_GetOverlappedResult ({%ws}, InputReport) failed, Error=%u [%ws]",
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

            ULONG num_usages =
              ULONG (pDevice->button_usages.size ());

            pDevice->xinput.report.Gamepad = { };

#define __SK_HID_CalculateLatency
#ifdef __SK_HID_CalculateLatency
            if ((config.input.gamepad.hid.calc_latency && SK_ImGui_Active ()) &&
                (pDevice->latency.last_syn > pDevice->latency.last_ack || pDevice->latency.last_ack == 0))
            {
              uint32_t ack =
                uint32_t (SK_QueryPerf ().QuadPart - pDevice->latency.timestamp_epoch);

              pDevice->latency.last_ack = ack;
              pDevice->latency.ping     = pDevice->latency.last_ack -
                                            pDevice->latency.last_syn;//pData->HostTimestamp;

              // Start a new ping
              WriteRelease (&pDevice->bNeedOutput, TRUE);
                             pDevice->write_output_report ();
            }
#endif

            if (dwBytesTransferred != 78 && (! (pDevice->bDualShock4 && pDevice->bBluetooth)))
            {
              if ( HIDP_STATUS_SUCCESS ==
                SK_HidP_GetUsages ( HidP_Input, pDevice->buttons [0].UsagePage, 0,
                                                pDevice->button_usages.data (),
                                                                &num_usages,
                                                pDevice->pPreparsedData,
                                          PCHAR(pDevice->input_report.data ()),
                                          ULONG(pDevice->input_report.size ()) )
                 )
              {
                for ( UINT i = 0; i < num_usages; ++i )
                {
                  if (i >= pDevice->button_usages.size ())
                    continue;

                  pDevice->buttons [
                    ULONG (pDevice->button_usages [i]) -
                    ULONG (pDevice->buttons       [0].Usage)
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
                                             (PCHAR) (pDevice->input_report.data ()),
                                 static_cast <ULONG> (pDevice->input_report.size ()) ) )
                {
                  switch (pDevice->value_caps [i].Range.UsageMin)
                  {
                    case 0x30: // X-axis
                      pDevice->xinput.report.Gamepad.sThumbLX =
                        static_cast <SHORT> (32767 * fmax (-1, (-128.0 + value) / 127));
                      break;

                    case 0x31: // Y-axis
                      pDevice->xinput.report.Gamepad.sThumbLY =
                        static_cast <SHORT> (32767 * fmax (-1, (+127.0 - value) / 127));
                      break;

                    case 0x32: // Z-axis
                      pDevice->xinput.report.Gamepad.sThumbRX =
                        static_cast <SHORT> (32767 * fmax (-1, (-128.0 + value) / 127));
                      break;

                    case 0x33: // Rotate-X
                      pDevice->xinput.report.Gamepad.bLeftTrigger  = sk::narrow_cast <BYTE> (value);
                      break;

                    case 0x34: // Rotate-Y
                      pDevice->xinput.report.Gamepad.bRightTrigger = sk::narrow_cast <BYTE> (value);
                      break;

                    case 0x35: // Rotate-Z
                      pDevice->xinput.report.Gamepad.sThumbRY =
                        static_cast <SHORT> (32767 * fmax (-1, (+127.0 - value) / 127));
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
                const auto *pDualSense = 
                  (SK_HID_DualSense_GetStateData *)&(pDevice->input_report.data ()[1]);

                const auto *pDualShock4 =
                  (SK_HID_DualShock4_GetStateData *)&(pDevice->input_report.data ()[1]);

                float batteryPercent = 0.0f;

                if (! pDevice->bDualShock4)
                {
                  pDevice->battery.state =
                    (SK_HID_PlayStationDevice::PowerState)((((BYTE *)pDualSense)[52] & 0xF0) >> 4);

                  batteryPercent =
                    std::clamp (
                      ( pDevice->battery.state == Charging    ? static_cast <float> (std::min (11, ((BYTE *)pDualSense)[52] & 0xF)) - 1 :
                        pDevice->battery.state == Discharging ? static_cast <float> (std::min (10, ((BYTE *)pDualSense)[52] & 0xF))     :
                                                                                                          0.0f) * 10.0f +
                      ( pDevice->battery.state == Charging    ? 0.0f :
                        pDevice->battery.state == Discharging ? 5.0f :
                                                                0.0f ), 0.0f, 100.0f );
                }

                else
                {
                  pDevice->battery.state =
                    pDualShock4->PluggedPowerCable ?
                                          Charging :
                                       Discharging;

                  batteryPercent =
                    std::clamp (
                      ( pDevice->battery.state == Charging    ? static_cast <float> (std::min (11, (pDualShock4->PowerPercent & 0xF))) - 1 :
                        pDevice->battery.state == Discharging ? static_cast <float> (std::min (10, (pDualShock4->PowerPercent & 0xF)))     :
                                                                                                          0.0f) * 10.0f +
                      ( pDevice->battery.state == Charging    ? 0.0f :
                        pDevice->battery.state == Discharging ? 5.0f :
                                                                0.0f ), 0.0f, 100.0f );

                  // Implicitly assign Charging Complete status if battery is full and plugged-in
                  if ( batteryPercent         >= 100.0f &&
                       pDevice->battery.state == Charging )
                       pDevice->battery.state  = Complete;
                }

                if (pDevice->battery.state == Discharging || 
                    pDevice->battery.state == Charging    ||
                    pDevice->battery.state == Complete)
                {
                  pDevice->battery.percentage = batteryPercent;

                  if (pDevice->battery.state == Complete)
                      pDevice->battery.percentage = 100.0f;
                }

                else 
                {
                  if (! (pDevice->bDualSense && pDevice->bBluetooth && pDevice->bSimpleMode))
                  {
                    SK_RunOnce (
                      SK_ImGui_WarningWithTitle (
                        SK_FormatStringW (L"Unexpected Battery Charge State: %ws!", pDevice->battery.state == AbnormalVoltage     ? L"Abnormal Voltage" :
                                                                                    pDevice->battery.state == AbnormalTemperature ? L"Abnormal Temperature" :
                                                                                    pDevice->battery.state == ChargingError       ? L"Charging Error" :
                                               SK_FormatStringW (L"%02x (Unknown)", pDevice->battery.state).c_str ()
                                         ).c_str (),
                        L"SONY Controller Has Abnormal Battery State");
                     SK_LOGi0 (L"PlayStation Battery Charge State: %02x", pDevice->battery.state);
                    );

                    if (AbnormalVoltage     != pDevice->battery.state &&
                        AbnormalTemperature != pDevice->battery.state)
                    {
                      pDevice->battery.percentage = 100.0f;
                    }
                  }
                }

                if (pDevice->buttons.size () < 19)
                    pDevice->buttons.resize (  19);

                if (pDevice->bDualSense || pDevice->bDualSenseEdge)
                {
                  pDevice->buttons [SK_HID_PlayStationButton::TrackPad   ].state =  pDualSense->ButtonPad           != 0;
                  pDevice->buttons [SK_HID_PlayStationButton::Mute       ].state =  pDualSense->ButtonMute          != 0;
                  pDevice->buttons [SK_HID_PlayStationButton::LeftFn     ].state = (pDualSense->ButtonLeftFunction  != 0) && pDevice->bDualSenseEdge;
                  pDevice->buttons [SK_HID_PlayStationButton::RightFn    ].state = (pDualSense->ButtonRightFunction != 0) && pDevice->bDualSenseEdge;
                  pDevice->buttons [SK_HID_PlayStationButton::LeftPaddle ].state = (pDualSense->ButtonLeftPaddle    != 0) && pDevice->bDualSenseEdge;
                  pDevice->buttons [SK_HID_PlayStationButton::RightPaddle].state = (pDualSense->ButtonRightPaddle   != 0) && pDevice->bDualSenseEdge;
                }

                else
                {
                  pDevice->buttons [SK_HID_PlayStationButton::TrackPad   ].state = pDualShock4->ButtonPad != 0;
                  // These buttons do not exist...
                  pDevice->buttons [SK_HID_PlayStationButton::Mute       ].state = false;
                  pDevice->buttons [SK_HID_PlayStationButton::LeftFn     ].state = false;
                  pDevice->buttons [SK_HID_PlayStationButton::RightFn    ].state = false;
                  pDevice->buttons [SK_HID_PlayStationButton::LeftPaddle ].state = false;
                  pDevice->buttons [SK_HID_PlayStationButton::RightPaddle].state = false;
                }
              }
            }

            else if (! pDevice->bDualShock4)
            {
              pDevice->bBluetooth = true;
                    bHasBluetooth = true;
                    
              SK_RunOnce (SK_Bluetooth_InitPowerMgmt ());

              if (! config.input.gamepad.bt_input_only)
                WriteRelease (&pDevice->bNeedOutput, TRUE);

              if (pDevice->buttons.size () < 19)
                  pDevice->buttons.resize (  19);

              auto pInputRaw =
                pDevice->input_report.data ();

              const auto *pData =
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
                  const bool legacy_input_detected =
                    ( SK_WinMM_Backend->reads [2] + 
                      SK_DI7_Backend->reads   [2] +
                      SK_DI8_Backend->reads   [2] > 0 );

                  if (legacy_input_detected)
                  {
                    SK_ImGui_CreateNotification ( "HID.Bluetooth.ModeChange", SK_ImGui_Toast::Warning,
                      "Reconnect them to Activate DualShock 3 Compatibility Mode",
                      "Special K has Disconnected all Bluetooth Controllers!",
                      //"Bluetooth PlayStation Controller(s) Running in DualShock 4 / DualSense Mode!",
                          15000UL, SK_ImGui_Toast::UseDuration | SK_ImGui_Toast::ShowTitle |
                                   SK_ImGui_Toast::ShowCaption | SK_ImGui_Toast::ShowNewest );

                    SK_DeferCommand ("Input.Gamepad.PowerOff 1");
                  }
                }

                else
                {
                  pDevice->bSimpleMode = false;
                }

                pDevice->xinput.report.Gamepad.sThumbLX =
                  static_cast <SHORT> (32767 * fmax (-1, (-128.0 + pData-> LeftStickX) / 127));
                pDevice->xinput.report.Gamepad.sThumbLY =
                  static_cast <SHORT> (32767 * fmax (-1, (+127.0 - pData-> LeftStickY) / 127));
                pDevice->xinput.report.Gamepad.sThumbRX =
                  static_cast <SHORT> (32767 * fmax (-1, (-128.0 + pData->RightStickX) / 127));
                pDevice->xinput.report.Gamepad.sThumbRY =
                  static_cast <SHORT> (32767 * fmax (-1, (+127.0 - pData->RightStickY) / 127));

                pDevice->xinput.report.Gamepad.bLeftTrigger  = pData->TriggerLeft;
                pDevice->xinput.report.Gamepad.bRightTrigger = pData->TriggerRight;

                switch (pData->DPad)
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

                pDevice->buttons [SK_HID_PlayStationButton::Square  ].state = pData->ButtonSquare   != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Cross   ].state = pData->ButtonCross    != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Circle  ].state = pData->ButtonCircle   != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Triangle].state = pData->ButtonTriangle != 0;
                pDevice->buttons [SK_HID_PlayStationButton::L1      ].state = pData->ButtonL1       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::R1      ].state = pData->ButtonR1       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::L2      ].state = pData->ButtonL2       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::R2      ].state = pData->ButtonR2       != 0;

                pDevice->buttons [SK_HID_PlayStationButton::Create  ].state = pData->ButtonCreate   != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Options ].state = pData->ButtonOptions  != 0;

                pDevice->buttons [SK_HID_PlayStationButton::L3      ].state = pData->ButtonL3       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::R3      ].state = pData->ButtonR3       != 0;

                pDevice->buttons [SK_HID_PlayStationButton::Home    ].state = pData->ButtonHome     != 0;
                pDevice->buttons [SK_HID_PlayStationButton::TrackPad].state = pData->ButtonPad      != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Mute    ].state = pData->ButtonMute     != 0;

                // DualSense does not have these buttons...
                if (pDevice->bDualSenseEdge)
                {
                  pDevice->buttons [SK_HID_PlayStationButton::LeftFn     ].state = pData->ButtonLeftFunction  != 0;
                  pDevice->buttons [SK_HID_PlayStationButton::RightFn    ].state = pData->ButtonRightFunction != 0;
                  pDevice->buttons [SK_HID_PlayStationButton::LeftPaddle ].state = pData->ButtonLeftPaddle    != 0;
                  pDevice->buttons [SK_HID_PlayStationButton::RightPaddle].state = pData->ButtonRightPaddle   != 0;
                }

                pDevice->sensor_timestamp = pData->SensorTimestamp;

                if (pDevice->sensor_timestamp < 10200000)
                    pDevice->reset_rgb = false;
              }

              // Simple Reporting Mode
              else
              {
                // Prevent battery warnings, we can't actually get battery status in simple mode.
                pDevice->battery.state      = Complete;
                pDevice->battery.percentage = 100.0f;

                const auto *pSimpleData =
                  (SK_HID_DualSense_GetSimpleStateDataBt *)&pInputRaw [1];

                pDevice->xinput.report.Gamepad.sThumbLX =
                  static_cast <SHORT> (32767 * fmax (-1, (-128.0 + pSimpleData-> LeftStickX) / 127));
                pDevice->xinput.report.Gamepad.sThumbLY =
                  static_cast <SHORT> (32767 * fmax (-1, (+127.0 - pSimpleData-> LeftStickY) / 127));
                pDevice->xinput.report.Gamepad.sThumbRX =
                  static_cast <SHORT> (32767 * fmax (-1, (-128.0 + pSimpleData->RightStickX) / 127));
                pDevice->xinput.report.Gamepad.sThumbRY =
                  static_cast <SHORT> (32767 * fmax (-1, (+127.0 - pSimpleData->RightStickY) / 127));

                pDevice->xinput.report.Gamepad.bLeftTrigger  = pSimpleData->TriggerLeft;
                pDevice->xinput.report.Gamepad.bRightTrigger = pSimpleData->TriggerRight;

                switch (pSimpleData->DPad)
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

                pDevice->buttons [SK_HID_PlayStationButton::Square  ].state = pSimpleData->ButtonSquare   != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Cross   ].state = pSimpleData->ButtonCross    != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Circle  ].state = pSimpleData->ButtonCircle   != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Triangle].state = pSimpleData->ButtonTriangle != 0;
                pDevice->buttons [SK_HID_PlayStationButton::L1      ].state = pSimpleData->ButtonL1       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::R1      ].state = pSimpleData->ButtonR1       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::L2      ].state = pSimpleData->ButtonL2       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::R2      ].state = pSimpleData->ButtonR2       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Share   ].state = pSimpleData->ButtonShare    != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Options ].state = pSimpleData->ButtonOptions  != 0;
                pDevice->buttons [SK_HID_PlayStationButton::L3      ].state = pSimpleData->ButtonL3       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::R3      ].state = pSimpleData->ButtonR3       != 0;
                pDevice->buttons [SK_HID_PlayStationButton::Home    ].state = pSimpleData->ButtonHome     != 0;
                pDevice->buttons [SK_HID_PlayStationButton::TrackPad].state = pSimpleData->ButtonPad      != 0;

                // No mute button in simple mode
                pDevice->buttons [SK_HID_PlayStationButton::Mute    ].state = false;
              }

              pDevice->battery.state =
                (SK_HID_PlayStationDevice::PowerState)((((BYTE *)pData)[52] & 0xF0) >> 4);

              const float batteryPercent =
               std::clamp (
                ( pDevice->battery.state == Charging    ? static_cast <float> (std::min (11, ((BYTE *)pData)[52] & 0xF)) - 1 :
                  pDevice->battery.state == Discharging ? static_cast <float> (std::min (10, ((BYTE *)pData)[52] & 0xF))     :
                                                                                                    0.0f) * 10.0f +
                ( pDevice->battery.state == Charging    ? 0.0f :
                  pDevice->battery.state == Discharging ? 5.0f :
                                                          0.0f ), 0.0f, 100.0f
               );

              if (pDevice->battery.state == Discharging || 
                  pDevice->battery.state == Charging    ||
                  pDevice->battery.state == Complete)
              {
                pDevice->battery.percentage = batteryPercent;

                if (pDevice->battery.state == Complete)
                    pDevice->battery.percentage = 100.0f;
              }

              else
              {
                pDevice->battery.percentage = 100.0f;
              }

              switch (pDevice->battery.state)
              {
                case Charging:
                case Discharging:
                {
                  if (pDevice->battery.percentage >= config.input.gamepad.low_battery_percent || pDevice->battery.state == Charging || (pDevice->bDualSense && pDevice->bSimpleMode))
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
                        if (pBatteryState->percentage >= config.input.gamepad.low_battery_percent)
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

            else// if (pDevice->bDualShock4)
            {
              pDevice->bBluetooth = true;
                    bHasBluetooth = true;

              SK_RunOnce (SK_Bluetooth_InitPowerMgmt ());

              if (! config.input.gamepad.bt_input_only)
                WriteRelease (&pDevice->bNeedOutput, TRUE);

              if (pDevice->buttons.size () < 14)
                  pDevice->buttons.resize (  14);

              auto pInputRaw =
                pDevice->input_report.data ();

              const auto *pData =
                (SK_HID_DualShock4_GetStateData *)&pInputRaw [3];

              pDevice->xinput.report.Gamepad.sThumbLX =
                static_cast <SHORT> (32767 * fmax (-1, (-128.0 + pData-> LeftStickX) / 127));
              pDevice->xinput.report.Gamepad.sThumbLY =
                static_cast <SHORT> (32767 * fmax (-1, (+127.0 - pData-> LeftStickY) / 127));
              pDevice->xinput.report.Gamepad.sThumbRX =
                static_cast <SHORT> (32767 * fmax (-1, (-128.0 + pData->RightStickX) / 127));
              pDevice->xinput.report.Gamepad.sThumbRY =
                static_cast <SHORT> (32767 * fmax (-1, (+127.0 - pData->RightStickY) / 127));

              pDevice->xinput.report.Gamepad.bLeftTrigger  = pData->TriggerLeft;
              pDevice->xinput.report.Gamepad.bRightTrigger = pData->TriggerRight;

              switch (pData->DPad)
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

              pDevice->buttons [SK_HID_PlayStationButton::Square  ].state = pData->ButtonSquare   != 0;
              pDevice->buttons [SK_HID_PlayStationButton::Cross   ].state = pData->ButtonCross    != 0;
              pDevice->buttons [SK_HID_PlayStationButton::Circle  ].state = pData->ButtonCircle   != 0;
              pDevice->buttons [SK_HID_PlayStationButton::Triangle].state = pData->ButtonTriangle != 0;
              pDevice->buttons [SK_HID_PlayStationButton::L1      ].state = pData->ButtonL1       != 0;
              pDevice->buttons [SK_HID_PlayStationButton::R1      ].state = pData->ButtonR1       != 0;
              pDevice->buttons [SK_HID_PlayStationButton::L2      ].state = pData->ButtonL2       != 0;
              pDevice->buttons [SK_HID_PlayStationButton::R2      ].state = pData->ButtonR2       != 0;
              pDevice->buttons [SK_HID_PlayStationButton::Share   ].state = pData->ButtonShare    != 0;
              pDevice->buttons [SK_HID_PlayStationButton::Options ].state = pData->ButtonOptions  != 0;
              pDevice->buttons [SK_HID_PlayStationButton::L3      ].state = pData->ButtonL3       != 0;
              pDevice->buttons [SK_HID_PlayStationButton::R3      ].state = pData->ButtonR3       != 0;
              pDevice->buttons [SK_HID_PlayStationButton::Home    ].state = pData->ButtonHome     != 0;
              pDevice->buttons [SK_HID_PlayStationButton::TrackPad].state = pData->ButtonPad      != 0;

              pDevice->sensor_timestamp = pData->Timestamp;

              //if (pDevice->sensor_timestamp < 10200000)
              //    pDevice->reset_rgb = false;

              pDevice->battery.state =
                pData->PluggedPowerCable ?
                                Charging :
                             Discharging;

              const float batteryPercent =
                std::clamp (
                  ( pDevice->battery.state == Charging    ? static_cast <float> (std::min (11, (pData->PowerPercent & 0xF))) - 1 :
                    pDevice->battery.state == Discharging ? static_cast <float> (std::min (10, (pData->PowerPercent & 0xF)))     :
                                                                                                      0.0f) * 10.0f +
                  ( pDevice->battery.state == Charging    ? 0.0f :
                    pDevice->battery.state == Discharging ? 5.0f : //TODO: V547 https://pvs-studio.com/en/docs/warnings/v547/ Expression 'pDevice->battery.state == Discharging' is always true.
                                                            0.0f ), 0.0f, 100.0f );

              // Implicitly assign Charging Complete status if battery is full and plugged-in
              if ( batteryPercent         >= 100.0f &&
                   pDevice->battery.state == Charging )
                   pDevice->battery.state  = Complete;

              if (pDevice->battery.state == Discharging || 
                  pDevice->battery.state == Charging    ||
                  pDevice->battery.state == Complete)
              {
                pDevice->battery.percentage = batteryPercent;

                if (pDevice->battery.state == Complete)
                    pDevice->battery.percentage = 100.0f;
              }

              else 
              {
                SK_RunOnce (
                  SK_ImGui_WarningWithTitle (
                    SK_FormatStringW (L"Unexpected Battery Charge State: %ws!", pDevice->battery.state == AbnormalVoltage     ? L"Abnormal Voltage" :
                                                                                pDevice->battery.state == AbnormalTemperature ? L"Abnormal Temperature" :
                                                                                pDevice->battery.state == ChargingError       ? L"Charging Error" :
                                           SK_FormatStringW (L"%02x (Unknown)", pDevice->battery.state).c_str ()
                                     ).c_str (),
                    L"SONY Controller Has Abnormal Battery State")
                );

                if (AbnormalVoltage     != pDevice->battery.state &&
                    AbnormalTemperature != pDevice->battery.state)
                {
                  pDevice->battery.percentage = 100.0f;
                }
              }

              switch (pDevice->battery.state)
              {
                case Charging:
                case Discharging:
                {
                  if (pDevice->battery.percentage >= config.input.gamepad.low_battery_percent || pDevice->battery.state == Charging || (pDevice->bDualSense && pDevice->bSimpleMode))
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
                        const SK_HID_PlayStationDevice::battery_s *pBatteryState =
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
                        if (pBatteryState->percentage >= config.input.gamepad.low_battery_percent)
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
              if (pDevice->isButtonDown (SK_HID_PlayStationButton::Square  )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_X;
              if (pDevice->isButtonDown (SK_HID_PlayStationButton::Cross   )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_A;
              if (pDevice->isButtonDown (SK_HID_PlayStationButton::Circle  )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_B;
              if (pDevice->isButtonDown (SK_HID_PlayStationButton::Triangle)) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_Y;

              if (pDevice->isButtonDown (SK_HID_PlayStationButton::L1      )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
              if (pDevice->isButtonDown (SK_HID_PlayStationButton::R1      )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
              if (pDevice->isButtonDown (SK_HID_PlayStationButton::L2      )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;
              if (pDevice->isButtonDown (SK_HID_PlayStationButton::R2      )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;

              if (pDevice->isButtonDown (SK_HID_PlayStationButton::Select  )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;
              if (pDevice->isButtonDown (SK_HID_PlayStationButton::Start   )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_START;

              if (config.input.gamepad.scepad.alias_trackpad_share &&
                  pDevice->isButtonDown (SK_HID_PlayStationButton::TrackPad)) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;

              if (pDevice->isButtonDown (SK_HID_PlayStationButton::L3      )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
              if (pDevice->isButtonDown (SK_HID_PlayStationButton::R3      )) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
            }

            // Dual Shock 3
            else if (pDevice->bDualShock3)  // Mappings are non-standard
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

            LARGE_INTEGER
                   tNow = SK_QueryPerf ();
            INT64 tLast = std::exchange (pDevice->latency.last_poll, tNow.QuadPart);

            const double dt =
              static_cast <double> (tNow.QuadPart - tLast) /
              static_cast <double> (SK_QpcTicksPerMs);

            pDevice->latency.pollrate->addSample (dt, tNow);

            if (pDevice->buttons.size () >= 13 &&
                pDevice->isButtonDown (SK_HID_PlayStationButton::PlayStation)) pDevice->xinput.report.Gamepad.wButtons |= XINPUT_GAMEPAD_GUIDE;

            //
            // Apply an aggressive deadzone, moreso than necessary for gameplay, to
            //   filter out stick drift and accidentally bumping controllers when
            //     determining which gamepad was most recently interacted with...
            //
#pragma region (internal_deadzone)
            pDevice->xinput.internal.report = pDevice->xinput.report;

            const float LX   = pDevice->xinput.internal.report.Gamepad.sThumbLX;
            const float LY   = pDevice->xinput.internal.report.Gamepad.sThumbLY;
                  float norm = sqrt ( LX*LX + LY*LY );
                  float unit = 1.0f;

            if (norm > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            {
              norm = std::min (norm, 32767.0f) - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
              unit =           norm/(32767.0f  - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            }
            else
            {
              norm = 0.0f;
              unit = 0.0f;
            }

            const float uLX = (LX / 32767.0f) * unit;
            const float uLY = (LY / 32767.0f) * unit;

            pDevice->xinput.internal.report.Gamepad.sThumbLX =
              static_cast <SHORT> (uLX < 0 ? std::max (-32768.0f, std::min (    0.0f, 32768.0f * uLX))
                                           : std::max (     0.0f, std::min (32767.0f, 32767.0f * uLX)));
            pDevice->xinput.internal.report.Gamepad.sThumbLY =
              static_cast <SHORT> (uLY < 0 ? std::max (-32768.0f, std::min (    0.0f, 32768.0f * uLY))
                                           : std::max (     0.0f, std::min (32767.0f, 32767.0f * uLY)));

            const float RX   = pDevice->xinput.internal.report.Gamepad.sThumbRX;
            const float RY   = pDevice->xinput.internal.report.Gamepad.sThumbRY;
                        norm = sqrt ( RX*RX + RY*RY );
                        unit = 1.0f;

            if (norm > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
            {
              norm = std::min (norm, 32767.0f) - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
              unit =           norm/(32767.0f  - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            }
            else
            {
              norm = 0.0f;
              unit = 0.0f;
            }

            const float uRX = (RX / 32767.0f) * unit;
            const float uRY = (RY / 32767.0f) * unit;

            pDevice->xinput.internal.report.Gamepad.sThumbRX =
              static_cast <SHORT> (uLX < 0 ? std::max (-32768.0f, std::min (    0.0f, 32768.0f * uRX))
                                           : std::max (     0.0f, std::min (32767.0f, 32767.0f * uRX)));
            pDevice->xinput.internal.report.Gamepad.sThumbRY =
              static_cast <SHORT> (uLY < 0 ? std::max (-32768.0f, std::min (    0.0f, 32768.0f * uRY))
                                           : std::max (     0.0f, std::min (32767.0f, 32767.0f * uRY)));

            if (  pDevice->xinput.internal.report.Gamepad.bLeftTrigger   < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
                  pDevice->xinput.internal.report.Gamepad.bLeftTrigger  = 0;
            else
            {
              pDevice->xinput.internal.report.Gamepad.bLeftTrigger =
                static_cast <BYTE> (                             255.0  * std::clamp (
                   (((float)pDevice->xinput.internal.report.Gamepad.bLeftTrigger - (float)XINPUT_GAMEPAD_TRIGGER_THRESHOLD) /
                                                                         (255.0f - (float)XINPUT_GAMEPAD_TRIGGER_THRESHOLD)), 0.0f, 1.0f)
                                   );
            }
            if (  pDevice->xinput.internal.report.Gamepad.bRightTrigger  < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
                  pDevice->xinput.internal.report.Gamepad.bRightTrigger = 0;
            else
            {
              pDevice->xinput.internal.report.Gamepad.bRightTrigger =
                static_cast <BYTE> (                              255.0  * std::clamp (
                   (((float)pDevice->xinput.internal.report.Gamepad.bRightTrigger - (float)XINPUT_GAMEPAD_TRIGGER_THRESHOLD) /
                                                                          (255.0f - (float)XINPUT_GAMEPAD_TRIGGER_THRESHOLD)), 0.0f, 1.0f)
                                   );
            }
#pragma endregion

            SK_XInput_ApplyDeadzone (&pDevice->xinput.report, config.input.gamepad.xinput.deadzone);

#define SK_HID_BROKEN_DUALSHOCK4_REV2
#ifdef  SK_HID_BROKEN_DUALSHOCK4_REV2
            if ( pDevice->pid == SK_HID_PID_DUALSHOCK4_REV2 &&
                 pDevice->bBluetooth                        &&
                 pDevice->bSimpleMode )
            {
              ResetEvent (pDevice->hInputEvent);
              continue;
            }
#endif

            //
            // God-awful hacks for games with impulse triggers that might register
            //   the changing analog input caused by force feedback as actual input.
            //
            if (pDevice->_vibration.trigger.last_left == 0 && pDevice->_vibration.trigger.left != 0)
                pDevice->_vibration.trigger.start_left = pDevice->xinput.report.Gamepad.bLeftTrigger;

            if (pDevice->_vibration.trigger.last_left != 0 || pDevice->_vibration.trigger.left != 0)
            {
              if (pDevice->_vibration.trigger.start_left >= 127)
                  pDevice->xinput.report.Gamepad.bLeftTrigger = pDevice->xinput.report.Gamepad.bLeftTrigger;
              else
              {
                if (pDevice->xinput.report.Gamepad.bLeftTrigger >= 127)
                    pDevice->_vibration.trigger.start_left = pDevice->xinput.report.Gamepad.bLeftTrigger;

                pDevice->xinput.report.Gamepad.bLeftTrigger = (BYTE)pDevice->_vibration.trigger.start_left;
              }
            }

            if (pDevice->_vibration.trigger.last_right == 0 && pDevice->_vibration.trigger.right != 0)
                pDevice->_vibration.trigger.start_right = pDevice->xinput.report.Gamepad.bRightTrigger;

            if (pDevice->_vibration.trigger.last_right != 0 || pDevice->_vibration.trigger.right != 0)
            {
              if (pDevice->_vibration.trigger.start_right >= 127)
                  pDevice->xinput.report.Gamepad.bRightTrigger = pDevice->xinput.report.Gamepad.bRightTrigger;
              else
              {
                if (pDevice->xinput.report.Gamepad.bRightTrigger >= 127)
                    pDevice->_vibration.trigger.start_right = pDevice->xinput.report.Gamepad.bRightTrigger;

                pDevice->xinput.report.Gamepad.bRightTrigger = (BYTE)pDevice->_vibration.trigger.start_right;
              }
            }


                  bool bIsInputActive      = false;
            const bool bIsInputNewInternal =
              memcmp ( &pDevice->xinput.internal.prev_report.Gamepad,
                       &pDevice->xinput.internal.     report.Gamepad, sizeof (XINPUT_GAMEPAD) );
            const bool bIsInputNew =
              memcmp ( &pDevice->xinput.prev_report.Gamepad,
                       &pDevice->xinput.     report.Gamepad,          sizeof (XINPUT_GAMEPAD) );

            const bool bIsAnyButtonDown =
              (pDevice->xinput.report.Gamepad.wButtons != 0);

                                                      // Implicitly treat gamepads with buttons pressed as active
            if (bIsInputNew || bIsInputNewInternal || bIsAnyButtonDown)
            {
              if (bIsInputNewInternal || bIsAnyButtonDown)
              {
                bIsInputActive = true;

                if (bIsInputNewInternal)
                {
                  pDevice->xinput.internal.report.dwPacketNumber++;
                  std::scoped_lock                       _(*pDevice->xinput.lock_report);
                  pDevice->xinput.internal.prev_report = pDevice->xinput.internal.report;
                }
              }

              if (bIsInputNew)
              {
                pDevice->xinput.     report.dwPacketNumber++;
                std::scoped_lock     _(*pDevice->xinput.lock_report);
                pDevice->xinput.prev_report = pDevice->xinput.report;
              }
            }

            const bool bHasNewExtraButtonData =
              pDevice->isButtonChanging (SK_HID_PlayStationButton::TrackPad)    ||
              pDevice->isButtonChanging (SK_HID_PlayStationButton::Mute)        ||
              pDevice->isButtonChanging (SK_HID_PlayStationButton::LeftFn)      ||
              pDevice->isButtonChanging (SK_HID_PlayStationButton::RightFn)     ||
              pDevice->isButtonChanging (SK_HID_PlayStationButton::LeftPaddle)  ||
              pDevice->isButtonChanging (SK_HID_PlayStationButton::RightPaddle);


            // Handle some special buttons that DualSense controllers have that don't map to XInput
            if (bHasNewExtraButtonData)
            {
              bIsInputActive = true;

              SK_RunOnce (
              {
                for ( auto& cfg_binding : binding_map )
                {
                  auto& binding =
                    *cfg_binding.wszValName;

                  if (! (         binding.empty () ||
                        StrStrIW (binding.c_str (), L"<Not Bound>")) )
                  {
                    cfg_binding.uiVirtKey =
                      (UINT)humanKeyNameToVirtKeyCode.get ()[
                        hash_string (binding.c_str ())];
                  }
                }
              });

              for ( auto& binding : binding_map )
              {
                if (! pDevice->bDualSenseEdge)
                {
                  // DualSense Edge and DualShock 4 do not have these buttons...
                  if (binding.uiButtonIdx > 14)
                    continue;
                }

                const UINT VirtualKey  = binding.uiVirtKey;
                if (       VirtualKey != 0 ) // A user-configured binding exists
                {
                  bool bPressed  = false;
                  bool bReleased = false;

                  if (pDevice->buttons [binding.uiButtonIdx].state)
                  {
                    if (! pDevice->buttons [binding.uiButtonIdx].last_state)
                    {
                      bPressed = true;
                    }

                    bIsInputActive    = true;
                    binding.thisFrame = true;
                    binding.frameNum  = SK_GetFramesDrawn ();
                  }

                  else if (! pDevice->buttons [binding.uiButtonIdx].     state)
                  { if (     pDevice->buttons [binding.uiButtonIdx].last_state)
                    {
                      bReleased      = true;
                      bIsInputActive = true;
                    }
                    binding.thisFrame = false;
                    binding.frameNum  = SK_GetFramesDrawn ();
                  }
                }
              }
            }

            if (bIsInputActive)
              WriteULong64Release (&pDevice->xinput.last_active, SK_QueryPerf ().QuadPart);

            bool bIsDeviceMostRecentlyActive = true;

            for ( auto& ps_controller : SK_HID_PlayStationControllers )
            {
              if (ps_controller.bConnected && ps_controller.xinput.isNewer (pDevice->xinput))
              {
                bIsDeviceMostRecentlyActive = false;
                break;
              }
            }

            if (bIsInputActive) // This indicates that the controller is active -after- deadzone checks
            {
              // Give most recently active device a +75 ms advantage,
              //   we may have the same device connected over two
              //     protocols and do not want repeats of the same
              //       input...
              if (bIsDeviceMostRecentlyActive)
              { // Need interlock
                const UINT64 last_active =
                        ReadULong64Acquire (&pDevice->xinput.last_active);
                InterlockedCompareExchange (&pDevice->xinput.last_active, last_active + (SK_PerfTicksPerMs * 75), last_active);
              }
            }

#if 0
            // Deadzone checks make it inconclusive if this is the most recently used
            //   controller, but it does have new input and the game should see it...
            else if (bIsInputNew)
            {
              UINT64 last_active =
                      ReadULong64Acquire (&pDevice->xinput.last_active);
              InterlockedCompareExchange (&pDevice->xinput.last_active, SK_QueryPerf ().QuadPart, last_active);

              if (bIsDeviceMostRecentlyActive)
              { // Need interlock
                last_active =
                        ReadULong64Acquire (&pDevice->xinput.last_active);
                InterlockedCompareExchange (&pDevice->xinput.last_active, last_active + (SK_PerfTicksPerMs * 75), last_active);
              }
            }
#endif

            const bool bAllowSpecialButtons =
              ( config.input.gamepad.disabled_to_game == 0 ||
                SK_IsGameWindowActive () ) &&
                       bIsDeviceMostRecentlyActive;
                  
            if ( config.input.gamepad.scepad.mute_applies_to_game &&
                 bAllowSpecialButtons                             &&
                 pDevice->bDualSense                              &&
                 pDevice->buttons.size () >= 15 )
            {
              if (   pDevice->isButtonDown  (SK_HID_PlayStationButton::Mute) &&
                  (! pDevice->wasButtonDown (SK_HID_PlayStationButton::Mute)) )
              {
                // We need to force an HID output report to
                //   change the state of the mute LED...
                WriteRelease (&pDevice->bNeedOutput, TRUE);
            
                SK_SetGameMute (! SK_IsGameMuted ());
              }
            }

            pDevice->write_output_report ();

            for ( auto& button : pDevice->buttons )
            {
              button.last_state =
                std::exchange (button.state, false);
            }

            ResetEvent (pDevice->hInputEvent);
          }
        }
      }

      TerminateThread (GetCurrentThread (), 0x0);

      return 0;
    }, L"[SK] HID Input Report Consumer", this);
  }

  return true;
}

XINPUT_STATE
SK_HID_PlayStationDevice::hid_to_xi::getLatestState (void)
{
  std::scoped_lock <SK_Thread_HybridSpinlock> _(*lock_report);

  XINPUT_STATE
         copy = prev_report;

  return copy;
}

bool
SK_HID_PlayStationDevice::write_output_report (bool force)
{
  if (ReadAcquire (&__SK_DLL_Ending))
  {
    if (std::exchange (bTerminating, true))
      return false;
  }

  if (! bConnected)
    return false;

  if (! config.input.gamepad.hook_hid)
    return false;

  if (bBluetooth)
  {
    if (config.input.gamepad.bt_input_only)
      return false;

    if (config.input.gamepad.scepad.enable_full_bluetooth)
        force = true;

    if (bSimpleMode && (! force))
    {
      return false;
    }

    if (force)
      bSimpleMode = false;
  }

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

    InterlockedIncrement (&output_requests);

    if (hOutputEnqueued == nullptr)
    {
      std::scoped_lock _initlock(*xinput.lock_report);

      hOutputEnqueued =
        SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

      hOutputFinished =
        SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

      SK_Thread_CreateEx ([](LPVOID pData)->DWORD
      {
        SK_Thread_SetCurrentPriority (THREAD_PRIORITY_TIME_CRITICAL);
        SK_SetThreadIOPriority       (SK_GetCurrentThread (), 3);

        auto *pDevice =
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

        auto _FinishPollRequest =
       [&](bool reset_finished = true)
        {
          pDevice->ulLastFrameOutput =
                   SK_GetFramesDrawn ();

          pDevice->dwLastTimeOutput =
            SK::ControlPanel::current_time;

          if (     true     ) bEnqueued = false;
          if (reset_finished) bFinished = false;
        };

        DWORD dwWaitState = WAIT_FAILED;
        while (true)
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

          if (! pDevice->bConnected)
          {
            if (! ReadAcquire (&__SK_DLL_Ending))
            {
              if ( WAIT_TIMEOUT ==
                     SK_WaitForSingleObject (pDevice->hOutputEnqueued, 125UL) )
              {
                continue;
              }

              bEnqueued = true;
            }

            else
            {
              bQuit = true;
              continue;
            }
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
                TerminateThread (GetCurrentThread (), 0x0);

                return 0;
              }
            }
          }

          WriteRelease (&pDevice->bNeedOutput, FALSE);

          ZeroMemory ( pDevice->output_report.data (),
                       pDevice->output_report.size () );

          // Report Type
          pDevice->output_report [0] =
            pDevice->bBluetooth ? 0x31 :
                                  0x02;

#ifdef __SK_HID_CalculateLatency
          if ((config.input.gamepad.hid.calc_latency && SK_ImGui_Active ()) &&
              (pDevice->latency.last_ack > pDevice->latency.last_syn || pDevice->latency.last_syn == 0))
          {
            pDevice->latency.ping =
              ( pDevice->latency.ping * 19 + ( pDevice->latency.last_ack - 
                                               pDevice->latency.last_syn ) ) / 20;

            pDevice->latency.last_syn =
              sk::narrow_cast <uint32_t> (SK_QueryPerf ().QuadPart - pDevice->latency.timestamp_epoch);

            //output->HostTimestamp = pDevice->latency.last_syn;
          }
#endif

          if (! pDevice->bBluetooth)
          {
            auto pOutputRaw =
              pDevice->output_report.data ();

            auto* output =
              (SK_HID_DualSense_SetStateData *)&pOutputRaw [1];

            const ULONG dwRightMotor   = ReadULongAcquire (&pDevice->_vibration.right);
            const ULONG dwLeftMotor    = ReadULongAcquire (&pDevice->_vibration.left);
            const ULONG dwRightTrigger = ReadULongAcquire (&pDevice->_vibration.trigger.right);
            const ULONG dwLeftTrigger  = ReadULongAcquire (&pDevice->_vibration.trigger.left);

            const ULONG last_trigger_r = pDevice->_vibration.trigger.last_right;
            const ULONG last_trigger_l = pDevice->_vibration.trigger.last_left;

            float& fLastResistStrL = pDevice->_vibration.trigger.last_resist_str_l;
            float& fLastResistStrR = pDevice->_vibration.trigger.last_resist_str_r;
            float& fLastResistPosL = pDevice->_vibration.trigger.last_resist_start_l;
            float& fLastResistPosR = pDevice->_vibration.trigger.last_resist_start_r;

            const bool bResistChange =
              (std::exchange (fLastResistStrL, config.input.gamepad.dualsense.resist_strength_l) != config.input.gamepad.dualsense.resist_strength_l) +
              (std::exchange (fLastResistStrR, config.input.gamepad.dualsense.resist_strength_r) != config.input.gamepad.dualsense.resist_strength_r) +
              (std::exchange (fLastResistPosR, config.input.gamepad.dualsense.resist_start_r)    != config.input.gamepad.dualsense.resist_start_r)    +
              (std::exchange (fLastResistPosL, config.input.gamepad.dualsense.resist_start_l)    != config.input.gamepad.dualsense.resist_start_l);

            const bool bRumble = (dwRightMotor != 0 || dwLeftMotor != 0);

            // 500 msec grace period before allowing controller to use native haptics
            output->UseRumbleNotHaptics = bRumble || last_trigger_r != 0
                                                  || last_trigger_l != 0
                                                  || bResistChange  ||
              (ReadULongAcquire (&pDevice->_vibration.last_set) > SK::ControlPanel::current_time - 500UL);

            if (bRumble || (last_trigger_r != 0 || last_trigger_l != 0))
            {
              WriteULongRelease (&pDevice->_vibration.last_set, SK::ControlPanel::current_time);
              output->AllowMotorPowerLevel   = true;
              output->EnableRumbleEmulation  = true;
            }

            output->AllowMuteLight           = true;

            if (config.input.gamepad.scepad.led_color_r    >= 0 ||
                config.input.gamepad.scepad.led_color_g    >= 0 ||
                config.input.gamepad.scepad.led_color_b    >= 0 ||
                config.input.gamepad.scepad.led_brightness >= 0)
            {
              output->AllowLedColor          = true;
            }

            output->AllowPlayerIndicators    = config.input.gamepad.xinput.debug;

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

            if (dwLeftTrigger != 0 || dwRightTrigger != 0 || last_trigger_r != 0 || last_trigger_l != 0 || bResistChange)
            {
              auto effect =      (dwLeftTrigger != 0) ?
                playstation_trigger_effect::Vibration :
                playstation_trigger_effect::Feedback;

              if (dwLeftTrigger != 0)
              {
                output->setTriggerEffectL (effect, -1.0f, (static_cast <float> (dwLeftTrigger) * config.input.gamepad.impulse_strength_l) / 255.0f, 0.025f);
              }
              
              else
              {
                output->setTriggerEffectL (effect, config.input.gamepad.dualsense.resist_strength_l >= 0.0f ?
                                                      config.input.gamepad.dualsense.resist_start_l >= 0.0f ?
                                                      config.input.gamepad.dualsense.resist_start_l         : 1.0f
                                                                                                            : 1.0f,
                                                   config.input.gamepad.dualsense.resist_strength_l);
              }

              effect =          (dwRightTrigger != 0) ?
                playstation_trigger_effect::Vibration :
                playstation_trigger_effect::Feedback;

              if (dwRightTrigger != 0)
              {
                output->setTriggerEffectR (effect, -1.0f, (static_cast <float> (dwRightTrigger) * config.input.gamepad.impulse_strength_r) / 255.0f, 0.025f);
              }
              
              else
              {
                output->setTriggerEffectR (effect, config.input.gamepad.dualsense.resist_strength_r >= 0.0f ?
                                                      config.input.gamepad.dualsense.resist_start_r >= 0.0f ?
                                                      config.input.gamepad.dualsense.resist_start_r         : 1.0f
                                                                                                            : 1.0f,
                                                   config.input.gamepad.dualsense.resist_strength_r);
              }

              pDevice->_vibration.trigger.last_right = dwRightTrigger;
              pDevice->_vibration.trigger.last_left  = dwLeftTrigger;
            }

            static bool       bMuted     = SK_IsGameMuted ();
            static DWORD dwLastMuteCheck = SK_timeGetTime ();

            if (dwLastMuteCheck < SK::ControlPanel::current_time - 50UL)
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
            output->HapticPowerSave     = false;

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

            InterlockedIncrement (&pDevice->output.writes_attempted);

            uint32_t base_data_crc =
              crc32c (0, output, sizeof (*output));

            if (std::exchange (pDevice->output.last_crc32c, base_data_crc) ==
                                                            base_data_crc)
            {
              bFinished = true;

              _FinishPollRequest (false);
              continue;
            }

            InterlockedIncrement (&pDevice->output.writes_retired);
          }

          else //if (pDevice->bBluetooth)
          {
            BYTE bt_data [79] = { }; //TODO: V1048 https://pvs-studio.com/en/docs/warnings/v1048/ The 'bFinished' variable was assigned the same value.

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

            auto* output =
              (SK_HID_DualSense_SetStateData *)&bt_data [3];

            const ULONG dwRightMotor   = ReadULongAcquire (&pDevice->_vibration.right);
            const ULONG dwLeftMotor    = ReadULongAcquire (&pDevice->_vibration.left);
            const ULONG dwRightTrigger = ReadULongAcquire (&pDevice->_vibration.trigger.right);
            const ULONG dwLeftTrigger  = ReadULongAcquire (&pDevice->_vibration.trigger.left);

            const ULONG last_trigger_r = pDevice->_vibration.trigger.last_right;
            const ULONG last_trigger_l = pDevice->_vibration.trigger.last_left;

            float& fLastResistStrL = pDevice->_vibration.trigger.last_resist_str_l;
            float& fLastResistStrR = pDevice->_vibration.trigger.last_resist_str_r;
            float& fLastResistPosL = pDevice->_vibration.trigger.last_resist_start_l;
            float& fLastResistPosR = pDevice->_vibration.trigger.last_resist_start_r;

            const bool bResistChange =
              (std::exchange (fLastResistStrL, config.input.gamepad.dualsense.resist_strength_l) != config.input.gamepad.dualsense.resist_strength_l) +
              (std::exchange (fLastResistStrR, config.input.gamepad.dualsense.resist_strength_r) != config.input.gamepad.dualsense.resist_strength_r) +
              (std::exchange (fLastResistPosR, config.input.gamepad.dualsense.resist_start_r)    != config.input.gamepad.dualsense.resist_start_r)    +
              (std::exchange (fLastResistPosL, config.input.gamepad.dualsense.resist_start_l)    != config.input.gamepad.dualsense.resist_start_l);

            const bool bRumble = (dwRightMotor != 0 || dwLeftMotor != 0);

            // 500 msec grace period before allowing controller to use native haptics
            output->UseRumbleNotHaptics = bRumble || last_trigger_r != 0
                                                  || last_trigger_l != 0
                                                  || bResistChange  ||
              (ReadULongAcquire (&pDevice->_vibration.last_set) > SK::ControlPanel::current_time - 500UL);

            if (bRumble || (last_trigger_r != 0 || last_trigger_l != 0))
            {
              WriteULongRelease (&pDevice->_vibration.last_set, SK::ControlPanel::current_time);
              output->AllowMotorPowerLevel  = true;
              output->EnableRumbleEmulation = true;
            }

            output->AllowMuteLight          = true;

            if (config.input.gamepad.scepad.led_color_r    >= 0 || 
                config.input.gamepad.scepad.led_color_g    >= 0 ||
                config.input.gamepad.scepad.led_color_b    >= 0 ||
                config.input.gamepad.scepad.led_brightness >= 0)
            {
              output->AllowLedColor         = true;
            }

            output->AllowPlayerIndicators   = config.input.gamepad.xinput.debug;

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

            if (dwLeftTrigger != 0 || dwRightTrigger != 0 || last_trigger_r != 0 || last_trigger_l != 0 || bResistChange)
            {
              auto effect =      (dwLeftTrigger != 0) ?
                playstation_trigger_effect::Vibration :
                playstation_trigger_effect::Feedback;

              if (dwLeftTrigger != 0)
              {
                output->setTriggerEffectL (effect, -1.0f, (static_cast <float> (dwLeftTrigger) * config.input.gamepad.impulse_strength_l) / 255.0f, 0.025f);
              }
              
              else
              {
                output->setTriggerEffectL (effect, config.input.gamepad.dualsense.resist_strength_l >= 0.0f ?
                                                      config.input.gamepad.dualsense.resist_start_l >= 0.0f ?
                                                      config.input.gamepad.dualsense.resist_start_l         : 1.0f
                                                                                                            : 1.0f,
                                                   config.input.gamepad.dualsense.resist_strength_l);
              }

              effect =          (dwRightTrigger != 0) ?
                playstation_trigger_effect::Vibration :
                  playstation_trigger_effect::Feedback;

              if (dwRightTrigger != 0)
              {
                output->setTriggerEffectR (effect, -1.0f, (static_cast <float> (dwRightTrigger) * config.input.gamepad.impulse_strength_r) / 255.0f, 0.025f);
              }
              
              else
              {
                output->setTriggerEffectR (effect, config.input.gamepad.dualsense.resist_strength_r >= 0.0f ?
                                                      config.input.gamepad.dualsense.resist_start_r >= 0.0f ?
                                                      config.input.gamepad.dualsense.resist_start_r         : 1.0f
                                                                                                            : 1.0f,
                                                   config.input.gamepad.dualsense.resist_strength_r);
              }

              pDevice->_vibration.trigger.last_right = dwRightTrigger;
              pDevice->_vibration.trigger.last_left  = dwLeftTrigger;
            }

            static bool       bMuted     = SK_IsGameMuted ();
            static DWORD dwLastMuteCheck = SK_timeGetTime ();

            if (dwLastMuteCheck < SK::ControlPanel::current_time - 50UL)
            {   dwLastMuteCheck = SK::ControlPanel::current_time;
                     bMuted     = SK_IsGameMuted ();
                     // This API is rather expensive
            }

            output->MuteLightMode =
                   bMuted               ?
                 (! game_window.active) ? Breathing
                                        : On
                                        : Off;

            output->TouchPowerSave    = config.input.gamepad.scepad.power_save_mode;
            output->MotionPowerSave   = config.input.gamepad.scepad.power_save_mode;
            output->AudioPowerSave    = config.input.gamepad.scepad.power_save_mode;
            output->HapticPowerSave   = false;

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

            InterlockedIncrement (&pDevice->output.writes_attempted);

            uint32_t base_data_crc =
              crc32c (0, output, sizeof (*output));

            if (std::exchange (pDevice->output.last_crc32c, base_data_crc) ==
                                                            base_data_crc)
            {
              bFinished = true;

              _FinishPollRequest (false);
              continue;
            }

            InterlockedIncrement (&pDevice->output.writes_retired);

            const uint32_t crc =
             crc32 (0x0, bt_data, 75);

            memcpy (                                &bt_data [75], &crc, 4);
            memcpy (pDevice->output_report.data (), &bt_data [ 1],      78);
          }

          async_output_request        = { };
          async_output_request.hEvent = pDevice->hOutputFinished;

          const BOOL bWriteAsync =
            SK_WriteFile ( pDevice->hDeviceFile, pDevice->output_report.data (),
                                          DWORD (pDevice->bBluetooth ? 78 :
                                                 pDevice->output_report.size ()), nullptr, &async_output_request );

          if (! bWriteAsync)
          {
            DWORD dwLastErr =
                 GetLastError ();

            // Invalid Handle and Device Not Connected get a 250 ms cooldown
            if ( dwLastErr != NOERROR                 &&
                 dwLastErr != ERROR_IO_PENDING        &&
                 dwLastErr != ERROR_INVALID_HANDLE    &&
                 dwLastErr != ERROR_NO_SUCH_DEVICE    &&
                 dwLastErr != ERROR_NOT_READY         &&
                 dwLastErr != ERROR_INVALID_PARAMETER &&
                 dwLastErr != ERROR_DEVICE_NOT_CONNECTED )
            {
              _com_error err (dwLastErr);

              SK_LOGs0 (
                L"InputBkEnd",
                L"SK_WriteFile ({%ws}, OutputReport) failed, Error=%u [%ws]",
                  pDevice->wszDevicePath, dwLastErr,
                    err.ErrorMessage ()
              );
            }

            if (SK_HID_IsDisconnectedError (dwLastErr))
            {
              pDevice->bConnected = false;
            }

            if (dwLastErr != ERROR_IO_PENDING && dwLastErr != NOERROR)
            {
              if (dwLastErr != ERROR_INVALID_HANDLE)
                SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

              SK_SleepEx (333UL, TRUE); // Prevent runaway CPU usage on failure

              SetEvent (pDevice->hOutputFinished);
              continue;
            }

            else
            {
              DWORD                                                                         dwBytesRead = 0x0;
              if (! SK_GetOverlappedResultEx (pDevice->hDeviceFile, &async_output_request, &dwBytesRead, 333UL, FALSE))
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
                        L"SK_GetOverlappedResultEx ({%ws}, OutputReport) failed, Error=%u [%ws]",
                          pDevice->wszDevicePath, dwLastErr,
                            err.ErrorMessage ()
                      );
                    }

                    if (dwLastErr != ERROR_INVALID_HANDLE)
                      SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

                    if (dwLastErr == ERROR_GEN_FAILURE       || 
                        dwLastErr == ERROR_OPERATION_ABORTED ||
                        dwLastErr == ERROR_INVALID_PARAMETER)
                    {
                      SK_SleepEx (333UL, TRUE);
                    }
                  }

                  SetEvent (pDevice->hOutputFinished);
                  continue;
                }
              }
            }
          }
          _FinishPollRequest ();
        }

        TerminateThread (GetCurrentThread (), 0x0);

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
    }

    InterlockedIncrement (&output_requests);

    if (hOutputEnqueued == nullptr)
    {
      std::scoped_lock _initlock(*xinput.lock_report);

      hOutputEnqueued =
        SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

      hOutputFinished =
        SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

      SK_Thread_CreateEx ([](LPVOID pData)->DWORD
      {
        SK_Thread_SetCurrentPriority (THREAD_PRIORITY_TIME_CRITICAL);
        SK_SetThreadIOPriority       (SK_GetCurrentThread (), 3);

        SK_HID_PlayStationDevice* pDevice =
          (SK_HID_PlayStationDevice *)pData;

        HANDLE hEvents [] = {
          __SK_DLL_TeardownEvent,
           pDevice->hOutputFinished,
           pDevice->hOutputEnqueued
        };

        OVERLAPPED async_output_request = {};

        bool bEnqueued = false;
        bool bFinished = false;

        // Will be signaled when DLL teardown is processed and any vibration is cleared to 0.
        bool bQuit     = false;

        auto _FinishPollRequest =
       [&](bool reset_finished = true)
        {
#ifdef __SK_HID_CalculateLatency
          if ((config.input.gamepad.hid.calc_latency && SK_ImGui_Active ()) &&
              (pDevice->latency.last_ack > pDevice->latency.last_syn || pDevice->latency.last_syn == 0))
          {
            pDevice->latency.ping =
              ( pDevice->latency.ping * 19 + ( pDevice->latency.last_ack - 
                                               pDevice->latency.last_syn ) ) / 20;

            pDevice->latency.last_syn =
              static_cast <uint32_t> (SK_QueryPerf ().QuadPart - pDevice->latency.timestamp_epoch);

            //output->HostTimestamp = pDevice->latency.last_syn;
          }
#endif

          pDevice->ulLastFrameOutput =
                   SK_GetFramesDrawn ();

          pDevice->dwLastTimeOutput =
            SK::ControlPanel::current_time;

          if (     true     ) bEnqueued = false;
          if (reset_finished) bFinished = false;
        };

        DWORD dwWaitState = WAIT_FAILED;
        while (true)
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

          WriteRelease (&pDevice->bNeedOutput, FALSE);

          ZeroMemory ( pDevice->output_report.data (),
                       pDevice->output_report.size () );

          // Report Type
          pDevice->output_report [0] =
            pDevice->bBluetooth  ? 0x11 : 0x05;

          if (! pDevice->bBluetooth)
          {
            auto pOutputRaw =
              pDevice->output_report.data ();

            auto output =
              (SK_HID_DualShock4_SetStateData *)&pOutputRaw [1];

            output->EnableRumbleUpdate = true;

            if (config.input.gamepad.scepad.led_color_r    >= 0 ||
                config.input.gamepad.scepad.led_color_g    >= 0 ||
                config.input.gamepad.scepad.led_color_b    >= 0 ||
                config.input.gamepad.scepad.led_brightness >= 0)
            {
              output->EnableLedUpdate  = true;
            }

            output->RumbleRight =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.right)
              );
            output->RumbleLeft  =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.left)
              );

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

            InterlockedIncrement (&pDevice->output.writes_attempted);

            uint32_t base_data_crc =
              crc32c (0, output, sizeof (*output));

            if (std::exchange (pDevice->output.last_crc32c, base_data_crc) ==
                                                            base_data_crc)
            {
              bFinished = true;

              _FinishPollRequest (false);
              continue;
            }

            InterlockedIncrement (&pDevice->output.writes_retired);
          }

          else //if (pDevice->bBluetooth)
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

            auto *pOutputRaw =
                   &bt_data [4];

            auto* output =
              (SK_HID_DualShock4_SetStateDataBt *)&pOutputRaw [0];

            output->EnableRumbleUpdate = true;

            if (config.input.gamepad.scepad.led_color_r    >= 0 ||
                config.input.gamepad.scepad.led_color_g    >= 0 ||
                config.input.gamepad.scepad.led_color_b    >= 0 ||
                config.input.gamepad.scepad.led_brightness >= 0)
            {
              output->EnableLedUpdate  = true;
            }

            output->RumbleRight =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.right)
              );
            output->RumbleLeft  =
              sk::narrow_cast <BYTE> (
                ReadULongAcquire (&pDevice->_vibration.left)
              );

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

            InterlockedIncrement (&pDevice->output.writes_attempted);

            uint32_t base_data_crc =
              crc32c (0, output, sizeof (*output));

            if (std::exchange (pDevice->output.last_crc32c, base_data_crc) ==
                                                            base_data_crc)
            {
              bFinished = true;

              _FinishPollRequest (false);
              continue;
            }

            InterlockedIncrement (&pDevice->output.writes_retired);

            const uint32_t crc =
             crc32 (0x0, bt_data, 75);

            memcpy (                                &bt_data [75], &crc, 4);
            memcpy (pDevice->output_report.data (), &bt_data [ 1],      78);
          }

          async_output_request        = { };
          async_output_request.hEvent = pDevice->hOutputFinished;

          const BOOL bWriteAsync =
            SK_WriteFile ( pDevice->hDeviceFile, pDevice->output_report.data (),
                                          DWORD (pDevice->bBluetooth ? 78 :
                                                                       32), nullptr, &async_output_request );

          if (! bWriteAsync)
          {
            DWORD dwLastErr =
                 GetLastError ();

            // Invalid Handle and Device Not Connected get a 250 ms cooldown
            if ( dwLastErr != NOERROR                    &&
                 dwLastErr != ERROR_GEN_FAILURE          && // Happens during power-off
                 dwLastErr != ERROR_IO_PENDING           &&
                 dwLastErr != ERROR_INVALID_HANDLE       &&
                 dwLastErr != ERROR_NO_SUCH_DEVICE       &&
                 dwLastErr != ERROR_NOT_READY            &&
                 dwLastErr != ERROR_OPERATION_ABORTED    && // Happens during power-off
                 dwLastErr != ERROR_DEVICE_NOT_CONNECTED &&
                 dwLastErr != ERROR_INVALID_PARAMETER )
            {
              _com_error err (dwLastErr);

              SK_LOGs0 (
                L"InputBkEnd",
                L"SK_WriteFile ({%ws}, OutputReport) failed, Error=%u [%ws]",
                  pDevice->wszDevicePath, dwLastErr,
                    err.ErrorMessage ()
              );
            }

            if (SK_HID_IsDisconnectedError (dwLastErr))
            {
              pDevice->bConnected = false;
            }

            if (dwLastErr != ERROR_IO_PENDING && dwLastErr != NOERROR)
            {
              if (dwLastErr != ERROR_INVALID_HANDLE)
                SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

              SK_SleepEx (333UL, TRUE); // Prevent runaway CPU usage on failure

              SetEvent (pDevice->hOutputFinished);
              continue;
            }

            else
            {
              DWORD                                                                         dwBytesRead = 0x0;
              if (! SK_GetOverlappedResultEx (pDevice->hDeviceFile, &async_output_request, &dwBytesRead, 333UL, FALSE))
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
                        L"SK_GetOverlappedResultEx ({%ws}, OutputReport) failed, Error=%u [%ws]",
                          pDevice->wszDevicePath, dwLastErr,
                            err.ErrorMessage ()
                      );
                    }

                    if (dwLastErr != ERROR_INVALID_HANDLE)
                      SK_CancelIoEx (pDevice->hDeviceFile, &async_output_request);

                    if (dwLastErr == ERROR_GEN_FAILURE       ||
                        dwLastErr == ERROR_OPERATION_ABORTED ||
                        dwLastErr == ERROR_INVALID_PARAMETER)
                    {
                      SK_SleepEx (333UL, TRUE);
                    }
                  }

                  SetEvent (pDevice->hOutputFinished);
                  continue;
                }
              }
            }
          }
          _FinishPollRequest ();
        }

        TerminateThread (GetCurrentThread (), 0x0);

        return 0;
      }, L"[SK] HID Output Report Producer", this);
    }

    else if (ReadAcquire (&bNeedOutput))
      SetEvent (hOutputEnqueued);

    return true;
  }

  return false;
}


bool SK_HID_DeviceFile::filterHidOutput (uint8_t report_id, DWORD dwSize, LPVOID data)
{
  bool data_changed = false;

  switch (device_vid)
  {
    case SK_HID_VID_SONY:
    {
      switch (device_pid)
      {
        case SK_HID_PID_DUALSHOCK4:
        case SK_HID_PID_DUALSHOCK4_REV2:
        {
          // Not implemented yet
        } break;

        case SK_HID_PID_DUALSENSE_EDGE:
        case SK_HID_PID_DUALSENSE:
        {
          SK_HID_DualSense_SetStateData *pSetState = nullptr;

          if (dwSize >= 47 && report_id == 0x02)
          {
            // libScePad has an extra 17 bytes for some reason...?  (dwSize=64)

            pSetState =
              (SK_HID_DualSense_SetStateData *)(&((uint8_t *)data) [1]);
          }

          // This one needs extra checksum handling...
          else if (dwSize == 79 && report_id == 0x31)
          {
            //SK_ReleaseAssertg ( pDevice->bBluetooth);

            //pSetState =
            //  (SK_HID_DualSense_SetStateData *)(&(uint8_t *)data [3]);
          }

          else
          {
            SK_RunOnce (
              SK_LOGi0 (L"DualSense HID Output, dwSize=%d, report_id=%x", dwSize, report_id)
            );
          }

          if (pSetState != nullptr)
          {
            if (config.input.gamepad.scepad.led_color_r    >= 0 ||
                config.input.gamepad.scepad.led_color_g    >= 0 ||
                config.input.gamepad.scepad.led_color_b    >= 0 ||
                config.input.gamepad.scepad.led_brightness >= 0)
            {
              if (config.input.gamepad.scepad.led_brightness != SK_NoPreference)
              {
                pSetState->AllowLightBrightnessChange = false;

                data_changed = true;
              }

              if (config.input.gamepad.scepad.led_color_r >= 0 ||
                  config.input.gamepad.scepad.led_color_g >= 0 ||
                  config.input.gamepad.scepad.led_color_b >= 0)
              {
                pSetState->AllowLedColor = false;

                data_changed = true;
              }

              if (config.input.gamepad.disable_rumble)
              {
                pSetState->AllowLeftTriggerFFB  = false;
                pSetState->AllowRightTriggerFFB = false;
                pSetState->UseRumbleNotHaptics  = 0;
                pSetState->RumbleEmulationLeft  = 0;
                pSetState->RumbleEmulationRight = 0;
              }

              // SK controls this!
              pSetState->AllowMuteLight = false;
            }
          }
        } break;
      }
    } break;
  }

  return data_changed;
}

int SK_HID_DeviceFile::neutralizeHidInput (uint8_t report_id, DWORD dwSize)
{
  if (_cachedInputReportsByReportId [report_id].empty ())
    return 0;

  switch (device_vid)
  {
    case SK_HID_VID_SONY:
    {
      switch (device_pid)
      {
        case SK_HID_PID_DUALSHOCK4:
        case SK_HID_PID_DUALSHOCK4_REV2:
        {
          //if (report_id == 1 && _cachedInputReportsByReportId [report_id].size () >= dwSize)
          {
            if (_cachedInputReportsByReportId [report_id].data ()[0] == report_id && report_id == 1)
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
          if (dwSize != 78 && _cachedInputReportsByReportId [report_id].data ()[0] == report_id && report_id == 1)
          //if (dwSize >= 64 && (report_id == 1 || report_id == dwSize))
          {
            //SK_ReleaseAssert (_cachedInputReportsByReportId [report_id].data ()[0] == 1 ||
            //                  dwSize == report_id);

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

          if (_cachedInputReportsByReportId [report_id].data ()    != nullptr   &&
              _cachedInputReportsByReportId [report_id].data ()[0] == report_id &&
                                                  (report_id == 49 || report_id == 1))
          //if (dwSize >= 78 && (report_id == 1 || report_id == 49 || report_id == dwSize))
          {
            //SK_ReleaseAssert (
            //  ((uint8_t *)&_cachedInputReportsByReportId [report_id])[0] == 0x1 ||
            //  ((uint8_t *)&_cachedInputReportsByReportId [report_id])[0] == 49  ||
            //  dwSize == report_id
            //);

            auto *pInputRaw =
              _cachedInputReportsByReportId [report_id].data ();

            auto *pData = pInputRaw == nullptr ?
                                       nullptr :
              (SK_HID_DualSense_GetStateData *)&pInputRaw [2];

            bool bSimple = false;

            // We're in simplified mode...
            if (pData != nullptr && pInputRaw [0] == 0x1)
            {
              bSimple = true;
            }

            if (pData != nullptr && (! bSimple))
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

            else if (pInputRaw != nullptr)
            {
              auto *pSimpleData =
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

            return 78;
          }

          else
          {
            SK_RunOnce (
              SK_LOGi0 (
                L"Cannot neutralize a DualSense HID Input Report (id=%u) with Unexpected Size=%u-bytes",
                  report_id, dwSize
              )
            );
          }
        } break;
      }
    } break;
  }

  return 0;
}

int SK_HID_DeviceFile::remapHidInput (void)
{
  return 0;
}


bool
SK_HID_PlayStationDevice::initialize_serial (void)
{
  DWORD dwBytesRead = 0x0;

  SK_DeviceIoControl (
    hDeviceFile, IOCTL_HID_GET_SERIALNUMBER_STRING, 0, 0,
        wszSerialNumber, 128, &dwBytesRead, nullptr );
  
  // The HID minidriver couldn't get a serial number (MAC address), but
  //   we can get the address manually using a special Feature Report...
  if (*wszSerialNumber == L'\0')
  {
    // The structures are the same across all controllers, but USB
    //   report IDs differ; for Bluetooth we don't even need this.
    if (bDualSense)
      feature_report [0] = 0x09;
    else
      feature_report [0] = 0x12;
  
    if (SK_HidD_GetFeature (hDeviceFile, feature_report.data (),
                    static_cast <ULONG> (feature_report.size ())))
    {
      const auto *pGetHWAddr =
        (SK_HID_DualSense_GetHWAddr *)&feature_report.data ()[1];
  
      // If this fails, what the hell did we just read?
      SK_ReleaseAssert ( pGetHWAddr->Hard00 == 0x00 &&
                         pGetHWAddr->Hard08 == 0x08 &&
                         pGetHWAddr->Hard25 == 0x25 );
  
      swprintf (
        wszSerialNumber, L"%02x%02x%02x%02x%02x%02x",
          pGetHWAddr->ClientMAC [5], pGetHWAddr->ClientMAC [4],
          pGetHWAddr->ClientMAC [3], pGetHWAddr->ClientMAC [2],
          pGetHWAddr->ClientMAC [1], pGetHWAddr->ClientMAC [0] );
    }
  }
  
  if (*wszSerialNumber != L'\0')
  {
    return
      (1 == swscanf (wszSerialNumber, L"%llx", &ullHWAddr));
  }

  return false;
}

void
SK_HID_PlayStationDevice::reset_force_feedback (void) noexcept
{
  _vibration.trigger.last_resist_start_l = -2.0f;
  _vibration.trigger.last_resist_start_r = -2.0f;
  _vibration.trigger.last_resist_str_l   = -2.0f;
  _vibration.trigger.last_resist_str_r   = -2.0f;
}

void
SK_HID_PlayStationDevice::reset_device (void)
{
#ifdef _DEBUG
  SK_LOGi0 (
    L"SK_HID_PlayStationDevice::reset_device (...) for DeviceFile='%ws'",
                                                    wszDevicePath );
#endif

  battery.percentage = 100.0f;
  battery.state      = ChargingError;

  output.last_crc32c = 0;

  latency.last_syn  = 0;
  latency.last_ack  = 0;
  latency.ping      = 0;
  latency.last_poll = 0;

  WriteULongRelease (&_vibration.left,     0);
  WriteULongRelease (&_vibration.right,    0);
  WriteULongRelease (&_vibration.last_set, 0);
  WriteULongRelease (&_vibration.max_val,  0);

  _vibration.trigger.last_left   = 0;
  _vibration.trigger.last_right  = 0;
  _vibration.trigger.start_left  = 0;
  _vibration.trigger.start_right = 0;

  reset_force_feedback ();

  WriteULong64Release (&xinput.last_active, 0);

  xinput.         prev_report = {};
  xinput.              report = {};
  xinput.internal.prev_report = {};
  xinput.internal.     report = {};

  dwLastTimeOutput  = 0;
  ulLastFrameOutput = 0;

  WriteRelease (&bNeedOutput, TRUE);
  bSimpleMode = true;

  for ( auto& button : buttons )
  {
    button.last_state = 0;
    button.     state = 0;
  }

  sensor_timestamp = 0;
  reset_rgb        = false;

  // Reset the translated device input so that when this controller reconnects,
  //   it will not still be reading stale input until the user pushes a button.
  extern XINPUT_STATE hid_to_xi;
                      hid_to_xi.Gamepad = {};
}

void
SK_HID_DualSense_SetStateDataImpl (SK_HID_DualSense_SetStateData* report, bool left, playstation_trigger_effect effect, float param0, float param1, float param2)
{
  if (left) report->AllowLeftTriggerFFB  = true;
  else      report->AllowRightTriggerFFB = true;

  uint8_t effects [][11] = {
    { 0x05, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0 }, // Off
    { 0x01, 0, 63, 0, 0, 0, 0, 0, 0, 0, 0 }, // Feedback
    { 0x00, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0 }, // ...
    { 0x06, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // Vibration
  };

  if (effect >= playstation_trigger_effect::Invalid)
      effect  = playstation_trigger_effect::Off;

  uint8_t* effect_data = effects [effect];

  auto data =
    left ? report->LeftTriggerFFB
         : report->RightTriggerFFB;

  memcpy (data, effect_data, sizeof (effects [0]));

  if (effect != playstation_trigger_effect::Off)
  {
    if (param0 != -1.0f) data [1] = static_cast <uint8_t> (std::clamp (param0 * 255.0f, 0.0f, 255.0f));
    if (param1 != -1.0f) data [2] = static_cast <uint8_t> (std::clamp (param1 * 255.0f, 0.0f, 255.0f));
    if (param2 != -1.0f) data [3] = static_cast <uint8_t> (std::clamp (param2 * 255.0f, 0.0f, 255.0f));
  }
}

void
SK_HID_DualSense_SetStateData::setTriggerEffectL (playstation_trigger_effect effect, float param0, float param1, float param2)
{
  return SK_HID_DualSense_SetStateDataImpl (this, true, effect, param0, param1, param2);
}

void
SK_HID_DualSense_SetStateData::setTriggerEffectR (playstation_trigger_effect effect, float param0, float param1, float param2)
{
  return SK_HID_DualSense_SetStateDataImpl (this, false, effect, param0, param1, param2);
}