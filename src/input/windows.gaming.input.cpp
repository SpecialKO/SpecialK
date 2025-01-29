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
#include <SpecialK/input/windows.gaming.input.h>

#include <imgui/font_awesome.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Input Mgr."

#define SK_LOG_INPUT_CALL { static int  calls  = 0;             \
  { SK_LOG0 ( (L"[!] > Call #%lu: %hs", calls++, __FUNCTION__), \
               L"Input Mgr." ); } }

///////////////////////////////////////////////////////
//
// Windows.Gaming.Input.dll  (SK_WGI)
//
///////////////////////////////////////////////////////

#define SK_WGI_READ(backend,type)  (backend)->markRead   (type);
#define SK_WGI_WRITE(backend,type) (backend)->markWrite  (type);
#define SK_WGI_VIEW(backend,slot)  (backend)->markViewed ((sk_input_dev_type)(1 << slot));
#define SK_WGI_HIDE(backend,type)  (backend)->markHidden (type);

#define WGI_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {      \
  void** _vftable = *(void***)*(_Base);                                       \
                                                                              \
  if ((_Original) == nullptr) {                                               \
    SK_CreateVFTableHook2 ( L##_Name,                                         \
                              _vftable,                                       \
                                (_Index),                                     \
                                  (_Override),                                \
                                    (LPVOID *)&(_Original));                  \
  }                                                                           \
}
#define WGI_VIRTUAL_HOOK_IMM(_Base,_Index,_Name,_Override,_Original,_Type) {  \
  void** _vftable = *(void***)*(_Base);                                       \
                                                                              \
  if ((_Original) == nullptr) {                                               \
    SK_CreateVFTableHook ( L##_Name,                                          \
                             _vftable,                                        \
                               (_Index),                                      \
                                 (_Override),                                 \
                                   (LPVOID *)&(_Original));                   \
  }                                                                           \
}

#include <roapi.h>
#include <wrl.h>
#include <windows.gaming.input.h>

using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Gaming::Input;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

#pragma comment (lib, "runtimeobject.lib")

using RoGetActivationFactory_pfn = HRESULT (WINAPI *)(HSTRING, REFIID, void **);
      RoGetActivationFactory_pfn
      RoGetActivationFactory_Original = nullptr;

using WGI_GamepadStatistics2_FromGameController_pfn = HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepadStatics2  *This,
                                                                                    ABI::Windows::Gaming::Input::IGameController   *gameController,
                                                                                    ABI::Windows::Gaming::Input::IGamepad         **value);

WGI_GamepadStatistics2_FromGameController_pfn
WGI_GamepadStatistics2_FromGameController_Original = nullptr;

using WGI_GamepadStatistics_add_ChangeEvent_pfn    = HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepadStatics  *This,
                                                                           __FIEventHandler_1_Windows__CGaming__CInput__CGamepad *value,
                                                                                                          EventRegistrationToken *token);
using WGI_GamepadStatistics_remove_ChangeEvent_pfn = HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepadStatics  *This,
                                                                                                           EventRegistrationToken token);
using WGI_GamepadStatistics_get_Gamepads_pfn       = HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepadStatics  *This,
                                                                      IVectorView <ABI::Windows::Gaming::Input::Gamepad*>       **value);
using WGI_Gamepad_GetCurrentReading_pfn            = HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepad         *This,
                                                                                   ABI::Windows::Gaming::Input::GamepadReading   *value);
using WGI_Gamepad_get_Vibration_pfn                = HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepad         *This,
                                                                                   ABI::Windows::Gaming::Input::GamepadVibration *value);
using WGI_Gamepad_put_Vibration_pfn                = HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepad         *This,
                                                                                   ABI::Windows::Gaming::Input::GamepadVibration  value);

WGI_GamepadStatistics_get_Gamepads_pfn
WGI_GamepadStatistics_get_Gamepads_Original = nullptr;

WGI_GamepadStatistics_add_ChangeEvent_pfn
WGI_GamepadStatistics_add_GamepadAdded_Original   = nullptr,
WGI_GamepadStatistics_add_GamepadRemoved_Original = nullptr;

WGI_GamepadStatistics_remove_ChangeEvent_pfn
WGI_GamepadStatistics_remove_GamepadAdded_Original   = nullptr,
WGI_GamepadStatistics_remove_GamepadRemoved_Original = nullptr;

WGI_Gamepad_GetCurrentReading_pfn
WGI_Gamepad_GetCurrentReading_Original = nullptr;

WGI_Gamepad_get_Vibration_pfn
WGI_Gamepad_get_Vibration_Original = nullptr;

WGI_Gamepad_put_Vibration_pfn
WGI_Gamepad_put_Vibration_Original = nullptr;

using WGI_VectorView_Gamepads_GetAt_pfn    = HRESULT (STDMETHODCALLTYPE *)(IVectorView<ABI::Windows::Gaming::Input::Gamepad*> *This, _In_     unsigned  index,      _Out_ void     **item);
using WGI_VectorView_Gamepads_get_Size_pfn = HRESULT (STDMETHODCALLTYPE *)(IVectorView<ABI::Windows::Gaming::Input::Gamepad*> *This, _Out_    unsigned *size);
using WGI_VectorView_Gamepads_IndexOf_pfn  = HRESULT (STDMETHODCALLTYPE *)(IVectorView<ABI::Windows::Gaming::Input::Gamepad*> *This, _In_opt_ void     *value,      _Out_ unsigned *index,    _Out_                             boolean *found);
using WGI_VectorView_Gamepads_GetMany_pfn  = HRESULT (STDMETHODCALLTYPE *)(IVectorView<ABI::Windows::Gaming::Input::Gamepad*> *This, _In_     unsigned  startIndex, _In_  unsigned  capacity, _Out_writes_to_(capacity,*actual) void     *value, _Out_ unsigned *actual);

static WGI_VectorView_Gamepads_GetAt_pfn
       WGI_VectorView_Gamepads_GetAt_Original = nullptr;
static WGI_VectorView_Gamepads_get_Size_pfn
       WGI_VectorView_Gamepads_get_Size_Original = nullptr;
static WGI_VectorView_Gamepads_IndexOf_pfn
       WGI_VectorView_Gamepads_IndexOf_Original = nullptr;
static WGI_VectorView_Gamepads_GetMany_pfn
       WGI_VectorView_Gamepads_GetMany_Original = nullptr;

struct SK_HID_WGI_Gamepad;

