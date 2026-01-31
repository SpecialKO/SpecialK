#include <windows.h>
#include <tlhelp32.h>
#include <cwchar>
#include <cstdio>
#include <string>
#include <atomic>
#include <csignal>
#include <windows.h>
#include <tuple>
#include <vector>


static std::atomic_bool g_shutdown{ false };
static bool g_frame_source_seen = false;

static DWORD g_host_pid = 0;


static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType)
{
  switch (ctrlType)
  {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    g_shutdown.store(true);
    return TRUE;
  default:
    return FALSE;
  }
}

#pragma pack(push, 1)
struct OverlayFrameHeader
{
  uint32_t magic;        // 'SKOF'
  uint32_t version;      // 2
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  uint32_t pixel_format_fourcc; // 'BGRA' (BGRA8_UNORM)
  uint32_t alpha_mode;          // 1 = premultiplied
  uint64_t frame_id;
  uint32_t active_buffer; // 0 or 1
};
#pragma pack(pop)

static constexpr uint32_t OVERLAY_MAGIC_SKOF = 0x464F4B53u; // 'SKOF'
static constexpr uint32_t OVERLAY_HEADER_VERSION = 2u;
static constexpr uint32_t OVERLAY_PIXEL_FORMAT_BGRA = 0x41524742u; // 'BGRA'
static constexpr uint32_t OVERLAY_ALPHA_MODE_PREMULTIPLIED = 1u;

static HANDLE g_frame_map = nullptr;
static OverlayFrameHeader g_frame_header{};

static void* g_frame_view = nullptr;
static uint64_t g_last_frame_id = 0;
static BYTE* g_frame_base = nullptr;
static BYTE* g_frame_buf0 = nullptr;
static BYTE* g_frame_buf1 = nullptr;
static size_t g_frame_total_bytes = 0;
static size_t g_frame_buffer_bytes = 0;
static bool g_frames_streaming = false;
static std::vector<uint8_t> g_staging;

static void AppendLog(const std::wstring& logPath, const wchar_t* msg);

static void HostLogAppend(const wchar_t* line)
{
  if (!line || !*line) return;

  if (g_host_pid == 0)
    g_host_pid = GetCurrentProcessId();

  wchar_t wszTempPath[MAX_PATH]{};
  DWORD cchTemp = GetTempPathW((DWORD)_countof(wszTempPath), wszTempPath);
  if (cchTemp == 0 || cchTemp >= (DWORD)_countof(wszTempPath))
    return;

  if (cchTemp > 0 && wszTempPath[cchTemp - 1] != L'\\')
  {
    if (cchTemp + 1 >= (DWORD)_countof(wszTempPath))
      return;
    wszTempPath[cchTemp] = L'\\';
    wszTempPath[cchTemp + 1] = L'\0';
  }

  wchar_t wszPath[MAX_PATH]{};
  wsprintfW(wszPath, L"%ssidecarkhost_log_%lu.txt", wszTempPath, (unsigned long)g_host_pid);

  HANDLE hFile = CreateFileW(
    wszPath,
    FILE_APPEND_DATA,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr,
    OPEN_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    nullptr);

  if (hFile == INVALID_HANDLE_VALUE)
    return;

  const DWORD cch = (DWORD)lstrlenW(line);
  DWORD cbWritten = 0;
  WriteFile(hFile, line, cch * sizeof(wchar_t), &cbWritten, nullptr);
  WriteFile(hFile, L"\r\n", 2 * sizeof(wchar_t), &cbWritten, nullptr);
  CloseHandle(hFile);
}

static void HostLogAppendMilestone(const wchar_t* line)
{
  HostLogAppend(line);
}

static bool ReadBGRAAt(const uint8_t* base_ptr,
  uint32_t width, uint32_t height, uint32_t stride,
  uint32_t x, uint32_t y,
  uint8_t& outB, uint8_t& outG, uint8_t& outR, uint8_t& outA)
{
  if (!base_ptr) return false;
  if (width == 0 || height == 0 || stride < width * 4u) return false;
  if (x >= width || y >= height) return false;

  const size_t off = (size_t)y * (size_t)stride + (size_t)x * 4u;
  outB = base_ptr[off + 0];
  outG = base_ptr[off + 1];
  outR = base_ptr[off + 2];
  outA = base_ptr[off + 3];
  return true;
}

