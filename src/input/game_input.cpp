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
#include <SpecialK/input/game_input.h>

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
// GameInput.dll  (SK_GI)
//
///////////////////////////////////////////////////////

extern SK_LazyGlobal <sk_input_api_context_s> SK_GameInput_Backend;

#define SK_GAMEINPUT_READ(type)  SK_GameInput_Backend->markRead   (type);
#define SK_GAMEINPUT_WRITE(type) SK_GameInput_Backend->markWrite  (type);
#define SK_GAMEINPUT_VIEW(type)  SK_GameInput_Backend->markViewed (type);
#define SK_GAMEINPUT_HIDE(type)  SK_GameInput_Backend->markHidden (type);

#define GI_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {       \
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

using GameInputCreate_pfn = HRESULT (WINAPI *)(IGameInput**);
      GameInputCreate_pfn
      GameInputCreate_Original = nullptr;

HRESULT
__stdcall
SK_IWrapGameInput::QueryInterface (REFIID riid, void **ppvObject) noexcept
{
  if (ppvObject == nullptr)
  {
    return E_POINTER;
  }

  if ( riid == __uuidof (IUnknown)   ||
       riid == __uuidof (IGameInput) )
  {
    auto _GetVersion = [](REFIID riid) ->
    UINT
    {
      if (riid == __uuidof (IGameInput))  return 0;

      return 0;
    };

    UINT required_ver =
      _GetVersion (riid);

    if (ver_ < required_ver)
    {
      IUnknown* pPromoted = nullptr;

      if ( FAILED (
             pReal->QueryInterface ( riid,
                           (void **)&pPromoted )
                  ) || pPromoted == nullptr
         )
      {
        return E_NOINTERFACE;
      }

      ver_ =
        SK_COM_PromoteInterface (&pReal, pPromoted) ?
                                       required_ver : ver_;
    }

    else
    {
      AddRef ();
    }

    *ppvObject = this;

    return S_OK;
  }

  HRESULT hr =
    pReal->QueryInterface (riid, ppvObject);

  static
    concurrency::concurrent_unordered_set <std::wstring> reported_guids;

  wchar_t                wszGUID [41] = { };
  StringFromGUID2 (riid, wszGUID, 40);

  bool once =
    reported_guids.count (wszGUID) > 0;

  if (! once)
  {
    reported_guids.insert (wszGUID);

    SK_LOG0 ( ( L"QueryInterface on wrapped IGameInput for Mystery UUID: %s",
                    wszGUID ), L"Game Input" );
  }

  return hr;
};

ULONG
__stdcall
SK_IWrapGameInput::AddRef (void) noexcept
{
  InterlockedIncrement (&refs_);

  return
    pReal->AddRef ();
};

ULONG
__stdcall
SK_IWrapGameInput::Release (void) noexcept
{
  ULONG xrefs =
    InterlockedDecrement (&refs_),
         refs = pReal->Release ();

  if (xrefs == 0)
  {
    if (refs != 0)
    {
      SK_LOG0 ( ( L"Inconsistent reference count for IGameInput; expected=%d, actual=%d",
                  xrefs, refs ),
                  L"Game Input" );
    }

    else
      delete this;
  }

  return
    refs;
};

uint64_t
__stdcall
SK_IWrapGameInput::GetCurrentTimestamp (void) noexcept
{
  SK_LOG_FIRST_CALL

  return pReal->GetCurrentTimestamp ();
}

static concurrency::concurrent_unordered_map <IGameInputDevice*,concurrency::concurrent_unordered_map <GameInputKind,SK_ComPtr<IGameInputReading>>> _current_readings;