struct DECLSPEC_UUID("1baf6522-5f64-42c5-8267-b9fe2215bfbd")
SK_HID_WGI_GameController : ABI::Windows::Gaming::Input::IGameController
{
public:
  SK_HID_WGI_Gamepad* parent = nullptr;

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject) override
  {
    SK_LOG_FIRST_CALL

    return
      ((IUnknown *)parent)->QueryInterface (riid, ppvObject);
  }
  
  virtual ULONG STDMETHODCALLTYPE AddRef (void) override
  {
    SK_LOG_FIRST_CALL

    return InterlockedIncrement (&ulRefs);
  }

  virtual ULONG STDMETHODCALLTYPE Release (void) override
  {
    SK_LOG_FIRST_CALL

    return InterlockedDecrement (&ulRefs);
  }

  virtual HRESULT STDMETHODCALLTYPE GetIids (
            /* [out] */ __RPC__out ULONG *iidCount,
            /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = iidCount;
    std::ignore = iids;

    return E_NOTIMPL;
  }
        
  virtual HRESULT STDMETHODCALLTYPE GetRuntimeClassName (
    /* [out] */ __RPC__deref_out_opt HSTRING *className ) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = className;

    return E_NOTIMPL;
  }        

  virtual HRESULT STDMETHODCALLTYPE GetTrustLevel (
    /* [out] */ __RPC__out TrustLevel *trustLevel ) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = trustLevel;

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE add_HeadsetConnected (__FITypedEventHandler_2_Windows__CGaming__CInput__CIGameController_Windows__CGaming__CInput__CHeadset *value,
                                                          EventRegistrationToken                                                                                *token) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = value;
    std::ignore = token;

    return S_OK; // ?
  }
  
  virtual HRESULT STDMETHODCALLTYPE remove_HeadsetConnected (EventRegistrationToken token) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = token;

    return S_OK; // ?
  }
  
  virtual HRESULT STDMETHODCALLTYPE add_HeadsetDisconnected (__FITypedEventHandler_2_Windows__CGaming__CInput__CIGameController_Windows__CGaming__CInput__CHeadset *value,
                                                             EventRegistrationToken                                                                                *token) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = value;
    std::ignore = token;

    return S_OK; // ?
  }
  
  virtual HRESULT STDMETHODCALLTYPE remove_HeadsetDisconnected (EventRegistrationToken token) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = token;

    return S_OK; // ?
  }

  virtual HRESULT STDMETHODCALLTYPE add_UserChanged (__FITypedEventHandler_2_Windows__CGaming__CInput__CIGameController_Windows__CSystem__CUserChangedEventArgs *value,
                                                     EventRegistrationToken                                                                                     *token) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = value;
    std::ignore = token;

    return S_OK; // ?
  }
  
  virtual HRESULT STDMETHODCALLTYPE remove_UserChanged (EventRegistrationToken token) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = token;

    return S_OK; // ?
  }
  
  virtual HRESULT STDMETHODCALLTYPE get_Headset (ABI::Windows::Gaming::Input::IHeadset **value) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = value;

    return E_NOTIMPL;
  }
  
  virtual HRESULT STDMETHODCALLTYPE get_IsWireless (boolean *value) override
  {
    SK_LOG_FIRST_CALL

    if (value == nullptr)
      return E_INVALIDARG;

    // Check if any attached controllers are wireless...
    //   the same controller may be attached using multiple methods.
    for ( auto& controller : SK_HID_PlayStationControllers )
    {
      if (controller.bBluetooth && controller.bConnected)
      {
        *value = true;
        return S_OK;
      }
    }

    *value = false;

    return S_OK;
  }
  
  virtual HRESULT STDMETHODCALLTYPE get_User (ABI::Windows::System::IUser **value) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = value;

    return E_NOTIMPL;
  }

private:
  volatile ULONG ulRefs = 1;
};

SK_HID_WGI_GameController SK_HID_WGI_VirtualController_USB;
SK_HID_WGI_GameController SK_HID_WGI_VirtualController_Bluetooth;

bool SK_WGI_EmulatedPlayStation = false;

SK_LazyGlobal <
  concurrency::concurrent_unordered_map < ABI::Windows::Gaming::Input::IGamepad*,
                                          ABI::Windows::Gaming::Input::GamepadVibration >
> SK_WGI_LastPutVibration;

HRESULT
STDMETHODCALLTYPE
WGI_Gamepad_get_Vibration_Override (ABI::Windows::Gaming::Input::IGamepad         *This,
                                    ABI::Windows::Gaming::Input::GamepadVibration *value)
{
  SK_LOG_FIRST_CALL

  if (value == nullptr)
    return E_INVALIDARG;

  // Forza Horizon 5 uses XInput and Windows.Gaming.Input simultaneously...
  //   if we did not mark this as a read, activity would not show up in SK's
  //     control panel.
  SK_WGI_READ (SK_WGI_Backend, sk_input_dev_type::Gamepad);

  *value =
    SK_WGI_LastPutVibration.get()[This];

  if (WGI_Gamepad_get_Vibration_Original != nullptr && This != nullptr)
  {
    return
      WGI_Gamepad_get_Vibration_Original (This, value);
  }

  return S_OK;
}

HRESULT
STDMETHODCALLTYPE
WGI_Gamepad_put_Vibration_Override (ABI::Windows::Gaming::Input::IGamepad         *This,
                                    ABI::Windows::Gaming::Input::GamepadVibration  value)
{
  SK_LOG_FIRST_CALL

  std::ignore = This;

  // Forza Horizon 5 uses XInput and Windows.Gaming.Input simultaneously...
  //   if we did not mark this as a read, activity would not show up in SK's
  //     control panel.
  SK_WGI_READ (SK_WGI_Backend, sk_input_dev_type::Gamepad);

  // Swallow vibration while capturing gamepad input
  if (config.input.gamepad.disable_rumble || SK_ImGui_WantGamepadCapture ())
  {
    SK_WGI_HIDE (SK_WGI_Backend, sk_input_dev_type::Gamepad);

    SK_XInput_ZeroHaptics (0);
    return S_OK;
  }

  auto *pNewestInputDevice =
    SK_HID_GetActivePlayStationDevice (true);

  if (pNewestInputDevice != nullptr && config.input.gamepad.xinput.emulate)
  {
    SK_WGI_VIEW (SK_WGI_Backend, 0);

    SK_WGI_EmulatedPlayStation = true;

    pNewestInputDevice->setVibration (
      static_cast <USHORT> (std::min (65535UL, static_cast <ULONG> (std::clamp (value.LeftMotor,    0.0, 1.0) * 65536.0))),
      static_cast <USHORT> (std::min (65535UL, static_cast <ULONG> (std::clamp (value.RightMotor,   0.0, 1.0) * 65536.0))),
      static_cast <USHORT> (std::min (65535UL, static_cast <ULONG> (std::clamp (value.LeftTrigger,  0.0, 1.0) * 65536.0))),
      static_cast <USHORT> (std::min (65535UL, static_cast <ULONG> (std::clamp (value.RightTrigger, 0.0, 1.0) * 65536.0))),
                                      65535ui16
    );

    pNewestInputDevice->write_output_report ();
  }

  else
  {
    SK_WGI_EmulatedPlayStation = false;
  }

  // Forward the input to XInput or real Windows.Gaming.Input because there was no PlayStation device
  if (! SK_WGI_EmulatedPlayStation)
  {
    if (WGI_Gamepad_put_Vibration_Original != nullptr && This != nullptr)
    {
      return
        WGI_Gamepad_put_Vibration_Original (This, value);
    }
  }
  
  return S_OK;
}

