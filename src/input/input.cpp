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

#define NOMINMAX

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


bool
SK_InputUtil_IsHWCursorVisible (void)
{
  CURSORINFO cursor_info;
             cursor_info.cbSize = sizeof (CURSORINFO);
  
  GetCursorInfo_Original (&cursor_info);
 
  return (cursor_info.flags & CURSOR_SHOWING);
}


#define SK_LOG_INPUT_CALL { static int  calls  = 0;                   { SK_LOG0 ( (L"[!] > Call #%lu: %hs", calls++, __FUNCTION__), L"Input Mgr." ); } }
#define SK_LOG_FIRST_CALL { static bool called = false; if (! called) { SK_LOG0 ( (L"[!] > First Call: %hs", __FUNCTION__), L"Input Mgr." ); called = true; } }

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
#include <Hidsdi.h>

bool
SK_HID_FilterPreparsedData (PHIDP_PREPARSED_DATA pData)
{
  bool filter = false;

  HIDP_CAPS caps;
  NTSTATUS  stat =
    HidP_GetCaps (pData, &caps);

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

  //SK_LOG0 ( ( L"HID Preparsed Data - Stat: %04x, UsagePage: %02x, Usage: %02x",
                //stat, caps.UsagePage, caps.Usage ),
              //L" HIDInput ");

  return filter;
}

typedef BOOLEAN (__stdcall *HidD_GetPreparsedData_pfn)(
  _In_  HANDLE                HidDeviceObject,
  _Out_ PHIDP_PREPARSED_DATA *PreparsedData
);

HidD_GetPreparsedData_pfn HidD_GetPreparsedData_Original = nullptr;

PHIDP_PREPARSED_DATA* SK_HID_PreparsedDataP = nullptr;
PHIDP_PREPARSED_DATA  SK_HID_PreparsedData = nullptr;

BOOLEAN
__stdcall
HidD_GetPreparsedData_Detour (
  _In_  HANDLE                HidDeviceObject,
  _Out_ PHIDP_PREPARSED_DATA *PreparsedData )
{
  SK_LOG_FIRST_CALL

  PHIDP_PREPARSED_DATA pData;
  BOOL bRet = HidD_GetPreparsedData_Original (HidDeviceObject, &pData);

  if (bRet)
  {
    SK_HID_PreparsedDataP = PreparsedData;
    SK_HID_PreparsedData  = pData;

    if (SK_HID_FilterPreparsedData (pData) || config.input.gamepad.disable_ps4_hid)
      return FALSE;

    *PreparsedData   =  pData;
  }

  // Can't figure out how The Witness works yet, but it will bypass input blocking
  //   on HID using a PS4 controller unless we return FALSE here.
  //return FALSE;
  return bRet;
}

typedef BOOLEAN (__stdcall *HidD_FreePreparsedData_pfn)(
  _In_ PHIDP_PREPARSED_DATA PreparsedData
);

HidD_FreePreparsedData_pfn HidD_FreePreparsedData_Original = nullptr;

BOOLEAN
__stdcall
HidD_FreePreparsedData_Detour (
  _In_ PHIDP_PREPARSED_DATA PreparsedData )
{
  BOOLEAN bRet = HidD_FreePreparsedData_Original (PreparsedData);

  if (PreparsedData == SK_HID_PreparsedData)
    SK_HID_PreparsedData = nullptr;

  return bRet;
}

typedef BOOLEAN (__stdcall *HidD_GetFeature_pfn)(
  _In_  HANDLE HidDeviceObject,
  _Out_ PVOID  ReportBuffer,
  _In_  ULONG  ReportBufferLength
);

HidD_GetFeature_pfn HidD_GetFeature_Original = nullptr;

BOOLEAN
__stdcall
HidD_GetFeature_Detour ( _In_  HANDLE HidDeviceObject,
                         _Out_ PVOID  ReportBuffer,
                         _In_  ULONG  ReportBufferLength )
{
  bool filter = false;

  PHIDP_PREPARSED_DATA pData;
  if (HidD_GetPreparsedData_Original (HidDeviceObject, &pData))
  {
    if (SK_HID_FilterPreparsedData (pData))
      filter = true;

    HidD_FreePreparsedData (pData);
  }

  if (! filter)
    return HidD_GetFeature_Original ( HidDeviceObject, ReportBuffer, ReportBufferLength );

  return FALSE;
}

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
    filter = SK_HID_FilterPreparsedData (PreparsedData);
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
  if (! config.input.gamepad.hook_hid)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedExchangeAdd (&hooked, 0))
  {
    SK_CreateDLLHook2 ( L"HID.DLL", "HidP_GetData",
                          HidP_GetData_Detour,
                (LPVOID*)&HidP_GetData_Original );

    SK_CreateDLLHook2 ( L"HID.DLL", "HidD_GetPreparsedData",
                          HidD_GetPreparsedData_Detour,
                (LPVOID*)&HidD_GetPreparsedData_Original );

    SK_CreateDLLHook2 ( L"HID.DLL", "HidD_FreePreparsedData",
                          HidD_FreePreparsedData_Detour,
                (LPVOID*)&HidD_FreePreparsedData_Original );

    SK_CreateDLLHook2 ( L"HID.DLL", "HidD_GetFeature",
                          HidD_GetFeature_Detour,
                (LPVOID*)&HidD_GetFeature_Original );

    MH_ApplyQueued ();

    if (HidP_GetData_Original != nullptr)
      InterlockedIncrement (&hooked);
  }
}