static void LogPremultipliedInvariantOncePerSecond(const std::wstring& logPath,
  const OverlayFrameHeader& h,
  const uint8_t* activeBuf)
{
  static ULONGLONG s_lastLogMs = 0;
  static bool s_loggedSkipReason = false;

  const ULONGLONG now = GetTickCount64();
  if (s_lastLogMs != 0 && (now - s_lastLogMs) < 1000ull)
    return;

  if (h.pixel_format_fourcc != OVERLAY_PIXEL_FORMAT_BGRA)
  {
    if (!s_loggedSkipReason)
    {
      AppendLog(logPath, L"premult_skip=fmt_not_bgra");
      s_loggedSkipReason = true;
    }
    return;
  }

  if (h.alpha_mode != OVERLAY_ALPHA_MODE_PREMULTIPLIED)
  {
    if (!s_loggedSkipReason)
    {
      AppendLog(logPath, L"premult_skip=alpha_not_premultiplied");
      s_loggedSkipReason = true;
    }
    return;
  }

  if (h.width == 0 || h.height == 0 || h.stride == 0 || h.stride < h.width * 4u)
  {
    if (!s_loggedSkipReason)
    {
      AppendLog(logPath, L"premult_skip=invalid_dims");
      s_loggedSkipReason = true;
    }
    return;
  }

  s_loggedSkipReason = false;
  s_lastLogMs = now;

  const uint32_t w = h.width;
  const uint32_t hh = h.height;
  const uint32_t w_1 = (w > 0) ? (w - 1u) : 0u;
  const uint32_t h_1 = (hh > 0) ? (hh - 1u) : 0u;

  const std::pair<uint32_t, uint32_t> samples[] = {
    {0u, 0u}, {w_1, 0u}, {0u, h_1}, {w_1, h_1},
    {w / 2u, hh / 2u},
    {w / 4u, hh / 4u}, {(3u * w) / 4u, hh / 4u}, {w / 4u, (3u * hh) / 4u}, {(3u * w) / 4u, (3u * hh) / 4u},
    {w / 2u, hh / 4u}, {w / 2u, (3u * hh) / 4u}, {w / 4u, hh / 2u}, {(3u * w) / 4u, hh / 2u},
    {w / 2u, hh / 3u}, {w / 3u, hh / 2u},
  };

  const int N = (int)_countof(samples);
  int ok = 0;

  bool haveFail = false;
  uint32_t failX = 0, failY = 0;
  uint8_t fb = 0, fg = 0, fr = 0, fa = 0;
  int bOk = 0, gOk = 0, rOk = 0;

  for (int i = 0; i < N; ++i)
  {
    uint32_t x = samples[i].first;
    uint32_t y = samples[i].second;
    if (x >= w) x = w_1;
    if (y >= hh) y = h_1;

    uint8_t b = 0, g = 0, r = 0, a = 0;
    const bool readOk = ReadBGRAAt(activeBuf, w, hh, h.stride, x, y, b, g, r, a);
    const bool pass = readOk && (b <= a) && (g <= a) && (r <= a);
    if (pass)
    {
      ++ok;
    }
    else if (!haveFail)
    {
      haveFail = true;
      failX = x; failY = y;
      fb = b; fg = g; fr = r; fa = a;
      bOk = readOk ? (b <= a ? 1 : 0) : 0;
      gOk = readOk ? (g <= a ? 1 : 0) : 0;
      rOk = readOk ? (r <= a ? 1 : 0) : 0;
    }
  }

  wchar_t msg[256]{};
  if (!haveFail)
  {
    swprintf(msg, _countof(msg), L"premult_ok=%d/%d fail=none", ok, N);
  }
  else
  {
    swprintf(msg, _countof(msg),
      L"premult_ok=%d/%d fail=(x=%u,y=%u) BGRA=%02X %02X %02X %02X (B<=A?%d G<=A?%d R<=A?%d)",
      ok, N,
      (unsigned)failX, (unsigned)failY,
      (unsigned)fb, (unsigned)fg, (unsigned)fr, (unsigned)fa,
      bOk, gOk, rOk);
  }
  AppendLog(logPath, msg);
}

static void ReleaseFrameMapping()
{
  if (g_frame_view)
  {
    UnmapViewOfFile(g_frame_view);
    g_frame_view = nullptr;
  }

  g_staging.clear();

  g_frame_base = nullptr;
  g_frame_buf0 = nullptr;
  g_frame_buf1 = nullptr;
  g_frame_total_bytes = 0;
  g_frame_buffer_bytes = 0;

  if (g_frame_map)
  {
    CloseHandle(g_frame_map);
    g_frame_map = nullptr;
  }

  g_frame_source_seen = false;
}

