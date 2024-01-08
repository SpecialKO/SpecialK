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

#include <SpecialK/stdafx.h>

BOOL
SK_File_GetNameFromHandle ( HANDLE   hFile,
                            wchar_t *pwszFileName,
                      const DWORD    uiMaxLen )
{
  if (uiMaxLen == 0 || pwszFileName == nullptr)
    return FALSE;

  *pwszFileName = L'\0';

  auto ptrcFni =
    reinterpret_cast <FILE_NAME_INFO *>
    ( SK_TLS_Bottom ()->scratch_memory->cmd.alloc (
            sizeof (  FILE_NAME_INFO  ) +
                              _MAX_PATH * 2, true )
    );

  auto *pFni = ptrcFni;

  const BOOL success =
    GetFileInformationByHandleEx ( hFile,
                                     FileNameInfo,
                                       pFni,
                             sizeof (FILE_NAME_INFO) +
                                           _MAX_PATH * 2 );

  if (success && pFni != nullptr)
  {
    wcsncpy_s (
      pwszFileName,
        std::min ( uiMaxLen,
                    (pFni->FileNameLength /
      (DWORD)sizeof (pFni->FileName [0]))  + 1
                 ),  pFni->FileName,
                      _TRUNCATE
    );
  }

  else
    return FALSE;

  return
    success;
}

NtReadFile_pfn  NtReadFile_Original  = nullptr;
NtWriteFile_pfn NtWriteFile_Original = nullptr;

iSK_Logger *read_log  = nullptr;
iSK_Logger *write_log = nullptr;

NTSTATUS
WINAPI
NtReadFile_Detour (
  _In_     HANDLE           FileHandle,
  _In_opt_ HANDLE           Event,
  _In_opt_ PVOID            ApcRoutine,
  _In_opt_ PVOID            ApcContext,
  _Out_    PVOID            IoStatusBlock,
  _Out_    PVOID            Buffer,
  _In_     ULONG            Length,
  _In_opt_ PLARGE_INTEGER   ByteOffset,
  _In_opt_ PULONG           Key )
{
  const NTSTATUS ntStatus =
    NtReadFile_Original ( FileHandle, Event, ApcRoutine, ApcContext,
                            IoStatusBlock,
                              Buffer, Length, ByteOffset,
                                Key );

  if (NT_SUCCESS (ntStatus))
  {
    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    if (pTLS == nullptr)
      return ntStatus;

    UINT64 read_total =
      InterlockedAdd64 ( &pTLS->disk->bytes_read,
                   static_cast <LONG64> (Length) );

    if ( pTLS->scheduler->mmcs_task == nullptr  &&
           config.render.framerate.enable_mmcss &&
                 read_total > (666 * 1024 * 1024)    )
    {
      const DWORD dwTid =
        SK_Thread_GetCurrentId ();

      auto& rb =
        SK_GetCurrentRenderBackend ();

      const auto dwRenderTid =
        ReadULongAcquire (&rb.thread);

      if ( dwRenderTid != 0UL&&
           dwRenderTid != dwTid )
      {
        static SK_LazyGlobal <concurrency::concurrent_unordered_set <DWORD>> dwDiskTids;

        if ( (dwDiskTids->find (dwTid) ==
              dwDiskTids->cend (     )  ) )
        {
          auto* task =
            SK_MMCS_GetTaskForThreadIDEx ( dwTid,
              "[GAME] File I/O Centric Thread",
                "Distribution", "Playback" );

          // The first thread seen gets less special treatment
          if (dwDiskTids->empty ())
          {
            static bool bFFXV =
              (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV);
            if (bFFXV)
              task->queuePriority (AVRT_PRIORITY_CRITICAL);
            else
              task->queuePriority (AVRT_PRIORITY_NORMAL);
          }
          else
            task->queuePriority (AVRT_PRIORITY_HIGH);

          dwDiskTids->insert (dwTid);
        }
      }
    }

    if (config.file_io.trace_reads)
    {
      if (pTLS->disk->ignore_reads)
        return ntStatus;

      if (pTLS->disk->last_file_read != FileHandle)
      {   pTLS->disk->last_file_read  = FileHandle;
        wchar_t                                wszFileName [MAX_PATH + 2] = { };
        SK_File_GetNameFromHandle (FileHandle, wszFileName, MAX_PATH);

        if (config.file_io.ignore_reads->entire_thread.count (wszFileName))
          pTLS->disk->ignore_reads = TRUE;

        else if (config.file_io.ignore_reads->single_file.count (wszFileName))
          return ntStatus;

        else if (StrStrIW (wszFileName, LR"(\client_TOBII)") != nullptr)
        {
          pTLS->disk->ignore_reads  = TRUE;
          pTLS->disk->ignore_writes = TRUE;
        }

        auto& file_log =
          *read_log;

        if (ByteOffset != nullptr)
        {
          file_log.Log ( L"[tid=%4x]   File Read:  '%130ws'  <%#8lu bytes @+%-8llu>",
            SK_Thread_GetCurrentId (),
              wszFileName, Length, *ByteOffset
          );
        }

        else
        {
          file_log.Log ( L"[tid=%4x]   File Read:  '%130ws'  <%#8lu bytes>",
            SK_Thread_GetCurrentId (),
              wszFileName, Length
          );
        }
      }
    }
  }

  return
    ntStatus;
}