HRESULT
__stdcall
SK_IWrapGameInput::GetCurrentReading (_In_          GameInputKind      inputKind,
                                      _In_opt_     IGameInputDevice   *device,
                                      _COM_Outptr_ IGameInputReading **reading) noexcept
{
  SK_LOG_FIRST_CALL

  bool capture_input = false;

  switch (inputKind)
  {
    case GameInputKindKeyboard:
      if (SK_ImGui_WantKeyboardCapture ())
      {
        capture_input = true;
      }
      break;

    case GameInputKindMouse:
    case GameInputKindTouch:
      if (SK_ImGui_WantMouseCapture ())
      {
        capture_input = true;
      }
      break;

    case GameInputKindGamepad:
    case GameInputKindController:
    case GameInputKindControllerSwitch:
    case GameInputKindControllerButton:
    case GameInputKindControllerAxis:
    case GameInputKindRawDeviceReport:
    case GameInputKindUiNavigation:
      if (SK_ImGui_WantGamepadCapture ())
      {
        capture_input = true;
      }
      break;

    default:
      break;
  }

  if (! capture_input)
  {
    IGameInputReading* reading_;

    HRESULT hr =
      pReal->GetCurrentReading (inputKind, device, &reading_);

    if (reading != nullptr)
    {
      *reading = (IGameInputReading *)new SK_IWrapGameInputReading (reading_);
      _current_readings [device][inputKind] = *reading;
    }

    else
    {
      reading_->Release ();
    }
      

    return hr;
  }

  else
  {
    if (reading != nullptr) {
                  _current_readings [device][inputKind].p->AddRef ();
       *reading = _current_readings [device][inputKind];
     }

    return S_OK;
  }
}

HRESULT
__stdcall
SK_IWrapGameInput::GetNextReading (_In_         IGameInputReading  *referenceReading,
                                   _In_          GameInputKind      inputKind,
                                   _In_opt_     IGameInputDevice   *device,
                                   _COM_Outptr_ IGameInputReading **reading) noexcept
{
  SK_LOG_FIRST_CALL

  bool capture_input = false;

  switch (inputKind)
  {
    case GameInputKindKeyboard:
      SK_GAMEINPUT_READ (sk_input_dev_type::Keyboard);
      if (SK_ImGui_WantKeyboardCapture ())
      { SK_GAMEINPUT_HIDE (sk_input_dev_type::Keyboard);
        capture_input = true;
      } else
      { SK_GAMEINPUT_VIEW (sk_input_dev_type::Keyboard); }
      break;

    case GameInputKindMouse:
    case GameInputKindTouch:
      SK_GAMEINPUT_READ (sk_input_dev_type::Mouse);
      if (SK_ImGui_WantMouseCapture ())
      { SK_GAMEINPUT_HIDE (sk_input_dev_type::Mouse);
        capture_input = true;
      } else
      { SK_GAMEINPUT_VIEW (sk_input_dev_type::Mouse); }
      break;

    case GameInputKindGamepad:
    case GameInputKindController:
    case GameInputKindControllerSwitch:
    case GameInputKindControllerButton:
    case GameInputKindControllerAxis:
    case GameInputKindRawDeviceReport:
    case GameInputKindUiNavigation:
      SK_GAMEINPUT_READ (sk_input_dev_type::Gamepad);
      if (SK_ImGui_WantGamepadCapture ())
      { SK_GAMEINPUT_HIDE (sk_input_dev_type::Gamepad);
        capture_input = true;
      } else
      { SK_GAMEINPUT_VIEW (sk_input_dev_type::Gamepad); }

    default:
      break;
  }

  if (capture_input)
  {
    if (reading != nullptr)
       *reading = nullptr;
    return GAMEINPUT_E_READING_NOT_FOUND;
  }

  const GameInputDeviceInfo* info =
    device->GetDeviceInfo ();

  SK_RunOnce (
  {
    SK_LOGi0 (L"GameInputDevice: VID=%x",                   info->vendorId);
    SK_LOGi0 (L"GameInputDevice: PID=%x",                   info->productId);
    SK_LOGi0 (L"GameInputDevice: Revision=%d",              info->revisionNumber);
    SK_LOGi0 (L"GameInputDevice: Id=%x",                    info->deviceId);
    SK_LOGi0 (L"GameInputDevice: RootId=%x",                info->deviceRootId);
    SK_LOGi0 (L"GameInputDevice: DeviceFamily=%x",          info->deviceFamily);
    SK_LOGi0 (L"GameInputDevice: Usage={%x,%x}",            info->usage.id, info->usage.page);
    SK_LOGi0 (L"GameInputDevice: Interface=%d",             info->interfaceNumber);
    SK_LOGi0 (L"GameInputDevice: Collection=%d",            info->collectionNumber);
    SK_LOGi0 (L"GameInputDevice: HW Version="
                             "%01d.%01d.%01d.%01d",         info->hardwareVersion.major, info->hardwareVersion.minor,
                                                            info->hardwareVersion.build, info->hardwareVersion.revision);
    SK_LOGi0 (L"GameInputDevice: FW Version="
                             "%01d.%01d.%01d.%01d",         info->firmwareVersion.major, info->firmwareVersion.minor,
                                                            info->firmwareVersion.build, info->firmwareVersion.revision);
    SK_LOGi0 (L"GameInputDevice: Capabilities=%x",          info->capabilities);
    SK_LOGi0 (L"GameInputDevice: SupportedInput=%x",        info->supportedInput);
    SK_LOGi0 (L"GameInputDevice: SupportedRumbleMotors=%x", info->supportedRumbleMotors);
    SK_LOGi0 (L"GameInputDevice: SupportedSystemButtons=%x",info->supportedSystemButtons);
  });

  if (inputKind == GameInputKindGamepad)
  {
    IGameInputReading *reading_;

    HRESULT hr =
      pReal->GetNextReading (referenceReading, inputKind, device, &reading_);

    if (SUCCEEDED (hr))
    {
      if (reading != nullptr)
      {
        *reading = (IGameInputReading *)new SK_IWrapGameInputReading (reading_);
        _current_readings [device][inputKind] = *reading;
      }

      else
      {
        reading_->Release ();
      }

      return hr;
    }
  }

  return
    pReal->GetNextReading (referenceReading, inputKind, device, reading);
}

