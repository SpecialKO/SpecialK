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

#ifndef __SK__MEMORY_H__
#define __SK__MEMORY_H__

#include <minwindef.h>

void
SK_Memory_InitHooks (void);

void
SK_Memory_RemoveHooks (void);

typedef HGLOBAL (WINAPI *GlobalAlloc_pfn)(
  _In_ UINT   uFlags,
  _In_ SIZE_T dwBytes
);

typedef HLOCAL (WINAPI *LocalAlloc_pfn)(
  _In_ UINT   uFlags,
  _In_ SIZE_T uBytes
);

typedef LPVOID (WINAPI *RtlAllocateHeap_pfn)(
  _In_ HANDLE hHeap,
  _In_ DWORD  dwFlags,
  _In_ SIZE_T dwBytes
);

typedef LPVOID (WINAPI *VirtualAlloc_pfn)(
  _In_opt_ LPVOID lpAddress,
  _In_     SIZE_T dwSize,
  _In_     DWORD  flAllocationType,
  _In_     DWORD  flProtect
);

typedef HGLOBAL (WINAPI *GlobalFree_pfn)(
  _In_ HGLOBAL hMem
);

typedef HLOCAL (WINAPI *LocalFree_pfn)(
  _In_ HLOCAL hMem
);

typedef BOOL (WINAPI *VirtualFree_pfn)(
  _In_ LPVOID lpAddress,
  _In_ SIZE_T dwSize,
  _In_ DWORD  dwFreeType
);

typedef BOOL (WINAPI *HeapFree_pfn)(
  _In_ HANDLE hHeap,
  _In_ DWORD  dwFlags,
  _In_ LPVOID lpMem
);

  extern GlobalAlloc_pfn  GlobalAlloc_Original;
  extern GlobalFree_pfn   GlobalFree_Original;

  extern LocalAlloc_pfn   LocalAlloc_Original;
  extern LocalFree_pfn    LocalFree_Original;

  extern VirtualAlloc_pfn VirtualAlloc_Original;
  extern VirtualFree_pfn  VirtualFree_Original;

  extern RtlAllocateHeap_pfn RtlAllocaeHeap_Original;
  extern HeapFree_pfn        HeapFree_Original;



BOOL
WINAPI
SK_VirtualFree           (
  _In_ LPVOID lpAddress,
  _In_ SIZE_T dwSize,
  _In_ DWORD  dwFreeType );

LPVOID
WINAPI
SK_VirtualAlloc (
  _In_opt_ LPVOID lpAddress,
  _In_     SIZE_T dwSize,
  _In_     DWORD  flAllocationType,
  _In_     DWORD  flProtect);


#endif /* __SK__MEMORY_H__ */