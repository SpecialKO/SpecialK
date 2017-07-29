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

#include <SpecialK/input/xinput.h>
#include <SpecialK/input/xinput_hotplug.h>
#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/hooks.h>
#include <SpecialK/utility.h>

#include <SpecialK/diagnostics/compatibility.h>

#include <cstdint>
#include <algorithm>

#define SK_LOG_INPUT_CALL { static int  calls  = 0;                   { SK_LOG0 ( (L"[!] > Call #%lu: %hs", calls++, __FUNCTION__), L"Input Mgr." ); } }
#define SK_LOG_FIRST_CALL { static bool called = false; if (! called) { SK_LOG0 ( (L"[!] > First Call: %34hs", __FUNCTION__), L"Input Mgr." ); called = true; } }

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

    uint8_t                         orig_inst    [12]                    =   {   };

    XInputGetCapabilities_pfn       XInputGetCapabilities_Detour         = nullptr;
    XInputGetCapabilities_pfn       XInputGetCapabilities_Original       = nullptr;
    LPVOID                          XInputGetCapabilities_Target         = nullptr;

    uint8_t                         orig_inst_caps [12]                  =   {   };

    XInputGetBatteryInformation_pfn XInputGetBatteryInformation_Detour   = nullptr;
    XInputGetBatteryInformation_pfn XInputGetBatteryInformation_Original = nullptr;
    LPVOID                          XInputGetBatteryInformation_Target   = nullptr;

    uint8_t                         orig_inst_batt [12]                  =   {   };

    XInputSetState_pfn              XInputSetState_Detour                = nullptr;
    XInputSetState_pfn              XInputSetState_Original              = nullptr;
    LPVOID                          XInputSetState_Target                = nullptr;

    uint8_t                         orig_inst_set [12]                   =   {   };

    //
    // Extended stuff (XInput1_3 and XInput1_4 ONLY)
    //
    XInputGetStateEx_pfn            XInputGetStateEx_Detour              = nullptr;
    XInputGetStateEx_pfn            XInputGetStateEx_Original            = nullptr;
    LPVOID                          XInputGetStateEx_Target              = nullptr;

    uint8_t                         orig_inst_ex [12]                    =   {   };
  } XInput1_3 { }, XInput1_4 { }, XInput9_1_0 { };

  instance_s* primary_hook = nullptr;
} xinput_ctx;

#define SK_XINPUT_READ(type)  SK_XInput_Backend.markRead  (type);
#define SK_XINPUT_WRITE(type) SK_XInput_Backend.markWrite (type);


#include <unordered_set>