static bool ValidateOverlayHeader(const OverlayFrameHeader& h, size_t& outBufferBytes, size_t& outTotalBytes)
{
  if (h.magic != OVERLAY_MAGIC_SKOF) return false;
  if (h.version != OVERLAY_HEADER_VERSION) return false;
  if (h.width == 0 || h.height == 0 || h.stride == 0) return false;
  if (h.stride < h.width * 4u) return false;
  if (h.pixel_format_fourcc != OVERLAY_PIXEL_FORMAT_BGRA) return false;
  if (h.alpha_mode != OVERLAY_ALPHA_MODE_PREMULTIPLIED) return false;
  if (h.active_buffer > 1u) return false;

  const uint64_t bufferBytes64 = (uint64_t)h.height * (uint64_t)h.stride;
  const uint64_t totalBytes64 = (uint64_t)sizeof(OverlayFrameHeader) + 2ull * bufferBytes64;
  if (totalBytes64 > 256ull * 1024ull * 1024ull) return false;

  outBufferBytes = (size_t)bufferBytes64;
  outTotalBytes = (size_t)totalBytes64;
  return true;
}




static std::wstring GetExeDir()
{
  wchar_t buf[MAX_PATH]{};
  GetModuleFileNameW(nullptr, buf, MAX_PATH);
  std::wstring p(buf);
  auto slash = p.find_last_of(L"\\/");
  return (slash == std::wstring::npos) ? L"." : p.substr(0, slash);
}

static std::wstring ArgValue(int argc, wchar_t** argv, const wchar_t* key)
{
  for (int i = 1; i < argc - 1; ++i)
    if (wcscmp(argv[i], key) == 0)
      return argv[i + 1];
  return L"";
}

static bool HasFlag(int argc, wchar_t** argv, const wchar_t* key)
{
  for (int i = 1; i < argc; ++i)
    if (wcscmp(argv[i], key) == 0)
      return true;
  return false;
}

static void AppendLog(const std::wstring& logPath, const wchar_t* msg)
{
  FILE* f = nullptr;
  _wfopen_s(&f, logPath.c_str(), L"a+, ccs=UTF-8");
  if (!f) return;

  SYSTEMTIME st{};
  GetLocalTime(&st);
  fwprintf(f, L"%04u-%02u-%02u %02u:%02u:%02u.%03u %s\n",
    st.wYear, st.wMonth, st.wDay,
    st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
    msg);

  fclose(f);
}

static void LogOverlayHeaderDetails(const std::wstring& logPath, DWORD pid, const std::wstring& shmName, const OverlayFrameHeader& h)
{
  wchar_t msg[512]{};
  swprintf(msg, _countof(msg),
    L"overlay shm attach pid=%lu name=%s ver=%u w=%u h=%u stride=%u fmt=0x%08X alpha=%u last_frame=%llu",
    (unsigned long)pid,
    shmName.c_str(),
    (unsigned)h.version,
    (unsigned)h.width,
    (unsigned)h.height,
    (unsigned)h.stride,
    (unsigned)h.pixel_format_fourcc,
    (unsigned)h.alpha_mode,
    (unsigned long long)h.frame_id);
  AppendLog(logPath, msg);
}

