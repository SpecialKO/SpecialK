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
#include <SpecialK/input/sce_pad.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Input Mgr."

#define SK_LOG_INPUT_CALL { static int  calls  = 0;             \
  { SK_LOG0 ( (L"[!] > Call #%lu: %hs", calls++, __FUNCTION__), \
               L"Input Mgr." ); } }

///////////////////////////////////////////////////////
//
// libScePad.dll  (SK_ScePad)
//
///////////////////////////////////////////////////////

#define SK_SCEPAD_READ(type)  SK_ScePad_Backend->markRead   (type);
#define SK_SCEPAD_WRITE(type) SK_ScePad_Backend->markWrite  (type);
#define SK_SCEPAD_VIEW(slot)  SK_ScePad_Backend->markViewed ((sk_input_dev_type)(1 << slot));

struct SK_SceInputContext
{
  enum
  {
    ScePadType_NONE      =  0,
    ScePadType_DualShock =  1,
    ScePadType_DualSense =  2,
  };

  struct instance_s
  {
    const wchar_t*                   wszModuleName                         =     L"";
    HMODULE                          hMod                                  = nullptr;

    scePadInit_pfn                   scePadInit_Detour                     = nullptr;
    scePadInit_pfn                   scePadInit_Original                   = nullptr;
    LPVOID                           scePadInit_Target                     = nullptr;

    scePadGetHandle_pfn              scePadGetHandle_Detour                = nullptr;
    scePadGetHandle_pfn              scePadGetHandle_Original              = nullptr;
    LPVOID                           scePadGetHandle_Target                = nullptr;

    scePadOpen_pfn                   scePadOpen_Detour                     = nullptr;
    scePadOpen_pfn                   scePadOpen_Original                   = nullptr;
    LPVOID                           scePadOpen_Target                     = nullptr;

    scePadClose_pfn                  scePadClose_Detour                    = nullptr;
    scePadClose_pfn                  scePadClose_Original                  = nullptr;
    LPVOID                           scePadClose_Target                    = nullptr;

    scePadSetParticularMode_pfn      scePadSetParticularMode_Detour        = nullptr;
    scePadSetParticularMode_pfn      scePadSetParticularMode_Original      = nullptr;
    LPVOID                           scePadSetParticularMode_Target        = nullptr;

    scePadRead_pfn                   scePadRead_Detour                     = nullptr;
    scePadRead_pfn                   scePadRead_Original                   = nullptr;
    LPVOID                           scePadRead_Target                     = nullptr;

    scePadReadState_pfn              scePadReadState_Detour                = nullptr;
    scePadReadState_pfn              scePadReadState_Original              = nullptr;
    LPVOID                           scePadReadState_Target                = nullptr;

    scePadResetOrientation_pfn       scePadResetOrientation_Detour         = nullptr;
    scePadResetOrientation_pfn       scePadResetOrientation_Original       = nullptr;
    LPVOID                           scePadResetOrientation_Target         = nullptr;

    scePadSetAngularVelocity\
DeadbandState_pfn                    scePadSetAngularVelocity\
DeadbandState_Detour                                                       = nullptr;
    scePadSetAngularVelocity\
DeadbandState_pfn                    scePadSetAngularVelocity\
DeadbandState_Original                                                     = nullptr;
    LPVOID                           scePadSetAngularVelocity\
DeadbandState_Target                                                       = nullptr;

    scePadSetMotionSensorState_pfn   scePadSetMotionSensorState_Detour     = nullptr;
    scePadSetMotionSensorState_pfn   scePadSetMotionSensorState_Original   = nullptr;
    LPVOID                           scePadSetMotionSensorState_Target     = nullptr;

    scePadSetTiltCorrectionState_pfn scePadSetTiltCorrectionState_Detour   = nullptr;
    scePadSetTiltCorrectionState_pfn scePadSetTiltCorrectionState_Original = nullptr;
    LPVOID                           scePadSetTiltCorrectionState_Target   = nullptr;

