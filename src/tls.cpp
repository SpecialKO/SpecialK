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
#include <SpecialK/tls.h>
#include <winternl.h>

volatile LONG _SK_TLS_AllocationFailures = 0;

SK_LazyGlobal <
  concurrency::concurrent_unordered_map <
    DWORD, SK_TLS*>
  > __SK_EmergencyTLS;
   // ^^^ Backing store for games that have @#$%'d up TLS

volatile DWORD __SK_TLS_INDEX =
  TLS_OUT_OF_INDEXES;


SK_TLS*
SK_TLS_FastTEBLookup (DWORD dwTlsIndex)
{
  // It's not often we get an index this low,
  //   if we have one, it has a lower address in the TEB
  if (dwTlsIndex < 64)
  {
    return
      (SK_TLS *)(SK_Thread_GetTEB_FAST ()->TlsSlots [dwTlsIndex]);
  }

  //
  // The remainder of the indexes are into an array at the end of
  //   the Windows NT Thread Environment Block data structure.
  //
  //  * The original claimed limitation of 64 indexes has not been true
  //      for a very long time. But there's still an upper-bound of ~1088.
  //
  else if (dwTlsIndex <= 1088)
  {
    TEB* pTEB =
      (TEB *)SK_Thread_GetTEB_FAST ();

    return ( pTEB->TlsExpansionSlots == nullptr ?
                                        nullptr : (SK_TLS *)
          ((&pTEB->TlsExpansionSlots) [dwTlsIndex - 64]) );
  }

  return nullptr;
}

//
// Circumvent some anti-debug stuff by getting our hands dirty with the TEB
//   manually and storing our allocated TLS data in the slot we rightfully own.
//
SK_TLS*
SK_TLS_SetValue_NoFail (DWORD dwTlsIndex, SK_TLS *pTLS)
{
  TEB* pTEB =
    (TEB *)SK_Thread_GetTEB_FAST ();

  // It's not often we get an index this low,
  //   if we have one, it has a lower address in the TEB
  if (dwTlsIndex < 64)
  {
    SK_ReleaseAssert (pTEB->TlsSlots [dwTlsIndex] == nullptr ||
                      pTEB->TlsSlots [dwTlsIndex] == pTLS);

    if (pTEB->TlsSlots [dwTlsIndex] == nullptr)
        pTEB->TlsSlots [dwTlsIndex] =  pTLS;

    return
      (SK_TLS *)pTEB->TlsSlots [dwTlsIndex];
  }

  //
  // The remainder of the indexes are into an array at the end of
  //   the Windows NT Thread Environment Block data structure.
  //
  //  * The original claimed limitation of 64 indexes has not been true
  //      for a very long time. But there's still an upper-bound of ~1088.
  //
  else if (dwTlsIndex <= 1088)
  {
    SK_ReleaseAssert (pTEB->TlsExpansionSlots != nullptr);

    if (pTEB->TlsExpansionSlots != nullptr)
    {
      SK_ReleaseAssert ((&pTEB->TlsExpansionSlots)[dwTlsIndex - 64] == nullptr ||
                        (&pTEB->TlsExpansionSlots)[dwTlsIndex - 64] == pTLS);

      if ((&pTEB->TlsExpansionSlots)[dwTlsIndex - 64] == nullptr)
      {   (&pTEB->TlsExpansionSlots)[dwTlsIndex - 64]  = pTLS;  }

      return
        (SK_TLS *)
          ((&pTEB->TlsExpansionSlots) [dwTlsIndex - 64]);
    }
  }

  return nullptr;
}