HRESULT
STDMETHODCALLTYPE
WGI_Gamepad_GetCurrentReading_Override (ABI::Windows::Gaming::Input::IGamepad       *This,
                                        ABI::Windows::Gaming::Input::GamepadReading *value)
{
  SK_LOG_FIRST_CALL

  SK_WGI_READ (SK_WGI_Backend, sk_input_dev_type::Gamepad);

  auto hr_real =
    This != nullptr && value != nullptr && WGI_Gamepad_GetCurrentReading_Original != nullptr ?
                                           WGI_Gamepad_GetCurrentReading_Original (This, value) : E_NOTIMPL;

  if (SK_ImGui_WantGamepadCapture ())
  {
    HRESULT hr =
      config.input.gamepad.xinput.emulate ? S_OK
                                          : hr_real;

    if (SUCCEEDED (hr) && value != nullptr)
    {
      SK_WGI_HIDE (SK_WGI_Backend, sk_input_dev_type::Gamepad);

      value->Buttons          = GamepadButtons::GamepadButtons_None;
      value->LeftThumbstickX  = 0.0;
      value->LeftThumbstickY  = 0.0;
      value->RightThumbstickX = 0.0;
      value->RightThumbstickY = 0.0;
      value->LeftTrigger      = 0.0;
      value->RightTrigger     = 0.0;
    }

    else
      SK_ReleaseAssert (FAILED (hr) || value != nullptr);

    return hr;
  }

  // Windows.Gaming.Input cannot poll controller state while the window is not technically focused.
  //  * So we fallback to XInput, which can only give a limited subset of button state.
  else if ((! config.input.gamepad.xinput.emulate) && SK_WantBackgroundRender () && (! game_window.active) && config.input.gamepad.disabled_to_game == 0)
  {
    HRESULT hr = hr_real;

    if (SUCCEEDED (hr))
    {
      SK_WGI_EmulatedPlayStation = false;

      SK_WGI_VIEW (SK_WGI_Backend, 0);

      static DWORD        dwLastTime =  0 ;
      static XINPUT_STATE xi_state   = { };

      if (std::exchange (dwLastTime, SK_timeGetTime ()) <
                                     SK_timeGetTime () - 2)
      {
        SK_XInput_PollController (0, &xi_state);
      }

      value->Buttons = GamepadButtons::GamepadButtons_None;

      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadUp;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadDown;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadLeft;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadRight;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER))
                      value->Buttons |= GamepadButtons::GamepadButtons_LeftShoulder;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER))
                      value->Buttons |= GamepadButtons::GamepadButtons_RightShoulder;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB))
                      value->Buttons |= GamepadButtons::GamepadButtons_LeftThumbstick;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB))
                      value->Buttons |= GamepadButtons::GamepadButtons_RightThumbstick;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_A))
                      value->Buttons |= GamepadButtons::GamepadButtons_A;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_B))
                      value->Buttons |= GamepadButtons::GamepadButtons_B;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_X))
                      value->Buttons |= GamepadButtons::GamepadButtons_X;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y))
                      value->Buttons |= GamepadButtons::GamepadButtons_Y;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_START))
                      value->Buttons |= GamepadButtons::GamepadButtons_Menu;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK))
                      value->Buttons |= GamepadButtons::GamepadButtons_View;

      value->LeftThumbstickX  = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbLX / 32767);
      value->LeftThumbstickY  = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbLY / 32767);

      value->RightThumbstickX = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbRX / 32767);
      value->RightThumbstickY = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbRY / 32767);

      value->LeftTrigger      = (double)xi_state.Gamepad.bLeftTrigger  / 255;
      value->RightTrigger     = (double)xi_state.Gamepad.bRightTrigger / 255;
    }

    return hr;
  }

  else if (! config.input.gamepad.xinput.emulate)
  {
    SK_WGI_EmulatedPlayStation = false;

    HRESULT hr = hr_real;

    if (SUCCEEDED (hr) && game_window.active)
    {
      SK_WGI_VIEW (SK_WGI_Backend, 0);
    }

    return hr;
  } 

  else if (! config.input.gamepad.xinput.blackout_api)
  {
    XINPUT_STATE xi_state = { };

    if (SK_HID_PlayStationControllers.size () > 0)
    {
      XINPUT_STATE                    _state = {};
      SK_ImGui_PollGamepad_EndFrame (&_state);
    }

    auto pNewestInputDevice =
      SK_HID_GetActivePlayStationDevice (true);

    if (pNewestInputDevice != nullptr)
    {
      SK_WGI_VIEW (SK_WGI_Backend, 0);

      const auto latest_state   =
        pNewestInputDevice->xinput.getLatestState ();

      const auto last_timestamp = ReadULong64Acquire (&hid_to_xi_time);
      const auto timestamp      = ReadULong64Acquire (&pNewestInputDevice->xinput.last_active);

      memcpy (&xi_state, &latest_state, sizeof (XINPUT_STATE));

      if (                                              timestamp> last_timestamp  &&
           InterlockedCompareExchange (&hid_to_xi_time, timestamp, last_timestamp) == last_timestamp )
      {
        hid_to_xi = latest_state;
      }

      // Enable XInputSetState to redirect to this controller
      extern bool bUseEmulationForSetState;
                  bUseEmulationForSetState = true;

      value->Timestamp = timestamp;
      value->Buttons   = GamepadButtons::GamepadButtons_None;

      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadUp;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadDown;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadLeft;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadRight;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER))
                      value->Buttons |= GamepadButtons::GamepadButtons_LeftShoulder;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER))
                      value->Buttons |= GamepadButtons::GamepadButtons_RightShoulder;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB))
                      value->Buttons |= GamepadButtons::GamepadButtons_LeftThumbstick;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB))
                      value->Buttons |= GamepadButtons::GamepadButtons_RightThumbstick;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_A))
                      value->Buttons |= GamepadButtons::GamepadButtons_A;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_B))
                      value->Buttons |= GamepadButtons::GamepadButtons_B;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_X))
                      value->Buttons |= GamepadButtons::GamepadButtons_X;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y))
                      value->Buttons |= GamepadButtons::GamepadButtons_Y;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_START))
                      value->Buttons |= GamepadButtons::GamepadButtons_Menu;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK))
                      value->Buttons |= GamepadButtons::GamepadButtons_View;

      if (pNewestInputDevice->bDualSenseEdge)
      {
        // Xbox Elite <--> DualSense Edge mapping
        //   Paddle 1 = Right Paddle
        //   Paddle 2 = Right Function
        //   Paddle 3 = Left Paddle
        //   Paddle 4 = Left Function

        if (pNewestInputDevice->isButtonDown (SK_HID_PlayStationButton::RightPaddle))
          value->Buttons |= GamepadButtons::GamepadButtons_Paddle1;
        if (pNewestInputDevice->isButtonDown (SK_HID_PlayStationButton::LeftPaddle))
          value->Buttons |= GamepadButtons::GamepadButtons_Paddle3;
        if (pNewestInputDevice->isButtonDown (SK_HID_PlayStationButton::RightFn))
          value->Buttons |= GamepadButtons::GamepadButtons_Paddle2;
        if (pNewestInputDevice->isButtonDown (SK_HID_PlayStationButton::LeftFn))
          value->Buttons |= GamepadButtons::GamepadButtons_Paddle4;
      }

      value->LeftThumbstickX  = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbLX / 32767);
      value->LeftThumbstickY  = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbLY / 32767);

      value->RightThumbstickX = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbRX / 32767);
      value->RightThumbstickY = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbRY / 32767);

      value->LeftTrigger      = (double)xi_state.Gamepad.bLeftTrigger  / 255;
      value->RightTrigger     = (double)xi_state.Gamepad.bRightTrigger / 255;

      if (config.input.gamepad.xinput.debug)
      {
        SK_ImGui_CreateNotification (
          "WGI.PacketNum", SK_ImGui_Toast::Info, SK_FormatString ("Windows.Gaming.Input Packet: %d", xi_state.dwPacketNumber).c_str (), nullptr, INFINITE,
                           SK_ImGui_Toast::UseDuration  |
                           SK_ImGui_Toast::ShowCaption  |
                           SK_ImGui_Toast::ShowNewest   |
                           SK_ImGui_Toast::Unsilencable |
                           SK_ImGui_Toast::DoNotSaveINI );
      }

      SK_WGI_EmulatedPlayStation = true;

      return S_OK;
    }

    else
    {
      SK_WGI_EmulatedPlayStation = false;

      SK_WGI_VIEW (SK_WGI_Backend, 0);

      static DWORD dwLastTime =  0 ;

      if (std::exchange (dwLastTime, SK_timeGetTime ()) <
                                     SK_timeGetTime () - 2)
      {
        SK_XInput_PollController (0, &xi_state);
      }

      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadUp;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadDown;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadLeft;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT))
                      value->Buttons |= GamepadButtons::GamepadButtons_DPadRight;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER))
                      value->Buttons |= GamepadButtons::GamepadButtons_LeftShoulder;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER))
                      value->Buttons |= GamepadButtons::GamepadButtons_RightShoulder;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB))
                      value->Buttons |= GamepadButtons::GamepadButtons_LeftThumbstick;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB))
                      value->Buttons |= GamepadButtons::GamepadButtons_RightThumbstick;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_A))
                      value->Buttons |= GamepadButtons::GamepadButtons_A;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_B))
                      value->Buttons |= GamepadButtons::GamepadButtons_B;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_X))
                      value->Buttons |= GamepadButtons::GamepadButtons_X;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y))
                      value->Buttons |= GamepadButtons::GamepadButtons_Y;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_START))
                      value->Buttons |= GamepadButtons::GamepadButtons_Menu;
      if ((xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK))
                      value->Buttons |= GamepadButtons::GamepadButtons_View;

      value->LeftThumbstickX  = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbLX / 32767);
      value->LeftThumbstickY  = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbLY / 32767);

      value->RightThumbstickX = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbRX / 32767);
      value->RightThumbstickY = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbRY / 32767);

      value->LeftTrigger      = (double)xi_state.Gamepad.bLeftTrigger  / 255;
      value->RightTrigger     = (double)xi_state.Gamepad.bRightTrigger / 255;
    }
  }

  return S_OK;
}