HRESULT
__stdcall
SK_IWrapGameInput::GetPreviousReading (_In_         IGameInputReading  *referenceReading,
                                       _In_          GameInputKind      inputKind,
                                       _In_opt_     IGameInputDevice   *device,
                                       _COM_Outptr_ IGameInputReading **reading) noexcept
{
  SK_LOG_FIRST_CALL

  bool capture_input = false;
  static concurrency::concurrent_unordered_map <IGameInputDevice*,concurrency::concurrent_unordered_map <GameInputKind,SK_ComPtr<IGameInputReading>>> readings;

  switch (inputKind)
  {
    case GameInputKindKeyboard:
      if (SK_ImGui_WantKeyboardCapture ())
      {
        capture_input = true;
      }
      break;

    case GameInputKindMouse:
    case GameInputKindTouch:
      if (SK_ImGui_WantMouseCapture ())
      {
        capture_input = true;
      }
      break;

    case GameInputKindGamepad:
    case GameInputKindController:
    case GameInputKindControllerSwitch:
    case GameInputKindControllerButton:
    case GameInputKindControllerAxis:
    case GameInputKindRawDeviceReport:
    case GameInputKindUiNavigation:
      if (SK_ImGui_WantGamepadCapture ())
      {
        capture_input = true;
      }
      break;

    default:
      break;
  }

  if (! capture_input)
  {
    HRESULT hr =
      pReal->GetPreviousReading (referenceReading, inputKind, device, reading);

    if (reading != nullptr)
      readings [device][inputKind] = *reading;

    return hr;
  }

  else
  {
    if (reading != nullptr) {
                  readings [device][inputKind].p->AddRef ();
       *reading = readings [device][inputKind];
    }

    return S_OK;
  }
}

HRESULT
__stdcall
SK_IWrapGameInput::GetTemporalReading (_In_         uint64_t            timestamp,
                                       _In_         IGameInputDevice   *device,
                                       _COM_Outptr_ IGameInputReading **reading) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetTemporalReading (timestamp, device, reading);
}