    scePadSetVibration_pfn           scePadSetVibration_Detour             = nullptr;
    scePadSetVibration_pfn           scePadSetVibration_Original           = nullptr;
    LPVOID                           scePadSetVibration_Target             = nullptr;

    scePadResetLightBar_pfn          scePadResetLightBar_Detour            = nullptr;
    scePadResetLightBar_pfn          scePadResetLightBar_Original          = nullptr;
    LPVOID                           scePadResetLightBar_Target            = nullptr;

    scePadSetLightBar_pfn            scePadSetLightBar_Detour              = nullptr;
    scePadSetLightBar_pfn            scePadSetLightBar_Original            = nullptr;
    LPVOID                           scePadSetLightBar_Target              = nullptr;
  } scePad { };
};

SK_SceInputContext sceinput_ctx;

int
SK_ScePadInit (void)
{
  int ret = sceinput_ctx.scePad.
                         scePadInit_Original ();

  SK_ScePadSetParticularMode (true);

  return ret;
}

int
SK_ScePadGetHandle (SK_SceUserID userID, int type,
                                         int index)
{
  return sceinput_ctx.scePad.
    scePadGetHandle_Original (userID, type, index);
}

int
SK_ScePadOpen (SK_SceUserID userID, int type,
                                    int index, SK_ScePadOpenParam *inputParam)
{
  return sceinput_ctx.scePad.
    scePadOpen_Original (userID, type, index, inputParam);
}

int
SK_ScePadClose (int handle)
{
  return sceinput_ctx.scePad.
    scePadClose_Original (handle);
}

int
SK_ScePadSetParticularMode (bool mode)
{
  SK_LOG_FIRST_CALL;

  std::ignore = mode;

  return
    sceinput_ctx.scePad.scePadSetParticularMode_Original (true);
}

int
SK_ScePadRead (int handle, SK_ScePadData *iData, int count)
{
  SK_LOG_FIRST_CALL;

  for (int i = 0 ; i < count ; ++i)
    SK_SCEPAD_READ (sk_input_dev_type::Gamepad);

  return sceinput_ctx.scePad.
    scePadRead_Original (handle, iData, count);
}

extern bool
WINAPI
SK_ImGui_ToggleEx ( bool& toggle_ui,
                    bool& toggle_nav );

