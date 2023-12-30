//
// Copyright 2021-2023 Andon "Kaldaien" Coleman
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
#include <imgui/font_awesome.h>


#include <SpecialK/control_panel/plugins.h>

#define RADICAL_REPLICANT_VERSION_NUM L"0.9.3.0"
#define RADICAL_REPLICANT_VERSION_STR L"Radical Replicant v " RADICAL_REPLICANT_VERSION_NUM

#define _RR_HDF

volatile LONG       _SK_NIER_RAD_InputPollingPeriod     = 8;
volatile BYTE       _SK_NIER_RAD_HighDynamicFramerate   = true;
         bool       _SK_NIER_RAD_AutoCursorHide         = true;
         bool       _SK_NIER_RAD_FixDInput8EnumDevices  = true;
         bool       _SK_NIER_RAD_LightSleepMode         = TRUE;
         bool       _SK_NIER_RAD_EnumOnlyForceFeedback  = false;
         bool       _SK_NIER_RAD_IgnoreJoystickHIDUsage = true;
         bool       _SK_NIER_RAD_CacheLastGamepad       = true;

sk::ParameterInt*   __SK_NIER_RAD_InputPollingPeriod     = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_AutoCursorHide         = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_HighDynamicFramerate   = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_FixDInput8EnumDevices  = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_EnumOnlyForceFeedback  = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_IgnoreJoystickHIDUsage = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_LightSleepMode         = nullptr;
sk::ParameterBool*  __SK_NIER_RAD_CacheLastGamepad       = nullptr;
sk::ParameterInt64* __SK_NIER_RAD_LastKnownTSOffset      = nullptr;

HANDLE __SK_NIER_RAD_DeviceArrivalEvent = INVALID_HANDLE_VALUE;
HANDLE __SK_NIER_RAD_DeviceRemovalEvent = INVALID_HANDLE_VALUE;

struct SK_NIER_RAD_DInput8Ctx {
  HRESULT                         hr;
  std::vector <DIDEVICEINSTANCEW> devices;
  std::vector <DIDEVICEINSTANCEW> attached;
  std::recursive_mutex            lock;
  DIDEVICEINSTANCEW               last_good = { };
};

SK_LazyGlobal <SK_NIER_RAD_DInput8Ctx> __DInput8;

struct {
  bool enabled = false;
  bool newVersion = false;

  struct {
    bool   paused =   false;
    float *dt     = nullptr;

    void pause        (void) { paused =       true; }
    void unpause      (void) { paused =      false; }
    void toggle_pause (void) { paused = (! paused); }
  } npc, player;

  struct {
    uint64_t offset    = 0x42C37A;
    uint64_t offsetNew = 0x42D81A;
    void*    addr      =  nullptr;

    struct {
      const char* instructions;
    } normal { "\xF3\x0F\x11\x01" },
      bypass { "\x90\x90\x90\x90" };
  } tick; // tock

  struct {
    uint64_t offset    = 0x42C371;
    uint64_t offsetNew = 0x42D811;
    void*    addr      =  nullptr;

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

    if (!(SK_ValidatePointer(tick.addr) && !memcmp(tick.addr, tick.normal.instructions, 4) &&
      SK_ValidatePointer(dt.addr) && !memcmp(dt.addr, dt.normal.instructions, 5)))
    {

      tick.addr =
        (LPVOID)((uintptr_t)pBase + tick.offsetNew);
      dt.addr =
        (LPVOID)((uintptr_t)pBase + dt.offsetNew);

      if (!(SK_ValidatePointer(tick.addr) && !memcmp(tick.addr, tick.normal.instructions, 4) &&
        SK_ValidatePointer(dt.addr) && !memcmp(dt.addr, dt.normal.instructions, 5)))
      {
        tick.addr = nullptr;
        dt.addr = nullptr;

        return false;
      }

      newVersion = true;
      return true;
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

  void step (float _dt)
  {
    assert (enabled);

    if (! enabled)
      return;

    static auto
      tickOffset = (newVersion ? 0x4B1D340: 0x4B1B0B0),
      stepOffset = (newVersion ? 0x4B1D344 : 0x4B1B0B4);

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
    {
      VirtualProtect ( pTick, sizeof (float),
                         PAGE_EXECUTE_READWRITE, &dwOrigProtect );
      VirtualProtect ( pStep, sizeof (float),
                         PAGE_EXECUTE_READWRITE, &dwOrigProtect );
      VirtualProtect ( pStepNPC, sizeof (float),
                         PAGE_EXECUTE_READWRITE, &dwOrigProtect );
    });

    *pStep    = player.paused ? 0.0f : _dt;
    *pStepNPC = npc.paused    ? 0.0f : _dt;
    *pTick    =                       1.0f;
  }
} static clockOverride;

struct _framerate_ctx_s {
  bool compatible = false;
  bool dynamic    = false;

