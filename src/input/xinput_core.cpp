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
#include <system_error>

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
#define XUSER_MAX_INDEX (DWORD)XUSER_MAX_COUNT-1


using SetupDiGetClassDevsW_pfn = HDEVINFO (WINAPI *)(
  /*[in, optional]*/ const GUID *ClassGuid,
  /*[in, optional]*/ PCWSTR     Enumerator,
  /*[in, optional]*/ HWND       hwndParent,
  /*[in]*/           DWORD      Flags
);

static SetupDiGetClassDevsW_pfn
       SetupDiGetClassDevsW_Original = nullptr;

HDEVINFO
SetupDiGetClassDevsW_Detour (
  /*[in, optional]*/ const GUID* ClassGuid,
  /*[in, optional]*/ PCWSTR     Enumerator,
  /*[in, optional]*/ HWND       hwndParent,
  /*[in]*/           DWORD      Flags )
{
  if (SK_GetCallingDLL () != SK_GetDLL ())
  {
    SK_LOG_FIRST_CALL

    static DWORD
        dwLastCall = SK_timeGetTime ();
    if (dwLastCall > SK_timeGetTime () - 500UL)
    {
      SetLastError (ERROR_NOT_FOUND);

      return INVALID_HANDLE_VALUE;
    }

    dwLastCall = SK_timeGetTime ();
  }

  return
    SetupDiGetClassDevsW_Original (ClassGuid, Enumerator, hwndParent, Flags);
}


struct SK_XInputContext
{
  enum
  {
    XInputLevel_NONE  =  0,
    XInputLevel_9_1_0 =  1,
    XInputLevel_1_3   =  2,
    XInputLevel_1_4   =  4,
    XInputLevel_1_2   =  8,
    XInputLevel_1_1   = 16,
    XInputLevel_Uap   = 32,
  };

  struct instance_s
  {
    const wchar_t*                  wszModuleName                        =     L"";
    HMODULE                         hMod                                 = nullptr;

    XInputGetState_pfn              XInputGetState_Detour                = nullptr;
    XInputGetState_pfn              XInputGetState_Original              = nullptr;
    LPVOID                          XInputGetState_Target                = nullptr;

    uint8_t                         orig_inst      [7]                   =   {   };

    XInputGetKeystroke_pfn          XInputGetKeystroke_Detour            = nullptr;
    XInputGetKeystroke_pfn          XInputGetKeystroke_Original          = nullptr;
    LPVOID                          XInputGetKeystroke_Target            = nullptr;

    uint8_t                         orig_keystroke_inst [7]              =   {   };

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

    XInputPowerOff_pfn              XInputPowerOff_Detour                = nullptr;
    XInputPowerOff_pfn              XInputPowerOff_Original              = nullptr;
    LPVOID                          XInputPowerOff_Target                = nullptr;

    uint8_t                         orig_inst_poweroff [7]               =   {   };
  } XInputUap, XInput1_1, XInput1_2,
    XInput1_3, XInput1_4, XInput9_1_0,
    XInput_SK;

  std::recursive_mutex cs_poll   [XUSER_MAX_COUNT] = { };
  std::recursive_mutex cs_haptic [XUSER_MAX_COUNT] = { };
  std::recursive_mutex cs_hook   [XUSER_MAX_COUNT] = { };
  std::recursive_mutex cs_caps   [XUSER_MAX_COUNT] = { };

  volatile instance_s*              primary_hook                         = nullptr;
  volatile LONG                     primary_level                        = XInputLevel_NONE;

  bool preventHapticRecursion (DWORD dwUserIndex, bool enter)
  {
    if (! config.input.gamepad.xinput.hook_setstate)
      return true;

    dwUserIndex =
      std::min (dwUserIndex, (DWORD)XUSER_MAX_COUNT); // +1 for overflow

    if (enter)
    {
      if (! InterlockedCompareExchange (&haptic_locks  [dwUserIndex], 1, 0))
        return false;

      if (! InterlockedCompareExchange (&haptic_warned [dwUserIndex], 1, 0))
      {
        SK_LOG0 ( ( L"WARNING: Recursive haptic feedback loop detected on XInput controller %lu!",
                     dwUserIndex ),
                  L"  Input   " );

        SK_ImGui_Warning (L"Problematic third-party XInput software detected (infinite haptic feedback loop), disabling vibration."
                          L"\n\n\tRestart your game to restore vibration support.");
      }

      config.input.gamepad.xinput.hook_setstate = false;

      return true;
    }

    InterlockedDecrement (&haptic_locks [dwUserIndex]);

    return false;
  }

  volatile ULONG haptic_locks  [XUSER_MAX_COUNT+1] = {     0,     0,     0,     0,     0 };
  volatile ULONG haptic_warned [XUSER_MAX_COUNT+1] = { FALSE, FALSE, FALSE, FALSE, FALSE };

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
  if (xinput_ctx.primary_hook == &xinput_ctx.XInput1_4)
    return "XInput 1.4";

  if (xinput_ctx.primary_hook == &xinput_ctx.XInput1_3)
#ifdef XINPUT_UPGRADE
    return (const char *)u8"XInput 1.3→1.4";
#else
    return                 "XInput 1.3";
#endif

  if (xinput_ctx.primary_hook == &xinput_ctx.XInput1_2)
#ifdef XINPUT_UPGRADE
    return (const char *)u8"XInput 1.2→1.4";
#else
    return                 "XInput 1.2";
#endif

  if (xinput_ctx.primary_hook == &xinput_ctx.XInput1_1)
#ifdef XINPUT_UPGRADE
    return (const char *)u8"XInput 1.1→1.4";
#else
    return                 "XInput 1.1";
#endif

  if (xinput_ctx.primary_hook == &xinput_ctx.XInputUap)
#ifdef XINPUT_UPGRADE
    return (const char *)u8"XInput UAP→1.4";
#else
    return                 "XInput UAP";
#endif

