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
#ifndef __SK__LOG_H__
#define __SK__LOG_H__

#include <intrin.h>
#include <Unknwn.h>

#if 0
#include <cstdio>
#include <string>

#include <minwindef.h>
#include <minwinbase.h>

// {A4BF1773-CAAB-48F3-AD88-C2AB5C23BD6F}
static const GUID IID_SK_Logger = 
{ 0xa4bf1773, 0xcaab, 0x48f3, { 0xad, 0x88, 0xc2, 0xab, 0x5c, 0x23, 0xbd, 0x6f } };

#define SK_AutoClose_Log(log) iSK_Logger::AutoClose closeme_##log = (log).auto_close ();

//
// NOTE: This is a barbaric approach to the problem... we clearly have a
//         multi-threaded execution model but the logging assumes otherwise.
//
//       The log system _is_ thread-safe, but the output can be non-sensical
//         when multiple threads are logging calls or even when a recursive
//           call is logged in a single thread.
//
//        * Consdier using a stack-based approach if these logs become
//            indecipherable in the future.
//            
interface iSK_Logger : public IUnknown
{
  class AutoClose
  {
  friend interface iSK_Logger;
  public:
    ~AutoClose (void)
    {
      if (log_ != nullptr)
        log_->close ();

      log_ = nullptr;
    }

  protected:
    AutoClose (iSK_Logger* log) : log_ (log) { }

  private:
    iSK_Logger* log_;
  };

  AutoClose auto_close (void) {
    return AutoClose (this);
  }

  iSK_Logger (void) {
    AddRef ();
  }

  virtual ~iSK_Logger (void) {
    Release ();
  }

  /*** IUnknown methods ***/
  STDMETHOD  (       QueryInterface)(THIS_ REFIID riid, void** ppvObj);
  STDMETHOD_ (ULONG, AddRef)        (THIS);
  STDMETHOD_ (ULONG, Release)       (THIS);

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
  STDMETHOD_ (void, Log)  (THIS_ _In_z_ _Printf_format_string_
                                 char const* const    _Format,
                                                      ... );

  HANDLE           hLogFile              = INVALID_HANDLE_VALUE;
  wchar_t          name [ MAX_PATH + 2 ] = { };
  bool             silent                = false;
  bool             initialized           = false;
  int              lines                 =   0;
  CRITICAL_SECTION log_mutex             = { 0 };
  ULONG            refs                  =   0UL;

  struct {
    wchar_t        raw       [ 8192] = { };
    wchar_t        formatted [16384] = { };
  } buffers;
};

extern iSK_Logger dll_log;
extern iSK_Logger crash_log;
extern iSK_Logger budget_log;

iSK_Logger*
__stdcall
SK_CreateLog (const wchar_t* const wszName);
#else

#include <cstdio>
#include <string>

//#include <minwindef.h>
//#include <minwinbase.h>

// {A4BF1773-CAAB-48F3-AD88-C2AB5C23BD6F}
static const GUID IID_SK_Logger = 
{ 0xa4bf1773, 0xcaab, 0x48f3, { 0xad, 0x88, 0xc2, 0xab, 0x5c, 0x23, 0xbd, 0x6f } };

#define SK_AutoClose_Log(log) iSK_Logger::AutoClose closeme_##log = (log).auto_close ();

//
// NOTE: This is a barbaric approach to the problem... we clearly have a
//         multi-threaded execution model but the logging assumes otherwise.
//
//       The log system _is_ thread-safe, but the output can be non-sensical
//         when multiple threads are logging calls or even when a recursive
//           call is logged in a single thread.
//
//        * Consdier using a stack-based approach if these logs become
//            indecipherable in the future.
//            
interface iSK_Logger : public IUnknown
{
  class AutoClose
  {
  friend interface iSK_Logger;
  public:
    ~AutoClose (void)
    {
      if (log_ != nullptr)
        log_->close ();

      log_ = nullptr;
    }

