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


volatile LONG __SK_TLS_INDEX = TLS_OUT_OF_INDEXES;


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
SK_TLS::imgui_tls_util_s::allocPolylineStorage (size_t needed)
{
  if (polyline_capacity < needed)
  {
    polyline_storage = realloc (polyline_storage, needed);

    if (polyline_storage != nullptr)
      polyline_capacity = needed;
  }

  return polyline_storage;
}

size_t
SK_TLS::Cleanup (SK_TLS::cleanup_reason_e reason)
{
  size_t freed = 0UL;

  if (d3d9.temp_fullscreen != nullptr)
  {
    delete d3d9.temp_fullscreen;
           d3d9.temp_fullscreen = nullptr;

           freed += sizeof (D3DDISPLAYMODEEX);
  }

  if (imgui.polyline_storage != nullptr)
  {

    freed += imgui.polyline_capacity;

    delete imgui.polyline_storage;
           imgui.polyline_storage = nullptr;
  }


  if (reason == Unload)
  {
    if (steam_ctx.Utils () != nullptr && steam.client_user != 0 && reason == Unload)
    {
      if (steam_ctx.ReleaseThreadUser ())
        steam.client_user = 0;
    }

    if (steam_ctx.Utils () != nullptr && steam.client_pipe != 0 && reason == Unload)
    {
      if (steam_ctx.ReleaseThreadPipe ())
        steam.client_pipe = 0;
    }
  }


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