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
#include <SpecialK/resource.h>

#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>

#include <SpecialK/nvapi.h>
#include <SpecialK/adl.h>


#include <SpecialK/commands/mem.inl>
#include <SpecialK/commands/update.inl>


#ifdef _WIN64
#pragma comment (lib, R"(depends\lib\DirectXTex\x64\DirectXTex.lib)")
#pragma comment (lib, R"(depends\lib\MinHook\x64\libMinHook64.lib)")
#pragma comment (lib, R"(depends\lib\lzma\x64\libzma.lib)")
#else
#pragma comment (lib, R"(depends\lib\DirectXTex\Win32\DirectXTex.lib)")
#pragma comment (lib, R"(depends\lib\MinHook\Win32\libMinHook.lib)")
#pragma comment (lib, R"(depends\lib\lzma\Win32\libzma.lib)")
#endif

volatile HANDLE   hInitThread     = { INVALID_HANDLE_VALUE };
volatile DWORD    dwInitThreadId  = 0;

         BOOL     nvapi_init      = FALSE;
         HMODULE  backend_dll     = nullptr;

volatile LONG     __SK_Init       = FALSE;
         bool     __SK_bypass     = false;
   const wchar_t* __SK_BootedCore = L"";

 extern float     __target_fps;


using  ChangeDisplaySettingsA_pfn = LONG (WINAPI *)(
                                     _In_opt_ DEVMODEA *lpDevMode,
                                     _In_     DWORD     dwFlags );
extern ChangeDisplaySettingsA_pfn
       ChangeDisplaySettingsA_Original;



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


typedef enum _THREADINFOCLASS_SK {
  ThreadBasicInformation,
  ThreadIsIoPending = 16
} THREADINFOCLASS_SK;

NTSTATUS WINAPI NtQueryInformationThread(
  _In_      HANDLE             ThreadHandle,
  _In_      THREADINFOCLASS_SK ThreadInformationClass,
  _Inout_   PVOID              ThreadInformation,
  _In_      ULONG              ThreadInformationLength,
  _Out_opt_ PULONG             ReturnLength
);

extern void SK_D3D11_BeginFrame (void);
extern void SK_D3D11_EndFrame   (SK_TLS* pTLS = SK_TLS_Bottom ());
extern void SK_D3D12_EndFrame   (SK_TLS* pTLS = SK_TLS_Bottom ());

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
  wcsncpy_s ( SKX_GetBackend (), 128,
                wszBackend,      _TRUNCATE );

  return
    SKX_GetBackend ();
}

wchar_t*
SKX_GetRootPath (void)
{
  static wchar_t SK_RootPath [MAX_PATH * 2 + 1] = { };
  return         SK_RootPath;
}

const wchar_t*
__stdcall
SK_GetRootPath (void)
{
  return SKX_GetRootPath ();
}



wchar_t*
SKX_GetNaiveConfigPath (void)
{
  static wchar_t SK_ConfigPath [MAX_PATH * 2 + 1] = { };
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
  wcsncpy_s ( SKX_GetNaiveConfigPath (), MAX_PATH,
                path,                    _TRUNCATE );
}

void
__stdcall
SK_StartPerfMonThreads (void)
{
  // Handle edge-case that might re-spawn WMI threads during de-init.
  if (ReadAcquire (&__SK_DLL_Ending))
    return;

  auto SpawnMonitorThread =
  []( volatile HANDLE                 *phThread,
      const    wchar_t                *wszName,
               LPTHREAD_START_ROUTINE  pThunk ) ->
  bool
  {
    if ( INVALID_HANDLE_VALUE ==
           InterlockedCompareExchangePointer (phThread, nullptr, INVALID_HANDLE_VALUE)
       )
    {
      dll_log->LogEx (true, L"[ Perfmon. ] Spawning %ws...  ", wszName);

      InterlockedExchangePointer ( (void **)phThread,
        SK_Thread_CreateEx ( pThunk )
      );

      // Most WMI stuff will be replaced with NtDll in the future
      //   -- for now, CPU monitoring is the only thing that has abandoned
      //        WMI
      //
      if (pThunk != SK_MonitorCPU)
      {
        SK_RunOnce (SK_WMI_Init ());
      }

      if (ReadPointerAcquire (phThread) != INVALID_HANDLE_VALUE)
      {
        dll_log->LogEx (false, L"tid=0x%04x\n",
                          GetThreadId (ReadPointerAcquire (phThread)));
        return true;
      }

      dll_log->LogEx (false, L"Failed!\n");
    }

    return false;
  };

  //
  // Spawn CPU Refresh Thread
  //
  if (config.cpu.show || ( SK_ImGui_Widgets->cpu_monitor != nullptr &&
                           SK_ImGui_Widgets->cpu_monitor->isActive () ))
  {
    SpawnMonitorThread ( &SK_WMI_CPUStats->hThread,
                         L"CPU Monitor",      SK_MonitorCPU      );
  }

  if (config.disk.show)
  {
    SpawnMonitorThread ( &SK_WMI_DiskStats->hThread,
                         L"Disk Monitor",     SK_MonitorDisk     );
  }

  if (config.pagefile.show)
  {
    SpawnMonitorThread ( &SK_WMI_PagefileStats->hThread,
                         L"Pagefile Monitor", SK_MonitorPagefile );
  }
}


