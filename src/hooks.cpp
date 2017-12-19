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

#include <Windows.h>

#include <SpecialK/hooks.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>
#include <SpecialK/diagnostics/compatibility.h>

#include <vector>

class SK_HookedFunction
{
public:
  enum Type {
    DLL,
    VFTable,
    Generic,
    Invalid
  };

protected:
  Type             type     = Invalid;
  const char*      name     = "<Initialize_Me>";

  struct {
    uintptr_t      detour   = 0xCaFACADE;
    uintptr_t      original = 0x8badf00d;
    uintptr_t      target   = 0xdeadc0de;
  } addr;

  union {
    struct {
      int_fast16_t idx      = -1;
    } vftbl;

    struct {
      HANDLE       module   = nullptr; // Hold a reference; don't let
                                       //   software unload the DLL while it is
                                       //     hooked!
    } dll;
  };

  bool             enabled  = false;
};

class HookManager {
public:
  bool validateVFTables (void);
  void rehookVFTables   (void);

  void uninstallAll     (void);
  void reinstallAll     (void);
  void install          (SK_HookedFunction* pfn);

std::vector <SK_HookedFunction> hooks;
};

MH_STATUS
__stdcall
SK_EnableHookEx (void *pTarget, UINT idx);

MH_STATUS
__stdcall
SK_CreateFuncHook ( const wchar_t  *pwszFuncName,
                          void     *pTarget,
                          void     *pDetour,
                          void    **ppOriginal )
{
  MH_STATUS status =
    MH_CreateHook ( pTarget,
                      pDetour,
                        ppOriginal );

  if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED)
  {
    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for '%s' "
                  L"[Address: %04ph]!  (Status: \"%hs\")",
                    pwszFuncName,
                      pTarget,
                        MH_StatusToString (status) );
  }

  else if (status == MH_ERROR_ALREADY_CREATED)
  {
    if (MH_OK == (status = MH_RemoveHook (pTarget)))
    {
      dll_log.Log ( L"[HookEngine] Removing Corrupted Hook for '%s'... software "
                    L"is probably going to explode!", pwszFuncName );

      return SK_CreateFuncHook (pwszFuncName, pTarget, pDetour, ppOriginal);
    } else
      dll_log.Log ( L"[ Min Hook ] Failed to Uninstall Hook for '%s' "
                    L"[Address: %04ph]!  (Status: \"%hs\")",
                      pwszFuncName,
                        pTarget,
                          MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
__stdcall
SK_CreateFuncHookEx ( const wchar_t *pwszFuncName,
                            void    *pTarget,
                            void    *pDetour,
                            void   **ppOriginal,
                            UINT     idx )
{
  MH_STATUS status =
    MH_CreateHookEx ( pTarget,
                        pDetour,
                          ppOriginal,
                            idx );

  if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED)
  {
    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook (idx=%lu) "
                  L"for '%s' [Address: %04ph]!  (Status: \"%hs\")",
                    idx,
                      pwszFuncName,
                        pTarget,
                          MH_StatusToString (status) );
  }

  else if (status == MH_ERROR_ALREADY_CREATED)
  {
    if (MH_OK == (status = MH_RemoveHookEx (pTarget, idx)))
    {
      dll_log.Log ( L"[HookEngine] Removing Corrupted Hook for '%s'... software "
                    L"is probably going to explode!", pwszFuncName );

      return SK_CreateFuncHookEx (pwszFuncName, pTarget, pDetour, ppOriginal, idx);
    } else
      dll_log.Log ( L"[ Min Hook ] Failed to Uninstall Hook for '%s' "
                    L"[Address: %04ph]!  (Status: \"%hs\")",
                      pwszFuncName,
                        pTarget,
                          MH_StatusToString (status) );
  }

  return status;
}

