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
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>

#include <SpecialK/input/dinput8_backend.h>
#include <SpecialK/input/xinput.h>
#include <SpecialK/input/input.h>

#include <SpecialK/dxgi_backend.h>

#include <SpecialK/core.h>
#include <SpecialK/log.h>
#include <SpecialK/import.h>

#include <SpecialK/config.h>

#include <SpecialK/utility.h>
#include <SpecialK/command.h>
#include <SpecialK/hooks.h>
#include <SpecialK/window.h>
#include <SpecialK/steam_api.h>

#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/compatibility.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include <atlbase.h>
#include <comdef.h>
#include <guiddef.h>


extern bool nav_usable;


typedef void (WINAPI *finish_pfn)(void);


#define SK_DI8_READ(type)  SK_DI8_Backend.markRead  (type);
#define SK_DI8_WRITE(type) SK_DI8_Backend.markWrite (type);


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
DirectInput8Create_pfn
         DirectInput8Create_Import                           = nullptr;

IDirectInput8_CreateDevice_pfn
        IDirectInput8_CreateDevice_Original                  = nullptr;

IDirectInputDevice8_GetDeviceState_pfn
        IDirectInputDevice8_GetDeviceState_MOUSE_Original    = nullptr;

IDirectInputDevice8_GetDeviceState_pfn
        IDirectInputDevice8_GetDeviceState_KEYBOARD_Original = nullptr;

IDirectInputDevice8_GetDeviceState_pfn
        IDirectInputDevice8_GetDeviceState_GAMEPAD_Original  = nullptr;

IDirectInputDevice8_SetCooperativeLevel_pfn
        IDirectInputDevice8_SetCooperativeLevel_Original     = nullptr;

HRESULT
WINAPI
IDirectInput8_CreateDevice_Detour ( IDirectInput8       *This,
                                    REFGUID              rguid,
                                    LPDIRECTINPUTDEVICE *lplpDirectInputDevice,
                                    LPUNKNOWN            pUnkOuter );


volatile LONG __di8_ready = FALSE;

void
WINAPI
WaitForInit_DI8 (void)
{
  while (! InterlockedCompareExchange (&__di8_ready, FALSE, FALSE))
    SleepEx (config.system.init_delay, TRUE);
}


