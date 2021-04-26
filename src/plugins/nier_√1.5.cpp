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

#define RADICAL_REPLICANT_VERSION_NUM L"0.3.1"
#define RADICAL_REPLICANT_VERSION_STR L"Radical Replicant v " RADICAL_REPLICANT_VERSION_NUM

volatile LONG     _SK_NIER_RAD_InputPollingPeriod = 8;
volatile LONG     _SK_NIER_RAD_ResilientFramerate = 0;

sk::ParameterInt* __SK_NIER_RAD_InputPollingPeriod;
sk::ParameterInt* __SK_NIER_RAD_ResilientFramerate;

/*
NieR Replicant ver.1.22474487139.exe+42C37A - F3 0F11 01            - movss [rcx],xmm0
Write 1.0f there for perfect time dilation / smooth framerate
whole function:
NieR Replicant ver.1.22474487139.exe+42C360 - FF 41 18              - inc [rcx+18]
NieR Replicant ver.1.22474487139.exe+42C363 - 0F28 C1               - movaps xmm0,xmm1
NieR Replicant ver.1.22474487139.exe+42C366 - F3 0F59 05 261A9200   - mulss xmm0,["NieR Replicant ver.1.22474487139.exe"+D4DD94] { (60,00) }
NieR Replicant ver.1.22474487139.exe+42C36E - FF 41 1C              - inc [rcx+1C]
NieR Replicant ver.1.22474487139.exe+42C371 - F3 0F11 49 04         - movss [rcx+04],xmm1
NieR Replicant ver.1.22474487139.exe+42C376 - F3 0F5A C9            - cvtss2sd xmm1,xmm1
NieR Replicant ver.1.22474487139.exe+42C37A - F3 0F11 01            - movss [rcx],xmm0                << Write time dilation: 0.0 is pause. Vegetation still moves. 
NieR Replicant ver.1.22474487139.exe+42C37E - F2 0F58 49 10         - addsd xmm1,[rcx+10]
NieR Replicant ver.1.22474487139.exe+42C383 - F2 0F11 49 10         - movsd [rcx+10],xmm1
NieR Replicant ver.1.22474487139.exe+42C388 - C3                    - ret 

xmm1 contains the frame time, e.g. 0.016666
*/




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

        if (SK_XInput_PollController (0, &states [i % 2]))
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
    changed |=
      ImGui::Checkbox ( "Disable HID / XInput Stutter Quagmire",
                         &config.input.gamepad.disable_ps4_hid );
    
    if (ImGui::IsItemHovered ())
        ImGui::SetTooltip (
            "Requires a full game restart to fully take effect");

    changed |=
      ImGui::Checkbox ( "Enable InputThread Rescheduling",
               &config.render.framerate.sleepless_window );
  }
  ImGui::EndGroup    ();

  if (config.render.framerate.sleepless_window)
  {
    int tmp =
     ReadAcquire (&_SK_NIER_RAD_InputPollingPeriod);

    ImGui::SameLine    ();
    ImGui::BeginGroup  ();
    {
      if (ImGui::SliderInt (ICON_FA_GAMEPAD 
                              " InputThread Polling (ms)",    &tmp, 4, 12))
      {
        changed = true;
        WriteRelease (&_SK_NIER_RAD_InputPollingPeriod,        tmp);
                      __SK_NIER_RAD_InputPollingPeriod->store (tmp);
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Game Default = 8 ms, which is typical gamepad polling latency.");
        ImGui::Separator    ();
        ImGui::BulletText   ("Higher values may prevent performance problems, but with added input latency.");
        ImGui::BulletText   ("Use SK's gamepad latency tester to see where your gamepad stacks up...");
        ImGui::EndTooltip   ();
      }

      SK_NIER_RAD_GamepadLatencyTester ();
    }
    ImGui::EndGroup ();
  }

  bool bLightSleep =
    config.render.framerate.max_delta_time == 1;

  ImGui::BeginGroup ();
  {
    if (ImGui::Checkbox ("Light Sleep Mode", &bLightSleep))
    {
      config.render.framerate.max_delta_time =
        bLightSleep ?
        1 : 0;
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip ("Enable potentially unlimited framerate, but with added CPU load");
    }
  }
  ImGui::EndGroup   ();
  ImGui::SameLine   ();
  ImGui::BeginGroup ();
  {
    if (bLightSleep && (!config.render.framerate.sleepless_window))
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
  ImGui::TreePush ("");
  ImGui::BeginGroup ();

  bool bResilientFramerate =
    ReadAcquire (&_SK_NIER_RAD_ResilientFramerate) != 0;

  if (ImGui::Checkbox ( "Framerate Resilience",
                          &bResilientFramerate ))
  {
    if (bResilientFramerate) WriteRelease (
         &_SK_NIER_RAD_ResilientFramerate,
                      bResilientFramerate ? 1 
                                          : 0 );


  }
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
      ImGui::SetTooltip ("Adds minor CPU overhead for render state tracking; turn off for better perf.");
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

    if (orig_deferred != config.render.dxgi.deferred_isolation &&
        orig_deferred == false)
    {
      ImGui::TextUnformatted (ICON_FA_INFO_CIRCLE); ImGui::SameLine ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.5f, 1.0f, 0.1f, 1.0f));
      ImGui::TextUnformatted ("Game restart required for changes to apply.");
      ImGui::PopStyleColor   (1);
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

  plugin_mgr->config_fns.emplace (SK_NIER_RAD_PlugInCfg);

  SK_GetDLLConfig ()->write (
    SK_GetDLLConfig ()->get_filename ()
  );
}
