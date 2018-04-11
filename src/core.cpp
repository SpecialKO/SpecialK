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

#include <SpecialK/SpecialK.h>
#include <SpecialK/core.h>
#include <SpecialK/stdafx.h>
#include <SpecialK/hooks.h>
#include <SpecialK/window.h>

#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>
#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/diagnostics/crash_handler.h>
#include <SpecialK/diagnostics/debug_utils.h>

#include <SpecialK/performance/memory_monitor.h>
#include <SpecialK/performance/io_monitor.h>
#include <SpecialK/performance/gpu_monitor.h>
#include <SpecialK/framerate.h>
#include <SpecialK/com_util.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>
#include <SpecialK/sound.h>
#include <SpecialK/tls.h>

#include <SpecialK/osd/text.h>
#include <SpecialK/import.h>
#include <SpecialK/console.h>
#include <SpecialK/command.h>

#include <SpecialK/render/backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>
#include <SpecialK/render/vk/vulkan_backend.h>

#include <SpecialK/plugin/plugin_mgr.h>

#include <SpecialK/resource.h>

#include <SpecialK/nvapi.h>
#include <SpecialK/adl.h>

#include <SpecialK/steam_api.h>

#include <SpecialK/update/version.h>
#include <SpecialK/update/network.h>

#include <SpecialK/widgets/widget.h>

#include <SpecialK/input/dinput7_backend.h>
#include <SpecialK/input/dinput8_backend.h>

#include <SpecialK/injection/injection.h>

#include <SpecialK/control_panel.h>

#include <atlbase.h>
#include <comdef.h>
#include <delayimp.h>
#include <ShlObj.h>
#include <LibLoaderAPI.h>

#pragma warning   (push)
#pragma warning   (disable: 4091)
#define _IMAGEHLP_SOURCE_
#  include <DbgHelp.h>
#pragma warning   (pop)

#include <avrt.h>
#pragma comment (lib, "avrt.lib")

#include <d3d9.h>
#include <d3d11.h>
#include <wingdi.h>
#include <gl/gl.h>

#include <imgui/imgui.h>

extern iSK_Logger game_debug;

extern void SK_Input_PreInit (void);

volatile HANDLE hInitThread    = { INVALID_HANDLE_VALUE };
volatile LONG   dwInitThreadId = 0;

NV_GET_CURRENT_SLI_STATE sli_state;
BOOL                     nvapi_init  = FALSE;
HMODULE                  backend_dll = nullptr;

volatile LONG            __SK_Init   = FALSE;
         bool            __SK_bypass = false;


using ChangeDisplaySettingsA_pfn = LONG (WINAPI *)(
  _In_opt_ DEVMODEA *lpDevMode,
  _In_     DWORD     dwFlags
);

extern ChangeDisplaySettingsA_pfn ChangeDisplaySettingsA_Original;


struct init_params_s {
  std::wstring  backend    = L"INVALID";
  void*         callback   =    nullptr;
  LARGE_INTEGER start_time {   0,   0 };
};


init_params_s&
SKX_GetInitParams (void)
{
  static init_params_s params;
  return               params;
};

const init_params_s&
SK_GetInitParams (void)
{
  return SKX_GetInitParams ();
};

#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED        0x00000001
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH      0x00000002
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER      0x00000004
#define THREAD_CREATE_FLAGS_HAS_SECURITY_DESCRIPTOR 0x00000010
#define THREAD_CREATE_FLAGS_ACCESS_CHECK_IN_TARGET  0x00000020
#define THREAD_CREATE_FLAGS_INITIAL_THREAD          0x00000080


enum THREADINFOCLASS {
  ThreadBasicInformation,
};

NTSTATUS WINAPI NtQueryInformationThread(
  _In_      HANDLE          ThreadHandle,
  _In_      THREADINFOCLASS ThreadInformationClass,
  _Inout_   PVOID           ThreadInformation,
  _In_      ULONG           ThreadInformationLength,
  _Out_opt_ PULONG          ReturnLength
);



wchar_t*
SKX_GetBackend (void)
{
  static wchar_t SK_Backend [     128    ] = { };
  return         SK_Backend;
}

const wchar_t*
__stdcall
SK_GetBackend (void)
{
  return SKX_GetBackend ();
}

const wchar_t*
__stdcall
SK_SetBackend (const wchar_t* wszBackend)
{
  return
    lstrcpynW ( SKX_GetBackend (),
                  wszBackend,
                    127 );
}

wchar_t*
SKX_GetRootPath (void)
{
  static wchar_t SK_RootPath [MAX_PATH + 2 + 1] = { };
  return         SK_RootPath;
}

const wchar_t*
__stdcall
SK_GetRootPath (void)
{
  return SKX_GetRootPath ();
}




const wchar_t*
__stdcall
SK_GetConfigPath (void);

wchar_t*
SKX_GetNaiveConfigPath (void)
{
  static wchar_t SK_ConfigPath [MAX_PATH + 2 + 1] = { };
  return         SK_ConfigPath;
}
__declspec (noinline)
const wchar_t*
__stdcall
SK_GetNaiveConfigPath (void)
{
  return SKX_GetNaiveConfigPath ();
}

//
// To be used internally only, by the time any plug-in has
//   been activated, Special K will have already established
//     this path and loaded its config.
//
void
__stdcall
SK_SetConfigPath (const wchar_t* path)
{
  lstrcpynW (SKX_GetNaiveConfigPath (), path, MAX_PATH);
}


volatile LONG SK_RenderBackend::frames_drawn = 0;

__declspec (noinline)
ULONG
__stdcall
SK_GetFramesDrawn (void)
{
  return
    static_cast <ULONG> (ReadNoFence (&SK_RenderBackend::frames_drawn));
}


void
__stdcall
SK_StartPerfMonThreads (void)
{
  auto SpawnMonitorThread =
  []( volatile HANDLE                 *phThread,
      const    wchar_t                *wszName,
               LPTHREAD_START_ROUTINE  pThunk ) ->
  bool
  {
    if ( InterlockedCompareExchangePointer (phThread, nullptr, INVALID_HANDLE_VALUE) ==
           INVALID_HANDLE_VALUE )
    {
      dll_log.LogEx (true, L"[ WMI Perf ] Spawning %ws...  ", wszName);

      InterlockedExchangePointer ( (void **)phThread,
        (HANDLE)
        CreateThread ( nullptr,
                           0,
                             pThunk,
                               nullptr,
                                 0,
                                   nullptr )
      );

      if (ReadPointerAcquire (phThread) != INVALID_HANDLE_VALUE)
      {
        dll_log.LogEx (false, L"tid=0x%04x\n",
                         GetThreadId (ReadPointerAcquire (phThread)));
        return true;
      }

      else
      {
        dll_log.LogEx (false, L"Failed!\n");
      }
    }

    return false;
  };

  //
  // Spawn CPU Refresh Thread
  //
  if (config.cpu.show || SK_ImGui_Widgets.cpu_monitor->isActive ())
    SpawnMonitorThread (&cpu_stats.hThread, L"CPU Monitor", SK_MonitorCPU);

  //
  // Spawn Process Monitor Thread
  //
  if (config.mem.show)
    SpawnMonitorThread (&process_stats.hThread,  L"Process Monitor",  SK_MonitorProcess);

  if (config.disk.show)
    SpawnMonitorThread (&disk_stats.hThread,     L"Disk Monitor",     SK_MonitorDisk);

  if (config.pagefile.show)
    SpawnMonitorThread (&pagefile_stats.hThread, L"Pagefile Monitor", SK_MonitorPagefile);
}


