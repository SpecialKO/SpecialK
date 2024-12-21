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

#ifndef __SK__GAME_INPUT_H__
#define __SK__GAME_INPUT_H__

#include <GameInput.h>

class SK_IWrapGameInput : IGameInput
{
public:
  SK_IWrapGameInput (IGameInput *pGameInput) : pReal (pGameInput),
                                                ver_ (0)
  {
    if (pGameInput == nullptr)
      return;
  }

#pragma region IUnknown
  virtual HRESULT  __stdcall QueryInterface      (REFIID riid, void **ppvObject) noexcept override;
  virtual ULONG    __stdcall AddRef              (void)                          noexcept override;
  virtual ULONG    __stdcall Release             (void)                          noexcept override;
#pragma endregion

#pragma region IGameInput
  virtual uint64_t __stdcall GetCurrentTimestamp (void) noexcept override;
  virtual HRESULT  __stdcall GetCurrentReading   (_In_          GameInputKind      inputKind,
                                                  _In_opt_     IGameInputDevice   *device,
                                                  _COM_Outptr_ IGameInputReading **reading) noexcept override;
  virtual HRESULT  __stdcall GetNextReading      (_In_         IGameInputReading  *referenceReading,
                                                  _In_          GameInputKind      inputKind,
                                                  _In_opt_     IGameInputDevice   *device,
                                                  _COM_Outptr_ IGameInputReading **reading) noexcept override;
  virtual HRESULT  __stdcall GetPreviousReading  (_In_         IGameInputReading  *referenceReading,
                                                  _In_          GameInputKind      inputKind,
                                                  _In_opt_     IGameInputDevice   *device,
                                                  _COM_Outptr_ IGameInputReading **reading) noexcept override;
  virtual HRESULT  __stdcall GetTemporalReading  (_In_         uint64_t            timestamp,
                                                  _In_         IGameInputDevice   *device,
                                                  _COM_Outptr_ IGameInputReading **reading) noexcept override;

  virtual HRESULT  __stdcall RegisterReadingCallback (_In_opt_                         IGameInputDevice          *device,
                                                      _In_                              GameInputKind             inputKind,
                                                      _In_                              float                     analogThreshold,
                                                      _In_opt_                          void                     *context,
                                                      _In_                              GameInputReadingCallback  callbackFunc,
                                                      _Out_opt_ _Result_zeroonfailure_  GameInputCallbackToken   *callbackToken) noexcept override;
  virtual HRESULT  __stdcall RegisterDeviceCallback  (_In_opt_                        IGameInputDevice         *device,
                                                      _In_                             GameInputKind            inputKind,
                                                      _In_                             GameInputDeviceStatus    statusFilter,
                                                      _In_                             GameInputEnumerationKind enumerationKind,
                                                      _In_opt_                         void                    *context,
                                                      _In_                             GameInputDeviceCallback  callbackFunc,
                                                      _Out_opt_ _Result_zeroonfailure_ GameInputCallbackToken  *callbackToken) noexcept override;

  virtual HRESULT  __stdcall RegisterSystemButtonCallback   (_In_opt_                         IGameInputDevice               *device,
                                                             _In_                              GameInputSystemButtons         buttonFilter,
                                                             _In_opt_                          void                          *context,
                                                             _In_                              GameInputSystemButtonCallback  callbackFunc,
                                                             _Out_opt_ _Result_zeroonfailure_  GameInputCallbackToken        *callbackToken)   noexcept override;
  virtual HRESULT  __stdcall RegisterKeyboardLayoutCallback (_In_opt_                         IGameInputDevice                 *device,
                                                             _In_opt_                          void                            *context,
                                                             _In_                              GameInputKeyboardLayoutCallback  callbackFunc,
                                                             _Out_opt_ _Result_zeroonfailure_  GameInputCallbackToken          *callbackToken) noexcept override;

