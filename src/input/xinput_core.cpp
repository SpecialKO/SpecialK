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
// XInput
//
///////////////////////////////////////////////////////

// Route XInput 9.1.0 and 1.3 hooks through 1.4
#define XINPUT_UPGRADE

struct SK_XInputContext
{
  enum
  {
    XInputLevel_NONE  = 0,
    XInputLevel_9_1_0 = 1,
    XInputLevel_1_3   = 2,
    XInputLevel_1_4   = 4
  };

  struct instance_s
  {
    const wchar_t*                  wszModuleName                        =     L"";
    HMODULE                         hMod                                 = nullptr;

    XInputGetState_pfn              XInputGetState_Detour                = nullptr;
    XInputGetState_pfn              XInputGetState_Original              = nullptr;
    LPVOID                          XInputGetState_Target                = nullptr;

    uint8_t                         orig_inst      [7]                   =   {   };

    XInputGetCapabilities_pfn       XInputGetCapabilities_Detour         = nullptr;
    XInputGetCapabilities_pfn       XInputGetCapabilities_Original       = nullptr;
    LPVOID                          XInputGetCapabilities_Target         = nullptr;

    uint8_t                         orig_inst_caps [7]                   =   {   };

    XInputGetBatteryInformation_pfn XInputGetBatteryInformation_Detour   = nullptr;
    XInputGetBatteryInformation_pfn XInputGetBatteryInformation_Original = nullptr;
    LPVOID                          XInputGetBatteryInformation_Target   = nullptr;

    uint8_t                         orig_inst_batt [7]                   =   {   };

    XInputSetState_pfn              XInputSetState_Detour                = nullptr;
    XInputSetState_pfn              XInputSetState_Original              = nullptr;
    LPVOID                          XInputSetState_Target                = nullptr;

    uint8_t                         orig_inst_set  [7]                   =   {   };

    XInputEnable_pfn                XInputEnable_Detour                  = nullptr;
    XInputEnable_pfn                XInputEnable_Original                = nullptr;
    LPVOID                          XInputEnable_Target                  = nullptr;

    uint8_t                         orig_inst_enable [7]                 =   {   };

    //
    // Extended stuff (XInput1_3 and XInput1_4 ONLY)
    //
    XInputGetStateEx_pfn            XInputGetStateEx_Detour              = nullptr;
    XInputGetStateEx_pfn            XInputGetStateEx_Original            = nullptr;
    LPVOID                          XInputGetStateEx_Target              = nullptr;

    uint8_t                         orig_inst_ex [7]                     =   {   };
  } XInput1_3   { },
    XInput1_4   { },
    XInput9_1_0 { };

  std::recursive_mutex cs_poll   [XUSER_MAX_COUNT];
  std::recursive_mutex cs_haptic [XUSER_MAX_COUNT];
  std::recursive_mutex cs_hook   [XUSER_MAX_COUNT];
  std::recursive_mutex cs_caps   [XUSER_MAX_COUNT];

  volatile instance_s*              primary_hook                         = nullptr;
  volatile LONG                     primary_level                        = XInputLevel_NONE;

  bool preventHapticRecursion (DWORD dwUserIndex, bool enter)
  {
    if (! config.input.gamepad.xinput.hook_setstate)
      return true;

    if (enter)
    {
      if (! InterlockedCompareExchange (&haptic_locks [dwUserIndex], 1, 0))
        return false;

      if (! InterlockedCompareExchange (&haptic_warned [dwUserIndex], 1, 0))
      {
        SK_LOG0 ( ( L"WARNING: Recursive haptic feedback loop detected on XInput controller %lu!",
                     dwUserIndex ),
                  L"  Input   " );

        ////extern void
        ////SK_ImGui_Warning (const wchar_t* wszMessage);
        ////
        SK_ImGui_Warning (L"Problematic third-party XInput software detected (infinite haptic feedback loop), disabling vibration."
                          L"\n\n\tRestart your game to restore vibration support.");
      }

      config.input.gamepad.xinput.hook_setstate = false;

      return true;
    }

    InterlockedDecrement (&haptic_locks [dwUserIndex]);

    return false;
  }

  volatile ULONG haptic_locks  [XUSER_MAX_COUNT] = {     0,     0,     0,     0 };
  volatile ULONG haptic_warned [XUSER_MAX_COUNT] = { FALSE, FALSE, FALSE, FALSE };

  volatile DWORD
    LastSlotState [XUSER_MAX_COUNT] = {
      ERROR_DEVICE_NOT_CONNECTED, ERROR_DEVICE_NOT_CONNECTED,
      ERROR_DEVICE_NOT_CONNECTED, ERROR_DEVICE_NOT_CONNECTED
    };
};

SK_XInputContext xinput_ctx;

////
// Safe, but too slow for some games (i.e. Scarlet Nexus) that hammer
//   XInput on multiple threads simultaneously
//
#if 0
#define SK_XINPUT_CALL(lock,idx,_invoke) [&]()-> \
        auto                                     \
        {                                        \
          std::scoped_lock <                     \
            std::recursive_mutex,                \
            std::recursive_mutex                 \
          > lock_and_call (                      \
            lock, xinput_ctx.cs_hook [idx]       \
          );                                     \
                                                 \
          return (_invoke);                      \
        }()
#else
#define SK_XINPUT_CALL(lock,idx,_invoke) [&]()-> \
        auto                                     \
        {                                        \
          return (_invoke);                      \
        }()
#endif

void
SK_ApplyQueuedHooksIfInit (void)
{
  //extern volatile LONG __SK_Init;
  //if (ReadAcquire (&__SK_Init) == 1)


  if (ReadAcquire (&__SK_Init) > 0) SK_ApplyQueuedHooks ();
}

const char*
SK_XInput_GetPrimaryHookName (void)
{
  if (xinput_ctx.primary_hook == &xinput_ctx.XInput1_3)
    return "XInput 1.3";

  if (xinput_ctx.primary_hook == &xinput_ctx.XInput1_4)
    return "XInput 1.4";

  if (xinput_ctx.primary_hook == &xinput_ctx.XInput9_1_0)
    return "XInput 9.1.0";

  return "Unknown";
}

#define SK_XINPUT_READ(type)  SK_XInput_Backend->markRead   (type);
#define SK_XINPUT_WRITE(type) SK_XInput_Backend->markWrite  (type);
#define SK_XINPUT_VIEW(slot)  SK_XInput_Backend->markViewed ((sk_input_dev_type)(1 << slot));

static SK_LazyGlobal <std::unordered_set <HMODULE>> warned_modules;

void
SK_XInput_EstablishPrimaryHook ( HMODULE                       hModCaller,
                                 SK_XInputContext::instance_s* pCtx )
{
  // Calling module (return address) indicates the game made this call
  if (hModCaller == SK_Modules->HostApp ())
    InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, pCtx);

  // Third-party software polled the controller, it better be using the same
  //   interface version as the game is or a lot of input emulation software
  //     will not work correctly.
  else if (pCtx != ReadPointerAcquire ((volatile LPVOID*)&xinput_ctx.primary_hook) &&
                   ReadPointerAcquire ((volatile LPVOID*)&xinput_ctx.primary_hook) != nullptr)
  {
    if (! warned_modules->count (hModCaller))
    {
      SK_LOG0 ( ( L"WARNING: Third-party module '%s' uses different XInput interface version "
                  L"(%s) than the game (%s); input reassignment software may not work correctly.",
                    SK_GetModuleName (hModCaller).c_str (),
                    pCtx->wszModuleName,
                   ((SK_XInputContext::instance_s *)ReadPointerAcquire (
                     (LPVOID *)&xinput_ctx.primary_hook
                   ))->wszModuleName ),
                  L"Input Mgr." );

      warned_modules->insert (hModCaller);
    }
  }
}


