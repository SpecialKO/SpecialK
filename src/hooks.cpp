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

struct IUnknown;
#include <Unknwnbase.h>

#include <string>
#include <cinttypes>
#include <SpecialK/ini.h>
#include <SpecialK/core.h>
#include <SpecialK/log.h>
#include <SpecialK/hooks.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>

#include <winternl.h>

#include <vector>
#include <concurrent_unordered_map.h>

HMODULE
SK_GetModuleHandleW (PCWSTR lpModuleName)
{
  if (lpModuleName == nullptr)
    return GetModuleHandleW (nullptr);
  HMODULE hMod = nullptr;

  typedef void (WINAPI *RtlInitUnicodeString_pfn)(
      PUNICODE_STRING DestinationString,
      PCWSTR          SourceString
  );

  typedef NTSTATUS (WINAPI *LdrGetDllHandle_pfn)(
       ULONG,           ULONG,
 const UNICODE_STRING*, HMODULE* );

  static RtlInitUnicodeString_pfn
    RtlInitUnicodeString =
    (RtlInitUnicodeString_pfn) GetProcAddress (
                                 LoadLibraryW ( L"NtDll.dll" ),
                                                  "RtlInitUnicodeString" );

  static LdrGetDllHandle_pfn
         LdrGetDllHandle =
        (LdrGetDllHandle_pfn) GetProcAddress (
                                LoadLibraryW ( L"NtDll.dll" ),
                                                "LdrGetDllHandle" );

  UNICODE_STRING         ucsModuleName          = { };
  RtlInitUnicodeString (&ucsModuleName, lpModuleName);

  LdrGetDllHandle (
    0, 0,
      &ucsModuleName,
        &hMod
  );

  if (hMod != nullptr)
    return hMod;

  return
    GetModuleHandleW (lpModuleName);
}



#define SK_LOG_MINHOOK(status, msg, ...)      \
  SK_LOG0 ( ( msg LR"( (Status: "%hs"))",     \
              ##__VA_ARGS__,                  \
              MH_StatusToString ((status)) ), \
                  L" Min Hook " );

concurrency::concurrent_unordered_map <HMODULE, MODULEINFO> SK_KnownModules;

std::wstring
sk_hook_target_s::serialize_ini (void)
{
  return
    SK_FormatStringW ( L"%s?%x", module_path, offset );
}

std::wstring
sk_hook_target_s::deserialize_ini (const std::wstring& serial_data)
{
  wchar_t wszPath [MAX_PATH + 2] = { };

  std::swscanf ( serial_data.c_str (),
                   L"%260[^?]?%tx",
                     wszPath,
                    &offset );

  HMODULE hModLib =
    SK_GetModuleHandle (wszPath);

  if (hModLib == nullptr)
  {
    hModLib =
      SK_Modules.LoadLibraryLL (wszPath);
  }

  MODULEINFO mod_info = { };

  if ( GetModuleInformation ( GetCurrentProcess (),
                                hModLib, &mod_info,
                                  sizeof MODULEINFO ) )
  {
    if (SK_LoadLibrary_PinModule <wchar_t> (wszPath))
    {
      wcscpy (module_path, wszPath);

      addr =
        (LPVOID)((uintptr_t)hModLib + offset);

      return
        SK_GetDLLVersionStr (module_path);
    }
  }

  addr = nullptr;

  return L"";
}

bool
SK_Hook_PredictTarget (       sk_hook_cache_record_s &cache,
                        const wchar_t                *wszSectionName,
                              iSK_INI                *ini )
{
  if (ini == nullptr)
    return false;

  if (ini->contains_section (wszSectionName))
  {
    iSK_INISection& hook_cfg =
      ini->get_section (wszSectionName);

    std::wstring wide_symbol (
      SK_UTF8ToWideChar (cache.target.symbol_name)
    );

    if (hook_cfg.contains_key (wide_symbol.c_str ()))
    {
      std::wstring ver_str =
        cache.target.deserialize_ini (
          hook_cfg.get_value (wide_symbol.c_str ())
        );

      if ( hook_cfg.contains_key (cache.target.module_path) &&
             ver_str._Equal (
               hook_cfg.get_value (
                          cache.target.module_path
                                  )
                            )
         )
      {
        return true;
      }
    }
  }

  return false;
};

void
SK_Hook_RemoveTarget (       sk_hook_cache_record_s &cache,
                       const wchar_t                *wszSectionName,
                             iSK_INI                *ini )
{
  if (ini == nullptr)
    return;

  if (ini->contains_section (wszSectionName))
  {
    iSK_INISection& hook_cfg =
      ini->get_section (wszSectionName);

    hook_cfg.remove_key (SK_UTF8ToWideChar (cache.target.symbol_name).c_str ());

  // The INI parser can take as many as 1-2 ms to complete this operation,
  //    so don't bother immediately flushing.
  //
  // --> We'll take care of this after we draw our first frame.
  //------------------------------------
  //ini->write ( ini->get_filename () );
  }
}

