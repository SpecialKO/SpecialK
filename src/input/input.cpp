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
#define _CRT_SECURE_NO_WARNINGS
#define DIRECTINPUT_VERSION 0x0800

#include <SpecialK/input/input.h>
#include <SpecialK/window.h>
#include <SpecialK/console.h>

#include <SpecialK/utility.h>
#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <dinput.h>
#include <comdef.h>

#include <stdarg.h>

#include <imgui/imgui.h>

#define SK_LOG_FIRST_CALL

extern bool SK_ImGui_Visible;
extern bool nav_usable;

//////////////////////////////////////////////////////////////
//
// HIDClass (Usermode)
//
//////////////////////////////////////////////////////////////
#pragma comment (lib, "hid.lib")

#include <hidusage.h>
#include <Hidpi.h>

typedef NTSTATUS (__stdcall *HidP_GetData_pfn)(
  _In_    HIDP_REPORT_TYPE     ReportType,
  _Out_   PHIDP_DATA           DataList,
  _Inout_ PULONG               DataLength,
  _In_    PHIDP_PREPARSED_DATA PreparsedData,
  _In_    PCHAR                Report,
  _In_    ULONG                ReportLength
);

HidP_GetData_pfn HidP_GetData_Original = nullptr;

NTSTATUS
__stdcall
HidP_GetData_Detour (
  _In_    HIDP_REPORT_TYPE     ReportType,
  _Out_   PHIDP_DATA           DataList,
  _Inout_ PULONG               DataLength,
  _In_    PHIDP_PREPARSED_DATA PreparsedData,
  _In_    PCHAR                Report,
  _In_    ULONG                ReportLength )
{
  SK_LOG_FIRST_CALL

  NTSTATUS ret =
    HidP_GetData_Original ( ReportType, DataList,
                              DataLength, PreparsedData,
                                Report, ReportLength );


  // De we want block this I/O?
  bool filter = false;

  if ( ret == HIDP_STATUS_SUCCESS && ( ReportType == HidP_Input || ReportType == HidP_Output ))
  {

    HIDP_CAPS caps;
    NTSTATUS  stat =
      HidP_GetCaps (PreparsedData, &caps);

    if ( stat           == HIDP_STATUS_SUCCESS && 
         caps.UsagePage == HID_USAGE_PAGE_GENERIC )
    {
      switch (caps.Usage)
      {
        case HID_USAGE_GENERIC_GAMEPAD:
        case HID_USAGE_GENERIC_JOYSTICK:
        {
          if (SK_ImGui_WantGamepadCapture ())
            filter = true;
        } break;

        case HID_USAGE_GENERIC_MOUSE:
        {
          if (SK_ImGui_WantMouseCapture ())
            filter = true;
        } break;

        case HID_USAGE_GENERIC_KEYBOARD:
        {
          if (SK_ImGui_WantKeyboardCapture ())
            filter = true;
        } break;
      }
    }
  }


  if (! filter)
    return ret;

  else {
    memset (DataList, *DataLength, 0);
           *DataLength           = 0;
  }

  return ret;
}

void
SK_Input_HookHID (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedExchangeAdd (&hooked, 0))
  {
    SK_CreateDLLHook2 ( L"HID.DLL", "HidP_GetData",
                          HidP_GetData_Detour,
                (LPVOID*)&HidP_GetData_Original );

    MH_ApplyQueued ();

    if (HidP_GetData_Original != nullptr)
      InterlockedIncrement (&hooked);
  }
}

void
SK_Input_PreeHookHID (void)
{
  static sk_import_test_s tests [] = { { "hid.dll", false } };

  SK_TestImports (GetModuleHandle (nullptr), tests, 1);

  if (tests [0].used)
  {
    SK_LOG0 ( ( L"Game uses HID, installing input hooks..." ),
                L"   Input  " );

    SK_Input_HookHID ();
  }
}


//////////////////////////////////////////////////////////////////////////////////
//
// Raw Input
//
//////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI RegisterRawInputDevices_Detour (
  _In_ PCRAWINPUTDEVICE pRawInputDevices,
  _In_ UINT             uiNumDevices,
  _In_ UINT             cbSize
)
{
  //SK_LOG_FIRST_CALL

  if (cbSize != sizeof RAWINPUTDEVICE) {
    dll_log.Log ( L"[ RawInput ] RegisterRawInputDevices has wrong "
                  L"structure size (%lu bytes), expected: %lu",
                    cbSize,
                      sizeof RAWINPUTDEVICE );

    return
      RegisterRawInputDevices_Original ( pRawInputDevices,
                                           uiNumDevices,
                                             cbSize );
  }

  RAWINPUTDEVICE* pDevices = nullptr;

  if (pRawInputDevices && uiNumDevices > 0)
    pDevices = new RAWINPUTDEVICE [uiNumDevices];

  if (pDevices != nullptr)
  {
    // We need to continue receiving window messages for the console to work
    for (unsigned int i = 0; i < uiNumDevices; i++)
    {
      pDevices [i] = pRawInputDevices [i];
      pDevices [i].dwFlags &= ~(RIDEV_NOLEGACY | RIDEV_APPKEYS | RIDEV_REMOVE);
      pDevices [i].dwFlags &= ~RIDEV_CAPTUREMOUSE;
    }
  }

  BOOL bRet =
    RegisterRawInputDevices_Original (pDevices, uiNumDevices, cbSize);

  if (pDevices)
    delete [] pDevices;

  return bRet;
}

RegisterRawInputDevices_pfn RegisterRawInputDevices_Original = nullptr;
GetRawInputData_pfn         GetRawInputData_Original         = nullptr;
GetRawInputBuffer_pfn       GetRawInputBuffer_Original       = nullptr;