extern bool
SK_ImGui_FilterXInput (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState );

BOOL xinput_enabled = TRUE;

// Unlike our hook, this actually turns XInput off. The hook just causes
//   us to filter data while continuing to read it.
bool
SK_XInput_Enable ( BOOL bEnable )
{
  bool before =
   (xinput_enabled != FALSE);

  if (  xinput_ctx.primary_hook                        != nullptr)
  { if (xinput_ctx.primary_hook->XInputEnable_Original != nullptr)
        xinput_ctx.primary_hook->XInputEnable_Original (bEnable); }

  SK_XInputContext::instance_s*
    contexts [] =
      { //&(xinput_ctx->XInput9_1_0), // Undefined
        &(xinput_ctx.XInput1_3),
        &(xinput_ctx.XInput1_4) };

  for ( auto& context : contexts )
  {
    if (context->XInputEnable_Original != nullptr)
        context->XInputEnable_Original (bEnable);
  }

  xinput_enabled =
    bEnable;

  return before;
}

void
WINAPI
XInputEnable1_3_Detour (
  _In_ BOOL enable )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  if (config.window.background_render)
  {
    xinput_enabled             = TRUE;
    pCtx->XInputEnable_Original (TRUE);
  }

  else
  {
    xinput_enabled             = enable;
    pCtx->XInputEnable_Original (enable);
  }

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);
}


DWORD
WINAPI
XInputGetState1_3_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pState      == nullptr)         return ERROR_SUCCESS;

  RtlZeroMemory (&pState->Gamepad, sizeof (XINPUT_GAMEPAD));

  if (! xinput_enabled)
    return ERROR_SUCCESS;

  if (dwUserIndex >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL ( xinput_ctx.cs_poll      [dwUserIndex],
                                                dwUserIndex,
                 pCtx->XInputGetState_Original (dwUserIndex, pState)
      );

    SK_ImGui_FilterXInput           (dwUserIndex, pState);
  SK_XInput_PacketJournalize (dwRet, dwUserIndex, pState);

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

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

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pState      == nullptr)         return ERROR_SUCCESS;

  RtlZeroMemory (&pState->Gamepad, sizeof (XINPUT_GAMEPAD));

  if (! xinput_enabled)
    return ERROR_SUCCESS;

  if (dwUserIndex >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL ( xinput_ctx.cs_poll        [dwUserIndex],
                                                  dwUserIndex,
                 pCtx->XInputGetStateEx_Original (dwUserIndex, pState)
      );

    SK_ImGui_FilterXInput             (dwUserIndex, (XINPUT_STATE *)pState);
    SK_XInput_PacketJournalize (dwRet, dwUserIndex, (XINPUT_STATE *)pState);

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

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

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pCapabilities == nullptr)         return (DWORD)E_POINTER;

  RtlZeroMemory (pCapabilities, sizeof (XINPUT_CAPABILITIES));

  if (dwUserIndex   >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL ( xinput_ctx.cs_caps      [dwUserIndex],
                                                dwUserIndex,
          pCtx->XInputGetCapabilities_Original (dwUserIndex, dwFlags, pCapabilities)
      );

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

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

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pBatteryInformation == nullptr)         return (DWORD)E_POINTER;

  RtlZeroMemory (pBatteryInformation, sizeof (XINPUT_BATTERY_INFORMATION));

  if (dwUserIndex         >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL ( xinput_ctx.cs_caps         [dwUserIndex],
                                                   dwUserIndex,
       pCtx->XInputGetBatteryInformation_Original (dwUserIndex, devType, pBatteryInformation)
      );

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

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

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_WRITE (sk_input_dev_type::Gamepad)

  if (xinput_ctx.preventHapticRecursion (dwUserIndex, true))
    return ERROR_SUCCESS;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_3;

  if (! config.input.gamepad.xinput.hook_setstate)
    return pCtx->XInputSetState_Original (dwUserIndex, pVibration);

  if (! xinput_enabled)
  {
    xinput_ctx.preventHapticRecursion (dwUserIndex, false);
    return ERROR_SUCCESS;
  }

  if (pVibration  == nullptr)         { xinput_ctx.preventHapticRecursion            (dwUserIndex, false);
                                        return SK_XINPUT_CALL ( xinput_ctx.cs_haptic [dwUserIndex],
                                                                                      dwUserIndex,
                                                       pCtx->XInputSetState_Original (dwUserIndex, pVibration));
                                      }
  if (dwUserIndex >= XUSER_MAX_COUNT) { xinput_ctx.preventHapticRecursion (dwUserIndex, false);
                                        RtlZeroMemory (pVibration, sizeof (XINPUT_VIBRATION));
                                        return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
                                      }

  bool nop = ( SK_ImGui_WantGamepadCapture ()                       &&
                 /*dwUserIndex == config.input.gamepad.xinput.ui_slot &&*/
                   config.input.gamepad.haptic_ui ) ||
               config.input.gamepad.disable_rumble;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED :
                                                             nop ?
                                                   ERROR_SUCCESS :
    SK_XINPUT_CALL ( xinput_ctx.cs_haptic [dwUserIndex],
                                           dwUserIndex,
            pCtx->XInputSetState_Original (dwUserIndex, pVibration)
    );

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

  dwRet =
    SK_XInput_PlaceHoldSet (dwRet, dwUserIndex, pVibration);
    SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  xinput_ctx.preventHapticRecursion (dwUserIndex, false);

  return
    dwRet;
}


void
WINAPI
XInputEnable1_4_Detour (
  _In_ BOOL enable)
{
  HMODULE hModCaller =
    SK_GetCallingDLL ( );

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  if (config.window.background_render)
  {
    xinput_enabled             = TRUE;
    pCtx->XInputEnable_Original (TRUE);
  }
  else
  {
    xinput_enabled             = enable;
    pCtx->XInputEnable_Original (enable);
  }

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);
}