  if (xinput_ctx.primary_hook == &xinput_ctx.XInput9_1_0)
#ifdef XINPUT_UPGRADE
    return (const char *)u8"XInput 9.1.0→1.4";
#else
    return                 "XInput 9.1.0";
#endif

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
  if (hModCaller == skModuleRegistry::HostApp () ||
   ReadPointerAcquire ((volatile LPVOID *)&xinput_ctx.primary_hook) == nullptr)
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

extern bool
_Success_(false)
SK_ImGui_FilterXInputKeystroke (
  _In_  DWORD             dwUserIndex,
  _Out_ XINPUT_KEYSTROKE *pKeystroke );

BOOL xinput_enabled = TRUE;

// Unlike our hook, this actually turns XInput off. The hook just causes
//   us to filter data while continuing to read it.
bool
SK_XInput_Enable ( BOOL bEnable )
{
  bool before =
   (xinput_enabled != FALSE);

  static XInputEnable_pfn
         XInputEnable_SK =
        (XInputEnable_pfn)SK_GetProcAddress (xinput_ctx.XInput_SK.hMod,
        "XInputEnable"                      );

  SK_XInputContext::instance_s*
    contexts [] =
      { //&(xinput_ctx->XInput9_1_0), // Undefined
        &(xinput_ctx.XInput1_4),
        &(xinput_ctx.XInput1_3),
        &(xinput_ctx.XInputUap) };

  for ( auto& context : contexts )
  {
    if (context->XInputEnable_Original != nullptr)
        context->XInputEnable_Original (bEnable);
  }

  if (XInputEnable_SK != nullptr)
      XInputEnable_SK (bEnable);

  xinput_enabled =
    bEnable;

  return before;
}

void
WINAPI
XInputEnable1_4_Detour (
  _In_ BOOL enable)
{
  SK_LOG_FIRST_CALL

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
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, XUSER_MAX_INDEX)];

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

  if (dwUserIndex == config.input.gamepad.xinput.ui_slot && ReadAcquire (&SK_ScePad_Backend->reads [2]) > 0)
    return ERROR_DEVICE_NOT_CONNECTED;

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
      DWORD           ThreadId =                  0UL;
      DWORD           Status   = (DWORD)OLE_E_NOCACHE;
      XINPUT_STATE_EX Sample   = {                  };
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

    void updateState (DWORD dwRetVal, const XINPUT_STATE_EX& xiState)
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

      const LONGLONG qpcTime =
        SK_QueryPerf ().QuadPart;

      if (++call_count > 1)
      {
        if ((qpcTime - _timing.tCache) / (SK_PerfTicksPerMs) < 1)
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

    bool shouldThrottle (void) noexcept
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
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, XUSER_MAX_INDEX)];

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
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, XUSER_MAX_INDEX)];

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
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, XUSER_MAX_INDEX)];

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
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, XUSER_MAX_INDEX)];

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


void
WINAPI
XInputEnableUap_Detour (
  _In_ BOOL enable )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInputUap
  );

  return XInputEnable1_4_Detour (enable);
}


DWORD
WINAPI
XInputGetStateUap_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInputUap
  );

  return XInputGetState1_4_Detour (dwUserIndex, pState);
}

DWORD
WINAPI
XInputGetCapabilitiesUap_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInputUap
  );

  return XInputGetCapabilities1_4_Detour (dwUserIndex, dwFlags, pCapabilities);
}

DWORD
WINAPI
XInputGetBatteryInformationUap_Detour (
  _In_  DWORD                       dwUserIndex,
  _In_  BYTE                        devType,
  _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInputUap
  );

  return XInputGetBatteryInformation1_4_Detour (dwUserIndex, devType, pBatteryInformation);
}

DWORD
WINAPI
XInputSetStateUap_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInputUap
  );

  return XInputSetState1_4_Detour (dwUserIndex, pVibration);
}

void
WINAPI
XInputEnable1_3_Detour (
  _In_ BOOL enable )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_3
  );

  return XInputEnable1_4_Detour (enable);
}


DWORD
WINAPI
XInputGetState1_3_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_3
  );

  return XInputGetState1_4_Detour (dwUserIndex, pState);
}

DWORD
WINAPI
XInputGetStateEx1_3_Detour (
  _In_  DWORD            dwUserIndex,
  _Out_ XINPUT_STATE_EX *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_3
  );

  return XInputGetStateEx1_4_Detour (dwUserIndex, pState);
}

DWORD
WINAPI
XInputGetCapabilities1_3_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_3
  );

  return XInputGetCapabilities1_4_Detour (dwUserIndex, dwFlags, pCapabilities);
}

DWORD
WINAPI
XInputGetBatteryInformation1_3_Detour (
  _In_  DWORD                       dwUserIndex,
  _In_  BYTE                        devType,
  _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_3
  );

  return XInputGetBatteryInformation1_4_Detour (dwUserIndex, devType, pBatteryInformation);
}

DWORD
WINAPI
XInputSetState1_3_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_3
  );

  return XInputSetState1_4_Detour (dwUserIndex, pVibration);
}

DWORD
WINAPI
XInputGetState1_2_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_2
  );

  return XInputGetState1_4_Detour (dwUserIndex, pState);
}

DWORD
WINAPI
XInputGetCapabilities1_2_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_2
  );

  return XInputGetCapabilities1_4_Detour (dwUserIndex, dwFlags, pCapabilities);
}

DWORD
WINAPI
XInputGetBatteryInformation1_2_Detour (
  _In_  DWORD                       dwUserIndex,
  _In_  BYTE                        devType,
  _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_2
  );

  return XInputGetBatteryInformation1_4_Detour (dwUserIndex, devType, pBatteryInformation);
}

DWORD
WINAPI
XInputSetState1_2_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_2
  );

  return XInputSetState1_4_Detour (dwUserIndex, pVibration);
}

DWORD
WINAPI
XInputGetState1_1_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_1
  );

  return XInputGetState1_4_Detour (dwUserIndex, pState);
}

DWORD
WINAPI
XInputGetCapabilities1_1_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_1
  );

  return XInputGetCapabilities1_4_Detour (dwUserIndex, dwFlags, pCapabilities);
}

DWORD
WINAPI
XInputGetBatteryInformation1_1_Detour (
  _In_  DWORD                       dwUserIndex,
  _In_  BYTE                        devType,
  _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_1
  );

  return XInputGetBatteryInformation1_4_Detour (dwUserIndex, devType, pBatteryInformation);
}

DWORD
WINAPI
XInputSetState1_1_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_1
  );

  return XInputSetState1_4_Detour (dwUserIndex, pVibration);
}

DWORD
WINAPI
XInputPowerOff1_4_Detour (
  _In_ DWORD dwUserIndex )
{
  SK_LOG_FIRST_CALL

  HMODULE hModCaller =
    SK_GetCallingDLL ( );

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, XUSER_MAX_INDEX)];

  SK_XINPUT_WRITE (sk_input_dev_type::Gamepad)

  bool nop = ( SK_ImGui_WantGamepadCapture () );

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED :
                                                             nop ?
                                                   ERROR_SUCCESS :
    SK_XINPUT_CALL ( xinput_ctx.cs_haptic [dwUserIndex],
                                           dwUserIndex,
            pCtx->XInputPowerOff_Original (dwUserIndex)
    );

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  return dwRet;
}

