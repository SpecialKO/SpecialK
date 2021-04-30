//
// Copyright 2021 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <SpecialK/stdafx.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/utility.h>
#include <SpecialK/DLL_VERSION.H>
#include <imgui/font_awesome.h>


#include <SpecialK/control_panel/plugins.h>

#define RADICAL_REPLICANT_VERSION_NUM L"0.5.7"
#define RADICAL_REPLICANT_VERSION_STR L"Radical Replicant v " RADICAL_REPLICANT_VERSION_NUM

volatile LONG       _SK_NIER_RAD_InputPollingPeriod     = 8;
volatile BYTE       _SK_NIER_RAD_HighDynamicFramerate   = false;
         bool       _SK_NIER_RAD_AutoCursorHide         = true;
         bool       _SK_NIER_RAD_FixDInput8EnumDevices  = true;
         bool       _SK_NIER_RAD_LightSleepMode         = TRUE;
         bool       _SK_NIER_RAD_EnumOnlyForceFeedback  = false;
         bool       _SK_NIER_RAD_IgnoreJoystickHIDUsage = true;

sk::ParameterInt*   __SK_NIER_RAD_InputPollingPeriod     = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_AutoCursorHide         = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_HighDynamicFramerate   = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_FixDInput8EnumDevices  = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_EnumOnlyForceFeedback  = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_IgnoreJoystickHIDUsage = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_LightSleepMode         = nullptr;
sk::ParameterInt64* __SK_NIER_RAD_LastKnownTSOffset      = nullptr;

HANDLE __SK_NIER_RAD_DeviceArrivalEvent = INVALID_HANDLE_VALUE;
HANDLE __SK_NIER_RAD_DeviceRemovalEvent = INVALID_HANDLE_VALUE;

struct {
  bool enabled = false;
  
  struct {
    bool   paused =   false;
    float *dt     = nullptr;

    void pause        (void) { paused =       true; }
    void unpause      (void) { paused =      false; }
    void toggle_pause (void) { paused = (! paused); }
  } npc, player;

  struct {
    uint64_t offset = 0x42C37A;
    void*    addr   =  nullptr;

    struct {
      const char* instructions;
    } normal { "\xF3\x0F\x11\x01" },
      bypass { "\x90\x90\x90\x90" };
  } tick; // tock

  struct {
    uint64_t offset = 0x42C371;
    void*    addr   =  nullptr;

    struct {
      const char* instructions;
    } normal { "\xF3\x0F\x11\x49\x04" },
      bypass { "\x90\x90\x90\x90\x90" };
  } dt;

  bool init (void)
  {
    if ( tick.addr != nullptr &&
           dt.addr != nullptr ) return true;

    static LPCVOID pBase =
      SK_Debug_GetImageBaseAddr ();

    if (pBase == nullptr)
      return false;

    tick.addr =
      (LPVOID)((uintptr_t)pBase + tick.offset);
    dt.addr =
      (LPVOID)((uintptr_t)pBase + dt.offset);

    if (! ( SK_ValidatePointer (tick.addr) &&
            SK_ValidatePointer (  dt.addr) ) )
    {
      tick.addr = nullptr;
      dt.addr   = nullptr;

      return false;
    }

    return true;
  }

  bool enable (void)
  {
    if (! init ())
      return false;

    if (enabled) return true;

    DWORD dwOriginalTick = 0x0,
          dwOriginalDt   = 0x0;

    VirtualProtect (
      tick.addr, 4,
        PAGE_EXECUTE_READWRITE, &dwOriginalTick );

    VirtualProtect (
      dt.addr, 5,
        PAGE_EXECUTE_READWRITE, &dwOriginalDt );

    memcpy (tick.addr, tick.bypass.instructions, 4);
    memcpy (dt.addr,   dt.bypass.instructions,   5);

    VirtualProtect (
      tick.addr, 4,
        dwOriginalTick, &dwOriginalTick );

    VirtualProtect (
      dt.addr, 5,
        dwOriginalDt, &dwOriginalDt );

    enabled = true;

    return true;
  }