NTSTATUS
WINAPI
NtWriteFile_Detour (
  HANDLE           FileHandle,
  HANDLE           Event,
  PVOID            ApcRoutine,
  PVOID            ApcContext,
  PVOID            IoStatusBlock,
  PVOID            Buffer,
  ULONG            Length,
  PLARGE_INTEGER   ByteOffset,
  PULONG           Key )
{
  const NTSTATUS ntStatus =
    NtWriteFile_Original ( FileHandle, Event, ApcRoutine, ApcContext,
                             IoStatusBlock,
                               Buffer, Length, ByteOffset,
                                 Key );

  if (NT_SUCCESS (ntStatus))
  {
    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    if (pTLS == nullptr) return ntStatus;

    InterlockedAdd64 (&pTLS->disk->bytes_written, Length);

    if (config.file_io.trace_writes)
    {
      if (pTLS->disk->ignore_writes)
        return ntStatus;

      if (pTLS->disk->last_file_written != FileHandle)
      {   pTLS->disk->last_file_written  = FileHandle;
        wchar_t                                wszFileName [MAX_PATH + 2] = { };
        SK_File_GetNameFromHandle (FileHandle, wszFileName, MAX_PATH);

        if (StrStrIW (wszFileName, L"file_writes.log") != nullptr)
          return ntStatus;

        if (config.file_io.ignore_writes->entire_thread.count (wszFileName))
          pTLS->disk->ignore_writes = TRUE;

        else if (config.file_io.ignore_writes->single_file.count (wszFileName))
          return ntStatus;

        auto& file_log =
          *write_log;

        if (ByteOffset != nullptr)
        {
          file_log.Log ( L"[tid=%4x]   File Write:  '%130ws'  <%#8lu bytes @+%-8llu>",
            SK_Thread_GetCurrentId (),
              wszFileName, Length, *ByteOffset
          );
        }

        else
        {
          file_log.Log ( L"[tid=%4x]   File Write:  '%130ws'  <%#8lu bytes>",
            SK_Thread_GetCurrentId (),
              wszFileName, Length
          );
        }
      }
    }
  }

  return ntStatus;
}


void
SK_File_InitHooks (void)
{
  if (! ( config.threads.enable_file_io_trace ||
          config.file_io.trace_reads          ||
          config.file_io.trace_writes )          )
  {
    return;
  }


  SK_CreateDLLHook2 (      L"NtDll.dll",
                            "ZwReadFile",
                             NtReadFile_Detour,
    static_cast_p2p <void> (&NtReadFile_Original) );

  SK_CreateDLLHook2 (      L"NtDll.dll",
                            "ZwWriteFile",
                             NtWriteFile_Detour,
    static_cast_p2p <void> (&NtWriteFile_Original) );
}





