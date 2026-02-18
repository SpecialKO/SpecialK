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
#include <SpecialK/adl.h>
#include <SpecialK/utility.h>
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
#include <sal.h>
#include <variant>
#include <vulkan/vulkan.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Arknights Endfield"

/*
  Shared Memory
*/
namespace SK::ArknightsEndfield
{
  #pragma warning (push)
  #pragma warning (disable: 4324)

  struct alignas (std::hardware_destructive_interference_size) SharedMemory
  {
    volatile LONG has_pid;
    DWORD         akef_launch_pid;
  };

  #pragma warning (pop)

  static HANDLE        g_hMap     = INVALID_HANDLE_VALUE;
  static SharedMemory* g_pView    = nullptr;
  static LONG          g_isNewMap = FALSE;

  static constexpr wchar_t kSharedMapName[]          = LR"(Local\SK_AKEF_SharedMemory_v1)";
  static constexpr wchar_t kSharedMemoryMutexName[]  = LR"(Local\SK_AKEF_SharedMemoryMutex)";
  static constexpr wchar_t kProcessMutexName[]       = LR"(Local\SK_AKEF_ProcessMutex)";

  static HANDLE           GetSharedMemory     (void);
  static SharedMemory*    GetView             (void);
  void                    PublishPid          (DWORD pid);
  void                    ResetPid            (void);
  bool                    TryGetPid           (DWORD* out_pid);
  void                    CleanupSharedMemory (void);

  struct AutoMutex
  {
    HANDLE m_h      = nullptr;
    bool   m_locked = false;

    AutoMutex () = default;

    explicit AutoMutex (SK_AutoHandle& h, DWORD timeout = INFINITE) noexcept (false)
      : m_h (static_cast <HANDLE> (h))
    {
      lock (timeout);
    }

    ~AutoMutex ()
    {
      unlock ();
    }

    bool lock (DWORD timeout = INFINITE) noexcept
    {
      if (!m_h || m_locked)
        return false;

      const DWORD result = WaitForSingleObject (m_h, timeout);

      if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED)
      {
        m_locked = true;
        return true;
      }

      return false;
    }

    void unlock () noexcept
    {
      if (m_locked)
      {
        __try
        {
          ReleaseMutex (m_h);
          m_locked = false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
          std::ignore = m_locked;
        }
      }
    }

    bool owns_lock () const noexcept { return m_locked; }

    AutoMutex (const AutoMutex&)            = delete;
    AutoMutex& operator= (const AutoMutex&) = delete;
    AutoMutex (AutoMutex&&)                 = delete;
    AutoMutex& operator= (AutoMutex&&)      = delete;
  };
}


void SK_AKEF_PublishPid          (DWORD pid);
bool SK_AKEF_TryGetPid           (DWORD* out_pid);
void SK_AKEF_ResetPid            (void);
void SK_AKEF_CleanupSharedMemory (void);


static HANDLE
SK::ArknightsEndfield::GetSharedMemory (void)
{
  HANDLE hLocalMap =
    InterlockedCompareExchangePointer (&g_hMap, nullptr, nullptr);

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
      InterlockedExchange (&g_isNewMap, FALSE);
      hExistingFile.m_h = nullptr;
      return g_hMap;
    }

    hExistingFile.Close ();
  }

  SK_AutoHandle hNewFile (
    CreateFileMappingW (INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof (SharedMemory), kSharedMapName)
  );

  if (hNewFile.isValid ())
  {
    const DWORD last_error = GetLastError ();

    if (INVALID_HANDLE_VALUE ==
        InterlockedCompareExchangePointer (&g_hMap, hNewFile.m_h, INVALID_HANDLE_VALUE))
    {
      InterlockedExchange (&g_isNewMap, last_error == ERROR_ALREADY_EXISTS ? FALSE : TRUE);

      hNewFile.m_h = nullptr;
      return g_hMap;
    }

    hNewFile.Close ();
  }

  return nullptr;
}

static SK::ArknightsEndfield::SharedMemory*
SK::ArknightsEndfield::GetView (void)
{
  SharedMemory* local_view = static_cast <SharedMemory*> (
    InterlockedCompareExchangePointer (reinterpret_cast <volatile PVOID*> (&g_pView), nullptr, nullptr));

  if (local_view != nullptr)
    return local_view;

  HANDLE hMap = GetSharedMemory ();
  if (!hMap)
    return nullptr;

  local_view = static_cast <SharedMemory*> (
    MapViewOfFile (hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof (SharedMemory))
  );

  if (!local_view)
    return nullptr;

  SharedMemory* expected = nullptr;

  if (InterlockedCompareExchangePointer (
        reinterpret_cast <volatile PVOID*> (&g_pView),
        local_view,
        expected) != expected)
  {
    UnmapViewOfFile (local_view);
  }
  else if (InterlockedCompareExchange (&g_isNewMap, 0, 0) != 0)
  {
    local_view->has_pid        = 0;
    local_view->akef_launch_pid = 0;
  }

  return g_pView;
}

void
SK::ArknightsEndfield::PublishPid (DWORD pid)
{
  if (SharedMemory* shm = GetView ())
  {
    shm->akef_launch_pid = pid;
    InterlockedExchange (&shm->has_pid, 1);
  }
}

void
SK::ArknightsEndfield::ResetPid (void)
{
  if (SharedMemory* shm = GetView ())
  {
    InterlockedExchange (&shm->has_pid, 0);
    shm->akef_launch_pid = 0;
  }
}

bool
SK::ArknightsEndfield::TryGetPid (DWORD* out_pid)
{
  if (out_pid == nullptr)
    return false;

  if (SharedMemory* shm = GetView ())
  {
    if (InterlockedCompareExchange (&shm->has_pid, 1, 1) == 1)
    {
      *out_pid = shm->akef_launch_pid;
      return true;
    }
  }

  return false;
}

void
SK::ArknightsEndfield::CleanupSharedMemory (void)
{
  SharedMemory* view = static_cast <SharedMemory*> (
    InterlockedCompareExchangePointer (reinterpret_cast <volatile PVOID*> (&g_pView), nullptr, nullptr));

  if (view != nullptr &&
      InterlockedCompareExchangePointer (reinterpret_cast <volatile PVOID*> (&g_pView), nullptr, view) == view)
  {
    UnmapViewOfFile (view);
  }

  HANDLE hLocalMap = InterlockedExchangePointer (&g_hMap, INVALID_HANDLE_VALUE);

  if (hLocalMap != INVALID_HANDLE_VALUE && hLocalMap != nullptr)
  {
    SK_AutoHandle (hLocalMap).Close ();
  }
}


void SK_AKEF_PublishPid (DWORD pid)
{
  SK_AutoHandle hMutex (CreateMutexW (nullptr, FALSE, SK::ArknightsEndfield::kSharedMemoryMutexName));

  if (!hMutex.isValid ())
    return;

  SK::ArknightsEndfield::AutoMutex lock (hMutex, INFINITE);

  if (!lock.owns_lock ())
    return;

  SK::ArknightsEndfield::PublishPid (pid);
}


bool SK_AKEF_TryGetPid (DWORD* out_pid)
{
  return SK::ArknightsEndfield::TryGetPid (out_pid);
}

