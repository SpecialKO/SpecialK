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

struct IUnknown;
#include <Unknwnbase.h>

#undef COM_NO_WINDOWS_H
#include <WinDef.h>
#include <MinHook/MinHook.h>

#include <boost/container/static_vector.hpp>

#include <SpecialK/config.h>
#include <SpecialK/log.h>


#ifndef __SK__INI_H__
struct iSK_INI;
#endif

extern iSK_INI* SK_GetDLLConfig (void);


struct sk_hook_target_s
{
  char      symbol_name [128];          // Expected symbol name (*)
  wchar_t   module_path [MAX_PATH + 2]; // <*> VFTbl hooks do not often resolve

  LPVOID    addr;                       // [ Absolute Addr. ]
  ptrdiff_t offset;                     // Relative to parent module's base addr.

  DWORD     image_size;                 // The correct DLL image size


#ifdef _XSTRING_
  std::wstring serialize_ini   (void);
  std::wstring deserialize_ini (const std::wstring& serial_data);
#endif
};

struct sk_hook_cache_record_s
{
  sk_hook_target_s target;

  LPVOID           detour;
  LPVOID*          trampoline;

  volatile LONG    active;

  // Number of times this record was used (for global -> local caching)
  LONG             hits;
};

#define SK_HookCacheEntry(entry) sk_hook_cache_record_s entry ## = \
  { {#entry, L"", nullptr, 0ULL, 0UL},                             \
     nullptr,                                                      \
     nullptr,                                                      \
     false,                                                        \
     0                                                             \
  };

#define SK_HookCacheEntryGlobal(entry) \
        SK_HookCacheEntry(GlobalHook_##entry)

#define SK_HookCacheEntryLocal(entry,dll,new_target,bounce_pad)\
  sk_hook_cache_record_s LocalHook_##entry =                   \
  { {#entry, (dll), GlobalHook_##entry##.target.addr,          \
                    GlobalHook_##entry##.target.offset,        \
                    GlobalHook_##entry##.target.image_size},   \
     (LPVOID)  (new_target),                                   \
     (LPVOID *)(bounce_pad),                                   \
     false,                                                    \
     0                                                         \
  };

// Load the address and module from an INI file into the cache record
bool
SK_Hook_PredictTarget (       sk_hook_cache_record_s &cache,
                        const wchar_t                *wszSectionName,
                              iSK_INI                *ini = SK_GetDLLConfig () );

// Compute the offset address and module filename (fully qualified path)
void
SK_Hook_ResolveTarget ( sk_hook_cache_record_s &cache );

// Push the address and module in the cache record to an INI file
void
SK_Hook_CacheTarget   ( sk_hook_cache_record_s &cache,
                  const wchar_t                *wszSectionName,
                        iSK_INI                *ini = SK_GetDLLConfig () );

// Erase a cached address
void
SK_Hook_RemoveTarget (       sk_hook_cache_record_s &cache,
                       const wchar_t                *wszSectionName,
                             iSK_INI                *ini = SK_GetDLLConfig () );

static __forceinline
void
SK_Hook_TargetFromVFTable ( sk_hook_cache_record_s  &cache,
                            void                   **base,
                            int                      idx   )
{
  if (  base != nullptr &&
       *base != nullptr )
  {
    cache.active      = TRUE;
    cache.target.addr =
      (*(void***)*(base))[idx]; //-V206
  }

  else
  {
    SK_ReleaseAssert (! L"WTF?! Hooking a NULL VfTable?!");
  }
};


static
auto SK_Hook_PushLocalCacheOntoGlobal =
[]( sk_hook_cache_record_s& local,
    sk_hook_cache_record_s& global )
{
  if (global.hits == 0 && global.target.addr !=
                           local.target.addr)
  {
    global.hits++;
    global.target = local.target;
  }
};

static
auto SK_Hook_PullGlobalCacheDownToLocal =
[]( sk_hook_cache_record_s* global,
    sk_hook_cache_record_s* local   )
{
  if ( local  != nullptr &&
       global != nullptr )
  {
    // LOL, right?
    *local = *global;
  }
  else
  {
    SK_ReleaseAssert (!L"WTF?! Local or Global Hook Tables are Corrupt");
  }
};



struct sk_hook_cache_enablement_s
{
  struct
  {
    bool global = true,
         local  = false;
  } use_cached_addresses;

  struct {
    int  from_game_ini   = 0;
    int  from_shared_dll = 0;
  } hooks_loaded;
};

sk_hook_cache_enablement_s
SK_Hook_IsCacheEnabled ( const wchar_t *wszSecName,
                               iSK_INI *ini = SK_GetDLLConfig () );

using sk_hook_cache_array = boost::container::static_vector <sk_hook_cache_record_s *, 32>;

sk_hook_cache_enablement_s
SK_Hook_PreCacheModule ( const wchar_t             *wszModuleName,
                               sk_hook_cache_array &local_cache,
                               sk_hook_cache_array &global_cache,
                               iSK_INI             *ini = SK_GetDLLConfig () );



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
SK_DisableDLLHook ( const wchar_t *pwszModule,
	                const char    *pszProcName );
MH_STATUS
__stdcall
SK_QueueDisableDLLHook ( const wchar_t *pwszModule,
	                     const char    *pszProcName );


MH_STATUS
__stdcall
SK_CreateUser32Hook ( const char  *pszProcName,
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

MH_STATUS __stdcall SK_MinHook_Init   (void);
MH_STATUS __stdcall SK_MinHook_UnInit (void);

#endif /* __SK__HOOKS_H__ */