UINT
WINAPI
GetRawInputBuffer_Detour (_Out_opt_ PRAWINPUT pData,
                          _Inout_   PUINT     pcbSize,
                          _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  if (SK_ImGui_Visible)
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    bool filter = false;

    // Unconditional
    if (config.input.ui.capture)
      filter = true;

    // Only specific types
    if (io.WantCaptureKeyboard || io.WantCaptureMouse)
      filter = true;

    if (filter)
    {
      if (pData != nullptr)
      {
        const int max_items = (sizeof RAWINPUT / *pcbSize);
              int count     =                            0;
        uint8_t *pTemp      = (uint8_t *)
                                  new RAWINPUT [max_items];
        uint8_t *pInput     =                        pTemp;
        uint8_t *pOutput    =             (uint8_t *)pData;
        UINT     cbSize     =                     *pcbSize;
                  *pcbSize  =                            0;

        int       temp_ret  =
          GetRawInputBuffer_Original ( (RAWINPUT *)pTemp, &cbSize, cbSizeHeader );

        for (int i = 0; i < temp_ret; i++)
        {
          RAWINPUT* pItem = (RAWINPUT *)pInput;

          bool  remove = false;
          int  advance = pItem->header.dwSize;

          switch (pItem->header.dwType)
          {
            case RIM_TYPEKEYBOARD:
              if (SK_ImGui_WantKeyboardCapture ())
                remove = true;
              break;

            case RIM_TYPEMOUSE:
              if (SK_ImGui_WantMouseCapture ())
                remove = true;
              break;

            default:
              if (SK_ImGui_WantGamepadCapture ())
                remove = true;
              break;
          }

          if (config.input.ui.capture)
            remove = true;

          if (! remove) {
            memcpy (pOutput, pItem, pItem->header.dwSize);
             pOutput += advance;
                        ++count;
            *pcbSize += advance;
          }
          
          pInput += advance;
        }

        delete [] pTemp;

        return count;
      }
    }
  }

  return GetRawInputBuffer_Original (pData, pcbSize, cbSizeHeader);
}

UINT
WINAPI
SK_ImGui_ProcessRawInput (_In_      HRAWINPUT hRawInput,
                          _In_      UINT      uiCommand,
                          _Out_opt_ LPVOID    pData,
                          _Inout_   PUINT     pcbSize,
                          _In_      UINT      cbSizeHeader);

UINT
WINAPI
GetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                        _In_      UINT      uiCommand,
                        _Out_opt_ LPVOID    pData,
                        _Inout_   PUINT     pcbSize,
                        _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  return SK_ImGui_ProcessRawInput (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
}





#define DINPUT8_CALL(_Ret, _Call) {                                     \
  dll_log.LogEx (true, L"[   Input  ]  Calling original function: ");   \
  (_Ret) = (_Call);                                                     \
  _com_error err ((_Ret));                                              \
  if ((_Ret) != S_OK)                                                   \
    dll_log.LogEx (false, L"(ret=0x%04x - %s)\n", err.WCode (),         \
                                                  err.ErrorMessage ()); \
  else                                                                  \
    dll_log.LogEx (false, L"(ret=S_OK)\n");                             \
}

///////////////////////////////////////////////////////////////////////////////
//
// DirectInput 8
//
///////////////////////////////////////////////////////////////////////////////
typedef HRESULT (WINAPI *IDirectInput8_CreateDevice_pfn)(
  IDirectInput8       *This,
  REFGUID              rguid,
  LPDIRECTINPUTDEVICE *lplpDirectInputDevice,
  LPUNKNOWN            pUnkOuter
);

typedef HRESULT (WINAPI *IDirectInputDevice8_GetDeviceState_pfn)(
  LPDIRECTINPUTDEVICE  This,
  DWORD                cbData,
  LPVOID               lpvData
);

typedef HRESULT (WINAPI *IDirectInputDevice8_SetCooperativeLevel_pfn)(
  LPDIRECTINPUTDEVICE  This,
  HWND                 hwnd,
  DWORD                dwFlags
);

IDirectInput8_CreateDevice_pfn
        IDirectInput8_CreateDevice_Original              = nullptr;
IDirectInputDevice8_GetDeviceState_pfn
        IDirectInputDevice8_GetDeviceState_Original      = nullptr;
IDirectInputDevice8_SetCooperativeLevel_pfn
        IDirectInputDevice8_SetCooperativeLevel_Original = nullptr;

struct di8_keyboard_s {
  LPDIRECTINPUTDEVICE pDev = nullptr;
  uint8_t             state [512];
} _dik;

struct di8_mouse_s {
  LPDIRECTINPUTDEVICE pDev = nullptr;
  DIMOUSESTATE2       state;
} _dim;

