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

#define SK_SCEPAD_READ(backend,type)  (backend)->markRead   (type);
#define SK_SCEPAD_WRITE(backend,type) (backend)->markWrite  (type);
#define SK_SCEPAD_VIEW(backend,slot)  (backend)->markViewed ((sk_input_dev_type)(1 << slot));

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
    const wchar_t*                     wszModuleName                           = L"libScePad.dll";
    HMODULE                            hMod                                    = nullptr;

    scePadInit_pfn                     scePadInit_Detour                       = nullptr;
    scePadInit_pfn                     scePadInit_Original                     = nullptr;
    LPVOID                             scePadInit_Target                       = nullptr;

    scePadGetHandle_pfn                scePadGetHandle_Detour                  = nullptr;
    scePadGetHandle_pfn                scePadGetHandle_Original                = nullptr;
    LPVOID                             scePadGetHandle_Target                  = nullptr;

    scePadOpen_pfn                     scePadOpen_Detour                       = nullptr;
    scePadOpen_pfn                     scePadOpen_Original                     = nullptr;
    LPVOID                             scePadOpen_Target                       = nullptr;

    scePadClose_pfn                    scePadClose_Detour                      = nullptr;
    scePadClose_pfn                    scePadClose_Original                    = nullptr;
    LPVOID                             scePadClose_Target                      = nullptr;

    scePadSetParticularMode_pfn        scePadSetParticularMode_Detour          = nullptr;
    scePadSetParticularMode_pfn        scePadSetParticularMode_Original        = nullptr;
    LPVOID                             scePadSetParticularMode_Target          = nullptr;

    scePadGetControllerInformation_pfn scePadGetControllerInformation_Detour   = nullptr;
    scePadGetControllerInformation_pfn scePadGetControllerInformation_Original = nullptr;
    LPVOID                             scePadGetControllerInformation_Target   = nullptr;

    scePadRead_pfn                     scePadRead_Detour                       = nullptr;
    scePadRead_pfn                     scePadRead_Original                     = nullptr;
    LPVOID                             scePadRead_Target                       = nullptr;

    scePadReadState_pfn                scePadReadState_Detour                  = nullptr;
    scePadReadState_pfn                scePadReadState_Original                = nullptr;
    LPVOID                             scePadReadState_Target                  = nullptr;

    scePadResetOrientation_pfn         scePadResetOrientation_Detour           = nullptr;
    scePadResetOrientation_pfn         scePadResetOrientation_Original         = nullptr;
    LPVOID                             scePadResetOrientation_Target           = nullptr;

    scePadSetAngularVelocity\
DeadbandState_pfn                      scePadSetAngularVelocity\
DeadbandState_Detour                                                           = nullptr;
    scePadSetAngularVelocity\
DeadbandState_pfn                      scePadSetAngularVelocity\
DeadbandState_Original                                                         = nullptr;
    LPVOID                             scePadSetAngularVelocity\
DeadbandState_Target                                                           = nullptr;

    scePadSetMotionSensorState_pfn     scePadSetMotionSensorState_Detour       = nullptr;
    scePadSetMotionSensorState_pfn     scePadSetMotionSensorState_Original     = nullptr;
    LPVOID                             scePadSetMotionSensorState_Target       = nullptr;

    scePadSetTiltCorrectionState_pfn   scePadSetTiltCorrectionState_Detour     = nullptr;
    scePadSetTiltCorrectionState_pfn   scePadSetTiltCorrectionState_Original   = nullptr;
    LPVOID                             scePadSetTiltCorrectionState_Target     = nullptr;

    scePadSetVibration_pfn             scePadSetVibration_Detour               = nullptr;
    scePadSetVibration_pfn             scePadSetVibration_Original             = nullptr;
    LPVOID                             scePadSetVibration_Target               = nullptr;

    scePadResetLightBar_pfn            scePadResetLightBar_Detour              = nullptr;
    scePadResetLightBar_pfn            scePadResetLightBar_Original            = nullptr;
    LPVOID                             scePadResetLightBar_Target              = nullptr;

    scePadSetLightBar_pfn              scePadSetLightBar_Detour                = nullptr;
    scePadSetLightBar_pfn              scePadSetLightBar_Original              = nullptr;
    LPVOID                             scePadSetLightBar_Target                = nullptr;
  } scePad;

  Concurrency::concurrent_unordered_map <SK_ScePadHandle, ULONG64> _timeStamps;
};