static void WriteStatusAtomic(const std::wstring& statusPath, const wchar_t* state, DWORD pid, const wchar_t* lastError)
{
  std::wstring tmp = statusPath + L".tmp";

  FILE* f = nullptr;
  _wfopen_s(&f, tmp.c_str(), L"w, ccs=UTF-8");
  if (!f) return;

  fwprintf(f,
    L"{\n"
    L"  \"state\": \"%s\",\n"
    L"  \"target_pid\": %lu,\n"
    L"  \"api\": \"unknown\",\n"
    L"  \"last_error_code\": \"%s\"\n"
    L"}\n",
    state, (unsigned long)pid, (lastError ? lastError : L"none"));

  fclose(f);
  MoveFileExW(tmp.c_str(), statusPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}

static std::wstring Basename(const std::wstring& path)
{
  auto slash = path.find_last_of(L"\\/");
  return (slash == std::wstring::npos) ? path : path.substr(slash + 1);
}

static std::wstring GetProcessImagePath(DWORD pid)
{
  std::wstring out;

  HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (!h) return out;

  wchar_t buf[MAX_PATH]{};
  DWORD cch = (DWORD)_countof(buf);
  if (QueryFullProcessImageNameW(h, 0, buf, &cch))
    out.assign(buf, cch);

  CloseHandle(h);
  return out;
}

static void LogTargetSelected(const std::wstring& logPath, DWORD pid, const std::wstring& fallbackExeName)
{
  std::wstring exe = GetProcessImagePath(pid);
  if (exe.empty()) exe = fallbackExeName;

  wchar_t msg[1024]{};
  swprintf(msg, _countof(msg), L"target_selected pid=%lu exe=%s",
    (unsigned long)pid,
    exe.c_str());
  AppendLog(logPath, msg);
  HostLogAppend(msg);
}

static bool GetMatchingModuleInfo(DWORD pid, const wchar_t* moduleBasenameLower, MODULEENTRY32W& outMe)
{
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
  if (snap == INVALID_HANDLE_VALUE) return false;

  MODULEENTRY32W me{};
  me.dwSize = sizeof(me);

  bool found = false;
  if (Module32FirstW(snap, &me))
  {
    do {
      if (_wcsicmp(me.szModule, moduleBasenameLower) == 0)
      {
        outMe = me;
        found = true;
        break;
      }
    } while (Module32NextW(snap, &me));
  }

  CloseHandle(snap);
  return found;
}

enum class InjectResult
{
  Success,
  AlreadyLoaded,
  Fail
};

static const wchar_t* InjectResultToString(InjectResult r)
{
  switch (r)
  {
  case InjectResult::Success:       return L"success";
  case InjectResult::AlreadyLoaded: return L"already_loaded";
  default:                          return L"fail";
  }
}

static InjectResult InjectDllByCreateRemoteThread(DWORD pid, const std::wstring& dllPath)
{
  if (pid == 0 || dllPath.empty())
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return InjectResult::Fail;
  }

  // Already loaded check (deterministic already_loaded result)
  MODULEENTRY32W me{};
  if (GetMatchingModuleInfo(pid, L"SpecialK32.dll", me))
  {
    SetLastError(ERROR_ALREADY_EXISTS);
    return InjectResult::AlreadyLoaded;
  }

  HANDLE hProcess = OpenProcess(
    PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
    PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
    FALSE,
    pid);

  if (!hProcess)
    return InjectResult::Fail;

  const size_t cb = (dllPath.size() + 1) * sizeof(wchar_t);
  void* remoteMem = VirtualAllocEx(hProcess, nullptr, cb, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!remoteMem)
  {
    CloseHandle(hProcess);
    return InjectResult::Fail;
  }

  if (!WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), cb, nullptr))
  {
    DWORD le = GetLastError();
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    SetLastError(le);
    return InjectResult::Fail;
  }

  HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
  FARPROC pLoadLibraryW = hKernel32 ? GetProcAddress(hKernel32, "LoadLibraryW") : nullptr;
  if (!pLoadLibraryW)
  {
    DWORD le = GetLastError();
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    SetLastError(le ? le : ERROR_PROC_NOT_FOUND);
    return InjectResult::Fail;
  }

  HANDLE hThread = CreateRemoteThread(
    hProcess,
    nullptr,
    0,
    reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibraryW),
    remoteMem,
    0,
    nullptr);

  if (!hThread)
  {
    DWORD le = GetLastError();
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    SetLastError(le);
    return InjectResult::Fail;
  }

  WaitForSingleObject(hThread, INFINITE);

  DWORD remoteExit = 0;
  const BOOL haveExit = GetExitCodeThread(hThread, &remoteExit);

  CloseHandle(hThread);
  VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
  CloseHandle(hProcess);

  if (!haveExit)
    return InjectResult::Fail;

  if (remoteExit == 0)
  {
    SetLastError(ERROR_DLL_INIT_FAILED);
    return InjectResult::Fail;
  }

  SetLastError(ERROR_SUCCESS);
  return InjectResult::Success;
}

static bool TryGetFileMTime(const wchar_t* path, FILETIME& outFt)
{
  WIN32_FILE_ATTRIBUTE_DATA fad{};
  if (!GetFileAttributesExW(path, GetFileExInfoStandard, &fad))
    return false;

  outFt = fad.ftLastWriteTime;
  return true;
}

static void LogModuleCheckSpecialK32(const std::wstring& logPath, DWORD pid)
{
  MODULEENTRY32W me{};
  const bool found = GetMatchingModuleInfo(pid, L"SpecialK32.dll", me);

  {
    wchar_t msg[256]{};
    swprintf(msg, _countof(msg), L"module_check pid=%lu found_specialk32=%d",
      (unsigned long)pid,
      found ? 1 : 0);
    AppendLog(logPath, msg);
    HostLogAppend(msg);
  }

  if (!found)
  {
    AppendLog(logPath, L"ERROR: injected but SpecialK32.dll not present in module list");
    HostLogAppend(L"ERROR: injected but SpecialK32.dll not present in module list");
    return;
  }

  {
    wchar_t msg[1024]{};
    swprintf(msg, _countof(msg), L"specialk32_loaded path=%s base=0x%p",
      me.szExePath,
      me.modBaseAddr);
    AppendLog(logPath, msg);
    HostLogAppend(msg);
  }

  FILETIME ft{};
  if (TryGetFileMTime(me.szExePath, ft))
  {
    wchar_t msg[256]{};
    swprintf(msg, _countof(msg), L"specialk32_disk_mtime ft=0x%08lX%08lX",
      (unsigned long)ft.dwHighDateTime,
      (unsigned long)ft.dwLowDateTime);
    AppendLog(logPath, msg);
    HostLogAppend(msg);
  }
  else
  {
    AppendLog(logPath, L"specialk32_disk_mtime ft=unavailable");
    HostLogAppend(L"specialk32_disk_mtime ft=unavailable");
  }
}

