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

#ifndef __SK__LAZY_GLOBAL_H__
#define __SK__LAZY_GLOBAL_H__

#include <mutex>
#include <memory>
#include <concurrent_vector.h>
#include <SpecialK/thread.h>
#include <SpecialK/diagnostics/memory.h>

#pragma warning (push)
#pragma warning (disable: 4244)


class SK_LazyDeinit
{
public:
  virtual void Deinit (void) = 0;
};

class SK_LazyObjectFoundry
{
public:
  void atExit (void)
  {
#ifdef _SK_CONSISTENCY_CHECK
    for ( auto it : globals )
               it->Deinit ();
#endif

    globals.clear ();
  }

  Concurrency::concurrent_vector <SK_LazyDeinit *> globals;
};

template <typename T>
class SK_LazyGlobal : public SK_LazyDeinit
{
  enum _AllocState {
    Uninitialized = 0,
    Reserved      = 1, // Heap (reserving)
    Committed     = 2, // Heap (normal)
    FailsafeLocal = 3  // LocalAlloc
  };

public:
  T* getPtr (void) noexcept
  {
    static_assert ( std::is_reference_v <T> != true,
                    "SK_LazyGlobal does not support reference types" );

    auto pRet =
      pDeferredObject.get ();

    if (     pRet != nullptr)
      return pRet;

    const volatile ULONG lock_val =
      InterlockedCompareExchange (&_initlock, Reserved, Uninitialized);

    if (lock_val == Uninitialized)
    {
      try {
        pDeferredObject =
          std::make_unique <T> ();

#ifdef _SK_CONSISTENCY_CHECK
        __SK_LazyRiver->globals.push_back (this);
#endif
      }

      catch (...) {
      }

      if (pDeferredObject.get () == nullptr)
      {
        pDeferredObject.reset (
          reinterpret_cast    <T *>
            ( SK_LocalAlloc ( LMEM_FIXED |
                              LMEM_ZEROINIT, sizeof (T) )
            )
        );

        // Couldn't allocate anything, pointing to a dummy value
        //   and hoping we can limp this thing home before something
        //     really bad happens.
        InterlockedExchange (&_initlock, FailsafeLocal);
      }

      else
        InterlockedIncrement (&_initlock);
    }

    else if (lock_val < Committed)
    {
      SK_Thread_SpinUntilAtomicMin (&_initlock, Committed);
    }

    return
      pDeferredObject.get ();
  }

__forceinline    T& get         (void)            noexcept         { return   *getPtr ();       }
__forceinline    T* operator->  (void)            noexcept         { return    getPtr ();       }
__forceinline    T& operator*   (void)            noexcept         { return   *getPtr ();       }
__forceinline       operator T& (void)            noexcept         { return    get    ();       }
__forceinline auto& operator [] (const ULONG idx) noexcept (false) { return  (*getPtr ())[idx]; }
__forceinline bool  isAllocated (void) const      noexcept
{
  return
    ( ReadAcquire (&_initlock) == Committed ) && pDeferredObject.get () != nullptr;
}

  SK_LazyGlobal              (const SK_LazyGlobal& ) = delete;
  SK_LazyGlobal& operator=   (const SK_LazyGlobal& ) = delete;
  SK_LazyGlobal              (      SK_LazyGlobal&&) = delete;
  SK_LazyGlobal& operator=   (      SK_LazyGlobal&&) = delete;

  constexpr SK_LazyGlobal (void) noexcept {

  }

  void Deinit (void) noexcept override 
  {
    // Allocated off heap w/ C++ new
    if (isAllocated ())
    {
      pDeferredObject.reset ();
      InterlockedExchange (&_initlock, Uninitialized);
    }

    // Used SK's LocalAlloc wrapper, don't let unique_ptr delete!
    else if ( FailsafeLocal ==
      InterlockedCompareExchange (&_initlock, Uninitialized, FailsafeLocal)
            )
    {
      SK_LocalFree (
        static_cast <HLOCAL>         (
          pDeferredObject.release () )
                   );
    }
  }

  virtual ~SK_LazyGlobal (void) noexcept
  {
    Deinit ();
  }

protected:
           std::unique_ptr <T>
                pDeferredObject = nullptr;
  volatile LONG _initlock       = Uninitialized;
};

#pragma warning (pop)

extern SK_LazyGlobal <SK_LazyObjectFoundry>
                    __SK_LazyRiver;

#endif /* __SK__LAZY_GLOBAL_H__*/