HRESULT
__stdcall
SK_IWrapGameInput::RegisterReadingCallback (_In_opt_                         IGameInputDevice          *device,
                                            _In_                              GameInputKind             inputKind,
                                            _In_                              float                     analogThreshold,
                                            _In_opt_                          void                     *context,
                                            _In_                              GameInputReadingCallback  callbackFunc,
                                            _Out_opt_ _Result_zeroonfailure_  GameInputCallbackToken   *callbackToken) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->RegisterReadingCallback (device, inputKind, analogThreshold, context, callbackFunc, callbackToken);
}

HRESULT
__stdcall
SK_IWrapGameInput::RegisterDeviceCallback (_In_opt_                        IGameInputDevice         *device,
                                           _In_                             GameInputKind            inputKind,
                                           _In_                             GameInputDeviceStatus    statusFilter,
                                           _In_                             GameInputEnumerationKind enumerationKind,
                                           _In_opt_                         void                    *context,
                                           _In_                             GameInputDeviceCallback  callbackFunc,
                                           _Out_opt_ _Result_zeroonfailure_ GameInputCallbackToken  *callbackToken) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->RegisterDeviceCallback (device, inputKind, statusFilter, enumerationKind, context, callbackFunc, callbackToken);
}

HRESULT
__stdcall
SK_IWrapGameInput::RegisterSystemButtonCallback (_In_opt_                         IGameInputDevice               *device,
                                                 _In_                              GameInputSystemButtons         buttonFilter,
                                                 _In_opt_                          void                          *context,
                                                 _In_                              GameInputSystemButtonCallback  callbackFunc,
                                                 _Out_opt_ _Result_zeroonfailure_  GameInputCallbackToken        *callbackToken) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->RegisterSystemButtonCallback (device, buttonFilter, context, callbackFunc, callbackToken);
}

HRESULT
__stdcall
SK_IWrapGameInput::RegisterKeyboardLayoutCallback (_In_opt_                         IGameInputDevice                 *device,
                                                   _In_opt_                          void                            *context,
                                                   _In_                              GameInputKeyboardLayoutCallback  callbackFunc,
                                                   _Out_opt_ _Result_zeroonfailure_  GameInputCallbackToken          *callbackToken) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->RegisterKeyboardLayoutCallback (device, context, callbackFunc, callbackToken);
}

void
__stdcall
SK_IWrapGameInput::StopCallback (_In_ GameInputCallbackToken callbackToken) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->StopCallback (callbackToken);
}

bool
__stdcall
SK_IWrapGameInput::UnregisterCallback (_In_ GameInputCallbackToken callbackToken,
                                       _In_ uint64_t               timeoutInMicroseconds) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->UnregisterCallback (callbackToken, timeoutInMicroseconds);
}

HRESULT
__stdcall
SK_IWrapGameInput::CreateDispatcher (_COM_Outptr_ IGameInputDispatcher **dispatcher) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->CreateDispatcher (dispatcher);
}

HRESULT
__stdcall
SK_IWrapGameInput::CreateAggregateDevice (_In_          GameInputKind     inputKind,
                                          _COM_Outptr_ IGameInputDevice **device) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->CreateAggregateDevice (inputKind, device);
}

HRESULT
__stdcall
SK_IWrapGameInput::FindDeviceFromId (_In_         APP_LOCAL_DEVICE_ID const  *value,
                                     _COM_Outptr_ IGameInputDevice          **device) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->FindDeviceFromId (value, device);
}

HRESULT
__stdcall
SK_IWrapGameInput::FindDeviceFromObject (_In_         IUnknown          *value,
                                         _COM_Outptr_ IGameInputDevice **device) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->FindDeviceFromObject (value, device);
}

HRESULT
__stdcall
SK_IWrapGameInput::FindDeviceFromPlatformHandle (_In_         HANDLE             value,
                                                 _COM_Outptr_ IGameInputDevice **device) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->FindDeviceFromPlatformHandle (value, device);
}

HRESULT
__stdcall
SK_IWrapGameInput::FindDeviceFromPlatformString (_In_         LPCWSTR            value,
                                                 _COM_Outptr_ IGameInputDevice **device) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->FindDeviceFromPlatformString (value, device);
}

