#pragma once

#include <SpecialK/steam_api.h>
#include <SpecialK/core.h>
#include <SpecialK/config.h>
#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>
#include <imgui/imgui.h>

#include <concurrent_unordered_map.h>

extern sk_input_api_context_s SK_Steam_Backend;

#define SK_Steam_READ(type)  SK_Steam_Backend.markRead  (type);
#define SK_Steam_WRITE(type) SK_Steam_Backend.markWrite (type);

extern XINPUT_STATE* steam_to_xi;

void
SKX_Steam_PollGamepad (void)
{
  static DWORD dwPacket = 0;
  static ULONG last_frame =
    SK_GetFramesDrawn ();
         ULONG num_frames =
    SK_GetFramesDrawn ();

  if (last_frame != num_frames)
  {
    last_frame = num_frames;
    ++dwPacket;

    // Don't create the remapping proxy until the game does this multiple times
    if (steam_to_xi == nullptr)
      steam_to_xi = new XINPUT_STATE { };
  }

  if (steam_to_xi != nullptr)
    steam_to_xi->dwPacketNumber = dwPacket;
}


using ISteamController_Init_pfn     =
  bool (S_CALLTYPE *)(ISteamController *This);

using ISteamController_RunFrame_pfn =
  void (S_CALLTYPE *)(ISteamController *This);

using ISteamController_GetAnalogActionData_pfn =
  ControllerAnalogActionData_t (S_CALLTYPE *)( ISteamController               *This,
                                               ControllerHandle_t              controllerHandle,
                                               ControllerAnalogActionHandle_t  analogActionHandle );

using ISteamController_GetDigitalActionData_pfn =
  ControllerDigitalActionData_t (S_CALLTYPE *)( ISteamController                *This,
                                                ControllerHandle_t               controllerHandle,
                                                ControllerDigitalActionHandle_t  digitalActionHandle );

ISteamController_Init_pfn                 ISteamController_Init_Original                 = nullptr;
ISteamController_RunFrame_pfn             ISteamController_RunFrame_Original             = nullptr;
ISteamController_GetAnalogActionData_pfn  ISteamController_GetAnalogActionData_Original  = nullptr;
ISteamController_GetDigitalActionData_pfn ISteamController_GetDigitalActionData_Original = nullptr;



ISteamController*
S_CALLTYPE
SteamAPI_ISteamClient_GetISteamController_Detour ( ISteamClient *This,
                                                   HSteamUser    hSteamUser,
                                                   HSteamPipe    hSteamPipe,
                                                   const char   *pchVersion );
using SteamAPI_ISteamClient_GetISteamController_pfn = ISteamController* (S_CALLTYPE *)(
  ISteamClient *This,
  HSteamUser    hSteamUser,
  HSteamPipe    hSteamPipe,
  const char   *pchVersion
  );
extern SteamAPI_ISteamClient_GetISteamController_pfn SteamAPI_ISteamClient_GetISteamController_Original;


bool
S_CALLTYPE
ISteamController_Init_Detour (ISteamController *This)
{
  bool bRet =
    ISteamController_Init_Original (This);

  if (bRet != false)
  {
    SKX_Steam_PollGamepad ();
  }

  return bRet;
}

void
S_CALLTYPE
ISteamController_RunFrame_Detour (ISteamController *This)
{
  SKX_Steam_PollGamepad ();

  return ISteamController_RunFrame_Original (This);
}