HRESULT
WINAPI
IDirectInputDevice8_GetDeviceState_Detour ( LPDIRECTINPUTDEVICE        This,
                                            DWORD                      cbData,
                                            LPVOID                     lpvData )
{
  HRESULT hr;
  hr = IDirectInputDevice8_GetDeviceState_Original ( This,
                                                       cbData,
                                                         lpvData );

  if (SUCCEEDED (hr) && lpvData != nullptr)
  {
    if (cbData == sizeof DIJOYSTATE2) 
    {
      DIJOYSTATE2* out = (DIJOYSTATE2 *)lpvData;

      if (nav_usable)
      {
        memset (out, 0, sizeof DIJOYSTATE2);

        out->rgdwPOV [0] = -1;
        out->rgdwPOV [1] = -1;
        out->rgdwPOV [2] = -1;
        out->rgdwPOV [3] = -1;
      }

      else {
#if 0
        DIJOYSTATE2  in  = *out;

        for (int i = 0; i < 12; i++) {
          // Negative values are for axes, we cannot remap those yet
          if (gamepad.remap.map [ i ] >= 0) {
            out->rgbButtons [ i ] = 
              in.rgbButtons [ gamepad.remap.map [ i ] ];
          }
        }
#endif
      }
    }

    else if (cbData == sizeof DIJOYSTATE) 
    {
      //dll_log.Log (L"Joy");

      DIJOYSTATE* out = (DIJOYSTATE *)lpvData;

      if (nav_usable)
      {
        memset (out, 0, sizeof DIJOYSTATE);

        out->rgdwPOV [0] = -1;
        out->rgdwPOV [1] = -1;
        out->rgdwPOV [2] = -1;
        out->rgdwPOV [3] = -1;
      }
    }

    else if (This == _dik.pDev || cbData == 256)
    {
      if (SK_ImGui_WantKeyboardCapture () && lpvData != nullptr)
        memset (lpvData, 0, cbData);
    }

    else if ( cbData == sizeof (DIMOUSESTATE2) ||
              cbData == sizeof (DIMOUSESTATE)  )
    {
      //dll_log.Log (L"Mouse");

      if (SK_ImGui_WantMouseCapture ())
      {
        switch (cbData)
        {
          case sizeof (DIMOUSESTATE2):
            ((DIMOUSESTATE2 *)lpvData)->lX = 0;
            ((DIMOUSESTATE2 *)lpvData)->lY = 0;
            ((DIMOUSESTATE2 *)lpvData)->lZ = 0;
            memset (((DIMOUSESTATE2 *)lpvData)->rgbButtons, 0, 8);
            break;

          case sizeof (DIMOUSESTATE):
            ((DIMOUSESTATE *)lpvData)->lX = 0;
            ((DIMOUSESTATE *)lpvData)->lY = 0;
            ((DIMOUSESTATE *)lpvData)->lZ = 0;
            memset (((DIMOUSESTATE *)lpvData)->rgbButtons, 0, 4);
            break;
        }
      }
    }
  }

  return hr;
}

HRESULT
WINAPI
IDirectInputDevice8_SetCooperativeLevel_Detour ( LPDIRECTINPUTDEVICE  This,
                                                 HWND                 hwnd,
                                                 DWORD                dwFlags )
{
  //if (config.input.block_windows)
    //dwFlags |= DISCL_NOWINKEY;

#if 0
  if (config.render.allow_background) {
    dwFlags &= ~DISCL_EXCLUSIVE;
    dwFlags &= ~DISCL_BACKGROUND;

    dwFlags |= DISCL_NONEXCLUSIVE;
    dwFlags |= DISCL_FOREGROUND;

    return IDirectInputDevice8_SetCooperativeLevel_Original (This, hwnd, dwFlags);
  }
#endif

  return IDirectInputDevice8_SetCooperativeLevel_Original (This, hwnd, dwFlags);
}

HRESULT
WINAPI
IDirectInput8_CreateDevice_Detour ( IDirectInput8       *This,
                                    REFGUID              rguid,
                                    LPDIRECTINPUTDEVICE *lplpDirectInputDevice,
                                    LPUNKNOWN            pUnkOuter )
{
  const wchar_t* wszDevice = (rguid == GUID_SysKeyboard)   ? L"Default System Keyboard" :
                                (rguid == GUID_SysMouse)   ? L"Default System Mouse"    :  
                                  (rguid == GUID_Joystick) ? L"Gamepad / Joystick"      :
                                                           L"Other Device";

  dll_log.Log ( L"[   Input  ][!] IDirectInput8::CreateDevice (%08Xh, %s, %08Xh, %08Xh)",
                   This,
                     wszDevice,
                       lplpDirectInputDevice,
                         pUnkOuter );

  HRESULT hr;
  DINPUT8_CALL ( hr,
                  IDirectInput8_CreateDevice_Original ( This,
                                                         rguid,
                                                          lplpDirectInputDevice,
                                                           pUnkOuter ) );

  static bool hooked = false;

  if (SUCCEEDED (hr) && (! hooked))
  {
    if (! hooked)
    {
      hooked = true;
      void** vftable = *(void***)*lplpDirectInputDevice;

      SK_CreateFuncHook ( L"IDirectInputDevice8::GetDeviceState",
                           vftable [9],
                           IDirectInputDevice8_GetDeviceState_Detour,
                (LPVOID *)&IDirectInputDevice8_GetDeviceState_Original );

      MH_QueueEnableHook (vftable [9]);

      SK_CreateFuncHook ( L"IDirectInputDevice8::SetCooperativeLevel",
                           vftable [13],
                           IDirectInputDevice8_SetCooperativeLevel_Detour,
                 (LPVOID*)&IDirectInputDevice8_SetCooperativeLevel_Original );

      MH_QueueEnableHook (vftable [13]);

      MH_ApplyQueued ();
    }

    if (rguid == GUID_SysMouse)
      _dim.pDev = *lplpDirectInputDevice;
    else if (rguid == GUID_SysKeyboard)
      _dik.pDev = *lplpDirectInputDevice;
  }

#if 0
  if (SUCCEEDED (hr) && lplpDirectInputDevice != nullptr) {
    DWORD dwFlag = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

    if (config.input.block_windows)
      dwFlag |= DISCL_NOWINKEY;

    (*lplpDirectInputDevice)->SetCooperativeLevel (SK_GetGameWindow (), dwFlag);
  }
#endif

  return hr;
}

typedef HRESULT (WINAPI *DirectInput8Create_pfn)(
 HINSTANCE hinst,
 DWORD     dwVersion,
 REFIID    riidltf,
 LPVOID*   ppvOut,
 LPUNKNOWN punkOuter
);

DirectInput8Create_pfn DirectInput8Create_Original = nullptr;

HRESULT
WINAPI
DirectInput8Create_Detour (
  HINSTANCE hinst,
  DWORD     dwVersion,
  REFIID    riidltf,
  LPVOID*   ppvOut,
  LPUNKNOWN punkOuter
)
{
  HRESULT hr = E_NOINTERFACE;

  if ( SUCCEEDED (
         (hr = DirectInput8Create_Original (hinst, dwVersion, riidltf, ppvOut, punkOuter))
       )
     )
  {
    if (! IDirectInput8_CreateDevice_Original)
    {
      void** vftable = *(void***)*ppvOut;
      
      SK_CreateFuncHook ( L"IDirectInput8::CreateDevice",
                           vftable [3],
                           IDirectInput8_CreateDevice_Detour,
                 (LPVOID*)&IDirectInput8_CreateDevice_Original );
      
      SK_EnableHook (vftable [3]);
    }
  }

  return hr;
}