void
SK_LoadGPUVendorAPIs (void)
{
  dll_log.Log (L"[  NvAPI   ] Initializing NVIDIA API          (NvAPI)...");

  nvapi_init =
    sk::NVAPI::InitializeLibrary (SK_GetHostApp ());

  dll_log.Log (L"[  NvAPI   ]              NvAPI Init         { %s }",
                                                     nvapi_init ? L"Success" :
                                                                  L"Failed ");

  if (nvapi_init)
  {
    int num_sli_gpus =
      sk::NVAPI::CountSLIGPUs ();

    dll_log.Log ( L"[  NvAPI   ] >> NVIDIA Driver Version: %s",
                    sk::NVAPI::GetDriverVersion ().c_str () );

    int gpu_count =
      sk::NVAPI::CountPhysicalGPUs ();

    dll_log.Log ( gpu_count > 1 ? L"[  NvAPI   ]  * Number of Installed NVIDIA GPUs: %i  "
                                  L"{ SLI: '%s' }"
                                :
                                  L"[  NvAPI   ]  * Number of Installed NVIDIA GPUs: %i  { '%s' }",
                                    gpu_count > 1 ? num_sli_gpus :
                                                    num_sli_gpus + 1,
                                      sk::NVAPI::EnumGPUs_DXGI ()[0].Description );

    if (num_sli_gpus > 0)
    {
      DXGI_ADAPTER_DESC* sli_adapters =
        sk::NVAPI::EnumSLIGPUs ();

      int sli_gpu_idx = 0;

      while (*sli_adapters->Description != L'\0')
      {
        dll_log.Log ( L"[  NvAPI   ]   + SLI GPU %d: %s",
                        sli_gpu_idx++,
                          (sli_adapters++)->Description );
      }
    }

    //
    // Setup a framerate limiter and (if necessary) restart
    //
    bool restart = (! sk::NVAPI::SetFramerateLimit (0));

    //
    // Install SLI Override Settings
    //
    if (sk::NVAPI::CountSLIGPUs () && config.nvidia.sli.override)
    {
      if (! sk::NVAPI::SetSLIOverride
              ( SK_GetDLLRole (),
                  config.nvidia.sli.mode.c_str (),
                    config.nvidia.sli.num_gpus.c_str (),
                      config.nvidia.sli.compatibility.c_str ()
              )
         )
      {
        restart = true;
      }
    }

    if (restart)
    {
      dll_log.Log (L"[  NvAPI   ] >> Restarting to apply NVIDIA driver settings <<");

      ShellExecute ( GetDesktopWindow (),
                       L"OPEN",
                         SK_GetHostApp (),
                           nullptr,
                             nullptr,
                               SW_SHOWDEFAULT );
      exit (0);
    }
  }

  // Not NVIDIA, maybe AMD?
  else
  {
    dll_log.Log (L"[DisplayLib] Initializing AMD Display Library (ADL)...");

    BOOL adl_init =
      SK_InitADL ();

    dll_log.Log   (L"[DisplayLib]              ADL   Init         { %s }",
                                                     adl_init ? L"Success" :
                                                                L"Failed ");

    // Yes, AMD driver is in working order ...
    if (adl_init > 0)
    {
      dll_log.Log ( L"[DisplayLib]  * Number of Reported AMD Adapters: %i (%i active)",
                      SK_ADL_CountPhysicalGPUs (),
                        SK_ADL_CountActiveGPUs () );
    }
  }

  HMODULE hMod =
    GetModuleHandle (SK_GetHostApp ());

  if (hMod != nullptr)
  {
    auto* dwOptimus =
      reinterpret_cast <DWORD *> (
        GetProcAddress ( hMod,
                           "NvOptimusEnablement" )
      );

    if (dwOptimus != nullptr)
    {
      dll_log.Log ( L"[Hybrid GPU]  NvOptimusEnablement..................: 0x%02X (%s)",
                      *dwOptimus,
                    ((*dwOptimus) & 0x1) ? L"Max Perf." :
                                           L"Don't Care" );
    }

    else
    {
      dll_log.Log (L"[Hybrid GPU]  NvOptimusEnablement..................: UNDEFINED");
    }

    auto* dwPowerXpress =
      reinterpret_cast <DWORD *> (
        GetProcAddress ( hMod,
                           "AmdPowerXpressRequestHighPerformance" )
      );

    if (dwPowerXpress != nullptr)
    {
      dll_log.Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: 0x%02X (%s)",
        *dwPowerXpress,
        (*dwPowerXpress & 0x1) ? L"High Perf." :
                                 L"Don't Care" );
    }

    else
      dll_log.Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: UNDEFINED");

    dll_log.LogEx (false, L"================================================"
                          L"===========================================\n" );
  }
}

void
__stdcall
SK_InitFinishCallback (void);

void
__stdcall
SK_InitCore (std::wstring, void* callback)
{
  using finish_pfn   = void (WINAPI *)  (void);
  using callback_pfn = void (WINAPI *)(_Releases_exclusive_lock_ (init_mutex) finish_pfn);

  auto callback_fn =
    (callback_pfn)callback;


  init_mutex->lock ();

  switch (SK_GetCurrentGameID ())
  {
#ifdef _WIN64
    case SK_GAME_ID::NieRAutomata:
      SK_FAR_InitPlugin ();
      break;

    case SK_GAME_ID::BlueReflection:
      SK_IT_InitPlugin ();
      break;

    case SK_GAME_ID::DotHackGU:
      SK_DGPU_InitPlugin ();
      break;

    case SK_GAME_ID::NiNoKuni2:
      extern void
      SK_NNK2_InitPlugin (void);
      SK_NNK2_InitPlugin ();
      break;
#else
    case SK_GAME_ID::SecretOfMana:
      SK_SOM_InitPlugin ();
      break;

    case SK_GAME_ID::DragonBallFighterZ:
      wchar_t      wszPath       [MAX_PATH * 2] = { };
      wchar_t      wszWorkingDir [MAX_PATH * 2] = { };
      wcscpy      (wszWorkingDir, SK_GetHostPath  ());
      PathAppendW (wszWorkingDir, L"RED\\Binaries\\Win64\\");

      wcscpy      (wszPath,       wszWorkingDir);
      PathAppendW (wszPath,       L"RED-Win64-Shipping.exe");

      ShellExecuteW (0, L"open", wszPath, L"-eac-nop-loaded", wszWorkingDir, SW_SHOWNORMAL);
      ExitProcess   (0);
      break;
#endif
  }

  callback_fn (SK_InitFinishCallback);
}


void
WaitForInit (void)
{
  const auto _SpinMax = 32;

  if (ReadAcquire (&__SK_Init) == 1)
    return;

  LONG dwThreadId =
    (LONG)GetCurrentThreadId ();

  while (ReadPointerAcquire (&hInitThread) != INVALID_HANDLE_VALUE)
  {
    if ( ReadAcquire (&dwInitThreadId) == dwThreadId )
      break;

    for (int i = 0; i < _SpinMax && (ReadPointerAcquire (&hInitThread) != INVALID_HANDLE_VALUE); i++)
      ;

    if ( ReadPointerAcquire (&hInitThread) == INVALID_HANDLE_VALUE )
      break;

    if ( WAIT_OBJECT_0 ==
           MsgWaitForMultipleObjectsEx (1, const_cast <HANDLE *>(&hInitThread), 16UL, QS_ALLINPUT, MWMO_ALERTABLE) )
      break;
  }


  if (! InterlockedCompareExchange (&__SK_Init, TRUE, FALSE))
  {
    if ( ReadAcquire        (&dwInitThreadId) != dwThreadId &&
         ReadPointerAcquire (&hInitThread)    != INVALID_HANDLE_VALUE )
    {
      InterlockedExchange (&dwInitThreadId, 0);

      CloseHandle (
        reinterpret_cast <HANDLE> (
          InterlockedExchangePointer ( const_cast <void **> (&hInitThread),
                                         INVALID_HANDLE_VALUE )
        )
      );
    }

    // Load user-defined DLLs (Lazy)
    SK_RunLHIfBitness ( 64, SK_LoadLazyImports64 (),
                            SK_LoadLazyImports32 () );

    if (config.system.handle_crashes)
      SK::Diagnostics::CrashHandler::Reinstall ();
  }
}



#include <SpecialK/commands/mem.inl>
#include <SpecialK/commands/update.inl>

auto SK_UnpackD3DShaderCompiler =
[](void) -> void
{
  HMODULE hModSelf = 
    SK_GetDLL ();

  HRSRC res =
    FindResource ( hModSelf, MAKEINTRESOURCE (IDR_D3DCOMPILER_PACKAGE), L"7ZIP" );

  if (res)
  {
    SK_LOG0 ( ( L"Unpacking D3DCompiler_43.dll because user does not have June 2010 DirectX Redistributables installed." ),
                L"D3DCompile" );

    DWORD   res_size     =
      SizeofResource ( hModSelf, res );

    HGLOBAL packed_compiler =
      LoadResource   ( hModSelf, res );

    if (! packed_compiler) return;


    const void* const locked =
      (void *)LockResource (packed_compiler);


    if (locked != nullptr)
    {
      wchar_t      wszArchive     [MAX_PATH * 2 + 1] = { };
      wchar_t      wszDestination [MAX_PATH * 2 + 1] = { };

      wcscpy (wszDestination, SK_GetHostPath ());

      if (GetFileAttributesW (wszDestination) == INVALID_FILE_ATTRIBUTES)
        SK_CreateDirectories (wszDestination);

      wcscpy      (wszArchive, wszDestination);
      PathAppendW (wszArchive, L"D3DCompiler_43.7z");

      SK_LOG0 ( ( L" >> Archive: %s [Destination: %s]", wszArchive,wszDestination ),
                  L"D3DCompile" );

      FILE* fPackedCompiler =
        _wfopen   (wszArchive, L"wb");

      fwrite      (locked, 1, res_size, fPackedCompiler);
      fclose      (fPackedCompiler);

      using SK_7Z_DECOMP_PROGRESS_PFN = int (__stdcall *)(int current, int total);

      extern
      HRESULT
      SK_Decompress7zEx ( const wchar_t*            wszArchive,
                          const wchar_t*            wszDestination,
                          SK_7Z_DECOMP_PROGRESS_PFN callback );

      SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
      DeleteFileW       (wszArchive);
    }

    UnlockResource (packed_compiler);
  }
};

