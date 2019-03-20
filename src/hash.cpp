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

struct IUnknown;
#include <Unknwnbase.h>

#include <cstdint>
#include <Windows.h>
#include <atlbase.h>

#include <SpecialK/hash.h>
#include <SpecialK/utility.h>
#include <SpecialK/crc32.h>

uint32_t
__stdcall
SK_File_GetHash_32 (       sk_hash_algo                 algorithm,
                     const wchar_t                     *wszFile,
                           SK_HashProgressCallback_pfn  callback )
{
  uint32_t _hash32 = 0UL;

  uint64_t size =
    SK_File_GetSize (wszFile);

  // Invalid file
  if (size == 0)
    return _hash32;

  switch (algorithm)
  {
    // Traditional CRC32 (but XOR operands swapped)
    case SK_CRC32_KAL:
    case SK_CRC32C:
    {
      DWORD dwFileAttribs =
        GetFileAttributes (wszFile);

      SK_AutoHandle hFile (
        CreateFile ( wszFile,
                       GENERIC_READ,
                         FILE_SHARE_READ |
                         FILE_SHARE_WRITE,
                           nullptr,
                             OPEN_EXISTING,
                               dwFileAttribs,
        nullptr)
      );

      if (hFile == INVALID_HANDLE_VALUE)
        return _hash32;

      // Read up to 16 MiB at a time.
      const uint32_t MAX_CHUNK =
        (1024UL * 1024UL * 16UL);

      const uint32_t read_size =
        size <= std::numeric_limits <uint32_t>::max () ?
          std::min (MAX_CHUNK, (uint32_t)size) :
                    MAX_CHUNK;

      CHeapPtr <uint8_t> buf;
      buf.Allocate (read_size);

      DWORD    dwReadChunk = 0UL;
      uint64_t qwReadTotal = 0ULL;

      do
      {
        ReadFile ( hFile,
                     buf,
                       read_size,
                         &dwReadChunk,
                           nullptr );

        switch (algorithm)
        {
          case SK_CRC32_KAL:
            _hash32 = crc32 ( _hash32, buf, dwReadChunk );
            break;

          case SK_CRC32C:
          {
            _hash32 = crc32c (_hash32, buf, dwReadChunk);
          } break;
        }
        qwReadTotal += dwReadChunk;

        if (callback != nullptr)
          callback (qwReadTotal, size);
      } while ( qwReadTotal < size && dwReadChunk > 0 );

      return _hash32;
    }
    break;

    // Invalid Algorithm
    default:
      return _hash32;
  }
}