/*
 *  MinHook - The Minimalistic API Hooking Library for x64/x86
 *  Copyright (C) 2009-2016 Tsuda Kageyu.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 *  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MIN_HOOK_H__
#define __MIN_HOOK_H__

#if !(defined _M_IX86) && !(defined _M_X64)
    #error MinHook supports only x86 and x64 systems.
#endif

#include <WinDef.h>

// MinHook Error Codes.
typedef enum MH_STATUS
{
    // Unknown error. Should not be returned.
    MH_UNKNOWN = -1,

    // Successful.
    MH_OK = 0,

    // MinHook is already initialized.
    MH_ERROR_ALREADY_INITIALIZED,

    // MinHook is not initialized yet, or already uninitialized.
    MH_ERROR_NOT_INITIALIZED,

    // The hook for the specified target function is already created.
    MH_ERROR_ALREADY_CREATED,

    // The hook for the specified target function is not created yet.
    MH_ERROR_NOT_CREATED,

    // The hook for the specified target function is already enabled.
    MH_ERROR_ENABLED,

    // The hook for the specified target function is not enabled yet, or already
    // disabled.
    MH_ERROR_DISABLED,

    // The specified pointer is invalid. It points the address of non-allocated
    // and/or non-executable region.
    MH_ERROR_NOT_EXECUTABLE,

    // The specified target function cannot be hooked.
    MH_ERROR_UNSUPPORTED_FUNCTION,

    // Failed to allocate memory.
    MH_ERROR_MEMORY_ALLOC,

    // Failed to change the memory protection.
    MH_ERROR_MEMORY_PROTECT,

    // The specified module is not loaded.
    MH_ERROR_MODULE_NOT_FOUND,

    // The specified function is not found.
    MH_ERROR_FUNCTION_NOT_FOUND
} MH_STATUS;

// Can be passed as a parameter to MH_EnableHook, MH_DisableHook,
// MH_QueueEnableHook or MH_QueueDisableHook.
#define MH_ALL_HOOKS NULL

#ifdef __cplusplus
extern "C" {
#endif

    // Initialize the MinHook library. You must call this function EXACTLY ONCE
    // at the beginning of your program.
    MH_STATUS WINAPI MH_Initialize(VOID);

    // Uninitialize the MinHook library. You must call this function EXACTLY
    // ONCE at the end of your program.
    MH_STATUS WINAPI MH_Uninitialize(VOID);

    // Creates a Hook for the specified target function, in disabled state.
    // Parameters:
    //   pTarget    [in]  A pointer to the target function, which will be
    //                    overridden by the detour function.
    //   pDetour    [in]  A pointer to the detour function, which will override
    //                    the target function.
    //   ppOriginal [out] A pointer to the trampoline function, which will be
    //                    used to call the original target function.
    //                    This parameter can be NULL.
    MH_STATUS WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID *ppOriginal);
    MH_STATUS WINAPI MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, LPVOID *ppOriginal, UINT idx);

    // Creates a Hook for the specified API function, in disabled state.
    // Parameters:
    //   pszModule  [in]  A pointer to the loaded module name which contains the
    //                    target function.
    //   pszTarget  [in]  A pointer to the target function name, which will be
    //                    overridden by the detour function.
    //   pDetour    [in]  A pointer to the detour function, which will override
    //                    the target function.
    //   ppOriginal [out] A pointer to the trampoline function, which will be
    //                    used to call the original target function.
    //                    This parameter can be NULL.
    MH_STATUS WINAPI MH_CreateHookApi(
        LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID *ppOriginal);
    MH_STATUS WINAPI MH_CreateHookApi2(
        LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID *ppOriginal, UINT idx);

    // Creates a Hook for the specified API function, in disabled state.
    // Parameters:
    //   pszModule  [in]  A pointer to the loaded module name which contains the
    //                    target function.
    //   pszTarget  [in]  A pointer to the target function name, which will be
    //                    overridden by the detour function.
    //   pDetour    [in]  A pointer to the detour function, which will override
    //                    the target function.
    //   ppOriginal [out] A pointer to the trampoline function, which will be
    //                    used to call the original target function.
    //                    This parameter can be NULL.
    //   ppTarget   [out] A pointer to the target function, which will be used
    //                    with other functions.
    //                    This parameter can be NULL.
    MH_STATUS WINAPI MH_CreateHookApiEx(
        LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID *ppOriginal, LPVOID *ppTarget);
    MH_STATUS WINAPI MH_CreateHookApiEx2(
        LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID *ppOriginal, LPVOID *ppTarget, UINT idx);

    // Removes an already created hook.
    // Parameters:
    //   pTarget [in] A pointer to the target function.
    MH_STATUS WINAPI MH_RemoveHook   (LPVOID pTarget);
    MH_STATUS WINAPI MH_RemoveHookEx (LPVOID pTarget, UINT idx);

    // Enables an already created hook.
    // Parameters:
    //   pTarget [in] A pointer to the target function.
    //                If this parameter is MH_ALL_HOOKS, all created hooks are
    //                enabled in one go.
    MH_STATUS WINAPI MH_EnableHook   (LPVOID pTarget);
    MH_STATUS WINAPI MH_EnableHookEx (LPVOID pTarget, UINT idx);

    // Disables an already created hook.
    // Parameters:
    //   pTarget [in] A pointer to the target function.
    //                If this parameter is MH_ALL_HOOKS, all created hooks are
    //                disabled in one go.
    MH_STATUS WINAPI MH_DisableHook   (LPVOID pTarget);
    MH_STATUS WINAPI MH_DisableHookEx (LPVOID pTarget, UINT idx);

    // Queues to enable an already created hook.
    // Parameters:
    //   pTarget [in] A pointer to the target function.
    //                If this parameter is MH_ALL_HOOKS, all created hooks are
    //                queued to be enabled.
    MH_STATUS WINAPI MH_QueueEnableHook   (LPVOID pTarget);
    MH_STATUS WINAPI MH_QueueEnableHookEx (LPVOID pTarget, UINT idx);

    // Queues to disable an already created hook.
    // Parameters:
    //   pTarget [in] A pointer to the target function.
    //                If this parameter is MH_ALL_HOOKS, all created hooks are
    //                queued to be disabled.
    MH_STATUS WINAPI MH_QueueDisableHook   (LPVOID pTarget);
    MH_STATUS WINAPI MH_QueueDisableHookEx (LPVOID pTarget, UINT idx);

    // Applies all queued changes in one go.
    MH_STATUS WINAPI MH_ApplyQueued   (VOID);
    MH_STATUS WINAPI MH_ApplyQueuedEx (UINT idx);

    // Translates the MH_STATUS to its name as a string.
    const char * WINAPI MH_StatusToString(MH_STATUS status);


    // SKinHook Introspectable States
    typedef enum SH_INTROSPECT_ID
    {
      // The trampoline function
      SH_TRAMPOLINE     = 0x01,

      // The detour function
      SH_DETOUR         = 0x02,

      // Is the target hotpatched?
      SH_HOTPATCH       = 0x04,

      // Is the hook enabled?
      SH_ENABLED        = 0x08,

      // Is the hook queued to be enabled?
      SH_QUEUED_ENABLE  = 0x10,

      SH_NUM_IPS        = 0x20,
      SH_OLD_IPS        = 0x40,
      SH_NEW_IPS        = 0x80
    } SH_INTROSPECT_ID;

    MH_STATUS WINAPI SH_Introspect (LPVOID pTarget, SH_INTROSPECT_ID type, LPVOID *ppResult);
    MH_STATUS WINAPI SH_HookCount  (PUINT  pHookCount);
    MH_STATUS WINAPI SH_HookTarget (UINT   hook_idx,                       LPVOID *ppTarget);

    MH_STATUS WINAPI SH_IntrospectEx ( LPVOID  pTarget,    SH_INTROSPECT_ID type,
                                       LPVOID *ppResult,   UINT             idx      );
    MH_STATUS WINAPI SH_HookCountEx  ( PUINT   pHookCount, UINT             Idx      );
    MH_STATUS WINAPI SH_HookTargetEx ( UINT    hook_idx,   LPVOID          *ppTarget,
                                       UINT    idx                                   );

#ifdef __cplusplus
}
#endif

#endif /* __MIN_HOOK_H__ */