DWORD
WINAPI
XInputGetState1_4_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pState      == nullptr)         return ERROR_SUCCESS;

  RtlZeroMemory (&pState->Gamepad, sizeof (XINPUT_GAMEPAD));

  if (! xinput_enabled)
    return ERROR_SUCCESS;

  if (dwUserIndex >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL ( xinput_ctx.cs_poll [dwUserIndex],
                                           dwUserIndex,
            pCtx->XInputGetState_Original (dwUserIndex, pState)
      );

    SK_ImGui_FilterXInput           (dwUserIndex, pState);
  SK_XInput_PacketJournalize (dwRet, dwUserIndex, pState);

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

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
  struct cached_state_s
  {
    ULONGLONG call_count = 0ULL;

    struct call_result_s
    {
      DWORD           ThreadId =             0;
      DWORD           Status   = OLE_E_NOCACHE;
      XINPUT_STATE_EX Sample   = {           };
    } _last_result;

    struct timing_s
    {
      LONGLONG        t0;
      LONGLONG        tCache;
      ULONG64         ulFrame0;
      ULONG64         ulCacheFrame;

      struct polling_traits_s
      {
        ULONG         ulSubMillisecondReqs  =   0UL;
        ULONG         ulSubFrameReqs        =   0UL;
        ULONG         ulContextSwitches     =   0UL;
        BOOL          bPolledByRenderThread = FALSE;
        BOOL          bPolledByWindowThread = FALSE;
      } _observed;
    } _timing;

    void updateState (DWORD dwRetVal, XINPUT_STATE_EX& xiState)
    {
      DWORD ThreadId =
        SK_GetCurrentThreadId ();

      if ( ThreadId !=
             std::exchange (_last_result.ThreadId,
                                         ThreadId) )
      {
        if (_timing.t0 != 0)
        {
          ++_timing._observed.ulContextSwitches;
            _timing._observed.ulSubMillisecondReqs = 0;
            _timing._observed.ulSubFrameReqs       = 0;
        }

        else
        {
          _timing.      t0 = SK_QueryPerf ().QuadPart;
          _timing.ulFrame0 = SK_GetFramesDrawn ();
        }
      }

      _last_result.Status = dwRetVal;
      _last_result.Sample = xiState;

      LONGLONG qpcTime =
        SK_QueryPerf ().QuadPart;

      if (++call_count > 1)
      {
        if ((qpcTime - _timing.tCache) / (SK_QpcTicksPerMs) < 1)
          ++_timing._observed.ulSubMillisecondReqs;
        if (_timing.ulCacheFrame == SK_GetFramesDrawn ())
          ++_timing._observed.ulSubFrameReqs;
      }

      _timing.ulCacheFrame = SK_GetFramesDrawn ();
      _timing.tCache       = qpcTime;

      if (! _timing._observed.bPolledByWindowThread)
            _timing._observed.bPolledByWindowThread =
                               SK_Win32_IsGUIThread ();

      if (! _timing._observed.bPolledByRenderThread)
            _timing._observed.bPolledByRenderThread =
            ( ThreadId == SK_GetCurrentRenderBackend ().thread );
    }

    bool shouldThrottle (void)
    {
      if (_timing._observed.bPolledByRenderThread)
        return false;

    //if (_timing._observed.bPolledByWindowThread)
    //  return false;

      if (_timing._observed.ulSubMillisecondReqs > 1)
        return true;

      if (_last_result.Status != ERROR_SUCCESS)
        return true;

      return false;
    }
  } static throttle_cache [XUSER_MAX_COUNT];

  HMODULE hModCaller = SK_GetCallingDLL ();

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pState      == nullptr)         return ERROR_SUCCESS;

  RtlZeroMemory (&pState->Gamepad, sizeof (XINPUT_GAMEPAD));

  if (! xinput_enabled)
    return ERROR_SUCCESS;

  if (dwUserIndex >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL ( xinput_ctx.cs_poll [dwUserIndex],
                                           dwUserIndex,
          pCtx->XInputGetStateEx_Original (dwUserIndex, pState)
      );

    SK_ImGui_FilterXInput           (       dwUserIndex, (XINPUT_STATE *)pState);
    SK_XInput_PacketJournalize      (dwRet, dwUserIndex, (XINPUT_STATE *)pState);

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

  dwRet =
    SK_XInput_PlaceHoldEx           (dwRet, dwUserIndex, pState);

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  ////if (config.input.gamepad.xinput.placehold [dwUserIndex])
  ////{
  ////      throttle_cache [dwUserIndex].updateState (dwRet, *pState);
  ////  if (throttle_cache [dwUserIndex].shouldThrottle ())
  ////  {
  ////    if (throttle_cache [dwUserIndex]._timing._observed.bPolledByWindowThread && SK_Win32_IsGUIThread ())
  ////      MsgWaitForMultipleObjects (0, nullptr, FALSE, 1, QS_ALLINPUT);
  ////    else
  ////    {
  ////      YieldProcessor (       );
  ////      SK_SleepEx     (1, TRUE);
  ////    }
  ////  }
  ////}

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

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pCapabilities == nullptr)         return (DWORD)E_POINTER;

  RtlZeroMemory (pCapabilities, sizeof (XINPUT_CAPABILITIES));

  if (dwUserIndex   >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL (   xinput_ctx.cs_caps [dwUserIndex],
                                             dwUserIndex,
       pCtx->XInputGetCapabilities_Original (dwUserIndex, dwFlags, pCapabilities)
      );

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

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

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pBatteryInformation == nullptr) return (DWORD)E_POINTER;

  RtlZeroMemory (pBatteryInformation, sizeof (XINPUT_BATTERY_INFORMATION));

  if (dwUserIndex >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL (          xinput_ctx.cs_caps [dwUserIndex],
                                                    dwUserIndex,
        pCtx->XInputGetBatteryInformation_Original (dwUserIndex, devType,
                                                    pBatteryInformation)
      );

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

  dwRet =
    SK_XInput_PlaceHoldBattery (dwRet, dwUserIndex, devType, pBatteryInformation);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  if (config.render.framerate.max_delta_time > 0)
  {
    if (dwRet != ERROR_DEVICE_NOT_CONNECTED)
    {
      extern DWORD WINAPI SK_DelayExecution (double dMilliseconds, BOOL bAlertable) noexcept;
      SK_DelayExecution (0.5, TRUE);
    }
  }

  return dwRet;
}

DWORD
WINAPI
XInputSetState1_4_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_WRITE (sk_input_dev_type::Gamepad)

  if (xinput_ctx.preventHapticRecursion (dwUserIndex, true))
    return ERROR_SUCCESS;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  if (! config.input.gamepad.xinput.hook_setstate)
    return pCtx->XInputSetState_Original (dwUserIndex, pVibration);

  if (! xinput_enabled)
  {
    xinput_ctx.preventHapticRecursion (dwUserIndex, false);
    return ERROR_SUCCESS;
  }

  if (pVibration  == nullptr)         { xinput_ctx.preventHapticRecursion            (dwUserIndex, false);
                                        return SK_XINPUT_CALL ( xinput_ctx.cs_haptic [dwUserIndex],
                                                                                      dwUserIndex,
                                                       pCtx->XInputSetState_Original (dwUserIndex, pVibration));
                                      }
  if (dwUserIndex >= XUSER_MAX_COUNT) { xinput_ctx.preventHapticRecursion (dwUserIndex, false);
                                        RtlZeroMemory (pVibration, sizeof (XINPUT_VIBRATION));
                                        return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
                                      }

  bool nop = ( SK_ImGui_WantGamepadCapture ()                       &&
                 /*dwUserIndex == config.input.gamepad.xinput.ui_slot &&*/
                   config.input.gamepad.haptic_ui ) ||
               config.input.gamepad.disable_rumble;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED :
                                                             nop ?
                                                   ERROR_SUCCESS :
    SK_XINPUT_CALL ( xinput_ctx.cs_haptic [dwUserIndex],
                                           dwUserIndex,
            pCtx->XInputSetState_Original (dwUserIndex, pVibration)
    );

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

  dwRet =
    SK_XInput_PlaceHoldSet (dwRet, dwUserIndex, pVibration);
    SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  xinput_ctx.preventHapticRecursion (dwUserIndex, false);

  return
    dwRet;
}