ControllerAnalogActionData_t
S_CALLTYPE
ISteamController_GetAnalogActionData_Detour ( ISteamController               *This,
                                              ControllerHandle_t              controllerHandle,
                                              ControllerAnalogActionHandle_t  analogActionHandle )
{
  SK_Steam_READ (sk_input_dev_type::Gamepad);

  static std::unordered_map < ControllerHandle_t,
                                std::unordered_map < ControllerAnalogActionHandle_t,
                                                       ControllerAnalogActionData_t >
                            > last_data;


  SKX_Steam_PollGamepad ();


  ControllerAnalogActionData_t data =
    ISteamController_GetAnalogActionData_Original ( This,
                                                      controllerHandle,
                                                        analogActionHandle );

  unsigned int slot = (unsigned int)
    This->GetGamepadIndexForController (controllerHandle);

  if ( slot != config.input.gamepad.xinput.ui_slot ||
           (! SK_ImGui_WantGamepadCapture ())         )
  {
    last_data [controllerHandle][analogActionHandle] = data;
  }

  if ( steam_to_xi != nullptr &&
        slot       == config.input.gamepad.xinput.ui_slot )
  {
    EControllerActionOrigin origins [ STEAM_CONTROLLER_MAX_ORIGINS ] = { };
    int                     count =
      This->GetAnalogActionOrigins (  controllerHandle,
           This->GetCurrentActionSet (controllerHandle),
                                    analogActionHandle, origins);

    auto ComputeAxialPos_XInput =
      [ ] (UINT min, UINT max, DWORD pos) ->
      SHORT
    {
      float range  = ( static_cast <float> ( max ) - static_cast <float> ( min ) );
      float center = ( static_cast <float> ( max ) + static_cast <float> ( min ) ) / 2.0f;
      float rpos = 0.5f;

      if (static_cast <float> ( pos ) < center)
        rpos = center - ( center - static_cast <float> ( pos ) );
      else
        rpos = static_cast <float> ( pos ) - static_cast <float> ( min );

      float max_xi    = static_cast <float> ( std::numeric_limits <unsigned short>::max ( ) );
      float center_xi = static_cast <float> ( std::numeric_limits <unsigned short>::max ( ) / 2 );

      return
        static_cast <SHORT> ( max_xi * ( rpos / range ) - center_xi );
    };

    for (int i = 0; i < count; i++)
    {
      switch (origins [i])
      {
        case k_EControllerActionOrigin_LeftStick_Move:
        case k_EControllerActionOrigin_PS4_LeftStick_Move:
        case k_EControllerActionOrigin_XBoxOne_LeftStick_Move:
        case k_EControllerActionOrigin_XBox360_LeftStick_Move:
        case k_EControllerActionOrigin_SteamV2_LeftStick_Move:
        {
          steam_to_xi->Gamepad.sThumbLX =
            ComputeAxialPos_XInput (0, UINT_MAX, static_cast <DWORD> (UINT_MAX * data.x * 0.5f + 0.5f) );
          steam_to_xi->Gamepad.sThumbLY =
            ComputeAxialPos_XInput (0, UINT_MAX, static_cast <DWORD> (UINT_MAX * data.y * 0.5f + 0.5f) );
        }
        break;

        case k_EControllerActionOrigin_RightPad_Swipe:
        case k_EControllerActionOrigin_PS4_RightStick_Move:
        case k_EControllerActionOrigin_XBoxOne_RightStick_Move:
        case k_EControllerActionOrigin_XBox360_RightStick_Move:
        case k_EControllerActionOrigin_SteamV2_RightPad_Swipe:
        {
          steam_to_xi->Gamepad.sThumbRX =
            ComputeAxialPos_XInput (0, UINT_MAX, static_cast <DWORD> (UINT_MAX * data.x * 0.5f + 0.5f) );
          steam_to_xi->Gamepad.sThumbRY =
            ComputeAxialPos_XInput (0, UINT_MAX, static_cast <DWORD> (UINT_MAX * data.y * 0.5f + 0.5f) );
        }
        break;

        //case k_EControllerActionOrigin_LeftTrigger_Pull:
        //case k_EControllerActionOrigin_PS4_LeftTrigger_Pull:
        //case k_EControllerActionOrigin_XBoxOne_LeftTrigger_Pull:
        //case k_EControllerActionOrigin_XBox360_LeftTrigger_Pull:
        //case k_EControllerActionOrigin_SteamV2_LeftTrigger_Pull:
        //{
        //}
        //
        //case k_EControllerActionOrigin_RightTrigger_Pull:
        //case k_EControllerActionOrigin_PS4_RightTrigger_Pull:
        //case k_EControllerActionOrigin_XBoxOne_RightTrigger_Pull:
        //case k_EControllerActionOrigin_XBox360_RightTrigger_Pull:
        //case k_EControllerActionOrigin_SteamV2_RightTrigger_Pull:
        //{
        //}
        //break;
      }
    }
  }


  return slot == config.input.gamepad.xinput.ui_slot ?
           last_data [controllerHandle][analogActionHandle] :
                data;
}