void
__stdcall
SK_InitFinishCallback (void)
{
  bool rundll_invoked =
    (StrStrIW (SK_GetHostApp (), L"Rundll32") != nullptr);

  if (rundll_invoked || SK_IsSuperSpecialK ())
  {
    init_mutex->unlock ();
    return;
  }

  bool local_install = false;
  if (! SK_COMPAT_IsSystemDllInstalled (L"D3DCompiler_43.dll", &local_install))
  {
    if (! local_install)
    {
      SK_UnpackD3DShaderCompiler ();
    }

    SK_ImGui_Warning (L"Your system is missing the June 2010 DirectX Runtime.\t\t\n"
                      L"\n"
                      L"\t\t\t\t* Please install it as soon as possible.");
  }

  // NOTE: This is case-sensitive and a nightmare to debug; always use this
  //         unorthodox case because it's what the DLL uses internally as
  //           its name (unrelated to the name in System32\...)
  if ( FAILED (__HrLoadAllImportsForDll ("D3DCOMPILER_43.dll")) )
    abort ();


  SK_DeleteTemporaryFiles ();
  SK_DeleteTemporaryFiles (L"Version", L"*.old");

  SK::Framerate::Init ();

  extern int32_t SK_D3D11_amount_to_purge;
  SK_GetCommandProcessor ()->AddVariable (
    "VRAM.Purge",
      new SK_IVarStub <int32_t> (
        (int32_t *)&SK_D3D11_amount_to_purge
      )
  );

  SK_GetCommandProcessor ()->AddVariable (
    "GPU.StatPollFreq",
      new SK_IVarStub <float> (
        &config.gpu.interval
      )
  );

  SK_GetCommandProcessor ()->AddVariable (
    "ImGui.FontScale",
      new SK_IVarStub <float> (
        &config.imgui.scale
      )
  );

  SK_InitRenderBackends ();

  SK_GetCommandProcessor ()->AddCommand ("mem",       new skMemCmd    ());
  SK_GetCommandProcessor ()->AddCommand ("GetUpdate", new skUpdateCmd ());


  //
  // Game-Specific Stuff that I am not proud of
  //
  switch (SK_GetCurrentGameID ())
  {
#ifdef _WIN64
    case SK_GAME_ID::DarkSouls3:
      SK_DS3_InitPlugin ();
      break;
#else
    case SK_GAME_ID::Tales_of_Zestiria:
      SK_GetCommandProcessor ()->ProcessCommandFormatted (
        "TargetFPS %f",
          config.render.framerate.target_fps
      );
      break;
#endif
  }


  // Get rid of the game output log if the user doesn't want it...
  if (! config.system.game_output)
  {
    game_debug.close ();
    game_debug.silent = true;
  }


  const wchar_t* config_name =
    SK_GetBackend ();

  // Use a generic "SpecialK" name instead of the primary wrapped/hooked API name
  //   for this DLL when it is injected at run-time rather than a proxy DLL.
  if (SK_IsInjected ())
    config_name = L"SpecialK";

  SK_SaveConfig (config_name);

  SK_Console::getInstance ()->Start ();

  if (! (SK_GetDLLRole () & DLL_ROLE::DXGI))
    SK::DXGI::StartBudgetThread_NoAdapter ();

  dll_log.LogEx (false, L"----------------------------------------------- -"
                        L"-------------------------------------------\n" );
  dll_log.Log   (       L"[ SpecialK ] === Initialization Finished! ===   "
                        L"       (%6.2f ms)",
                          SK_DeltaPerfMS ( SK_GetInitParams ().start_time.QuadPart, 1 )
                );
  dll_log.LogEx (false, L"------------------------------------------------"
                        L"-------------------------------------------\n" );

  // NvAPI takes an excessively long time to startup and we don't need it
  //   immediately...
  CreateThread (nullptr, 0, [](LPVOID) -> DWORD
  {
    SetCurrentThreadDescription (    L"[SK] GPU Vendor Support Library Thread" );
    SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_LOWEST );
    SetThreadPriorityBoost      ( SK_GetCurrentThread (), TRUE                   );

    SleepEx (15, FALSE);

    SK_LoadGPUVendorAPIs ();

    SK_Thread_CloseSelf ();

    return 0;
  }, nullptr, 0x00, nullptr);

  init_mutex->unlock     ();
}


DWORD
WINAPI
CheckVersionThread (LPVOID)
{
  SetCurrentThreadDescription (                   L"[SK] Auto-Update Thread"   );
  SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_LOWEST );
  SleepEx                     ( 5,                      FALSE                  );

  // If a local repository is present, use that.
  if (GetFileAttributes (LR"(Version\installed.ini)") == INVALID_FILE_ATTRIBUTES)
  {
    if (SK_FetchVersionInfo (L"SpecialK"))
    {
      // ↑ Check, but ↓ don't update unless running the global injector version
      if ( (SK_IsInjected () && (! SK_IsSuperSpecialK ())) )
      {
        SK_UpdateSoftware (L"SpecialK");
      }
    }
  }

  SK_Thread_CloseSelf ();

  return 0;
}


DWORD
WINAPI
DllThread (LPVOID user)
{
  InterlockedExchange (&dwInitThreadId, GetCurrentThreadId ());

  SetCurrentThreadDescription (                 L"[SK] Primary Initialization Thread" );
  SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL );
  SetThreadPriorityBoost      ( SK_GetCurrentThread (), TRUE                          );

  auto* params =
    static_cast <init_params_s *> (user);
 
  SK_InitCore ( params->backend, params->callback );

  return 0;
}


std::wstring
SK_RecursiveFileSearch ( const wchar_t* wszDir,
                         const wchar_t* wszFile )
{
  std::wstring found = L"";

  wchar_t   wszPath [MAX_PATH * 2] = { };
  swprintf (wszPath, LR"(%s\*)", wszDir);

  WIN32_FIND_DATA fd          = {   };
  HANDLE          hFind       =
    FindFirstFileW ( wszPath, &fd);

  if (hFind == INVALID_HANDLE_VALUE) { return found; }

  do
  {
    if ( wcscmp (fd.cFileName, L".")  == 0 ||
         wcscmp (fd.cFileName, L"..") == 0 )
    {
      continue;
    }

    if (! _wcsicmp (fd.cFileName, wszFile))
    {
      found = wszDir;
      found += L"\\"; found += wszFile;

      break;
    }

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      wchar_t   wszDescend [MAX_PATH * 2] = { };
      swprintf (wszDescend, LR"(%s\%s)", wszDir, fd.cFileName);

      found = SK_RecursiveFileSearch (wszDescend, wszFile);
    }

  } while ((found.empty ()) && FindNextFile (hFind, &fd));

  FindClose (hFind);

  return found;
}


bool
__stdcall
SK_HasGlobalInjector (void)
{
  static int last_test = 0;

  if (last_test == 0)
  {
    wchar_t wszBasePath [MAX_PATH * 2 - 1] = { };

    wcsncpy ( wszBasePath,
                std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\)" ).c_str (),
                  MAX_PATH - 1 );

    lstrcatW (wszBasePath, SK_RunLHIfBitness ( 64, L"SpecialK64.dll",
                                                   L"SpecialK32.dll" ));

    bool result = (GetFileAttributesW (wszBasePath) != INVALID_FILE_ATTRIBUTES);
    last_test   = result ? 1 : -1;
  }

  return (last_test != -1);
}

extern std::pair <std::queue <DWORD>, BOOL> __stdcall SK_BypassInject (void);

struct sk_user_profile_s
{
  wchar_t wszProfile    [MAX_PATH] = { };
  wchar_t wszDocs       [MAX_PATH] = { };
  wchar_t wszEnvProfile [MAX_PATH] = { };
  wchar_t wszEnvDocs    [MAX_PATH] = { };
} static user_profile;

void
__stdcall
SK_EstablishRootPath (void)
{
  FILE* fTest = 
    _wfopen (L"SpecialK.permissions", L"wtc+");

  if (fTest != nullptr)
  {
    fclose      (fTest);
    DeleteFileW (L"SpecialK.permissions");
  }

  // File permissions don't permit us to store logs in the game's directory,
  //   so implicitly turn on the option to relocate this stuff.
  if (! fTest) config.system.central_repository = true;


  wchar_t wszConfigPath [MAX_PATH * 2] = { };

  // Store config profiles in a centralized location rather than relative to the game's executable
  //
  //   Currently, this location is always Documents\My Mods\SpecialK\
  //
  if (config.system.central_repository)
  {
    if (! SK_IsSuperSpecialK ())
    {
      lstrcpynW ( SKX_GetRootPath (),
                    std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)" ).c_str (),
                      MAX_PATH - 1 );
    }

    else
    {
      GetCurrentDirectory (MAX_PATH, SKX_GetRootPath ());
    }

    lstrcpynW (wszConfigPath, SKX_GetRootPath (), MAX_PATH);
    lstrcatW  (wszConfigPath, LR"(\Profiles\)");
    lstrcatW  (wszConfigPath, SK_GetHostApp  ());
  }


  // Relative to game's executable path
  //
  else
  {
    if (! SK_IsSuperSpecialK ())
    {
      lstrcpynW (SKX_GetRootPath (), SK_GetHostPath (), MAX_PATH);
    }

    else
    {
      GetCurrentDirectory (MAX_PATH, SKX_GetRootPath ());
    }

    lstrcpynW (wszConfigPath, SKX_GetRootPath (), MAX_PATH);
  }


  // Not using the ShellW API because at this (init) stage,
  //   we can only reliably use Kernel32 functions.
  lstrcatW (SKX_GetRootPath (), L"\\");
  lstrcatW (wszConfigPath,      L"\\");

  SK_SetConfigPath (wszConfigPath);
}

////void
////__stdcall
////SK_ReenterCore  (void) // During startup, we have the option of bypassing init and resuming later
////{
////  auto* params =
////    static_cast <init_params_s *> (&reentrant_core);
////
////  SK_StartupCore (params->backend.c_str (), params->callback);
////}


typedef BOOL (WINAPI *SetProcessDEPPolicy_pfn)(_In_ DWORD dwFlags);


#include <SpecialK/steam_api.h>