void SK_AKEF_ResetPid (void)
{
  SK_AutoHandle hMutex(CreateMutexW(nullptr, FALSE, SK::ArknightsEndfield::kSharedMemoryMutexName));

  if (!hMutex.isValid ())
    return;

  SK::ArknightsEndfield::AutoMutex lock (hMutex, INFINITE);

  if (!lock.owns_lock ())
    return;

  SK::ArknightsEndfield::ResetPid ();
}

void SK_AKEF_CleanupSharedMemory (void)
{
  SK_AutoHandle hMutex(CreateMutexW(nullptr, FALSE, SK::ArknightsEndfield::kSharedMemoryMutexName));

  if (!hMutex.isValid ())
    return;

  SK::ArknightsEndfield::AutoMutex lock (hMutex, INFINITE);

  if (!lock.owns_lock ())
    return;

  SK::ArknightsEndfield::CleanupSharedMemory ();
}

/*
  Utilities
*/

extern HRESULT WINAPI
SK_TaskDialogIndirect (
  _In_        const TASKDIALOGCONFIG* pTaskConfig,
  _Out_opt_   int* pnButton,
  _Out_opt_   int* pnRadioButton,
  _Out_opt_   BOOL* pfVerificationFlagChecked
);

extern void            SK_Inject_ParseWhiteAndBlacklists (const std::wstring& base_path);
extern PROCESSENTRY32W FindProcessByName                 (const wchar_t* wszName);
extern void            SK_Inject_ParseWhiteAndBlacklists (const std::wstring& base_path);

namespace SK::ArknightsEndfield::Utils
{
  struct window_info_s
  {
    HWND         hwnd = nullptr;
    std::wstring title;
    std::wstring class_name;
  };

  struct window_context_s
  {
    DWORD                      pid; 
    std::vector <window_info_s> windows; 
  };

  using NtQueryInformationProcess_t = LONG (WINAPI*) (
    HANDLE ProcessHandle,
    ULONG  ProcessInformationClass,
    PVOID  ProcessInformation,
    ULONG  ProcessInformationLength,
    PULONG ReturnLength
  );

  static HRESULT CALLBACK
  TaskDialogTimeoutCallback (
    _In_ HWND     hWnd,
    _In_ UINT     uNotification,
    _In_ WPARAM   wParam,
    _In_ LPARAM   lParam,
    _In_ LONG_PTR dwRefData
  );

  static HRESULT ShowTimedTaskDialog (PCWSTR mainInstruction, PCWSTR content, LPVOID callback, LONG_PTR callbackData);

  static BOOL CALLBACK EnumWindowsProc (HWND hwnd, LPARAM lParam);

  static bool WalkPebLdr (HANDLE hProcess, FARPROC ntQueryAddr, PROCESS_BASIC_INFORMATION& out_pbi, PEB& out_peb, PEB_LDR_DATA_SK& out_ldr);

  static std::wstring GetProcessCommandLine (HANDLE hProcess, FARPROC ntQueryAddr);

  static HMODULE GetRemoteModule (HANDLE hProcess, FARPROC ntQueryAddr, const std::wstring& lowercaseModName, bool fullPath = false);

  static bool InjectDLL (HANDLE hTargetProc, const std::wstring& dllPath);
}


static HRESULT
SK::ArknightsEndfield::Utils::TaskDialogTimeoutCallback (
    _In_ HWND     hWnd,
    _In_ UINT     uNotification,
    _In_ WPARAM   wParam,
    _In_ LPARAM   lParam,
    _In_ LONG_PTR dwRefData
)
{
  UNREFERENCED_PARAMETER (lParam);

  if (uNotification == TDN_TIMER)
  {
    const auto timeout_ms = gsl::narrow_cast <DWORD> (dwRefData);
    const auto elapsed_ms = gsl::narrow_cast <DWORD> (wParam);

    if (elapsed_ms >= timeout_ms)
      SendMessageW (hWnd, TDM_CLICK_BUTTON, IDOK, 0);
  }

  return S_OK;
}

static HRESULT
SK::ArknightsEndfield::Utils::ShowTimedTaskDialog (
  PCWSTR mainInstruction,
  PCWSTR content,
  LPVOID callback,
  LONG_PTR callbackData
)
{
  static const std::wstring ver_str =
    SK_FormatStringW (L"Special K (v %ws)", SK_GetVersionStrW ());

  TASKDIALOGCONFIG cfg;
  ZeroMemory (&cfg, sizeof (cfg));
  cfg.cbSize = sizeof (cfg);

  cfg.dwFlags =
    TDF_CALLBACK_TIMER |
    TDF_ALLOW_DIALOG_CANCELLATION;

  cfg.dwCommonButtons = TDCBF_OK_BUTTON;

  cfg.pszWindowTitle      = ver_str.c_str ();
  cfg.pszMainInstruction  = mainInstruction;
  cfg.pszContent          = content;

  cfg.pfCallback          = static_cast <PFTASKDIALOGCALLBACK> (callback);
  cfg.lpCallbackData      = callbackData;

  return SK_TaskDialogIndirect (&cfg, nullptr, nullptr, nullptr);
}

static BOOL CALLBACK
SK::ArknightsEndfield::Utils::EnumWindowsProc (HWND hwnd, LPARAM lParam)
{
  auto* pData = reinterpret_cast <window_context_s*> (lParam);
  if (pData == nullptr)
    return FALSE;

  DWORD windowPID = 0;
  GetWindowThreadProcessId (hwnd, &windowPID);

  if (windowPID == pData->pid)
  {
    std::wstring title (256, L'\0');
    std::wstring class_name (256, L'\0');

    GetWindowTextW (hwnd, title.data (), gsl::narrow_cast <int> (title.size ()));
    GetClassNameW (hwnd, class_name.data (), gsl::narrow_cast <int> (class_name.size ()));

    pData->windows.push_back ({ hwnd, title, class_name });
  }

  return TRUE;
}

static bool
SK::ArknightsEndfield::Utils::WalkPebLdr (HANDLE hProcess, FARPROC ntQueryAddr, PROCESS_BASIC_INFORMATION& out_pbi, PEB& out_peb, PEB_LDR_DATA_SK& out_ldr)
{
  static NtQueryInformationProcess_t NtQueryInformationProcess = nullptr;
  if (!NtQueryInformationProcess)
    NtQueryInformationProcess = reinterpret_cast <NtQueryInformationProcess_t> (ntQueryAddr);

  if (NtQueryInformationProcess (hProcess, ProcessBasicInformation, &out_pbi, sizeof (out_pbi), nullptr) != 0)
    return false;

  if (!ReadProcessMemory (hProcess, out_pbi.PebBaseAddress, &out_peb, sizeof (out_peb), nullptr))
    return false;

  if (!out_peb.Ldr)
    return false;

  if (!ReadProcessMemory (hProcess, out_peb.Ldr, &out_ldr, sizeof (out_ldr), nullptr))
    return false;

  if (!out_ldr.Initialized)
    return false;

  return true;
}