ControllerDigitalActionData_t
S_CALLTYPE
ISteamController_GetDigitalActionData_Detour ( ISteamController                *This,
                                               ControllerHandle_t               controllerHandle,
                                               ControllerDigitalActionHandle_t  digitalActionHandle )
{
  SK_Steam_READ (sk_input_dev_type::Gamepad);

  static std::unordered_map < ControllerHandle_t,
                                std::unordered_map < ControllerDigitalActionHandle_t,
                                                       ControllerDigitalActionData_t >
                            > last_data;


  SKX_Steam_PollGamepad ();


  ControllerDigitalActionData_t data =
    ISteamController_GetDigitalActionData_Original ( This,
                                                       controllerHandle,
                                                         digitalActionHandle );


  unsigned int slot = (unsigned int)
    This->GetGamepadIndexForController (controllerHandle);

  if ( slot != config.input.gamepad.xinput.ui_slot ||
           (! SK_ImGui_WantGamepadCapture ())         )
  {
    last_data [controllerHandle][digitalActionHandle] = data;
  }

  if ( steam_to_xi != nullptr &&
        slot       == config.input.gamepad.xinput.ui_slot )
  {
    EControllerActionOrigin origins [ STEAM_CONTROLLER_MAX_ORIGINS ] = { };
    int                     count =
      This->GetDigitalActionOrigins ( controllerHandle,
           This->GetCurrentActionSet (controllerHandle),
                                   digitalActionHandle, origins);

    for (int i = 0; i < count; i++)
    {
      switch (origins [i])
      {
        case k_EControllerActionOrigin_Start:
        case k_EControllerActionOrigin_PS4_Options:
        case k_EControllerActionOrigin_XBoxOne_Menu:
        case k_EControllerActionOrigin_XBox360_Start:
        case k_EControllerActionOrigin_SteamV2_Start:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_START;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_START;
        }
        break;

        case k_EControllerActionOrigin_Back:
        case k_EControllerActionOrigin_PS4_Share:
        case k_EControllerActionOrigin_XBoxOne_View:
        case k_EControllerActionOrigin_XBox360_Back:
        case k_EControllerActionOrigin_SteamV2_Back:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_BACK;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_BACK;
        }
        break;

        case k_EControllerActionOrigin_A:
        case k_EControllerActionOrigin_PS4_X:
        case k_EControllerActionOrigin_XBoxOne_A:
        case k_EControllerActionOrigin_XBox360_A:
        case k_EControllerActionOrigin_SteamV2_A:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_A;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_A;
        }
        break;

        case k_EControllerActionOrigin_B:
        case k_EControllerActionOrigin_PS4_Circle:
        case k_EControllerActionOrigin_XBoxOne_B:
        case k_EControllerActionOrigin_XBox360_B:
        case k_EControllerActionOrigin_SteamV2_B:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_B;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_B;
        }
        break;

        case k_EControllerActionOrigin_X:
        case k_EControllerActionOrigin_PS4_Square:
        case k_EControllerActionOrigin_XBoxOne_X:
        case k_EControllerActionOrigin_XBox360_X:
        case k_EControllerActionOrigin_SteamV2_X:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_X;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_X;
        }
        break;

        case k_EControllerActionOrigin_Y:
        case k_EControllerActionOrigin_PS4_Triangle:
        case k_EControllerActionOrigin_XBoxOne_Y:
        case k_EControllerActionOrigin_XBox360_Y:
        case k_EControllerActionOrigin_SteamV2_Y:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_Y;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_Y;
        }
        break;

        case k_EControllerActionOrigin_LeftTrigger_Click:
        case k_EControllerActionOrigin_PS4_LeftTrigger_Click:
        case k_EControllerActionOrigin_XBoxOne_LeftTrigger_Click:
        case k_EControllerActionOrigin_XBox360_LeftTrigger_Click:
        case k_EControllerActionOrigin_SteamV2_LeftTrigger_Click:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_LEFT_TRIGGER;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_LEFT_TRIGGER;
        }
        break;

        case k_EControllerActionOrigin_RightTrigger_Click:
        case k_EControllerActionOrigin_PS4_RightTrigger_Click:
        case k_EControllerActionOrigin_XBoxOne_RightTrigger_Click:
        case k_EControllerActionOrigin_XBox360_RightTrigger_Click:
        case k_EControllerActionOrigin_SteamV2_RightTrigger_Click:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_RIGHT_TRIGGER;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_TRIGGER;
        }
        break;

        case k_EControllerActionOrigin_LeftBumper:
        case k_EControllerActionOrigin_PS4_LeftBumper:
        case k_EControllerActionOrigin_XBoxOne_LeftBumper:
        case k_EControllerActionOrigin_XBox360_LeftBumper:
        case k_EControllerActionOrigin_SteamV2_LeftBumper:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_LEFT_SHOULDER;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_LEFT_SHOULDER;
        }
        break;

        case k_EControllerActionOrigin_RightBumper:
        case k_EControllerActionOrigin_PS4_RightBumper:
        case k_EControllerActionOrigin_XBoxOne_RightBumper:
        case k_EControllerActionOrigin_XBox360_RightBumper:
        case k_EControllerActionOrigin_SteamV2_RightBumper:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_RIGHT_SHOULDER;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_SHOULDER;
        }
        break;

        case k_EControllerActionOrigin_LeftStick_Click:
        case k_EControllerActionOrigin_PS4_LeftStick_Click:
        case k_EControllerActionOrigin_XBoxOne_LeftStick_Click:
        case k_EControllerActionOrigin_XBox360_LeftStick_Click:
        case k_EControllerActionOrigin_SteamV2_LeftStick_Click:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_LEFT_THUMB;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_LEFT_THUMB;
        }
        break;

        case k_EControllerActionOrigin_RightPad_Click:
        case k_EControllerActionOrigin_PS4_RightStick_Click:
        case k_EControllerActionOrigin_XBoxOne_RightStick_Click:
        case k_EControllerActionOrigin_XBox360_RightStick_Click:
        case k_EControllerActionOrigin_SteamV2_RightPad_Click:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_RIGHT_THUMB;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_RIGHT_THUMB;
        }
        break;

        case k_EControllerActionOrigin_LeftPad_DPadNorth:
        case k_EControllerActionOrigin_PS4_DPad_North:
        case k_EControllerActionOrigin_XBoxOne_DPad_North:
        case k_EControllerActionOrigin_XBox360_DPad_North:
        case k_EControllerActionOrigin_SteamV2_LeftPad_DPadNorth:

        case k_EControllerActionOrigin_LeftStick_DPadNorth:
        case k_EControllerActionOrigin_PS4_LeftStick_DPadNorth:
        case k_EControllerActionOrigin_XBoxOne_LeftStick_DPadNorth:
        case k_EControllerActionOrigin_XBox360_LeftStick_DPadNorth:
        case k_EControllerActionOrigin_SteamV2_LeftStick_DPadNorth:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_DPAD_UP;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_UP;
        }
        break;

        case k_EControllerActionOrigin_LeftPad_DPadSouth:
        case k_EControllerActionOrigin_PS4_DPad_South:
        case k_EControllerActionOrigin_XBoxOne_DPad_South:
        case k_EControllerActionOrigin_XBox360_DPad_South:
        case k_EControllerActionOrigin_SteamV2_LeftPad_DPadSouth:

        case k_EControllerActionOrigin_LeftStick_DPadSouth:
        case k_EControllerActionOrigin_PS4_LeftStick_DPadSouth:
        case k_EControllerActionOrigin_XBoxOne_LeftStick_DPadSouth:
        case k_EControllerActionOrigin_XBox360_LeftStick_DPadSouth:
        case k_EControllerActionOrigin_SteamV2_LeftStick_DPadSouth:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_DPAD_DOWN;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_DOWN;
        }
        break;

        case k_EControllerActionOrigin_LeftPad_DPadEast:
        case k_EControllerActionOrigin_PS4_DPad_East:
        case k_EControllerActionOrigin_XBoxOne_DPad_East:
        case k_EControllerActionOrigin_XBox360_DPad_East:
        case k_EControllerActionOrigin_SteamV2_LeftPad_DPadEast:

        case k_EControllerActionOrigin_LeftStick_DPadEast:
        case k_EControllerActionOrigin_PS4_LeftStick_DPadEast:
        case k_EControllerActionOrigin_XBoxOne_LeftStick_DPadEast:
        case k_EControllerActionOrigin_XBox360_LeftStick_DPadEast:
        case k_EControllerActionOrigin_SteamV2_LeftStick_DPadEast:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_DPAD_RIGHT;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_RIGHT;
        }
        break;

        case k_EControllerActionOrigin_LeftPad_DPadWest:
        case k_EControllerActionOrigin_PS4_DPad_West:
        case k_EControllerActionOrigin_XBoxOne_DPad_West:
        case k_EControllerActionOrigin_XBox360_DPad_West:
        case k_EControllerActionOrigin_SteamV2_LeftPad_DPadWest:

        case k_EControllerActionOrigin_LeftStick_DPadWest:
        case k_EControllerActionOrigin_PS4_LeftStick_DPadWest:
        case k_EControllerActionOrigin_XBoxOne_LeftStick_DPadWest:
        case k_EControllerActionOrigin_XBox360_LeftStick_DPadWest:
        case k_EControllerActionOrigin_SteamV2_LeftStick_DPadWest:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_DPAD_LEFT;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_LEFT;
        }
        break;
      }
    }
  }


  return slot == config.input.gamepad.xinput.ui_slot ?
           last_data [controllerHandle][digitalActionHandle] :
                data;
}





