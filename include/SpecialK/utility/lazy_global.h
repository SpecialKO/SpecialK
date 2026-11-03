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
#include <atomic>
#include <concurrent_vector.h>
#include <SpecialK/thread.h>
#include <SpecialK/diagnostics/memory.h>

#pragma warning (push)
#pragma warning (disable: 4244)


class SK_LazyDeinit
{
public:
#ifdef _SK_CONSISTENCY_CHECK
  virtual void Deinit (void) = 0;
#endif

protected:
  // Never deleted through a base pointer; non-virtual on purpose.
  ~SK_LazyDeinit (void) = default;
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

// The vtable is only needed so __SK_LazyRiver can Deinit every global in
//   consistency-check builds; normal builds get no vptr and no virtual calls.
#ifdef _SK_CONSISTENCY_CHECK
// Defined below, after __SK_LazyRiver is declared (two-phase name lookup).
inline void SK_LazyGlobal_Register (SK_LazyDeinit* pGlobal) noexcept;

# define SK_LAZY_GLOBAL_BASE     : public SK_LazyDeinit
# define SK_LAZY_GLOBAL_OVERRIDE override
#else
# define SK_LAZY_GLOBAL_BASE
# define SK_LAZY_GLOBAL_OVERRIDE
#endif

template <typename T>
class SK_LazyGlobal SK_LAZY_GLOBAL_BASE
{
  enum _AllocState : LONG {
    Uninitialized = 0,
    Reserved      = 1, // Heap (reserving)
    Committed     = 2, // Heap (normal)
    FailsafeLocal = 3  // LocalAlloc
  };

public:
  // Hot path: one acquire load (a plain MOV on x86/x64) and a predictable
  //   branch. Everything else lives in getPtrSlow so this always inlines.
  __forceinline
  T* getPtr (void) noexcept
  {
    static_assert ( std::is_reference_v <T> != true,
                    "SK_LazyGlobal does not support reference types" );

    T* pRet =
      pDeferredObject.load (std::memory_order_acquire);

    if (pRet != nullptr) [[likely]]
      return pRet;

    return
      getPtrSlow ();
  }

     T& get         (void)                noexcept         { return   *getPtr ();       }
     T* operator->  (void)                noexcept         { return    getPtr ();       }
     T& operator*   (void)                noexcept         { return   *getPtr ();       }
        operator T& (void)                noexcept         { return    get    ();       }
  auto& operator [] (const uintptr_t idx) noexcept (false) { return  (*getPtr ())[idx]; }
  bool  isAllocated (void) const          noexcept
{
  return
    ( ReadAcquire (&_initlock) == Committed ) &&
      pDeferredObject.load (std::memory_order_acquire) != nullptr;
}

  SK_LazyGlobal              (const SK_LazyGlobal& ) = delete;
  SK_LazyGlobal& operator=   (const SK_LazyGlobal& ) = delete;
  SK_LazyGlobal              (      SK_LazyGlobal&&) = delete;
  SK_LazyGlobal& operator=   (      SK_LazyGlobal&&) = delete;

  constexpr SK_LazyGlobal (void) noexcept = default;

  void Deinit (void) noexcept SK_LAZY_GLOBAL_OVERRIDE
  {
    // Allocated off heap w/ C++ new
    if (isAllocated ())
    {
      delete
        pDeferredObject.exchange (nullptr, std::memory_order_acq_rel);

      InterlockedExchange (&_initlock, Uninitialized);
    }

    // Used SK's LocalAlloc wrapper, don't let delete touch it!
    else if ( FailsafeLocal ==
      InterlockedCompareExchange (&_initlock, Uninitialized, FailsafeLocal)
            )
    {
      SK_LocalFree (
        HLOCAL (pDeferredObject.exchange (nullptr, std::memory_order_acq_rel))
      );
    }
  }

  // Still required: SK_LazyGlobal is also a member of objects destroyed at
  //   runtime (e.g. SK::Framerate::Limiter, SK_TLS), not only of globals.
  ~SK_LazyGlobal (void) noexcept
  {
    // Skip global cleanup on DLL unload.
    extern volatile LONG __SK_DLL_Ending;
    if (! ReadAcquire  (&__SK_DLL_Ending))
    {
      SK_LazyGlobal::Deinit ();
    }
  }

private:
  __declspec (noinline)
  T* getPtrSlow (void) noexcept
  {
    const LONG lock_val =
      InterlockedCompareExchange (&_initlock, Reserved, Uninitialized);

    if (lock_val == Uninitialized)
    {
      T* pNew = nullptr;

      try {
        pNew = new T ();
      }

      catch (...) {
      }

      if (pNew != nullptr)
      {
        // Publish the fully constructed object before anyone can see it.
        pDeferredObject.store (pNew, std::memory_order_release);

        InterlockedExchange (&_initlock, Committed);

#ifdef _SK_CONSISTENCY_CHECK
        // Must come after publishing: __SK_LazyRiver registers itself here,
        //   and would otherwise spin forever waiting on its own init.
        SK_LazyGlobal_Register (this);
#endif
      }

      else
      {
        pNew =
          static_cast         <T *>
            ( SK_LocalAlloc ( LMEM_FIXED |
                              LMEM_ZEROINIT, sizeof (T) )
            );

        // Couldn't allocate anything, pointing to a dummy value
        //   and hoping we can limp this thing home before something
        //     really bad happens.
        pDeferredObject.store (pNew, std::memory_order_release);

        InterlockedExchange (&_initlock, FailsafeLocal);
      }

      return pNew;
    }

    else if (lock_val < Committed)
    {
      SK_Thread_SpinUntilAtomicMin (&_initlock, Committed);
    }

    return
      pDeferredObject.load (std::memory_order_acquire);
  }

protected:
  std::atomic <T *> pDeferredObject { nullptr };
  volatile LONG     _initlock       = Uninitialized;
};

#undef SK_LAZY_GLOBAL_BASE
#undef SK_LAZY_GLOBAL_OVERRIDE

#pragma warning (pop)

extern SK_LazyGlobal <SK_LazyObjectFoundry>
                    __SK_LazyRiver;

#ifdef _SK_CONSISTENCY_CHECK
inline void
SK_LazyGlobal_Register (SK_LazyDeinit* pGlobal) noexcept
{
  try {
    __SK_LazyRiver->globals.push_back (pGlobal);
  }

  catch (...) {
  }
}
#endif

#endif /* __SK__LAZY_GLOBAL_H__*/