__declspec (noinline)
HRESULT
DirectInput8Create ( HINSTANCE hinst,
                     DWORD     dwVersion, 
                     REFIID    riidltf, 
                     LPVOID   *ppvOut, 
                     LPUNKNOWN punkOuter )
{
  if (SK_GetDLLRole () == DLL_ROLE::DInput8)
  {
    WaitForInit_DI8 ();
    WaitForInit     ();
  }

  dll_log.Log ( L"[ DInput 8 ] [!] %s (%ph, %lu, {...}, ppvOut=%p, %p) - "
                L"%s",
                  L"DirectInput8Create",
                    hinst,  dwVersion, /*,*/
                    ppvOut, punkOuter,
                      SK_SummarizeCaller ().c_str () );

  HRESULT hr = E_NOINTERFACE;

  if (DirectInput8Create_Import != nullptr)
  {
    if ( SUCCEEDED (
           (hr = DirectInput8Create_Import (hinst, dwVersion, riidltf, ppvOut, punkOuter))
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
  }

  return hr;
}


void
WINAPI
SK_BootDI8 (void)
{
  static volatile ULONG hooked = FALSE;

  if (InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    return;
  }

  HMODULE hBackend = 
    (SK_GetDLLRole () & DLL_ROLE::DInput8) ? backend_dll :
                                    GetModuleHandle (L"dinput8.dll");

  dll_log.Log (L"[ DInput 8 ] Importing DirectInput8Create....");
  dll_log.Log (L"[ DInput 8 ] ================================");
  
  if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dinput8.dll"))
  {
    dll_log.Log (L"[ DInput 8 ]   DirectInput8Create:   %ph",
      (DirectInput8Create_Import) =  \
        reinterpret_cast <DirectInput8Create_pfn> (
          GetProcAddress (hBackend, "DirectInput8Create")
        )
    );
  }

  else if (hBackend != nullptr)
  {
    const bool bProxy =
      ( GetModuleHandle (L"dinput8.dll") != hBackend );


    if ( MH_OK ==
            SK_CreateDLLHook2 ( L"dinput8.dll",
                                "DirectInput8Create",
                                  DirectInput8Create,
     reinterpret_cast <LPVOID *>(&DirectInput8Create_Import) )
        )
    {
      if (bProxy)
      {
        (DirectInput8Create_Import) =  \
          reinterpret_cast <DirectInput8Create_pfn> (
            GetProcAddress (hBackend, "DirectInput8Create")
          );
      }

      dll_log.Log (L"[ DInput 8 ]   DirectInput8Create:   %p  { Hooked }",
        (DirectInput8Create_Import) );
    }

    MH_ApplyQueued ();
  }





  //
  // This whole thing is as smart as a sack of wet mice in DirectInput mode...
  //   let's get to the real work and start booting graphics APIs!
  //
  bool gl   = false, vulkan = false, d3d9  = false, d3d11 = false,
       dxgi = false, d3d8   = false, ddraw = false, glide = false;

  SK_TestRenderImports (
    GetModuleHandle (nullptr),
      &gl, &vulkan,
        &d3d9, &dxgi, &d3d11,
          &d3d8, &ddraw, &glide
  );


// Load user-defined DLLs (Plug-In)
#ifdef _WIN64
    SK_LoadEarlyImports64 ();
#else
    SK_LoadEarlyImports32 ();
#endif


  // OpenGL
  //
  if (gl && GetModuleHandle (L"OpenGL32.dll"))
    SK_BootOpenGL ();


  // Vulkan
  //
  //if (vulkan && GetModuleHandle (L"Vulkan-1.dll"))
  //  SK_BootVulkan ();


  // D3D9
  //
  if (d3d9 && GetModuleHandle (L"d3d9.dll"))
    SK_BootD3D9 ();


  // D3D11
  //
  if (d3d11 || GetModuleHandle (L"d3d11.dll"))
    SK_BootDXGI ();

  // Alternate form (or D3D12, but we don't care about that right now)
  else if (dxgi || GetModuleHandle (L"dxgi.dll"))
    SK_BootDXGI ();


// Load user-defined DLLs (Plug-In)
#ifdef _WIN64
  SK_LoadPlugIns64 ();
#else
  SK_LoadPlugIns32 ();
#endif
}

unsigned int
__stdcall
SK_HookDI8 (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  //if (! config.apis.di8.hook)
  //{
  //  return 0;
  //}

  const bool success = SUCCEEDED (
    CoInitializeEx (nullptr, COINIT_MULTITHREADED)
  );

  {
    SK_BootDI8 ();

    InterlockedExchange (&__di8_ready, TRUE);

    if (! (SK_GetDLLRole () & DLL_ROLE::DXGI))
      SK::DXGI::StartBudgetThread_NoAdapter ();
  }

  if (success)
    CoUninitialize ();
 
  return 0;
}

void
WINAPI
di8_init_callback (finish_pfn finish)
{
  if (! SK_IsHostAppSKIM ())
  {
    SK_HookDI8 (nullptr);

    while (! InterlockedCompareExchange (&__di8_ready, FALSE, FALSE))
      SleepEx (8UL, TRUE);
  }

  finish ();
}


bool
SK::DI8::Startup (void)
{
  return SK_StartupCore (L"dinput8", di8_init_callback);
}

bool
SK::DI8::Shutdown (void)
{
  return SK_ShutdownCore (L"dinput8");
}


struct SK_DI8_Keyboard _dik;
struct SK_DI8_Mouse    _dim;


__declspec (noinline)
SK_DI8_Keyboard*
WINAPI
SK_Input_GetDI8Keyboard (void)
{
  return &_dik;
}

__declspec (noinline)
SK_DI8_Mouse*
WINAPI
SK_Input_GetDI8Mouse (void)
{
  return &_dim;
}

__declspec (noinline)
bool
WINAPI
SK_Input_DI8Mouse_Acquire (SK_DI8_Mouse* pMouse)
{
  if (pMouse == nullptr && _dim.pDev != nullptr)
    pMouse = &_dim;

  if (pMouse != nullptr)
  {
    IDirectInputDevice8_SetCooperativeLevel_Original (
      pMouse->pDev,
        game_window.hWnd,
          pMouse->coop_level
    );

    return true;
  }

  return false;
}

__declspec (noinline)
bool
WINAPI
SK_Input_DI8Mouse_Release (SK_DI8_Mouse* pMouse)
{
  if (pMouse == nullptr && _dim.pDev != nullptr)
    pMouse = &_dim;

  if (pMouse != nullptr)
  {
    IDirectInputDevice8_SetCooperativeLevel_Original (
      pMouse->pDev,
        game_window.hWnd,
          (pMouse->coop_level & (~DISCL_EXCLUSIVE)) | DISCL_NONEXCLUSIVE
    );

    return true;
  }

  return false;
}



XINPUT_STATE di8_to_xi { };
XINPUT_STATE joy_to_xi { };

XINPUT_STATE
SK_JOY_TranslateToXInput (JOYINFOEX* pJoy, const JOYCAPSW* pCaps)
{
  UNREFERENCED_PARAMETER (pCaps);

  static DWORD dwPacket = 0;

  ZeroMemory (&joy_to_xi.Gamepad, sizeof (XINPUT_STATE::Gamepad));
  
  if (pJoy->dwButtons & (1 << 1))
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_A;
  if (pJoy->dwButtons & (1 << 2))
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_B;
  if (pJoy->dwButtons & 1)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_X;
  if (pJoy->dwButtons & (1 << 3))
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_Y;

  if (pJoy->dwButtons & (1 << 9))
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_START;
  if (pJoy->dwButtons & (1 << 8))
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;

  if (pJoy->dwButtons & (1 << 10))
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;

  if (pJoy->dwButtons & (1 << 11))
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

  if (pJoy->dwButtons & (1 << 6))
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;

  if (pJoy->dwButtons & (1 << 7))
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;

  if (pJoy->dwButtons & (1 << 4))
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;

  if (pJoy->dwButtons & (1 << 5))
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

  //xi_state.Gamepad.bLeftTrigger =
  //  UNX_PollAxis (gamepad.remap.buttons.LT, joy_ex, caps);
  //
  //xi_state.Gamepad.bRightTrigger =
  //  UNX_PollAxis (gamepad.remap.buttons.RT, joy_ex, caps);
  //
  //if (UNX_PollAxis (gamepad.remap.buttons.LB, joy_ex, caps) > 190)
  //  xi_state.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
  //if (UNX_PollAxis (gamepad.remap.buttons.RB, joy_ex, caps) > 190)
  //  xi_state.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
  //
  //if (UNX_PollAxis (gamepad.remap.buttons.LS, joy_ex, caps) > 190)
  //  xi_state.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
  //if (UNX_PollAxis (gamepad.remap.buttons.RS, joy_ex, caps) > 190)
  //  xi_state.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

  //joy_to_xi.Gamepad.sThumbLX      =  (SHORT)((float)MAXSHORT * (((float)(pJoy->dwXpos - (pCaps->wXmin + pCaps->wXmax)) /
  //                                                               (float)(pCaps->wXmin + pCaps->wXmax))));
  //joy_to_xi.Gamepad.sThumbLY      =  (SHORT)((float)MAXSHORT * (((float)(pJoy->dwYpos - (pCaps->wYmin + pCaps->wYmax)) /
  //                                                               (float)(pCaps->wYmin + pCaps->wYmax))));

  //joy_to_xi.Gamepad.sThumbRX      =  (SHORT)((float)MAXSHORT * ((float)pJoy->dwZpos / 32767.0f));
  //joy_to_xi.Gamepad.sThumbRY      = -(SHORT)((float)MAXSHORT * ((float)pJoy->dwRpos / 32767.0f));
  //
  //joy_to_xi.Gamepad.bLeftTrigger  =   (BYTE)((float)MAXBYTE  * ((float)pJoy->dwUpos / 255.0f));
  //joy_to_xi.Gamepad.bRightTrigger =   (BYTE)((float)MAXBYTE  * ((float)pJoy->dwVpos / 255.0f));


  if (pJoy->dwPOV == JOY_POVFORWARD)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
  if (pJoy->dwPOV == JOY_POVBACKWARD)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
  if (pJoy->dwPOV == JOY_POVLEFT)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
  if (pJoy->dwPOV == JOY_POVRIGHT)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;

  joy_to_xi.dwPacketNumber = dwPacket++;

  return joy_to_xi;
}

XINPUT_STATE
SK_DI8_TranslateToXInput (DIJOYSTATE* pJoy)
{
  static DWORD dwPacket = 0;

  ZeroMemory (&di8_to_xi.Gamepad, sizeof (XINPUT_STATE::Gamepad));

  //
  // Hard-coded mappings for DualShock 4 -> XInput
  //

  if (pJoy->rgbButtons [ 9])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_START;

  if (pJoy->rgbButtons [ 8])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;

  if (pJoy->rgbButtons [10])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;

  if (pJoy->rgbButtons [11])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

  if (pJoy->rgbButtons [ 6])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;

  if (pJoy->rgbButtons [ 7])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;

  if (pJoy->rgbButtons [ 4])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;

  if (pJoy->rgbButtons [ 5])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

  if (pJoy->rgbButtons [ 1])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_A;

  if (pJoy->rgbButtons [ 2])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_B;

  if (pJoy->rgbButtons [ 0])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_X;

  if (pJoy->rgbButtons [ 3])
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_Y;

  if (pJoy->rgdwPOV [0] == 0)
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;

  if (pJoy->rgdwPOV [0] == 9000)
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;

  if (pJoy->rgdwPOV [0] == 18000)
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;

  if (pJoy->rgdwPOV [0] == 27000)
    di8_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;

  di8_to_xi.Gamepad.sThumbLX      =  (SHORT)((float)MAXSHORT * ((float)pJoy->lX / 255.0f));
  di8_to_xi.Gamepad.sThumbLY      = -(SHORT)((float)MAXSHORT * ((float)pJoy->lY / 255.0f));

  di8_to_xi.Gamepad.sThumbRX      =  (SHORT)((float)MAXSHORT * ((float)pJoy->lZ  / 255.0f));
  di8_to_xi.Gamepad.sThumbRY      = -(SHORT)((float)MAXSHORT * ((float)pJoy->lRz / 255.0f));

  di8_to_xi.Gamepad.bLeftTrigger  =   (BYTE)((float)MAXBYTE  * ((float)pJoy->lRx / 255.0f));
  di8_to_xi.Gamepad.bRightTrigger =   (BYTE)((float)MAXBYTE  * ((float)pJoy->lRy / 255.0f));

  di8_to_xi.dwPacketNumber = dwPacket++;

  return di8_to_xi;
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

  if (SUCCEEDED (hr) && lpvData != nullptr)
  {
    if (cbData == sizeof DIJOYSTATE2) 
    {
      SK_DI8_READ (sk_input_dev_type::Gamepad)
      static DIJOYSTATE2 last_state;

      DIJOYSTATE2* out = (DIJOYSTATE2 *)lpvData;

      SK_DI8_TranslateToXInput ((DIJOYSTATE *)out);

      if (nav_usable)
      {
        memcpy (out, &last_state, cbData);

        out->rgdwPOV [0] = std::numeric_limits <DWORD>::max ();
        out->rgdwPOV [1] = std::numeric_limits <DWORD>::max ();
        out->rgdwPOV [2] = std::numeric_limits <DWORD>::max ();
        out->rgdwPOV [3] = std::numeric_limits <DWORD>::max ();
      } else
        memcpy (&last_state, out, cbData);
    }

    else if (cbData == sizeof DIJOYSTATE) 
    {
      SK_DI8_READ (sk_input_dev_type::Gamepad)

      //dll_log.Log (L"Joy");

      static DIJOYSTATE last_state;

      DIJOYSTATE* out = (DIJOYSTATE *)lpvData;

      SK_DI8_TranslateToXInput (out);

      if (nav_usable)
      {
        memcpy (out, &last_state, cbData);

        out->rgdwPOV [0] = std::numeric_limits <DWORD>::max ();
        out->rgdwPOV [1] = std::numeric_limits <DWORD>::max ();
        out->rgdwPOV [2] = std::numeric_limits <DWORD>::max ();
        out->rgdwPOV [3] = std::numeric_limits <DWORD>::max ();
      }
      else
        memcpy (&last_state, out, cbData);

#if 0
      XINPUT_STATE xis;
      SK_XInput_PollController (0, &xis);

      out->rgbButtons [ 9] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_START          ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [ 8] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_BACK           ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [10] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB     ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [11] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB    ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [ 6] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_TRIGGER   ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [ 7] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_TRIGGER  ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [ 4] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER  ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [ 5] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [ 1] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_A              ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [ 2] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_B              ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [ 0] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_X              ) != 0x0 ? 0xFF : 0x00);
      out->rgbButtons [ 3] = (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_Y              ) != 0x0 ? 0xFF : 0x00);

      out->rgdwPOV [0] += (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP           ) != 0x0 ?      0 : 0);
      out->rgdwPOV [0] += (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT        ) != 0x0 ?  90000 : 0);
      out->rgdwPOV [0] += (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN         ) != 0x0 ? 180000 : 0);
      out->rgdwPOV [0] += (( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT         ) != 0x0 ? 270000 : 0);

      if (out->rgdwPOV [0] == 0)
        out->rgdwPOV [0] = -1;
