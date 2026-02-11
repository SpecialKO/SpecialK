// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <SpecialK/utility.h>
#include <SpecialK/plugin/arknights_endfield.h>
#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/plugin/plugin_mgr.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <imgui/imgui.h>
#include <winternl.h>
#include <imgui/font_awesome.h>
#include <aetherim/image.hpp>
#include <aetherim/field.hpp>
#include <aetherim/class.hpp>
#include <aetherim/wrapper.hpp>
#include <aetherim/api.hpp>
#include <aetherim/type.hpp>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Arknights Endfield"

namespace AKEF_SHM
{
  static volatile HANDLE g_hMap     = INVALID_HANDLE_VALUE;
  static volatile LPVOID g_pView    = nullptr;
  static volatile LONG   g_isNewMap = FALSE;

  static constexpr wchar_t kSharedMapName[] = LR"(Local\SK_AKEF_Shared_v1)";

  static HANDLE
  GetSharedMemory (void)
  {
    HANDLE hLocalMap = ReadPointerAcquire (&g_hMap);
    if (hLocalMap != INVALID_HANDLE_VALUE)
      return hLocalMap;

    SK_AutoHandle hExistingFile (
      OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, kSharedMapName)
    );

    if (hExistingFile.isValid ())
    {
      if (INVALID_HANDLE_VALUE ==
          InterlockedCompareExchangePointer (&g_hMap, hExistingFile.m_h, INVALID_HANDLE_VALUE))
      {
        WritePointerRelease (&g_hMap, hExistingFile.m_h);
        InterlockedExchange (&g_isNewMap, FALSE);
        hExistingFile.m_h = nullptr;
        return ReadPointerAcquire (&g_hMap);
      }

      hExistingFile.Close ();
    }

    SK_AutoHandle hNewFile (
      CreateFileMappingW (INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof (SharedMemory), kSharedMapName)
    );

    if (hNewFile.isValid ())
    {
      DWORD last_error = GetLastError ();
      if (INVALID_HANDLE_VALUE ==
          InterlockedCompareExchangePointer (&g_hMap, hNewFile.m_h, INVALID_HANDLE_VALUE))
      {
        InterlockedExchange (&g_isNewMap, last_error == ERROR_ALREADY_EXISTS ? FALSE : TRUE);
        hNewFile.m_h = nullptr;
        return ReadPointerAcquire (&g_hMap);
      }

      hNewFile.Close ();
    }

    return nullptr;
  }
}

AKEF_SHM::SharedMemory*
AKEF_SHM::GetView (void)
{
  auto* local_view = reinterpret_cast <SharedMemory *> (ReadPointerAcquire (&g_pView));
  if (local_view != nullptr)
    return local_view;

  HANDLE hMap = AKEF_SHM::GetSharedMemory ();
  if (hMap != nullptr)
  {
    local_view = reinterpret_cast <SharedMemory *> (
      MapViewOfFile (hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof (SharedMemory))
    );

    if (local_view != nullptr)
    {
      if (nullptr ==
          InterlockedCompareExchangePointer (&g_pView, local_view, nullptr))
      {
        if (ReadAcquire (&g_isNewMap))
        {
          WriteRelease (&local_view->has_pid, 0);
          local_view->akef_launch_pid = 0;
        }

        return local_view;
      }

      UnmapViewOfFile (local_view);
    }

    return reinterpret_cast <SharedMemory *> (ReadPointerAcquire (&g_pView));
  }

  return nullptr;
}

void
AKEF_SHM::PublishPid (DWORD pid)
{
  if (auto* shm = AKEF_SHM::GetView ())
  {
    shm->akef_launch_pid = pid;
    WriteRelease (&shm->has_pid, 1);
  }
}

void
AKEF_SHM::ResetPid (void)
{
  if (auto* shm = AKEF_SHM::GetView ())
  {
    WriteRelease (&shm->has_pid, 0);
    shm->akef_launch_pid = 0;
  }
}

bool
AKEF_SHM::TryGetPid (DWORD* out_pid)
{
  if (out_pid == nullptr)
    return false;

  if (auto* shm = AKEF_SHM::GetView ())
  {
    if (ReadAcquire (&shm->has_pid) == 1)
    {
      *out_pid = shm->akef_launch_pid;
      return true;
    }
  }

  return false;
}

void
AKEF_SHM::CleanupSharedMemory (void)
{
  auto* view = reinterpret_cast <SharedMemory *> (ReadPointerAcquire (&g_pView));
  if (view != nullptr &&
      InterlockedCompareExchangePointer (&g_pView, nullptr, view) == view)
  {
    UnmapViewOfFile (view);
  }

  HANDLE hLocalMap = reinterpret_cast <HANDLE> (
    InterlockedCompareExchangePointer (&g_hMap, INVALID_HANDLE_VALUE, ReadPointerAcquire (&g_hMap))
  );

  if (hLocalMap != INVALID_HANDLE_VALUE && hLocalMap != nullptr)
  {
    SK_CloseHandle (hLocalMap);
  }
}

void SK_AKEF_PublishPid          (DWORD pid) { AKEF_SHM::PublishPid (pid); }
bool SK_AKEF_TryGetPid           (DWORD* out_pid) { return AKEF_SHM::TryGetPid (out_pid); }
void SK_AKEF_ResetPid            (void) { AKEF_SHM::ResetPid (); }
void SK_AKEF_CleanupSharedMemory (void) { AKEF_SHM::CleanupSharedMemory (); }

