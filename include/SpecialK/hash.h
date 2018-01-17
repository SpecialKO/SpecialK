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

#include <SpecialK/SpecialK.h>

SK_INCLUDE_START (Hash)

#include <cstdint>

using SK_HashProgressCallback_pfn =
  void (__stdcall *)(uint64_t current, uint64_t total);

enum sk_hash_algo {
  SK_NO_HASH    = 0x0,

  // CRC32 (XOR operands swapped; exaggerates small differences in data)
  SK_CRC32_KAL  = 0x1,

  //
  // CRC32C ( Hardware Algorithm:  SSE4.2                  )
  //        ( Software Algorithm:  Optimized by Mark Adler )
  //
  SK_CRC32C     = 0x2,

  // SHA1
  SK_SHA1       = 0x4,

  SK_NUM_HASHES = 3
};

uint32_t
SK_PUBLIC_API
SK_File_GetHash_32 (       sk_hash_algo                 algorithm,
                     const wchar_t                     *wszFile,
                           SK_HashProgressCallback_pfn  callback = nullptr );


SK_INCLUDE_END (Hash)