HRESULT
__stdcall
SK_IWrapGameInput::EnableOemDeviceSupport (_In_ uint16_t vendorId,
                                           _In_ uint16_t productId,
                                           _In_ uint8_t interfaceNumber,
                                           _In_ uint8_t collectionNumber) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->EnableOemDeviceSupport (vendorId, productId, interfaceNumber, collectionNumber);
}

void
__stdcall
SK_IWrapGameInput::SetFocusPolicy (_In_ GameInputFocusPolicy policy) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->SetFocusPolicy (policy);
}


HRESULT
WINAPI
GameInputCreate_Detour (IGameInput** gameInput)
{
  SK_LOG_FIRST_CALL

  if (SK_IsCurrentGame (SK_GAME_ID::Stalker2) && config.input.gamepad.xinput.emulate)
  {
    if (! SK_XInput_PollController (0, nullptr))
    {
      SK_ImGui_Warning (
        L"This game uses GameInput and Special K cannot emulate it, you must use DS4Windows."
      );
    }
  }

  IGameInput* pReal;

  HRESULT hr =
    GameInputCreate_Original (&pReal);

  if (SUCCEEDED (hr))
  {
    if (gameInput != nullptr)
    {
      *gameInput = (IGameInput *)new SK_IWrapGameInput (pReal);

      pReal->EnableOemDeviceSupport (SK_HID_VID_SONY, SK_HID_PID_DUALSHOCK4,        0, 0);
      pReal->EnableOemDeviceSupport (SK_HID_VID_SONY, SK_HID_PID_DUALSHOCK4_REV2,   0, 0);
      pReal->EnableOemDeviceSupport (SK_HID_VID_SONY, SK_HID_PID_DUALSHOCK4_DONGLE, 0, 0);
      pReal->EnableOemDeviceSupport (SK_HID_VID_SONY, SK_HID_PID_DUALSENSE,         0, 0);
      pReal->EnableOemDeviceSupport (SK_HID_VID_SONY, SK_HID_PID_DUALSENSE_EDGE,    0, 0);
    }

    else
    {
      pReal->Release ();
    }
  }

  return hr;
}



#pragma region IUnknown
HRESULT
__stdcall
SK_IWrapGameInputReading::QueryInterface (REFIID riid, void **ppvObject) noexcept
{
  if (ppvObject == nullptr)
  {
    return E_POINTER;
  }

  if ( riid == __uuidof (IUnknown) ||
       riid == __uuidof (IGameInputReading) )
  {
    auto _GetVersion = [](REFIID riid) ->
    UINT
    {
      if (riid == __uuidof (IGameInputReading))  return 0;

      return 0;
    };

    UINT required_ver =
      _GetVersion (riid);

    if (ver_ < required_ver)
    {
      IUnknown* pPromoted = nullptr;

      if ( FAILED (
             pReal->QueryInterface ( riid,
                           (void **)&pPromoted )
                  ) || pPromoted == nullptr
         )
      {
        return E_NOINTERFACE;
      }

      ver_ =
        SK_COM_PromoteInterface (&pReal, pPromoted) ?
                                       required_ver : ver_;
    }

    else
    {
      AddRef ();
    }

    *ppvObject = this;

    return S_OK;
  }

  HRESULT hr =
    pReal->QueryInterface (riid, ppvObject);

  static
    concurrency::concurrent_unordered_set <std::wstring> reported_guids;

  wchar_t                wszGUID [41] = { };
  StringFromGUID2 (riid, wszGUID, 40);

  bool once =
    reported_guids.count (wszGUID) > 0;

  if (! once)
  {
    reported_guids.insert (wszGUID);

    SK_LOG0 ( ( L"QueryInterface on wrapped IGameInputReading for Mystery UUID: %s",
                    wszGUID ), L"Game Input" );
  }

  return hr;
}

