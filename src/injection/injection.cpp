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



#include <SpecialK/injection/injection.h>
#include <SpecialK/injection/address_cache.h>
#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/framerate.h>
#include <SpecialK/hooks.h>
#include <SpecialK/ini.h>
#include <SpecialK/window.h>
#include <SpecialK/core.h>
#include <SpecialK/config.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>

#include <set>
#include <Shlwapi.h>
#include <time.h>

extern "C"
{
SK_InjectionRecord_s __SK_InjectionHistory [MAX_INJECTED_PROC_HISTORY] = { };

#pragma data_seg (".SK_Hooks")
  __declspec (dllexport) SK_InjectionBase_s g_CBTHook;
  __declspec (dllexport) LONG               g_sHookedPIDs [MAX_INJECTED_PROCS]        = { 0 };

                wchar_t    __SK_InjectionHistory_name     [MAX_INJECTED_PROC_HISTORY * MAX_PATH] =  { 0 };
                DWORD      __SK_InjectionHistory_ids      [MAX_INJECTED_PROC_HISTORY]            =  { 0 };
                __time64_t __SK_InjectionHistory_inject   [MAX_INJECTED_PROC_HISTORY]            =  { 0 };
                __time64_t __SK_InjectionHistory_eject    [MAX_INJECTED_PROC_HISTORY]            =  { 0 };

                SK_RenderAPI __SK_InjectionHistory_api    [MAX_INJECTED_PROC_HISTORY]            =  { SK_RenderAPI::Reserved };
                ULONG64      __SK_InjectionHistory_frames [MAX_INJECTED_PROC_HISTORY]            =  { 0 };

  __declspec (dllexport) volatile LONG SK_InjectionRecord_s::count                 =  0L;
  __declspec (dllexport) volatile LONG SK_InjectionRecord_s::rollovers             =  0L;

  __declspec (dllexport) wchar_t whitelist_patterns [16 * MAX_PATH] = { 0 };
  __declspec (dllexport) int     whitelist_count                    =   0;

#pragma data_seg ()
#pragma comment  (linker, "/SECTION:.SK_Hooks,RWS")
};


extern volatile LONG  __SK_DLL_Attached;
extern volatile ULONG __SK_DLL_Refs;
extern volatile LONG  __SK_HookContextOwner;
extern volatile LONG  __SK_TLS_INDEX;

               HMODULE hModHookInstance = nullptr;



SK_InjectionRecord_s*
SK_Inject_GetRecord (int idx)
{
  wcsncpy (__SK_InjectionHistory [idx].process.name,   &__SK_InjectionHistory_name   [idx * MAX_PATH], MAX_PATH - 1);
           __SK_InjectionHistory [idx].process.id     = __SK_InjectionHistory_ids    [idx];
           __SK_InjectionHistory [idx].process.inject = __SK_InjectionHistory_inject [idx];
           __SK_InjectionHistory [idx].process.eject  = __SK_InjectionHistory_eject  [idx];
           
           __SK_InjectionHistory [idx].render.api     = __SK_InjectionHistory_api    [idx];
           __SK_InjectionHistory [idx].render.frames  = __SK_InjectionHistory_frames [idx];

  return &__SK_InjectionHistory [idx];
}

static LONG local_record = 0;

void
SK_Inject_InitShutdownEvent (void)
{
  if (g_CBTHook.hShutdownEvent == nullptr)
  {
    g_CBTHook.bAdmin           = SK_IsAdmin ();

    SECURITY_ATTRIBUTES sattr  = { };
    sattr.nLength              = sizeof SECURITY_ATTRIBUTES;
    sattr.bInheritHandle       = TRUE;
    sattr.lpSecurityDescriptor = nullptr;

    g_CBTHook.dwHookPID      = GetCurrentProcessId ();
    g_CBTHook.hShutdownEvent = 
      CreateEventW ( &sattr, TRUE, FALSE,
        SK_RunLHIfBitness ( 32, LR"(Global\SpecialK32_Reset)",
                                LR"(Global\SpecialK64_Reset)" ) );
  }
}


bool
__stdcall
SK_IsInjected (bool set)
{
// Indicates that the DLL is injected purely as a hooker, rather than
//   as a wrapper DLL.
  static std::atomic_bool __injected = false;

  if (__injected == true)
    return true;

  if (set)
  {
    __injected               = true;
  //SK_Inject_AddressManager = new SK_Inject_AddressCacheRegistry ();
  }

  return set;
}