SK_TlsRecord*
SK_GetTLSEx (SK_TLS** ppTLS, bool no_create = false)
{
  auto& pTLS =
    *ppTLS;

  ULONG dwTLSIndex =
    ReadULongAcquire (&__SK_TLS_INDEX);

  pTLS =
    static_cast <SK_TLS *> (
      FlsGetValue (dwTLSIndex)
    );


  //
  // If TLS allocation fails, a backing store will be recorded in this
  //   container. This is a lock-contended data store, so it is important
  //     to try and migrate the data into TLS as quickly as possible.
  //
  static auto& emergency_room =
    __SK_EmergencyTLS.get ();


  if ( pTLS == nullptr &&
       emergency_room.count (SK_Thread_GetCurrentId ()) != 0 )
  {
    pTLS =
      emergency_room [SK_Thread_GetCurrentId ()];

    // Try to store it in TLS again, because this hash map is thread-safe,
    //   but not exactly fast under high contention workloads.
    if (dwTLSIndex > 0 && dwTLSIndex < 1088)
      FlsSetValue ( dwTLSIndex, pTLS );
  }

  // We have no TLS data for this thread, time to allocate one.
  if (pTLS == nullptr)
  {
    if (no_create)
      return nullptr;

    bool success = false;

#ifdef _DEBUG
    if (GetLastError () == ERROR_SUCCESS)
    {
#endif
    InterlockedIncrement (&_SK_IgnoreTLSAlloc);
    try {
      pTLS =
        new SK_TLS (dwTLSIndex);

      if (! FlsSetValue ( dwTLSIndex,
                            pTLS ) )
      {
        // The Win32 API call failed, but did manipulating the TEB manually
        //   work?
        if ( pTLS == SK_TLS_SetValue_NoFail ( dwTLSIndex, pTLS ) )
        {
          // Yay! Catastrophe avoided.
          success = true;
        }

        else {
          emergency_room [GetCurrentThreadId ()] = pTLS;
          // We can't store the TLS index, but it's in a place where it can
          //   be looked up as needed.
          success = true;
          InterlockedIncrement (&_SK_TLS_AllocationFailures);
        }
      }
    }

    catch (std::bad_alloc&)
    {
      pTLS = nullptr;
      InterlockedIncrement (&_SK_TLS_AllocationFailures);
    }
    InterlockedDecrement (&_SK_IgnoreTLSAlloc);

     //if (! success)
     //{
     //  static bool warned = false;
     //
     //  if (! warned)
     //  {
     //    warned = true;
     //    SK_ReleaseAssert (!"TLS Allocation Failed! Falling back to global heap (slow).");
     //  }
     //}
    ////pTLS->scheduler.objects_waited =
    ////  new (std::unordered_map <HANDLE, SK_Sched_ThreadContext::wait_record_s>);
#ifdef _DEBUG
    }
#endif
  }

  if (pTLS != nullptr)
  {
    return
      &pTLS->context_record;
  }

  return nullptr;
}

Concurrency::concurrent_unordered_map <DWORD, SK_TlsRecord *>&
SK_TLS_Map (void)
{
  static concurrency::concurrent_unordered_map <DWORD, SK_TlsRecord *>
    __tls_map (64);

  return
    __tls_map;
}

SK_TlsRecord*
SK_GetTLS (SK_TLS** ppTLS)
{
  return
    SK_GetTLSEx (ppTLS, false);
}


SK_TLS*
SK_CleanupTLS (void)
{
  SK_TLS* pTLS       = nullptr;
  auto    pTLSRecord =
    SK_GetTLSEx (&pTLS, true);

  if ( pTLSRecord       == nullptr ||
       pTLSRecord->pTLS == nullptr ||
                   pTLS == nullptr )
  {
    return
      nullptr;
  }

  if ( pTLS->context_record.pTLS     ==  pTLS &&
       pTLS->context_record.dwTlsIdx != -1 )
  {
    // ...
  }

  if (pTLS->debug.mapped)
  {   pTLS->debug.mapped = false; }

#ifdef _DEBUG
  size_t freed =
#endif
  pTLS->Cleanup (
    SK_TLS_CleanupReason_e::Unload
  );

  if (pTLS->debug.handle != INVALID_HANDLE_VALUE)
  {
    CloseHandle (pTLS->debug.handle);
                 pTLS->debug.handle = INVALID_HANDLE_VALUE;
  }

#ifdef _DEBUG
  SK_LOG0 ( ( L"Freed %zu bytes of temporary heap storage for tid=%x",
                freed, SK_Thread_GetCurrentId () ), L"TLS Memory" );
#endif

  if ( FlsSetValue (
         ReadULongAcquire (&__SK_TLS_INDEX),
           nullptr )
     )
  {
    // ...
  }

  return
    pTLS;
}





void
SK_TLS_LogLeak ( const wchar_t* wszFunc,
                 const wchar_t* wszFile,
                           int line,
                        size_t size )
{
  SK_LOG0 ( ( L"TLS Memory Leak - [%s] < %s:%lu > - (%lu Bytes)",
                               wszFunc, wszFile,
                                          line, (uint32_t)size ),
              L"SK TLS Mem" );
}


