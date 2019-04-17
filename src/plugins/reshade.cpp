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
    gsl::narrow_cast <HMODULE> (INVALID_HANDLE_VALUE);

  static bool tried_once = false;

  if (! tried_once)
  {
    tried_once = true;

    for (int i = 0; i < SK_MAX_IMPORTS; i++)
    {
      if (imports [i].hLibrary != nullptr)
      {
        if (StrStrIW (imports [i].filename->get_value_ref ().c_str (), L"ReShade"))
        {
          typedef HMODULE (__stdcall *SK_SHIM_GetReShade_pfn)(void);

          SK_SHIM_GetReShade_pfn SK_SHIM_GetReShade =
            (SK_SHIM_GetReShade_pfn)GetProcAddress (imports [i].hLibrary, "SK_SHIM_GetReShade");

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
            imports [i].hLibrary;

          break;
        }
      }
    }
  }

  return
    hModReShade;
};