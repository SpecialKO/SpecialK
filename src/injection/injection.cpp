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

#include <Shlwapi.h>

#include <unordered_set>

//#pragma data_seg (".SK_Hooks")
HHOOK g_hHookCBT   = nullptr;
HHOOK g_hHookShell = nullptr; // In theory, this is a lighter-weight hook on a game oriented machine
//#pragma data_seg ()
//#pragma comment  (linker, "/section:.SK_Hooks,RWS")

extern volatile ULONG __SK_HookContextOwner;

LRESULT
CALLBACK
ShellProc ( _In_ int    nCode,
            _In_ WPARAM wParam,
            _In_ LPARAM lParam )
{
  return CallNextHookEx(g_hHookShell, nCode, wParam, lParam);
}


LRESULT
CALLBACK
CBTProc (int nCode, WPARAM wParam, LPARAM lParam)
{
  return CallNextHookEx(g_hHookCBT, nCode, wParam, lParam);
}



#if 0
extern "C" __declspec (dllexport)
void
__stdcall
SKX_InstallCBTHook (void)
{
  // Nothing to do here, move along.
  if (g_hHookCBT != nullptr)
    return;

  HMODULE hMod;

  if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             (wchar_t *) &SKX_InstallCBTHook,
                               (HMODULE *) &hMod ) )
  {
    g_hHookCBT =
      SetWindowsHookEx (WH_CBT, CBTProc, hMod, 0);
  }
}


extern "C" __declspec (dllexport)
void
__stdcall
SKX_RemoveCBTHook (void)
{
  if (g_hHookCBT)
  {
    if (UnhookWindowsHookEx (g_hHookCBT))
      g_hHookCBT = nullptr;
  }
}


extern "C" __declspec (dllexport)
bool
__stdcall
SKX_IsHookingCBT (void)
{
  return (g_hHookCBT != nullptr);
}
#endif




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



extern "C" __declspec (dllexport)
void
__stdcall
SKX_InstallShellHook (void)
{
  // Nothing to do here, move along.
  if (g_hHookShell != nullptr)
    return;

  HMODULE hMod;

  //if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           //GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             //(wchar_t *) &SKX_InstallShellHook,
                               //(HMODULE *) &hMod ) )
  
#ifdef _WIN64
  hMod = GetModuleHandle (L"SpecialK64.dll");
#else
  hMod = GetModuleHandle (L"SpecialK32.dll");
#endif

  extern HMODULE
  __stdcall
  SK_GetDLL (void);

  if (hMod == SK_GetDLL ())
  {
    // Shell hooks don't work very well, they run into problems with
    //   hooking XInput -- CBT is more reliable, but slower.
    //
    //  >>  ** Thank you GeForce Experience :-\
    //
    g_hHookShell =
      SetWindowsHookEx (WH_CBT, ShellProc, hMod, 0);

    if (g_hHookShell != 0)
      __SK_HookContextOwner = true;
  }
}


extern "C" __declspec (dllexport)
void
__stdcall
SKX_RemoveShellHook (void)
{
  if (g_hHookShell)
  {
    if (UnhookWindowsHookEx (g_hHookShell))
    {
      __SK_HookContextOwner = false;
      g_hHookShell          = nullptr;
    }
  }
}

extern "C" __declspec (dllexport)
bool
__stdcall
SKX_IsHookingShell (void)
{
  return (g_hHookShell != nullptr);
}




#if 0
extern "C" __declspec (dllexport)
bool
__stdcall
SKX_IsProcessHooked (DWORD dwPid)
{
  DWORD dwDesiredAccess = PROCESS_CREATE_THREAD;
  BOOL  bInheritHandle  = FALSE;

  HANDLE hProcess =
    OpenProcess ( dwDesiredAccess, bInheritHandle, dwPid );

  if (hProcess == nullptr)
    return FALSE;
  
  BOOL result =
    hooked_procs.count (hProcess);

  CloseHandle (hProcess);
  
  return result;
}
#endif





// Useful for manging injection of the 32-bit DLL from a 64-bit application or
//   visa versa.
void
CALLBACK
RunDLL_InjectionManager ( HWND  hwnd,        HINSTANCE hInst,
                          LPSTR lpszCmdLine, int       nCmdShow )
{
  if (StrStrA (lpszCmdLine, "Install") && (! SKX_IsHookingShell ()))
  {
    SKX_InstallShellHook ();

    if (SKX_IsHookingShell ())
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

        Sleep (INFINITE);
      }
    }
  }

  else if (StrStrA (lpszCmdLine, "Remove"))
  {
    SKX_RemoveShellHook ();

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

      if (SK_TerminatePID (dwPID, 0x00))
      {
#ifndef _WIN64
        DeleteFileA ("SpecialK32.pid");
#else
        DeleteFileA ("SpecialK64.pid");
#endif
      }
    }
  }
}




#include <SpecialK/utility.h>
#include <SpecialK/render_backend.h>

