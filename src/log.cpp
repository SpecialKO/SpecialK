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

using GetSystemTimePreciseAsFileTime_pfn = void ( WINAPI * )(
  _Out_ LPFILETIME lpSystemTimeAsFileTime
  );
      GetSystemTimePreciseAsFileTime_pfn
  _k32GetSystemTimePreciseAsFileTime = nullptr;

WORD
SK_Timestamp (wchar_t* const out)
{
  if (out == nullptr)
    return 0;

  SYSTEMTIME stLogTime = { };

  // Check for Windows 8 / Server 2012
  static bool __hasSystemTimePrecise =
    SK_GetProcAddress (L"kernel32", "GetSystemTimePreciseAsFileTime") != nullptr;
    //( LOBYTE (LOWORD (GetVersion ( ))) == 6   &&
    //  HIBYTE (LOWORD (GetVersion ( ))) >= 2 ) ||
    //  LOBYTE (LOWORD (GetVersion ( )    > 6));

 // More accurate timestamp is available on Windows 6.2+
  if (__hasSystemTimePrecise)
  {
    if (_k32GetSystemTimePreciseAsFileTime == nullptr)
    {
      _k32GetSystemTimePreciseAsFileTime =
        (GetSystemTimePreciseAsFileTime_pfn)
          SK_GetProcAddress ( L"kernel32",
                               "GetSystemTimePreciseAsFileTime" );
    }

    if (_k32GetSystemTimePreciseAsFileTime == nullptr)
    {
      __hasSystemTimePrecise = false;
      GetLocalTime (&stLogTime);
    }
    else
    {
      FILETIME                             ftLogTime;
      _k32GetSystemTimePreciseAsFileTime (&ftLogTime);
      FileTimeToSystemTime (&ftLogTime,
                            &stLogTime);
    }
  }

  else
  {
    GetLocalTime (&stLogTime);
  }

  wchar_t date [48] = { };
  wchar_t time [48] = { };

  GetDateFormatEx (LOCALE_NAME_INVARIANT, DATE_SHORTDATE,    &stLogTime, nullptr, date, 47, nullptr);
  GetTimeFormatEx (LOCALE_NAME_INVARIANT, TIME_NOTIMEMARKER, &stLogTime, nullptr, time, 47);

  out [0] = L'\0';

  lstrcatW (out, date);
  lstrcatW (out, L" ");
  lstrcatW (out, time);
  lstrcatW (out, L".");

  return
    stLogTime.wMilliseconds;
}

// Due to the way concurrent data structures grow, we can't shrink this beast
//   and this _is_ technically a set, of sorts... but knowing if an element is
//     present requires treating it like a map and reading a boolean.
SK_LazyGlobal <concurrency::concurrent_unordered_map <iSK_Logger *, bool>>
                                                           flush_set;
HANDLE                                                     hFlushReq  = nullptr;

DWORD
WINAPI
SK_Log_AsyncFlushThreadPump (LPVOID)
{
  SetCurrentThreadDescription (            L"[SK] Async Log Flush Thread Pump" );
  SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_LOWEST );

  HANDLE wait_objects [] = {
    hFlushReq, __SK_DLL_TeardownEvent
  };

  while (! ReadAcquire (&__SK_DLL_Ending))
  {
    if (! flush_set->empty ())
    {
      for ( auto& it : *flush_set )
      {
        if ( it.second != false  )
        {
          if ( it.first       != nullptr &&
               it.first->fLog != nullptr    )
          {
            fflush ( it.first->fLog );
          }

          it.second = false;
        }
      }
    }

    DWORD dwWait =
      WaitForMultipleObjects (2, wait_objects, FALSE, INFINITE);

    if (dwWait == WAIT_OBJECT_0)
      ResetEvent (hFlushReq);

    //else if (dwWait == WAIT_OBJECT_0 + 1)
    //  break;
  }

  SK_CloseHandle (hFlushReq);
                  hFlushReq = nullptr;

  SK_Thread_CloseSelf  ();

  return 0;
}


