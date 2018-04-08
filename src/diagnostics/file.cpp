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

#include <SpecialK/TLS.h>
#include <SpecialK/hooks.h>
#include <SpecialK/utility.h>
#include <SpecialK/diagnostics/file.h>

NtReadFile_pfn  NtReadFile_Original  = nullptr;
NtWriteFile_pfn NtWriteFile_Original = nullptr;

#define NT_SUCCESS(Status)                      ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS                          0

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
  NTSTATUS ntStatus =
    NtReadFile_Original (FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);

  if (NT_SUCCESS (ntStatus))
  {
    InterlockedAdd64 (&SK_TLS_Bottom ()->disk.bytes_read, Length);
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
  NTSTATUS ntStatus =
    NtWriteFile_Original (FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);

  if (NT_SUCCESS (ntStatus))
  {
    InterlockedAdd64 (&SK_TLS_Bottom ()->disk.bytes_written, Length);
  }

  return ntStatus;
}

ReadFile_pfn   ReadFile_Original     = nullptr;
ReadFileEx_pfn ReadFileEx_Original   = nullptr;

WriteFile_pfn   WriteFile_Original   = nullptr;
WriteFileEx_pfn WriteFileEx_Original = nullptr;


BOOL
WINAPI
ReadFile_Detour (
  _In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToRead,
  _Out_opt_   LPDWORD      lpNumberOfBytesRead,
  _Inout_opt_ LPOVERLAPPED lpOverlapped )
{
  DWORD BytesRead = 0;

  if (ReadFile_Original (hFile, lpBuffer, nNumberOfBytesToRead, &BytesRead, lpOverlapped))
  {
    if (! lpOverlapped)
    {
      SK_TLS_Bottom ()->disk.bytes_read += BytesRead;
    }

    // Async (TODO, handle the correct way)
    else
    {
      SK_TLS_Bottom ()->disk.bytes_read += nNumberOfBytesToRead;
    }

    if (lpNumberOfBytesRead)
      *lpNumberOfBytesRead = BytesRead;

    return TRUE;
  }

  return FALSE;
}

BOOL
WINAPI
ReadFileEx_Detour (
  _In_      HANDLE                          hFile,
  _Out_opt_ LPVOID                          lpBuffer,
  _In_      DWORD                           nNumberOfBytesToRead,
  _Inout_   LPOVERLAPPED                    lpOverlapped,
  _In_      LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
  if (ReadFileEx_Original (hFile, lpBuffer, nNumberOfBytesToRead, lpOverlapped, lpCompletionRoutine))
  {
    SK_TLS_Bottom ()->disk.bytes_read += nNumberOfBytesToRead;

    return TRUE;
  }

  return FALSE;
}

BOOL
WINAPI
WriteFile_Detour (
  _In_        HANDLE       hFile,
  _In_        LPCVOID      lpBuffer,
  _In_        DWORD        nNumberOfBytesToWrite,
  _Out_opt_   LPDWORD      lpNumberOfBytesWritten,
  _Inout_opt_ LPOVERLAPPED lpOverlapped )
{
  DWORD BytesWritten = 0;

  if (WriteFile_Original (hFile, lpBuffer, nNumberOfBytesToWrite, &BytesWritten, lpOverlapped))
  {
    if (! lpOverlapped)
    {
      SK_TLS_BottomEx (GetCurrentThreadId ())->disk.bytes_written += BytesWritten;
    }

    // Async (TODO, handle the correct way)
    else
    {
      SK_TLS_Bottom ()->disk.bytes_written += nNumberOfBytesToWrite;
    }

    if (lpNumberOfBytesWritten)
      *lpNumberOfBytesWritten = BytesWritten;

    return TRUE;
  }

  return FALSE;
}

BOOL
WINAPI
WriteFileEx_Detour (
  _In_     HANDLE                          hFile,
  _In_opt_ LPCVOID                         lpBuffer,
  _In_     DWORD                           nNumberOfBytesToWrite,
  _Inout_  LPOVERLAPPED                    lpOverlapped,
  _In_     LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
  if (WriteFileEx_Original (hFile, lpBuffer, nNumberOfBytesToWrite, lpOverlapped, lpCompletionRoutine))
  {
    SK_TLS_BottomEx (GetCurrentThreadId ())->disk.bytes_written += nNumberOfBytesToWrite;

    return TRUE;
  }

  return FALSE;
}



void
SK_File_InitHooks (void)
{
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "ReadFile",
  //                           ReadFile_Detour,
  //  static_cast_p2p <void> (&ReadFile_Original) );
  //
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "ReadFileEx",
  //                           ReadFileEx_Detour,
  //  static_cast_p2p <void> (&ReadFileEx_Original) );
  //
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "WriteFile",
  //                           WriteFile_Detour,
  //  static_cast_p2p <void> (&WriteFile_Original) );
  //
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "WriteFileEx",
  //                           WriteFileEx_Detour,
  //  static_cast_p2p <void> (&WriteFileEx_Original) );

  SK_CreateDLLHook2 (      L"NtDll.dll",
                            "ZwReadFile",
                             NtReadFile_Detour,
    static_cast_p2p <void> (&NtReadFile_Original) );

  SK_CreateDLLHook2 (      L"NtDll.dll",
                            "ZwWriteFile",
                             NtWriteFile_Detour,
    static_cast_p2p <void> (&NtWriteFile_Original) );
}