  virtual void     __stdcall StopCallback       (_In_ GameInputCallbackToken callbackToken)         noexcept override;
  virtual bool     __stdcall UnregisterCallback (_In_ GameInputCallbackToken callbackToken,
                                                 _In_ uint64_t               timeoutInMicroseconds) noexcept override;

  virtual HRESULT  __stdcall CreateDispatcher      (_COM_Outptr_ IGameInputDispatcher **dispatcher) noexcept override;
  virtual HRESULT  __stdcall CreateAggregateDevice (_In_          GameInputKind     inputKind,
                                                    _COM_Outptr_ IGameInputDevice **device)         noexcept override;

  virtual HRESULT  __stdcall FindDeviceFromId             (_In_         APP_LOCAL_DEVICE_ID const  *value,
                                                           _COM_Outptr_ IGameInputDevice          **device) noexcept override;
  virtual HRESULT  __stdcall FindDeviceFromObject         (_In_         IUnknown          *value,
                                                           _COM_Outptr_ IGameInputDevice **device)          noexcept override;
  virtual HRESULT  __stdcall FindDeviceFromPlatformHandle (_In_         HANDLE             value,
                                                           _COM_Outptr_ IGameInputDevice **device)          noexcept override;
  virtual HRESULT  __stdcall FindDeviceFromPlatformString (_In_         LPCWSTR            value,
                                                           _COM_Outptr_ IGameInputDevice **device)          noexcept override;

  virtual HRESULT  __stdcall EnableOemDeviceSupport (_In_ uint16_t vendorId,
                                                     _In_ uint16_t productId,
                                                     _In_ uint8_t interfaceNumber,
                                                     _In_ uint8_t collectionNumber)    noexcept override;
  virtual void     __stdcall SetFocusPolicy         (_In_ GameInputFocusPolicy policy) noexcept override;
#pragma endregion

private:
  volatile LONG  refs_ = 1;
  IGameInput    *pReal;
  unsigned int   ver_  = 0;
};


class SK_IGameInputDevice : IGameInputDevice
{
public:
  SK_IGameInputDevice (IGameInputDevice *pGameInputDevice) : pReal (pGameInputDevice),
                                                              ver_ (0)
  {
    if (pGameInputDevice == nullptr)
      return;
  };

#pragma region IUnknown
  virtual HRESULT                    __stdcall QueryInterface                  (REFIID riid, void **ppvObject)               noexcept override; // 0
  virtual ULONG                      __stdcall AddRef                          (void)                                        noexcept override; // 1
  virtual ULONG                      __stdcall Release                         (void)                                        noexcept override; // 2
#pragma endregion

#pragma region IGameInputDevice
  virtual GameInputDeviceInfo const* __stdcall GetDeviceInfo                   (void)                                        noexcept override; // 3
  virtual GameInputDeviceStatus      __stdcall GetDeviceStatus                 (void)                                        noexcept override; // 4
  virtual void                       __stdcall GetBatteryState                 (GameInputBatteryState *state)                noexcept override; // 5

  virtual HRESULT                    __stdcall CreateForceFeedbackEffect       (uint32_t motorIndex,
                                                                                GameInputForceFeedbackParams const *params,
                                                                               IGameInputForceFeedbackEffect       **effect) noexcept override; // 6
  virtual bool                       __stdcall IsForceFeedbackMotorPoweredOn   (uint32_t motorIndex)                         noexcept override; // 7
  virtual void                       __stdcall SetForceFeedbackMotorGain       (uint32_t motorIndex,       float masterGain) noexcept override; // 8
  virtual HRESULT                    __stdcall SetHapticMotorState             (uint32_t motorIndex,
                                                                                GameInputHapticFeedbackParams const *params) noexcept override; // 9
  virtual void                       __stdcall SetRumbleState                  (GameInputRumbleParams         const *params) noexcept override; // 10
  virtual void                       __stdcall SetInputSynchronizationState    (                               bool enabled) noexcept override; // 11
  virtual void                       __stdcall SendInputSynchronizationHint    (void)                                        noexcept override; // 12
  virtual void                       __stdcall PowerOff                        (void)                                        noexcept override; // 13
                                                                               
