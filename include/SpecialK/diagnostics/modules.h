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

#ifndef __SK__MODULES_H__
#define __SK__MODULES_H__

#include <SpecialK/diagnostics/load_library.h>

#include <SpecialK/utility/lazy_global.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/core.h>
#include <mutex>
#include <minwindef.h>

#ifndef _INC_SHLWAPI
#include <Shlwapi.h>
#endif


#define SK_GetModuleHandle SK_GetModuleHandleW
HMODULE SK_GetModuleHandleW (PCWSTR lpModuleName);


enum class SK_ModuleEnum {
  PreLoad    = 0x0,
  PostLoad   = 0x1,
  Checkpoint = 0x2
};

void
__stdcall
SK_EnumLoadedModules (SK_ModuleEnum when = SK_ModuleEnum::PreLoad);

#include <map>
#include <utility>
#include <cassert>
#include <unordered_map>

#ifdef __cplusplus
extern "C" {
#endif

  #pragma pack (push,8)
#ifndef _WINDEF_
typedef void *HANDLE;

#define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name

DECLARE_HANDLE(HINSTANCE);
typedef        HINSTANCE HMODULE;
#endif

typedef struct _MODULEINFO {
  LPVOID lpBaseOfDll;
  DWORD  SizeOfImage;
  LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

BOOL
WINAPI
GetModuleInformation
( _In_  HANDLE       hProcess,
  _In_  HMODULE      hModule,
  _Out_ LPMODULEINFO lpmodinfo,
  _In_  DWORD        cb );

BOOL
WINAPI
EnumProcessModules
( _In_                    HANDLE   hProcess,
  _Out_writes_bytes_ (cb) HMODULE *lphModule,
  _In_                    DWORD    cb,
  _Out_                   LPDWORD  lpcbNeeded );

typedef struct _PROCESS_MEMORY_COUNTERS {
  DWORD  cb;
  DWORD  PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;

typedef  PROCESS_MEMORY_COUNTERS
        *PPROCESS_MEMORY_COUNTERS;

BOOL
WINAPI
GetProcessMemoryInfo
( HANDLE                     Process,
  PPROCESS_MEMORY_COUNTERS ppsmemCounters,
  DWORD                      cb );

#pragma pack (pop)
#ifdef __cplusplus
};
#endif


__inline
SK_Thread_HybridSpinlock*
SK_DLL_LoaderLockGuard (void)
{
  static SK_Thread_HybridSpinlock  static_loader (15);
  static SK_Thread_HybridSpinlock* loader_lock;

  static volatile LONG               __init  =  0;
  if (! InterlockedCompareExchange (&__init, 1, 0))
  {
    if (loader_lock == nullptr)
        loader_lock  = &static_loader;

    InterlockedIncrement (&__init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&__init, 2);

  return
    loader_lock;
}


class skWin32Module
{
public:
   using _AddressRange = std::pair <LPVOID, LPVOID>;


   skWin32Module (void) noexcept            : hMod_ (Uninitialized),
                                              refs_ (0) { };

   skWin32Module ( HMODULE        hModWin32,
                   MODULEINFO&    info,
                   const wchar_t* name    ) noexcept : hMod_ (hModWin32), refs_ (1)
   {
     base_ = info.lpBaseOfDll;
     size_ = info.SizeOfImage;
     wcsncpy_s (name_, MAX_PATH * 2, name, _TRUNCATE);

     AddRef ();
   };

   skWin32Module ( HMODULE hModWin32 ) noexcept : hMod_ (hModWin32), refs_ (1)
   {
     DWORD dwNameLen=0;    BOOL       bHasValidInfo              = FALSE;
                           MODULEINFO mod_info                   = {   };
                           wchar_t    wszName [MAX_PATH * 2 + 1] = {   };
     dwNameLen     =
       GetModuleFileNameW (hModWin32, wszName, MAX_PATH * 2);

     bHasValidInfo =
       GetModuleInformation (
         GetCurrentProcess (), hModWin32, &mod_info, sizeof (mod_info)
       );

     assert (dwNameLen > 0 && dwNameLen <= MAX_PATH);
     assert (bHasValidInfo != FALSE);

     *this =
       std::move (skWin32Module (hModWin32, mod_info, wszName));
   };

  ~skWin32Module (void) noexcept///
  {
    const LONG refs =
      ReadAcquire (&refs_);

    assert ( refs == 0        || refs <= 2              );
    //       Pinned / Unowned || Owned and About to Die
    //          ... then there's you

    // Last remaining (owned) reference
    if (refs == 1)
    {
      Release ();
    }

    cache_name_.reset ();
  }


  LONG AddRef  (void) noexcept;
  LONG Release (void) noexcept;


  // Assigning an arbitrary HMODULE causes us to track, but not
  //   assume ownership of the Win32 module
  const skWin32Module&
  operator= (const HMODULE hModWin32) noexcept
  {
    if (hMod_ != Uninitialized) {
      Release (); assert (ReadAcquire (&refs_) == 0);
               // iff~ => you just leaked a module!
    }

    *this =
      skWin32Module (hModWin32);

    return *this;
  }

  operator const HMODULE&       (void) const noexcept {
    return hMod_;
  };
  operator const _AddressRange& (void) const noexcept {
    return
      std::make_pair ( base_,
                         reinterpret_cast   <LPVOID>    (
                           reinterpret_cast <uintptr_t>   (base_) + size_
                                                        )
                     );
  }
  operator const
  std::wstring& (void)
  {
    if (cache_name_ != nullptr)
      return *cache_name_.get ();

    cache_name_
      = std::make_shared <std::wstring> (name_);

    return
      *cache_name_;
  }
  operator const wchar_t*       (void) const noexcept {
    return name_;
  }


  static constexpr HMODULE  Uninitialized = nullptr;
  static constexpr LONG     Unreferenced  =  0;
  static constexpr LPVOID   Unaddressable = nullptr;
  static constexpr size_t   Unallocated   =  0;
  static constexpr const wchar_t*
                      const Unnamed       = L"";


protected:
           HMODULE      hMod_  { Uninitialized };
           LPVOID       base_  { Unaddressable };
           size_t       size_  { Unallocated   };
  volatile LONG         refs_  { Unreferenced  };
           wchar_t      name_ [MAX_PATH * 2 + 1] = { };
  std::shared_ptr <std::wstring>
                        cache_name_              = nullptr;
};


#ifdef _DEBUG
#define SK_DBG_ONLY(x)
#else
#define SK_DBG_ONLY(x) (void)0;
#endif

class skModuleRegistry
{
public:
  skModuleRegistry (void) noexcept
  {
    _known_module_bases.reserve (96);
    _known_module_names.reserve (96);
    _loaded_libraries.reserve   (96);
  }

  static constexpr HMODULE INVALID_MODULE = nullptr;

  inline
  bool
    isValid (const HMODULE hModTest) const noexcept
  {
    return ( hModTest > (HMODULE)0 //&&
                       );// hModTest != INVALID_MODULE );
  }

  HMODULE
    getLibrary ( const wchar_t *wszLibrary, bool add_ref    = true,
                                            bool force_load = true )
  {
    const bool bEmpty =
      _known_module_names.empty ();

    if (bEmpty && (! force_load))
      return INVALID_MODULE;

    HMODULE hMod =
      bEmpty ? INVALID_MODULE :
               _FindLibraryByName (wszLibrary);

    bool ref_added = false;

    if (hMod == INVALID_MODULE && force_load)
    {
      hMod =
        LoadLibraryLL (wszLibrary);

      if (hMod != INVALID_MODULE) ref_added = true;
    }

    if (hMod != INVALID_MODULE)
    {
      if (add_ref == false && ref_added == true)
      {
        std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (
          *SK_DLL_LoaderLockGuard ()
        );
        _loaded_libraries [hMod].Release ();
      }

      return hMod;
    }

    return INVALID_MODULE;
  }

  HMODULE
    getLoadedLibrary ( const wchar_t *wszLibrary, bool add_ref = false )
  {
    if (_known_module_names.empty ())
      return INVALID_MODULE;

    HMODULE hMod =
      _FindLibraryByName (wszLibrary);

    if (hMod == INVALID_MODULE)
    {
      hMod =
        SK_GetModuleHandle (wszLibrary);

      if (hMod != INVALID_MODULE)
        _RegisterLibrary (hMod, wszLibrary);
    }

    if (hMod != INVALID_MODULE)
    {
      if (add_ref == true)
      {
        std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (
          *SK_DLL_LoaderLockGuard ()
        );
        _loaded_libraries [hMod].AddRef ();
      }

      return hMod;
    }

    return INVALID_MODULE;
  }

private:
  // Don't forget to free anything you find!
  HMODULE _FindLibraryByName (const wchar_t *wszLibrary)
  {
    std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (
                    *SK_DLL_LoaderLockGuard ()
    );

    if (_known_module_names.empty ())
      return INVALID_MODULE;

    const auto it =
      _known_module_names.find ( wszLibrary );

    if ( it != _known_module_names.cend () )
    {
      _loaded_libraries [ it->second ].AddRef ();
                 return ( it->second );
    }

    return INVALID_MODULE;
  }

  bool _RegisterLibrary (HMODULE hMod, const wchar_t *wszLibrary)
  {
    MODULEINFO mod_info = { };

    //BOOL bHasValidInfo =
      GetModuleInformation (
        GetCurrentProcess (), hMod, &mod_info, sizeof (MODULEINFO)
      );

    std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (
      *SK_DLL_LoaderLockGuard ()
    );

    //assert (bHasValidInfo); // WTF?

    if ( _loaded_libraries.find (hMod) !=
         _loaded_libraries.cend (    ) )
    {
      return false;
    }

    return
      _loaded_libraries.emplace (hMod,
                  skWin32Module (hMod, mod_info, wszLibrary)
      ).second;
  }

  // Returns INVALID_MODULE if no more references exist
  HMODULE _ReleaseLibrary (skWin32Module& library)
  {
    std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (
                    *SK_DLL_LoaderLockGuard ()
    );

    assert (library != INVALID_MODULE);

    if (library.Release () == 0)
    {
      if (::FreeLibrary (library))
        return INVALID_MODULE;
    }

    return library;
  }

  // With benefits
  friend class
    ::skWin32Module;

public:

  // Special References that are valid for SK's entire lifetime
  static skWin32Module& HostApp (HMODULE hModToSet = skWin32Module::Uninitialized);
  static skWin32Module& Self    (HMODULE hModToSet = skWin32Module::Uninitialized);


public:
  //
  //    ( LL  =  Low-Level  or  Lethal-Liability! )
  //
  // This will bypass SK's DLL monitoring facilities and is capable
  //   of loading a library without SK noticing and analyzing hook
  //     dependencies.
  //
  //  ** More importantly, it can subvert third-party software that is
  //       tracing LoadLibrary (...) calls.
  //
  //   > This is extremely unsafe and should only be used during SK init <
  //
  HMODULE
  LoadLibraryLL (const wchar_t *wszLibrary)
  {
    if (wszLibrary == nullptr)
      return INVALID_MODULE;

    HMODULE hMod =
      _FindLibraryByName (wszLibrary);

    if (hMod != INVALID_MODULE)
      return hMod;

    hMod =
    {
      LoadLibraryW_Original != nullptr     ?
        LoadLibraryW_Original (wszLibrary) :
               ::LoadLibraryW (wszLibrary)
    };

    // We're not fooling anyone, third-party software can see this!
    ///assert (LoadLibraryW_Original != nullptr);

    if (hMod != INVALID_MODULE)
      _RegisterLibrary (hMod, wszLibrary);

    return hMod;
  }

  HMODULE
  LoadSystemLibrary (const wchar_t *wszLibrary)
  {
    if (wszLibrary == nullptr)
      return INVALID_MODULE;

    wchar_t wszFullPath [MAX_PATH * 4] = { };

#ifdef _M_IX86
    GetSystemWow64DirectoryW (wszFullPath, MAX_PATH * 3);
#else
    GetSystemDirectoryW      (wszFullPath, MAX_PATH * 3);
#endif

    PathAppendW (wszFullPath, wszLibrary);

    HMODULE hMod =
      _FindLibraryByName (wszFullPath);

    if (hMod != INVALID_MODULE)
      return hMod;

    hMod =
    {
      LoadLibraryW_Original != nullptr     ?
        LoadLibraryW_Original (wszFullPath) :
               ::LoadLibraryW (wszFullPath)
    };

    // We're not fooling anyone, third-party software can see this!
    ///assert (LoadLibraryW_Original != nullptr);

    if (hMod != INVALID_MODULE)
      _RegisterLibrary (hMod, wszFullPath);

    return hMod;
  }

  HMODULE
  LoadLibrary (const wchar_t *wszLibrary)
  {
    if (wszLibrary == nullptr)
      return INVALID_MODULE;

    HMODULE hMod =
      _FindLibraryByName (wszLibrary);

    if (hMod != INVALID_MODULE)
      return hMod;

    hMod = ::LoadLibraryW (wszLibrary);

    if (hMod != INVALID_MODULE)
      _RegisterLibrary (hMod, wszLibrary);

    return hMod;
  }


  // Win32 API doesn't normally allow for freeing libraries by name, it's
  //   not a good practice... but it's convenient.
  HMODULE
  FreeLibrary (const wchar_t *wszLibrary)
  {
    const auto it =
      _known_module_names.find ( std::wstring (wszLibrary) );

    if (it != _known_module_names.cend ())
    {
      return
        _ReleaseLibrary (_loaded_libraries [it->second]);
    }

    assert (false); // This is why freeing libraries by name is a bad idea!

    return skWin32Module::Uninitialized;
  }

  HMODULE
  FreeLibrary (skWin32Module&& module)
  {
                    assert ((HMODULE)module != 0);
    return _ReleaseLibrary (module);
  }


  // This is severely lacking in thread-safety, but Windows holds a loader
  //   lock on most module manipulation functions anyway.
  //
  //   ** It WOULD be nice to support concurrent module name lookups with
  //      out locking, but this is already more complicated than needed.
protected:
  std::unordered_map  <LPVOID,       HMODULE>        _known_module_bases;
  std::unordered_map  <std::wstring, HMODULE>        _known_module_names;

  std::unordered_map  <HMODULE,      skWin32Module>  _loaded_libraries;
};

extern SK_LazyGlobal <skModuleRegistry> SK_Modules;


#define __SK_hModSelf skModuleRegistry::Self    ()
#define __SK_hModHost skModuleRegistry::HostApp ()

__inline
LONG
skWin32Module::AddRef (void) noexcept
{
  // This would add an actual reference in Win32, but we should be able to
  //   get away with ONE OS-level reference and our own counter.
  //
  ///GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
  ///                      (LPCWSTR)hMod, &hMod );

  InterlockedIncrement (&refs_);

  return refs_;
}

__inline
LONG
skWin32Module::Release (void) noexcept
{
  const LONG ret =
    InterlockedDecrement (&refs_);

  ///if (refs_ == 0)
  ///{
  ///  auto&&         registrar = SK_Modules;
  ///   auto& libs  ( registrar._loaded_libraries   );
  ///   auto& names ( registrar._known_module_names );
  ///   auto& addrs ( registrar._known_module_bases );
  ///
  ///  const BOOL really_gone =
  ///    FreeLibrary (hMod_);
  ///
  ///  const auto& it =
  ///    libs.find (hMod_);
  ///
  ///  if (   it != libs.cend () )
  ///  {
  ///    // All of our references, plus all of the game's references are gone
  ///    if (really_gone)
  ///    {
  ///      // So we have to stop knowing that which is unknowable
  ///      names.erase (it->second);
  ///      addrs.erase (it->second);
  ///    }
  ///
  ///    libs.erase (hMod_);
  ///  }
  ///
  ///  delete this;
  ///}

  return ret;
}

#endif /* __SK__MODULES_H__ */