struct DECLSPEC_UUID("bc7bb43c-0a69-3903-9e9d-a50f86a45de5")
SK_HID_WGI_Gamepad : ABI::Windows::Gaming::Input::IGamepad
{
public:
  void _setupBinding (SK_HID_WGI_GameController *virtual_controller)
  {
    virtual_controller->parent = this;
    controller = virtual_controller;
  }

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject) override
  {
    SK_LOG_FIRST_CALL

    if (ppvObject == nullptr)
      return E_INVALIDARG;

    *ppvObject = nullptr;

    if ( IsEqualGUID (riid, IID_IUnknown)     ||
         IsEqualGUID (riid, IID_IInspectable) ||
         IsEqualGUID (riid, IID_IGamepad) )
    {
      AddRef ();
      *ppvObject = this;

      return S_OK;
    }

    if (IsEqualGUID (riid, IID_IGameController))
    {
      controller->AddRef ();

      *ppvObject = controller;

      return S_OK;
    }

    static
    concurrency::concurrent_unordered_set <std::wstring> reported_guids;

    wchar_t                wszGUID [41] = { };
    StringFromGUID2 (riid, wszGUID, 40);

    const bool
          once = reported_guids.count (wszGUID) > 0;
    if (! once)
    {
      reported_guids.insert (wszGUID);

      SK_LOG0 ( ( L"QueryInterface on ABI::Windows::Gaming::Input::IGamepad for Mystery UUID: %s",
                      wszGUID ), L"VirtualWGI" );
    }                            

    return E_NOINTERFACE;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef (void) override
  {
    SK_LOG_FIRST_CALL

    return InterlockedIncrement (&ulRefs);
  }

  virtual ULONG STDMETHODCALLTYPE Release (void) override
  {
    SK_LOG_FIRST_CALL

    return InterlockedDecrement (&ulRefs);
  }

  virtual HRESULT STDMETHODCALLTYPE GetIids (
            /* [out] */ __RPC__out ULONG *iidCount,
            /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*iidCount) IID **iids) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = iidCount;
    std::ignore = iids;

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetRuntimeClassName (
    /* [out] */ __RPC__deref_out_opt HSTRING *className ) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = className;

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetTrustLevel (
    /* [out] */ __RPC__out TrustLevel *trustLevel ) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = trustLevel;

    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE get_Vibration (ABI::Windows::Gaming::Input::GamepadVibration* value) override
  {
    SK_LOG_FIRST_CALL

    if (value != nullptr)
    {
      *value = vibes;
    }

    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE put_Vibration (ABI::Windows::Gaming::Input::GamepadVibration value) override
  {
    SK_LOG_FIRST_CALL

    return
      WGI_Gamepad_put_Vibration_Override (nullptr, value);
  }

  virtual HRESULT STDMETHODCALLTYPE GetCurrentReading (ABI::Windows::Gaming::Input::GamepadReading* value) override
  {
    SK_LOG_FIRST_CALL

    return
      WGI_Gamepad_GetCurrentReading_Override (nullptr, value);
  }

private:
  volatile ULONG ulRefs = 1;

  ABI::Windows::Gaming::Input::GamepadVibration vibes      = {};
  SK_HID_WGI_GameController*                    controller = nullptr;
};

SK_HID_WGI_Gamepad SK_HID_WGI_VirtualGamepad_Wired;
SK_HID_WGI_Gamepad SK_HID_WGI_VirtualGamepad_Wireless;

HRESULT
STDMETHODCALLTYPE
WGI_VectorView_Gamepads_GetAt_Override (       IVectorView<ABI::Windows::Gaming::Input::Gamepad*>     *This,
                                         _In_  unsigned  index,
                                         _Out_ void    **item )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    WGI_VectorView_Gamepads_GetAt_Original (This, index, item);

  if (0 != _wcsicmp (This->z_get_rc_name_impl (), L"Windows.Foundation.Collections.IVectorView`1<Windows.Gaming.Input.Gamepad>"))
    return hr;

  // All controllers not found in the real list of controllers will be substituted with SK's
  //   wrapper interface.
  if (FAILED (hr) && config.input.gamepad.xinput.emulate)
  {
    if (item != nullptr)
    {
      *item = (void *)&SK_HID_WGI_VirtualGamepad_Wired;
      return S_OK;
    }
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
WGI_VectorView_Gamepads_get_Size_Override (       IVectorView<ABI::Windows::Gaming::Input::Gamepad*>     *This,
                                            _Out_ unsigned *size )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    WGI_VectorView_Gamepads_get_Size_Original (This, size);

  if (0 != _wcsicmp (This->z_get_rc_name_impl (), L"Windows.Foundation.Collections.IVectorView`1<Windows.Gaming.Input.Gamepad>"))
    return hr;

  // Add 1 to the size so that games never see an empty list
  if (config.input.gamepad.xinput.emulate && size != nullptr && (*size == 0 || FAILED (hr)))
  {
    *size = 1;
  }

  return
    hr;
}