void
SK_LoadGPUVendorAPIs (void)
{
  dll_log->LogEx (false, L"================================================"
                         L"===========================================\n" );

  dll_log->Log (L"[  NvAPI   ] Initializing NVIDIA API          (NvAPI)...");

  nvapi_init =
    sk::NVAPI::InitializeLibrary (SK_GetHostApp ());

  dll_log->Log (L"[  NvAPI   ]              NvAPI Init         { %s }",
                                                     nvapi_init ? L"Success" :
                                                                  L"Failed ");

  if (nvapi_init)
  {
    const int num_sli_gpus =
      sk::NVAPI::CountSLIGPUs ();

    dll_log->Log ( L"[  NvAPI   ] >> NVIDIA Driver Version: %s",
                    sk::NVAPI::GetDriverVersion ().c_str () );

    const int gpu_count =
      sk::NVAPI::CountPhysicalGPUs ();

    dll_log->Log ( gpu_count > 1 ? L"[  NvAPI   ]  * Number of Installed NVIDIA GPUs: %i  "
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
        dll_log->Log ( L"[  NvAPI   ]   + SLI GPU %d: %s",
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

           uint32_t SK_Steam_GetAppID_NoAPI (void);
    extern BOOL     SK_NvAPI_EnableAnsel    (DLL_ROLE role);
    extern BOOL     SK_NvAPI_DisableAnsel   (DLL_ROLE role);

    extern
      void __stdcall
        SK_NvAPI_SetAppFriendlyName (const wchar_t* wszFriendlyName);
    extern
      void __stdcall
        SK_NvAPI_SetAppName         (const wchar_t* wszAppName);

    SK_NvAPI_SetAppName         (       SK_GetFullyQualifiedApp () );
    SK_NvAPI_SetAppFriendlyName (
      app_cache_mgr->getAppNameFromID ( SK_Steam_GetAppID_NoAPI () ).c_str ()
                                );

    if (! config.nvidia.bugs.snuffed_ansel)
    {
      if (SK_NvAPI_DisableAnsel (SK_GetDLLRole ()))
      {
        restart = true;

        SK_MessageBox (
          L"To Avoid Potential Compatibility Issues, Special K has Disabled Ansel for this Game.\r\n\r\n"
          L"You may re-enable Ansel for this Game using the Help Menu in Special K's Control Panel.",
            L"Special K Compatibility Layer:  [ Ansel Disabled ]",
              MB_ICONWARNING | MB_OK
        );
      }



      config.nvidia.bugs.snuffed_ansel = true;

      SK_GetDLLConfig ()->write (
        SK_GetDLLConfig ()->get_filename ()
      );
    }

    if (restart)
    {
      dll_log->Log (L"[  Nv API  ] >> Restarting to apply NVIDIA driver settings <<");

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
    dll_log->Log (L"[DisplayLib] Initializing AMD Display Library (ADL)...");

    BOOL adl_init =
      SK_InitADL ();

    dll_log->Log   (L"[DisplayLib]              ADL   Init         { %s }",
                                                      adl_init ? L"Success" :
                                                                 L"Failed ");

    // Yes, AMD driver is in working order ...
    if (adl_init > 0)
    {
      dll_log->Log ( L"[DisplayLib]  * Number of Reported AMD Adapters: %i (%i active)",
                       SK_ADL_CountPhysicalGPUs (),
                         SK_ADL_CountActiveGPUs () );
    }
  }

  const HMODULE hMod =
    SK_GetModuleHandle (SK_GetHostApp ());

  if (hMod != nullptr)
  {
    const auto* dwOptimus =
      reinterpret_cast <DWORD *> (
        SK_GetProcAddress ( SK_GetHostApp (),
                              "NvOptimusEnablement" )
      );

    if (dwOptimus != nullptr)
    {
      dll_log->Log ( L"[Hybrid GPU]  NvOptimusEnablement..................: 0x%02X (%s)",
                       *dwOptimus,
                     ((*dwOptimus) & 0x1) ? L"Max Perf." :
                                            L"Don't Care" );
    }

    else
    {
      dll_log->Log (L"[Hybrid GPU]  NvOptimusEnablement..................: UNDEFINED");
    }

    const auto* dwPowerXpress =
      reinterpret_cast <DWORD *> (
        SK_GetProcAddress ( SK_GetHostApp (),
                              "AmdPowerXpressRequestHighPerformance" )
      );

    if (dwPowerXpress != nullptr)
    {
      dll_log->Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: 0x%02X (%s)",
         *dwPowerXpress,
         (*dwPowerXpress & 0x1) ? L"High Perf." :
                                  L"Don't Care" );
    }

    else
      dll_log->Log (L"[Hybrid GPU]  AmdPowerXpressRequestHighPerformance.: UNDEFINED");

    dll_log->LogEx (false, L"================================================"
                           L"===========================================\n" );
  }
}

void
SK_UnpackD3DShaderCompiler (void);

extern ImGuiContext* SK_GImDefaultContext (void);

void
__stdcall
SK_InitCore (std::wstring, void* callback)
{
  using finish_pfn   = void (WINAPI *)  (void);
  using callback_pfn = void (WINAPI *)(_Releases_exclusive_lock_ (init_mutex) finish_pfn);

  const auto callback_fn =
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
      SK_NNK2_InitPlugin ();
      break;

    case SK_GAME_ID::Tales_of_Vesperia:
      SK_TVFix_InitPlugin ();
      break;

    case SK_GAME_ID::Sekiro:
      extern void SK_Sekiro_InitPlugin (void);
                  SK_Sekiro_InitPlugin (    );
      break;

    case SK_GAME_ID::FarCry5:
    {
      auto _UnpackEasyAntiCheatBypass = [&](void) ->
      void
      {
        HMODULE hModSelf =
          SK_GetDLL ();

        HRSRC res =
          FindResource ( hModSelf, MAKEINTRESOURCE (IDR_FC5_KILL_ANTI_CHEAT), L"7ZIP" );

        if (res)
        {
          DWORD   res_size     =
            SizeofResource ( hModSelf, res );

          HGLOBAL packed_anticheat =
            LoadResource   ( hModSelf, res );

          if (! packed_anticheat) return;

          const void* const locked =
            (void *)LockResource (packed_anticheat);

          if (locked != nullptr)
          {
            wchar_t      wszBackup      [MAX_PATH * 2 + 1] = { };
            wchar_t      wszArchive     [MAX_PATH * 2 + 1] = { };
            wchar_t      wszDestination [MAX_PATH * 2 + 1] = { };

            wcscpy (wszDestination, SK_GetHostPath ());

            if (GetFileAttributesW (wszDestination) == INVALID_FILE_ATTRIBUTES)
              SK_CreateDirectories (wszDestination);

            PathAppendW (wszDestination, L"EasyAntiCheat");
            wcscpy      (wszBackup,      wszDestination);
            PathAppendW (wszBackup,      L"EasyAntiCheat_x64_orig.dll");

            if (GetFileAttributesW (wszBackup) == INVALID_FILE_ATTRIBUTES)
            {
              wchar_t      wszBackupSrc [MAX_PATH * 2 + 1] = { };
              wcscpy      (wszBackupSrc, wszDestination);
              PathAppendW (wszBackupSrc, L"EasyAntiCheat_x64.dll");
              CopyFileW   (wszBackupSrc, wszBackup, TRUE);

              SK_LOG0 ( ( L"Unpacking EasyAntiCheatDefeat for FarCry 5" ),
                          L"AntiDefeat" );

              wcscpy      (wszArchive, wszDestination);
              PathAppendW (wszArchive, L"EasyAntiCheatDefeat.7z");

              FILE* fPackedCompiler =
                _wfopen   (wszArchive, L"wb");

              if (fPackedCompiler != nullptr)
              {
                fwrite    (locked, 1, res_size, fPackedCompiler);
                fclose    (fPackedCompiler);
              }

              SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
              DeleteFileW       (wszArchive);
            }
          }

          UnlockResource (packed_anticheat);
        }
      };

      _UnpackEasyAntiCheatBypass ();
    } break;
#else
    case SK_GAME_ID::SecretOfMana:
      SK_SOM_InitPlugin ();
      break;

    case SK_GAME_ID::Ys_Eight:
      SK_YS8_InitPlugin ();
      break;

    case SK_GAME_ID::DragonBallFighterZ:
      wchar_t      wszPath       [MAX_PATH * 2] = { };
      wchar_t      wszWorkingDir [MAX_PATH * 2] = { };
      wcscpy      (wszWorkingDir, SK_GetHostPath  ());
      PathAppendW (wszWorkingDir, L"RED\\Binaries\\Win64\\");

      wcscpy      (wszPath,       wszWorkingDir);
      PathAppendW (wszPath,       L"RED-Win64-Shipping.exe");

      ShellExecuteW (nullptr, L"open", wszPath, L"-eac-nop-loaded", wszWorkingDir, SW_SHOWNORMAL);
      ExitProcess   (0);
      break;
#endif
  }

  void
     __stdcall SK_InitFinishCallback (void);
  callback_fn (SK_InitFinishCallback);
}

using D3DStripShader_pfn =
HRESULT (WINAPI *)
(   LPCVOID   pShaderBytecode,
    SIZE_T    BytecodeLength,
    UINT      uStripFlags,
  ID3DBlob  **ppStrippedBlob );

D3DStripShader_pfn
D3DStripShader_40_Original;
D3DStripShader_pfn
D3DStripShader_41_Original;
D3DStripShader_pfn
D3DStripShader_42_Original;
D3DStripShader_pfn
D3DStripShader_43_Original;
D3DStripShader_pfn
D3DStripShader_47_Original;

#define __SK_SUBSYSTEM__ L"Shadr Comp"

HRESULT
WINAPI
D3DStripShader_40_Detour      (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob   )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}

HRESULT
WINAPI
D3DStripShader_41_Detour      (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob   )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}

HRESULT
WINAPI
D3DStripShader_42_Detour      (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob   )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}

HRESULT
WINAPI
D3DStripShader_43_Detour      (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob   )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}

HRESULT
WINAPI
D3DStripShader_47_Detour     (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob  )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}

