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
#ifndef __SK__Plugin__Manager_H__
#define __SK__Plugin__Manager_H__

#include <string>

extern bool isArkhamKnight;
extern bool isTalesOfZestiria;
extern bool isFallout4;
extern bool isNieRAutomata;
extern bool isDarkSouls3;
extern bool isDivinityOrigSin;

void
__stdcall
SK_SetPluginName (std::wstring name);

void
__stdcall
SKX_SetPluginName (const wchar_t* wszName);

std::wstring
__stdcall
SK_GetPluginName (void);

bool
__stdcall
SK_HasPlugin (void);

#endif /* __SK__Plugin__Manager_H__ */


