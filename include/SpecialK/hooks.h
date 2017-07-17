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

#ifndef __SK__HOOKS_H__
#define __SK__HOOKS_H__

#undef COM_NO_WINDOWS_H
#include <WInDef.h>

#include <MinHook/MinHook.h>

MH_STATUS
__stdcall
SK_CreateFuncHook ( LPCWSTR pwszFuncName,
                    LPVOID  pTarget,
                    LPVOID  pDetour,
                    LPVOID *ppOriginal );

MH_STATUS
__stdcall
SK_CreateFuncHookEx ( LPCWSTR pwszFuncName,
                      LPVOID  pTarget,
                      LPVOID  pDetour,
                      LPVOID *ppOriginal,
                      UINT    idx );

MH_STATUS
__stdcall
SK_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                   LPVOID  pDetour,    LPVOID *ppOriginal,
                   LPVOID *ppFuncAddr = nullptr );

// Queues a hook rather than enabling it immediately.
MH_STATUS
__stdcall
SK_CreateDLLHook2 ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                    LPVOID  pDetour,    LPVOID *ppOriginal,
                    LPVOID *ppFuncAddr = nullptr );

// Queues a hook rather than enabling it immediately.
//   (If already hooked, fails silently)
MH_STATUS
__stdcall
SK_CreateDLLHook3 ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                    LPVOID  pDetour,    LPVOID *ppOriginal,
                    LPVOID *ppFuncAddr = nullptr );

MH_STATUS
__stdcall
SK_CreateVFTableHook ( LPCWSTR pwszFuncName,
                       LPVOID *ppVFTable,
                       DWORD   dwOffset,
                       LPVOID  pDetour,
                       LPVOID *ppOriginal );

// Setup multiple hooks for the same function by using different
//   instances of MinHook; this functionality helps with enabling
//     or disabling Special K's OSD in video capture software.
MH_STATUS
__stdcall
SK_CreateVFTableHookEx ( LPCWSTR pwszFuncName,
                         LPVOID *ppVFTable,
                         DWORD   dwOffset,
                         LPVOID  pDetour,
                         LPVOID *ppOriginal,
                         UINT    idx = 1 );

// Queues a hook rather than enabling it immediately.
MH_STATUS
__stdcall
SK_CreateVFTableHook2 ( LPCWSTR pwszFuncName,
                        LPVOID *ppVFTable,
                        DWORD   dwOffset,
                        LPVOID  pDetour,
                        LPVOID *ppOriginal );

MH_STATUS __stdcall SK_ApplyQueuedHooks (void);

MH_STATUS __stdcall SK_EnableHook   (LPVOID pTarget);
MH_STATUS __stdcall SK_EnableHookEx (LPVOID pTarget, UINT idx);


MH_STATUS __stdcall SK_DisableHook (LPVOID pTarget);
MH_STATUS __stdcall SK_RemoveHook  (LPVOID pTarget);

MH_STATUS __stdcall SK_Init_MinHook   (void);
MH_STATUS __stdcall SK_UnInit_MinHook (void);

#endif /* __SK__HOOKS_H__ */