DWORD
WINAPI
SK_XInput_GetBatteryInformation (_In_  DWORD                       dwUserIndex,
                                 _In_  BYTE                        devType,
                                 _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation)
{
  if (xinput_ctx.XInput1_4.XInputGetBatteryInformation_Original != nullptr)
  {
    return
      xinput_ctx.XInput1_4.XInputGetBatteryInformation_Original (dwUserIndex, devType, pBatteryInformation);
  }

  else if (xinput_ctx.XInput1_3.XInputGetBatteryInformation_Original != nullptr)
  {
    return
      xinput_ctx.XInput1_3.XInputGetBatteryInformation_Original (dwUserIndex, devType, pBatteryInformation);
  }

  return E_NOTIMPL;
}


DWORD
WINAPI
XInputGetState9_1_0_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pState      == nullptr)         return ERROR_SUCCESS;

  RtlZeroMemory (&pState->Gamepad, sizeof (XINPUT_GAMEPAD));

  if (! xinput_enabled)
    return ERROR_SUCCESS;

  if (dwUserIndex >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput9_1_0;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL ( xinput_ctx.cs_poll [dwUserIndex],
                                           dwUserIndex,
            pCtx->XInputGetState_Original (dwUserIndex, pState)
      );

    SK_ImGui_FilterXInput           (dwUserIndex, pState);
  SK_XInput_PacketJournalize (dwRet, dwUserIndex, pState);

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

  dwRet =
    SK_XInput_PlaceHold      (dwRet, dwUserIndex, pState);

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

//// Yakuza speedup (9.1.0 always returns a fixed set of caps)
static std::pair <DWORD, XINPUT_CAPABILITIES>
    __cached_xi_9_1_0_caps [XUSER_MAX_COUNT] = {
       { ERROR_DEVICE_NOT_CONNECTED, { 0 } },
       { ERROR_DEVICE_NOT_CONNECTED, { 0 } },
       { ERROR_DEVICE_NOT_CONNECTED, { 0 } },
       { ERROR_DEVICE_NOT_CONNECTED, { 0 } }
    };

DWORD
WINAPI
XInputGetCapabilities9_1_0_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if ( static_cast <LONG> (dwUserIndex) >= 0 &&
                           dwUserIndex < XUSER_MAX_COUNT &&
   __cached_xi_9_1_0_caps [dwUserIndex].first == ERROR_SUCCESS )
  {
    if (pCapabilities != nullptr)
    {  *pCapabilities =
         __cached_xi_9_1_0_caps [dwUserIndex].second;
    }

    return
      __cached_xi_9_1_0_caps    [dwUserIndex].first;
  }
////////// --------------- End Cache Lookup ---------------


  if (pCapabilities == nullptr)         return (DWORD)E_POINTER;

  RtlZeroMemory (       pCapabilities,
          sizeof (XINPUT_CAPABILITIES) );

  if (dwUserIndex   >= XUSER_MAX_COUNT) return (DWORD)ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput9_1_0;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL (    xinput_ctx.cs_caps [dwUserIndex],
                                              dwUserIndex,
        pCtx->XInputGetCapabilities_Original (dwUserIndex, dwFlags, pCapabilities)
      );

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

  dwRet =
    SK_XInput_PlaceHoldCaps (dwRet, dwUserIndex, dwFlags, pCapabilities);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  __cached_xi_9_1_0_caps [dwUserIndex] =
    { dwRet, *pCapabilities };

  return dwRet;
}

DWORD
WINAPI
XInputSetState9_1_0_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_WRITE (sk_input_dev_type::Gamepad)

  if (xinput_ctx.preventHapticRecursion (dwUserIndex, true))
    return ERROR_SUCCESS;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput9_1_0;

  if (! config.input.gamepad.xinput.hook_setstate)
    return pCtx->XInputSetState_Original (dwUserIndex, pVibration);

  if (! xinput_enabled)
  {
    xinput_ctx.preventHapticRecursion (dwUserIndex, false);
    return ERROR_SUCCESS;
  }

  if (pVibration  == nullptr)         { xinput_ctx.preventHapticRecursion            (dwUserIndex, false);
                                        return SK_XINPUT_CALL ( xinput_ctx.cs_haptic [dwUserIndex],
                                                                                      dwUserIndex,
                                                       pCtx->XInputSetState_Original (dwUserIndex, pVibration));
                                      }
  if (dwUserIndex >= XUSER_MAX_COUNT) { xinput_ctx.preventHapticRecursion (dwUserIndex, false);
                                        RtlZeroMemory (pVibration, sizeof (XINPUT_VIBRATION));
                                        return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
                                      }

  bool nop = ( SK_ImGui_WantGamepadCapture ()                       &&
               /*dwUserIndex == config.input.gamepad.xinput.ui_slot &&*/
                   config.input.gamepad.haptic_ui ) ||
               config.input.gamepad.disable_rumble;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED :
                                                             nop ?
                                                   ERROR_SUCCESS :
    SK_XINPUT_CALL ( xinput_ctx.cs_haptic [dwUserIndex],
                                           dwUserIndex,
            pCtx->XInputSetState_Original (dwUserIndex, pVibration)
    );

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

  dwRet =
    SK_XInput_PlaceHoldSet (dwRet, dwUserIndex, pVibration);
    SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  xinput_ctx.preventHapticRecursion (dwUserIndex, false);

  return
    dwRet;
}


void
SK_Input_HookXInputContext (SK_XInputContext::instance_s* pCtx)
{
  pCtx->XInputGetState_Target =
    SK_GetProcAddress ( pCtx->wszModuleName,
                                   "XInputGetState" );

  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "XInputGetState",
                              pCtx->XInputGetState_Detour,
     static_cast_p2p <void> (&pCtx->XInputGetState_Original) );

  pCtx->XInputGetCapabilities_Target =
    SK_GetProcAddress ( pCtx->wszModuleName,
                                   "XInputGetCapabilities" );

  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "XInputGetCapabilities",
                              pCtx->XInputGetCapabilities_Detour,
     static_cast_p2p <void> (&pCtx->XInputGetCapabilities_Original) );

  if (config.input.gamepad.xinput.hook_setstate)
  {
    pCtx->XInputSetState_Target =
      SK_GetProcAddress ( pCtx->wszModuleName,
                                     "XInputSetState" );

    SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                     "XInputSetState",
                                pCtx->XInputSetState_Detour,
       static_cast_p2p <void> (&pCtx->XInputSetState_Original) );
  }

  pCtx->XInputGetBatteryInformation_Target =
    SK_GetProcAddress ( pCtx->wszModuleName,
                                   "XInputGetBatteryInformation" );

  // Down-level (XInput 9_1_0) does not have XInputGetBatteryInformation
  //
  if (pCtx->XInputGetBatteryInformation_Target != nullptr)
  {
    SK_CreateDLLHook2 (       pCtx->wszModuleName,
                                   "XInputGetBatteryInformation",
                              pCtx->XInputGetBatteryInformation_Detour,
     static_cast_p2p <void> (&pCtx->XInputGetBatteryInformation_Original) );
  }

  pCtx->XInputGetStateEx_Target =
    SK_GetProcAddress ( pCtx->wszModuleName,
                              XINPUT_GETSTATEEX_ORDINAL );

  // Down-level (XInput 9_1_0) does not have XInputGetStateEx (100)
  //
  if (pCtx->XInputGetStateEx_Target != nullptr)
  {
    SK_CreateDLLHook2 (       pCtx->wszModuleName,
                              XINPUT_GETSTATEEX_ORDINAL,
                              pCtx->XInputGetStateEx_Detour,
     static_cast_p2p <void> (&pCtx->XInputGetStateEx_Original) );
  }

  pCtx->XInputEnable_Target =
    SK_GetProcAddress ( pCtx->wszModuleName,
                              "XInputEnable" );

  // Down-level (XInput 9_1_0) does not have XInputEnable
  //
  if (pCtx->XInputEnable_Target != nullptr)
  {
    SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                     "XInputEnable",
                                pCtx->XInputEnable_Detour,
       static_cast_p2p <void> (&pCtx->XInputEnable_Original) );
  }

  SK_ApplyQueuedHooksIfInit ();

  if (pCtx->XInputGetState_Target != nullptr)
    memcpy (pCtx->orig_inst, pCtx->XInputGetState_Target,                   6);

  if (pCtx->XInputSetState_Target != nullptr)
    memcpy (pCtx->orig_inst_set, pCtx->XInputSetState_Target,               6);

  if (pCtx->XInputGetCapabilities_Target != nullptr)
    memcpy (pCtx->orig_inst_caps, pCtx->XInputGetCapabilities_Target,       6);

  if (pCtx->XInputGetBatteryInformation_Target != nullptr)
    memcpy (pCtx->orig_inst_batt, pCtx->XInputGetBatteryInformation_Target, 6);

  if (pCtx->XInputGetStateEx_Target != nullptr)
    memcpy (pCtx->orig_inst_ex, pCtx->XInputGetStateEx_Target,              6);

  if (pCtx->XInputEnable_Target != nullptr)
    memcpy (pCtx->orig_inst_enable, pCtx->XInputEnable_Target,              6);
}

