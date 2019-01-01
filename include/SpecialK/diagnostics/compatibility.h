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

#ifndef __SK__COMPATIBILITY_H__
#define __SK__COMPATIBILITY_H__

enum class SK_NV_Bloat {
  None       = 0x0,
  RxCore     = 0x1,
};

typedef _Return_type_success_(return >= 0) long HRESULT;

HRESULT SK_COMPAT_FixNahimicDeadlock (void);

bool SK_COMPAT_IsSystemDllInstalled (const wchar_t* wszDll, bool* locally = nullptr);


bool SK_COMPAT_IsFrapsPresent (void);
void SK_COMPAT_UnloadFraps    (void);

  #endif /* __SK_COMPATIBILITY_H__ */