SK_SceInputContext sceinput_ctx;

SK_ScePadResult
SK_ScePadInit (void)
{
  SK_ScePadResult ret = sceinput_ctx.scePad.
                                     scePadInit_Original ();

  SK_ScePadSetParticularMode (true);

  return ret;
}

SK_ScePadHandle
SK_ScePadGetHandle (SK_SceUserID userID, int type,
                                         int index)
{
  SK_ScePadHandle
          _handle = sceinput_ctx.scePad.
  scePadGetHandle_Original (userID, type, index);

  SK_LOG0 ( ( L"scePadGetHandle (%x, %x, %d) => %x",
                userID, type, index, _handle ),
              __SK_SUBSYSTEM__ );

  return _handle;
}

SK_ScePadHandle
SK_ScePadOpen (SK_SceUserID userID, int type,
                                    int index, SK_ScePadOpenParam *inputParam)
{
  SK_ScePadHandle
          _handle = sceinput_ctx.scePad.
     scePadOpen_Original (userID, type, index, inputParam);

  SK_LOG0 ( ( L"scePadOpen (%x, %x, %d, %p) => %x",
                userID, type, index, inputParam, _handle ),
              __SK_SUBSYSTEM__ );

  SK_ScePadData                                               _dontCare = { };
  if (sceinput_ctx.scePad.scePadReadState_Original (_handle, &_dontCare) == 0)
    WriteULong64Release (&sceinput_ctx._timeStamps [_handle], 1ULL);

  return _handle;
}

SK_ScePadResult
SK_ScePadClose (SK_ScePadHandle handle)
{
  WriteULong64Release (&sceinput_ctx._timeStamps [handle], 0ULL);

  return sceinput_ctx.scePad.
    scePadClose_Original (handle);
}

SK_ScePadResult
SK_ScePadSetParticularMode (bool mode)
{
  SK_LOG_FIRST_CALL;

  std::ignore = mode;

  return
    sceinput_ctx.scePad.scePadSetParticularMode_Original (true);
}

SK_ScePadResult
SK_ScePadRead (SK_ScePadHandle handle, SK_ScePadData *iData, int count)
{
  SK_LOG_FIRST_CALL;

  static auto& scePad =
    SK_ScePad_Backend;

  for (int i = 0 ; i < count ; ++i)
  {
    if (iData != nullptr && iData->connected)
      SK_SCEPAD_READ (scePad, sk_input_dev_type::Gamepad);
  }

  SK_ScePadData                surrogate = { }; SK_ScePadResult ret =
  SK_ScePadReadState (handle, &surrogate);

  if (! SK_ImGui_WantGamepadCapture ())
    return
      sceinput_ctx.scePad.
                   scePadRead_Original (handle, iData, count);

  return ret;
}

extern bool
WINAPI
SK_ImGui_ToggleEx ( bool& toggle_ui,
                    bool& toggle_nav );

// Safe in the sense that the exception is caught, the validity
//   and completeness of the memory copy is not guaranteed
bool
__cdecl
SK_SAFE_memcpy ( _Out_writes_bytes_all_ (_Size) void*       _Dst,
                 _In_reads_bytes_       (_Size) void const* _Src,
                 _In_                           size_t      _Size ) noexcept
{
  __try
  {
    memcpy (_Dst, _Src, _Size);
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return false;
  }

  return true;
}

volatile ULONG64 llLastScePadFrame = 0ULL;