static constexpr float kDefaultFirstFramerateLimit = -99.0f;

bool  __g_SK_AKEF_KeepOriginalSwapchain     = false;
bool  __SK_AKEF_OverrideUnityFramerateLimit = false;
bool  __SK_AKEF_KeepOriginalSwapchain       = false;
float __SK_AKEF_UnityFramerateLimit         = kDefaultFirstFramerateLimit;

sk::ParameterBool*  _SK_AKEF_OverrideUnityFramerateLimit = nullptr;
sk::ParameterBool*  _SK_AKEF_KeepOriginalSwapchain       = nullptr;
sk::ParameterFloat* _SK_AKEF_UnityFramerateLimit         = nullptr;

volatile LONG __AKEF_init = FALSE;

using NtQueryInformationProcess_t = LONG (WINAPI*) (
  HANDLE ProcessHandle,
  ULONG  ProcessInformationClass,
  PVOID  ProcessInformation,
  ULONG  ProcessInformationLength,
  PULONG ReturnLength
);

static float
CalcLimit (float fps)
{
  return std::abs (fps) + 1.0f;
}

extern HRESULT WINAPI
SK_TaskDialogIndirect (_In_      const TASKDIALOGCONFIG* pTaskConfig,
                       _Out_opt_       int* pnButton,
                       _Out_opt_       int* pnRadioButton,
                       _Out_opt_       BOOL* pfVerificationFlagChecked);

static HRESULT CALLBACK
AKEF_TaskDialogTimeoutCallback (
  _In_ HWND     hWnd,
  _In_ UINT     uNotification,
  _In_ WPARAM   wParam,
  _In_ LPARAM   lParam,
  _In_ LONG_PTR dwRefData)
{
  UNREFERENCED_PARAMETER (lParam);

  if (uNotification == TDN_TIMER)
  {
    const auto timeout_ms = static_cast <DWORD> (dwRefData);
    const auto elapsed_ms = static_cast <DWORD> (wParam);

    if (elapsed_ms >= timeout_ms)
      SendMessageW (hWnd, TDM_CLICK_BUTTON, IDOK, 0);
  }

  return S_OK;
}

static HRESULT
AKEF_ShowTimedTaskDialog (
  PCWSTR   mainInstruction,
  PCWSTR   content,
  LPVOID   callback,
  LONG_PTR callbackData)
{
  static const std::wstring ver_str =
    SK_FormatStringW (L"Special K (v %ws)", SK_GetVersionStrW ());

  TASKDIALOGCONFIG cfg = { };
  cfg.cbSize = sizeof (cfg);

  cfg.dwFlags =
    TDF_CALLBACK_TIMER |
    TDF_ALLOW_DIALOG_CANCELLATION;

  cfg.dwCommonButtons = TDCBF_OK_BUTTON;

  cfg.pszWindowTitle = ver_str.c_str ();
  cfg.pszMainInstruction = mainInstruction;
  cfg.pszContent = content;

  cfg.pfCallback = static_cast <PFTASKDIALOGCALLBACK> (callback);
  cfg.lpCallbackData = callbackData;

  return SK_TaskDialogIndirect (&cfg, nullptr, nullptr, nullptr);
}

static BOOL CALLBACK
AKEF_EnumWindowsProc (HWND hwnd, LPARAM lParam)
{
  auto* pData = reinterpret_cast <AKEF_WindowData *> (lParam);
  if (pData == nullptr)
    return FALSE;

  DWORD windowPID = 0;
  GetWindowThreadProcessId (hwnd, &windowPID);

  if (windowPID == pData->pid)
  {
    wchar_t title[256] = { 0 };
    wchar_t class_name[256] = { 0 };

    GetWindowTextW (hwnd, title, static_cast <int> (std::size (title)));
    GetClassNameW (hwnd, class_name, static_cast <int> (std::size (class_name)));

    pData->windows.push_back ({ hwnd, title, class_name });
  }

  return TRUE;
}

std::vector<DWORD>
SK_AKEF_GetExistingEndfieldPids (void)
{
  std::vector<DWORD> existingPids;
  HANDLE snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

  if (snapshot == INVALID_HANDLE_VALUE)
    return {};

  PROCESSENTRY32W processEntry = { };
  processEntry.dwSize = sizeof (PROCESSENTRY32W);
  if (Process32FirstW (snapshot, &processEntry))
  {
    do
    {
      if (_wcsicmp (processEntry.szExeFile, L"Endfield.exe") == 0)
      {
        existingPids.push_back (processEntry.th32ProcessID);
      }
    } while (Process32NextW (snapshot, &processEntry));
  }

  SK_CloseHandle (snapshot);
  return existingPids;
}

static std::wstring
AKEF_GetProcessCommandLine (HANDLE hProcess)
{
  if (hProcess == nullptr)
    return L"";

  static NtQueryInformationProcess_t NtQueryInformationProcess = nullptr;

  if (!NtQueryInformationProcess)
    NtQueryInformationProcess =
      reinterpret_cast<NtQueryInformationProcess_t> (SK_GetProcAddress (L"ntdll.dll", "NtQueryInformationProcess"));
  if (NtQueryInformationProcess == nullptr)
    return L"";

  PROCESS_BASIC_INFORMATION pbi = { };
  if (NtQueryInformationProcess (hProcess, ProcessBasicInformation, &pbi, sizeof (pbi), nullptr) != 0)
    return L"";

  PEB peb = { };
  if (!ReadProcessMemory (hProcess, pbi.PebBaseAddress, &peb, sizeof (peb), nullptr))
    return L"";

  RTL_USER_PROCESS_PARAMETERS params = { };
  if (!ReadProcessMemory (hProcess, peb.ProcessParameters, &params, sizeof (params), nullptr))
    return L"";

  std::wstring cmd;
  cmd.resize (params.CommandLine.Length / sizeof (wchar_t));
  ReadProcessMemory (hProcess, params.CommandLine.Buffer, cmd.data (), params.CommandLine.Length, nullptr);
  return cmd;
}

