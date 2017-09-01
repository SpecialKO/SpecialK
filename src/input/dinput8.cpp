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


using finish_pfn = void (WINAPI *)(void);


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
        
        SK_CreateFuncHook (       L"IDirectInput8::CreateDevice",
                                   vftable [3],
                                   IDirectInput8_CreateDevice_Detour,
          static_cast_p2p <void> (&IDirectInput8_CreateDevice_Original) );
        
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
            SK_CreateDLLHook2 (      L"dinput8.dll",
                                      "DirectInput8Create",
                                       DirectInput8Create,
              static_cast_p2p <void> (&DirectInput8Create_Import) )
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
  }





  //
  // This whole thing is as smart as a sack of wet mice in DirectInput mode...
  //   let's get to the real work and start booting graphics APIs!
  //
  static bool gl   = false, vulkan = false, d3d9  = false, d3d11 = false,
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



  InterlockedExchange (&__di8_ready, TRUE);


CreateThread (nullptr, 0x00, [](LPVOID user) -> DWORD
{
  UNREFERENCED_PARAMETER (user);

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

  CloseHandle (GetCurrentThread ());

  return 0;
}, nullptr, 0x00, nullptr);
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

    WaitForInit_DI8 ();
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


  auto ComputeAxialPos_XInput =
    [ ] (UINT min, UINT max, DWORD pos) ->
    SHORT
  {
    float range = ( static_cast <float> ( max ) - static_cast <float> ( min ) );
    float center = ( static_cast <float> ( max ) + static_cast <float> ( min ) ) / 2.0f;
    float rpos = 0.5f;

    if (static_cast <float> ( pos ) < center)
      rpos = center - ( center - static_cast <float> ( pos ) );
    else
      rpos = static_cast <float> ( pos ) - static_cast <float> ( min );

    std::numeric_limits <unsigned short>::max ( );

    float max_xi = static_cast <float> ( std::numeric_limits <unsigned short>::max ( ) );
    float center_xi = static_cast <float> ( std::numeric_limits <unsigned short>::max ( ) / 2 );

    return
      static_cast <SHORT> ( max_xi * ( rpos / range ) - center_xi );
  };

  auto TestTriggerThreshold_XInput =
    [ ] (UINT min, UINT max, float threshold, DWORD pos, bool& lt, bool& rt) ->
    void
  {
    float range = ( static_cast <float> ( max ) - static_cast <float> ( min ) );

    if (pos <= min + static_cast <UINT> ( threshold * range ))
      rt = true;

    if (pos >= max - static_cast <UINT> ( threshold * range ))
      lt = true;
  };

  bool lt = false,
       rt = false;
  
  switch (config.input.gamepad.predefined_layout)
  {
    case 0:
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
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;

      if (pJoy->dwButtons & (1 << 11))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

      if (pJoy->dwButtons & (1 << 6))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;

      if (pJoy->dwButtons & (1 << 7))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;

      if (pJoy->dwButtons & (1 << 4))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;

      if (pJoy->dwButtons & (1 << 5))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

      joy_to_xi.Gamepad.sThumbLX = ComputeAxialPos_XInput (pCaps->wXmin, pCaps->wXmax, pJoy->dwXpos);
      joy_to_xi.Gamepad.sThumbLY = ComputeAxialPos_XInput (pCaps->wYmin, pCaps->wYmax, pJoy->dwYpos);

      // Invert Y-Axis for Steam controller
      joy_to_xi.Gamepad.sThumbLY = -joy_to_xi.Gamepad.sThumbLY;

      joy_to_xi.Gamepad.sThumbRX = ComputeAxialPos_XInput (pCaps->wRmin, pCaps->wRmax, pJoy->dwRpos);
      joy_to_xi.Gamepad.sThumbRY = ComputeAxialPos_XInput (pCaps->wUmin, pCaps->wUmax, pJoy->dwUpos);

      TestTriggerThreshold_XInput (pCaps->wZmin, pCaps->wZmax, 0.00400f, pJoy->dwZpos, lt, rt);

      if (lt)
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;
      if (rt)
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;
      break;

    case 1:
      if (pJoy->dwButtons & (1 << 0))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_A;
      if (pJoy->dwButtons & (1 << 1))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_B;
      if (pJoy->dwButtons & (1 << 2))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_X;
      if (pJoy->dwButtons & (1 << 3))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_Y;

      if (pJoy->dwButtons & (1 << 4))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
      if (pJoy->dwButtons & (1 << 5))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;

      if (pJoy->dwButtons & (1 << 6))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK;
      if (pJoy->dwButtons & (1 << 7))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_START;

      if (pJoy->dwButtons & (1 << 8))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;

      if (pJoy->dwButtons & (1 << 9))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

      if (pJoy->dwButtons & (1 << 10))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;

      if (pJoy->dwButtons & (1 << 11))
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;

      joy_to_xi.Gamepad.sThumbLX = ComputeAxialPos_XInput (pCaps->wXmin, pCaps->wXmax, pJoy->dwXpos);
      joy_to_xi.Gamepad.sThumbLY = ComputeAxialPos_XInput (pCaps->wYmin, pCaps->wYmax, pJoy->dwYpos);

      // Invert Y-Axis for Steam controller
      joy_to_xi.Gamepad.sThumbLY = -joy_to_xi.Gamepad.sThumbLY;

      joy_to_xi.Gamepad.sThumbRX = ComputeAxialPos_XInput (pCaps->wRmin, pCaps->wRmax, pJoy->dwRpos);
      joy_to_xi.Gamepad.sThumbRY = ComputeAxialPos_XInput (pCaps->wUmin, pCaps->wUmax, pJoy->dwUpos);

      TestTriggerThreshold_XInput (pCaps->wZmin, pCaps->wZmax, 0.00400f, pJoy->dwZpos, lt, rt);

      if (lt)
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_TRIGGER;
      if (rt)
        joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_TRIGGER;
      break;
  };

  //joy_to_xi.Gamepad.bLeftTrigger  =   (BYTE)((float)MAXBYTE  * ((float)pJoy->dwZpos / 255.0f));
  //joy_to_xi.Gamepad.bRightTrigger =  -(BYTE)((float)MAXBYTE  * ((float)pJoy->dwZpos / 255.0f));
  //joy_to_xi.Gamepad.bLeftTrigger  =   (BYTE)((float)MAXBYTE  * ((float)pJoy->dwZpos / 255.0f));
  //joy_to_xi.Gamepad.bRightTrigger =   (BYTE)((float)MAXBYTE  * ((float)pJoy->dwZpos / 255.0f));

  // One-eighth of a full rotation
  //DWORD JOY_OCTSPACE = JOY_POVRIGHT / 2;