void
SK_Hook_ResolveTarget ( sk_hook_cache_record_s &cache )
{
  HMODULE hModBase =
    SK_GetModuleFromAddr (cache.target.addr);

  if (hModBase != INVALID_HANDLE_VALUE)
  {
    cache.target.offset =
      (uint64_t)cache.target.addr -
      (uint64_t)hModBase;

    wcsncpy ( cache.target.module_path,
                SK_GetModuleFullNameFromAddr (cache.target.addr).c_str (),
                  MAX_PATH );

    MODULEINFO mod_info = { };

    if ( GetModuleInformation ( GetCurrentProcess (),
                                  hModBase, &mod_info,
                                    sizeof MODULEINFO ) )
    {
      cache.target.image_size = mod_info.SizeOfImage;
    }
  }

  else
  {
    cache.target.offset = 0;
  }
}

void
SK_Hook_CacheTarget (       sk_hook_cache_record_s &cache,
                      const wchar_t                *wszSectionName,
                            iSK_INI                *ini )
{
  if (ini == nullptr)
    return;


  SK_Hook_ResolveTarget (cache);

  if (cache.target.offset != 0)
  {
    cache.hits++;

    const char* szSymbol =
      cache.target.symbol_name;

    //SK_LOG0 ( ( L" DXGI_HOOK <%64hs> [ %s # %li ]",
    //                        szSymbol,
    //  SK_MakePrettyAddress (cache.target.target_addr).c_str (),
    //                        cache.hits ),
    //           L"Hook Cache" );
    std::wstring wide_symbol (SK_UTF8ToWideChar (szSymbol));
    std::wstring serialized  (cache.target.serialize_ini ());

    if (cache.active && wszSectionName)
    {
      iSK_INISection& hook_cfg =
        ini->get_section (wszSectionName);

      if (hook_cfg.contains_key (wide_symbol.c_str ()))
      {
        std::wstring& val =
          hook_cfg.get_value (wide_symbol.c_str ());

        if (val != serialized)
            val  = serialized;
      }

      else
      {
        hook_cfg.add_key_value ( wide_symbol.c_str  (),
                                   serialized.c_str () );
      }

      std::wstring ver_str =
        SK_GetDLLVersionStr (cache.target.module_path);

      if (hook_cfg.contains_key (cache.target.module_path))
      {
        extern std::wstring __stdcall
        SK_GetDLLVersionStr (const wchar_t* wszName);

        std::wstring& val =
          hook_cfg.get_value (cache.target.module_path);

        if (! val._Equal (ver_str))
        {
          ini->remove_section (hook_cfg.name.c_str ());
          val = ver_str;
        }
      }

      else
      {
        hook_cfg.add_key_value ( cache.target.module_path,
                                   ver_str.c_str () );
      }
    }

    //if (wszSectionName)
    //  ini->write ( ini->get_filename () );
  }
};


#ifdef _WIN64
# include <../depends/src/SKinHook/hde/hde64.h>
#else
# include <../depends/src/SKinHook/hde/hde32.h>
#endif

bool
SKX_IsHotPatchableAddr (LPVOID addr)
{
  if (! SK_IsAddressExecutable (addr))
    return false;

#ifdef _WIN64
  hde64s               disasm = { };
  hde64_disasm (addr, &disasm);
#else
  hde32s               disasm = { };
  hde32_disasm (addr, &disasm);
#endif

  return disasm.len > 2 && (disasm.flags & ( F_ERROR | F_IMM16 | F_IMM8 )) == 0;
}


