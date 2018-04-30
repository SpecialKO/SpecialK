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

#ifndef __SK__ANSEL_H__
#define __SK__ANSEL_H__

#include <../depends/include/MinHook/MinHook.h>

extern "C"
MH_STATUS
__stdcall
SK_NvCamera_ApplyHook__AnselEnableCheck (HMODULE hModule, const char* lpProcName);

extern "C"
MH_STATUS
__stdcall
SK_AnselSDK_ApplyHook__isAnselAvailable (HMODULE hModule, const char* lpProcName);


BOOL
WINAPI
SK_NvCamera_IsActive (void);

enum SK_AnselEnablement {
  SK_ANSEL_DONTCARE         = 0,
  SK_ANSEL_FORCEAVAILABLE   = 1,
  SK_ANSEL_FORCEUNAVAILABLE = 2
};

extern SK_AnselEnablement __SK_EnableAnsel;


#endif /* __SK__ANSEL_H__ */