#endif
    }

    else if (This == _dik.pDev || cbData == 256)
    {
      SK_DI8_READ (sk_input_dev_type::Keyboard)

      if (SK_ImGui_WantKeyboardCapture () && lpvData != nullptr)
        memset (lpvData, 0, cbData);
    }

    else if ( cbData == sizeof (DIMOUSESTATE2) ||
              cbData == sizeof (DIMOUSESTATE)  )
    {
      SK_DI8_READ (sk_input_dev_type::Mouse)

      //dll_log.Log (L"Mouse");

      //((DIMOUSESTATE *)lpvData)->lZ += InterlockedAdd      (&SK_Input_GetDI8Mouse ()->delta_z, 0);
                                       //InterlockedExchange (&SK_Input_GetDI8Mouse ()->delta_z, 0);

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

bool
SK_DInput8_HasMouse (void)
{
  return (_dim.pDev && IDirectInputDevice8_SetCooperativeLevel_Original);
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

  HRESULT hr =
    IDirectInputDevice8_SetCooperativeLevel_Original (This, hwnd, dwFlags);

  if (SUCCEEDED (hr))
  {
    // Mouse
    if (This == _dim.pDev)
      _dim.coop_level = dwFlags;

    // Keyboard   (why do people use DirectInput for keyboards? :-\)
    else if (This == _dik.pDev)
      _dik.coop_level = dwFlags;

    // Anything else is probably not important
  }

  if (SK_ImGui_WantMouseCapture ())
  {
    dwFlags &= ~DISCL_EXCLUSIVE;

    IDirectInputDevice8_SetCooperativeLevel_Original (This, hwnd, dwFlags);
  }

  return hr;
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

  dll_log.Log ( L"[   Input  ][!] IDirectInput8::CreateDevice (%ph, %s, %ph, %ph)",
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

    //
    // This weird hack is necessary for EverQuest; crazy game hooks itself to try and thwart
    //   macro programs.
    //
    /*
    if (rguid == GUID_SysMouse && _dim.pDev == nullptr)
    {
      SK_CreateFuncHook ( L"IDirectInputDevice8::GetDeviceState",
                           vftable [9],
                           IDirectInputDevice8_GetDeviceState_MOUSE_Detour,
                (LPVOID *)&IDirectInputDevice8_GetDeviceState_MOUSE_Original );
      MH_QueueEnableHook (vftable [9]);
    }

    else if (rguid == GUID_SysKeyboard && _dik.pDev == nullptr)
    {
      SK_CreateFuncHook ( L"IDirectInputDevice8::GetDeviceState",
                           vftable [9],
                           IDirectInputDevice8_GetDeviceState_KEYBOARD_Detour,
                (LPVOID *)&IDirectInputDevice8_GetDeviceState_KEYBOARD_Original );
      MH_QueueEnableHook (vftable [9]);
    }

    else if (rguid != GUID_SysMouse && rguid != GUID_SysKeyboard)
    {
    */
    if (IDirectInputDevice8_GetDeviceState_GAMEPAD_Original == nullptr)
    {
      SK_CreateFuncHook ( L"IDirectInputDevice8::GetDeviceState",
                           vftable [9],
                           IDirectInputDevice8_GetDeviceState_GAMEPAD_Detour,
                (LPVOID *)&IDirectInputDevice8_GetDeviceState_GAMEPAD_Original );
      MH_QueueEnableHook (vftable [9]);
    }
    //}

    if (! IDirectInputDevice8_SetCooperativeLevel_Original)
    {
      SK_CreateFuncHook ( L"IDirectInputDevice8::SetCooperativeLevel",
                           vftable [13],
                           IDirectInputDevice8_SetCooperativeLevel_Detour,
                 (LPVOID*)&IDirectInputDevice8_SetCooperativeLevel_Original );
      MH_QueueEnableHook (vftable [13]);
    }

    MH_ApplyQueued ();

    if (rguid == GUID_SysMouse)
    {
      _dim.pDev = *lplpDirectInputDevice;
    }
    else if (rguid == GUID_SysKeyboard)
      _dik.pDev = *lplpDirectInputDevice;
  }

#if 0
  if (SUCCEEDED (hr) && lplpDirectInputDevice != nullptr)
  {
    DWORD dwFlag = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

    if (config.input.block_windows)
      dwFlag |= DISCL_NOWINKEY;

    (*lplpDirectInputDevice)->SetCooperativeLevel (SK_GetGameWindow (), dwFlag);
  }
#endif

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
    SK_LOG0 ( ( L"Game uses DirectInput, installing input hooks..." ),
                  L"   Input  " );

    HMODULE hBackend = 
      (SK_GetDLLRole () & DLL_ROLE::DInput8) ? backend_dll :
                                      GetModuleHandle (L"dinput8.dll");

    SK_CreateFuncHook ( L"DirectInput8Create",
                          GetProcAddress ( hBackend,
                                             "DirectInput8Create" ),
                            DirectInput8Create,
                 (LPVOID *)&DirectInput8Create_Import );

    InterlockedIncrement (&hooked);
  }
}

void
SK_Input_PreHookDI8 (void)
{
  if (! config.input.gamepad.hook_dinput8)
    return;

  if (DirectInput8Create_Import == nullptr)
  {
    static sk_import_test_s tests [] = { { "dinput.dll",  false },
                                         { "dinput8.dll", false } };

    SK_TestImports (GetModuleHandle (nullptr), tests, 2);

    if (tests [0].used || tests [1].used || GetModuleHandle (L"dinput8.dll"))
    {
      if (SK_GetDLLRole () != DLL_ROLE::DInput8)
        SK_Input_HookDI8 ();
    }
  }
}

sk_input_api_context_s SK_DI8_Backend;