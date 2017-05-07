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
#include <SpecialK/plugin/plugin_mgr.h>
#include <string>

//Stupid Hack, rewrite me... (IN PROGRESS - see isPlugin below)
bool isArkhamKnight    = false;
bool isTalesOfZestiria = false;
bool isFallout4        = false;
bool isNieRAutomata    = false;
bool isDarkSouls3      = false;
bool isDivinityOrigSin = false;

bool isPlugin          = false;


std::wstring plugin_name = L"";

// FIXME: For the love of @#$% do not pass std::wstring objects across
//          DLL boundaries !!
void
__stdcall
SK_SetPluginName (std::wstring name)
{
  plugin_name = name;
  isPlugin    = true;
}

void
__stdcall
SKX_SetPluginName (const wchar_t* wszName)
{
  plugin_name = wszName;
  isPlugin    = true;
}

std::wstring
__stdcall
SK_GetPluginName (void)
{
  if (isPlugin)
    return plugin_name;

  return L"Special K";
}

bool
__stdcall
SK_HasPlugin (void)
{
  return isPlugin;
}