typedef struct _MODULEINFO {
  LPVOID lpBaseOfDll;
  DWORD  SizeOfImage;
  LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

typedef BOOL (WINAPI *K32GetModuleInformation_pfn)(HANDLE, HMODULE, LPMODULEINFO, DWORD);

static K32GetModuleInformation_pfn K32GetModuleInformation =
  reinterpret_cast <K32GetModuleInformation_pfn> (
    GetProcAddress ( GetModuleHandle (L"Kernel32.dll"),
                                       "K32GetModuleInformation" )
  );

#include <concurrent_unordered_map.h>

concurrency::concurrent_unordered_map <HMODULE, MODULEINFO> SK_KnownModules;

bool
__stdcall
SK_ValidateHookAddress ( const wchar_t *wszModuleName,
                         const wchar_t *wszHookName,
                               HMODULE  hModule,
                               void    *pHookAddr )
{
  return true;


  MODULEINFO mod_info = { };
  bool       known    = false;


  if (SK_KnownModules.count (hModule))
  {
    mod_info = SK_KnownModules [hModule];
    known    = true;
  }

  else
  {
    known =
      K32GetModuleInformation ( GetCurrentProcess (), hModule,
                                         &mod_info, sizeof MODULEINFO );

    if (known)
      SK_KnownModules [hModule] = mod_info;
  }


  if (known)
  {
    uintptr_t hook_addr  =
      reinterpret_cast <uintptr_t> (
                           pHookAddr
    );
    uintptr_t base_addr =
      reinterpret_cast <uintptr_t> (
                           mod_info.lpBaseOfDll
    );
    uintptr_t end_addr   =
              base_addr  + mod_info.SizeOfImage;


    if ( hook_addr < base_addr || hook_addr > end_addr )
    {
      dll_log.Log ( L"[HookEngine] Function address for '%s' points to module '%s'; expected '%s'",
                    wszHookName, SK_GetModuleFullName ((HMODULE)hook_addr).c_str (),
                                 wszModuleName );
      return false;
    }
  }

  else
    return false;

  return true;
}

bool
__stdcall
SK_ValidateVFTableAddress ( const wchar_t *wszHookName,
                                  void    *pVFTable,
                                  void    *pVFAddr )
{
  return true;


  HMODULE hModVFTable = nullptr;
  HMODULE hModVFAddr  = nullptr;

  GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCWSTR)pVFTable,
                           &hModVFTable );

  GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCWSTR)pVFAddr,
                           &hModVFAddr );

  if (hModVFTable != hModVFAddr)
  {
    dll_log.Log ( L"[HookEngine] VFTable Entry for '%s' found in '%s'; expected '%s'",
                  wszHookName,
                    SK_GetModuleFullName   (hModVFAddr).c_str  (),
                      SK_GetModuleFullName (hModVFTable).c_str ()
                );

    return false;
  }

  return true;
}

MH_STATUS
__stdcall
SK_CreateDLLHook ( const wchar_t  *pwszModule, const char  *pszProcName,
                         void     *pDetour,          void **ppOriginal,
                         void    **ppFuncAddr )
{
  HMODULE hMod = nullptr;

  if (! GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod))
  {
    // In the future, establish queueing capabilities, for now, just pull the DLL in.
    //
    //  >> Pass the library load through the original (now hooked) function so that
    //       anything else that hooks this DLL on-load does not miss its initial load.
    //
    //if (LoadLibraryW (pwszModule))
    //  GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod);
    if (LoadLibraryW (pwszModule))
      GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod);
  }

  void      *pFuncAddr = nullptr;
  MH_STATUS  status    = MH_OK;

  if (hMod == nullptr)
    status = MH_ERROR_MODULE_NOT_FOUND;

  else
  {
    pFuncAddr =
      GetProcAddress (hMod, pszProcName);

    status =
      MH_CreateHook ( pFuncAddr,
                        pDetour,
                          ppOriginal );
  }

  if (status != MH_OK)
  {
    if (status == MH_ERROR_ALREADY_CREATED)
    {
      if (ppOriginal == nullptr)
      {
        SH_Introspect ( pFuncAddr,
                          SH_TRAMPOLINE,
                            ppOriginal );

        dll_log.Log ( L"[ Min Hook ] WARNING: Hook Already Exists for: '%hs' in '%s'! "
                      L"(Status: \"%hs\")",
                        reinterpret_cast <uintptr_t> (pszProcName) > 65536 ? pszProcName : "Ordinal",
                          pwszModule,
                            MH_StatusToString (status) );

        return status;
      }

      else if (MH_OK == (status = MH_RemoveHook (pFuncAddr)))
      {
        dll_log.Log ( L"[HookEngine] Removing Corrupted Hook for '%hs'... software "
                      L"is probably going to explode!", pszProcName );

        return SK_CreateDLLHook (pwszModule, pszProcName, pDetour, ppOriginal, ppFuncAddr);
      }

      else
      {
        dll_log.Log ( L"[ Min Hook ] Failed to Uninstall Hook for '%hs' "
                      L"[Address: %04ph]!  (Status: \"%hs\")",
                        pszProcName,
                          pFuncAddr,
                            MH_StatusToString (status) );
      }
    }

    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for: '%hs' in '%s'! "
                  L"(Status: \"%hs\")",
                    reinterpret_cast <uintptr_t> (pszProcName) > 65536 ? pszProcName : "Ordinal",
                      pwszModule,
                        MH_StatusToString (status) );

    if (ppFuncAddr != nullptr)
      *ppFuncAddr = nullptr;
  }

  else
  {
    HMODULE hModTest = nullptr;

    GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, pwszModule, &hModTest);

    SK_ValidateHookAddress (
           pwszModule,
             SK_UTF8ToWideChar  ((uintptr_t)pszProcName > 65536 ? pszProcName : "Ordinal").c_str (),
               hModTest,
                 pFuncAddr
         );

    if (ppFuncAddr != nullptr)
      *ppFuncAddr = pFuncAddr;
    else
      SK_EnableHook (pFuncAddr);
  }

  return status;
}

