//
// Copyright 2018 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, sub1ect to the following conditions:
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
#include <imgui/imgui.h>

extern volatile
  LONG SK_D3D11_DrawTrackingReqs;
extern volatile
  LONG SK_D3D11_CBufferTrackingReqs;

extern iSK_INI*             dll_ini;
extern sk::ParameterFactory g_ParameterFactory;

sk::ParameterBool*  _SK_MHW_JobParity;
bool               __SK_MHW_JobParity         = true;
sk::ParameterBool*  _SK_MHW_JobParityPhysical;
bool               __SK_MHW_JobParityPhysical = false;

sk::ParameterBool* _SK_MHW_KillAntiDebug;
bool              __SK_MHW_KillAntiDebug      = true;

sk::ParameterInt*   _SK_MHW_AlternateTonemap;

extern SK_LazyGlobal <
  concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s>
> __SK_D3D11_PixelShader_CBuffer_Overrides;

d3d11_shader_tracking_s::cbuffer_override_s* SK_MHW_CB_Override;

void
SK_MHW_PlugInInit (void)
{
#define SK_MHW_CPU_SECTION     L"MonsterHunterWorld.CPU"
#define SK_MHW_CPU_SECTION_OLD L"MonsterHuntersWorld.CPU"

  _SK_MHW_JobParity =
    _CreateConfigParameterBool ( SK_MHW_CPU_SECTION,
                                 L"LimitJobThreads",      __SK_MHW_JobParity,
                                                          L"Job Parity",
                                SK_MHW_CPU_SECTION_OLD );
  _SK_MHW_JobParityPhysical =
    _CreateConfigParameterBool ( SK_MHW_CPU_SECTION,
                                 L"LimitToPhysicalCores", __SK_MHW_JobParityPhysical,
                                                          L"Job Parity (Physical)",
                                SK_MHW_CPU_SECTION_OLD );

#define SK_MHW_HDR_SECTION     L"MonsterHunterWorld.HDR"
#define SK_MHW_HDR_SECTION_OLD L"MonsterHuntersWorld.HDR"

  __SK_D3D11_PixelShader_CBuffer_Overrides->push_back
  (
/*
 * 0: Hash,    1: CBuffer Size
 * 2: Enable?, 3: Binding Slot,
 * 4: Offset,  5: Value List Size (in bytes),
 * 6: Value List
 */
    { 0x08cc13a6, 52,
      false,      3,
      0,          4,
      { 0.0f }
    }
  );


  SK_MHW_CB_Override =
    &__SK_D3D11_PixelShader_CBuffer_Overrides->back ();

  *(reinterpret_cast <UINT *> (SK_MHW_CB_Override->Values)) =
    gsl::narrow_cast <UINT  > (-1);

  int* pCBufferOverrideVal =
    reinterpret_cast <int *> (SK_MHW_CB_Override->Values);

  _SK_MHW_AlternateTonemap =
    _CreateConfigParameterInt ( SK_MHW_HDR_SECTION,
                                L"AlternateTonemap", *pCBufferOverrideVal,
                                                     L"Tonemap Type",
                                SK_MHW_HDR_SECTION_OLD );

  if (*(reinterpret_cast <int *> (SK_MHW_CB_Override->Values)) > -1)
  {
    SK_MHW_CB_Override->Enable = true;
  }

  else
    SK_MHW_CB_Override->Enable = false;

  if (SK_MHW_CB_Override->Enable)
  {
    InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    InterlockedIncrement (&SK_D3D11_CBufferTrackingReqs);
  }



  _SK_MHW_KillAntiDebug =
    _CreateConfigParameterBool ( SK_MHW_CPU_SECTION,
                                 L"KillAntiDebugCode",    __SK_MHW_KillAntiDebug,
                                                          L"Anti-Debug Kill Switch",
                                 SK_MHW_CPU_SECTION_OLD );


#if 0
  extern bool __SK_HDR_16BitSwap;
  extern bool __SK_HDR_10BitSwap;
  if (__SK_HDR_16BitSwap)
  {
    SK_GetCurrentRenderBackend ().scanout.colorspace_override =
      DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
  }

  else if (__SK_HDR_10BitSwap)
  {
    SK_GetCurrentRenderBackend ().scanout.colorspace_override =
      DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
  }
#endif

#define SK_MHW_HUD_VS0_CRC32C  0x13ef8cc6 // General 2D HUD (after HDR was officially added)
#define SK_MHW_HUD_VS1_CRC32C  0x6f046ebc // General 2D HUD
#define SK_MHW_HUD_VS2_CRC32C  0x711c9eeb // The HUD cursor particles

  SK_D3D11_DeclHUDShader (SK_MHW_HUD_VS0_CRC32C, ID3D11VertexShader);
  SK_D3D11_DeclHUDShader (SK_MHW_HUD_VS1_CRC32C, ID3D11VertexShader);
  SK_D3D11_DeclHUDShader (SK_MHW_HUD_VS2_CRC32C, ID3D11VertexShader);


  iSK_INI* pINI =
    SK_GetDLLConfig ();

  pINI->remove_section (SK_MHW_CPU_SECTION_OLD);
  pINI->remove_section (SK_MHW_HDR_SECTION_OLD);

  pINI->write (pINI->get_filename ());
}