  bool disable (void)
  {
    if (! init ())
      return false;

    if (! enabled) return true;

    DWORD dwOriginalTick = 0x0,
          dwOriginalDt   = 0x0;

    VirtualProtect (
      tick.addr, 4,
        PAGE_EXECUTE_READWRITE, &dwOriginalTick );

    VirtualProtect (
      dt.addr, 5,
        PAGE_EXECUTE_READWRITE, &dwOriginalDt );

    memcpy (tick.addr, tick.normal.instructions, 4);
    memcpy (dt.addr,   dt.normal.instructions,   5);

    VirtualProtect (
      tick.addr, 4,
        dwOriginalTick, &dwOriginalTick );

    VirtualProtect (
      dt.addr, 5,
        dwOriginalDt, &dwOriginalDt );
    
    enabled = false;

    return true;
  }

  void step (float dt)
  {
    assert (enabled);

    if (! enabled)
      return;

    static auto constexpr
      tickOffset = 0x4B1B0B0,
      stepOffset = 0x4B1B0B4;
    
    static auto pBaseAddr =
      SK_Debug_GetImageBaseAddr ();

    static float *pTick =
      (float *)((uintptr_t)pBaseAddr + tickOffset);
    static float *pStep =
      (float *)((uintptr_t)pBaseAddr + stepOffset);
           float *pStepNPC = npc.dt;

    static DWORD
      dwOrigProtect = 0x0;

    SK_RunOnce (
      VirtualProtect ( pTick, sizeof (float),
                         PAGE_EXECUTE_READWRITE, &dwOrigProtect )
    );
    SK_RunOnce (
      VirtualProtect ( pStep, sizeof (float),
                         PAGE_EXECUTE_READWRITE, &dwOrigProtect )
    );
    SK_RunOnce (
      VirtualProtect ( pStepNPC, sizeof (float),
                         PAGE_EXECUTE_READWRITE, &dwOrigProtect )
    );

    *pStep    = player.paused ? 0.0f : dt;
    *pStepNPC = npc.paused    ? 0.0f : dt;
    *pTick    =                      1.0f;
  }
} static clockOverride;

void
SK_NIER_RAD_GamepadLatencyTester (void)
{
  static SK::Framerate::Stats gamepad_stats;

  static bool    init       = false;
  static HANDLE  hStartStop =
    SK_CreateEvent (nullptr, TRUE, FALSE, nullptr);

  if (! init)
  {     init = true;
    SK_Thread_Create ([](LPVOID) -> DWORD
    {
      XINPUT_STATE states [2] = { };
      ULONGLONG    times  [2] = { };
      int                   i = 0;
      
      do
      {
        WaitForSingleObject (hStartStop, INFINITE);

        if (SK_XInput_PollController (config.input.gamepad.xinput.ui_slot, &states [i % 2]))
        {
          XINPUT_STATE& old = states [(i + 1) % 2];
          XINPUT_STATE& now = states [ i++    % 2];

          if (old.dwPacketNumber != now.dwPacketNumber)
          {
            LARGE_INTEGER nowTime = SK_QueryPerf ();
            ULONGLONG     oldTime = times [0];
                                    times [0] = times [1];
                                    times [1] = nowTime.QuadPart;

            gamepad_stats.addSample ( 1000.0 *
              static_cast <double> (times [0] - oldTime) /
              static_cast <double> (SK_GetPerfFreq ().QuadPart),
                nowTime
            );
          }
        }
      } while (! ReadAcquire (&__SK_DLL_Ending));

      CloseHandle (hStartStop);

      SK_Thread_CloseSelf ();

      return 0;
    }, (LPVOID)hStartStop);
  }
  
  static bool started = false;

  if (ImGui::Button (started ? "Stop Gamepad Latency Test" :
                               "Start Gamepad Latency Test"))
  {
    if (! started) { started = true;  SetEvent   (hStartStop); }
    else           { started = false; ResetEvent (hStartStop); }
  }
  
  static double high_min = std::numeric_limits <double>::max (),
                high_max,
                avg;

  if (started)
  {
    ImGui::SameLine  ( );
    ImGui::Text      ( "%lu Samples - (Min | Max | Mean) - %4.2f ms | %4.2f ms | %4.2f ms",
                         gamepad_stats.calcNumSamples (),
                         gamepad_stats.calcMin        (),
                         gamepad_stats.calcMax        (),
                         gamepad_stats.calcMean       () );

    high_min = std::min (gamepad_stats.calcMin (), high_min);
  } high_max = std::max (gamepad_stats.calcMax (), high_max);

  if (high_min < 250.0)
    ImGui::Text     ( "Minimum Latency: %4.2f ms", high_min );
}