SK_ScePadResult
SK_ScePadReadState (SK_ScePadHandle handle, SK_ScePadData* iData)
{
  SK_LOG_FIRST_CALL;

  using result_record_t =       std::pair <SK_ScePadResult,   SK_ScePadData>;
  using result_map_t    =
    concurrency::concurrent_unordered_map <SK_ScePadHandle, result_record_t>;

  static result_map_t
    last_result;

  static auto& scePad =
    SK_ScePad_Backend;

  bool bToggleNav = false,
       bToggleVis = false;

  const ULONG64 ullLastTimeStamp =
    ReadULong64Acquire (&sceinput_ctx._timeStamps [handle]);

  const ULONG64 ullThisFrame =
                SK_GetFramesDrawn ();

  SK_ScePadResult result =
    sceinput_ctx.scePad.
                 scePadReadState_Original (handle, iData);

  if (iData != nullptr && iData->connected)
    SK_SCEPAD_READ (scePad, sk_input_dev_type::Gamepad);

  static volatile ULONG64 ullLastChordFrame  = 0ULL;
  static volatile ULONG64 ullFirstChordFrame = 1ULL;

  if (iData != nullptr && result == SK_SCE_ERROR_OK && ullLastTimeStamp < ullThisFrame)
  {
    static auto _JustPressed =
    []( const SK_ScePadButtonBitmap&             currentButtons,
        const SK_ScePadButtonBitmap&             previousButtons,
        const SK_ScePadButtonBitmap::enum_type_t testSet ) noexcept
 -> bool
    {
      static_assert (std::is_trivial <SK_ScePadButtonBitmap::enum_type_t> ());

      return (    currentButtons.isDown (testSet) &&
              (! previousButtons.isDown (testSet)) );
    };

    static auto _JustReleased =
    []( const SK_ScePadButtonBitmap&             currentButtons,
        const SK_ScePadButtonBitmap&             previousButtons,
        const SK_ScePadButtonBitmap::enum_type_t testSet ) noexcept
 -> bool
    {
      static_assert (std::is_trivial <SK_ScePadButtonBitmap::enum_type_t> ());

      return ((!  currentButtons.isDown (testSet))
              && previousButtons.isDown (testSet));
    };

    if (config.input.gamepad.scepad.enhanced_ps_button)
    {
      if ( _JustPressed  ( iData->buttonMask, last_result [handle].
                           second.buttonMask,
                             SK_ScePadButtonBitmap::PlayStation )
         )
      {
        WriteULong64Release (&ullLastChordFrame,  ullThisFrame);
        WriteULong64Release (&ullFirstChordFrame, ullThisFrame);
      }

// This is unneeded now that Special K can poll gamepad input using HID
#if 0
      static constexpr auto ullFrameGracePeriod = 20ULL;

      if ( _JustReleased ( iData->buttonMask, last_result [handle].
                           second.buttonMask,
                             SK_ScePadButtonBitmap::PlayStation ) ||
          ( iData->buttonMask.isDown (
                             SK_ScePadButtonBitmap::PlayStation ) &&
            ReadULong64Acquire (&ullLastChordFrame ) ==
            ReadULong64Acquire (&ullFirstChordFrame) && ullThisFrame >
            ReadULong64Acquire (&ullFirstChordFrame) +  ullFrameGracePeriod )
         )
      {
        // Check to see if any other buttons are held -- if so, do not toggle the
        //   control panel
        if ( ReadULong64Acquire (&ullLastChordFrame ) ==
             ReadULong64Acquire (&ullFirstChordFrame) )
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

          WriteULong64Release (&ullLastChordFrame, ullThisFrame);
        }
      }
#endif
    }


    if (! SK_ImGui_WantGamepadCapture ())
    {
      if ((! config.input.gamepad.scepad.share_clicks_touch) &&
            _JustPressed ( iData->buttonMask, last_result [handle].
                           second.buttonMask, SK_ScePadButtonBitmap::Share
                         )
         )
      {
        WriteULong64Release (&ullLastChordFrame, ullThisFrame);

        SK_SteamAPI_TakeScreenshot ();
      }

      if ( config.input.gamepad.scepad.enhanced_ps_button &&
           iData->buttonMask.isDown (SK_ScePadButtonBitmap::PlayStation) )
      {
        if ( _JustPressed ( iData->buttonMask, last_result [handle].
                            second.buttonMask, SK_ScePadButtonBitmap::Share
                          )
           )
        {
          WriteULong64Release (&ullLastChordFrame, ullThisFrame);

          SK_SteamAPI_TakeScreenshot ();
        }

        else if ( _JustPressed ( iData->buttonMask, last_result [handle].
                                 second.buttonMask, SK_ScePadButtonBitmap::L3
                               )
                )
        {
          WriteULong64Release (&ullLastChordFrame, ullThisFrame);

          config.render.framerate.latent_sync.show_fcat_bars =
            !config.render.framerate.latent_sync.show_fcat_bars;
        }

        else if ( _JustPressed ( iData->buttonMask, last_result [handle].
                                 second.buttonMask, SK_ScePadButtonBitmap::Up
                               )
                )
        {
          WriteULong64Release (&ullLastChordFrame, ullThisFrame);

          auto cp =
            SK_GetCommandProcessor ();

          cp->ProcessCommandLine ("LatentSync.TearLocation --");
          cp->ProcessCommandLine ("LatentSync.TearLocation --");
          cp->ProcessCommandLine ("LatentSync.TearLocation --");
          cp->ProcessCommandLine ("LatentSync.TearLocation --");
          cp->ProcessCommandLine ("LatentSync.TearLocation --");
          cp->ProcessCommandLine ("LatentSync.ResyncRate ++");
          cp->ProcessCommandLine ("LatentSync.ResyncRate --");
        }

        else if ( _JustPressed ( iData->buttonMask, last_result [handle].
                                 second.buttonMask, SK_ScePadButtonBitmap::Down
                               )
                )
        {
          WriteULong64Release (&ullLastChordFrame, ullThisFrame);

          auto cp =
            SK_GetCommandProcessor ();

          cp->ProcessCommandLine ("LatentSync.TearLocation ++");
          cp->ProcessCommandLine ("LatentSync.TearLocation ++");
          cp->ProcessCommandLine ("LatentSync.TearLocation ++");
          cp->ProcessCommandLine ("LatentSync.TearLocation ++");
          cp->ProcessCommandLine ("LatentSync.TearLocation ++");
          cp->ProcessCommandLine ("LatentSync.ResyncRate --");
          cp->ProcessCommandLine ("LatentSync.ResyncRate ++");
        }
      }
    }

    if ( config.input.gamepad.scepad.mute_applies_to_game &&
         _JustPressed ( iData->buttonMask, last_result [handle].
                        second.buttonMask, SK_ScePadButtonBitmap::Mute
                      )
       )
    {
      SK_SetGameMute (! SK_IsGameMuted ());
    }


    if (config.input.gamepad.scepad.disable_touch)
    {
      iData->touchData.touchNum  =  0 ;
      iData->touchData.touch [0] = { };
      iData->touchData.touch [1] = { };

      iData->buttonMask.clearButtons (SK_ScePadButtonBitmap::TouchPad);
    }

    if (config.input.gamepad.scepad.share_clicks_touch)
    {
      if (iData->buttonMask.isDown     (SK_ScePadButtonBitmap::Share))
          iData->buttonMask.setButtons (SK_ScePadButtonBitmap::TouchPad);
    }

    InterlockedCompareExchange64 (
      (volatile LONG64 *)&sceinput_ctx._timeStamps [handle], ullLastTimeStamp, ullThisFrame);
  }


  if ((! SK_ImGui_WantGamepadCapture ()) && ( config.input.gamepad.scepad.enhanced_ps_button == false ||
                                            ( iData != nullptr &&
                                            !(iData->buttonMask.isDown (SK_ScePadButtonBitmap::PlayStation)))))
  {
    last_result [handle] =
      std::make_pair (
         result, iData != nullptr ?
                *iData            : SK_ScePadData { }
      );

    if (result == SK_SCE_ERROR_OK)
      SK_SCEPAD_VIEW (scePad, 0); // TODO: Decode Handle Value
  }

  else
  {
    auto&
      result_cache       = last_result [handle];
      result_cache.first =      result;

    if (iData != nullptr)
    {
      auto cached = // Makin copies
        result_cache.second;
        result_cache.second = *iData;

      cached.buttonMask = SK_ScePadButtonBitmap::None;

      static constexpr uint8_t neutral_stick_pos =
        std::numeric_limits <uint8_t>::max () / 2;

      cached.leftStick       = { neutral_stick_pos, neutral_stick_pos };
      cached.rightStick      = { neutral_stick_pos, neutral_stick_pos };
      cached.analogButtons   = { .l2 = 0,                  .r2 = 0    };
      cached.acceleration    = { .x  = 0.0f,  .y = 0.00f,  .z  = 0.0f };
      cached.angularVelocity = { .x  = 0.0f,  .y = 0.00f,  .z  = 0.0f };

      for ( auto& touchPt : cached.touchData.touch )
      {
        touchPt = { .x = 0,
                    .y = 0, .id = 0 };
      }

      SK_SAFE_memcpy (iData, &cached, sizeof (SK_ScePadData));
    }
  }


  if (                 bToggleVis|bToggleNav)
    SK_ImGui_ToggleEx (bToggleVis,bToggleNav);


  if (last_result [handle].first == 0)
    WriteULong64Release (&llLastScePadFrame, SK_GetFramesDrawn ());


  return
    last_result [handle].first;
}

