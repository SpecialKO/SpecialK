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

#include <SpecialK/stdafx.h>
#include <SpecialK/DLL_VERSION.h>

#include <Aux_ulib.h>

#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>

#ifndef _M_AMD64
#include <SpecialK/render/d3d8/d3d8_backend.h>
#include <SpecialK/render/ddraw/ddraw_backend.h>
#endif

const  char*   SK_VersionStrA =    SK_VERSION_STR_A;
const wchar_t* SK_VersionStrW = _L(SK_VERSION_STR_A);

#pragma comment (lib, "Aux_ulib.lib")

char _RTL_CONSTANT_STRING_type_check (const char    *s);
char _RTL_CONSTANT_STRING_type_check (const wchar_t *s);

template <size_t N>
  class _RTL_CONSTANT_STRING_remove_const_template_class;
template <        >
  class _RTL_CONSTANT_STRING_remove_const_template_class <sizeof (char)>
{ public:                                               using T = char;};
template <        >
  class _RTL_CONSTANT_STRING_remove_const_template_class <sizeof (wchar_t)>
{ public:                                               using T = wchar_t;};

#define _RTL_CONSTANT_STRING_remove_const_macro(s)                       \
(const_cast <_RTL_CONSTANT_STRING_remove_const_template_class <          \
                                       sizeof ((s)[0])>::T*> (s))

#define RTL_CONSTANT_STRING(s)                              \
{                                                           \
  sizeof( s ) - sizeof( (s)[0] ),                           \
  sizeof( s ) / sizeof(_RTL_CONSTANT_STRING_type_check(s)), \
  _RTL_CONSTANT_STRING_remove_const_macro(s)                \
}

void SK_LazyCleanup (void)
{
  __SK_LazyRiver->atExit ();
}

SK_LazyGlobal <skModuleRegistry>
              SK_Modules;

SK_Thread_HybridSpinlock* init_mutex          = nullptr;
SK_Thread_HybridSpinlock* budget_mutex        = nullptr;
SK_Thread_HybridSpinlock* wmi_cs              = nullptr;
SK_Thread_HybridSpinlock* cs_dbghelp          = nullptr;
SK_Thread_HybridSpinlock* steam_callback_cs   = nullptr;
SK_Thread_HybridSpinlock* platform_popup_cs   = nullptr;
SK_Thread_HybridSpinlock* steam_init_cs       = nullptr;

                HANDLE __SK_DLL_TeardownEvent = nullptr;

// Various helpful quick watch variables for debugging
volatile          LONG __SK_DLL_Ending        = FALSE;
volatile          LONG __SK_DLL_Attached      = FALSE;
            __time64_t __SK_DLL_AttachTime    = 0ULL;
volatile          LONG __SK_Threads_Attached  = 0UL;
volatile          LONG __SK_DLL_Refs          = 0UL;
volatile          LONG __SK_HookContextOwner  = FALSE;

extern volatile  DWORD __SK_TLS_INDEX;
        volatile LONG  lLastThreadCreate      = 0;
          static bool  _HasLocalDll           = false;


class SK_DLL_Bootstrapper
{
  using BootstrapEntryPoint_pfn = bool (*)(void);
  using BootstrapTerminate_pfn  = bool (*)(void);

public:
  std::vector <const wchar_t *> wrapper_dlls;

  BootstrapEntryPoint_pfn start;
  BootstrapTerminate_pfn  shutdown;
};

SK_LazyGlobal
 < std::unordered_map <
          DLL_ROLE,
       SK_DLL_Bootstrapper
 >  >   __dll_bootstraps;

auto
SK_DLL_GetBootstraps (void)
{
  if (! __dll_bootstraps.isAllocated ())
  {
    __dll_bootstraps.get () =
    {
      { DLL_ROLE::DXGI,       { { L"dxgi.dll",
                                  L"d3d11.dll"    }, SK::DXGI::Startup,
                                                     SK::DXGI::Shutdown   } },
      { DLL_ROLE::D3D11_CASE, { { L"dxgi.dll",
                                  L"d3d11.dll"    }, SK::DXGI::Startup,
                                                     SK::DXGI::Shutdown   } },
      { DLL_ROLE::D3D9,       { { L"d3d9.dll"     }, SK::D3D9::Startup,
                                                     SK::D3D9::Shutdown   } },
      { DLL_ROLE::OpenGL,     { { L"OpenGL32.dll" }, SK::OpenGL::Startup,
                                                     SK::OpenGL::Shutdown } },
      { DLL_ROLE::DInput8,    { { L"dinput8.dll"  }, SK::DI8::Startup,
                                                     SK::DI8::Shutdown    } },
      #ifndef _M_AMD64
      { DLL_ROLE::D3D8,       { { L"d3d8.dll"     }, SK::D3D8::Startup,
                                                     SK::D3D8::Shutdown   } },
      { DLL_ROLE::DDraw,      { { L"ddraw.dll"    }, SK::DDraw::Startup,
                                                     SK::DDraw::Shutdown  } },
      #endif
    };
  }

  return
    __dll_bootstraps.get ();
}

skWin32Module&
skModuleRegistry::HostApp (HMODULE hModToSet)
{
  static skWin32Module hModApp  =
         skWin32Module::Uninitialized;

  if ( hModApp   == skWin32Module::Uninitialized &&
       hModToSet != skWin32Module::Uninitialized    )
  {    hModApp    = hModToSet;                      }
return hModApp;   }

skWin32Module&
skModuleRegistry::Self (HMODULE hModToSet)
{
  static skWin32Module hModSelf =
         skWin32Module::Uninitialized;

  if ( hModSelf  == skWin32Module::Uninitialized &&
       hModToSet != skWin32Module::Uninitialized    )
  {    hModSelf   = hModToSet;                      }
return hModSelf;  }


HMODULE
__stdcall
SK_GetDLL (void)
{
  return
    __SK_hModSelf;
}

std::wstring
__stdcall
SK_GetDLLName (void)
{
  return
    __SK_hModSelf;
}


#include <appmodel.h>