void
SK_Input_PreHookHID (void)
{
  if (! config.input.gamepad.hook_hid)
    return;

  static sk_import_test_s tests [] = { { "hid.dll", false } };

  SK_TestImports (GetModuleHandle (nullptr), tests, 1);

  if (tests [0].used)// || GetModuleHandle (L"hid.dll"))
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
  SK_LOG_FIRST_CALL

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
                          _In_      UINT      cbSizeHeader,
                                    BOOL      self);

UINT
WINAPI
GetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                        _In_      UINT      uiCommand,
                        _Out_opt_ LPVOID    pData,
                        _Inout_   PUINT     pcbSize,
                        _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  //if (SK_ImGui_WantGamepadCapture ())
  return SK_ImGui_ProcessRawInput (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader, false);

 //return GetRawInputData_Original (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
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
        IDirectInput8_CreateDevice_Original               = nullptr;

IDirectInputDevice8_GetDeviceState_pfn
        IDirectInputDevice8_GetDeviceState_MOUSE_Original = nullptr;

IDirectInputDevice8_GetDeviceState_pfn
        IDirectInputDevice8_GetDeviceState_KEYBOARD_Original = nullptr;

IDirectInputDevice8_GetDeviceState_pfn
        IDirectInputDevice8_GetDeviceState_GAMEPAD_Original = nullptr;

IDirectInputDevice8_SetCooperativeLevel_pfn
        IDirectInputDevice8_SetCooperativeLevel_Original  = nullptr;


struct SK_DI8_Keyboard {
  LPDIRECTINPUTDEVICE pDev = nullptr;
  uint8_t             state [512];
} _dik;

struct SK_DI8_Mouse {
  LPDIRECTINPUTDEVICE pDev = nullptr;
  DIMOUSESTATE2       state;
} _dim;


__declspec (noinline)
SK_DI8_Keyboard*
WINAPI
SK_Input_GetDI8Keyboard (void) {
  return &_dik;
}

__declspec (noinline)
SK_DI8_Mouse*
WINAPI
SK_Input_GetDI8Mouse (void) {
  return &_dim;
}



