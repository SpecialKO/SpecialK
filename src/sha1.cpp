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

/*
Original Code: SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain
Test Vectors (from FIPS PUB 180-1)
"abc"
  A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
  84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
A million repetitions of "a"
  34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
*/

/* #define LITTLE_ENDIAN * This should be #define'd already, if true. */
/* #define SHA1HANDSOFF * Copies data before messing with it. */

#define SHA1HANDSOFF

#include <SpecialK/sha1.h>
#include <SpecialK/utility.h>

#include <cstdio>
#include <cstring>
#include <cstdint>


#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#if BYTE_ORDER == LITTLE_ENDIAN
#define blk0(i) (block->l [i] = (rol (block->l [i], 24) & 0xFF00FF00) \
                               |(rol (block->l [i],  8) & 0x00FF00FF))
#elif BYTE_ORDER == BIG_ENDIAN
#define blk0(i) block->l [i]
#else
#error "Endianness not defined!"
#endif
#define blk(i) (block->l [ i & 15] = rol (block->l [(i + 13) & 15] ^ block->l [(i + 8) & 15] \
              ^ block->l [(i + 2) & 15] ^ block->l [ i & 15],  1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z += ((w & (x  ^ y)) ^ y)      + blk0 (i) + 0x5A827999 + rol (v, 5); w = rol (w, 30);
#define R1(v,w,x,y,z,i) z += ((w & (x  ^ y)) ^ y)      + blk  (i) + 0x5A827999 + rol (v, 5); w = rol (w, 30);
#define R2(v,w,x,y,z,i) z += (  w ^ x  ^ y)            + blk  (i) + 0x6ED9EBA1 + rol (v, 5); w = rol (w, 30);
#define R3(v,w,x,y,z,i) z += (((w | x) & y) | (w & x)) + blk  (i) + 0x8F1BBCDC + rol (v, 5); w = rol (w, 30);
#define R4(v,w,x,y,z,i) z += (  w ^ x  ^ y)            + blk  (i) + 0xCA62C1D6 + rol (v, 5); w = rol (w, 30);


/* Hash a single 512-bit block. This is the core of the algorithm. */

void
SHA1Transform (
        uint32_t      state  [5],
  const unsigned char buffer [64] )
{
  uint32_t a, b, c, d, e;

  typedef union
  {
    unsigned char c [64];
    uint32_t      l [16];
  } CHAR64LONG16;

#ifdef SHA1HANDSOFF
  CHAR64LONG16 block[1];      /* use array to appear as a pointer */
  
  memcpy (block, buffer, 64);
#else
  /* The following had better never be used because it causes the
   * pointer-to-const buffer to be cast into a pointer to non-const.
   * And the result is written through.  I threw a "const" in, hoping
   * this will cause a diagnostic.
   */
  CHAR64LONG16 *block = (const CHAR64LONG16 *) buffer;
#endif
  /* Copy context->state[] to working vars */
  a = state [0];
  b = state [1];
  c = state [2];
  d = state [3];
  e = state [4];

  /* 4 rounds of 20 operations each. Loop unrolled. */
  R0 (a, b, c, d, e, 0);
  R0 (e, a, b, c, d, 1);
  R0 (d, e, a, b, c, 2);
  R0 (c, d, e, a, b, 3);
  R0 (b, c, d, e, a, 4);
  R0 (a, b, c, d, e, 5);
  R0 (e, a, b, c, d, 6);
  R0 (d, e, a, b, c, 7);
  R0 (c, d, e, a, b, 8);
  R0 (b, c, d, e, a, 9);
  R0 (a, b, c, d, e, 10);
  R0 (e, a, b, c, d, 11);
  R0 (d, e, a, b, c, 12);
  R0 (c, d, e, a, b, 13);
  R0 (b, c, d, e, a, 14);
  R0 (a, b, c, d, e, 15);
  R1 (e, a, b, c, d, 16);
  R1 (d, e, a, b, c, 17);
  R1 (c, d, e, a, b, 18);
  R1 (b, c, d, e, a, 19);
  R2 (a, b, c, d, e, 20);
  R2 (e, a, b, c, d, 21);
  R2 (d, e, a, b, c, 22);
  R2 (c, d, e, a, b, 23);
  R2 (b, c, d, e, a, 24);
  R2 (a, b, c, d, e, 25);
  R2 (e, a, b, c, d, 26);
  R2 (d, e, a, b, c, 27);
  R2 (c, d, e, a, b, 28);
  R2 (b, c, d, e, a, 29);
  R2 (a, b, c, d, e, 30);
  R2 (e, a, b, c, d, 31);
  R2 (d, e, a, b, c, 32);
  R2 (c, d, e, a, b, 33);
  R2 (b, c, d, e, a, 34);
  R2 (a, b, c, d, e, 35);
  R2 (e, a, b, c, d, 36);
  R2 (d, e, a, b, c, 37);
  R2 (c, d, e, a, b, 38);
  R2 (b, c, d, e, a, 39);
  R3 (a, b, c, d, e, 40);
  R3 (e, a, b, c, d, 41);
  R3 (d, e, a, b, c, 42);
  R3 (c, d, e, a, b, 43);
  R3 (b, c, d, e, a, 44);
  R3 (a, b, c, d, e, 45);
  R3 (e, a, b, c, d, 46);
  R3 (d, e, a, b, c, 47);
  R3 (c, d, e, a, b, 48);
  R3 (b, c, d, e, a, 49);
  R3 (a, b, c, d, e, 50);
  R3 (e, a, b, c, d, 51);
  R3 (d, e, a, b, c, 52);
  R3 (c, d, e, a, b, 53);
  R3 (b, c, d, e, a, 54);
  R3 (a, b, c, d, e, 55);
  R3 (e, a, b, c, d, 56);
  R3 (d, e, a, b, c, 57);
  R3 (c, d, e, a, b, 58);
  R3 (b, c, d, e, a, 59);
  R4 (a, b, c, d, e, 60);
  R4 (e, a, b, c, d, 61);
  R4 (d, e, a, b, c, 62);
  R4 (c, d, e, a, b, 63);
  R4 (b, c, d, e, a, 64);
  R4 (a, b, c, d, e, 65);
  R4 (e, a, b, c, d, 66);
  R4 (d, e, a, b, c, 67);
  R4 (c, d, e, a, b, 68);
  R4 (b, c, d, e, a, 69);
  R4 (a, b, c, d, e, 70);
  R4 (e, a, b, c, d, 71);
  R4 (d, e, a, b, c, 72);
  R4 (c, d, e, a, b, 73);
  R4 (b, c, d, e, a, 74);
  R4 (a, b, c, d, e, 75);
  R4 (e, a, b, c, d, 76);
  R4 (d, e, a, b, c, 77);
  R4 (c, d, e, a, b, 78);
  R4 (b, c, d, e, a, 79);

  /* Add the working vars back into context.state[] */
  state [0] += a;
  state [1] += b;
  state [2] += c;
  state [3] += d;
  state [4] += e;

  /* Wipe variables */
  a = b = c = d = e = 0;
#ifdef SHA1HANDSOFF
  memset (block, '\0', sizeof block);
#endif
}