  float       dtFloatBackup = 0.0f;

  static bool _suspended;
  static auto _suspendThreads (void)
  {
    std::queue <DWORD> suspended_tids;

#ifdef SAFE_PATCH
    if (! std::exchange (_suspended, true))
      suspended_tids = SK_SuspendAllOtherThreads ();

    if (suspended_tids.empty ())
       _suspended = false;
#endif

    return suspended_tids;
  }

  static void _resumeThreads (std::queue <DWORD>& tids)
  {
    if (tids.empty ())
      return;

#ifdef SAFE_PATCH
    if (std::exchange (_suspended, false))
      SK_ResumeThreads (tids);
#endif
  }

  struct patch_s
  {
          size_t   size;
    const char*    replacement;

          bool     patched    = false;

          void*    patch_addr = nullptr;
          void*    orig_code  = nullptr;

    bool apply (void* addr)
    {
      bool  ret          = false;
      DWORD dwOrigAccess = 0x0UL,
            dwNewAccess  = PAGE_EXECUTE_READWRITE;

      assert (patch_addr == 0x0 || patch_addr == addr);

      auto suspended_tids =
        _suspendThreads ();

	    if (VirtualProtect (addr, size, dwNewAccess, &dwOrigAccess))
      {
        if (      orig_code == nullptr)
        {         orig_code = std::malloc (size);
          memcpy (orig_code, addr,         size);
        }

        memcpy         (addr, replacement, size);
	      VirtualProtect (addr, size, dwOrigAccess, &dwOrigAccess);

        ret = true;
      }

      if (ret)
        patched = true;

      _resumeThreads (suspended_tids);

      return ret;
    }

    bool revert (void)
    {
      if (! patched)
        return true; // nop

      auto suspended_tids =
        _suspendThreads ();

      bool  ret          = false;
      DWORD dwOrigAccess = 0x0UL,
            dwNewAccess  = PAGE_EXECUTE_READWRITE;

      if (VirtualProtect (patch_addr,            size, dwNewAccess,  &dwOrigAccess))
      {   memcpy         (patch_addr, orig_code, size);
          VirtualProtect (patch_addr,            size, dwOrigAccess, &dwOrigAccess);

        ret = true;
      }

      _resumeThreads (suspended_tids);

      return ret;
    }

    ~patch_s (void) { revert (); std::free (std::exchange (orig_code, nullptr)); }

  } dtPatch        { 4,  "\xAD\xBE\xFA\xFF"                        },
    rotationPatch0 { 8,  "\xE9\x2B\x8E\xE0\xFF\x90\x90\x90"        },
    rotationPatch1 { 26, "\xF3\x0F\x10\x0D\xCC\x7E\x8D\x00\xF3"
                         "\x0F\x5E\x0D\xE0\x51\x6A\x04\xF3\x0F"
                         "\x59\xD1\xE9\xBF\x71\x1F\x00\x90"        },

    dtPatchNew         { 1 , "\x81"                                },
    rotationPatch0New  { 8 , "\xE9\x6B\x8D\xE0\xFF\x90\x90\x90"    },
    rotationPatch1New  { 26, "\xF3\x0F\x10\x0D\xBC\x7D\x8D\x00\xF3"
                             "\x0F\x5E\x0D\x40\x60\x6A\x04\xF3\x0F"
                             "\x59\xD1\xE9\x7F\x72\x1F\x00\x90"    };

  struct address_s {
    uintptr_t offset;
    void*     ptr;
  } dtFloat        { 0x15228,   nullptr },
    dtFix          { 0,         nullptr },
    dtVal          { 0x4B1B0B4, nullptr },
    rotationCave   { 0,         nullptr },
    rotationDetour { 0,         nullptr },
    bLoadScreen    { 0,         nullptr };