static bool
AKEF_IsRemoteLdrInitialized (HANDLE hProcess, FARPROC ntQueryAddr)
{
  static NtQueryInformationProcess_t NtQueryInformationProcess = nullptr;
  if (!NtQueryInformationProcess)
    NtQueryInformationProcess = reinterpret_cast<NtQueryInformationProcess_t> (ntQueryAddr);

  PROCESS_BASIC_INFORMATION pbi = { };
  if (NtQueryInformationProcess (hProcess, ProcessBasicInformation, &pbi, sizeof (pbi), nullptr) != 0)
    return false;

  PEB peb = { };
  if (!ReadProcessMemory (hProcess, pbi.PebBaseAddress, &peb, sizeof (peb), nullptr))
    return false;

  if (!peb.Ldr)
    return false;

  PEB_LDR_DATA_SK ldr = { };
  if (!ReadProcessMemory (hProcess, peb.Ldr, &ldr, sizeof (ldr), nullptr))
    return false;

  return ldr.Initialized != FALSE;
}

static HMODULE
AKEF_GetRemoteModule (HANDLE hProcess, FARPROC ntQueryAddr, const std::wstring& lowercaseModName)
{
  static NtQueryInformationProcess_t NtQueryInformationProcess = nullptr;
  if (!NtQueryInformationProcess)
    NtQueryInformationProcess = reinterpret_cast<NtQueryInformationProcess_t> (ntQueryAddr);

  PROCESS_BASIC_INFORMATION pbi = { };
  if (NtQueryInformationProcess (hProcess, ProcessBasicInformation, &pbi, sizeof (pbi), nullptr) != 0)
    return nullptr;

  PEB peb = { };
  if (!ReadProcessMemory (hProcess, pbi.PebBaseAddress, &peb, sizeof (peb), nullptr))
    return nullptr;

  if (!peb.Ldr)
    return nullptr;

  PEB_LDR_DATA_SK ldr = { };
  if (!ReadProcessMemory (hProcess, peb.Ldr, &ldr, sizeof (ldr), nullptr))
    return nullptr;

  auto ReadRemoteUnicodeString = [hProcess](const UNICODE_STRING_SK& us, std::wstring& out) -> bool
    {
      if (!us.Buffer || us.Length == 0) return false;

      constexpr USHORT kMaxLen = 0x2000;
      if (us.Length > kMaxLen) return false;

      out.resize (us.Length / sizeof(wchar_t));
      return ReadProcessMemory (hProcess, us.Buffer, out.data(), us.Length, nullptr) != FALSE;
    };

  auto listHead =
    reinterpret_cast<LIST_ENTRY*> (reinterpret_cast<BYTE*> (peb.Ldr) +
      offsetof(PEB_LDR_DATA_SK, InLoadOrderModuleList)
      );

  LIST_ENTRY* curr = ldr.InLoadOrderModuleList.Flink;

  while (curr && curr != listHead)
  {
    LDR_DATA_TABLE_ENTRY__SK entry = { };

    if (!ReadProcessMemory (hProcess, curr, &entry, sizeof (entry), nullptr))
      break;

    std::wstring baseName;
    if (ReadRemoteUnicodeString (entry.BaseDllName, baseName))
    {
      std::transform (baseName.begin (), baseName.end (), baseName.begin (), ::towlower);
      if (baseName == lowercaseModName)
        return static_cast<HMODULE> (entry.DllBase);
    }

    curr = entry.InLoadOrderLinks.Flink;
  }

    return nullptr;
 }