HRESULT
STDMETHODCALLTYPE
WGI_VectorView_Gamepads_IndexOf_Override (          IVectorView<ABI::Windows::Gaming::Input::Gamepad*>     *This,
                                           _In_opt_ void     *value,
                                           _Out_    unsigned *index,
                                           _Out_    boolean  *found )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    WGI_VectorView_Gamepads_IndexOf_Original (This, value, index, found);

  if (0 != _wcsicmp (This->z_get_rc_name_impl (), L"Windows.Foundation.Collections.IVectorView`1<Windows.Gaming.Input.Gamepad>"))
    return hr;

  // All failing controllers will map to the end of the list
  if (FAILED (hr) && config.input.gamepad.xinput.emulate)
  {
    *index = 0;
    *found = true;
    return S_OK;
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
WGI_VectorView_Gamepads_GetMany_Override (                                   IVectorView<ABI::Windows::Gaming::Input::Gamepad*>     *This,
                                           _In_                              unsigned  startIndex,
                                           _In_                              unsigned  capacity,
                                           _Out_writes_to_(capacity,*actual) void     *value,
                                           _Out_                             unsigned *actual )
{
  SK_LOG_FIRST_CALL

  //
  // This API is UNIMPLEMENTED, hopefully nothing uses it.
  //
  SK_ReleaseAssert (!"Unimplemented Windows.Gaming.Input API");

  return
    WGI_VectorView_Gamepads_GetMany_Original (This, startIndex, capacity, value, actual);
}

// Required by Forza Horizon 5, otherwise when a wired controller is connected it will
//   constantly switch between gamepad and keyboard prompts and drop input.
HRESULT
STDMETHODCALLTYPE
WGI_GamepadStatistics2_FromGameController_Override (ABI::Windows::Gaming::Input::IGamepadStatics2  *This,
                                                    ABI::Windows::Gaming::Input::IGameController   *gameController,
                                                    ABI::Windows::Gaming::Input::IGamepad         **value)
{
  SK_LOG_FIRST_CALL

  if (value != nullptr && config.input.gamepad.xinput.emulate && SK_ImGui_HasPlayStationController ())
  {
    boolean                          wireless = false;
    gameController->get_IsWireless (&wireless);

    SK_ComQIPtr <ABI::Windows::Gaming::Input::IRawGameController>
        pRawGameController (gameController);
    if (pRawGameController.p != nullptr)
    {
      UINT16                                     controller_vid = 0;
      pRawGameController->get_HardwareVendorId (&controller_vid);

      if (controller_vid == SK_HID_VID_SONY)
      {
        SK_LOGi0 (
          L"Mapping Raw Windows.Gaming.Input Controller to %ws PlayStation Gamepad",
            wireless ? L"Bluetooth" : L"USB"
        );

         *value = wireless ? &SK_HID_WGI_VirtualGamepad_Wireless :
                             &SK_HID_WGI_VirtualGamepad_Wired;

        (*value)->AddRef ();

        return S_OK;
      }

      else if (controller_vid != SK_HID_VID_MICROSOFT)
      {
        UINT16                                     controller_pid = 0;
        pRawGameController->get_HardwareVendorId (&controller_pid);

        SK_LOGi0 (L"non-SONY/Microsoft HID Controller: VID=%04x, PID=%04x",
                  controller_vid, controller_pid);
      }
    }
  }

  return
    WGI_GamepadStatistics2_FromGameController_Original (This, gameController, value);
}


HRESULT
STDMETHODCALLTYPE
WGI_GamepadStatistics_get_Gamepads_Override ( ABI::Windows::Gaming::Input::IGamepadStatics  *This,
                                 IVectorView <ABI::Windows::Gaming::Input::Gamepad*>       **value )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    WGI_GamepadStatistics_get_Gamepads_Original (This, value);

  if (config.input.gamepad.xinput.emulate)
  {
    //  0 QueryInterface
    //  1 AddRef
    //  2 Release
    //  3 GetIids
    //  4 GetRuntimeClassName
    //  5 GetTrustLevel
    //  6 GetAt
    //  7 get_Size
    //  8 IndexOf
    //  9 GetMany

    // Hooks used to manipulate the returned list of gamepads so that we can
    //   dynamically add/remove/substitute gamepads.
    SK_RunOnce ({
      WGI_VIRTUAL_HOOK ( value, 6,
        "ABI::Windows::Foundation::Collections::IVectorView "
                        "<ABI::Windows::Gaming::Input::Gamepad*>::GetAt",
                                          WGI_VectorView_Gamepads_GetAt_Override,
                                          WGI_VectorView_Gamepads_GetAt_Original,
                                          WGI_VectorView_Gamepads_GetAt_pfn );

      WGI_VIRTUAL_HOOK ( value, 7,
        "ABI::Windows::Foundation::Collections::IVectorView "
                        "<ABI::Windows::Gaming::Input::Gamepad*>::get_Size",
                                          WGI_VectorView_Gamepads_get_Size_Override,
                                          WGI_VectorView_Gamepads_get_Size_Original,
                                          WGI_VectorView_Gamepads_get_Size_pfn );

      WGI_VIRTUAL_HOOK ( value, 8,
        "ABI::Windows::Foundation::Collections::IVectorView "
                        "<ABI::Windows::Gaming::Input::Gamepad*>::IndexOf",
                                          WGI_VectorView_Gamepads_IndexOf_Override,
                                          WGI_VectorView_Gamepads_IndexOf_Original,
                                          WGI_VectorView_Gamepads_IndexOf_pfn );
      
      WGI_VIRTUAL_HOOK ( value, 9,
        "ABI::Windows::Foundation::Collections::IVectorView "
                        "<ABI::Windows::Gaming::Input::Gamepad*>::GetMany",
                                          WGI_VectorView_Gamepads_GetMany_Override,
                                          WGI_VectorView_Gamepads_GetMany_Original,
                                          WGI_VectorView_Gamepads_GetMany_pfn );

      SK_ApplyQueuedHooks ();
    });
  }

  return hr;
}

static void hook_wgi_gamepad_vtable (ABI::Windows::Gaming::Input::IGamepad** ppGamepad)
{
  SK_LOG_FIRST_CALL

  // 0 QueryInterface
  // 1 AddRef
  // 2 Release
  // 3 GetIids
  // 4 GetRuntimeClassName
  // 5 GetTrustLevel
  // 6 get_Vibration
  // 7 put_Vibration
  // 8 GetCurrentReading

  if (ppGamepad != nullptr &&
     *ppGamepad != &SK_HID_WGI_VirtualGamepad_Wired &&
     *ppGamepad != &SK_HID_WGI_VirtualGamepad_Wireless)
  {
    SK_RunOnce ({
      WGI_VIRTUAL_HOOK ( ppGamepad, 8,
                "ABI::Windows::Gaming::Input::IGamepad::GetCurrentReading",
                 WGI_Gamepad_GetCurrentReading_Override,
                 WGI_Gamepad_GetCurrentReading_Original,
                 WGI_Gamepad_GetCurrentReading_pfn );
    
      WGI_VIRTUAL_HOOK ( ppGamepad, 6,
                  "ABI::Windows::Gaming::Input::IGamepad::get_Vibration",
                   WGI_Gamepad_get_Vibration_Override,
                   WGI_Gamepad_get_Vibration_Original,
                   WGI_Gamepad_get_Vibration_pfn );
    
      WGI_VIRTUAL_HOOK ( ppGamepad, 7,
                  "ABI::Windows::Gaming::Input::IGamepad::put_Vibration",
                   WGI_Gamepad_put_Vibration_Override,
                   WGI_Gamepad_put_Vibration_Original,
                   WGI_Gamepad_put_Vibration_pfn );
    
      SK_ApplyQueuedHooks ();
    });
  }
}

