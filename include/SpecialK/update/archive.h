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
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct sk_file_entry_s {
  uint32_t     fileno;
  uint64_t     filesize;
  std::wstring name;
};

using SK_7Z_DECOMP_PROGRESS_PFN = int (__stdcall *)(int current, int total);

std::vector <sk_file_entry_s>
SK_Get7ZFileContents (const wchar_t* wszArchive);

HRESULT
SK_Decompress7z ( const wchar_t*            wszArchive,
                  const wchar_t*            wszOldVersion,
                  bool                      backup,
                  SK_7Z_DECOMP_PROGRESS_PFN callback );