/* SHA1Init - Initialize new context */

void
SHA1Init (SHA1_CTX *context)
{
  /* SHA1 initialization constants */
  context->state [0] = 0x67452301;
  context->state [1] = 0xEFCDAB89;
  context->state [2] = 0x98BADCFE;
  context->state [3] = 0x10325476;
  context->state [4] = 0xC3D2E1F0;
  context->count [0] = context->count [1] = 0;
}


/* Run your data through this. */

void
SHA1Update (
       SHA1_CTX      *context,
 const unsigned char *data,
       uint32_t       len )
{
  uint32_t i, j;

  j = context->count [0];

  if ((context->count [0] += len << 3) < j)
       context->count [1]++;

  context->count [1] += (len >> 29);

  j = (j  >> 3) & 63;
  if ((j + len) > 63)
  {
    memcpy (&context->buffer [j], data, (i = 64 - j));

    SHA1Transform (context->state, context->buffer);

    for (; i + 63 < len; i += 64)
    {
      SHA1Transform (context->state, &data [i]);
    }

    j = 0;
  }

  else
    i = 0;

  memcpy (&context->buffer [j], &data [i], len - i);
}


/* Add padding and return the message digest. */

void
SHA1Final (
  unsigned char  digest [20],
  SHA1_CTX      *context )
{
  unsigned      i;
  unsigned char finalcount [8];
  unsigned char c;

#if 0    /* untested "improvement" by DHR */
  /* Convert context->count to a sequence of bytes
   * in finalcount.  Second element first, but
   * big-endian order within element.
   * But we do it all backwards.
   */
  unsigned char *fcp = &finalcount[8];
  for (i = 0; i < 2; i++)
  {
      uint32_t t = context->count[i];
      int j;
      for (j = 0; j < 4; t >>= 8, j++)
          *--fcp = (unsigned char) t}
#else
  for (i = 0; i < 8; i++)
  {
    /* Endian independent */
    finalcount [i] = (unsigned char)
      ((context->count [(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255);
  }
#endif
                        c = 0200;
  SHA1Update (context, &c, 1);

  while ((context->count [0] & 504) != 448)
  {
                          c = 0000;
    SHA1Update (context, &c, 1);
  }

  /* Should cause a SHA1Transform() */
  SHA1Update (context, finalcount, 8);

  for (i = 0; i < 20; i++)
  {
    digest [i] = (unsigned char)
      ((context->state [i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
  }
  /* Wipe variables */
  memset ( context,    '\0', sizeof *context);
  memset (&finalcount, '\0', sizeof  finalcount);
}

void
SHA1 (
        char                        *__restrict hash_out,
  const char                        *__restrict str,
        unsigned int                            len,
        SK_HashProgressCallback_pfn             callback )
{
  SHA1_CTX     ctx;
  unsigned int ii;
  
  SHA1Init (&ctx);

  for (ii = 0; ii < len; ii += 1)
  {
    SHA1Update (&ctx, (const unsigned char*)str + ii, 1);

    if (callback != nullptr) callback (ii, len);
  }

  SHA1Final ((unsigned char *)hash_out, &ctx);
}

#include <atlbase.h>

bool
SHA1_File (
  const wchar_t                     *wszFile,
        char                        *hash_out,
        SK_HashProgressCallback_pfn  callback )
{
  SHA1_CTX     ctx;
  unsigned int ii;
  
  SHA1Init (&ctx);

  DWORD dwFileAttribs =
    GetFileAttributes (wszFile);

  CHandle hFile (
    CreateFile ( wszFile,
                   GENERIC_READ,
                     FILE_SHARE_READ |
                     FILE_SHARE_WRITE,
                       nullptr,
                         OPEN_EXISTING,
                           dwFileAttribs,
                             nullptr
               )
    );

  if (hFile == INVALID_HANDLE_VALUE)
    return FALSE;

  uint64_t size =
    SK_File_GetSize (wszFile);

  // Read up to 16 MiB at a time.
  const uint64_t MAX_CHUNK =
    (1024ULL * 1024ULL * 16ULL);

  const uint64_t read_size =
    size <= std::numeric_limits <uint32_t>::max () ?
      std::min (MAX_CHUNK, (uint64_t)size) :
                MAX_CHUNK;

  CHeapPtr <uint8_t> buf;
  buf.Allocate ((size_t)read_size);

  DWORD    dwReadChunk = 0UL;
  uint64_t qwReadTotal = 0ULL;

  ii = 0;

  do
  {
    ReadFile ( hFile,
                 buf,
                   (DWORD)(read_size & 0xFFFFFFFFULL),
                     &dwReadChunk,
                       nullptr );

    qwReadTotal += dwReadChunk;

    for (unsigned int ii_chunk = 0 ; ii < size && ii_chunk < dwReadChunk ; ++ii, ++ii_chunk)
    {
      SHA1Update (&ctx, (const unsigned char*)buf + ii_chunk, 1);

      if (callback != nullptr) callback (ii, size);
    }
  } while ( qwReadTotal < size && dwReadChunk > 0 );

  SHA1Final ((unsigned char *)hash_out, &ctx);

  return TRUE;
}




void
SK_SHA1_Hash::toCString (char* szSHA1) const
{
  char* pszSHA1 = szSHA1;

  for (int i = 19; i >= 0; i--)
    sprintf (pszSHA1++, "%x", (uint8_t)hash [i]);

  szSHA1 [20] = '\0';
}

bool
SK_SHA1_Hash::operator == (const SK_SHA1_Hash &b) const
{
  return (memcmp (hash, b.hash, 20) == 0);
}

bool
SK_SHA1_Hash::operator != (const SK_SHA1_Hash &b) const
{
  return (memcmp (hash, b.hash, 20) != 0);
}

std::string
SK_SHA1_MakeHashString ( const SK_SHA1_Hash* sha1_hash )
{
  char szSHA1 [21] = { };

  sha1_hash->toCString (szSHA1);

  return szSHA1;
}




SK_SHA1_Hash
SK_GetFileHash_SHA (       sk_hash_algo               /*algorithm*/,
                     const wchar_t                     *wszFile,
                           SK_HashProgressCallback_pfn  callback = nullptr )
{
  SK_SHA1_Hash out;

  SHA1_File (wszFile, (char *)out.hash, callback);

  return out;
}


extern "C"
SK_SHA1_Hash
SK_PUBLIC_API
SK_File_GetSHA1 ( const wchar_t                     *wszFile,
                        SK_HashProgressCallback_pfn  callback )
{
  return SK_GetFileHash_SHA (SK_SHA1, wszFile, callback);
}


extern "C"
bool
SK_PUBLIC_API
SK_File_GetSHA1StrA ( const char                        *szFile,
                            char                        *szOut,
                            SK_HashProgressCallback_pfn  callback )
{
  SK_SHA1_Hash sha1_hash;

  bool bRet =
    SHA1_File ( SK_UTF8ToWideChar (szFile).c_str (), (char *)sha1_hash.hash, callback );

  if (bRet)
    sha1_hash.toCString (szOut);

  return bRet;
}

extern "C"
bool
SK_PUBLIC_API
SK_File_GetSHA1StrW ( const wchar_t                     *wszFile,
                            wchar_t                     *wszOut,
                            SK_HashProgressCallback_pfn  callback )
{
  SK_SHA1_Hash sha1_hash;

  bool bRet =
    SHA1_File ( wszFile, (char *)sha1_hash.hash, callback );

  if (bRet)
  {
    char szOut [21] = { };
    sha1_hash.toCString (szOut);

    wcsncpy (wszOut, SK_UTF8ToWideChar (szOut).c_str (), 20);
  }

  return bRet;
}