ULONG
__stdcall
SK_IWrapGameInputReading::AddRef (void) noexcept
{
  InterlockedIncrement (&refs_);

  return
    pReal->AddRef ();
}

ULONG
__stdcall
SK_IWrapGameInputReading::Release (void) noexcept
{
  ULONG xrefs =
    InterlockedDecrement (&refs_),
         refs = pReal->Release ();

  if (xrefs == 0)
  {
    if (refs != 0)
    {
      SK_LOG0 ( ( L"Inconsistent reference count for IGameInputReading; expected=%d, actual=%d",
                  xrefs, refs ),
                  L"Game Input" );
    }

    else
      delete this;
  }

  return
    refs;
}

GameInputKind
__stdcall
SK_IWrapGameInputReading::GetInputKind (void) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetInputKind ();
}

uint64_t
__stdcall
SK_IWrapGameInputReading::GetSequenceNumber (GameInputKind inputKind) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetSequenceNumber (inputKind);
}

uint64_t
__stdcall
SK_IWrapGameInputReading::GetTimestamp (void) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetTimestamp ();
}

void
__stdcall
SK_IWrapGameInputReading::GetDevice (IGameInputDevice **device) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetDevice (device);
}

bool
__stdcall
SK_IWrapGameInputReading::GetRawReport (IGameInputRawDeviceReport **report) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetRawReport (report);
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetControllerAxisCount (void) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetControllerAxisCount ();
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetControllerAxisState (uint32_t stateArrayCount,
                                                  float   *stateArray) noexcept
{
  SK_LOG_FIRST_CALL

  auto ret =
    pReal->GetControllerAxisState (stateArrayCount, stateArray);

  if (SK_ImGui_WantGamepadCapture () && stateArray != nullptr)
  {
    ZeroMemory (stateArray, stateArrayCount * sizeof (float));
  }

  return ret;
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetControllerButtonCount (void) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetControllerButtonCount ();
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetControllerButtonState (uint32_t stateArrayCount,
                                                    bool    *stateArray) noexcept
{
  SK_LOG_FIRST_CALL

  auto ret =
    pReal->GetControllerButtonState (stateArrayCount, stateArray);

  if (SK_ImGui_WantGamepadCapture () && stateArray != nullptr)
  {
    ZeroMemory (stateArray, stateArrayCount * sizeof (bool));
  }

  return ret;
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetControllerSwitchCount (void) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetControllerSwitchCount ();
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetControllerSwitchState (uint32_t                    stateArrayCount,
                                                    GameInputSwitchPosition    *stateArray) noexcept
{
  SK_LOG_FIRST_CALL

  auto ret =
    pReal->GetControllerSwitchState (stateArrayCount, stateArray);

  if (SK_ImGui_WantGamepadCapture () && stateArray != nullptr)
  {
    ZeroMemory (stateArray, stateArrayCount * sizeof (GameInputSwitchPosition));
  }

  return ret;
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetKeyCount (void) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetKeyCount ();
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetKeyState (uint32_t           stateArrayCount,
                                       GameInputKeyState *stateArray) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetKeyState (stateArrayCount, stateArray);
}

bool
__stdcall
SK_IWrapGameInputReading::GetMouseState (GameInputMouseState *state) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetMouseState (state);
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetTouchCount (void) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetTouchCount ();
}

uint32_t
__stdcall
SK_IWrapGameInputReading::GetTouchState (uint32_t             stateArrayCount,
                                         GameInputTouchState *stateArray) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetTouchState (stateArrayCount, stateArray);
}

bool
__stdcall
SK_IWrapGameInputReading::GetMotionState (GameInputMotionState *state) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetMotionState (state);
}

bool
__stdcall
SK_IWrapGameInputReading::GetArcadeStickState (GameInputArcadeStickState* state) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetArcadeStickState (state);
}

bool
__stdcall
SK_IWrapGameInputReading::GetFlightStickState (GameInputFlightStickState *state) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetFlightStickState (state);
}