bool
SK_NIER_RAD_PerfCpl (void)
{
  bool changed = false;

  ImGui::TreePush ("");
  ImGui::BeginGroup ();
  {
    if ( ImGui::Checkbox ( "Fix DirectInput Performance Issues",
              &_SK_NIER_RAD_FixDInput8EnumDevices ) )
    {
      __SK_NIER_RAD_FixDInput8EnumDevices->store (
        _SK_NIER_RAD_FixDInput8EnumDevices );

      changed = true;
    }
    
    changed |=
      ImGui::Checkbox ( "Enable InputThread Rescheduling",
               &config.render.framerate.sleepless_window );

    if (ImGui::Checkbox ("Light Sleep Mode", &_SK_NIER_RAD_LightSleepMode))
    {
      config.render.framerate.max_delta_time =
        _SK_NIER_RAD_LightSleepMode ?
                                  1 : 0;

      __SK_NIER_RAD_LightSleepMode->store (_SK_NIER_RAD_LightSleepMode);

      changed = true;
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip ("Enable potentially unlimited framerate, but with added CPU load");
    }
  }
  ImGui::EndGroup    ();
  ImGui::SameLine    ();
  ImGui::BeginGroup  ();
  {
    int tmp =
      ReadAcquire (&_SK_NIER_RAD_InputPollingPeriod);

    //ImGui::TextColored (ImVec4 (.2f, 1.f, 0.6f, 1.f), ICON_FA_INFO_CIRCLE);
    ImGui::TextColored (ImVec4 (1.f, 1.f, 0.f, 1.f), ICON_FA_EXCLAMATION_TRIANGLE);
    ImGui::SameLine ();
    ImGui::TextColored (ImVec4 (.85f, .85f, .85f, 1.f), "If your gamepad no longer works, turn off \"Steam Input\" for this game.");

    ImGui::SameLine    ();
    ImGui::TextColored (ImVec4 (.2f, 1.f, 0.6f, 1.f), ICON_FA_INFO_CIRCLE);
    ImGui::SameLine    ();

    changed |=
      ImGui::Checkbox  ("Steam Overlay Fix-Up", &config.steam.preload_overlay);

    if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Changing this setting may help prevent the Steam Overlay from breaking gamepad input.");

    //if (SK_DI8_Backend->viewed.gamepad != 0)
    //{
    //  ImGui::SameLine    ();
    //  ImGui::TextColored (ImVec4 (.2f, 1.f, 0.6f, 1.f), ICON_FA_QUESTION_CIRCLE);
    //  ImGui::SameLine    ();
    //  
    //  changed |=
    //    ImGui::Checkbox ("My controller is DirectInput", &config.input.gamepad.native_ps4);
    //}

    if (config.render.framerate.sleepless_window)
    {
      if (ImGui::SliderInt (ICON_FA_GAMEPAD
                            " InputThread Polling (ms)", &tmp, 4, 12))
      {
        changed = true;
        WriteRelease (&_SK_NIER_RAD_InputPollingPeriod, tmp);
        __SK_NIER_RAD_InputPollingPeriod->store (tmp);
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text ("Game Default = 8 ms, which is typical gamepad polling latency.");
        ImGui::Separator ();
        ImGui::BulletText ("Higher values may prevent performance problems, but with added input latency.");
        ImGui::BulletText ("Use SK's gamepad latency tester to see where your gamepad stacks up...");
        ImGui::EndTooltip ();
      }

      SK_NIER_RAD_GamepadLatencyTester ();
    }
    else
    {
      ImGui::Text ("");
      ImGui::Text ("");
    }
    

    if (_SK_NIER_RAD_LightSleepMode && (! config.render.framerate.sleepless_window))
    {
      ImGui::TextUnformatted (ICON_FA_INFO_CIRCLE); ImGui::SameLine ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.5f, 1.0f, 0.1f, 1.0f));
      ImGui::TextUnformatted ("For best results, also turn on InputThread Rescheduling.");
      ImGui::PopStyleColor   (1);
    }
  }
  ImGui::EndGroup ();
  ImGui::TreePop  ();

  return changed;
}

