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
#include "log.h"
#include "core.h"
#include "config.h"
#include "utility.h"

typedef void (WINAPI *GetSystemTimePreciseAsFileTime_WIN8_pfn)(_Out_ LPFILETIME);

static volatile GetSystemTimePreciseAsFileTime_WIN8_pfn
  GetSystemTimePreciseAsFileTime_WIN8 = nullptr;

WORD
__stdcall
SK_Timestamp (wchar_t* const out)
{
  SYSTEMTIME stLogTime;

  if (GetSystemTimePreciseAsFileTime_WIN8 != nullptr) {
    FILETIME                              ftLogTime;
    GetSystemTimePreciseAsFileTime_WIN8 (&ftLogTime);
    FileTimeToSystemTime                (&ftLogTime, &stLogTime);
  } else {
    GetLocalTime (&stLogTime);
  }

  wchar_t date [64] = { L'\0' };
  wchar_t time [64] = { L'\0' };

  GetDateFormat (LOCALE_INVARIANT,DATE_SHORTDATE,   &stLogTime,NULL,date,64);
  GetTimeFormat (LOCALE_INVARIANT,TIME_NOTIMEMARKER,&stLogTime,NULL,time,64);

  out [0] = L'\0';

  lstrcatW (out, date);
  lstrcatW (out, L" ");
  lstrcatW (out, time);
  lstrcatW (out, L".");

  return stLogTime.wMilliseconds;
}

iSK_Logger dll_log, budget_log;


void
__stdcall
iSK_Logger::close (void)
{
  if (hLogFile != INVALID_HANDLE_VALUE) {
    FlushFileBuffers (hLogFile);
    CloseHandle      (hLogFile);

    hLogFile = INVALID_HANDLE_VALUE;
  }

  if (lines == 0) {
    wchar_t full_name [MAX_PATH + 2] = { L'\0' };

    lstrcatW (full_name, SK_GetConfigPath ());
    lstrcatW (full_name, name);

    DeleteFileW (full_name);
  }

  initialized = false;
  silent      = true;

  DeleteCriticalSection (&log_mutex);
}

bool
__stdcall
iSK_Logger::init ( const wchar_t* const wszFileName,
                   const wchar_t* const wszMode )
{
  if (initialized)
    return true;

  lines = 0;
  lstrcpynW (name, wszFileName, MAX_PATH + 1);

  wchar_t full_name [MAX_PATH + 2] = { L'\0' };

  lstrcatW (full_name, SK_GetConfigPath ());
  lstrcatW (full_name, wszFileName);

  SK_CreateDirectories (full_name);

  hLogFile =
    CreateFileW ( full_name,
                    GENERIC_WRITE,
                      FILE_SHARE_READ,
                        nullptr,
                          CREATE_ALWAYS,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                              nullptr );

  BOOL bRet = InitializeCriticalSectionAndSpinCount (&log_mutex, 50000);

  if ((! bRet) || (hLogFile == INVALID_HANDLE_VALUE)) {
    silent = true;
    return false;
  }

  initialized = true;
  return initialized;
}

void
__stdcall
iSK_Logger::LogEx ( bool                 _Timestamp,
  _In_z_ _Printf_format_string_
                     wchar_t const* const _Format,
                                          ... )
{
  va_list _ArgList;

  if ((! initialized) || silent || hLogFile == INVALID_HANDLE_VALUE)
    return;

  wchar_t wszLogTime [128];
  int     len;

  if (_Timestamp) {
    WORD ms  = SK_Timestamp (wszLogTime);
         len = swprintf     (wszLogTime, L"%s%03u: ", wszLogTime, ms);
  }

  EnterCriticalSection (&log_mutex);

  if (_Timestamp) {
    WriteFile ( hLogFile, wszLogTime,
                  len * sizeof (wchar_t),
                    nullptr, nullptr );
  }

  len = 0;

  va_start (_ArgList, _Format);
  {
    // ASSERT: Length <= 1024 characters
    len += vswprintf (buffers.raw, _Format, _ArgList);
  }
  va_end   (_ArgList);

  len = min (8192, len);

  wchar_t* pwszDest = buffers.formatted;
  int      adj_len  = 0;

  for (int i = 0; i < len; i++) {
    if (buffers.raw [i] == L'\n') {
      *(pwszDest++) = L'\r';
      ++adj_len, ++lines;
    }

    *(pwszDest++) = buffers.raw [i];
    ++adj_len;
  }

  WriteFile ( hLogFile, buffers.formatted,
                adj_len * sizeof (wchar_t),
                  nullptr, nullptr );

  LeaveCriticalSection (&log_mutex);
}