int
SK_ScePadReadState (int handle, SK_ScePadData* iData)
{
  SK_LOG_FIRST_CALL;

  static concurrency::concurrent_unordered_map <int, std::pair <int, SK_ScePadData>> last_result;

  SK_SCEPAD_READ (sk_input_dev_type::Gamepad);

  bool bToggleNav = false,
       bToggleVis = false;

  int result =
    sceinput_ctx.scePad.
                 scePadReadState_Original (handle, iData);


  if (iData != nullptr)
  {
    const  DWORD LONG_PRESS   = 5UL;
    static DWORD dwLastToggle = 0;
    static DWORD dwLastPress  = 0;

    if ( ( last_result [handle].second.buttons &
             static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_PLAYSTATION) ) !=
             static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_PLAYSTATION) &&
         (                      iData->buttons &
             static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_PLAYSTATION) ) ==
             static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_PLAYSTATION)
       )
    {
      if (dwLastToggle < SK::ControlPanel::current_time - LONG_PRESS)
      {
        if (SK_ImGui_Active ())
        {
          bToggleVis |= true;
          bToggleNav |= true;
        }

        else
        {
          bToggleNav |= (! nav_usable);
          bToggleVis |= true;
        }

        dwLastToggle = SK::ControlPanel::current_time;
      }
    }

    if ( (                     iData->buttons & static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_MUTE)) &&
         (last_result [handle].second.buttons & static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_MUTE)) == 0 )
    {
      if (dwLastPress < SK::ControlPanel::current_time - LONG_PRESS)
      {
        SK_SetGameMute (! SK_IsGameMuted ());

        dwLastPress = SK::ControlPanel::current_time;
      }
    }


    if (config.input.gamepad.scepad.disable_touch)
    {
      iData->touchData.touchNum  =  0 ;
      iData->touchData.touch [0] = { };
      iData->touchData.touch [1] = { };

      iData->buttons &= ~static_cast <uint32_t> (
          SK_ScePadDataOffset::PAD_BUTTON_TOUCH_PAD );
    }

    if (config.input.gamepad.scepad.share_clicks_touch)
    {
      if ( (iData->buttons & static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_SHARE)) ==
                             static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_SHARE) )
      {
        iData->buttons |= static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_TOUCH_PAD);
      }
    }
  }


  if (! SK_ImGui_WantGamepadCapture ())
  {
    last_result [handle] =
      std::make_pair (result, iData != nullptr ? *iData : SK_ScePadData ());
  }

  else
  {
    if (iData != nullptr)
    {
      auto& last_good_result =
        last_result [handle].second;

      bool setPS     = iData->buttons & static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_PLAYSTATION);
      bool clearPS   = (! setPS);
      bool setMute   = iData->buttons & static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_MUTE);
      bool clearMute = (! setMute);

      last_good_result.buttons          = 0UL;
      last_good_result.leftStick.x      = std::numeric_limits <uint8_t> ().max () / 2;
      last_good_result.leftStick.y      = std::numeric_limits <uint8_t> ().max () / 2;
      last_good_result.rightStick.x     = std::numeric_limits <uint8_t> ().max () / 2;
      last_good_result.rightStick.y     = std::numeric_limits <uint8_t> ().max () / 2;
      last_good_result.analogButtons.l2 = 0;
      last_good_result.analogButtons.r2 = 0;

      last_good_result.buttons |= setPS   ?
        static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_PLAYSTATION) :  0;
      last_good_result.buttons &= clearPS ?
       ~static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_PLAYSTATION) : ~0;

      last_good_result.buttons |= setMute   ?
        static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_MUTE) :  0;
      last_good_result.buttons &= clearMute ?
       ~static_cast <uint32_t> (SK_ScePadDataOffset::PAD_BUTTON_MUTE) : ~0;

      memcpy (iData, &last_good_result, sizeof (SK_ScePadData));
    }
  }


  if (                 bToggleVis|bToggleNav)
    SK_ImGui_ToggleEx (bToggleVis,bToggleNav);


  return
    last_result [handle].first;
}

int
SK_ScePadResetOrientation (int handle)
{
  return sceinput_ctx.scePad.
    scePadResetOrientation_Original (handle);
}

int
SK_ScePadSetAngularVelocityDeadbandState (int handle, bool enable)
{
  return sceinput_ctx.scePad.
    scePadSetAngularVelocityDeadbandState_Original (handle, enable);
}

int
SK_ScePadSetMotionSensorState (int handle, bool enable)
{
  return sceinput_ctx.scePad.
    scePadSetMotionSensorState_Original (handle, enable);
}

int
SK_ScePadSetTiltCorrectionState (int handle, bool enable)
{
  return sceinput_ctx.scePad.
    scePadSetTiltCorrectionState_Original (handle, enable);
}

int
SK_ScePadSetVibration (int handle, SK_ScePadVibrationParam *param)
{
  return sceinput_ctx.scePad.
    scePadSetVibration_Original (handle, param);
}

int
SK_ScePadResetLightBar (int handle)
{
  return sceinput_ctx.scePad.
    scePadResetLightBar_Original (handle);
}

int
SK_ScePadSetLightBar (int handle, SK_ScePadColor* param)
{
  return sceinput_ctx.scePad.
    scePadSetLightBar_Original (handle, param);
}


static volatile LONG __hooked_scePad = FALSE;

