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
#define _CRT_SECURE_NO_WARNINGS

#include <SpecialK/injection/injection.h>
#include <SpecialK/window.h>
#include <SpecialK/core.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>

#include <set>
#include <Shlwapi.h>
#include <time.h>


#pragma data_seg (".SK_Hooks")
volatile HHOOK          g_hHookCBT    = nullptr;
volatile HANDLE         g_hShutdown   = 0;

// TODO: Replace with interlocked linked-list instead of obliterating cache
//         every time two processes try to walk this thing simultaneously.
//
static volatile DWORD g_sHookedPIDs [MAX_INJECTED_PROCS]        = { };

static          SK_InjectionRecord_s __SK_InjectionHistory
                                    [MAX_INJECTED_PROC_HISTORY];

       volatile LONG SK_InjectionRecord_s::count               = 0L;
       volatile LONG SK_InjectionRecord_s::rollovers           = 0L;

#pragma data_seg ()
#pragma comment  (linker, "/section:.SK_Hooks,RWS")


extern volatile ULONG   __SK_DLL_Attached;
extern volatile ULONG   __SK_HookContextOwner;
                HMODULE hModHookInstance = NULL;



SK_InjectionRecord_s*
SK_Inject_GetRecord (int idx)
{
  return &__SK_InjectionHistory [idx];
}


SK_InjectionRecord_s* local_record = nullptr;


void
SK_Inject_ValidateProcesses (void)
{
  for (int i = 0; i < MAX_INJECTED_PROCS; i++)
  {
    HANDLE hProc =
      OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE, 
                      InterlockedExchangeAdd (&g_sHookedPIDs [i], 0) );

    if (hProc == NULL)
    {
      InterlockedExchange (&g_sHookedPIDs [i], 0);
    }

    else
    {
      DWORD dwExitCode = STILL_ACTIVE;

      GetExitCodeProcess (hProc, &dwExitCode);

      if (dwExitCode != STILL_ACTIVE)
        InterlockedExchange (&g_sHookedPIDs [i], 0);

      CloseHandle (hProc);
    }
  }
}

void
SK_Inject_ReleaseProcess (void)
{
  if (! SK_IsInjected ())
    return;

  for (int i = 0; i < MAX_INJECTED_PROCS; i++)
  {
    InterlockedCompareExchange (&g_sHookedPIDs [i], 0, GetCurrentProcessId ());
  }

  if (local_record != nullptr)
  {
    _time64 (&local_record->process.eject);

    local_record->render.api    = SK_GetCurrentRenderBackend ().api;
    local_record->render.frames = SK_GetFramesDrawn ();

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

  for (int i = 0; i < MAX_INJECTED_PROCS; i++)
  {
    if (! InterlockedCompareExchange (&g_sHookedPIDs [i], GetCurrentProcessId (), 0))
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
        &__SK_InjectionHistory [injection_idx - 1];

                local_record->process.id = GetCurrentProcessId ();
      _time64 (&local_record->process.inject);
      wcsncpy ( local_record->process.name,
                  SK_GetModuleFullName (GetModuleHandle (nullptr)).c_str (),
                    MAX_PATH );

      PathStripPath (local_record->process.name);

      // Hold a reference so that removing the CBT hook doesn't crash the software
      GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_PIN,
                            (LPCWSTR)&SK_Inject_AcquireProcess,
                               &hModHookInstance );

      break;
    }
  }
}

bool
SK_Inject_InvadingProcess (DWORD dwThreadId)
{
  for (int i = 0; i < MAX_INJECTED_PROCS; i++)
  {
    if (InterlockedExchangeAdd (&g_sHookedPIDs [i], 0) == dwThreadId)
      return true;
  }

  return false;
}