MH_STATUS
__stdcall
SK_CreateDLLHook2 ( const wchar_t  *pwszModule, const char  *pszProcName,
                          void     *pDetour,          void **ppOriginal,
                          void    **ppFuncAddr )
{
  HMODULE hMod = nullptr;

  if (! GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod))
  {
    // In the future, establish queuing capabilities, for now, just pull the DLL in.
    //
    //  >> Pass the library load through the original (now hooked) function so that
    //       anything else that hooks this DLL on-load does not miss its initial load.
    //
    if (LoadLibraryW (pwszModule))
      GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod);
  }

  void      *pFuncAddr = nullptr;
  MH_STATUS  status    = MH_OK;

  if (hMod == nullptr)
    status = MH_ERROR_MODULE_NOT_FOUND;

  else
  {
    pFuncAddr =
      GetProcAddress (hMod, pszProcName);

    status =
      MH_CreateHook ( pFuncAddr,
                        pDetour,
                          ppOriginal );
  }

  if (status != MH_OK)
  {
    if (status == MH_ERROR_ALREADY_CREATED)
    {
      if (ppOriginal == nullptr)
      {
        SH_Introspect ( pFuncAddr,
                          SH_TRAMPOLINE,
                            ppOriginal );

        dll_log.Log ( L"[ Min Hook ] WARNING: Hook Already Exists for: '%hs' in '%s'! "
                      L"(Status: \"%hs\")",
                        (uintptr_t)pszProcName > 65536 ? pszProcName : "Ordinal",
                          pwszModule,
                            MH_StatusToString (status) );

        return status;
      }

      else if (MH_OK == (status = MH_RemoveHook (pFuncAddr)))
      {
        dll_log.Log ( L"[HookEngine] Removing Corrupted Hook for '%hs'... software "
                      L"is probably going to explode!", pszProcName );

        return SK_CreateDLLHook2 (pwszModule, pszProcName, pDetour, ppOriginal, ppFuncAddr);
      }

      else
      {
        dll_log.Log ( L"[ Min Hook ] Failed to Uninstall Hook for '%hs' "
                      L"[Address: %04ph]!  (Status: \"%hs\")",
                        pszProcName,
                          pFuncAddr,
                            MH_StatusToString (status) );
      }
    }

    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for: '%hs' in '%s'! "
                  L"(Status: \"%hs\")",
                    (uintptr_t)pszProcName > 65536 ? pszProcName : "Ordinal",
                      pwszModule,
                        MH_StatusToString (status) );

    if (ppFuncAddr != nullptr)
      *ppFuncAddr = nullptr;
  }

  else
  {
    HMODULE hModTest = nullptr;

    GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, pwszModule, &hModTest);

    SK_ValidateHookAddress (
           pwszModule,
             SK_UTF8ToWideChar  ((uintptr_t)pszProcName > 65536 ? pszProcName : "Ordinal").c_str (),
               hModTest,
                 pFuncAddr
         );

    if (ppFuncAddr != nullptr)
      *ppFuncAddr = pFuncAddr;

    MH_QueueEnableHook (pFuncAddr);
  }

  return status;
}