void
WaitForInit (void)
{
  const auto _SpinMax = 32;

  LONG init =
    ReadAcquire (&__SK_Init);

  if (init > 0 || init < 0)
    return;

  const DWORD dwThreadId =
    SK_Thread_GetCurrentId ();

  while (ReadPointerAcquire (&hInitThread) != INVALID_HANDLE_VALUE)
  {
    const DWORD dwInitTid =
      ReadULongAcquire (&dwInitThreadId);

    if ( dwInitTid                == dwThreadId ||
         dwInitTid                == 0          ||
         ReadAcquire (&__SK_Init) == TRUE )
    {
      break;
    }

    for (int i = 0; i < _SpinMax && (ReadPointerAcquire (&hInitThread) != INVALID_HANDLE_VALUE); i++)
      ;

    if ( ReadPointerAcquire (&hInitThread) == INVALID_HANDLE_VALUE )
      break;

    const DWORD dwWaitStatus =
      MsgWaitForMultipleObjectsEx (1, const_cast <HANDLE *>(&hInitThread), 16UL, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

    if ( dwWaitStatus == WAIT_OBJECT_0 ||
         dwWaitStatus == WAIT_ABANDONED ) break;
  }


  if (! InterlockedCompareExchangeAcquire (&__SK_Init, TRUE, FALSE))
  {
    if ( ReadULongAcquire   (&dwInitThreadId) != dwThreadId &&
         ReadPointerAcquire (&hInitThread)    != INVALID_HANDLE_VALUE )
    {
      bool local_install = false;
      if (! SK_COMPAT_IsSystemDllInstalled (L"D3DCompiler_43.dll", &local_install))
      {
        SK_ImGui_Warning (L"Your system is missing the June 2010 DirectX Runtime.\t\t\n"
                          L"\n"
                          L"\t\t\t\t* Please install it as soon as possible.");
      }

      local_install = false;
      if (! SK_COMPAT_IsSystemDllInstalled (L"D3DCompiler_47.dll", &local_install))
      {
        if (! local_install)
        {
          SK_UnpackD3DShaderCompiler ();
        }
      }

      struct SK_D3D_AntiStrip {
        const wchar_t*    wszDll;
        const    char*    szSymbol;
               LPVOID    pfnHookFunc;
                  void** ppfnTrampoline;
      } static strippers [] =
      { { L"D3DCOMPILER_47.dll",
           "D3DStripShader",   D3DStripShader_47_Detour,
      static_cast_p2p <void> (&D3DStripShader_47_Original) },
        { L"D3DCOMPILER_43.dll",
           "D3DStripShader",   D3DStripShader_43_Detour,
      static_cast_p2p <void> (&D3DStripShader_43_Original) },
        { L"D3DCOMPILER_42.dll",
           "D3DStripShader",   D3DStripShader_42_Detour,
      static_cast_p2p <void> (&D3DStripShader_42_Original) },
        { L"D3DCOMPILER_41.dll",
           "D3DStripShader",   D3DStripShader_41_Detour,
      static_cast_p2p <void> (&D3DStripShader_41_Original) },
        { L"D3DCOMPILER_40.dll",
           "D3DStripShader",   D3DStripShader_40_Detour,
      static_cast_p2p <void> (&D3DStripShader_40_Original) } };

      if (SUCCEEDED (__HrLoadAllImportsForDll ("D3DCOMPILER_47.dll")))
      {
        for ( auto& stripper : strippers )
        {
          if (SK_GetModuleHandleW (stripper.wszDll) != nullptr)
          {
            SK_CreateDLLHook2 ( stripper.wszDll,
                                stripper.szSymbol,
                                stripper.pfnHookFunc,
                                stripper.ppfnTrampoline );
          }
        }
      }

      WriteULongRelease   (                       &dwInitThreadId,
                              0                    );
      WritePointerRelease ( const_cast <void **> (&hInitThread),
                              INVALID_HANDLE_VALUE );
    }

    // Load user-defined DLLs (Lazy)
    SK_RunLHIfBitness ( 64, SK_LoadLazyImports64 (),
                            SK_LoadLazyImports32 () );

    if (config.system.handle_crashes)
      SK::Diagnostics::CrashHandler::Reinstall ();
  }
}


void
SK_UnpackD3DShaderCompiler (void)
{
  HMODULE hModSelf =
    SK_GetDLL ();

  HRSRC res =
    FindResource (
      hModSelf,
        MAKEINTRESOURCE (IDR_D3DCOMPILER_PACKAGE),
          L"7ZIP"
    );

  if (res)
  {
    SK_LOG0 ( ( L"Unpacking D3DCompiler_47.dll because user does not have "
                L"June 2010 DirectX Redistributables installed." ),
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
      PathAppendW (wszArchive, L"D3DCompiler_47.7z");

      ///SK_LOG0 ( ( L" >> Archive: %s [Destination: %s]", wszArchive,wszDestination ),
      ///            L"D3DCompile" );

      FILE* fPackedCompiler =
        _wfopen   (wszArchive, L"wb");

      if (fPackedCompiler != nullptr)
      {
        fwrite (locked, 1, res_size, fPackedCompiler);
        fclose (fPackedCompiler);
      }

      if (GetFileAttributes (wszArchive) != INVALID_FILE_ATTRIBUTES)
      {
        SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
        DeleteFileW       (wszArchive);
      }
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

  if (rundll_invoked || SK_IsSuperSpecialK () || (! wcslen (__SK_BootedCore)))
  {
    init_mutex->unlock ();
    return;
  }


  SK_DeleteTemporaryFiles ();
  SK_DeleteTemporaryFiles (L"Version", L"*.old");

  SK::Framerate::Init ();

  gsl::not_null <SK_ICommandProcessor *> cp (
    SK_GetCommandProcessor ()
  );

  extern int32_t SK_D3D11_amount_to_purge;
  cp->AddVariable (
    "VRAM.Purge",
      new SK_IVarStub <int32_t> (
        (int32_t *)&SK_D3D11_amount_to_purge
      )
  );

  cp->AddVariable (
    "GPU.StatPollFreq",
      new SK_IVarStub <float> (
        &config.gpu.interval
      )
  );

  cp->AddVariable (
    "ImGui.FontScale",
      new SK_IVarStub <float> (
        &config.imgui.scale
      )
  );

  SK_InitRenderBackends ();

  cp->AddCommand ("mem",       new skMemCmd    ());
  cp->AddCommand ("GetUpdate", new skUpdateCmd ());


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
    game_debug->close ();
    game_debug->silent = true;
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

  static const GUID  nil_guid = {     };
               GUID* pGUID    = nullptr;

  // Make note of the system's original power scheme
  if ( ERROR_SUCCESS ==
         PowerGetActiveScheme ( nullptr,
                                  &pGUID )
     )
  {
    config.cpu.power_scheme_guid_orig = *pGUID;

    SK_LocalFree ((HLOCAL)pGUID);
  }

  // Apply a powerscheme override if one is set
  if (! IsEqualGUID (config.cpu.power_scheme_guid, nil_guid))
  {
    PowerSetActiveScheme (nullptr, &config.cpu.power_scheme_guid);
  }

  SK_LoadGPUVendorAPIs ();

  dll_log->LogEx (false, L"------------------------------------------------"
                         L"-------------------------------------------\n" );
  dll_log->Log   (       L"[ SpecialK ] === Initialization Finished! ===   "
                         L"       (%6.2f ms)",
                           SK_DeltaPerfMS ( SK_GetInitParams ().start_time.QuadPart, 1 )
                 );
  dll_log->LogEx (false, L"------------------------------------------------"
                         L"-------------------------------------------\n" );

  init_mutex->unlock ();
}


DWORD
WINAPI
CheckVersionThread (LPVOID)
{
  SetCurrentThreadDescription (                   L"[SK] Auto-Update Worker"   );
  SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_LOWEST );

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
  WriteULongNoFence (&dwInitThreadId, SK_Thread_GetCurrentId ());

  SetCurrentThreadDescription (                 L"[SK] Primary Initialization Thread" );
  SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_HIGHEST       );
  SetThreadPriorityBoost      ( SK_GetCurrentThread (), TRUE                          );

  auto* params =
    static_cast <init_params_s *> (user);

  SK_InitCore ( params->backend, params->callback );

  WriteULongRelease (&dwInitThreadId, 0);

  return 0;
}


enum SK_File_SearchStopCondition {
  FirstMatchFound,
  AllMatchesFound,
  LastMatchFound
};

std::unordered_set <std::wstring>
SK_RecursiveFileSearchEx ( const wchar_t* wszDir,
                           const wchar_t* wszSearchExt,
 std::unordered_set <std::wstring_view>& cwsFileNames,
       std::vector        <
         std::pair          < std::wstring, bool >
                          >&&             preferred_dirs = { },
              SK_File_SearchStopCondition stop_condition = FirstMatchFound )
{
  std::unordered_set <
    std::wstring
  > found_set;

  for ( auto& preferred : preferred_dirs )
  {
    if ( preferred.second == false )
    {
      preferred.second = true;

      const DWORD dwAttribs  =
        GetFileAttributesW (preferred.first.c_str ());

      if (  dwAttribs != INVALID_FILE_ATTRIBUTES &&
          !(dwAttribs &  FILE_ATTRIBUTE_DIRECTORY) )
      {
        found_set.emplace (preferred.first);

        if (stop_condition == FirstMatchFound)
        {
          return found_set;
        }
      }

      PathRemoveFileSpec (
        preferred.first.data ()
      );

      std::unordered_set <std::wstring>&& recursive_finds =
        SK_RecursiveFileSearchEx ( preferred.first.c_str (), wszSearchExt,
                                                            cwsFileNames,
                                                       { }, stop_condition );

      if (! recursive_finds.empty () )
      {
        if (stop_condition == FirstMatchFound)
        {
          return recursive_finds;
        }
      }
    }
  }

  wchar_t   wszPath [MAX_PATH * 2 + 1] = { };
  swprintf (wszPath, LR"(%s\*)", wszDir);

  WIN32_FIND_DATA fd          = {   };
  HANDLE          hFind       =
    FindFirstFileW ( wszPath, &fd);

  if (hFind == INVALID_HANDLE_VALUE) { return found_set; }

  std::vector <std::wstring> dirs_to_traverse;

  do
  {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if ( *fd.cFileName == L'.' )
      {
        const wchar_t* wszNext =
          CharNextW (fd.cFileName);

        if ( (*wszNext == L'.'  ) ||
             (*wszNext == L'\0' )    )
        {
        //dll_log.Log (L"%ws\\%ws is a special directory", wszDir, fd.cFileName);
          continue;
        }
      }

      dirs_to_traverse.emplace_back (fd.cFileName);
    }

    wchar_t wszFoundFile      [MAX_PATH] = { };
    wchar_t wszFoundFileLower [MAX_PATH] = { };
    wchar_t wszFoundExtension [   16   ] = { };

    if ( _wsplitpath_s (
           fd.cFileName, nullptr, 0,
                         nullptr, 0,
                         wszFoundFile, MAX_PATH,
                         wszFoundExtension, 16
                       ) != 0
       )
    {
    //dll_log.Log (L"%ws did not split", fd.cFileName);
      continue;
    }

    if (_wcsicmp (wszFoundExtension, wszSearchExt) != 0)
    {
    //dll_log.Log (L"%ws is wrong extension    { Search Extension: %ws, First File To Check: %ws }",
    //             wszFoundExtension, wszSearchExt, cwsFileNames.begin ()->c_str () );
      continue;
    }

    wcscpy (
      wszFoundFileLower,
        wszFoundFile
    );

    wchar_t* pwszFileLower = wszFoundFileLower;
    while ( *pwszFileLower != L'\0' )
    {       *pwszFileLower = towlower  (*pwszFileLower);
             pwszFileLower = CharNextW ( pwszFileLower);
    }

    if (! cwsFileNames.count (wszFoundFileLower))
    {
      //dll_log.Log (L"%ws is not contained  { Search Extension: %ws, First File To Check: %ws }",
      //             wszFoundFileLower, wszSearchExt, cwsFileNames.begin ()->c_str () );

      continue;
    }

    else
    {
      std::wstring found (wszDir);
                   found += L"\\";
                   found += wszFoundFile;
                   found += wszFoundExtension;

      found_set.emplace (found);

      //dll_log.Log (L"Add to Found Set: %s", found.c_str ());

      if (stop_condition == FirstMatchFound)
      {
        FindClose (hFind);
        return found_set;
      }
    }
  } while (FindNextFile (hFind, &fd));

  FindClose (hFind);


  for ( auto& dir : dirs_to_traverse )
  {
    wchar_t   wszDescend [MAX_PATH * 2 + 1] = { };
    swprintf (wszDescend, LR"(%s\%s)", wszDir, dir.c_str () );

    const std::unordered_set <std::wstring>&& recursive_finds =
      SK_RecursiveFileSearchEx ( wszDescend,  wszSearchExt,
                                             cwsFileNames,
                                         { }, stop_condition );

    if (! recursive_finds.empty ())
    {
      for ( auto& found_entry : recursive_finds )
      {
        found_set.emplace (found_entry);

        if (stop_condition == FirstMatchFound)
          return found_set;
      }
    }
  }


  return found_set;
}

std::wstring
SK_RecursiveFileSearch ( const wchar_t* wszDir,
                         const wchar_t* wszFile )
{
  dll_log->Log ( L"Recursive File Search for '%ws', beginning in '%ws'",
                   SK_ConcealUserDir (std::wstring (wszFile).data ()),
                   SK_ConcealUserDir (std::wstring (wszDir).data  ()) );

  std::wstring extension (
    PathFindExtensionW     (wszFile)
                         ),
               filename    (wszFile);

  PathRemoveExtensionW (filename.data ());
  PathFindFileNameW    (filename.data ());

  std::unordered_set <std::wstring_view> file_pattern = {
    filename.data ()
  };

  const std::unordered_set <std::wstring> matches =
    SK_RecursiveFileSearchEx (
      wszDir, extension.c_str (),
        file_pattern, {{wszFile, false}}, FirstMatchFound
    );

  if (! matches.empty ())
  {
    dll_log->Log ( L"Success!  [%ws]",
                   SK_ConcealUserDir (std::wstring (*matches.begin ()).data ()) );
  }

  else
  {
    dll_log->Log ( L"No Such File Exists",
                   SK_ConcealUserDir (std::wstring (*matches.begin ()).data ()) );
  }

  return matches.empty () ?
    L"" : *matches.begin ();
}