static std::wstring
SK::ArknightsEndfield::Utils::GetProcessCommandLine (HANDLE hProcess, FARPROC ntQueryAddr)
{
  if (hProcess == nullptr)
    return L"";

  PROCESS_BASIC_INFORMATION pbi = { };
  PEB peb = { };
  PEB_LDR_DATA_SK ldr = { };

  if (!WalkPebLdr (hProcess, ntQueryAddr, pbi, peb, ldr))
    return L"";

  RTL_USER_PROCESS_PARAMETERS params = { };
  if (!ReadProcessMemory (hProcess, peb.ProcessParameters, &params, sizeof (params), nullptr))
    return L"";

  std::wstring cmd;
  cmd.resize (params.CommandLine.Length / sizeof (wchar_t));
  ReadProcessMemory (hProcess, params.CommandLine.Buffer, cmd.data (), params.CommandLine.Length, nullptr);
  return cmd;
}

static HMODULE
SK::ArknightsEndfield::Utils::GetRemoteModule (
  HANDLE hProcess,
  FARPROC ntQueryAddr,
  const std::wstring& lowercaseModName,
  bool fullPath
)
{
  PROCESS_BASIC_INFORMATION pbi = { };
  PEB peb = { };
  PEB_LDR_DATA_SK ldr = { };

  if (!WalkPebLdr (hProcess, ntQueryAddr, pbi, peb, ldr))
    return nullptr;

  auto ReadRemoteUnicodeString = [hProcess](const UNICODE_STRING_SK& us, std::wstring& out) -> bool
  {
    if (!us.Buffer || us.Length == 0) return false;
    constexpr USHORT kMaxLen = 0x2000;
    if (us.Length > kMaxLen) return false;
    out.resize (us.Length / sizeof (wchar_t));
    return ReadProcessMemory (hProcess, us.Buffer, out.data (), us.Length, nullptr) != FALSE;
  };

  const auto* listHead =
    reinterpret_cast <LIST_ENTRY*> (reinterpret_cast <std::uintptr_t> (peb.Ldr) +
      offsetof (PEB_LDR_DATA_SK, InLoadOrderModuleList)
    );

  LIST_ENTRY* curr = ldr.InLoadOrderModuleList.Flink;

  while (curr && curr != listHead)
  {
    LDR_DATA_TABLE_ENTRY__SK entry;
    ZeroMemory (&entry, sizeof (entry));

    if (!ReadProcessMemory (hProcess, curr, &entry, sizeof (entry), nullptr))
      break;

    std::wstring name;
    if (ReadRemoteUnicodeString (fullPath ? entry.FullDllName : entry.BaseDllName, name))
    {
      std::transform (name.begin (), name.end (), name.begin (), ::towlower);
      if (name == lowercaseModName)
        return static_cast <HMODULE> (entry.DllBase);

      if (fullPath && StrStrIW (name.c_str (), lowercaseModName.c_str ()) != nullptr)
        return static_cast <HMODULE> (entry.DllBase);
    }

    curr = entry.InLoadOrderLinks.Flink;
  }

  return nullptr;
}

static bool
SK::ArknightsEndfield::Utils::InjectDLL (HANDLE hTargetProc, const std::wstring& dllPath)
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

  SK_AutoHandle hThread (
    CreateRemoteThread (
      hTargetProc, nullptr, 0,
      static_cast <LPTHREAD_START_ROUTINE> (loadLibraryAddr),
      dllPathAddr, 0, nullptr
    )
  );

  if (!hThread.isValid ())
  {
    VirtualFreeEx (hTargetProc, dllPathAddr, 0, MEM_RELEASE);
    return false;
  }

  WaitForSingleObject (hThread, INFINITE);

  DWORD exitCode = 0;
  GetExitCodeThread (hThread, &exitCode);

  VirtualFreeEx (hTargetProc, dllPathAddr, 0, MEM_RELEASE);
  return exitCode != 0;
}

std::vector<DWORD>
SK_AKEF_GetExistingEndfieldPids (void)
{
  std::vector<DWORD> existingPids;
  SK_AutoHandle snapshot (CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0));

  if (!snapshot.isValid ())
    return { };

  PROCESSENTRY32W processEntry;
  ZeroMemory (&processEntry, sizeof (processEntry));
  processEntry.dwSize = sizeof (PROCESSENTRY32W);

  if (Process32FirstW (snapshot, &processEntry))
  {
    do
    {
      const std::wstring_view szExeFile (processEntry.szExeFile, wcsnlen (processEntry.szExeFile, MAX_PATH));
      if (_wcsicmp (szExeFile.data (), L"Endfield.exe") == 0)
      {
        existingPids.push_back (processEntry.th32ProcessID);
      }
    } while (Process32NextW (snapshot, &processEntry));
  }

  return existingPids;
}


/*
  Global Variables and Plugin Settings (Hooks, CodePatches, etc.)
*/

// Update the struct-layout if reflex.cpp is updated
extern struct SK_VK_REFLEX {
  VkDevice       device;
  VkSwapchainKHR swapchain;
  uint64_t       last_frame;
  VkSemaphore    NvLL_semaphore;
};

extern struct NVLL_VK_SET_SLEEP_MODE_PARAMS {
  bool     bLowLatencyMode;
  bool     bLowLatencyBoost;
  uint32_t minimumIntervalUs;
};

extern DWORD NvLL_VK_SetSleepMode_Detour(VkDevice device, NVLL_VK_SET_SLEEP_MODE_PARAMS* sleepModeParams);
extern SK_VK_REFLEX SK_VK_Reflex;
extern NVLL_VK_SET_SLEEP_MODE_PARAMS SK_NVLL_LastSleepParams;
extern PFN_vkWaitSemaphores vkWaitSemaphores_SK = nullptr;

bool  __g_SK_AKEF_KeepOriginalSwapchain = false;

namespace SK::ArknightsEndfield
{
  static constexpr float DefaultFirstFramerateLimit = -99.0f;

  enum class OverrideType
  {
    FrameRate
  };

  bool  b_OverrideUnityFramelimit = false;
  bool  b_KeepOriginalSwapchain = false;
  float f_UnityFramelimit = DefaultFirstFramerateLimit;

  struct {
    sk::ParameterBool* override_unity_framelimit = nullptr;
    sk::ParameterBool* keep_original_swapchain = nullptr;
    sk::ParameterFloat* unity_frameratelimit = nullptr;
  } ini;

  std::atomic<SK_RenderAPI> render_api { SK_RenderAPI::None };
  volatile LONG init = FALSE;

  template <std::size_t N>
  struct CodePatch
  {
    void* address = nullptr;
    std::wstring description;

    boost::container::static_vector<uint8_t, N> original    { };
    boost::container::static_vector<uint8_t, N> replacement { };

    void applyReplacement (bool store_original = true);
  };

  static void LaunchArknightsEndfield_Impl (const wchar_t* wszExecutableName);
  static void ApplyUnityOverride (const OverrideType& overrideType, const std::variant<int, float, bool>& value);

  namespace Hooks
  {
    // From nvidia streamline sl.chi/nvllvk.cpp
    struct alignas(8) NvLowLatencyVk {
      void*       vtable;                   // 0x0000
      VkDevice    device;                   // 0x0008
      char        pad_0010[0xEA8 - 0x10];   // 0x0010 - 0xEA7
      VkSemaphore m_lowLatencySemaphore;    // 0xEA8
      uint64_t    reflexSemaphoreValue;     // 0xEB0
      HMODULE     m_hmodReflex;             // 0xEB8
    };