static bool
AKEF_InjectDLL (HANDLE hTargetProc, const std::wstring& dllPath)
{
  void* loadLibraryAddr = SK_GetProcAddress (L"kernel32.dll", "LoadLibraryW");
  if (loadLibraryAddr == nullptr)
    return false;

  const SIZE_T pathLen = (dllPath.length () + 1) * sizeof (wchar_t);
  LPVOID dllPathAddr = VirtualAllocEx (hTargetProc, nullptr, pathLen,
    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (dllPathAddr == nullptr)
    return false;

  if (!WriteProcessMemory (hTargetProc, dllPathAddr, dllPath.c_str (), pathLen, nullptr))
  {
    VirtualFreeEx (hTargetProc, dllPathAddr, 0, MEM_RELEASE);
    return false;
  }

  HANDLE hThread = CreateRemoteThread (
    hTargetProc, nullptr, 0,
    static_cast <LPTHREAD_START_ROUTINE> (loadLibraryAddr),
    dllPathAddr, 0, nullptr
  );

  if (hThread == nullptr)
  {
    VirtualFreeEx (hTargetProc, dllPathAddr, 0, MEM_RELEASE);
    return false;
  }

  WaitForSingleObject (hThread, INFINITE);

  DWORD exitCode = 0;
  GetExitCodeThread (hThread, &exitCode);

  VirtualFreeEx (hTargetProc, dllPathAddr, 0, MEM_RELEASE);
  CloseHandle (hThread);

  return exitCode != 0;
}

static void
SK_LaunchArknightEndfield_Impl (const wchar_t* wszExecutableName)
{
  AKEF_ShowTimedTaskDialog (
    L"Special K is preparing to launch a new Arknight Endfield process",
    L"The original process will be closed automatically.\n\n"
    L"Consider injecting SK to the original Endfield Launcher.\n"
    L"SK will automatically inject itself into the new process created by the launcher.\n",
    AKEF_TaskDialogTimeoutCallback,
    4000
  );

  auto ctx = SK_SuspendAllOtherThreads ();
  SK_Sleep (1500);

  extern PROCESSENTRY32W FindProcessByName (const wchar_t* wszName);
  DWORD origEndfieldPID = FindProcessByName (L"Endfield.exe").th32ProcessID;

  AKEF_WindowData data = { origEndfieldPID, { } };
  SK_LOGi1 (L"Found original Arknight: Endfield PID: %d\n", origEndfieldPID);
  EnumWindows (AKEF_EnumWindowsProc, reinterpret_cast <LPARAM> (&data));

  STARTUPINFOW        sinfo = { };
  PROCESS_INFORMATION pinfo = { };
  BOOL                created = FALSE;

  sinfo.cb = sizeof (STARTUPINFOW);
  sinfo.wShowWindow = SW_SHOWNORMAL;
  sinfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;

  auto CleanupProcess = [&](DWORD exitCode)
  {
    AKEF_SHM::ResetPid ();
    if (pinfo.hProcess)
    {
      TerminateProcess (pinfo.hProcess, exitCode);
      SK_CloseHandle (pinfo.hProcess);
      pinfo.hProcess = nullptr;
    }

    if (pinfo.hThread)
    {
      SK_CloseHandle (pinfo.hThread);
      pinfo.hThread = nullptr;
    }
  };

  std::wstring skDllPath = SK_FormatStringW (
    SK_RunLHIfBitness (64, LR"(%ws\SpecialK64.dll)", LR"(%ws\SpecialK32.dll)"),
    SK_GetInstallPath ()
  );
  SK_LOGi1 (L"DLL Path: %s, hostPath: %s\n", skDllPath.c_str (), SK_GetHostPath ());

  HANDLE hOrigEndfield = OpenProcess (
    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
    FALSE,
    origEndfieldPID
  );

  if (hOrigEndfield == nullptr)
  {
    SK_ResumeThreads (ctx);
    return;
  }

  std::wstring cmdLine = AKEF_GetProcessCommandLine (hOrigEndfield);
  SK_CloseHandle (hOrigEndfield);

  if (cmdLine.empty ())
  {
    created = CreateProcess (wszExecutableName, nullptr, nullptr, nullptr,
      TRUE, CREATE_SUSPENDED, nullptr, SK_GetHostPath (), &sinfo, &pinfo);
  }
  else
  {
    created = CreateProcess (nullptr, cmdLine.data (), nullptr, nullptr,
      TRUE, CREATE_SUSPENDED, nullptr, SK_GetHostPath (), &sinfo, &pinfo);
  }

  if (!created)
  {
    SK_ResumeThreads (ctx);
    return;
  }

  SK_LOGi1 (L"Launching Arknight: Endfield with PID: %d\n", pinfo.dwProcessId);
  AKEF_SHM::PublishPid (pinfo.dwProcessId);

  SK_SelfDestruct ();
  SK_ResumeThreads (ctx);

  if (pinfo.hProcess != nullptr)
  {
    auto startTime = std::chrono::steady_clock::now ();
    constexpr auto maxWait = std::chrono::seconds (15);

    FARPROC ntQueryAddr = SK_GetProcAddress (L"ntdll.dll", "NtQueryInformationProcess");
    ResumeThread (pinfo.hThread);

    while (!AKEF_IsRemoteLdrInitialized (pinfo.hProcess, ntQueryAddr))
    {
      DWORD exitCode = 0;
      if (WaitForSingleObject (pinfo.hProcess, 0) == WAIT_OBJECT_0 ||
          (GetExitCodeProcess (pinfo.hProcess, &exitCode) && exitCode != STILL_ACTIVE))
      {
        CleanupProcess (0x1);
        return;
      }

      if (std::chrono::steady_clock::now () - startTime > maxWait)
      {
        CleanupProcess (0x1);
        return;
      }

      SK_Sleep (5);
    }

    while (!AKEF_GetRemoteModule(pinfo.hProcess, ntQueryAddr, L"kernel32.dll"))
    {
      DWORD exitCode = 0;
      if (WaitForSingleObject (pinfo.hProcess, 0) == WAIT_OBJECT_0 ||
          (GetExitCodeProcess (pinfo.hProcess, &exitCode) && exitCode != STILL_ACTIVE))
      {
        CleanupProcess (0x1);
        return;
      }
      if (std::chrono::steady_clock::now () - startTime > maxWait)
      {
        CleanupProcess (0x1);
        return;
      }
      SK_Sleep (10);
    }

    UINT exitCode = 0;
    if (AKEF_InjectDLL (pinfo.hProcess, skDllPath))
    {
      SK_LOGi1 (L"SpecialK has been injected successfully.");
    }
    else
    {
      exitCode = 1;
      SK_LOGi1 (L"SpecialK injection failed.");
      AKEF_ShowTimedTaskDialog (
        L"Special K failed to inject into Arknight: Endfield.",
        L"The game may not launch correctly.",
        AKEF_TaskDialogTimeoutCallback,
        4000
      );
    }

    SK_CloseHandle (pinfo.hThread);
    SK_CloseHandle (pinfo.hProcess);

    for (const auto& window : data.windows)
    {
      if (IsWindow (window.hwnd))
        PostMessageW (window.hwnd, WM_CLOSE, 0, 0);
    }

    HANDLE hProcess = OpenProcess (SYNCHRONIZE | PROCESS_TERMINATE, FALSE, origEndfieldPID);
    if (hProcess)
    {
      WaitForSingleObject (hProcess, 200);
      TerminateProcess (hProcess, 1);
      SK_CloseHandle (hProcess);
    }

    SK_TerminateProcess (exitCode);
  }
  else
  {
    AKEF_ShowTimedTaskDialog (
      L"Special K failed to launch Arknight: Endfield.",
      L"The game may not launch correctly.",
      AKEF_TaskDialogTimeoutCallback,
      4000
    );
  }
}

void
SK_LaunchArknightEndfield (const wchar_t* wszExecutableName)
{
  __try
  {
    SK_Thread_CreateEx ([](LPVOID param) -> DWORD
      {
        SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);
        const auto* exe_name = reinterpret_cast <const wchar_t*> (param);
        SK_LaunchArknightEndfield_Impl (exe_name);
        SK_Thread_CloseSelf ();
        return 0;
      }, L"[SK] AKEF Launcher",
         reinterpret_cast <LPVOID> (const_cast <wchar_t*> (wszExecutableName)));
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  
  }
}

