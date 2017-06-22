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

#pragma data_seg (".SK_Hooks")
HHOOK   g_hHookCBT             = nullptr;
HANDLE  g_hShutdown            = 0;
#pragma data_seg ()
#pragma comment  (linker, "/section:.SK_Hooks,RWS")

HMODULE hModHookInstance       = NULL;
extern volatile ULONG __SK_HookContextOwner;

LRESULT
CALLBACK
CBTProc ( _In_ int    nCode,
          _In_ WPARAM wParam,
          _In_ LPARAM lParam )
{
  if (nCode < 0)
    return CallNextHookEx (0, nCode, wParam, lParam);

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



extern "C"
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

    if (g_hHookCBT != 0) {
      InterlockedExchange (&__SK_HookContextOwner, TRUE);
    }
  }
}


extern "C"
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

extern "C"
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

               ExitProcess (0x00);

               return 0;
             },
             nullptr,
           0x00,
         nullptr );

        Sleep (INFINITE);
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
      break;

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
      break;

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



bool SK_Inject_JournalRecord (HMODULE hModule)
{
  return false;
}


void
SK_Inject_Stop (void)
{
  //std::queue <DWORD> suspended = SK_SuspendAllOtherThreads ();

  wchar_t wszCurrentDir [MAX_PATH * 2] = { };
  GetCurrentDirectoryW (MAX_PATH * 2 - 1, wszCurrentDir);

  SetCurrentDirectory (SK_SYS_GetInstallPath ().c_str ());

  ShellExecuteA (NULL, "open", "rundll32.exe", "SpecialK32.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE);
  ShellExecuteA (NULL, "open", "rundll32.exe", "SpecialK64.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE);

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

  ShellExecuteA (NULL, "open", "rundll32.exe", "SpecialK32.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE);
  ShellExecuteA (NULL, "open", "rundll32.exe", "SpecialK64.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE);

  SetCurrentDirectoryW (wszCurrentDir);

  //SK_ResumeThreads (suspended);
}