HRESULT
WINAPI
IDirectInputDevice8_GetDeviceState_Detour ( LPDIRECTINPUTDEVICE        This,
                                            DWORD                      cbData,
                                            LPVOID                     lpvData )
{
  SK_LOG_FIRST_CALL

  SK_LOG4 ( ( L" DirectInput 8 - GetDeviceState: cbData = %lu",
                cbData ),
              L"Direct Inp" );

  HRESULT hr = S_OK;
  //hr = IDirectInputDevice8_GetDeviceState_Original ( This,
                                                       //cbData,
                                                         //lpvData );



  if (SUCCEEDED (hr) && lpvData != nullptr)
  {
    if (cbData == sizeof DIJOYSTATE2) 
    {
      static DIJOYSTATE2 last_state;

      DIJOYSTATE2* out = (DIJOYSTATE2 *)lpvData;

      if (nav_usable)
      {
        memcpy (out, &last_state, cbData);

        out->rgdwPOV [0] = -1;
        out->rgdwPOV [1] = -1;
        out->rgdwPOV [2] = -1;
        out->rgdwPOV [3] = -1;
      } else
        memcpy (&last_state, out, cbData);

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

    else if (cbData == sizeof DIJOYSTATE) 
    {
      //dll_log.Log (L"Joy");

      static DIJOYSTATE last_state;

      DIJOYSTATE* out = (DIJOYSTATE *)lpvData;

      if (nav_usable)
      {
        memcpy (out, &last_state, cbData);

        out->rgdwPOV [0] = -1;
        out->rgdwPOV [1] = -1;
        out->rgdwPOV [2] = -1;
        out->rgdwPOV [3] = -1;
      }
      else
        memcpy (&last_state, out, cbData);
    }

    else if (This == _dik.pDev || cbData == 256)
    {
      //static uint8_t last_keys [256] = { '\0' };

      if (SK_ImGui_WantKeyboardCapture () && lpvData != nullptr)
      {
        memset (lpvData, 0, cbData);
      }

      //else 
        //memcpy (last_keys, lpvData, 256);
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

bool
SK_DInput8_HasKeyboard (void)
{
  return (_dik.pDev && IDirectInputDevice8_SetCooperativeLevel_Original);
}
bool
SK_DInput8_BlockWindowsKey (bool block)
{
  DWORD dwFlags =
    block ? DISCL_NOWINKEY : 0x0;

  dwFlags &= ~DISCL_EXCLUSIVE;
  dwFlags &= ~DISCL_BACKGROUND;

  dwFlags |= DISCL_NONEXCLUSIVE;
  dwFlags |= DISCL_FOREGROUND;

  if (SK_DInput8_HasKeyboard ())
    IDirectInputDevice8_SetCooperativeLevel_Original (_dik.pDev, game_window.hWnd, dwFlags);
  else
    return false;

  return block;
}

//
// TODO: Create a wrapper instead of flat hooks like this, this won't work when
//         multiple hardware vendor devices are present.
//

HRESULT
WINAPI
IDirectInputDevice8_GetDeviceState_MOUSE_Detour ( LPDIRECTINPUTDEVICE        This,
                                                  DWORD                      cbData,
                                                  LPVOID                     lpvData )
{
  HRESULT hr = IDirectInputDevice8_GetDeviceState_MOUSE_Original ( This, cbData, lpvData );

  if (SUCCEEDED (hr))
    IDirectInputDevice8_GetDeviceState_Detour ( This, cbData, lpvData );

  return hr;
}

HRESULT
WINAPI
IDirectInputDevice8_GetDeviceState_KEYBOARD_Detour ( LPDIRECTINPUTDEVICE        This,
                                                     DWORD                      cbData,
                                                     LPVOID                     lpvData )
{
  HRESULT hr = IDirectInputDevice8_GetDeviceState_KEYBOARD_Original ( This, cbData, lpvData );

  if (SUCCEEDED (hr))
    IDirectInputDevice8_GetDeviceState_Detour ( This, cbData, lpvData );

  return hr;
}

HRESULT
WINAPI
IDirectInputDevice8_GetDeviceState_GAMEPAD_Detour ( LPDIRECTINPUTDEVICE        This,
                                                    DWORD                      cbData,
                                                    LPVOID                     lpvData )
{
  HRESULT hr = IDirectInputDevice8_GetDeviceState_GAMEPAD_Original ( This, cbData, lpvData );

  if (SUCCEEDED (hr))
    IDirectInputDevice8_GetDeviceState_Detour ( This, cbData, lpvData );

  return hr;
}

HRESULT
WINAPI
IDirectInputDevice8_SetCooperativeLevel_Detour ( LPDIRECTINPUTDEVICE  This,
                                                 HWND                 hwnd,
                                                 DWORD                dwFlags )
{
  if (config.input.keyboard.block_windows_key)
    dwFlags |= DISCL_NOWINKEY;

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

  if (SUCCEEDED (hr))
  {
    void** vftable = *(void***)*lplpDirectInputDevice;

    if (rguid == GUID_SysMouse)
    {
      SK_CreateFuncHook ( L"IDirectInputDevice8::GetDeviceState",
                           vftable [9],
                           IDirectInputDevice8_GetDeviceState_MOUSE_Detour,
                (LPVOID *)&IDirectInputDevice8_GetDeviceState_MOUSE_Original );
    }

    else if (rguid == GUID_SysKeyboard)
    {
      SK_CreateFuncHook ( L"IDirectInputDevice8::GetDeviceState",
                           vftable [9],
                           IDirectInputDevice8_GetDeviceState_KEYBOARD_Detour,
                (LPVOID *)&IDirectInputDevice8_GetDeviceState_KEYBOARD_Original );
    }

    else
    {
      SK_CreateFuncHook ( L"IDirectInputDevice8::GetDeviceState",
                           vftable [9],
                           IDirectInputDevice8_GetDeviceState_GAMEPAD_Detour,
                (LPVOID *)&IDirectInputDevice8_GetDeviceState_GAMEPAD_Original );
    }

    MH_QueueEnableHook (vftable [9]);

    if (rguid == GUID_SysKeyboard)
    {
      SK_CreateFuncHook ( L"IDirectInputDevice8::SetCooperativeLevel",
                           vftable [13],
                           IDirectInputDevice8_SetCooperativeLevel_Detour,
                 (LPVOID*)&IDirectInputDevice8_SetCooperativeLevel_Original );
      MH_QueueEnableHook (vftable [13]);
    }

    MH_ApplyQueued ();

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
  if (! config.input.gamepad.hook_dinput8)
    return;

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
  if (! config.input.gamepad.hook_dinput8)
    return;

  if (DirectInput8Create_Original == nullptr)
  {
    static sk_import_test_s tests [] = { { "dinput.dll",  false },
                                         { "dinput8.dll", false } };

    SK_TestImports (GetModuleHandle (nullptr), tests, 2);

    if (tests [0].used || tests [1].used)// || GetModuleHandle (L"dinput8.dll"))
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

struct SK_XInputContext
{
  struct instance_s
  {
    const wchar_t*                  wszModuleName                        =     L"";
    HMODULE                         hMod                                 =       0;

    XInputGetState_pfn              XInputGetState_Detour                = nullptr;
    XInputGetState_pfn              XInputGetState_Original              = nullptr;
    LPVOID                          XInputGetState_Target                = nullptr;

    uint8_t                         orig_inst    [64]                    =   { 0 };
    LPVOID                          orig_addr                            =     0  ;

    XInputGetCapabilities_pfn       XInputGetCapabilities_Detour         = nullptr;
    XInputGetCapabilities_pfn       XInputGetCapabilities_Original       = nullptr;
    LPVOID                          XInputGetCapabilities_Target         = nullptr;

    uint8_t                         orig_inst_caps [64]                  =   { 0 };
    LPVOID                          orig_addr_caps                       =     0  ;

    XInputGetBatteryInformation_pfn XInputGetBatteryInformation_Detour   = nullptr;
    XInputGetBatteryInformation_pfn XInputGetBatteryInformation_Original = nullptr;
    LPVOID                          XInputGetBatteryInformation_Target   = nullptr;

    uint8_t                         orig_inst_batt [64]                  =   { 0 };
    LPVOID                          orig_addr_batt                       =     0  ;

    XInputSetState_pfn              XInputSetState_Detour                = nullptr;
    XInputSetState_pfn              XInputSetState_Original              = nullptr;
    LPVOID                          XInputSetState_Target                = nullptr;

    uint8_t                         orig_inst_set [64]                   =   { 0 };
    LPVOID                          orig_addr_set                        =     0  ;

    //
    // Extended stuff (XInput1_3 and XInput1_4 ONLY)
    //
    XInputGetStateEx_pfn            XInputGetStateEx_Detour              = nullptr;
    XInputGetStateEx_pfn            XInputGetStateEx_Original            = nullptr;
    LPVOID                          XInputGetStateEx_Target              = nullptr;

    uint8_t                         orig_inst_ex [64]                    =   { 0 };
    LPVOID                          orig_addr_ex                         =     0  ;
  } XInput1_3 { 0 }, XInput1_4 { 0 }, XInput9_1_0 { 0 };

  instance_s* primary_hook = nullptr;
} xinput_ctx;

#include <SpecialK/input/xinput_hotplug.h>

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
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetState_Original   (dwUserIndex, pState);

    SK_ImGui_FilterXInput           (dwUserIndex, pState);
  SK_XInput_PacketJournalize (dwRet, dwUserIndex, pState);

  dwRet =
    SK_XInput_PlaceHold      (dwRet, dwUserIndex, pState);

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputGetStateEx1_3_Detour (
  _In_  DWORD            dwUserIndex,
  _Out_ XINPUT_STATE_EX *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetStateEx_Original   (dwUserIndex, pState);

    SK_ImGui_FilterXInput             (dwUserIndex, (XINPUT_STATE *)pState);
    SK_XInput_PacketJournalize (dwRet, dwUserIndex, (XINPUT_STATE *)pState);

  dwRet =
    SK_XInput_PlaceHoldEx      (dwRet, dwUserIndex, pState);

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputGetCapabilities1_3_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetCapabilities_Original (dwUserIndex, dwFlags, pCapabilities);

  dwRet =
    SK_XInput_PlaceHoldCaps (dwRet, dwUserIndex, dwFlags, pCapabilities);

  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputGetBatteryInformation1_3_Detour (
  _In_  DWORD                       dwUserIndex,
  _In_  BYTE                        devType,
  _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetBatteryInformation_Original (dwUserIndex, devType, pBatteryInformation);

  dwRet =
    SK_XInput_PlaceHoldBattery (dwRet, dwUserIndex, devType, pBatteryInformation);

  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputSetState1_3_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  bool nop = SK_ImGui_WantGamepadCapture () && config.input.gamepad.haptic_ui;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
      nop ? ERROR_SUCCESS :
            pCtx->XInputSetState_Original (dwUserIndex, pVibration);

  dwRet =
    SK_XInput_PlaceHoldSet (dwRet, dwUserIndex, pVibration);

  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}


DWORD
WINAPI
XInputGetState1_4_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetState_Original   (dwUserIndex, pState);

    SK_ImGui_FilterXInput           (dwUserIndex, pState);
  SK_XInput_PacketJournalize (dwRet, dwUserIndex, pState);

  dwRet =
    SK_XInput_PlaceHold      (dwRet, dwUserIndex, pState);

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputGetStateEx1_4_Detour (
  _In_  DWORD            dwUserIndex,
  _Out_ XINPUT_STATE_EX *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetStateEx_Original        (dwUserIndex, pState);

    SK_ImGui_FilterXInput           (       dwUserIndex, (XINPUT_STATE *)pState);
    SK_XInput_PacketJournalize      (dwRet, dwUserIndex, (XINPUT_STATE *)pState);

  dwRet =
    SK_XInput_PlaceHoldEx           (dwRet, dwUserIndex, pState);

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputGetCapabilities1_4_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetCapabilities_Original (dwUserIndex, dwFlags, pCapabilities);

  dwRet =
    SK_XInput_PlaceHoldCaps (dwRet, dwUserIndex, dwFlags, pCapabilities);

  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputGetBatteryInformation1_4_Detour (
  _In_  DWORD                       dwUserIndex,
  _In_  BYTE                        devType,
  _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetBatteryInformation_Original (dwUserIndex, devType, pBatteryInformation);

  dwRet =
    SK_XInput_PlaceHoldBattery (dwRet, dwUserIndex, devType, pBatteryInformation);

  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputSetState1_4_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  bool nop = SK_ImGui_WantGamepadCapture () && config.input.gamepad.haptic_ui;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
      nop ? ERROR_SUCCESS :
            pCtx->XInputSetState_Original (dwUserIndex, pVibration);

  dwRet =
    SK_XInput_PlaceHoldSet (dwRet, dwUserIndex, pVibration);

  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}


DWORD
WINAPI
XInputGetState9_1_0_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput9_1_0;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetState_Original   (dwUserIndex, pState);

    SK_ImGui_FilterXInput           (dwUserIndex, pState);
  SK_XInput_PacketJournalize (dwRet, dwUserIndex, pState);

  dwRet =
    SK_XInput_PlaceHold      (dwRet, dwUserIndex, pState);

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputGetCapabilities9_1_0_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput9_1_0;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetCapabilities_Original (dwUserIndex, dwFlags, pCapabilities);

  dwRet =
    SK_XInput_PlaceHoldCaps (dwRet, dwUserIndex, dwFlags, pCapabilities);

  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}

DWORD
WINAPI
XInputSetState9_1_0_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  SK_LOG_FIRST_CALL

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput9_1_0;

  bool nop = SK_ImGui_WantGamepadCapture () && config.input.gamepad.haptic_ui;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
      nop ?
        ERROR_SUCCESS :
        pCtx->XInputSetState_Original (dwUserIndex, pVibration);

  dwRet =
    SK_XInput_PlaceHoldSet (dwRet, dwUserIndex, pVibration);

  xinput_ctx.primary_hook = pCtx;

  return dwRet;
}


void
SK_Input_HookXInputContext (SK_XInputContext::instance_s* pCtx)
{
  pCtx->orig_addr = SK_GetProcAddress (pCtx->wszModuleName, "XInputGetState");

  SK_CreateDLLHook2 ( pCtx->wszModuleName,
                       "XInputGetState",
                       pCtx->XInputGetState_Detour,
            (LPVOID *)&pCtx->XInputGetState_Original,
            (LPVOID *)&pCtx->XInputGetState_Target );

  MH_QueueEnableHook (pCtx->XInputGetState_Target);


  pCtx->orig_addr_caps = SK_GetProcAddress (pCtx->wszModuleName, "XInputGetCapabilities");

  SK_CreateDLLHook2 ( pCtx->wszModuleName,
                       "XInputGetCapabilities",
                       pCtx->XInputGetCapabilities_Detour,
            (LPVOID *)&pCtx->XInputGetCapabilities_Original,
            (LPVOID *)&pCtx->XInputGetCapabilities_Target );

  MH_QueueEnableHook (pCtx->XInputGetCapabilities_Target);


  pCtx->orig_addr_batt = SK_GetProcAddress (pCtx->wszModuleName, "XInputGetBatteryInformation");

  // Down-level (XInput 9_1_0) does not have XInputGetBatteryInformation
  //
  if (pCtx->orig_addr_batt != nullptr)
  {
    SK_CreateDLLHook2 ( pCtx->wszModuleName,
                         "XInputGetBatteryInformation",
                         pCtx->XInputGetBatteryInformation_Detour,
              (LPVOID *)&pCtx->XInputGetBatteryInformation_Original,
              (LPVOID *)&pCtx->XInputGetBatteryInformation_Target );

    MH_QueueEnableHook (pCtx->XInputGetBatteryInformation_Target);
  }


  pCtx->orig_addr_set = SK_GetProcAddress (pCtx->wszModuleName, "XInputSetState");

  SK_CreateDLLHook2 ( pCtx->wszModuleName,
                       "XInputSetState",
                       pCtx->XInputSetState_Detour,
            (LPVOID *)&pCtx->XInputSetState_Original,
            (LPVOID *)&pCtx->XInputSetState_Target );

  MH_QueueEnableHook (pCtx->XInputSetState_Target);


  pCtx->orig_addr_ex = SK_GetProcAddress (pCtx->wszModuleName, XINPUT_GETSTATEEX_ORDINAL);

  // Down-level (XInput 9_1_0) does not have XInputGetStateEx (100)
  //
  if (pCtx->orig_addr_ex != nullptr)
  {
    SK_CreateDLLHook2 ( pCtx->wszModuleName,
                         XINPUT_GETSTATEEX_ORDINAL,
                         pCtx->XInputGetStateEx_Detour,
              (LPVOID *)&pCtx->XInputGetStateEx_Original,
              (LPVOID *)&pCtx->XInputGetStateEx_Target );

    MH_QueueEnableHook (pCtx->XInputGetStateEx_Target);
  }

  MH_ApplyQueued ();

  if (pCtx->orig_addr != nullptr)
    memcpy (pCtx->orig_inst, pCtx->orig_addr, 64);

  if (pCtx->orig_addr_caps != nullptr)
    memcpy (pCtx->orig_inst_caps, pCtx->orig_addr_caps, 64);

  if (pCtx->orig_addr_batt != nullptr)
    memcpy (pCtx->orig_inst_batt, pCtx->orig_addr_batt, 64);

  if (pCtx->orig_addr_set != nullptr)
    memcpy (pCtx->orig_inst_set, pCtx->orig_addr_set, 64);

  if (pCtx->orig_addr_ex != nullptr)
    memcpy (pCtx->orig_inst_ex, pCtx->orig_addr_ex, 64);
}

void
SK_Input_HookXInput1_4 (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedExchangeAdd (&hooked, 0))
  {
    SK_XInputContext::instance_s* pCtx =
      &xinput_ctx.XInput1_4;

    SK_LOG0 ( ( L"  >> Hooking XInput 1.4" ),
                L"   Input  " );

    pCtx->wszModuleName                      = L"XInput1_4.dll";
    pCtx->hMod                               = GetModuleHandle (pCtx->wszModuleName);
    pCtx->XInputGetState_Detour              = XInputGetState1_4_Detour;
    pCtx->XInputGetStateEx_Detour            = XInputGetStateEx1_4_Detour;
    pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities1_4_Detour;
    pCtx->XInputSetState_Detour              = XInputSetState1_4_Detour;
    pCtx->XInputGetBatteryInformation_Detour = XInputGetBatteryInformation1_4_Detour;

    SK_Input_HookXInputContext (pCtx);

    if (xinput_ctx.primary_hook == nullptr)
      xinput_ctx.primary_hook = &xinput_ctx.XInput1_4;

    InterlockedIncrement (&hooked);
  }
}

void
SK_Input_HookXInput1_3 (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedExchangeAdd (&hooked, 0))
  {
    SK_XInputContext::instance_s* pCtx =
      &xinput_ctx.XInput1_3;

    SK_LOG0 ( ( L"  >> Hooking XInput 1.3" ),
                L"   Input  " );

    pCtx->wszModuleName                      = L"XInput1_3.dll";
    pCtx->hMod                               = GetModuleHandle (pCtx->wszModuleName);
    pCtx->XInputGetState_Detour              = XInputGetState1_3_Detour;
    pCtx->XInputGetStateEx_Detour            = XInputGetStateEx1_3_Detour;
    pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities1_3_Detour;
    pCtx->XInputSetState_Detour              = XInputSetState1_3_Detour;
    pCtx->XInputGetBatteryInformation_Detour = XInputGetBatteryInformation1_3_Detour;

    SK_Input_HookXInputContext (pCtx);

    if (xinput_ctx.primary_hook == nullptr)
      xinput_ctx.primary_hook = &xinput_ctx.XInput1_3;

    InterlockedIncrement (&hooked);
  }
}

void
SK_Input_HookXInput9_1_0 (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedExchangeAdd (&hooked, 0))
  {
    SK_XInputContext::instance_s* pCtx =
      &xinput_ctx.XInput9_1_0;

    SK_LOG0 ( ( L"  >> Hooking XInput9_1_0" ),
                L"   Input  " );

    pCtx->wszModuleName                      = L"XInput9_1_0.dll";
    pCtx->hMod                               = GetModuleHandle (pCtx->wszModuleName);
    pCtx->XInputGetState_Detour              = XInputGetState9_1_0_Detour;
    pCtx->XInputGetStateEx_Detour            = nullptr; // Not supported
    pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities9_1_0_Detour;
    pCtx->XInputSetState_Detour              = XInputSetState9_1_0_Detour;
    pCtx->XInputGetBatteryInformation_Detour = nullptr; // Not supported

    SK_Input_HookXInputContext (pCtx);

    if (xinput_ctx.primary_hook == nullptr)
      xinput_ctx.primary_hook = &xinput_ctx.XInput9_1_0;

    InterlockedIncrement (&hooked);
  }
}


void
SK_XInput_RehookIfNeeded (void)
{
  // This shouldn't be needed now that we know SDL uses XInputGetStateEx (ordinal=100),
  //   but may improve compatibility with X360ce.
  if (! config.input.gamepad.rehook_xinput)
    return;

  SK_XInputContext::instance_s* pCtx =
    xinput_ctx.primary_hook;

  MH_STATUS ret = 
    MH_EnableHook (pCtx->XInputGetState_Target);


  // Test for modified hooks
  if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                 ( pCtx->orig_addr != 0 &&
           memcmp (pCtx->orig_inst,
                   pCtx->orig_addr, 64 ) )
     )
  {
    if ( MH_OK == MH_RemoveHook (pCtx->XInputGetState_Target) )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2 (  pCtx->wszModuleName, "XInputGetState",
                                  pCtx->XInputGetState_Detour,
                       (LPVOID *)&pCtx->XInputGetState_Original,
                       (LPVOID *)&pCtx->XInputGetState_Target )
         )
      {
        MH_QueueEnableHook (pCtx->XInputGetState_Target);

        SK_LOG0 ( ( L" Re-hooked XInput using '%s'...",
                       pCtx->wszModuleName ),
                    L"Input Mgr." );

        if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
          xinput_ctx.XInput1_3 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
          xinput_ctx.XInput1_4 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput9_1_0.hMod)
          xinput_ctx.XInput9_1_0 = *pCtx;
      }

      else
      {
        SK_LOG0 ( ( L" Failed to re-hook XInput using '%s'...",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }

    else
    {
      SK_LOG0 ( ( L" Failed to remove XInput hook from '%s'...",
             pCtx->wszModuleName ),
          L"Input Mgr." );
    }
  }


  ret =
    MH_EnableHook (pCtx->XInputSetState_Target);


  // Test for modified hooks
  if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                 ( pCtx->orig_addr_set != 0 &&
           memcmp (pCtx->orig_inst_set,
                   pCtx->orig_addr_set, 64 ) )
     )
  {
    if ( MH_OK == MH_RemoveHook (pCtx->XInputSetState_Target) )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2 (  pCtx->wszModuleName, "XInputSetState",
                                  pCtx->XInputSetState_Detour,
                       (LPVOID *)&pCtx->XInputSetState_Original,
                       (LPVOID *)&pCtx->XInputSetState_Target )
         )
      {
        MH_QueueEnableHook (pCtx->XInputSetState_Target);

        SK_LOG0 ( ( L" Re-hooked XInput (Set) using '%s'...",
                       pCtx->wszModuleName ),
                    L"Input Mgr." );

        if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
          xinput_ctx.XInput1_3 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
          xinput_ctx.XInput1_4 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput9_1_0.hMod)
          xinput_ctx.XInput9_1_0 = *pCtx;
      }

      else
      {
        SK_LOG0 ( ( L" Failed to re-hook XInput (Set) using '%s'...",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }

    else
    {
      SK_LOG0 ( ( L" Failed to remove XInput (Set) hook from '%s'...",
             pCtx->wszModuleName ),
          L"Input Mgr." );
    }
  }


  ret =
    MH_EnableHook (pCtx->XInputGetCapabilities_Target);


  // Test for modified hooks
  if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                 ( pCtx->orig_addr_caps != 0 &&
           memcmp (pCtx->orig_inst_caps,
                   pCtx->orig_addr_caps, 64 ) )
     )
  {
    if ( MH_OK == MH_RemoveHook (pCtx->XInputGetCapabilities_Target) )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2 (  pCtx->wszModuleName, "XInputGetCapabilities",
                                  pCtx->XInputGetCapabilities_Detour,
                       (LPVOID *)&pCtx->XInputGetCapabilities_Original,
                       (LPVOID *)&pCtx->XInputGetCapabilities_Target )
         )
      {
        MH_QueueEnableHook (pCtx->XInputGetCapabilities_Target);

        SK_LOG0 ( ( L" Re-hooked XInput (Caps) using '%s'...",
                       pCtx->wszModuleName ),
                    L"Input Mgr." );

        if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
          xinput_ctx.XInput1_3 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
          xinput_ctx.XInput1_4 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput9_1_0.hMod)
          xinput_ctx.XInput9_1_0 = *pCtx;
      }

      else
      {
        SK_LOG0 ( ( L" Failed to re-hook XInput (Caps) using '%s'...",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }

    else
    {
      SK_LOG0 ( ( L" Failed to remove XInput (Caps) hook from '%s'...",
             pCtx->wszModuleName ),
          L"Input Mgr." );
    }
  }


  // XInput9_1_0 won't have this, so skip it.
  if (pCtx->XInputGetBatteryInformation_Target != nullptr)
  {
    ret =
      MH_EnableHook (pCtx->XInputGetBatteryInformation_Target);


    // Test for modified hooks
    if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                   ( pCtx->orig_addr_batt != 0 &&
             memcmp (pCtx->orig_inst_batt,
                     pCtx->orig_addr_batt, 64 ) )
       )
    {
      if ( MH_OK == MH_RemoveHook (pCtx->XInputGetBatteryInformation_Target) )
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 (  pCtx->wszModuleName, "XInputGetBatteryInformation",
                                    pCtx->XInputGetBatteryInformation_Detour,
                         (LPVOID *)&pCtx->XInputGetBatteryInformation_Original,
                         (LPVOID *)&pCtx->XInputGetBatteryInformation_Target )
           )
        {
          MH_QueueEnableHook (pCtx->XInputGetBatteryInformation_Target);

          SK_LOG0 ( ( L" Re-hooked XInput (Battery) using '%s'...",
                         pCtx->wszModuleName ),
                      L"Input Mgr." );

          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput9_1_0.hMod)
            xinput_ctx.XInput9_1_0 = *pCtx;
        }

        else
        {
          SK_LOG0 ( ( L" Failed to re-hook XInput (Battery) using '%s'...",
                 pCtx->wszModuleName ),
              L"Input Mgr." );
        }
      }

      else
      {
        SK_LOG0 ( ( L" Failed to remove XInput (Battery) hook from '%s'...",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }
  }


  // XInput9_1_0 won't have this, so skip it.
  if (pCtx->XInputGetStateEx_Target != nullptr)
  {
    ret = 
      MH_EnableHook (pCtx->XInputGetStateEx_Target);

    // Test for modified hooks
    if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                   ( pCtx->orig_addr_ex != 0 &&
             memcmp (pCtx->orig_inst_ex,
                     pCtx->orig_addr_ex, 64 ) )
       )
    {
      if ( MH_OK == MH_RemoveHook (pCtx->XInputGetStateEx_Target) )
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 (  pCtx->wszModuleName, XINPUT_GETSTATEEX_ORDINAL,
                                    pCtx->XInputGetStateEx_Detour,
                         (LPVOID *)&pCtx->XInputGetStateEx_Original,
                         (LPVOID *)&pCtx->XInputGetStateEx_Target )
           )
        {
          MH_QueueEnableHook (pCtx->XInputGetStateEx_Target);

          SK_LOG0 ( ( L" Re-hooked XInput (Ex) using '%s'...",
                         pCtx->wszModuleName ),
                      L"Input Mgr." );

          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput9_1_0.hMod)
            xinput_ctx.XInput9_1_0 = *pCtx;
        }

        else
        {
          SK_LOG0 ( ( L" Failed to re-hook XInput (Ex) using '%s'...",
                 pCtx->wszModuleName ),
              L"Input Mgr." );
        }
      }

      else
      {
        SK_LOG0 ( ( L" Failed to remove XInput (Ex) hook from '%s'...",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }
  }

  MH_ApplyQueued ();

  if (pCtx->orig_addr != nullptr)
    memcpy (pCtx->orig_inst, pCtx->orig_addr, 64);

  if (pCtx->orig_addr_set != nullptr)
    memcpy (pCtx->orig_inst_set, pCtx->orig_addr_set, 64);

  if (pCtx->orig_addr_caps != nullptr)
    memcpy (pCtx->orig_inst_caps, pCtx->orig_addr_caps, 64);

  if (pCtx->orig_addr_batt != nullptr)
    memcpy (pCtx->orig_inst_batt, pCtx->orig_addr_batt, 64);

  if (pCtx->orig_addr_ex != nullptr)
    memcpy (pCtx->orig_inst_ex, pCtx->orig_addr_ex, 64);
}

