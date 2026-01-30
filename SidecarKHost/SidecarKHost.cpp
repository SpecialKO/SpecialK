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
  uint32_t version;      // 1
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  uint64_t frame_id;
  uint32_t active_buffer; // 0 or 1
};
#pragma pack(pop)

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
  if (h.magic != 0x464F4B53u) return false; // 'SKOF'
  if (h.version != 1u) return false;
  if (h.width == 0 || h.height == 0 || h.stride == 0) return false;
  if (h.stride < h.width * 4u) return false;
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
    WriteStatusAtomic(statusPath, L"error", 0, L"attach_failed");
    return ATTACH_FAILED;
  }

  if (!attachPidStr.empty())
  {
    if (!TryParseU32(attachPidStr, targetPid) || targetPid == 0)
    {
      AppendLog(logPath, L"error: invalid --attach-pid");
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
      WriteStatusAtomic(statusPath, L"error", 0, L"attach_failed");
      return ATTACH_FAILED;
    }
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
        WriteStatusAtomic(statusPath, L"error", 0, L"timeout");
        return TIMEOUT;
      }

      Sleep(200);
    }
  }

  AppendLog(logPath, L"attached");
  WriteStatusAtomic(statusPath, L"attached", targetPid, L"none");

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

      size_t bufferBytes = 0;
      size_t totalBytes = 0;
      const bool ok = ValidateOverlayHeader(tmp, bufferBytes, totalBytes);
      UnmapViewOfFile(headerView);

      if (!ok)
      {
        AppendLog(logPath, L"shared memory header invalid");
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
  WriteStatusAtomic(statusPath, L"exiting", targetPid, L"none");
  return OK;
}