LRESULT
CALLBACK
CBTProc ( _In_ int    nCode,
          _In_ WPARAM wParam,
          _In_ LPARAM lParam )
{
  if (nCode < 0)
    return CallNextHookEx (g_hHookCBT, nCode, wParam, lParam);


  if (hModHookInstance == NULL && g_hHookCBT)
  {
    static volatile LONG lHookIters = 0L;

    // Don't create that thread more than once, but don't bother with a complete
    //   critical section.
    if (InterlockedAdd (&lHookIters, 1L) > 1L)
      return CallNextHookEx (g_hHookCBT, nCode, wParam, lParam);

    GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
#ifdef _WIN64
                          L"SpecialK64.dll",
#else
                          L"SpecialK32.dll",
#endif
                            (HMODULE *) &hModHookInstance );

    // Get and keep a reference to this DLL if this is the first time we are injecting.
    CreateThread ( nullptr, 0,
         [](LPVOID user) ->
           DWORD
             {
               UNREFERENCED_PARAMETER (user);

               if (g_hShutdown != 0)
                 WaitForSingleObject (g_hShutdown, INFINITE);

               hModHookInstance = NULL;

               CloseHandle (GetCurrentThread ());

               return 0;
             },
           nullptr,
         0x00,
       nullptr
    );
  }


  return CallNextHookEx (g_hHookCBT, nCode, wParam, lParam);
}


BOOL
SK_TerminatePID ( DWORD dwProcessId, UINT uExitCode )
{
  DWORD dwDesiredAccess = PROCESS_TERMINATE;
  BOOL  bInheritHandle  = FALSE;

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
  if (g_hHookCBT != nullptr)
    return;

  HMODULE hMod = 0;

  GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
#ifdef _WIN64
                          L"SpecialK64.dll",
#else
                          L"SpecialK32.dll",
#endif
                          (HMODULE *) &hMod );

  extern HMODULE
  __stdcall
  SK_GetDLL (void);

  if (hMod == SK_GetDLL ())
  {
    if (g_hShutdown == 0)
    {
#ifdef _WIN64
      g_hShutdown = CreateEvent (nullptr, TRUE, FALSE, L"SpecialK64_Reset");
#else
      g_hShutdown = CreateEvent (nullptr, TRUE, FALSE, L"SpecialK32_Reset");
#endif
    }

    // Shell hooks don't work very well, they run into problems with
    //   hooking XInput -- CBT is more reliable, but slower.
    //
    //  >>  ** Thank you GeForce Experience :-\
    //
    g_hHookCBT =
      SetWindowsHookEx (WH_CBT, CBTProc, hMod, 0);

    if (g_hHookCBT != 0)
    {
      InterlockedExchange (&__SK_HookContextOwner, TRUE);
    }
  }
}


void
__stdcall
SKX_RemoveCBTHook (void)
{
  if (g_hShutdown != 0)
    SetEvent (g_hShutdown);

  if (g_hHookCBT)
  {
    if (UnhookWindowsHookEx (g_hHookCBT))
    {
      InterlockedExchange (&__SK_HookContextOwner, FALSE);
      g_hHookCBT = nullptr;
    }
  }
}

bool
__stdcall
SKX_IsHookingCBT (void)
{
  return (g_hHookCBT != nullptr);
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
#ifndef _WIN64
      FILE* fPID = fopen ("SpecialK32.pid", "w");
#else
      FILE* fPID = fopen ("SpecialK64.pid", "w");
#endif

      if (fPID)
      {
        fprintf (fPID, "%lu\n", GetCurrentProcessId ());
        fclose  (fPID);

        CreateThread ( nullptr, 0,
         [](LPVOID user) ->
           DWORD
             {
               if (g_hShutdown != 0)
                 WaitForSingleObject (g_hShutdown, INFINITE);

               while (  InterlockedCompareExchangeAcquire (
                        &__SK_DLL_Attached,
                          FALSE,
                            FALSE ) || (! SK_IsHostAppSKIM ()))
                 SleepEx (250UL, FALSE);


               if (PtrToInt (user) != -128)
                 ExitProcess (0x00);
               else
                 CloseHandle (GetCurrentThread ());

               return 0;
             },
             UIntToPtr (nCmdShow),
           0x00,
         nullptr );

        SleepEx (INFINITE, FALSE);
      }
    }
  }

  else if (StrStrA (lpszCmdLine, "Remove"))
  {
    SKX_RemoveCBTHook ();

#ifndef _WIN64
    FILE* fPID = fopen ("SpecialK32.pid", "r");
#else
    FILE* fPID = fopen ("SpecialK64.pid", "r");
#endif

    if (fPID != nullptr)
    {
                      DWORD dwPID = 0;
      fscanf (fPID, "%lu", &dwPID);
      fclose (fPID);

      if (dwPID == GetCurrentProcessId () || SK_TerminatePID (dwPID, 0x00))
      {
#ifndef _WIN64
        DeleteFileA ("SpecialK32.pid");
#else
        DeleteFileA ("SpecialK64.pid");
#endif
      }
    }
  }

  if (nCmdShow != -128)
   ExitProcess (0x00);
}