bool
SK_XInput_PulseController ( INT   iJoyID,
                            float fStrengthLeft,
                            float fStrengthRight )
{
  XINPUT_VIBRATION vibes;
  vibes.wLeftMotorSpeed  = (WORD)(std::min (0.99999f, fStrengthLeft)  * 65535.0f);
  vibes.wRightMotorSpeed = (WORD)(std::min (0.99999f, fStrengthRight) * 65535.0f);

  if (xinput_ctx.primary_hook && xinput_ctx.primary_hook->XInputSetState_Original) {
    xinput_ctx.primary_hook->XInputSetState_Original ( iJoyID, &vibes );
    return true;
  }

  return false;
}

bool
SK_XInput_PollController ( INT           iJoyID,
                           XINPUT_STATE* pState )
{
  SK_XInputContext::instance_s* pCtx =
    xinput_ctx.primary_hook;

  static bool first_frame = true;

  if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr)
  {
    static bool tried_to_hook = false;

    if (tried_to_hook)
      return false;

    tried_to_hook = true;

    // First try 1.3, that's generally available.
    SK_Input_HookXInput1_3 ();
    pCtx = xinput_ctx.primary_hook;

    // Then 1.4
    if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr) {
      SK_Input_HookXInput1_4 ();
      pCtx = xinput_ctx.primary_hook;
    }

    // Down-level 9_1_0 if all else fails (does not support XInputEx)
    if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr) {
      SK_Input_HookXInput9_1_0 ();
      pCtx = xinput_ctx.primary_hook;
    }


    // No XInput?! User shouldn't be playing games :P
    if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr)
    {
      SK_LOG0 ( ( L"Unable to hook XInput, attempting to enter limp-mode..."
                  L" input-related features may not work as intended." ),
                  L"Input Mgr." );
      xinput_ctx.primary_hook =
        &xinput_ctx.XInput1_3;
      pCtx = xinput_ctx.primary_hook;
      pCtx->XInputGetState_Original =
        (XInputGetState_pfn)
          GetProcAddress ( LoadLibrary (L"XInput1_3.dll"),
                           "XInputGetState" );
    }

    SK_ApplyQueuedHooks ();
  }

  // Lazy-load DLLs if somehow a game uses an XInput DLL not listed
  //   in its import table and also not caught by our LoadLibrary hook
  if (first_frame)
  {
    if (GetModuleHandle (L"XInput1_3.dll"))
      SK_Input_HookXInput1_3 ();

    if (GetModuleHandle (L"XInput1_4.dll"))
      SK_Input_HookXInput1_4 ();

    if (GetModuleHandle (L"XInput9_1_0.dll"))
      SK_Input_HookXInput9_1_0 ();

    first_frame = false;

    SK_ApplyQueuedHooks ();
  }

  if (iJoyID == -1)
    return true;



  SK_XInput_RehookIfNeeded ();



  const int MAX_CONTROLLERS = 4;

  XINPUT_STATE_EX xstate;

  static DWORD last_poll [MAX_CONTROLLERS] = { 0 };
  static DWORD dwRet     [MAX_CONTROLLERS] = { 0 };

  // This function is actually a performance hazzard when no controllers
  //   are plugged in, so ... throttle the sucker.
  if (last_poll [iJoyID] < timeGetTime () - 500UL)
  {
    if (xinput_ctx.primary_hook->XInputGetStateEx_Original != nullptr)
    {
      dwRet [iJoyID] =
        xinput_ctx.primary_hook->XInputGetStateEx_Original (iJoyID, &xstate);
    }

    // Down-level XInput
    else
    {
      if (xinput_ctx.primary_hook->XInputGetState_Original != nullptr)
      {
        dwRet [iJoyID] =
          xinput_ctx.primary_hook->XInputGetState_Original (iJoyID, (XINPUT_STATE *)&xstate);
      }
    }

    if (dwRet [iJoyID] == ERROR_DEVICE_NOT_CONNECTED)
      last_poll [iJoyID] = timeGetTime ();
  }

  if (dwRet [iJoyID] == ERROR_DEVICE_NOT_CONNECTED)
    return false;

  last_poll [iJoyID] = 0; // Feel free to poll this controller again immediately,
                          //   the performance penalty from a disconnected controller
                          //     won't be there.

  if (pState != nullptr)
    memcpy (pState, &xstate, sizeof XINPUT_STATE);

  return true;
}