bool
SK_NIER_RAD_FramerateCpl (void)
{
  bool changed = false;

  ImGui::TreePush ("");
  ImGui::BeginGroup ();

  bool bHighDynamicFramerate =
    ReadUCharAcquire (
      &_SK_NIER_RAD_HighDynamicFramerate);

  if (ImGui::Checkbox ( "High Dynamic Framerate",
                          &bHighDynamicFramerate ))
  {
    changed = true;

    WriteUCharRelease (
    &_SK_NIER_RAD_HighDynamicFramerate,
                 bHighDynamicFramerate);
    __SK_NIER_RAD_HighDynamicFramerate->store (bHighDynamicFramerate);
  }

  if (ImGui::IsItemHovered ())
  {
    ImGui::SetTooltip ("Enable framerates > 60; some game mechanics may not work correctly.");
  }

  if (bHighDynamicFramerate)
  {
    ImGui::SameLine ();

    if ( ImGui::Button (clockOverride.npc.paused ? ICON_FA_PLAY " NPCs" :
                                                   ICON_FA_PAUSE " NPCs") )
    {
      clockOverride.npc.toggle_pause ();
    }
    
    ImGui::SameLine ();

    if ( ImGui::Button (clockOverride.player.paused ? ICON_FA_PLAY  " Player" :
                                                      ICON_FA_PAUSE " Player") )
    {
      clockOverride.player.toggle_pause ();
    }
  }
  
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto* pLimiter =
    SK::Framerate::GetLimiter (rb.swapchain);

  if ( bHighDynamicFramerate && 
         ( pLimiter->get_limit () == 60.0 ||
           pLimiter->get_limit () == 0.0f ) )
  {
    ImGui::TextColored     (ImVec4 (1.f, 1.f, 0.f, 1.f), ICON_FA_EXCLAMATION_TRIANGLE);
    ImGui::SameLine        ();
    ImGui::TextColored     (ImVec4 (1.f, 1.f, 1.f, 1.f),
                            "\tPlease Configure Special K's Framerate Limiter");
  }

  ImGui::Separator       ();
  ImGui::TextUnformatted (
    ICON_FA_SEARCH_LOCATION
      " Thanks to Ersh and SkacikPL for the initial implementation"
  );

  ImGui::EndGroup ();
  ImGui::TreePop  ();

  if (changed) {
    SK_GetDLLConfig ()->write (
      SK_GetDLLConfig ()->get_filename ()
    );
  }


  return changed;
}

bool
SK_NIER_RAD_VisualCpl (void)
{
  ImGui::TreePush ("");

  bool changed = false;

  static bool orig_deferred =
    config.render.dxgi.deferred_isolation;

  ImGui::BeginGroup ();
  {
    if (ImGui::Checkbox ("Enable HUDless Screenshots", &config.render.dxgi.deferred_isolation))
    {
      changed = true;
      //
    }

    if (ImGui::IsItemHovered ())
        ImGui::SetTooltip (
          "Adds minor CPU overhead for render state tracking; turn off for better perf."
        );
  }
  ImGui::EndGroup   ();
  ImGui::SameLine   ();
  ImGui::BeginGroup ();
  {
    if (config.render.dxgi.deferred_isolation)
    {
      ImGui::TextUnformatted (ICON_FA_INFO_CIRCLE);
      ImGui::SameLine        ();
      ImGui::TextColored     (ImVec4 (0.75f, 0.75f, 0.75f, 1.f),
          "Refer to the Screenshots section of the Control Panel for more."
      );
    }
  }
  ImGui::EndGroup   ();
  ImGui::TreePop    ();

  return changed;
}


