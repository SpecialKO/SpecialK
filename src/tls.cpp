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

#include <SpecialK/tls.h>
#include <SpecialK/log.h>
#include <SpecialK/config.h>


volatile long __SK_TLS_INDEX = TLS_OUT_OF_INDEXES;


SK_TlsRecord
SK_GetTLS (bool initialize)
{
  DWORD dwTlsIdx =
    ReadAcquire (&__SK_TLS_INDEX);

  LPVOID lpvData =
    nullptr;

  if ( dwTlsIdx != TLS_OUT_OF_INDEXES )
  {
    lpvData =
      TlsGetValue (dwTlsIdx);

    if (lpvData == nullptr)
    {
      if (GetLastError () == ERROR_SUCCESS)
      {
        lpvData =
          static_cast <LPVOID> (
            LocalAlloc (LPTR, sizeof (SK_TLS) * SK_TLS::stack::max)
        );

        if (! TlsSetValue (dwTlsIdx, lpvData))
        {
          LocalFree (lpvData);
                     lpvData = nullptr;
        }

        else initialize = true;
      }
    }

    if (lpvData != nullptr && initialize)
    {
      SK_TLS* pTLS =
        static_cast <SK_TLS *> (lpvData);

      *pTLS = SK_TLS::SK_TLS ();

      // Stack semantics are deprecated and will be removed soon
      pTLS->stack.current = 0;
    }
  }

  else
    dwTlsIdx = TLS_OUT_OF_INDEXES;

  return
    { dwTlsIdx, lpvData };
}


void
SK_CleanupTLS (void)
{
  auto tls_slot =
    SK_GetTLS ();

  if (tls_slot.lpvData != nullptr)
  {
#ifdef _DEBUG
    size_t freed =
#endif
    static_cast <SK_TLS *> (tls_slot.lpvData)->Cleanup (
      SK_TLS_CleanupReason_e::Unload
    );

#ifdef _DEBUG
    SK_LOG0 ( ( L"Freed %zu bytes of temporary heap storage for tid=%x",
                  freed, GetCurrentThreadId () ), L"TLS Memory" );
#endif

    if (TlsSetValue (tls_slot.dwTlsIdx, nullptr))
      LocalFree     (tls_slot.lpvData);
  }
}



SK_TLS*
__stdcall
SK_TLS_Bottom (void)
{
  auto tls_slot =
    SK_GetTLS ();

  if (tls_slot.dwTlsIdx == TLS_OUT_OF_INDEXES)
    return nullptr;

  return
    static_cast <SK_TLS *> (tls_slot.lpvData);
}



char*
SK_TLS_ScratchMemory::cmd_s::allocFormattedStorage (size_t needed_len)
{
  if (len < needed_len)
  {
    line = (char *)realloc (line, needed_len);

    if (line != nullptr)
    {
      len = needed_len;
    }
  }

  return line;
}

char*
SK_TLS_ScratchMemory::eula_s::allocTextStorage (size_t needed_len)
{
  if (len < needed_len)
  {
    text = (char *)realloc (text, needed_len);

    if (text != nullptr)
    {
      len = needed_len;
    }
  }

  return text;
}





SK_ModuleAddrMap::SK_ModuleAddrMap (void) = default;

bool
SK_ModuleAddrMap::contains (LPCVOID pAddr, HMODULE* phMod)
{
  if (pResolved == nullptr)
      pResolved = new std::unordered_map <LPCVOID, HMODULE> ();

  std::unordered_map <LPCVOID, HMODULE> *pResolved_ =
    ((std::unordered_map <LPCVOID, HMODULE> *)pResolved);

  const auto&& it =
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
  if (polyline_capacity < needed)
  {
    polyline_storage = realloc (polyline_storage, needed);

    if (polyline_storage != nullptr)
      polyline_capacity = needed;
  }

  return polyline_storage;
}

char*
SK_OSD_ThreadContext::allocText (size_t needed)
{
  if (text_capacity < needed)
  {
    text = (char *)realloc (text, needed);

    if (text != nullptr)
      text_capacity = needed;
  }

  return text;
}


uint8_t*
SK_RawInput_ThreadContext::allocData (size_t needed)
{
  if (capacity < needed)
  {
    data = (uint8_t *)realloc (data, needed);

    if (data != nullptr)
      capacity = needed;
  }

  return (uint8_t *)data;
}

RAWINPUTDEVICE*
SK_RawInput_ThreadContext::allocateDevices (size_t needed)
{
  if (num_devices < needed)
  {
    devices = (RAWINPUTDEVICE *)realloc (devices, needed * sizeof (RAWINPUTDEVICE));

    if (devices != nullptr)
      num_devices = needed;
  }

  return devices;
}


size_t
SK_TLS_ScratchMemory::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0UL;

  if (cmd.line != nullptr)
  {
    freed += cmd.len;

       free (cmd.line);
             cmd.line = nullptr;

             cmd.len  = 0;
  }

  if (eula.text != nullptr)
  {
    freed += eula.len;

       free (eula.text);
             eula.text = nullptr;

             eula.len  = 0;
  }

  return freed;
}

size_t
SK_ImGui_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (polyline_storage != nullptr)
  {
    freed += polyline_capacity;

    free (polyline_storage);
          polyline_storage = nullptr;

          polyline_capacity = 0;
  }

  return freed;
}

size_t
SK_OSD_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (text != nullptr)
  {
    freed += text_capacity;

    free (text);
          text = nullptr;

          text_capacity = 0;
  }

  return freed;
}

size_t
SK_RawInput_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (data != nullptr)
  {
    freed += capacity;

    free (data);
          data = nullptr;

          capacity = 0;
  }

  if (devices != nullptr)
  {
    freed += num_devices * sizeof (RAWINPUTDEVICE);

    free (devices);
          devices = nullptr;

          num_devices = 0;
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
    thread_prio            = GetThreadPriority (GetCurrentThread ());
    last_tested_prio.time  = timeGetTime ();
    last_tested_prio.frame = SK_GetFramesDrawn ();

    return thread_prio;
  }

  if (last_tested_prio.frame < SK_GetFramesDrawn ())
  {
    DWORD dwNow = timeGetTime ();

    if (last_tested_prio.time < dwNow - _RefreshInterval)
    {
      thread_prio            = GetThreadPriority (GetCurrentThread ());
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
    temp_fullscreen = new D3DDISPLAYMODEEX { };
  }

  return temp_fullscreen;
}

size_t
SK_D3D9_ThreadContext::Cleanup (SK_TLS_CleanupReason_e /*reason*/)
{
  size_t freed = 0;

  if (temp_fullscreen != nullptr)
  {
    delete temp_fullscreen;
           temp_fullscreen = nullptr;

           freed += sizeof (D3DDISPLAYMODEEX);
  }

  return freed;
}


size_t
SK_TLS::Cleanup (SK_TLS_CleanupReason_e reason)
{
  size_t freed = 0UL;

  freed += d3d9          .Cleanup (reason);
  freed += imgui         .Cleanup (reason);
  freed += osd           .Cleanup (reason);
  freed += raw_input     .Cleanup (reason);
  freed += scratch_memory.Cleanup (reason);
  freed += steam         .Cleanup (reason);


  if (known_modules.pResolved != nullptr)
  {
    freed +=
      sizeof HMODULE *
        ((std::unordered_map <LPCVOID, HMODULE> *)known_modules.pResolved)->size () * 2;

    delete known_modules.pResolved;
           known_modules.pResolved = nullptr;
  }

  // Include the size of the TLS data structure on thread unload
  if (reason == Unload)
    freed += sizeof (SK_TLS);

  return freed;
}