DWORD
WINAPI
XInputPowerOff1_3_Detour (
  _In_ DWORD dwUserIndex)
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_3
  );

  return
    XInputPowerOff1_4_Detour (dwUserIndex);
}


DWORD
WINAPI
SK_XInput_PowerOff (_In_  DWORD                dwUserIndex)
{
  if (xinput_ctx.XInput1_4.XInputPowerOff_Original != nullptr)
  {
    return
      xinput_ctx.XInput1_4.XInputPowerOff_Original (dwUserIndex);
  }

  if (xinput_ctx.XInput1_3.XInputPowerOff_Original != nullptr)
  {
    return
      xinput_ctx.XInput1_3.XInputPowerOff_Original (dwUserIndex);
  }

  return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
}


DWORD
WINAPI
XInputGetKeystroke1_4_Detour (
  DWORD             dwUserIndex,
  DWORD             dwReserved,
  PXINPUT_KEYSTROKE pKeystroke )
{
  HMODULE hModCaller = SK_GetCallingDLL ();

  if (config.input.gamepad.xinput.auto_slot_assign && dwUserIndex == 0)
    dwUserIndex = config.input.gamepad.xinput.ui_slot;

  dwUserIndex =
    config.input.gamepad.xinput.assignment [std::min (dwUserIndex, XUSER_MAX_INDEX)];

  SK_LOG_FIRST_CALL
  SK_XINPUT_READ (dwUserIndex)

  if (pKeystroke == nullptr)
    return ERROR_SUCCESS; // What is the proper response?

  if (! xinput_enabled)
    return ERROR_EMPTY;

  if (dwUserIndex >= XUSER_MAX_COUNT) return ERROR_DEVICE_NOT_CONNECTED;

  SK_XInputContext::instance_s* pCtx =
    &xinput_ctx.XInput1_4;

  DWORD dwRet =
    SK_XInput_Holding (dwUserIndex) ? ERROR_DEVICE_NOT_CONNECTED
                                    :
      SK_XINPUT_CALL ( xinput_ctx.cs_poll [dwUserIndex],
                                           dwUserIndex,
        pCtx->XInputGetKeystroke_Original (dwUserIndex, dwReserved, pKeystroke)
      );

  if (SK_ImGui_FilterXInputKeystroke      (dwUserIndex, pKeystroke))
    return ERROR_EMPTY;

  InterlockedExchange (&xinput_ctx.LastSlotState [dwUserIndex], dwRet);

  // Migrate the function that we use internally over to
  //   whatever the game is actively using -- helps with X360Ce
  SK_XInput_EstablishPrimaryHook (hModCaller, pCtx);

  if (dwUserIndex == config.input.gamepad.xinput.ui_slot && ReadAcquire (&SK_ScePad_Backend->reads [2]) > 0)
    return ERROR_DEVICE_NOT_CONNECTED;

  return dwRet;
}

DWORD
WINAPI
XInputGetKeystroke1_3_Detour (
  DWORD             dwUserIndex,
  DWORD             dwReserved,
  PXINPUT_KEYSTROKE pKeystroke )
{
  SK_LOG_FIRST_CALL

    SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput1_3
  );

  return XInputGetKeystroke1_4_Detour (dwUserIndex, dwReserved, pKeystroke);
}


DWORD
WINAPI
SK_XInput_GetCapabilities (_In_  DWORD                dwUserIndex,
                           _In_  DWORD                dwFlags,
                           _Out_ XINPUT_CAPABILITIES *pCapabilities)
{
  static XInputGetCapabilities_pfn
         XInputGetCapabilities_SK =
        (XInputGetCapabilities_pfn)SK_GetProcAddress (xinput_ctx.XInput_SK.hMod,
        "XInputGetCapabilities"                      );

  if (XInputGetCapabilities_SK != nullptr)
    return
      XInputGetCapabilities_SK (dwUserIndex, dwFlags, pCapabilities);

  return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
}

DWORD
WINAPI
SK_XInput_GetBatteryInformation (_In_  DWORD                       dwUserIndex,
                                 _In_  BYTE                        devType,
                                 _Out_ XINPUT_BATTERY_INFORMATION *pBatteryInformation)
{
#if 1
  static XInputGetBatteryInformation_pfn
         XInputGetBatteryInformation_SK =
        (XInputGetBatteryInformation_pfn)SK_GetProcAddress (xinput_ctx.XInput_SK.hMod,
        "XInputGetBatteryInformation"                      );

  if (XInputGetBatteryInformation_SK != nullptr)
    return
      XInputGetBatteryInformation_SK (dwUserIndex, devType, pBatteryInformation);

  return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
#else
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

  return (DWORD)ERROR_DEVICE_NOT_CONNECTED;
#endif
}


DWORD
WINAPI
XInputGetState9_1_0_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput9_1_0
  );

  return XInputGetState1_4_Detour (dwUserIndex, pState);
}

DWORD
WINAPI
XInputGetCapabilities9_1_0_Detour (
  _In_  DWORD                dwUserIndex,
  _In_  DWORD                dwFlags,
  _Out_ XINPUT_CAPABILITIES *pCapabilities )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput9_1_0
  );

  return XInputGetCapabilities1_4_Detour (dwUserIndex, dwFlags, pCapabilities);
}

DWORD
WINAPI
XInputSetState9_1_0_Detour (
  _In_    DWORD             dwUserIndex,
  _Inout_ XINPUT_VIBRATION *pVibration )
{
  SK_LOG_FIRST_CALL

  SK_XInput_EstablishPrimaryHook (
    SK_GetCallingDLL (),
      &xinput_ctx.XInput9_1_0
  );

  return XInputSetState1_4_Detour (dwUserIndex, pVibration);
}