sk_hook_cache_enablement_s
SK_Hook_PreCacheModule ( const wchar_t                                *wszModuleName,
                               std::vector <sk_hook_cache_record_s *> &local_cache,
                               std::vector <sk_hook_cache_record_s *> &global_cache,
                               iSK_INI                                *ini )
{
  if (ini == nullptr)
    return {};


  extern bool __SK_bypass;
          if (__SK_bypass) return sk_hook_cache_enablement_s { };

  UNREFERENCED_PARAMETER (global_cache);

  std::wstring               ini_name     = std::wstring (wszModuleName) +
                                            L".Hooks";
  sk_hook_cache_enablement_s cache_state =
    SK_Hook_IsCacheEnabled (ini_name.c_str (), ini);


  // This first pass will iterate over any records in the DLL's shared
  //   data segment (for global injection).
  //
  if ( SK_IsInjected () && cache_state.use_cached_addresses.global )
  {
    auto it_global = global_cache.begin ();

    for ( auto& it : local_cache )
    {
      it->active = false;

      if ((*it_global)->target.addr != nullptr)
      {
        LPVOID target_addr = (*it_global)->target.addr;

        it->target.addr = nullptr;

        // If hot-patched, then don't bother
        if (! SKX_IsHotPatchableAddr (target_addr))
        {
          SK_LOG1 ( ( L"Discarding global address for '%50hs' (not hot patched)",
                      it->target.symbol_name ),
                      L"Hook Cache" );

          ++it_global;

          continue;
        }

        if (SK_Modules.LoadLibraryLL (it->target.module_path))
        {
          SK_LOG0 ( ( L"Trying global address for '%50hs' :: '%72s'"
                      L" { Last seen in '%s' }",
                                  it->target.symbol_name,
            SK_MakePrettyAddress (    target_addr).c_str (),
               SK_ConcealUserDir (
                    std::wstring (it->target.module_path).data ()) ),
                      L"Hook Cache" );

          if ( MH_CreateHook ( target_addr,
                               it->detour,
                               it->trampoline
                             ) == MH_OK )
          {
            if (MH_QueueEnableHook (target_addr) == MH_OK)
            {
              it->hits        = 1;
              it->active      = true;
              it->target.addr = target_addr;

              ++cache_state.hooks_loaded.from_shared_dll;
            }
          }
        }
      }

      ++it_global;
    }
  }

  // After trying the shared data segment, examine the current game's
  //   INI for any cached addresses and try to load those if needed.
  //
  if ( (SK_IsInjected () && cache_state.use_cached_addresses.global) ||
                            cache_state.use_cached_addresses.local )
  {
    for ( auto& it : local_cache )
    {
      if (! it->active)
      {
        it->target.addr = nullptr;

        if ( SK_Hook_PredictTarget ( *it, ini_name.c_str () ) )
        {
          // If hot-patched, then don't bother
          if (! SKX_IsHotPatchableAddr (it->target.addr))
          {
            SK_LOG1 ( ( L"Discarding  local address for '%50hs' (not hot patched)",
                        it->target.symbol_name ),
                        L"Hook Cache" );
            continue;
          }

          SK_LOG0 ( ( L"Trying  local address for '%50hs' :: '%72s'"
                      L" { Last seen in '%s' }",
                                  it->target.symbol_name,
            SK_MakePrettyAddress (it->target.addr).c_str (),
               SK_ConcealUserDir (
                    std::wstring (it->target.module_path).data ()) ),
                      L"Hook Cache");

          if ( MH_CreateHook ( it->target.addr,
                               it->detour,
                               it->trampoline
                             ) == MH_OK )
          {
            if (MH_QueueEnableHook (it->target.addr) == MH_OK)
            {
              it->active = true;
              ++cache_state.hooks_loaded.from_game_ini;
            }
          }
        }
        else
        {
          it->active = false;
        }
      }
    }
  }

  // It is the calling function's responsibility to do this.
  //if ( cache_state.hooks_loaded.from_game_ini +
  //     cache_state.hooks_loaded.from_shared_dll > 0 )
  //{
  //  SK_ApplyQueuedHooks ();
  //}

  return cache_state;
};


sk_hook_cache_enablement_s
SK_Hook_IsCacheEnabled ( const wchar_t *wszSecName,
                               iSK_INI *ini         )
{
  if (ini == nullptr)
    return {};


  sk_hook_cache_enablement_s ret;

  struct cache_pool_s {
    const wchar_t  *wszName;
          bool     *pEnable;
  } pools [] = { { L"Global", &ret.use_cached_addresses.global },
                 { L"Local",  &ret.use_cached_addresses.local  } };

  if (ini->contains_section (wszSecName))
  {
    iSK_INISection& cfg_sec =
      ini->get_section (wszSecName);

    for ( auto& it : pools )
    {
      std::wstring key_name =
        SK_FormatStringW (L"Enable%sCache", it.wszName);

      if (cfg_sec.contains_key (key_name.c_str ()))
      {
        (*it.pEnable) =
          SK_IsTrue (cfg_sec.get_value (key_name.c_str ()).c_str ());
      }

      else
      {
        if ((! _wcsicmp (wszSecName, L"D3D11.Hooks")) && (! SK_IsInjected ()) )
        {
          (*it.pEnable) = true;
        }

        else
          *it.pEnable = (! _wcsicmp (it.wszName, L"Global"));

        cfg_sec.add_key_value ( key_name.c_str (),
                                  *(it.pEnable) ? L"true" :
                                                  L"false" );
        //ini->write (ini->get_filename ());
      }
    }
  }

  return ret;
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
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    MH_CreateHook ( pTarget,
                      pDetour,
                        ppOriginal );

  if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED)
  {
    SK_LOG_MINHOOK ( status,
                       L"Failed to Install Hook for '%s' "
                       L"[Address: %04ph]! ",
                         pwszFuncName, pTarget );
  }

  else if (status == MH_ERROR_ALREADY_CREATED)
  {
    if (MH_OK == (status = MH_RemoveHook (pTarget)))
    {
      dll_log->Log ( L"[HookEngine] Removing Corrupted Hook for '%s'... software "
                     L"is probably going to explode!", pwszFuncName );

      return SK_CreateFuncHook (pwszFuncName, pTarget, pDetour, ppOriginal);
    } else
    {
      SK_LOG_MINHOOK ( status,
                         L"Failed to Uninstall Hook for '%s' [Address: %04ph]!",
                           pwszFuncName, pTarget );
    }
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
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    MH_CreateHookEx ( pTarget,
                        pDetour,
                          ppOriginal,
                            idx );

  if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED)
  {
    SK_LOG_MINHOOK ( status,
                       L"Failed to Install Hook (idx=%lu) for '%s' [Address: %04ph]!",
                         idx, pwszFuncName, pTarget );
  }

  else if (status == MH_ERROR_ALREADY_CREATED)
  {
    if (MH_OK == (status = MH_RemoveHookEx (pTarget, idx)))
    {
      dll_log->Log ( L"[HookEngine] Removing Corrupted Hook for '%s'... software "
                     L"is probably going to explode!", pwszFuncName );

      return SK_CreateFuncHookEx (pwszFuncName, pTarget, pDetour, ppOriginal, idx);
    } else
    {
      SK_LOG_MINHOOK ( status,
                         L"Failed to Uninstall Hook for '%s' [Address: %04ph]!",
                           pwszFuncName, pTarget );
    }
  }

  return status;
}