#include <SpecialK/utility.h>
#include <SpecialK/render_backend.h>

extern void
__stdcall
SK_EstablishRootPath (void);

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

  if (CopyFile (wszIn, wszOut, TRUE))
  {
    *wszOut = L'\0';

    lstrcatW (wszOut, SK_GetHostPath ());
    lstrcatW (wszOut, L"\\SpecialK.central");

    FILE* fOut = _wfopen (wszOut, L"w");
                 fputws (L" ", fOut);
                       fclose (fOut);

    config.system.central_repository = true;

    *wszOut = L'\0';
    *wszIn  = L'\0';

    lstrcatW (wszOut, SK_GetHostPath ());

#ifdef _WIN64
    lstrcatW (wszIn,  L"SpecialK64.pdb");
    lstrcatW (wszOut, L"\\SpecialK64.pdb");
#else
    lstrcatW (wszIn,  L"SpecialK32.pdb");
    lstrcatW (wszOut, L"\\SpecialK32.pdb");
#endif

    if (! CopyFileW (wszIn, wszOut, TRUE))
      ReplaceFileW (wszOut, wszIn, nullptr, 0x00, nullptr, nullptr);

    SK_EstablishRootPath ();

    switch (role)
    {
      case DLL_ROLE::D3D9:
        SK_SaveConfig (L"d3d9");
        break;

      case DLL_ROLE::DXGI:
        SK_SaveConfig (L"dxgi");
        break;

      case DLL_ROLE::OpenGL:
        SK_SaveConfig (L"OpenGL32");
        break;

#ifndef _WIN64
      case DLL_ROLE::DDraw:
        SK_SaveConfig (L"ddraw");
        break;

      case DLL_ROLE::D3D8:
        SK_SaveConfig (L"d3d8");
        break;
#endif

      //case SK_RenderAPI::Vulkan:
        //lstrcatW (wszOut, L"\\vk-1.dll");
        //break;
    }

    *wszIn = L'\0';

    std::string ver_dir   =
      SK_FormatString ("%ws\\Version", SK_GetConfigPath ());

    const DWORD dwAttribs =
      GetFileAttributesA (ver_dir.c_str ());

    if ( (dwAttribs != INVALID_FILE_ATTRIBUTES) ||
          CreateDirectoryA (ver_dir.c_str (), nullptr) )
    {
      lstrcatW (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
      PathRemoveFileSpecW  (wszIn);

      lstrcatW (wszIn, L"\\Version\\installed.ini");

      *wszOut = L'\0';

      lstrcatW (wszOut, SK_UTF8ToWideChar (ver_dir).c_str ());
      lstrcatW (wszOut, L"\\installed.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);

      *wszIn = L'\0';

      lstrcatW (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
      PathRemoveFileSpecW  (wszIn);

      lstrcatW (wszIn, L"\\Version\\repository.ini");

      *wszOut = L'\0';

      lstrcatW (wszOut, SK_UTF8ToWideChar (ver_dir).c_str ());
      lstrcatW (wszOut, L"\\repository.ini");

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
  wchar_t wszIn [MAX_PATH * 2] = { };
  lstrcatW (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());

  wchar_t wszOut [MAX_PATH * 2] = { };
  lstrcatW (wszOut, SK_GetHostPath ());

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

  if (CopyFile (wszIn, wszOut, TRUE))
  {
    *wszOut = L'\0';

    lstrcatW (wszOut, SK_GetHostPath ());
    lstrcatW (wszOut, L"\\SpecialK.central");

    FILE* fOut = _wfopen (wszOut, L"w");
                 fputws (L" ", fOut);
                       fclose (fOut);

    config.system.central_repository = true;

    *wszOut = L'\0';
    *wszIn  = L'\0';

    lstrcatW (wszOut, SK_GetHostPath ());

#ifdef _WIN64
    lstrcatW (wszIn,  L"SpecialK64.pdb");
    lstrcatW (wszOut, L"\\SpecialK64.pdb");
#else
    lstrcatW (wszIn,  L"SpecialK32.pdb");
    lstrcatW (wszOut, L"\\SpecialK32.pdb");
#endif

    if (! CopyFileW (wszIn, wszOut, TRUE))
      ReplaceFileW (wszOut, wszIn, nullptr, 0x00, nullptr, nullptr);

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

    *wszIn = L'\0';

    std::string ver_dir   =
      SK_FormatString ("%ws\\Version", SK_GetConfigPath ());

    const DWORD dwAttribs =
      GetFileAttributesA (ver_dir.c_str ());

    if ( (dwAttribs != INVALID_FILE_ATTRIBUTES) ||
          CreateDirectoryA (ver_dir.c_str (), nullptr) )
    {
      lstrcatW (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
      PathRemoveFileSpecW  (wszIn);

      lstrcatW (wszIn, L"\\Version\\installed.ini");

      *wszOut = L'\0';

      lstrcatW (wszOut, SK_UTF8ToWideChar (ver_dir).c_str ());
      lstrcatW (wszOut, L"\\installed.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);

      *wszIn = L'\0';

      lstrcatW (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
      PathRemoveFileSpecW  (wszIn);

      lstrcatW (wszIn, L"\\Version\\repository.ini");

      *wszOut = L'\0';

      lstrcatW (wszOut, SK_UTF8ToWideChar (ver_dir).c_str ());
      lstrcatW (wszOut, L"\\repository.ini");

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

  wchar_t wszOut [MAX_PATH * 2] = { };
  lstrcatW (wszOut, SK_GetHostPath ());

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

  wchar_t wszTemp [MAX_PATH] = {  };
  GetTempFileNameW (SK_GetHostPath (), L"SKI", timeGetTime (), wszTemp);

  MoveFileW (wszOut, wszTemp);

  SK_SaveConfig (L"SpecialK");

  return true;
}

bool
SK_Inject_SwitchToGlobalInjectorEx (DLL_ROLE role)
{
  config.system.central_repository = true;
  SK_EstablishRootPath ();

  wchar_t wszOut [MAX_PATH * 2] = { };
  lstrcatW (wszOut, SK_GetHostPath ());

  switch (role)
  {
    case DLL_ROLE::D3D9:
      lstrcatW (wszOut, L"\\d3d9.dll");
      break;

#ifndef _WIN64
    case DLL_ROLE::D3D8:
      lstrcatW (wszOut, L"\\d3d8.dll");
      break;
    case DLL_ROLE::DDraw:
      lstrcatW (wszOut, L"\\ddraw.dll");
      break;
#endif

    case DLL_ROLE::DXGI:
    {
      lstrcatW (wszOut, L"\\dxgi.dll");
    } break;

    case DLL_ROLE::OpenGL:
      lstrcatW (wszOut, L"\\OpenGL32.dll");
      break;

    //case SK_RenderAPI::Vulkan:
      //lstrcatW (wszOut, L"\\vk-1.dll");
      //break;
  }

  wchar_t wszTemp [MAX_PATH] = { };
  GetTempFileNameW (SK_GetHostPath (), L"SKI", timeGetTime (), wszTemp);

  MoveFileW (wszOut, wszTemp);

  SK_SaveConfig (L"SpecialK");

  return true;
}

extern std::wstring
SK_SYS_GetInstallPath (void);



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
  HANDLE         hProcSnap =
    CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

  if (hProcSnap == INVALID_HANDLE_VALUE)
    return false;

  pe32.dwSize = sizeof PROCESSENTRY32;

  if (! Process32First (hProcSnap, &pe32))
  {
    CloseHandle (hProcSnap);
    return false;
  }

  do
  {
    if (StrStrIW (wszProcName, pe32.szExeFile))
    {
      window_t win = SK_FindRootWindow (pe32.th32ProcessID);

      SendMessage (win.root, WM_USER + 0x123, 0x00, 0x00);

      CloseHandle (hProcSnap);
      return true;
    }
  } while (Process32Next (hProcSnap, &pe32));

  CloseHandle (hProcSnap);

  return false;
}


void
SK_Inject_Stop (void)
{
  //std::queue <DWORD> suspended = SK_SuspendAllOtherThreads ();

  wchar_t wszCurrentDir [MAX_PATH * 2] = { };
  GetCurrentDirectoryW (MAX_PATH * 2 - 1, wszCurrentDir);

  SetCurrentDirectory (SK_SYS_GetInstallPath ().c_str ());

  LoadLibrary (L"SpecialK32.dll");
  LoadLibrary (L"SpecialK64.dll");

#ifdef _WIN64
  wchar_t wszWOW64 [MAX_PATH + 2] = { };
  GetSystemWow64DirectoryW (wszWOW64, MAX_PATH);
#else
  wchar_t wszWOW64 [MAX_PATH + 2] = { };
  GetSystemDirectoryW      (wszWOW64, MAX_PATH);
#endif

  wchar_t wszSys32 [MAX_PATH + 2] = { };
  GetSystemDirectoryW      (wszSys32, MAX_PATH);

  //SK_ExitRemoteProcess (L"SKIM64.exe", 0x00);

  if (GetFileAttributes (L"SKIM64.exe") == INVALID_FILE_ATTRIBUTES)
  {
    PathAppendW   (wszWOW64, L"rundll32.exe");
    ShellExecuteA (NULL, "open", SK_WideCharToUTF8 (wszWOW64).c_str (), "SpecialK32.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE);

    PathAppendW   (wszSys32, L"rundll32.exe");
    ShellExecuteA (NULL, "open", SK_WideCharToUTF8 (wszSys32).c_str (), "SpecialK64.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE);
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
      ShellExecuteA        (NULL, "open", "SKIM64.exe", "-Inject", SK_WideCharToUTF8 (SK_SYS_GetInstallPath ()).c_str (), SW_FORCEMINIMIZE);
      SleepEx              (100UL, FALSE);
      SK_ExitRemoteProcess (L"SKIM64.exe", 0x00);
    }
  }

  SetCurrentDirectoryW (wszCurrentDir);

  //SK_ResumeThreads (suspended);
}

void
SK_Inject_Start (void)
{
  //std::queue <DWORD> suspended = SK_SuspendAllOtherThreads ();

  wchar_t wszCurrentDir [MAX_PATH * 2] = { };
  GetCurrentDirectoryW (MAX_PATH * 2 - 1, wszCurrentDir);

  SetCurrentDirectory (SK_SYS_GetInstallPath ().c_str ());

#ifdef _WIN64
  wchar_t wszWOW64 [MAX_PATH + 2] = { };
  GetSystemWow64DirectoryW (wszWOW64, MAX_PATH);
#else
  wchar_t wszWOW64 [MAX_PATH + 2] = { };
  GetSystemDirectoryW      (wszWOW64, MAX_PATH);
#endif

  wchar_t wszSys32 [MAX_PATH + 2] = { };
  GetSystemDirectoryW      (wszSys32, MAX_PATH);

  if (GetFileAttributes (L"SKIM64.exe") == INVALID_FILE_ATTRIBUTES)
  {
    if (SKX_IsHookingCBT ())
    {
      RunDLL_InjectionManager ( NULL, NULL,
                                "Remove", -128 );
    }

    PathAppendW   (wszSys32, L"rundll32.exe");
    ShellExecuteA (NULL, "open", SK_WideCharToUTF8 (wszSys32).c_str (), "SpecialK64.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE);

    PathAppendW   (wszWOW64, L"rundll32.exe");
    ShellExecuteA (NULL, "open", SK_WideCharToUTF8 (wszWOW64).c_str (), "SpecialK32.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE);
  }

  else
  {
    if (SKX_IsHookingCBT ())
    {
      RunDLL_InjectionManager ( NULL, NULL,
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
      ShellExecuteA (NULL, "open", "SKIM64.exe", "+Inject", SK_WideCharToUTF8 (SK_SYS_GetInstallPath ()).c_str (), SW_FORCEMINIMIZE);
  }

  SetCurrentDirectoryW (wszCurrentDir);

  //SK_ResumeThreads (suspended);
}


size_t
__stdcall
SKX_GetInjectedPIDs ( DWORD* pdwList,
                      size_t capacity )
{
  DWORD*  pdwListIter = pdwList;
  SSIZE_T i           = 0;

  SK_Inject_ValidateProcesses ();

  for (int idx = 0; idx < MAX_INJECTED_PROCS; idx++)
  {
    if (InterlockedExchangeAdd (&g_sHookedPIDs [idx], 0) != 0)
    {
      if (i < (SSIZE_T)capacity - 1)
      {
        if (pdwListIter != nullptr)
        {
          *pdwListIter = InterlockedExchangeAdd (&g_sHookedPIDs [idx], 0);
          pdwListIter++;
        }
      }

      ++i;
    }
  }

  if (pdwListIter != nullptr)
    *pdwListIter = 0;

  return i;
}