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

#include <memory>
#include <SpecialK/thread.h>

#pragma warning (push)
#pragma warning (disable: 4244)
template <typename T>
class SK_LazyGlobal
{
  enum _AllocState {
    Uninitialized = 0,
    Reserved      = 1, // Heap (reserving)
    Committed     = 2, // Heap (normal)
    FailsafeLocal = 3  // LocalAlloc
  };

public:
  __forceinline T* getPtr (void) noexcept
  {
    static_assert ( std::is_reference_v <T> != true,
                    "SK_LazyGlobal does not support reference types" );

    const ULONG lock_val =
      InterlockedCompareExchange (&_initlock, Reserved, Uninitialized);

    if (lock_val == Uninitialized)
    {
      pDeferredObject.reset (
        new (std::nothrow) T
      );

      if (pDeferredObject._Myptr () == nullptr)
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

      InterlockedIncrement (&_initlock);
    }

    else if (lock_val < Committed)
    {
      SK_Thread_SpinUntilAtomicMin (&_initlock, Committed);
    }

    return
      pDeferredObject.get ();
  }

__forceinline    T& get         (void)          noexcept { return   *getPtr ();       }
__forceinline    T* operator->  (void)          noexcept { return    getPtr ();       }
__forceinline    T& operator*   (void)          noexcept { return   *getPtr ();       }
__forceinline       operator T& (void)          noexcept { return    get    ();       }
__forceinline auto& operator [] (const int idx) noexcept { return  (*getPtr ())[idx]; }
__forceinline bool  isAllocated (void)    const noexcept
{
  return
    ( ReadAcquire (&_initlock) == Committed );
}

  SK_LazyGlobal              (const SK_LazyGlobal&) = delete;
  SK_LazyGlobal& operator=   (const SK_LazyGlobal ) = delete;

  constexpr SK_LazyGlobal (void) noexcept {
  }

  ~SK_LazyGlobal (void)
  {
    if (isAllocated ())
    {
      InterlockedExchange (&_initlock, Uninitialized);
      pDeferredObject.reset ();
    }

    else if ( FailsafeLocal ==
      InterlockedCompareExchange (&_initlock, Uninitialized, FailsafeLocal)
            )
    {
      SK_LocalFree ((HLOCAL)pDeferredObject.release ());
    }
  }

protected:
  std::unique_ptr <T>
                pDeferredObject = nullptr;
  volatile LONG _initlock       = Uninitialized;
};
#pragma warning (pop)

#endif /* __SK__LAZY_GLOBAL_H__*/