SK_TLS*
SK_TLS_Bottom (void)
{
  // This doesn't work for WOW64 executables, so
  //   basically any 32-bit program on a modern
  //     OS has to take the slow path.
#ifdef _M_AMD64
  // ----------- Fast Path ------------
  ULONG dwTLSIndex =
    ReadULongAcquire (&__SK_TLS_INDEX);

  SK_TLS *pTLS =
    SK_TLS_FastTEBLookup (dwTLSIndex);

  if (pTLS != nullptr) return pTLS;
  // ----------- Fast Path ------------
#else
  SK_TLS* pTLS = nullptr;
#endif

  InterlockedIncrement (&_SK_IgnoreTLSAlloc);

  auto pTLSRecord =
    SK_GetTLS (&pTLS);

  if ( (pTLSRecord          != nullptr) &&
       (pTLSRecord->pTLS    != nullptr) &&
     (! pTLSRecord->pTLS->debug.mapped) )
  {
    static auto& tls_map =
      SK_TLS_Map ();

    try
    {
      tls_map [pTLS->debug.tid] =
        &pTLS->context_record;

      pTLS->debug.mapped = true;
    }

    catch (...) {}
  }

  InterlockedDecrement (&_SK_IgnoreTLSAlloc);

  if (pTLSRecord == nullptr)
  {
    return nullptr;
  }

  return
    pTLS;
}

_On_failure_ (_Ret_maybenull_)
SK_TLS*
SK_TLS_BottomEx (DWORD dwTid)
{
  auto& tls_map =
    SK_TLS_Map ();

  auto tls_slot =
    tls_map [dwTid];

  SK_TLS* pTLS = nullptr;

  // NOTE: Do not use SK_SEH_SetTranslator here because it expects TLS to be
  //         setup for the calling thread.
  auto orig_se =
  SK_SEH_ApplyTranslator (SK_FilteringStructuredExceptionTranslator (EXCEPTION_ACCESS_VIOLATION));
  try {
    // If out-of-indexes, then the thread was probably destroyed
    if ( tls_slot                                !=     nullptr &&
         tls_slot->pTLS                          !=     nullptr &&
         tls_slot->pTLS->context_record.dwTlsIdx == ReadULongAcquire (&__SK_TLS_INDEX) )
    {
      pTLS =
        tls_slot->pTLS;
    }
  }

  catch (const SK_SEH_IgnoredException&)
  {
    SK_ReleaseAssert (! (L"Bad TLS"))
  }
  SK_SEH_RemoveTranslator (orig_se);

  //SK_ReleaseAssert (tls_slot != tls_map.cend ())

  return pTLS;
}


SK_ModuleAddrMap::SK_ModuleAddrMap (void) = default;

bool
SK_ModuleAddrMap::contains (LPCVOID pAddr, HMODULE* phMod)
{
  if (pResolved == nullptr)
      pResolved = new std::unordered_map <LPCVOID, HMODULE> ();

  std::unordered_map <LPCVOID, HMODULE> *pResolved_ =
    ((std::unordered_map <LPCVOID, HMODULE> *)pResolved);

  const auto it =
    pResolved_->find (pAddr);

  if (it != pResolved_->cend ())
  {
    *phMod = (*pResolved_) [pAddr];
    return true;
  }

  return false;
}

void
SK_ModuleAddrMap::insert (LPCVOID pAddr, HMODULE hMod)
{
  if (pResolved == nullptr)
    pResolved = new std::unordered_map <LPCVOID, HMODULE> ( );

  std::unordered_map <LPCVOID, HMODULE> *pResolved_ =
    ((std::unordered_map <LPCVOID, HMODULE> *)pResolved);

  (*pResolved_) [pAddr] = hMod;
}

#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/steam_api.h>

void*
SK_ImGui_ThreadContext::allocPolylineStorage (size_t needed)
{
  if (polyline_capacity < needed || polyline_storage == nullptr)
  {
    if (polyline_storage != nullptr && polyline_capacity > 0)
      _aligned_free (polyline_storage);
                     polyline_storage = _aligned_malloc (needed, 16);

    if (polyline_storage != nullptr)
      polyline_capacity = needed;
    else
      polyline_capacity = 0;
  }

  return polyline_storage;
}

char*
SK_OSD_ThreadContext::allocText (size_t needed)
{
  if (text_capacity < needed || text == nullptr)
  {
    if (text != nullptr && text_capacity > 0)
      _aligned_free (text);
                     text =
    (char *)_aligned_malloc (needed, 16);

    if (text != nullptr)
      text_capacity = needed;
    else
      text_capacity = 0;
  }

  return text;
}