bool
__stdcall
SK_ValidateHookAddress ( const wchar_t * /*wszModuleName*/,
                         const wchar_t * /*wszHookName*/,
                               HMODULE   /*hModule*/,
                               void    * /*pHookAddr*/ )
{

  ///MODULEINFO mod_info = { };
  ///bool       known    = false;
  ///
  ///
  ///if (SK_KnownModules.count (hModule))
  ///{
  ///  mod_info = SK_KnownModules [hModule];
  ///  known    = true;
  ///}
  ///
  ///else
  ///{
  ///  using K32GetModuleInformation_pfn = BOOL (WINAPI *)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
  ///
  ///  static K32GetModuleInformation_pfn K32GetModuleInformation =
  ///    reinterpret_cast <K32GetModuleInformation_pfn> (
  ///      SK_GetProcAddress ( L"kernel32",
  ///                           "K32GetModuleInformation" )
  ///      );
  ///
  ///  known =
  ///    K32GetModuleInformation ( GetCurrentProcess (), hModule,
  ///                                       &mod_info, sizeof MODULEINFO );
  ///
  ///  if (known)
  ///    SK_KnownModules [hModule] = mod_info;
  ///}
  ///
  ///
  ///if (known)
  ///{
  ///  auto hook_addr  =
  ///    reinterpret_cast <uintptr_t> (
  ///                         pHookAddr
  ///  );
  ///  auto base_addr =
  ///    reinterpret_cast <uintptr_t> (
  ///                         mod_info.lpBaseOfDll
  ///  );
  ///  uintptr_t end_addr   =
  ///            base_addr  + mod_info.SizeOfImage;
  ///
  ///
  ///  if ( hook_addr < base_addr || hook_addr > end_addr )
  ///  {
  ///    dll_log.Log ( L"[HookEngine] Function address for '%s' points to module '%s'; expected '%s'",
  ///                  wszHookName,
  ///                    SK_StripUserNameFromPathW (SK_GetModuleFullName ((HMODULE)hook_addr).data ()),
  ///                      wszModuleName );
  ///    return false;
  ///  }
  ///}
  ///
  ///else
  ///  return false;

  return true;
}

bool
__stdcall
SK_ValidateVFTableAddress ( const wchar_t * /*wszHookName*/,
                                  void    * /*pVFTable*/,
                                  void    * /*pVFAddr*/ )
{
  ////HMODULE hModVFTable = nullptr;
  ////HMODULE hModVFAddr  = nullptr;
  ////
  ////GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
  ////                     GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
  ////                       (LPCWSTR)pVFTable,
  ////                         &hModVFTable );
  ////
  ////GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
  ////                     GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
  ////                       (LPCWSTR)pVFAddr,
  ////                         &hModVFAddr );
  ////
  ////if (hModVFTable != hModVFAddr)
  ////{
  ////  dll_log.Log ( L"[HookEngine] VFTable Entry for '%s' found in '%s'; expected '%s'",
  ////                wszHookName,
  ////                  SK_StripUserNameFromPathW (  SK_GetModuleFullName (hModVFAddr).data  ()),
  ////                    SK_StripUserNameFromPathW (SK_GetModuleFullName (hModVFTable).data () )
  ////              );
  ////
  ////  return false;
  ////}

  return true;
}