// Various detection methods to prevent injection into software that is
//   either untested or known incompatible.
//
//  * Applies primarily to global injection, and would prevent games from
//      running at all if extended to proxy/wrapper DLL code injection.
//
INT
SK_KeepAway (void)
{
  enum {
    Unlisted    = 0,
    Graylisted  = 1,
    Blacklisted = 2,
    Bluelisted  = 3
  };

  wchar_t             wszDllFullName [MAX_PATH + 2] = { };
  GetModuleFileName ( skModuleRegistry::Self (),
                      wszDllFullName, MAX_PATH    );
  if ( StrStrIW (     wszDllFullName,
           SK_RunLHIfBitness ( 64, L"SpecialK64.dll",
                                   L"SpecialK32.dll" )
                ) == nullptr
     )
  {
    return Unlisted;
  }

  HMODULE hMod =
    skModuleRegistry::HostApp (GetModuleHandle (nullptr));

  wchar_t              wszAppFullName [MAX_PATH + 2] = { };
  GetModuleFileName ( hMod,
                       wszAppFullName, MAX_PATH       );
  if ( StrStrIW (      wszAppFullName, L"rundll32"    )            )
  {
    return Unlisted;
  }

  wchar_t    wszAppShortName                 [MAX_PATH + 2] = { };
  wcsncpy_s (wszAppShortName, wszAppFullName, MAX_PATH);

  PathStripPath (wszAppShortName);

  // If user-interactive, check against an internal blacklist
  #include <SpecialK/injection/blacklist.h>

  static size_t hashed_self =
    hash_lower (wszAppShortName);

  const bool xbox =
    ((StrStrIW (wszAppFullName, L"\\Content\\") != nullptr) || PathFileExistsW (L"gamelaunchhelper.exe"));

  auto _TestUndesirableDll = [&]
   (const std::initializer_list <constexpr_module_s>& list,
                                                  INT list_type) ->
  INT
  {
    if (constexpr_module_s::get (list, hashed_self))
      return list_type;

    // Run through all modules for this list
    if (list_type == Bluelisted)
    {
      static constexpr auto       _MaxModules  = 1024;
      static HMODULE hMods       [_MaxModules] = {  };
      static size_t  hashed_mods [_MaxModules] = {  };
      DWORD          cbNeeded                  =    0;
      unsigned int   i                         =    0;

      if ( EnumProcessModulesEx ( GetCurrentProcess (), hMods,
                                                sizeof (hMods), &cbNeeded, LIST_MODULES_ALL ) )
      {
        cbNeeded =
          std::min ((DWORD)(_MaxModules * sizeof (HMODULE)), cbNeeded);

        for (i = 0; i < (cbNeeded / sizeof (HMODULE)); i++)
        {
          if (hMods [i] != nullptr)
          {
            wchar_t     wszModuleName [MAX_PATH + 2] = { };
            wcsncpy_s ( wszModuleName,
                     SK_GetModuleName (hMods [i]).c_str (),
                                       MAX_PATH );

            PathStripPathW (              wszModuleName);
            hashed_mods [i] = hash_lower (wszModuleName);
          }
        }

        for (i = 0; i < (cbNeeded / sizeof (HMODULE)); i++)
        {
          if (hashed_mods [i] != 0)
          {
            if (constexpr_module_s::get (list, hashed_mods [i]))
            {
              return list_type;
            }
          }
        }
      }
    }

    return Unlisted;
  };

  INT status =
    _TestUndesirableDll ( __blacklist, Blacklisted );

  if (status == Unlisted)
      status =
    _TestUndesirableDll ( __graylist,  Graylisted );

  if (status == Unlisted)
      status =
    _TestUndesirableDll ( __bluelist,  Bluelisted );

  if ( status ==   Unlisted ||
       status == Bluelisted )
  {
    // GetCurrentPackageFullName (Windows 8+)
    using GetCurrentPackageFullName_pfn =
      LONG (WINAPI *)(IN OUT UINT32*, OUT OPTIONAL PWSTR);

    static GetCurrentPackageFullName_pfn
      SK_GetCurrentPackageFullName =
          (GetCurrentPackageFullName_pfn)GetProcAddress (LoadLibraryEx (L"kernel32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32),
          "GetCurrentPackageFullName");

    wchar_t wszPackageName [PACKAGE_FULL_NAME_MAX_LENGTH] = { };
    UINT32  uiLen                                         =  0;
    LONG    rc                                            =  0;
    if (   SK_GetCurrentPackageFullName != nullptr)
      rc = SK_GetCurrentPackageFullName (&uiLen, wszPackageName);

    if ((rc != APPMODEL_ERROR_NO_PACKAGE && (! xbox)) || status == Bluelisted)
    {
      // Epic's Overlay Loads This
      //
      ////if (SK_GetModuleHandleW (L"AppXDeploymentClient.dll") != nullptr)
      ////{
      ////  SK_GetHostAppUtil ()->setBlacklisted (true);
      ////}

      bool gl    = false, vulkan = false, d3d9  = false, d3d11 = false,
           dxgi  = false, d3d8   = false, ddraw = false, d3d12 = false,
           glide = false;

      SK_TestRenderImports (
        hMod,
          &gl, &vulkan,
            &d3d9, &dxgi, &d3d11, &d3d12,
              &d3d8, &ddraw, &glide
      );

      // UWP, oh no!

      if (! (d3d11 || d3d12 || d3d9 || gl))
      {
        // Look for tells that this is a game, if none exist, ignore the UWP trash app
        if (! ( GetModuleHandle (L"hid.dll"      ) ||
                GetModuleHandle (L"dsound.dll"   ) ||
                GetModuleHandle (L"XInput1_4.dll"))) return Graylisted;
      }

      if (status == Bluelisted)
      {
        const auto full_app =
          SK_GetFullyQualifiedApp ();

        // Potential second-chance for explicitly whitelisted apps
        if ( SK_Inject_TestWhitelists (full_app) &&
          (! SK_Inject_TestBlacklists (full_app)))
        {
          status = Unlisted;
        }
      }
    }
  }

  if (status != Unlisted)
    SK_GetHostAppUtil ()->setBlacklisted (true);

  return status;
}

// Ideally, we want to keep the TLS data stores around so that built-in
//   debug features can analyze finished threads. _I see dead threads..._
SK_LazyGlobal <concurrency::concurrent_queue <SK_TLS *>> __SK_TLS_FreeList;

bool
SK_TLS_HasSlot (void) noexcept
{
  const DWORD dwTlsSlot =
    ReadULongAcquire (&__SK_TLS_INDEX);

  return ( dwTlsSlot > 0 &&
           dwTlsSlot < TLS_OUT_OF_INDEXES );
}

DWORD
SK_TLS_Acquire (void) noexcept
{
  if (! SK_TLS_HasSlot ())
  {
    WriteULongRelease (
      &__SK_TLS_INDEX,
        FlsAlloc (nullptr)
      );
  }

  return
    ReadULongAcquire (&__SK_TLS_INDEX);
}

void
SK_TLS_Release (void) noexcept
{
  // This is a kernel limit, I did not make it up
  static constexpr
        DWORD SK_TLS_MAX_IDX = 1088;
  const DWORD dwTlsIdx       =
    InterlockedExchange ( &__SK_TLS_INDEX,
                                TLS_OUT_OF_INDEXES );

  if (dwTlsIdx <= SK_TLS_MAX_IDX)
  {
    FlsFree (dwTlsIdx);
  }
}

bool
SK_DLL_IsAttached (void) noexcept
{
  return
    ( ReadAcquire (&__SK_DLL_Attached) != 0 );
}

void
SK_DLL_SetAttached (bool bAttached) noexcept
{
  WriteRelease ( &__SK_DLL_Attached,
                          bAttached ?
                                1UL : 0UL );
}

int
SK_NT_GetProcessProtection (void)
{
#ifdef SK_AVOID_CET_PROCESSES
  using GetProcessMitigationPolicy_pfn =
    BOOL (WINAPI *)(HANDLE,PROCESS_MITIGATION_POLICY,PVOID,SIZE_T);

  GetProcessMitigationPolicy_pfn
 _GetProcessMitigationPolicy =
 (GetProcessMitigationPolicy_pfn)SK_GetProcAddress ( L"kernel32.dll",
 "GetProcessMitigationPolicy" );

  if (_GetProcessMitigationPolicy != nullptr)
  {
    PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY
      shadow_stack_policy = { };

    if (_GetProcessMitigationPolicy (
         GetCurrentProcess (),
                   ProcessUserShadowStackPolicy, &shadow_stack_policy,
                                          sizeof (shadow_stack_policy)))
    {
      if (shadow_stack_policy.EnableUserShadowStack)
        return ProcessUserShadowStackPolicy;
    }

#ifdef SK_AVOID_CFG_PROCESSES
    PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY
      control_flow_guard_policy = { };

    if (_GetProcessMitigationPolicy (
         GetCurrentProcess (),
                   ProcessControlFlowGuardPolicy, &control_flow_guard_policy,
                                           sizeof (control_flow_guard_policy)))
    {
      if (control_flow_guard_policy.EnableControlFlowGuard)
      {
        return ProcessControlFlowGuardPolicy;
      }
    }
#endif
  }
#endif

  return 0;
}

//=========================================================================
BOOL
APIENTRY
DllMain ( HMODULE hModule,
          DWORD   ul_reason_for_call,
          LPVOID  lpReserved )
{
  UNREFERENCED_PARAMETER (lpReserved);

  switch (ul_reason_for_call)
  {
    // DLL Loaded into Process (not necessarily the first bit of code run)
    //
    //   The CBT hook in injection.cpp actually runs some code from the DLL
    //     before DllMain is invoked, so care must be taken to avoid using
    //       any part of Special K that relies on the Visual C++ Runtime in
    //         the context of that hook.
    //
    case DLL_PROCESS_ATTACH:
    {
      if (SK_IsServiceHost ())
        return FALSE;

      int process_protection =
        SK_NT_GetProcessProtection ();

      // Stay out of web browsers and such, SK gets stuck in them a lot
      if (process_protection == ProcessUserShadowStackPolicy)
        return FALSE;

      AuxUlibInitialize ();

#ifdef _SK_CONSISTENCY_CHECK
      std::atexit (SK_LazyCleanup);
#endif

      game_window =  sk_window_s {};
      config      =
        sk_config_t::sk_config_t ();


      skModuleRegistry::Self (hModule);

      auto CreateTeardownEvent =
      [&](void)
      { if (__SK_DLL_TeardownEvent == 0)
            __SK_DLL_TeardownEvent =
                    SK_CreateEvent ( nullptr, TRUE,
                                             FALSE, nullptr );

        SK_Inject_SpawnUnloadListener ();
      };

      auto EarlyOut =
      [&](BOOL bRet = TRUE)
      {
        if (bRet)
        {
          // This is not the process SKIF is looking for :)
          SK_Inject_SuppressExitNotify (       );
          SK_DLL_SetAttached           ( false );
          CreateTeardownEvent          (       );
        }

        return bRet;
      };

      // We use SKIM for injection and rundll32 for various tricks
      //   involving restarting the currently running game; neither
      //     needs or even wants this DLL fully initialized!
      if (SK_GetHostAppUtil ()->isInjectionTool ())
      {
        // Minimal Initialization (reserve TLS & determine config paths)
        SK_TLS_Acquire       ();
        SK_EstablishRootPath ();

        BOOL bRet = EarlyOut (TRUE);
        if ( bRet )
          SK_DLL_SetAttached (true);

        config.system.log_level = -1;

        return bRet;
      }

      // Social distancing like a boss!
      INT dll_isolation_lvl =
        SK_KeepAway ();

      // Will be implicitly set in call to SK_KeepAway
      if (SK_GetHostAppUtil ()->isBlacklisted ())
      {
#ifdef DEBUG
        OutputDebugStringW (L"Special K Disabled For Blacklisted App");
#endif
        return EarlyOut (TRUE);
      }

      SK_TLS_Acquire       ();
      SK_EstablishRootPath ();

      SK_TLS_Bottom ()->debug.in_DllMain = true;

      if (dll_isolation_lvl >  0)                  return EarlyOut (TRUE);

      // -> Nothing below this can return FALSE until TLS is tidied up (!!)

      // We reserve the right to deny attaching the DLL, this will
      //   generally happen if a game has opted-out of global injection.
      if (! SK_EstablishDllRole (hModule))         return EarlyOut (TRUE);

      // We don't want to initialize the DLL, but we also don't want it to
      //   re-inject itself constantly; just return TRUE here.
      if (DLL_ROLE::INVALID == SK_GetDLLRole ())   return EarlyOut (TRUE);
      if (! SK_Attach         (SK_GetDLLRole ()))  return EarlyOut (TRUE);

      CreateTeardownEvent ();

      InterlockedIncrementRelease (
        &__SK_DLL_Refs
      );

      // If we got this far, it's because this is an injection target
      //
      //   Must hold a reference to this DLL so that removing the global
      //     hook does not crash the game.
      SK_Inject_AcquireProcess ();

      return
        ( __SK_DLL_TeardownEvent != nullptr );
    } break;




    // DLL is Being Unloaded from Process
    //
    //   --> Once this returns, expect a follow-up DLL_THREAD_DETACH, and
    //         make sure that any code it executes does not trip over
    //           removed hooks.  ( Deinit is still in progress )
    //
    case DLL_PROCESS_DETACH:
    {
      bool bAttached =
        SK_DLL_IsAttached ();

      if ((intptr_t)__SK_DLL_TeardownEvent > (intptr_t)nullptr) SetEvent (
                    __SK_DLL_TeardownEvent                               );

      if ( FALSE == InterlockedCompareExchangeRelease ( &__SK_DLL_Ending,
                                                           TRUE,
                                                             FALSE )
         )
      {
        // Stop injection on normal unload
        if (SK_GetFramesDrawn () > 1)
        {
          SK_Inject_BroadcastInjectionNotify ();
        }

        // If the DLL being unloaded is the source of a global hook, then
        //   shut that down before detaching the DLL.
        if (ReadAcquire (&__SK_HookContextOwner))
        {
          SKX_RemoveCBTHook ();
        }
      }

      // Attached _AND_ non-trivially Initialized
      if (bAttached)
        SK_Memory_RemoveHooks ();

      if (! SK_GetHostAppUtil ()->
              isInjectionTool () )
      {
        SK_Detach (SK_GetDLLRole ());
      }

      if (bAttached || SK_TLS_HasSlot ())
      {
        SK_TLS *pTLS     = nullptr;
        auto    tls_slot =
          SK_GetTLS (&pTLS);

        if ( tls_slot != nullptr &&
             tls_slot->dwTlsIdx  == ReadULongAcquire (&__SK_TLS_INDEX) )
        {
          delete
            SK_CleanupTLS ();
        }

        auto *pFreeList =
          __SK_TLS_FreeList.getPtr ();

        if (pFreeList != nullptr)
        {
          SK_TLS  *pZombieTLS = nullptr;

          while (! pFreeList->empty ())
          { if (   pFreeList->try_pop (
                   pZombieTLS         )
               )
            delete pZombieTLS;
          }
        }

        SK_TLS_Release ();
      }

      if ((uintptr_t)__SK_DLL_TeardownEvent > (uintptr_t)nullptr)
    {SK_CloseHandle (__SK_DLL_TeardownEvent);
                     __SK_DLL_TeardownEvent = INVALID_HANDLE_VALUE;
      }

      // If SKX_RemoveCBTHook (...) is successful: (__SK_HookContextOwner = 0)
      if (InterlockedCompareExchange (&__SK_HookContextOwner, FALSE, TRUE))
      {
        SK_RunLHIfBitness (
          64, DeleteFileW (L"SpecialK64.pid"),
              DeleteFileW (L"SpecialK32.pid")
        );
      }

#ifdef DEBUG
      else if (dll_log != nullptr) {
      //Sanity FAILURE:  Attempt to detach something that was not properly
      //                   attached?!
        dll_log->Log (
          L"[ SpecialK ]  ** SANITY CHECK FAILED: DLL was never "
            L"attached !! **"
        );
      }
#endif
      
      SK_Inject_CleanupSharedMemory ();

      // Give time for any threads waiting on something such as a message pump
      //   to wake up before we unload the DLL, because our hook procedures are
      //     about to be unloaded from the process.
      if (SK_GetFramesDrawn () > 1)
          SK_Sleep (250UL);
    } break;




    // New Thread
    // ----------
    //
    //  Sneaky anti-debug software is known to create threads using a
    //    special NtCreateThread flag that can bypass thread attach notify;
    //
    //   * Be prepared to encounter threads where the TLS initialization
    //       that is supposed to take place here never does.
    //
    //  This can mean there will be threads that the thread profiler cannot
    //    access the TLS thread context for. Once those threads run one of
    //      Special K's hooks, TLS will finally be initialized.
    //
    case DLL_THREAD_ATTACH:
    {
      InterlockedIncrementAcquire (&lLastThreadCreate);
      InterlockedIncrementAcquire (&__SK_Threads_Attached);

      if (SK_DLL_IsAttached ())
      {
        SetThreadErrorMode (
          SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS,
            nullptr
        );

        SK_TLS *pTLS =
              SK_TLS_Bottom ();

        if (pTLS != nullptr)
        {   pTLS->debug.mapped = true;

          BOOL                                  bHoldingLock;
          if (AuxUlibIsDLLSynchronizationHeld (&bHoldingLock))
          {
            if (bHoldingLock)
              return TRUE;

            // Kick-off data collection on external thread creation
            SK_Widget_InvokeThreadProfiler ();
          }
        }
      }
    }
    break;



    // Thread Destroyed
    //
    //   ( Final DllMain invocation on clean unload )
    //
    case DLL_THREAD_DETACH:
    {
      InterlockedIncrementRelease (&lLastThreadCreate);

      if (SK_DLL_IsAttached ())
      {
        BOOL                                  bHoldingLock;
        if (AuxUlibIsDLLSynchronizationHeld (&bHoldingLock))
        {
          if (! bHoldingLock)
            SK_Widget_InvokeThreadProfiler ();
        }


        // Strip TLS and Mark Free able
        // ----------------------------
        //
        // Partially clean TLS; a small portion of debug data lingers
        //   per-thread until the DLL's complete process detach.
        //
        auto *pFreeList =
          __SK_TLS_FreeList.getPtr ();

        if (pFreeList != nullptr)
        {
          pFreeList->push (
            SK_CleanupTLS ()
          );
        }
      }
    }
    break;
  }

  return TRUE;
}
//=========================================================================




HMODULE
SK_GetLocalModuleHandle (const wchar_t* wszModule)
{
  wchar_t          wszLocalModulePath [MAX_PATH + 2] = { };
  SK_PathCombineW (wszLocalModulePath, SK_GetHostPath  ( ),
                        wszModule);
  return
    SK_GetModuleHandleW (wszLocalModulePath);
};

HMODULE
SK_LoadLocalModule (const wchar_t* wszModule)
{
  wchar_t          wszLocalModulePath [MAX_PATH + 2] = { };
  SK_PathCombineW (wszLocalModulePath, SK_GetHostPath  ( ),
                        wszModule);

  HMODULE hMod =
    SK_GetModuleHandleW (wszLocalModulePath);

  if (hMod == nullptr && PathFileExistsW (wszLocalModulePath))
  {
    // This is generally unsafe from DllMain, but SK's local DLLs should have
    //   the same set of dependent imports as this one, thus avoiding loader
    //     deadlocks.
    return
      LoadLibraryW (wszLocalModulePath);
  }

  return hMod;
};

// If this is the global injector and there is a wrapper version
//   of Special K in the DLL search path, then bail-out!
BOOL
SK_TryLocalWrapperFirst (const std::vector <const wchar_t *>& dlls)
{
  for ( const auto dll : dlls )
  {
    if ( SK_IsDLLSpecialK   (dll) &&
         SK_LoadLocalModule (dll) != nullptr )
    {
      return TRUE;
    }
  }

  return FALSE;
};


BOOL
SK_DontInject (void)
{
  _HasLocalDll = true;

  SK_TLS_Release     ();
  SK_DLL_SetAttached (false);

  SK_SetDLLRole      (DLL_ROLE::INVALID);

  return FALSE;
}

bool
_SKM_AutoBootLastKnownAPI (SK_RenderAPI last_known)
{
  bool auto_boot_viable = false;

  using role_from_api_tbl =
    std::map < SK_RenderAPI, std::tuple < DLL_ROLE, bool > >;

  role_from_api_tbl
  role_reversal  =
  {
    { SK_RenderAPI::D3D9,
        { DLL_ROLE::D3D9,           config.apis.d3d9.hook } },
    { SK_RenderAPI::D3D9Ex,
        { DLL_ROLE::D3D9,         config.apis.d3d9ex.hook } },

    { SK_RenderAPI::D3D10,
        { DLL_ROLE::DXGI,  false/* Stupid API--begone! */ } },
    { SK_RenderAPI::D3D11,
        { DLL_ROLE::DXGI,     config.apis.dxgi.d3d11.hook } },
    { SK_RenderAPI::D3D12,
        { DLL_ROLE::DXGI,                            true } },

   { SK_RenderAPI::OpenGL,
       { DLL_ROLE::OpenGL,        config.apis.OpenGL.hook } },

#ifdef _M_IX86

    // Bitness:  32-Bit  (Add:  DDraw, D3D8 and Glide)

    { SK_RenderAPI::D3D8,
        { DLL_ROLE::D3D8,           config.apis.d3d8.hook } },
    { SK_RenderAPI::D3D8On11,
        { DLL_ROLE::D3D8,     config.apis.d3d8.hook   &&
                              config.apis.dxgi.d3d11.hook } },
    { SK_RenderAPI::D3D8On12,
        { DLL_ROLE::D3D8,     config.apis.d3d8.hook   &&
                              config.apis.dxgi.d3d12.hook } },

    { SK_RenderAPI::Glide,
        { DLL_ROLE::Glide,         config.apis.glide.hook } },
    { SK_RenderAPI::GlideOn11,
        { DLL_ROLE::Glide,    config.apis.glide.hook  &&
                              config.apis.dxgi.d3d11.hook } },
    { SK_RenderAPI::GlideOn12,
        { DLL_ROLE::Glide,    config.apis.glide.hook  &&
                              config.apis.dxgi.d3d12.hook } },

    { SK_RenderAPI::DDraw,
        { DLL_ROLE::DDraw,         config.apis.ddraw.hook } },
    { SK_RenderAPI::DDrawOn11,
        { DLL_ROLE::DDraw,    config.apis.ddraw.hook  &&
                              config.apis.dxgi.d3d11.hook } },
    { SK_RenderAPI::DDrawOn12,
        { DLL_ROLE::DDraw,    config.apis.ddraw.hook  &&
                              config.apis.dxgi.d3d12.hook } },

    { SK_RenderAPI::Vulkan,    { DLL_ROLE::INVALID, false } },
#else

    // Bitness:  64-Bit  (Remove Legacy APIs  +  Add Vulkan)

    { SK_RenderAPI::D3D8,      { DLL_ROLE::INVALID, false } },
    { SK_RenderAPI::D3D8On11,  { DLL_ROLE::INVALID, false } },
    { SK_RenderAPI::D3D8On12,  { DLL_ROLE::INVALID, false } },

    { SK_RenderAPI::Glide,     { DLL_ROLE::INVALID, false } },
    { SK_RenderAPI::GlideOn11, { DLL_ROLE::INVALID, false } },
    { SK_RenderAPI::GlideOn12, { DLL_ROLE::INVALID, false } },

    { SK_RenderAPI::DDraw,     { DLL_ROLE::INVALID, false } },
    { SK_RenderAPI::DDrawOn11, { DLL_ROLE::INVALID, false } },
    { SK_RenderAPI::DDrawOn12, { DLL_ROLE::INVALID, false } },

    { SK_RenderAPI::Vulkan,
        { DLL_ROLE::Vulkan,       config.apis.Vulkan.hook } },
#endif
};

  if (role_reversal.count (last_known) != 0)
  {
    auto_boot_viable =
      std::get <1> (role_reversal [last_known]);

    if (auto_boot_viable)
    {
      SK_SetDLLRole (std::get <0> (role_reversal [last_known]));



      // This actually _saves_ the config, after parsing and
      //   trimming it.
      SK_LoadConfig (L"SpecialK");

      config.apis.last_known = SK_RenderAPI::Reserved;
    }
  }

  return
    auto_boot_viable;
}


bool
__stdcall
SK_dgVoodoo_CheckForInterop (void)
{
  const std::map <const wchar_t *, SK_RenderAPI> dgvoodoo_dll_map =
  {
    { L"d3d9.dll",    SK_RenderAPI::D3D9  },
    { L"d3d8.dll",    SK_RenderAPI::D3D8  },
    { L"d3dimm.dll",  SK_RenderAPI::DDraw },
    { L"ddraw.dll",   SK_RenderAPI::DDraw },
    { L"glide3x.dll", SK_RenderAPI::Glide },
    { L"glide2x.dll", SK_RenderAPI::Glide },
    { L"glide.dll",   SK_RenderAPI::Glide }
  };

  for ( auto& it : dgvoodoo_dll_map )
  {
    auto dgvoodoo_dll = it.first;

    HMODULE hMod_dgVoodoo =
      SK_GetModuleHandle (dgvoodoo_dll);

    if (hMod_dgVoodoo == 0)
    {
      wchar_t     wszDllPath [MAX_PATH] = { };
      PathAppend (wszDllPath, SK_GetHostPath ());
      PathAppend (wszDllPath, dgvoodoo_dll);

      if (PathFileExistsW (wszDllPath))
      {
        hMod_dgVoodoo =
          SK_LoadLibraryW (wszDllPath);
      }
    }

    if (hMod_dgVoodoo != 0)
    {
      std::wstring str_dgvoodoo_ver =
        SK_GetDLLVersionStr (
          SK_GetModuleFullName (hMod_dgVoodoo).c_str ()
                            );

      if (str_dgvoodoo_ver.find (L"dgVoodoo") != std::wstring::npos)
      {
        config.apis.translated = it.second;

        if (config.apis.translated == SK_RenderAPI::D3D9)
        {
          config.apis.d3d9.hook   = false;
          config.apis.d3d9ex.hook = false;
        }

        config.render.dxgi.use_factory_cache = false;
        config.render.dxgi.skip_mode_changes = true;

        return true;
      }
    }
  }

  return false;
}

#if 0
class SK_ScopedAttach {
public:
  SK_ScopedAttach (void)
  {
    InterlockedIncrement (&__SK_DLL_Attached);
  }

  ~SK_ScopedAttach (void)
  {
    InterlockedDecrement (&__SK_DLL_Attached);
  }
};
#endif

BOOL
__stdcall
SK_EstablishDllRole (skWin32Module&& _sk_module)
{
  SK_WarmupRenderBackends ();

  SK_SetDLLRole (DLL_ROLE::INVALID);

#ifndef _M_AMD64
  bool has_dgvoodoo =
   ( GetFileAttributesW (
       SK_FormatStringW ( LR"(%ws\PlugIns\ThirdParty\dgVoodoo\d3dimm.dll)",
                            SK_GetInstallPath ()
                        ).c_str ()
                        ) != INVALID_FILE_ATTRIBUTES );
#endif

  const wchar_t* wszSelfTitledDLL =
    _sk_module;

  const wchar_t* wszShort =
    SK_CharNextW ( SK_Path_wcsrchr ( wszSelfTitledDLL, *LR"(\)" ) );

  // The DLL path was _already_ in non-fully-qualified form... oops?
  if (wszShort == reinterpret_cast <const wchar_t *> (1))
      wszShort = wszSelfTitledDLL;


  if (0 == SK_Path_wcsicmp (wszShort, L"dinput8.dll"))
  {
    SK_SetDLLRole (DLL_ROLE::DInput8);

    SK_dgVoodoo_CheckForInterop ();

    if ( SK_IsDLLSpecialK (L"dxgi.dll")     ||
         SK_IsDLLSpecialK (L"d3d9.dll")     ||
         SK_IsDLLSpecialK (L"d3d11.dll")    ||
         SK_IsDLLSpecialK (L"OpenGL32.dll") ||
         SK_IsDLLSpecialK (L"ddraw.dll")    ||
         SK_IsDLLSpecialK (L"d3d8.dll"))
    {
      SK_MessageBox ( L"Please limit Special K to one (1) "
                       "of the following: d3d8.dll, d3d9.dll,"
                                         " d3d11.dll, ddraw.dll, dinput8.dll,"
                                         " dxgi.dll or OpenGL32.dll",
                      L"Conflicting Special K Injection DLLs Detected",
                        MB_SYSTEMMODAL | MB_SETFOREGROUND |
                        MB_ICONERROR   | MB_OK );
    }
  }

  else if (0 == SK_Path_wcsicmp (wszShort, L"dxgi.dll"))
  {
    SK_SetDLLRole (DLL_ROLE::DXGI);

    SK_dgVoodoo_CheckForInterop ();
  }

  else if (0 == SK_Path_wcsicmp (wszShort, L"d3d11.dll"))
  {
    SK_SetDLLRole ( static_cast <DLL_ROLE> ( (int)DLL_ROLE::DXGI |
                                             (int)DLL_ROLE::D3D11 ) );

    SK_dgVoodoo_CheckForInterop ();
  }

#ifndef _M_AMD64
  else if (0 == SK_Path_wcsicmp (wszShort, L"d3d8.dll")  && has_dgvoodoo)
  {
    SK_SetDLLRole (DLL_ROLE::D3D8);

    config.render.dxgi.use_factory_cache = false;
    config.render.dxgi.skip_mode_changes = true;
  }

  else if (0 == SK_Path_wcsicmp (wszShort, L"ddraw.dll") && has_dgvoodoo)
  {
    SK_SetDLLRole (DLL_ROLE::DDraw);

    config.render.dxgi.use_factory_cache = false;
    config.render.dxgi.skip_mode_changes = true;
  }
#endif

  else if (0 == SK_Path_wcsicmp (wszShort, L"d3d9.dll"))
    SK_SetDLLRole (DLL_ROLE::D3D9);

  else if (0 == SK_Path_wcsicmp (wszShort, L"OpenGL32.dll"))
    SK_SetDLLRole (DLL_ROLE::OpenGL);


  //
  // This is an injected DLL, not a wrapper DLL...
  //
  else if ( SK_Path_wcsstr (wszShort, L"SpecialK32.dll") != nullptr ||
            SK_Path_wcsstr (wszShort, L"SpecialK64.dll") != nullptr )
  {
             // SET the injected state
             SK_IsInjected (true);
    bool explicit_inject = false;

    config.system.central_repository = true;

    auto SK_InitCentralConfig = [&](void)
    {
      SK_TLS_Acquire ();

      SK_RunOnce ({
        SK_EstablishRootPath ();
        SK_LoadConfigEx      (L"SpecialK", false);

        SK_GetFullyQualifiedApp ();
      });
    };

    auto SK_Inject_ProcessBlacklist = [&](void) ->
    BOOL
    {
      if (SK_Inject_TestBlacklists (SK_GetFullyQualifiedApp ()))
      {
        SK_GetHostAppUtil ()->setBlacklisted (true);

        SK_SetDLLRole (DLL_ROLE::INVALID);

        return TRUE;
      }

      return FALSE;
    };

    const wchar_t* wszHostPath =
      SK_GetHostPath ();

    // Path to empty files that signal API
    using api_magic_file_t =
      wchar_t [MAX_PATH + 2];

    struct api_stub_s {
      api_magic_file_t path;
      const wchar_t*   vfile;
      DLL_ROLE         role;
    };

    api_stub_s stubs_ [] =
    {
      { { }, L"SpecialK.d3d9",     DLL_ROLE::D3D9    },
#ifndef _M_AMD64
      { { }, L"SpecialK.d3d8",       has_dgvoodoo ?
                                   DLL_ROLE::D3D8 :
                                   DLL_ROLE::INVALID },
      { { }, L"SpecialK.ddraw",       has_dgvoodoo ?
                                   DLL_ROLE::DDraw :
                                   DLL_ROLE::INVALID },
#endif
      { { }, L"SpecialK.dxgi",     DLL_ROLE::DXGI    },
      { { }, L"SpecialK.d3d11",
                       static_cast <DLL_ROLE> (
                 static_cast <int> (DLL_ROLE::DXGI) |
                 static_cast <int> (DLL_ROLE::D3D11)
                                              )      },
      { { }, L"SpecialK.OpenGL32", DLL_ROLE::OpenGL  },
      { { }, L"SpecialK.DInput8",  DLL_ROLE::DInput8 }
    };

    for ( auto& _stub : stubs_ )
    {
      SK_PathCombineW ( _stub.path,
                        wszHostPath,
                        _stub.vfile );
    }

    // Returns 0 if no match, or the entire bitset if matched
    auto
    SK_File_TestAttribs =
    []( const wchar_t* wszPath,
              DWORD    dwAttribMask ) ->
    DWORD
    {
      const DWORD dwFileAttribs =
        GetFileAttributesW (wszPath);

      if (dwFileAttribs != INVALID_FILE_ATTRIBUTES)
      {
        return ( ( ( dwFileAttribs & dwAttribMask ) != 0UL )
                                   ?
                     dwFileAttribs : 0UL );
      }

      return 0UL;
    };

    for ( auto& _stub : stubs_ )
    {
      if ( SK_File_TestAttribs ( _stub.path,
                                 INVALID_FILE_ATTRIBUTES ) != 0 )
      {
        const DLL_ROLE role =
                 _stub.role;

        if (role != DLL_ROLE::INVALID)
        {
          if (SK_Inject_ProcessBlacklist ())
            return SK_DontInject ();

          SK_InitCentralConfig ();

          SK_SetDLLRole (role);
          explicit_inject = true;

          break;
        }
      }
    }

    // Opted out of explicit injection, now try automatic
    //
    if (! explicit_inject)
    {
      // This order is not arbitrary, but not worth explaining
      std::vector <const wchar_t *> local_dlls =
        { L"dxgi.dll",   L"d3d9.dll",
          L"d3d11.dll",  L"OpenGL32.dll",
          L"ddraw.dll",  L"d3d8.dll",
          L"dinput8.dll"                };

      // If there is a local Special K DLL in the game's directory,
      //   load it and then bow-out -- we are done here.
      //
      if ( (! SK_IsHostAppSKIM        (            ) ) &&
              SK_TryLocalWrapperFirst ( local_dlls ) )
      {
        return
          SK_DontInject ();
      }


      DWORD   dwProcessSize = MAX_PATH;
      wchar_t wszProcessName [MAX_PATH + 2] = { };

      GetModuleFileNameW (nullptr, wszProcessName, dwProcessSize);

      // To catch all remaining Steam games, look for "\SteamApps\" in the
      //   executable path.
      //
      //  These games may not use SteamAPI, but they are designed for the
      //    Steamworks platform and we can connect to the client using the
      //      steam_api{64}.dll files distributed with Special K.
      //
      bool is_steamworks_game =
           SK_Path_wcsstr (wszProcessName, L"SteamApps") != nullptr;

      bool is_epic_game = (! is_steamworks_game) &&
           (    StrStrIW (GetCommandLineW (), L"-epicapp") ||
              SK_Path_wcsstr (wszProcessName, LR"(Epic Games\)") != nullptr );

      bool is_microsoft_game = (! is_steamworks_game) && (! is_epic_game) &&
           SK_IsModuleLoaded (L"AppXDeploymentClient.dll");

      bool is_ubisoft_game =
        (! is_steamworks_game) && (! is_epic_game) &&
        (! is_microsoft_game ) && ( SK_IsModuleLoaded (L"uplay_aux_r164.dll") ||
                                    SK_IsModuleLoaded (L"uplay_aux_r264.dll") );

      bool is_gog_game =
        (! is_steamworks_game) && (! is_epic_game)    &&
        (! is_microsoft_game ) && (! is_ubisoft_game) &&
           SK_Path_wcsstr (wszProcessName, LR"(GOG Galaxy\Games)") != nullptr;

      bool is_origin_game =
        (! is_steamworks_game) && (! is_epic_game)    && (! is_gog_game) &&
        (! is_microsoft_game ) && (! is_ubisoft_game) &&
           SK_Path_wcsstr (wszProcessName, LR"(Origin Games\)") != nullptr;



      // Most frequently imported DLLs for games that use SteamAPI
      //
      //  It is trivial to use SteamAPI without linking to the DLL, so
      //    this is not the final test to determine Steam compatibility.
      //
      sk_import_test_s steam_tests [] =
      {
        { SK_RunLHIfBitness (64, "steam_api64.dll",
                                 "steam_api.dll"),  false },
        {                        "steamnative.dll", false }
      };

      if (! is_steamworks_game)
      {
        SK_TestImports (GetModuleHandle (nullptr), steam_tests, 2);

        is_steamworks_game =
          ( steam_tests [0].used ||
            steam_tests [1].used );
      }

      // If this is a Steamworks game, then lets start doing stuff to it!
      //
      //   => We still need to figure out the primary graphics API.
      //
      if ( ( is_steamworks_game || is_epic_game    || is_origin_game ||
            /*is_microsoft_game || */ is_ubisoft_game || is_gog_game ||
             SK_Inject_TestWhitelists (SK_GetFullyQualifiedApp ()) ) &&
          (! SK_Inject_TestBlacklists (SK_GetFullyQualifiedApp ()) )  )
      {
        SK_InitCentralConfig ();

        if (SK_Inject_ProcessBlacklist ())
          return SK_DontInject ();

        // Try the last-known API first -- if we have one.
        //
        if (config.apis.last_known != SK_RenderAPI::Reserved)
        {
          if (! SK_dgVoodoo_CheckForInterop ())
          {
            if (_SKM_AutoBootLastKnownAPI (config.apis.last_known))
            {
              return true;
            }
          }
        }


        ///// That did not work; examine the game's Import Address Table
        /////
        bool gl    = false, vulkan = false, d3d9  = false, d3d11 = false,
             dxgi  = false, d3d8   = false, ddraw = false, d3d12 = false,
             glide = false;

        SK_TestRenderImports (
          __SK_hModHost,
            &gl, &vulkan,
              &d3d9, &dxgi, &d3d11, &d3d12,
                &d3d8, &ddraw, &glide
        );

        gl     |= (SK_GetModuleHandle (L"OpenGL32.dll")  != nullptr);
        d3d9   |= (SK_GetModuleHandle (L"d3d9.dll")      != nullptr);

        // Not specific enough; some engines will pull in DXGI even if they
        //   do not use D3D10/11/12/D2D/DWrite
        //
      //dxgi   |= (SK_GetModuleHandle (L"dxgi.dll")      != nullptr);

        d3d11  |= (SK_GetModuleHandle (L"d3d11.dll")     != nullptr);
        d3d11  |= (SK_GetModuleHandle (L"d3dx11_43.dll") != nullptr);
        d3d11  |= (SK_GetModuleHandle (L"dxcore.dll")    != nullptr); // Unity

        d3d12  |= (SK_GetModuleHandle (L"d3d12.dll")     != nullptr);

        dxgi   |= ( d3d11 | d3d12 );

        if (SK_dgVoodoo_CheckForInterop ())
        {
          if (config.apis.translated == SK_RenderAPI::D3D9)
          {
            d3d9 = false;
          }

          d3d11 = true;
          d3d12 = true;
        }

#ifndef _M_AMD64
        d3d8   |= (SK_GetModuleHandle (L"d3d8.dll")      != nullptr);
        ddraw  |= (SK_GetModuleHandle (L"ddraw.dll")     != nullptr);

        // Don't use dgVoodoo Plug-In if a game is already translated
        if (config.apis.translated != SK_RenderAPI::None)
        {
          config.apis.d3d8.hook  = false;
          config.apis.ddraw.hook = false;
          config.apis.glide.hook = false;

          d3d8  = false;
          ddraw = false;
          glide = false;
        }

        if (config.apis.d3d8.hook && d3d8 && has_dgvoodoo)
        {
          if (SK_TryLocalWrapperFirst ({ L"d3d8.dll" }))
          {
            return SK_DontInject ();
          }

          config.render.dxgi.use_factory_cache = false;
          config.render.dxgi.skip_mode_changes = true;

          config.apis.dxgi.d3d11.hook = true;
          config.apis.dxgi.d3d12.hook = true;

          SK_SetDLLRole (DLL_ROLE::D3D8);
        }

        else if (config.apis.ddraw.hook && ddraw && has_dgvoodoo)
        {
          if (SK_TryLocalWrapperFirst ({ L"ddraw.dll" }))
          {
            return SK_DontInject ();
          }

          config.render.dxgi.use_factory_cache = false;
          config.render.dxgi.skip_mode_changes = true;

          config.apis.dxgi.d3d11.hook = true;
          config.apis.dxgi.d3d12.hook = true;

          SK_SetDLLRole (DLL_ROLE::DDraw);
        }

        else
#endif
        if (SK_TryLocalWrapperFirst ({ L"dinput8.dll" }))
        {
          return
            SK_DontInject ();
        }

        else if (config.apis.OpenGL.hook && gl)
        {
          if (SK_TryLocalWrapperFirst ({ L"OpenGL32.dll" }))
          {
            return SK_DontInject ();
          }

          SK_SetDLLRole (DLL_ROLE::OpenGL);
        }

        else if ( ( config.apis.dxgi.d3d11.hook ||
                    config.apis.dxgi.d3d12.hook    )
                                                && (dxgi || d3d11 || d3d12))
        {
          if (SK_TryLocalWrapperFirst ({ L"dxgi.dll", L"d3d11.dll" }))
          {
            return SK_DontInject ();
          }

          //if (d3d11)
          //{
          //  SK_SetDLLRole ( static_cast <DLL_ROLE> ( (int)DLL_ROLE::DXGI |
          //                                           (int)DLL_ROLE::D3D11 ) );
          //}
          //
          //else
            SK_SetDLLRole (DLL_ROLE::DXGI);
        }

        else if (config.apis.d3d9.hook && d3d9)
        {
          if (SK_TryLocalWrapperFirst ({ L"d3d9.dll" }))
          {
            return SK_DontInject ();
          }

          SK_SetDLLRole (DLL_ROLE::D3D9);
        }

#ifdef _M_AMD64
        else if (config.apis.Vulkan.hook && vulkan)
          SK_SetDLLRole (DLL_ROLE::Vulkan);
#endif


        //
        // *** No Freaking Clue What Graphics API This Game Uses ?!
        //
        //  Use the config file to filter out any APIs the user
        //    knows are not valid.
        //
        else
        {
          if (config.apis.dxgi.d3d11.hook ||
              config.apis.dxgi.d3d12.hook)
            SK_SetDLLRole (DLL_ROLE::DXGI);
          else if (config.apis.d3d9.hook  ||
              config.apis.d3d9ex.hook)
            SK_SetDLLRole (DLL_ROLE::D3D9);
          else if (config.apis.OpenGL.hook)
            SK_SetDLLRole (DLL_ROLE::OpenGL);
#ifdef _M_AMD64
          else if (config.apis.Vulkan.hook)
            SK_SetDLLRole (DLL_ROLE::Vulkan);
#else
          else if (config.apis.d3d8.hook && has_dgvoodoo)
            SK_SetDLLRole (DLL_ROLE::D3D8);
          else if (config.apis.ddraw.hook && has_dgvoodoo)
            SK_SetDLLRole (DLL_ROLE::DDraw);
#endif
        }

        // Auto-Guess OpenGL if all else fails...
        if (SK_GetDLLRole () == DLL_ROLE::INVALID)
        {
          //config.apis.dxgi.d3d11.hook = true;
          //SK_SetDLLRole (     DLL_ROLE::DXGI   );
          config.apis.OpenGL.hook = true;
          SK_SetDLLRole (     DLL_ROLE::OpenGL   );
        }

        // Write any default values to the config file
        SK_LoadConfig (L"SpecialK");

        config.apis.last_known =
          SK_RenderAPI::Reserved;
      }
    }
  }

  return
    ( SK_GetDLLRole () != DLL_ROLE::INVALID );
}




void
SK_CleanupMutex (SK_Thread_HybridSpinlock **ppMutex)
{
  if (  ppMutex != nullptr &&
       *ppMutex != nullptr    )
  {
    auto    to_delete = ppMutex;
    delete* to_delete; *ppMutex = nullptr;
  }
};


BOOL
__stdcall
SK_Attach (DLL_ROLE role)
{
  auto bootstraps =
    SK_DLL_GetBootstraps ();

  auto _CleanupMutexes =
  [&](void)->
      void
      {
        SK_CleanupMutex (&budget_mutex); SK_CleanupMutex (&init_mutex);
        SK_CleanupMutex (&wmi_cs);

        SK_CleanupMutex (&steam_callback_cs);
        SK_CleanupMutex (&platform_popup_cs);
        SK_CleanupMutex (&steam_init_cs);

        void SK_D3D11_CleanupMutexes (void);
             SK_D3D11_CleanupMutexes (    );
      };

  const SK_DLL_Bootstrapper *pBootStrapper = nullptr;

  if ( bootstraps.find (role) !=
       bootstraps.cend (    )  )
  {
    pBootStrapper = &bootstraps.at (role);
  }

  if (FALSE == InterlockedCompareExchangeAcquire (
                 &__SK_DLL_Attached, TRUE, FALSE )
     )
  {
    auto _InitMutexes =
      [&](void)->
          void
          {
            cs_dbghelp =
              new SK_Thread_HybridSpinlock (2048UL);
            budget_mutex =
              new SK_Thread_HybridSpinlock ( 100UL);
            init_mutex   =
              new SK_Thread_HybridSpinlock ( 768UL);
            wmi_cs       =
              new SK_Thread_HybridSpinlock ( 128UL);

            steam_callback_cs =
              new SK_Thread_HybridSpinlock ( 256UL);
            platform_popup_cs =
              new SK_Thread_HybridSpinlock ( 512UL);
            steam_init_cs     =
              new SK_Thread_HybridSpinlock ( 128UL);
          };

    _InitMutexes ();

    skModuleRegistry::HostApp (
      GetModuleHandle (nullptr)
    );

    if (pBootStrapper != nullptr)
    {
      if ( SK_IsInjected           () &&
           SK_TryLocalWrapperFirst (pBootStrapper->wrapper_dlls))
      {
        _CleanupMutexes ();

        return
          SK_DontInject ();
      }
    }

    if ( INVALID_FILE_ATTRIBUTES !=
           GetFileAttributesW (L"SpecialK.WaitForDebugger") )
    {
      while (! SK_IsDebuggerPresent ())
        SK_Sleep (50);
    }

    try
    {
      SK_TLS_Acquire ();

      _time64 (&__SK_DLL_AttachTime);

      InterlockedCompareExchangeAcquire (
        &__SK_DLL_Attached,
          pBootStrapper != nullptr ?
          pBootStrapper->start ()  : TRUE, FALSE
      );

      if (SK_DLL_IsAttached ())
      {
        return TRUE;
      }
    }

    catch (const std::exception&)
    {
      OutputDebugStringW ( L"[ SpecialK ] Caught an exception during DLL Attach,"
                           L" game may not be stable..." );
    }
  }

  _CleanupMutexes ();

  return
    SK_DontInject ();
}



BOOL
__stdcall
SK_Detach (DLL_ROLE role)
{
  ULONG local_refs =
    InterlockedDecrementRelease (&__SK_DLL_Refs);

  if ( InterlockedCompareExchangeRelease (
                    &__SK_DLL_Attached,
                      FALSE,
                        TRUE        )
     )
  {
    SK_ReleaseAssert (local_refs == 0);

#ifdef _SK_NO_ATOMIC_SHARED_PTR
    SK_CleanupMutex (&budget_mutex);     SK_CleanupMutex (&init_mutex);
    SK_CleanupMutex (&cs_dbghelp);       SK_CleanupMutex (&wmi_cs);

    SK_CleanupMutex (&steam_callback_cs);SK_CleanupMutex (&platform_popup_cs);
    SK_CleanupMutex (&steam_init_cs);

    void SK_D3D11_CleanupMutexes (void);
         SK_D3D11_CleanupMutexes (    );
#endif

    SK_Inject_ReleaseProcess ();

    auto bootstraps =
      SK_DLL_GetBootstraps ();

    if (! bootstraps.count (role))
    {
    }

    else if (bootstraps.at (role).shutdown ())
    {
      SK_TLS_Release ();

      return TRUE;
    }
  }

  else {
  //dll_log->Log (L"[ SpecialK ]  ** UNCLEAN DLL Process Detach !! **");
    return TRUE;
  }

  return FALSE;
}

const wchar_t*
__stdcall
SK_GetVersionStrW (void) noexcept
{
  return SK_VERSION_STR_W;
}

const char*
__stdcall
SK_GetVersionStrA (void) noexcept
{
  return SK_VERSION_STR_A;
}