void
SK_Input_HookScePadContext (SK_SceInputContext::instance_s *pCtx)
{
  pCtx->scePadInit_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadInit" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadInit",
                              pCtx->scePadInit_Detour,
     static_cast_p2p <void> (&pCtx->scePadInit_Original) );

  pCtx->scePadGetHandle_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadGetHandle" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadGetHandle",
                              pCtx->scePadGetHandle_Detour,
     static_cast_p2p <void> (&pCtx->scePadGetHandle_Original) );

  pCtx->scePadOpen_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadOpen" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadOpen",
                              pCtx->scePadOpen_Detour,
     static_cast_p2p <void> (&pCtx->scePadOpen_Original) );

  pCtx->scePadClose_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadClose" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadClose",
                              pCtx->scePadClose_Detour,
     static_cast_p2p <void> (&pCtx->scePadClose_Original) );

  pCtx->scePadSetParticularMode_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetParticularMode" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadSetParticularMode",
                              pCtx->scePadSetParticularMode_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetParticularMode_Original) );

  pCtx->scePadRead_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadRead" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadRead",
                              pCtx->scePadRead_Detour,
     static_cast_p2p <void> (&pCtx->scePadRead_Original) );

  pCtx->scePadReadState_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadReadState" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadReadState",
                              pCtx->scePadReadState_Detour,
     static_cast_p2p <void> (&pCtx->scePadReadState_Original) );

  pCtx->scePadResetOrientation_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadResetOrientation" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadResetOrientation",
                              pCtx->scePadResetOrientation_Detour,
     static_cast_p2p <void> (&pCtx->scePadResetOrientation_Original) );

  pCtx->scePadSetAngularVelocityDeadbandState_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetAngularVelocityDeadbandState" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadSetAngularVelocityDeadbandState",
                              pCtx->scePadSetAngularVelocityDeadbandState_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetAngularVelocityDeadbandState_Original) );

  pCtx->scePadSetMotionSensorState_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetMotionSensorState" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadSetMotionSensorState",
                              pCtx->scePadSetMotionSensorState_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetMotionSensorState_Original) );

  pCtx->scePadSetTiltCorrectionState_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetTiltCorrectionState" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadSetTiltCorrectionState",
                              pCtx->scePadSetTiltCorrectionState_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetTiltCorrectionState_Original) );

  pCtx->scePadSetVibration_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetVibration" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadSetVibration",
                              pCtx->scePadSetVibration_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetVibration_Original) );

  pCtx->scePadResetLightBar_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadResetLightBar" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadResetLightBar",
                              pCtx->scePadResetLightBar_Detour,
     static_cast_p2p <void> (&pCtx->scePadResetLightBar_Original) );

  pCtx->scePadSetLightBar_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetLightBar" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName,
                                   "scePadSetLightBar",
                              pCtx->scePadSetLightBar_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetLightBar_Original) );
}

void
SK_Input_HookScePad (void)
{
  if (! config.input.gamepad.hook_scepad)
    return;

  HMODULE hModScePad =
    SK_GetModuleHandleW (L"libScePad.dll");

  if (! hModScePad)
        hModScePad = SK_LoadLibraryW (L"libScePad.dll");

  if (! hModScePad)
    return;

  if (! InterlockedCompareExchangeAcquire (&__hooked_scePad, TRUE, FALSE))
  {
    SK_LOG0 ( ( L"  >> Hooking libScePad" ),
                L"  Input   " );

    auto pCtx = &sceinput_ctx.scePad;

    pCtx->wszModuleName                         = L"libScePad.dll";
    pCtx->hMod                                  = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (hModScePad))
    {
      pCtx->scePadInit_Detour                   = SK_ScePadInit;
      pCtx->scePadGetHandle_Detour              = SK_ScePadGetHandle;
      pCtx->scePadOpen_Detour                   = SK_ScePadOpen;
      pCtx->scePadClose_Detour                  = SK_ScePadClose;
      pCtx->scePadSetParticularMode_Detour      = SK_ScePadSetParticularMode;
      pCtx->scePadRead_Detour                   = SK_ScePadRead;
      pCtx->scePadReadState_Detour              = SK_ScePadReadState;
      pCtx->scePadResetOrientation_Detour       = SK_ScePadResetOrientation;
      pCtx->scePadSetAngularVelocity\
DeadbandState_Detour                            = SK_ScePadSetAngularVelocityDeadbandState;
      pCtx->scePadSetAngularVelocity\
DeadbandState_Detour                            = SK_ScePadSetAngularVelocityDeadbandState;
      pCtx->scePadSetMotionSensorState_Detour   = SK_ScePadSetMotionSensorState;

      pCtx->scePadSetTiltCorrectionState_Detour = SK_ScePadSetTiltCorrectionState;
      pCtx->scePadSetVibration_Detour           = SK_ScePadSetVibration;
      pCtx->scePadResetLightBar_Detour          = SK_ScePadResetLightBar;
      pCtx->scePadSetLightBar_Detour            = SK_ScePadSetLightBar;

      SK_Input_HookScePadContext (pCtx);

      SK_ApplyQueuedHooks ();
    }

    InterlockedIncrementRelease (&__hooked_scePad);
  }

  //if (! pTLS->input_core->ctx_init_thread)
    SK_Thread_SpinUntilAtomicMin (&__hooked_scePad, 2);
}