class IWrapSteamController : public ISteamController
{
public:
  IWrapSteamController (ISteamController* pController) :
                                                         pRealController (pController) {
  };

  virtual bool Init (void) override
  {
    return ISteamController_Init_Detour (pRealController);
  } // 0
  virtual bool Shutdown (void) override
  {
    return pRealController->Shutdown ();
  } // 1
  virtual void RunFrame (void) override
  {
    ISteamController_RunFrame_Detour (pRealController);
  } // 2

// Enumerate currently connected controllers
// handlesOut should point to a STEAM_CONTROLLER_MAX_COUNT sized array of ControllerHandle_t handles
// Returns the number of handles written to handlesOut
  virtual int GetConnectedControllers (ControllerHandle_t *handlesOut) override
  {
    return pRealController->GetConnectedControllers (handlesOut);
  } // 3

  // Invokes the Steam overlay and brings up the binding screen
  // Returns false is overlay is disabled / unavailable, or the user is not in Big Picture mode
  virtual bool ShowBindingPanel (ControllerHandle_t controllerHandle) override
  {
    return pRealController->ShowBindingPanel (controllerHandle);
  } // 4

  // ACTION SETS
  // Lookup the handle for an Action Set. Best to do this once on startup, and store the handles for all future API calls.
  virtual ControllerActionSetHandle_t GetActionSetHandle (const char *pszActionSetName) override
  {
    return pRealController->GetActionSetHandle (pszActionSetName);
  } // 5