SK_ScePadResult
SK_ScePadGetControllerInformation ( SK_ScePadHandle                handle,
                                    SK_ScePadControllerInformation *pInfo )
{
  SK_LOG_FIRST_CALL;

  return sceinput_ctx.scePad.
    scePadGetControllerInformation_Original (handle, pInfo);
}

SK_ScePadData _PrimaryPad = { };
XINPUT_STATE  sce_to_xi   = { };

bool
SK_ScePad_TranslateToXInput (SK_ScePadHandle handle, XINPUT_STATE& bridge_out)
{
  std::ignore = handle;
  std::ignore = bridge_out;

  //sceinput_ctx.scePad.scePadReadState_Original (handle, &_PrimaryPad);
  //
  //const XINPUT_STATE lastState =
  //                  bridge_out;
  //
  //static const
  //  std::vector <std::pair <SK_ScePadButtonBitmap::enum_type_t, DWORD>>
  //    fwd_map = {
  //    { SK_ScePadButtonBitmap::Share,       XINPUT_GAMEPAD_BACK           },
  //    { SK_ScePadButtonBitmap::Options,     XINPUT_GAMEPAD_START          },
  //    { SK_ScePadButtonBitmap::Up,          XINPUT_GAMEPAD_DPAD_UP        },
  //    { SK_ScePadButtonBitmap::Down,        XINPUT_GAMEPAD_DPAD_DOWN      },
  //    { SK_ScePadButtonBitmap::Left,        XINPUT_GAMEPAD_DPAD_LEFT      },
  //    { SK_ScePadButtonBitmap::Right,       XINPUT_GAMEPAD_DPAD_RIGHT     },
  //    { SK_ScePadButtonBitmap::R1,          XINPUT_GAMEPAD_RIGHT_SHOULDER },
  //    { SK_ScePadButtonBitmap::L1,          XINPUT_GAMEPAD_LEFT_SHOULDER  },
  //    { SK_ScePadButtonBitmap::R2,          XINPUT_GAMEPAD_RIGHT_TRIGGER  },
  //    { SK_ScePadButtonBitmap::L2,          XINPUT_GAMEPAD_LEFT_TRIGGER   },
  //    { SK_ScePadButtonBitmap::R3,          XINPUT_GAMEPAD_RIGHT_THUMB    },
  //    { SK_ScePadButtonBitmap::L3,          XINPUT_GAMEPAD_LEFT_THUMB     },
  //    { SK_ScePadButtonBitmap::Triangle,    XINPUT_GAMEPAD_Y              },
  //    { SK_ScePadButtonBitmap::Circle,      XINPUT_GAMEPAD_B              },
  //    { SK_ScePadButtonBitmap::Cross,       XINPUT_GAMEPAD_A              },
  //    { SK_ScePadButtonBitmap::Square,      XINPUT_GAMEPAD_X              },
  //    { SK_ScePadButtonBitmap::PlayStation, XINPUT_GAMEPAD_GUIDE          }
  //    };
  //
  //for ( const auto &[native, xinput] : fwd_map )
  //{
  //  if ( _PrimaryPad.buttonMask.isDown (native) )
  //       bridge_out.Gamepad.wButtons |= xinput;
  //}
  //
  //if (memcmp ( &bridge_out.Gamepad,
  //              &lastState.Gamepad,
  //   sizeof (XINPUT_STATE::Gamepad) ))
  //{
  //  bridge_out.dwPacketNumber++;
  //  return true;
  //}

  return true;
}