static const IID IID_IEventHandler_Gamepad = { 0x8a7639ee, 0x624a, 0x501a, { 0xbb, 0x53, 0x56, 0x2d, 0x1e, 0xc1, 0x1b, 0x52 } };

class SK_HID_WGI_GamepadListener_Impl : public __FIEventHandler_1_Windows__CGaming__CInput__CGamepad
{
public:
  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject) override
  {
    if (ppvObject == nullptr)
      return E_INVALIDARG;

    *ppvObject = nullptr;

    if ( IsEqualGUID (riid, IID_IUnknown)     ||
         IsEqualGUID (riid, IID_IAgileObject) ||
         IsEqualGUID (riid, IID_IEventHandler_Gamepad) )
    {
      AddRef ();
      *ppvObject = this;

      return S_OK;
    }

    // This hardly matters... (famous last words)
#ifdef _DEBUG
    static
    concurrency::concurrent_unordered_set <std::wstring> reported_guids;

    wchar_t                wszGUID [41] = { };
    StringFromGUID2 (riid, wszGUID, 40);

    bool once =
      reported_guids.count (wszGUID) > 0;

    if (! once)
    {
      reported_guids.insert (wszGUID);

      SK_LOG0 ( ( L"QueryInterface on SK_HID_WGI_GamepadListener_Impl for Mystery UUID: %s",
                      wszGUID ), L"VirtualWGI" );
    }
#endif

    return E_NOINTERFACE;
  }
  
  virtual ULONG STDMETHODCALLTYPE AddRef (void) override
  {
    return InterlockedIncrement (&ulRefs);
  }

  virtual ULONG STDMETHODCALLTYPE Release (void) override
  {
    return InterlockedDecrement (&ulRefs);
  }

  virtual HRESULT STDMETHODCALLTYPE Invoke (_In_ IInspectable*, _In_ ABI::Windows::Gaming::Input::IGamepad* gamepad) override
  {
    SK_LOG_FIRST_CALL

    // Hook the first _real_ gamepad seen's vtable
    hook_wgi_gamepad_vtable (&gamepad);

    return S_OK;
  }

  ABI::Windows::Gaming::Input::IGamepadStatics* factory = nullptr;

private:
  volatile ULONG ulRefs = 1;
} static SK_HID_WGI_GamepadListener;

using WGI_GamepadStatistics_EventRegistration_t =
  std::pair <EventRegistrationToken,__FIEventHandler_1_Windows__CGaming__CInput__CGamepad*>;

concurrency::concurrent_unordered_multimap <ABI::Windows::Gaming::Input::IGamepadStatics*, WGI_GamepadStatistics_EventRegistration_t> SK_WGI_GamepadAddedEvents;
concurrency::concurrent_unordered_multimap <ABI::Windows::Gaming::Input::IGamepadStatics*, WGI_GamepadStatistics_EventRegistration_t> SK_WGI_GamepadRemovedEvents;