    // Not sure if these needed, but to prevent compiler re-arrange struct members order/offset
    static_assert(offsetof(NvLowLatencyVk, device) == 0x8, "device offset mismatch");
    static_assert(offsetof(NvLowLatencyVk, m_lowLatencySemaphore) == 0xEA8, "Semaphore offset mismatch");
    static_assert(offsetof(NvLowLatencyVk, reflexSemaphoreValue) == 0xEB0, "reflexSemaphoreValue offset mismatch");
    static_assert(offsetof(NvLowLatencyVk, m_hmodReflex) == 0xEB8, "m_hmodReflex offset mismatch");
    static_assert(sizeof(NvLowLatencyVk) == 0xEC0, "NvLowLatencyVk size mismatch");

    using UnityEngine_Application_set_targetFrameRate_pfn = void (__fastcall*) (void* __this, int value);
    using UnityEngine_Application_get_targetFrameRate_pfn = int (__fastcall*) (void* __this);

    using slCommon_ReflexSleep_pfn = int (__fastcall*) (NvLowLatencyVk* nvLowLatencyVk);
    using NvLL_VK_Sleep_pfn        = int (__fastcall*) (VkDevice device, uint64_t reflexSemaphoreValue);

    using slGetPluginFunction_pfn = void* (*) (const char* functionName);
    using slDLSSGSetOptions_pfn = sl::Result (*) (const sl::ViewportHandle& viewport, sl::DLSSGOptions& options);

    using CreateProcess_pfn = BOOL (WINAPI*) (
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

    static UnityEngine_Application_set_targetFrameRate_pfn
    UnityEngine_Application_set_targetFrameRate_Original = nullptr;

    static UnityEngine_Application_get_targetFrameRate_pfn
    UnityEngine_Application_get_targetFrameRate_Original = nullptr;

    static slDLSSGSetOptions_pfn slDLSSGSetOptions_Original     = nullptr;
    static slDLSSGSetOptions_pfn slDLSSGSetOptions_OTA_Original = nullptr;

    static slCommon_ReflexSleep_pfn slCommon_ReflexSleep_Original     = nullptr;
    static slCommon_ReflexSleep_pfn slCommon_ReflexSleep_OTA_Original = nullptr;

    static CreateProcess_pfn CreateProcessW_Original = nullptr;

    static BOOL WINAPI CreateProcessW_Detour (_In_opt_ LPCWSTR, _Inout_opt_ LPWSTR, _In_opt_ LPSECURITY_ATTRIBUTES,
      _In_opt_ LPSECURITY_ATTRIBUTES, _In_ BOOL, _In_ DWORD, _In_opt_ LPVOID, _In_opt_ LPCWSTR, _In_ LPSTARTUPINFOW,
      _Out_ LPPROCESS_INFORMATION);

    static void __fastcall
    UnityEngine_Application_set_targetFrameRate_Detour (void* __this, int value)
    {
      SK_LOG_FIRST_CALL

      UnityEngine_Application_set_targetFrameRate_Original (__this, value);
    }

    static int __fastcall
    UnityEngine_Application_get_targetFrameRate_Detour (void* __this)
    {
      SK_LOG_FIRST_CALL

      if (b_OverrideUnityFramelimit)
        return static_cast <int> (std::clamp (f_UnityFramelimit, -1.0f, 600.0f));

      return UnityEngine_Application_get_targetFrameRate_Original (__this);
    }

    static sl::Result
    slDLSSGSetOptions_Detour (const sl::ViewportHandle& viewport, sl::DLSSGOptions& options)
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

    static sl::Result
    slDLSSGSetOptions_OTA_Detour (const sl::ViewportHandle& viewport, sl::DLSSGOptions& options)
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
      return slDLSSGSetOptions_OTA_Original (viewport, options);
    }

    __forceinline int __fastcall
    NvLL_VK_Sleep (const NvLL_VK_Sleep_pfn NvLL_VK_Sleep_Original, NvLowLatencyVk* nvLowLatencyVk)
    {
      ++nvLowLatencyVk->reflexSemaphoreValue;

      const int ret = NvLL_VK_Sleep_Original (nvLowLatencyVk->device, nvLowLatencyVk->reflexSemaphoreValue);

      if (ret == 0)
      {
        VkSemaphoreWaitInfo waitInfo { };
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.pNext = NULL;
        waitInfo.flags = 0;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &nvLowLatencyVk->m_lowLatencySemaphore;
        waitInfo.pValues = &nvLowLatencyVk->reflexSemaphoreValue;

        // wait for 500ms (same as original sl.common code)
        const auto result = vkWaitSemaphores_SK (nvLowLatencyVk->device, &waitInfo, 500000000);
        if (result == VK_TIMEOUT)
        {
          SK_LOGi0(L"Timeout while waiting (100 ms) for Reflex semaphore.");
          config.nvidia.reflex.use_limiter = false;
        }
        else if (result == VK_SUCCESS)
        {
          auto& rb =
            SK_GetCurrentRenderBackend ();

          if (config.render.framerate.enforcement_policy == 2 && rb.vulkan_reflex.isPacingEligible ())
            SK::Framerate::Tick (true, 0.0, { 0,0 }, rb.swapchain.p);         
        }
      }
      return ret;
    }

    static int __fastcall
    slCommon_ReflexSleep_Detour (NvLowLatencyVk* nvLowLatencyVk)
    {
      SK_LOG_FIRST_CALL

      SK_VK_Reflex.device = nvLowLatencyVk->device;
      if (config.nvidia.reflex.override || __SK_ForceDLSSGPacing)
        NvLL_VK_SetSleepMode_Detour (nvLowLatencyVk->device, &SK_NVLL_LastSleepParams);
      
      static NvLL_VK_Sleep_pfn NvLL_VK_Sleep_Original = nullptr;
      if (NvLL_VK_Sleep_Original == nullptr)
      {
        if (nvLowLatencyVk->m_hmodReflex != nullptr)
        {
          NvLL_VK_Sleep_Original = reinterpret_cast <NvLL_VK_Sleep_pfn> (
            SK_GetProcAddress (nvLowLatencyVk->m_hmodReflex, "NvLL_VK_Sleep"));

          if (NvLL_VK_Sleep_Original == nullptr)
            return slCommon_ReflexSleep_Original (nvLowLatencyVk);
        }
      }

      return NvLL_VK_Sleep (NvLL_VK_Sleep_Original, nvLowLatencyVk);
    }

    static int __fastcall
    slCommon_ReflexSleep_OTA_Detour (NvLowLatencyVk* nvLowLatencyVk)
    {
      SK_LOG_FIRST_CALL

      if (config.nvidia.reflex.override || __SK_ForceDLSSGPacing)
          NvLL_VK_SetSleepMode_Detour (nvLowLatencyVk->device, &SK_NVLL_LastSleepParams);

        static NvLL_VK_Sleep_pfn NvLL_VK_Sleep_Original = nullptr;
      if (NvLL_VK_Sleep_Original == nullptr)
      {
        if (nvLowLatencyVk->m_hmodReflex != nullptr)
        {
          NvLL_VK_Sleep_Original = reinterpret_cast <NvLL_VK_Sleep_pfn> (
            SK_GetProcAddress (nvLowLatencyVk->m_hmodReflex, "NvLL_VK_Sleep"));

          if (NvLL_VK_Sleep_Original == nullptr)
            return slCommon_ReflexSleep_OTA_Original (nvLowLatencyVk);
        }
      }

      return NvLL_VK_Sleep (NvLL_VK_Sleep_Original, nvLowLatencyVk);
    }
  }