void
SK_Input_PreHookXInput (void)
{
  if (xinput_ctx.primary_hook == nullptr)
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
      // Brutually stupid, let's not do this -- use the import table instead
      //                                          try the other DLLs later
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

  extern bool __FAR_Freelook;
  if (__FAR_Freelook)
    imgui_capture = true;

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

    if (config.input.ui.capture_hidden && (! SK_InputUtil_IsHWCursorVisible ()))
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

  if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open && SK_ImGui_Visible                  ) ||
       ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
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
    //return 0;
  }

  return SendInput_Original (nInputs, pInputs, cbSize);
}

keybd_event_pfn keybd_event_Original = nullptr;

void
WINAPI
keybd_event_Detour (
    _In_ BYTE bVk,
    _In_ BYTE bScan,
    _In_ DWORD dwFlags,
    _In_ ULONG_PTR dwExtraInfo
)
{
  SK_LOG_FIRST_CALL

// TODO: Process this the right way...

  if (SK_ImGui_Visible)
  {
    return;
  }

  keybd_event_Original (bVk, bScan, dwFlags, dwExtraInfo);
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
    if (SK_ImGui_WantKeyboardCapture ())
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
    if (SK_ImGui_WantKeyboardCapture ())
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

  if (SK_ImGui_WantKeyboardCapture ()) {
    memset (lpKeyState, 0, 256);
    return TRUE;
  }

  BOOL bRet = GetKeyboardState_Original (lpKeyState);

  if (bRet)
  {
    // Some games use this API for mouse buttons, for reasons that are beyond me...
    if (SK_ImGui_WantMouseCapture ())
      memset (lpKeyState, 0, 5);
  }

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

  if (lpMsg->message >= WM_MOUSEFIRST && lpMsg->message <= WM_MOUSELAST)
  {
    switch (lpMsg->message)
    {
      // Pre-Dispose These Mesages (fixes The Witness)
      case WM_LBUTTONDBLCLK:
      case WM_LBUTTONDOWN:
      case WM_MBUTTONDBLCLK:
      case WM_MBUTTONDOWN:
      case WM_MBUTTONUP:
      case WM_MOUSEHWHEEL:
      case WM_MOUSEWHEEL:
      case WM_RBUTTONDBLCLK:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
      case WM_XBUTTONDBLCLK:
      case WM_XBUTTONDOWN:
      case WM_XBUTTONUP:
      case WM_LBUTTONUP:
        if (SK_ImGui_WantMouseCapture ())
        {
          DispatchMessage (lpMsg);
          lpMsg->message = WM_NULL;

          return true;
        }
        break;
    }
  }

  return handled;
}



// Parts of the Win32 API that are safe to hook from DLL Main
void SK_Input_PreInit (void)
{
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

  SK_CreateDLLHook2 ( L"user32.dll", "keybd_event",
                     keybd_event_Detour,
           (LPVOID*)&keybd_event_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "RegisterRawInputDevices",
                     RegisterRawInputDevices_Detour,
           (LPVOID*)&RegisterRawInputDevices_Original );

#if 0
  SK_CreateDLLHook2 ( L"user32.dll", "GetRawInputBuffer",
                     GetRawInputBuffer_Detour,
           (LPVOID*)&GetRawInputBuffer_Original );
#endif

  if (config.input.gamepad.hook_xinput)
    SK_XInput_InitHotPlugHooks ();

  MH_ApplyQueued ();
}


void
SK_Input_Init (void)
{
  SK_Input_PreHookHID    ();
  SK_Input_PreHookDI8    ();
  SK_Input_PreHookXInput ();

  SK_ApplyQueuedHooks    ();
}