BOOL
SK_FlushLog (iSK_Logger* pLog)
{
  static volatile HANDLE
    hFlushThread = INVALID_HANDLE_VALUE;

  if ( INVALID_HANDLE_VALUE ==
         InterlockedCompareExchangePointer (
           &hFlushThread,
             reinterpret_cast <PVOID> (-2),
               INVALID_HANDLE_VALUE
         )
     )
  {
    hFlushReq =
      SK_CreateEvent ( nullptr, TRUE, TRUE, nullptr
        //SK_FormatStringW ( LR"(Local\SK_LogFlush_pid%x)",
        //              GetCurrentProcessId () ).c_str ()
                     );

    InterlockedExchangePointer (
      const_cast         <         LPVOID *> (
        reinterpret_cast <volatile LPVOID *>   (&hFlushThread)
                                             ),
        SK_Thread_CreateEx ( SK_Log_AsyncFlushThreadPump )
    );
  }

  if (ReadAcquire (&__SK_DLL_Ending) < 1 && hFlushReq != nullptr)
  {
    if ( ( flush_set->find ( pLog )   ==
           flush_set->cend (      ) ) ||
           flush_set.get( )[ pLog ] == false )
    { flush_set.get ()     [ pLog ]  = true;
      SetEvent               ( hFlushReq );
    }
  }

  else if ( pLog       != nullptr &&
            pLog->fLog != nullptr    )
  {
    _fflush_nolock (pLog->fLog);
    _flushall      (          );
  }

  return TRUE;
}


std::wstring
__stdcall
SK_Log_GetPath (const wchar_t* wszFileName)
{
  wchar_t wszFormattedFile [MAX_PATH] = { };
  std::wstring_view
             formatted_view
         (wszFormattedFile, MAX_PATH);

  SK_FormatStringViewW (
             formatted_view, LR"(%slogs\%s)",
               SK_GetConfigPath (), wszFileName );

  SK_CreateDirectories (
    formatted_view.data ()
  );

  return
    formatted_view.data ();
}


__declspec(nothrow)
void
iSK_Logger::close (void)
{
  if (initialized)
    EnterCriticalSection (&log_mutex);

  else
    return;


  if (fLog != nullptr)
  {
    if ( flush_set->find (this) !=
         flush_set->cend (    )    )
    {    flush_set.get( )[this] = false; }

    fflush (fLog);
    fclose (fLog);
            fLog = nullptr;
  }


  if (lines == 0)
  {
    std::wstring  full_name (
      SK_GetConfigPath ()
    );

    if (StrStrIW (name.c_str (), LR"(crash\)") != nullptr)
      full_name = name;

    else
      full_name += name;

    DeleteFileW  (full_name.c_str ());
  }

  initialized = false;
  silent      = true;

  Release ();

  LeaveCriticalSection  (&log_mutex);
  DeleteCriticalSection (&log_mutex);
}


__declspec(nothrow)
bool
iSK_Logger::init ( const wchar_t* const wszFileName,
                   const wchar_t* const wszMode )
{
  if (initialized)
    return true;

  if (name.empty ())
    lines = 0;

  name = wszFileName;

  std::wstring full_name (
    SK_GetConfigPath ()
  );

  SK_CreateDirectories (
    std::wstring (full_name + LR"(logs\)").c_str ()
  );

  if (StrStrIW (wszFileName, LR"(crash\)") != nullptr)
  {
    full_name = wszFileName;
  }

  else
    full_name += wszFileName;

  fLog   = _wfopen (full_name.c_str (), wszMode);
  silent = false;

  bool bRet =
    InitializeCriticalSectionEx (&log_mutex, 400, RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN |
                                                  SK_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO) != FALSE;
   lockless = true;

  if ((! bRet) || (fLog == nullptr))
  {
    silent = true;
    return false;
  }

  AddRef ();

  return (initialized = true);
}

#if 0
void
SK_OutputDebugStringW (const wchar_t* wszOutput)
{
  OutputDebugStringW (wszOutput);

  EXCEPTION_RECORD DebugOutputException = {
    .ExceptionCode            = DBG_PRINTEXCEPTION_WIDE_C,
    .ExceptionFlags           = EXCEPTION_SOFTWARE_ORIGINATE,
    .ExceptionAddress         = _ReturnAddress (),
    .NumberParameters         = 2,
    .ExceptionInformation     = { (ULONG_PTR)wcslen (wszOutput),
                                          (ULONG_PTR)wszOutput }
  };

  RtlRaiseException (&DebugOutputException);
}

void
SK_OutputDebugStringA (const char *szOutput)
{
  OutputDebugStringA (szOutput);

  EXCEPTION_RECORD DebugOutputException = {
    .ExceptionCode            = DBG_PRINTEXCEPTION_WIDE_C,
    .ExceptionFlags           = EXCEPTION_SOFTWARE_ORIGINATE,
    .ExceptionAddress         = _ReturnAddress (),
    .NumberParameters         = 2,
    .ExceptionInformation     = { (ULONG_PTR)strlen (szOutput),
                                          (ULONG_PTR)szOutput }
  };

  RtlRaiseException (&DebugOutputException);
}
#endif