HRESULT
STDMETHODCALLTYPE
WGI_GamepadStatistics_add_GamepadAdded_Override (
    ABI::Windows::Gaming::Input::IGamepadStatics* This, __FIEventHandler_1_Windows__CGaming__CInput__CGamepad* value,
                          EventRegistrationToken* token )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    WGI_GamepadStatistics_add_GamepadAdded_Original (This, value, token);

  if (SUCCEEDED (hr) && token != nullptr)
  {
    SK_WGI_GamepadAddedEvents.insert (
      std::make_pair (This,
        std::make_pair (*token, value)
      )
    );
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
WGI_GamepadStatistics_remove_GamepadAdded_Override (
    ABI::Windows::Gaming::Input::IGamepadStatics* This,
                           EventRegistrationToken token )
{
  SK_LOG_FIRST_CALL

  const HRESULT hr =
    WGI_GamepadStatistics_remove_GamepadAdded_Original (This, token);

  if (SUCCEEDED (hr))
  {
    // Did we miss the installation of this event handler...?
    SK_ReleaseAssert (SK_WGI_GamepadAddedEvents.count (This) != 0);

    for (   auto &[sender,  handler] : SK_WGI_GamepadAddedEvents )
    { if ( auto &&[_token, dispatch] = handler; _token.value == token.value)
      {
        _token.value = 0;
        dispatch     = nullptr;
      }
    }
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
WGI_GamepadStatistics_add_GamepadRemoved_Override (
    ABI::Windows::Gaming::Input::IGamepadStatics* This, __FIEventHandler_1_Windows__CGaming__CInput__CGamepad* value,
                          EventRegistrationToken* token )
{
  SK_LOG_FIRST_CALL

  const HRESULT hr =
    WGI_GamepadStatistics_add_GamepadRemoved_Original (This, value, token);

  if (SUCCEEDED (hr) && token != nullptr)
  {
    SK_WGI_GamepadRemovedEvents.insert (
      std::make_pair (This,
        std::make_pair (*token, value)
      )
    );
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
WGI_GamepadStatistics_remove_GamepadRemoved_Override (
    ABI::Windows::Gaming::Input::IGamepadStatics* This,
                           EventRegistrationToken token )
{
  SK_LOG_FIRST_CALL

  const HRESULT hr =
    WGI_GamepadStatistics_remove_GamepadRemoved_Original (This, token);

  if (SUCCEEDED (hr))
  {
    // Did we miss the installation of this event handler...?
    SK_ReleaseAssert (SK_WGI_GamepadRemovedEvents.count (This) != 0);

    for (   auto &[sender,  handler] : SK_WGI_GamepadRemovedEvents )
    { if ( auto &&[_token, dispatch] = handler; _token.value == token.value)
      {
        _token.value = 0;
        dispatch     = nullptr;
      }
    }
  }

  return hr;
}

int
SK_WGI_BroadcastGamepadAdded (ABI::Windows::Gaming::Input::IGamepad*        pGamepad,
                              ABI::Windows::Gaming::Input::IGamepadStatics* pFactory = nullptr)
{
  if (! config.input.gamepad.xinput.emulate)
    return 0;

  int recipients = 0;

  for ( const auto &[sender, handler] : SK_WGI_GamepadAddedEvents )
    if (const auto &[token, dispatch] = handler;
                            dispatch != nullptr &&
                                  (pFactory == sender || pFactory == nullptr) &&
                  SUCCEEDED (dispatch->Invoke (sender, pGamepad))) ++recipients;

  SK_LOGi0 (L"Windows.Gaming.Input Virtual Gamepad Addition Broadcast to %d recipients", recipients);

  return recipients;
}

int
SK_WGI_BroadcastGamepadRemoved (ABI::Windows::Gaming::Input::IGamepad*        pGamepad,
                                ABI::Windows::Gaming::Input::IGamepadStatics* pFactory = nullptr)
{
  int recipients = 0;

  for ( const auto &[sender, handler] : SK_WGI_GamepadAddedEvents )
    if (const auto &[token, dispatch] = handler;
                            dispatch != nullptr &&
                                  (pFactory == sender || pFactory == nullptr) &&
                  SUCCEEDED (dispatch->Invoke (sender, pGamepad))) ++recipients;

  return recipients;
}

void
SK_WGI_DeferGamepadHooks (ABI::Windows::Gaming::Input::IGamepadStatics* pGamepadStatistics)
{
  SK_RunOnce (
    SK_HID_WGI_GamepadListener.factory = pGamepadStatistics;

    static EventRegistrationToken                                       gamepad_added = {};
    pGamepadStatistics->add_GamepadAdded (&SK_HID_WGI_GamepadListener, &gamepad_added);

    pGamepadStatistics->AddRef (); // Keep-alive so we can get add/remove notifications

    SK_ComPtr <IVectorView <ABI::Windows::Gaming::Input::Gamepad*>> pGamepads;

    if (SUCCEEDED (pGamepadStatistics->get_Gamepads (&pGamepads.p)) &&
                                           nullptr != pGamepads.p)
    {
      SK_ComPtr <ABI::Windows::Gaming::Input::IGamepad> pGamepad;

      uint32_t num_pads = 0;

      if (SUCCEEDED (pGamepads->get_Size (&num_pads)) && num_pads > 0 &&
          SUCCEEDED (pGamepads->GetAt (0, &pGamepad.p)))
      {
        hook_wgi_gamepad_vtable (&pGamepad.p);
      }
    }
  );
}

HRESULT
WINAPI
RoGetActivationFactory_Detour ( _In_  HSTRING activatableClassId,
                                _In_  REFIID  iid,
                                _Out_ void**  factory )
{
  SK_LOG_FIRST_CALL

  // Right now we ONLY hook RoGetActivationFactory for WGI, but check if WGI hooks
  //   are enabled before doing this anyway.
  if ( config.input.gamepad.hook_windows_gaming &&
         ( iid == IID_IGamepadStatics  ||
           iid == IID_IGamepadStatics2 ||
           iid == IID_IRawGameControllerStatics ) )
  {
    SK_HID_SetupPlayStationControllers ();

    const bool bHasPlayStationControllers =
         (! SK_HID_PlayStationControllers.empty ());

    if (bHasPlayStationControllers)
    { 
      if ((! config.input.gamepad.xinput.emulate) && SK_GetCurrentGameID () != SK_GAME_ID::ForzaHorizon5   &&
                                                     SK_GetCurrentGameID () != SK_GAME_ID::StarWarsOutlaws &&
                                                     (! SK_GetCurrentRenderBackend ().windows.sdl)         &&
                                                     (! SK_GetCurrentRenderBackend ().windows.nixxes)      &&
                                                     (! SK_GetCurrentRenderBackend ().windows.unity)       &&
                                                     (! config.input.gamepad.xinput.blackout_api)          &&
                                                       !config.input.gamepad.xinput.disable [0])
      {
        SK_RunOnce (SK_Thread_CreateEx([](LPVOID)->DWORD
        {
          while (game_window.hWnd == 0 || !IsWindow (game_window.hWnd))
          {
            SK_SleepEx (25, FALSE);
          }

          if ((! SK_GetCurrentRenderBackend ().windows.sdl) &&
              (! SK_GetCurrentRenderBackend ().windows.nixxes) &&
              (! SK_GetCurrentRenderBackend ().windows.unity))
          {
            SK_ImGui_CreateNotification ( "WindowsGamingInput.Compatibility",
                                          SK_ImGui_Toast::Warning,
              "This game uses Windows.Gaming.Input\r\n\r\n  "
              ICON_FA_PLAYSTATION "  "
              "Your PlayStation controller may not work unless Xbox Mode is"
              " enabled\r\n\r\n\t(Input Management | PlayStation > Xbox Mode)"
              " and restart the game.\r\n\r\n"
              "Disable XInput Slot 0 to stop seeing this message.",
              "Windows.Gaming.Input Incompatibility Detected",
                                          15000UL,
                                        SK_ImGui_Toast::UseDuration |
                                        SK_ImGui_Toast::ShowTitle   |
                                        SK_ImGui_Toast::ShowCaption |
                                        SK_ImGui_Toast::ShowOnce );
          }

          SK_Thread_CloseSelf ();

          return 0;
        }, L"[SK] Windows.Gaming.Input Warning Thread"));
      }
    }

    if (iid == IID_IRawGameControllerStatics)
    {
      SK_LOGi0 (L"RoGetActivationFactory (IID_IRawGameControllerStatics)");

      if (config.input.gamepad.windows_gaming_input.blackout_api)
      {
        SK_LOGi0 (L" => Disabling Interface for Current Game.");
        return E_NOTIMPL;
      }

      if (config.input.gamepad.xinput.emulate)
      {
        // XXX: TODO.
        //
        //   SDL uses this, but SDL also has support for PlayStation controllers natively.
        //
      }
    }

    if (iid == IID_IGamepadStatics2)
    {
      SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepadStatics2)");

      if (config.input.gamepad.windows_gaming_input.blackout_api)
      {
        SK_LOGi0 (L" => Disabling Interface for Current Game.");
        return E_NOTIMPL;
      }

      if (config.input.gamepad.xinput.emulate)
      {
        SK_ComPtr <ABI::Windows::Gaming::Input::IGamepadStatics2> pGamepadStats2Factory;

        HRESULT hr =
          RoGetActivationFactory_Original ( activatableClassId,
                                              iid,
                                              (void **)&pGamepadStats2Factory.p );

        if (SUCCEEDED (hr) && pGamepadStats2Factory != nullptr)
        {
          //  0 QueryInterface
          //  1 AddRef
          //  2 Release
          //  3 GetIids
          //  4 GetRuntimeClassName
          //  5 GetTrustLevel
          //  6 FromGameController

          SK_RunOnce ({
            // One and done, don't use hook queuing
            WGI_VIRTUAL_HOOK_IMM ( &pGamepadStats2Factory.p, 6,
                      "ABI::Windows::Gaming::Input::IGamepadStatics2::FromGameController",
                       WGI_GamepadStatistics2_FromGameController_Override,
                       WGI_GamepadStatistics2_FromGameController_Original,
                       WGI_GamepadStatistics2_FromGameController_pfn );
          });
        }
      }
    }

    if (iid == IID_IGamepadStatics)
    {
      SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepadStatics)");

      if (config.input.gamepad.windows_gaming_input.blackout_api)
      {
        SK_LOGi0 (L" => Disabling Interface for Current Game.");
        return E_NOTIMPL;
      }

      SK_ComPtr <ABI::Windows::Gaming::Input::IGamepadStatics> pGamepadStatsFactory;

      HRESULT hr =
        RoGetActivationFactory_Original ( activatableClassId,
                                            iid,
                                              (void **)&pGamepadStatsFactory.p );

      if (SUCCEEDED (hr) && pGamepadStatsFactory != nullptr)
      {
        //  0 QueryInterface
        //  1 AddRef
        //  2 Release
        //  3 GetIids
        //  4 GetRuntimeClassName
        //  5 GetTrustLevel
        //  6 add_GamepadAdded
        //  7 remove_GamepadAdded
        //  8 add_GamepadRemoved
        //  9 remove_GamepadRemoved
        // 10 get_Gamepads

        // We will hook the IVectorView interface to insert virtual controllers if there are no
        //   -real- Xbox controllers.
        SK_RunOnce (
        {
          WGI_VIRTUAL_HOOK ( &pGamepadStatsFactory.p, 6,
                    "ABI::Windows::Gaming::Input::IGamepadStatics::add_GamepadAdded",
                     WGI_GamepadStatistics_add_GamepadAdded_Override,
                     WGI_GamepadStatistics_add_GamepadAdded_Original,
                     WGI_GamepadStatistics_add_ChangeEvent_pfn );

          WGI_VIRTUAL_HOOK ( &pGamepadStatsFactory.p, 7,
                    "ABI::Windows::Gaming::Input::IGamepadStatics::remove_GamepadAdded",
                     WGI_GamepadStatistics_remove_GamepadAdded_Override,
                     WGI_GamepadStatistics_remove_GamepadAdded_Original,
                     WGI_GamepadStatistics_remove_ChangeEvent_pfn );

          WGI_VIRTUAL_HOOK ( &pGamepadStatsFactory.p, 8,
                    "ABI::Windows::Gaming::Input::IGamepadStatics::add_GamepadRemoved",
                     WGI_GamepadStatistics_add_GamepadRemoved_Override,
                     WGI_GamepadStatistics_add_GamepadRemoved_Original,
                     WGI_GamepadStatistics_add_ChangeEvent_pfn );

          WGI_VIRTUAL_HOOK ( &pGamepadStatsFactory.p, 9,
                    "ABI::Windows::Gaming::Input::IGamepadStatics::remove_GamepadRemoved",
                     WGI_GamepadStatistics_remove_GamepadRemoved_Override,
                     WGI_GamepadStatistics_remove_GamepadRemoved_Original,
                     WGI_GamepadStatistics_remove_ChangeEvent_pfn );

          SK_ApplyQueuedHooks ();
        });

        if (static bool hooked = false;
                       !hooked)
        {
          SK_ComPtr <IVectorView <ABI::Windows::Gaming::Input::Gamepad *>> pGamepads;

          if (SUCCEEDED (pGamepadStatsFactory->get_Gamepads (&pGamepads.p)) &&
                                                   nullptr != pGamepads.p)
          {
            bool async = false;

            // Stupid hack I'm not proud of for Forza Horizon 5 since it has anti-debug that
            //   makes it impossible to make progress on this issue...
            //
            //  * Wait multiple seconds for the game to get its head together and then invoke
            //      its gamepad added event, otherwise it would crash.
          //if (SK_GetCurrentGameID () == SK_GAME_ID::ForzaHorizon5)
            {
              async = true;
            }

                                               pGamepadStatsFactory.p->AddRef ();
            static auto future_stats_factory = pGamepadStatsFactory;

            auto vector_hook_impl = [](LPVOID bAsync)->DWORD
            {
              if (bAsync)
              {
                const DWORD dwWaitBegin = SK_timeGetTime ();

                // Wait at least 7.5 seconds before broadcasting controller attach notification
                //
                //  * Forza Horizon 5 waits almost exactly 5 seconds before instantiating a
                //      IID_IGamepadStatics2... we need to wait even longer than that.
                while (SK_HID_PlayStationControllers.empty () || dwWaitBegin > SK_timeGetTime () - 7500UL)
                {
                  SK_SleepEx (1000ul, FALSE);
                }
              }

              if (std::exchange (hooked, true) == false)
              {
                // One and done, don't use hook queuing
                WGI_VIRTUAL_HOOK_IMM ( &future_stats_factory.p, 10,
                          "ABI::Windows::Gaming::Input::IGamepadStatics::get_Gamepads",
                           WGI_GamepadStatistics_get_Gamepads_Override,
                           WGI_GamepadStatistics_get_Gamepads_Original,
                           WGI_GamepadStatistics_get_Gamepads_pfn );

                SK_ComPtr <IVectorView <ABI::Windows::Gaming::Input::Gamepad *>> pGamepads;

                // Self-invoke our hooked specialization of IVectorView to trigger the
                //   installation of additional hooks.
                future_stats_factory->get_Gamepads (&pGamepads.p);

                bool bHasWireless = false;

                for ( auto& ps_controller : SK_HID_PlayStationControllers )
                {
                  if (ps_controller.bConnected && ps_controller.bBluetooth)
                  {
                    bHasWireless = true;
                    break;
                  }
                }

                SK_WGI_BroadcastGamepadAdded (bHasWireless ? &SK_HID_WGI_VirtualGamepad_Wireless :
                                                             &SK_HID_WGI_VirtualGamepad_Wired, future_stats_factory.p);
              }

              if (bAsync)
              {
                SK_Thread_CloseSelf ();
              }

              return 0;
            };

            if (async) SK_Thread_CreateEx (vector_hook_impl, L"[SK] Async Windows.Gaming.Input Init", &async);
            else                           vector_hook_impl (nullptr);
            
            SK_WGI_DeferGamepadHooks (pGamepadStatsFactory.p);
          }
        }

        *factory = pGamepadStatsFactory.Detach ();

        return S_OK;
      }
    }
  }

  return
    RoGetActivationFactory_Original ( activatableClassId,
                                        iid,
                                          factory );
}

void
SK_Input_HookWGI (void)
{
  SK_RunOnce (
  {
    SK_HID_WGI_VirtualGamepad_Wired._setupBinding    (&SK_HID_WGI_VirtualController_USB);
    SK_HID_WGI_VirtualGamepad_Wireless._setupBinding (&SK_HID_WGI_VirtualController_Bluetooth);

    void* pfnRoGetActivationFactory = nullptr;

    // This has other applications, but for now the only thing we need it
    // for is to observe the creation of Windows.Gaming.Input factories...
    SK_CreateDLLHook2 (     L"Combase.dll",
                             "RoGetActivationFactory",
                              RoGetActivationFactory_Detour,
     static_cast_p2p <void> (&RoGetActivationFactory_Original),
                          &pfnRoGetActivationFactory );
    SK_EnableHook (        pfnRoGetActivationFactory );


    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      DWORD dwTimeoutPeriodInMs        = 25;
      DWORD dwTimeStartedLookingForWGI = SK_timeGetTime ();

      do
      {
        // No real need to do this if the game hasn't loaded Windows.Gaming.Input.dll yet
        if (! SK_GetModuleHandleW (L"Windows.Gaming.Input.dll"))
        {
          dwTimeoutPeriodInMs =
            std::min (dwTimeoutPeriodInMs * 2, 5000UL);

          if (SK_timeGetTime () - dwTimeStartedLookingForWGI > 30000)
          {
            // After 30 seconds, it's likely the game is never going to load Windows.Gaming.Input...
            break;
          }

          SK_SleepEx (dwTimeoutPeriodInMs, FALSE);
          continue;
        }

        // Init COM on this thread
        SK_AutoCOMInit _;
    
        PCWSTR         wszNamespace = L"Windows.Gaming.Input.Gamepad";
        HSTRING_HEADER   hNamespaceStringHeader;
        HSTRING          hNamespaceString;
    
        if (SUCCEEDED (WindowsCreateStringReference (wszNamespace,
                       static_cast <UINT32> (wcslen (wszNamespace)),
                                                      &hNamespaceStringHeader,
                                                      &hNamespaceString)))
        {
          SK_ComPtr <ABI::Windows::Gaming::Input::IGamepadStatics> pGamepadStatics;
    
          if (SUCCEEDED (RoGetActivationFactory_Original (hNamespaceString, IID_IGamepadStatics,
                                                                      (void **)&pGamepadStatics.p)))
          {
            SK_WGI_DeferGamepadHooks (pGamepadStatics.p);
          }
        }

        break;
      } while (WaitForSingleObject (__SK_DLL_TeardownEvent, dwTimeoutPeriodInMs) != WAIT_OBJECT_0);
    
      SK_Thread_CloseSelf ();
    
      return 0;
    }, L"[SK] Deferred WinRT (Windows.Gaming.Input) Init");
  });
}