void
SK_Input_HookDI8 (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedExchangeAdd (&hooked, 0))
  {
    SK_CreateDLLHook ( L"dinput8.dll",
                        "DirectInput8Create",
                        DirectInput8Create_Detour,
             (LPVOID *)&DirectInput8Create_Original );

    InterlockedIncrement (&hooked);
  }
}

void
SK_Input_PreHookDI8 (void)
{
  if (DirectInput8Create_Original == nullptr)
  {
    static sk_import_test_s tests [] = { { "dinput.dll",  false },
                                         { "dinput8.dll", false } };

    SK_TestImports (GetModuleHandle (nullptr), tests, 2);

    if (tests [0].used || tests [1].used)
    {
      SK_LOG0 ( ( L"Game uses DirectInput, installing input hooks..." ),
                  L"   Input  " );

      SK_Input_HookDI8 ();
    }
  }
}


///////////////////////////////////////////////////////
//
// XInput
//
///////////////////////////////////////////////////////

typedef DWORD (WINAPI *XInputGetState_pfn)(
  _In_  DWORD        dwUserIndex,
  _Out_ XINPUT_STATE *pState
);

static XInputGetState_pfn XInputGetState1_3_Original   = nullptr;
static XInputGetState_pfn XInputGetState1_4_Original   = nullptr;
static XInputGetState_pfn XInputGetState9_1_0_Original = nullptr;
static XInputGetState_pfn XInputGetState_Original      = nullptr;


extern bool
SK_ImGui_FilterXInput (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState );


DWORD
WINAPI
XInputGetState1_3_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  DWORD dwRet =
    XInputGetState1_3_Original (dwUserIndex, pState);

    SK_ImGui_FilterXInput      (dwUserIndex, pState);

  return dwRet;
}

DWORD
WINAPI
XInputGetState1_4_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  DWORD dwRet =
    XInputGetState1_4_Original (dwUserIndex, pState);

    SK_ImGui_FilterXInput      (dwUserIndex, pState);

  return dwRet;
}

DWORD
WINAPI
XInputGetState9_1_0_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  DWORD dwRet =
    XInputGetState9_1_0_Original (dwUserIndex, pState);

    SK_ImGui_FilterXInput        (dwUserIndex, pState);

  return dwRet;
}


void
SK_Input_HookXInput1_4 (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedExchangeAdd (&hooked, 0))
  {
    SK_LOG0 ( ( L"  >> Hooking XInput 1.4" ),
                L"   Input  " );

    SK_CreateDLLHook3 ( L"XInput1_4.dll",
                         "XInputGetState",
                         XInputGetState1_4_Detour,
              (LPVOID *)&XInputGetState1_4_Original );

    if (XInputGetState_Original == nullptr)
      XInputGetState_Original = XInputGetState1_4_Original;

    InterlockedIncrement (&hooked);
  }
}

void
SK_Input_HookXInput1_3 (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedExchangeAdd (&hooked, 0))
  {
    SK_LOG0 ( ( L"  >> Hooking XInput 1.3" ),
                L"   Input  " );

    SK_CreateDLLHook3 ( L"XInput1_3.dll",
                         "XInputGetState",
                         XInputGetState1_3_Detour,
              (LPVOID *)&XInputGetState1_3_Original );

    if (XInputGetState_Original == nullptr)
      XInputGetState_Original = XInputGetState1_3_Original;

    InterlockedIncrement (&hooked);
  }
}

void
SK_Input_HookXInput9_1_0 (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedExchangeAdd (&hooked, 0))
  {
    SK_LOG0 ( ( L"  >> Hooking XInput9_1_0" ),
                L"   Input  " );

    SK_CreateDLLHook3 ( L"XInput9_1_0.dll",
                         "XInputGetState",
                         XInputGetState9_1_0_Detour,
              (LPVOID *)&XInputGetState9_1_0_Original );

    if (XInputGetState_Original == nullptr)
      XInputGetState_Original = XInputGetState9_1_0_Original;

    InterlockedIncrement (&hooked);
  }
}


bool
SK_XInput_PollController ( INT           iJoyID,
                           XINPUT_STATE* pState )
{
#if 0
  static bool first_frame = true;

  if (first_frame && XInputGetState_Original == nullptr)
  {
    if (GetModuleHandle (L"XInput1_3.dll"))
      SK_Input_HookXInput1_3 ();

    if (GetModuleHandle (L"XInput1_4.dll"))
      SK_Input_HookXInput1_4 ();

    if (GetModuleHandle (L"XInput9_1_0.dll"))
      SK_Input_HookXInput9_1_0 ();

    first_frame = false;
  }
#endif


  if (XInputGetState_Original == nullptr)
  {
    static bool tried_to_hook = false;

    if (tried_to_hook)
      return false;

    tried_to_hook = true;


    // First try 1.3, that's generally available.
    SK_Input_HookXInput1_3 ();

    // Then 1.4
    if (XInputGetState_Original == nullptr)
      SK_Input_HookXInput1_4 ();

    // Down-level 9_1_0 if all else failes
    if (XInputGetState_Original == nullptr)
      SK_Input_HookXInput9_1_0 ();


    // No XInput?! User shouldn't be playing games :P
    if (XInputGetState_Original == nullptr)
      return false;
  }

  if (iJoyID == -1)
    return true;

  const int MAX_CONTROLLERS = 4;

  XINPUT_STATE xstate;

  static DWORD last_poll [MAX_CONTROLLERS] = { 0 };
  static DWORD dwRet     [MAX_CONTROLLERS] = { 0 };

  // This function is actually a performance hazzard when no controllers
  //   are plugged in, so ... throttle the sucker.
  if (last_poll [iJoyID] < timeGetTime () - 500UL)
    dwRet [iJoyID] = XInputGetState_Original (iJoyID, &xstate);

  if (dwRet [iJoyID] == ERROR_DEVICE_NOT_CONNECTED)
    return false;

  if (pState != nullptr)
    memcpy (pState, &xstate, sizeof XINPUT_STATE);

  return true;
}