using CreateProcess_pfn = BOOL (WINAPI*)(
  _In_opt_ LPCWSTR lpApplicationName,
  _Inout_opt_ LPWSTR lpCommandLine,
  _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
  _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
  _In_ BOOL bInheritHandles,
  _In_ DWORD dwCreationFlags,
  _In_opt_ LPVOID lpEnvironment,
  _In_opt_ LPCWSTR lpCurrentDirectory,
  _In_ LPSTARTUPINFOW lpStartupInfo,
  _Out_ LPPROCESS_INFORMATION lpProcessInformation
);

static CreateProcess_pfn CreateProcessW_Original = nullptr;

static BOOL WINAPI
AKEF_CreateProcessW_Detour (
  _In_opt_ LPCWSTR lpApplicationName,
  _Inout_opt_ LPWSTR lpCommandLine,
  _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
  _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
  _In_ BOOL bInheritHandles,
  _In_ DWORD dwCreationFlags,
  _In_opt_ LPVOID lpEnvironment,
  _In_opt_ LPCWSTR lpCurrentDirectory,
  _In_ LPSTARTUPINFOW lpStartupInfo,
  _Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
  bool isApplicationEndfield = lpApplicationName != nullptr &&
    StrStrIW (lpApplicationName, L"Endfield.exe");
  bool isCommandLineEndfield = lpCommandLine != nullptr &&
    StrStrIW (lpCommandLine, L"Endfield.exe");

  if (!isApplicationEndfield && !isCommandLineEndfield)
    return CreateProcessW_Original (
      lpApplicationName,
      lpCommandLine,
      lpProcessAttributes,
      lpThreadAttributes,
      bInheritHandles,
      dwCreationFlags,
      lpEnvironment,
      lpCurrentDirectory,
      lpStartupInfo,
      lpProcessInformation
    );

  if (lpProcessInformation == nullptr)
    return false;

  auto CleanupProcess = [&](DWORD exitCode)
  {
    if (lpProcessInformation->hProcess)
    {
      TerminateProcess (lpProcessInformation->hProcess, exitCode);
      SK_CloseHandle (lpProcessInformation->hProcess);
      lpProcessInformation->hProcess = nullptr;
    }

    if (lpProcessInformation->hThread)
    {
      SK_CloseHandle (lpProcessInformation->hThread);
      lpProcessInformation->hThread = nullptr;
    }
  };

  extern void SK_Inject_ParseWhiteAndBlacklists (const std::wstring& base_path);
  std::filesystem::path global_cfg_dir =
    std::filesystem::path (SK_GetInstallPath ()) / LR"(Global\)";
  SK_Inject_ParseWhiteAndBlacklists (global_cfg_dir);

  FARPROC ntQueryAddr = SK_GetProcAddress (L"ntdll.dll", "NtQueryInformationProcess");

  auto startTime = std::chrono::steady_clock::now ();
  constexpr auto maxWait = std::chrono::seconds (15);

  BOOL result = CreateProcessW_Original (
    lpApplicationName,
    lpCommandLine,
    lpProcessAttributes,
    lpThreadAttributes,
    true,
    dwCreationFlags | CREATE_SUSPENDED,
    lpEnvironment,
    lpCurrentDirectory,
    lpStartupInfo,
    lpProcessInformation
  );

  if (!result || lpProcessInformation->hProcess == nullptr)
  {
    CleanupProcess (0x1);
    return false;
  }

  ResumeThread (lpProcessInformation->hThread);

  while (!AKEF_IsRemoteLdrInitialized (lpProcessInformation->hProcess, ntQueryAddr))
  {
    DWORD exitCode = 0;
    if (WaitForSingleObject (lpProcessInformation->hProcess, 0) == WAIT_OBJECT_0 ||
        (GetExitCodeProcess (lpProcessInformation->hProcess, &exitCode) && exitCode != STILL_ACTIVE))
    {
      CleanupProcess (0x1);
      return false;
    }

    if (std::chrono::steady_clock::now () - startTime > maxWait)
    {
      CleanupProcess (0x1);
      return false;
    }

    SK_Sleep (5);
  }

  while (!AKEF_GetRemoteModule (lpProcessInformation->hProcess, ntQueryAddr, L"kernel32.dll"))
  {
    DWORD exitCode = 0;
    if (WaitForSingleObject (lpProcessInformation->hProcess, 0) == WAIT_OBJECT_0 ||
      (GetExitCodeProcess (lpProcessInformation->hProcess, &exitCode) && exitCode != STILL_ACTIVE))
    {
      CleanupProcess(0x1);
      return false;
    }
    if (std::chrono::steady_clock::now() - startTime > maxWait)
    {
      CleanupProcess(0x1);
      return false;
    }
    SK_Sleep(10);
  }

  std::wstring skDllPath = SK_FormatStringW (
    SK_RunLHIfBitness (64, LR"(%ws\SpecialK64.dll)", LR"(%ws\SpecialK32.dll)"),
    SK_GetInstallPath ()
  );

  UINT exitCode = 0;
  if (AKEF_InjectDLL (lpProcessInformation->hProcess, skDllPath))
  {
    SK_LOGi1 (L"SpecialK has been injected successfully.");
    AKEF_ShowTimedTaskDialog (
      L"Special K has successfully launched Arknights: Endfield",
      L"Please wait until the game has finished loading.",
      AKEF_TaskDialogTimeoutCallback,
      3000
    );
  }
  else
  {
    exitCode = 1;
    SK_LOGi1 (L"SpecialK injection failed.");
    AKEF_ShowTimedTaskDialog (
      L"Special K failed to inject into Arknights: Endfield.",
      L"Please close any running Endfield.exe or Platform.exe processes, then try again.",
      AKEF_TaskDialogTimeoutCallback,
      3000
    );
  }

  SK_CloseHandle (lpProcessInformation->hThread);
  SK_CloseHandle (lpProcessInformation->hProcess);
  return result;
}