bool SK_NIER_RAD_PlugInCfg (void)
{
  bool changed = false;

  if ( ImGui::CollapsingHeader ( "NieR Replicant ver.1.22474487139...",
                                   ImGuiTreeNodeFlags_DefaultOpen )
     )
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    bool bPerfOpen =
      ImGui::CollapsingHeader (
        ICON_FA_TACHOMETER_ALT "\tPerformance Settings",
                       ImGuiTreeNodeFlags_DefaultOpen );

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::TextColored     (ImVec4 (1.f, 1.f, 0.f, 1.f), ICON_FA_EXCLAMATION_TRIANGLE); ImGui::SameLine ();
      ImGui::TextColored     (ImVec4 (1.f, 1.f, 1.f, 1.f), "\tIt's Over 9000");
      ImGui::Separator       ();
      ImGui::TextUnformatted ("\t... turn everything on for free performance\t");         ImGui::SameLine ();
      ImGui::TextColored     (ImVec4 (0.02f, 0.68f, 0.90f, 0.45f), ICON_FA_GRIN_STARS);
      ImGui::EndTooltip      ();
    }
        
    if (bPerfOpen)
      changed |= SK_NIER_RAD_PerfCpl ();


    if (ImGui::CollapsingHeader (ICON_FA_IMAGE "\tGraphics Settings", 0x0))
    {
      changed |=
        SK_NIER_RAD_VisualCpl ();
    }

    if (clockOverride.npc.dt != nullptr)
    {
      bool hdf =
        ImGui::CollapsingHeader (ICON_FA_FLAG_CHECKERED "\tHigh Dynamic Framerate");

      if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Feature is experimental and requires an area change for speed to normalize.");

      if (hdf)
      {
        changed |=
          SK_NIER_RAD_FramerateCpl ();

        if (ReadUCharAcquire (&_SK_NIER_RAD_HighDynamicFramerate))
        {
          ImGui::TextUnformatted (ICON_FA_INFO_CIRCLE); ImGui::SameLine ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.5f, 1.0f, 0.1f, 1.0f));
          ImGui::TextUnformatted ("For best results, turn everything on in the Performance tab.");
          ImGui::PopStyleColor   (1);
        }
      }
    }

    if (ImGui::CollapsingHeader (ICON_FA_MOUSE "\tInput Settings"))
    {
      ImGui::TreePush ("");

      if (ImGui::Checkbox ("Automatic Cursor Management", &_SK_NIER_RAD_AutoCursorHide))
      {
        changed = true;

        if (! _SK_NIER_RAD_AutoCursorHide)
          config.input.cursor.manage = false;

        __SK_NIER_RAD_AutoCursorHide->store (_SK_NIER_RAD_AutoCursorHide);
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Auto-enable idle cursor hiding if gamepad input is detected");

      if (! _SK_NIER_RAD_AutoCursorHide)
      {
        changed =
          ImGui::Checkbox ("Hide Inactive Mouse Cursor", &config.input.cursor.manage);

        if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Disable this if you only play with Mouse and Keyboard and the "
                               "cursor pops up during gameplay");

        if (config.input.cursor.manage)
        {
          ImGui::SameLine ();

          float seconds =
            (float)config.input.cursor.timeout  / 1000.0f;

          if ( ImGui::SliderFloat ( "Seconds Before Hiding###NIER_RAD_HideTime",
                                      &seconds, 0.0f, 10.0f ) )
          {
            config.input.cursor.timeout =
              static_cast <LONG> (( seconds * 1000.0f ));
          }
        }
      }

      if (ImGui::Checkbox ("Ignore DirectInput Gamepads that do not support Force Feedback", &_SK_NIER_RAD_EnumOnlyForceFeedback))
      {
        __SK_NIER_RAD_EnumOnlyForceFeedback->store (
          _SK_NIER_RAD_EnumOnlyForceFeedback
        );

        changed = true;
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Prevent certain non-gamepad devices from preventing Mouse and Keyboard from working");

      if (ImGui::Checkbox ("Ignore DirectInput Joysticks", &_SK_NIER_RAD_IgnoreJoystickHIDUsage))
      {
        __SK_NIER_RAD_IgnoreJoystickHIDUsage->store (
          _SK_NIER_RAD_IgnoreJoystickHIDUsage
        );

        changed = true;
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Prevent certain non-gamepad devices from preventing Mouse and Keyboard from working");

      ImGui::TreePop ();
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);

    if (changed)
    {
      SK_GetDLLConfig ()->write (
        SK_GetDLLConfig ()->get_filename ()
      );
    }
  }

  return true;
}


#include <SpecialK/plugin/plugin_mgr.h>