uint32_t
SK_Steam_GetAppID_NoAPI (void)
{
  if (StrStrIW (SK_GetHostPath (), LR"(SteamApps\common)") == nullptr)
    return 0;

  DWORD    dwSteamGameIdLen     =  0 ;
  uint32_t AppID                =  0 ;
  char     szSteamGameId [0x21] = { };

  if (GetFileAttributesW (L"steam_appid.txt") != INVALID_FILE_ATTRIBUTES)
  {
    FILE* fAppID = fopen ("steam_appid.txt", "r");

    if (fAppID != nullptr)
    {
      fgets (szSteamGameId, 0x20, fAppID);
                          fclose (fAppID);

      dwSteamGameIdLen = (DWORD)strlen (szSteamGameId);
      AppID            =          atoi (szSteamGameId);
    }
  }

  if (AppID == 0)
  {
    dwSteamGameIdLen =
      GetEnvironmentVariableA ( "SteamGameId",
                                  szSteamGameId,
                                    0x20 );

    
    if (dwSteamGameIdLen > 1)
      AppID = atoi (szSteamGameId);


    //if (GetFileAttributesW (L"steam_appid.txt") == INVALID_FILE_ATTRIBUTES)
    //{
    //  FILE* fAppID =
    //    fopen ("steam_appid.txt", "w+");
    //
    //  if (fAppID != nullptr)
    //  {
    //    fputs   (szSteamGameId,   fAppID);
    //    fflush                   (fAppID);
    //                     fclose  (fAppID);
    //  }
    //}
  }

  if (AppID != 0)
    config.steam.appid = AppID;

  if (dwSteamGameIdLen > 1 && config.steam.appid != 0)
  {
    __declspec (noinline)
    const wchar_t*
    __stdcall
    SK_GetConfigPathEx (bool reset = false);

    SK_GetConfigPathEx (true);

    app_cache_mgr.loadAppCacheForExe (SK_GetFullyQualifiedApp ());
    app_cache_mgr.addAppToCache
    (
      SK_GetFullyQualifiedApp (),
                SK_GetHostApp (),
            SK_UTF8ToWideChar (
             SK_UseManifestToGetAppName   (AppID)
                               ).c_str (), AppID
    );

    app_cache_mgr.saveAppCache       (true);
    app_cache_mgr.loadAppCacheForExe (SK_GetFullyQualifiedApp ());

    // Trigger profile migration if necessary
    app_cache_mgr.getConfigPathForAppID (AppID);

    return config.steam.appid;
  }

  return 0;
}

const wchar_t* __SK_BootedCore = L"";
bool
__stdcall
SK_StartupCore (const wchar_t* backend, void* callback)
{
  QueryPerformanceCounter_Original =
    reinterpret_cast <QueryPerformanceCounter_pfn> (
      GetProcAddress (
        GetModuleHandle ( L"NtDll.dll"),
                            "RtlQueryPerformanceCounter" )
    );

  __SK_BootedCore = backend;

  // Before loading any config files, test the game's environment variables
  //  to determine if the Steam client has given us the AppID without having
  //    to initialize SteamAPI first.
  SK_Steam_GetAppID_NoAPI      ();

  SK_MinHook_Init              ();
  SK_Thread_InitDebugExtras    ();
//SK_COMPAT_FixNahimicDeadlock (); // Use load-time link dependencies to resolve race conditions
//                                 //   involving WASAPI and MMDevAPI in MSI Nahimic.

  static SetProcessDEPPolicy_pfn _SetProcessDEPPolicy =
    (SetProcessDEPPolicy_pfn)
      GetProcAddress ( GetModuleHandleW (L"Kernel32.dll"),
                         "SetProcessDEPPolicy" );

  // Disable DEP for stupid Windows 7 machines
  if (_SetProcessDEPPolicy != nullptr) _SetProcessDEPPolicy (0);


  // Allow users to centralize all files if they want
  //
  //   Stupid hack, if the application is running with a different working-directory than
  //     the executable -- compensate!
  if ( GetFileAttributes ( ( std::wstring (SK_GetHostPath ()) +
                             std::wstring (L"\\SpecialK.central") ).c_str () ) != INVALID_FILE_ATTRIBUTES )
    config.system.central_repository = true;

  if (SK_IsInjected ())
    config.system.central_repository = true;

  SK_EstablishRootPath ();
  SK_CreateDirectories (SK_GetConfigPath ());

  if (config.system.central_repository)
  {
    // Create Symlink for end-user's convenience
    if ( GetFileAttributes ( ( std::wstring (SK_GetHostPath ()) +
                               std::wstring (LR"(\SpecialK\)")
                             ).c_str ()
                           ) == INVALID_FILE_ATTRIBUTES )
    {
      std::wstring link (SK_GetHostPath ());
                   link += LR"(\SpecialK\)";

      CreateSymbolicLink (
        link.c_str         (),
          SK_GetConfigPath (),
            SYMBOLIC_LINK_FLAG_DIRECTORY
      );
    }

    if ( GetFileAttributes ( ( std::wstring (SK_GetConfigPath ()) +
                               std::wstring (L"Game\\") ).c_str ()
                           ) == INVALID_FILE_ATTRIBUTES )
    {
      std::wstring link (SK_GetConfigPath ());
                   link += L"Game\\";

      CreateSymbolicLink (
        link.c_str         (),
          SK_GetHostPath   (),
            SYMBOLIC_LINK_FLAG_DIRECTORY
      );
    }
  }

  extern bool __SK_RunDLL_Bypass;

  bool rundll_invoked =
    (StrStrIW (SK_GetHostApp (), L"Rundll32") != nullptr);
  bool skim =
    rundll_invoked || SK_IsSuperSpecialK () || __SK_RunDLL_Bypass;

  __SK_bypass |= skim;


  auto& init_ =
    SKX_GetInitParams ();

  init_          = {     };
  init_.backend  = backend;
  init_.callback = callback;


  init_mutex->lock ();

  SK_SetBackend (backend);

  // Injection Compatibility Menu
  if ( (! __SK_bypass) && (GetAsyncKeyState (VK_SHIFT  ) & 0x8000) != 0 &&
                          (GetAsyncKeyState (VK_CONTROL) & 0x8000) != 0 )
  {
    InterlockedExchange (&__SK_Init, -1);
    __SK_bypass = true;

    SK_BypassInject ();
  }

  else
  {
    SetThreadPriority (SK_GetCurrentThread (), THREAD_PRIORITY_ABOVE_NORMAL);
    SetThreadPriorityBoost ( SK_GetCurrentThread (), FALSE                 );

    wchar_t   log_fname [MAX_PATH + 2] = { };
    swprintf (log_fname, L"logs/%s.log", SK_IsInjected () ? L"SpecialK" : backend);

    dll_log.init (log_fname, L"wS+,ccs=UTF-8");
    dll_log.Log  ( L"%s.log created\t\t(Special K  %s,  %hs)",
                     SK_IsInjected () ? L"SpecialK" :
                                        backend,
                                                      SK_VER_STR,
                                        __DATE__ );

    bool blacklist =
      SK_IsInjected () &&
      (GetFileAttributesW (SK_GetBlacklistFilename ()) != INVALID_FILE_ATTRIBUTES);

    //
    // Internal blacklist, the user will have the ability to setup their
    //   own later...
    //
    if ( blacklist )
    {
      init_mutex->unlock ();

      return false;
    }

    init_.start_time = SK_QueryPerf ();

    void
    SK_NvAPI_PreInitHDR (void);

    SK_NvAPI_PreInitHDR    ();
    SK_WMI_Init            ();
    SK_InitCompatBlacklist ();

    // Do this from the startup thread [these functions queue, but don't apply]
    SK_HookWinAPI       ();
    SK_Input_PreInit    (); // Hook only symbols in user32 and kernel32

    // For the global injector, when not started by SKIM, check its version
    if ( (SK_IsInjected () && (! SK_IsSuperSpecialK ())) )
      CreateThread (nullptr, 0, CheckVersionThread, nullptr, 0x00, nullptr);

    // Don't let Steam prevent me from attaching a debugger at startup
    game_debug.init                  (L"logs/game_output.log", L"w");
    game_debug.lockless = true;
    SK::Diagnostics::Debugger::Allow ();

    SK::Diagnostics::CrashHandler::InitSyms ();
  }



  if (skim)
  {
    init_mutex->unlock ();

    return TRUE;
  }



  budget_log.init ( LR"(logs\dxgi_budget.log)", L"wc+,ccs=UTF-8" );

  dll_log.LogEx (false,
    L"------------------------------------------------------------------------"
    L"-------------------\n");


  std::wstring   module_name   = SK_GetModuleName (SK_GetDLL ());
  const wchar_t* wszModuleName = module_name.c_str ();

  dll_log.Log   (      L">> (%s) [%s] <<",
                         SK_GetHostApp (),
                           wszModuleName );

  const wchar_t* config_name = backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  LARGE_INTEGER liStartConfig = SK_CurrentPerf ();

  dll_log.LogEx (true, L"Loading user preferences from %s.ini... ", config_name);

  if (SK_LoadConfig (config_name))
    dll_log.LogEx (false, L"done! (%6.2f ms)\n",
      SK_DeltaPerfMS (liStartConfig.QuadPart, 1));

  else
  {
    dll_log.LogEx (false, L"failed!\n");

    std::wstring default_name (L"default_");
                 default_name += config_name;

    std::wstring default_ini (default_name + L".ini");

    if (GetFileAttributesW (default_ini.c_str ()) != INVALID_FILE_ATTRIBUTES)
    {
      dll_log.LogEx (true, L"Loading default preferences from %s.ini... ", default_name.c_str ());

      if (! SK_LoadConfig (default_name))
        dll_log.LogEx (false, L"failed!\n");
      else
        dll_log.LogEx (false, L"done!\n");
    }

    // If no INI file exists, write one immediately.
    dll_log.LogEx (true, L"  >> Writing base INI file, because none existed... ");
    SK_SaveConfig (config_name);
    dll_log.LogEx (false, L"done!\n");
  }

//config.compatibility.rehook_loadlibrary = true;

  if (config.system.display_debug_out)
    SK::Diagnostics::Debugger::SpawnConsole ();

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Init     ();


  // Steam Overlay and SteamAPI Manipulation
  //
  if (! config.steam.silent)
  {
    config.steam.force_load_steamapi = false;

    // TODO: Rename -- this initializes critical sections and performance
    //                   counters needed by SK's SteamAPI backend.
    //
    SK_Steam_InitCommandConsoleVariables  ();

    // Lazy-load SteamAPI into a process that doesn't use it; this brings
    //   a number of general-purpose benefits (such as battery charge monitoring).
    bool kick_start = config.steam.force_load_steamapi;

    if ((! SK_Steam_TestImports (GetModuleHandle (nullptr))))
    {
      // Implicitly kick-start anything in SteamApps\common that does not import
      //   SteamAPI...
      if ((! kick_start) && config.steam.auto_inject)
      {
        if (StrStrIW (SK_GetHostPath (), LR"(SteamApps\common)") != nullptr)
        {
          extern const std::wstring
          SK_Steam_GetDLLPath (void);

          // Only do this if the game doesn't have a copy of the DLL lying around somewhere,
          //   because if we init Special K's SteamAPI DLL, the game's will fail to init and
          //     the game won't be happy about that!
          kick_start = (! SK_Modules.LoadLibrary (SK_Steam_GetDLLPath ().c_str ())) || config.steam.force_load_steamapi;
        }
      }

      if (kick_start)
        SK_Steam_KickStart ();
    }

    SK_Steam_PreHookCore ();
  }

  if (__SK_bypass)
    goto BACKEND_INIT;


  SK_EnumLoadedModules (SK_ModuleEnum::PreLoad);


BACKEND_INIT:
  dll_log.LogEx (false,
    L"----------------------------------------------------------------------"
    L"---------------------\n");

  // If the module name is this, then we need to load the system-wide DLL...
  wchar_t   wszProxyName [MAX_PATH];
  wsprintf (wszProxyName, L"%s.dll", backend);


#ifndef _WIN64
  //
  // TEMP HACK: dgVoodoo2
  //
  if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    wsprintf (wszProxyName, L"%s\\PlugIns\\ThirdParty\\dgVoodoo\\d3d8.dll",
                std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());

  else if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    wsprintf (wszProxyName, L"%s\\PlugIns\\ThirdParty\\dgVoodoo\\ddraw.dll",
                std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK").c_str ());
#endif


  wchar_t wszBackendDLL [MAX_PATH + 2] = { };
  wchar_t wszWorkDir    [MAX_PATH + 2] = { };

  wcsncpy (wszBackendDLL, SK_GetSystemDirectory (), MAX_PATH);

  GetCurrentDirectoryW (MAX_PATH, wszWorkDir);
       SK_StripUserNameFromPathW (wszWorkDir);

  dll_log.Log (L" Working Directory:          %s", SK_StripUserNameFromPathW (std::wstring (wszWorkDir).data    ()));
  dll_log.Log (L" System Directory:           %s", SK_StripUserNameFromPathW (std::wstring (wszBackendDLL).data ()));

  lstrcatW (wszBackendDLL, L"\\");
  lstrcatW (wszBackendDLL, backend);
  lstrcatW (wszBackendDLL, L".dll");

  const wchar_t* dll_name = wszBackendDLL;

  if (! SK_Path_wcsicmp (wszProxyName, wszModuleName))
    dll_name = wszBackendDLL;
  else
    dll_name = wszProxyName;

  bool load_proxy = false;

  if (! SK_IsInjected ())
  {
    for (auto& import : imports)
    {
      if (import.role != nullptr && import.role->get_value () == backend)
      {
        dll_log.LogEx (true, L" Loading proxy %s.dll:    ", backend);
        dll_name   = _wcsdup (import.filename->get_value ().c_str ());
        load_proxy = true;
        break;
      }
    }
  }

  if (! load_proxy)
    dll_log.LogEx (true, L" Loading default %s.dll: ", backend);

  // Pre-Load the original DLL into memory
  if (dll_name != wszBackendDLL) {
                  SK_Modules.LoadLibraryLL (wszBackendDLL);
    backend_dll = SK_Modules.LoadLibraryLL (dll_name);
  }

  else
    backend_dll = SK_Modules.LoadLibraryLL (dll_name);

  if (backend_dll != nullptr)
    dll_log.LogEx (false, L" (%s)\n",         SK_StripUserNameFromPathW (std::wstring (dll_name).data ()));
  else
    dll_log.LogEx (false, L" FAILED (%s)!\n", SK_StripUserNameFromPathW (std::wstring (dll_name).data ()));

  // Free the temporary string storage
  if (load_proxy)
    free ((void *)dll_name);

  dll_log.LogEx (false,
    L"----------------------------------------------------------------------"
    L"---------------------\n");


  if (config.system.silent)
  {
    dll_log.silent = true;

    std::wstring log_fnameW;

    if (! SK_IsInjected ())
      log_fnameW = backend;
    else
      log_fnameW = L"SpecialK";

    log_fnameW += L".log";

    DeleteFile (log_fnameW.c_str ());
  }

  else
    dll_log.silent = false;


  if (! __SK_bypass)
  {
    bool gl   = false, vulkan = false, d3d9  = false, d3d11 = false,
         dxgi = false, d3d8   = false, ddraw = false, glide = false;

    dxgi  |= GetModuleHandle (L"dxgi.dll")  != nullptr;
    d3d11 |= GetModuleHandle (L"d3d11.dll") != nullptr;
    d3d9  |= GetModuleHandle (L"d3d9.dll")  != nullptr;

    SK_TestRenderImports (
      GetModuleHandle (nullptr),
        &gl, &vulkan,
          &d3d9, &dxgi, &d3d11,
            &d3d8, &ddraw, &glide );

    if ((dxgi || d3d11 || d3d8 || ddraw) && config.apis.dxgi.d3d11.hook /*|| config.apis.dxgi.d3d12.hook*/)
    {
      SK_DXGI_QuickHook ();
    }

    if (d3d9 && (config.apis.d3d9.hook || config.apis.d3d9ex.hook))
    {
      SK_D3D9_QuickHook ();
    }


    if (config.steam.preload_overlay)
    {
      SK_Steam_LoadOverlayEarly ();
    }


    CreateThread (nullptr, 0x00, [](LPVOID) -> DWORD
    {
      SetCurrentThreadDescription (                          L"[SK] Init Cleanup Thread" );
      SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL );
      SetThreadPriorityBoost      ( SK_GetCurrentThread (), TRUE                         );

      WaitForInit         ();

      if (GetModuleHandle (L"dinput8.dll"))
        SK_Input_HookDI8  ();

      if (GetModuleHandle (L"dinput.dll"))
        SK_Input_HookDI7  ();

      SK_Input_Init       ();
      SK_ApplyQueuedHooks ();

      // Setup the compatibility backend, which monitors loaded libraries,
      //   blacklists bad DLLs and detects render APIs...
      SK_EnumLoadedModules (SK_ModuleEnum::PostLoad);

      SK_Thread_CloseSelf ();

      return 0;
    }, nullptr, 0x00, nullptr);


    InterlockedExchangePointer (
      const_cast <void **> (&hInitThread), (HANDLE)
        CreateThread ( nullptr,
                           0,
                             DllThread,
                               &init_,
                                 0x00,
                                   nullptr )
    ); // Avoid the temptation to wait on this thread
  }

  init_mutex->unlock ();

  return true;
}