  virtual HRESULT                    __stdcall CreateRawDeviceReport           (uint32_t                     reportId,
                                                                                GameInputRawDeviceReportKind reportKind,
                                                                               IGameInputRawDeviceReport   **report)         noexcept override; // 14
  virtual HRESULT                    __stdcall GetRawDeviceFeature             (uint32_t                     reportId,
                                                                               IGameInputRawDeviceReport   **report)         noexcept override; // 15
  virtual HRESULT                    __stdcall SetRawDeviceFeature             (IGameInputRawDeviceReport   *report)         noexcept override; // 16
  virtual HRESULT                    __stdcall SendRawDeviceOutput             (IGameInputRawDeviceReport   *report)         noexcept override; // 17
  virtual HRESULT                    __stdcall SendRawDeviceOutputWithResponse (IGameInputRawDeviceReport   *requestReport,
                                                                                IGameInputRawDeviceReport **responseReport)  noexcept override; // 18
  virtual HRESULT                    __stdcall ExecuteRawDeviceIoControl       (uint32_t    controlCode,
                                                                                size_t      inputBufferSize,
                                                                                void const *inputBuffer,
                                                                                size_t      outputBufferSize,
                                                                                void       *outputBuffer,
                                                                                size_t     *outputSize)                      noexcept override; // 19
  virtual bool                       __stdcall AcquireExclusiveRawDeviceAccess (uint64_t timeoutInMicroseconds)              noexcept override; // 20
  virtual void                       __stdcall ReleaseExclusiveRawDeviceAccess (void)                                        noexcept override; // 21
#pragma endregion

private:
  volatile LONG         refs_ = 1;
  IGameInputDevice     *pReal;
  unsigned int          ver_  = 0;
};

class SK_IWrapGameInputReading : IGameInputReading
{
public:
  SK_IWrapGameInputReading (IGameInputReading *pGameInputReading) : pReal (pGameInputReading),
                                                                     ver_ (0)
  {
    if (pGameInputReading == nullptr)
      return;
  };

