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

  #ifndef __SK__FILE_H__
  #define __SK__FILE_H__

#include <minwindef.h>

void
SK_File_InitHooks (void);

typedef BOOL (WINAPI *ReadFile_pfn)(
  _In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToRead,
  _Out_opt_   LPDWORD      lpNumberOfBytesRead,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
);

typedef BOOL (WINAPI *ReadFileEx_pfn)(
  _In_      HANDLE                          hFile,
  _Out_opt_ LPVOID                          lpBuffer,
  _In_      DWORD                           nNumberOfBytesToRead,
  _Inout_   LPOVERLAPPED                    lpOverlapped,
  _In_      LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

typedef BOOL (WINAPI *WriteFile_pfn)(
  _In_        HANDLE       hFile,
  _In_        LPCVOID      lpBuffer,
  _In_        DWORD        nNumberOfBytesToWrite,
  _Out_opt_   LPDWORD      lpNumberOfBytesWritten,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
);

typedef BOOL (WINAPI *WriteFileEx_pfn)(
  _In_     HANDLE                          hFile,
  _In_opt_ LPCVOID                         lpBuffer,
  _In_     DWORD                           nNumberOfBytesToWrite,
  _Inout_  LPOVERLAPPED                    lpOverlapped,
  _In_     LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

  typedef NTSTATUS (WINAPI *NtReadFile_pfn)(
    _In_     HANDLE           FileHandle,
    _In_opt_ HANDLE           Event,
    _In_opt_ PVOID            ApcRoutine,
    _In_opt_ PVOID            ApcContext,
    _Out_    PVOID            IoStatusBlock,
    _Out_    PVOID            Buffer,
    _In_     ULONG            Length,
    _In_opt_ PLARGE_INTEGER   ByteOffset,
    _In_opt_ PULONG           Key
  );

  typedef NTSTATUS (WINAPI *NtWriteFile_pfn)(
    HANDLE           FileHandle,
    HANDLE           Event,
    PVOID            ApcRoutine,
    PVOID            ApcContext,
    PVOID            IoStatusBlock,
    PVOID            Buffer,
    ULONG            Length,
    PLARGE_INTEGER   ByteOffset,
    PULONG           Key
  );

extern ReadFile_pfn   ReadFile_Original;
extern ReadFileEx_pfn ReadFileEx_Original;

extern WriteFile_pfn   WriteFile_Original;
extern WriteFileEx_pfn WriteFileEx_Original;

extern NtReadFile_pfn  NtReadFile_Original;
extern NtWriteFile_pfn NtWriteFile_Original;

#endif /* __SK__FILE_H__ */