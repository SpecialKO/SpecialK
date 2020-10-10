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

#include <SpecialK/stdafx.h>


#include <SpecialK/control_panel/plugins.h>

extern iSK_INI* dll_ini;

using namespace SK::ControlPanel;


bool
SK_ImGui_SavePlugInPreference (iSK_INI* ini, bool enable, const wchar_t* import_name, const wchar_t* role, int order, const wchar_t* path)
{
  if (! ini)
    return false;

  if (! enable)
  {
    ini->remove_section (import_name);
    ini->write          (ini->get_filename ());

    return true;
  }

  if (GetFileAttributesW (path) != INVALID_FILE_ATTRIBUTES)
  {
    wchar_t wszImportRecord [4096] = { };

    swprintf ( wszImportRecord, L"[%s]\n"
#ifdef _WIN64
                                 L"Architecture=x64\n"
#else
                                 L"Architecture=Win32\n"
#endif
                                 L"Role=%s\n"
                                 L"When=%s\n"
                                 L"Filename=%s\n\n",
                                   import_name,
                                     role,
                                       order == 0 ? L"Early" :
                                                    L"PlugIn",
                                         path );

    ini->import (wszImportRecord);
    ini->write  (ini->get_filename ());

    return true;
  }

  return false;
}


void
SK_ImGui_PlugInDisclaimer (void)
{
  ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.15f, 0.95f, 0.98f));
  ImGui::TextWrapped    ("If you run into problems with a Plug-In, pressing and holding Ctrl + Shift at game startup can disable them.");
  ImGui::PopStyleColor  ();
}

bool
SK_ImGui_PlugInSelector (iSK_INI* ini, const std::string& name, const wchar_t* path, const wchar_t* import_name, bool& enable, int& order, int default_order = 1)
{
  if (! ini)
    return false;

  std::string hash_name  = name + "##PlugIn";
  std::string hash_load  = "Load Order##";
              hash_load += name;

  bool changed =
    ImGui::Checkbox (hash_name.c_str (), &enable);

  if (ImGui::IsItemHovered ())
  {
    if (GetFileAttributesW (path) == INVALID_FILE_ATTRIBUTES)
      ImGui::SetTooltip ("Please install %s to %ws", name.c_str (), path);
  }

  if (ini->contains_section (import_name))
  {
    if (ini->get_section (import_name).get_value (L"When") == L"Early")
      order = 0;
    else
      order = 1;
  }
  else
    order = default_order;

  ImGui::SameLine ();

  changed |=
    ImGui::Combo (hash_load.c_str (), &order, "Early\0Plug-In\0\0");

  if (ImGui::IsItemHovered ())
  {
    ImGui::BeginTooltip ();
    ImGui::Text         ("Plug-In Load Order is Suggested by Default.");
    ImGui::Separator    ();
    ImGui::BulletText   ("If a plug-in does not show up or the game crashes, try loading it early.");
    ImGui::BulletText   ("Early plug-ins handle rendering before Special K; ReShade will apply its effects to Special K's UI if loaded early.");
    ImGui::EndTooltip   ();
  }

  return changed;
}


bool
SK::ControlPanel::PlugIns::Draw (void)
{
  if (ImGui::CollapsingHeader ("Plug-Ins"))
  {
    ImGui::TreePush ("");

    wchar_t imp_path_reshade    [MAX_PATH + 2] = { };
    wchar_t imp_name_reshade    [64]           = { };

    wchar_t imp_path_reshade_ex [MAX_PATH + 2] = { };
    wchar_t imp_name_reshade_ex [64]           = { };

#ifdef _WIN64
    wcscat   (imp_name_reshade, L"Import.ReShade64");
    wsprintf (imp_path_reshade, LR"(%s\PlugIns\ThirdParty\ReShade\ReShade64.dll)", std::wstring (SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)").c_str ());

    wcscat   (imp_name_reshade_ex, L"Import.ReShade64_Custom");

    if (SK_IsInjected ())
    {
      wsprintf (imp_path_reshade_ex, LR"(%s\PlugIns\Unofficial\ReShade\ReShade64.dll)", std::wstring (SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)").c_str ());
    }

    else
    {
      const wchar_t* wszShimFormat =
        LR"(%s\PlugIns\Unofficial\ReShade\ReShade%u.dll)";

      wsprintf ( imp_path_reshade_ex, wszShimFormat,
                   std::wstring (SK_GetDocumentsDir        (                 ) +
                                             LR"(\My Mods\SpecialK)").c_str (  ),
                                 SK_GetBitness             (                 ) );
    }
#else
    wcscat   (imp_name_reshade, L"Import.ReShade32");
    wsprintf (imp_path_reshade, L"%s\\PlugIns\\ThirdParty\\ReShade\\ReShade32.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());

    wcscat   (imp_name_reshade_ex, L"Import.ReShade32_Custom");

    if (SK_IsInjected ())
    {
      wsprintf (imp_path_reshade_ex, L"%s\\PlugIns\\Unofficial\\ReShade\\ReShade32.dll", std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());
    }

    else
    {
      const wchar_t* wszShimFormat =
        LR"(%s\PlugIns\Unofficial\ReShade\ReShade%u.dll)";

      wsprintf ( imp_path_reshade_ex, wszShimFormat,
                   std::wstring (SK_GetDocumentsDir        (                ) +
                                             L"\\My Mods\\SpecialK").c_str (  ),
                                 SK_GetBitness             (                ) );
    }
#endif
    bool reshade_official   = dll_ini->contains_section (imp_name_reshade);
    bool reshade_unofficial = dll_ini->contains_section (imp_name_reshade_ex);

    static int order    = 0;
    static int order_ex = 1;

    bool changed = false;

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

    if (ImGui::CollapsingHeader ("Third-Party"))
    {
      ImGui::TreePush    ("");
      changed |=
          SK_ImGui_PlugInSelector (dll_ini, "ReShade (Official)", imp_path_reshade, imp_name_reshade, reshade_official, order, 1);
      ImGui::TreePop     (  );
    }
    ImGui::PopStyleColor ( 3);

    if (SK_IsInjected () || StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dxgi.dll")     ||
                            StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"d3d11.dll")    ||
                            StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"d3d9.dll")     ||
                            StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"OpenGL32.dll") ||
                            StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dinput8.dll"))
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

      const bool unofficial =
        ImGui::CollapsingHeader ("Unofficial");

      ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.247f, .95f, .98f));
      ImGui::SameLine       ();
      ImGui::Text           ("           Customized for Special K");
      ImGui::PopStyleColor  ();

      if (unofficial)
      {
        ImGui::TreePush    ("");
        changed |=
            SK_ImGui_PlugInSelector (dll_ini, "ReShade (Custom)", imp_path_reshade_ex, imp_name_reshade_ex, reshade_unofficial, order_ex, 1);
        ImGui::TreePop     (  );
      }
      ImGui::PopStyleColor ( 3);
    }

    SK_ImGui_PlugInDisclaimer ();

    if (changed)
    {
      if (reshade_unofficial)
        reshade_official = false;

      if (reshade_official)
        reshade_unofficial = false;

      SK_ImGui_SavePlugInPreference (dll_ini, reshade_official,   imp_name_reshade,    L"ThirdParty", order,    imp_path_reshade   );
      SK_ImGui_SavePlugInPreference (dll_ini, reshade_unofficial, imp_name_reshade_ex, L"Unofficial", order_ex, imp_path_reshade_ex);
    }

    ImGui::TreePop ();

    return true;
  }

  return false;
}