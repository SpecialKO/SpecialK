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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Streamline"

using  sl_DXGIGetDebugInterface1_pfn = HRESULT (WINAPI *)(UINT   Flags, REFIID riid, _Out_ void **pDebug);
static sl_DXGIGetDebugInterface1_pfn
       sl_DXGIGetDebugInterface1_Original = nullptr;

HRESULT
WINAPI
sl_DXGIGetDebugInterface1_Detour (
        UINT   Flags,
        REFIID riid,
  _Out_ void   **pDebug )
{
  wchar_t                wszGUID [41] = { };
  StringFromGUID2 (riid, wszGUID, 40);

  SK_LOGi0 ( L"sl_DXGIGetDebugInterface1 (%x, %ws, %p)", Flags, riid, pDebug );

  return
    sl_DXGIGetDebugInterface1_Original (Flags, riid, pDebug);
}

void
SK_Streamline_InitBypass (void)
{
#if 0
  SK_CreateDLLHook2 ( L"sl.interposer.dll",
                       "DXGIGetDebugInterface1",
                     sl_DXGIGetDebugInterface1_Detour,
           (void **)&sl_DXGIGetDebugInterface1_Original );

  SK_ApplyQueuedHooks ();
#endif
}