void
SK_Input_HookXInputContext (SK_XInputContext::instance_s* pCtx)
{
  // This causes periodic hitches, so hook and disable it.
//SK_RunOnce (
//  SK_CreateDLLHook2 (L"SetupAPI.dll", "SetupDiGetClassDevsW",
//                                       SetupDiGetClassDevsW_Detour,
//              static_cast_p2p <void> (&SetupDiGetClassDevsW_Original))
//);

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

  pCtx->XInputGetKeystroke_Target =
    SK_GetProcAddress ( pCtx->wszModuleName,
                              "XInputGetKeystroke" );

  // Down-level (XInput 9_1_0) does not have XInputGetKeystroke
  //
  if (pCtx->XInputGetKeystroke_Target != nullptr)
  {
    SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                     "XInputGetKeystroke",
                                pCtx->XInputGetKeystroke_Detour,
       static_cast_p2p <void> (&pCtx->XInputGetKeystroke_Original) );
  }

  pCtx->XInputPowerOff_Target =
      SK_GetProcAddress ( pCtx->wszModuleName,
                                XINPUT_POWEROFF_ORDINAL );

  // Down-level (XInput 9_1_0) does not have XInputPowerOff (103)
  //
  if (pCtx->XInputPowerOff_Target != nullptr)
  {
    SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                     XINPUT_POWEROFF_ORDINAL,
                                pCtx->XInputPowerOff_Detour,
       static_cast_p2p <void> (&pCtx->XInputPowerOff_Original) );
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

  if (pCtx->XInputPowerOff_Target != nullptr)
    memcpy (pCtx->orig_inst_poweroff, pCtx->XInputPowerOff_Target,          6);

  if (pCtx->XInputGetKeystroke_Target != nullptr)
    memcpy (pCtx->orig_keystroke_inst, pCtx->XInputGetKeystroke_Target,     6);
}

static volatile LONG __hooked_xi_1_4   = FALSE;
static volatile LONG __hooked_xi_1_3   = FALSE;
static volatile LONG __hooked_xi_1_2   = FALSE;
static volatile LONG __hooked_xi_1_1   = FALSE;
static volatile LONG __hooked_xi_9_1_0 = FALSE;
static volatile LONG __hooked_xi_uap   = FALSE;

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

    pCtx->wszModuleName                        = L"XInput1_4.dll";
    pCtx->hMod                                 = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (pCtx->hMod))
    {
      pCtx->XInputEnable_Detour                = XInputEnable1_4_Detour;
      pCtx->XInputGetState_Detour              = XInputGetState1_4_Detour;
      pCtx->XInputGetStateEx_Detour            = XInputGetStateEx1_4_Detour;
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities1_4_Detour;
      pCtx->XInputGetBatteryInformation_Detour = XInputGetBatteryInformation1_4_Detour;
      pCtx->XInputPowerOff_Detour              = XInputPowerOff1_4_Detour;
      pCtx->XInputGetKeystroke_Detour          = XInputGetKeystroke1_4_Detour;

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState1_4_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);

      pCtx->XInputSetState_Original =
        config.input.gamepad.xinput.hook_setstate ? xinput_ctx.XInput1_4.XInputSetState_Original :
                   (XInputSetState_pfn)SK_GetProcAddress (pCtx->hMod,   "XInputSetState");

    //if (ReadPointerAcquire       ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
    //  InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, &xinput_ctx.XInput1_4);
    }

    InterlockedIncrementRelease (&__hooked_xi_1_4);
  }

  if (! pTLS->input_core->ctx_init_thread)
    SK_Thread_SpinUntilAtomicMin (&__hooked_xi_1_4, 2);
}

void
SK_Input_HookXInputUap (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  if (ReadAcquire (&__hooked_xi_uap) >= 2)
    return;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return;

  if (! InterlockedCompareExchangeAcquire (&__hooked_xi_uap, TRUE, FALSE))
  {
    if (ReadPointerAcquire ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
      pTLS->input_core->ctx_init_thread = TRUE;

    SK_XInputContext::instance_s* pCtx =
      &xinput_ctx.XInputUap;

    SK_LOG0 ( ( L"  >> Hooking XInput UAP" ),
                L"  Input   " );

    pCtx->wszModuleName                        = L"XInputUap.dll";
    pCtx->hMod                                 = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (pCtx->hMod))
    {
      pCtx->XInputEnable_Detour                = XInputEnableUap_Detour;
      pCtx->XInputGetState_Detour              = XInputGetStateUap_Detour;
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilitiesUap_Detour;
      pCtx->XInputGetBatteryInformation_Detour = XInputGetBatteryInformationUap_Detour;

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetStateUap_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);

      pCtx->XInputSetState_Original =
        config.input.gamepad.xinput.hook_setstate ? xinput_ctx.XInputUap.XInputSetState_Original :
                   (XInputSetState_pfn)SK_GetProcAddress (pCtx->hMod,   "XInputSetState");

      if (ReadPointerAcquire       ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
        InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, &xinput_ctx.XInputUap);

      // Upgrade
      //   Passthrough to 1.4
      SK_Input_HookXInput1_4 ();
    }

    InterlockedIncrementRelease (&__hooked_xi_uap);
  }

  if (! pTLS->input_core->ctx_init_thread)
    SK_Thread_SpinUntilAtomicMin (&__hooked_xi_uap, 2);
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

    pCtx->wszModuleName                        = L"XInput1_3.dll";
    pCtx->hMod                                 = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (pCtx->hMod))
    {
      pCtx->XInputEnable_Detour                = XInputEnable1_3_Detour;
      pCtx->XInputGetState_Detour              = XInputGetState1_3_Detour;
      pCtx->XInputGetStateEx_Detour            = XInputGetStateEx1_3_Detour;
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities1_3_Detour;
      pCtx->XInputGetBatteryInformation_Detour = XInputGetBatteryInformation1_3_Detour;
      pCtx->XInputPowerOff_Detour              = XInputPowerOff1_3_Detour;
      pCtx->XInputGetKeystroke_Detour          = XInputGetKeystroke1_3_Detour;

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState1_3_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);

      pCtx->XInputSetState_Original =
        config.input.gamepad.xinput.hook_setstate ? xinput_ctx.XInput1_3.XInputSetState_Original :
                   (XInputSetState_pfn)SK_GetProcAddress (pCtx->hMod,   "XInputSetState");

      if (ReadPointerAcquire       ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
        InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, &xinput_ctx.XInput1_3);

      // Upgrade
      //   Passthrough to 1.4
      SK_Input_HookXInput1_4 ();
    }

    InterlockedIncrementRelease (&__hooked_xi_1_3);
  }

  if (! pTLS->input_core->ctx_init_thread)
    SK_Thread_SpinUntilAtomicMin (&__hooked_xi_1_3, 2);
}

