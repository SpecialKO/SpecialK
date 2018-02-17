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


volatile long __SK_TLS_INDEX = TLS_OUT_OF_INDEXES;


SK_TLS*
__stdcall
SK_TLS_Bottom (void)
{
  if (ReadAcquire (&__SK_TLS_INDEX) == TLS_OUT_OF_INDEXES)
    return nullptr;

  LPVOID lpvData =
    TlsGetValue (ReadAcquire (&__SK_TLS_INDEX));

  if (lpvData == nullptr)
  {
    lpvData =
      static_cast <LPVOID> (
        LocalAlloc ( LPTR,
                       sizeof (SK_TLS) * SK_TLS::stack::max )
      );

    if (lpvData != nullptr)
    {
      if (! TlsSetValue (ReadAcquire (&__SK_TLS_INDEX), lpvData))
      {
        LocalFree (lpvData);
        return nullptr;
      }

      static_cast <SK_TLS *> (lpvData)->stack.current = 0;
    }
  }

  return static_cast <SK_TLS *> (lpvData);
}

SK_TLS*
__stdcall
SK_TLS_Top (void)
{
  if (ReadAcquire (&__SK_TLS_INDEX) == TLS_OUT_OF_INDEXES)
    return nullptr;

  return &(SK_TLS_Bottom ()[SK_TLS_Bottom ()->stack.current]);
}

bool
__stdcall
SK_TLS_Push (void)
{
  if (ReadAcquire (&__SK_TLS_INDEX) == TLS_OUT_OF_INDEXES)
    return false;

  if (SK_TLS_Bottom ()->stack.current < SK_TLS::stack::max)
  {
    static_cast <SK_TLS *>   (SK_TLS_Bottom ())[SK_TLS_Bottom ()->stack.current + 1] =
      static_cast <SK_TLS *> (SK_TLS_Bottom ())[SK_TLS_Bottom ()->stack.current++];

    return true;
  }

  // Overflow
  return false;
}

bool
__stdcall
SK_TLS_Pop  (void)
{
  if (ReadAcquire (&__SK_TLS_INDEX) == TLS_OUT_OF_INDEXES)
    return false;

  if (SK_TLS_Bottom ()->stack.current > 0)
  {
    static_cast <SK_TLS *> (SK_TLS_Bottom ())->stack.current--;

    return true;
  }

  // Underflow
  return false;
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