  template<std::size_t N>
  void CodePatch<N>::applyReplacement (bool store_original)
  {
    if (address == nullptr || replacement.empty ())
      return;

    if (store_original)
    {
      original.resize (replacement.size ());
      std::memcpy (original.data (), address, replacement.size ());

      std::wstring hexStr;
      for (const auto& byte : original)
      {
        hexStr += std::format (L"{:02X} ", byte);
      }
      SK_LOGi1 (L"Stored original bytes for patch %s: %s\n", description.c_str (), hexStr.c_str ());
    }

    if (!SK_InjectMemory (address, replacement.data (), replacement.size (), PAGE_EXECUTE_READWRITE))
      SK_LOGi1 (L"Failed to apply patch: %s\n", description.c_str ());

    std::vector<uint8_t> verifyBytes (replacement.size ());
    std::memcpy (verifyBytes.data (), address, replacement.size ());

    if (!std::equal (verifyBytes.begin (), verifyBytes.end (), replacement.begin ()))
      SK_LOGi1 (L"Injected bytes verification failed in addr: %p\n", address);
    else
      FlushInstructionCache (GetCurrentProcess (), address, replacement.size ());
  }
}


static void
SK::ArknightsEndfield::LaunchArknightsEndfield_Impl (const wchar_t* wszExecutableName)
{
  Utils::ShowTimedTaskDialog (
    L"Special K is preparing to launch a new Arknights Endfield process",
    L"The original process will be closed automatically.\n\n"
    L"Consider injecting SK to the original Endfield Launcher.\n"
    L"SK will automatically inject itself into the new process created by the launcher.\n",
    Utils::TaskDialogTimeoutCallback,
    4000
  );

  auto ctx = SK_SuspendAllOtherThreads ();
  SK_Sleep (1500);

  const DWORD origEndfieldPID = FindProcessByName (L"Endfield.exe").th32ProcessID;

  Utils::window_context_s data = { origEndfieldPID, { } };
  SK_LOGi0 (L"Found original Arknight: Endfield PID: %ul\n", origEndfieldPID);
  EnumWindows (Utils::EnumWindowsProc, reinterpret_cast <LPARAM> (&data));

  STARTUPINFOW        sinfo = { };
  PROCESS_INFORMATION pinfo = { };
  BOOL                created = FALSE;

  sinfo.cb = sizeof (STARTUPINFOW);
  sinfo.wShowWindow = SW_SHOWNORMAL;
  sinfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;

  auto CleanupProcess = [&](DWORD exitCode)
    {
      ResetPid ();
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
  SK_LOGi0 (L"DLL Path: %s, hostPath: %s\n", skDllPath.c_str (), SK_GetHostPath ());

  SK_AutoHandle hOrigEndfield (OpenProcess (
    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
    FALSE,
    origEndfieldPID
  ));

  if (!hOrigEndfield.isValid ())
  {
    SK_ResumeThreads (ctx);
    return;
  }

  FARPROC ntQueryAddr = SK_GetProcAddress (L"ntdll.dll", "NtQueryInformationProcess");
  std::wstring cmdLine = Utils::GetProcessCommandLine (hOrigEndfield, ntQueryAddr);
  hOrigEndfield.Close ();

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

  PublishPid (pinfo.dwProcessId);
  SK_LOGi0 (L"Launching Arknight: Endfield with PID: %ul\n", pinfo.dwProcessId);

  SK_SelfDestruct ();
  SK_ResumeThreads (ctx);

  if (pinfo.hProcess == nullptr || pinfo.hThread == nullptr)
  {
    Utils::ShowTimedTaskDialog (
      L"Special K failed to launch Arknights: Endfield.",
      L"The game may not launch correctly.",
      Utils::TaskDialogTimeoutCallback,
      4000
    );
    CleanupProcess (0x1);
    SK_TerminateProcess (0x1);
    return;
  }

  const auto startTime = std::chrono::steady_clock::now ();
  constexpr auto maxWait = std::chrono::seconds (15);
  ResumeThread (pinfo.hThread);

  while (!Utils::GetRemoteModule (pinfo.hProcess, ntQueryAddr, L"kernel32.dll"))
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
    SK_Sleep (20);
  }

  UINT exitCode = 0;
  if (Utils::InjectDLL (pinfo.hProcess, skDllPath))
  {
    SK_LOGi0 (L"SpecialK has been injected successfully.");
  }
  else
  {
    exitCode = 1;
    SK_LOGi0 (L"SpecialK injection failed.");
    Utils::ShowTimedTaskDialog (
      L"Special K failed to inject into Arknights: Endfield.",
      L"The game may not launch correctly.",
      Utils::TaskDialogTimeoutCallback,
      4000
    );
    ResetPid ();
  }

  SK_CloseHandle (pinfo.hThread);
  SK_CloseHandle (pinfo.hProcess);

  for (const auto& window : data.windows)
  {
    if (IsWindow (window.hwnd))
      PostMessageW (window.hwnd, WM_CLOSE, 0, 0);
  }

  SK_AutoHandle hProcess (OpenProcess (SYNCHRONIZE | PROCESS_TERMINATE, FALSE, origEndfieldPID));
  if (hProcess.isValid ())
  {
    WaitForSingleObject (hProcess, 200);
    TerminateProcess (hProcess, 1);
    hProcess.Close ();
  }
  SK_TerminateProcess (exitCode);
}

void
SK_LaunchArknightsEndfield (const wchar_t* wszExecutableName)
{
  SK_Thread_CreateEx ([](LPVOID param) -> DWORD
    {
      try {
        SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);
        SK_AutoHandle hMutex (CreateMutexW (nullptr, FALSE, SK::ArknightsEndfield::kProcessMutexName));
        if (!hMutex.isValid ())
        {
          SK_Thread_CloseSelf ();
          return 1;
        }

        SK::ArknightsEndfield::AutoMutex lock (hMutex, 8000);
        if (!lock.owns_lock ())
        {
          SK::ArknightsEndfield::Utils::ShowTimedTaskDialog (
            L"SpecialK can not launch Arknights: Endfield",
            L"Reason: Failed to acquire mutex. Please terminate any remaining Arknights: Endfield processes.",
            SK::ArknightsEndfield::Utils::TaskDialogTimeoutCallback,
            3000
          );
          SK_Thread_CloseSelf ();
          return 1;
        }

        DWORD injectedPid = 0;
        bool injectedTargetExist = false;
        std::vector<DWORD> existingPids = SK_AKEF_GetExistingEndfieldPids ();

        if (SK::ArknightsEndfield::TryGetPid (&injectedPid))
          injectedTargetExist =
            std::find (existingPids.begin (), existingPids.end (), injectedPid) != existingPids.end ();

        if (injectedTargetExist)
        {
          SK::ArknightsEndfield::Utils::ShowTimedTaskDialog (
            L"Another Arknights Endfield process is already being launched.",
            L"Please wait for it to finish launching before starting another or terminate it.",
            SK::ArknightsEndfield::Utils::TaskDialogTimeoutCallback,
            3000
          );
          SK_Thread_CloseSelf ();
          return 1;
        }
        const auto* exe_name = static_cast <const wchar_t*> (param);
        SK::ArknightsEndfield::LaunchArknightsEndfield_Impl (exe_name);
      }
      catch (const std::exception& ex)
      {
        SK_LOGi0 (L"Exception occurred while launching Arknights: Endfield - %hs", ex.what ());
      }
      SK_Thread_CloseSelf ();
      return 0;
    }, L"[SK] Launch Arknights Endfield",
        reinterpret_cast <LPVOID> (const_cast <wchar_t*> (wszExecutableName)));
}

