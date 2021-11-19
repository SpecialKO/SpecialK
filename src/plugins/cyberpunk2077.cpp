//
// Copyright 2020 Andon "Kaldaien" Coleman
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

#define CP2077_VERSION_NUM L"0.0.1"
#define CP2077_VERSION_STR L"Cyberpunk 2077 Plug-In v " CP2077_VERSION_NUM

//static NtReadFile_pfn NtReadFile_Original = nullptr;

#include <SpecialK/diagnostics/file.h>

bool SK_CP2077_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Cyberpunk 2077", ImGuiTreeNodeFlags_DefaultOpen))
  {
    extern std::atomic_int __SK_RenderThreadCount;

    ImGui::BulletText ("Engine is using %u Render Threads", __SK_RenderThreadCount.load ());

    static bool changed = false;

    bool spoof =
      config.render.framerate.override_num_cpus != -1;

    extern size_t SK_CPU_CountPhysicalCores (void);
    extern size_t SK_CPU_CountLogicalCores  (void);

    if (ImGui::Checkbox ("Override Render Thread Count", &spoof))
    { changed = true;

      if (config.render.framerate.override_num_cpus == -1)
      {
        if (spoof)
        {
          config.render.framerate.override_num_cpus =
            static_cast <int> (SK_CPU_CountPhysicalCores ());
        }
      }

      else if (! spoof)
      {
        config.render.framerate.override_num_cpus = -1;
      }

      SK_SaveConfig ();
    }

    if (spoof)
    {
      ImGui::SameLine ();

      if (ImGui::SliderInt ( "CPU Cores", &config.render.framerate.override_num_cpus,
                               static_cast <int> (SK_CPU_CountPhysicalCores ()),
                               static_cast <int> (SK_CPU_CountLogicalCores  ()) ) )
      { changed = true;
        SK_SaveConfig ();
      }
    }

    if (changed)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }
  }

  return true;
}


void SK_CP2077_InitPlugin (void)
{
  SK_SetPluginName (L"Special K v " SK_VERSION_STR_W L" // " CP2077_VERSION_STR);

  plugin_mgr->config_fns.emplace (SK_CP2077_PlugInCfg);

  auto val =
    SK_GetDLLConfig ()->get_section (L"Cyberpunk.2077").
                          get_value (L"FirstRun");

  if (val.empty ())
  {
    config.input.keyboard.catch_alt_f4       = false;
    config.render.framerate.sleepless_render = false;
    config.render.framerate.sleepless_window = false;

    SK_GetDLLConfig ()->write (
      SK_GetDLLConfig ()->get_filename ()
    );
  }
}