void
SK_XInput_EstablishPrimaryHook (HMODULE hModCaller, SK_XInputContext::instance_s* pCtx)
{
  // Calling module (return address) indicates the game made this call
  if (hModCaller == GetModuleHandle (nullptr))
    xinput_ctx.primary_hook = pCtx;

  // Third-party software polled the controller, it better be using the same
  //   interface version as the game is or a lot of input emulation software
  //     will not work correctly.
  else if (pCtx != xinput_ctx.primary_hook && xinput_ctx.primary_hook != nullptr)
  {
    static std::unordered_set <HMODULE> warned_modules;

    if (! warned_modules.count (hModCaller))
    {
      SK_LOG0 ( ( L"WARNING: Third-party module '%s' uses different XInput interface version "
                  L"(%s) than the game (%s); input remapping software may not work correctly.",
                    SK_GetModuleName (hModCaller).c_str (),
                    pCtx->wszModuleName, xinput_ctx.primary_hook->wszModuleName ),
                  L"Input Mgr." );

      warned_modules.insert (hModCaller);
    }
  }
}


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
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pState      == nullptr)         return (DWORD)E_POINTER;

  ZeroMemory (pState, sizeof (XINPUT_STATE));

  if (dwUserIndex >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

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
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputGetStateEx1_3_Detour (
  _In_  DWORD            dwUserIndex,
  _Out_ XINPUT_STATE_EX *pState )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pState      == nullptr)         return (DWORD)E_POINTER;

  ZeroMemory (pState, sizeof (XINPUT_STATE));

  if (dwUserIndex >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

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
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputGetCapabilities1_3_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pCapabilities == nullptr)         return (DWORD)E_POINTER;

  ZeroMemory (pCapabilities, sizeof XINPUT_CAPABILITIES);

  if (dwUserIndex   >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetCapabilities_Original (dwUserIndex, dwFlags, pCapabilities);

  dwRet =
    SK_XInput_PlaceHoldCaps (dwRet, dwUserIndex, dwFlags, pCapabilities);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputGetBatteryInformation1_3_Detour (
  _In_  DWORD                       dwUserIndex,
  _In_  BYTE                        devType,
  _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pBatteryInformation == nullptr)         return (DWORD)E_POINTER;

  ZeroMemory (pBatteryInformation, sizeof (XINPUT_BATTERY_INFORMATION));

  if (dwUserIndex         >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetBatteryInformation_Original (dwUserIndex, devType, pBatteryInformation);

  dwRet =
    SK_XInput_PlaceHoldBattery (dwRet, dwUserIndex, devType, pBatteryInformation);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputSetState1_3_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_WRITE (sk_input_dev_type::Gamepad)

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  if (pVibration  == nullptr)           return pCtx->XInputSetState_Original (dwUserIndex, pVibration);
  if (dwUserIndex >= XUSER_MAX_COUNT) { ZeroMemory (pVibration, sizeof (XINPUT_VIBRATION));
                                        return ERROR_DEVICE_NOT_CONNECTED;
                                      }

  bool nop = ( SK_ImGui_WantGamepadCapture ()                       &&
                 dwUserIndex == config.input.gamepad.xinput.ui_slot &&
                   config.input.gamepad.haptic_ui ) ||
               config.input.gamepad.xinput.disable_rumble;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
      nop ? ERROR_SUCCESS :
            pCtx->XInputSetState_Original (dwUserIndex, pVibration);

  dwRet =
    SK_XInput_PlaceHoldSet (dwRet, dwUserIndex, pVibration);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}


DWORD
WINAPI
XInputGetState1_4_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pState      == nullptr)         return (DWORD)E_POINTER;

  ZeroMemory (pState, sizeof (XINPUT_STATE));

  if (dwUserIndex >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

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
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputGetStateEx1_4_Detour (
  _In_  DWORD            dwUserIndex,
  _Out_ XINPUT_STATE_EX *pState )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pState      == nullptr)         return (DWORD)E_POINTER;

  ZeroMemory (pState, sizeof (XINPUT_STATE));

  if (dwUserIndex >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

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
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputGetCapabilities1_4_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pCapabilities == nullptr)         return (DWORD)E_POINTER;

  ZeroMemory (pCapabilities, sizeof (XINPUT_CAPABILITIES));

  if (dwUserIndex   >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetCapabilities_Original (dwUserIndex, dwFlags, pCapabilities);

  dwRet =
    SK_XInput_PlaceHoldCaps (dwRet, dwUserIndex, dwFlags, pCapabilities);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputGetBatteryInformation1_4_Detour (
  _In_  DWORD                       dwUserIndex,
  _In_  BYTE                        devType,
  _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pBatteryInformation == nullptr) return (DWORD)E_POINTER;

  ZeroMemory (pBatteryInformation, sizeof (XINPUT_BATTERY_INFORMATION));

  if (dwUserIndex >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetBatteryInformation_Original (dwUserIndex, devType, pBatteryInformation);

  dwRet =
    SK_XInput_PlaceHoldBattery (dwRet, dwUserIndex, devType, pBatteryInformation);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputSetState1_4_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_WRITE (sk_input_dev_type::Gamepad)

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  if (pVibration  == nullptr)         return pCtx->XInputSetState_Original (dwUserIndex, pVibration);
  if (dwUserIndex >= XUSER_MAX_COUNT) { ZeroMemory (pVibration, sizeof (XINPUT_VIBRATION));
                                        return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
                                      }

  bool nop = ( SK_ImGui_WantGamepadCapture ()                       &&
                 dwUserIndex == config.input.gamepad.xinput.ui_slot &&
                   config.input.gamepad.haptic_ui ) ||
               config.input.gamepad.xinput.disable_rumble;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
      nop ? ERROR_SUCCESS :
            pCtx->XInputSetState_Original (dwUserIndex, pVibration);

  dwRet =
    SK_XInput_PlaceHoldSet (dwRet, dwUserIndex, pVibration);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}


DWORD
WINAPI
XInputGetState9_1_0_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pState      == nullptr)         return (DWORD)E_POINTER;

  ZeroMemory (pState, sizeof (XINPUT_STATE));

  if (dwUserIndex >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;

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
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputGetCapabilities9_1_0_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (sk_input_dev_type::Gamepad)

  if (pCapabilities == nullptr)         return (DWORD)E_POINTER;

  ZeroMemory (pCapabilities, sizeof (XINPUT_CAPABILITIES));

  if (dwUserIndex   >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput9_1_0;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
    pCtx->XInputGetCapabilities_Original (dwUserIndex, dwFlags, pCapabilities);

  dwRet =
    SK_XInput_PlaceHoldCaps (dwRet, dwUserIndex, dwFlags, pCapabilities);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputSetState9_1_0_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_LOG_FIRST_CALL
  SK_XINPUT_WRITE (sk_input_dev_type::Gamepad)

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput9_1_0;

  if (pVibration  == nullptr)         return pCtx->XInputSetState_Original (dwUserIndex, pVibration);
  if (dwUserIndex >= XUSER_MAX_COUNT) { ZeroMemory (pVibration, sizeof (XINPUT_VIBRATION));
                                      return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
                                      }

  bool nop = ( SK_ImGui_WantGamepadCapture ()                       &&
                 dwUserIndex == config.input.gamepad.xinput.ui_slot &&
                   config.input.gamepad.haptic_ui ) ||
               config.input.gamepad.xinput.disable_rumble;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ?
      ERROR_DEVICE_NOT_CONNECTED :
      nop ?
        ERROR_SUCCESS :
        pCtx->XInputSetState_Original (dwUserIndex, pVibration);

  dwRet =
    SK_XInput_PlaceHoldSet (dwRet, dwUserIndex, pVibration);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}


void
SK_Input_HookXInputContext (SK_XInputContext::instance_s* pCtx)
{
  pCtx->XInputGetState_Target =
    SK_GetProcAddress (pCtx->wszModuleName, "XInputGetState");

  SK_CreateDLLHook2 ( pCtx->wszModuleName,
                       "XInputGetState",
                       pCtx->XInputGetState_Detour,
            (LPVOID *)&pCtx->XInputGetState_Original );

  pCtx->XInputGetCapabilities_Target =
    SK_GetProcAddress (pCtx->wszModuleName, "XInputGetCapabilities");

  SK_CreateDLLHook2 ( pCtx->wszModuleName,
                       "XInputGetCapabilities",
                       pCtx->XInputGetCapabilities_Detour,
            (LPVOID *)&pCtx->XInputGetCapabilities_Original );

  pCtx->XInputSetState_Target =
    SK_GetProcAddress (pCtx->wszModuleName, "XInputSetState");

  SK_CreateDLLHook2 ( pCtx->wszModuleName,
                       "XInputSetState",
                       pCtx->XInputSetState_Detour,
            (LPVOID *)&pCtx->XInputSetState_Original );

  pCtx->XInputGetBatteryInformation_Target =
    SK_GetProcAddress (pCtx->wszModuleName, "XInputGetBatteryInformation");

  // Down-level (XInput 9_1_0) does not have XInputGetBatteryInformation
  //
  if (pCtx->XInputGetBatteryInformation_Target != nullptr)
  {
    SK_CreateDLLHook2 ( pCtx->wszModuleName,
                         "XInputGetBatteryInformation",
                         pCtx->XInputGetBatteryInformation_Detour,
              (LPVOID *)&pCtx->XInputGetBatteryInformation_Original );
  }

  pCtx->XInputGetStateEx_Target =
    SK_GetProcAddress (pCtx->wszModuleName, XINPUT_GETSTATEEX_ORDINAL);

  // Down-level (XInput 9_1_0) does not have XInputGetStateEx (100)
  //
  if (pCtx->XInputGetStateEx_Target != nullptr)
  {
    SK_CreateDLLHook2 ( pCtx->wszModuleName,
                         XINPUT_GETSTATEEX_ORDINAL,
                         pCtx->XInputGetStateEx_Detour,
              (LPVOID *)&pCtx->XInputGetStateEx_Original );
  }

  MH_ApplyQueued ();

  if (pCtx->XInputGetState_Target != nullptr)
    memcpy (pCtx->orig_inst, pCtx->XInputGetState_Target,                   11);

  if (pCtx->XInputSetState_Target != nullptr)
    memcpy (pCtx->orig_inst_set, pCtx->XInputSetState_Target,               11);

  if (pCtx->XInputGetCapabilities_Target != nullptr)
    memcpy (pCtx->orig_inst_caps, pCtx->XInputGetCapabilities_Target,       11);

  if (pCtx->XInputGetBatteryInformation_Target != nullptr)
    memcpy (pCtx->orig_inst_batt, pCtx->XInputGetBatteryInformation_Target, 11);

  if (pCtx->XInputGetStateEx_Target != nullptr)
    memcpy (pCtx->orig_inst_ex, pCtx->XInputGetStateEx_Target,              11);
}

void
SK_Input_HookXInput1_4 (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, 1, 0))
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

  }
}

