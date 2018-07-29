#include <SpecialK/steam_api.h>
#include <SpecialK/input/xinput.h>
#include <SpecialK/input/steam.h>

ISteamController_Init_pfn                 ISteamController_Init_Original                 = nullptr;
ISteamController_RunFrame_pfn             ISteamController_RunFrame_Original             = nullptr;
ISteamController_GetAnalogActionData_pfn  ISteamController_GetAnalogActionData_Original  = nullptr;
ISteamController_GetDigitalActionData_pfn ISteamController_GetDigitalActionData_Original = nullptr;

STEAMINPUT_STATE steam_input;

concurrency::concurrent_unordered_map <ControllerIndex_t,  STEAMINPUT_STATE*> steam_controllers;
concurrency::concurrent_unordered_map <ControllerHandle_t, ControllerIndex_t> steam_controllers_rev;

ISteamController*  STEAMINPUT_STATE::pipe  = nullptr;
int                STEAMINPUT_STATE::count = 0;

STEAMINPUT_STATE&
STEAMINPUT_STATE::operator [] (ControllerIndex_t idx)
{
  if (idx == INVALID_CONTROLLER_INDEX)
  {
    assert (false);

    static STEAMINPUT_STATE nul_controller = {
      new XINPUT_STATE { },
        INVALID_CONTROLLER_INDEX,
        INVALID_CONTROLLER_HANDLE,
      false
    };

    return nul_controller;
  }


  if (steam_controllers.count (idx))
    return *steam_controllers [idx];


  steam_controllers [idx] =
    new STEAMINPUT_STATE { new XINPUT_STATE { }, idx, INVALID_CONTROLLER_HANDLE, true };

  return *steam_controllers [idx];
}

ControllerIndex_t
ControllerIndex (ControllerHandle_t handle)
{
  if (handle == INVALID_CONTROLLER_HANDLE)
    return INVALID_CONTROLLER_INDEX;

  if (steam_controllers_rev.count (handle))
    return steam_controllers_rev [handle];

  for (auto& it : steam_controllers)
  {
    if (it.second->handle == handle)
    {
      steam_controllers_rev [handle] = it.first;
      return it.first;
    }
  }

  ControllerIndex_t new_idx =
    static_cast <ControllerIndex_t> (steam_controllers.size ());

  steam_controllers [new_idx] =
    new STEAMINPUT_STATE { new XINPUT_STATE { },
                           new_idx, handle, false };

  steam_controllers_rev [handle] = new_idx;

  return new_idx;
}

bool
ControllerPresent (ControllerIndex_t index)
{
  return (steam_controllers.count (index) && steam_input [index].connected);
}

ControllerIndex_t
STEAMINPUT_STATE::getFirstActive (void)
{
  ControllerIndex_t first =
    INVALID_CONTROLLER_INDEX;

  for (auto it : steam_controllers)
  {
    if ( it.second->connected              &&
         it.second->controller_idx < first    )
    {
      first = it.second->controller_idx;
    }
  }

  return first;
}

ControllerIndex_t
STEAMINPUT_STATE::getNextActive (void)
{
  ControllerIndex_t next =
    INVALID_CONTROLLER_INDEX;

  for (auto it : steam_controllers)
  {
    if ( it.second->connected                       &&
         it.second->controller_idx < next           &&
         it.second->controller_idx > controller_idx    )
    {
      next = it.second->controller_idx;
    }
  }

  return next;
}

void
SKX_Steam_PollGamepad (void)
{
  static DWORD dwPacket   = 0;
  static ULONG last_frame = (ULONG)-1;
         ULONG num_frames =
    SK_GetFramesDrawn ();

  if (steam_input.pipe != nullptr && ( last_frame != num_frames ))
  {
    ControllerHandle_t handles [STEAM_CONTROLLER_MAX_COUNT];

    steam_input.count =
      steam_input.pipe->GetConnectedControllers (handles);

    for (ControllerIndex_t i = 0; i < steam_controllers.size (); i++)
    {
      steam_input [i].connected = false;
    }


    for (int i = 0; i < steam_input.count; i++)
    {
      ControllerIndex_t  idx    =
        static_cast <ControllerIndex_t> (steam_controllers.size ());
      ControllerHandle_t handle =
        handles [i];

      if (steam_controllers_rev.count (handle))
      {
        idx =
          steam_controllers_rev [handle];
      }

      else
      {
        steam_controllers_rev [handles [i]] = idx;

        steam_input [idx].handle            = handles [i];
        steam_input [idx].controller_idx    = idx;
      }

      steam_input   [idx].connected             =  true;
      steam_input   [idx].to_xi->dwPacketNumber = (dwPacket + 1);
    }

    last_frame = num_frames;
    ++dwPacket;
  }
}