  // Reconfigure the controller to use the specified action set (ie 'Menu', 'Walk' or 'Drive')
  // This is cheap, and can be safely called repeatedly. It's often easier to repeatedly call it in
  // your state loops, instead of trying to place it in all of your state transitions.
  virtual void ActivateActionSet (ControllerHandle_t controllerHandle, ControllerActionSetHandle_t actionSetHandle) override
  {
    SKX_Steam_PollGamepad ();

    return pRealController->ActivateActionSet (controllerHandle, actionSetHandle);
  } // 6
  virtual ControllerActionSetHandle_t GetCurrentActionSet (ControllerHandle_t controllerHandle) override
  {
    return pRealController->GetCurrentActionSet (controllerHandle);
  } // 7

  // ACTIONS
  // Lookup the handle for a digital action. Best to do this once on startup, and store the handles for all future API calls.
  virtual ControllerDigitalActionHandle_t GetDigitalActionHandle (const char *pszActionName) override
  {
    return pRealController->GetDigitalActionHandle (pszActionName);
  } // 8

  // Returns the current state of the supplied digital game action
  virtual ControllerDigitalActionData_t GetDigitalActionData (ControllerHandle_t controllerHandle, ControllerDigitalActionHandle_t digitalActionHandle) override
  {
    return ISteamController_GetDigitalActionData_Detour (pRealController, controllerHandle, digitalActionHandle);
  } // 9

