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

using WGI_GamepadStatistics_get_Gamepads_pfn = HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepadStatics  *This,
                                                                IVectorView <ABI::Windows::Gaming::Input::Gamepad*>       **value);
using WGI_Gamepad_GetCurrentReading_pfn      = HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepad         *This,
                                                                             ABI::Windows::Gaming::Input::GamepadReading   *value);

WGI_GamepadStatistics_get_Gamepads_pfn
WGI_GamepadStatistics_get_Gamepads_Original = nullptr;

WGI_Gamepad_GetCurrentReading_pfn
WGI_Gamepad_GetCurrentReading_Original = nullptr;

using WGI_VectorView_Gamepads_GetAt_pfn    = HRESULT (STDMETHODCALLTYPE *)(void *This, _In_     unsigned  index,      _Out_ void     **item);
using WGI_VectorView_Gamepads_get_Size_pfn = HRESULT (STDMETHODCALLTYPE *)(void *This, _Out_    unsigned *size);
using WGI_VectorView_Gamepads_IndexOf_pfn  = HRESULT (STDMETHODCALLTYPE *)(void *This, _In_opt_ void     *value,      _Out_ unsigned *index,    _Out_                             boolean *found);
using WGI_VectorView_Gamepads_GetMany_pfn  = HRESULT (STDMETHODCALLTYPE *)(void *This, _In_     unsigned  startIndex, _In_  unsigned  capacity, _Out_writes_to_(capacity,*actual) void     *value, _Out_ unsigned *actual);

static WGI_VectorView_Gamepads_GetAt_pfn
       WGI_VectorView_Gamepads_GetAt_Original = nullptr;
static WGI_VectorView_Gamepads_get_Size_pfn
       WGI_VectorView_Gamepads_get_Size_Original = nullptr;
static WGI_VectorView_Gamepads_IndexOf_pfn
       WGI_VectorView_Gamepads_IndexOf_Original = nullptr;
static WGI_VectorView_Gamepads_GetMany_pfn
       WGI_VectorView_Gamepads_GetMany_Original = nullptr;

static volatile DWORD  last_packet [XUSER_MAX_COUNT] = {};
static volatile UINT64 last_time   [XUSER_MAX_COUNT] = {};

struct SK_HID_WGI_Gamepad;