void
SK_Input_PreHookScePad (void)
{
  if (! config.input.gamepad.hook_scepad)
    return;

#if 0
  xinput_ctx.XInput_SK.wszModuleName =
#ifdef _WIN64
    L"XInput_SK64.dll";
#else
    L"XInput_SK32.dll";
#endif

  static std::wstring path_to_driver =
        SK_FormatStringW ( LR"(%ws\Drivers\XInput\%ws)",
            std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)" ).c_str (),
                            xinput_ctx.XInput_SK.wszModuleName
                         );

  if (! PathFileExistsW (path_to_driver.c_str ()))
  {SK_CreateDirectories (path_to_driver.c_str ());

    wchar_t                  wszSystemHighestXInput [MAX_PATH] = { };
    PathCombineW (           wszSystemHighestXInput, SK_GetSystemDirectory (), L"XInput1_4.dll");
    if (! CopyFileW (        wszSystemHighestXInput, path_to_driver.c_str (), FALSE))
    {                       *wszSystemHighestXInput = L'\0';
      PathCombineW (         wszSystemHighestXInput, SK_GetSystemDirectory (), L"XInput1_3.dll");
      if (! CopyFileW (      wszSystemHighestXInput, path_to_driver.c_str (), FALSE))
      {                     *wszSystemHighestXInput = L'\0';
        PathCombineW (       wszSystemHighestXInput, SK_GetSystemDirectory (), L"XInput1_2.dll");
        if (! CopyFileW (    wszSystemHighestXInput, path_to_driver.c_str (), FALSE))
        {                   *wszSystemHighestXInput = L'\0';
          PathCombineW (     wszSystemHighestXInput, SK_GetSystemDirectory (), L"XInput1_1.dll");
          if (! CopyFileW (  wszSystemHighestXInput, path_to_driver.c_str (), FALSE))
          {                 *wszSystemHighestXInput = L'\0';
            PathCombineW (   wszSystemHighestXInput, SK_GetSystemDirectory (), L"XInput9_1_0.dll");
            if (! CopyFileW (wszSystemHighestXInput, path_to_driver.c_str (), FALSE))
            {
              SK_ReleaseAssert (! L"XInput Is Borked, Yay!");
            }
          }
        }
      }
    }
  }
#endif

  HMODULE hModScePad =
    SK_LoadLibraryW (L"libScePad.dll");

  if (! hModScePad)
    return;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (pTLS == nullptr)
    return;

  static scePadSetParticularMode_pfn
         scePadSetParticularMode     = (scePadSetParticularMode_pfn)SK_GetProcAddress (hModScePad,
        "scePadSetParticularMode");

  if (scePadSetParticularMode != nullptr)
      scePadSetParticularMode (true);

  //SK_RunOnce (SK_XInput_NotifyDeviceArrival ());
}