MH_STATUS
__stdcall
SK_CreateDLLHook ( const wchar_t  *pwszModule, const char  *pszProcName,
                         void     *pDetour,          void **ppOriginal,
                         void    **ppFuncAddr )
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  HMODULE hMod = nullptr;

  if (! GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod))
  {
    // In the future, establish queueing capabilities, for now, just pull the DLL in.
    //
    //  >> Pass the library load through the original (now hooked) function so that
    //       anything else that hooks this DLL on-load does not miss its initial load.
    //
    hMod =
      SK_Modules.LoadLibrary (pwszModule);

    if (hMod != skModuleRegistry::INVALID_MODULE)

      GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_PIN |
                           GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (wchar_t *)hMod, &hMod );
  }

  void      *pFuncAddr = nullptr;
  MH_STATUS  status    = MH_OK;

  if (hMod == nullptr)
    status = MH_ERROR_MODULE_NOT_FOUND;

  else
  {
    pFuncAddr =
      SK_GetProcAddress (pwszModule, pszProcName);

    status =
      MH_CreateHook ( pFuncAddr,
                        pDetour,
                          ppOriginal );
  }


  if (status != MH_OK)
  {
    // Win32 Quirk  (Procedure Name Strings)
    // ===========   ----------------------
    //
    // If the procedure address fits in the lowest 16-bits, we're referencing it
    //   by ordinal rather than name.
    //
    //   Uninitialized pointers will tend to be reported as ordinals; almost no
    //     modern game is going to be throwing around DLL ordinals...
    //
    const   uintptr_t         ordinal = reinterpret_cast <uintptr_t> (pszProcName);
    char    szProcName [         128] = { };
    wchar_t wszModName [MAX_PATH + 2] = { };

    if (ordinal > 65535)
      strncpy_s (  szProcName, 128,
                  pszProcName, _TRUNCATE );
    else
      snprintf  ( szProcName, 127, "Ordinal%u",
                    gsl::narrow_cast <WORD> ( ordinal & 0xFFFFU ) );

    wcsncpy_s         ( wszModName, MAX_PATH + 2,
                        pwszModule, _TRUNCATE );
    SK_ConcealUserDir ( wszModName );


    if (status == MH_ERROR_ALREADY_CREATED)
    {
      if (ppOriginal == nullptr)
      {
        SH_Introspect ( pFuncAddr,
                          SH_TRAMPOLINE,
                            ppOriginal );

        SK_LOG_MINHOOK ( status,
                           L"WARNING: Hook Already Exists for: '%hs' in '%s'!",
                             szProcName,
                               wszModName );

        return status;
      }

      else if (MH_OK == (status = MH_RemoveHook (pFuncAddr)))
      {
        dll_log->Log ( L"[HookEngine] Removing Corrupted Hook for '%hs'... software "
                       L"is probably going to explode!", szProcName );

        return SK_CreateDLLHook (pwszModule, pszProcName, pDetour, ppOriginal, ppFuncAddr);
      }

      else
      {
        SK_LOG_MINHOOK ( status,
                           L"Failed to Uninstall Hook for '%hs' "
                           L"[Address: %04ph]! ",
                             szProcName,
                              wszModName );
      }
    }

    SK_LOG_MINHOOK ( status,
                       L"Failed to Install Hook for: '%hs' in '%s'!",
                        szProcName,
                          wszModName );

    if (ppFuncAddr != nullptr)
       *ppFuncAddr  = nullptr;
  }

  else
  {
    //HMODULE hModTest = nullptr;
    //
    //GetModuleHandleExW (
    //  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    //    pwszModule,
    //      &hModTest );
    //
    //SK_ValidateHookAddress (
    //  pwszModule,
    //    SK_UTF8ToWideChar (proc_name).c_str (),
    //      hModTest,
    //        pFuncAddr );

    if (ppFuncAddr != nullptr)
      *ppFuncAddr = pFuncAddr;
    else
      SK_EnableHook (pFuncAddr);
  }

  return status;
}

BOOL
WINAPI
SK_Module_IsProcAddrLocal ( HMODULE  hModExpected,
                             LPCSTR  lpProcName,
                            FARPROC  lpProcAddr,
              PLDR_DATA_TABLE_ENTRY *ppldrEntry = nullptr );

MH_STATUS
__stdcall
SK_DisableDLLHook ( const wchar_t *pwszModule,
                    const char    *pszProcName )
{
   LPVOID pFuncAddr =
      SK_GetProcAddress ( pwszModule,
                            pszProcName );

  if (pFuncAddr == nullptr)
    return MH_ERROR_FUNCTION_NOT_FOUND;

  return
    MH_DisableHook (pFuncAddr);
}

MH_STATUS
__stdcall
SK_QueueDisableDLLHook ( const wchar_t *pwszModule,
                         const char    *pszProcName )
{
  LPVOID pFuncAddr =
    SK_GetProcAddress ( pwszModule, pszProcName );

  if (pFuncAddr == nullptr) return MH_ERROR_FUNCTION_NOT_FOUND;

  return
    MH_QueueDisableHook (pFuncAddr);
}