MH_STATUS
__stdcall
SK_CreateDLLHook3 ( const wchar_t  *pwszModule, const char  *pszProcName,
                          void     *pDetour,          void **ppOriginal,
                          void    **ppFuncAddr )
{
  HMODULE hMod = nullptr;

  if (! GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod))
  {
    // In the future, establish queueing capabilities, for now, just pull the DLL in.
    //
    //  >> Pass the library load through the original (now hooked) function so that
    //       anything else that hooks this DLL on-load does not miss its initial load.
    //
    if (LoadLibraryW (pwszModule))
      GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod);
  }

  void      *pFuncAddr = nullptr;
  MH_STATUS  status    = MH_OK;

  if (hMod == nullptr)
    status = MH_ERROR_MODULE_NOT_FOUND;

  else {
    pFuncAddr =
      GetProcAddress (hMod, pszProcName);

    status =
      MH_CreateHook ( pFuncAddr,
                        pDetour,
                          ppOriginal );
  }

  if (status != MH_OK)
  {
    // Silently ignore this problem
    if (status == MH_ERROR_ALREADY_CREATED && ppOriginal != nullptr)
    {
      if (ppFuncAddr != nullptr)
         *ppFuncAddr  = pFuncAddr;

      return MH_OK;
    }

    if (status == MH_ERROR_ALREADY_CREATED)
    {
      if (ppOriginal == nullptr)
      {
        SH_Introspect ( pFuncAddr,
                          SH_TRAMPOLINE,
                            ppOriginal );

        dll_log.Log ( L"[ Min Hook ] WARNING: Hook Already Exists for: '%hs' in '%s'! "
                      L"(Status: \"%hs\")",
                        (uintptr_t)pszProcName > 65536 ? pszProcName : "Ordinal",
                          pwszModule,
                            MH_StatusToString (status) );

        return status;
      }
    }

    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for: '%hs' in '%s'! "
                  L"(Status: \"%hs\")",
                    (uintptr_t)pszProcName > 65536 ? pszProcName : "Ordinal",
                      pwszModule,
                        MH_StatusToString (status) );
  }

  else
  {
    SK_ValidateHookAddress (
           pwszModule,
             SK_UTF8ToWideChar  ((uintptr_t)pszProcName > 65536 ? pszProcName : "Ordinal").c_str (),
               GetModuleHandleW (pwszModule),
                 pFuncAddr
         );

    if (ppFuncAddr != nullptr)
      *ppFuncAddr = pFuncAddr;

    MH_QueueEnableHook (pFuncAddr);
  }

  return status;
}

MH_STATUS
__stdcall
SK_CreateVFTableHook ( const wchar_t  *pwszFuncName,
                             void    **ppVFTable,
                             DWORD     dwOffset,
                             void     *pDetour,
                             void    **ppOriginal )
{
  MH_STATUS ret =
    SK_CreateFuncHook (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal );

  if (ret == MH_OK)
  {
    SK_ValidateVFTableAddress (pwszFuncName, *ppVFTable, ppVFTable [dwOffset]);

    ret = SK_EnableHook (ppVFTable [dwOffset]);
  }

  if (ret != MH_OK)
  {
    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for '%s' "
                  L"[VFTable Index: %lu]!  (Status: \"%hs\")",
                    pwszFuncName,
                      dwOffset,
                        MH_StatusToString (ret) );
  }

  return ret;
}

MH_STATUS
__stdcall
SK_CreateVFTableHookEx ( const wchar_t  *pwszFuncName,
                               void    **ppVFTable,
                               DWORD     dwOffset,
                               void     *pDetour,
                               void    **ppOriginal, 
                               UINT      idx )
{
  MH_STATUS ret =
    SK_CreateFuncHookEx (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal,
              idx );

  if (ret == MH_OK)
  {
    SK_ValidateVFTableAddress (pwszFuncName, *ppVFTable, ppVFTable [dwOffset]);

    ret =
      SK_EnableHookEx (ppVFTable [dwOffset], idx);
  }

  if (ret != MH_OK)
  {
    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for '%s' "
                  L"[VFTable Index: %lu]!  (Status: \"%hs\")",
                    pwszFuncName,
                      dwOffset,
                        MH_StatusToString (ret) );
  }

  return ret;
}

MH_STATUS
__stdcall
SK_CreateVFTableHook2 ( const wchar_t  *pwszFuncName,
                              void    **ppVFTable,
                              DWORD     dwOffset,
                              void     *pDetour,
                              void    **ppOriginal )
{
  MH_STATUS ret =
    SK_CreateFuncHook (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal );

  if (ret == MH_OK)
  {
    SK_ValidateVFTableAddress (pwszFuncName, *ppVFTable, ppVFTable [dwOffset]);

    ret =
      MH_QueueEnableHook (ppVFTable [dwOffset]);
  }

  if (ret != MH_OK)
  {
    dll_log.Log ( L"[ Min Hook ] Failed to Install Hook for '%s' "
                  L"[VFTable Index: %lu]!  (Status: \"%hs\")",
                    pwszFuncName,
                      dwOffset,
                        MH_StatusToString (ret) );
  }

  return ret;
}

