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
#include <SpecialK/log.h>
#include <SpecialK/core.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>

extern
ULONG
SK_GetSymbolNameFromModuleAddr (HMODULE hMod, uintptr_t addr, char* pszOut, ULONG ulLen);

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
  if (__hasSystemTimePrecise)
  {
    FILETIME   ftLogTime;
    GetSystemTimePreciseAsFileTime (&ftLogTime);
    FileTimeToSystemTime           (&ftLogTime, &stLogTime);
  } else {
#endif
    GetLocalTime (&stLogTime);
#if 0
  }
#endif

  wchar_t date [64] = { };
  wchar_t time [64] = { };

  GetDateFormat (LOCALE_INVARIANT,DATE_SHORTDATE,   &stLogTime,nullptr,date,64);
  GetTimeFormat (LOCALE_INVARIANT,TIME_NOTIMEMARKER,&stLogTime,nullptr,time,64);

  out [0] = L'\0';

  lstrcatW (out, date);
  lstrcatW (out, L" ");
  lstrcatW (out, time);
  lstrcatW (out, L".");

  return stLogTime.wMilliseconds;
}

BOOL
SK_FlushLog (iSK_Logger* pLog)
{
#if 1
  fflush (pLog->fLog);
  return TRUE;
#else
  DWORD dwTime = timeGetTime ();

  if ( pLog->flush_freq == 00 ||
       pLog->last_flush <= dwTime - pLog->flush_freq )
  {
    fflush (pLog->fLog);
            pLog->last_flush = dwTime;

    return TRUE;
  }

  return FALSE;
#endif
}

iSK_Logger dll_log, budget_log;


void
iSK_Logger::close (void)
{
  if (initialized)
    EnterCriticalSection (&log_mutex);

  else
    return;

  if (fLog != nullptr)
  {
    fflush (fLog);
    fclose (fLog);
  }

  if (lines == 0)
  {
    std::wstring full_name =
      SK_GetConfigPath ();

    full_name += name;

    DeleteFileW (full_name.c_str ());
  }

  if (initialized)
  {
    initialized = false;
    silent      = true;

    LeaveCriticalSection  (&log_mutex);
    DeleteCriticalSection (&log_mutex);
  }
}

bool
iSK_Logger::init ( const wchar_t* const wszFileName,
                   const wchar_t* const wszMode )
{
  if (initialized)
    return true;

  lines = 0;
  name  = wszFileName;

  std::wstring full_name =
    SK_GetConfigPath ();

  SK_CreateDirectories (
    std::wstring (full_name + L"logs\\").c_str ()
  );

  full_name += wszFileName;

  fLog = _wfopen (full_name.c_str (), wszMode);

  BOOL bRet = InitializeCriticalSectionAndSpinCount (&log_mutex, 250000);
   lockless = true;

  if ((! bRet) || (fLog == nullptr))
  {
    silent = true;
    return false;
  }

  initialized = true;
  return initialized;
}

void
iSK_Logger::LogEx ( bool                 _Timestamp,
  _In_z_ _Printf_format_string_
                     wchar_t const* const _Format,
                                          ... )
{
  if ((! initialized) || (! fLog) || silent)
    return;

  ++lines;

  if (_Timestamp)
  {
    wchar_t wszLogTime [128];

    WORD ms = SK_Timestamp (wszLogTime);

    lock ();
    fwprintf (fLog, L"%s%03u: ", wszLogTime, ms);
  }

  else
  {
    lock ();
  }

  va_list   _ArgList;
  va_start (_ArgList, _Format);
  {
    vfwprintf (fLog, _Format, _ArgList);
  }
  va_end   (_ArgList);

  unlock ();

  SK_FlushLog (this);
}

void
iSK_Logger::Log   ( _In_z_ _Printf_format_string_
                    wchar_t const* const _Format,
                                         ... )
{
  if ((! initialized) || (! fLog) || silent)
    return;

  wchar_t wszLogTime [128];

  WORD ms = SK_Timestamp (wszLogTime);

  lock ();

  ++lines;

  fwprintf (fLog, L"%s%03u: ", wszLogTime, ms);

  va_list   _ArgList;
  va_start (_ArgList, _Format);
  {
    vfwprintf (fLog, _Format, _ArgList);
  }
  va_end   (_ArgList);

  fwprintf (fLog, L"\n");

  unlock   ();

  SK_FlushLog (this);
}

void
iSK_Logger::Log   ( _In_z_ _Printf_format_string_
                    char const* const _Format,
                                      ... )
{
  if ((! initialized) || (! fLog) || silent)
    return;

  wchar_t wszLogTime [128];

  WORD ms = SK_Timestamp (wszLogTime);

  lock ();

  ++lines;

  fwprintf (fLog, L"%s%03u: ", wszLogTime, ms);

  va_list   _ArgList;
  va_start (_ArgList, _Format);
  {
    vfprintf (fLog, _Format, _ArgList);
  }
  va_end   (_ArgList);

  fwprintf (fLog, L"\n");

  unlock   ();

  SK_FlushLog (this);
}

HRESULT
iSK_Logger::QueryInterface (THIS_ REFIID riid, void** ppvObj)
{
  if (IsEqualGUID (riid, IID_SK_Logger))
  {
    AddRef ();
    *ppvObj = this;
    return S_OK;
  }

  return E_NOTIMPL;
}

ULONG
iSK_Logger::AddRef (THIS)
{
  return InterlockedIncrement (&refs);
}

ULONG
iSK_Logger::Release (THIS)
{
  return InterlockedDecrement (&refs);
}

iSK_Logger*
__stdcall
SK_CreateLog (const wchar_t* const wszName)
{
  auto* pLog =
    new iSK_Logger ();

  pLog->init   (wszName, L"w+");
  pLog->silent = false;

  return pLog;
}

std::wstring
__stdcall
SK_SummarizeCaller (LPVOID lpReturnAddr)
{
  wchar_t wszSummary [ 256] = { };

  char    szSymbol   [1024] = { };
  ULONG   ulLen             = 1024;
    
  ulLen = SK_GetSymbolNameFromModuleAddr (
            SK_GetCallingDLL (),
  reinterpret_cast <uintptr_t> (lpReturnAddr),
                szSymbol,
                  ulLen );

  if (ulLen > 0)
  {
    _snwprintf ( wszSummary, 255,
                   L"[ %-18s <%30hs>, tid=0x%04x ]",

           SK_GetCallerName (lpReturnAddr).c_str (),
             szSymbol,
               GetCurrentThreadId                ()
    );
  }

  else {
    _snwprintf ( wszSummary, 255,
                   L"[ %-28s,                        tid=0x%04x ]",
                              
           SK_GetCallerName (lpReturnAddr).c_str (),
               GetCurrentThreadId                ()
    );
  }

  return wszSummary;
}