bool
S_CALLTYPE
ISteamController_Init_Detour (ISteamController *This)
{
  if (! ISteamController_Init_Original)
  {
    bool
    SK_Steam_HookController (void);
  
    if (! ( SK_Steam_HookController () && ISteamController_Init_Original != nullptr) )
    {
      return false;
    }
  }


  bool bRet =
    ISteamController_Init_Original (This);

  if (bRet != false)
  {
    steam_input.pipe = This;

    SKX_Steam_PollGamepad ();
  }

  return bRet;
}

void
S_CALLTYPE
ISteamController_RunFrame_Detour (ISteamController *This)
{
  steam_input.pipe = This;

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

  ControllerIndex_t slot =
    std::max (0ui32, ControllerIndex (controllerHandle));

  bool disable =
    ( config.input.gamepad.disabled_to_game           ||
      ( slot == config.input.gamepad.steam.ui_slot &&
          SK_ImGui_WantGamepadCapture () )               );

  if (! disable)
  {
    last_data [controllerHandle][analogActionHandle]   = data;
  }

  else
  {
    last_data [controllerHandle][analogActionHandle].x = 0.0f;
    last_data [controllerHandle][analogActionHandle].y = 0.0f;
  }

  if ( steam_input [slot].to_xi != nullptr &&
       slot                     == config.input.gamepad.steam.ui_slot )
  {
    XINPUT_STATE* steam_to_xi =
      steam_input [slot].to_xi;

    EControllerActionOrigin origins [ STEAM_CONTROLLER_MAX_ORIGINS ] = { };
    int                     count =
      This->GetAnalogActionOrigins (  controllerHandle,
           This->GetCurrentActionSet (controllerHandle),
                                    analogActionHandle, origins);

    auto ComputeAxialPos_XInput =
      [ ] (auto min, auto max, auto pos) ->
      SHORT
    {
      float max_xi = static_cast <float> ( std::numeric_limits <short>::max ( ) );

      return
        static_cast <SHORT> ( max_xi * std::min (max, std::max (min, pos)) );
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
            ComputeAxialPos_XInput (-1.0f, 1.0f, data.x);
          steam_to_xi->Gamepad.sThumbLY =
            ComputeAxialPos_XInput (-1.0f, 1.0f, data.y);
        }
        break;

        case k_EControllerActionOrigin_RightPad_Swipe:
        case k_EControllerActionOrigin_PS4_RightStick_Move:
        case k_EControllerActionOrigin_XBoxOne_RightStick_Move:
        case k_EControllerActionOrigin_XBox360_RightStick_Move:
        case k_EControllerActionOrigin_SteamV2_RightPad_Swipe:
        {
          steam_to_xi->Gamepad.sThumbRX =
            ComputeAxialPos_XInput (-1.0f, 1.0f, data.x);
          steam_to_xi->Gamepad.sThumbRY =
            ComputeAxialPos_XInput (-1.0f, 1.0f, data.y);
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


  return ( slot == config.input.gamepad.steam.ui_slot ||
                   config.input.gamepad.disabled_to_game ) ?
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

  ControllerIndex_t slot =
    std::max (0ui32, ControllerIndex (controllerHandle));

  bool disable =
    ( config.input.gamepad.disabled_to_game           ||
      ( slot == config.input.gamepad.steam.ui_slot &&
          SK_ImGui_WantGamepadCapture () )               );

  if (! disable)
  {
    last_data [controllerHandle][digitalActionHandle]        = data;
  }
  else
    last_data [controllerHandle][digitalActionHandle].bState = false;

  if ( steam_input [slot].to_xi != nullptr &&
       slot                     == config.input.gamepad.steam.ui_slot )
  {
    XINPUT_STATE* steam_to_xi =
      steam_input [slot].to_xi;

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

        //case k_EControllerActionOrigin_LeftStick_DPadNorth:
        //case k_EControllerActionOrigin_PS4_LeftStick_DPadNorth:
        //case k_EControllerActionOrigin_XBoxOne_LeftStick_DPadNorth:
        //case k_EControllerActionOrigin_XBox360_LeftStick_DPadNorth:
        //case k_EControllerActionOrigin_SteamV2_LeftStick_DPadNorth:
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

        //case k_EControllerActionOrigin_LeftStick_DPadSouth:
        //case k_EControllerActionOrigin_PS4_LeftStick_DPadSouth:
        //case k_EControllerActionOrigin_XBoxOne_LeftStick_DPadSouth:
        //case k_EControllerActionOrigin_XBox360_LeftStick_DPadSouth:
        //case k_EControllerActionOrigin_SteamV2_LeftStick_DPadSouth:
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

        //case k_EControllerActionOrigin_LeftStick_DPadEast:
        //case k_EControllerActionOrigin_PS4_LeftStick_DPadEast:
        //case k_EControllerActionOrigin_XBoxOne_LeftStick_DPadEast:
        //case k_EControllerActionOrigin_XBox360_LeftStick_DPadEast:
        //case k_EControllerActionOrigin_SteamV2_LeftStick_DPadEast:
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

        //case k_EControllerActionOrigin_LeftStick_DPadWest:
        //case k_EControllerActionOrigin_PS4_LeftStick_DPadWest:
        //case k_EControllerActionOrigin_XBoxOne_LeftStick_DPadWest:
        //case k_EControllerActionOrigin_XBox360_LeftStick_DPadWest:
        //case k_EControllerActionOrigin_SteamV2_LeftStick_DPadWest:
        {
          if (data.bState) steam_to_xi->Gamepad.wButtons |=  XINPUT_GAMEPAD_DPAD_LEFT;
          else             steam_to_xi->Gamepad.wButtons &= ~XINPUT_GAMEPAD_DPAD_LEFT;
        }
        break;
      }
    }
  }


  return ( slot == config.input.gamepad.steam.ui_slot ||
                   config.input.gamepad.disabled_to_game ) ?
           last_data [controllerHandle][digitalActionHandle] :
                data;
}