void
SK_Input_PreHookXInput (void)
{
  if (XInputGetState_Original == nullptr)
  {
    static sk_import_test_s tests [] = { { "XInput1_3.dll",   false },
                                         { "XInput1_4.dll",   false },
                                         { "XInput9_1_0.dll", false } };

    SK_TestImports (GetModuleHandle (nullptr), tests, 3);

    if (tests [0].used || tests [1].used || tests [2].used)
    {
      SK_LOG0 ( ( L"Game uses XInput, installing input hooks..." ),
                  L"   Input  " );

#if 0
SK_Input_HookXInput1_3 ();
SK_Input_HookXInput1_4 ();
SK_Input_HookXInput9_1_0 ();
#else
      if (tests [0].used) SK_Input_HookXInput1_3   ();
      if (tests [1].used) SK_Input_HookXInput1_4   ();
      if (tests [2].used) SK_Input_HookXInput9_1_0 ();
#endif
    }

    MH_ApplyQueued ();
  }
}












/////////////////////////////////////////////////
//
// ImGui Cursor Management
//
/////////////////////////////////////////////////
#include <imgui/imgui.h>
sk_imgui_cursor_s SK_ImGui_Cursor;

typedef HCURSOR (WINAPI *SetCursor_pfn)(HCURSOR hCursor);
extern SetCursor_pfn SetCursor_Original;

HCURSOR GetGameCursor (void);


void
sk_imgui_cursor_s::update (void)
{
  if (GetGameCursor () != nullptr)
    SK_ImGui_Cursor.orig_img = GetGameCursor ();

  if (SK_ImGui_Visible)
  {
    if (ImGui::GetIO ().WantCaptureMouse || SK_ImGui_Cursor.orig_img == nullptr)
      SK_ImGui_Cursor.showSystemCursor (false);
    else
      SK_ImGui_Cursor.showSystemCursor (true);
  }

  else
    SK_ImGui_Cursor.showSystemCursor ();
}

void
sk_imgui_cursor_s::showImGuiCursor (void)
{
  showSystemCursor (false);
}

void
sk_imgui_cursor_s::LocalToScreen (LPPOINT lpPoint)
{
  LocalToClient  (lpPoint);
  ClientToScreen (game_window.hWnd, lpPoint);
}

void
sk_imgui_cursor_s::LocalToClient    (LPPOINT lpPoint)
{
  RECT real_client;
  GetClientRect (game_window.hWnd, &real_client);

  ImVec2 local_dims =
    ImGui::GetIO ().DisplayFramebufferScale;

  struct {
    float width,
          height;
  } in, out;

  in.width   = local_dims.x;
  in.height  = local_dims.y;

  out.width  = (float)(real_client.right  - real_client.left);
  out.height = (float)(real_client.bottom - real_client.top);

  float x = 2.0f * ((float)lpPoint->x / in.width ) - 1.0f;
  float y = 2.0f * ((float)lpPoint->y / in.height) - 1.0f;

  lpPoint->x = (LONG)( ( x * out.width  + out.width  ) / 2.0f );
  lpPoint->y = (LONG)( ( y * out.height + out.height ) / 2.0f );
}

void
sk_imgui_cursor_s::ClientToLocal    (LPPOINT lpPoint)
{
  RECT real_client;
  GetClientRect (game_window.hWnd, &real_client);

  ImVec2 local_dims =
    ImGui::GetIO ().DisplayFramebufferScale;

  struct {
    float width,
          height;
  } in, out;

  out.width  = local_dims.x;
  out.height = local_dims.y;

  in.width   = (float)(real_client.right  - real_client.left);
  in.height  = (float)(real_client.bottom - real_client.top);

  float x = 2.0f * ((float)lpPoint->x / in.width ) - 1.0f;
  float y = 2.0f * ((float)lpPoint->y / in.height) - 1.0f;

  lpPoint->x = (LONG)( ( x * out.width  + out.width  ) / 2.0f );
  lpPoint->y = (LONG)( ( y * out.height + out.height ) / 2.0f );
}

void
sk_imgui_cursor_s::ScreenToLocal    (LPPOINT lpPoint)
{
  ScreenToClient (game_window.hWnd, lpPoint);
  ClientToLocal  (lpPoint);
}


#include <resource.h>

HCURSOR
ImGui_DesiredCursor (void)
{
  static HCURSOR last_cursor = 0;

  if (ImGui::GetIO ().MouseDownDuration [0] <= 0.0f || last_cursor == 0)
  {
    switch (ImGui::GetMouseCursor ())
    {
      case ImGuiMouseCursor_Arrow:
        //SetCursor_Original ((last_cursor = LoadCursor (nullptr, IDC_ARROW)));
        return ((last_cursor = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_POINTER)));
        break;                          
      case ImGuiMouseCursor_TextInput:  
        return ((last_cursor = LoadCursor (nullptr, IDC_IBEAM)));
        break;                          
      case ImGuiMouseCursor_ResizeEW:
        //SetCursor_Original ((last_cursor = LoadCursor (nullptr, IDC_SIZEWE)));
        return ((last_cursor = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_HORZ)));
        break;                          
      case ImGuiMouseCursor_ResizeNWSE: 
        return ((last_cursor = LoadCursor (nullptr, IDC_SIZENWSE)));
        break;
    }
  }

  else
    return (last_cursor);

  return GetCursor ();
}

