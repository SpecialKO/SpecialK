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

#ifndef __SK__THREAD_H__
#define __SK__THREAD_H__

#include <Windows.h>

class SK_Thread_CriticalSection
{
public:
  SK_Thread_CriticalSection ( CRITICAL_SECTION* pCS )
  {
    cs_ = pCS;
  };

  ~SK_Thread_CriticalSection (void) = default;

  void lock (void) {
    EnterCriticalSection (cs_);
  }

  void unlock (void)
  {
    LeaveCriticalSection (cs_);
  }

protected:
  CRITICAL_SECTION* cs_;
};

class SK_Thread_HybridSpinlock : public SK_Thread_CriticalSection
{
public:
  SK_Thread_HybridSpinlock (int spin_count = 3000) :
                                                     SK_Thread_CriticalSection (new CRITICAL_SECTION)
  {
    InitializeCriticalSectionAndSpinCount (cs_, spin_count);
  }

  ~SK_Thread_HybridSpinlock (void)
  {
    DeleteCriticalSection (cs_);
    delete cs_;
  }
};

class SK_AutoCriticalSection {
public:
  SK_AutoCriticalSection ( CRITICAL_SECTION* pCS,
                           bool              try_only = false )
  {
    acquired_ = false;
    cs_       = pCS;

    if (try_only)
      TryEnter ();
    else {
      Enter ();
    }
  }

  ~SK_AutoCriticalSection (void)
  {
    Leave ();
  }

  bool try_result (void)
  {
    return acquired_;
  }

  void enter (void)
  {
    EnterCriticalSection (this->cs_);

    acquired_ = true;
  }

protected:
  bool TryEnter (_Acquires_lock_(* this->cs_) void)
  {
    return (acquired_ = (TryEnterCriticalSection (cs_) != FALSE));
  }

  void Enter (_Acquires_lock_(* this->cs_) void)
  {
    EnterCriticalSection (cs_);

    acquired_ = true;
  }

  void Leave (_Releases_lock_(* this->cs_) void)
  {
    if (acquired_ != false)
      LeaveCriticalSection (cs_);

    acquired_ = false;
  }

private:
  bool              acquired_;
  CRITICAL_SECTION* cs_;
};



__forceinline
static void
SK_Thread_SpinUntilFlagged (volatile LONG* pFlag, LONG _SpinMax = 750L)
{
  while (! ReadAcquire (pFlag))
  {
    for (int i = 0; i < _SpinMax && (! ReadAcquire (pFlag)); i++)
      ;

    if (ReadAcquire (pFlag))
      break;

    MsgWaitForMultipleObjectsEx (0, nullptr, 16UL, QS_ALLINPUT, MWMO_ALERTABLE);
  }
}

__forceinline
static void
SK_Thread_SpinUntilAtomicMin (volatile LONG* pVar, LONG count, LONG _SpinMax = 750L)
{
  while (ReadAcquire (pVar) < count)
  {
    for (int i = 0; i < _SpinMax && (ReadAcquire (pVar) < count); i++)
      ;

    if (ReadAcquire (pVar) < count)
      break;

    MsgWaitForMultipleObjectsEx (0, nullptr, 16UL, QS_ALLINPUT, MWMO_ALERTABLE);
  }
}


#endif /* __SK__THREAD_H__ */