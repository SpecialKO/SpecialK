#pragma once

#include <Windows.h>
#include <SpecialK/core.h>
#include <SpecialK/resource.h>

#include <imgui/imgui.h>

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

    char* res_data = (char *)malloc (res_size + 1);
    memset (res_data, 0, res_size+1);

    strncpy (res_data, (char *)LockResource (license_ref), res_size - 1);

    std::string str (res_data);

    free (res_data);

    return str;
  }

  return std::string ("");
}

void
__stdcall
SK_ImGui_DrawEULA (LPVOID reserved)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  static float last_width  = -1;
  static float last_height = -1;

  if (last_width != io.DisplaySize.x || last_height != io.DisplaySize.y) {
    ImGui::SetNextWindowPosCenter       (ImGuiSetCond_Always);
    last_width = io.DisplaySize.x; last_height = io.DisplaySize.y;
  }

  ImGui::SetNextWindowSizeConstraints (ImVec2 (768, 256), ImVec2 ( 0.666 * io.DisplaySize.x,
                                                                   0.666 * io.DisplaySize.y ) );

  const char* szTitle = "Special K (and Plug-In) Software License Agreement";
  static bool open    = true;

  ImGui::SetNextWindowPosCenter (ImGuiSetCond_Always);
  ImGui::SetNextWindowFocus     ();

  ImGui::Begin (szTitle, &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders);

  if (ImGui::CollapsingHeader ("Overview of Products Licensed"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_OVERVIEW).c_str ());
  }

  ImGui::Separator ();

  if (ImGui::CollapsingHeader ("7zip"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_7ZIP).c_str ());
  }

  if (ImGui::CollapsingHeader ("ADL"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_ADL).c_str ());
  }

  if (ImGui::CollapsingHeader("CEGUI (D3D9/11/GL)"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_CEGUI).c_str ());
  }

  if (ImGui::CollapsingHeader ("DirectXTex (D3D11/12)"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_DXTEX).c_str ());
  }

  if (ImGui::CollapsingHeader ("FreeType 2"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_FREETYPE2).c_str ());
  }


  if (ImGui::CollapsingHeader ("GLEW (OpenGL)"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_GLEW).c_str ());
  }

  //IDR_LICENSE_GLFW_2_2    TEXTFILE  "licenses/GLFW_2_2.txt"

  if (ImGui::CollapsingHeader ("GLM v 0.9.4.5"))
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

  if (ImGui::CollapsingHeader ("NvAPI"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_NVAPI).c_str ());
  }

  if (ImGui::CollapsingHeader ("PCRE"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_PCRE).c_str ());
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_PCRE_CPP).c_str ());
  }

  if (ImGui::CollapsingHeader ("Special K"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_SPECIALK).c_str ());
  }

  if (ImGui::CollapsingHeader ("STB"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_STB).c_str ());
  }

  if (ImGui::CollapsingHeader ("Vulkan"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_VULKAN).c_str ());
  }

  if (ImGui::CollapsingHeader ("ZLIB"))
  {
    ImGui::TextWrapped ("%s", SK_GetLicenseText (IDR_LICENSE_ZLIB).c_str ());
  }

  ImGui::Separator ();

  struct show_eula_s {
    bool show;
    bool never_show_again;
  };

  if (ImGui::Button ("Close")) {
    open = false;
    ((show_eula_s *)reserved)->show = open;
  }

  ImGui::SameLine ();

  ImGui::Checkbox ("Fascinating ... never show me this again!", &((show_eula_s *)reserved)->never_show_again);

  ImGui::End ();
}