extern const wchar_t*
SK_Steam_GetDLLPath (void);

bool
SK_Steam_HookController (void)
{
  if (ISteamController_Init_Original != nullptr)
    return true;

  const wchar_t *steam_dll =
    SK_Steam_GetDLLPath ();

  if ( GetProcAddress (
         GetModuleHandleW (steam_dll),
         "SteamAPI_ISteamController_GetAnalogActionData"
       )
     )
  {
    SK_CreateDLLHook2 (       steam_dll,
                     "SteamAPI_ISteamController_Init",
                               ISteamController_Init_Detour,
      static_cast_p2p <void> (&ISteamController_Init_Original) );

    SK_CreateDLLHook2 (       steam_dll,
                     "SteamAPI_ISteamController_RunFrame",
                               ISteamController_RunFrame_Detour,
      static_cast_p2p <void> (&ISteamController_RunFrame_Original) );

    SK_CreateDLLHook2 (       steam_dll,
                     "SteamAPI_ISteamController_GetAnalogActionData",
                               ISteamController_GetAnalogActionData_Detour,
      static_cast_p2p <void> (&ISteamController_GetAnalogActionData_Original) );

    SK_CreateDLLHook2 (       steam_dll,
                     "SteamAPI_ISteamController_GetDigitalActionData",
                               ISteamController_GetDigitalActionData_Detour,
      static_cast_p2p <void> (&ISteamController_GetDigitalActionData_Original) );

    if (SK_GetFramesDrawn   () > 1)
        SK_ApplyQueuedHooks ();

    return true;
  }

  else
  {
    SK_LOG0 ( ( L"SteamAPI DLL ('%s') does not export SteamAPI_ISteamController_GetAnalogActionData; "
                L"disabling SK SteamInput support.", steam_dll ),
               L"SteamInput" );
  }

  return false;
}