MH_STATUS
__stdcall
SK_CreateDLLHook2 ( const wchar_t  *pwszModule, const char  *pszProcName,
                          void     *pDetour,          void **ppOriginal,
                          void    **ppFuncAddr )
{
  if (   ReadAcquire (&__SK_DLL_Ending) ||
      (! ReadAcquire (&__SK_DLL_Attached)) )
  {
    return MH_ERROR_DISABLED;
  }


  HMODULE hMod = nullptr;

  if (! GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod))
  {
    // In the future, establish queuing capabilities, for now, just pull the DLL in.
    //
    //  >> Pass the library load through the original (now hooked) function so that
    //       anything else that hooks this DLL on-load does not miss its initial load.
    //
    hMod =
      SK_Modules.LoadLibrary (pwszModule);

    if (hMod != skModuleRegistry::INVALID_MODULE)
      GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_PIN |
                           GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (wchar_t *)hMod, &hMod );
  }

  void      *pFuncAddr = nullptr;
  MH_STATUS  status    = MH_OK;

  if (hMod == nullptr)
    status = MH_ERROR_MODULE_NOT_FOUND;

  else
  {
    pFuncAddr =
      SK_GetProcAddress (pwszModule, pszProcName);

    SK_Module_IsProcAddrLocal (
      hMod, pszProcName, (FARPROC)pFuncAddr
    );

    status =
      MH_CreateHook ( pFuncAddr,
                        pDetour,
                          ppOriginal );
  }


  if (status != MH_OK)
  {
    // Win32 Quirk  (Procedure Name Strings)
    // ===========   ----------------------
    //
    // If the procedure address fits in the lowest 16-bits, we're referencing it
    //   by ordinal rather than name.
    //
    //   Uninitialized pointers will tend to be reported as ordinals; almost no
    //     modern game is going to be throwing around DLL ordinals...
    //
    const   uintptr_t         ordinal = reinterpret_cast <uintptr_t> (pszProcName);
    char    szProcName [         128] = { };
    wchar_t wszModName [MAX_PATH + 2] = { };

    if (ordinal > 65535)
      strncpy_s (  szProcName, 128,
                  pszProcName, _TRUNCATE );
    else
      snprintf  ( szProcName, 127, "Ordinal%u",
                    gsl::narrow_cast <WORD> ( ordinal & 0xFFFFU ) );

    wcsncpy_s         ( wszModName, MAX_PATH + 2,
                        pwszModule, _TRUNCATE );
    SK_ConcealUserDir ( wszModName );


    if (status == MH_ERROR_ALREADY_CREATED)
    {
      if (ppOriginal == nullptr)
      {
        SH_Introspect ( pFuncAddr,
                          SH_TRAMPOLINE,
                            ppOriginal );

        SK_LOG_MINHOOK ( status,
                           L"WARNING: Hook Already Exists for: '%hs' in '%s'!",
                             szProcName,
                               wszModName );

        return status;
      }

      else if (MH_OK == (status = MH_RemoveHook (pFuncAddr)))
      {
        dll_log->Log ( L"[HookEngine] Removing Corrupted Hook for '%hs'... software "
                       L"is probably going to explode!", szProcName );

        return SK_CreateDLLHook2 (pwszModule, pszProcName, pDetour, ppOriginal, ppFuncAddr);
      }

      else
      {
        SK_LOG_MINHOOK ( status,
                           L"Failed to Uninstall Hook for '%hs' "
                           L"[Address: %04ph]! ",
                             szProcName,
                               pFuncAddr );
      }
    }

    SK_LOG_MINHOOK ( status,
                       L"Failed to Install Hook for: '%hs' in '%s'!",
                         szProcName,
                           wszModName );

    if (ppFuncAddr != nullptr)
       *ppFuncAddr  = nullptr;
  }

  else
  {
    //HMODULE hModTest = nullptr;
    //
    //GetModuleHandleExW (
    //  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    //    pwszModule,
    //      &hModTest );
    //
    //SK_ValidateHookAddress (
    //  pwszModule,
    //    SK_UTF8ToWideChar (proc_name).c_str (),
    //      hModTest,
    //        pFuncAddr );

    if (ppFuncAddr != nullptr)
       *ppFuncAddr  = pFuncAddr;

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
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  HMODULE hMod = nullptr;

  if (! GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, pwszModule, &hMod))
  {
    // In the future, establish queueing capabilities, for now, just pull the DLL in.
    //
    //  >> Pass the library load through the original (now hooked) function so that
    //       anything else that hooks this DLL on-load does not miss its initial load.
    //
    hMod =
      SK_Modules.LoadLibrary (pwszModule);

    if (hMod != skModuleRegistry::INVALID_MODULE)
      GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_PIN |
                           GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (wchar_t *)hMod, &hMod );
  }

  void      *pFuncAddr = nullptr;
  MH_STATUS  status    = MH_OK;

  if (hMod == nullptr)
    status = MH_ERROR_MODULE_NOT_FOUND;

  else {
    pFuncAddr =
      SK_GetProcAddress (pwszModule, pszProcName);

    status =
      MH_CreateHook ( pFuncAddr,
                        pDetour,
                          ppOriginal );
  }


  if (status != MH_OK)
  {
    // Win32 Quirk  (Procedure Name Strings)
    // ===========   ----------------------
    //
    // If the procedure address fits in the lowest 16-bits, we're referencing it
    //   by ordinal rather than name.
    //
    //   Uninitialized pointers will tend to be reported as ordinals; almost no
    //     modern game is going to be throwing around DLL ordinals...
    //
    const   uintptr_t         ordinal = reinterpret_cast <uintptr_t> (pszProcName);
    char    szProcName [         128] = { };
    wchar_t wszModName [MAX_PATH + 2] = { };

    if (ordinal > 65535)
      strncpy_s (  szProcName, 128,
                  pszProcName, _TRUNCATE );
    else
      snprintf  ( szProcName, 127, "Ordinal%u",
                    gsl::narrow_cast <WORD> ( ordinal & 0xFFFFU ) );

    wcsncpy_s         ( wszModName, MAX_PATH + 2,
                        pwszModule, _TRUNCATE );
    SK_ConcealUserDir ( wszModName );


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

        SK_LOG_MINHOOK ( status,
                           L"WARNING: Hook Already Exists for: '%hs' in '%s'!",
                             szProcName,
                               wszModName );

        return status;
      }
    }

    SK_LOG_MINHOOK ( status,
                       L"Failed to Install Hook for: '%hs' in '%s'!",
                         szProcName,
                           wszModName );
  }

  else
  {
    //SK_ValidateHookAddress (
    //       pwszModule,
    //         proc_name.c_str (),
    //           GetModuleHandleW (pwszModule),
    //             pFuncAddr
    //     );

    if (ppFuncAddr != nullptr)
      *ppFuncAddr = pFuncAddr;

    MH_QueueEnableHook (pFuncAddr);
  }

  return status;
}


