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

#include "archive.h"

static ISzAlloc g_Alloc = { SzAlloc, SzFree };

std::vector <sk_file_entry_s>
SK_Get7ZFileContents (const wchar_t* wszArchive)
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

  std::vector <sk_file_entry_s> files;

  if (InFile_OpenW (&arc_stream.file, wszArchive))
  {
    return files;
  }

  CSzArEx       arc;
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

      uint64_t fileSize = SzArEx_GetFileSize (&arc, i);

      wchar_t* pwszEntry = wszEntry;

      if (*wszEntry == L'\\')
        pwszEntry++;

      files.push_back (sk_file_entry_s { i, fileSize, pwszEntry });
    }

    File_Close  (&arc_stream.file);
  }

  SzArEx_Free (&arc, &thread_alloc);

  return files;
}

HRESULT
SK_Decompress7z ( const wchar_t*            wszArchive,
                  const wchar_t*            wszOldVersion,
                  bool                      backup,
                  SK_7Z_DECOMP_PROGRESS_PFN callback )
{
  std::vector <sk_file_entry_s> files =
    SK_Get7ZFileContents (wszArchive);

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

  SzArEx_Init (&arc);

  uint32_t block_idx     = 0xFFFFFFFF;

  if ( InFile_OpenW (&arc_stream.file, wszArchive) ||
       SzArEx_Open ( &arc,
                       &look_stream.s,
                         &thread_alloc,
                           &thread_tmp_alloc ) != SZ_OK )
  {
    dll_log.Log ( L"[AutoUpdate]  ** Cannot open archive file: %s",
                    wszArchive );

    SzArEx_Free (&arc, &thread_alloc);

    return E_FAIL;
  }

  for (int i = 0; i < files.size (); i++)
  {
    int fileno = files [i].fileno;

    Byte*    out           = nullptr;
    size_t   out_len       = 0;
    size_t   offset        = 0;
    size_t   decomp_size   = 0;

    dll_log.Log ( L"[AutoUpdate] Extracting file ('%s')",
                    files [i].name.c_str () );

    if ( SZ_OK !=
           SzArEx_Extract ( &arc,          &look_stream.s, files [i].fileno,
                            &block_idx,    &out,           &out_len,
                            &offset,       &decomp_size,
                            &thread_alloc, &thread_tmp_alloc ) ) {
      dll_log.Log ( L"[AutoUpdate] Failed to extract 7-zip file ('%s')",
                      files [i].name.c_str () );

      File_Close  (&arc_stream.file);
      SzArEx_Free (&arc, &thread_alloc);

      return E_FAIL;
    }

    extern const wchar_t*
    __stdcall
    SK_GetRootPath (void);

    wchar_t wszDestPath [MAX_PATH] = { L'\0' };
    wchar_t wszMovePath [MAX_PATH] = { L'\0' };

    wcscpy (wszDestPath, SK_GetRootPath ());

    lstrcatW (wszDestPath, files [i].name.c_str ());

    if (GetFileAttributes (wszDestPath) != INVALID_FILE_ATTRIBUTES) {
      wcscpy (wszMovePath, SK_GetRootPath ());

      lstrcatW (wszMovePath, L"Version\\");

      if (wszOldVersion != nullptr && lstrlenW (wszOldVersion) && backup) {
        lstrcatW (wszMovePath, wszOldVersion);
        lstrcatW (wszMovePath, L"\\");
      }

      lstrcatW (wszMovePath, files [i].name.c_str ());

      // If the archive contains sub-directories, this will create them
      SK_CreateDirectories (wszMovePath);

      if (wszOldVersion == nullptr || (! lstrlenW (wszOldVersion)) || (! backup)) {
        lstrcatW (wszMovePath, L".old");
      }

      MoveFileExW (wszDestPath, wszMovePath, MOVEFILE_REPLACE_EXISTING);
    }

    HANDLE hOutFile =
      CreateFileW ( wszDestPath,
                      GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr,
                            CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL |
                              FILE_FLAG_SEQUENTIAL_SCAN,
                                nullptr );

    if (hOutFile != INVALID_HANDLE_VALUE) {
      DWORD dwWritten;

      WriteFile ( hOutFile,
                    out,
                      out_len,
                        &dwWritten,
                            nullptr );

      CloseHandle (hOutFile);

      // We still need to move files in order to replace ones currently
      //   involving execution, but if the user does not want backups,
      //     attempt to delete the moved files.
      if (! backup) {
        DeleteFileW (wszMovePath);
      }
    }

    else {
      dll_log.Log ( L"[AutoUpdate] Failed to open file: '%s'",
                      wszDestPath );

      File_Close  (&arc_stream.file);
      SzArEx_Free (&arc, &thread_alloc);

      return E_FAIL;
    }

    if (callback != nullptr) {
      callback (i + 1, files.size ());
    }
  }

  File_Close  (&arc_stream.file);
  SzArEx_Free (&arc, &thread_alloc);

  return S_OK;
}