void
SK_MHW_ThreadStartStop (HANDLE hThread, int op = 0)
{
  static concurrency::concurrent_unordered_set <HANDLE>
    stopped_threads;

  if (op == 0)
  {
    if (! stopped_threads.count (hThread))
    {
      stopped_threads.insert (hThread);
      SuspendThread          (hThread);
    }
  }

  if (op == 1)
  {
    if (stopped_threads.count (hThread))
    {
      std::unordered_set <HANDLE> stopped_copy {
        stopped_threads.begin (), stopped_threads.end ()
      };

      stopped_threads.clear ();

      for (auto& it : stopped_threads)
      {
        if (it != hThread)
          stopped_threads.insert (hThread);

        else
          ResumeThread (hThread);
      }
    }
  }

  if (op == 2)
  {
    std::unordered_set <HANDLE> stopped_copy {
      stopped_threads.begin (), stopped_threads.end ()
    };

    stopped_threads.clear ();

    for (auto& it : stopped_copy)
    {
      TerminateThread (it, 0x0);
      //ResumeThread (it);
    }
  }
}

extern void
SK_MHW_SuspendThread (HANDLE hThread)
{
  SK_MHW_ThreadStartStop (hThread, 0);
}

void
SK_MHW_PlugIn_Shutdown (void)
{
  // Resume all stopped threads prior
  //   to shutting down
  SK_MHW_ThreadStartStop (0, 2);
}

bool
SK_MHW_PlugInCfg (void)
{
  iSK_INI* pINI =
    SK_GetDLLConfig ();

  if (ImGui::CollapsingHeader ("MONSTER HUNTER: WORLD", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    static bool parity_orig = __SK_MHW_JobParity;

    if (ImGui::Checkbox ("Limit Job Threads to number of CPU cores", &__SK_MHW_JobParity))
    {
      _SK_MHW_JobParity->store (__SK_MHW_JobParity);
      pINI->write (pINI->get_filename ());
    }

    static bool rule_orig =
      __SK_MHW_JobParityPhysical;

    if (__SK_MHW_JobParity)
    {
      ImGui::SameLine (); ImGui::Spacing ();
      ImGui::SameLine (); ImGui::Spacing ();
      ImGui::SameLine (); ImGui::Text ("Limit: ");
      ImGui::SameLine (); ImGui::Spacing ();
      ImGui::SameLine ();

      int rule = __SK_MHW_JobParityPhysical ? 1 : 0;

      bool changed = false;

      extern size_t
      SK_CPU_CountLogicalCores (void);

      static bool has_logical_processors =
        SK_CPU_CountLogicalCores () > 0;

      if (has_logical_processors)
      {
        changed |=
          ImGui::RadioButton ("Logical Cores", &rule, 0);
        ImGui::SameLine ();
      }
      else
        rule = 1;

      changed |=
        ImGui::RadioButton ("Physical Cores", &rule, 1);

      if (changed)
      {
        __SK_MHW_JobParityPhysical = (rule == 1);
        _SK_MHW_JobParityPhysical->store (__SK_MHW_JobParityPhysical);
        pINI->write (pINI->get_filename ());
      }
    }

    if ( parity_orig != __SK_MHW_JobParity ||
         rule_orig   != __SK_MHW_JobParityPhysical )
    {
      ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
      ImGui::BulletText ("Game Restart Required");
      ImGui::PopStyleColor ();
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Without this option, the game spawns 32 job threads and nobody can get that many running efficiently.");

    if (ImGui::Checkbox ("Anti-Debug Workaround", &__SK_MHW_KillAntiDebug))
    {
      _SK_MHW_KillAntiDebug->store (__SK_MHW_KillAntiDebug);
      pINI->write (pINI->get_filename ());
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Eliminate the kernel bottleneck Capcom added to prevent debugging.");

    if (ImGui::CollapsingHeader ("HDR Fix", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush  ("");

      static auto& rb =
        SK_GetCurrentRenderBackend ();

      if (rb.isHDRCapable ())
      {
        bool hdr_open =
          SK_ImGui_Widgets->hdr_control->isVisible ();

        if ( ImGui::Button ( u8"HDR Signal Control Panel" ) )
        {
          SK_ImGui_Widgets->hdr_control->setVisible (! hdr_open);

          if (! hdr_open)
          {
            SK_RunOnce (
              SK_ImGui_Warning ( L"Congratulations: You Opened a Widget -- the Hard Way!\n\n\t"
              L"Pro Tip:\tRight-click the Widget and then Assign a Toggle Keybind!" )
            );
          }
        }
      }

      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

      bool changed =
        ImGui::Checkbox ("Enable Alternate Tonemap", &SK_MHW_CB_Override->Enable);

      if (changed)
      {
        if (SK_MHW_CB_Override->Enable)
        {
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_CBufferTrackingReqs);
        }
        else
        {
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_CBufferTrackingReqs);
        }
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("You can significantly improve the washed out image by using an alternate tonemap.");
      }

      if (SK_MHW_CB_Override->Enable)
      {
        if (*(int *)(SK_MHW_CB_Override->Values) < 0)
        {
               *(int *)(SK_MHW_CB_Override->Values) = 1 +
          abs (*(int *) SK_MHW_CB_Override->Values);
        }

        ImGui::SameLine    ();
        ImGui::BeginGroup  ();
        changed |=
          ImGui::SliderInt ("Tonemap Type##SK_MHW_TONEMAP", (int *)SK_MHW_CB_Override->Values, 0, 8);
        ImGui::EndGroup    ();
      }

      if (changed)
      {
        int tonemap =
          ( SK_MHW_CB_Override->Enable ?        abs (*(int *)SK_MHW_CB_Override->Values)
                                       : (-1) - abs (*(int *)SK_MHW_CB_Override->Values) );

        _SK_MHW_AlternateTonemap->store (tonemap);
        pINI->write (pINI->get_filename ());
      }

      ImGui::PopStyleColor (3);
      ImGui::TreePop       ( );
    }

    ImGui::TreePop ();

    return true;
  }

  return false;
}