static volatile LONG __hooked_xi_1_4   = FALSE;
static volatile LONG __hooked_xi_1_3   = FALSE;
static volatile LONG __hooked_xi_9_1_0 = FALSE;

void
SK_Input_HookXInput1_4 (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  if (ReadAcquire (&__hooked_xi_1_4) >= 2)
    return;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return;

  if (! InterlockedCompareExchangeAcquire (&__hooked_xi_1_4, TRUE, FALSE))
  {
    if (ReadPointerAcquire ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
      pTLS->input_core->ctx_init_thread = TRUE;

    SK_XInputContext::instance_s* pCtx =
      &xinput_ctx.XInput1_4;

    SK_LOG0 ( ( L"  >> Hooking XInput 1.4" ),
                L"  Input   " );

    pCtx->wszModuleName                      = L"XInput1_4.dll";
    pCtx->hMod                               = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (pCtx->hMod))
    {
      pCtx->XInputEnable_Detour                = XInputEnable1_4_Detour;
      pCtx->XInputGetState_Detour              = XInputGetState1_4_Detour;
      pCtx->XInputGetStateEx_Detour            = XInputGetStateEx1_4_Detour;
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities1_4_Detour;
      pCtx->XInputGetBatteryInformation_Detour = XInputGetBatteryInformation1_4_Detour;

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState1_4_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);

      if (ReadPointerAcquire       ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
        InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, &xinput_ctx.XInput1_4);
    }

    InterlockedIncrementRelease (&__hooked_xi_1_4);
  }

  if (! pTLS->input_core->ctx_init_thread)
    SK_Thread_SpinUntilAtomicMin (&__hooked_xi_1_4, 2);
}

void
SK_Input_HookXInput1_3 (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  if (ReadAcquire (&__hooked_xi_1_3) >= 2)
    return;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return;

  if (! InterlockedCompareExchangeAcquire (&__hooked_xi_1_3, TRUE, FALSE))
  {
    if (ReadPointerAcquire ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
      pTLS->input_core->ctx_init_thread = TRUE;

    SK_XInputContext::instance_s* pCtx =
      &xinput_ctx.XInput1_3;

    SK_LOG0 ( ( L"  >> Hooking XInput 1.3" ),
                L"  Input   " );

    pCtx->wszModuleName                      = L"XInput1_3.dll";
    pCtx->hMod                               = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (pCtx->hMod))
    {
#ifdef XINPUT_UPGRADE
      pCtx->XInputEnable_Detour                = XInputEnable1_3_Detour;
      pCtx->XInputGetState_Detour              = XInputGetState1_3_Detour;
      pCtx->XInputGetStateEx_Detour            = XInputGetStateEx1_3_Detour;
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities1_3_Detour;
      pCtx->XInputGetBatteryInformation_Detour = XInputGetBatteryInformation1_3_Detour;

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState1_3_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);

      pCtx->XInputSetState_Original =
        config.input.gamepad.xinput.hook_setstate? xinput_ctx.XInput1_4.XInputSetState_Original : nullptr;
      pCtx->XInputEnable_Original                = xinput_ctx.XInput1_4.XInputEnable_Original;
      pCtx->XInputGetState_Original              = xinput_ctx.XInput1_4.XInputGetState_Original;
      pCtx->XInputGetStateEx_Original            = xinput_ctx.XInput1_4.XInputGetStateEx_Original;
      pCtx->XInputGetCapabilities_Original       = xinput_ctx.XInput1_4.XInputGetCapabilities_Original;
      pCtx->XInputGetBatteryInformation_Original = xinput_ctx.XInput1_4.XInputGetBatteryInformation_Original;
#else
      pCtx->XInputEnable_Detour                = XInputEnable1_3_Detour;
      pCtx->XInputGetState_Detour              = XInputGetState1_3_Detour;
      pCtx->XInputGetStateEx_Detour            = XInputGetStateEx1_3_Detour;
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities1_3_Detour;
      pCtx->XInputGetBatteryInformation_Detour = XInputGetBatteryInformation1_3_Detour;

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState1_3_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);
#endif

      if (ReadPointerAcquire       ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
#ifdef XINPUT_UPGRADE
        InterlockedExchangePointer((LPVOID*)&xinput_ctx.primary_hook, &xinput_ctx.XInput1_4);
#else
        InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, &xinput_ctx.XInput1_3);
#endif
    }

    InterlockedIncrementRelease (&__hooked_xi_1_3);
  }

  if (! pTLS->input_core->ctx_init_thread)
    SK_Thread_SpinUntilAtomicMin (&__hooked_xi_1_3, 2);
}