  bool init (bool newVersion)
  {
    compatible =
      clockOverride.init ();

    auto pBaseAddr =
      reinterpret_cast <uintptr_t> (SK_Debug_GetImageBaseAddr ());

    if (newVersion)
    {
      dtFix.offset          = 0x6938B;
      rotationCave.offset   = 0x4772F0;
      rotationDetour.offset = 0x66E580;
      bLoadScreen.offset    = 0x4B1C9B4;
    }
    else
    {
      dtFix.offset          = 0x69377;
      rotationCave.offset   = 0x475EC0;
      rotationDetour.offset = 0x66D090;
      bLoadScreen.offset    = 0x2704050;
    }

    dtFloat.ptr        = reinterpret_cast <void *>(pBaseAddr + dtFloat.offset       );
    dtFix.ptr          = reinterpret_cast <void *>(pBaseAddr + dtFix.offset         );
    dtVal.ptr          = reinterpret_cast <void *>(pBaseAddr + dtVal.offset         );
    rotationCave.ptr   = reinterpret_cast <void *>(pBaseAddr + rotationCave.offset  );
    rotationDetour.ptr = reinterpret_cast <void *>(pBaseAddr + rotationDetour.offset);
    bLoadScreen.ptr    = reinterpret_cast <void *>(pBaseAddr + bLoadScreen.offset   );

    return true;
  }

  //bLoadScreen for the original version returns 0 or 1 if one is on the menu, for the new one 0, 1, 2 depending on the actual type of loading screen (!= 0 handles both)
  bool isInLoadScreen(void) { return compatible && (*(uint8_t*)bLoadScreen.ptr != 0); }

  bool enable (bool newVersion)
  {
    bool ret = false;

    if (! init (newVersion))
      return ret;

    if (compatible && (! dynamic))
    {
      if (newVersion)
      {
        ret = dtPatchNew.apply(dtFix.ptr);
        ret &= rotationPatch0New.apply(rotationDetour.ptr);
        ret &= rotationPatch1New.apply(rotationCave.ptr);

        if (!ret)
        {
          dtPatchNew.revert();
          rotationPatch0New.revert();
          rotationPatch1New.revert();
        }
      }
      else
      {
        DWORD                                                                dwOrigProt;
        if (VirtualProtect(dtFloat.ptr, 4, PAGE_EXECUTE_READWRITE, &dwOrigProt))
        {
          dtFloatBackup = *(float*)dtFloat.ptr;
          *(float*)dtFloat.ptr = 0.0f;
          VirtualProtect(dtFloat.ptr, 4, dwOrigProt, &dwOrigProt);

          ret = dtPatch.apply(dtFix.ptr);
          ret &= rotationPatch0.apply(rotationDetour.ptr);
          ret &= rotationPatch1.apply(rotationCave.ptr);
        }

        if (!ret)
        {
          dtPatch.revert();
          rotationPatch0.revert();
          rotationPatch1.revert();
        }
      }

      if (ret)
        dynamic = true;
    }

    return ret;
  }

  bool disable (bool newVersion)
  {
    bool ret = false;

    if (! init (newVersion))
      return ret;

    if (compatible && dynamic)
    {
      if (newVersion)
      {
        ret = dtPatchNew.revert();

        ret &= rotationPatch0New.revert();
        ret &= rotationPatch1New.revert();
      }
      else
      {
        DWORD                                                                dwOrigProt;
        if (VirtualProtect(dtFloat.ptr, 4, PAGE_EXECUTE_READWRITE, &dwOrigProt))
        {
          *(float*)dtFloat.ptr = dtFloatBackup;
          VirtualProtect(dtFloat.ptr, 4, dwOrigProt, &dwOrigProt);
        }

        ret = dtPatch.revert();

        ret &= rotationPatch0.revert();
        ret &= rotationPatch1.revert();
      }

      if (ret)
        dynamic = false;
    }

    if (! dynamic)
      SK_GetCommandProcessor ()->ProcessCommandLine ("TargetFPS 60.0");

    return ret;
  }
} framerate_ctl;

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
        SK_WaitForSingleObject (hStartStop, INFINITE);

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
              static_cast <double> (SK_PerfFreq),
                nowTime
            );
          }
        }
      } while (! ReadAcquire (&__SK_DLL_Ending));

      SK_CloseHandle (hStartStop);

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
#ifdef _RR_HDF
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

    if (bHighDynamicFramerate)
      framerate_ctl.enable (clockOverride.newVersion);
    else
    {
      framerate_ctl.disable(clockOverride.newVersion);
    }
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto* pLimiter =
    SK::Framerate::GetLimiter (rb.swapchain);

  if ( bHighDynamicFramerate &&
         ( (pLimiter->get_limit () == 60.0 && (! framerate_ctl.isInLoadScreen ())) ||
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
      " Thanks to Ersh and SkacikPL for the initial implementation, Banz99 for updating it"
  );

  ImGui::EndGroup ();
  ImGui::TreePop  ();

  if (changed) {
    SK_GetDLLConfig ()->write (
      SK_GetDLLConfig ()->get_filename ()
    );
  }


  return changed;
#else
  return true;
#endif
}