void
SK_ScePad_PaceMaker (void)
{
  for ( const auto &[handle , timeStamp] : sceinput_ctx._timeStamps )
  {
    if ( const auto lastTimeStamp = ReadULong64Acquire (&timeStamp);
                    lastTimeStamp > 0ULL &&
                    lastTimeStamp < SK_GetFramesDrawn () )
    {
      if (timeStamp != 0x0)
      {
        SK_ScePadReadState (handle, &_PrimaryPad);
      //sceinput_ctx.scePad.scePadReadState_Original (handle, &_PrimaryPad);
      }
    }
  }
}

SK_ScePadResult
SK_ScePadResetOrientation (SK_ScePadHandle handle)
{
  return sceinput_ctx.scePad.
    scePadResetOrientation_Original (handle);
}

SK_ScePadResult
SK_ScePadSetAngularVelocityDeadbandState (SK_ScePadHandle handle, bool enable)
{
  return sceinput_ctx.scePad.
    scePadSetAngularVelocityDeadbandState_Original (handle, enable);
}

SK_ScePadResult
SK_ScePadSetMotionSensorState (SK_ScePadHandle handle, bool enable)
{
  return sceinput_ctx.scePad.
    scePadSetMotionSensorState_Original (handle, enable);
}

SK_ScePadResult
SK_ScePadSetTiltCorrectionState (SK_ScePadHandle handle, bool enable)
{
  return sceinput_ctx.scePad.
    scePadSetTiltCorrectionState_Original (handle, enable);
}