void
SK_Inject_ValidateProcesses (void)
{
  for (volatile LONG& g_sHookedPID : g_sHookedPIDs)
  {
    CHandle hProc (
      OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE, 
                      ReadAcquire (&g_sHookedPID) )
                  );

    if (hProc == nullptr)
    {
      ReadAcquire (&g_sHookedPID);
    }

    else
    {
      DWORD dwExitCode = STILL_ACTIVE;

      GetExitCodeProcess (hProc, &dwExitCode);

      if (dwExitCode != STILL_ACTIVE)
        ReadAcquire (&g_sHookedPID);
    }
  }
}

void
SK_Inject_ReleaseProcess (void)
{
  if (! SK_IsInjected ())
    return;

  for (volatile LONG& g_sHookedPID : g_sHookedPIDs)
  {
    InterlockedCompareExchange (&g_sHookedPID, 0, GetCurrentProcessId ());
  }

  //if (local_record != 0)
  {
    _time64 (&__SK_InjectionHistory_eject [local_record]);

    __SK_InjectionHistory_api    [local_record] = SK_GetCurrentRenderBackend ().api;
    __SK_InjectionHistory_frames [local_record] = SK_GetFramesDrawn          ();

    //local_record.input.xinput  = SK_Input_
    // ...
    // ...
    // ...
    // ...
  }
}

void
SK_Inject_AcquireProcess (void)
{
  if (! SK_IsInjected ())
    return;

  for (volatile LONG& g_sHookedPID : g_sHookedPIDs)
  {
    if (! InterlockedCompareExchange (&g_sHookedPID, GetCurrentProcessId (), 0))
    {
      ULONG injection_idx = InterlockedIncrement (&SK_InjectionRecord_s::count);

      // Rollover and start erasing the oldest history
      if (injection_idx >= MAX_INJECTED_PROC_HISTORY)
      {
        if (InterlockedCompareExchange (&SK_InjectionRecord_s::count, 0, injection_idx))
        {
          injection_idx = 1;
          InterlockedIncrement (&SK_InjectionRecord_s::rollovers);
        }

        else
          injection_idx = InterlockedIncrement (&SK_InjectionRecord_s::count);
      }

      local_record =
        injection_idx - 1;

                __SK_InjectionHistory_ids    [local_record] = GetCurrentProcessId ();
      _time64 (&__SK_InjectionHistory_inject [local_record]);

      wchar_t wszName [MAX_PATH] = { 0 };

      wcsncpy (wszName, SK_GetModuleFullName (GetModuleHandle (nullptr)).c_str (),
                    MAX_PATH - 1);
      PathStripPath (wszName);

      wcsncpy (&__SK_InjectionHistory_name [local_record * MAX_PATH], wszName, MAX_PATH - 1);

      // Hold a reference so that removing the CBT hook doesn't crash the software
      GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                            (LPCWSTR)&SK_Inject_AcquireProcess,
                               &hModHookInstance );

      break;
    }
  }
}

bool
SK_Inject_InvadingProcess (DWORD dwThreadId)
{
  for (volatile LONG& g_sHookedPID : g_sHookedPIDs)
  {
    if (ReadAcquire (&g_sHookedPID) == static_cast <LONG> (dwThreadId))
      return true;
  }

  return false;
}

const HHOOK
SKX_GetCBTHook (void)
{
  return
    static_cast <HHOOK> (
      ReadPointerAcquire ( reinterpret_cast <volatile PVOID *> (
                                 const_cast <         HHOOK *> (&g_CBTHook.hHookCBT)
                           )
                         )
    );
}

void
SKX_WaitForCBTHookShutdown (void)
{
  HANDLE hShutdown  = INVALID_HANDLE_VALUE;
  HANDLE hHookOwner = 
    OpenProcess ( PROCESS_DUP_HANDLE, FALSE, g_CBTHook.dwHookPID );

  if (hHookOwner != nullptr)
  {
    BOOL success =
      DuplicateHandle ( hHookOwner,  g_CBTHook.hShutdownEvent,
                        GetCurrentProcess (), &hShutdown,
                        0x00, TRUE, DUPLICATE_SAME_ACCESS );

    CloseHandle (hHookOwner);

    if (success && hShutdown != INVALID_HANDLE_VALUE)
    {
      SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_IDLE);

      WaitForSingleObjectEx (hShutdown, INFINITE, FALSE);

      CloseHandle (hShutdown);
    }
  }
}

