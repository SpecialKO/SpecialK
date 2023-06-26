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
#include <SpecialK/plugin/reshade.h>

HMODULE
__stdcall
SK_ReShade_GetDLL (void)
{
  static HMODULE hModReShade =
    sk::narrow_cast <HMODULE> (nullptr);

  static bool tried_once = false;

  if (! tried_once)
  {
    tried_once = true;

    for (int i = 0; i < SK_MAX_IMPORTS; i++)
    {
      auto& import =
        imports->imports [i];

      if (import.hLibrary != nullptr)
      {
        if (StrStrIW (import.filename->get_value_ref ().c_str (), L"ReShade"))
        {
          typedef HMODULE (__stdcall *SK_SHIM_GetReShade_pfn)(void);

          SK_SHIM_GetReShade_pfn SK_SHIM_GetReShade =
            (SK_SHIM_GetReShade_pfn)SK_GetProcAddress (import.hLibrary, "SK_SHIM_GetReShade");

          if (SK_SHIM_GetReShade != nullptr)
          {
            HMODULE hModReal =
              SK_SHIM_GetReShade ();

            if (hModReal)
            {
              hModReShade =
                hModReal;

              break;
            }
          }

          hModReShade =
            import.hLibrary;

          break;
        }
      }
    }
  }

  return
    hModReShade;
};

void
SK_ReShade_LoadIfPresent (void)
{
  const wchar_t *wszDLL =
    SK_RunLHIfBitness (64, L"ReShade64.dll",
                           L"ReShade32.dll");

  if (PathFileExistsW (wszDLL))
  {
    if (! PathFileExistsW (L"ReShade.ini"))
    {
      FILE *fINI =
        fopen ("ReShade.ini", "w+");

      if (fINI != nullptr)
      {
        fputs (R"(
[GENERAL]
EffectSearchPaths=.\,.\reshade-shaders\Shaders
TextureSearchPaths=.\,.\reshade-shaders\Textures
PresetPath=.\ReShadePreset.ini

[OVERLAY]
NoFontScaling=1

[STYLE]
EditorFont=ProggyClean.ttf
EditorFontSize=13
EditorStyleIndex=0
Font=ProggyClean.ttf
FontSize=13
StyleIndex=2)", fINI);
        fclose (fINI);
      }
    }

    LoadLibraryW (wszDLL);
  }
}