bool
__stdcall
SK_HasGlobalInjector (void)
{
  static int last_test = 0;

  if (last_test == 0)
  {
    wchar_t wszBasePath [MAX_PATH * 2 + 1] = { };

    wcsncpy_s ( wszBasePath, MAX_PATH * 2,
                  std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\)" ).c_str (),
                    _TRUNCATE );

    lstrcatW (wszBasePath, SK_RunLHIfBitness ( 64, L"SpecialK64.dll",
                                                   L"SpecialK32.dll" ));

    bool result = (GetFileAttributesW (wszBasePath) != INVALID_FILE_ATTRIBUTES);
    last_test   = result ? 1 : -1;
  }

  return (last_test != -1);
}

extern std::pair <std::queue <DWORD>, BOOL> __stdcall SK_BypassInject (void);

const wchar_t*
__stdcall
SK_GetDebugSymbolPath (void)
{
  static volatile LONG    __init                            = 0;
  static          wchar_t wszDbgSymbols [MAX_PATH * 3 + 1] = { };

  if (ReadAcquire (&__init) == 2)
    return wszDbgSymbols;

  if (! InterlockedCompareExchange (&__init, 1, 0))
  {
    if (! crash_log->initialized)
    {
      crash_log->flush_freq = 0;
      crash_log->lockless   = true;
      crash_log->init       (L"logs/crash.log", L"wt+,ccs=UTF-8");
    }


    std::wstring symbol_file =
      SK_GetModuleFullName (SK_Modules->Self ());

    wchar_t wszSelfName    [MAX_PATH * 3 + 1] = { };
    wchar_t wszGenericName [MAX_PATH * 3 + 1] = { };
    wcsncpy_s ( wszSelfName,           MAX_PATH * 3,
                 symbol_file.c_str (), _TRUNCATE );


    PathRemoveExtensionW (wszSelfName);
    lstrcatW             (wszSelfName, L".pdb");

    wcsncpy_s ( wszGenericName, MAX_PATH * 3,
                 wszSelfName,   _TRUNCATE );

    PathRemoveFileSpecW ( wszGenericName );
    PathAppendW         ( wszGenericName,
      SK_RunLHIfBitness ( 64, L"SpecialK64.pdb",
                              L"SpecialK32.pdb" ) );

    bool generic_symbols =
      ( INVALID_FILE_ATTRIBUTES !=
          GetFileAttributesW ( wszGenericName )
      );

    if (generic_symbols)
    {
      symbol_file = wszGenericName;
    }

    else
    {
      symbol_file = wszSelfName;
    }

    // Not the correct way of validating the existence of this DLL's symbol file,
    //   but it is good enough for now ...
    if ( (! generic_symbols) &&
         INVALID_FILE_ATTRIBUTES == GetFileAttributesW (wszSelfName) )
    {
      static wchar_t wszCurrentPath [MAX_PATH * 3 + 1] = { };

      if (*wszCurrentPath == L'\0')
      {
        SymGetSearchPathW (GetCurrentProcess (), wszCurrentPath, MAX_PATH * 3);
      }

      std::wstring dir (wszCurrentPath); dir += L";";
                   dir.append (SK_GetDocumentsDir ());
                   dir.append (LR"(\My Mods\SpecialK\)");

      wcsncpy_s ( wszDbgSymbols,  MAX_PATH * 3,
                    dir.c_str (), _TRUNCATE );

      symbol_file = SK_GetDocumentsDir ();
      symbol_file.append (LR"(\My Mods\SpecialK\SpecialK)");
      symbol_file.append (
        SK_RunLHIfBitness ( 64, L"64.pdb",
                                L"32.pdb"
                          )
                         );
    }

    else
    {
      static wchar_t                           wszCurrentPath [MAX_PATH * 3 + 1] = { };
      SymGetSearchPathW (GetCurrentProcess (), wszCurrentPath, MAX_PATH * 3);

      wcsncpy_s ( wszDbgSymbols,  MAX_PATH * 3,
                  wszCurrentPath, _TRUNCATE );
    }


    // Strip the username from the logged path
    static wchar_t wszDbgSymbolsEx  [MAX_PATH * 3 + 1] = { };
       wcsncpy_s ( wszDbgSymbolsEx,  MAX_PATH * 3,
                      wszDbgSymbols, _TRUNCATE );

    SK_ConcealUserDir (wszDbgSymbolsEx);
    crash_log->Log (L"DebugHelper Symbol Search Path......: %ws", wszDbgSymbolsEx);


    if (GetFileAttributesW (symbol_file.c_str ()) != INVALID_FILE_ATTRIBUTES)
    {
      std::wstring stripped (symbol_file);

      stripped =
        SK_ConcealUserDir ((wchar_t *)stripped.c_str ());

      crash_log->Log ( L"Special K Debug Symbols Loaded From.: %ws",
                         stripped.c_str () );
    }

    else
    {
      std::wstring stripped (symbol_file);

      stripped =
        SK_ConcealUserDir ((wchar_t *)stripped.c_str ());

      crash_log->Log ( L"Unable to load Special K Debug Symbols ('%ws'), "
                       L"crash log will not be accurate.",
                                 stripped.c_str () );
    }

    wcsncpy_s ( wszDbgSymbols,        MAX_PATH * 3,
                symbol_file.c_str (), _TRUNCATE );

    PathRemoveFileSpec (wszDbgSymbols);

    InterlockedIncrementRelease (&__init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&__init, 2);

  return
    wszDbgSymbols;
}

void
__stdcall
SK_EstablishRootPath (void)
{
  wchar_t wszConfigPath [MAX_PATH * 2 + 1] = { };
  GetCurrentDirectory   (MAX_PATH, wszConfigPath);
  lstrcatW (wszConfigPath, LR"(\)");

  // File permissions don't permit us to store logs in the game's directory,
  //   so implicitly turn on the option to relocate this stuff.
  if (! SK_File_CanUserWriteToPath (wszConfigPath)) {
    config.system.central_repository = true;
  }

  SecureZeroMemory (wszConfigPath, sizeof (wchar_t) * (MAX_PATH * 2 + 1));

  // Store config profiles in a centralized location rather than relative to the game's executable
  //
  //   Currently, this location is always Documents\My Mods\SpecialK\
  //
  if (config.system.central_repository)
  {
    if (! SK_IsSuperSpecialK ())
    {
      _swprintf ( SKX_GetRootPath (), LR"(%s\My Mods\SpecialK)",
                  SK_GetDocumentsDir ().c_str () );
    }

    else
    {
      GetCurrentDirectory (MAX_PATH, SKX_GetRootPath ());
    }

    wcsncpy_s ( wszConfigPath,      MAX_PATH * 2,
                SKX_GetRootPath (), _TRUNCATE  );
    lstrcatW  ( wszConfigPath, LR"(\Profiles\)");
    lstrcatW  ( wszConfigPath, SK_GetHostApp  ());
  }


  // Relative to game's executable path
  //
  else
  {
    if (! SK_IsSuperSpecialK ())
    {
      wcsncpy_s ( SKX_GetRootPath (), MAX_PATH,
                  SK_GetHostPath  (), _TRUNCATE );
    }

    else
    {
      GetCurrentDirectory (MAX_PATH, SKX_GetRootPath ());
    }

    wcsncpy_s ( wszConfigPath,      MAX_PATH,
                SKX_GetRootPath (), _TRUNCATE );
  }


  // Not using the ShellW API because at this (init) stage,
  //   we can only reliably use Kernel32 functions.
  lstrcatW (SKX_GetRootPath (), LR"(\)");
  lstrcatW (wszConfigPath,      LR"(\)");

  SK_SetConfigPath (wszConfigPath);
}

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
      AppID            =        strtol (szSteamGameId, nullptr, 0);
    }
  }

  if (AppID == 0)
  {
    dwSteamGameIdLen =
      GetEnvironmentVariableA ( "SteamGameId",
                                  szSteamGameId,
                                    0x20 );


    if (dwSteamGameIdLen > 1)
      AppID = strtol (szSteamGameId, nullptr, 0);
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

    app_cache_mgr->loadAppCacheForExe (SK_GetFullyQualifiedApp ());
    app_cache_mgr->addAppToCache
    (
      SK_GetFullyQualifiedApp (),
                SK_GetHostApp (),
            SK_UTF8ToWideChar (
             SK_UseManifestToGetAppName   (AppID)
                               ).c_str (), AppID
    );

    app_cache_mgr->saveAppCache       (true);
    app_cache_mgr->loadAppCacheForExe (SK_GetFullyQualifiedApp ());

    // Trigger profile migration if necessary
    app_cache_mgr->getConfigPathForAppID (AppID);

    return config.steam.appid;
  }

  return 0;
}

extern void SK_CPU_InstallHooks (void);

BOOL __SK_DisableQuickHook = FALSE;

void
SK_DisableDPIScaling (void)
{
  DWORD   dwProcessSize = MAX_PATH;
  wchar_t wszProcessName [MAX_PATH + 2] = { };

  HANDLE hProc =
   SK_GetCurrentProcess ();

  QueryFullProcessImageName (
    hProc, 0,
      wszProcessName, &dwProcessSize
  );

  const wchar_t* wszKey        =
    LR"(Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers)";
  DWORD          dwDisposition = 0x00;
  HKEY           hKey          = nullptr;

  const LSTATUS status =
    RegCreateKeyExW ( HKEY_CURRENT_USER,
                        wszKey,      0,
                          nullptr, 0x0,
                          KEY_READ | KEY_WRITE,
                             nullptr,
                               &hKey,
                                 &dwDisposition );

  if ( status == ERROR_SUCCESS &&
       hKey   != nullptr          )
  {
    RegSetValueExW (
      hKey, wszProcessName,
        0, REG_SZ,
          (BYTE *)L"~ HIGHDPIAWARE",
             16 * sizeof (wchar_t) );

    RegFlushKey (hKey);
    RegCloseKey (hKey);
  }
}

