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
#pragma once

#ifndef __SK__LOG_H__
#define __SK__LOG_H__

struct IUnknown;
#include <Unknwnbase.h>
#include <Unknwn.h>

#include <intrin.h>
#include <SpecialK/core.h>
#include <SpecialK/diagnostics/crash_handler.h>

#include <locale>
#include <string>

// {A4BF1773-CAAB-48F3-AD88-C2AB5C23BD6F}
static const GUID IID_SK_Logger =
{ 0xa4bf1773, 0xcaab, 0x48f3, { 0xad, 0x88, 0xc2, 0xab, 0x5c, 0x23, 0xbd, 0x6f } };

#define SK_AutoClose_Log(log)        iSK_Logger::AutoClose closeme_##log  ((log).auto_close ());
#define SK_AutoClose_LogEx(log,name) iSK_Logger::AutoClose closeme_##name (log->auto_close ());

//
// NOTE: This is a barbaric approach to the problem... we clearly have a
//         multi-threaded execution model but the logging assumes otherwise.
//
//       The log system _is_ thread-safe, but the output can be non-sensical
//         when multiple threads are logging calls or even when a recursive
//           call is logged in a single thread.
//
//        * Consider using a stack-based approach if these logs become
//            indecipherable in the future.
//
interface iSK_Logger : public IUnknown
{
  class AutoClose
  {
  friend interface iSK_Logger;
  public:
    ~AutoClose (void) noexcept
    {
      if (log_ != nullptr)
      {
        __try {
          log_->close ();
        }

        // To satisfy the noexcept, swallow exceptions
        __except (EXCEPTION_EXECUTE_HANDLER) {
          // Can't exactly log this, so ignore it.
        };

        log_ = nullptr;
      }
    }

    AutoClose (AutoClose &)  = delete;
    AutoClose (AutoClose &&) = default;

  protected:
    AutoClose (iSK_Logger* log)                  noexcept : log_ (log)           { }
  //AutoClose (SK_LazyGlobal <iSK_Logger>&& log) noexcept : log_ (log.getPtr ()) { }

  private:
    iSK_Logger *log_;
  };

  AutoClose auto_close (void) noexcept {
    return this;
  }

  iSK_Logger (void) noexcept {
    iSK_Logger::AddRef ();

    locale =
      _wcreate_locale (LC_ALL, L"en_us.utf8");
  }

  virtual ~iSK_Logger (void) noexcept {
    _free_locale (locale);

    iSK_Logger::Release ();
  }

  /*** IUnknown methods ***/
  STDMETHOD  (       QueryInterface)(THIS_ REFIID riid, void** ppvObj) noexcept override;
  STDMETHOD_ (ULONG, AddRef)        (THIS) noexcept override;
  STDMETHOD_ (ULONG, Release)       (THIS) noexcept override;

  STDMETHOD_ (bool, init)(THIS_ const wchar_t* const wszFilename,
                                const wchar_t* const wszMode );
  STDMETHOD_ (void, close)(THIS);

  STDMETHOD_ (void, LogEx)(THIS_ bool                 _Timestamp,
                              _In_z_ _Printf_format_string_
                                 wchar_t const* const _Format,
                                                      ... );
  STDMETHOD_ (void, Log)  (THIS_ _In_z_ _Printf_format_string_
                                 wchar_t const* const _Format,
                                                      ... );
  [[deprecated]]
  STDMETHOD_ (void, Log)  (THIS_ _In_z_ _Printf_format_string_
                                 char const* const    _Format,
                                                      ... );

  FILE*            fLog        = nullptr;
  std::wstring     name;
  bool             silent      =  true;
  bool             initialized = false;
  int              lines       =   0;
  CRITICAL_SECTION log_mutex   = {   };
  volatile LONG    refs        =   0UL;
  DWORD            last_flush  =   0;
  DWORD            flush_freq  =   100; // msecs
  _locale_t        locale      = {   };

public:
  bool             lockless    = true;
  volatile LONG    relocated   = FALSE;