struct {
  std::set <HWND> list;
  std::mutex      lock;
} dummy_windows;

HWND
SK_Win32_CreateDummyWindow (void)
{
  std::lock_guard <std::mutex> auto_lock (dummy_windows.lock);

  static WNDCLASSW wc          = {
    CS_OWNDC,
    DefWindowProcW,
    0x00, 0x00,
    SK_GetDLL        (                        ),
    LoadIcon         (nullptr, IDI_APPLICATION),
    LoadCursor       (nullptr, IDC_WAIT       ),
    static_cast <HBRUSH> (
      GetStockObject (         BLACK_BRUSH    )
                         ),
    nullptr,
    L"Special K Dummy Window Class"
  };
  static WNDCLASSW wc_existing = { };

  if ( RegisterClassW (&wc) ||
       GetClassInfo   ( SK_GetDLL (),
                          L"Special K Dummy Window Class",
                            &wc_existing ) )
  {
    HWND hWnd =
      CreateWindowExW ( 0L, L"Special K Dummy Window Class",
                            L"Special K Dummy Window",
                              WS_OVERLAPPED,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                  HWND_MESSAGE, nullptr,
                                    SK_GetDLL (), nullptr );

    if (hWnd != HWND_DESKTOP)
    {
      ShowWindowAsync (hWnd, SW_SHOWMINNOACTIVE);
      dummy_windows.list.emplace (hWnd);
    }

    return hWnd;
  }

  else
  {
    dll_log.Log (L"Window Class Registration Failed!");
  }

  return nullptr;
}

void
SK_Win32_CleanupDummyWindow (HWND hwnd)
{
  std::lock_guard <std::mutex> auto_lock (dummy_windows.lock);

  std::set <HWND> cleaned_windows;

  for ( auto& it : dummy_windows.list )
  {
    if (it == hwnd || hwnd == nullptr)
    {
      if (DestroyWindow (it))
      {
        cleaned_windows.emplace (it);
      }
    }
  }

  for ( auto& it : cleaned_windows )
    dummy_windows.list.erase (it);

  if (dummy_windows.list.empty ())
    UnregisterClassW ( L"Special K Dummy Window Class", SK_GetDLL () );
}