void
SK_Input_HookXInput9_1_0 (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  // Upgrade
  //   Passthrough to 1.4
  SK_Input_HookXInput1_4 ();

  if (ReadAcquire (&__hooked_xi_9_1_0) >= 2)
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return;

  if (! InterlockedCompareExchangeAcquire (&__hooked_xi_9_1_0, TRUE, FALSE))
  {
    if (ReadPointerAcquire ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
      pTLS->input_core->ctx_init_thread = TRUE;

    SK_XInputContext::instance_s* pCtx =
      &xinput_ctx.XInput9_1_0;

    SK_LOG0 ( ( L"  >> Hooking XInput9_1_0" ),
                L"  Input   " );

    pCtx->wszModuleName                      = L"XInput9_1_0.dll";
    pCtx->hMod                               = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (pCtx->hMod))
    {
#ifdef XINPUT_UPGRADE
      pCtx->XInputGetState_Detour              = XInputGetState9_1_0_Detour;
      pCtx->XInputGetStateEx_Detour            = nullptr; // Not supported
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities9_1_0_Detour;
      pCtx->XInputGetBatteryInformation_Detour = nullptr; // Not supported

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState9_1_0_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);

      pCtx->XInputSetState_Original =
        config.input.gamepad.xinput.hook_setstate? xinput_ctx.XInput1_4.XInputSetState_Original : nullptr;
      pCtx->XInputEnable_Original                = xinput_ctx.XInput1_4.XInputEnable_Original;
      pCtx->XInputGetState_Original              = xinput_ctx.XInput1_4.XInputGetState_Original;
      pCtx->XInputGetStateEx_Original            = xinput_ctx.XInput1_4.XInputGetStateEx_Original;
      pCtx->XInputGetCapabilities_Original       = xinput_ctx.XInput1_4.XInputGetCapabilities_Original;
      pCtx->XInputGetBatteryInformation_Original = xinput_ctx.XInput1_4.XInputGetBatteryInformation_Original;
#else
      pCtx->XInputGetState_Detour              = XInputGetState9_1_0_Detour;
      pCtx->XInputGetStateEx_Detour            = nullptr; // Not supported
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities9_1_0_Detour;
      pCtx->XInputGetBatteryInformation_Detour = nullptr; // Not supported

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState9_1_0_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);
#endif

      if (ReadPointerAcquire       ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
#ifdef XINPUT_UPGRADE
        InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, &xinput_ctx.XInput1_4);
#else
          InterlockedExchangePointer((LPVOID*)&xinput_ctx.primary_hook, &xinput_ctx.XInput9_1_0);
#endif
    }

    InterlockedIncrementRelease (&__hooked_xi_9_1_0);
  }

  if (! pTLS->input_core->ctx_init_thread)
    SK_Thread_SpinUntilAtomicMin (&__hooked_xi_9_1_0, 2);
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

  auto* pCtx =
    static_cast <SK_XInputContext::instance_s *>
      (ReadPointerAcquire ((volatile LPVOID *)&xinput_ctx.primary_hook));

  if (! pCtx)
    return;


  std::scoped_lock < std::recursive_mutex, std::recursive_mutex,
                     std::recursive_mutex, std::recursive_mutex >
      hook_lock ( xinput_ctx.cs_hook [0], xinput_ctx.cs_hook [1],
                  xinput_ctx.cs_hook [2], xinput_ctx.cs_hook [3] );


  MH_STATUS ret =
    MH_QueueEnableHook (pCtx->XInputGetState_Target);


  // Test for modified hooks
  if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                 ( pCtx->XInputGetState_Target != nullptr &&
           memcmp (pCtx->orig_inst,
                   pCtx->XInputGetState_Target, 6 ) )
     )
  {
    if ( MH_OK == MH_RemoveHook (pCtx->XInputGetState_Target) )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2 (  pCtx->wszModuleName,
                                       "XInputGetState",
                                  pCtx->XInputGetState_Detour,
         static_cast_p2p <void> (&pCtx->XInputGetState_Original) )
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


  if (config.input.gamepad.xinput.hook_setstate)
  {
    ret =
      MH_QueueEnableHook (pCtx->XInputSetState_Target);


    // Test for modified hooks
    if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                   ( pCtx->XInputSetState_Target != nullptr &&
             memcmp (pCtx->orig_inst_set,
                     pCtx->XInputSetState_Target, 6 ) )
       )
    {
      if ( MH_OK == MH_RemoveHook (pCtx->XInputSetState_Target) )
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 (  pCtx->wszModuleName,
                                         "XInputSetState",
                                    pCtx->XInputSetState_Detour,
           static_cast_p2p <void> (&pCtx->XInputSetState_Original) )
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
  }
  pCtx->XInputSetState_Original =
    (XInputSetState_pfn)SK_GetProcAddress (pCtx->hMod, "XInputSetState");


  ret =
    MH_QueueEnableHook (pCtx->XInputGetCapabilities_Target);


  // Test for modified hooks
  if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                 ( pCtx->XInputGetCapabilities_Target != nullptr &&
           memcmp (pCtx->orig_inst_caps,
                   pCtx->XInputGetCapabilities_Target, 6 ) )
     )
  {
    if ( MH_OK == MH_RemoveHook (pCtx->XInputGetCapabilities_Target) )
    {
      if ( MH_OK ==
             SK_CreateDLLHook2 ( pCtx->wszModuleName,
                                      "XInputGetCapabilities",
                                 pCtx->XInputGetCapabilities_Detour,
        static_cast_p2p <void> (&pCtx->XInputGetCapabilities_Original) )
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
      MH_QueueEnableHook (pCtx->XInputGetBatteryInformation_Target);


    // Test for modified hooks
    if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                   ( pCtx->XInputGetBatteryInformation_Target != nullptr &&
             memcmp (pCtx->orig_inst_batt,
                     pCtx->XInputGetBatteryInformation_Target, 6 ) )
       )
    {
      if ( MH_OK == MH_RemoveHook (pCtx->XInputGetBatteryInformation_Target) )
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 ( pCtx->wszModuleName,
                                        "XInputGetBatteryInformation",
                                   pCtx->XInputGetBatteryInformation_Detour,
          static_cast_p2p <void> (&pCtx->XInputGetBatteryInformation_Original) )
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
      MH_QueueEnableHook (pCtx->XInputGetStateEx_Target);

    // Test for modified hooks
    if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                   ( pCtx->XInputGetStateEx_Target != nullptr &&
             memcmp (pCtx->orig_inst_ex,
                     pCtx->XInputGetStateEx_Target, 6 ) )
       )
    {
      if ( MH_OK == MH_RemoveHook (pCtx->XInputGetStateEx_Target) )
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 (  pCtx->wszModuleName,
                                    XINPUT_GETSTATEEX_ORDINAL,
                                    pCtx->XInputGetStateEx_Detour,
           static_cast_p2p <void> (&pCtx->XInputGetStateEx_Original) )
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


  if (pCtx->XInputEnable_Target != nullptr)
  {
    ret =
      MH_QueueEnableHook (pCtx->XInputEnable_Target);


    // Test for modified hooks
    if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                   ( pCtx->XInputEnable_Target != nullptr &&
             memcmp (pCtx->orig_inst_enable,
                     pCtx->XInputEnable_Target, 6 ) )
       )
    {
      if ( MH_OK == MH_RemoveHook (pCtx->XInputEnable_Target) )
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 ( pCtx->wszModuleName,
                                        "XInputEnable",
                                   pCtx->XInputEnable_Detour,
          static_cast_p2p <void> (&pCtx->XInputEnable_Original) )
           )
        {
          SK_LOG0 ( ( L" Re-hooked XInput (Enable) using '%s'...",
                         pCtx->wszModuleName ),
                      L"Input Mgr." );

          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
        }

        else
        {
          SK_LOG0 ( ( L" Failed to re-hook XInput (Enable) using '%s'...",
                 pCtx->wszModuleName ),
              L"Input Mgr." );
        }
      }

      else
      {
        SK_LOG0 ( ( L" Failed to remove XInput (Enable) hook from '%s'...",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }
  }



  SK_ApplyQueuedHooksIfInit ();


  if (pCtx->XInputGetState_Target != nullptr)
    memcpy (pCtx->orig_inst, pCtx->XInputGetState_Target,                   6);

  if (config.input.gamepad.xinput.hook_setstate)
  {
    if (pCtx->XInputSetState_Target != nullptr)
      memcpy (pCtx->orig_inst_set, pCtx->XInputSetState_Target,             6);
  }

  if (pCtx->XInputGetCapabilities_Target != nullptr)
    memcpy (pCtx->orig_inst_caps, pCtx->XInputGetCapabilities_Target,       6);

  if (pCtx->XInputGetBatteryInformation_Target != nullptr)
    memcpy (pCtx->orig_inst_batt, pCtx->XInputGetBatteryInformation_Target, 6);

  if (pCtx->XInputGetStateEx_Target != nullptr)
    memcpy (pCtx->orig_inst_ex, pCtx->XInputGetStateEx_Target,              6);

  if (pCtx->XInputEnable_Target != nullptr)
    memcpy (pCtx->orig_inst_enable, pCtx->XInputEnable_Target,              6);
}



static std::unordered_map <INT, XINPUT_VIBRATION> __xi_pulse_set_values;
static std::unordered_map <INT, ULONG64>          __xi_pulse_set_frames;

bool
WINAPI
SK_XInput_PulseController ( INT   iJoyID,
                            float fStrengthLeft,
                            float fStrengthRight )
{
  // SANITY:  SteamAPI Controller Handle Recycling Gone Out of Control
  //            Something needs to be done about Ghost of a Tale, but
  //              this prevents catastrophe for the time being.
  //
  if (iJoyID > 15)
    return false;

  if (! xinput_enabled)
    return false;

  iJoyID =
    config.input.gamepad.xinput.assignment [std::max (0, std::min (iJoyID, 3))];

  if (ReadULongAcquire (&xinput_ctx.LastSlotState [iJoyID]) != ERROR_SUCCESS)
    return false;

  auto* pCtx =
    static_cast <SK_XInputContext::instance_s *>
      (ReadPointerAcquire ((volatile LPVOID *)&xinput_ctx.primary_hook));

  if ( pCtx                        != nullptr &&
       pCtx->XInputEnable_Original != nullptr )
       pCtx->XInputEnable_Original (true);

  if (config.input.gamepad.disable_rumble)
    return false;

  if (iJoyID < 0 || iJoyID >= XUSER_MAX_COUNT)
    return false;

  XINPUT_VIBRATION vibes;

  vibes.wLeftMotorSpeed  = (WORD)(std::min (0.99999f, fStrengthLeft)  * 65535.0f);
  vibes.wRightMotorSpeed = (WORD)(std::min (0.99999f, fStrengthRight) * 65535.0f);

  if ( pCtx                          != nullptr &&
       pCtx->XInputSetState_Original != nullptr )
  {
    if (__xi_pulse_set_values.count (iJoyID))
    {
      auto& last_val =
        __xi_pulse_set_values [iJoyID];
      auto& last_frame =
        __xi_pulse_set_frames [iJoyID];

      // Avoid redundant call and performance bottleneck caused by Steam overlay
      if ( last_val.wLeftMotorSpeed  == vibes.wLeftMotorSpeed  &&
           last_val.wRightMotorSpeed == vibes.wRightMotorSpeed &&
           last_frame                > ( SK_GetFramesDrawn () - 2 )
         )
      {
        return true;
      }
    }

    __xi_pulse_set_values [iJoyID] = vibes;
    __xi_pulse_set_frames [iJoyID] = SK_GetFramesDrawn ();

    DWORD dwRet =
      SK_XINPUT_CALL ( xinput_ctx.cs_haptic    [iJoyID],
                                                iJoyID,
                 pCtx->XInputSetState_Original (iJoyID, &vibes)
      );

    if ( ERROR_SUCCESS ==
           dwRet )
    {
      return true;
    }
  }

  return false;
}


static volatile DWORD last_poll [XUSER_MAX_COUNT] = { 0, 0, 0, 0 };
static volatile DWORD dwRet     [XUSER_MAX_COUNT] =
  { ERROR_DEVICE_NOT_CONNECTED, ERROR_DEVICE_NOT_CONNECTED,
    ERROR_DEVICE_NOT_CONNECTED, ERROR_DEVICE_NOT_CONNECTED };

volatile ULONG SK_XInput_RefreshTime = 500UL;

void SK_XInput_SetRefreshInterval (ULONG ulIntervalMS)
{
  InterlockedExchange (&SK_XInput_RefreshTime, ulIntervalMS);
}

void SK_XInput_Refresh (UINT iJoyID)
{
  WriteULongRelease (&last_poll [iJoyID], 0UL);
}

static bool
_ShouldRecheckStatus (INT iJoyID)
{
  return (
                         ReadULongAcquire (&last_poll [iJoyID]   ) <
    (SK_timeGetTime () - ReadULongAcquire (&SK_XInput_RefreshTime))
  );
}

static std::wstring  SK_XInput_LinkedVersion = L"";
static bool          SK_XInput_FirstFrame    = true;
static volatile LONG SK_XInput_TriedToHook   = FALSE;

bool
WINAPI
SK_XInput_PollController ( INT           iJoyID,
                           XINPUT_STATE* pState )
{
  SK_RunOnce (SK_XInput_NotifyDeviceArrival ());

  bool queued_hooks = false;

  iJoyID =
    config.input.gamepad.xinput.assignment [
      std::max (0, std::min (iJoyID, 3))
    ];

  if (! config.input.gamepad.hook_xinput)
    return false;

  auto* pCtx =
    static_cast <SK_XInputContext::instance_s *>
      (ReadPointerAcquire ((volatile LPVOID *)&xinput_ctx.primary_hook));

  if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr)
  {
    if (InterlockedCompareExchange (&SK_XInput_TriedToHook, 1, 0))
      return false;

    // First try 1.3, that's generally available.
    /*if (SK_XInput_LinkedVersion.empty () || StrStrIW (SK_XInput_LinkedVersion.c_str (), L"XInput1_3"))*/ SK_Input_HookXInput1_3 ();
    pCtx =
      static_cast <SK_XInputContext::instance_s*>
      (ReadPointerAcquire ((volatile LPVOID*)&xinput_ctx.primary_hook));

  // Then 1.4
    if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr)
    {
      /*if (SK_XInput_LinkedVersion.empty () || StrStrIW (SK_XInput_LinkedVersion.c_str (), L"XInput1_4"))*/ SK_Input_HookXInput1_4 ();
      pCtx =
        static_cast <SK_XInputContext::instance_s*>
        (ReadPointerAcquire ((volatile LPVOID*)&xinput_ctx.primary_hook));
    }

    // Down-level 9_1_0 if all else fails (does not support XInputEx)
    if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr)
    {
      /*if (SK_XInput_LinkedVersion.empty () || StrStrIW (SK_XInput_LinkedVersion.c_str (), L"XInput9_1_0"))*/ SK_Input_HookXInput9_1_0 ();
      pCtx =
        static_cast <SK_XInputContext::instance_s*>
        (ReadPointerAcquire ((volatile LPVOID*)&xinput_ctx.primary_hook));
    }


    // No XInput?! User shouldn't be playing games :P
    if (pCtx == nullptr || pCtx->XInputGetState_Original == nullptr)
    {
      SK_LOG0 ((L"Unable to hook XInput, attempting to enter limp-mode..."
                L" input-related features may not work as intended."),
               L"Input Mgr.");
      InterlockedExchangePointer (
        (LPVOID*)&xinput_ctx.primary_hook,
        &xinput_ctx.XInput1_3);

      pCtx =
        static_cast <SK_XInputContext::instance_s*>
        (ReadPointerAcquire ((volatile LPVOID*)&xinput_ctx.primary_hook));

      HMODULE hModXInput1_3 =
        SK_Modules->LoadLibrary (L"XInput1_3.dll");

      if (SK_Modules->isValid (hModXInput1_3))
      {
        pCtx->XInputGetState_Original =
          (XInputGetState_pfn)
          SK_GetProcAddress (L"XInput1_3.dll",
                             "XInputGetState");
      }
    }

    queued_hooks = true;
  }

  // Lazy-load DLLs if somehow a game uses an XInput DLL not listed
  //   in its import table and also not caught by our LoadLibrary hook
  if (SK_XInput_FirstFrame)
  {
    if ( SK_Modules->isValid    (
         SK_GetModuleHandleW    (L"XInput1_3.dll")
                                )
       ) SK_Input_HookXInput1_3 ();

    if ( SK_Modules->isValid    (
         SK_GetModuleHandleW    (L"XInput1_4.dll")
                                )
       ) SK_Input_HookXInput1_4 ();

    if ( SK_Modules->isValid    (
         SK_GetModuleHandleW    (L"XInput9_1_0.dll")
                                )
       ) SK_Input_HookXInput9_1_0 ();

    SK_XInput_FirstFrame  = false;
    queued_hooks          = true;
  }

  if (iJoyID == -1)
    return true;

  if (iJoyID < 0 || iJoyID >= XUSER_MAX_COUNT)
    return false;



  if (queued_hooks)
    SK_ApplyQueuedHooksIfInit ();
  else
    SK_XInput_RehookIfNeeded ();




  XINPUT_STATE_EX xstate = { };
  xstate.dwPacketNumber  =  1;

  // This function is actually a performance hazzard when no controllers
  //   are plugged in, so ... throttle the sucker.
  if (_ShouldRecheckStatus (iJoyID) && pCtx != nullptr)
  {
    if (pCtx->XInputGetStateEx_Original != nullptr)
    {
      WriteULongRelease ( &dwRet             [iJoyID],
        SK_XINPUT_CALL  ( xinput_ctx.cs_poll [iJoyID],
                                              iJoyID,
             pCtx->XInputGetStateEx_Original (iJoyID, &xstate) )
      );
    }

    // Down-level XInput
    else
    {
      if (pCtx->XInputGetState_Original != nullptr)
      {
        WriteULongRelease ( &dwRet             [iJoyID],
          SK_XINPUT_CALL  ( xinput_ctx.cs_poll [iJoyID],
                                                iJoyID,
                 pCtx->XInputGetState_Original (iJoyID, (XINPUT_STATE *)&xstate) )
        );
      }
    }

    if (ReadULongAcquire (&dwRet     [iJoyID]) == ERROR_DEVICE_NOT_CONNECTED)
       WriteULongRelease (&last_poll [iJoyID], SK_timeGetTime ());
  }

  InterlockedExchange ( &xinput_ctx.LastSlotState [iJoyID],
                         ReadULongAcquire (&dwRet [iJoyID]) );

  if (ReadULongAcquire (&dwRet [iJoyID]) == ERROR_DEVICE_NOT_CONNECTED)
    return false;

  WriteULongRelease (&last_poll [iJoyID], 0); // Feel free to poll this controller again immediately,
                                              //   the performance penalty from a disconnected controller
                                              //     won't be there.

  if (pState != nullptr)
    memcpy (pState, &xstate, sizeof XINPUT_STATE); //-V512

  return true;
}