void
SK_Input_HookXInput1_2 (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  if (ReadAcquire (&__hooked_xi_1_2) >= 2)
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return;

  if (! InterlockedCompareExchangeAcquire (&__hooked_xi_1_2, TRUE, FALSE))
  {
    if (ReadPointerAcquire ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
      pTLS->input_core->ctx_init_thread = TRUE;

    SK_XInputContext::instance_s* pCtx =
      &xinput_ctx.XInput1_2;

    SK_LOG0 ( ( L"  >> Hooking XInput 1.2" ),
                L"  Input   " );

    pCtx->wszModuleName                        = L"XInput1_2.dll";
    pCtx->hMod                                 = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (pCtx->hMod))
    {
      pCtx->XInputGetState_Detour              = XInputGetState1_2_Detour;
      pCtx->XInputGetStateEx_Detour            = nullptr; // Not supported
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities1_2_Detour;
      pCtx->XInputGetBatteryInformation_Detour = nullptr; // Not supported
      pCtx->XInputPowerOff_Detour              = nullptr; // Not supported

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState1_2_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);

      pCtx->XInputSetState_Original =
        config.input.gamepad.xinput.hook_setstate ? xinput_ctx.XInput1_2.XInputSetState_Original :
                   (XInputSetState_pfn)SK_GetProcAddress (pCtx->hMod,   "XInputSetState");

      if (ReadPointerAcquire       ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
        InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, &xinput_ctx.XInput1_2);

      // Upgrade
      //   Passthrough to 1.4
      SK_Input_HookXInput1_4 ();
    }

    InterlockedIncrementRelease (&__hooked_xi_1_2);
  }

  if (! pTLS->input_core->ctx_init_thread)
    SK_Thread_SpinUntilAtomicMin (&__hooked_xi_1_2, 2);
}

void
SK_Input_HookXInput1_1 (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  if (ReadAcquire (&__hooked_xi_1_1) >= 2)
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return;

  if (! InterlockedCompareExchangeAcquire (&__hooked_xi_1_1, TRUE, FALSE))
  {
    if (ReadPointerAcquire ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
      pTLS->input_core->ctx_init_thread = TRUE;

    SK_XInputContext::instance_s* pCtx =
      &xinput_ctx.XInput1_1;

    SK_LOG0 ( ( L"  >> Hooking XInput 1.1" ),
                L"  Input   " );

    pCtx->wszModuleName                        = L"XInput1_1.dll";
    pCtx->hMod                                 = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (pCtx->hMod))
    {
      pCtx->XInputGetState_Detour              = XInputGetState1_1_Detour;
      pCtx->XInputGetStateEx_Detour            = nullptr; // Not supported
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities1_1_Detour;
      pCtx->XInputGetBatteryInformation_Detour = nullptr; // Not supported
      pCtx->XInputPowerOff_Detour              = nullptr; // Not supported

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState1_1_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);

      pCtx->XInputSetState_Original =
        config.input.gamepad.xinput.hook_setstate ? xinput_ctx.XInput1_1.XInputSetState_Original :
                   (XInputSetState_pfn)SK_GetProcAddress (pCtx->hMod,   "XInputSetState");

      if (ReadPointerAcquire       ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
        InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, &xinput_ctx.XInput1_1);

      // Upgrade
      //   Passthrough to 1.4
      SK_Input_HookXInput1_4 ();
    }

    InterlockedIncrementRelease (&__hooked_xi_1_1);
  }

  if (! pTLS->input_core->ctx_init_thread)
    SK_Thread_SpinUntilAtomicMin (&__hooked_xi_1_1, 2);
}