MH_STATUS
__stdcall
SK_CreateUser32Hook ( const char  *pszProcName,
                         void     *pDetour,          void **ppOriginal,
                         void    **ppFuncAddr )
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  // Win32u is faster on systems that dispatch system calls through it
  //
  static sk_import_test_s win32u_test [] = { { "win32u.dll", false } };
  static bool             tested         =                   false;

  if (! tested)
  {
    SK_TestImports (SK_GetModuleHandle (L"user32"), win32u_test, sizeof (win32u_test) / sizeof sk_import_test_s);
    tested = true;

    if (! win32u_test [0].used)
    {
      ///SK_LOG0 ( (L" *** WARNING: System's user32.dll does not dispatch system calls through Win32U.DLL ***"),
      ///           L"HookEngine" );
    }
  }

  char    proc_name   [128] = { };
  wchar_t module_name [128] = L"Win32U.DLL";

  ///if (win32u_test [0].used)
  ///{
  ///  strncpy (proc_name, pszProcName, 127);
  ///}
  ///
  ///else
  {
    wcscpy (module_name, L"user32");

    if (StrStrIA (pszProcName, "NtUser"))
      strncpy_s (proc_name, 128, pszProcName + 6, _TRUNCATE);
    else
      strncpy_s (proc_name, 128, pszProcName,     _TRUNCATE);
  }

  return
    SK_CreateDLLHook2 (module_name, proc_name, pDetour, ppOriginal, ppFuncAddr);
}



MH_STATUS
__stdcall
SK_CreateVFTableHook ( const wchar_t  *pwszFuncName,
                             void    **ppVFTable,
                             DWORD     dwOffset,
                             void     *pDetour,
                             void    **ppOriginal )
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    SK_CreateFuncHook (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal );

  if (status == MH_OK)
  {
    SK_ValidateVFTableAddress (pwszFuncName, *ppVFTable, ppVFTable [dwOffset]);

    status =
      SK_EnableHook (ppVFTable [dwOffset]);
  }

  if (status != MH_OK)
  {
    SK_LOG_MINHOOK ( status,
                       L"Failed to Install Hook for '%s' [VFTable Index: %lu]! ",
                         pwszFuncName, dwOffset );
  }

  return status;
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
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    SK_CreateFuncHookEx (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal,
              idx );

  if (status == MH_OK)
  {
    SK_ValidateVFTableAddress (pwszFuncName, *ppVFTable, ppVFTable [dwOffset]);

    status =
      SK_EnableHookEx (ppVFTable [dwOffset], idx);
  }

  if (status != MH_OK)
  {
    SK_LOG_MINHOOK ( status,
                       L"Failed to Install Hook for '%s' [VFTable Index: %lu]! ",
                         pwszFuncName, dwOffset );
  }

  return status;
}

MH_STATUS
__stdcall
SK_CreateVFTableHook2 ( const wchar_t  *pwszFuncName,
                              void    **ppVFTable,
                              DWORD     dwOffset,
                              void     *pDetour,
                              void    **ppOriginal )
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    SK_CreateFuncHook (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal );

  if (status == MH_OK)
  {
    SK_ValidateVFTableAddress (pwszFuncName, *ppVFTable, ppVFTable [dwOffset]);

    status =
      MH_QueueEnableHook (ppVFTable [dwOffset]);
  }

  if (status != MH_OK)
  {
    SK_LOG_MINHOOK ( status,
                       L"Failed to Install Hook for '%s' [VFTable Index: %lu]! ",
                         pwszFuncName, dwOffset );
  }

  return status;
}

MH_STATUS
__stdcall
SK_CreateVFTableHook3 ( const wchar_t  *pwszFuncName,
                              void    **ppVFTable,
                              DWORD     dwOffset,
                              void     *pDetour,
                              void    **ppOriginal )
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    SK_CreateFuncHook (
      pwszFuncName,
        ppVFTable [dwOffset],
          pDetour,
            ppOriginal );

  if (status == MH_OK)
  {
    SK_ValidateVFTableAddress (pwszFuncName, *ppVFTable, ppVFTable [dwOffset]);

    //status =
    //  MH_QueueEnableHook (ppVFTable [dwOffset]);
  }

  if (status != MH_OK)
  {
    SK_LOG_MINHOOK ( status,
                       L"Failed to Install Hook for '%s' [VFTable Index: %lu]! ",
                         pwszFuncName, dwOffset );
  }

  return status;
}