static sk_import_test_s
  _XInput_ImportsToTry [] = { { "XInput1_3.dll",   false },
                              { "XInput1_4.dll",   false },
                              { "XInput9_1_0.dll", false } };

void
SK_Input_PreHookXInput (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  std::scoped_lock < std::recursive_mutex, std::recursive_mutex,
                     std::recursive_mutex, std::recursive_mutex >
      hook_lock ( xinput_ctx.cs_hook [0], xinput_ctx.cs_hook [1],
                  xinput_ctx.cs_hook [2], xinput_ctx.cs_hook [3] );

  auto* pCtx =
    static_cast <SK_XInputContext::instance_s *>
      (ReadPointerAcquire ((volatile LPVOID *)&xinput_ctx.primary_hook));

  if (pCtx == nullptr)
  {
    auto& tests =
      _XInput_ImportsToTry;

    SK_TestImports (SK_Modules->HostApp (), tests, 3);

    if (tests [0].used || tests [1].used || tests [2].used)
    {
#if 0
      SK_LOG0 ( ( L"Game uses XInput, deferring input hooks..." ),
                  L"  Input   " );
      if (tests [0].used) { /*SK_Input_HookXInput1_3   ();*/ SK_XInput_LinkedVersion = L"XInput1_3.dll";   }
      if (tests [1].used) { /*SK_Input_HookXInput1_4   ();*/ SK_XInput_LinkedVersion = L"XInput1_4.dll";   }
      if (tests [2].used) { /*SK_Input_HookXInput9_1_0 ();*/ SK_XInput_LinkedVersion = L"XInput9_1_0.dll"; }
#else
      SK_LOG0 ( ( L"Game uses XInput, installing input hooks..." ),
                  L"  Input   " );
      if (tests [0].used) { SK_Input_HookXInput1_3   (); }
      if (tests [1].used) { SK_Input_HookXInput1_4   (); }
      if (tests [2].used) { SK_Input_HookXInput9_1_0 (); }

#endif
    }

    if (SK_GetModuleHandleW (L"XInput1_3.dll"))
      SK_Input_HookXInput1_3 ();

    if (SK_GetModuleHandleW (L"XInput1_4.dll"))
      SK_Input_HookXInput1_4 ();

    if (SK_GetModuleHandleW (L"XInput9_1_0.dll"))
      SK_Input_HookXInput9_1_0 ();
  }
}