void
SK_Input_HookXInput9_1_0 (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

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

    pCtx->wszModuleName                        = L"XInput9_1_0.dll";
    pCtx->hMod                                 = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (pCtx->hMod))
    {
      pCtx->XInputGetState_Detour              = XInputGetState9_1_0_Detour;
      pCtx->XInputGetStateEx_Detour            = nullptr; // Not supported
      pCtx->XInputGetCapabilities_Detour       = XInputGetCapabilities9_1_0_Detour;
      pCtx->XInputGetBatteryInformation_Detour = nullptr; // Not supported
      pCtx->XInputPowerOff_Detour              = nullptr; // Not supported

      pCtx->XInputSetState_Detour              =
        config.input.gamepad.xinput.hook_setstate ? XInputSetState9_1_0_Detour : nullptr;

      SK_Input_HookXInputContext (pCtx);

      pCtx->XInputSetState_Original =
        config.input.gamepad.xinput.hook_setstate ? xinput_ctx.XInput9_1_0.XInputSetState_Original :
                   (XInputSetState_pfn)SK_GetProcAddress (pCtx->hMod,     "XInputSetState");

      if (ReadPointerAcquire       ((LPVOID *)&xinput_ctx.primary_hook) == nullptr)
        InterlockedExchangePointer ((LPVOID *)&xinput_ctx.primary_hook, &xinput_ctx.XInput9_1_0);

      // Upgrade
      //   Passthrough to 1.4
      SK_Input_HookXInput1_4 ();
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
        SK_LOG0 ( ( L" Re-hooked XInput (%8s) using '%s'...", L"Get",
                       pCtx->wszModuleName ),
                    L"Input Mgr." );

        if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
          xinput_ctx.XInput1_4 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
          xinput_ctx.XInput1_3 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput1_2.hMod)
          xinput_ctx.XInput1_2 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput1_1.hMod)
          xinput_ctx.XInput1_1 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput9_1_0.hMod)
          xinput_ctx.XInput9_1_0 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInputUap.hMod)
          xinput_ctx.XInputUap = *pCtx;
      }

      else
      {
        SK_LOG0 ( ( L" Failed to re-hook XInput (%8s) using '%s'...", L"Get",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }

    else
    {
      SK_LOG0 ( ( L" Failed to remove XInput (%8s) hook from '%s'...", L"Get",
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
          SK_LOG0 ( ( L" Re-hooked XInput (%8s) using '%s'...", L"Set",
                                    pCtx->wszModuleName ),
              L"Input Mgr." );

          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_2.hMod)
            xinput_ctx.XInput1_2 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_1.hMod)
            xinput_ctx.XInput1_1 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput9_1_0.hMod)
            xinput_ctx.XInput9_1_0 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInputUap.hMod)
            xinput_ctx.XInputUap = *pCtx;
        }

        else
        {
          SK_LOG0 ( ( L" Failed to re-hook XInput (%8s) using '%s'...", L"Set",
                 pCtx->wszModuleName ),
              L"Input Mgr." );
        }
      }

      else
      {
        SK_LOG0 ( ( L" Failed to remove XInput (%8s) hook from '%s'...", L"Set",
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
        SK_LOG0 ( ( L" Re-hooked XInput (%8s) using '%s'...", L"Caps",
                       pCtx->wszModuleName ),
                    L"Input Mgr." );

        if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
          xinput_ctx.XInput1_4 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
          xinput_ctx.XInput1_3 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput1_2.hMod)
          xinput_ctx.XInput1_2 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput1_1.hMod)
          xinput_ctx.XInput1_1 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInput9_1_0.hMod)
          xinput_ctx.XInput9_1_0 = *pCtx;
        if (pCtx->hMod == xinput_ctx.XInputUap.hMod)
          xinput_ctx.XInputUap = *pCtx;
      }

      else
      {
        SK_LOG0 ( ( L" Failed to re-hook XInput (%8s) using '%s'...", L"Caps",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }

    else
    {
      SK_LOG0 ( ( L" Failed to remove XInput (%8s) hook from '%s'...", L"Caps",
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
          SK_LOG0 ( ( L" Re-hooked XInput (%8s) using '%s'...", L"Battery",
                         pCtx->wszModuleName ),
                      L"Input Mgr." );

          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInputUap.hMod)
            xinput_ctx.XInputUap = *pCtx;
        }

        else
        {
          SK_LOG0 ( ( L" Failed to re-hook XInput (%8s) using '%s'...", L"Battery",
                 pCtx->wszModuleName ),
              L"Input Mgr." );
        }
      }

      else
      {
        SK_LOG0 ( ( L" Failed to remove XInput (%8s) hook from '%s'...", L"Battery",
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
          SK_LOG0 ( ( L" Re-hooked XInput (%8s) using '%s'...", L"GetEx",
                         pCtx->wszModuleName ),
                      L"Input Mgr." );

          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
        }

        else
        {
          SK_LOG0 ( ( L" Failed to re-hook XInput (%8s) using '%s'...", L"GetEx",
                 pCtx->wszModuleName ),
              L"Input Mgr." );
        }
      }

      else
      {
        SK_LOG0 ( ( L" Failed to remove XInput (%8s) hook from '%s'...", L"GetEx",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }
  }

  if (pCtx->XInputPowerOff_Target != nullptr)
  {
    ret =
      MH_QueueEnableHook (pCtx->XInputPowerOff_Target);

    // Test for modified hooks
    if ( ( ret != MH_OK && ret != MH_ERROR_ENABLED ) ||
                   ( pCtx->XInputPowerOff_Target != nullptr &&
             memcmp (pCtx->orig_inst_poweroff,
                     pCtx->XInputPowerOff_Target, 6 ) )
       )
    {
      if ( MH_OK == MH_RemoveHook (pCtx->XInputPowerOff_Target) )
      {
        if ( MH_OK ==
               SK_CreateDLLHook2 (  pCtx->wszModuleName,
                                    XINPUT_POWEROFF_ORDINAL,
                                    pCtx->XInputPowerOff_Detour,
           static_cast_p2p <void> (&pCtx->XInputPowerOff_Original) )
           )
        {
          SK_LOG0 ( ( L" Re-hooked XInput (%8s) using '%s'...", L"PowerOff",
                         pCtx->wszModuleName ),
                      L"Input Mgr." );

          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
        }

        else
        {
          SK_LOG0 ( ( L" Failed to re-hook XInput (%8s) using '%s'...", L"PowerOff",
                 pCtx->wszModuleName ),
              L"Input Mgr." );
        }
      }

      else
      {
        SK_LOG0 ( ( L" Failed to remove XInput (%8s) hook from '%s'...", L"PowerOff",
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
          SK_LOG0 ( ( L" Re-hooked XInput (%8s) using '%s'...", L"Enable",
                         pCtx->wszModuleName ),
                      L"Input Mgr." );

          if (pCtx->hMod == xinput_ctx.XInput1_4.hMod)
            xinput_ctx.XInput1_4 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInput1_3.hMod)
            xinput_ctx.XInput1_3 = *pCtx;
          if (pCtx->hMod == xinput_ctx.XInputUap.hMod)
            xinput_ctx.XInputUap = *pCtx;
        }

        else
        {
          SK_LOG0 ( ( L" Failed to re-hook XInput (%8s) using '%s'...", L"Enable",
                 pCtx->wszModuleName ),
              L"Input Mgr." );
        }
      }

      else
      {
        SK_LOG0 ( ( L" Failed to remove XInput (%8s) hook from '%s'...", L"Enable",
               pCtx->wszModuleName ),
            L"Input Mgr." );
      }
    }
  }



  SK_ApplyQueuedHooksIfInit ();


  if (pCtx->XInputGetState_Target != nullptr)
    memcpy (pCtx->orig_inst, pCtx->XInputGetState_Target,                   6);

  if (pCtx->XInputGetKeystroke_Target != nullptr)
    memcpy (pCtx->orig_keystroke_inst, pCtx->XInputGetKeystroke_Target,     6);

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

  if (pCtx->XInputPowerOff_Target != nullptr)
    memcpy (pCtx->orig_inst_poweroff, pCtx->XInputPowerOff_Target,          6);
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

  XINPUT_VIBRATION
    vibes {
      .wLeftMotorSpeed  =
        sk::narrow_cast <WORD>(std::min (0.99999f, fStrengthLeft)  * 65535.0f),
      .wRightMotorSpeed =
        sk::narrow_cast <WORD>(std::min (0.99999f, fStrengthRight) * 65535.0f)
    };

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

    static XInputSetState_pfn
           XInputSetState_SK =
          (XInputSetState_pfn)SK_GetProcAddress (xinput_ctx.XInput_SK.hMod,
          "XInputSetState"                      );

    XInputSetState_pfn XInputSetState =
                       XInputSetState_SK;

    DWORD dwRet = XInputSetState == nullptr ? ERROR_DEVICE_NOT_CONNECTED :
      SK_XINPUT_CALL ( xinput_ctx.cs_haptic [iJoyID],
                                             iJoyID,
                             XInputSetState (iJoyID, &vibes) );

    if ( ERROR_SUCCESS ==
           dwRet )
    {
      return true;
    }
  }

  return false;
}


static volatile DWORD last_poll [XUSER_MAX_COUNT] = { 0, 0, 0, 0 };

volatile ULONG SK_XInput_RefreshTime = 7500UL;

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
  auto idx =
    std::clamp (iJoyID, 0, (INT)XUSER_MAX_INDEX);

  static const DWORD& dwTime =
    SK::ControlPanel::current_time;

  // Stagger rechecks (250 ms * ID) so they do not all happen in a single frame
  const bool bRecheck = ReadULongAcquire (&last_poll [idx]   ) <
      (DWORD)std::max (
        0L, ( (INT)dwTime - (INT)ReadULongAcquire (&SK_XInput_RefreshTime) -
                     250L * idx ) );

  return bRecheck;
}

static std::wstring  SK_XInput_LinkedVersion = L"";
static bool          SK_XInput_FirstFrame    = true;
static volatile LONG SK_XInput_TriedToHook   = FALSE;

static DWORD SK_XInput_UI_LastSeenController = DWORD_MAX;
static DWORD SK_XInput_UI_LastSeenTime       =         0;

void
SK_XInput_UpdateSlotForUI ( BOOL  success,
                            DWORD dwUserIndex,
                            DWORD dwPacketCount ) noexcept
{
  constexpr DWORD MIGRATION_PERIOD = 750;

  if (success)
  {
    DWORD dwTime =
     SK::ControlPanel::current_time;

    bool migrate = ( dwPacketCount > 1 )
         &&
      ( SK_XInput_UI_LastSeenController == DWORD_MAX             ||
        SK_XInput_UI_LastSeenTime       < dwTime - MIGRATION_PERIOD );

    if (SK_XInput_UI_LastSeenController == config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)])
        SK_XInput_UI_LastSeenTime        = dwTime;

    if (migrate)
    {   SK_XInput_UI_LastSeenController = config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];
        SK_XInput_UI_LastSeenTime       = dwTime;

        if (config.input.gamepad.xinput.ui_slot < XUSER_MAX_COUNT) // XUSER_MAX_COUNT = Disable
            config.input.gamepad.xinput.ui_slot = config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)];

      if (! config.input.gamepad.disabled_to_game)
      {
        SK_XInput_Backend->viewed.gamepad = SK_QueryPerf ().QuadPart;
      }
    }
  }

  else if (config.input.gamepad.xinput.assignment [std::min (dwUserIndex, 3UL)] == config.input.gamepad.xinput.ui_slot)
  {
    SK_XInput_UI_LastSeenController     = DWORD_MAX;
    config.input.gamepad.xinput.ui_slot = 0;
  }
}