  // Temporary augmentation for log issues during thread suspension
  _Acquires_exclusive_lock_ (log_mutex)
  bool             lock     (void) { if (! lockless) { EnterCriticalSection (&log_mutex); return true; } return false; }
  _Releases_exclusive_lock_ (log_mutex)
  bool             unlock   (void) { if (! lockless) { LeaveCriticalSection (&log_mutex); return true; } return false; }
};

namespace sk {
  namespace logs {
    extern int base_log_lvl;
  };
};

interface iSK_Logger*
__stdcall
SK_CreateLog (const wchar_t* const wszName);

std::wstring
__stdcall
SK_Log_GetPath (const wchar_t* wszFileName);

std::wstring
__stdcall
SK_SummarizeCaller (LPVOID lpReturnAddr = _ReturnAddress ());

#define SK_LOG_CALL(source) do {                    \
    char  szSymbol [1024] = { };                    \
    ULONG ulLen  =  1024;                           \
                                                    \
    ulLen = SK_GetSymbolNameFromModuleAddr (        \
              SK_GetCallingDLL (),                  \
                (uintptr_t)_ReturnAddress (),       \
                  szSymbol,                         \
                    ulLen );                        \
                                                    \
  dll_log->Log ( L"[%hs][!] %32s - %s",             \
                 (source),                          \
                   __FUNCTIONW__,                   \
                     SK_SummarizeCaller ().c_str () \
             );                                     \
} while (0)


#define SK_ARGS(...)            __VA_ARGS__
#define SK_STRIP_PARENTHESIS(X) X
#define SK_VARIADIC(X)          SK_STRIP_PARENTHESIS( SK_ARGS X )

#define SK_LOG_EX(level,first,expr) do {        \
  if (config.system.log_level >= (level))       \
    dll_log->LogEx (first, SK_VARIADIC (expr)); \
} while (0)

#define SK_LOG0_EX(first,expr) SK_LOG_EX(0,first,expr)
#define SK_LOG1_EX(first,expr) SK_LOG_EX(1,first,expr)
#define SK_LOG2_EX(first,expr) SK_LOG_EX(2,first,expr)
#define SK_LOG3_EX(first,expr) SK_LOG_EX(3,first,expr)
#define SK_LOG4_EX(first,expr) SK_LOG_EX(4,first,expr)

#define SK_LOG(expr,level,source) do {  \
  if (config.system.log_level >= level) \
    dll_log->Log (L"[" source L"] "     \
          SK_VARIADIC (expr));          \
} while (0)

#define SK_LOG0(expr,src) SK_LOG(expr,0,src)
#define SK_LOG1(expr,src) SK_LOG(expr,1,src)
#define SK_LOG2(expr,src) SK_LOG(expr,2,src)
#define SK_LOG3(expr,src) SK_LOG(expr,3,src)
#define SK_LOG4(expr,src) SK_LOG(expr,4,src)

#define SK_LOGN(lvl,src,...) SK_LOG (      (__VA_ARGS__),\
                                    lvl,src              )
#define SK_LOGs0(src,   ...) SK_LOGN( 0,src,__VA_ARGS__  )
#define SK_LOGs1(src,   ...) SK_LOGN( 1,src,__VA_ARGS__  )
#define SK_LOGs2(src,   ...) SK_LOGN( 2,src,__VA_ARGS__  )
#define SK_LOGs3(src,   ...) SK_LOGN( 3,src,__VA_ARGS__  )
#define SK_LOGs4(src,   ...) SK_LOGN( 4,src,__VA_ARGS__  )

#define SK_LOGn(lvl,    ...) SK_LOG (      (__VA_ARGS__),\
                                   lvl,__SK_SUBSYSTEM__  )
#define SK_LOGi0(       ...) SK_LOGn(0,     __VA_ARGS__  )
#define SK_LOGi1(       ...) SK_LOGn(1,     __VA_ARGS__  )
#define SK_LOGi2(       ...) SK_LOGn(2,     __VA_ARGS__  )
#define SK_LOGi3(       ...) SK_LOGn(3,     __VA_ARGS__  )
#define SK_LOGi4(       ...) SK_LOGn(4,     __VA_ARGS__  )

#endif /* __SK__LOG_H__ */