  // Get the origin(s) for a digital action within an action set. Returns the number of origins supplied in originsOut. Use this to display the appropriate on-screen prompt for the action.
  // originsOut should point to a STEAM_CONTROLLER_MAX_ORIGINS sized array of EControllerActionOrigin handles
  virtual int GetDigitalActionOrigins (ControllerHandle_t controllerHandle, ControllerActionSetHandle_t actionSetHandle, ControllerDigitalActionHandle_t digitalActionHandle, EControllerActionOrigin *originsOut) override
  {
    return pRealController->GetDigitalActionOrigins (controllerHandle, actionSetHandle, digitalActionHandle, originsOut);
  } // 10

  // Lookup the handle for an analog action. Best to do this once on startup, and store the handles for all future API calls.
  virtual ControllerAnalogActionHandle_t GetAnalogActionHandle (const char *pszActionName) override
  {
    return pRealController->GetAnalogActionHandle (pszActionName);
  } // 11

  // Returns the current state of these supplied analog game action
  virtual ControllerAnalogActionData_t GetAnalogActionData (ControllerHandle_t controllerHandle, ControllerAnalogActionHandle_t analogActionHandle) override
  {
    return ISteamController_GetAnalogActionData_Detour (pRealController, controllerHandle, analogActionHandle);
  } // 12

  // Get the origin(s) for an analog action within an action set. Returns the number of origins supplied in originsOut. Use this to display the appropriate on-screen prompt for the action.
  // originsOut should point to a STEAM_CONTROLLER_MAX_ORIGINS sized array of EControllerActionOrigin handles
  virtual int GetAnalogActionOrigins (ControllerHandle_t controllerHandle, ControllerActionSetHandle_t actionSetHandle, ControllerAnalogActionHandle_t analogActionHandle, EControllerActionOrigin *originsOut) override
  {
    return pRealController->GetAnalogActionOrigins (controllerHandle, actionSetHandle, analogActionHandle, originsOut);
  } // 13

  virtual void StopAnalogActionMomentum (ControllerHandle_t controllerHandle, ControllerAnalogActionHandle_t eAction) override
  {
    return pRealController->StopAnalogActionMomentum (controllerHandle, eAction);
  } // 14

  // Trigger a haptic pulse on a controller
  virtual void TriggerHapticPulse (ControllerHandle_t controllerHandle, ESteamControllerPad eTargetPad, unsigned short usDurationMicroSec) override
  {
    unsigned int slot = (unsigned int)
      GetGamepadIndexForController (controllerHandle);

    if ( slot != config.input.gamepad.xinput.ui_slot ||
             (! SK_ImGui_WantGamepadCapture ())         )
    {
      pRealController->TriggerHapticPulse (controllerHandle, eTargetPad, usDurationMicroSec);
    }
  } // 15

  // Trigger a pulse with a duty cycle of usDurationMicroSec / usOffMicroSec, unRepeat times.
  // nFlags is currently unused and reserved for future use.
  virtual void TriggerRepeatedHapticPulse (ControllerHandle_t controllerHandle, ESteamControllerPad eTargetPad, unsigned short usDurationMicroSec, unsigned short usOffMicroSec, unsigned short unRepeat, unsigned int nFlags) override
  {
    unsigned int slot = (unsigned int)
      GetGamepadIndexForController (controllerHandle);

    if ( slot != config.input.gamepad.xinput.ui_slot ||
             (! SK_ImGui_WantGamepadCapture ())         )
    {
      pRealController->TriggerRepeatedHapticPulse (controllerHandle, eTargetPad, usDurationMicroSec, usOffMicroSec, unRepeat, nFlags);
    }
  } // 16