bool _lastXInputPollState [XUSER_MAX_COUNT] = { false, false, false, false };

// Return the last polling status; when actual gamepad state is not needed
bool
WINAPI
SK_XInput_WasLastPollSuccessful ( INT iJoyID )
{
  return
    iJoyID >= 0 && iJoyID <= XUSER_MAX_COUNT && _lastXInputPollState [iJoyID];
}

bool
WINAPI
SK_XInput_PollController ( INT           iJoyID,
                           XINPUT_STATE* pState )
{
  if (! config.input.gamepad.hook_xinput)
    return false;

  bool queued_hooks = false;

  iJoyID =
    config.input.gamepad.xinput.assignment [
      std::max (0, std::min (iJoyID, (INT)XUSER_MAX_INDEX))
    ];



  // Lazy-load DLLs if somehow a game uses an XInput DLL not listed
  //   in its import table and also not caught by our LoadLibrary hook
  if (std::exchange (SK_XInput_FirstFrame, false))
  {
    if ( SK_Modules->isValid    (
         SK_GetModuleHandleW    (L"XInputUap.dll")
                                )
       ) SK_Input_HookXInputUap ();

    if ( SK_Modules->isValid    (
         SK_GetModuleHandleW    (L"XInput9_1_0.dll")
                                )
       ) SK_Input_HookXInput9_1_0 ();

    if ( SK_Modules->isValid    (
         SK_GetModuleHandleW    (L"XInput1_1.dll")
                                )
       ) SK_Input_HookXInput1_1 ();

    if ( SK_Modules->isValid    (
         SK_GetModuleHandleW    (L"XInput1_2.dll")
                                )
       ) SK_Input_HookXInput1_2 ();

    if ( SK_Modules->isValid    (
         SK_GetModuleHandleW    (L"XInput1_3.dll")
                                )
       ) SK_Input_HookXInput1_3 ();

    if ( SK_Modules->isValid    (
         SK_GetModuleHandleW    (L"XInput1_4.dll")
                                )
       ) SK_Input_HookXInput1_4 ();

    queued_hooks = true;
  }

  if (queued_hooks)
    SK_ApplyQueuedHooksIfInit ();
  else
    SK_XInput_RehookIfNeeded ();



  if (iJoyID == -1) // Uh, how?
    return true;

  if (iJoyID < 0 || iJoyID >= XUSER_MAX_COUNT)
    return false;



  XINPUT_STATE_EX
    xstate                 = { };
    xstate.dwPacketNumber  =  1;

  DWORD dwRet = ERROR_DEVICE_NOT_CONNECTED;

  // This function is actually a performance hazzard when no controllers
  //   are plugged in, so ... throttle the sucker.
  if (_ShouldRecheckStatus (iJoyID))
  {
    // Steam does not disable XInput 1.1, awesome!
    static XInputGetState_pfn
           XInputGetState_SK =
          (XInputGetState_pfn)SK_GetProcAddress (xinput_ctx.XInput_SK.hMod,
          "XInputGetState"                      );

    XInputGetState_pfn XInputGetState =
                       XInputGetState_SK;

    if (XInputGetState != nullptr)
    {
      dwRet =
        SK_XINPUT_CALL  ( xinput_ctx.cs_poll [iJoyID],
                                              iJoyID,
                              XInputGetState (iJoyID, (XINPUT_STATE *)&xstate) );

      // Error? Mmark the time we saw this error and we'll try again on another frame
      if (dwRet == ERROR_DEVICE_NOT_CONNECTED)
        WriteULongRelease (&last_poll [iJoyID], SK_timeGetTime ());
    }
  }

  InterlockedExchange ( &xinput_ctx.LastSlotState [iJoyID],
                         dwRet );

  // Broken controllers should not be controlling UIs.
  if (dwRet != ERROR_SUCCESS)
  {
    SK_XInput_UpdateSlotForUI (FALSE, iJoyID, 0);

    return (_lastXInputPollState [iJoyID] = false);
  }

  WriteULongRelease (&last_poll [iJoyID], 0); // Feel free to poll this controller again immediately,
                                              //   the performance penalty from a disconnected controller
                                              //     won't be there.

  void SK_XInput_StopHolding (DWORD dwUserIndex);
       SK_XInput_StopHolding (iJoyID);

  static DWORD       dwLastPacketUI [4] = { 0, 0, 0, 0 };
  if (std::exchange (dwLastPacketUI [iJoyID], xstate.dwPacketNumber) != xstate.dwPacketNumber)
    SK_XInput_UpdateSlotForUI (TRUE, iJoyID,  xstate.dwPacketNumber);

  if (pState != nullptr)
    memcpy (pState, &xstate, sizeof (XINPUT_STATE));

  return (_lastXInputPollState [iJoyID] = true);
}



static sk_import_test_s
  _XInput_ImportsToTry [] = { { "XInput1_4.dll",   false },
                              { "XInput1_3.dll",   false },
                              { "XInput1_2.dll",   false },
                              { "XInput1_1.dll",   false },
                              { "XInput9_1_0.dll", false },
                              { "XInputUap.dll",   false } };

