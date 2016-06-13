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

#include <cstdio>
#include <string>

#include <minwindef.h>
#include <minwinbase.h>

#define SK_AutoClose_Log(log) sk_logger_t::AutoClose closeme_##log = (log).auto_close ();

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
struct sk_logger_t
{
  class AutoClose
  {
  friend struct sk_logger_t;
  public:
    ~AutoClose (void)
    {
      if (log_ != nullptr)
        log_->close ();

      log_ = nullptr;
    }

  protected:
    AutoClose (sk_logger_t* log) : log_ (log) { }

  private:
    sk_logger_t* log_;
  };

  AutoClose auto_close (void) {
    return AutoClose (this);
  }

  bool init ( const char* const szFilename,
              const char* const szMode );

  void close (void);

  void LogEx (bool                 _Timestamp,
    _In_z_ _Printf_format_string_
    wchar_t const* const _Format, ...);

  void Log   (_In_z_ _Printf_format_string_
    wchar_t const* const _Format, ...);

  void Log   (_In_z_ _Printf_format_string_
    char const* const _Format, ...);

  FILE*            fLog        = NULL;
  std::string      name        = "";
  bool             silent      = false;
  bool             initialized = false;
  int              lines       =   0;
  CRITICAL_SECTION log_mutex   = { 0 };
};

//
// TODO, we may want to wrap some synchronization construct around these
//
extern sk_logger_t dll_log;
extern sk_logger_t crash_log;
extern sk_logger_t budget_log;

#endif /* __SK__LOG_H__ */