void
ImGuiCursor_Impl (void)
{
  CURSORINFO ci;
  ci.cbSize = sizeof CURSORINFO;

  GetCursorInfo_Original (&ci);

  //
  // Hardware Cursor
  //
  if (config.input.ui.use_hw_cursor)
    SetCursor_Original (ImGui_DesiredCursor ());

  if ( config.input.ui.use_hw_cursor && (ci.flags & CURSOR_SHOWING) )
  {
    ImGui::GetIO ().MouseDrawCursor = false;
  }
  
  //
  // Software
  //
  else
  {
    SetCursor_Original (nullptr);
    ImGui::GetIO ().MouseDrawCursor = (! SK_ImGui_Cursor.idle);
  }
}

void
sk_imgui_cursor_s::showSystemCursor (bool system)
{
  CURSORINFO cursor_info;
  cursor_info.cbSize = sizeof (CURSORINFO);

  static HCURSOR wait_cursor = LoadCursor (nullptr, IDC_WAIT);

  if (SK_ImGui_Cursor.orig_img == wait_cursor)
    SK_ImGui_Cursor.orig_img = LoadCursor (nullptr, IDC_ARROW);

  if (system)
  {
    SetCursor_Original     (SK_ImGui_Cursor.orig_img);
    GetCursorInfo_Original (&cursor_info);

    if ((! SK_ImGui_Visible) || (cursor_info.flags & CURSOR_SHOWING))
      ImGui::GetIO ().MouseDrawCursor = false;

    else
      ImGuiCursor_Impl ();
  }

  else
    ImGuiCursor_Impl ();
}


void
sk_imgui_cursor_s::activateWindow (bool active)
{
  CURSORINFO ci;
  ci.cbSize = sizeof ci;
  
  GetCursorInfo_Original (&ci);
  
  if (active)
  {
    if (SK_ImGui_Visible)
    {
      if (SK_ImGui_WantMouseCapture ())
      {
  
      } else if (SK_ImGui_Cursor.orig_img)
        SetCursor_Original (SK_ImGui_Cursor.orig_img);
    }
  }
}




HCURSOR game_cursor    = 0;

bool
SK_ImGui_WantKeyboardCapture (void)
{
  bool imgui_capture = false;

  if (SK_ImGui_Visible)
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    extern bool nav_usable;
    if (nav_usable || io.WantCaptureKeyboard || io.WantTextInput)
      imgui_capture = true;
  }

  return imgui_capture;
}

bool
SK_ImGui_WantTextCapture (void)
{
  bool imgui_capture = false;

  if (SK_ImGui_Visible)
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    if (io.WantTextInput)
      imgui_capture = true;
  }

  return imgui_capture;
}

bool
SK_ImGui_WantGamepadCapture (void)
{
  bool imgui_capture = false;

  if (SK_ImGui_Visible)
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    extern bool nav_usable;
    if (nav_usable)
      imgui_capture = true;
  }

  return imgui_capture;
}

bool
SK_ImGui_WantMouseCapture (void)
{
  bool imgui_capture = false;

  if (SK_ImGui_Visible)
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    if (config.input.ui.capture_mouse || io.WantCaptureMouse/* || io.WantTextInput*/)
      imgui_capture = true;
  }

  return imgui_capture;
}

typedef HCURSOR (WINAPI *SetCursor_pfn)(HCURSOR hCursor);
SetCursor_pfn SetCursor_Original = nullptr;

extern
void
SK_ImGui_CenterCursorOnWindow (void);

HCURSOR GetGameCursor (void)
{
  static HCURSOR sk_imgui_arrow = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_POINTER);
  static HCURSOR sk_imgui_horz  = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_HORZ);
  static HCURSOR sk_imgui_ibeam = LoadCursor (nullptr, IDC_IBEAM);
  static HCURSOR sys_arrow      = LoadCursor (nullptr, IDC_ARROW);
  static HCURSOR sys_wait       = LoadCursor (nullptr, IDC_WAIT);

  static HCURSOR hCurLast = 0;
         HCURSOR hCur     = GetCursor ();

  if ( hCur != sk_imgui_horz && hCur != sk_imgui_arrow && hCur != sk_imgui_ibeam &&
       hCur != sys_arrow     && hCur != sys_wait )
    hCurLast = hCur;

  return hCurLast;
}

void
ImGui_ToggleCursor (void)
{
  if (! SK_ImGui_Cursor.visible)
  {
    if (SK_ImGui_Cursor.orig_img == nullptr)
      SK_ImGui_Cursor.orig_img = GetGameCursor ();

    //GetClipCursor         (&SK_ImGui_Cursor.clip_rect);

    SK_ImGui_CenterCursorOnWindow ();

    // Save original cursor position
    GetCursorPos_Original         (&SK_ImGui_Cursor.orig_pos);
    SK_ImGui_Cursor.ScreenToLocal (&SK_ImGui_Cursor.orig_pos);

    SK_ImGui_Cursor.pos              = SK_ImGui_Cursor.orig_pos;
    ImGui::GetIO ().WantCaptureMouse = true;
  }

  else
  {
    if (SK_ImGui_Cursor.orig_img == nullptr)
      SK_ImGui_Cursor.orig_img = GetGameCursor ();

    if (SK_ImGui_WantMouseCapture ())
    {
      //ClipCursor_Original   (&SK_ImGui_Cursor.clip_rect);

      POINT screen = SK_ImGui_Cursor.orig_pos;
      SK_ImGui_Cursor.LocalToScreen (&screen);
      SetCursorPos_Original ( screen.x,
                              screen.y );
    }

    ImGui::GetIO ().WantCaptureMouse = false;
  }

  SK_ImGui_Cursor.visible = (! SK_ImGui_Cursor.visible);
  SK_ImGui_Cursor.update ();
}




HCURSOR
WINAPI
SetCursor_Detour (
  _In_opt_ HCURSOR hCursor )
{
  SK_LOG_FIRST_CALL

  SK_ImGui_Cursor.orig_img = hCursor;

  if (! SK_ImGui_Visible)
    return SetCursor_Original (hCursor);
  else if (! (ImGui::GetIO ().WantCaptureMouse || hCursor == nullptr))
    return SetCursor_Original (hCursor);

  return GetGameCursor ();
}