  protected:
    AutoClose (iSK_Logger* log) : log_ (log) { }

  private:
    iSK_Logger* log_;
  };

  AutoClose auto_close (void) {
    return AutoClose (this);
  }

  iSK_Logger (void) {
    AddRef ();
  }

  virtual ~iSK_Logger (void) {
    Release ();
  }

  /*** IUnknown methods ***/
  STDMETHOD  (       QueryInterface)(THIS_ REFIID riid, void** ppvObj);
  STDMETHOD_ (ULONG, AddRef)        (THIS);
  STDMETHOD_ (ULONG, Release)       (THIS);

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
  STDMETHOD_ (void, Log)  (THIS_ _In_z_ _Printf_format_string_
                                 char const* const    _Format,
                                                      ... );

  FILE*            fLog        = nullptr;
  std::wstring     name        = L"";
  bool             silent      = false;
  bool             initialized = false;
  int              lines       =   0;
  CRITICAL_SECTION log_mutex   = {   };
  ULONG            refs        =   0UL;
  DWORD            last_flush  =   0;
  DWORD            flush_freq  =   100; // msecs

public:
  // Temporary augmentation for log issues during thread suspension
  bool             lock   (void) { if (! lockless) { EnterCriticalSection (&log_mutex); return true; } return false; }
  bool             unlock (void) { if (! lockless) { LeaveCriticalSection (&log_mutex); return true; } return false; }
  bool             lockless    = true;
};

extern iSK_Logger dll_log;
extern iSK_Logger crash_log;
extern iSK_Logger budget_log;

interface iSK_Logger*
__stdcall
SK_CreateLog (const wchar_t* const wszName);

#include <SpecialK/diagnostics/crash_handler.h>

std::wstring
__stdcall
SK_SummarizeCaller (LPVOID lpReturnAddr = _ReturnAddress ());

#define SK_LOG_CALL(source) {                       \
    char  szSymbol [1024] = { };                    \
    ULONG ulLen  =  1024;                           \
                                                    \
    ulLen = SK_GetSymbolNameFromModuleAddr (        \
              SK_GetCallingDLL (),                  \
                (uintptr_t)_ReturnAddress (),       \
                  szSymbol,                         \
                    ulLen );                        \
                                                    \
  dll_log.Log ( L"[%hs][!] %32hs - %s",             \
                 (source),                          \
                   __FUNCTION__,                    \
                     SK_SummarizeCaller ().c_str () \
             );                                     \
}

#endif


#define SK_ARGS(...)            __VA_ARGS__
#define SK_STRIP_PARENTHESIS(X) X
#define SK_VARIADIC(X)          SK_STRIP_PARENTHESIS( SK_ARGS X )

#define SK_LOG_EX(level,first,expr)       \
  if (config.system.log_level >= (level)) \
    dll_log.LogEx (first, SK_VARIADIC (expr))

#define SK_LOG0_EX(first,expr) SK_LOG_EX(0,first,expr)
#define SK_LOG1_EX(first,expr) SK_LOG_EX(1,first,expr)
#define SK_LOG2_EX(first,expr) SK_LOG_EX(2,first,expr)
#define SK_LOG3_EX(first,expr) SK_LOG_EX(3,first,expr)
#define SK_LOG4_EX(first,expr) SK_LOG_EX(4,first,expr)

#define SK_LOG(expr,level,source)       \
  if (config.system.log_level >= level) \
    dll_log.Log (L"[" source L"] " SK_VARIADIC (expr))

#define SK_LOG0(expr,src) SK_LOG(expr,0,src)
#define SK_LOG1(expr,src) SK_LOG(expr,1,src)
#define SK_LOG2(expr,src) SK_LOG(expr,2,src)
#define SK_LOG3(expr,src) SK_LOG(expr,3,src)
#define SK_LOG4(expr,src) SK_LOG(expr,4,src)


#endif /* __SK__LOG_H__ */
