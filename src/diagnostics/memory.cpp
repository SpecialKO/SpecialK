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
#include <SpecialK/config.h>
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
       volatile LONG  _SK_IgnoreTLSAlloc = 0;
extern volatile LONG __SK_DLL_Attached;

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
    if (ReadAcquire (&__SK_DLL_Attached) &&
        ReadAcquire (&_SK_IgnoreTLSAlloc) == 0)
    {
      SK_TLS* pTLS =
        SK_TLS_Bottom ();

      if (pTLS != nullptr)
      {
        InterlockedAdd64 (&pTLS->memory.global_bytes, dwBytes);
      }
    }
  }

  return hgRet;
}


HLOCAL
WINAPI
SK_LocalAlloc (
  _In_ UINT   uFlags,
  _In_ SIZE_T uBytes )
{
  if (LocalAlloc_Original != nullptr)
    return LocalAlloc_Original (uFlags, uBytes);

  return LocalAlloc (uFlags, uBytes);
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
    if (ReadAcquire (&__SK_DLL_Attached)  &&
        ReadAcquire (&_SK_IgnoreTLSAlloc) == 0)
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
  LPVOID lpRet =
    RtlAllocateHeap_Original (hHeap, dwFlags, dwBytes);

  if (lpRet != nullptr)
  {
    if ( ReadAcquire (&__SK_DLL_Attached)  &&
         ReadAcquire (&_SK_IgnoreTLSAlloc) == 0 )
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
    if (ReadAcquire (&__SK_DLL_Attached)  &&
        ReadAcquire (&_SK_IgnoreTLSAlloc) == 0)
    {
      SK_TLS* pTLS = 
        SK_TLS_Bottom ();
      
      if (pTLS != nullptr)
      {
        InterlockedAdd64 (&pTLS->memory.virtual_bytes, dwSize);
      }
    }
  }

  return lpRet;
}


LPVOID
WINAPI
SK_VirtualAlloc (
  _In_opt_ LPVOID lpAddress,
  _In_     SIZE_T dwSize,
  _In_     DWORD  flAllocationType,
  _In_     DWORD  flProtect)
{
  if ( VirtualAlloc_Original != nullptr )
    return VirtualAlloc_Original (lpAddress, dwSize, flAllocationType, flProtect);

  return VirtualAlloc (lpAddress, dwSize, flAllocationType, flProtect);
}


HGLOBAL
WINAPI
GlobalFree_Detour   (
  _In_ HGLOBAL hMem ) 
{
  return
    GlobalFree_Original (hMem);
}

HLOCAL
WINAPI
SK_LocalFree       (
  _In_ HLOCAL hMem )
{
  if (LocalFree_Original != nullptr)
    return LocalFree_Original (hMem);

  return LocalFree (hMem);
}

HLOCAL
WINAPI
LocalFree_Detour   (
  _In_ HLOCAL hMem ) 
{
  return
    LocalFree_Original (hMem);
}

BOOL
WINAPI
SK_VirtualFree           (
  _In_ LPVOID lpAddress,
  _In_ SIZE_T dwSize,
  _In_ DWORD  dwFreeType )
{
  if (VirtualFree_Original != nullptr)
    return VirtualFree_Original (lpAddress, dwSize, dwFreeType);

  return VirtualFree (lpAddress, dwSize, dwFreeType);
}

BOOL
WINAPI
VirtualFree_Detour       (
  _In_ LPVOID lpAddress,
  _In_ SIZE_T dwSize,
  _In_ DWORD  dwFreeType )
{
  const BOOL bRet =
    VirtualFree_Original (lpAddress, dwSize, dwFreeType);

  if (bRet)
  {
    if (ReadAcquire (&__SK_DLL_Attached) &&
        ReadAcquire (&_SK_IgnoreTLSAlloc) == 0)
    {
      SK_TLS* pTLS =
        SK_TLS_Bottom ();

      if (pTLS != nullptr)
      {
        InterlockedAdd64 (&pTLS->memory.virtual_bytes, -(LONG64) dwSize);
      }
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
  return
    HeapFree_Original (hHeap, dwFlags, lpMem);
}



void
SK_Memory_InitHooks (void)
{
  if (! config.threads.enable_mem_alloc_trace)
    return;

  SK_CreateDLLHook2 (      L"kernel32",
                            "LocalAlloc",
                             LocalAlloc_Detour,
    static_cast_p2p <void> (&LocalAlloc_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "GlobalAlloc",
                             GlobalAlloc_Detour,
    static_cast_p2p <void> (&GlobalAlloc_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "VirtualAlloc",
                             VirtualAlloc_Detour,
    static_cast_p2p <void> (&VirtualAlloc_Original) );

  SK_CreateDLLHook2 (      L"NtDll.dll",
                            "RtlAllocateHeap",
                             RtlAllocateHeap_Detour,
    static_cast_p2p <void> (&RtlAllocateHeap_Original) );


  SK_CreateDLLHook2 (      L"kernel32",
                            "LocalFree",
                             LocalFree_Detour,
    static_cast_p2p <void> (&LocalFree_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "GlobalFree",
                             GlobalFree_Detour,
    static_cast_p2p <void> (&GlobalFree_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "VirtualFree",
                             VirtualFree_Detour,
    static_cast_p2p <void> (&VirtualFree_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "HeapFree",
                             HeapFree_Detour,
    static_cast_p2p <void> (&HeapFree_Original) );
}

void
SK_Memory_RemoveHooks (void)
{
  SK_QueueDisableDLLHook ( L"kernel32",
                            "LocalAlloc" );

  SK_QueueDisableDLLHook ( L"kernel32",
                            "GlobalAlloc" );

  SK_QueueDisableDLLHook ( L"kernel32",
                            "VirtualAlloc" );

  SK_QueueDisableDLLHook ( L"NtDll.dll",
                            "RtlAllocateHeap" );

  SK_QueueDisableDLLHook ( L"kernel32",
                            "LocalFree" );

  SK_QueueDisableDLLHook ( L"kernel32",
                            "GlobalFree" );

  SK_QueueDisableDLLHook ( L"kernel32",
                            "VirtualFree" );

  SK_QueueDisableDLLHook ( L"kernel32",
                            "HeapFree" );

  LocalAlloc_Original      = nullptr;
  GlobalAlloc_Original     = nullptr;
  VirtualAlloc_Original    = nullptr;
  RtlAllocateHeap_Original = nullptr;
  LocalFree_Original       = nullptr;
  GlobalFree_Original      = nullptr;
  VirtualFree_Original     = nullptr;
  HeapFree_Original        = nullptr;

  SK_ApplyQueuedHooks ();
} 