BOOL
WINAPI
GetCursorInfo_Detour (PCURSORINFO pci)
{
  SK_LOG_FIRST_CALL

  BOOL ret = GetCursorInfo_Original (pci);

  pci->hCursor = SK_ImGui_Cursor.orig_img;


  if (SK_ImGui_Visible)
  {
    if (SK_ImGui_WantMouseCapture ())
    {
      POINT client = SK_ImGui_Cursor.orig_pos;

      SK_ImGui_Cursor.LocalToScreen (&client);
      pci->ptScreenPos.x = client.x;
      pci->ptScreenPos.y = client.y;
    }

    else {
      POINT client = SK_ImGui_Cursor.pos;

      SK_ImGui_Cursor.LocalToScreen (&client);
      pci->ptScreenPos.x = client.x;
      pci->ptScreenPos.y = client.y;

      SK_ImGui_Cursor.ScreenToLocal (&client);
      SK_ImGui_Cursor.orig_pos = client;
    }

    return TRUE;
  }


  return ret;
}

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  SK_LOG_FIRST_CALL

  BOOL ret = GetCursorPos_Original (lpPoint);


  if (SK_ImGui_Visible)
  {
    if (SK_ImGui_WantMouseCapture ())
    {
      POINT client = SK_ImGui_Cursor.orig_pos;

      SK_ImGui_Cursor.LocalToScreen (&client);
      lpPoint->x = client.x;
      lpPoint->y = client.y;
    }

    else {
      POINT client = SK_ImGui_Cursor.pos;

      SK_ImGui_Cursor.LocalToScreen (&client);
      lpPoint->x = client.x;
      lpPoint->y = client.y;

      SK_ImGui_Cursor.ScreenToLocal (&client);
      SK_ImGui_Cursor.orig_pos = client;
    }

    return TRUE;
  }


  return ret;
}

BOOL
WINAPI
SetCursorPos_Detour (_In_ int x, _In_ int y)
{
  SK_LOG_FIRST_CALL

  // Game WANTED to change its position, so remember that.
  SK_ImGui_Cursor.orig_pos.x = x;
  SK_ImGui_Cursor.orig_pos.y = y;

  SK_ImGui_Cursor.ScreenToLocal (&SK_ImGui_Cursor.orig_pos);

  // Don't let the game continue moving the cursor while
  //   Alt+Tabbed out
  if (config.window.background_render && (! game_window.active))
    return TRUE;

  // Prevent Mouse Look while Drag Locked
  if (config.window.drag_lock)
    return TRUE;

  if (SK_ImGui_Visible)
  {
    //game_mouselook = SK_GetFramesDrawn ();
  }

  else {
    return SetCursorPos_Original (x, y);
  }

  return TRUE;
}

UINT
WINAPI
SendInput_Detour (
  _In_ UINT    nInputs,
  _In_ LPINPUT pInputs,
  _In_ int     cbSize
)
{
  SK_LOG_FIRST_CALL

  // TODO: Process this the right way...

  if (SK_ImGui_Visible)
  {
    return 0;
  }

  return SendInput_Original (nInputs, pInputs, cbSize);
}

VOID
WINAPI
mouse_event_Detour (
  _In_ DWORD     dwFlags,
  _In_ DWORD     dx,
  _In_ DWORD     dy,
  _In_ DWORD     dwData,
  _In_ ULONG_PTR dwExtraInfo
)
{
  SK_LOG_FIRST_CALL

// TODO: Process this the right way...

  if (SK_ImGui_Visible)
  {
    return;
  }

  mouse_event_Original (dwFlags, dx, dy, dwData, dwExtraInfo);
}


GetKeyState_pfn             GetKeyState_Original             = nullptr;
GetAsyncKeyState_pfn        GetAsyncKeyState_Original        = nullptr;
GetKeyboardState_pfn        GetKeyboardState_Original        = nullptr;

SHORT
WINAPI
GetAsyncKeyState_Detour (_In_ int vKey)
{
  SK_LOG_FIRST_CALL

#define SK_ConsumeVKey(vKey) { GetAsyncKeyState_Original(vKey); return 0; }

  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible ())
    SK_ConsumeVKey (vKey);

  // Block keyboard input to the game while it's in the background
  if (config.window.background_render && (! game_window.active))
    SK_ConsumeVKey (vKey);

  if (vKey >= 5)
  {
    if (SK_ImGui_WantKeyboardCapture () || SK_ImGui_WantTextCapture ())
      SK_ConsumeVKey (vKey);
  }

  else
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
      SK_ConsumeVKey (vKey);
  }

  return GetAsyncKeyState_Original (vKey);
}

SHORT
WINAPI
GetKeyState_Detour (_In_ int nVirtKey)
{
  SK_LOG_FIRST_CALL

#define SK_ConsumeVirtKey(nVirtKey) { GetKeyState_Original(nVirtKey); return 0; }

  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible ())
    SK_ConsumeVirtKey (nVirtKey);

  // Block keyboard input to the game while it's in the background
  if (config.window.background_render && (! game_window.active))
    SK_ConsumeVirtKey (nVirtKey);

  if (nVirtKey >= 5)
  {
    if (SK_ImGui_WantKeyboardCapture () || SK_ImGui_WantTextCapture ())
      SK_ConsumeVirtKey (nVirtKey);
  }

  else
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
      SK_ConsumeVirtKey (nVirtKey);
  }

  return GetKeyState_Original (nVirtKey);
}

//typedef BOOL (WINAPI *SetKeyboardState_pfn)(PBYTE lpKeyState); // TODO