bool
__stdcall
SK_IWrapGameInputReading::GetGamepadState (GameInputGamepadState *state) noexcept
{
  SK_LOG_FIRST_CALL

  auto ret =
    pReal->GetGamepadState (state);

  if (SK_ImGui_WantGamepadCapture () && ret && state != nullptr)
    ZeroMemory (state, sizeof (GameInputGamepadState));

#if 0
  if (! SK_HID_PlayStationControllers.empty ())
  {
    if (config.input.gamepad.xinput.emulate && (! config.input.gamepad.xinput.blackout_api))
    {
      SK_HID_PlayStationDevice *pNewestInputDevice = nullptr;

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

      if (pNewestInputDevice != nullptr)
      {
        if (pReal->GetGamepadState (state))
        {
          ZeroMemory (state, sizeof (GameInputGamepadState));

          if (! SK_ImGui_WantGamepadCapture ())
          {
            state->leftThumbstickX =
              static_cast <float> (static_cast <double> (pNewestInputDevice->xinput.report.Gamepad.sThumbLX) / 32767.0);
            state->leftThumbstickY =
              static_cast <float> (static_cast <double> (pNewestInputDevice->xinput.report.Gamepad.sThumbLY) / 32767.0);

            state->rightThumbstickX =
              static_cast <float> (static_cast <double> (pNewestInputDevice->xinput.report.Gamepad.sThumbRX) / 32767.0);
            state->rightThumbstickY =
              static_cast <float> (static_cast <double> (pNewestInputDevice->xinput.report.Gamepad.sThumbRY) / 32767.0);

            state->leftTrigger =
              static_cast <float> (static_cast <double> (pNewestInputDevice->xinput.report.Gamepad.bLeftTrigger) / 255.0);
            state->rightTrigger =
              static_cast <float> (static_cast <double> (pNewestInputDevice->xinput.report.Gamepad.bRightTrigger) / 255.0);

            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0 ? GameInputGamepadA : GameInputGamepadNone;
            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0 ? GameInputGamepadB : GameInputGamepadNone;
            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0 ? GameInputGamepadX : GameInputGamepadNone;
            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0 ? GameInputGamepadY : GameInputGamepadNone;

            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0 ? GameInputGamepadMenu : GameInputGamepadNone;
            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)  != 0 ? GameInputGamepadView : GameInputGamepadNone;

            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)    != 0 ? GameInputGamepadDPadUp    : GameInputGamepadNone;
            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  != 0 ? GameInputGamepadDPadDown  : GameInputGamepadNone;
            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)  != 0 ? GameInputGamepadDPadLeft  : GameInputGamepadNone;
            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0 ? GameInputGamepadDPadRight : GameInputGamepadNone;

            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)  != 0 ? GameInputGamepadLeftShoulder  : GameInputGamepadNone;
            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0 ? GameInputGamepadRightShoulder : GameInputGamepadNone;

            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)  != 0 ? GameInputGamepadLeftThumbstick  : GameInputGamepadNone;
            state->buttons |= (pNewestInputDevice->xinput.report.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0 ? GameInputGamepadRightThumbstick : GameInputGamepadNone;
          }

          return true;
        }

        else
          return false;
      }
    }
  }
#endif

  return ret;
}

bool
__stdcall
SK_IWrapGameInputReading::GetRacingWheelState (GameInputRacingWheelState *state) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetRacingWheelState (state);
}

bool
__stdcall
SK_IWrapGameInputReading::GetUiNavigationState (GameInputUiNavigationState *state) noexcept
{
  SK_LOG_FIRST_CALL

  return
    pReal->GetUiNavigationState (state);
}

void
SK_Input_HookGameInput (void)
{
  if (! config.input.gamepad.hook_game_input)
    return;

  if (GetModuleHandleW (L"GameInput.dll") != nullptr)
  {
    static volatile LONG               hooked = FALSE;
    if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
    {
      SK_CreateDLLHook2 (      L"GameInput.dll",
                                "GameInputCreate",
                                 GameInputCreate_Detour,
        static_cast_p2p <void> (&GameInputCreate_Original) );
      SK_ApplyQueuedHooks ();
    }
  }
}