LRESULT
CALLBACK
CBTProc ( _In_ int    nCode,
          _In_ WPARAM wParam,
          _In_ LPARAM lParam )
{
  LRESULT ret = CallNextHookEx (SKX_GetCBTHook (), nCode, wParam, lParam);

  static volatile LONG lHookIters = 0L;

  // Don't create that thread more than once, but don't bother with a complete
  //   critical section.
  if (InterlockedIncrement (&lHookIters) == 1L)
  {
    if ( GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                (LPCWSTR)SK_GetDLL (),
                                  &hModHookInstance ) )
    {
      // Get and keep a reference to this DLL if this is the first time we are injecting.
      HANDLE hThread =
        CreateThread ( nullptr, 0,
             [](LPVOID user) ->
               DWORD
                 {
                   SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_IDLE);

                   SKX_WaitForCBTHookShutdown ();

                   CloseHandle (GetCurrentThread ());

                   FreeLibraryAndExitThread (static_cast <HMODULE> (user), 0x0);

                   return 0;
                 },
               (hModHookInstance),
             0x00,
           nullptr
        );

      // Closes itself
      DBG_UNREFERENCED_LOCAL_VARIABLE (hThread);
    }

    else
      InterlockedExchange (&lHookIters, 0);
  }

  return ret;
}

BOOL
SK_TerminatePID ( DWORD dwProcessId, UINT uExitCode )
{
  const DWORD dwDesiredAccess = PROCESS_TERMINATE;
  const BOOL  bInheritHandle  = FALSE;

  HANDLE hProcess =
    OpenProcess ( dwDesiredAccess, bInheritHandle, dwProcessId );

  if (hProcess == nullptr)
    return FALSE;
  
  BOOL result =
    TerminateProcess (hProcess, uExitCode);
  
  CloseHandle (hProcess);
  
  return result;
}


void
__stdcall
SKX_InstallCBTHook (void)
{
  // Nothing to do here, move along.
  if (SKX_GetCBTHook () != nullptr)
    return;

  ZeroMemory (whitelist_patterns, sizeof (whitelist_patterns));
              whitelist_count = 0;

  HMODULE hMod = nullptr;

  GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      SK_RunLHIfBitness ( 32, L"SpecialK32.dll",
                              L"SpecialK64.dll" ),
                          (HMODULE *) &hMod );

  if (hMod == SK_GetDLL () && hMod != nullptr)
  {
    SK_Inject_InitShutdownEvent ();

    // Shell hooks don't work very well, they run into problems with
    //   hooking XInput -- CBT is more reliable, but slower.
    g_CBTHook.hHookCBT  =
      SetWindowsHookEx (WH_CBT, CBTProc, hMod, 0);
  }
}


void
__stdcall
SKX_RemoveCBTHook (void)
{
START_OVER:
  if (g_CBTHook.hShutdownEvent != nullptr)
    SetEvent (g_CBTHook.hShutdownEvent);

  HHOOK hHookOrig = SKX_GetCBTHook ();

  if ( InterlockedCompareExchangePointer ( reinterpret_cast <LPVOID *> (
                                                 const_cast < HHOOK *> (&g_CBTHook.hHookCBT)
                                           ),
                                             nullptr,
                                               hHookOrig )
     )
  {
    if (UnhookWindowsHookEx (hHookOrig))
    {
      SecureZeroMemory (whitelist_patterns, sizeof (whitelist_patterns));
                        whitelist_count = 0;

      InterlockedExchange (&__SK_HookContextOwner, FALSE);

                   g_CBTHook.dwHookPID = 0x0;
      CloseHandle (g_CBTHook.hShutdownEvent);
    }

    else
    {
      // Couldn't remove the hook, so atomically set it back to non-NULL
      InterlockedExchangePointer ( reinterpret_cast <LPVOID *> (
                                         const_cast < HHOOK *> (&g_CBTHook.hHookCBT)
                                   ), hHookOrig );
    }
  }

  while (ReadPointerAcquire ((PVOID *)&g_CBTHook.hHookCBT) != nullptr)
    goto START_OVER;
}

bool
__stdcall
SKX_IsHookingCBT (void)
{
  return ReadPointerAcquire ( reinterpret_cast <LPVOID *> (
                                    const_cast < HHOOK *> (&g_CBTHook.hHookCBT)
                              )
                            ) != nullptr;
}