static BOOL WINAPI
SK::ArknightsEndfield::Hooks::CreateProcessW_Detour (
   LPCWSTR lpApplicationName,
   LPWSTR lpCommandLine,
   LPSECURITY_ATTRIBUTES lpProcessAttributes,
   LPSECURITY_ATTRIBUTES lpThreadAttributes,
   BOOL bInheritHandles,
   DWORD dwCreationFlags,
   LPVOID lpEnvironment,
   LPCWSTR lpCurrentDirectory,
   LPSTARTUPINFOW lpStartupInfo,
   LPPROCESS_INFORMATION lpProcessInformation
)
{
  const bool isApplicationEndfield = lpApplicationName != nullptr &&
    StrStrIW (lpApplicationName, L"Endfield.exe");
  const bool isCommandLineEndfield = lpCommandLine != nullptr &&
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

  std::filesystem::path global_cfg_dir = std::filesystem::path (SK_GetInstallPath ()) / LR"(Global\)";
  SK_Inject_ParseWhiteAndBlacklists (global_cfg_dir);

  FARPROC ntQueryAddr = SK_GetProcAddress (L"ntdll.dll", "NtQueryInformationProcess");

  const auto startTime = std::chrono::steady_clock::now ();
  constexpr auto maxWait = std::chrono::seconds (15);

  const BOOL result = CreateProcessW_Original (
    lpApplicationName,
    lpCommandLine,
    lpProcessAttributes,
    lpThreadAttributes,
    bInheritHandles,
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

  while (!Utils::GetRemoteModule (lpProcessInformation->hProcess, ntQueryAddr, L"kernel32.dll"))
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
    SK_Sleep (10);
  }

  std::wstring skDllPath = SK_FormatStringW (
    SK_RunLHIfBitness (64, LR"(%ws\SpecialK64.dll)", LR"(%ws\SpecialK32.dll)"),
    SK_GetInstallPath ()
  );

  UINT exitCode = 0;
  if (Utils::InjectDLL (lpProcessInformation->hProcess, skDllPath))
  {
    SK_LOGi0 (L"SpecialK has been injected successfully.");
    Utils::ShowTimedTaskDialog (
      L"Special K has successfully launched Arknights: Endfield",
      L"Please wait until the game has finished loading.",
      Utils::TaskDialogTimeoutCallback,
      3000
    );
  }
  else
  {
    exitCode = 1;
    SK_LOGi0 (L"SpecialK injection failed.");
    Utils::ShowTimedTaskDialog (
      L"Special K failed to inject into Arknights: Endfield.",
      L"Please close any running Endfield.exe or Platform.exe processes, then try again.",
      Utils::TaskDialogTimeoutCallback,
      3000
    );
  }

  return result;
}

void
SK_AKEF_InitFromLauncher (void)
{
  SK_Thread_CreateEx ([](LPVOID) -> DWORD
    {
      SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);
      SK_AutoCOMInit _require_COM;

      SK_RunOnce (
        LPVOID CreateProcessW_target = nullptr;
        if (SK_CreateDLLHook2 (L"kernel32",
              "CreateProcessW",
              SK::ArknightsEndfield::Hooks::CreateProcessW_Detour,
              static_cast_p2p <void> (&SK::ArknightsEndfield::Hooks::CreateProcessW_Original),
              &CreateProcessW_target) == MH_OK)
        {
          MH_EnableHook (CreateProcessW_target);
        }
      );

      SK_Thread_CloseSelf ();
      return 0;
    }, L"[SK] AKEF Launcher Init");
}