BOOL
SK_GetFileSD ( const wchar_t              *wszPath,
                     PSECURITY_DESCRIPTOR *pFileSD,
                     PACL                 *pACL )
{
  BOOL bRetVal = FALSE;

  if (wszPath [0] == L'\0')
    return bRetVal;

  SK_AutoHandle hFile (
    CreateFile ( wszPath,
                   READ_CONTROL,
                     0, nullptr,
                     OPEN_EXISTING,
                       FILE_ATTRIBUTE_DIRECTORY |
                       FILE_FLAG_BACKUP_SEMANTICS,
                         nullptr )
  );

  if ((intptr_t)hFile.m_h <= 0) { bRetVal = FALSE; }

  else
  {
    SECURITY_INFORMATION secInfo =
      DACL_SECURITY_INFORMATION;

    DWORD dwErr =
      GetSecurityInfo (
        hFile,   SE_FILE_OBJECT,
        secInfo, nullptr, nullptr,
           pACL, nullptr, pFileSD
      );

    bRetVal =
      ( dwErr == ERROR_SUCCESS );
  }

  return
    bRetVal;
}

bool
SK_File_CanUserWriteToPath (const wchar_t* wszPath)
{
  bool  bRet   = false;
  DWORD length = 0;

//PACL                 pFileDACL = nullptr;
//PSECURITY_DESCRIPTOR pFileSD   = nullptr;

  SK_AutoHandle hToken (INVALID_HANDLE_VALUE);

  if (! GetFileSecurity ( wszPath,
                            OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION,
                              nullptr, 0, &length ) &&
     ERROR_INSUFFICIENT_BUFFER == GetLastError () )
  {
    PSECURITY_DESCRIPTOR security =
      static_cast< PSECURITY_DESCRIPTOR > (new (std::nothrow) uint8_t [length]);

    if ( security && GetFileSecurity ( wszPath,
                                       OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION,
                                         security, length, &length ) )
    {
      if ( OpenProcessToken ( SK_GetCurrentProcess (), TOKEN_IMPERSONATE | TOKEN_QUERY |
                                                       TOKEN_DUPLICATE   | STANDARD_RIGHTS_READ,
                                                         &hToken.m_h ) )
      {
        SK_AutoHandle hImpersonatedToken (INVALID_HANDLE_VALUE);

        if ( DuplicateToken ( hToken.m_h, SecurityImpersonation, &hImpersonatedToken.m_h ) )
        {
          GENERIC_MAPPING mapping          = { 0xFFFFFFFF,
                                               0xFFFFFFFF,
                                               0xFFFFFFFF,
                                               0xFFFFFFFF };
          PRIVILEGE_SET   privileges       = {            };
          DWORD           grantedAccess    = 0,
                          privilegesLength = sizeof privileges;
          BOOL            result           = FALSE;

          mapping.GenericRead    = FILE_GENERIC_READ;
          mapping.GenericWrite   = FILE_GENERIC_WRITE;
          mapping.GenericExecute = FILE_GENERIC_EXECUTE;
          mapping.GenericAll     = FILE_ALL_ACCESS;

          DWORD dwMask =
            GENERIC_WRITE;

          MapGenericMask (&dwMask, &mapping);

          if ( AccessCheck ( security, hImpersonatedToken.m_h, dwMask,
                               &mapping, &privileges,
                                 &privilegesLength, &grantedAccess,
                                   &result ) )
          {
            bRet =
              ( result != FALSE );
          }
        }
      }
    }

    delete [] (PSECURITY_DESCRIPTOR*)security;
  }

//if (pFileSD != nullptr)
//{
//  SK_LocalFree (pFileSD);
//                pFileSD = nullptr;
//}

  if (! bRet)
  {
    ////if (SK_GetFileSD (wszPath, &pFileSD, &pFileDACL))
    ////{
    ////  // ALL ACCESS if NULL
    ////  if (pFileDACL == nullptr)
    ////  {
    ////    if (pFileSD != nullptr) SK_LocalFree (pFileSD);
    ////
    ////    return TRUE;
    ////  }
    ////}
    ////
    ////FILE *fTest =
    ////  _wfsopen (L"SK_TEST_FILE.tmp", L"wbS", _SH_DENYNO);
    ////
    ////if (fTest != nullptr)
    ////{
    ////  fclose      (fTest);
    ////  DeleteFileW (L"SK_TEST_FILE.tmp");
    ////
    ////  bRet = true;
    ////}
  }

  return bRet;
}