__declspec(nothrow)
void
iSK_Logger::LogEx ( bool                 _Timestamp,
  _In_z_ _Printf_format_string_
                     wchar_t const* const _Format,
                                          ... )
{
  const bool                                                 _UseOutputDebugString =
      ((! initialized) || (fLog == nullptr));
  if (((! initialized) || (fLog == nullptr) || silent) && (! _UseOutputDebugString))
    return;


  wchar_t wszFormattedTime [48] = { };
  size_t     formattedLen       =  0L;

  if (_Timestamp)
  {
    wchar_t                    wszLogTime [48] = { };
    UINT    ms = SK_Timestamp (wszLogTime);

    
    formattedLen =
      std::clamp (
     _swprintf_s_l ( wszFormattedTime,  31,
                       L"%s%03u: ", locale,
                         wszLogTime,
                           ms ),
                  0, 31 );
  }


  va_list   _ArgList;
  va_start (_ArgList, _Format);
  size_t len =
    (size_t)
    _vscwprintf_l (   _Format,
    locale, _ArgList) + 1 + 48 + 2; // 32 extra for timestamp
  va_end   (_ArgList);


  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  wchar_t* wszOut = nullptr;


  __try
  {
    wszOut =
      pTLS ? pTLS->scratch_memory->log.formatted_output.alloc (
             len, true
         )
           :
      static_cast <wchar_t *> (
        _alloca ( len *
           sizeof (wchar_t)
        )
      );
  }
  __except ( GetExceptionCode () == STATUS_STACK_OVERFLOW ?
                                EXCEPTION_EXECUTE_HANDLER :
                                EXCEPTION_CONTINUE_SEARCH )
  {
    _resetstkoflw ();
  }

  if (wszOut == nullptr)
    return;


  wchar_t *wszAfterStamp =
    ( _Timestamp ? ( wszOut + formattedLen ) :
                     wszOut );
  if (_Timestamp)
  {
    _wcsncpy_s_l ( wszOut,                formattedLen + 1,
                     wszFormattedTime, _TRUNCATE, locale );
  }

  va_start     (_ArgList,      _Format);
  _vswprintf_l (wszAfterStamp, _Format, locale, _ArgList);
  va_end       (_ArgList);

  if (! _UseOutputDebugString)
  {
    lock   ();

    //if (! lockless)
    //  _fwrite_nolock (wszOut, 1, wcslen (wszOut), fLog);
    //else
      fputws (wszOut, fLog);

    unlock ();

    ++lines;

    SK_FlushLog (this);
  }

  else
  {
    SK_OutputDebugStringW (wszOut);
  }
}

__declspec(nothrow)
void
iSK_Logger::Log   ( _In_z_ _Printf_format_string_
                    wchar_t const* const _Format,
                                         ... )
{
  const bool                                                 _UseOutputDebugString =
      ((! initialized) || (fLog == nullptr));
  if (((! initialized) || (fLog == nullptr) || silent) && (! _UseOutputDebugString))
    return;


  wchar_t              wszFormattedTime [48] = { };
  wchar_t                    wszLogTime [48] = { };
  UINT    ms = SK_Timestamp (wszLogTime);

  size_t formattedLen =
    std::clamp (
   _swprintf_s_l ( wszFormattedTime,  31,
                     L"%s%03u: ", locale,
                       wszLogTime,
                         ms ),
                0, 31
    );

  va_list   _ArgList;
  va_start (_ArgList, _Format);

  const
  size_t len =
    (size_t)
    _vscwprintf_l (   _Format, locale,
            _ArgList) + 1 + 2  //  2 extra for CrLf
                         + 48; // 32 extra for timestamp
  va_end   (_ArgList);


  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  wchar_t* wszOut = nullptr;

  __try
  {
    wszOut =
      pTLS ? pTLS->scratch_memory->log.formatted_output.alloc (
             len, true
         )
           :
      static_cast <wchar_t *> (
        _alloca ( len *
           sizeof (wchar_t)
        )
      );
  }
  __except ( GetExceptionCode () == STATUS_STACK_OVERFLOW ?
                                EXCEPTION_EXECUTE_HANDLER :
                                EXCEPTION_CONTINUE_SEARCH )
  {
    _resetstkoflw ();
  }

  if (wszOut == nullptr)
    return;


  wchar_t *wszAfterStamp =
    wszOut + formattedLen;

  _wcsncpy_s_l ( wszOut,           formattedLen + 1,
                 wszFormattedTime, _TRUNCATE, locale );

  va_start (_ArgList,          _Format);
  _vswprintf_l (wszAfterStamp, _Format, locale,
            _ArgList);
  va_end   (_ArgList);

  _wcsncat_l (wszAfterStamp, L"\n", 1, locale);

  if (! _UseOutputDebugString)
  {
    lock   ();
    //if (! lockless)
    //  _fwrite_nolock (wszOut, 1, wcslen (wszOut), fLog);
    //else
      fputws (wszOut, fLog);
    unlock ();

    ++lines;

    SK_FlushLog (this);
  }

  else
  {
    SK_OutputDebugStringW (wszOut);
  }
}

