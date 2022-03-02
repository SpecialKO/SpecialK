//
// Copyright 2022 Andon "Kaldaien" Coleman
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
#include <SpecialK/render/d3d12/d3d12_interfaces.h>
#include <SpecialK/utility.h>
#include <imgui/font_awesome.h>
#include <algorithm>

struct {
  float fSpeed            = 1.0f;
  bool  bFixPrioInversion = true;
  bool  bUncapFramerate   = true;

  struct {
    sk::ParameterFloat* game_speed         = nullptr;
    sk::ParameterBool*  fix_prio_inversion = nullptr;
    sk::ParameterBool*  uncap_framerate    = nullptr;
  } ini;
} SK_ER_PlugIn;

struct code_patch_s {
  void* pAddr;

  struct executable_code_s {
    std::vector <uint8_t> inst_bytes;
  } original, replacement;
  
  void apply (executable_code_s *pExec);
};



code_patch_s clock_tick0
{ .pAddr       = (void *)0xDFF397,//0x0DFEF87,
  .original    = std::vector <uint8_t> {0   ,0   ,0,   0,0,0,0},
  .replacement = std::vector <uint8_t> {0xC7,0x43,0x20,0,0,0,0}};

code_patch_s clock_tick1
{ .pAddr       = (void *)0xDFF3B3,//0x0DFEFA3,
  .original    = std::vector <uint8_t> {0   ,0   ,0,   0,0,0,0},
  .replacement = std::vector <uint8_t> {0xC7,0x43,0x20,0,0,0,0}};

code_patch_s clock_tick2
{ .pAddr       = (void *)0xDFF3BF,//0x0DFEFAF,
  .original    = std::vector <uint8_t> {0   ,0   ,0,   0,0,0,0},
  .replacement = std::vector <uint8_t> {0xC7,0x43,0x20,0,0,0,0}};

code_patch_s clock_tick3
{ .pAddr       = (void *)0xDFF3D0,//0x0DFEFC0,
  .original    = std::vector <uint8_t> {0   ,0   ,0,   0,0,0,0},
  .replacement = std::vector <uint8_t> {0xC7,0x43,0x20,0,0,0,0}};

code_patch_s clock_tick4
{ .pAddr       = (void *)0xDFF3DD,//0x0DFEFCD,
  .original    = std::vector <uint8_t> {0   ,0   ,0,   0,0,0,0},
  .replacement = std::vector <uint8_t> {0xC7,0x43,0x20,0,0,0,0}};

code_patch_s clock_tick5
{ .pAddr       = (void *)0xDFF3ED,//0DFEFDD,
  .original    = std::vector <uint8_t> {0   ,0   ,0,   0,0,0,0},
  .replacement = std::vector <uint8_t> {0xC7,0x43,0x20,0,0,0,0}};

static auto
SK_VirtualProtect =
[]( LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  flNewProtect )
{
  DWORD dwRet = 0x0;

  VirtualProtect (
    lpAddress,     dwSize,
    flNewProtect, &dwRet);

  return dwRet;
};

void
code_patch_s::apply (code_patch_s::executable_code_s *pExec)
{
  DWORD dwOldProtect =
    SK_VirtualProtect (
          pAddr, pExec->inst_bytes.size (), PAGE_EXECUTE_READWRITE);
  memcpy (pAddr, pExec->inst_bytes.data (),
                 pExec->inst_bytes.size ());
    SK_VirtualProtect (
          pAddr, pExec->inst_bytes.size (), dwOldProtect);
}

void
__stdcall
SK_ER_EndFrame (void)
{
  if (SK_ER_PlugIn.bFixPrioInversion)
  {
    if (GetThreadPriority (GetCurrentThread ()) == THREAD_PRIORITY_BELOW_NORMAL)
        SetThreadPriority (GetCurrentThread (),    THREAD_PRIORITY_LOWEST);
  }

  static float* fAddr =
    (float *)((uintptr_t)SK_Debug_GetImageBaseAddr () + 0x3B4FE38);//0x3B4FE28); // 1.0.2: 0x3B4FE08);

  *fAddr =
    SK_ER_PlugIn.fSpeed * static_cast <float> (
       std::min (0.5000,
       std::max (0.0001, static_cast <double> (SK_GetCurrentRenderBackend ().frame_delta.getDeltaTime ())  /
                         static_cast <double> (SK_QpcFreq)))
    );
}

