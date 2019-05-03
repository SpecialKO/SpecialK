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
public:
  __forceinline T* getPtr (void) noexcept
  {
    static_assert ( std::is_reference_v <T> != true,
                    "SK_LazyGlobal does not support reference types" );

    const ULONG lock_val =
      InterlockedCompareExchange (&_initlock, 1, 0);

    if (lock_val == 0)
    {
      pDeferredObject =
        std::make_unique <T> ();

      InterlockedIncrement (&_initlock);
    }

    else if (lock_val < 2)
    {
      SK_Thread_SpinUntilAtomicMin (&_initlock, 2);
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
    ( ReadAcquire (&_initlock) > 1 );
}

  SK_LazyGlobal              (const SK_LazyGlobal&) = delete;
  SK_LazyGlobal& operator=   (const SK_LazyGlobal ) = delete;

  constexpr SK_LazyGlobal (void) {
  }

  ~SK_LazyGlobal (void)
  {
    if (isAllocated ())
    {
      InterlockedExchange (&_initlock, 0);
      pDeferredObject.reset ();
    }
  }

protected:
  std::unique_ptr <T>
                pDeferredObject;
  volatile LONG _initlock = 0;
};
#pragma warning (pop)

#endif /* __SK__LAZY_GLOBAL_H__*/