static DWORD FindPidByExeName(const std::wstring& exeName) // case-insensitive
{
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE) return 0;

  PROCESSENTRY32W pe{};
  pe.dwSize = sizeof(pe);

  DWORD pid = 0;
  if (Process32FirstW(snap, &pe))
  {
    do {
      if (_wcsicmp(pe.szExeFile, exeName.c_str()) == 0)
      {
        pid = pe.th32ProcessID;
        break;
      }
    } while (Process32NextW(snap, &pe));
  }

  CloseHandle(snap);
  return pid;
}

static bool TryParseU32(const std::wstring& s, DWORD& out)
{
  if (s.empty()) return false;
  wchar_t* end = nullptr;
  unsigned long v = wcstoul(s.c_str(), &end, 10);
  if (end == s.c_str() || *end != L'\0') return false;
  out = (DWORD)v;
  return true;
}

static std::wstring PipeNameForTarget(DWORD pid)
{
  wchar_t buf[128]{};
  swprintf(buf, _countof(buf), L"\\\\.\\pipe\\SK_OverlayControl_%lu", (unsigned long)pid);
  return buf;
}

static bool ReadOnePipeMessage(HANDLE hPipe, std::wstring& out)
{
  wchar_t wbuf[8192]{};
  DWORD bytesRead = 0;

  if (!ReadFile(hPipe, wbuf, sizeof(wbuf) - sizeof(wchar_t), &bytesRead, nullptr))
    return false;

  if (bytesRead < sizeof(wchar_t))
    return false;

  wbuf[bytesRead / sizeof(wchar_t)] = L'\0';
  out.assign(wbuf);
  return true;
}

static bool CmdHasShutdownTrue(const std::wstring& s)
{
  // MVP: accept any message containing both "shutdown" and "true"
  // (tighten later to a real parser if needed)
  return (s.find(L"shutdown") != std::wstring::npos) &&
    (s.find(L"true") != std::wstring::npos);
}

static void RunShutdownPipeServer(const std::wstring& pipeName,
  const std::wstring& logPath,
  const std::wstring& statusPath,
  DWORD targetPid)
{
  while (!g_shutdown.load())
  {
    HANDLE hPipe = CreateNamedPipeW(
      pipeName.c_str(),
      PIPE_ACCESS_INBOUND,
      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
      1,
      0,
      64 * 1024,
      0,
      nullptr
    );

    if (hPipe == INVALID_HANDLE_VALUE)
    {
      AppendLog(logPath, L"error: CreateNamedPipe failed");
      WriteStatusAtomic(statusPath, L"error", targetPid, L"pipe_create_failed");
      g_shutdown.store(true);
      return;
    }

    BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected)
    {
      CloseHandle(hPipe);
      continue;
    }

    std::wstring msg;
    if (ReadOnePipeMessage(hPipe, msg))
    {
      AppendLog(logPath, (L"cmd: " + msg).c_str());

      if (msg.find(L"shutdown") != std::wstring::npos)
      {
        g_shutdown.store(true);
      }
    }

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
  }
}

static std::wstring SharedMemoryNameForTarget(DWORD pid)
{
  wchar_t buf[128]{};
  swprintf(buf, _countof(buf), L"SK_OverlayFrame_%lu", (unsigned long)pid);
  return buf;
}


enum ExitCode : int
{
  OK = 0,
  ATTACH_FAILED = 10,
  TIMEOUT = 11
};