bool
SK_ER_PlugInCfg (void)
{
  static
    std::string utf8VersionString =
        SK_WideCharToUTF8 (
          SK_GetDLLVersionStr (
             SK_GetHostApp () ) ) +
              "###EldenRingHeader\0";

  if ( ImGui::CollapsingHeader ( utf8VersionString.data (),
                                   ImGuiTreeNodeFlags_DefaultOpen )
     )
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    if ( ImGui::CollapsingHeader (
           ICON_FA_TACHOMETER_ALT "\tPerformance Settings",
                          ImGuiTreeNodeFlags_DefaultOpen )
       )
    {
      ImGui::TreePush    ("");
      ImGui::BeginGroup  (  );
  if (ImGui::Checkbox    ("Fix Render Thread Priority Inversion", &SK_ER_PlugIn.bFixPrioInversion))
      {
        if (! SK_ER_PlugIn.bFixPrioInversion)
          SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL); // Game default
        else
          SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_LOWEST);       // Sensible default

        SK_ER_PlugIn.ini.game_speed->store (SK_ER_PlugIn.bFixPrioInversion);
      }
      ImGui::SameLine    (  );
  if (ImGui::SliderFloat ("Game Speed",                           &SK_ER_PlugIn.fSpeed, 0.75f, 1.5f, "%.3fx"))
      {
        SK_ER_PlugIn.ini.game_speed->store (SK_ER_PlugIn.fSpeed);
      }
      ImGui::EndGroup    (  );

      int                             sel = SK_ER_PlugIn.bUncapFramerate ? 0 : 1;
  if (ImGui::Combo ("Framerate Cap", &sel, "Unlocked\0Normal\0\0"))
      {
        SK_ER_PlugIn.bUncapFramerate = (sel == 0);

        static auto patches =
          { &clock_tick0, &clock_tick1, &clock_tick2,
            &clock_tick3, &clock_tick4, &clock_tick5 };

        for (auto patch : patches)
        {
          if (SK_ER_PlugIn.bUncapFramerate) patch->apply (&patch->replacement);
          else                              patch->apply (&patch->original);
        }

        SK_ER_PlugIn.ini.uncap_framerate->store (SK_ER_PlugIn.bUncapFramerate);
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    ();
        ImGui::TextUnformatted ("For Best Unlocked Results");
        ImGui::Separator       ();
        ImGui::BulletText      ("Use Borderless Window Mode");
        ImGui::BulletText      ("Configure Refresh Rate using SK's Display Menu");
        ImGui::BulletText      ("Right-Click Framerate Limit to use Latent Sync (VSYNC Off Mode)");
        ImGui::EndTooltip      ();
      }

      ImGui::TreePop     (  );
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }

  return true;
}

void
SK_ER_InitConfig (void)
{
  SK_ER_PlugIn.ini.game_speed =
    _CreateConfigParameterFloat ( L"EldenRing.PlugIn",
                                  L"GameSpeed", SK_ER_PlugIn.fSpeed,
                                                       L"Game Speed" );

  if (! SK_ER_PlugIn.ini.game_speed->load  (SK_ER_PlugIn.fSpeed))
        SK_ER_PlugIn.ini.game_speed->store (1.0f);

  SK_ER_PlugIn.ini.fix_prio_inversion =
    _CreateConfigParameterBool ( L"EldenRing.PlugIn",
                                 L"FixPrioInversion", SK_ER_PlugIn.bFixPrioInversion,
                                                                     L"Priority Fix" );

  if (! SK_ER_PlugIn.ini.fix_prio_inversion->load  (SK_ER_PlugIn.bFixPrioInversion))
        SK_ER_PlugIn.ini.fix_prio_inversion->store (true);

  SK_ER_PlugIn.ini.uncap_framerate =
    _CreateConfigParameterBool ( L"EldenRing.PlugIn",
                                 L"UncapFramerate", SK_ER_PlugIn.bUncapFramerate,
                                                          L"Remove 60 FPS Limit" );

  if (! SK_ER_PlugIn.ini.uncap_framerate->load  (SK_ER_PlugIn.bUncapFramerate))
        SK_ER_PlugIn.ini.uncap_framerate->store (true);
}