bool
__stdcall
SK_ShutdownCore (const wchar_t* backend)
{
  // Fast path for DLLs that were never really attached.
  extern __time64_t
        __SK_DLL_AttachTime;
  if (! __SK_DLL_AttachTime)
  {
    SK_MinHook_UnInit ();
    return true;
  }

  SK_PrintUnloadedDLLs (&dll_log);
  dll_log.LogEx ( false, 
               L"========================================================="
               L"========= (End  Unloads) ================================"
               L"==================================\n" );

  dll_log.Log (L"[ SpecialK ] *** Initiating DLL Shutdown ***");
  SK_Win32_CleanupDummyWindow ();


  if (config.window.background_mute)
    SK_SetGameMute (FALSE);

  // These games do not handle resolution correctly
  switch (SK_GetCurrentGameID ())
  {
    case SK_GAME_ID::DarkSouls3:
    case SK_GAME_ID::Fallout4:
    case SK_GAME_ID::FinalFantasyX_X2:
    case SK_GAME_ID::DisgaeaPC:
      ChangeDisplaySettingsA_Original (nullptr, CDS_RESET);
      break;
  }

  SK_AutoClose_Log (game_debug);
  SK_AutoClose_Log (   dll_log);

  SK_Console::getInstance ()->End ();

  SK::DXGI::ShutdownBudgetThread ();

  dll_log.LogEx    (true, L"[ GPU Stat ] Shutting down Prognostics Thread...          ");

  DWORD dwTime =
       timeGetTime ();
  SK_EndGPUPolling ();

  dll_log.LogEx    (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);


  SK_Steam_KillPump ();

  auto ShutdownWMIThread =
  []( volatile HANDLE&  hSignal,
      volatile HANDLE&  hThread,
               wchar_t* wszName )
  {
    if ( ReadPointerAcquire (&hSignal) == INVALID_HANDLE_VALUE &&
         ReadPointerAcquire (&hThread) == INVALID_HANDLE_VALUE )
      return;

    wchar_t wszFmtName [32] = { };

    lstrcatW (wszFmtName, wszName);
    lstrcatW (wszFmtName, L"...");

    dll_log.LogEx (true, L"[ WMI Perf ] Shutting down %-30s ", wszFmtName);

    DWORD dwTime = timeGetTime ();

    // Signal the thread to shutdown
    if (SignalObjectAndWait (hSignal, hThread, 1000UL, TRUE) != WAIT_OBJECT_0) // Give 1 second, and
    {                                                                          // then we're killing
      TerminateThread (hThread, 0x00);                                         // the thing!
    }

    CloseHandle (hThread);
                 hThread  = INVALID_HANDLE_VALUE;

    dll_log.LogEx (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);
  };

  ShutdownWMIThread (process_stats.hShutdownSignal,  process_stats.hThread,  L"Process Monitor" );
  ShutdownWMIThread (cpu_stats.hShutdownSignal,      cpu_stats.hThread,      L"CPU Monitor"     );
  ShutdownWMIThread (disk_stats.hShutdownSignal,     disk_stats.hThread,     L"Disk Monitor"    );
  ShutdownWMIThread (pagefile_stats.hShutdownSignal, pagefile_stats.hThread, L"Pagefile Monitor");

  const wchar_t* config_name = backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  if (sk::NVAPI::app_name != L"ds3t.exe" && SK_GetFramesDrawn () > 0)
  {
    dll_log.LogEx        (true,  L"[ SpecialK ] Saving user preferences to %10s.ini... ", config_name);
    dwTime = timeGetTime ();
    SK_SaveConfig        (config_name);
    dll_log.LogEx        (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);
  }


  SK_GetCurrentRenderBackend ().releaseOwnedResources ();


  SK_UnloadImports        ();
  SK::Framerate::Shutdown ();

  dll_log.LogEx        (true, L"[ SpecialK ] Shutting down MinHook...                     ");

  dwTime = timeGetTime ();
  SK_MinHook_UnInit    ();
  dll_log.LogEx        (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);


  dll_log.LogEx        (true, L"[ WMI Perf ] Shutting down WMI WbemLocator...             ");
  dwTime = timeGetTime ();
  SK_WMI_Shutdown      ();
  dll_log.LogEx        (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);



  if (nvapi_init)
    sk::NVAPI::UnloadLibrary ();


  dll_log.Log (L"[ SpecialK ] Custom %s.dll Detached (pid=0x%04x)",
    backend, GetCurrentProcessId ());

  if (config.system.handle_crashes)
    SK::Diagnostics::CrashHandler::Shutdown ();

  InterlockedExchange (&__SK_Init, FALSE);

  return true;
}




auto SK_UnpackCEGUI =
[](void) -> void
{
  HMODULE hModSelf = 
    SK_GetDLL ();

  HRSRC res =
    FindResource ( hModSelf, MAKEINTRESOURCE (IDR_CEGUI_PACKAGE), L"7ZIP" );

  if (res)
  {
    DWORD   res_size     =
      SizeofResource ( hModSelf, res );

    HGLOBAL packed_cegui =
      LoadResource   ( hModSelf, res );

    if (! packed_cegui) return;


    const void* const locked =
      (void *)LockResource (packed_cegui);


    if (locked != nullptr)
    {
      wchar_t      wszArchive     [MAX_PATH * 2 + 1] = { };
      wchar_t      wszDestination [MAX_PATH * 2 + 1] = { };

      _snwprintf ( wszDestination, MAX_PATH, LR"(%s\My Mods\SpecialK\)",
                     SK_GetDocumentsDir ().c_str () );

      if (GetFileAttributesW (wszDestination) == INVALID_FILE_ATTRIBUTES)
        SK_CreateDirectories (wszDestination);

      wcscpy      (wszArchive, wszDestination);
      PathAppendW (wszArchive, L"CEGUI.7z");

      FILE* fPackedCEGUI =
        _wfopen   (wszArchive, L"wb");

      fwrite      (locked, 1, res_size, fPackedCEGUI);
      fclose      (fPackedCEGUI);

      using SK_7Z_DECOMP_PROGRESS_PFN = int (__stdcall *)(int current, int total);

      extern
      HRESULT
      SK_Decompress7zEx ( const wchar_t*            wszArchive,
                          const wchar_t*            wszDestination,
                          SK_7Z_DECOMP_PROGRESS_PFN callback );

      SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
      DeleteFileW       (wszArchive);
    }

    UnlockResource (packed_cegui);
  }
};



extern void SK_ImGui_LoadFonts (void);
extern void SK_ImGui_Warning   (const wchar_t *wszMessage);


extern void SK_Window_RepositionIfNeeded (void);

void
SKX_Window_EstablishRoot (void)
{
  HWND  hWndActive     = GetActiveWindow     ();
  HWND  hWndFocus      = GetFocus            ();
  HWND  hWndForeground = GetForegroundWindow ();

  HWND  hWndTarget     = SK_GetCurrentRenderBackend ().windows.device;
  DWORD dwWindowPid    = 0;

  if (SK_Win32_IsGUIThread () && hWndTarget == HWND_DESKTOP)
  {
    GetWindowThreadProcessId (hWndActive, &dwWindowPid);
    if (dwWindowPid == GetCurrentProcessId ())
    {
      hWndTarget = hWndActive;
    }
  }

  if (hWndTarget == HWND_DESKTOP)
  { 
     GetWindowThreadProcessId (hWndFocus, &dwWindowPid);

     if (dwWindowPid == GetCurrentProcessId ())
     {
       hWndTarget = hWndFocus;
     }

     else
     {
       GetWindowThreadProcessId (hWndForeground, &dwWindowPid);

       if (dwWindowPid == GetCurrentProcessId ())
       {
         hWndTarget = hWndForeground;
       }
     }
  }

  SK_InstallWindowHook (hWndTarget);
}


__declspec (dllexport)
void
SK_ImGui_Toggle (void);

void
SK_ImGui_PollGamepad_EndFrame (void);


static volatile LONG CEGUI_Init = FALSE;

