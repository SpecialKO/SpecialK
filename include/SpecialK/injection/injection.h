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

#ifndef __SK__INJECTION_H__
#define __SK__INJECTION_H__

#include <SpecialK/window.h>

LRESULT
CALLBACK
CBTProc (int nCode, WPARAM wParam, LPARAM lParam);

extern "C" void __stdcall SKX_InstallCBTHook (void);
extern "C" void __stdcall SKX_RemoveCBTHook  (void);
extern "C" bool __stdcall SKX_IsHookingCBT   (void);

#endif /* __SK__INJECTION_H__ */