int wmain(int argc, wchar_t** argv)
{
  // Unavoidable proof-of-execution artifact (must run before any attach/inject logic)
  g_host_pid = GetCurrentProcessId();
  {
    wchar_t wszTempPath[MAX_PATH]{};
    DWORD cchTemp = GetTempPathW((DWORD)_countof(wszTempPath), wszTempPath);
    if (cchTemp != 0 && cchTemp < (DWORD)_countof(wszTempPath))
    {
      if (cchTemp > 0 && wszTempPath[cchTemp - 1] != L'\\')
      {
        if (cchTemp + 1 < (DWORD)_countof(wszTempPath))
        {
          wszTempPath[cchTemp] = L'\\';
          wszTempPath[cchTemp + 1] = L'\0';
        }
      }

      wchar_t wszStartedPath[MAX_PATH]{};
      wsprintfW(wszStartedPath, L"%ssidecarkhost_started_%lu.txt", wszTempPath, (unsigned long)g_host_pid);

      HANDLE hFile = CreateFileW(
        wszStartedPath,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

      if (hFile != INVALID_HANDLE_VALUE)
      {
        const ULONGLONG t = GetTickCount64();
        wchar_t line[128]{};
        wsprintfW(line, L"started hostpid=%lu tick=%llu\r\n",
          (unsigned long)g_host_pid,
          (unsigned long long)t);

        DWORD cbWritten = 0;
        WriteFile(hFile, line, (DWORD)lstrlenW(line) * sizeof(wchar_t), &cbWritten, nullptr);
        CloseHandle(hFile);
      }
    }
  }

  HostLogAppendMilestone(L"parsed_args_ok");

  const std::wstring exeDir = GetExeDir();

  std::wstring statusPath = ArgValue(argc, argv, L"--status");
  std::wstring logPath = ArgValue(argc, argv, L"--log");
  if (statusPath.empty()) statusPath = exeDir + L"\\status.json";
  if (logPath.empty())    logPath = exeDir + L"\\overlay_engine.log";

  AppendLog(logPath, L"starting");
  WriteStatusAtomic(statusPath, L"starting", 0, L"none");
  SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);


  // ---- Determine target PID (no injection here) ----
  DWORD targetPid = 0;

  const std::wstring attachPidStr = ArgValue(argc, argv, L"--attach-pid");
  const std::wstring attachExe = ArgValue(argc, argv, L"--attach-exe");
  const std::wstring waitExe = ArgValue(argc, argv, L"--wait-for-exe");
  const std::wstring timeoutStr = ArgValue(argc, argv, L"--timeout-ms");

  int modeCount = 0;
  if (!attachPidStr.empty()) modeCount++;
  if (!attachExe.empty())    modeCount++;
  if (!waitExe.empty())      modeCount++;

  if (modeCount != 1)
  {
    AppendLog(logPath, L"error: specify exactly one of --attach-pid, --attach-exe, --wait-for-exe");
    HostLogAppend(L"error: specify exactly one of --attach-pid, --attach-exe, --wait-for-exe");
    WriteStatusAtomic(statusPath, L"error", 0, L"attach_failed");
    return ATTACH_FAILED;
  }

  if (!attachPidStr.empty())
  {
    if (!TryParseU32(attachPidStr, targetPid) || targetPid == 0)
    {
      AppendLog(logPath, L"error: invalid --attach-pid");
      HostLogAppend(L"error: invalid --attach-pid");
      WriteStatusAtomic(statusPath, L"error", 0, L"attach_failed");
      return ATTACH_FAILED;
    }
  }
  else if (!attachExe.empty())
  {
    targetPid = FindPidByExeName(Basename(attachExe));
    if (targetPid == 0)
    {
      AppendLog(logPath, L"error: --attach-exe not found running");
      HostLogAppend(L"error: --attach-exe not found running");
      WriteStatusAtomic(statusPath, L"error", 0, L"attach_failed");
      return ATTACH_FAILED;
    }

    // Deterministic selection log (after PID choice)
    LogTargetSelected(logPath, targetPid, Basename(attachExe));
  }
  else // wait-for-exe
  {
    DWORD timeoutMs = 0;
    if (!TryParseU32(timeoutStr, timeoutMs) || timeoutMs == 0)
      timeoutMs = 30000; // default 30s

    const ULONGLONG start = GetTickCount64();
    const std::wstring name = Basename(waitExe);

    while (true)
    {
      targetPid = FindPidByExeName(name);
      if (targetPid != 0) break;

      if (GetTickCount64() - start >= timeoutMs)
      {
        AppendLog(logPath, L"error: wait-for-exe timeout");
        HostLogAppend(L"error: wait-for-exe timeout");
        WriteStatusAtomic(statusPath, L"error", 0, L"timeout");
        return TIMEOUT;
      }

      Sleep(200);
    }

    // Deterministic selection log (after PID choice)
    LogTargetSelected(logPath, targetPid, Basename(waitExe));
  }

  if (!attachPidStr.empty())
  {
    // Deterministic selection log (after PID choice)
    LogTargetSelected(logPath, targetPid, L"");
  }

  AppendLog(logPath, L"attached");
  HostLogAppendMilestone(L"begin_attach");
  WriteStatusAtomic(statusPath, L"attached", targetPid, L"none");

  // Deterministic injection + module verification logging (host-side)
  {
    const std::wstring dllToInject = exeDir + L"\\SpecialK32.dll";
    wchar_t msg[1024]{};
    swprintf(msg, _countof(msg), L"inject_attempt pid=%lu dll=%s",
      (unsigned long)targetPid,
      dllToInject.c_str());
    AppendLog(logPath, msg);
    HostLogAppend(msg);
  }

  HostLogAppendMilestone(L"begin_inject");

  const std::wstring dllToInject = exeDir + L"\\SpecialK32.dll";
  const InjectResult injectResult = InjectDllByCreateRemoteThread(targetPid, dllToInject);
  const DWORD injectLastError = GetLastError();

  {
    wchar_t msg[256]{};
    swprintf(msg, _countof(msg), L"inject_result=%s last_error=%lu",
      InjectResultToString(injectResult),
      (unsigned long)injectLastError);
    AppendLog(logPath, msg);
    HostLogAppend(msg);
  }

  // Probe the target process module list now (acts as ground-truth for what is actually loaded).
  LogModuleCheckSpecialK32(logPath, targetPid);

  // Post-inject explicit verification log (module_check is required to flip on success)
  {
    MODULEENTRY32W me{};
    const bool found = GetMatchingModuleInfo(targetPid, L"SpecialK32.dll", me);

    wchar_t msg[256]{};
    swprintf(msg, _countof(msg), L"module_check pid=%lu found_specialk32=%d",
      (unsigned long)targetPid,
      found ? 1 : 0);
    AppendLog(logPath, msg);
    HostLogAppend(msg);

    if (found)
    {
      wchar_t msg2[1024]{};
      swprintf(msg2, _countof(msg2), L"specialk32_loaded path=%s base=0x%p",
        me.szExePath,
        me.modBaseAddr);
      AppendLog(logPath, msg2);
      HostLogAppend(msg2);
    }
  }

  const std::wstring pipeName = PipeNameForTarget(targetPid);
  AppendLog(logPath, (L"pipe: " + pipeName).c_str());
  WriteStatusAtomic(statusPath, L"attached", targetPid, L"none");




  // Run pipe server on a background thread so the main thread can remain your lifecycle owner
  HANDLE hPipeThread = CreateThread(
    nullptr, 0,
    [](LPVOID p) -> DWORD {
      auto* ctx = reinterpret_cast<std::tuple<std::wstring, std::wstring, std::wstring, DWORD>*>(p);
      RunShutdownPipeServer(std::get<0>(*ctx), std::get<1>(*ctx), std::get<2>(*ctx), std::get<3>(*ctx));
      delete ctx;
      return 0;
    },
    new std::tuple<std::wstring, std::wstring, std::wstring, DWORD>(pipeName, logPath, statusPath, targetPid),
    0, nullptr
  );

  if (!hPipeThread)
  {
    AppendLog(logPath, L"error: CreateThread(pipe) failed");
    WriteStatusAtomic(statusPath, L"error", targetPid, L"pipe_thread_failed");
    return ATTACH_FAILED;
  }

  bool lastVisible = false;
  bool lastStreaming = false;

  while (!g_shutdown.load())
  {
    if (!g_frame_source_seen)
    {
      const std::wstring shmName = SharedMemoryNameForTarget(targetPid);
      g_frame_map = OpenFileMappingW(FILE_MAP_READ, FALSE, shmName.c_str());
      if (!g_frame_map)
      {
        Sleep(50);
        continue;
      }

      void* headerView = MapViewOfFile(g_frame_map, FILE_MAP_READ, 0, 0, sizeof(OverlayFrameHeader));
      if (!headerView)
      {
        CloseHandle(g_frame_map);
        g_frame_map = nullptr;
        Sleep(50);
        continue;
      }

      OverlayFrameHeader tmp{};
      memcpy(&tmp, headerView, sizeof(tmp));

      LogOverlayHeaderDetails(logPath, targetPid, shmName, tmp);

      size_t bufferBytes = 0;
      size_t totalBytes = 0;
      const bool ok = ValidateOverlayHeader(tmp, bufferBytes, totalBytes);
      UnmapViewOfFile(headerView);

      if (!ok)
      {
        wchar_t msg[256]{};
        swprintf(msg, _countof(msg),
          L"shared memory header invalid/unsupported (expected ver=%u fmt=0x%08X alpha=%u)",
          (unsigned)OVERLAY_HEADER_VERSION,
          (unsigned)OVERLAY_PIXEL_FORMAT_BGRA,
          (unsigned)OVERLAY_ALPHA_MODE_PREMULTIPLIED);
        AppendLog(logPath, msg);
        CloseHandle(g_frame_map);
        g_frame_map = nullptr;
        Sleep(50);
        continue;
      }

      g_frame_view = MapViewOfFile(g_frame_map, FILE_MAP_READ, 0, 0, totalBytes);
      if (!g_frame_view)
      {
        CloseHandle(g_frame_map);
        g_frame_map = nullptr;
        Sleep(50);
        continue;
      }

      g_frame_base = reinterpret_cast<BYTE*>(g_frame_view);
      g_frame_total_bytes = totalBytes;
      g_frame_buffer_bytes = bufferBytes;
      g_frame_buf0 = g_frame_base + sizeof(OverlayFrameHeader);
      g_frame_buf1 = g_frame_buf0 + g_frame_buffer_bytes;

      g_staging.resize(g_frame_buffer_bytes);

      memcpy(&g_frame_header, g_frame_base, sizeof(OverlayFrameHeader));
      g_last_frame_id = g_frame_header.frame_id;

      AppendLog(logPath, L"shared memory mapped (full)");
      g_frame_source_seen = true;
    }
    else
    {
      OverlayFrameHeader a{};
      OverlayFrameHeader b{};
      memcpy(&a, g_frame_base, sizeof(a));

      size_t bufferBytes = 0;
      size_t totalBytes = 0;
      if (!ValidateOverlayHeader(a, bufferBytes, totalBytes) || totalBytes != g_frame_total_bytes)
      {
        AppendLog(logPath, L"shared memory header invalid/resize; remapping");
        ReleaseFrameMapping();
        Sleep(50);
        continue;
      }

      const BYTE* src = (a.active_buffer == 0) ? g_frame_buf0 : g_frame_buf1;
      (void)src;

      memcpy(&b, g_frame_base, sizeof(b));
      if (b.frame_id == a.frame_id && b.active_buffer == a.active_buffer)
      {
        g_frame_header = b;

        if (!g_staging.empty() && g_staging.size() == g_frame_buffer_bytes)
          memcpy(g_staging.data(), src, g_frame_buffer_bytes);

        if (g_frame_header.frame_id != g_last_frame_id)
        {
          g_last_frame_id = g_frame_header.frame_id;
          g_frames_streaming = true;
          AppendLog(logPath, L"frame advanced");

          LogPremultipliedInvariantOncePerSecond(logPath, g_frame_header, src);

          if ((g_last_frame_id % 60ull) == 0ull)
          {
            uint8_t r = 0, g = 0, bl = 0, a8 = 0;
            if (g_staging.size() >= 4)
            {
              r = g_staging[0];
              g = g_staging[1];
              bl = g_staging[2];
              a8 = g_staging[3];
            }

            const size_t n = (g_staging.size() < 4096) ? g_staging.size() : 4096;
            uint32_t fnv = 2166136261u;
            for (size_t i = 0; i < n; ++i)
            {
              fnv ^= (uint32_t)g_staging[i];
              fnv *= 16777619u;
            }

            wchar_t msg[256]{};
            swprintf(msg, _countof(msg),
              L"frame %llu buf=%u first_rgba=%02X%02X%02X%02X fnv4096=%08X",
              (unsigned long long)g_last_frame_id,
              (unsigned)g_frame_header.active_buffer,
              (unsigned)r, (unsigned)g, (unsigned)bl, (unsigned)a8,
              (unsigned)fnv);
            AppendLog(logPath, msg);
          }
        }
      }
    }

    bool nowVisible = g_frames_streaming;
    if (nowVisible != lastVisible)
    {
      WriteStatusAtomic(statusPath,
        nowVisible ? L"overlay_visible" : L"overlay_hidden",
        targetPid,
        L"none");
      lastVisible = nowVisible;
    }

    if (g_frames_streaming != lastStreaming)
    {
      WriteStatusAtomic(statusPath,
        g_frames_streaming ? L"frames_streaming" : L"attached",
        targetPid,
        L"none");
      lastStreaming = g_frames_streaming;
    }

    Sleep(50);
  }




  WaitForSingleObject(hPipeThread, 2000);
  CloseHandle(hPipeThread);

  ReleaseFrameMapping();


  // Placeholder: next step adds named-pipe control + visible/hidden state.
  AppendLog(logPath, L"exiting");
  HostLogAppendMilestone(L"end");
  WriteStatusAtomic(statusPath, L"exiting", targetPid, L"none");
  return OK;
}
