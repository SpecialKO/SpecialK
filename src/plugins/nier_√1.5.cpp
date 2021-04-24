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

#define RADICAL_REPLICANT_VERSION_NUM L"0.0.1"
#define RADICAL_REPLICANT_VERSION_STR L"Radical Replicant v " RADICAL_REPLICANT_VERSION_NUM

volatile LONG      _SK_NIER_RAD_InputPollingPeriod = 6;
sk::ParameterInt* __SK_NIER_RAD_InputPollingPeriod;

bool SK_NIER_RAD_PlugInCfg (void)
{
  if ( ImGui::CollapsingHeader ( "NieR Replicant ver.1.22474487139...",
                                   ImGuiTreeNodeFlags_DefaultOpen )
     )
  {
    bool changed = false;

    ImGui::BeginGroup ();

    changed |=
      ImGui::Checkbox ("Disable HID / XInput Stutter Quagmire",    &config.input.gamepad.disable_ps4_hid);
    if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Requires a full game restart to fully take effect");

    changed |=
      ImGui::Checkbox ("Enable InputThread Rescheduling", &config.render.framerate.sleepless_window);

    ImGui::EndGroup    ();
    ImGui::SameLine    ();
    ImGui::BeginGroup  ();

    int tmp =
       ReadAcquire (&_SK_NIER_RAD_InputPollingPeriod);

    if (config.render.framerate.sleepless_window)
    {
      ImGui::SameLine ();

      if (ImGui::SliderInt (ICON_FA_GAMEPAD " InputThread Polling (ms)", &tmp, 1, 16)) {
        changed = true;
        WriteRelease (&_SK_NIER_RAD_InputPollingPeriod, tmp);

        __SK_NIER_RAD_InputPollingPeriod->store (tmp);
      }

      if (ImGui::IsItemHovered ()) {
        ImGui::BeginTooltip ();
        ImGui::Text ("Game Default = 8 ms, which is typical gamepad polling latency.");
        ImGui::Separator  ();
        ImGui::BulletText ("Higher values may prevent performance problems, but with added input latency.");
        ImGui::BulletText ("Use SK's gamepad latency tester to see where your gamepad stacks up...");
        ImGui::EndTooltip ();
      }

      static SK::Framerate::Stats gamepad_stats;

      static bool   init       = false;
      static HANDLE hStartStop =
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
      }
  //high_max = std::max (gamepad_stats.calcMax (), high_max);

      if (high_min < 250.0)
        ImGui::Text     ( "Minimum Latency: %4.2f ms", high_min );
    }
    ImGui::EndGroup ();

    if (changed)
    {
      SK_GetDLLConfig ()->write (
        SK_GetDLLConfig ()->get_filename ()
      );
    }
  }

  return true;
}


void SK_NIER_RAD_InitPlugin (void)
{
  SK_SetPluginName (L"Special K v " SK_VERSION_STR_W L" // " RADICAL_REPLICANT_VERSION_STR);

  __SK_NIER_RAD_InputPollingPeriod =
    _CreateConfigParameterInt ( L"Radical.Replicant",
                                L"InputThreadPollingMs", (int &)_SK_NIER_RAD_InputPollingPeriod,
                                                          L"Input Polling" );

  if (! __SK_NIER_RAD_InputPollingPeriod->load  ((int &)_SK_NIER_RAD_InputPollingPeriod))
        __SK_NIER_RAD_InputPollingPeriod->store (6);

  plugin_mgr->config_fns.emplace (SK_NIER_RAD_PlugInCfg);

  SK_GetDLLConfig ()->write (
    SK_GetDLLConfig ()->get_filename ()
  );
}
