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

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include <Windows.h>
#include <SpecialK/core.h>
#include <SpecialK/resource.h>
#include <SpecialK/render_backend.h>
#include <SpecialK/config.h>

#include <imgui/imgui.h>

extern const wchar_t* __stdcall SK_GetBackend (void);

std::string
SK_GetLicenseText (SHORT id)
{
  HRSRC res;

  // NOTE: providing g_hInstance is important, NULL might not work
  res = FindResource ( SK_GetDLL (), MAKEINTRESOURCE (id), L"WAVE" );

  if (res)
  {
    DWORD res_size = SizeofResource ( SK_GetDLL (), res );

    HGLOBAL license_ref =
      LoadResource ( SK_GetDLL (), res );

    // There's no forseeable reason this would be NULL, but the Application Verifier won't shutup about it.
    if (! license_ref) return std::string ("");

    char* res_data =
      (char *)malloc (res_size + 1);

    if (res_data != nullptr)
    {
      ZeroMemory (res_data, res_size + 1);

      const char* const locked = (char *)LockResource (license_ref);

      if (locked != nullptr)
        strncat (res_data, locked, res_size);

      std::string str (res_data);

      free (res_data);

      return str;
    }
  }

  return std::string ("");
}

// PlugIns may hook this to insert additional EULA terms
__declspec (noinline)
void
__stdcall
SK_ImGui_DrawEULA_PlugIn (LPVOID reserved)
{
  UNREFERENCED_PARAMETER (reserved);

  // Need a minimal function body to avoid compiler optimization
  ImGuiIO& io =
    ImGui::GetIO ();

  DBG_UNREFERENCED_LOCAL_VARIABLE (io);

  return;
}

extern std::wstring
__stdcall
SK_GetPluginName (void);