bool
__stdcall
SK_StartupCore (const wchar_t* backend, void* callback)
{
  SK_DisableDPIScaling ();

  __SK_BootedCore = backend;

  // Before loading any config files, test the game's environment variables
  //  to determine if the Steam client has given us the AppID without having
  //    to initialize SteamAPI first.
  SK_Steam_GetAppID_NoAPI ();


  // Allow users to centralize all files if they want
  //
  //   Stupid hack, if the application is running with a different working-directory than
  //     the executable -- compensate!
  wchar_t    wszCentralPathVFile [MAX_PATH * 2 + 1] = { };
  _swprintf (wszCentralPathVFile, LR"(%ws\SpecialK.central)", SK_GetHostPath ());

  if ( GetFileAttributes (wszCentralPathVFile) != INVALID_FILE_ATTRIBUTES )
    config.system.central_repository = true;

  if (SK_IsInjected ())
    config.system.central_repository = true;

  SK_EstablishRootPath ();
  SK_CreateDirectories (SK_GetConfigPath ());

#if 0
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
                               std::wstring (LR"(Game\)") ).c_str ()
                           ) == INVALID_FILE_ATTRIBUTES )
    {
      std::wstring link (SK_GetConfigPath ());
                   link += LR"(Game\)";

      CreateSymbolicLink (
        link.c_str         (),
          SK_GetHostPath   (),
            SYMBOLIC_LINK_FLAG_DIRECTORY
      );
    }
  }