void
SK_Input_PreHookXInput (void)
{
  if (! config.input.gamepad.hook_xinput)
    return;

  xinput_ctx.XInput_SK.wszModuleName =
#ifdef _WIN64
    L"XInput_SK64.dll";
#else
    L"XInput_SK32.dll";
#endif

  static std::filesystem::path path_to_driver =
        (std::filesystem::path (SK_GetInstallPath ()) /
                                LR"(Drivers\XInput)") /
                                 xinput_ctx.XInput_SK.wszModuleName;

  std::filesystem::path
    path_to_highest_xinput_ver = L"";

  std::error_code ec =
    std::error_code ();

  static const auto *pSystemDirectory =
    SK_GetSystemDirectory ();
  
  for ( auto&& version :
          { ( std::filesystem::path (pSystemDirectory) / L"XInput1_4.dll"   ),
            ( std::filesystem::path (pSystemDirectory) / L"XInput1_3.dll"   ),
            ( std::filesystem::path (pSystemDirectory) / L"XInput1_2.dll"   ),
            ( std::filesystem::path (pSystemDirectory) / L"XInput1_1.dll"   ),
            ( std::filesystem::path (pSystemDirectory) / L"XInput9_1_0.dll" )
          } )
  {
    if (std::filesystem::exists (version, ec))
    {
      path_to_highest_xinput_ver = version;
      break;
    }
  }

  if ( (! std::filesystem::exists ( path_to_driver,              ec))||
       (! SK_Assert_SameDLLVersion (path_to_driver.            c_str (),
                                    path_to_highest_xinput_ver.c_str ()) ) )
  { SK_CreateDirectories           (path_to_driver.c_str ());

    if (    std::filesystem::exists (path_to_highest_xinput_ver, ec))
    { if (  std::filesystem::exists (path_to_driver.c_str (),    ec))
      if (! std::filesystem::remove (path_to_driver.c_str (),    ec))
      {
        SK_File_MoveNoFail (         path_to_driver.c_str (),
          ( std::filesystem::current_path () /
                            L"XInput_Old.tmp" ).    c_str () );
      }

      // Done (re)moving out-of-date XInput DLL
    
      if (! std::filesystem::copy_file (path_to_highest_xinput_ver,
                                        path_to_driver, ec))
      {
        SK_ReleaseAssert (! L"XInput Is Borked, Yay!");
      }

      // Cleanup any temporary XInput DLLs
      SK_DeleteTemporaryFiles (
        std::filesystem::current_path ().c_str ()
      );
    }
  }

  xinput_ctx.XInput_SK.hMod =
    SK_LoadLibraryW (path_to_driver.c_str ());

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

    SK_TestImports (skModuleRegistry::HostApp (), tests, 6);

    if ( tests [0].used || tests [1].used || tests [2].used ||
         tests [3].used || tests [4].used || tests [5].used )
    {
#if 0
      SK_LOG0 ( ( L"Game uses XInput, deferring input hooks..." ),
                  L"  Input   " );
      if (tests [5].used) { /*SK_Input_HookXInputUap   ();*/ SK_XInput_LinkedVersion = L"XInputUap.dll";   }
      if (tests [4].used) { /*SK_Input_HookXInput9_1_0 ();*/ SK_XInput_LinkedVersion = L"XInput9_1_0.dll"; }
      if (tests [3].used) { /*SK_Input_HookXInput1_1   ();*/ SK_XInput_LinkedVersion = L"XInput1_1.dll";   }
      if (tests [2].used) { /*SK_Input_HookXInput1_2   ();*/ SK_XInput_LinkedVersion = L"XInput1_2.dll";   }
      if (tests [1].used) { /*SK_Input_HookXInput1_3   ();*/ SK_XInput_LinkedVersion = L"XInput1_3.dll";   }
      if (tests [0].used) { /*SK_Input_HookXInput1_4   ();*/ SK_XInput_LinkedVersion = L"XInput1_4.dll";   }


#else
      SK_LOG0 ( ( L"Game uses XInput, installing input hooks..." ),
                  L"  Input   " );

      if (tests [5].used) { SK_Input_HookXInputUap   (); }
      if (tests [4].used) { SK_Input_HookXInput9_1_0 (); }
      if (tests [3].used) { SK_Input_HookXInput1_1   (); }
      if (tests [2].used) { SK_Input_HookXInput1_2   (); }
      if (tests [1].used) { SK_Input_HookXInput1_3   (); }
      if (tests [0].used) { SK_Input_HookXInput1_4   (); }
      
      

#endif
    }

    if (SK_GetModuleHandleW (L"XInputUap.dll"))
      SK_Input_HookXInputUap ();

    if (SK_GetModuleHandleW (L"XInput9_1_0.dll"))
      SK_Input_HookXInput9_1_0 ();

    if (SK_GetModuleHandleW (L"XInput1_1.dll"))
      SK_Input_HookXInput1_1 ();

    if (SK_GetModuleHandleW (L"XInput1_2.dll"))
      SK_Input_HookXInput1_2 ();

    if (SK_GetModuleHandleW (L"XInput1_3.dll"))
      SK_Input_HookXInput1_3 ();

    if (SK_GetModuleHandleW (L"XInput1_4.dll"))
      SK_Input_HookXInput1_4 ();
  }

  SK_RunOnce (SK_XInput_NotifyDeviceArrival ());
}



void
WINAPI
SK_XInput_ZeroHaptics (INT iJoyID)
{
#ifdef SK_STEAM_CONTROLLER_SUPPORT
  auto steam_idx =
    sk::narrow_cast <ControllerIndex_t> (iJoyID);

  if (steam_input.count && ControllerPresent (steam_idx))
  {
    steam_input.pipe->TriggerVibration (
      steam_input [steam_idx].handle, 0U, 0U
    );
  }
#endif

  iJoyID =
    config.input.gamepad.xinput.assignment [
      std::max (0, std::min (iJoyID, (INT)XUSER_MAX_INDEX))
    ];

//if ( ERROR_SUCCESS !=
//       ReadULongAcquire (&xinput_ctx.LastSlotState [iJoyID]) )
//{
//  return;
//}

  const auto* pCtx = (
    static_cast <SK_XInputContext::instance_s *>
      (ReadPointerAcquire ((volatile LPVOID *)&xinput_ctx.primary_hook))
  );

  if (pCtx == nullptr)
    return;

  if (iJoyID >= XUSER_MAX_COUNT)
    return;

  const bool
    orig_enable =
      SK_XInput_Enable (FALSE);

  SK_XInput_PulseController (iJoyID, 0.0f, 0.0f);
  SK_XInput_Enable          (orig_enable);
}