  SK_IWrapGameInputReading (SK_IWrapGameInputReading&& wrapped);

#pragma region IUnknown
  virtual HRESULT       __stdcall QueryInterface           (REFIID riid, void **ppvObject)          noexcept override;
  virtual ULONG         __stdcall AddRef                   (void)                                   noexcept override;
  virtual ULONG         __stdcall Release                  (void)                                   noexcept override;
#pragma endregion

#pragma region IGameInputReading
  virtual GameInputKind __stdcall GetInputKind             (void)                                   noexcept override;
  virtual uint64_t      __stdcall GetSequenceNumber        (GameInputKind               inputKind)  noexcept override;
  virtual uint64_t      __stdcall GetTimestamp             (void)                                   noexcept override;
  virtual void          __stdcall GetDevice                (IGameInputDevice           **device)    noexcept override;
  virtual bool          __stdcall GetRawReport             (IGameInputRawDeviceReport  **report)    noexcept override;
  virtual uint32_t      __stdcall GetControllerAxisCount   (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetControllerAxisState   (uint32_t                    stateArrayCount,
                                                            float                      *stateArray) noexcept override;
  virtual uint32_t      __stdcall GetControllerButtonCount (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetControllerButtonState (uint32_t                    stateArrayCount,
                                                            bool                       *stateArray) noexcept override;
  virtual uint32_t      __stdcall GetControllerSwitchCount (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetControllerSwitchState (uint32_t                    stateArrayCount,
                                                            GameInputSwitchPosition    *stateArray) noexcept override;
  virtual uint32_t      __stdcall GetKeyCount              (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetKeyState              (uint32_t                    stateArrayCount,
                                                            GameInputKeyState          *stateArray) noexcept override;
  virtual bool          __stdcall GetMouseState            (GameInputMouseState        *state)      noexcept override;
  virtual uint32_t      __stdcall GetTouchCount            (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetTouchState            (uint32_t                    stateArrayCount,
                                                            GameInputTouchState        *stateArray) noexcept override;
  virtual bool          __stdcall GetMotionState           (GameInputMotionState       *state)      noexcept override;
  virtual bool          __stdcall GetArcadeStickState      (GameInputArcadeStickState  *state)      noexcept override;
  virtual bool          __stdcall GetFlightStickState      (GameInputFlightStickState  *state)      noexcept override;
  virtual bool          __stdcall GetGamepadState          (GameInputGamepadState      *state)      noexcept override;
  virtual bool          __stdcall GetRacingWheelState      (GameInputRacingWheelState  *state)      noexcept override;
  virtual bool          __stdcall GetUiNavigationState     (GameInputUiNavigationState *state)      noexcept override;
#pragma endregion

private:
  volatile LONG         refs_ = 1;
  IGameInputReading    *pReal;
  unsigned int          ver_  = 0;
};

class SK_IPlayStationGameInputReading : IGameInputReading
{
public:
  SK_IPlayStationGameInputReading (HANDLE hDeviceFile) : hDevice (hDeviceFile),
                                                            ver_ (0)
  {
  };

#pragma region IUnknown
  virtual HRESULT       __stdcall QueryInterface           (REFIID riid, void **ppvObject)          noexcept override;
  virtual ULONG         __stdcall AddRef                   (void)                                   noexcept override;
  virtual ULONG         __stdcall Release                  (void)                                   noexcept override;
#pragma endregion

#pragma region IGameInputReading
  virtual GameInputKind __stdcall GetInputKind             (void)                                   noexcept override;
  virtual uint64_t      __stdcall GetSequenceNumber        (GameInputKind               inputKind)  noexcept override;
  virtual uint64_t      __stdcall GetTimestamp             (void)                                   noexcept override;
  virtual void          __stdcall GetDevice                (IGameInputDevice           **device)    noexcept override;
  virtual bool          __stdcall GetRawReport             (IGameInputRawDeviceReport  **report)    noexcept override;
  virtual uint32_t      __stdcall GetControllerAxisCount   (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetControllerAxisState   (uint32_t                    stateArrayCount,
                                                            float                      *stateArray) noexcept override;
  virtual uint32_t      __stdcall GetControllerButtonCount (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetControllerButtonState (uint32_t                    stateArrayCount,
                                                            bool                       *stateArray) noexcept override;
  virtual uint32_t      __stdcall GetControllerSwitchCount (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetControllerSwitchState (uint32_t                    stateArrayCount,
                                                            GameInputSwitchPosition    *stateArray) noexcept override;
  virtual uint32_t      __stdcall GetKeyCount              (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetKeyState              (uint32_t                    stateArrayCount,
                                                            GameInputKeyState          *stateArray) noexcept override;
  virtual bool          __stdcall GetMouseState            (GameInputMouseState        *state)      noexcept override;
  virtual uint32_t      __stdcall GetTouchCount            (void)                                   noexcept override;
  virtual uint32_t      __stdcall GetTouchState            (uint32_t                    stateArrayCount,
                                                            GameInputTouchState        *stateArray) noexcept override;
  virtual bool          __stdcall GetMotionState           (GameInputMotionState       *state)      noexcept override;
  virtual bool          __stdcall GetArcadeStickState      (GameInputArcadeStickState  *state)      noexcept override;
  virtual bool          __stdcall GetFlightStickState      (GameInputFlightStickState  *state)      noexcept override;
  virtual bool          __stdcall GetGamepadState          (GameInputGamepadState      *state)      noexcept override;
  virtual bool          __stdcall GetRacingWheelState      (GameInputRacingWheelState  *state)      noexcept override;
  virtual bool          __stdcall GetUiNavigationState     (GameInputUiNavigationState *state)      noexcept override;
#pragma endregion

private:
  volatile LONG         refs_ = 1;
  HANDLE                hDevice;
  unsigned int          ver_  = 0;
};

void SK_Input_HookGameInput (void);

#endif /* __SK__GAME_INPUT_H__ */