void
__stdcall
SK_NIER_RAD_BeginFrame (void)
{
  // Enable cursor management for gamepad users
  if ( SK_XInput_Backend->getInputAge (sk_input_dev_type::Gamepad) < 1.0f ||
       SK_DI8_Backend->getInputAge    (sk_input_dev_type::Gamepad) < 1.0f )
  {
    extern XINPUT_STATE di8_to_xi;

    if ( SK_XInput_Backend->viewed.gamepad <  2 &&
                  di8_to_xi.dwPacketNumber != 0 )
    {
      config.input.gamepad.native_ps4 = true;
    }
    
    if (_SK_NIER_RAD_AutoCursorHide)
    {
      config.input.cursor.manage = true;
    }

    // Enable this for XInput users only
    if (_SK_NIER_RAD_FixDInput8EnumDevices)
    {
      if (SK_XInput_Backend->getInputAge (sk_input_dev_type::Gamepad) < 1.0f)
        config.input.gamepad.disable_ps4_hid = true;
    }
  }
  
  if (clockOverride.npc.dt != nullptr)
  {
    DWORD dwOrig = 0x0;

    SK_RunOnce (
      VirtualProtect (
        clockOverride.npc.dt, sizeof (float),
           PAGE_EXECUTE_READWRITE, &dwOrig )
    );

    static
      LARGE_INTEGER
        liLastFrame = SK_QueryPerf   (),
        liPerfFreq  = SK_GetPerfFreq ();
      LARGE_INTEGER
        liThisFrame = SK_QueryPerf   ();

    static float fOrigTimestep =
      *clockOverride.npc.dt;

    if (! clockOverride.enabled)
      fOrigTimestep = *clockOverride.npc.dt;

    if (ReadUCharAcquire (&_SK_NIER_RAD_HighDynamicFramerate) != 0)
    {
      clockOverride.enable ();

      auto& rb =
        SK_GetCurrentRenderBackend ();
      
      double dt =
        static_cast <double> ( liThisFrame.QuadPart -
                               liLastFrame.QuadPart ) /
        static_cast <double> (  liPerfFreq.QuadPart );

      clockOverride.step (dt);
    }

    else
    {
      if (clockOverride.enabled)
      {
        clockOverride.disable ();

        static constexpr float
          fStandardDynamicTimestep =
                       1000.0f / 60.0f / 1000.0f;

        *clockOverride.npc.dt =
          fOrigTimestep;
      }
    }

    liLastFrame =
      liThisFrame;
  }
}


static IDirectInput8W_EnumDevices_pfn _IDirectInput8W_EnumDevices = nullptr;

HRESULT                                hr_devices;
static std::vector <DIDEVICEINSTANCEW> devices_w;
static std::vector <DIDEVICEINSTANCEW> devices_attached_w;
std::recursive_mutex                   device_lock;

BOOL DIEnumDevicesCallback_ ( LPCDIDEVICEINSTANCE lpddi,
                              LPVOID pvRef )
{
  wchar_t                              wszGUID [41] = { };
  StringFromGUID2 (lpddi->guidProduct, wszGUID, 40);

  SK_LOG0 ( ( L"DirectInput Device:\t\t%s    '%s'  -  ( Usage: %d, Page: %d )",
              wszGUID, lpddi->tszProductName,
                       lpddi->wUsage,
                       lpddi->wUsagePage ), L"RadicalRep" );

  if (_SK_NIER_RAD_IgnoreJoystickHIDUsage && lpddi->wUsage != 0x05)
  {
    SK_LOG0 ( ( L" >> Ignored Non-Gamepad Device" ),
                L"RadicalRep" );
  }

  else
    devices_w.push_back (*lpddi);

  return
    DIENUM_CONTINUE;
}

HRESULT
WINAPI
IDirectInput8W_EnumDevices_Bypass ( IDirectInput8W*          This,
                                    DWORD                    dwDevType,
                                    LPDIENUMDEVICESCALLBACKW lpCallback,
                                    LPVOID                   pvRef,
                                    DWORD                    dwFlags )
{
  if (dwDevType == DI8DEVCLASS_GAMECTRL && _SK_NIER_RAD_FixDInput8EnumDevices)
  {
    std::scoped_lock <std::recursive_mutex> lock
                                    (device_lock);

    static bool          once = false;
    if (! std::exchange (once, true))
    {
      SK_LOG0 ( ( L"IDirectInput8W::EnumDevices (DevType=%x, Flags: %x) { Allowed }",
                                             dwDevType,  dwFlags),
                  L"RadicalRep" );

      hr_devices =
        _IDirectInput8W_EnumDevices ( This, dwDevType,
                                        DIEnumDevicesCallback_, pvRef,
                                          _SK_NIER_RAD_EnumOnlyForceFeedback ? DIEDFL_FORCEFEEDBACK
                                                                             : DIEDFL_ALLDEVICES );
    }

    else
    {
      SK_LOG1 ( ( L"IDirectInput8W::EnumDevices (DevType=%x, Flags: %x) { Ignored }",
                                               dwDevType,  dwFlags),
                  L"RadicalRep" );
    }

    HANDLE events [] = {
      __SK_NIER_RAD_DeviceArrivalEvent,
      __SK_NIER_RAD_DeviceRemovalEvent
    };

    DWORD dwWait =
      WaitForMultipleObjects (2, events, FALSE, 0);

    if (  dwWait ==  WAIT_OBJECT_0 ||
          dwWait == (WAIT_OBJECT_0 + 1) )
    {
      ResetEvent (
        events [dwWait - WAIT_OBJECT_0]
      );

      devices_attached_w.clear ();

      for ( auto& device : devices_w )
      {
        if ( DI_OK == This->GetDeviceStatus (device.guidInstance) )
        {
          devices_attached_w.push_back (device);
        }
      }
    }

    for ( auto& attached : devices_attached_w )
    {
      if ( lpCallback (&attached, pvRef) == DIENUM_STOP )
        break;
    }
    
    return S_OK;
  }

  return
    _IDirectInput8W_EnumDevices ( This, dwDevType,
                                    lpCallback, pvRef,
                                      dwFlags );
}