#endif

  extern bool __SK_RunDLL_Bypass;

  const bool rundll_invoked =
    (StrStrIW (SK_GetHostApp (), L"Rundll32") != nullptr);
  const bool skim           =
    rundll_invoked || SK_IsSuperSpecialK () || __SK_RunDLL_Bypass;

  __SK_bypass |= skim;


  auto& init_ =
    SKX_GetInitParams ();

  init_          = {     };
  init_.backend  = backend;
  init_.callback = callback;

  SK_SetBackend (backend);

  bool blacklist = false;

  // Injection Compatibility Menu
  if ( (gsl::narrow_cast <USHORT> (SK_GetAsyncKeyState (VK_SHIFT  )) & 0x8000) != 0 &&
       (gsl::narrow_cast <USHORT> (SK_GetAsyncKeyState (VK_CONTROL)) & 0x8000) != 0 )
  {
    WriteRelease (&__SK_Init, -1);
                   __SK_bypass = true;

    SK_BypassInject ();
  }

  else
  {
    blacklist =
      SK_IsInjected () &&
      (GetFileAttributesW (SK_GetBlacklistFilename ()) != INVALID_FILE_ATTRIBUTES);

    if (! blacklist)
    {
       wchar_t   log_fname [MAX_PATH * 2 + 1] = { };
      _swprintf (log_fname, L"logs/%s.log", SK_IsInjected () ? L"SpecialK" : backend);

      dll_log->init (log_fname, L"wS+,ccs=UTF-8");
      dll_log->Log  ( L"%s.log created\t\t(Special K  %s,  %hs)",
                        SK_IsInjected () ? L"SpecialK" :
                                           backend,
                                           SK_GetVersionStrW (),
                                           __DATE__ );

      init_.start_time =
        SK_QueryPerf ();

      game_debug->init (L"logs/game_output.log", L"w");
      game_debug->lockless = true;


      // Setup unhooked function pointers
      SK_PreInitLoadLibrary     ();
      SK_MinHook_Init           ();
      //// Do this from the startup thread [these functions queue, but don't apply]
      SK_Input_PreInit          (); // Hook only symbols in user32 and kernel32
      SK_HookWinAPI             ();
      SK_Thread_InitDebugExtras ();

      SK_CPU_InstallHooks       ();

      SK_NvAPI_PreInitHDR       ();
      SK_NvAPI_InitializeHDR    ();
      SK::Diagnostics::Debugger::Allow        ();
      SK::Diagnostics::CrashHandler::InitSyms ();
      SK_ApplyQueuedHooks       ();

      SK_Thread_Create ([](LPVOID) -> DWORD
      {
        SetCurrentThreadDescription (                         L"[SK] Init Cleanup"    );
        SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_HIGHEST );
        SetThreadPriorityBoost      ( SK_GetCurrentThread (), TRUE                    );

        WaitForInit           ();

        SK_Memory_InitHooks   ();

        if (SK_GetDLLRole () != DLL_ROLE::DInput8)
        {
          if (SK_GetModuleHandle (L"dinput8.dll"))
            SK_Input_HookDI8  ();

          if (SK_GetModuleHandle (L"dinput.dll"))
            SK_Input_HookDI7  ();

          SK_Input_Init       ();
        }

        SK_ApplyQueuedHooks   ();

        // Setup the compatibility backend, which monitors loaded libraries,
        //   blacklists bad DLLs and detects render APIs...
        SK_EnumLoadedModules  (SK_ModuleEnum::PostLoad);
        SK_ApplyQueuedHooks   ();

        SK_Thread_CloseSelf   ();

        return 0;
      });

      // For the global injector, when not started by SKIM, check its version
      if ( (SK_IsInjected () && (! SK_IsSuperSpecialK ())) )
        SK_Thread_Create ( CheckVersionThread );
    }
  }


  if (skim || blacklist)
  {
    return true;
  }


  budget_log->init ( LR"(logs\dxgi_budget.log)", L"wc+,ccs=UTF-8" );

  dll_log->LogEx (false,
    L"------------------------------------------------------------------------"
    L"-------------------\n");

  std::wstring   module_name   = SK_GetModuleName (SK_GetDLL ());
  const wchar_t* wszModuleName = module_name.c_str ();

  dll_log->Log   (      L">> (%s) [%s] <<",
                          SK_GetHostApp (),
                            wszModuleName );

  const wchar_t* config_name = backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  LARGE_INTEGER liStartConfig = SK_CurrentPerf ();

  dll_log->LogEx (true, L"Loading user preferences from %s.ini... ", config_name);

  if (SK_LoadConfig (config_name))
    dll_log->LogEx (false, L"done! (%6.2f ms)\n",
      SK_DeltaPerfMS (liStartConfig.QuadPart, 1));

  else
  {
    dll_log->LogEx (false, L"failed!\n");

    std::wstring default_name (L"default_");
                 default_name += config_name;

    std::wstring default_ini (default_name + L".ini");

    if (GetFileAttributesW (default_ini.c_str ()) != INVALID_FILE_ATTRIBUTES)
    {
      dll_log->LogEx (true, L"Loading default preferences from %s.ini... ", default_name.c_str ());

      if (! SK_LoadConfig (default_name))
        dll_log->LogEx (false, L"failed!\n");
      else
        dll_log->LogEx (false, L"done!\n");
    }

    // If no INI file exists, write one immediately.
    dll_log->LogEx (true, L"  >> Writing base INI file, because none existed... ");
    SK_SaveConfig (config_name);
    dll_log->LogEx (false, L"done!\n");
  }


  if (! __SK_bypass)
  {
    SK_File_InitHooks    ();
    SK_Network_InitHooks ();

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

      //// It is not permissable to modify steam_api64.dll in this game
      //                                     (Denuvo will crash)
      //
      if ( SK_GetCurrentGameID () != SK_GAME_ID::MonsterHunterWorld)
      {
        ///// Lazy-load SteamAPI into a process that doesn't use it; this brings
        /////   a number of general-purpose benefits (such as battery charge monitoring).
        bool kick_start = config.steam.force_load_steamapi;

        if ((! SK_Steam_TestImports (GetModuleHandle (nullptr))))
        {
          // Implicitly kick-start anything in SteamApps\common that does not import
          //   SteamAPI...
          if ((! kick_start) && config.steam.auto_inject)
          {
            if (StrStrIW (SK_GetHostPath (), LR"(SteamApps\common)") != nullptr)
            {
              extern const wchar_t*
              SK_Steam_GetDLLPath (void);

              // Only do this if the game doesn't have a copy of the DLL lying around somewhere,
              //   because if we init Special K's SteamAPI DLL, the game's will fail to init and
              //     the game won't be happy about that!
              kick_start =
                (! SK_Modules->LoadLibrary (SK_Steam_GetDLLPath ())) ||
                      config.steam.force_load_steamapi;
            }
          }

          if (kick_start)
            SK_Steam_KickStart ();
        }

        extern BOOL
        SK_Steam_PreHookCore (const wchar_t* wszTry = nullptr);
        SK_Steam_PreHookCore ();
      }
    }

    if (SK_COMPAT_IsFrapsPresent ())
        SK_COMPAT_UnloadFraps ();

    SK_EnumLoadedModules (SK_ModuleEnum::PreLoad);
  }


  dll_log->LogEx (false,
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
    wsprintf (wszProxyName, LR"(%s\PlugIns\ThirdParty\dgVoodoo\d3d8.dll)",
                std::wstring (SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)").c_str ());

  else if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    wsprintf (wszProxyName, LR"(%s\PlugIns\ThirdParty\dgVoodoo\ddraw.dll)",
                std::wstring (SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)").c_str ());
#endif


  wchar_t wszBackendDLL [MAX_PATH * 2 + 1] = { };
  wchar_t wszWorkDir    [MAX_PATH * 2 + 1] = { };

  wcsncpy_s ( wszBackendDLL,             MAX_PATH,
              SK_GetSystemDirectory (), _TRUNCATE );

  GetCurrentDirectoryW (MAX_PATH, wszWorkDir);
               SK_ConcealUserDir (wszWorkDir);

  dll_log->Log (L" Working Directory:          %s", SK_ConcealUserDir (std::wstring (wszWorkDir).data    ()));
  dll_log->Log (L" System Directory:           %s", SK_ConcealUserDir (std::wstring (wszBackendDLL).data ()));

  lstrcatW (wszBackendDLL, LR"(\)");
  lstrcatW (wszBackendDLL, backend);
  lstrcatW (wszBackendDLL, L".dll");

  const wchar_t* dll_name =
    wszBackendDLL;

  if (! SK_Path_wcsicmp (wszProxyName, wszModuleName))
    dll_name = wszBackendDLL;
  else
    dll_name = wszProxyName;

  bool load_proxy = false;

  if (! SK_IsInjected ())
  {
    for (auto& import : imports->imports)
    {
      if (            import.role != nullptr &&
           backend == import.role->get_value () )
      {
        dll_log->LogEx (true, L" Loading proxy %s.dll:    ", backend);
        dll_name   = _wcsdup (import.filename->get_value ().c_str ());
        load_proxy = true;
        break;
      }
    }
  }

  if (! load_proxy)
    dll_log->LogEx (true, L" Loading default %s.dll: ", backend);

  // Pre-Load the original DLL into memory
  if (dll_name != wszBackendDLL)
  {
                  SK_Modules->LoadLibraryLL (wszBackendDLL);
    backend_dll = SK_Modules->LoadLibraryLL (dll_name);
  }

  else
    backend_dll = SK_Modules->LoadLibraryLL (dll_name);

  if (backend_dll != nullptr)
    dll_log->LogEx (false, L" (%s)\n",         SK_ConcealUserDir (std::wstring (dll_name).data ()));
  else
    dll_log->LogEx (false, L" FAILED (%s)!\n", SK_ConcealUserDir (std::wstring (dll_name).data ()));

  // Free the temporary string storage
  if (load_proxy)
    free ((void *)dll_name);

  dll_log->LogEx (false,
    L"----------------------------------------------------------------------"
    L"---------------------\n");


  if (config.system.silent)
  {
    dll_log->silent = true;

    std::wstring log_fnameW;

    if (! SK_IsInjected ())
      log_fnameW = backend;
    else
      log_fnameW = L"SpecialK";

    log_fnameW += L".log";

    DeleteFile (log_fnameW.c_str ());
  }

  else
    dll_log->silent = false;

  if (! __SK_bypass)
  {
#ifdef _M_AMD64
    switch (SK_GetCurrentGameID ())
    {
      case SK_GAME_ID::StarOcean4:
        extern bool                       SK_SO4_PlugInCfg (void);
        plugin_mgr->config_fns.push_back (SK_SO4_PlugInCfg);
        break;

      case SK_GAME_ID::AssassinsCreed_Odyssey:
        extern void
        SK_ACO_PlugInInit (void);
        SK_ACO_PlugInInit (    );
        break;
      case SK_GAME_ID::MonsterHunterWorld:
        extern void
        SK_MHW_PlugInInit (void);
        SK_MHW_PlugInInit (    );
        break;

      case SK_GAME_ID::DragonQuestXI:
        extern void
        SK_DQXI_PlugInInit (void);
        SK_DQXI_PlugInInit (    );
        break;

      case SK_GAME_ID::Shenmue:
        extern void
        SK_SM_PlugInInit (void);
        SK_SM_PlugInInit (    );
        break;

      case SK_GAME_ID::FinalFantasyXV:
        extern void
        SK_FFXV_InitPlugin (void);
        SK_FFXV_InitPlugin (    );
        break;

      case SK_GAME_ID::PillarsOfEternity2:
        extern bool                       SK_POE2_PlugInCfg (void);
        plugin_mgr->config_fns.push_back (SK_POE2_PlugInCfg);
        break;

      case SK_GAME_ID::LifeIsStrange_BeforeTheStorm:
        extern bool                       SK_LSBTS_PlugInCfg (void);
        plugin_mgr->config_fns.push_back (SK_LSBTS_PlugInCfg);
        break;

      case SK_GAME_ID::Okami:
        extern bool                       SK_Okami_PlugInCfg (void);
        plugin_mgr->config_fns.push_back (SK_Okami_PlugInCfg);
        break;
    }
#endif

    extern void SK_Widget_InitHDR            (void);
    SK_RunOnce (SK_Widget_InitHDR            ());

    SK_ImGui_Widgets->hdr_control->run ();

    bool gl   = false, vulkan = false, d3d9  = false, d3d11 = false,
         dxgi = false, d3d8   = false, ddraw = false, glide = false;

    SK_TestRenderImports (
      GetModuleHandle (nullptr),
        &gl, &vulkan,
          &d3d9, &dxgi, &d3d11,
            &d3d8, &ddraw, &glide );

    dxgi  |= SK_GetModuleHandle (L"dxgi.dll")     != nullptr;
    d3d11 |= SK_GetModuleHandle (L"d3d11.dll")    != nullptr;
    d3d9  |= SK_GetModuleHandle (L"d3d9.dll")     != nullptr;
    gl    |= SK_GetModuleHandle (L"OpenGL32.dll") != nullptr;
    gl    |= SK_GetModuleHandle (L"gdi32.dll")    != nullptr;
    gl    |= SK_GetModuleHandle (L"gdi32full.dll")!= nullptr;

    if ((dxgi || d3d11 || d3d8 || ddraw) && config.apis.dxgi.d3d11.hook /*|| config.apis.dxgi.d3d12.hook*/)
    {
      SK_DXGI_QuickHook   ();
    }

    if (d3d9 && (config.apis.d3d9.hook || config.apis.d3d9ex.hook))
    {
      SK_D3D9_QuickHook   ();
    }

    if (config.steam.preload_overlay)
    {
      SK_Steam_LoadOverlayEarly ();
    }

    SK_ApplyQueuedHooks ();

    InterlockedExchangePointer (
      const_cast <void **> (&hInitThread),
      SK_Thread_CreateEx ( DllThread, nullptr,
                               &init_ )
    ); // Avoid the temptation to wait on this thread
  }

  return true;
}


struct SK_DummyWindows {
  std::set <HWND> list;
  std::mutex      lock;
};

SK_LazyGlobal <SK_DummyWindows> dummy_windows;

HWND
SK_Win32_CreateDummyWindow (void)
{
  std::scoped_lock <std::mutex> auto_lock (dummy_windows->lock);

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

    if (hWnd != SK_HWND_DESKTOP)
    {
      ShowWindowAsync (hWnd, SW_SHOWMINNOACTIVE);
      dummy_windows->list.emplace (hWnd);
    }

    return hWnd;
  }

  else
  {
    dll_log->Log (L"Window Class Registration Failed!");
  }

  return nullptr;
}

void
SK_Win32_CleanupDummyWindow (HWND hwnd)
{
  std::scoped_lock <std::mutex>
    auto_lock (dummy_windows->lock);

  std::set <HWND> cleaned_windows;

  for ( auto& it : dummy_windows->list )
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
    dummy_windows->list.erase (it);

  if (dummy_windows->list.empty ())
    UnregisterClassW ( L"Special K Dummy Window Class", SK_GetDLL () );
}

bool
__stdcall
SK_ShutdownCore (const wchar_t* backend)
{
  if (        __SK_DLL_TeardownEvent != nullptr)
    SetEvent (__SK_DLL_TeardownEvent);

  // Fast path for DLLs that were never really attached.
  extern __time64_t
        __SK_DLL_AttachTime;
  if (! __SK_DLL_AttachTime)
  {
    SK_MinHook_UnInit ();
    return true;
  }

  SK_PrintUnloadedDLLs (&dll_log.get ());
  dll_log->LogEx ( false,
                L"========================================================="
                L"========= (End  Unloads) ================================"
                L"==================================\n" );

  dll_log->Log (L"[ SpecialK ] *** Initiating DLL Shutdown ***");
  SK_Win32_CleanupDummyWindow ();


  if (config.window.background_mute)
    SK_SetGameMute (false);

  // These games do not handle resolution correctly
  switch (SK_GetCurrentGameID ())
  {
    case SK_GAME_ID::DarkSouls3:
    case SK_GAME_ID::Fallout4:
    case SK_GAME_ID::FinalFantasyX_X2:
    case SK_GAME_ID::DisgaeaPC:
      ChangeDisplaySettingsA_Original (nullptr, CDS_RESET);
      break;

#ifdef _M_AMD64
    case SK_GAME_ID::MonsterHunterWorld:
    {
      extern void SK_MHW_PlugIn_Shutdown (void);
                  SK_MHW_PlugIn_Shutdown ();
    } break;
#endif
  }

  SK_AutoClose_LogEx (game_debug, game);
  SK_AutoClose_LogEx (dll_log,    dll);

  SK_Console::getInstance ()->End ();

  SK::DXGI::ShutdownBudgetThread ();

  dll_log->LogEx    (true, L"[ GPU Stat ] Shutting down Prognostics "
                           L"Thread...          ");

  DWORD dwTime =
       timeGetTime ();
  SK_EndGPUPolling ();

  dll_log->LogEx    (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);


  SK_Steam_KillPump ();

  auto ShutdownWMIThread =
  []( volatile HANDLE&  hSignal,
      volatile HANDLE&  hThread,
        const wchar_t* wszName )
  {
    if ( ReadPointerAcquire (&hSignal) == INVALID_HANDLE_VALUE &&
         ReadPointerAcquire (&hThread) == INVALID_HANDLE_VALUE )
      return;

    wchar_t wszFmtName [32] = { };

    lstrcatW (wszFmtName, wszName);
    lstrcatW (wszFmtName, L"...");

    dll_log->LogEx (true, L"[ Perfmon. ] Shutting down %-30s ", wszFmtName);

    DWORD dwTime = timeGetTime ();

    // Signal the thread to shutdown
    if (SignalObjectAndWait (hSignal, hThread, 66UL, TRUE) != WAIT_OBJECT_0) // Give 66 milliseconds, and
    {                                                                        // then we're killing
      TerminateThread (hThread, 0x00);                                       // the thing!
    }

    CloseHandle (hThread);
                 hThread  = INVALID_HANDLE_VALUE;

    dll_log->LogEx (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);
  };

  auto& cpu_stats      = *SK_WMI_CPUStats;
  auto& disk_stats     = *SK_WMI_DiskStats;
  auto& pagefile_stats = *SK_WMI_PagefileStats;

  ShutdownWMIThread (cpu_stats .hShutdownSignal,
                     cpu_stats.hThread,               L"CPU Monitor"     );
  ShutdownWMIThread (disk_stats.hShutdownSignal,
                     disk_stats.hThread,              L"Disk Monitor"    );
  ShutdownWMIThread (pagefile_stats.hShutdownSignal,
                     pagefile_stats.hThread,          L"Pagefile Monitor");

  const wchar_t* config_name = backend;

  if (SK_IsInjected ())
    config_name = L"SpecialK";

  PowerSetActiveScheme (nullptr, &config.cpu.power_scheme_guid_orig);

  if (sk::NVAPI::app_name != L"ds3t.exe" && SK_GetFramesDrawn () > 0)
  {
    dll_log->LogEx       (true,  L"[ SpecialK ] Saving user preferences to"
                                 L" %10s.ini... ", config_name);
    dwTime = timeGetTime ();
    SK_SaveConfig        (config_name);
    dll_log->LogEx       (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);
  }


  if (! SK_Debug_IsCrashing ())
  {
    SK_GetCurrentRenderBackend ().releaseOwnedResources ();

    SK_UnloadImports        ();
    SK::Framerate::Shutdown ();

    dll_log->LogEx       (true, L"[ SpecialK ] Shutting down MinHook...                     ");

    dwTime = timeGetTime ();
    SK_MinHook_UnInit    ();
    dll_log->LogEx       (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);


    dll_log->LogEx       (true, L"[ WMI Perf ] Shutting down WMI WbemLocator...             ");
    dwTime = timeGetTime ();
    SK_WMI_Shutdown      ();
    dll_log->LogEx       (false, L"done! (%4u ms)\n", timeGetTime () - dwTime);



    if (nvapi_init)
      sk::NVAPI::UnloadLibrary ();
  }


  dll_log->Log (L"[ SpecialK ] Custom %s.dll Detached (pid=0x%04x)",
    backend, GetCurrentProcessId ());


  if (! SK_Debug_IsCrashing ())
  {
    if (config.system.handle_crashes)
      SK::Diagnostics::CrashHandler::Shutdown ();
  }

  WriteRelease (&__SK_Init, -2);

  return true;
}




void
SK_UnpackCEGUI (void)
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
      SK_LOG0 ( ( L"Unpacking CEGUI for first-time execution" ),
                  L"CEGUI-Inst" );

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

      if (fPackedCEGUI != nullptr)
      {
        fwrite    (locked, 1, res_size, fPackedCEGUI);
        fclose    (fPackedCEGUI);
      }

      if (GetFileAttributesW (wszArchive) != INVALID_FILE_ATTRIBUTES)
      {
        SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
        DeleteFileW       (wszArchive);
      }
    }

    UnlockResource (packed_cegui);
  }
};



