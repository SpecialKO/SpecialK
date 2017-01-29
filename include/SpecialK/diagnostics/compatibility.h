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

enum class SK_ModuleEnum {
  PreLoad    = 0x0,
  PostLoad   = 0x1,
  Checkpoint = 0x2
};

void __stdcall SK_EnumLoadedModules   (SK_ModuleEnum when = SK_ModuleEnum::PreLoad);
void __stdcall SK_InitCompatBlacklist (void);

void __stdcall SK_ReHookLoadLibrary   (void);

void __stdcall SK_LockDllLoader       (void);
void __stdcall SK_UnlockDllLoader     (void);

#if 0
#define LoadLibraryW_Original LoadLibraryW
#define LoadLibraryA_Original LoadLibraryA
#define LoadLibraryExW_Original LoadLibraryExW
#define LoadLibraryExA_Original LoadLibraryA
#endif

#endif /* __SK_COMPATIBILITY_H__ */