#ifdef _DEBUG
#define SK_IsDebug() true
#else
#define SK_IsDebug() false
#endif

MH_STATUS
__stdcall
SK_ApplyQueuedHooks (void)
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


#ifdef _DEBUG
  SK_LOG_CALL (" Min Hook ");
#else
  if (config.system.log_level > 0) SK_LOG_CALL (" Min Hook ");
#endif

  DWORD dwStart = timeGetTime ();

  MH_STATUS status =
    MH_ApplyQueued ();

  if (status != MH_OK)
  {
    SK_LOG_MINHOOK ( status, L"Failed to Enable Deferred Hooks!", 0 );
  }

  const int lvl =
    SK_IsDebug () ? 0 : 1;

  SK_LOG ((L" >> %lu ms", timeGetTime () - dwStart), lvl, L" Min Hook ");

  return status;
}

MH_STATUS
__stdcall
SK_EnableHook (void *pTarget)
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    MH_EnableHook (pTarget);

  if (status != MH_OK && status != MH_ERROR_ENABLED)
  {
    if (pTarget != MH_ALL_HOOKS)
    {
      SK_LOG_MINHOOK ( status,
                         L"Failed to Enable Hook with Address: %04ph!",
                           pTarget );
    }

    else
    {
      SK_LOG_MINHOOK ( status, L"Failed to Enable All Hooks!", 0 );
    }
  }

  return status;
}

MH_STATUS
__stdcall
SK_EnableHookEx (void *pTarget, UINT idx)
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    MH_EnableHookEx (pTarget, idx);

  if (status != MH_OK && status != MH_ERROR_ENABLED)
  {
    if (pTarget != MH_ALL_HOOKS)
    {
      SK_LOG_MINHOOK ( status,
                         L"Failed to Enable Hook (Idx=%lu) with "
                         L"Address: %04ph!",
                           idx, pTarget );
    }

    else
    {
      SK_LOG_MINHOOK ( status, L"Failed to Enable All (Idx=%lu) Hooks!", idx );
    }
  }

  return status;
}

MH_STATUS
__stdcall
SK_DisableHook (void *pTarget)
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    MH_DisableHook (pTarget);

  if (status != MH_OK && status != MH_ERROR_DISABLED)
  {
    if (pTarget != MH_ALL_HOOKS)
    {
      SK_LOG_MINHOOK ( status, L"Failed to Disable Hook with Address: %08"
                               PRIxPTR L"h!",
                         (uintptr_t)pTarget );
    }

    else
    {
      SK_LOG_MINHOOK ( status, L"Failed to Disable All Hooks!", 0 );
    }
  }

  return status;
}

MH_STATUS
__stdcall
SK_RemoveHook (void *pTarget)
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status =
    MH_RemoveHook (pTarget);

  if (status != MH_OK)
  {
    SK_LOG_MINHOOK ( status, L"Failed to Remove Hook with Address: %08"
                             PRIxPTR L"h!",
                       (uintptr_t)pTarget );
  }

  return status;
}

MH_STATUS
__stdcall
SK_MinHook_Init (void)
{
  if (ReadAcquire (&__SK_DLL_Ending) || (! ReadAcquire (&__SK_DLL_Attached)))
    return MH_ERROR_DISABLED;


  MH_STATUS status;

  if ((status = MH_Initialize ()) != MH_OK)
  {
#if 0
    dll_log.Log ( L"[ Min Hook ] Failed to Initialize MinHook Library!"
                  LR"( (Status: "%hs"))",
                    MH_StatusToString (status) );
#endif
  }

  return status;
}

MH_STATUS
__stdcall
SK_MinHook_UnInit (void)
{
  if (! ReadAcquire (&__SK_DLL_Attached))
    return MH_ERROR_NOT_INITIALIZED;


  MH_STATUS status;

  if ((status = MH_Uninitialize ()) != MH_OK)
  {
    SK_LOG_MINHOOK ( status, L"Failed to Uninitialize MinHook Library!",0);
  }

  return status;
}


#define SK_LEGACY_DEPRECATE_PUBLIC_API(old_fn,new_fn)                     \
  SK_LOG0 ( ( L"WARNING: '%ws' is deprecated, please use: '%ws' instead.",\
              ##old_fn, ##new_fn ),                                       \
              L"Deprecated" );                                            \
            return new_fn

extern "C"
__declspec (dllexport)
MH_STATUS
__stdcall
SK_UnInit_MinHook (void)
{
  SK_LEGACY_DEPRECATE_PUBLIC_API (__FUNCTIONW__, SK_MinHook_UnInit ());
}

extern "C"
__declspec (dllexport)
MH_STATUS
__stdcall
SK_Init_MinHook (void)
{
  SK_LEGACY_DEPRECATE_PUBLIC_API (__FUNCTIONW__, SK_MinHook_Init ());
}