void
__stdcall
SK_ImGui_DrawEULA (LPVOID reserved)
{
  extern uint32_t __stdcall SK_Steam_PiratesAhoy (void);
  extern uint32_t __stdcall SK_SteamAPI_AppID    (void);

  ImGuiIO& io =
    ImGui::GetIO ();

  struct show_eula_s {
    bool show;
    bool never_show_again;
  };

  static float last_width  = -1;
  static float last_height = -1;

  if (last_width != io.DisplaySize.x || last_height != io.DisplaySize.y)
  {
    ImGui::SetNextWindowPosCenter       (ImGuiSetCond_Always);
    last_width = io.DisplaySize.x; last_height = io.DisplaySize.y;
  }

  ImGui::SetNextWindowSizeConstraints (ImVec2 (780.0f, 325.0f), ImVec2 ( 0.925f * io.DisplaySize.x,
                                                                         0.925f * io.DisplaySize.y ) );

  std::wstring plugin = SK_GetPluginName ();

         char szTitle [256] = { };
  static bool open          = true;

  sprintf (szTitle, "%ws Software License Agreement", plugin.c_str ());

  if (((show_eula_s *)reserved)->show)
    ImGui::OpenPopup (szTitle);

  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;


  ImGui::SetNextWindowPosCenter (ImGuiSetCond_Always);
  ImGui::SetNextWindowFocus     ();

  if (ImGui::BeginPopupModal (szTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders))
  {
    if (io.DisplaySize.x < 1024.0f || io.DisplaySize.y < 720.0f)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, 0.6f, 0.2f, 1.0f));
      ImGui::Bullet   ();
      ImGui::SameLine ();
      ImGui::TextWrapped (
           "This software only runs at resolutions >= 1024x768 or 1280x720, please uninstall the software or use a higher resolution than (%lux%lu).",
             (int)io.DisplaySize.x, (int)io.DisplaySize.y
      );
      ImGui::PopStyleColor ();
      goto END_POPUP;
    }

    bool pirate = ( SK_SteamAPI_AppID    () != 0 && 
                    SK_Steam_PiratesAhoy () != 0x0 );


    ImGui::BeginGroup ();

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.9f, 0.1f, 1.0f));
    ImGui::Bullet   ();
    ImGui::SameLine ();
    ImGui::TextWrapped (
         "Use of this software is granted on the condition that any products being modified have been licensed to you under the "
         "terms and conditions set forth by their respective copyright holders.\n"
    );
    ImGui::PopStyleColor ();

    ImGui::Separator ();
    ImGui::EndGroup  ();


    ImGui::BeginChild ("EULA_Body",  ImVec2 (0.0f, font_size_multiline * 14), false);
    ImGui::BeginChild ("EULA_Inset", ImVec2 (0.0f, 0.0f),                     false, ImGuiWindowFlags_NavFlattened);
    ImGui::BeginGroup ();

    if (ImGui::CollapsingHeader (pirate ? "Overview of Products Unlicensed" : 
                                          "Overview of Products Licensed"))
    {
      ImGui::PushFont (ImGui::GetIO ().Fonts->Fonts [1]); // Fixed-width font
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_OVERVIEW).c_str ());
      ImGui::PopFont  ();
    }

    ImGui::Separator  ();

    SK_ImGui_DrawEULA_PlugIn (reserved);

    if (ImGui::CollapsingHeader ("7-Zip"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_7ZIP).c_str ());
    }

    if (config.apis.ADL.enable && ImGui::CollapsingHeader ("ADL"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_ADL).c_str ());
    }

    if (config.cegui.enable && ImGui::CollapsingHeader ("CEGUI (D3D9/11/GL)"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_CEGUI).c_str ());
    }

    if ( ((int)SK_GetCurrentRenderBackend ().api &  (int)SK_RenderAPI::D3D11 ||
               SK_GetCurrentRenderBackend ().api ==      SK_RenderAPI::D3D12 ) &&
         ImGui::CollapsingHeader ("DirectXTex (D3D11/12)")
       )
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_DXTEX).c_str ());
    }

    if (config.cegui.enable && ImGui::CollapsingHeader ("FreeType 2"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_FREETYPE2).c_str ());
    }


    if ( config.cegui.enable &&  SK_GetCurrentRenderBackend ().api == SK_RenderAPI::OpenGL &&
           ImGui::CollapsingHeader ("GLEW (OpenGL)")
       )
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_GLEW).c_str ());
    }

    //IDR_LICENSE_GLFW_2_2    TEXTFILE  "licenses/GLFW_2_2.txt"

    if (config.cegui.enable && ImGui::CollapsingHeader ("GLM v 0.9.4.5"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_GLM_0_9_4_5).c_str ());
    }

    if (ImGui::CollapsingHeader ("ImGui"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_IMGUI).c_str ());
    }

    //IDR_LICENSE_MESA_7_0    TEXTFILE  "licenses/Mesa_7_0.txt"

    if (ImGui::CollapsingHeader ("MinHook"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_MINHOOK).c_str ());
    }

    if (config.apis.NvAPI.enable && ImGui::CollapsingHeader ("NvAPI"))
    {
      ImGui::PushFont    (ImGui::GetIO ().Fonts->Fonts [1]); // Fixed-width font
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_NVAPI).c_str ());
      ImGui::PopFont     ();
    }

    if (config.cegui.enable && ImGui::CollapsingHeader ("PCRE"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_PCRE).c_str ());
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_PCRE_CPP).c_str ());
    }

    if (ImGui::CollapsingHeader ("Special K"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_SPECIALK).c_str ());
    }

    if (config.cegui.enable && ImGui::CollapsingHeader ("STB"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_STB).c_str ());
    }

    
    if ( SK_GetCurrentRenderBackend ().api == SK_RenderAPI::Vulkan &&
           ImGui::CollapsingHeader ("Vulkan")
       )
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_VULKAN).c_str ());
    }

    if (config.cegui.enable && ImGui::CollapsingHeader ("ZLIB"))
    {
      ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_ZLIB).c_str ());
    }

    ImGui::EndGroup ();
    ImGui::EndChild (); // EULA_Body
    ImGui::EndChild (); // EULA_Body

    ImGui::Separator  ();
    ImGui::BeginGroup ();

    ImGui::Columns  (2, "", false);
    ImGui::TreePush (   "");

    if (ImGui::Button (" Decline "))
    {
      ExitProcess (0x00);
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Bullet       ();                                              ImGui::SameLine ();
      ImGui::TextColored  (ImVec4 (1.0f, 1.0f, 0.0f, 1.0f), "WARNING:  "); ImGui::SameLine ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.9f, 0.9f, 1.0f), "Game will exit!");
      ImGui::EndTooltip   ();
    }

    ImGui::TreePop    ();
    ImGui::NextColumn ();

    if (! pirate)
    {
      ImGui::Checkbox ("I agree ... never show me this again!", &((show_eula_s *)reserved)->never_show_again);
    }

    ImGui::SameLine ();

    if (ImGui::Button (" Accept ") && (! pirate))
    {
      ImGui::CloseCurrentPopup ();

      open = false;
      ((show_eula_s *)reserved)->show = open;

      config.imgui.show_eula = ! ((show_eula_s *)reserved)->never_show_again;

      const wchar_t* config_name = SK_GetBackend ();

      if (SK_IsInjected ())
        config_name = L"SpecialK";

      SK_SaveConfig (config_name);
    }

    if (pirate && ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();

      ImGui::TextColored (ImColor (255,255,255), "Read the Yellow Text");
      ImGui::Separator   ();
      ImGui::BulletText  ("Please use the other button, you already broke the terms.");
      ImGui::BulletText  ("You may be able to find a modified version with these terms removed");
      ImGui::TreePush    ("");
      ImGui::TextWrapped ("The authors listed above are not responsible for said modified version and will not "
                          "provide you support no matter how much you make life difficult for them.");
      ImGui::TreePop     ();

      ImGui::EndTooltip  ();
    }

    ImGui::EndGroup ();

END_POPUP:
    ImGui::EndPopup ();
  }
}