void
SK_AKEF_InitFromLauncher (void)
{
  SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      extern void SK_Inject_ParseWhiteAndBlacklists (const std::wstring& base_path);
      SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);
      SK_AutoCOMInit _require_COM;

      SK_RunOnce (
        LPVOID CreateProcessW_target = nullptr;
        if (SK_CreateDLLHook2 (L"kernel32",
              "CreateProcessW",
              AKEF_CreateProcessW_Detour,
              static_cast_p2p <void> (&CreateProcessW_Original),
              &CreateProcessW_target) == MH_OK)
        {
          MH_EnableHook (CreateProcessW_target);
        }
      );

      SK_Thread_CloseSelf ();
      return 0;
    }, L"[SK] AKEF Launcher Init");
}

/*
    ---------------- Init Plugin ----------------
*/
static void
AKEF_PatchMutex (void* hUnity)
{
  const char* pattern = "48 83 7E ? ? 0F 95 C0 48 83 C4";

  void* addr = SK_ScanIdaStyle (hUnity, pattern);
  SK_LOGi1 (L"AKEF_PatchMutex: Scan result address: %p\n", addr);

  if (!addr)
    return;

  const uint8_t patchBytes[] = { 0xB0, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

  if (!SK_InjectMemory (addr, patchBytes, sizeof (patchBytes), PAGE_EXECUTE_READWRITE))
    SK_LOGi1 (L"Failed to inject mutex bypass patch into UnityPlayer.dll.\n");

  uint8_t verifyBytes[sizeof (patchBytes)] = { };
  ReadProcessMemory (GetCurrentProcess (), addr, reinterpret_cast <LPVOID> (verifyBytes), sizeof (verifyBytes), nullptr);

  bool match = true;
  for (size_t i = 0; i < std::size (patchBytes); ++i)
  {
    if (verifyBytes[i] != patchBytes[i])
    {
      match = false;
      break;
    }
  }

  if (!match)
  {
    SK_LOGi1 ("[AKEF_PatchMutex]: Injected bytes verification failed.\n");
    return;
  }
}

void
SK_AKEF_InitPlugin (void)
{
  SK_RunOnce (
    _SK_AKEF_OverrideUnityFramerateLimit =
      _CreateConfigParameterBool (L"ArknightEndfield.System",
        L"OverrideUnityFramerateLimit",
        __SK_AKEF_OverrideUnityFramerateLimit,
        L"Override Unity Framerate Limit");

    _SK_AKEF_KeepOriginalSwapchain =
      _CreateConfigParameterBool (L"ArknightEndfield.System",
        L"KeepOriginalSwapchain",
        __SK_AKEF_KeepOriginalSwapchain,
        L"Keep Original Swapchain");

    __g_SK_AKEF_KeepOriginalSwapchain = __SK_AKEF_KeepOriginalSwapchain;

    _SK_AKEF_UnityFramerateLimit =
      _CreateConfigParameterFloat (L"ArknightEndfield.System",
        L"UnityFramerateLimit",
        __SK_AKEF_UnityFramerateLimit,
        L"Unity Framerate Limit");

    plugin_mgr->config_fns.emplace (SK_AKEF_PlugInCfg);
    plugin_mgr->first_frame_fns.emplace (SK_AKEF_PresentFirstFrame);
  );

  SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      constexpr int kMaxRetries = 120;
      int i = 0;
      HMODULE hUnity = nullptr;

      SK_RunOnce (
        while (i < kMaxRetries)
        {
          hUnity = SK_GetModuleHandle (L"UnityPlayer.dll");
          if (hUnity != nullptr)
            break;
          ++i;
          SK_Sleep (500);
        }
      );

      if (hUnity)
        AKEF_PatchMutex (hUnity);
      else
        SK_LOGi1 ("AKEF_InitPlugin: UnityPlayer.dll not detected within timeout period.\n");

      InterlockedExchange (&__AKEF_init, 1);
      SK_Thread_CloseSelf ();
      return 0;
    }, L"[SK] Plug-In Init");
}

