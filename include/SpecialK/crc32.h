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

SK_INCLUDE_START (CRC32)

#include <SpecialK/Hash.h>
#include <cstdint>

uint32_t                                                                 SK_PUBLIC_API
SK_File_GetCRC32  ( const wchar_t* wszFile,
                       SK_HashProgressCallback_pfn callback = nullptr );
uint32_t                                                                 SK_PUBLIC_API
SK_File_GetCRC32C ( const wchar_t* wszFile,
                       SK_HashProgressCallback_pfn callback = nullptr );



uint32_t
__cdecl
crc32       (uint32_t crc, const void *buf, size_t size);

// Returns the value of crc if attempting to checksum this memory throws an access violation exception
uint32_t
__cdecl
safe_crc32c (uint32_t crc, const void *buf, size_t size);

/*
    Computes CRC-32C (Castagnoli) checksum. Uses Intel's CRC32 instruction if it is available.
    Otherwise it uses a very fast software fallback.
*/
uint32_t
__cdecl
crc32c (
    uint32_t    crc,            // Initial CRC value. Typically it's 0.
                                // You can supply non-trivial initial value here.
                                // Initial value can be used to chain CRC from multiple buffers.
    const void *input,          // Data to be put through the CRC algorithm.
    size_t      length);        // Length of the data in the input buffer.


void __cdecl __crc32_init (void);

/*
	Software fallback version of CRC-32C (Castagnoli) checksum.
*/
uint32_t
__cdecl
crc32c_append_sw (uint32_t crc, const void *input, size_t length);

/*
	Hardware version of CRC-32C (Castagnoli) checksum. Will fail, if CPU does not support related instructions. Use a crc32c_append version instead of.
*/
uint32_t
__cdecl
crc32c_append_hw (uint32_t crc, const void *input, size_t length);

/*
	Checks is hardware version of CRC-32C is available.
*/
int
__cdecl
crc32c_hw_available (void);

SK_INCLUDE_END (CRC32)