bool
SK_NIER_RAD_VisualCpl (void)
{
  ImGui::TreePush ("");

  bool changed = false;

  changed |= ImGui::Checkbox ("D3D11 State Tracker Performance Mode", &config.render.dxgi.low_spec_mode);

  if (ImGui::IsItemHovered ())
      ImGui::SetTooltip (
        "Reduce Special K's Render Mod Functionality Unless Mod Tools are Open or Texture Mods are Present"
      );

  ImGui::BeginGroup ();
  {
    changed |= ImGui::Checkbox ("Enable HUDless Screenshots and Texture Mods", &config.render.dxgi.deferred_isolation);

    if (ImGui::IsItemHovered ())
        ImGui::SetTooltip (
          "Turning this off may boost framerate slightly on CPU-limited systems"
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

#ifdef _RR_HDF
    if (clockOverride.npc.dt != nullptr) // Validated pointer avoids showing this option if executable is invalid
    {
      bool hdf =
        ImGui::CollapsingHeader (ICON_FA_FLAG_CHECKERED "\tHigh Dynamic Framerate");

      if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("NOTE: The game will be limited to 60 FPS at the title screen and during load screens");

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
#endif

    if (ImGui::CollapsingHeader (ICON_FA_MOUSE "\tInput Settings"))
    {
      ImGui::TreePush ("");

      auto& di8 =
        __DInput8.get ();

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
              static_cast <LONG> ( seconds * 1000.0f );
          }
        }
      }

      if (ImGui::Checkbox ("Ignore non-rumble DirectInput Gamepads", &_SK_NIER_RAD_EnumOnlyForceFeedback))
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

      if (ImGui::Checkbox ("Enable Gamepad Hot-Plug Support", &_SK_NIER_RAD_CacheLastGamepad))
      {
        __SK_NIER_RAD_CacheLastGamepad->store (_SK_NIER_RAD_CacheLastGamepad);

        changed = true;
      }

      if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Cache the last used controller so gamepads do not need to be turned on / plugged in before starting the game.");

      if (_SK_NIER_RAD_CacheLastGamepad || di8.last_good.wUsage == 0x05)
      {
        ImGui::SameLine      ();
        ImGui::Text          ("\t" ICON_FA_GAMEPAD " Hot-Plug Device:  %hs\t", SK_WideCharToUTF8 (di8.last_good.tszProductName).c_str ());
        ImGui::SameLine      ();

        if (ImGui::Button ("Clear Cache"))
        {
           _SK_NIER_RAD_CacheLastGamepad = false;
          __SK_NIER_RAD_CacheLastGamepad->store (_SK_NIER_RAD_CacheLastGamepad);

          changed = true;

          ZeroMemory ( &di8.last_good,
                sizeof (di8.last_good) );

          DeleteFileW (L"dinput8.devcache");
        }
      }

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
  if ( SK_XInput_Backend->getInputAge (4UL) < 1.0f || // 4 == Any XInput Slot
          SK_DI8_Backend->getInputAge (sk_input_dev_type::Gamepad) < 1.0f )
  {
    extern XINPUT_STATE di8_to_xi;

    if ( SK_XInput_Backend->viewed.gamepad  < 2 && SK_XInput_Backend->viewed.mouse < 2 &&
         SK_XInput_Backend->viewed.keyboard < 2 && SK_XInput_Backend->viewed.other < 2 &&
             di8_to_xi.dwPacketNumber != 0 )
    {
      config.input.gamepad.native_ps4        = true;
      config.input.gamepad.predefined_layout = 0; // PS4, not Steam controller
    }

    if (_SK_NIER_RAD_AutoCursorHide)
    {
      config.input.cursor.manage = true;
    }

    // Enable this for XInput users only
    if (_SK_NIER_RAD_FixDInput8EnumDevices)
    {
      //if (SK_XInput_Backend->getInputAge (4UL) < 1.0f /*|| SK_DI8_Backend->getInputAge (sk_input_dev_type::Gamepad ???*/)
        config.input.gamepad.disable_ps4_hid = true;
    }
  }


  extern float __target_fps;

  static float origFpsLimit    =  0.0f;
  static bool  wasInLoadScreen = false;

  if (framerate_ctl.isInLoadScreen ())
  {
    if (! std::exchange (wasInLoadScreen, true))
      origFpsLimit = __target_fps;

    __target_fps = 60.0;
  }

  else
  {
    if (std::exchange (wasInLoadScreen, false))
      __target_fps = origFpsLimit;
  }
}

#include <minwindef.h>

using  KeyboardProc_pfn = LRESULT (CALLBACK *)(int, WPARAM, LPARAM);
static KeyboardProc_pfn
      _KeyboardProc = nullptr;

extern
LRESULT
CALLBACK
SK_Proxy_KeyboardProc (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam  );

LRESULT
CALLBACK
SK_NIER_RAD_KeyboardProc (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam  )
{
  // Game uses a keyboard hook for input that the Steam overlay cannot block
  if (SK::SteamAPI::GetOverlayState (true))
  {
    return
      CallNextHookEx (0, nCode, wParam, lParam);
  }

  return
    _KeyboardProc (nCode, wParam, lParam);
}

static IDirectInput8W_EnumDevices_pfn _IDirectInput8W_EnumDevices = nullptr;

BOOL DIEnumDevicesCallback_ ( LPCDIDEVICEINSTANCE lpddi,
                              LPVOID pvRef )
{
  UNREFERENCED_PARAMETER (pvRef);

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
    __DInput8->devices.push_back (*lpddi);

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
  auto& di8 =
    __DInput8.get ();

  if (dwDevType == DI8DEVCLASS_GAMECTRL && _SK_NIER_RAD_FixDInput8EnumDevices)
  {
    std::scoped_lock <std::recursive_mutex> lock
                                       (di8.lock);

    static bool          once = false;
    if (! std::exchange (once, true))
    {
      SK_LOG0 ( ( L"IDirectInput8W::EnumDevices (DevType=%x, Flags: %x) { Allowed }",
                                             dwDevType,  dwFlags),
                  L"RadicalRep" );

      di8.hr =
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

      di8.attached.clear ();

      for ( auto& device : di8.devices )
      {
        if ( DI_OK == This->GetDeviceStatus (device.guidInstance) )
        {
          di8.attached.push_back (device);

          if (_SK_NIER_RAD_CacheLastGamepad && device.wUsage == 0x05)
          {
            di8.last_good = device;

            if (FILE* fLastKnownController  = fopen ("dinput8.devcache", "wb");
                      fLastKnownController != nullptr)
            {
              fwrite (&di8.last_good, sizeof (di8.last_good), 1,
                      fLastKnownController);
              fclose (fLastKnownController);
            }
          }
        }
      }
    }

    for ( auto& attached : di8.attached )
    {
      if ( lpCallback (&attached, pvRef) == DIENUM_STOP )
        break;
    }

    if (di8.attached.empty () && di8.last_good.wUsage == 0x05)
      lpCallback (&di8.last_good, pvRef);

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

  auto& di8 =
    __DInput8.get ();

  __SK_NIER_RAD_InputPollingPeriod =
    _CreateConfigParameterInt ( L"Radical.Replicant",
                                L"InputThreadPollingMs", (int &)_SK_NIER_RAD_InputPollingPeriod,
                                                         L"Input Polling" );

  if (! __SK_NIER_RAD_InputPollingPeriod->load  ((int &)_SK_NIER_RAD_InputPollingPeriod))
        __SK_NIER_RAD_InputPollingPeriod->store (8);

#ifdef _RR_HDF
  __SK_NIER_RAD_HighDynamicFramerate =
    _CreateConfigParameterBool ( L"Radical.Replicant",
                                 L"HighDynamicFramerate", (bool &)_SK_NIER_RAD_HighDynamicFramerate,
                                                          L"Moar Powa!" );

  if (! __SK_NIER_RAD_HighDynamicFramerate->load  ((bool &)_SK_NIER_RAD_HighDynamicFramerate))
        __SK_NIER_RAD_HighDynamicFramerate->store (true);

  static const
    int64_t    default_tsOffset = 0xABCBD0;
    int64_t default_newtsOffset = 0xABDC14;
    int64_t        lasttsOffset = 0x0;
    int64_t            tsOffset = 0x0;

  __SK_NIER_RAD_LastKnownTSOffset =
    _CreateConfigParameterInt64 ( L"Radical.Replicant",
                                  L"LastKnownTimestepOffset", tsOffset,
                                                              L"Testing..." );

  if (! __SK_NIER_RAD_LastKnownTSOffset->load  (tsOffset))
        __SK_NIER_RAD_LastKnownTSOffset->store (default_tsOffset);

  static const float* pfTimestep =
    reinterpret_cast <float *> (
      (uintptr_t)SK_Debug_GetImageBaseAddr () + tsOffset);

  static bool bValid =
    SK_ValidatePointer (pfTimestep, true)  &&
                      (*pfTimestep > 0.016 &&
                       *pfTimestep < 0.017);

  lasttsOffset = tsOffset;

  if (!bValid)
  {
    pfTimestep =
      reinterpret_cast <float*> (
        (uintptr_t)SK_Debug_GetImageBaseAddr() + default_tsOffset);

    bValid =
      SK_ValidatePointer(pfTimestep, true) &&
      (*pfTimestep > 0.016 &&
        *pfTimestep < 0.017);

    lasttsOffset = default_tsOffset;
  }

  if (!bValid)
  {
    pfTimestep =
      reinterpret_cast <float*> (
        (uintptr_t)SK_Debug_GetImageBaseAddr() + default_newtsOffset);

    bValid =
      SK_ValidatePointer(pfTimestep, true) &&
      (*pfTimestep > 0.016 &&
        *pfTimestep < 0.017);

    lasttsOffset = default_newtsOffset;
  }

  if (bValid && clockOverride.init ())
  {
    clockOverride.npc.dt =
      const_cast <float *>(pfTimestep);

    __SK_NIER_RAD_LastKnownTSOffset->store (
      ( tsOffset = lasttsOffset)
    );

    if (_SK_NIER_RAD_HighDynamicFramerate)
      framerate_ctl.enable (clockOverride.newVersion);
  }

  else
  {
    if (tsOffset != 0)
    {
      SK_ImGui_Warning(
        L"Timestep Address Invalid, framerate uncap disabled.");

      __SK_NIER_RAD_LastKnownTSOffset->store(
        (tsOffset = 0)
      );
    }
      _SK_NIER_RAD_HighDynamicFramerate = false;
    __SK_NIER_RAD_HighDynamicFramerate->store(_SK_NIER_RAD_HighDynamicFramerate);

  }

#endif


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

  __SK_NIER_RAD_CacheLastGamepad =
    _CreateConfigParameterBool ( L"Radical.Replicant",
                                 L"EnabeHotPlug", _SK_NIER_RAD_CacheLastGamepad,
                      L"Use a device cache to support late insertion" );

  if (! __SK_NIER_RAD_CacheLastGamepad->load  (_SK_NIER_RAD_CacheLastGamepad))
        __SK_NIER_RAD_CacheLastGamepad->store (true);

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

  SK_CreateFuncHook (       L"Keyboard Hook",
                              SK_Proxy_KeyboardProc,
                              SK_NIER_RAD_KeyboardProc,
                static_cast_p2p <void> (&_KeyboardProc) );

  SK_EnableHook (             IDirectInput8W_EnumDevices_Detour);
  SK_EnableHook (             SK_Proxy_KeyboardProc);

  config.input.gamepad.disable_ps4_hid = false;

  plugin_mgr->config_fns.emplace (SK_NIER_RAD_PlugInCfg);

  // Disable cursor management until gamepad activity is registered
  if (_SK_NIER_RAD_AutoCursorHide)
    config.input.cursor.manage = false;

  if (_SK_NIER_RAD_CacheLastGamepad)
  {
    if (FILE* fLastKnownController  = fopen ("dinput8.devcache", "rb");
              fLastKnownController != nullptr)
    {
      fread  (&di8.last_good, sizeof (di8.last_good), 1,
              fLastKnownController);
      fclose (fLastKnownController);
    }
  }

  SK_GetDLLConfig ()->write (
    SK_GetDLLConfig ()->get_filename ()
  );
}

bool _framerate_ctx_s::_suspended = false;