#if 0
  // 315 - 45
  if ( (pJoy->dwPOV >= JOY_POVLEFT   + JOY_OCTSPACE && pJoy->dwPOV != JOY_POVCENTERED) || pJoy->dwPOV <= JOY_POVRIGHT - JOY_OCTSPACE )
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;

  if (pJoy->dwPOV >= JOY_POVBACKWARD - JOY_OCTSPACE && pJoy->dwPOV <= JOY_POVBACKWARD + JOY_OCTSPACE)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;

  if (pJoy->dwPOV >= JOY_POVLEFT     - JOY_OCTSPACE && pJoy->dwPOV <= JOY_POVLEFT     + JOY_OCTSPACE)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;

  if (pJoy->dwPOV >= JOY_POVRIGHT    - JOY_OCTSPACE && pJoy->dwPOV <= JOY_POVRIGHT    + JOY_OCTSPACE)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
#else
  // 315 - 45
  if (pJoy->dwPOV == JOY_POVFORWARD)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;

  if (pJoy->dwPOV == JOY_POVBACKWARD)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;

  if (pJoy->dwPOV == JOY_POVLEFT)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;

  if (pJoy->dwPOV == JOY_POVRIGHT)
    joy_to_xi.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
#endif

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

      auto* out =
        static_cast <DIJOYSTATE2 *> (lpvData);

      SK_DI8_TranslateToXInput (reinterpret_cast <DIJOYSTATE *> (out));

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

      auto* out =
        static_cast <DIJOYSTATE *> (lpvData);

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
            static_cast <DIMOUSESTATE2 *> (lpvData)->lX = 0;
            static_cast <DIMOUSESTATE2 *> (lpvData)->lY = 0;
            static_cast <DIMOUSESTATE2 *> (lpvData)->lZ = 0;
            memset (static_cast <DIMOUSESTATE2 *> (lpvData)->rgbButtons, 0, 8);
            break;

          case sizeof (DIMOUSESTATE):
            static_cast <DIMOUSESTATE *> (lpvData)->lX = 0;
            static_cast <DIMOUSESTATE *> (lpvData)->lY = 0;
            static_cast <DIMOUSESTATE *> (lpvData)->lZ = 0;
            memset (static_cast <DIMOUSESTATE *> (lpvData)->rgbButtons, 0, 4);
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
    void** vftable =
      *reinterpret_cast <void ***> (*lplpDirectInputDevice);

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
      SK_CreateFuncHook (      L"IDirectInputDevice8::GetDeviceState",
                                 vftable [9],
                                 IDirectInputDevice8_GetDeviceState_GAMEPAD_Detour,
        static_cast_p2p <void> (&IDirectInputDevice8_GetDeviceState_GAMEPAD_Original) );
      MH_QueueEnableHook (vftable [9]);
    }
    //}

    if (! IDirectInputDevice8_SetCooperativeLevel_Original)
    {
      SK_CreateFuncHook (      L"IDirectInputDevice8::SetCooperativeLevel",
                                 vftable [13],
                                 IDirectInputDevice8_SetCooperativeLevel_Detour,
        static_cast_p2p <void> (&IDirectInputDevice8_SetCooperativeLevel_Original) );
      MH_QueueEnableHook (vftable [13]);
    }

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

  if (! InterlockedExchangeAdd (&hooked, 1))
  {
    SK_LOG0 ( ( L"Game uses DirectInput, installing input hooks..." ),
                  L"   Input  " );

    HMODULE hBackend = 
      (SK_GetDLLRole () & DLL_ROLE::DInput8) ? backend_dll :
                                      GetModuleHandle (L"dinput8.dll");

    SK_CreateFuncHook (      L"DirectInput8Create",
             GetProcAddress ( hBackend,
                              "DirectInput8Create" ),
                               DirectInput8Create,
      static_cast_p2p <void> (&DirectInput8Create_Import) );
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