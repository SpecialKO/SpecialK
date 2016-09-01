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
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>

#include "../config.h"

#include "../core.h"
#include "../log.h"

#define PSAPI_VERSION 1

#include <Windows.h>
#include <psapi.h>

#pragma comment (lib, "psapi.lib")

bool SK_LoadLibrary_SILENCE = true;

typedef HMODULE (WINAPI *LoadLibraryA_pfn)(LPCSTR  lpFileName);
typedef HMODULE (WINAPI *LoadLibraryW_pfn)(LPCWSTR lpFileName);

LoadLibraryA_pfn LoadLibraryA_Original = nullptr;
LoadLibraryW_pfn LoadLibraryW_Original = nullptr;

typedef HMODULE (WINAPI *LoadLibraryExA_pfn)
( _In_       LPCSTR  lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);

typedef HMODULE (WINAPI *LoadLibraryExW_pfn)
( _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);

LoadLibraryExA_pfn LoadLibraryExA_Original = nullptr;
LoadLibraryExW_pfn LoadLibraryExW_Original = nullptr;

extern HMODULE hModSelf;

#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")

#pragma intrinsic(_ReturnAddress)

BOOL
BlacklistLibraryW (LPCWSTR lpFileName)
{
  if ( StrStrIW (lpFileName, L"ltc_game32") ||
       StrStrIW (lpFileName, L"ltc_game64") ) {
    if (! (SK_LoadLibrary_SILENCE))
      dll_log.Log (L"[Black List] Preventing Raptr's overlay, it likes to crash games!");
    return TRUE;
  }

#if 0
  if (StrStrIW (lpFileName, L"igdusc32")) {
    dll_log->Log (L"[Black List] Intel D3D11 Driver Bypassed");
    return TRUE;
  }
#endif

#if 0
  if (StrStrIW (lpFileName, L"ig75icd32")) {
    dll_log->Log (L"[Black List] Preventing Intel Integrated OpenGL driver from activating...");
    return TRUE;
  }
#endif

  return FALSE;
}

BOOL
BlacklistLibraryA (LPCSTR lpFileName)
{
  wchar_t wszWideLibName [MAX_PATH];

  MultiByteToWideChar (CP_OEMCP, 0x00, lpFileName, -1, wszWideLibName, MAX_PATH);

  return BlacklistLibraryW (wszWideLibName);
}


void
SK_TraceLoadLibraryA ( HMODULE hCallingMod,
                       LPCSTR  lpFileName,
                       LPCSTR  lpFunction )
{
  char szBaseName [MAX_PATH + 2] = { '\0' };

  GetModuleBaseNameA ( GetCurrentProcess (),
                        hCallingMod,
                          szBaseName,
                            MAX_PATH );

  dll_log.Log ( "[DLL Loader]   ( %-28s ) loaded '%#64s' <%s>",
                  szBaseName,
                    lpFileName,
                      lpFunction );
}

void
SK_TraceLoadLibraryW ( HMODULE hCallingMod,
                       LPCWSTR lpFileName,
                       LPCWSTR lpFunction )
{
  wchar_t wszBaseName [MAX_PATH + 2] = { L'\0' };

  GetModuleBaseNameW ( GetCurrentProcess (),
                        hCallingMod,
                          wszBaseName,
                            MAX_PATH );

  dll_log.Log ( L"[DLL Loader]   ( %-28s ) loaded '%#64s' <%s>",
                  wszBaseName,
                    lpFileName,
                      lpFunction );
}