void
SK_ER_InitPlugin (void)
{
  SK_ER_InitConfig ();

  __try
  {
    if (*((uint8_t *)SK_Debug_GetImageBaseAddr () + 0xDFF397) == 0xC7)
    {
      //F3 0F11 05 DE7E5A01
      // 1.0.2.1: 0x25A7F72
      
      DWORD dwOldProt = 0x0;
      uint8_t* pNOP   = (uint8_t *)SK_Debug_GetImageBaseAddr () + 0x25A8412;//0x25A7F72;

      //EldenRing.exe+DFEF87 - C7 43 20 00000000     - mov [rbx+20],00000000 { 0 }
      //EldenRing.exe+DFEFA3 - C7 43 20 00000000     - mov [rbx+20],00000000 { 0 }
      //EldenRing.exe+DFEFAF - C7 43 20 00000000     - mov [rbx+20],00000000 { 0 }

      for ( auto *patch : { &clock_tick0, &clock_tick1, &clock_tick2,
                            &clock_tick3, &clock_tick4, &clock_tick5 } )
      {
        (uintptr_t&)patch->pAddr +=
          (uintptr_t)SK_Debug_GetImageBaseAddr ();

        VirtualProtect ( patch->pAddr, 7, PAGE_EXECUTE_READWRITE, &dwOldProt );
        memcpy         ( patch->original.inst_bytes.data (),
                         patch->pAddr, 7 );
        VirtualProtect ( patch->pAddr, 7, dwOldProt,              &dwOldProt );

        if (SK_ER_PlugIn.bUncapFramerate)
        {
          patch->apply (&patch->replacement);
        }
      }

      // Disable the code that writes delta time every frame
      VirtualProtect (pNOP,   8, PAGE_EXECUTE_READWRITE, &dwOldProt);
      memcpy (        pNOP,  "\x90\x90\x90\x90\x90\x90\x90\x90",  8);
      VirtualProtect (pNOP,   8,              dwOldProt, &dwOldProt);

      plugin_mgr->end_frame_fns.emplace (SK_ER_EndFrame );
      plugin_mgr->config_fns.emplace    (SK_ER_PlugInCfg);

      if (SK_ER_PlugIn.bFixPrioInversion)
        SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_LOWEST);

      return;
    }
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                    EXCEPTION_EXECUTE_HANDLER  :
                                    EXCEPTION_CONTINUE_SEARCH )
  {
    // Warning comes after this
  }

  SK_ImGui_Warning (
    L"This version of Elden Ring is not Compatible with Special K"
  );
}




// 32-bit Launcher Bypass Code
void
SK_SEH_LaunchEldenRing (void)
{
  __try {
    STARTUPINFOW        sinfo = { };
    PROCESS_INFORMATION pinfo = { };

    sinfo.cb          = sizeof (STARTUPINFOW);
    sinfo.wShowWindow = SW_SHOWNORMAL;
    sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;

    // EAC Launcher has SteamNoOverlayUIDrawing set to 1, we don't want
    //   to inherit that (!!)
    SetEnvironmentVariable (L"SteamNoOverlayUIDrawing", L"0");

    CreateProcess ( L"eldenring.exe", nullptr, nullptr, nullptr,
                    TRUE,    CREATE_SUSPENDED, nullptr, SK_GetHostPath (),
                    &sinfo,  &pinfo );

    if (pinfo.hProcess != 0)
    {
      // Save config prior to comitting suicide
      SK_SelfDestruct ();

      ResumeThread     (pinfo.hThread);
      SK_CloseHandle   (pinfo.hThread);
      SK_CloseHandle   (pinfo.hProcess);
      WaitForInputIdle (pinfo.hProcess, 5UL);

      SK_TerminateProcess (0x00);
    }
  }

  __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}