void
SetupCEGUI (SK_RenderAPI& LastKnownAPI)
{
  if ( (SK_GetCurrentRenderBackend ().api != SK_RenderAPI::Reserved) &&
        SK_GetCurrentRenderBackend ().api == LastKnownAPI            &&
       ( (! InterlockedCompareExchange (&CEGUI_Init, TRUE, FALSE)) ) )
  {
    InterlockedIncrement (&SK_GetCurrentRenderBackend ().frames_drawn);

    // Brutally stupid hack for brutally stupid OS (Windows 7)
    //
    //   1. Lock the DLL loader + Suspend all Threads
    //   2. Change Working Dir  + Delay-Load CEGUI DLLs
    //   3. Restore Working Dir
    //   4. Resume all Threads  + Unlock DLL Loader
    //
    //     >> Not necessary if the kernel supports altered DLL serarch
    //          paths <<
    //

    SK_LockDllLoader ();

    // Disable until we validate CEGUI's state
    config.cegui.enable = false;

    wchar_t wszCEGUIModPath [MAX_PATH]        = { };
    wchar_t wszCEGUITestDLL [MAX_PATH]        = { };
    wchar_t wszEnvPath      [ MAX_PATH + 32 ] = { };

    const wchar_t* wszArch = SK_RunLHIfBitness ( 64, L"x64",
                                                     L"Win32" );

    _swprintf (wszCEGUIModPath, LR"(%sCEGUI\bin\%s)", SK_GetRootPath (), wszArch);

    if (GetFileAttributes (wszCEGUIModPath) == INVALID_FILE_ATTRIBUTES)
    {
      _swprintf ( wszCEGUIModPath, LR"(%s\My Mods\SpecialK\CEGUI\bin\%s)",
                    SK_GetDocumentsDir ().c_str (), wszArch );

      _swprintf (wszEnvPath, LR"(CEGUI_PARENT_DIR=%s\My Mods\SpecialK\)", SK_GetDocumentsDir ().c_str ());
    }
    else
      _swprintf (wszEnvPath, L"CEGUI_PARENT_DIR=%s", SK_GetRootPath ());

    if (GetFileAttributes (wszCEGUIModPath) == INVALID_FILE_ATTRIBUTES)
    {
      SK_UnpackCEGUI ();
    }

    _wputenv  (wszEnvPath);

    lstrcatW  (wszCEGUITestDLL, wszCEGUIModPath);
    lstrcatW  (wszCEGUITestDLL, L"\\CEGUIBase-0.dll");

    wchar_t wszWorkingDir [MAX_PATH + 2] = { };

    if (! config.cegui.safe_init)
    {
      //std::queue <DWORD> tids =
      //  SK_SuspendAllOtherThreads ();

      GetCurrentDirectory    (MAX_PATH, wszWorkingDir);
      SetCurrentDirectory    (        wszCEGUIModPath);
    }

    // This is only guaranteed to be supported on Windows 8, but Win7 and Vista
    //   do support it if a certain Windows Update (KB2533623) is installed.
    using AddDllDirectory_pfn          = DLL_DIRECTORY_COOKIE (WINAPI *)(_In_ PCWSTR               NewDirectory);
    using RemoveDllDirectory_pfn       = BOOL                 (WINAPI *)(_In_ DLL_DIRECTORY_COOKIE Cookie);
    using SetDefaultDllDirectories_pfn = BOOL                 (WINAPI *)(_In_ DWORD                DirectoryFlags);

    static auto k32_AddDllDirectory =
      (AddDllDirectory_pfn)
        GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                           "AddDllDirectory" );

    static auto k32_RemoveDllDirectory =
      (RemoveDllDirectory_pfn)
        GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                           "RemoveDllDirectory" );

    static auto k32_SetDefaultDllDirectories =
      (SetDefaultDllDirectories_pfn)
        GetProcAddress ( GetModuleHandle (L"kernel32.dll"),
                           "SetDefaultDllDirectories" );

    if ( k32_AddDllDirectory          && k32_RemoveDllDirectory &&
         k32_SetDefaultDllDirectories &&

           GetFileAttributesW (wszCEGUITestDLL) != INVALID_FILE_ATTRIBUTES )
    {
      SK_StripUserNameFromPathW (wszCEGUITestDLL);

      dll_log.Log (L"[  CEGUI   ] Enabling CEGUI: (%s)", wszCEGUITestDLL);

      wchar_t wszEnvVar [ MAX_PATH + 32 ] = { };

      _swprintf (wszEnvVar, L"CEGUI_MODULE_DIR=%s", wszCEGUIModPath);
      _wputenv  (wszEnvVar);

      // This tests for the existence of the DLL before attempting to load it...
      auto DelayLoadDLL = [&](const char* szDLL)->
        bool
        {
          if (! config.cegui.safe_init)
            return SUCCEEDED ( __HrLoadAllImportsForDll (szDLL) );

          k32_SetDefaultDllDirectories (
            LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
            LOAD_LIBRARY_SEARCH_SYSTEM32        | LOAD_LIBRARY_SEARCH_USER_DIRS
          );

          DLL_DIRECTORY_COOKIE cookie = nullptr;
          bool                 ret    = false;

          __try
          {
            cookie =               k32_AddDllDirectory    (wszCEGUIModPath);
            ret    = SUCCEEDED ( __HrLoadAllImportsForDll (szDLL)           );
          }

          __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ?
                     EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
          {
          }
          k32_RemoveDllDirectory (cookie);

          return ret;
        };

      if (DelayLoadDLL ("CEGUIBase-0.dll"))
      {
        FILE* fTest = 
          _wfopen (L"CEGUI.log", L"wtc+");

        if (fTest != nullptr)
        {
          fclose (fTest);

          if (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::OpenGL)
          {
            if (config.apis.OpenGL.hook)
            {
              config.cegui.enable =
                DelayLoadDLL ("CEGUIOpenGLRenderer-0.dll");
            }
          }

          if ( SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D9 ||
               SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D9Ex )
          {
            if (config.apis.d3d9.hook || config.apis.d3d9ex.hook)
            {
              config.cegui.enable =
                DelayLoadDLL ("CEGUIDirect3D9Renderer-0.dll");
            }
          }

          if (config.apis.dxgi.d3d11.hook)
          {
            if ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
                 static_cast <int> (SK_RenderAPI::D3D11              ) )
            {
              config.cegui.enable =
                DelayLoadDLL ("CEGUIDirect3D11Renderer-0.dll");
            }
          }
        }

        else
        {
          const wchar_t* wszDisableMsg = 
            L"File permissions are preventing CEGUI from functioning;"
            L" it has been disabled.";

          SK_ImGui_Warning (
            SK_FormatStringW ( L"%s\n\n"
                               L"\t\t\t\t>> To fix this, run the game %s "
                                                         L"administrator.",
                               wszDisableMsg,
                                 SK_IsInjected              () &&
                              (! SK_Inject_IsAdminSupported ()) ?
                                   L"and SKIM64 as" :
                                              L"as" ).c_str ()
          );

          SK_LOG0 ( (L"%ws", wszDisableMsg),
                     L"  CEGUI   " );
        }
      }
    }

    // If we got this far and CEGUI's not enabled, it's because something went horribly wrong.
    if (! (config.cegui.enable && GetModuleHandle (L"CEGUIBase-0")))
      InterlockedExchange (&CEGUI_Init, FALSE);

    if (! config.cegui.safe_init)
    {
      SetCurrentDirectory (wszWorkingDir);
    //SK_ResumeThreads    (tids);
    }

    SK_UnlockDllLoader  ();
  }
}


struct {
  DWORD  dwTid     = 0;
  DWORD  dwTaskIdx = 0;                    // MMCSS Task Idx
  HANDLE hTask     = INVALID_HANDLE_VALUE; // MMCSS Priority Boost Handle
} SK_BufferFlinger;

__declspec (noinline)
void
__stdcall
SK_BeginBufferSwap (void)
{
  static SK_RenderAPI LastKnownAPI = SK_RenderAPI::Reserved;

  assert ( SK_BufferFlinger.dwTid == 0 ||
           SK_BufferFlinger.dwTid == GetCurrentThreadId () );

  if ( SK_BufferFlinger.dwTid     == 0      &&
       config.render.framerate.enable_mmcss &&
       SK_BufferFlinger.dwTaskIdx == 0 )
  {
    SK_BufferFlinger.dwTid = GetCurrentThreadId ();
    SK_BufferFlinger.hTask = 
      AvSetMmMaxThreadCharacteristics ( L"Games",
                                        L"Window Manager",
                                          &SK_BufferFlinger.dwTaskIdx );
  }

  if ( SK_BufferFlinger.hTask != INVALID_HANDLE_VALUE &&
       SK_BufferFlinger.dwTid == GetCurrentThreadId   () )
  {
    AvSetMmThreadPriority ( SK_BufferFlinger.hTask,
                              AVRT_PRIORITY_HIGH );
  }



  ImGuiIO& io =
    ImGui::GetIO ();

  if (io.Fonts == nullptr)
  {
    SK_ImGui_LoadFonts ();
  }



  ULONG frames_drawn =
    SK_GetFramesDrawn ();

  switch (frames_drawn)
  {
    // First frame
    //
    case 0:
    {
      SetCurrentThreadDescription (L"[SK] Primary Render Thread");


      // Maybe make this into an option, but for now just get this the hell out of there
      //   almost no software should be shipping with FP exceptions, it causes compatibility problems.
      _controlfp (MCW_EM, MCW_EM);


      // Load user-defined DLLs (Late)
      SK_RunLHIfBitness ( 64, SK_LoadLateImports64 (),
                              SK_LoadLateImports32 () );

      if (ReadAcquire64 (&SK_SteamAPI_CallbackRunCount) < 1)
      {
        // Steam Init: Better late than never

        SK_Steam_TestImports (GetModuleHandle (nullptr));
      }

      if (config.system.handle_crashes)
        SK::Diagnostics::CrashHandler::Reinstall ();

                extern float target_fps;
                             target_fps = config.render.framerate.target_fps;
      SK::Framerate::GetLimiter ()->init (config.render.framerate.target_fps);
    } break;


    // Grace period frame, just ignore it
    //
    case 1:
      break;


    // 2+ frames drawn
    //
    default:
    {
      //
      // Defer this process to rule out dummy init. windows in some engines
      //
      if (game_window.WndProc_Original == nullptr)
      {
        SKX_Window_EstablishRoot ();
      }

      if (game_window.WndProc_Original != nullptr)
      {
        // If user wants position / style overrides, kick them off on the first
        //   frame after a window procedure has been established.
        //
        //  (nb: Must be implemented asynchronously)
        //
        SK_RunOnce (SK_Window_RepositionIfNeeded ());
        SK_RunOnce (game_window.active = true);
      }
    } break;
  }



  if (config.cegui.enable)
  {
    SetupCEGUI (LastKnownAPI);

    if (config.cegui.frames_drawn > 0)
    {
      if (! SK::SteamAPI::GetOverlayState (true))
      {
        SK_DrawOSD     ();
        SK_DrawConsole ();
      }
    }
  }


  static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");

  if (hModTBFix)
  {
    if (SK_Steam_PiratesAhoy () != 0x00)
    {
      const char* szFirst = "First-frame Done";

      extern float target_fps;
                   target_fps =
        static_cast <float> (
          *reinterpret_cast <const uint8_t *> (szFirst + 5)
        );
    }

    SK::Framerate::GetLimiter ()->wait ();
  }

  if (SK_Steam_PiratesAhoy () && (! SK_ImGui_Active ()))
  {
    SK_ImGui_Toggle ();
  }


  SK_ImGui_PollGamepad_EndFrame ();


  LastKnownAPI =
    SK_GetCurrentRenderBackend ().api;
}


