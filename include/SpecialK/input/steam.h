#pragma once

#include <SpecialK/steam_api.h>
#include <SpecialK/core.h>
#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>
#include <imgui/imgui.h>

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

  ///steam_to_xi->dwPacketNumber = dwPacket;
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


  if (! SK_ImGui_WantGamepadCapture ())
  {
    last_data [controllerHandle][analogActionHandle] =
      ISteamController_GetAnalogActionData_Original ( This,
                                                        controllerHandle,
                                                          analogActionHandle );
  }

  return last_data [controllerHandle][analogActionHandle];
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


  if (! SK_ImGui_WantGamepadCapture ())
  {
    last_data [controllerHandle][digitalActionHandle] =
      ISteamController_GetDigitalActionData_Original ( This,
                                                         controllerHandle,
                                                           digitalActionHandle );
  }

  return last_data [controllerHandle][digitalActionHandle];
}





class ISteamControllerFake : public ISteamController
{
public:
  ISteamControllerFake (ISteamController* pController) :
                                                         pRealController (pController) {
  };

  virtual bool Init (void) override
  {
    return ISteamController_Init_Detour (pRealController);
  }
  virtual bool Shutdown (void) override
  {
    return pRealController->Shutdown ();
  }
  virtual void RunFrame (void) override
  {
    ISteamController_RunFrame_Detour (pRealController);
  }

// Enumerate currently connected controllers
// handlesOut should point to a STEAM_CONTROLLER_MAX_COUNT sized array of ControllerHandle_t handles
// Returns the number of handles written to handlesOut
  virtual int GetConnectedControllers (ControllerHandle_t *handlesOut) override
  {
    return pRealController->GetConnectedControllers (handlesOut);
  }

  // Invokes the Steam overlay and brings up the binding screen
  // Returns false is overlay is disabled / unavailable, or the user is not in Big Picture mode
  virtual bool ShowBindingPanel (ControllerHandle_t controllerHandle) override
  {
    return pRealController->ShowBindingPanel (controllerHandle);
  }

  // ACTION SETS
  // Lookup the handle for an Action Set. Best to do this once on startup, and store the handles for all future API calls.
  virtual ControllerActionSetHandle_t GetActionSetHandle (const char *pszActionSetName) override
  {
    return pRealController->GetActionSetHandle (pszActionSetName);
  }

  // Reconfigure the controller to use the specified action set (ie 'Menu', 'Walk' or 'Drive')
  // This is cheap, and can be safely called repeatedly. It's often easier to repeatedly call it in
  // your state loops, instead of trying to place it in all of your state transitions.
  virtual void ActivateActionSet (ControllerHandle_t controllerHandle, ControllerActionSetHandle_t actionSetHandle) override
  {
    SKX_Steam_PollGamepad ();

    return pRealController->ActivateActionSet (controllerHandle, actionSetHandle);
  }
  virtual ControllerActionSetHandle_t GetCurrentActionSet (ControllerHandle_t controllerHandle) override
  {
    return pRealController->GetCurrentActionSet (controllerHandle);
  }

  // ACTIONS
  // Lookup the handle for a digital action. Best to do this once on startup, and store the handles for all future API calls.
  virtual ControllerDigitalActionHandle_t GetDigitalActionHandle (const char *pszActionName) override
  {
    return pRealController->GetDigitalActionHandle (pszActionName);
  }

  // Returns the current state of the supplied digital game action
  virtual ControllerDigitalActionData_t GetDigitalActionData (ControllerHandle_t controllerHandle, ControllerDigitalActionHandle_t digitalActionHandle) override
  {
    return ISteamController_GetDigitalActionData_Detour (pRealController, controllerHandle, digitalActionHandle);
  }

  // Get the origin(s) for a digital action within an action set. Returns the number of origins supplied in originsOut. Use this to display the appropriate on-screen prompt for the action.
  // originsOut should point to a STEAM_CONTROLLER_MAX_ORIGINS sized array of EControllerActionOrigin handles
  virtual int GetDigitalActionOrigins (ControllerHandle_t controllerHandle, ControllerActionSetHandle_t actionSetHandle, ControllerDigitalActionHandle_t digitalActionHandle, EControllerActionOrigin *originsOut) override
  {
    return pRealController->GetDigitalActionOrigins (controllerHandle, actionSetHandle, digitalActionHandle, originsOut);
  }

  // Lookup the handle for an analog action. Best to do this once on startup, and store the handles for all future API calls.
  virtual ControllerAnalogActionHandle_t GetAnalogActionHandle (const char *pszActionName) override
  {
    return pRealController->GetAnalogActionHandle (pszActionName);
  }

  // Returns the current state of these supplied analog game action
  virtual ControllerAnalogActionData_t GetAnalogActionData (ControllerHandle_t controllerHandle, ControllerAnalogActionHandle_t analogActionHandle) override
  {
    return ISteamController_GetAnalogActionData_Detour (pRealController, controllerHandle, analogActionHandle);
  }