void
__stdcall
iSK_Logger::Log   ( _In_z_ _Printf_format_string_
                    wchar_t const* const _Format,
                                         ... )
{
  va_list _ArgList;

  if ((! initialized) || silent || hLogFile == INVALID_HANDLE_VALUE)
    return;

  wchar_t wszLogTime [128];
  int     len;

  WORD ms  = SK_Timestamp (wszLogTime);
       len = swprintf     (wszLogTime, L"%s%03u: ", wszLogTime, ms);

  EnterCriticalSection (&log_mutex);

  WriteFile ( hLogFile, wszLogTime,
                len * sizeof (wchar_t),
                  nullptr, nullptr );

  len = 0;

  va_start (_ArgList, _Format);
  {
    // ASSERT: Length <= 1024 characters
    len += vswprintf (buffers.raw, _Format, _ArgList);
  }
  va_end   (_ArgList);

  len = min (8192, len);

  wchar_t* pwszDest = buffers.formatted;
  int      adj_len  = 0;

  for (int i = 0; i < len; i++) {
    if (buffers.raw [i] == L'\n') {
      *(pwszDest++) = L'\r';
      ++adj_len, ++lines;
    }

    *(pwszDest++) = buffers.raw [i];
    ++adj_len;
  }

  WriteFile ( hLogFile, buffers.formatted,
                adj_len * sizeof (wchar_t),
                  nullptr, nullptr );

  WriteFile (hLogFile, L"\r\n", sizeof (wchar_t) * 2, nullptr, nullptr);
  ++lines;

  LeaveCriticalSection (&log_mutex);
}

void
__stdcall
iSK_Logger::Log   ( _In_z_ _Printf_format_string_
                    char const* const _Format,
                                      ... )
{
  va_list _ArgList;

  if ((! initialized) || silent || hLogFile == INVALID_HANDLE_VALUE)
    return;

  wchar_t wszLogTime [128];

  WORD ms  = SK_Timestamp (wszLogTime);
  int  len = swprintf     (wszLogTime, L"%s%03u: ", wszLogTime, ms);

  EnterCriticalSection (&log_mutex);

  WriteFile ( hLogFile, wszLogTime,
                len * sizeof (wchar_t),
                  nullptr, nullptr );

  len = 0;

  va_start (_ArgList, _Format);
  {
    // ASSERT: Length <= 1024 characters
    len += vsprintf ((char *)buffers.raw, _Format, _ArgList);
  }
  va_end   (_ArgList);

  len = min (16384, len);

  char* pszDest = (char *)buffers.formatted;
  int   adj_len = 0;

  for (int i = 0; i < len; i++) {
    if (buffers.raw [i] == '\n') {
      *(pszDest++) = '\r';
      ++adj_len, ++lines;
    }

    *(pszDest++) = buffers.raw [i];
    ++adj_len;
  }

  adj_len = swprintf (buffers.formatted, L"%hs", (char *)buffers.raw);

  WriteFile ( hLogFile, buffers.formatted,
                adj_len * sizeof (wchar_t),
                  nullptr, nullptr );

  WriteFile (hLogFile, L"\r\n", sizeof (wchar_t) * 2, nullptr, nullptr);
  ++lines;

  LeaveCriticalSection (&log_mutex);
}

HRESULT
__stdcall
iSK_Logger::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
  if (IsEqualGUID (riid, IID_SK_Logger)) {
    AddRef ();
    *ppvObj = this;
    return S_OK;
  }

  return E_NOTIMPL;
}

ULONG
__stdcall
iSK_Logger::AddRef (THIS)
{
  return InterlockedIncrement (&refs);
}

ULONG
__stdcall
iSK_Logger::Release (THIS)
{
  return InterlockedDecrement (&refs);
}

iSK_Logger*
__stdcall
SK_CreateLog (const wchar_t* const wszName)
{
  // Check for Windows 8 / Server 2012
  static bool __hasSystemTimePrecise =
    (LOBYTE (LOWORD (GetVersion ())) == 6  &&
     HIBYTE (LOWORD (GetVersion ())) >= 2) ||
     LOBYTE (LOWORD (GetVersion () > 6));

  static volatile ULONG init = FALSE;

  iSK_Logger* pLog = new iSK_Logger ();

  pLog->init   (wszName, L"w+");
  pLog->silent = false;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE)) {
    if (__hasSystemTimePrecise) {
      InterlockedExchangePointer (
        (volatile LPVOID *)
          &GetSystemTimePreciseAsFileTime_WIN8,
            GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                               "GetSystemTimePreciseAsFileTime" )
      );

      dll_log.Log ( L"[ ModernOS ] Using 1us precision log timestamp only "
                    L"available on modern operating systems." );
    }
  }

  return pLog;
}