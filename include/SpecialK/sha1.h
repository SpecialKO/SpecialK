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

#ifndef __SK__SHA1_H__
#define __SK__SHA1_H__

#include <cstdint>

#include <SpecialK/SpecialK.h>
#include <SpecialK/hash.h>

/*
   Original Code:

   SHA-1 in C
   By Steve Reid <steve@edmweb.com>
   100% Public Domain
 */

typedef struct
{
       uint32_t state  [5];
       uint32_t count  [2];
  unsigned char buffer [64];
} SHA1_CTX;

void
SHA1Transform (       uint32_t                     state  [5],
                const unsigned char                buffer [64]        );

void
SHA1Init      (       SHA1_CTX                    *context            );

void
SHA1Update    (       SHA1_CTX                    *context,
                const unsigned char               *data,
                      uint32_t                     len                );

void
SHA1Final     (       unsigned char                digest [20],
                      SHA1_CTX                    *context            );

void
SHA1          (       char                        *hash_out,
                const char                        *str,
                      unsigned int                 len,
                      SK_HashProgressCallback_pfn  callback = nullptr );

bool
SHA1_File     ( const wchar_t                     *wszFile,
                      char                        *hash_out,
                      SK_HashProgressCallback_pfn  callback = nullptr );


//
// Public Interface: These functions will be DLL Exported in the future
//
typedef uint8_t SK_SHA1_HashElement;

struct SK_SHA1_Hash
{
  SK_SHA1_HashElement hash [20];

  void   toCString (      char         *szSHA1) const;
  bool operator == (const SK_SHA1_Hash &b     ) const;
  bool operator != (const SK_SHA1_Hash &b     ) const;
};


#ifdef __cplusplus
extern "C" {
#endif

SK_SHA1_Hash                                               SK_PUBLIC_API
SK_File_GetSHA1     ( const wchar_t* wszFile,
         SK_HashProgressCallback_pfn callback = nullptr );

bool                                                       SK_PUBLIC_API
SK_File_GetSHA1StrA ( const char* szFile,
                           char*  szOut,
      SK_HashProgressCallback_pfn callback    = nullptr );

bool                                                       SK_PUBLIC_API
SK_File_GetSHA1StrW ( const wchar_t* wszFile,
                            wchar_t* wszOut,
         SK_HashProgressCallback_pfn callback = nullptr );

#ifdef __cplusplus
};
#endif

#endif /* __SK__SHA1_H__ */