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

#if 0
#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"XInputUpgr"

struct {
  size_t start;
  size_t end;
  size_t now;
} static __xinput_patch_progress;

void
SK_XInput_PatchDialog (void)
{
  if ( ImGui::BeginPopupModal ( "XInput Version Upgrade",
                                    nullptr,
                                      ImGuiWindowFlags_AlwaysAutoResize |
                                      ImGuiWindowFlags_NoScrollbar      |
                                      ImGuiWindowFlags_NoScrollWithMouse )
     )
  {
    ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f),     " Patching game to use XInput 1.4");

    ImGui::SameLine    (); ImGui::Spacing (); ImGui::SameLine ();

    const float  ratio            = static_cast <float> (__xinput_patch_progress.now - __xinput_patch_progress.start) /
                                    static_cast <float> (__xinput_patch_progress.end - __xinput_patch_progress.start);

    snprintf ( szProgress, 127, "%.2f%% of Executable Processed (%u/%u)",
                 100.0 * ratio,  __xinput_patch_progress.now - __xinput_patch_progress.start,
                                 __xinput_patch_progress.end - __xinput_patch_progress.start );

    ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f) );
    ImGui::ProgressBar    ( ratio,
                              ImVec2 (-1, 0),
                                szProgress );
    ImGui::PopStyleColor  ();

    ImGui::SameLine    (); ImGui::Spacing (); ImGui::SameLine ();

    if (ImGui::Button ("Cancel"))
    {
      //SK_ImGui_UnconfirmedDisplayChanges = false;
      ImGui::CloseCurrentPopup ();
    }

    ImGui::EndPopup ();
  }
}

void SK_XInput_PatchExecutable (const wchar_t *wszExecutable)
{
  if (PathFileExistsW (wszExecutable))
  {
    std::wstring temp_name =
      SK_FormatStringW (LR"(%ws.xinput)", wszExecutable);

    DeleteFileW (               temp_name.c_str ());
    CopyFile    (wszExecutable, temp_name.c_str (), FALSE);

    if (PathFileExistsW (temp_name.c_str ()))
    {
      FILE *fExecutable =
        _wfopen (temp_name.c_str (), L"r+b");

      if (fExecutable != nullptr)
      {
        fseek (fExecutable, 0, SEEK_END);

        SIZE_T size =
          ftell (fExecutable);

        char *exec_bytes =
          (char *)std::malloc (size + 1);

        rewind (                     fExecutable);
        fread  (exec_bytes, size, 1, fExecutable);
        rewind (                     fExecutable);

        char *ptr = exec_bytes;
        char *end = exec_bytes + std::min (size, 512UL * 1024UL * 1024UL) - 16;

        while (ptr < end)
        {
          if ( *ptr != 'x' &&
               *ptr != 'X' )
          {   ++ptr; continue; }

          if (StrStrIA (ptr, "XInput"))
          {
            ptr += 6;

            if (StrStrIA (ptr, "9_1_0.dll")) {
                  memcpy (ptr, "1_4.dll\0\0", 9);

              patched9_1_0 = true;
            }

            else if (StrStrIA (ptr, "1_3.dll")) {
                       memcpy (ptr, "1_4.dll", 7);

              patched1_3 = true;
            }
          }

          else ptr += 6;
        }

        if (patched9_1_0 || patched1_3)
          fwrite (exec_bytes, size, 1, fExecutable);

        std::free (exec_bytes);

        fclose (fExecutable);
      }

      if (patched9_1_0)
        SK_ImGui_WarningWithTitle (L"Patched Executable: XInput9_1_0 -> XInput1_4", L"Success");
      if (patched1_3)
        SK_ImGui_WarningWithTitle (L"Patched Executable: XInput1_3 -> XInput1_4",   L"Success");

      if (! (patched9_1_0 || patched1_3))
      {
        SK_ImGui_WarningWithTitle (L"No XInput Patching Performed", L"Failure");
      }
    }
  }
}
#endif