HMODULE
WINAPI
LoadLibraryA_Detour (LPCSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleA (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryA (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryA_Original (lpFileName);

  if (hModEarly != hMod && (! SK_LoadLibrary_SILENCE)) {
    HMODULE hCallingMod;
    GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          (LPCWSTR)_ReturnAddress (),
                            &hCallingMod );

    SK_TraceLoadLibraryA ( hCallingMod,
                             lpFileName,
                               "LoadLibraryA" );
  }

  return hMod;
}

HMODULE
WINAPI
LoadLibraryW_Detour (LPCWSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleW (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryW (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryW_Original (lpFileName);

  if (hModEarly != hMod && (! SK_LoadLibrary_SILENCE)) {
    HMODULE hCallingMod;
    GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          (LPCWSTR)_ReturnAddress (),
                            &hCallingMod );

    SK_TraceLoadLibraryW ( hCallingMod,
                             lpFileName,
                               L"LoadLibraryW" );
  }

  return hMod;
}

HMODULE
WINAPI
LoadLibraryExA_Detour (
  _In_       LPCSTR lpFileName,
  _Reserved_ HANDLE hFile,
  _In_       DWORD  dwFlags )
{
  LPVOID retn =
    _ReturnAddress ();

  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleA (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryA (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryExA_Original (lpFileName, hFile, dwFlags);

  if ( hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE)))
                         && (! SK_LoadLibrary_SILENCE) ) {
    HMODULE hCallingMod;
    GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          (LPCWSTR)_ReturnAddress (),
                            &hCallingMod );

    SK_TraceLoadLibraryA ( hCallingMod,
                             lpFileName,
                               "LoadLibraryExA" );
  }

  return hMod;
}

HMODULE
WINAPI
LoadLibraryExW_Detour (
  _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags )
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleW (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryW (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryExW_Original (lpFileName, hFile, dwFlags);

  if ( hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE)))
                         && (! SK_LoadLibrary_SILENCE) ) {
    HMODULE hCallingMod;
    GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          (LPCWSTR)_ReturnAddress (),
                            &hCallingMod );

    SK_TraceLoadLibraryW ( hCallingMod,
                             lpFileName,
                               L"LoadLibraryExW" );
  }

  return hMod;
}

void
SK_InitCompatBlacklist (void)
{
  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryA",
                     LoadLibraryA_Detour,
           (LPVOID*)&LoadLibraryA_Original );

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryW",
                     LoadLibraryW_Detour,
           (LPVOID*)&LoadLibraryW_Original );

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryExA",
                     LoadLibraryExA_Detour,
           (LPVOID*)&LoadLibraryExA_Original );

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryExW",
                     LoadLibraryExW_Detour,
           (LPVOID*)&LoadLibraryExW_Original );
}

void
EnumLoadedModules (void)
{
  // Begin logging new loads after this
  SK_LoadLibrary_SILENCE = false;

  static iSK_Logger* pLogger = SK_CreateLog (L"logs/preloads.log");

  DWORD        dwProcID = GetCurrentProcessId ();

  HMODULE      hMods [1024];
  HANDLE       hProc = nullptr;
  DWORD        cbNeeded;
  unsigned int i;

  // Get a handle to the process.
  hProc = OpenProcess ( PROCESS_QUERY_INFORMATION |
                        PROCESS_VM_READ,
                          FALSE,
                            dwProcID );

  if (hProc == nullptr) {
    pLogger->close ();
    delete pLogger;
    return;
  }

  if ( EnumProcessModules ( hProc,
                              hMods,
                                sizeof (hMods),
                                  &cbNeeded) )
  {
    for ( i = 0; i < (cbNeeded / sizeof (HMODULE)); i++ )
    {
      wchar_t szModName [MAX_PATH + 2];

      // Get the full path to the module's file.
      if ( GetModuleFileNameExW ( hProc,
                                    hMods [i],
                                      szModName,
                                        sizeof (szModName) /
                                        sizeof (wchar_t) ) )
      {
        pLogger->Log ( L"[ Module ]  ( %ph )   -:-   * File: %s ",
                        (uintptr_t)hMods [i],
                          szModName );
      }
    }
  }

  // Release the handle to the process.
  CloseHandle (hProc);

  pLogger->close ();
  delete pLogger;
}