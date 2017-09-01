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

enum class SK_NV_Bloat {
  None       = 0x0,
  RxCore     = 0x1,
};

void __stdcall SK_EnumLoadedModules   (SK_ModuleEnum when = SK_ModuleEnum::PreLoad);
void __stdcall SK_InitCompatBlacklist (void);
void __stdcall SK_PreInitLoadLibrary  (void); // Before we hook load library, just import it

void __stdcall SK_ReHookLoadLibrary   (void);
void __stdcall SK_UnhookLoadLibrary   (void);

void __stdcall SK_LockDllLoader       (void);
void __stdcall SK_UnlockDllLoader     (void);

#ifdef _INC_WINDOWS

typedef BOOL    (WINAPI *FreeLibrary_pfn)  (HMODULE hLibModule);
typedef HMODULE (WINAPI *LoadLibraryA_pfn) (LPCSTR  lpFileName);
typedef HMODULE (WINAPI *LoadLibraryW_pfn) (LPCWSTR lpFileName);
typedef HMODULE (WINAPI *LoadLibraryExA_pfn)
( _In_       LPCSTR  lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);
typedef HMODULE (WINAPI *LoadLibraryExW_pfn)
( _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);

typedef HMODULE (WINAPI *LoadPackagedLibrary_pfn)
( _In_       LPCWSTR lpwLibFileName,
  _Reserved_ DWORD   Reserved
);

extern FreeLibrary_pfn         FreeLibrary_Original;

extern LoadLibraryA_pfn        LoadLibraryA_Original;
extern LoadLibraryW_pfn        LoadLibraryW_Original;

extern LoadLibraryExA_pfn      LoadLibraryExA_Original;
extern LoadLibraryExW_pfn      LoadLibraryExW_Original;

extern LoadPackagedLibrary_pfn LoadPackagedLibrary_Original;

template <typename _T>
BOOL
__stdcall
SK_LoadLibrary_PinModule (const _T* pStr);

#endif
#endif /* __SK_COMPATIBILITY_H__ */