// Useful for managing injection of the 32-bit DLL from a 64-bit application
//   or visa versa.
void
CALLBACK
RunDLL_InjectionManager ( HWND  hwnd,        HINSTANCE hInst,
                          LPSTR lpszCmdLine, int       nCmdShow )
{
  UNREFERENCED_PARAMETER (hInst);
  UNREFERENCED_PARAMETER (hwnd);

  if (StrStrA (lpszCmdLine, "Install") && (! SKX_IsHookingCBT ()))
  {
    SKX_InstallCBTHook ();

    if (SKX_IsHookingCBT ())
    {
      const char* szPIDFile =
        SK_RunLHIfBitness ( 32, "SpecialK32.pid",
                                "SpecialK64.pid" );

      FILE* fPID =
        fopen (szPIDFile, "w");

      if (fPID != nullptr)
      {
        fprintf (fPID, "%lu\n", GetCurrentProcessId ());
        fclose  (fPID);

        HANDLE hThread = 
        CreateThread ( nullptr, 0,
         [](LPVOID user) ->
           DWORD
             {
               SKX_WaitForCBTHookShutdown ();

               while ( ReadAcquire (&__SK_DLL_Attached) || (! SK_IsHostAppSKIM ()))
                 SleepEx (15UL, TRUE);

               CloseHandle (GetCurrentThread ());

               if (PtrToInt (user) != -128)
               {
                 ExitProcess (0x00);
               }

               return 0;
             },
             UIntToPtr (nCmdShow),
           0x00,
         nullptr );

        // Closes itself
        DBG_UNREFERENCED_LOCAL_VARIABLE (hThread);

        SleepEx (INFINITE, TRUE);
      }
    }
  }

  else if (StrStrA (lpszCmdLine, "Remove"))
  {
    SKX_RemoveCBTHook ();

    const char* szPIDFile =
      SK_RunLHIfBitness ( 32, "SpecialK32.pid",
                              "SpecialK64.pid" );

    FILE* fPID =
      fopen (szPIDFile, "r");

    if (fPID != nullptr)
    {
                      DWORD dwPID = 0;
      fscanf (fPID, "%lu", &dwPID);
      fclose (fPID);

      if (dwPID == GetCurrentProcessId () || SK_TerminatePID (dwPID, 0x00))
      {
        DeleteFileA (szPIDFile);
      }
    }
  }

  if (nCmdShow != -128)
   ExitProcess (0x00);
}


void
SK_Inject_EnableCentralizedConfig (void)
{
  wchar_t wszOut [MAX_PATH * 2] = { };

  lstrcatW (wszOut, SK_GetHostPath ());
  lstrcatW (wszOut, LR"(\SpecialK.central)");

  FILE* fOut = _wfopen (wszOut, L"w");
               fputws (L" ", fOut);
                     fclose (fOut);

  config.system.central_repository = true;

  SK_EstablishRootPath ();

  switch (SK_GetCurrentRenderBackend ().api)
  {
    case SK_RenderAPI::D3D9:
    case SK_RenderAPI::D3D9Ex:
      SK_SaveConfig (L"d3d9");
      break;

#ifndef _WIN64
  case SK_RenderAPI::D3D8On11:
    SK_SaveConfig (L"d3d8");
    break;

  case SK_RenderAPI::DDrawOn11:
    SK_SaveConfig (L"ddraw");
    break;
#endif

    case SK_RenderAPI::D3D10:
    case SK_RenderAPI::D3D11:
#ifdef _WIN64
    case SK_RenderAPI::D3D12:
#endif
    {
      SK_SaveConfig (L"dxgi");
    } break;

    case SK_RenderAPI::OpenGL:
      SK_SaveConfig (L"OpenGL32");
      break;

    //case SK_RenderAPI::Vulkan:
      //lstrcatW (wszOut, L"\\vk-1.dll");
      //break;
  }
}


