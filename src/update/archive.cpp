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

#include <Windows.h>

#include "../log.h"
#include "../utility.h"

#include <cstdint>

#include <lzma/7z.h>
#include <lzma/7zAlloc.h>
#include <lzma/7zBuf.h>
#include <lzma/7zCrc.h>
#include <lzma/7zFile.h>
#include <lzma/7zVersion.h>

static ISzAlloc g_Alloc = { SzAlloc, SzFree };

HRESULT
SK_Decompress7z (const wchar_t* wszArchive, const wchar_t* wszOldVersion)
{
  CrcGenerateTable ();

  CFileInStream arc_stream;
  CLookToRead   look_stream;

  FileInStream_CreateVTable (&arc_stream);
  LookToRead_CreateVTable   (&look_stream, False);

  look_stream.realStream = &arc_stream.s;
  LookToRead_Init         (&look_stream);

  ISzAlloc      thread_alloc;
  ISzAlloc      thread_tmp_alloc;

  thread_alloc.Alloc     = SzAlloc;
  thread_alloc.Free      = SzFree;

  thread_tmp_alloc.Alloc = SzAllocTemp;
  thread_tmp_alloc.Free  = SzFreeTemp;

  CSzArEx      arc;

  if (InFile_OpenW (&arc_stream.file, wszArchive))
  {
    dll_log.Log ( L"[AutoUpdate]  ** Cannot open archive file: %s",
                    wszArchive );
    return E_FAIL;
  }

  SzArEx_Init (&arc);

  if ( SzArEx_Open ( &arc,
                       &look_stream.s,
                         &thread_alloc,
                           &thread_tmp_alloc ) == SZ_OK )
  {
    uint32_t i;

    wchar_t wszEntry [MAX_PATH];

    for (i = 0; i < arc.NumFiles; i++)
    {
      if (SzArEx_IsDir (&arc, i))
        continue;

      SzArEx_GetFileNameUtf16 (&arc, i, (UInt16 *)wszEntry);

      // Truncate to 32-bits --> there's no way in hell a texture will ever be >= 2 GiB
      uint32_t fileSize = SzArEx_GetFileSize (&arc, i);

      uint8_t* pData =
        (uint8_t *)malloc (fileSize * 20);

      uint32_t block_idx     = 0xFFFFFFFF;
      Byte*    out           = (Byte *)pData;
      size_t   out_len       =         fileSize * 20;
      size_t   offset        = 0;
      size_t   decomp_size   = 0;

      wchar_t* pwszEntry = wszEntry;

      if (*wszEntry == L'\\')
        pwszEntry++;

      if ( SZ_OK !=
             SzArEx_Extract ( &arc,          &look_stream.s, i,
                              &block_idx,    &out,           &out_len,
                              &offset,       &decomp_size,
                              &thread_alloc, &thread_tmp_alloc ) ) {
        dll_log.Log ( L"[AutoUpdate] Failed to extract 7-zip file ('%s')",
                        pwszEntry );
        free (pData);
        return E_FAIL;
      }

      extern const wchar_t*
      __stdcall
      SK_GetRootPath (void);

      wchar_t wszDestPath [MAX_PATH] = { L'\0' };
      wcscpy (wszDestPath, SK_GetRootPath ());

      lstrcatW (wszDestPath, pwszEntry);

      if (GetFileAttributes (wszDestPath) != INVALID_FILE_ATTRIBUTES) {
        wchar_t wszMovePath [MAX_PATH] = { L'\0' };

        wcscpy (wszMovePath, SK_GetRootPath ());

        lstrcatW (wszMovePath, L"Version\\");

        if (wszOldVersion != nullptr && lstrlenW (wszOldVersion)) {
          lstrcatW (wszMovePath, wszOldVersion);
          lstrcatW (wszMovePath, L"\\");
          SK_CreateDirectories (wszMovePath);
        }

        lstrcatW (wszMovePath, pwszEntry);

        if (wszOldVersion == nullptr || (! lstrlenW (wszOldVersion))) {
          lstrcatW (wszMovePath, L".old");
        }

        MoveFileExW (wszDestPath, wszMovePath, MOVEFILE_REPLACE_EXISTING);
      }

      HANDLE hOutFile =
        CreateFileW ( wszDestPath,
                        GENERIC_WRITE,
                          FILE_SHARE_READ,
                            nullptr,
                              CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr );

      if (hOutFile != INVALID_HANDLE_VALUE) {
        DWORD dwWritten;

        WriteFile ( hOutFile,
                      pData,
                        fileSize,
                          &dwWritten,
                              nullptr );

        CloseHandle (hOutFile);
      }

      else {
        dll_log.Log ( L"[AutoUpdate] Failed to open file: '%s'",
                        wszDestPath );
        free (pData);
        return E_FAIL;
      }

      free (pData);
    }
  }

  SzArEx_Free (&arc, &thread_alloc);
  File_Close  (&arc_stream.file);

  return S_OK;
}