__declspec(nothrow)
[[deprecated]]
void
iSK_Logger::Log   ( _In_z_ _Printf_format_string_
                    char const* const _Format,
                                      ... )
{
  const bool                                                 _UseOutputDebugString =
      ((! initialized) || (fLog == nullptr));
  if (((! initialized) || (fLog == nullptr) || silent) && (! _UseOutputDebugString))
    return;


  wchar_t                    wszLogTime [48] = { };
  UINT    ms = SK_Timestamp (wszLogTime);

  if (! _UseOutputDebugString)
  {
    lock ();

    _fwprintf_l ( fLog,
                    L"%s%03u: ", locale,
                             wszLogTime, ms );

    va_list   _ArgList;
    va_start (_ArgList, _Format);
    {
      _vfprintf_s_l (
              fLog,     _Format, locale,
              _ArgList);
    }
    va_end   (_ArgList);

    _fwprintf_l (fLog, L"%ws",   locale, L"\n");

            ++lines;

    unlock ();

    SK_FlushLog (this);
  }

  else
  {
    char            szLogOutput [4096] = { }; auto advance =
    _snprintf_s_l ( szLogOutput, 4096, _TRUNCATE,
                       "%ws%03u: ", locale, wszLogTime, ms );

    va_list   _ArgList;
    va_start (_ArgList, _Format);
    {
                                     advance +=
      _vsnprintf_s_l ( szLogOutput + advance,
                              4096 - advance - 1, _TRUNCATE,
                        _Format,          locale,
              _ArgList );
    }
    va_end   (_ArgList);

    _snprintf_s_l         (szLogOutput, 4096, _TRUNCATE, "%hs\n", locale, szLogOutput);
    SK_OutputDebugStringA (szLogOutput);
  }
}

__declspec(nothrow)
HRESULT
iSK_Logger::QueryInterface (THIS_ REFIID riid, void** ppvObj) noexcept
{
  if (ppvObj == nullptr)
    return E_POINTER;

  if (IsEqualGUID (riid, IID_SK_Logger) == 1)
  {
    AddRef ();

    *ppvObj = this;

    return S_OK;
  }

  return E_NOINTERFACE;
}

__declspec(nothrow)
ULONG
iSK_Logger::AddRef (THIS) noexcept
{
  return
    InterlockedIncrement (&refs);
}

__declspec(nothrow)
ULONG
iSK_Logger::Release (THIS) noexcept
{
  if (   InterlockedDecrement (&refs) != 0UL )
    return (ULONG)ReadAcquire (&refs);

  if (initialized)
  {
    initialized = false;
    delete this;
  }

  return 0;
}


iSK_Logger*
__stdcall
SK_CreateLog (const wchar_t* const wszName)
{
  auto* pLog =
    new iSK_Logger ();

  if (SK_GetCurrentGameID () != SK_GAME_ID::Launcher && (! SK_GetHostAppUtil ()->isBlacklisted ()))
  {
    if (wszName != nullptr)
    {
      pLog->init   (wszName, L"wtc+,ccs=UTF-8");
      pLog->silent = false;
    }
  }

  return pLog;
}


std::wstring
__stdcall
SK_SummarizeCaller (LPVOID lpReturnAddr)
{
  wchar_t wszSummary [256] = { };
  char    szSymbol   [256] = { };
  ULONG   ulLen            = 191;

  std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

  ulLen = SK_GetSymbolNameFromModuleAddr (
              SK_GetCallingDLL (lpReturnAddr),
  reinterpret_cast <uintptr_t> (lpReturnAddr),
                szSymbol,
                  ulLen );

  std::wstring caller =
    SK_GetCallerName (lpReturnAddr) + L"\0";

  SK_StripUserNameFromPathW (caller.data ());

  if (ulLen > 0)
  {
    swprintf_s ( wszSummary, 255,
                   L"[ %-25s <%30hs>, tid=0x%04x ]",

           caller.c_str               (),
             szSymbol,
               SK_Thread_GetCurrentId ()
    );
  }

  else
  {
    swprintf_s ( wszSummary, 255,
                   L"[ %-58s, tid=0x%04x ]",

           caller.c_str             (),
             SK_Thread_GetCurrentId ()
    );
  }

  return wszSummary;
}