struct DECLSPEC_UUID("1baf6522-5f64-42c5-8267-b9fe2215bfbd")
SK_HID_WGI_GameController : ABI::Windows::Gaming::Input::IGameController
{
  SK_HID_WGI_Gamepad* parent_;

public:
  SK_HID_WGI_GameController (SK_HID_WGI_Gamepad *pParent)
  {
    parent_ = pParent;
  }

  SK_HID_WGI_GameController            (const SK_HID_WGI_GameController &) = delete;
  SK_HID_WGI_GameController &operator= (const SK_HID_WGI_GameController &) = delete;

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject) override
  {
    SK_LOG_FIRST_CALL

    return
      ((IUnknown *)parent_)->QueryInterface (riid, ppvObject);
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

  virtual HRESULT STDMETHODCALLTYPE GetIids( 
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

  virtual HRESULT STDMETHODCALLTYPE GetTrustLevel( 
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

struct DECLSPEC_UUID("bc7bb43c-0a69-3903-9e9d-a50f86a45de5")
SK_HID_WGI_Gamepad : ABI::Windows::Gaming::Input::IGamepad
{
public:
  SK_HID_WGI_Gamepad (void)
  {
    controller = new
      SK_HID_WGI_GameController (this);
  }

  SK_HID_WGI_Gamepad            (const SK_HID_WGI_Gamepad &) = delete;
  SK_HID_WGI_Gamepad &operator= (const SK_HID_WGI_Gamepad &) = delete;

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

    bool once =
      reported_guids.count (wszGUID) > 0;

    if (! once)
    {
      reported_guids.insert (wszGUID);

      SK_LOG0 ( ( L"QueryInterface on ABI::Windows::Gaming::Input::IGamepad for Mystery UUID: %s",
                      wszGUID ), L"SK.FakeWGI" );
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

  virtual HRESULT STDMETHODCALLTYPE GetIids( 
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

    vibes = value;

    //SK_HID_PlayStationDevice *pNewestInputDevice = nullptr;

    bool bConnected = false;

    for ( auto& ps_controller : SK_HID_PlayStationControllers )
    {
      if (ps_controller.bConnected)
      {
        ps_controller.setVibration (
          (std::min (255ui16, static_cast <USHORT> (vibes.LeftMotor  * 255.0 + vibes.LeftTrigger  * 255.0))),
          (std::min (255ui16, static_cast <USHORT> (vibes.RightMotor * 255.0 + vibes.RightTrigger * 255.0))), 255ui16
        );

        if ((ps_controller.bBluetooth && config.input.gamepad.bt_input_only))
        {
        }

        else if ((! (ps_controller.bBluetooth && ps_controller.bSimpleMode)) || (vibes.LeftMotor > 0.0 || vibes.RightMotor > 0.0))
        {
          if (ps_controller.bBluetooth && (vibes.LeftMotor <= 0.0 && vibes.RightMotor <= 0.0))
          {
            if (ps_controller.write_output_report ()) // Let the device decide whether to process this or not
              bConnected = true;
          }
          else
          {
            // Force an update
            if (ps_controller.write_output_report (true))
              bConnected = true;
          }
        }
      }
    }

    if (! bConnected)
      SK_XInput_PulseController (0, (float)vibes.LeftMotor, (float)vibes.RightMotor);

    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetCurrentReading (ABI::Windows::Gaming::Input::GamepadReading* value) override
  {
    SK_LOG_FIRST_CALL

    std::ignore = value;

    return S_OK;
  }

private:
  volatile ULONG ulRefs = 1;

  ABI::Windows::Gaming::Input::GamepadVibration vibes;
  SK_HID_WGI_GameController*                    controller;
};

SK_HID_WGI_Gamepad SK_HID_WGI_FakeGamepad;

HRESULT
STDMETHODCALLTYPE
WGI_VectorView_Gamepads_GetAt_Override (       void     *This,
                                         _In_  unsigned  index,
                                         _Out_ void    **item )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    WGI_VectorView_Gamepads_GetAt_Original (This, index, item);

  if (FAILED (hr) && config.input.gamepad.xinput.emulate)
  {
    *item = (void *)&SK_HID_WGI_FakeGamepad;
    return S_OK;
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
WGI_VectorView_Gamepads_get_Size_Override (       void     *This,
                                            _Out_ unsigned *size )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    WGI_VectorView_Gamepads_get_Size_Original (This, size);

  if (SUCCEEDED (hr) && config.input.gamepad.xinput.emulate)
  {
    *size = *size + 1;
  }

  return
    hr;
}

HRESULT
STDMETHODCALLTYPE
WGI_VectorView_Gamepads_IndexOf_Override (          void     *This,
                                           _In_opt_ void     *value,
                                           _Out_    unsigned *index,
                                           _Out_    boolean  *found )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    WGI_VectorView_Gamepads_IndexOf_Original (This, value, index, found);

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
WGI_VectorView_Gamepads_GetMany_Override (                                   void     *This,
                                           _In_                              unsigned  startIndex,
                                           _In_                              unsigned  capacity,
                                           _Out_writes_to_(capacity,*actual) void     *value,
                                           _Out_                             unsigned *actual )
{
  SK_LOG_FIRST_CALL

  //if (config.input.gamepad.xinput.emulate)
  //{
  //  value = pFakeGamepad;
  //  return S_OK;
  //}

  return
    WGI_VectorView_Gamepads_GetMany_Original (This, startIndex, capacity, value, actual);
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

#if 1
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
#endif
  }

  return hr;
}

bool SK_WGI_EmulatedPlayStation = false;

HRESULT
STDMETHODCALLTYPE
WGI_Gamepad_GetCurrentReading_Override (ABI::Windows::Gaming::Input::IGamepad       *This,
                                        ABI::Windows::Gaming::Input::GamepadReading *value)
{
  SK_LOG_FIRST_CALL

  SK_WGI_READ (SK_WGI_Backend, sk_input_dev_type::Gamepad);

  if (SK_ImGui_WantGamepadCapture ())
  {
    HRESULT hr =
      config.input.gamepad.xinput.emulate ? S_OK
                                          : WGI_Gamepad_GetCurrentReading_Original (This, value);

    if (SUCCEEDED (hr))
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

    return hr;
  }

  else if ((! config.input.gamepad.xinput.emulate) && SK_WantBackgroundRender () && (! game_window.active) && config.input.gamepad.disabled_to_game == 0)
  {
    HRESULT hr =
      WGI_Gamepad_GetCurrentReading_Original (This, value);

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

    HRESULT hr =
      WGI_Gamepad_GetCurrentReading_Original (This, value);

    if (SUCCEEDED (hr) && game_window.active)
    {
      SK_WGI_VIEW (SK_WGI_Backend, 0);
    }

    return hr;
  } 

  else if (config.input.gamepad.xinput.emulate && (! config.input.gamepad.xinput.blackout_api))
  {
    // Use emulation if there is no Xbox controller, but also if PlayStation input is newer
    bool bUseEmulation = true;

    static XINPUT_STATE xi_state = { };

    SK_HID_PlayStationDevice *pNewestInputDevice = nullptr;

    if (SK_HID_PlayStationControllers.size () > 0)
    {
      for ( auto& controller : SK_HID_PlayStationControllers )
      {
        if (controller.bConnected)
        {
          if (pNewestInputDevice == nullptr ||
              pNewestInputDevice->xinput.last_active <= controller.xinput.last_active)
          {
            pNewestInputDevice = &controller;
          }
        }
      }
    }

    if (pNewestInputDevice != nullptr && (bUseEmulation || pNewestInputDevice->xinput.last_active > ReadULong64Acquire (&last_time [0])))
    {
      SK_WGI_VIEW (SK_WGI_Backend, 0);

      extern XINPUT_STATE hid_to_xi;
                          hid_to_xi = pNewestInputDevice->xinput.prev_report;

      memcpy (    &xi_state, &hid_to_xi, sizeof (XINPUT_STATE) );

      value->Timestamp = SK_QueryPerf ().QuadPart;
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
        if (pNewestInputDevice->buttons [15].state)
          value->Buttons |= GamepadButtons::GamepadButtons_Paddle1;
        if (pNewestInputDevice->buttons [16].state)
          value->Buttons |= GamepadButtons::GamepadButtons_Paddle2;
        if (pNewestInputDevice->buttons [17].state)
          value->Buttons |= GamepadButtons::GamepadButtons_Paddle3;
        if (pNewestInputDevice->buttons [18].state)
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
          "XInput.PacketNum", SK_ImGui_Toast::Info, SK_FormatString ("XInputEx Packet: %d", xi_state.dwPacketNumber).c_str (), nullptr, INFINITE,
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
  }

  return S_OK;
}

HRESULT
WINAPI
RoGetActivationFactory_Detour ( _In_  HSTRING activatableClassId,
                                _In_  REFIID  iid,
                                _Out_ void**  factory )
{
  SK_LOG_FIRST_CALL

  if ( iid == IID_IGamepad        ||
       iid == IID_IGamepad2       ||
       iid == IID_IGamepadStatics ||
       iid == IID_IGamepadStatics2 )
  {
    SK_RunOnce (
      SK_HID_SetupPlayStationControllers ()
    );

    const bool bHasPlayStationControllers =
         (! SK_HID_PlayStationControllers.empty ());

    if (bHasPlayStationControllers)
    { 
      if ((! config.input.gamepad.xinput.emulate) && SK_GetCurrentGameID () != SK_GAME_ID::HorizonForbiddenWest      &&
                                                     SK_GetCurrentGameID () != SK_GAME_ID::RatchetAndClank_RiftApart &&
                                                     SK_GetCurrentGameID () != SK_GAME_ID::ForzaHorizon5             &&
                                                     SK_GetCurrentGameID () != SK_GAME_ID::StarWarsOutlaws           &&
                                                     (! SK_GetCurrentRenderBackend ().windows.sdl))
      {
        SK_ImGui_CreateNotification ( "WindowsGamingInput.Compatibility",
                                      SK_ImGui_Toast::Warning,
          "This game uses Windows.Gaming.Input\r\n\r\n  "
          ICON_FA_PLAYSTATION "  "
          "Your PlayStation controller may not work unless Xbox Mode is"
          " enabled\r\n\r\n\t(Input Management | PlayStation > Xbox Mode)"
          " and restart the game.",
          "Windows.Gaming.Input Incompatibility Detected",
                                      15000UL,
                                    SK_ImGui_Toast::UseDuration |
                                    SK_ImGui_Toast::ShowTitle   |
                                    SK_ImGui_Toast::ShowCaption |
                                    SK_ImGui_Toast::ShowOnce );
      }
    }

    if (iid == IID_IGamepad)
      SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepad)");

    if (iid == IID_IGamepad2)
      SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepad2)");

    if (iid == IID_IGamepadStatics)
      SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepadStatics)");

    if (iid == IID_IGamepadStatics2)
      SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepadStatics2)");

    if (iid == IID_IGamepadStatics)
    {
      ABI::Windows::Gaming::Input::IGamepadStatics *pGamepadStatsFactory;

      HRESULT hr =
        RoGetActivationFactory_Original ( activatableClassId,
                                            iid,
                                              (void **)&pGamepadStatsFactory );

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

        if (config.input.gamepad.xinput.emulate && SK_GetCurrentGameID () != SK_GAME_ID::ForzaHorizon5)
        {
          SK_RunOnce ({
            WGI_VIRTUAL_HOOK ( &pGamepadStatsFactory, 10,
                      "ABI::Windows::Gaming::Input::IGamepadStatics::get_Gamepads",
                       WGI_GamepadStatistics_get_Gamepads_Override,
                       WGI_GamepadStatistics_get_Gamepads_Original,
                       WGI_GamepadStatistics_get_Gamepads_pfn );

            SK_ApplyQueuedHooks ();
          });
        }

        IVectorView <ABI::Windows::Gaming::Input::Gamepad *>* pGamepads;

        if (SUCCEEDED (pGamepadStatsFactory->get_Gamepads (&pGamepads)) &&
                                                 nullptr != pGamepads)
        {
          uint32_t num_pads = 0;

          if (SUCCEEDED (pGamepads->get_Size (&num_pads)) && num_pads > 0)
          {
            ABI::Windows::Gaming::Input::IGamepad* pGamepad;

            if (SUCCEEDED (pGamepads->GetAt (0, &pGamepad)) &&
                                      nullptr != pGamepad)
            {
              // 0 QueryInterface
              // 1 AddRef
              // 2 Release
              // 3 GetIids
              // 4 GetRuntimeClassName
              // 5 GetTrustLevel
              // 6 get_Vibration
              // 7 put_Vibration
              // 8 GetCurrentReading

              SK_RunOnce ({
                WGI_VIRTUAL_HOOK ( &pGamepad, 8,
                          "ABI::Windows::Gaming::Input::IGamepad::GetCurrentReading",
                           WGI_Gamepad_GetCurrentReading_Override,
                           WGI_Gamepad_GetCurrentReading_Original,
                           WGI_Gamepad_GetCurrentReading_pfn );

/*
              WGI_VIRTUAL_HOOK ( &pGamepad, 6,
                          "ABI::Windows::Gaming::Input::IGamepad::get_Vibration",
                           WGI_Gamepad_get_Vibration_Override,
                           WGI_Gamepad_get_Vibration_Original,
                           WGI_Gamepad_get_Vibration_pfn );

              WGI_VIRTUAL_HOOK ( &pGamepad, 7,
                          "ABI::Windows::Gaming::Input::IGamepad::put_Vibration",
                           WGI_Gamepad_put_Vibration_Override,
                           WGI_Gamepad_put_Vibration_Original,
                           WGI_Gamepad_put_Vibration_pfn );
*/

                SK_ApplyQueuedHooks ();
              });

              pGamepad->Release ();
            }
          }

          pGamepads->Release ();
        }

        pGamepadStatsFactory->Release ();
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
  // This has other applications, but for now the only thing we need it
  // for is to observe the creation of Windows.Gaming.Input factories...
  SK_CreateDLLHook2 (      L"Combase.dll",
                            "RoGetActivationFactory",
                             RoGetActivationFactory_Detour,
    static_cast_p2p <void> (&RoGetActivationFactory_Original) );
}