MH_STATUS
__stdcall
SK_ApplyQueuedHooks (void)
{
  MH_STATUS status =
    MH_ApplyQueued ();

  if (status != MH_OK)
  {
    dll_log.Log(L"[ Min Hook ] Failed to Enable Deferred Hooks!"
                  L" (Status: \"%hs\")",
                      MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
__stdcall
SK_EnableHook (void *pTarget)
{
  MH_STATUS status =
    MH_EnableHook (pTarget);

  if (status != MH_OK && status != MH_ERROR_ENABLED)
  {
    if (pTarget != MH_ALL_HOOKS)
    {
      dll_log.Log(L"[ Min Hook ] Failed to Enable Hook with Address: %ph!"
                  L" (Status: \"%hs\")",
                    pTarget,
                      MH_StatusToString (status) );
    }

    else
    {
      dll_log.Log ( L"[ Min Hook ] Failed to Enable All Hooks! "
                    L"(Status: \"%hs\")",
                      MH_StatusToString (status) );
    }
  }

  return status;
}

MH_STATUS
__stdcall
SK_EnableHookEx (void *pTarget, UINT idx)
{
  MH_STATUS status =
    MH_EnableHookEx (pTarget, idx);

  if (status != MH_OK && status != MH_ERROR_ENABLED)
  {
    if (pTarget != MH_ALL_HOOKS)
    {
      dll_log.Log ( L"[ Min Hook ] Failed to Enable Hook (Idx=%lu) with "
                    L"Address: %04ph! (Status: \"%hs\")",
                    idx,
                      pTarget,
                        MH_StatusToString (status) );
    }

    else
    {
      dll_log.Log ( L"[ Min Hook ] Failed to Enable All (Idx=%lu) Hooks! "
                    L"(Status: \"%hs\")",
                      idx,
                        MH_StatusToString (status) );
    }
  }

  return status;
}

MH_STATUS
__stdcall
SK_DisableHook (void *pTarget)
{
  MH_STATUS status =
    MH_DisableHook (pTarget);

  if (status != MH_OK && status != MH_ERROR_DISABLED)
  {
    if (pTarget != MH_ALL_HOOKS)
    {
      dll_log.Log ( L"[ Min Hook ] Failed to Disable Hook with Address: %ph!"
                    L" (Status: \"%hs\")",
                      pTarget,
                        MH_StatusToString (status) );
    }

    else
    {
      dll_log.Log ( L"[ Min Hook ] Failed to Disable All Hooks! "
                    L"(Status: \"%hs\")",
                      MH_StatusToString (status) );
    }
  }

  return status;
}

MH_STATUS
__stdcall
SK_RemoveHook (void *pTarget)
{
  MH_STATUS status =
    MH_RemoveHook (pTarget);

  if (status != MH_OK)
  {
    dll_log.Log ( L"[ Min Hook ] Failed to Remove Hook with Address: %ph! "
                  L"(Status: \"%hs\")",
                    pTarget,
                      MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
__stdcall
SK_Init_MinHook (void)
{
  MH_STATUS status;

  if ((status = MH_Initialize ()) != MH_OK)
  {
#if 0
    dll_log.Log ( L"[ Min Hook ] Failed to Initialize MinHook Library! "
                  L"(Status: \"%hs\")",
                    MH_StatusToString (status) );
#endif
  }

  return status;
}

MH_STATUS
__stdcall
SK_UnInit_MinHook (void)
{
  MH_STATUS status;

  std::queue <DWORD> suspended_tids =
    SK_SuspendAllOtherThreads ();

  if ((status = MH_Uninitialize ()) != MH_OK)
  {
    dll_log.Log ( L"[ Min Hook ] Failed to Uninitialize MinHook Library! "
                  L"(Status: \"%hs\")",
                    MH_StatusToString (status) );
  }

  else
  {
    //GetKeyState_Original = GetProcAddress (GetModuleHandle (L""))
  }

  SK_ResumeThreads (suspended_tids);

  return status;
}