  // Tigger a vibration event on supported controllers.  
  virtual void TriggerVibration (ControllerHandle_t controllerHandle, unsigned short usLeftSpeed, unsigned short usRightSpeed) override
  {
    unsigned int slot = (unsigned int)
      GetGamepadIndexForController (controllerHandle);

    if ( slot != config.input.gamepad.xinput.ui_slot ||
             (! SK_ImGui_WantGamepadCapture ())         )
    {
      pRealController->TriggerVibration (controllerHandle, usLeftSpeed, usRightSpeed);
    }
  } // 17

  // Set the controller LED color on supported controllers.  
  virtual void SetLEDColor (ControllerHandle_t controllerHandle, uint8 nColorR, uint8 nColorG, uint8 nColorB, unsigned int nFlags) override
  {
    pRealController->SetLEDColor (controllerHandle, nColorR, nColorG, nColorB, nFlags);
  } // 18

  // Returns the associated gamepad index for the specified controller, if emulating a gamepad
  virtual int GetGamepadIndexForController (ControllerHandle_t ulControllerHandle) override
  {
    return pRealController->GetGamepadIndexForController (ulControllerHandle);
  } // 19

  // Returns the associated controller handle for the specified emulated gamepad
  virtual ControllerHandle_t GetControllerForGamepadIndex (int nIndex) override
  {
    return pRealController->GetControllerForGamepadIndex (nIndex);
  } // 20

  // Returns raw motion data from the specified controller
  virtual ControllerMotionData_t GetMotionData (ControllerHandle_t controllerHandle) override
  {
    return pRealController->GetMotionData (controllerHandle);
  } // 21

  // Attempt to display origins of given action in the controller HUD, for the currently active action set
  // Returns false is overlay is disabled / unavailable, or the user is not in Big Picture mode
  virtual bool ShowDigitalActionOrigins (ControllerHandle_t controllerHandle, ControllerDigitalActionHandle_t digitalActionHandle, float flScale, float flXPosition, float flYPosition) override
  {
    return pRealController->ShowDigitalActionOrigins (controllerHandle, digitalActionHandle, flScale, flXPosition, flYPosition);
  } // 22
  virtual bool ShowAnalogActionOrigins (ControllerHandle_t controllerHandle, ControllerAnalogActionHandle_t analogActionHandle, float flScale, float flXPosition, float flYPosition) override
  {
    return pRealController->ShowAnalogActionOrigins (controllerHandle, analogActionHandle, flScale, flXPosition, flYPosition);
  } // 23

  // Returns a localized string (from Steam's language setting) for the specified origin
  virtual const char *GetStringForActionOrigin (EControllerActionOrigin eOrigin) override
  {
    return pRealController->GetStringForActionOrigin (eOrigin);
  } // 24

  // Get a local path to art for on-screen glyph for a particular origin 
  virtual const char *GetGlyphForActionOrigin (EControllerActionOrigin eOrigin) override
  {
    return pRealController->GetGlyphForActionOrigin (eOrigin);
  } // 25

private:
  ISteamController* pRealController;
};

#include <SpecialK/utility.h>

bool
SK_Steam_HookController (void)
{
  const wchar_t* wszSteamLib =
#ifdef _WIN64
    L"steam_api64.dll";
#else
    L"steam_api.dll";
#endif

  SK_CreateDLLHook2 (       wszSteamLib,
                   "SteamAPI_ISteamController_Init",
                             ISteamController_Init_Detour,
    static_cast_p2p <void> (&ISteamController_Init_Original) );

  SK_CreateDLLHook2 (       wszSteamLib,
                   "SteamAPI_ISteamController_RunFrame",
                             ISteamController_RunFrame_Detour,
    static_cast_p2p <void> (&ISteamController_RunFrame_Original) );

  SK_CreateDLLHook2 (       wszSteamLib,
                   "SteamAPI_ISteamController_GetAnalogActionData",
                             ISteamController_GetAnalogActionData_Detour,
    static_cast_p2p <void> (&ISteamController_GetAnalogActionData_Original) );

  SK_CreateDLLHook2 (       wszSteamLib,
                   "SteamAPI_ISteamController_GetDigitalActionData",
                             ISteamController_GetDigitalActionData_Detour,
    static_cast_p2p <void> (&ISteamController_GetDigitalActionData_Original) );

  SK_ApplyQueuedHooks ();

  return true;
}