SK_ScePadResult
SK_ScePadSetVibration (SK_ScePadHandle handle, SK_ScePadVibrationParam *param)
{
  return sceinput_ctx.scePad.
    scePadSetVibration_Original (handle, param);
}

SK_ScePadResult
SK_ScePadResetLightBar (SK_ScePadHandle handle)
{
  return sceinput_ctx.scePad.
    scePadResetLightBar_Original (handle);
}

SK_ScePadResult
SK_ScePadSetLightBar (SK_ScePadHandle handle, SK_ScePadColor* param)
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
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadInit",
                              pCtx->scePadInit_Detour,
     static_cast_p2p <void> (&pCtx->scePadInit_Original) );

  pCtx->scePadGetHandle_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadGetHandle" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadGetHandle",
                              pCtx->scePadGetHandle_Detour,
     static_cast_p2p <void> (&pCtx->scePadGetHandle_Original) );

  pCtx->scePadOpen_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadOpen" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadOpen",
                              pCtx->scePadOpen_Detour,
     static_cast_p2p <void> (&pCtx->scePadOpen_Original) );

  pCtx->scePadClose_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadClose" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadClose",
                              pCtx->scePadClose_Detour,
     static_cast_p2p <void> (&pCtx->scePadClose_Original) );

  pCtx->scePadSetParticularMode_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetParticularMode" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadSetParticularMode",
                              pCtx->scePadSetParticularMode_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetParticularMode_Original) );

  pCtx->scePadRead_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadRead" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadRead",
                              pCtx->scePadRead_Detour,
     static_cast_p2p <void> (&pCtx->scePadRead_Original) );

  pCtx->scePadReadState_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadReadState" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadReadState",
                              pCtx->scePadReadState_Detour,
     static_cast_p2p <void> (&pCtx->scePadReadState_Original) );

  pCtx->scePadResetOrientation_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadResetOrientation" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadResetOrientation",
                              pCtx->scePadResetOrientation_Detour,
     static_cast_p2p <void> (&pCtx->scePadResetOrientation_Original) );

  pCtx->scePadSetAngularVelocityDeadbandState_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetAngularVelocityDeadbandState" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadSetAngularVelocityDeadbandState",
                              pCtx->scePadSetAngularVelocityDeadbandState_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetAngularVelocityDeadbandState_Original) );

  pCtx->scePadSetMotionSensorState_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetMotionSensorState" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadSetMotionSensorState",
                              pCtx->scePadSetMotionSensorState_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetMotionSensorState_Original) );

  pCtx->scePadSetTiltCorrectionState_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetTiltCorrectionState" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadSetTiltCorrectionState",
                              pCtx->scePadSetTiltCorrectionState_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetTiltCorrectionState_Original) );

  pCtx->scePadSetVibration_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetVibration" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadSetVibration",
                              pCtx->scePadSetVibration_Detour,
     static_cast_p2p <void> (&pCtx->scePadSetVibration_Original) );

  pCtx->scePadGetControllerInformation_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadGetControllerInformation" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadGetControllerInformation",
                              pCtx->scePadGetControllerInformation_Detour,
     static_cast_p2p <void> (&pCtx->scePadGetControllerInformation_Original) );

  pCtx->scePadResetLightBar_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadResetLightBar" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadResetLightBar",
                              pCtx->scePadResetLightBar_Detour,
     static_cast_p2p <void> (&pCtx->scePadResetLightBar_Original) );

  pCtx->scePadSetLightBar_Target =
  SK_GetProcAddress (         pCtx->wszModuleName, "scePadSetLightBar" );
  SK_CreateDLLHook2 (         pCtx->wszModuleName, "scePadSetLightBar",
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

    auto pCtx =
      &sceinput_ctx.scePad;

    pCtx->wszModuleName                         = L"libScePad.dll";
    pCtx->hMod                                  = SK_Modules->LoadLibrary (pCtx->wszModuleName);

    if (SK_Modules->isValid (hModScePad))
    {
      pCtx->scePadInit_Detour                     = SK_ScePadInit;
      pCtx->scePadGetHandle_Detour                = SK_ScePadGetHandle;
      pCtx->scePadOpen_Detour                     = SK_ScePadOpen;
      pCtx->scePadClose_Detour                    = SK_ScePadClose;
      pCtx->scePadSetParticularMode_Detour        = SK_ScePadSetParticularMode;
      pCtx->scePadRead_Detour                     = SK_ScePadRead;
      pCtx->scePadReadState_Detour                = SK_ScePadReadState;
      pCtx->scePadResetOrientation_Detour         = SK_ScePadResetOrientation;
      pCtx->scePadSetAngularVelocity\
DeadbandState_Detour                              = SK_ScePadSetAngularVelocityDeadbandState;
      pCtx->scePadSetMotionSensorState_Detour     = SK_ScePadSetMotionSensorState;

      pCtx->scePadSetTiltCorrectionState_Detour   = SK_ScePadSetTiltCorrectionState;
      pCtx->scePadSetVibration_Detour             = SK_ScePadSetVibration;
      pCtx->scePadGetControllerInformation_Detour = SK_ScePadGetControllerInformation;
      pCtx->scePadResetLightBar_Detour            = SK_ScePadResetLightBar;
      pCtx->scePadSetLightBar_Detour              = SK_ScePadSetLightBar;

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

  static std::filesystem::path path_to_driver =
    std::filesystem::path (SK_GetInstallPath ()) /
     LR"(Drivers\PlayStation\libScePad_sk64.dll)";

  if (! PathFileExistsW (path_to_driver.c_str ()))
  {SK_CreateDirectories (path_to_driver.c_str ());}

  HMODULE hModScePad =
    SK_LoadLibraryW (sceinput_ctx.scePad.wszModuleName);

  if (! hModScePad)
    return;

  static auto
      scePadSetParticularMode =
     (scePadSetParticularMode_pfn)SK_GetProcAddress (hModScePad,
     "scePadSetParticularMode");

  if (scePadSetParticularMode != nullptr)
      scePadSetParticularMode (true);
}