wchar_t*
SK_Steam_ThreadContext::allocScratchText (size_t needed)
{
  if (text_capacity < needed || text == nullptr)
  {
    if (text != nullptr && text_capacity > 0)
      _aligned_free (text);

    text =
      (wchar_t *)_aligned_malloc (
        needed * sizeof (wchar_t), 16
      );

    if (text != nullptr)
      text_capacity = needed;
    else
      text_capacity = 0;
  }

  return text;
}


uint8_t*
SK_RawInput_ThreadContext::allocData (size_t needed)
{
  if (capacity < needed || data == nullptr)
  {
    if (data != nullptr && capacity > 0)
      _aligned_free (data);
                     data =
      (uint8_t *)_aligned_malloc (needed, 16);

    if (data != nullptr)
      capacity = needed;
    else
      capacity = 0;
  }

  return (uint8_t *)data;
}

RAWINPUTDEVICE*
SK_RawInput_ThreadContext::allocateDevices (size_t needed)
{
  if (num_devices < needed || devices == nullptr)
  {
    if (             devices != nullptr &&
                 num_devices > 0           )
      _aligned_free (devices);
                     devices =
    (RAWINPUTDEVICE *)_aligned_malloc (needed * sizeof (RAWINPUTDEVICE), 16);

    if (devices != nullptr)
      num_devices = needed;
    else
      num_devices = 0;
  }

  return devices;
}


size_t
SK_TLS_DynamicContext::Cleanup (SK_TLS_CleanupReason_e reason)
{
  UNREFERENCED_PARAMETER (reason);

  return 0;
}

size_t
SK_TLS_ScratchMemory::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0UL;

  freed += cmd.reclaim                  ();
  freed += sym_resolve.reclaim          ();
  freed += eula.reclaim                 ();
  freed += cpu_info.reclaim             ();
  freed += log.formatted_output.reclaim ();

  for ( auto* segment : { &ini.key, &ini.val, &ini.sec } )
  {
    if (segment != nullptr)
    {
      freed += segment->reclaim ();
    }
  }

  return freed;
}

size_t
SK_TLS_ScratchMemoryLocal::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0UL;

  freed += query [0].NtInfo.reclaim ();
  freed += query [1].NtInfo.reclaim ();

  return freed;
}

size_t
SK_ImGui_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (polyline_capacity > 0)
  {
    if (polyline_storage != nullptr)
    {
      freed += polyline_capacity;

      _aligned_free (polyline_storage);
                     polyline_storage = nullptr;

               polyline_capacity = 0;
    }

    else
    {
      SK_TLS_LogLeak (__FUNCTIONW__, __FILEW__, __LINE__, polyline_capacity);
      polyline_capacity = 0;
    }
  }

  return freed;
}

size_t
SK_OSD_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (text_capacity > 0)
  {
    if (text != nullptr)
    {
      freed += text_capacity;

      _aligned_free (text);
                     text = nullptr;

            text_capacity = 0;
    }

    else
    {
      SK_TLS_LogLeak (__FUNCTIONW__, __FILEW__, __LINE__, text_capacity);
                                                          text_capacity = 0;
    }
  }

  return freed;
}

size_t
SK_RawInput_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (capacity > 0)
  {
    if (data != nullptr)
    {
      freed += capacity;

      _aligned_free (data);
                     data = nullptr;

            capacity = 0;
    }

    else
    {
      SK_TLS_LogLeak (__FUNCTIONW__, __FILEW__, __LINE__, capacity);
                                                          capacity = 0;
    }
  }

  if (num_devices > 0)
  {
    if (devices != nullptr)
    {
      freed += num_devices * sizeof (RAWINPUTDEVICE);

      _aligned_free (devices);
                     devices = nullptr;

            num_devices = 0;
    }

    else
    {
      SK_TLS_LogLeak (__FUNCTIONW__, __FILEW__, __LINE__, num_devices * sizeof (RAWINPUTDEVICE));
      num_devices = 0;
    }
  }

  return freed;
}

