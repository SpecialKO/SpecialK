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

WORD
SK_Timestamp (wchar_t* const out)
{
  SYSTEMTIME stLogTime;

#if 0
  // Check for Windows 8 / Server 2012
  static bool __hasSystemTimePrecise =
    (LOBYTE (LOWORD (GetVersion ())) == 6  &&
     HIBYTE (LOWORD (GetVersion ())) >= 2) ||
     LOBYTE (LOWORD (GetVersion () > 6));

  // More accurate timestamp is available on Windows 6.2+
  if (__hasSystemTimePrecise) {
    FILETIME   ftLogTime;
    GetSystemTimePreciseAsFileTime (&ftLogTime);
    FileTimeToSystemTime           (&ftLogTime, &stLogTime);
  } else {
#endif
    GetLocalTime (&stLogTime);
#if 0
  }
#endif

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

sk_logger_t dll_log, budget_log;


void
sk_logger_t::close (void)
{
  if (fLog != NULL) {
    fflush (fLog);
    fclose (fLog);
  }

  if (lines == 0) {
    DeleteFileA (name.c_str ());
  }

  initialized = false;
  silent      = true;

  DeleteCriticalSection (&log_mutex);
}

bool
sk_logger_t::init (const char* const szFileName,
                   const char* const szMode)
{
  if (initialized)
    return true;

  lines = 0;
  name  = szFileName;

  //
  // Split the path, so we can create the log directory if necessary
  //
  if (strstr (szFileName, "\\") ||
      strstr (szFileName, "/")) {
    char* szSplitPath = _strdup (szFileName);

    // Replace all instances of '/' with '\'
    size_t len = strlen (szSplitPath);
    for (size_t i = 0; i < len; i++) {
      if (szSplitPath [i] == '/')
        szSplitPath [i] = '\\';
    }

    char* szSplitter  = strrchr (szSplitPath, '\\');
         *szSplitter  = '\0';

    char path [MAX_PATH] = { '\0' };

    char* subpath = strtok (szSplitPath, "\\");

    // For each subdirectory, create it...
    while (subpath != nullptr) {
      strcat           (path, subpath);
      CreateDirectoryA (path, NULL);
      strcat           (path, "\\");

      subpath = strtok (NULL, "\\");
    }

    free (szSplitPath);
  }

  fLog = fopen (szFileName, szMode);

  BOOL bRet = InitializeCriticalSectionAndSpinCount (&log_mutex, 2500);

  if ((! bRet) || (fLog == NULL)) {
    silent = true;
    return false;
  }

  initialized = true;
  return initialized;
}

void
sk_logger_t::LogEx (bool                 _Timestamp,
  _In_z_ _Printf_format_string_
  wchar_t const* const _Format, ...)
{
  va_list _ArgList;

  if (! initialized)
    return;

  EnterCriticalSection (&log_mutex);

  if ((! fLog) || silent) {
    LeaveCriticalSection (&log_mutex);
    return;
  }

  ++lines;

  if (_Timestamp) {
    wchar_t wszLogTime [128];

    WORD ms = SK_Timestamp (wszLogTime);

    fwprintf (fLog, L"%s%03u: ", wszLogTime, ms);
  }

  va_start (_ArgList, _Format);
  {
    vfwprintf (fLog, _Format, _ArgList);
  }
  va_end   (_ArgList);

  fflush (fLog);

  LeaveCriticalSection (&log_mutex);
}

void
sk_logger_t::Log   (_In_z_ _Printf_format_string_
  wchar_t const* const _Format, ...)
{
  va_list _ArgList;

  if (! initialized)
    return;

  EnterCriticalSection (&log_mutex);

  if ((! fLog) || silent) {
    LeaveCriticalSection (&log_mutex);
    return;
  }

  ++lines;

  wchar_t wszLogTime [128];

  WORD ms = SK_Timestamp (wszLogTime);

  fwprintf (fLog, L"%s%03u: ", wszLogTime, ms);

  va_start (_ArgList, _Format);
  {
    vfwprintf (fLog, _Format, _ArgList);
  }
  va_end   (_ArgList);

  fwprintf  (fLog, L"\n");
  fflush    (fLog);

  LeaveCriticalSection (&log_mutex);
}

void
sk_logger_t::Log   (_In_z_ _Printf_format_string_
  char const* const _Format, ...)
{
  va_list _ArgList;

  if (! initialized)
    return;

  EnterCriticalSection (&log_mutex);

  if ((! fLog) || silent) {
    LeaveCriticalSection (&log_mutex);
    return;
  }

  ++lines;

  wchar_t wszLogTime [128];

  WORD ms = SK_Timestamp (wszLogTime);

  fwprintf (fLog, L"%s%03u: ", wszLogTime, ms);

  va_start (_ArgList, _Format);
  {
    vfprintf (fLog, _Format, _ArgList);
  }
  va_end   (_ArgList);

  fwprintf  (fLog, L"\n");
  fflush    (fLog);

  LeaveCriticalSection (&log_mutex);
}