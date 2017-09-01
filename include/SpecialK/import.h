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

#ifndef __SK__IMPORT_H__
#define __SK__IMPORT_H__

#include <SpecialK/parameter.h>

#define SK_MAX_IMPORTS 8

extern const std::wstring SK_IMPORT_EARLY;
extern const std::wstring SK_IMPORT_LATE;
extern const std::wstring SK_IMPORT_LAZY;

extern const std::wstring SK_IMPORT_ROLE_DXGI;
extern const std::wstring SK_IMPORT_ROLE_D3D11;

extern const std::wstring SK_IMPORT_ARCH_X64;
extern const std::wstring SK_IMPORT_ARCH_WIN32;

struct import_s
{
  // Parameters populated after load
  HMODULE               hLibrary     = nullptr;
  std::wstring          product_desc = L"";

  // User-defined parameters
  std::wstring          name         = L"";
  sk::ParameterStringW* filename     = nullptr;
  sk::ParameterStringW* when         = nullptr; // 0 = Early,  1 = Late,  2 = Lazy
  sk::ParameterStringW* role         = nullptr; // 0 = dxgi,   1 = d3d11
  sk::ParameterStringW* architecture = nullptr; // 0 = 64-bit, 1 = 32-bit
  sk::ParameterStringW* blacklist    = nullptr;
};


extern import_s  imports          [SK_MAX_IMPORTS];
extern import_s  host_executable;
extern import_s* dgvoodoo_d3d8;
extern import_s* dgvoodoo_ddraw;
extern import_s* dgvoodoo_d3dimm;

void SK_LoadEarlyImports64 (void);
void SK_LoadLateImports64  (void);
void SK_LoadLazyImports64  (void);
void SK_LoadPlugIns64      (void);

void SK_LoadEarlyImports32 (void);
void SK_LoadLateImports32  (void);
void SK_LoadLazyImports32  (void);
void SK_LoadPlugIns32      (void);

void SK_UnloadImports (void);

#endif /* __SK__IMPORT_H__ */