void
SK_AKEF_InitPlugin (void)
{
  SK_RunOnce (
    using namespace SK::ArknightsEndfield;

    ini.override_unity_framelimit =
    _CreateConfigParameterBool (L"ArknightEndfield.System",
      L"OverrideUnityFramerateLimit",
      b_OverrideUnityFramelimit,
      L"Override Unity Framerate Limit");

    ini.keep_original_swapchain =
    _CreateConfigParameterBool (L"ArknightEndfield.System",
      L"KeepOriginalSwapchain",
      b_KeepOriginalSwapchain,
      L"Keep Original Swapchain");

    __g_SK_AKEF_KeepOriginalSwapchain = ini.keep_original_swapchain;

    ini.unity_frameratelimit =
    _CreateConfigParameterFloat (L"ArknightEndfield.System",
      L"UnityFramerateLimit",
      f_UnityFramelimit,
      L"Unity Framerate Limit");

    plugin_mgr->config_fns.emplace (SK_AKEF_PlugInCfg);
    plugin_mgr->first_frame_fns.emplace (SK_AKEF_PresentFirstFrame);
  );

  const PPEB peb = reinterpret_cast <PPEB> (__readgsqword (0x60));
  if (peb)
  {
    UNICODE_STRING cmd = peb->ProcessParameters->CommandLine;
    std::wstring cmdLine = std::wstring (cmd.Buffer, cmd.Length / sizeof (wchar_t));
    if (StrStrIW (cmdLine.c_str (), L"-force-d3d11") != nullptr)
    {
      SK::ArknightsEndfield::render_api.store (SK_RenderAPI::D3D11, std::memory_order_relaxed);
    }
  }

  if (SK::ArknightsEndfield::render_api == SK_RenderAPI::None)
    SK::ArknightsEndfield::render_api.store (SK_RenderAPI::Vulkan, std::memory_order_relaxed);

  SK_Thread_CreateEx ([](LPVOID) -> DWORD
    {
      const auto current_render_api = SK::ArknightsEndfield::render_api.load (std::memory_order_relaxed);
      SK_AutoHandle hProcess(GetCurrentProcess());

      auto InitUnityPatches = []() -> void
      {
        constexpr int kMaxRetries = 60;
        int i = 0;
        HMODULE hUnity = nullptr;

        while (i < kMaxRetries)
        {
          hUnity = SK_GetModuleHandle (L"UnityPlayer.dll");
          if (hUnity != nullptr)
            break;
          ++i;
          SK_Sleep (500);
        }

        if (hUnity)
        {
          SK::ArknightsEndfield::CodePatch <8> mutexPatch = { };
          mutexPatch.address = SK_ScanIdaStyle (hUnity, "48 83 7E ? ? 0F 95 C0 48 83 C4");
          mutexPatch.description = L"Unity Mutex Bypass Patch";
          mutexPatch.replacement = { 0xB0, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
          mutexPatch.applyReplacement ();
        }
        else
        {
          SK_LOGi0 ("SK_AKEF_InitPlugin: UnityPlayer.dll not detected within timeout period.\n");
        }
      };

      auto InitDLSSGHooks = [hProcess]() -> void
      {
        // Wait for unityplayer.dll to load sl.interposer.dll,
        // which indicates that the game has finished loading the main modules and is loading the DLSS plugin soon after.
        constexpr int kMaxRetries_interposer = 40;
        constexpr DWORD kRetryDelayMs = 1000;
        int curr_try = 0;
        while (curr_try < kMaxRetries_interposer)
        {
          if (SK_GetModuleHandle (L"sl.interposer.dll"))
            break;
          ++curr_try;
          SK_Sleep (kRetryDelayMs);
        }

        constexpr int  kMaxAttempts     = 30;
        static HMODULE hModSLDLSSG      = nullptr;
        static HMODULE hModSLDLSSG_OTA  = nullptr;
        static HMODULE hModSLCOMMON     = nullptr;
        static HMODULE hModSLCOMMON_OTA = nullptr;

        for (int i = 0; i < kMaxAttempts; ++i)
        {
          if (ADL_init != ADL_FALSE)
            break;

          // Ideally, we should make it to early break of this loop if one of the hooked function get called
          if (hModSLDLSSG == nullptr)
          {
            if (GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, L"sl.dlss_g.dll", &hModSLDLSSG))
            {
              auto slGetPluginFunction =
                reinterpret_cast <SK::ArknightsEndfield::Hooks::slGetPluginFunction_pfn> (
                  SK_GetProcAddress (hModSLDLSSG, "slGetPluginFunction"));
              if (slGetPluginFunction != nullptr)
              {
                auto slDLSSGSetOptions =
                  reinterpret_cast <SK::ArknightsEndfield::Hooks::slDLSSGSetOptions_pfn> (slGetPluginFunction ("slDLSSGSetOptions"));

                if (slDLSSGSetOptions != nullptr)
                {
                  if (SK_CreateFuncHook (L"AKEF_slDLSSGSetOptions",
                    slDLSSGSetOptions,
                    SK::ArknightsEndfield::Hooks::slDLSSGSetOptions_Detour,
                    static_cast_p2p <void> (&SK::ArknightsEndfield::Hooks::slDLSSGSetOptions_Original)) == MH_OK)
                  {
                    MH_EnableHook (slDLSSGSetOptions);
                  }
                }
              }
            }
          }

          if (hModSLCOMMON == nullptr)
          {
            if (GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, L"sl.common.dll", &hModSLCOMMON))
            {
              void* reflex_sleep_addr = SK_ScanIdaStyle (
                hModSLCOMMON,
                "48 89 5C 24 ? 57 48 81 EC ? ? ? ? 48 8D B9"
              );

              if (reflex_sleep_addr == nullptr)
              {
                OutputDebugStringW(L"Failed to find Reflex Sleep function in sl.common.dll.\n");
              }
              else
              {
                if (SK_CreateFuncHook(
                  L"AKEF_slCommon_ReflexSleep",
                  reflex_sleep_addr,
                  SK::ArknightsEndfield::Hooks::slCommon_ReflexSleep_Detour,
                  static_cast_p2p <void>(&SK::ArknightsEndfield::Hooks::slCommon_ReflexSleep_Original)
                ) == MH_OK)
                  MH_EnableHook(reflex_sleep_addr);
              }
            }
          }

          if (hModSLDLSSG_OTA == nullptr)
          {
            static FARPROC ntQueryAddr = SK_GetProcAddress (L"ntdll.dll", "NtQueryInformationProcess");
            hModSLDLSSG_OTA = SK::ArknightsEndfield::Utils::GetRemoteModule (
              hProcess,
              ntQueryAddr,
              L"NVIDIA\\NGX\\models\\sl_dlss_g_0",
              true);

            if (hModSLDLSSG_OTA != nullptr)
            {
              auto slGetPluginFunction =
                reinterpret_cast <SK::ArknightsEndfield::Hooks::slGetPluginFunction_pfn> (
                  SK_GetProcAddress (hModSLDLSSG_OTA, "slGetPluginFunction"));
              if (slGetPluginFunction != nullptr)
              {
                auto slDLSSGSetOptions =
                  reinterpret_cast <SK::ArknightsEndfield::Hooks::slDLSSGSetOptions_pfn> (slGetPluginFunction ("slDLSSGSetOptions"));

                if (slDLSSGSetOptions != nullptr)
                {
                  if (SK_CreateFuncHook (L"AKEF_slDLSSGSetOptions_OTA",
                    slDLSSGSetOptions,
                    SK::ArknightsEndfield::Hooks::slDLSSGSetOptions_OTA_Detour,
                    static_cast_p2p <void> (&SK::ArknightsEndfield::Hooks::slDLSSGSetOptions_OTA_Original)) == MH_OK)
                  {
                    MH_EnableHook (slDLSSGSetOptions);
                  }
                }
              }
            }
          }

          if (hModSLCOMMON_OTA == nullptr)
          {
            OutputDebugStringW(L"Trying to find sl.common OTA module...\n");
            static FARPROC ntQueryAddr = SK_GetProcAddress (L"ntdll.dll", "NtQueryInformationProcess");

            hModSLDLSSG_OTA = SK::ArknightsEndfield::Utils::GetRemoteModule (
              hProcess,
              ntQueryAddr,
              L"NVIDIA\\NGX\\models\\sl_common_0",
              true);

            if (hModSLCOMMON_OTA != nullptr)
            {
              void* reflex_sleep_addr = SK_ScanIdaStyle (
                hModSLCOMMON_OTA,
                "48 89 5C 24 ? 57 48 81 EC ? ? ? ? 48 8D B9"
              );

              if (reflex_sleep_addr == nullptr)
              {
                OutputDebugStringW(L"Failed to find Reflex Sleep function in sl.common.dll.\n");
              }
              else
              {
                if (SK_CreateFuncHook(
                  L"AKEF_slCommon_OTA_ReflexSleep",
                  reflex_sleep_addr,
                  SK::ArknightsEndfield::Hooks::slCommon_ReflexSleep_OTA_Detour,
                  static_cast_p2p<void>(&SK::ArknightsEndfield::Hooks::slCommon_ReflexSleep_OTA_Original)
                ) == MH_OK)
                  if (MH_EnableHook(reflex_sleep_addr) == MH_OK)
                  {
                    OutputDebugStringW(L"Successfully hooked sl.common OTA module.\n");
                  }
                  else
                  {
                    OutputDebugStringW(L"Failed to enable hook for sl.common OTA module.\n");
                  }
                else
                  OutputDebugStringW(L"Failed to create hook for sl.common OTA module.\n");
              }
            }
          }

          if (hModSLDLSSG && hModSLDLSSG_OTA && hModSLCOMMON && hModSLCOMMON_OTA)
            break;
          SK_Sleep (kRetryDelayMs);
        }
      };

      InitUnityPatches ();
      if (current_render_api == SK_RenderAPI::Vulkan)
      {
        SK_LOGi1(L"Detected Vulkan renderer, initializing DLSSG hooks.\n");
        InitDLSSGHooks ();
      }

      InterlockedExchange (&SK::ArknightsEndfield::init, 1);
      SK_Thread_CloseSelf ();
      return 0;
    }, L"[SK] Plug-In Init");
}