size_t
SK_Steam_ThreadContext::Cleanup (SK_TLS_CleanupReason_e reason)
{
  size_t freed = 0;

  if (reason == Unload)
  {
    if (steam_ctx.Utils () != nullptr && client_user != 0 && reason == Unload)
    {
      if (steam_ctx.ReleaseThreadUser ())
      {
      //freed += 4;

        client_user = 0;
      }
    }

    if (steam_ctx.Utils () != nullptr && client_pipe != 0 && reason == Unload)
    {
      if (steam_ctx.ReleaseThreadPipe ())
      {
      //freed += 4;

        client_pipe = 0;
      }
    }

    if (text != nullptr && text_capacity > 0)
    {
      freed +=       text_capacity * sizeof (wchar_t);
                     text_capacity = 0;
      _aligned_free (text);
                     text = nullptr;
    }
  }

  return freed;
}

#include <SpecialK/render/backend.h>

int
SK_Win32_ThreadContext::getThreadPriority (bool nocache)
{
  const DWORD _RefreshInterval = 30000UL;

  if (nocache)
  {
    thread_prio            = GetThreadPriority (SK_GetCurrentThread ());
    last_tested_prio.time  = timeGetTime       ();
    last_tested_prio.frame = SK_GetFramesDrawn ();

    return thread_prio;
  }

  if (last_tested_prio.frame < SK_GetFramesDrawn ())
  {
    DWORD dwNow = timeGetTime ();

    if (last_tested_prio.time < dwNow - _RefreshInterval)
    {
      thread_prio            = GetThreadPriority (SK_GetCurrentThread ());
      last_tested_prio.time  = dwNow;
      last_tested_prio.frame = SK_GetFramesDrawn ();
    }
  }

  return thread_prio;
}

void*
SK_D3D9_ThreadContext::allocTempFullscreenStorage (size_t /*dontcare*/)
{
  if (temp_fullscreen == nullptr)
  {
    temp_fullscreen =
      _aligned_malloc ( sizeof (D3DDISPLAYMODEEX), 16 );
  }

  return temp_fullscreen;
}

void*
SK_D3D9_ThreadContext::allocStackScratchStorage (size_t size)
{
  if (stack_scratch.storage == nullptr)
  {
    stack_scratch.storage =
                         _aligned_malloc (size, 16);

    if (stack_scratch.storage != nullptr)
    {
        stack_scratch.size = (uint32_t)size;
      RtlZeroMemory (
        stack_scratch.storage, size);
    }
  }

  else
  {
    if (stack_scratch.size < size)
    {
      if (             stack_scratch.size > 0              )
        _aligned_free (stack_scratch.storage);
                       stack_scratch.storage = _aligned_malloc (size, 16);

      if (stack_scratch.storage != nullptr)
      {
                       stack_scratch.size = (uint32_t)size;
        RtlZeroMemory (stack_scratch.storage,         size);
      } else SK_ReleaseAssert (false)
    }
  }

  return stack_scratch.storage;
}

size_t
SK_D3D9_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (stack_scratch.size > 0)
  {
    if (stack_scratch.storage != nullptr)
    {
      _aligned_free (stack_scratch.storage);
                     stack_scratch.storage = nullptr;

      freed += stack_scratch.size;
               stack_scratch.size = 0;
    }

    else
    {
      SK_TLS_LogLeak (__FUNCTIONW__, __FILEW__, __LINE__, stack_scratch.size);
      stack_scratch.size = 0;
    }
  }

  if (temp_fullscreen != nullptr)
  {
    _aligned_free (temp_fullscreen);
                   temp_fullscreen = nullptr;

    freed += sizeof (D3DDISPLAYMODEEX);
  }

  return freed;
}



extern SK_D3D11_Stateblock_Lite*
SK_D3D11_AllocStateBlock (size_t& size);

extern void
SK_D3D11_FreeStateBlock (SK_D3D11_Stateblock_Lite* sb);

SK_D3D11_Stateblock_Lite*
SK_D3D11_ThreadContext::getStateBlock (void)
{
  if (stateBlock == nullptr)
  {
    stateBlock =
      SK_D3D11_AllocStateBlock (stateBlockSize);
  }

  return stateBlock;
}

size_t
SK_D3D11_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (screenshot.reserve > 0)
  {
    if (screenshot.buffer != nullptr)
    {
      _aligned_free (screenshot.buffer);
            freed += screenshot.reserve;
                     screenshot.buffer = nullptr;
    }
  }

  if (stateBlockSize > 0)
  {
    if (stateBlock != nullptr)
    {
      SK_D3D11_FreeStateBlock (stateBlock);
                               stateBlock = nullptr;

      freed += stateBlockSize;
               stateBlockSize = 0;
    }

    else
    {
      SK_TLS_LogLeak (__FUNCTIONW__, __FILEW__, __LINE__, stateBlockSize);
      stateBlockSize = 0;
    }
  }

  return freed;
}



