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

using WGI_Gamepad_GetCurrentReading_pfn =  HRESULT (STDMETHODCALLTYPE *)(ABI::Windows::Gaming::Input::IGamepad       *This,
                                                                         ABI::Windows::Gaming::Input::GamepadReading *value);
      WGI_Gamepad_GetCurrentReading_pfn
      WGI_Gamepad_GetCurrentReading_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
WGI_Gamepad_GetCurrentReading_Override (ABI::Windows::Gaming::Input::IGamepad       *This,
                                        ABI::Windows::Gaming::Input::GamepadReading *value)
{
  SK_WGI_VIEW (SK_WGI_Backend, 0);

  if (SK_ImGui_WantGamepadCapture ())
  {
    HRESULT hr =
      WGI_Gamepad_GetCurrentReading_Original (This, value);

    if (SUCCEEDED (hr))
    {
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

  else if (SK_WantBackgroundRender () && (! game_window.active) && config.input.gamepad.disabled_to_game == 0)
  {
    SK_WGI_READ (SK_WGI_Backend, sk_input_dev_type::Gamepad);

    HRESULT hr =
      WGI_Gamepad_GetCurrentReading_Original (This, value);

    if (SUCCEEDED (hr))
    {
      XINPUT_STATE                  xi_state = { };
      SK_XInput_PollController (0, &xi_state);

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

      value->LeftThumbstickX = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbLX / 32767);
      value->LeftThumbstickY = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbLY / 32767);

      value->RightThumbstickX = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbRX / 32767);
      value->RightThumbstickY = (double)fmaxf(-1, (float)xi_state.Gamepad.sThumbRY / 32767);

      value->LeftTrigger  = (double)xi_state.Gamepad.bLeftTrigger  / 255;
      value->RightTrigger = (double)xi_state.Gamepad.bRightTrigger / 255;
    }

    return hr;
  }

  else if (game_window.active)
    SK_WGI_READ (SK_WGI_Backend, sk_input_dev_type::Gamepad);

  return
    WGI_Gamepad_GetCurrentReading_Original (This, value);
}

HRESULT
WINAPI
RoGetActivationFactory_Detour ( _In_  HSTRING activatableClassId,
                                _In_  REFIID  iid,
                                _Out_ void**  factory )
{
  SK_LOG_FIRST_CALL

  if (iid == IID_IGamepad)
    SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepad)");

  if (iid == IID_IGamepad2)
    SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepad2)");

  if (iid == IID_IGamepadStatics)
  {
    SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepadStatics)");

    ABI::Windows::Gaming::Input::IGamepadStatics *pGamepadStatsFactory;

    HRESULT hr =
      RoGetActivationFactory_Original ( activatableClassId,
                                          iid,
                                            (void **)&pGamepadStatsFactory );

    if (SUCCEEDED (hr) && pGamepadStatsFactory != nullptr)
    {
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
            });

            pGamepad->Release ();
          }
        }

        pGamepads->Release ();
      }

      pGamepadStatsFactory->Release ();
    }
  }

  if (iid == IID_IGamepadStatics2)
    SK_LOGi0 (L"RoGetActivationFactory (IID_IGamepadStatics2)");

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