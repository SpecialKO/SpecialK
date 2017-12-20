#pragma once

#include <SpecialK/steam_api.h>
#include <SpecialK/core.h>
#include <SpecialK/config.h>
#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>
#include <imgui/imgui.h>
#include <SpecialK/utility.h>

#include <concurrent_unordered_map.h>

extern sk_input_api_context_s SK_Steam_Backend;

#define SK_Steam_READ(type)  SK_Steam_Backend.markRead  (type);
#define SK_Steam_WRITE(type) SK_Steam_Backend.markWrite (type);

typedef uint32_t ControllerIndex_t;

#define INVALID_CONTROLLER_INDEX  std::numeric_limits <ControllerIndex_t>::max () - 1
#define INVALID_CONTROLLER_HANDLE STEAM_CONTROLLER_HANDLE_ALL_CONTROLLERS         - 1

struct STEAMINPUT_STATE
{
         XINPUT_STATE*      to_xi          = nullptr;
         ControllerIndex_t  controller_idx = INVALID_CONTROLLER_INDEX;
         ControllerHandle_t handle         = INVALID_CONTROLLER_HANDLE;
         bool               connected      = false;

  static ISteamController*  pipe;
  static int                count;

         STEAMINPUT_STATE&  operator [] (ControllerIndex_t  idx);
       //STEAMINPUT_STATE&  operator [] (ControllerHandle_t handle);

  static ControllerIndex_t  getFirstActive (void);
         ControllerIndex_t  getNextActive  (void);
} extern steam_input;

ControllerIndex_t
ControllerIndex (ControllerHandle_t handle);

bool
ControllerPresent (ControllerIndex_t index);

void
SKX_Steam_PollGamepad (void);

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

extern ISteamController_Init_pfn                 ISteamController_Init_Original;
extern ISteamController_RunFrame_pfn             ISteamController_RunFrame_Original;
extern ISteamController_GetAnalogActionData_pfn  ISteamController_GetAnalogActionData_Original;
extern ISteamController_GetDigitalActionData_pfn ISteamController_GetDigitalActionData_Original;



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
ISteamController_Init_Detour (ISteamController *This);

void
S_CALLTYPE
ISteamController_RunFrame_Detour (ISteamController *This);

ControllerAnalogActionData_t
S_CALLTYPE
ISteamController_GetAnalogActionData_Detour ( ISteamController               *This,
                                              ControllerHandle_t              controllerHandle,
                                              ControllerAnalogActionHandle_t  analogActionHandle );

ControllerDigitalActionData_t
S_CALLTYPE
ISteamController_GetDigitalActionData_Detour ( ISteamController                *This,
                                               ControllerHandle_t               controllerHandle,
                                               ControllerDigitalActionHandle_t  digitalActionHandle );





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
    ControllerIndex_t slot =
      std::max (0ui32, ControllerIndex (controllerHandle));

    if ( slot != config.input.gamepad.steam.ui_slot ||
             (! SK_ImGui_WantGamepadCapture ())         )
    {
      if (! config.input.gamepad.disable_rumble)
      {
        pRealController->TriggerHapticPulse (controllerHandle, eTargetPad, usDurationMicroSec);
      }
    }
  } // 15

  // Trigger a pulse with a duty cycle of usDurationMicroSec / usOffMicroSec, unRepeat times.
  // nFlags is currently unused and reserved for future use.
  virtual void TriggerRepeatedHapticPulse (ControllerHandle_t controllerHandle, ESteamControllerPad eTargetPad, unsigned short usDurationMicroSec, unsigned short usOffMicroSec, unsigned short unRepeat, unsigned int nFlags) override
  {
    ControllerIndex_t slot =
      std::max (0ui32, ControllerIndex (controllerHandle));

    if ( slot != config.input.gamepad.steam.ui_slot ||
             (! SK_ImGui_WantGamepadCapture ())         )
    {
      if (! config.input.gamepad.disable_rumble)
      {
        pRealController->TriggerRepeatedHapticPulse (controllerHandle, eTargetPad, usDurationMicroSec, usOffMicroSec, unRepeat, nFlags);
      }
    }
  } // 16

  // Trigger a vibration event on supported controllers.  
  virtual void TriggerVibration (ControllerHandle_t controllerHandle, unsigned short usLeftSpeed, unsigned short usRightSpeed) override
  {
    ControllerIndex_t slot =
      std::max (0ui32, ControllerIndex (controllerHandle));

    if ( slot != config.input.gamepad.steam.ui_slot ||
             (! SK_ImGui_WantGamepadCapture ())         )
    {
      if (! config.input.gamepad.disable_rumble)
      {
        pRealController->TriggerVibration (controllerHandle, usLeftSpeed, usRightSpeed);
      }
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

bool
SK_Steam_HookController (void);