extern void SK_ImGui_LoadFonts           (void);
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

  else if (hWndTarget == HWND_DESKTOP)
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

static volatile LONG CEGUI_Init = FALSE;

void
SetupCEGUI (SK_RenderAPI& LastKnownAPI)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

#ifdef _DEBUG
return;
#endif

  if ( (rb.api != SK_RenderAPI::Reserved) &&
        rb.api == LastKnownAPI            &&
       ( (! InterlockedCompareExchange (&CEGUI_Init, TRUE, FALSE)) ) )
  {
    bool local = false;
    if (! SK_COMPAT_IsSystemDllInstalled (L"D3DCompiler_43.dll", &local))
    {
      if (! local)
      {
        config.cegui.enable = false;
        SK_ImGui_Warning    (L"CEGUI has been disabled due to missing DirectX files.");
        InterlockedExchange (&CEGUI_Init, TRUE);

        return;
      }
    }

    InterlockedIncrement (&SK_RenderBackend::frames_drawn);

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
    wchar_t wszEnvPath      [ MAX_PATH + 64 ] = { };

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
      // Disable CEGUI if unpack fails
      SK_SaveConfig  ();
      SK_UnpackCEGUI ();
    }

    _wputenv  (wszEnvPath);

    lstrcatW  (wszCEGUITestDLL, wszCEGUIModPath);
    lstrcatW  (wszCEGUITestDLL, L"\\CEGUIBase-0.dll");

    wchar_t wszWorkingDir [MAX_PATH * 2 + 1] = { };

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
        GetProcAddress ( SK_GetModuleHandle (L"kernel32"),
                                                 "AddDllDirectory" );

    static auto k32_RemoveDllDirectory =
      (RemoveDllDirectory_pfn)
        GetProcAddress ( SK_GetModuleHandle (L"kernel32"),
                                                 "RemoveDllDirectory" );

    static auto k32_SetDefaultDllDirectories =
      (SetDefaultDllDirectories_pfn)
        GetProcAddress ( SK_GetModuleHandle (L"kernel32"),
                                                 "SetDefaultDllDirectories" );

    const int thread_locale =
      _configthreadlocale (0);
      _configthreadlocale (_ENABLE_PER_THREAD_LOCALE);

    char* szLocale =
      setlocale (LC_ALL, nullptr);

    CHeapPtr <char> locale_orig ( szLocale == nullptr ? szLocale :
                                               _strdup (szLocale) );
    if        (szLocale) setlocale (LC_ALL, "C");

    if ( k32_AddDllDirectory          && k32_RemoveDllDirectory &&
         k32_SetDefaultDllDirectories &&

           GetFileAttributesW (wszCEGUITestDLL) != INVALID_FILE_ATTRIBUTES )
    {
      SK_ConcealUserDir (wszCEGUITestDLL);

      dll_log->Log (L"[  CEGUI   ] Enabling CEGUI: (%s)", wszCEGUITestDLL);

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

          static auto
               setRet = [&](bool set_val) ->
          void {  ret =          set_val; };

          auto orig_se =
          SK_SEH_ApplyTranslator (
            [](unsigned int nExceptionCode, EXCEPTION_POINTERS* pException)->
            void
            {
              if (nExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
                  nExceptionCode == 0xc06d007e)
              {
                throw (SK_SEH_IgnoredException ());
              }

              RaiseException ( nExceptionCode,
                               pException->ExceptionRecord->ExceptionFlags,
                               pException->ExceptionRecord->NumberParameters,
                               pException->ExceptionRecord->ExceptionInformation );
            }
          );

          try
          {
            cookie = k32_AddDllDirectory (wszCEGUIModPath);
            ret    = SUCCEEDED           (__HrLoadAllImportsForDll (szDLL));
          }

          catch (const SK_SEH_IgnoredException&)
          {
            // The magic number 0xc06d007e will come about if the DLL we're trying
            //   to delayload has some sort of unsatisfied dependency
            ret = false;
          }
          SK_SEH_RemoveTranslator (orig_se);

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

          if (rb.api == SK_RenderAPI::OpenGL)
          {
            if (config.apis.OpenGL.hook)
            {
              config.cegui.enable =
                DelayLoadDLL ("CEGUIOpenGLRenderer-0.dll");
            }
          }

          if ( rb.api == SK_RenderAPI::D3D9 ||
               rb.api == SK_RenderAPI::D3D9Ex )
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

    if (locale_orig != nullptr)
      setlocale (LC_ALL, locale_orig);
    _configthreadlocale (thread_locale);


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


__declspec (noinline)
void
__stdcall
SK_BeginBufferSwap (void)
{
  static concurrency::concurrent_unordered_set <DWORD> render_threads;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  static const auto&
    game_id = SK_GetCurrentGameID ();

  if ( (int)rb.api        &
       (int)SK_RenderAPI::D3D11 )
  {
    SK_D3D11_BeginFrame ();

    if (__SK_FramerateLimitApplicationSite == 0)
      SK::Framerate::GetLimiter ()->wait ();
  }

  rb.present_staging.begin_overlays.time.QuadPart =
    SK_QueryPerf ().QuadPart;

  static SK_RenderAPI LastKnownAPI =
    SK_RenderAPI::Reserved;

  if (config.render.framerate.enable_mmcss)
  {
    if ( ! render_threads.count (SK_Thread_GetCurrentId ()) )
    {
      static bool   first = true;
      auto*  task = first ?
        SK_MMCS_GetTaskForThreadIDEx ( SK_Thread_GetCurrentId (),
                                         "[GAME] Primary Render Thread",
                                           "Games", "DisplayPostProcessing" )
                         :
        SK_MMCS_GetTaskForThreadIDEx ( SK_Thread_GetCurrentId (),
                                         "[GAME] Ancillary Render Thread",
                                           "Games", "DisplayPostProcessing" );

      if ( task != nullptr )
      {
        if (game_id != SK_GAME_ID::AssassinsCreed_Odyssey)
        {
          if (first)
            task->queuePriority (AVRT_PRIORITY_CRITICAL);
          else
            task->queuePriority (AVRT_PRIORITY_HIGH);
        }

        else
          task->queuePriority (AVRT_PRIORITY_LOW);

        render_threads.insert (SK_Thread_GetCurrentId ());

        first = false;
      }
    }
  }


  for ( auto begin_frame_fn : plugin_mgr->begin_frame_fns )
  {
    begin_frame_fn ();
  }


  const ImGuiIO& io =
    ImGui::GetIO ();

  if (io.Fonts == nullptr)
  {
    SK_ImGui_LoadFonts ();
  }

  const ULONG frames_drawn =
    SK_GetFramesDrawn ();

  switch (frames_drawn)
  {
    // First frame
    //
    case 0:
    {
      wchar_t *wszDescription = nullptr;

      if ( SUCCEEDED ( GetCurrentThreadDescription (&wszDescription)) &&
                                            wcslen ( wszDescription))
      {
        if (wcscmp (wszDescription, L"[GAME] Primary Render Thread") != 0)
        {
          SK_RunOnce (
            SetCurrentThreadDescription (
              SK_FormatStringW ( L"[GAME] Primary Render < %s >",
                wszDescription ).c_str ()
                                        )
          );
          SK_LocalFree (wszDescription);
        }
      }

      else
      {
        SK_RunOnce (
          SetCurrentThreadDescription (L"[GAME] Primary Render Thread")
        );
      }


      extern SK_Widget* SK_HDR_GetWidget (void);
                SK_HDR_GetWidget ()->run (    );


      // Maybe make this into an option, but for now just get this the hell out
      //   of there almost no software should be shipping with FP exceptions,
      //     it causes compatibility problems.
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

                           __target_fps = config.render.framerate.target_fps;
      SK::Framerate::GetLimiter ()->init (config.render.framerate.target_fps);
    } break;


    // Grace period frame, just ignore it
    //
    case 1:
    case 2:
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


  rb.present_staging.begin_cegui.time =
    SK_QueryPerf ();

  if (rb.api != SK_RenderAPI::D3D12)
  {
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
  }

  rb.present_staging.end_cegui.time =
    SK_QueryPerf ();


  static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");

  if (hModTBFix)
  {
    if (SK_Steam_PiratesAhoy () != 0x00)
    {
      const char* szFirst = "First-frame Done";
             __target_fps =
        static_cast <float> (
          *reinterpret_cast <const uint8_t *> (szFirst + 5)
        );
    }

    SK::Framerate::GetLimiter ()->wait ();
  }

  if ( (int)rb.api        &
       (int)SK_RenderAPI::D3D11 )
  {
    if (__SK_FramerateLimitApplicationSite == 1)
            SK::Framerate::GetLimiter ()->wait ();
  }

  if (SK_Steam_PiratesAhoy () && (! SK_ImGui_Active ()))
  {
    SK_ImGui_Toggle ();
  }


  LastKnownAPI =
    rb.api;


  rb.present_staging.submit.time =
    SK_QueryPerf ();

  if (__SK_FramerateLimitApplicationSite == 4)
    SK::Framerate::GetLimiter ()->wait ();
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

  if ( game_window.active )
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

  const ImGuiIO& io (ImGui::GetIO ());

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



void
SK_BackgroundRender_EndFrame (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  static bool background_last_frame = false;
  static bool fullscreen_last_frame = false;
  static bool first_frame           = true;

  if (            first_frame ||
       (background_last_frame != config.window.background_render) )
  {
    first_frame = false;

    // Does not indicate the window was IN the background, but that it
    //   was rendering in a special mode that would allow the game to
    //     continue running while it is in the background.
    background_last_frame =
        config.window.background_render;
    if (config.window.background_render)
    {
      HWND     hWndGame  = SK_GetGameWindow ();
      LONG_PTR lpStyleEx =
        SK_GetWindowLongPtrW ( hWndGame,
                                 GWL_EXSTYLE );

      LONG_PTR lpStyleExNew =
            // Add style to ensure the game shows in the taskbar ...
        ( ( lpStyleEx |  WS_EX_APPWINDOW  )
                      & ~WS_EX_TOOLWINDOW );
                     // And remove one that prevents taskbar activation...

      ShowWindowAsync            ( hWndGame,
                                     SW_SHOW );

      if (lpStyleExNew != lpStyleEx)
      {
        SK_SetWindowLongPtrW     ( hWndGame,
                                     GWL_EXSTYLE,
                                       lpStyleExNew );
      }

      SK_RealizeForegroundWindow ( hWndGame );
    }
  }

  fullscreen_last_frame =
    rb.fullscreen_exclusive;
}


__declspec (noinline) // lol
HRESULT
__stdcall
SK_EndBufferSwap (HRESULT hr, IUnknown* device, SK_TLS* pTLS)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if ( (int)rb.api        &
       (int)SK_RenderAPI::D3D11 )
  {
    if (__SK_FramerateLimitApplicationSite == 3)
         SK::Framerate::GetLimiter ()->wait ();
  }


  // Various required actions at the end of every frame in order to
  //   support the background render mode in most games.
  SK_BackgroundRender_EndFrame ();



  static SK_RenderAPI LastKnownAPI =
         SK_RenderAPI::Reserved;

  static const auto&
    game_id = SK_GetCurrentGameID ();

  assert ( ReadULongAcquire (&rb.thread) == (LONG)SK_Thread_GetCurrentId () ||
           LastKnownAPI                  ==       SK_RenderAPI::Reserved );

  if (device != nullptr && LastKnownAPI != rb.api)
  {
    WriteULongRelease (&rb.thread, SK_Thread_GetCurrentId ());

    SK_LOG0 ( ( L"SwapChain Presentation Thread has Priority=%i",
                GetThreadPriority (SK_GetCurrentThread ()) ),
                L"RenderBack" );

    SK_ComPtr <IDirect3DDevice9>   pDev9   = nullptr;
    SK_ComPtr <IDirect3DDevice9Ex> pDev9Ex = nullptr;
    SK_ComPtr <ID3D11Device>       pDev11  = nullptr;
  //SK_ComPtr <ID3D12Device>       pDev12  = nullptr;

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

    ///else if (SUCCEEDED (device->QueryInterface <ID3D12Device> (&pDev12)))
    ///{
    ///  rb.api  = SK_RenderAPI::D3D12;
    ///  wcsncpy (rb.name, L"D3D12 ", 8);
    ///}

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

      SK_ComPtr <IUnknown> pTest = nullptr;

      if (       SUCCEEDED (device->QueryInterface (IID_ID3D11Device5, (void **)&pTest))) {
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

    SK_ComQIPtr <IDXGISwapChain> pSwapChain (rb.swapchain);

    if ( pSwapChain != nullptr &&
  SUCCEEDED (pSwapChain->GetFullscreenState (&fullscreen, nullptr)) )
    {
      rb.fullscreen_exclusive =               fullscreen;
    }

    else
      rb.fullscreen_exclusive = false;


    extern volatile
           LONG
    __SK_D3D11_InitiateHudFreeShot;

    extern LONG
    SK_D3D11_ShowGameHUD (void);

    if (InterlockedCompareExchange (&__SK_D3D11_InitiateHudFreeShot, 0, -1) == -1)
    {
      SK_D3D11_ShowGameHUD ();
    }

    // Also clear any resources we were tracking for the shader mod subsystem
    SK_D3D11_EndFrame (pTLS);
  }

  ///if ( static_cast <int> (rb.api)  &
  ///     static_cast <int> (SK_RenderAPI::D3D12) )
  ///{
  ///  BOOL fullscreen = FALSE;
  ///
  ///  SK_ComPtr                        <IDXGISwapChain>   pSwapChain = nullptr;
  ///  if (rb.swapchain)
  ///      rb.swapchain->QueryInterface <IDXGISwapChain> (&pSwapChain);
  ///
  ///  if ( pSwapChain &&
  ///       SUCCEEDED (pSwapChain->GetFullscreenState (&fullscreen, nullptr)))
  ///  { rb.fullscreen_exclusive =                        fullscreen;        }
  ///
  ///  else
  ///    rb.fullscreen_exclusive = false;
  ///
  ///  // Also clear any resources we were tracking for the shader mod subsystem
  ///  SK_D3D12_EndFrame (pTLS);
  ///}


#ifdef _M_AMD64
  switch (game_id)
  {
    case SK_GAME_ID::Shenmue:
      extern volatile LONG  __SK_SHENMUE_FinishedButNotPresented;
      WriteRelease        (&__SK_SHENMUE_FinishedButNotPresented, 0L);
      break;
    default:
      break;
  }
#endif


  LastKnownAPI = config.apis.last_known =
    rb.api;


  SK_RunOnce (SK::DXGI::StartBudgetThread_NoAdapter ());

  bool
  SK_SteamAPI_RunQueuedCallbacks (void);
  SK_SteamAPI_RunQueuedCallbacks (    );


  SK_Input_PollKeyboard ();

  InterlockedIncrementAcquire (&SK_RenderBackend::frames_drawn);


  if (config.sli.show && device != nullptr)
  {
    // Get SLI status for the frame we just displayed... this will show up
    //   one frame late, but this is the safest approach.
    if (nvapi_init && sk::NVAPI::CountSLIGPUs () > 0)
    {
      extern SK_LazyGlobal <NV_GET_CURRENT_SLI_STATE> sli_state;

      *sli_state =
        sk::NVAPI::GetSLIState (device);
    }
  }

  if (config.cegui.enable && ReadAcquire (&CEGUI_Init))
  {
    config.cegui.frames_drawn++;
  }


#ifdef _M_AMD64
  static const bool bFFXV =
    (game_id == SK_GAME_ID::FinalFantasyXV);

  if (bFFXV)
  {
    void SK_FFXV_SetupThreadPriorities (void);
         SK_FFXV_SetupThreadPriorities ();
  }
#endif

  SK_StartPerfMonThreads ();

  static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");
  static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");


  long double          dt      =    0.0l;
  LARGE_INTEGER            now = { 0, 0 };
  SK::Framerate::Tick (dt, now);

  //
  // TZFix has its own limiter
  //
  if (! (hModTZFix || hModTBFix))
  {
    if (__SK_FramerateLimitApplicationSite == 2)
      SK::Framerate::GetLimiter ()->wait ();
  }

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


SK_LazyGlobal <SK_ImGui_WidgetRegistry> SK_ImGui_Widgets;

HANDLE
WINAPI
SK_CreateEvent (
  _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
  _In_     BOOL                  bManualReset,
  _In_     BOOL                  bInitialState,
  _In_opt_ LPCWSTR               lpName )
{
  //if (SK_GetFramesDrawn ())
  //  SK_LOG_CALL ("CreateEventW");

  return
    CreateEventW (
      lpEventAttributes, bManualReset, bInitialState, lpName
    );
}

SK_LazyGlobal <iSK_Logger> dll_log;
SK_LazyGlobal <iSK_Logger> crash_log;
SK_LazyGlobal <iSK_Logger> budget_log;
SK_LazyGlobal <iSK_Logger> game_debug;
SK_LazyGlobal <iSK_Logger> tex_log;
SK_LazyGlobal <iSK_Logger> steam_log;