void
SK_Input_HookXInput1_3 (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, 1, 0))
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
  }
}

void
SK_Input_HookXInput9_1_0 (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, 1, 0))
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
  }
}


void
SK_XInput_RehookIfNeeded (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

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
                 ( pCtx->XInputGetState_Target != 0 &&
           memcmp (pCtx->orig_inst,
                   pCtx->XInputGetState_Target, 11 ) )
     )
  {
    if ( MH_OK == MH_RemoveHook (pCtx->XInputGetState_Target) )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2 (  pCtx->wszModuleName, "XInputGetState",
                                  pCtx->XInputGetState_Detour,
                       (LPVOID *)&pCtx->XInputGetState_Original )
         )
      {
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
                 ( pCtx->XInputSetState_Target != 0 &&
           memcmp (pCtx->orig_inst_set,
                   pCtx->XInputSetState_Target, 11 ) )
     )
  {
    if ( MH_OK == MH_RemoveHook (pCtx->XInputSetState_Target) )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2 (  pCtx->wszModuleName, "XInputSetState",
                                  pCtx->XInputSetState_Detour,
                       (LPVOID *)&pCtx->XInputSetState_Original )
         )
      {
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
                 ( pCtx->XInputGetCapabilities_Target != 0 &&
           memcmp (pCtx->orig_inst_caps,
                   pCtx->XInputGetCapabilities_Target, 11 ) )
     )
  {
    if ( MH_OK == MH_RemoveHook (pCtx->XInputGetCapabilities_Target) )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2(  pCtx->wszModuleName, "XInputGetCapabilities",
                                 pCtx->XInputGetCapabilities_Detour,
                      (LPVOID *)&pCtx->XInputGetCapabilities_Original )
         )
      {
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
                   ( pCtx->XInputGetBatteryInformation_Target != 0 &&
             memcmp (pCtx->orig_inst_batt,
                     pCtx->XInputGetBatteryInformation_Target, 11 ) )
       )
    {
      if ( MH_OK == MH_RemoveHook (pCtx->XInputGetBatteryInformation_Target) )
      {
        if ( MH_OK ==
               SK_CreateDLLHook2(  pCtx->wszModuleName, "XInputGetBatteryInformation",
                                   pCtx->XInputGetBatteryInformation_Detour,
                        (LPVOID *)&pCtx->XInputGetBatteryInformation_Original )
           )
        {
          SK_LOG0 ( ( L" Re-hooked XInput (Battery) using '%s'...",
                         pCtx->wszModuleName ),
                      L"Input Mgr." );

          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
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
                   ( pCtx->XInputGetStateEx_Target != 0 &&
             memcmp (pCtx->orig_inst_ex,
                     pCtx->XInputGetStateEx_Target, 11 ) )
       )
    {
      if ( MH_OK == MH_RemoveHook (pCtx->XInputGetStateEx_Target) )
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 (  pCtx->wszModuleName, XINPUT_GETSTATEEX_ORDINAL,
                                    pCtx->XInputGetStateEx_Detour,
                         (LPVOID *)&pCtx->XInputGetStateEx_Original )
           )
        {
          SK_LOG0 ( ( L" Re-hooked XInput (Ex) using '%s'...",
                         pCtx->wszModuleName ),
                      L"Input Mgr." );
  
          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
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


  if (pCtx->XInputGetState_Target != nullptr)
    memcpy (pCtx->orig_inst, pCtx->XInputGetState_Target,                   11);

  if (pCtx->XInputSetState_Target != nullptr)
    memcpy (pCtx->orig_inst_set, pCtx->XInputSetState_Target,               11);

  if (pCtx->XInputGetCapabilities_Target != nullptr)
    memcpy (pCtx->orig_inst_caps, pCtx->XInputGetCapabilities_Target,       11);

  if (pCtx->XInputGetBatteryInformation_Target != nullptr)
    memcpy (pCtx->orig_inst_batt, pCtx->XInputGetBatteryInformation_Target, 11);

  if (pCtx->XInputGetStateEx_Target != nullptr)
    memcpy (pCtx->orig_inst_ex, pCtx->XInputGetStateEx_Target,              11);
}


bool
SK_XInput_PulseController ( INT   iJoyID,
                            float fStrengthLeft,
                            float fStrengthRight )
{
  if (config.input.gamepad.xinput.disable_rumble)
    return false;

  if (iJoyID < 0 || iJoyID >= XUSER_MAX_COUNT)
    return false;

  XINPUT_VIBRATION vibes;

  vibes.wLeftMotorSpeed  = (WORD)(std::min (0.99999f, fStrengthLeft)  * 65535.0f);
  vibes.wRightMotorSpeed = (WORD)(std::min (0.99999f, fStrengthRight) * 65535.0f);

  if (xinput_ctx.primary_hook && xinput_ctx.primary_hook->XInputSetState_Original)
  {
    xinput_ctx.primary_hook->XInputSetState_Original ( iJoyID, &vibes );
    return true;
  }

  return false;
}

bool
SK_XInput_PollController ( INT           iJoyID,
                           XINPUT_STATE* pState )
{
  if (! config.input.gamepad.hook_xinput)
    return false;

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
    if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr)
    {
      SK_Input_HookXInput1_4 ();
      pCtx = xinput_ctx.primary_hook;
    }

    // Down-level 9_1_0 if all else fails (does not support XInputEx)
    if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr)
    {
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

      HMODULE hModXInput1_3 =
        LoadLibraryW_Original (L"XInput1_3.dll");

      if (hModXInput1_3 != 0)
      {
        pCtx->XInputGetState_Original =
          (XInputGetState_pfn)
            GetProcAddress ( hModXInput1_3,
                             "XInputGetState" );
      }
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

  if (iJoyID < 0 || iJoyID >= XUSER_MAX_COUNT)
    return false;



  SK_XInput_RehookIfNeeded ();




  XINPUT_STATE_EX xstate = { };
  xstate.dwPacketNumber  =  1;

  static DWORD last_poll [XUSER_MAX_COUNT] = { 0, 0, 0, 0 };
  static DWORD dwRet     [XUSER_MAX_COUNT] = { ERROR_DEVICE_NOT_CONNECTED, ERROR_DEVICE_NOT_CONNECTED,
                                               ERROR_DEVICE_NOT_CONNECTED, ERROR_DEVICE_NOT_CONNECTED };

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
  if (! config.input.gamepad.hook_xinput)
    return;

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
      if (tests [0].used) SK_Input_HookXInput1_3   ();
      if (tests [1].used) SK_Input_HookXInput1_4   ();
      if (tests [2].used) SK_Input_HookXInput9_1_0 ();
    }

    if (GetModuleHandle (L"XInput1_3.dll"))
      SK_Input_HookXInput1_3 ();

    if (GetModuleHandle (L"XInput1_4.dll"))
      SK_Input_HookXInput1_4 ();
    
    if (GetModuleHandle (L"XInput9_1_0.dll"))
      SK_Input_HookXInput9_1_0 ();
  }
}



void
SK_XInput_ZeroHaptics (INT iJoyID)
{
  if (iJoyID >= XUSER_MAX_COUNT) return;

  SK_XInput_PulseController (iJoyID, 0.0f, 0.0f);
}