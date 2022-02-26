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
#include <SpecialK/DLL_VERSION.H>
#include <imgui/font_awesome.h>
#include <algorithm>

struct {
  float fSpeed = 1.0f;
} SK_ER_PlugIn;

void
__stdcall
SK_ER_EndFrame (void)
{
  static float* fAddr =
    (float *)((uintptr_t)SK_Debug_GetImageBaseAddr () + 0x3B4FE28); // 1.0.2: 0x3B4FE08);

  *fAddr =
    SK_ER_PlugIn.fSpeed * static_cast <float> (
       std::min (0.25,
       std::max (0.00001, static_cast <double> (SK_GetCurrentRenderBackend ().frame_delta.getDeltaTime ())  /
                          static_cast <double> (SK_QpcFreq)))
    );
}

bool
SK_ER_PlugInCfg (void)
{
  if ( ImGui::CollapsingHeader ( "Elden Ring",
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
      ImGui::SliderFloat ("Game Speed", &SK_ER_PlugIn.fSpeed, 0.8f, 1.2f, "%.3fx");
      ImGui::EndGroup    (  );

      ImGui::BulletText  ("Framerate Limit Unlock:  Active");
      ImGui::TreePop     (  );
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }

  return true;
}

void
SK_ER_InitPlugin (void)
{
  if (*((uint8_t *)SK_Debug_GetImageBaseAddr () + 0x0DFEFC0) == 0xC7)
  {
    //F3 0F11 05 DE7E5A01
    // 1.0.2.1: 0x25A7F72
    
    DWORD dwOldProt = 0x0;
    uint8_t* pNOP   = (uint8_t *)SK_Debug_GetImageBaseAddr () + 0x25A7F72;
    
    uint8_t* pTick0 = (uint8_t *)SK_Debug_GetImageBaseAddr () + 0x0DFEF87;
    uint8_t* pTick1 = (uint8_t *)SK_Debug_GetImageBaseAddr () + 0x0DFEFA3;
    uint8_t* pTick2 = (uint8_t *)SK_Debug_GetImageBaseAddr () + 0x0DFEFAF;
    uint8_t* pTick3 = (uint8_t *)SK_Debug_GetImageBaseAddr () + 0x0DFEFC0;
    uint8_t* pTick4 = (uint8_t *)SK_Debug_GetImageBaseAddr () + 0x0DFEFCD;
    uint8_t* pTick5 = (uint8_t *)SK_Debug_GetImageBaseAddr () + 0x0DFEFDD;
    
    VirtualProtect (pNOP,   8, PAGE_EXECUTE_READWRITE, &dwOldProt);
    VirtualProtect (pTick0, 7, PAGE_EXECUTE_READWRITE, &dwOldProt);
    VirtualProtect (pTick1, 7, PAGE_EXECUTE_READWRITE, &dwOldProt);
    VirtualProtect (pTick2, 7, PAGE_EXECUTE_READWRITE, &dwOldProt);
    VirtualProtect (pTick3, 7, PAGE_EXECUTE_READWRITE, &dwOldProt);
    
    memcpy (pNOP,  "\x90\x90\x90\x90\x90\x90\x90\x90",         8);
    memcpy (pTick0,"\xC7\x43\x20\x00\x00\x00\x00\x00\x00\x00", 7);
    memcpy (pTick1,"\xC7\x43\x20\x00\x00\x00\x00\x00\x00\x00", 7);
    memcpy (pTick2,"\xC7\x43\x20\x00\x00\x00\x00\x00\x00\x00", 7);
    memcpy (pTick3,"\xC7\x43\x20\x00\x00\x00\x00\x00\x00\x00", 7);
    memcpy (pTick4,"\xC7\x43\x20\x00\x00\x00\x00\x00\x00\x00", 7);
    memcpy (pTick5,"\xC7\x43\x20\x00\x00\x00\x00\x00\x00\x00", 7);
    
    //EldenRing.exe+DFEF87 - C7 43 20 00000000     - mov [rbx+20],00000000 { 0 }
    //EldenRing.exe+DFEFA3 - C7 43 20 00000000     - mov [rbx+20],00000000 { 0 }
    //EldenRing.exe+DFEFAF - C7 43 20 00000000     - mov [rbx+20],00000000 { 0 }
    
    VirtualProtect (pTick5, 7, dwOldProt, &dwOldProt);
    VirtualProtect (pTick4, 7, dwOldProt, &dwOldProt);
    VirtualProtect (pTick3, 7, dwOldProt, &dwOldProt);
    VirtualProtect (pTick2, 7, dwOldProt, &dwOldProt);
    VirtualProtect (pTick1, 7, dwOldProt, &dwOldProt);
    VirtualProtect (pTick0, 7, dwOldProt, &dwOldProt);
    VirtualProtect (pNOP,   8, dwOldProt, &dwOldProt);

    plugin_mgr->end_frame_fns.emplace (SK_ER_EndFrame );
    plugin_mgr->config_fns.emplace    (SK_ER_PlugInCfg);
  }
}