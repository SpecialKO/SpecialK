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

    SK_LOG0 ( ( L"QueryInterface on wrapped GameInput for Mystery UUID: %s",
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

HRESULT
__stdcall
SK_IWrapGameInput::GetCurrentReading (_In_          GameInputKind      inputKind,
                                      _In_opt_     IGameInputDevice   *device,
                                      _COM_Outptr_ IGameInputReading **reading) noexcept
{
  SK_LOG_FIRST_CALL

  bool capture_input = false;
  static concurrency::concurrent_unordered_map <IGameInputDevice*,SK_ComPtr<IGameInputReading>> readings;

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
      pReal->GetCurrentReading (inputKind, device, reading);

    if (reading != nullptr)
      readings [device] = *reading;

    return hr;
  }

  else
  {
    if (reading != nullptr)
       *reading = readings [device];

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
  static concurrency::concurrent_unordered_map <IGameInputDevice*,SK_ComPtr<IGameInputReading>> readings;

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
      readings [device] = *reading;

    return hr;
  }

  else
  {
    if (reading != nullptr)
       *reading = readings [device];

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