bool
SKinja_SwitchToRenderWrapper (void)
{
  wchar_t wszIn [MAX_PATH * 2] = { L'\0' };
  lstrcatW (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());

  wchar_t wszOut [MAX_PATH * 2] = { L'\0' };
  lstrcatW (wszOut, SK_GetHostPath ());

  switch (SK_GetCurrentRenderBackend ().api)
  {
    case SK_RenderAPI::D3D9:
    case SK_RenderAPI::D3D9Ex:
      lstrcatW (wszOut, L"\\d3d9.dll");
      break;

    case SK_RenderAPI::D3D10:
    case SK_RenderAPI::D3D11:
    case SK_RenderAPI::D3D12:
      lstrcatW (wszOut, L"\\dxgi.dll");
      break;

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

    *wszOut = L'\0';
    *wszIn  = L'\0';

    lstrcatW (wszIn, SK_GetConfigPath ());
    lstrcatW (wszIn, L"\\SpecialK.ini");

    lstrcatW (wszOut, SK_GetConfigPath ());

    switch (SK_GetCurrentRenderBackend ().api)
    {
      case SK_RenderAPI::D3D9:
      case SK_RenderAPI::D3D9Ex:
        lstrcatW (wszOut, L"\\d3d9.ini");
        break;

      case SK_RenderAPI::D3D10:
      case SK_RenderAPI::D3D11:
      case SK_RenderAPI::D3D12:
        lstrcatW (wszOut, L"\\dxgi.ini");
        break;

      case SK_RenderAPI::OpenGL:
        lstrcatW (wszOut, L"\\OpenGL32.ini");
        break;

      //case SK_RenderAPI::Vulkan:
        //lstrcatW (wszOut, L"\\vk-1.dll");
        //break;
    }

    if (! CopyFileW (wszIn, wszOut, TRUE))
      ReplaceFileW (wszOut, wszIn, nullptr, 0x00, nullptr, nullptr);

    return true;
  }

  return false;
}

bool
SKinja_SwitchToGlobalInjector (void)
{
  wchar_t wszOut [MAX_PATH * 2] = { L'\0' };
  lstrcatW (wszOut, SK_GetHostPath ());

  switch (SK_GetCurrentRenderBackend().api)
  {
    case SK_RenderAPI::D3D9:
    case SK_RenderAPI::D3D9Ex:
      lstrcatW (wszOut, L"\\d3d9.dll");
      break;

    case SK_RenderAPI::D3D10:
    case SK_RenderAPI::D3D11:
    case SK_RenderAPI::D3D12:
      lstrcatW (wszOut, L"\\dxgi.dll");
      break;

    case SK_RenderAPI::OpenGL:
      lstrcatW (wszOut, L"\\OpenGL32.dll");
      break;

    //case SK_RenderAPI::Vulkan:
      //lstrcatW (wszOut, L"\\vk-1.dll");
      //break;
  }

  wchar_t wszTemp [MAX_PATH] = { L'\0' };
  GetTempFileNameW (SK_GetHostPath (), L"SKI", timeGetTime (), wszTemp);

  MoveFileW (wszOut, wszTemp);

  return true;
}




bool
SK_bIsSteamClient (HMODULE hMod);

bool SK_Injection_JournalRecord (HMODULE hModule)
{
  return false;
  //return SK_bIsSteamClient (hModule);
}



extern std::wstring
SK_SYS_GetInstallPath (void);

#if 1
void
SK_Inject_Stop (void)
{
  std::queue <DWORD> suspended = SK_SuspendAllOtherThreads ();

  wchar_t wszCurrentDir [MAX_PATH * 2] = { L'\0' };
  GetCurrentDirectoryW (MAX_PATH * 2 - 1, wszCurrentDir);

  SetCurrentDirectory (SK_SYS_GetInstallPath ().c_str ());

  ShellExecuteA (NULL, "open", "rundll32.exe", "SpecialK32.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE);
  ShellExecuteA (NULL, "open", "rundll32.exe", "SpecialK64.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE);

  SetCurrentDirectoryW (wszCurrentDir);

  SK_ResumeThreads (suspended);
}

void
SK_Inject_Start (void)
{
  std::queue <DWORD> suspended = SK_SuspendAllOtherThreads ();

  wchar_t wszCurrentDir [MAX_PATH * 2] = { L'\0' };
  GetCurrentDirectoryW (MAX_PATH * 2 - 1, wszCurrentDir);

  SetCurrentDirectory (SK_SYS_GetInstallPath ().c_str ());

  ShellExecuteA (NULL, "open", "rundll32.exe", "SpecialK32.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE);
  ShellExecuteA (NULL, "open", "rundll32.exe", "SpecialK64.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE);

  SetCurrentDirectoryW (wszCurrentDir);

  SK_ResumeThreads (suspended);
}
#else
void
SK_Inject_Stop (void)
{
  STARTUPINFOW si = { 0 };
  si.cb = sizeof si;

  PROCESS_INFORMATION pi = { 0 };

  CreateProcessW ( L"rundll32.exe",
                     L"SpecialK32.dll,RunDLL_InjectionManager Remove",
                       nullptr, nullptr,
                         FALSE,
                           CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                             nullptr,
                               SK_SYS_GetInstallPath ().c_str (),
                                 &si, &pi );

  si = { 0 };
  si.cb = sizeof si;

  CreateProcessW ( L"rundll32.exe",
                     L"SpecialK64.dll,RunDLL_InjectionManager Remove",
                       nullptr, nullptr,
                         FALSE,
                           CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                             nullptr,
                               SK_SYS_GetInstallPath ().c_str (),
                                 &si, &pi );
}

void
SK_Inject_Start (void)
{
  STARTUPINFOW si = { 0 };
  si.cb = sizeof si;

  PROCESS_INFORMATION pi = { 0 };

  CreateProcessW ( L"rundll32.exe",
                     L"SpecialK32.dll,RunDLL_InjectionManager Install",
                       nullptr, nullptr,
                         FALSE,
                           CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                             nullptr,
                               SK_SYS_GetInstallPath ().c_str (),
                                 &si, &pi );

  si = { 0 };
  si.cb = sizeof si;

  CreateProcessW ( L"rundll32.exe",
                     L"SpecialK64.dll,RunDLL_InjectionManager Install",
                       nullptr, nullptr,
                         FALSE,
                           CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                             nullptr,
                               SK_SYS_GetInstallPath ().c_str (),
                                 &si, &pi );
}
#endif