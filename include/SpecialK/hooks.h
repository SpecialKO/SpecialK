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
SK_CreateFuncHook ( const wchar_t  *pwszFuncName,
                          void     *pTarget,
                          void     *pDetour,
                          void    **ppOriginal );

MH_STATUS
__stdcall
SK_CreateFuncHookEx ( const wchar_t  *pwszFuncName,
                            void     *pTarget,
                            void     *pDetour,
                            void    **ppOriginal,
                            UINT      idx );

MH_STATUS
__stdcall
SK_CreateDLLHook ( const wchar_t  *pwszModule, const char  *pszProcName,
                         void     *pDetour,          void **ppOriginal,
                         void    **ppFuncAddr 
                                     = nullptr );

// Queues a hook rather than enabling it immediately.
MH_STATUS
__stdcall
SK_CreateDLLHook2 ( const wchar_t  *pwszModule, const char  *pszProcName,
                          void     *pDetour,          void **ppOriginal,
                          void    **ppFuncAddr 
                                      = nullptr );

// Queues a hook rather than enabling it immediately.
//   (If already hooked, fails silently)
MH_STATUS
__stdcall
SK_CreateDLLHook3 ( const wchar_t  *pwszModule, const char  *pszProcName,
                          void     *pDetour,          void **ppOriginal,
                          void    **ppFuncAddr
                                      = nullptr );

MH_STATUS
__stdcall
SK_CreateVFTableHook ( const wchar_t  *pwszFuncName,
                             void    **ppVFTable,
                             DWORD     dwOffset,
                             void     *pDetour,
                             void    **ppOriginal );

// Setup multiple hooks for the same function by using different
//   instances of MinHook; this functionality helps with enabling
//     or disabling Special K's OSD in video capture software.
MH_STATUS
__stdcall
SK_CreateVFTableHookEx ( const wchar_t  *pwszFuncName,
                               void    **ppVFTable,
                               DWORD     dwOffset,
                               void     *pDetour,
                               void    **ppOriginal,
                               UINT      idx
                                          = 1 );

// Queues a hook rather than enabling it immediately.
MH_STATUS
__stdcall
SK_CreateVFTableHook2 ( const wchar_t  *pwszFuncName,
                              void    **ppVFTable,
                              DWORD     dwOffset,
                              void     *pDetour,
                              void    **ppOriginal );

MH_STATUS __stdcall SK_ApplyQueuedHooks (void);

MH_STATUS __stdcall SK_EnableHook     (void *pTarget);
MH_STATUS __stdcall SK_EnableHookEx   (void *pTarget, UINT idx);


MH_STATUS __stdcall SK_DisableHook    (void *pTarget);
MH_STATUS __stdcall SK_RemoveHook     (void *pTarget);

MH_STATUS __stdcall SK_Init_MinHook   (void);
MH_STATUS __stdcall SK_UnInit_MinHook (void);

#endif /* __SK__HOOKS_H__ */