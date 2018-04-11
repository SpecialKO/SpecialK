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

#include <SpecialK/TLS.h>
#include <SpecialK/hooks.h>
#include <SpecialK/utility.h>
#include <SpecialK/diagnostics/memory.h>

GlobalAlloc_pfn  GlobalAlloc_Original  = nullptr;
GlobalFree_pfn   GlobalFree_Original   = nullptr;
                                       
LocalAlloc_pfn   LocalAlloc_Original   = nullptr;
LocalFree_pfn    LocalFree_Original    = nullptr;

VirtualAlloc_pfn VirtualAlloc_Original = nullptr;
VirtualFree_pfn  VirtualFree_Original  = nullptr;

RtlAllocateHeap_pfn RtlAllocateHeap_Original = nullptr;
HeapFree_pfn        HeapFree_Original        = nullptr;


// If > 0, then skip memory tracking momentarily because we are going
//   to allocate storage for TLS
volatile LONG _SK_IgnoreTLSAlloc = 0;

HGLOBAL
WINAPI
GlobalAlloc_Detour (
  _In_ UINT   uFlags,
  _In_ SIZE_T dwBytes )
{
  HGLOBAL hgRet =
    GlobalAlloc_Original (uFlags, dwBytes);

  if (hgRet != 0)
  {
    SK_TLS* pTLS = 
      SK_TLS_Bottom ();

    if (pTLS != nullptr)
    {
      InterlockedAdd64 (&pTLS->memory.global_bytes, dwBytes);
    }
  }

  return hgRet;
}

HLOCAL
WINAPI
LocalAlloc_Detour (
  _In_ UINT   uFlags,
  _In_ SIZE_T uBytes )
{
  HLOCAL hlRet =
    LocalAlloc_Original (uFlags, uBytes);

  if (hlRet != 0)
  {
    if (ReadAcquire (&_SK_IgnoreTLSAlloc) == 0)
    {
      SK_TLS* pTLS = 
        SK_TLS_Bottom ();

      if (pTLS != nullptr)
      {
        InterlockedExchangeAdd64 (&pTLS->memory.local_bytes, uBytes);
      }
    }
  }

  return hlRet;
}


LPVOID
WINAPI
RtlAllocateHeap_Detour (
  _In_ HANDLE hHeap,
  _In_ DWORD  dwFlags,
  _In_ SIZE_T dwBytes )
{
  bool bZero = false;
  if (dwFlags & HEAP_ZERO_MEMORY)
  {
    bZero    = true;
    dwFlags &= ~HEAP_ZERO_MEMORY;
  }

  LPVOID lpRet =
    RtlAllocateHeap_Original (hHeap, dwFlags, dwBytes);

  if (lpRet != nullptr)
  {
    if (bZero) RtlZeroMemory (lpRet, dwBytes);

    if (ReadAcquire (&_SK_IgnoreTLSAlloc) == 0)
    {
      SK_TLS* pTLS = 
        SK_TLS_Bottom ();

      if (pTLS != nullptr)
      {
        InterlockedExchangeAdd64 (&pTLS->memory.heap_bytes, dwBytes);
      }
    }
  }

  return lpRet;
}

LPVOID
WINAPI
VirtualAlloc_Detour (
  _In_opt_ LPVOID lpAddress,
  _In_     SIZE_T dwSize,
  _In_     DWORD  flAllocationType,
  _In_     DWORD  flProtect)
{
  LPVOID lpRet =
    VirtualAlloc_Original (lpAddress, dwSize, flAllocationType, flProtect);

  if (lpRet != nullptr)
  {
    SK_TLS* pTLS = 
      SK_TLS_Bottom ();

    if (pTLS != nullptr)
    {
      InterlockedAdd64 (&pTLS->memory.virtual_bytes, dwSize);
    }
  }

  return lpRet;
}


HGLOBAL
WINAPI
GlobalFree_Detour   (
  _In_ HGLOBAL hMem )
{
  return GlobalFree_Original (hMem);
}

HLOCAL
WINAPI
LocalFree_Detour   (
  _In_ HLOCAL hMem )
{
  return LocalFree_Original (hMem);
}

BOOL
WINAPI
VirtualFree_Detour       (
  _In_ LPVOID lpAddress,
  _In_ SIZE_T dwSize,
  _In_ DWORD  dwFreeType )
{
  BOOL bRet =
    VirtualFree_Original (lpAddress, dwSize, dwFreeType);

  if (bRet)
  {
    SK_TLS* pTLS = 
      SK_TLS_Bottom ();

    if (pTLS != nullptr)
    {
      InterlockedAdd64 (&pTLS->memory.virtual_bytes, -(LONG64)dwSize);
    }
  }

  return bRet;
}

BOOL
WINAPI
HeapFree_Detour (
  _In_ HANDLE hHeap,
  _In_ DWORD  dwFlags,
  _In_ LPVOID lpMem )
{
  return HeapFree_Original (hHeap, dwFlags, lpMem);
}



void
SK_Memory_InitHooks (void)
{
  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "LocalAlloc",
                             LocalAlloc_Detour,
    static_cast_p2p <void> (&LocalAlloc_Original) );

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "GlobalAlloc",
                             GlobalAlloc_Detour,
    static_cast_p2p <void> (&GlobalAlloc_Original) );

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "VirtualAlloc",
                             VirtualAlloc_Detour,
    static_cast_p2p <void> (&VirtualAlloc_Original) );

  SK_CreateDLLHook2 (      L"NtDll.dll",
                            "RtlAllocateHeap",
                             RtlAllocateHeap_Detour,
    static_cast_p2p <void> (&RtlAllocateHeap_Original) );


  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "LocalFree",
                             LocalFree_Detour,
    static_cast_p2p <void> (&LocalFree_Original) );

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "GlobalFree",
                             GlobalFree_Detour,
    static_cast_p2p <void> (&GlobalFree_Original) );

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "VirtualFree",
                             VirtualFree_Detour,
    static_cast_p2p <void> (&VirtualFree_Original) );

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "HeapFree",
                             HeapFree_Detour,
    static_cast_p2p <void> (&HeapFree_Original) );
}