bool
SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE role)
{
  wchar_t wszIn  [MAX_PATH * 2] = { };
  lstrcatW (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());

  wchar_t wszOut [MAX_PATH * 2] = { };
  lstrcatW (wszOut, SK_GetHostPath ());

  switch (role)
  {
    case DLL_ROLE::D3D9:
      lstrcatW (wszOut, L"\\d3d9.dll");
      break;

    case DLL_ROLE::DXGI:
      lstrcatW (wszOut, L"\\dxgi.dll");
      break;

    case DLL_ROLE::DInput8:
      lstrcatW (wszOut, L"\\dinput8.dll");
      break;

    case DLL_ROLE::D3D11:
    case DLL_ROLE::D3D11_CASE:
      lstrcatW (wszOut, L"\\d3d11.dll");
      break;

    case DLL_ROLE::OpenGL:
      lstrcatW (wszOut, L"\\OpenGL32.dll");
      break;

#ifndef _WIN64
    case DLL_ROLE::DDraw:
      lstrcatW (wszOut, L"\\ddraw.dll");
      break;

    case DLL_ROLE::D3D8:
      lstrcatW (wszOut, L"\\d3d8.dll");
      break;
#endif

    //case SK_RenderAPI::Vulkan:
      //lstrcatW (wszOut, L"\\vk-1.dll");
      //break;
  }


  //std::queue <DWORD> suspended =
  //  SK_SuspendAllOtherThreads ();
  //
  //extern volatile LONG   SK_bypass_dialog_active;
  //InterlockedIncrement (&SK_bypass_dialog_active);
  //
  //int mb_ret = 
  //       SK_MessageBox ( L"Link the Installed Wrapper to the Global DLL?\r\n"
  //                       L"\r\n"
  //                       L"Linked installs allow you to update wrapped games the same way "
  //                       L"as global injection, but require administrator privileges to setup.",
  //                         L"Perform a Linked Wrapper Install?",
  //                           MB_YESNO | MB_ICONQUESTION
  //                     );
  //
  //InterlockedIncrement (&SK_bypass_dialog_active);
  //
  //SK_ResumeThreads (suspended);

  //if ( mb_ret == IDYES )
  //{
  //  wchar_t   wszCmd [MAX_PATH * 3] = { };
  //  swprintf (wszCmd, L"/c mklink \"%s\" \"%s\"", wszOut, wszIn);
  //
  //  ShellExecuteW ( GetActiveWindow (),
  //                    L"runas",
  //                      L"cmd.exe",
  //                        wszCmd,
  //                          nullptr,
  //                            SW_HIDE );
  //
  //  SK_Inject_EnableCentralizedConfig ();
  //
  //  return true;
  //}


  if (CopyFile (wszIn, wszOut, TRUE))
  {
    SK_Inject_EnableCentralizedConfig ();

    *wszOut = L'\0';
    *wszIn  = L'\0';

    lstrcatW (wszOut, SK_GetHostPath ());

    SK_RunLHIfBitness (64, PathAppendW (wszIn,  L"SpecialK64.pdb"),
                           PathAppendW (wszIn,  L"SpecialK32.pdb") );
    SK_RunLHIfBitness (64, PathAppendW (wszOut, L"SpecialK64.pdb"),
                           PathAppendW (wszOut, L"SpecialK32.pdb") );

    if (! CopyFileW (wszIn, wszOut, TRUE))
      ReplaceFileW (wszOut, wszIn, nullptr, 0x00, nullptr, nullptr);

    *wszIn = L'\0';

    std::wstring ver_dir   =
      SK_FormatStringW (LR"(%s\Version)", SK_GetConfigPath ());

    const DWORD dwAttribs =
      GetFileAttributesW (ver_dir.c_str ());

    lstrcatW             (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
    PathRemoveFileSpecW  (wszIn);
    PathAppendW          (wszIn, LR"(Version\installed.ini)");

    if ( GetFileAttributesW (wszIn) != INVALID_FILE_ATTRIBUTES &&
         ( (dwAttribs != INVALID_FILE_ATTRIBUTES) ||
           CreateDirectoryW (ver_dir.c_str (), nullptr) ) )
    {
      *wszOut = L'\0';

      lstrcatW    (wszOut, ver_dir.c_str ());
      PathAppendW (wszOut, L"installed.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);

      *wszIn = L'\0';

      lstrcatW             (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
      PathRemoveFileSpecW  (wszIn);
      PathAppendW          (wszIn, LR"(Version\repository.ini)");

      *wszOut = L'\0';

      lstrcatW    (wszOut, ver_dir.c_str ());
      PathAppendW (wszOut, L"repository.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);
    }

    return true;
  }

  return false;
}

bool
SK_Inject_SwitchToRenderWrapper (void)
{
  wchar_t   wszIn  [MAX_PATH * 2] = { };
  wchar_t   wszOut [MAX_PATH * 2] = { };

  lstrcatW (wszIn,  SK_GetModuleFullName (SK_GetDLL ()).c_str ());
  lstrcatW (wszOut, SK_GetHostPath       (                     ));

  switch (SK_GetCurrentRenderBackend ().api)
  {
    case SK_RenderAPI::D3D9:
    case SK_RenderAPI::D3D9Ex:
      lstrcatW (wszOut, L"\\d3d9.dll");
      break;

#ifndef _WIN64
    case SK_RenderAPI::D3D8On11:
      lstrcatW (wszOut, L"\\d3d8.dll");
      break;

    case SK_RenderAPI::DDrawOn11:
      lstrcatW (wszOut, L"\\ddraw.dll");
      break;
#endif

    case SK_RenderAPI::D3D10:
    case SK_RenderAPI::D3D11:
#ifdef _WIN64
    case SK_RenderAPI::D3D12:
#endif
    {
      lstrcatW (wszOut, L"\\dxgi.dll");
    } break;

    case SK_RenderAPI::OpenGL:
      lstrcatW (wszOut, L"\\OpenGL32.dll");
      break;

    //case SK_RenderAPI::Vulkan:
      //lstrcatW (wszOut, L"\\vk-1.dll");
      //break;
  }


  //std::queue <DWORD> suspended =
  //  SK_SuspendAllOtherThreads ();
  //
  //extern volatile LONG   SK_bypass_dialog_active;
  //InterlockedIncrement (&SK_bypass_dialog_active);
  //
  //int mb_ret = 
  //       SK_MessageBox ( L"Link the Installed Wrapper to the Global DLL?\r\n"
  //                       L"\r\n"
  //                       L"Linked installs allow you to update wrapped games the same way "
  //                       L"as global injection, but require administrator privileges to setup.",
  //                         L"Perform a Linked Wrapper Install?",
  //                           MB_YESNO | MB_ICONQUESTION
  //                     );
  //
  //InterlockedIncrement (&SK_bypass_dialog_active);
  //
  //SK_ResumeThreads (suspended);
  //
  //if ( mb_ret == IDYES )
  //{
  //  wchar_t   wszCmd [MAX_PATH * 3] = { };
  //  swprintf (wszCmd, L"/c mklink \"%s\" \"%s\"", wszOut, wszIn);
  //
  //  ShellExecuteW ( GetActiveWindow (),
  //                    L"runas",
  //                      L"cmd.exe",
  //                        wszCmd,
  //                          nullptr,
  //                            SW_HIDE );
  //
  //  SK_Inject_EnableCentralizedConfig ();
  //
  //  return true;
  //}


  if (CopyFile (wszIn, wszOut, TRUE))
  {
    SK_Inject_EnableCentralizedConfig ();

    *wszOut = L'\0';
    *wszIn  = L'\0';

    lstrcatW (wszOut, SK_GetHostPath ());

    const wchar_t* wszPDBFile =
      SK_RunLHIfBitness (64, L"SpecialK64.pdb",
                             L"SpecialK32.pdb");
    lstrcatW (wszIn,  wszPDBFile);
    lstrcatW (wszOut, L"\\");
    lstrcatW (wszOut, wszPDBFile);

    if (! CopyFileW (wszIn, wszOut, TRUE))
      ReplaceFileW (wszOut, wszIn, nullptr, 0x00, nullptr, nullptr);

    *wszIn = L'\0';

    std::wstring ver_dir   =
      SK_FormatStringW (LR"(%s\Version)", SK_GetConfigPath ());

    const DWORD dwAttribs =
      GetFileAttributesW (ver_dir.c_str ());

    lstrcatW             (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
    PathRemoveFileSpecW  (wszIn);
    PathAppendW          (wszIn, LR"(Version\installed.ini)");

    if ( GetFileAttributesW (wszIn) != INVALID_FILE_ATTRIBUTES &&
         ( (dwAttribs != INVALID_FILE_ATTRIBUTES) ||
           CreateDirectoryW (ver_dir.c_str (), nullptr) ) )
    {
      *wszOut = L'\0';

      lstrcatW    (wszOut, ver_dir.c_str ());
      PathAppendW (wszOut, L"installed.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);

      *wszIn = L'\0';

      lstrcatW             (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
      PathRemoveFileSpecW  (wszIn);
      PathAppendW          (wszIn, LR"(Version\repository.ini)");

      *wszOut = L'\0';

      lstrcatW    (wszOut, ver_dir.c_str ());
      PathAppendW (wszOut, L"repository.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);
    }

    return true;
  }

  return false;
}

bool
SK_Inject_SwitchToGlobalInjector (void)
{
  config.system.central_repository = true;
  SK_EstablishRootPath ();

  wchar_t wszOut  [MAX_PATH * 2] = { };
  wchar_t wszTemp [MAX_PATH * 2] = { };

  lstrcatW (wszOut, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
  GetTempFileNameW (SK_GetHostPath (), L"SKI", timeGetTime (), wszTemp);

  MoveFileW (wszOut, wszTemp);

  SK_SaveConfig (L"SpecialK");

  return true;
}


#if 0
bool SK_Inject_JournalRecord (HMODULE hModule)
{
  return false;
}
#endif



#include <TlHelp32.h>
#include <Shlwapi.h>

bool
SK_ExitRemoteProcess (const wchar_t* wszProcName, UINT uExitCode = 0x0)
{
  UNREFERENCED_PARAMETER (uExitCode);


  PROCESSENTRY32 pe32      = { };
  CHandle hProcSnap          (
    CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0) );

  if (hProcSnap == INVALID_HANDLE_VALUE)
    return false;

  pe32.dwSize = sizeof PROCESSENTRY32;

  if (! Process32First (hProcSnap, &pe32))
  {
    return false;
  }

  do
  {
    if (StrStrIW (wszProcName, pe32.szExeFile))
    {
      window_t win = SK_FindRootWindow (pe32.th32ProcessID);

      SendMessage (win.root, WM_USER + 0x123, 0x00, 0x00);

      return true;
    }
  } while (Process32Next (hProcSnap, &pe32));

  return false;
}


void
SK_Inject_Stop (void)
{
  wchar_t wszCurrentDir [MAX_PATH * 2] = { };
  wchar_t wszWOW64      [MAX_PATH + 2] = { };
  wchar_t wszSys32      [MAX_PATH + 2] = { };

  GetCurrentDirectoryW  (MAX_PATH * 2 - 1, wszCurrentDir);
  SetCurrentDirectoryW  (SK_SYS_GetInstallPath ().c_str ());
  GetSystemDirectoryW   (wszSys32, MAX_PATH);

  SK_RunLHIfBitness ( 32, GetSystemDirectoryW      (wszWOW64, MAX_PATH),
                          GetSystemWow64DirectoryW (wszWOW64, MAX_PATH) );

  //SK_ExitRemoteProcess (L"SKIM64.exe", 0x00);

  if (GetFileAttributes (L"SKIM64.exe") == INVALID_FILE_ATTRIBUTES)
  {
    PathAppendW   ( wszWOW64, L"rundll32.exe");
    ShellExecuteA ( nullptr,
                      "open", SK_WideCharToUTF8 (wszWOW64).c_str (),
                      "SpecialK32.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );

    PathAppendW   ( wszSys32, L"rundll32.exe");
    ShellExecuteA ( nullptr,
                      "open", SK_WideCharToUTF8 (wszSys32).c_str (),
                      "SpecialK64.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );
  }

  else
  {
    HWND hWndExisting =
      FindWindow (L"SKIM_Frontend", nullptr);

    // Best-case, SKIM restarts itself
    if (hWndExisting)
    {
      SendMessage (hWndExisting, WM_USER + 0x122, 0, 0);
      SleepEx     (100UL, FALSE);
    }

    // Worst-case, we do this manually and confuse Steam
    else
    {
      ShellExecuteA        ( nullptr,
                               "open", "SKIM64.exe",
                               "-Inject", SK_WideCharToUTF8 (SK_SYS_GetInstallPath ()).c_str (),
                                 SW_FORCEMINIMIZE );
      SleepEx              ( 100UL, FALSE);
      SK_ExitRemoteProcess ( L"SKIM64.exe", 0x00);
    }
  }

  SetCurrentDirectoryW (wszCurrentDir);
}

void
SK_Inject_Start (void)
{
  wchar_t wszCurrentDir [MAX_PATH * 2] = { };
  wchar_t wszWOW64      [MAX_PATH + 2] = { };
  wchar_t wszSys32      [MAX_PATH + 2] = { };

  GetCurrentDirectoryW  (MAX_PATH * 2 - 1, wszCurrentDir);
  SetCurrentDirectoryW  (SK_SYS_GetInstallPath ().c_str ());
  GetSystemDirectoryW   (wszSys32, MAX_PATH);

  SK_RunLHIfBitness ( 32, GetSystemDirectoryW      (wszWOW64, MAX_PATH),
                          GetSystemWow64DirectoryW (wszWOW64, MAX_PATH) );

  if (GetFileAttributes (L"SKIM64.exe") == INVALID_FILE_ATTRIBUTES)
  {
    if (SKX_IsHookingCBT ())
    {
      RunDLL_InjectionManager ( nullptr, nullptr,
                                "Remove", -128 );
    }

    PathAppendW   ( wszSys32, L"rundll32.exe");
    ShellExecuteA ( nullptr,
                      "open", SK_WideCharToUTF8 (wszSys32).c_str (),
                      "SpecialK64.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE );

    PathAppendW   ( wszWOW64, L"rundll32.exe");
    ShellExecuteA ( nullptr,
                      "open", SK_WideCharToUTF8 (wszWOW64).c_str (),
                      "SpecialK32.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE );
  }

  else
  {
    if (SKX_IsHookingCBT ())
    {
      RunDLL_InjectionManager ( nullptr, nullptr,
                                "Remove", -128 );
    }

    HWND hWndExisting =
      FindWindow (L"SKIM_Frontend", nullptr);

    // Best-case, SKIM restarts itself
    if (hWndExisting)
    {
      SendMessage (hWndExisting, WM_USER + 0x125, 0, 0);
      SleepEx     (100, FALSE);
      SendMessage (hWndExisting, WM_USER + 0x124, 0, 0);
    }

    // Worst-case, we do this manually and confuse Steam
    else
    {
      ShellExecuteA ( nullptr,
                        "open", "SKIM64.exe", "+Inject",
                        SK_WideCharToUTF8 (SK_SYS_GetInstallPath ()).c_str (),
                          SW_FORCEMINIMIZE );
    }
  }

  SetCurrentDirectoryW (wszCurrentDir);
}


size_t
__stdcall
SKX_GetInjectedPIDs ( DWORD* pdwList,
                      size_t capacity )
{
  DWORD*  pdwListIter = pdwList;
  SSIZE_T i           = 0;

  SK_Inject_ValidateProcesses ();

  for (volatile LONG& g_sHookedPID : g_sHookedPIDs)
  {
    if (ReadAcquire (&g_sHookedPID) != 0)
    {
      if (i < (SSIZE_T)capacity - 1)
      {
        if (pdwListIter != nullptr)
        {
          *pdwListIter = ReadAcquire (&g_sHookedPID);
           pdwListIter++;
        }
      }

      ++i;
    }
  }

  if (pdwListIter != nullptr)
     *pdwListIter  = 0;

  return i;
}


#include <fstream>
#include <regex>

bool
SK_Inject_TestUserWhitelist (const wchar_t* wszExecutable)
{
  if (whitelist_count == 0)
  {
    std::wifstream whitelist_file (
      std::wstring (SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\Global\whitelist.ini)")
    );

    if (whitelist_file.is_open ())
    {
      std::wstring line;

      while (whitelist_file.good ())
      {
        std::getline (whitelist_file, line);

        // Skip blank lines, since they would match everything....
        for (const auto& it : line)
        {
          if (iswalpha (it))
          {
            wcsncpy ( (wchar_t *)&whitelist_patterns [MAX_PATH * whitelist_count++],
                        line.c_str (),
                          MAX_PATH - 1 );
            break;
          }
        }
      }

      if (whitelist_count == 0)
        whitelist_count = -1;
    }

    else
      whitelist_count = -1;
  }


  for ( int i = 0; i < whitelist_count; i++ )
  {
    std::wregex regexp (
      (wchar_t *)&whitelist_patterns [MAX_PATH * i],
        std::regex_constants::icase
    );

    if (std::regex_search (wszExecutable, regexp))
    {
      return true;
    }
  }

  return false;
}

bool
SK_Inject_TestWhitelists (const wchar_t* wszExecutable)
{
  // Sort of a temporary hack for important games that I support that are
  //   sold on alternative stores to Steam.
  if (StrStrIW (wszExecutable, L"ffxv.exe"))
    return true;


  return SK_Inject_TestUserWhitelist (wszExecutable);
}


//bool
//SK_Inject_TestUserBlacklist (const wchar_t* wszExecutable)
//{
//  if (blacklist.count == 0)
//  {
//    std::wifstream blacklist_file (std::wstring (SK_GetDocumentsDir () + L"\\My Mods\\SpecialK\\Global\\blacklist.ini"));
//
//    if (blacklist_file.is_open ())
//    {
//      std::wstring line;
//
//      while (blacklist_file.good ())
//      {
//        std::getline (blacklist_file, line);
//
//        // Skip blank lines, since they would match everything....
//        for (const auto& it : line)
//        {
//          if (iswalpha (it))
//          {
//            wcsncpy (blacklist.patterns [blacklist.count++], line.c_str (), MAX_PATH);
//            break;
//          }
//        }
//      }
//    }
//
//    else
//      blacklist.count = -1;
//  }
//
//  for ( int i = 0; i < blacklist.count; i++ )
//  {
//    std::wregex regexp (blacklist.patterns [i], std::regex_constants::icase);
//
//    if (std::regex_search (wszExecutable, regexp))
//    {
//      return true;
//    }
//  }
//
//  return false;
//}

bool
SK_Inject_IsAdminSupported (void)
{
  return g_CBTHook.bAdmin;
}