void
WINAPI
SK_XInput_ZeroHaptics (INT iJoyID)
{
#ifdef SK_STEAM_CONTROLLER_SUPPORT
  auto steam_idx =
    gsl::narrow_cast <ControllerIndex_t> (iJoyID);

  if (steam_input.count && ControllerPresent (steam_idx))
  {
    steam_input.pipe->TriggerVibration (
      steam_input [steam_idx].handle, 0U, 0U
    );
  }
#endif

  iJoyID =
    config.input.gamepad.xinput.assignment [
      std::max (0, std::min (iJoyID, 3))
    ];

  if ( ERROR_SUCCESS !=
         ReadULongAcquire (&xinput_ctx.LastSlotState [iJoyID]) )
  {
    return;
  }


  auto* pCtx = (
    static_cast <SK_XInputContext::instance_s *>
      (ReadPointerAcquire ((volatile LPVOID *)&xinput_ctx.primary_hook))
  );

  if (pCtx == nullptr)
    return;

  if (iJoyID >= XUSER_MAX_COUNT) return;

  if (pCtx->XInputEnable_Original != nullptr)
      pCtx->XInputEnable_Original (FALSE);

  SK_XInput_PulseController (iJoyID, 0.0f, 0.0f);

  if (pCtx->XInputEnable_Original != nullptr)
      pCtx->XInputEnable_Original (xinput_enabled);
}