BOOL
WINAPI
GetKeyboardState_Detour (PBYTE lpKeyState)
{
  SK_LOG_FIRST_CALL

  static BYTE last_keys [256] = { 0 };
         BYTE      keys [256] = { 0 };

  BOOL bRet = GetKeyboardState_Original (keys);

  if (bRet)
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ()) {
      last_keys [0] = 0; last_keys [1] = 0;
      last_keys [2] = 0; last_keys [3] = 0;
      last_keys [4] = 0;
    }

    memcpy (lpKeyState, last_keys, 256);

    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ()) {
      keys [0] = 0; keys [1] = 0;
      keys [2] = 0; keys [3] = 0;
      keys [4] = 0;
    }

    // Block keyboard input to the game while the console is active
    if (SK_Console::getInstance ()->isVisible ())
      return bRet;

    // Block keyboard input to the game while it's in the background
    if (config.window.background_render && (! game_window.active))
      return bRet;
    
    if (SK_ImGui_WantKeyboardCapture () || SK_ImGui_WantTextCapture ())
      return bRet;
  }

  memcpy (lpKeyState, keys, 256);
  memcpy (last_keys,  keys, 256);

  return bRet;
}

#include <Windowsx.h>


LRESULT
WINAPI
ImGui_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool
SK_ImGui_HandlesMessage (LPMSG lpMsg, bool remove)
{
  bool handled = false;

  switch (lpMsg->message)
  {
    //case WM_INPUT_DEVICE_CHANGE:
    case WM_INPUT:
    {
      RAWINPUT data = { 0 };
      UINT     size = sizeof RAWINPUT;

      int      ret  =
        GetRawInputData_Original ((HRAWINPUT)lpMsg->lParam, RID_HEADER, &data, &size, sizeof (RAWINPUTHEADER) );

      if (ret)
      {
        uint8_t *pData = new uint8_t [size];

        if (! pData)
          return 0;

        bool cap = SK_ImGui_ProcessRawInput ((HRAWINPUT)lpMsg->lParam, RID_INPUT, &data, &size, sizeof (data.header));

        switch (data.header.dwType)
        {
          case RIM_TYPEMOUSE:            
            //cap = SK_ImGui_WantMouseCapture ();
            break;

          case RIM_TYPEKEYBOARD:
            if (SK_ImGui_WantKeyboardCapture ())
              return true;
            break;

          default:
            if (nav_usable)
              return true;
        }
      }
    }
    break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
      if (nav_usable)
      {
        if (ImGui_WndProcHandler (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam))
          if (SK_ImGui_WantKeyboardCapture () && (! SK_ImGui_WantTextCapture ()))
          {
          // Don't capture release notifications, or games may think the key
          //   is stuck down indefinitely
          if (lpMsg->message != WM_KEYUP && lpMsg->message != WM_SYSKEYUP)
            return true;
          }
      } break;

    case WM_CHAR:
    case WM_UNICHAR:
    case WM_SYSDEADCHAR:
    case WM_DEADCHAR:
      if (nav_usable)
      {
        if (ImGui_WndProcHandler(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam))
          if (SK_ImGui_WantTextCapture ())
            return true;
      }
      break;


    case WM_CAPTURECHANGED:

    case WM_MOUSEHOVER:
    case WM_MOUSELEAVE:
    case WM_NCMOUSEHOVER:
    case WM_NCMOUSELEAVE:
      return false;

    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
      if (SK_ImGui_Visible)
      {
        bool filter = false;

        static POINTS last_pos;
        const short   threshold = 2;

        // Filter out small movements / mouselook warps
        //
        //   This does create a weird deadzone in the center of the screen,
        //     but most people will not notice ;)
        //
        if ( abs (last_pos.x - GET_X_LPARAM (lpMsg->lParam)) < threshold &&
             abs (last_pos.y - GET_Y_LPARAM (lpMsg->lParam)) < threshold )
          filter = true;

        last_pos = MAKEPOINTS (lpMsg->lParam);

        if (filter)
          return true;

        ImGui_WndProcHandler (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

        if (SK_ImGui_WantMouseCapture ())
          return true;
      } break;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEACTIVATE:
    case WM_MOUSEHWHEEL:
    case WM_MOUSEWHEEL:
    case WM_NCHITTEST:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONUP:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    case WM_NCXBUTTONDBLCLK:
    case WM_NCXBUTTONDOWN:
    case WM_NCXBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_XBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_LBUTTONUP:
      ImGui_WndProcHandler (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
      if (SK_ImGui_WantMouseCapture ())
          return true;
      break;
  }

  return handled;
}














void
SK_Input_Init (void)
{
  SK_Input_PreeHookHID   ();
  SK_Input_PreHookDI8    ();
  SK_Input_PreHookXInput ();

  SK_CreateDLLHook2 ( L"user32.dll", "GetRawInputData",
                     GetRawInputData_Detour,
           (LPVOID*)&GetRawInputData_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetAsyncKeyState",
                     GetAsyncKeyState_Detour,
           (LPVOID*)&GetAsyncKeyState_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetKeyState",
                     GetKeyState_Detour,
           (LPVOID*)&GetKeyState_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetKeyboardState",
                     GetKeyboardState_Detour,
           (LPVOID*)&GetKeyboardState_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetCursorPos",
                     GetCursorPos_Detour,
           (LPVOID*)&GetCursorPos_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetCursorInfo",
                     GetCursorInfo_Detour,
           (LPVOID*)&GetCursorInfo_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SetCursor",
                     SetCursor_Detour,
           (LPVOID*)&SetCursor_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SetCursorPos",
                     SetCursorPos_Detour,
           (LPVOID*)&SetCursorPos_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SendInput",
                     SendInput_Detour,
           (LPVOID*)&SendInput_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "mouse_event",
                     mouse_event_Detour,
           (LPVOID*)&mouse_event_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "RegisterRawInputDevices",
                     RegisterRawInputDevices_Detour,
           (LPVOID*)&RegisterRawInputDevices_Original );

#if 0
  SK_CreateDLLHook2 ( L"user32.dll", "GetRawInputBuffer",
                     GetRawInputBuffer_Detour,
           (LPVOID*)&GetRawInputBuffer_Original );
#endif

  MH_ApplyQueued ();
}
