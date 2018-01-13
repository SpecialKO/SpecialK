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
#ifndef __SK__DI7_BACKEND_H__
#define __SK__DI7_BACKEND_H__

#include <SpecialK/input/dinput8_backend.h>

namespace SK
{
  namespace DI7
  {
    bool Startup  (void);
    bool Shutdown (void);
  }
}

void SK_Input_HookDI7    (void);
void SK_Input_PreHookDI7 (void);

using DirectInputCreateEx_pfn = HRESULT (WINAPI *)(
  HINSTANCE hinst,
  DWORD     dwVersion,
  REFIID    riidltf, 
  LPVOID   *ppvOut, 
  LPUNKNOWN punkOuter
);

using DirectInputCreateA_pfn = HRESULT (WINAPI *)(
  HINSTANCE       hinst,
  DWORD           dwVersion,
  LPDIRECTINPUTA* lplpDirectInput,
  LPUNKNOWN       punkOuter
);

using DirectInputCreateW_pfn = HRESULT (WINAPI *)(
  HINSTANCE       hinst,
  DWORD           dwVersion,
  LPDIRECTINPUTW* lplpDirectInput,
  LPUNKNOWN       punkOuter
);


using IDirectInput7W_CreateDevice_pfn = HRESULT (WINAPI *)(
  IDirectInput7W        *This,
  REFGUID                rguid,
  LPDIRECTINPUTDEVICE7W *lplpDirectInputDevice,
  LPUNKNOWN              pUnkOuter
);

using IDirectInputDevice7W_GetDeviceState_pfn = HRESULT (WINAPI *)(
  LPDIRECTINPUTDEVICE7W This,
  DWORD                 cbData,
  LPVOID                lpvData
);

using IDirectInputDevice7W_SetCooperativeLevel_pfn = HRESULT (WINAPI *)(
  LPDIRECTINPUTDEVICE7W This,
  HWND                  hwnd,
  DWORD                 dwFlags
);

using IDirectInput7A_CreateDevice_pfn = HRESULT (WINAPI *)(
  IDirectInput7A        *This,
  REFGUID                rguid,
  LPDIRECTINPUTDEVICE7A *lplpDirectInputDevice,
  LPUNKNOWN              pUnkOuter
);

using IDirectInputDevice7A_GetDeviceState_pfn = HRESULT (WINAPI *)(
  LPDIRECTINPUTDEVICE7A This,
  DWORD                 cbData,
  LPVOID                lpvData
);

using IDirectInputDevice7A_SetCooperativeLevel_pfn = HRESULT (WINAPI *)(
  LPDIRECTINPUTDEVICE7A This,
  HWND                  hwnd,
  DWORD                 dwFlags
);


#include <cstdint>

struct SK_DI7_Keyboard {
  LPDIRECTINPUTDEVICE7W pDev = nullptr;
  uint8_t               state [512];
  DWORD                 coop_level;     // The level the game requested, not necessarily
                                        //   its current state (changes based on UI).
};

struct SK_DI7_Mouse {
  LPDIRECTINPUTDEVICE7W pDev = nullptr;
  DIMOUSESTATE2         state;
  DWORD                 coop_level;     // The level the game requested, not necessarily
                                        //   its current state (changes based on UI).

  // Weird hack for some touchpads that don't send out mousewheel events in any API
  //   other than Win32.
  volatile LONG       delta_z = 0;
};


__declspec (noinline)
SK_DI7_Keyboard*
WINAPI
SK_Input_GetDI7Keyboard (void);

__declspec (noinline)
SK_DI7_Mouse*
WINAPI
SK_Input_GetDI7Mouse (void);


__declspec (noinline)
bool
WINAPI
SK_Input_DI7Mouse_Acquire (SK_DI7_Mouse* pMouse = nullptr);

__declspec (noinline)
bool
WINAPI
SK_Input_DI7Mouse_Release (SK_DI7_Mouse* pMouse = nullptr);



#endif /* __SK__DI7_BACKEND_H__ */