void SK_NIER_RAD_InitPlugin (void)
{
  plugin_mgr->begin_frame_fns.emplace (SK_NIER_RAD_BeginFrame);

  SK_SetPluginName (L"Special K Plug-In :: (" RADICAL_REPLICANT_VERSION_STR ")");

  __SK_NIER_RAD_InputPollingPeriod =
    _CreateConfigParameterInt ( L"Radical.Replicant",
                                L"InputThreadPollingMs", (int &)_SK_NIER_RAD_InputPollingPeriod,
                                                         L"Input Polling" );

  if (! __SK_NIER_RAD_InputPollingPeriod->load  ((int &)_SK_NIER_RAD_InputPollingPeriod))
        __SK_NIER_RAD_InputPollingPeriod->store (8);


  __SK_NIER_RAD_HighDynamicFramerate =
    _CreateConfigParameterBool ( L"Radical.Replicant",
                                 L"HighDynamicFramerate", (bool &)_SK_NIER_RAD_HighDynamicFramerate,
                                                          L"Moar Powa!" );

  if (! __SK_NIER_RAD_HighDynamicFramerate->load  ((bool &)_SK_NIER_RAD_HighDynamicFramerate))
        __SK_NIER_RAD_HighDynamicFramerate->store (false);

  __SK_NIER_RAD_AutoCursorHide =
    _CreateConfigParameterBool ( L"Radical.Replicant",
                                 L"AutoCursorManagement", (bool &)_SK_NIER_RAD_AutoCursorHide,
                                                          L"Turn on cursor auto-hide if gamepad input is detected" );

  if (! __SK_NIER_RAD_AutoCursorHide->load  (_SK_NIER_RAD_AutoCursorHide))
        __SK_NIER_RAD_AutoCursorHide->store (true);

  __SK_NIER_RAD_FixDInput8EnumDevices =
    _CreateConfigParameterBool ( L"Radical.Replicant",
                                 L"FixDInput8EnumDevices", _SK_NIER_RAD_FixDInput8EnumDevices,
                      L"Prevent excessive enumeration on every frame (performance kryptonite)" );

  if (! __SK_NIER_RAD_FixDInput8EnumDevices->load  (_SK_NIER_RAD_FixDInput8EnumDevices))
        __SK_NIER_RAD_FixDInput8EnumDevices->store (true);

  __SK_NIER_RAD_EnumOnlyForceFeedback =
    _CreateConfigParameterBool ( L"Radical.Replicant",
                                 L"EnumOnlyForceFeedbackDevices", _SK_NIER_RAD_EnumOnlyForceFeedback,
                      L"Prevent certain keyboards from acting as gamepads" );

  if (! __SK_NIER_RAD_EnumOnlyForceFeedback->load  (_SK_NIER_RAD_EnumOnlyForceFeedback))
        __SK_NIER_RAD_EnumOnlyForceFeedback->store (false);

  __SK_NIER_RAD_IgnoreJoystickHIDUsage =
    _CreateConfigParameterBool ( L"Radical.Replicant",
                                 L"IgnoreJoystickHIDUsage", _SK_NIER_RAD_IgnoreJoystickHIDUsage,
                      L"Prevent certain keyboards from acting as gamepads" );

  if (! __SK_NIER_RAD_IgnoreJoystickHIDUsage->load  (_SK_NIER_RAD_IgnoreJoystickHIDUsage))
        __SK_NIER_RAD_IgnoreJoystickHIDUsage->store (true);

  __SK_NIER_RAD_LightSleepMode =
    _CreateConfigParameterBool ( L"Radical.Replicant",
                                 L"LightSleepMode", _SK_NIER_RAD_LightSleepMode,
                                                    L"No Sleep For the Wicked" );

  if (! __SK_NIER_RAD_LightSleepMode->load  (_SK_NIER_RAD_LightSleepMode))
        __SK_NIER_RAD_LightSleepMode->store (true);

  if (! _SK_NIER_RAD_LightSleepMode)
              config.render.framerate.max_delta_time = 0;
  else        config.render.framerate.max_delta_time =
    std::max (config.render.framerate.max_delta_time, 1);

  config.input.gamepad.native_ps4 = false;

  static const
    int64_t default_tsOffset = 0xABCBD0;
    int64_t         tsOffset = 0x0;

  __SK_NIER_RAD_LastKnownTSOffset =
    _CreateConfigParameterInt64 ( L"Radical.Replicant",
                                  L"LastKnownTimestepOffset", tsOffset,
                                                              L"Testing..." );

  if (! __SK_NIER_RAD_LastKnownTSOffset->load  (tsOffset))
        __SK_NIER_RAD_LastKnownTSOffset->store (default_tsOffset);

  static const float* pfTimestep =
    reinterpret_cast <float *> (
      (((uintptr_t)SK_Debug_GetImageBaseAddr () + tsOffset) +
                                                ( tsOffset == 0 ?
                                          default_tsOffset      : 0x0 ) ) );

  static bool bValid =
    SK_ValidatePointer (pfTimestep, true)  &&
                      (*pfTimestep > 0.016 &&
                       *pfTimestep < 0.017);

  if (bValid && clockOverride.init ())
  {
    clockOverride.npc.dt =
      const_cast <float *>(pfTimestep);

    // Config had no idea, but the default value worked.
    if (tsOffset == 0)
    {
      __SK_NIER_RAD_LastKnownTSOffset->store (
        ( tsOffset = default_tsOffset )
      );
    }
  }

  else if (tsOffset != 0)
  {
    SK_ImGui_Warning (
      L"Timestep Address Invalid, framerate uncap disabled.");

    __SK_NIER_RAD_LastKnownTSOffset->store (
      ( tsOffset = 0 )
    );
  }


  // DirectInput8 EnumDevices Fixup
  // ------------------------------
  extern
  HRESULT
  WINAPI
  IDirectInput8W_EnumDevices_Detour ( IDirectInput8W*          This,
                                      DWORD                    dwDevType,
                                      LPDIENUMDEVICESCALLBACKW lpCallback,
                                      LPVOID                   pvRef,
                                      DWORD                    dwFlags );

  SK_CreateFuncHook (         L"IDirectInput8W::EnumDevices",
                              IDirectInput8W_EnumDevices_Detour,
                              IDirectInput8W_EnumDevices_Bypass,
    static_cast_p2p <void> (&_IDirectInput8W_EnumDevices) );

  extern void SK_HID_AddDeviceArrivalEvent (HANDLE hEvent);
  extern void SK_HID_AddDeviceRemovalEvent (HANDLE hEvent);

  SK_HID_AddDeviceArrivalEvent (
    ( __SK_NIER_RAD_DeviceArrivalEvent =
        SK_CreateEvent (nullptr, TRUE, TRUE, L"Device Arrival")
    )                          );

  SK_HID_AddDeviceRemovalEvent (
    ( __SK_NIER_RAD_DeviceRemovalEvent =
        SK_CreateEvent (nullptr, TRUE, FALSE, L"Device Removal")
    )                          );

  SK_EnableHook (             IDirectInput8W_EnumDevices_Detour);
  
  config.input.gamepad.disable_ps4_hid = false;

  plugin_mgr->config_fns.emplace (SK_NIER_RAD_PlugInCfg);
  
  // Disable cursor management until gamepad activity is registered
  if (_SK_NIER_RAD_AutoCursorHide)
    config.input.cursor.manage = false;

  SK_GetDLLConfig ()->write (
    SK_GetDLLConfig ()->get_filename ()
  );
}