// Todo, move out of here
void
SK_Input_PollKeyboard (void)
{
  const ULONGLONG poll_interval = 1ULL;

  //
  // Do not poll the keyboard while the game window is inactive
  //
  bool skip = true;

  if ( SK_GetGameWindow () == GetForegroundWindow () ||
       SK_GetGameWindow () == GetFocus            () ||
       game_window.active )
    skip = false;

  if (skip)
    return;

  static ULONGLONG last_osd_scale { 0ULL };
  static ULONGLONG last_poll      { 0ULL };
  static ULONGLONG last_drag      { 0ULL };

  SYSTEMTIME    stNow;
  FILETIME      ftNow;
  LARGE_INTEGER ullNow;

  GetSystemTime        (&stNow);
  SystemTimeToFileTime (&stNow, &ftNow);

  ullNow.HighPart = ftNow.dwHighDateTime;
  ullNow.LowPart  = ftNow.dwLowDateTime;

  static bool toggle_drag = false;

  ImGuiIO& io (ImGui::GetIO ());

  if (io.KeysDown [VK_CONTROL] && io.KeysDown [VK_SHIFT] && io.KeysDown [VK_SCROLL])
  {
    if (! toggle_drag)
      config.window.drag_lock = (! config.window.drag_lock);
    toggle_drag = true;

    if (config.window.drag_lock)
      ClipCursor (nullptr);
  } else {
    toggle_drag = false;
  }

  if (config.window.drag_lock)
    SK_CenterWindowAtMouse (config.window.persistent_drag);


  if (ullNow.QuadPart - last_osd_scale > 25ULL * poll_interval)
  {
    if (io.KeysDown [config.osd.keys.expand [0]] &&
        io.KeysDown [config.osd.keys.expand [1]] &&
        io.KeysDown [config.osd.keys.expand [2]])
    {
      last_osd_scale = ullNow.QuadPart;
      SK_ResizeOSD (+0.1f);
    }

    if (io.KeysDown [config.osd.keys.shrink [0]] &&
        io.KeysDown [config.osd.keys.shrink [1]] &&
        io.KeysDown [config.osd.keys.shrink [2]])
    {
      last_osd_scale = ullNow.QuadPart;
      SK_ResizeOSD (-0.1f);
    }
  }

#if 0
  if (ullNow.QuadPart < last_poll + poll_interval)
  {
    SleepEx (10, TRUE);
    last_poll = ullNow.QuadPart;
  }
#endif

  static bool toggle_time = false;
  if (io.KeysDown [config.time.keys.toggle [0]] &&
      io.KeysDown [config.time.keys.toggle [1]] &&
      io.KeysDown [config.time.keys.toggle [2]])
  {
    if (! toggle_time) {
      SK_Steam_UnlockAchievement (0);

      config.time.show = (! config.time.show);
    }
    toggle_time = true;
  } else {
    toggle_time = false;
  }
}



__declspec (noinline) // lol
HRESULT
__stdcall
SK_EndBufferSwap (HRESULT hr, IUnknown* device)
{
  static SK_RenderAPI LastKnownAPI =
         SK_RenderAPI::Reserved;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  assert ( ReadAcquire (&rb.thread) == (LONG)GetCurrentThreadId () ||
           LastKnownAPI             ==       SK_RenderAPI::Reserved );

  InterlockedExchange (&rb.thread, GetCurrentThreadId ());

  if (device != nullptr && LastKnownAPI != rb.api)
  {
    SK_LOG0 ( ( L"SwapChain Presentation Thread has Priority=%i",
                GetThreadPriority (SK_GetCurrentThread ()) ),
                L"RenderBack" );

    SK_RunOnce (SetThreadPriority ( SK_GetCurrentThread (), THREAD_PRIORITY_ABOVE_NORMAL ));

    CComPtr <IDirect3DDevice9>   pDev9   = nullptr;
    CComPtr <IDirect3DDevice9Ex> pDev9Ex = nullptr;
    CComPtr <ID3D11Device>       pDev11  = nullptr;

    if (SUCCEEDED (device->QueryInterface <IDirect3DDevice9Ex> (&pDev9Ex)))
    {
      reinterpret_cast <int &> (rb.api) =
        ( static_cast <int> (SK_RenderAPI::D3D9  ) |
          static_cast <int> (SK_RenderAPI::D3D9Ex)  );

      wcsncpy (rb.name, L"D3D9Ex", 8);
    }

    else if (SUCCEEDED (device->QueryInterface <IDirect3DDevice9> (&pDev9)))
    {
               rb.api  = SK_RenderAPI::D3D9;
      wcsncpy (rb.name, L"D3D9  ", 8);
    }

    else if (SUCCEEDED (device->QueryInterface <ID3D11Device> (&pDev11)))
    {
      // Establish the API used this frame (and handle possible translation layers)
      //
      switch (SK_GetDLLRole ())
      {
        case DLL_ROLE::D3D8:
          rb.api = SK_RenderAPI::D3D8On11;
          break;
        case DLL_ROLE::DDraw:
          rb.api = SK_RenderAPI::DDrawOn11;
          break;
        default:
          rb.api = SK_RenderAPI::D3D11;
          break;
      }

      CComPtr <IUnknown> pTest = nullptr;

      if (SUCCEEDED (       device->QueryInterface (IID_ID3D11Device5, (void **)&pTest))) {
        wcsncpy (rb.name, L"D3D11.4", 8); // Creators Update
      } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device4, (void **)&pTest))) {
        wcsncpy (rb.name, L"D3D11.4", 8);
      } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device3, (void **)&pTest))) {
        wcsncpy (rb.name, L"D3D11.3", 8);
      } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device2, (void **)&pTest))) {
        wcsncpy (rb.name, L"D3D11.2", 8);
      } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device1, (void **)&pTest))) {
        wcsncpy (rb.name, L"D3D11.1", 8);
      } else {
        wcsncpy (rb.name, L"D3D11 ", 8);
      }

      if (     SK_GetDLLRole () == DLL_ROLE::D3D8)  {
        wcscpy (rb.name, L"D3D8");
      }
      else if (SK_GetDLLRole () == DLL_ROLE::DDraw) {
        wcscpy (rb.name, L"DDraw");
      }
    }

    else
    {
               rb.api  = SK_RenderAPI::Reserved;
      wcsncpy (rb.name, L"UNKNOWN", 8);
    }
  }

  else if (LastKnownAPI != rb.api)
  {
    if (config.apis.OpenGL.hook && SK_GL_GetCurrentContext () != nullptr)
    {
               rb.api  = SK_RenderAPI::OpenGL;
      wcsncpy (rb.name, L"OpenGL", 8);
    }
  }


  // Determine Fullscreen Exclusive state in D3D11
  //
  if ( static_cast <int> (rb.api)  &
       static_cast <int> (SK_RenderAPI::D3D11) )
  {
    BOOL fullscreen = FALSE;

    CComPtr                          <IDXGISwapChain>   pSwapChain = nullptr;
    if (rb.swapchain)
        rb.swapchain->QueryInterface <IDXGISwapChain> (&pSwapChain);

    if ( pSwapChain &&
  SUCCEEDED (pSwapChain->GetFullscreenState (&fullscreen, nullptr)) )
    {
      rb.fullscreen_exclusive =               fullscreen;
    }

    else
      rb.fullscreen_exclusive = false;

    // Also clear any resources we were tracking for the shader mod subsystem
    extern void SK_D3D11_EndFrame (void);
                SK_D3D11_EndFrame ();
  }


  LastKnownAPI = config.apis.last_known =
    rb.api;


  SK_RunOnce (SK::DXGI::StartBudgetThread_NoAdapter ());


  SK_Input_PollKeyboard ();

  InterlockedIncrement (&SK_GetCurrentRenderBackend ().frames_drawn);


  if (config.sli.show && device != nullptr)
  {
    // Get SLI status for the frame we just displayed... this will show up
    //   one frame late, but this is the safest approach.
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0)
    {
      sli_state = sk::NVAPI::GetSLIState (device);
    }
  }


  static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");
  static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");

  //
  // TZFix has its own limiter
  //
  if (! (hModTZFix || hModTBFix))
  {
    SK::Framerate::GetLimiter ()->wait ();
  }


  if (config.cegui.enable && ReadAcquire (&CEGUI_Init))
  {
    config.cegui.frames_drawn++;
  }


  if ( SK_BufferFlinger.hTask != INVALID_HANDLE_VALUE &&
       SK_BufferFlinger.dwTid == GetCurrentThreadId () )
  {
    AvRevertMmThreadCharacteristics ( SK_BufferFlinger.hTask );
                                      SK_BufferFlinger.dwTid = 0;
  }



  if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV)
  {
    void SK_FFXV_SetupThreadPriorities (void);
         SK_FFXV_SetupThreadPriorities ();
  }

  SK_StartPerfMonThreads ();

  return hr;
}

DLL_ROLE&
SKX_GetDLLRole (void)
{
  static DLL_ROLE dll_role (DLL_ROLE::INVALID);
  return          dll_role;
}

extern "C"
DLL_ROLE
__stdcall
SK_GetDLLRole (void)
{
  return SKX_GetDLLRole ();
}


void
__cdecl
SK_SetDLLRole (DLL_ROLE role)
{
  SKX_GetDLLRole () = role;
}


// Stupid solution, but very effective way of re-launching a game as admin without
//   the Steam client throwing a temper tantrum.
void
CALLBACK
RunDLL_ElevateMe ( HWND  hwnd,        HINSTANCE hInst,
                   LPSTR lpszCmdLine, int       nCmdShow )
{
  UNREFERENCED_PARAMETER (hInst);

  ShellExecuteA ( hwnd, "runas", lpszCmdLine, nullptr, nullptr, nCmdShow );
}

void
CALLBACK
RunDLL_RestartGame ( HWND  hwnd,        HINSTANCE hInst,
                     LPSTR lpszCmdLine, int       nCmdShow )
{
  UNREFERENCED_PARAMETER (hInst);

  ShellExecuteA ( hwnd, "open", lpszCmdLine, nullptr, nullptr, nCmdShow );
}



SK_ImGui_WidgetRegistry SK_ImGui_Widgets;