/*
    ---------------- Config Plugin ----------------
*/
using UnityEngine_Application_set_targetFrameRate_pfn = void (__fastcall*) (void* __this, int value);
using UnityEngine_Application_get_targetFrameRate_pfn = int  (__fastcall*) (void* __this);

using slGetPluginFunction_pfn = void* (*) (const char* functionName);
using slDLSSGSetOptions_pfn   = sl::Result (*) (const sl::ViewportHandle& viewport, sl::DLSSGOptions& options);

static UnityEngine_Application_set_targetFrameRate_pfn
UnityEngine_Application_set_targetFrameRate_Original = nullptr;

static UnityEngine_Application_get_targetFrameRate_pfn
UnityEngine_Application_get_targetFrameRate_Original = nullptr;

static slDLSSGSetOptions_pfn slDLSSGSetOptions_Original = nullptr;

static void __fastcall
AKEF_UnityEngine_Application_set_targetFrameRate_Detour (void* __this, int value)
{
  SK_LOG_FIRST_CALL
  UnityEngine_Application_set_targetFrameRate_Original (__this, value);
}

static int __fastcall
AKEF_UnityEngine_Application_get_targetFrameRate_Detour (void* __this)
{
  SK_LOG_FIRST_CALL

  if (__SK_AKEF_OverrideUnityFramerateLimit)
    return static_cast <int> (std::clamp (__SK_AKEF_UnityFramerateLimit, -1.0f, 480.0f));

  return UnityEngine_Application_get_targetFrameRate_Original (__this);
}

static sl::Result
AKEF_slDLSSGSetOptions_Detour (const sl::ViewportHandle& viewport, sl::DLSSGOptions& options)
{
  SK_LOG_FIRST_CALL

  switch (options.mode)
  {
  case sl::DLSSGMode::eOn:
    __SK_IsDLSSGActive = true;
    __SK_ForceDLSSGPacing = true;
    break;
  case sl::DLSSGMode::eOff:
    __SK_IsDLSSGActive = false;
    __SK_ForceDLSSGPacing = false;
    break;
  default:
    break;
  }

  return slDLSSGSetOptions_Original (viewport, options);
}

void
AKEF_ApplyUnityTargetFrameRate (float targetFrameRate)
{
  static float s_targetFrameRate = kDefaultFirstFramerateLimit;
  s_targetFrameRate = targetFrameRate;

  SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      Il2cpp::thread_attach (Il2cpp::get_domain ());

      static il2cpp::Wrapper wrapper;
      static auto* image =
        wrapper.get_image ("UnityEngine.CoreModule.dll") != nullptr ?
        wrapper.get_image ("UnityEngine.CoreModule.dll") :
        wrapper.get_image ("UnityEngine.dll");

      static auto* klass =
        image != nullptr ? image->get_class ("Application", "UnityEngine") : nullptr;

      static Method* set_targetFrameRate =
        klass != nullptr ? klass->get_method ("set_targetFrameRate", 1) : nullptr;

      if (set_targetFrameRate != nullptr && *(void**)set_targetFrameRate != nullptr)
      {
        SK_RunOnce (
          SK_CreateFuncHook (L"UnityEngine.Application.set_targetFrameRate",
            *(void**)set_targetFrameRate,
            AKEF_UnityEngine_Application_set_targetFrameRate_Detour,
            static_cast_p2p<void> (&UnityEngine_Application_set_targetFrameRate_Original));

          if (MH_EnableHook (*(void**)set_targetFrameRate) == MH_OK)
            SK_LOGi1 (L"Successfully hooked UnityEngine.Application.set_targetFrameRate.\n");
        );
      }

      static Method* get_targetFrameRate =
        klass != nullptr ? klass->get_method ("get_targetFrameRate", 0) : nullptr;

      if (get_targetFrameRate != nullptr && *(void**)get_targetFrameRate != nullptr)
      {
        SK_RunOnce (
          SK_CreateFuncHook (L"UnityEngine.Application.get_targetFrameRate",
            *(void**)get_targetFrameRate,
            AKEF_UnityEngine_Application_get_targetFrameRate_Detour,
            static_cast_p2p<void> (&UnityEngine_Application_get_targetFrameRate_Original));

          if (MH_EnableHook (*(void**)get_targetFrameRate) == MH_OK)
            SK_LOGi1 (L"Successfully hooked UnityEngine.Application.get_targetFrameRate.\n");
        );
      }

      if (set_targetFrameRate != nullptr && __SK_AKEF_OverrideUnityFramerateLimit)
      {
        int value = static_cast <int> (std::clamp (s_targetFrameRate, -1.0f, 480.0f));
        void* params[1] = { &value };
        Il2cpp::method_call (set_targetFrameRate, nullptr, params, nullptr);
      }

      Il2cpp::thread_detach (Il2cpp::thread_current ());
      SK_Thread_CloseSelf ();
      return 0;
    }, L"[SK] SK_AKEF_ApplyUnityTargetFrameRate");
}

