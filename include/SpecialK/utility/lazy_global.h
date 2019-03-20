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
  inline T* getPtr (void)
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

    inline T& get         (void)          { return *getPtr ();      }
           T* operator->  (void)          { return  getPtr ();      }
  operator T&             (void)          { return  get    ();      }
        auto& operator [] (const int idx) { return  get    ()[idx]; }

  SK_LazyGlobal            (const SK_LazyGlobal&) = delete;
  SK_LazyGlobal& operator= (const SK_LazyGlobal ) = delete;

  constexpr SK_LazyGlobal (void) {
  }

protected:
  std::unique_ptr <T>
                pDeferredObject;
  volatile LONG _initlock = 0;
};
#pragma warning (pop)

#endif /* __SK__LAZY_GLOBAL_H__*/