static void
SK::ArknightsEndfield::ApplyUnityOverride (const OverrideType& overrideType, const std::variant<int, float, bool>& value)
{
  static OverrideType s_overrideType;
  static std::variant<int, float, bool> s_value;

  s_overrideType = overrideType;
  s_value = value;

  if (!Il2cpp::initialized)
    return;

  SK_Thread_CreateEx ([](LPVOID) -> DWORD
    {
      Il2cpp::thread_attach (Il2cpp::get_domain ());

      auto ExitThreadAndCleanup = [] (DWORD exitCode)
        {
          Il2cpp::thread_detach (Il2cpp::thread_current ());
          SK_Thread_CloseSelf ();
          return exitCode;
        };

      static il2cpp::Wrapper wrapper;
      static Image* image = nullptr;
      static Class* klass = nullptr;
      static Method* set_targetFrameRate = nullptr;
      static Method* get_targetFrameRate = nullptr;

      if (!image)
        image = wrapper.get_image ("UnityEngine.CoreModule.dll") ?
                wrapper.get_image ("UnityEngine.CoreModule.dll") :
                wrapper.get_image ("UnityEngine.dll");
      if (!image)
        return ExitThreadAndCleanup(1);

      if (!klass)
        klass = image->get_class ("Application", "UnityEngine");

      if (!klass)
        return ExitThreadAndCleanup(1);

      if (!set_targetFrameRate)
        set_targetFrameRate = klass->get_method ("set_targetFrameRate", 1);

      if (set_targetFrameRate != nullptr && *(reinterpret_cast<LPVOID*> (set_targetFrameRate)) != nullptr)
      {
        SK_RunOnce (
          SK_CreateFuncHook (L"UnityEngine.Application.set_targetFrameRate",
                             *(reinterpret_cast<LPVOID*> (set_targetFrameRate)),
                             SK::ArknightsEndfield::Hooks::UnityEngine_Application_set_targetFrameRate_Detour,
                             static_cast_p2p<void> (&SK::ArknightsEndfield::Hooks::UnityEngine_Application_set_targetFrameRate_Original))
        );

        if (MH_EnableHook (*(reinterpret_cast<LPVOID*> (set_targetFrameRate))) == MH_OK)
          SK_LOGi1 (L"Successfully hooked UnityEngine.Application.set_targetFrameRate.\n");
      }

      if (!get_targetFrameRate)
        get_targetFrameRate = klass->get_method ("get_targetFrameRate", 0);

      if (get_targetFrameRate != nullptr && *(reinterpret_cast<LPVOID*> (get_targetFrameRate)) != nullptr)
      {
        SK_RunOnce (
          SK_CreateFuncHook (L"UnityEngine.Application.get_targetFrameRate",
                             *(reinterpret_cast<LPVOID*> (get_targetFrameRate)),
                             SK::ArknightsEndfield::Hooks::UnityEngine_Application_get_targetFrameRate_Detour,
                             static_cast_p2p<void> (&SK::ArknightsEndfield::Hooks::UnityEngine_Application_get_targetFrameRate_Original))
        );

        if (MH_EnableHook (*(reinterpret_cast<LPVOID*> (get_targetFrameRate))) == MH_OK)
          SK_LOGi1 (L"Successfully hooked UnityEngine.Application.get_targetFrameRate.\n");
      }

      switch (s_overrideType)
      {
        case OverrideType::FrameRate:
        {
          if (const auto* pValue = std::get_if<float> (&s_value))
          {
            const float target_UnityFrameLimit = *pValue;

            if (set_targetFrameRate != nullptr && SK::ArknightsEndfield::b_OverrideUnityFramelimit)
            {
              int value = static_cast<int> (std::clamp (target_UnityFrameLimit, -1.0f, 480.0f));

              std::array<void*, 1> params = { &value };
              Il2cpp::method_call (set_targetFrameRate, nullptr, params.data (), nullptr);
            }
          }
        }
      }

      Il2cpp::thread_detach (Il2cpp::thread_current ());
      SK_Thread_CloseSelf ();
      return 0;
    }, L"[SK] SK_AKEF_ApplyUnityTargetFrameRate");
}

bool
SK_AKEF_PlugInCfg (void)
{
  using namespace SK::ArknightsEndfield;

  if (ImGui::CollapsingHeader ("Arknights: Endfield", ImGuiTreeNodeFlags_DefaultOpen))
  {
    static bool restartRequired = false;
    static bool unityFpsSeeded = false;
    bool changed = false;

    if (ImGui::Checkbox ("Override Unity Framerate Limit", &b_OverrideUnityFramelimit))
    {
      ini.override_unity_framelimit->store (b_OverrideUnityFramelimit);
      changed = true;
    }

    if (b_OverrideUnityFramelimit)
    {
      if (!unityFpsSeeded)
      {
        f_UnityFramelimit =
          f_UnityFramelimit == DefaultFirstFramerateLimit ?
          std::abs (__target_fps) + 1.0f : f_UnityFramelimit;

        ini.unity_frameratelimit->store (f_UnityFramelimit);

        ApplyUnityOverride (OverrideType::FrameRate, f_UnityFramelimit);
        unityFpsSeeded = true;
        changed = true;
      }

      if (ImGui::SliderFloat ("Unity Framerate Limit", &f_UnityFramelimit, -1.0f, 520.0f, "%.0f"))
      {
        ini.unity_frameratelimit->store (f_UnityFramelimit);
        ApplyUnityOverride (OverrideType::FrameRate, f_UnityFramelimit);
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
        ImGui::TextUnformatted (" Set to 0 for unlimited.");
        ImGui::EndGroup ();

        ImGui::EndTooltip ();
      }
    }

    if (ImGui::Checkbox ("Keep Original Swapchain", &b_KeepOriginalSwapchain))
    {
      ini.keep_original_swapchain->store (b_KeepOriginalSwapchain);
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

HRESULT STDMETHODCALLTYPE
SK_AKEF_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  SK_RunOnce (
    SK_Thread_CreateEx ([](LPVOID) -> DWORD
      {
        while (0 == InterlockedAdd (&SK::ArknightsEndfield::init, 0))
          SK_Sleep (1000);

        while (SK_GetFramesDrawn () < 240)
          SK_Sleep (250);

        float _unityFrameLimit = SK::ArknightsEndfield::f_UnityFramelimit ==
                                  SK::ArknightsEndfield::DefaultFirstFramerateLimit ?
                                  std::abs (__target_fps) + 1 : SK::ArknightsEndfield::f_UnityFramelimit;

        SK::ArknightsEndfield::ApplyUnityOverride (SK::ArknightsEndfield::OverrideType::FrameRate, _unityFrameLimit);
        SK_Thread_CloseSelf ();
        return 0;
      }, L"[SK] SK_AKEF_PresentFirstFrame");
  );

  return S_OK;
}

void
__stdcall
SK_AKEF_ExitGame (void)
{
  SK_RunOnce (
    SK::ArknightsEndfield::ResetPid ();
    SK::ArknightsEndfield::CleanupSharedMemory ();
  );
}