uint8_t*
SK_D3D11_ThreadContext::allocScreenshotMemory (size_t bytesNeeded)
{
  if (screenshot.buffer != nullptr)
  {
    if (screenshot.reserve < bytesNeeded)
    {
      _aligned_free (screenshot.buffer);

      screenshot.buffer = (uint8_t *)
        _aligned_malloc (bytesNeeded, 16);

      if (screenshot.buffer != nullptr)

      {
        screenshot.reserve = bytesNeeded;
      }
    }
  }

  else
  {
    screenshot.buffer = (uint8_t *)
      _aligned_malloc (bytesNeeded, 16);

    if (screenshot.buffer != nullptr)
    {
      screenshot.reserve = bytesNeeded;
    }
  }

  return
    screenshot.buffer;
}

uint8_t*
SK_DXTex_ThreadContext::alignedAlloc (size_t alignment, size_t elems)
{
  SK_ReleaseAssert (alignment == 16)

  bool new_alloc = true;

  if (buffer == nullptr)
  {
    buffer =
      (uint8_t *)_aligned_malloc (elems, alignment);
                        reserve = elems;
  }

  else
  {
    if (reserve < elems)
    {
      dll_log->Log (L"Growing tid %x's DXTex memory pool from %lu to %lu",
                    SK_Thread_GetCurrentId (), reserve, elems);

      _aligned_free (buffer);
                     buffer = (uint8_t *)_aligned_malloc (elems, alignment);
                     reserve= elems;
    }

    else
      new_alloc = false;
  }

  if (new_alloc)
  {
    last_realloc = timeGetTime ();
    last_trim    = last_realloc;
  }

  return buffer;
}

void
SK_DXTex_ThreadContext::moveAlloc (void)
{
  last_realloc = timeGetTime ();
  last_trim    = last_realloc;

  reserve = 0;
  buffer  = nullptr;
}

bool
SK_DXTex_ThreadContext::tryTrim (void)
{
  if (        reserve > _SlackSpace &&
       timeGetTime () - last_trim   >= _TimeBetweenTrims )
  {
    dll_log->Log (L"Trimming tid %x's DXTex memory pool from %lu to %lu",
                  SK_Thread_GetCurrentId (), reserve, _SlackSpace);

    buffer = static_cast <uint8_t *>              (
      _aligned_realloc (buffer,  _SlackSpace, 16) );
                     reserve   = _SlackSpace;
                     last_trim = timeGetTime ();

    return true;
  }

  return false;
}


size_t
SK_DXTex_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (reserve > 0)
  {
    if (buffer != nullptr)
    {
      _aligned_free (buffer);

      freed += reserve;
               reserve = 0;
    }

    else
    {
      SK_TLS_LogLeak (__FUNCTIONW__, __FILEW__, __LINE__, reserve);
      reserve = 0;
    }
  }

  return freed;
}

size_t
SK_Sched_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  return 0;
}





size_t
SK_TLS::Cleanup (SK_TLS_CleanupReason_e reason)
{
  size_t freed = 0UL;

  freed += d3d9          ->Cleanup (reason);
  freed += imgui         ->Cleanup (reason);
  freed += osd           ->Cleanup (reason);
  freed += raw_input     ->Cleanup (reason);
  freed += scratch_memory->Cleanup (reason);
  freed += local_scratch ->Cleanup (reason);
  freed += steam         ->Cleanup (reason);
  freed += d3d11         ->Cleanup (reason);
  freed += scheduler     ->Cleanup (reason);
  freed += dxtex          .Cleanup (reason);

  if (debug.handle > 0)
  {
    CloseHandle (debug.handle);
                 debug.handle = INVALID_HANDLE_VALUE;
  }


  if ( known_modules.isAllocated () &&
       known_modules->pResolved  != nullptr )
  {
    auto pResolved =
      static_cast <std::unordered_map <LPCVOID, HMODULE>*>
      ( known_modules->pResolved );

    freed +=
      ( sizeof HMODULE *
                   pResolved->size () * 2 );

    delete         pResolved;
    known_modules->pResolved = nullptr;
  }

  // Include the size of the TLS data structure on thread unload
  if (reason == Unload)
  {
    freed += sizeof (SK_TLS);
  }

  return freed;
}