bool
SK_AKEF_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Arknights: Endfield", ImGuiTreeNodeFlags_DefaultOpen))
  {
    static bool restartRequired = false;
    static bool unityFpsSeeded = false;
    bool changed = false;

    if (ImGui::Checkbox ("Override Unity Framerate Limit", &__SK_AKEF_OverrideUnityFramerateLimit))
    {
      _SK_AKEF_OverrideUnityFramerateLimit->store (__SK_AKEF_OverrideUnityFramerateLimit);
      changed = true;
    }

    if (__SK_AKEF_OverrideUnityFramerateLimit)
    {
      if (!unityFpsSeeded)
      {
        __SK_AKEF_UnityFramerateLimit =
          __SK_AKEF_UnityFramerateLimit == kDefaultFirstFramerateLimit ?
          CalcLimit (__target_fps) : __SK_AKEF_UnityFramerateLimit;

        _SK_AKEF_UnityFramerateLimit->store (__SK_AKEF_UnityFramerateLimit);

        AKEF_ApplyUnityTargetFrameRate (__SK_AKEF_UnityFramerateLimit);
        unityFpsSeeded = true;
        changed = true;
      }

      if (ImGui::SliderFloat ("Unity Framerate Limit", &__SK_AKEF_UnityFramerateLimit, -1.0f, 480.0f, "%.0f"))
      {
        _SK_AKEF_UnityFramerateLimit->store (__SK_AKEF_UnityFramerateLimit);
        AKEF_ApplyUnityTargetFrameRate (__SK_AKEF_UnityFramerateLimit);
        changed = true;
      }

      if (ImGui::BeginItemTooltip ())
      {
        ImGui::BeginGroup ();
        ImGui::TextColored (ImColor::HSV (0.18f, 0.88f, 0.94f), "  Ctrl Click");
        ImGui::TextColored (ImColor::HSV (0.18f, 0.88f, 0.94f), " ->");
        ImGui::EndGroup ();

        ImGui::SameLine ();

        ImGui::BeginGroup ();
        ImGui::TextUnformatted (" Enter an Exact Framerate");
        ImGui::TextUnformatted (" Set to -1 for unlimited.");
        ImGui::EndGroup ();

        ImGui::EndTooltip ();
      }
    }

    if (ImGui::Checkbox ("Keep Original Swapchain", &__SK_AKEF_KeepOriginalSwapchain))
    {
      _SK_AKEF_KeepOriginalSwapchain->store (__SK_AKEF_KeepOriginalSwapchain);
      changed = true;
      restartRequired = true;
    }

    if (restartRequired)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
      ImGui::BulletText ("Game Restart Required");
      ImGui::PopStyleColor ();
    }

    if (changed)
      config.utility.save_async ();
  }

  return true;
}

/*
    ---------------- First Frame Plugin ----------------
*/
HRESULT STDMETHODCALLTYPE
SK_AKEF_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  SK_RunOnce (
    SK_Thread_CreateEx ([](LPVOID)->DWORD
      {
        while (0 == InterlockedAdd (&__AKEF_init, 0))
          SK_Sleep (1000);

        while (SK_GetFramesDrawn () < 20)
          SK_Sleep (250);

        AKEF_ApplyUnityTargetFrameRate (
          __SK_AKEF_UnityFramerateLimit == kDefaultFirstFramerateLimit ?
          CalcLimit (__target_fps) : __SK_AKEF_UnityFramerateLimit
        );

        constexpr int  kMaxAttempts  = 20;
        constexpr DWORD kRetryDelayMs = 1000;
        static HMODULE hModSLDLSSG = nullptr;

        for (int i = 0; i < kMaxAttempts && !hModSLDLSSG; ++i)
        {
          if (!GetModuleHandleExW (
            GET_MODULE_HANDLE_EX_FLAG_PIN,
            L"sl.dlss_g.dll",
            &hModSLDLSSG))
          {
            SK_Sleep (kRetryDelayMs);
          }
        }

        if (hModSLDLSSG != nullptr)
        {
          auto slGetPluginFunction =
            reinterpret_cast <slGetPluginFunction_pfn> (
              SK_GetProcAddress (hModSLDLSSG, "slGetPluginFunction"));

          if (slGetPluginFunction == nullptr)
          {
            SK_LOGi1 (L"Failed to find slGetPluginFunction.\n");
            return 1;
          }

          auto slDLSSGSetOptions =
            reinterpret_cast <slDLSSGSetOptions_pfn> (slGetPluginFunction ("slDLSSGSetOptions"));

          if (slDLSSGSetOptions == nullptr)
          {
            SK_LOGi1 (L"Failed to find slDLSSGSetOptions.\n");
            return 1;
          }

          if (SK_CreateFuncHook (L"AKEF_slDLSSGSetOptions",
            slDLSSGSetOptions,
            AKEF_slDLSSGSetOptions_Detour,
            static_cast_p2p<void> (&slDLSSGSetOptions_Original)) != MH_OK)
          {
            SK_LOGi1 (L"Failed to create hook for slDLSSGSetOptions.\n");
          }

          if (MH_EnableHook (slDLSSGSetOptions) != MH_OK)
            SK_LOGi1 (L"Failed to enable hook for slDLSSGSetOptions.\n");
        }

        SK_Thread_CloseSelf ();
        return 0;
      }, L"[SK] SK_AKEF_PresentFirstFrame");
  );

  return S_OK;
}