  // Get the origin(s) for an analog action within an action set. Returns the number of origins supplied in originsOut. Use this to display the appropriate on-screen prompt for the action.
  // originsOut should point to a STEAM_CONTROLLER_MAX_ORIGINS sized array of EControllerActionOrigin handles
  virtual int GetAnalogActionOrigins (ControllerHandle_t controllerHandle, ControllerActionSetHandle_t actionSetHandle, ControllerAnalogActionHandle_t analogActionHandle, EControllerActionOrigin *originsOut) override
  {
    return pRealController->GetAnalogActionOrigins (controllerHandle, actionSetHandle, analogActionHandle, originsOut);
  }

  virtual void StopAnalogActionMomentum (ControllerHandle_t controllerHandle, ControllerAnalogActionHandle_t eAction) override
  {
    return pRealController->StopAnalogActionMomentum (controllerHandle, eAction);
  }

  // Trigger a haptic pulse on a controller
  virtual void TriggerHapticPulse (ControllerHandle_t controllerHandle, ESteamControllerPad eTargetPad, unsigned short usDurationMicroSec) override
  {
    if (! SK_ImGui_WantGamepadCapture ())
      pRealController->TriggerHapticPulse (controllerHandle, eTargetPad, usDurationMicroSec);
  }

  // Trigger a pulse with a duty cycle of usDurationMicroSec / usOffMicroSec, unRepeat times.
  // nFlags is currently unused and reserved for future use.
  virtual void TriggerRepeatedHapticPulse (ControllerHandle_t controllerHandle, ESteamControllerPad eTargetPad, unsigned short usDurationMicroSec, unsigned short usOffMicroSec, unsigned short unRepeat, unsigned int nFlags) override
  {
    if (! SK_ImGui_WantGamepadCapture ())
      pRealController->TriggerRepeatedHapticPulse (controllerHandle, eTargetPad, usDurationMicroSec, usOffMicroSec, unRepeat, nFlags);
  }

  // Tigger a vibration event on supported controllers.  
  virtual void TriggerVibration (ControllerHandle_t controllerHandle, unsigned short usLeftSpeed, unsigned short usRightSpeed) override
  {
    if (! SK_ImGui_WantGamepadCapture ())
      pRealController->TriggerVibration (controllerHandle, usLeftSpeed, usRightSpeed);
  }

  // Set the controller LED color on supported controllers.  
  virtual void SetLEDColor (ControllerHandle_t controllerHandle, uint8 nColorR, uint8 nColorG, uint8 nColorB, unsigned int nFlags) override
  {
    pRealController->SetLEDColor (controllerHandle, nColorR, nColorG, nColorB, nFlags);
  }

  // Returns the associated gamepad index for the specified controller, if emulating a gamepad
  virtual int GetGamepadIndexForController (ControllerHandle_t ulControllerHandle) override
  {
    return pRealController->GetGamepadIndexForController (ulControllerHandle);
  }

  // Returns the associated controller handle for the specified emulated gamepad
  virtual ControllerHandle_t GetControllerForGamepadIndex (int nIndex) override
  {
    return pRealController->GetControllerForGamepadIndex (nIndex);
  }

  // Returns raw motion data from the specified controller
  virtual ControllerMotionData_t GetMotionData (ControllerHandle_t controllerHandle) override
  {
    return pRealController->GetMotionData (controllerHandle);
  }

  // Attempt to display origins of given action in the controller HUD, for the currently active action set
  // Returns false is overlay is disabled / unavailable, or the user is not in Big Picture mode
  virtual bool ShowDigitalActionOrigins (ControllerHandle_t controllerHandle, ControllerDigitalActionHandle_t digitalActionHandle, float flScale, float flXPosition, float flYPosition) override
  {
    return pRealController->ShowDigitalActionOrigins (controllerHandle, digitalActionHandle, flScale, flXPosition, flYPosition);
  }
  virtual bool ShowAnalogActionOrigins (ControllerHandle_t controllerHandle, ControllerAnalogActionHandle_t analogActionHandle, float flScale, float flXPosition, float flYPosition) override
  {
    return pRealController->ShowAnalogActionOrigins (controllerHandle, analogActionHandle, flScale, flXPosition, flYPosition);
  }

  // Returns a localized string (from Steam's language setting) for the specified origin
  virtual const char *GetStringForActionOrigin (EControllerActionOrigin eOrigin) override
  {
    return pRealController->GetStringForActionOrigin (eOrigin);
  }

  // Get a local path to art for on-screen glyph for a particular origin 
  virtual const char *GetGlyphForActionOrigin (EControllerActionOrigin eOrigin) override
  {
    return pRealController->GetGlyphForActionOrigin (eOrigin);
  }

private:
  ISteamController* pRealController;
};

#include <SpecialK/utility.h>

bool
SK_Steam_HookController (void)
{
  const wchar_t* wszSteamLib =
#ifdef _WIN32
    L"steam_api.dll";
#elif (defined _WIN64)
    L"steam_api64.dll";
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
