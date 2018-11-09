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

#include <SpecialK/log.h>
#include <SpecialK/TLS.h>
#include <SpecialK/hooks.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>
#include <SpecialK/diagnostics/file.h>

#include <memory>

BOOL
SK_File_GetNameFromHandle ( HANDLE   hFile,
                            wchar_t *pwszFileName,
                      const DWORD    uiMaxLen )
{
  if (uiMaxLen == 0)
    return FALSE;

  *pwszFileName = L'\0';

  wchar_t* ptrcFni =
    reinterpret_cast <wchar_t *>
    ( SK_TLS_Bottom ()->scratch_memory.cmd.alloc   (
        sizeof (wchar_t)        *
       (sizeof (FILE_NAME_INFO) + _MAX_PATH), true )
    );

  FILE_NAME_INFO *pFni =
    reinterpret_cast <FILE_NAME_INFO *>(ptrcFni);

  const BOOL success =
    GetFileInformationByHandleEx ( hFile, 
                                     FileNameInfo,
                                       pFni,
                                         sizeof FILE_NAME_INFO +
                            (_MAX_PATH * sizeof (wchar_t)) );

  if (success && pFni != nullptr)
  {
    wcsncpy_s (
      pwszFileName,
        std::min ( uiMaxLen,
                   (pFni->FileNameLength /
      (DWORD)sizeof pFni->FileName [0])  + 1
                 ), pFni->FileName,
                      _TRUNCATE
    );
  }

  else
    return FALSE;

  return
    success;
}

extern volatile LONG __SK_Init;


NtReadFile_pfn  NtReadFile_Original  = nullptr;
NtWriteFile_pfn NtWriteFile_Original = nullptr;

#define NT_SUCCESS(Status)                      ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS                          0

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

    if (! pTLS) return ntStatus;

    InterlockedAdd64 (&pTLS->disk.bytes_read, Length);

    if (config.file_io.trace_reads)
    {
      if (pTLS->disk.ignore_reads)
        return ntStatus;

      auto& file_log =
        *read_log;

      if (pTLS->disk.last_file_read != FileHandle)
      {   pTLS->disk.last_file_read  = FileHandle;
        wchar_t                                wszFileName [MAX_PATH] = { L'\0' };
        SK_File_GetNameFromHandle (FileHandle, wszFileName, MAX_PATH);

        if (config.file_io.ignore_reads.entire_thread.count (wszFileName))
          pTLS->disk.ignore_reads = TRUE;

        else if (config.file_io.ignore_reads.single_file.count (wszFileName))
          return ntStatus;

        else if (StrStrIW (wszFileName, LR"(\client_TOBII)") != nullptr)
        {
          pTLS->disk.ignore_reads  = TRUE;
          pTLS->disk.ignore_writes = TRUE;
        }

        if (ByteOffset != nullptr)
        {
          file_log.Log ( L"[tid=%4x]   File Read:  '%130ws'  <%#8lu bytes @+%-8llu>",
            GetCurrentThreadId (),
              wszFileName, Length, *ByteOffset
          );
        }

        else
        {
          file_log.Log ( L"[tid=%4x]   File Read:  '%130ws'  <%#8lu bytes>",
            GetCurrentThreadId (),
              wszFileName, Length
          );
        }
      }
    }
  }

  return ntStatus;
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

    if (! pTLS) return ntStatus;

    InterlockedAdd64 (&pTLS->disk.bytes_written, Length);

    if (config.file_io.trace_writes)
    {
      if (pTLS->disk.ignore_writes)
        return ntStatus;

      auto& file_log =
        *write_log;

      if (pTLS->disk.last_file_written != FileHandle)
      {   pTLS->disk.last_file_written  = FileHandle;
        wchar_t                                wszFileName [MAX_PATH] = { L'\0' };
        SK_File_GetNameFromHandle (FileHandle, wszFileName, MAX_PATH);

        if (StrStrIW (wszFileName, L"file_writes.log"))
          return ntStatus;

        if (config.file_io.ignore_writes.entire_thread.count (wszFileName))
          pTLS->disk.ignore_writes = TRUE;

        else if (config.file_io.ignore_writes.single_file.count (wszFileName))
          return ntStatus;

        if (ByteOffset != nullptr)
        {
          file_log.Log ( L"[tid=%4x]   File Write:  '%130ws'  <%#8lu bytes @+%-8llu>",
            GetCurrentThreadId (),
              wszFileName, Length, *ByteOffset
          );
        }

        else
        {
          file_log.Log ( L"[tid=%4x]   File Write:  '%130ws'  <%#8lu bytes>",
            GetCurrentThreadId (),
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
          config.file_io.trace_writes            ))
  return;


  SK_CreateDLLHook2 (      L"NtDll.dll",
                            "ZwReadFile",
                             NtReadFile_Detour,
    static_cast_p2p <void> (&NtReadFile_Original) );
  
  SK_CreateDLLHook2 (      L"NtDll.dll",
                            "ZwWriteFile",
                             NtWriteFile_Detour,
    static_cast_p2p <void> (&NtWriteFile_Original) );
}




#include <aclapi.h>
#include <SpecialK/log.h>

PSID
SK_Win32_GetTokenSid (_TOKEN_INFORMATION_CLASS tic);

PSID
SK_Win32_ReleaseTokenSid (PSID pSid);


bool
SK_File_CanUserWriteToPath (const wchar_t* wszPath)
{
  bool        bWritable = false;
  PSID pSid    =
    (PSID)SK_Win32_GetTokenSid (TokenUser);

  if (pSid == nullptr)
  {
    return bWritable;
  }

  PACL                 pDACL = nullptr;
  PSECURITY_DESCRIPTOR pSD   = nullptr;

  if ( ERROR_SUCCESS ==
         GetNamedSecurityInfoW ( wszPath,
                                   SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                     nullptr,  nullptr,
                                       &pDACL, nullptr,
                                         &pSD
                               )
     )
  {
    ACCESS_MASK access_mask = { };
    TRUSTEE_W   trustee     = { };

    BuildTrusteeWithSidW (&trustee, pSid);

    if ( ERROR_SUCCESS ==
           GetEffectiveRightsFromAclW ( pDACL, &trustee, &access_mask )
       )
    {
      bWritable =
        (access_mask & WRITE_DAC) != 0;
    }

    //SK_LocalFree ((HLOCAL)pDACL);
    SK_LocalFree ((HLOCAL)pSD);
  }

  SK_Win32_ReleaseTokenSid (pSid);

  return bWritable;
}