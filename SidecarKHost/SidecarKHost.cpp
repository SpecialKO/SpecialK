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

#include <dwmapi.h>

static constexpr bool kEnableLegacySKOF_DebugOnly = false;

static HANDLE g_diag_enable_map = nullptr;

static void CreateDiagnosticsEnableTokenForPid(DWORD targetPid)
{
  if (targetPid == 0)
    return;

  if (g_diag_enable_map != nullptr)
    return;

  char name[128]{};
  wsprintfA(name, "Local\\SidecarK_Diagnostics_Enable_%lu", (unsigned long)targetPid);

  g_diag_enable_map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4, name);
  if (!g_diag_enable_map)
    return;

  void* view = MapViewOfFile(g_diag_enable_map, FILE_MAP_WRITE, 0, 0, 4);
  if (!view)
  {
    CloseHandle(g_diag_enable_map);
    g_diag_enable_map = nullptr;
    return;
  }

  memcpy(view, "SKD1", 4);
  UnmapViewOfFile(view);
}

static bool HasModule(DWORD pid, const wchar_t* basename)
{
  if (pid == 0 || basename == nullptr || *basename == L'\0')
    return false;

  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
  if (snap == INVALID_HANDLE_VALUE)
    return false;

  MODULEENTRY32W me{};
  me.dwSize = sizeof(me);

  bool found = false;
  if (Module32FirstW(snap, &me))
  {
    do {
      const wchar_t* mod = me.szModule;
      if (mod == nullptr) mod = L"";

      const wchar_t* b = mod;
      for (const wchar_t* p = mod; *p; ++p)
      {
        if (*p == L'\\' || *p == L'/')
          b = p + 1;
      }

      if (_wcsicmp(b, basename) == 0)
      {
        found = true;
        break;
      }
    } while (Module32NextW(snap, &me));
  }

  CloseHandle(snap);
  return found;
}

static std::wstring DirnameOfPath(const std::wstring& path)
{
  auto slash = path.find_last_of(L"\\/");
  return (slash == std::wstring::npos) ? L"." : path.substr(0, slash);
}

static void LogTargetGraphicsSnapshot(DWORD pid, const std::wstring& targetExeFullPath)
{
  return;

  const int dxgi   = HasModule(pid, L"dxgi.dll")        ? 1 : 0;
  const int d3d11  = HasModule(pid, L"d3d11.dll")       ? 1 : 0;
  const int d3d9   = HasModule(pid, L"d3d9.dll")        ? 1 : 0;
  const int d3d12  = HasModule(pid, L"d3d12.dll")       ? 1 : 0;
  const int opengl = HasModule(pid, L"opengl32.dll")    ? 1 : 0;
  const int vulkan = HasModule(pid, L"vulkan-1.dll")    ? 1 : 0;

  const std::wstring dir = DirnameOfPath(targetExeFullPath);

  auto _ExistsInDir = [&](const wchar_t* base) -> int
  {
    std::wstring p = dir;
    if (!p.empty() && p.back() != L'\\') p += L"\\";
    p += base;
    return (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) ? 1 : 0;
  };

  const int local_opengl32 = _ExistsInDir(L"opengl32.dll");
  const int local_dxgi     = _ExistsInDir(L"dxgi.dll");
  const int local_d3d11    = _ExistsInDir(L"d3d11.dll");
  const int local_d3d9     = _ExistsInDir(L"d3d9.dll");
  const int local_dinput8  = _ExistsInDir(L"dinput8.dll");

  wchar_t msg[512]{};
  swprintf(msg, _countof(msg),
    L"target_gfx pid=%lu dxgi=%d d3d11=%d d3d9=%d d3d12=%d opengl32=%d vulkan=%d local_opengl32=%d local_dxgi=%d local_d3d11=%d local_d3d9=%d local_dinput8=%d",
    (unsigned long)pid,
    dxgi, d3d11, d3d9, d3d12, opengl, vulkan,
    local_opengl32, local_dxgi, local_d3d11, local_d3d9, local_dinput8);

  if (*msg)
  {
    const DWORD hostPid = GetCurrentProcessId();

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

      wchar_t wszPath[MAX_PATH]{};
      wsprintfW(wszPath, L"%ssidecarkhost_log_%lu.txt", wszTempPath, (unsigned long)hostPid);

      HANDLE hFile = CreateFileW(
        wszPath,
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

      if (hFile != INVALID_HANDLE_VALUE)
      {
        const DWORD cch = (DWORD)lstrlenW(msg);
        DWORD cbWritten = 0;
        WriteFile(hFile, msg, cch * sizeof(wchar_t), &cbWritten, nullptr);
        WriteFile(hFile, L"\r\n", 2 * sizeof(wchar_t), &cbWritten, nullptr);
        CloseHandle(hFile);
      }
    }
  }
}


static std::wstring GetDirname(const std::wstring& path)
{
  auto slash = path.find_last_of(L"\\/");
  return (slash == std::wstring::npos) ? L"." : path.substr(0, slash);
}

static HRESULT (WINAPI* pDwmGetWindowAttribute)(HWND, DWORD, PVOID, DWORD) = nullptr;

static void EnsureDwmGetWindowAttribute()
{
  if (pDwmGetWindowAttribute != nullptr)
    return;

  HMODULE h = LoadLibraryW(L"dwmapi.dll");
  if (h == nullptr)
    return;

  pDwmGetWindowAttribute = reinterpret_cast<HRESULT(WINAPI*)(HWND, DWORD, PVOID, DWORD)>(
    GetProcAddress(h, "DwmGetWindowAttribute"));
}

struct WaitGateEnumCtx
{
  DWORD pid = 0;
  HWND best = nullptr;
};

static BOOL CALLBACK EnumWindowsFindTopLevelForPid(HWND hwnd, LPARAM lParam)
{
  auto* ctx = reinterpret_cast<WaitGateEnumCtx*>(lParam);
  if (!ctx || ctx->pid == 0)
    return TRUE;

  DWORD wpid = 0;
  GetWindowThreadProcessId(hwnd, &wpid);
  if (wpid != ctx->pid)
    return TRUE;

  if (GetWindow(hwnd, GW_OWNER) != nullptr)
    return TRUE;

  const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
  if (style & WS_CHILD)
    return TRUE;

  auto IsUncloakedVisible = [&](HWND w) -> bool
  {
    if (!IsWindowVisible(w))
      return false;

    BOOL cloaked = FALSE;
    EnsureDwmGetWindowAttribute();
    if (pDwmGetWindowAttribute != nullptr)
    {
      HRESULT hr = pDwmGetWindowAttribute(w, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
      if (FAILED(hr))
        cloaked = FALSE;
    }

    return cloaked == FALSE;
  };

  if (ctx->best == nullptr)
  {
    ctx->best = hwnd;
    return TRUE;
  }

  if (IsUncloakedVisible(hwnd) && !IsUncloakedVisible(ctx->best))
    ctx->best = hwnd;

  return TRUE;
}

static HWND FindTopLevelWindowForPid(DWORD pid)
{
  WaitGateEnumCtx ctx{};
  ctx.pid = pid;
  EnumWindows(EnumWindowsFindTopLevelForPid, (LPARAM)&ctx);
  return ctx.best;
}

static void WriteWindowProofOnceForPid(DWORD pid, HWND hwnd)
{
  return;

  static std::atomic<DWORD> s_logged_pid{ 0 };
  const DWORD expected = 0;
  if (!s_logged_pid.compare_exchange_strong(const_cast<DWORD&>(expected), pid))
  {
    if (s_logged_pid.load() == pid)
      return;
  }

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
  wsprintfW(wszPath, L"%ssidecark_host_window_%lu.txt", wszTempPath, (unsigned long)pid);

  HANDLE hFile = CreateFileW(
    wszPath,
    GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    nullptr);

  if (hFile == INVALID_HANDLE_VALUE)
    return;

  wchar_t title[512]{};
  RECT rc{ 0,0,0,0 };

  if (hwnd != nullptr)
  {
    GetWindowTextW(hwnd, title, (int)_countof(title));
    GetWindowRect(hwnd, &rc);
  }

  wchar_t line[2048]{};
  if (hwnd != nullptr)
  {
    swprintf(line, _countof(line),
      L"pid=%lu hwnd=0x%p title=%ls rect=%ld,%ld,%ld,%ld\r\n",
      (unsigned long)pid,
      hwnd,
      title,
      (long)rc.left, (long)rc.top, (long)rc.right, (long)rc.bottom);
  }
  else
  {
    swprintf(line, _countof(line),
      L"pid=%lu no_top_level_window=1\r\n",
      (unsigned long)pid);
  }

  DWORD cbWritten = 0;
  WriteFile(hFile, line, (DWORD)wcslen(line) * sizeof(wchar_t), &cbWritten, nullptr);
  CloseHandle(hFile);
}

static bool GetCloakedState(HWND hwnd, BOOL& outCloaked)
{
  outCloaked = FALSE;
  if (!hwnd)
    return false;

  EnsureDwmGetWindowAttribute();
  if (pDwmGetWindowAttribute == nullptr)
    return false;

  HRESULT hr = pDwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &outCloaked, sizeof(outCloaked));
  if (FAILED(hr))
  {
    outCloaked = FALSE;
    return false;
  }

  return true;
}

static std::wstring GetRundllPath(bool is64)
{
  wchar_t wszRoot[MAX_PATH]{};
  const UINT cch = GetWindowsDirectoryW(wszRoot, (UINT)_countof(wszRoot));
  if (cch == 0 || cch >= (UINT)_countof(wszRoot))
    return L"";

  std::wstring p(wszRoot);
  if (!p.empty() && p.back() != L'\\')
    p += L"\\";

  if (is64)
    p += L"System32\\rundll32.exe";
  else
    p += L"SysWOW64\\rundll32.exe";

  return p;
}

static bool RunRundll(bool is64, const std::wstring& specialkDllFullPath, const wchar_t* verb, bool wait)
{
  const std::wstring rundll = GetRundllPath(is64);
  if (rundll.empty() || specialkDllFullPath.empty() || verb == nullptr || *verb == L'\0')
    return false;

  std::wstring cmd;
  cmd.reserve(rundll.size() + specialkDllFullPath.size() + 64);
  cmd += L"\"";
  cmd += rundll;
  cmd += L"\" \"";
  cmd += specialkDllFullPath;
  cmd += L"\",RunDLL_InjectionManager ";
  cmd += verb;

  std::wstring cwd = GetDirname(specialkDllFullPath);

  STARTUPINFOW si{};
  si.cb = sizeof(si);

  PROCESS_INFORMATION pi{};

  std::vector<wchar_t> cmdline(cmd.begin(), cmd.end());
  cmdline.push_back(L'\0');

  SetEnvironmentVariableA("SIDECARK_DIAGNOSTICS", nullptr);

  const BOOL ok = CreateProcessW(
    nullptr,
    cmdline.data(),
    nullptr,
    nullptr,
    FALSE,
    0,
    nullptr,
    cwd.c_str(),
    &si,
    &pi);

  if (!ok)
    return false;

  if (wait)
    WaitForSingleObject(pi.hProcess, INFINITE);

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

static bool WaitForModuleLoaded(DWORD pid, const wchar_t* moduleBasename, DWORD timeoutMs)
{
  if (pid == 0 || moduleBasename == nullptr || *moduleBasename == L'\0')
    return false;

  const ULONGLONG start = GetTickCount64();

  while (true)
  {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap != INVALID_HANDLE_VALUE)
    {
      MODULEENTRY32W me{};
      me.dwSize = sizeof(me);

      if (Module32FirstW(snap, &me))
      {
        do {
          if (_wcsicmp(me.szModule, moduleBasename) == 0)
          {
            CloseHandle(snap);
            return true;
          }
        } while (Module32NextW(snap, &me));
      }

      CloseHandle(snap);
    }

    if (GetTickCount64() - start >= timeoutMs)
      return false;

    Sleep(50);
  }
}


static std::atomic_bool g_shutdown{ false };
static bool g_frame_source_seen = false;

static DWORD g_host_pid = 0;

static HANDLE g_control_map = nullptr;
static void* g_control_view = nullptr;
static volatile uint32_t* g_control_overlay_enabled = nullptr;
static HANDLE g_control_pipe_thread = nullptr;

static HANDLE g_frame_host_map = nullptr;
static void* g_frame_host_view = nullptr;
static constexpr DWORD g_frame_host_size = 64u * 1024u * 1024u;

#pragma pack(push, 1)
struct SidecarKFrameHeaderV1
{
  char magic[4];
  uint32_t version;
  uint32_t header_bytes;
  uint32_t data_offset;
  uint32_t pixel_format;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  uint32_t frame_counter;
};
#pragma pack(pop)

static_assert(sizeof(SidecarKFrameHeaderV1) == 0x24, "SidecarKFrameHeaderV1 size mismatch");

static void CreateHostFrameMappingForPid(DWORD pid)
{
  if (pid == 0)
    return;

  if (g_frame_host_map != nullptr && g_frame_host_view != nullptr)
    return;

  wchar_t name[128]{};
  wsprintfW(name, L"Local\\SidecarK_Frame_%lu", (unsigned long)pid);

  g_frame_host_map = CreateFileMappingW(
    INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    g_frame_host_size,
    name);
  if (!g_frame_host_map)
    return;

  g_frame_host_view = MapViewOfFile(g_frame_host_map, FILE_MAP_ALL_ACCESS, 0, 0, g_frame_host_size);
  if (!g_frame_host_view)
  {
    CloseHandle(g_frame_host_map);
    g_frame_host_map = nullptr;
    return;
  }

  auto* hdr = reinterpret_cast<SidecarKFrameHeaderV1*>(g_frame_host_view);
  if (memcmp(hdr->magic, "SKF1", 4) != 0 || hdr->version != 1u)
  {
    memcpy(hdr->magic, "SKF1", 4);
    hdr->version = 1u;
    hdr->header_bytes = 0x20u;
    hdr->data_offset = 0x20u;
    hdr->pixel_format = 1u;
    hdr->width = 0u;
    hdr->height = 0u;
    hdr->stride = 0u;
    hdr->frame_counter = 0u;
  }
}


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

static void AppendLogf(const std::wstring& logPath, const wchar_t* fmt, ...)
{
  if (fmt == nullptr)
    return;

  wchar_t buf[2048]{};
  va_list args;
  va_start(args, fmt);
  _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
  va_end(args);
  AppendLog(logPath, buf);
}

static bool SidecarKHost_DiagnosticsEnabledForPid(DWORD pid)
{
  if (pid == 0)
    return false;

  wchar_t name[128]{};
  wsprintfW(name, L"Local\\SidecarK_Diagnostics_Enable_%lu", (unsigned long)pid);

  HANDLE hMap = OpenFileMappingW(FILE_MAP_READ, FALSE, name);
  if (!hMap)
    return false;

  bool ok = false;
  void* view = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 4);
  if (view)
  {
    ok = (memcmp(view, "SKD1", 4) == 0);
    UnmapViewOfFile(view);
  }

  CloseHandle(hMap);
  return ok;
}

static void HostLogAppend(const wchar_t* line)
{
  if (!line || !*line) return;

  if (g_host_pid == 0)
    g_host_pid = GetCurrentProcessId();

  if (!SidecarKHost_DiagnosticsEnabledForPid(g_host_pid))
    return;

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

static void HostLogAppendf(const wchar_t* fmt, ...)
{
  if (fmt == nullptr)
    return;

  wchar_t buf[2048]{};
  va_list args;
  va_start(args, fmt);
  _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
  va_end(args);
  HostLogAppend(buf);
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
  {
    const DWORD gle = GetLastError();
    HostLogAppendf(L"inject fail: OpenProcess gle=%lu (0x%08lX)", (unsigned long)gle, (unsigned long)gle);
    return InjectResult::Fail;
  }

  const size_t cb = (dllPath.size() + 1) * sizeof(wchar_t);
  void* remoteMem = VirtualAllocEx(hProcess, nullptr, cb, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!remoteMem)
  {
    const DWORD gle = GetLastError();
    HostLogAppendf(L"inject fail: VirtualAllocEx gle=%lu (0x%08lX)", (unsigned long)gle, (unsigned long)gle);
    CloseHandle(hProcess);
    return InjectResult::Fail;
  }

  if (!WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), cb, nullptr))
  {
    DWORD le = GetLastError();
    HostLogAppendf(L"inject fail: WriteProcessMemory gle=%lu (0x%08lX)", (unsigned long)le, (unsigned long)le);
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
    HostLogAppendf(L"inject fail: GetProcAddress(LoadLibraryW) gle=%lu (0x%08lX)", (unsigned long)le, (unsigned long)le);
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
    HostLogAppendf(L"inject fail: CreateRemoteThread gle=%lu (0x%08lX)", (unsigned long)le, (unsigned long)le);
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
  {
    const DWORD gle = GetLastError();
    HostLogAppendf(L"inject fail: GetExitCodeThread gle=%lu (0x%08lX)", (unsigned long)gle, (unsigned long)gle);
    return InjectResult::Fail;
  }

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

static void LogModuleCheckSpecialK32Delayed(const std::wstring& logPath, DWORD pid)
{
  Sleep(3000);

  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
  if (snap == INVALID_HANDLE_VALUE)
  {
    const DWORD gle = GetLastError();
    wchar_t msg[256]{};
    swprintf(msg, _countof(msg), L"module_check_delayed snapshot_fail pid=%lu gle=%lu",
      (unsigned long)pid,
      (unsigned long)gle);
    AppendLog(logPath, msg);
    HostLogAppend(msg);
    return;
  }

  MODULEENTRY32W me{};
  me.dwSize = sizeof(me);

  bool found = false;
  if (Module32FirstW(snap, &me))
  {
    do {
      if (_wcsicmp(me.szModule, L"SpecialK32.dll") == 0)
      {
        found = true;
        break;
      }
    } while (Module32NextW(snap, &me));
  }
  else
  {
    const DWORD gle = GetLastError();
    CloseHandle(snap);
    wchar_t msg[256]{};
    swprintf(msg, _countof(msg), L"module_check_delayed enum_fail pid=%lu gle=%lu",
      (unsigned long)pid,
      (unsigned long)gle);
    AppendLog(logPath, msg);
    HostLogAppend(msg);
    return;
  }

  CloseHandle(snap);

  {
    wchar_t msg[256]{};
    swprintf(msg, _countof(msg), L"module_check_delayed pid=%lu found_specialk32=%d",
      (unsigned long)pid,
      found ? 1 : 0);
    AppendLog(logPath, msg);
    HostLogAppend(msg);
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

static std::wstring ControlPipeNameForTarget(DWORD pid)
{
  wchar_t buf[128]{};
  swprintf(buf, _countof(buf), L"\\\\.\\pipe\\SidecarK_Control_%lu", (unsigned long)pid);
  return buf;
}

static bool InitSidecarKControlPlaneForPid(DWORD pid)
{
  if (pid == 0)
    return false;

  if (g_control_map != nullptr && g_control_view != nullptr && g_control_overlay_enabled != nullptr)
    return true;

  wchar_t name[128]{};
  wsprintfW(name, L"Local\\SidecarK_Control_%lu", (unsigned long)pid);

  g_control_map = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 4096, name);
  if (!g_control_map)
    return false;

  g_control_view = MapViewOfFile(g_control_map, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 4096);
  if (!g_control_view)
  {
    CloseHandle(g_control_map);
    g_control_map = nullptr;
    return false;
  }

  BYTE* base = reinterpret_cast<BYTE*>(g_control_view);
  uint32_t* ver = reinterpret_cast<uint32_t*>(base + 0x04);
  uint32_t* overlay_enabled = reinterpret_cast<uint32_t*>(base + 0x08);

  if (memcmp(base + 0x00, "SKC1", 4) != 0 || *ver != 1u)
  {
    memcpy(base + 0x00, "SKC1", 4);
    *ver = 1u;
    *overlay_enabled = 1u;
  }

  g_control_overlay_enabled = reinterpret_cast<volatile uint32_t*>(overlay_enabled);
  return true;
}

static void ShutdownSidecarKControlPlane()
{
  if (g_control_pipe_thread)
  {
    WaitForSingleObject(g_control_pipe_thread, 2000);
    CloseHandle(g_control_pipe_thread);
    g_control_pipe_thread = nullptr;
  }

  g_control_overlay_enabled = nullptr;

  if (g_control_view)
  {
    UnmapViewOfFile(g_control_view);
    g_control_view = nullptr;
  }

  if (g_control_map)
  {
    CloseHandle(g_control_map);
    g_control_map = nullptr;
  }
}

static void RunControlPipeServer(const std::wstring& pipeName, volatile uint32_t* overlayEnabled)
{
  while (!g_shutdown.load())
  {
    HANDLE hPipe = CreateNamedPipeW(
      pipeName.c_str(),
      PIPE_ACCESS_DUPLEX,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
      1,
      0,
      0,
      0,
      nullptr
    );

    if (hPipe == INVALID_HANDLE_VALUE)
      return;

    BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected)
    {
      CloseHandle(hPipe);
      continue;
    }

    char line[65]{};
    DWORD lineLen = 0;
    bool done = false;

    while (!g_shutdown.load())
    {
      char tmp[32]{};
      DWORD br = 0;
      if (!ReadFile(hPipe, tmp, (DWORD)sizeof(tmp), &br, nullptr) || br == 0)
        break;

      for (DWORD i = 0; i < br; ++i)
      {
        const char ch = tmp[i];
        if (ch == '\n')
        {
          line[lineLen] = '\0';
          done = true;
          break;
        }

        if (lineLen >= 64)
        {
          const char* resp = "err\n";
          DWORD bw = 0;
          WriteFile(hPipe, resp, 4, &bw, nullptr);
          FlushFileBuffers(hPipe);
          lineLen = 0;
          continue;
        }

        line[lineLen++] = ch;
      }

      if (done)
        break;
    }

    const char* resp = "err\n";
    DWORD respLen = 4;

    if (done)
    {
      if (strcmp(line, "overlay on") == 0)
      {
        if (overlayEnabled) *overlayEnabled = 1u;
        resp = "ok\n"; respLen = 3;
      }
      else if (strcmp(line, "overlay off") == 0)
      {
        if (overlayEnabled) *overlayEnabled = 0u;
        resp = "ok\n"; respLen = 3;
      }
      else if (strcmp(line, "ping") == 0)
      {
        resp = "pong\n"; respLen = 5;
      }
    }

    DWORD bw = 0;
    WriteFile(hPipe, resp, respLen, &bw, nullptr);
    FlushFileBuffers(hPipe);

    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
  }
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

  HostLogAppendMilestone(L"parsed_args_ok");

  const std::wstring exeDir = GetExeDir();

  std::wstring statusPath = ArgValue(argc, argv, L"--status");
  std::wstring logPath = ArgValue(argc, argv, L"--log");
  if (statusPath.empty()) statusPath = exeDir + L"\\status.json";
  if (logPath.empty())    logPath = exeDir + L"\\overlay_engine.log";

  AppendLog(logPath, L"starting");
  WriteStatusAtomic(statusPath, L"starting", 0, L"none");
  SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

  {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, (DWORD)_countof(exePath));
    AppendLogf(logPath, L"build_stamp exe_path=%ls pid=%lu compiled=%hs %hs",
      exePath,
      (unsigned long)GetCurrentProcessId(),
      __DATE__, __TIME__);
  }


  // ---- Determine target PID (no injection here) ----
  DWORD targetPid = 0;

  const std::wstring attachPidStr = ArgValue(argc, argv, L"--attach-pid");
  const std::wstring attachExe = ArgValue(argc, argv, L"--attach-exe");
  const std::wstring waitExe = ArgValue(argc, argv, L"--wait-for-exe");
  const std::wstring timeoutStr = ArgValue(argc, argv, L"--timeout-ms");
  const bool forceLate = HasFlag(argc, argv, L"--force-late");
  std::wstring injectRequested = ArgValue(argc, argv, L"--inject");
  const wchar_t* injectSource = L"argv";
  if (injectRequested.empty())
  {
    injectRequested = L"remote";
    injectSource = L"default";
  }

  const bool waitForExeMode = (!waitExe.empty());

  std::wstring injectMode = injectRequested;
  if (waitForExeMode)
  {
    if (_wcsicmp(injectRequested.c_str(), L"rundll") == 0)
    {
      AppendLog(logPath, L"WARN wait_for_exe_forces_remote: rundll hookchain is experimental/incompatible for this title; using remote injection with window gate.");
      HostLogAppend(L"WARN wait_for_exe_forces_remote: rundll hookchain is experimental/incompatible for this title; using remote injection with window gate.");
    }

    injectMode = L"remote";
  }

  const bool injectRemote = (_wcsicmp(injectMode.c_str(), L"remote") == 0);
  const bool injectRundll = (_wcsicmp(injectMode.c_str(), L"rundll") == 0);
  if (!injectRemote && !injectRundll)
  {
    AppendLog(logPath, L"error: invalid --inject (expected remote|rundll)");
    HostLogAppend(L"error: invalid --inject (expected remote|rundll)");
    WriteStatusAtomic(statusPath, L"error", 0, L"attach_failed");
    return ATTACH_FAILED;
  }

  const wchar_t* attachMode = attachPidStr.empty() ? (attachExe.empty() ? L"wait-for-exe" : L"attach-exe") : L"attach-pid";
  const std::wstring attachExeForLog = attachPidStr.empty() ? (attachExe.empty() ? Basename(waitExe) : Basename(attachExe)) : L"";
  const std::wstring specialkForLog = exeDir + L"\\SpecialK32.dll";
  const bool is64ForLog = (_wcsicmp(Basename(specialkForLog).c_str(), L"SpecialK64.dll") == 0);

  AppendLogf(logPath,
    L"startup inject_requested=%ls inject_mode=%ls inject_source=%ls attach_mode=%ls pid=%lu exe=%ls specialk=%ls is64=%d",
    injectRequested.c_str(),
    injectMode.c_str(),
    injectSource,
    attachMode,
    (unsigned long)0,
    attachExeForLog.c_str(),
    specialkForLog.c_str(),
    is64ForLog ? 1 : 0);
  HostLogAppendf(
    L"startup inject_requested=%ls inject_mode=%ls inject_source=%ls attach_mode=%ls pid=%lu exe=%ls specialk=%ls is64=%d",
    injectRequested.c_str(),
    injectMode.c_str(),
    injectSource,
    attachMode,
    (unsigned long)0,
    attachExeForLog.c_str(),
    specialkForLog.c_str(),
    is64ForLog ? 1 : 0);

  if (injectRundll)
  {
    if (_wcsicmp(attachMode, L"wait-for-exe") != 0)
    {
      if (!forceLate)
      {
        AppendLog(logPath, L"ERROR rundll_requires_prelaunch_workflow: use --wait-for-exe and start the host BEFORE launching the game.");
        HostLogAppend(L"ERROR rundll_requires_prelaunch_workflow: use --wait-for-exe and start the host BEFORE launching the game.");
        WriteStatusAtomic(statusPath, L"error", 0, L"attach_failed");
        return ATTACH_FAILED;
      }

      AppendLog(logPath, L"WARN rundll_late_attach_best_effort: nondeterministic for already-running processes; may never inject.");
      HostLogAppend(L"WARN rundll_late_attach_best_effort: nondeterministic for already-running processes; may never inject.");
    }
  }

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

    AppendLogf(logPath, L"wait_gate_pid pid=%lu exe=%ls", (unsigned long)targetPid, name.c_str());
    HostLogAppendf(L"wait_gate_pid pid=%lu exe=%ls", (unsigned long)targetPid, name.c_str());

    static constexpr DWORD POLL_MS = 100;
    static constexpr DWORD LOG_RATE_MS = 200;

    const ULONGLONG gateStartMs = GetTickCount64();
    ULONGLONG visibleSinceMs = 0;
    HWND lastHwnd = nullptr;
    int lastVis = 0;
    ULONGLONG lastLogMs = 0;

    HWND gateHwnd = nullptr;
    DWORD gateVisibleMs = 0;
    bool gateSeenDxgi = false;
    bool gateSeenD3d11 = false;
    bool gateSeenOpenGL = false;
    bool gateFallbackUsed = false;

    while (true)
    {
      HWND hwnd = FindTopLevelWindowForPid(targetPid);

      int vis = 0;
      if (hwnd != nullptr)
      {
        RECT rc{ 0,0,0,0 };
        if (GetWindowRect(hwnd, &rc))
        {
          const LONG ww = rc.right - rc.left;
          const LONG hh = rc.bottom - rc.top;
          if (ww >= 100 && hh >= 100 && IsWindowVisible(hwnd))
            vis = 1;
        }
      }

      const bool seenDxgi = HasModule(targetPid, L"dxgi.dll");
      const bool seenD3d11 = HasModule(targetPid, L"d3d11.dll");
      const bool seenOpenGL = HasModule(targetPid, L"opengl32.dll");

      gateSeenDxgi |= seenDxgi;
      gateSeenD3d11 |= seenD3d11;
      gateSeenOpenGL |= seenOpenGL;

      const bool strongReady = seenDxgi || seenD3d11;
      const bool weakReadyOnly = (!strongReady) && seenOpenGL;

      const ULONGLONG now = GetTickCount64();

      if (hwnd != lastHwnd || vis != lastVis)
      {
        if (now - lastLogMs >= LOG_RATE_MS)
        {
          lastLogMs = now;
          AppendLogf(logPath, L"wait_gate_hwnd pid=%lu hwnd=0x%p vis=%d", (unsigned long)targetPid, hwnd, vis);
          HostLogAppendf(L"wait_gate_hwnd pid=%lu hwnd=0x%p vis=%d", (unsigned long)targetPid, hwnd, vis);
        }
      }

      if (vis == 1)
      {
        if (hwnd != lastHwnd)
          visibleSinceMs = now;
        else if (visibleSinceMs == 0)
          visibleSinceMs = now;

        const DWORD visibleMs = (visibleSinceMs != 0) ? (DWORD)(now - visibleSinceMs) : 0u;

        const DWORD requiredMs = strongReady ? 400u : (weakReadyOnly ? 1500u : 0u);

        const bool gfxReady = strongReady || weakReadyOnly;
        const bool gateReady = (hwnd != nullptr) && (vis == 1) && gfxReady && (requiredMs > 0u) && (visibleMs >= requiredMs);

        if (gateReady)
        {
          gateHwnd = hwnd;
          gateVisibleMs = visibleMs;
          break;
        }
      }
      else
      {
        visibleSinceMs = 0;
      }

      const ULONGLONG elapsed = now - gateStartMs;
      if (elapsed >= timeoutMs)
      {
        gateFallbackUsed = true;
        gateHwnd = hwnd;
        gateVisibleMs = (visibleSinceMs != 0) ? (DWORD)(now - visibleSinceMs) : 0u;
        AppendLogf(logPath, L"wait_gate_timeout pid=%lu ms=%lu proceeding_best_effort=1", (unsigned long)targetPid, (unsigned long)elapsed);
        HostLogAppendf(L"wait_gate_timeout pid=%lu ms=%lu proceeding_best_effort=1", (unsigned long)targetPid, (unsigned long)elapsed);
        break;
      }

      lastHwnd = hwnd;
      lastVis = vis;
      Sleep(POLL_MS);
    }

    HostLogAppendf(L"wait_gate_preinject pid=%lu hwnd=0x%p window_visible_ms=%lu gfx_modules_seen:dxgi=%d d3d11=%d opengl32=%d fallback=%d",
      (unsigned long)targetPid,
      gateHwnd,
      (unsigned long)gateVisibleMs,
      gateSeenDxgi ? 1 : 0,
      gateSeenD3d11 ? 1 : 0,
      gateSeenOpenGL ? 1 : 0,
      gateFallbackUsed ? 1 : 0);

    WriteWindowProofOnceForPid(targetPid, gateHwnd);

    // Deterministic selection log (after PID choice)
    LogTargetSelected(logPath, targetPid, Basename(waitExe));
  }

  if (!attachPidStr.empty())
  {
    // Deterministic selection log (after PID choice)
    LogTargetSelected(logPath, targetPid, L"");
  }

  CreateHostFrameMappingForPid(targetPid);
  CreateDiagnosticsEnableTokenForPid(targetPid);

  InitSidecarKControlPlaneForPid(targetPid);

  AppendLog(logPath, L"attached");
  HostLogAppendMilestone(L"begin_attach");
  WriteStatusAtomic(statusPath, L"attached", targetPid, L"none");

  bool target_is64 = false;
  {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, targetPid);
    if (hProc != nullptr)
    {
      typedef BOOL(WINAPI* IsWow64Process2_t)(HANDLE, USHORT*, USHORT*);
      const auto pIsWow64Process2 = reinterpret_cast<IsWow64Process2_t>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "IsWow64Process2"));

      if (pIsWow64Process2 != nullptr)
      {
        USHORT processMachine = IMAGE_FILE_MACHINE_UNKNOWN;
        USHORT nativeMachine = IMAGE_FILE_MACHINE_UNKNOWN;
        if (pIsWow64Process2(hProc, &processMachine, &nativeMachine))
          target_is64 = (processMachine == IMAGE_FILE_MACHINE_UNKNOWN) && (nativeMachine != IMAGE_FILE_MACHINE_UNKNOWN);
      }
      else
      {
        BOOL wow64 = FALSE;
        if (IsWow64Process(hProc, &wow64))
          target_is64 = (wow64 == FALSE) && (sizeof(void*) == 8);
      }

      CloseHandle(hProc);
    }
  }

  const std::wstring dllToInject =
    exeDir + (target_is64 ? L"\\SpecialK64.dll" : L"\\SpecialK32.dll");

  AppendLogf(logPath, L"inject_select pid=%lu host_ptr_bits=%d target_is64=%d dll=%ls",
    (unsigned long)targetPid,
    (int)(sizeof(void*) * 8),
    target_is64 ? 1 : 0,
    dllToInject.c_str());
  HostLogAppendf(L"inject_select pid=%lu host_ptr_bits=%d target_is64=%d dll=%ls",
    (unsigned long)targetPid,
    (int)(sizeof(void*) * 8),
    target_is64 ? 1 : 0,
    dllToInject.c_str());

  if ((sizeof(void*) == 4) && target_is64)
  {
    AppendLog(logPath, L"error: 32-bit host cannot inject into 64-bit target");
    HostLogAppend(L"error: 32-bit host cannot inject into 64-bit target");
    WriteStatusAtomic(statusPath, L"error", targetPid, L"attach_failed");
    return ATTACH_FAILED;
  }

  if (g_control_overlay_enabled != nullptr)
  {
    const std::wstring controlPipeName = ControlPipeNameForTarget(targetPid);
    g_control_pipe_thread = CreateThread(
      nullptr, 0,
      [](LPVOID p) -> DWORD {
        auto* ctx = reinterpret_cast<std::tuple<std::wstring, volatile uint32_t*>*>(p);
        RunControlPipeServer(std::get<0>(*ctx), std::get<1>(*ctx));
        delete ctx;
        return 0;
      },
      new std::tuple<std::wstring, volatile uint32_t*>(controlPipeName, g_control_overlay_enabled),
      0, nullptr
    );
  }

  const bool is64 = (_wcsicmp(Basename(dllToInject).c_str(), L"SpecialK64.dll") == 0);
  const wchar_t* moduleBasename = is64 ? L"SpecialK64.dll" : L"SpecialK32.dll";
  const std::wstring specialkDllFullPath = dllToInject;

  bool rundllInstallAttempted = false;
  auto RundllRemoveGuard = [&]() {
    if (rundllInstallAttempted)
    {
      AppendLogf(logPath, L"rundll_remove_start pid=%lu", (unsigned long)targetPid);
      HostLogAppend(L"rundll_remove_start");
      RunRundll(is64, specialkDllFullPath, L"Remove", true);
      AppendLogf(logPath, L"rundll_remove_ok pid=%lu", (unsigned long)targetPid);
      HostLogAppend(L"rundll_remove_ok");
    }
  };

  // Deterministic injection + module verification logging (host-side)
  {
    wchar_t msg[1024]{};
    swprintf(msg, _countof(msg), L"inject_attempt pid=%lu dll=%s",
      (unsigned long)targetPid,
      dllToInject.c_str());
    AppendLog(logPath, msg);
    HostLogAppend(msg);
  }

  HostLogAppendMilestone(L"begin_inject");

  if (injectRundll)
  {
    rundllInstallAttempted = true;

    const std::wstring rundllPath = GetRundllPath(is64);
    AppendLogf(logPath, L"rundll_install_start pid=%lu rundll=%ls dll=%ls", (unsigned long)targetPid, rundllPath.c_str(), specialkDllFullPath.c_str());
    HostLogAppendf(L"rundll_install_start pid=%lu rundll=%ls dll=%ls", (unsigned long)targetPid, rundllPath.c_str(), specialkDllFullPath.c_str());

    if (!RunRundll(is64, specialkDllFullPath, L"Install", false))
    {
      RundllRemoveGuard();
      return ATTACH_FAILED;
    }

    AppendLogf(logPath, L"rundll_install_spawned pid=%lu", (unsigned long)targetPid);
    HostLogAppendf(L"rundll_install_spawned pid=%lu", (unsigned long)targetPid);

    if (!WaitForModuleLoaded(targetPid, moduleBasename, 5000))
    {
      AppendLogf(logPath, L"rundll_module_timeout pid=%lu module=%ls", (unsigned long)targetPid, moduleBasename);
      HostLogAppendf(L"rundll_module_timeout pid=%lu module=%ls", (unsigned long)targetPid, moduleBasename);
      RunRundll(is64, specialkDllFullPath, L"Remove", true);
      return ATTACH_FAILED;
    }

    AppendLogf(logPath, L"rundll_module_seen pid=%lu module=%ls", (unsigned long)targetPid, moduleBasename);
    HostLogAppendf(L"rundll_module_seen pid=%lu module=%ls", (unsigned long)targetPid, moduleBasename);
  }
  else
  {
    if (waitForExeMode)
    {
      const std::wstring targetExeFullPath = GetProcessImagePath(targetPid);
      LogTargetGraphicsSnapshot(targetPid, targetExeFullPath);
    }

    AppendLogf(logPath, L"remote_inject_start pid=%lu dll=%ls", (unsigned long)targetPid, dllToInject.c_str());
    HostLogAppendf(L"remote_inject_start pid=%lu dll=%ls", (unsigned long)targetPid, dllToInject.c_str());

    const InjectResult injectResult = InjectDllByCreateRemoteThread(targetPid, dllToInject);
    const DWORD injectLastError = GetLastError();

    const int remoteOk = (injectResult == InjectResult::Success || injectResult == InjectResult::AlreadyLoaded) ? 1 : 0;
    AppendLogf(logPath, L"remote_inject_result pid=%lu ok=%d last_error=%lu", (unsigned long)targetPid, remoteOk, (unsigned long)injectLastError);
    HostLogAppendf(L"remote_inject_result pid=%lu ok=%d last_error=%lu", (unsigned long)targetPid, remoteOk, (unsigned long)injectLastError);

    {
      wchar_t msg[256]{};
      swprintf(msg, _countof(msg), L"inject_result=%s last_error=%lu",
        InjectResultToString(injectResult),
        (unsigned long)injectLastError);
      AppendLog(logPath, msg);
      HostLogAppend(msg);
    }

    LogModuleCheckSpecialK32(logPath, targetPid);

    {
      MODULEENTRY32W me{};
      const bool found = GetMatchingModuleInfo(targetPid, L"SpecialK32.dll", me);
      if (found)
      {
        const std::wstring targetExeFullPath = GetProcessImagePath(targetPid);
        LogTargetGraphicsSnapshot(targetPid, targetExeFullPath);
      }
    }

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

    if (remoteOk == 1)
    {
      MODULEENTRY32W me{};
      const bool found = GetMatchingModuleInfo(targetPid, L"SpecialK32.dll", me);
      if (found)
        LogModuleCheckSpecialK32Delayed(logPath, targetPid);
    }
  }

  const std::wstring pipeName = PipeNameForTarget(targetPid);
  AppendLog(logPath, (L"pipe: " + pipeName).c_str());
  WriteStatusAtomic(statusPath, L"attached", targetPid, L"none");

  CreateHostFrameMappingForPid(targetPid);




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
      if (kEnableLegacySKOF_DebugOnly)
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

  RundllRemoveGuard();
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

  ShutdownSidecarKControlPlane();

  ReleaseFrameMapping();


  // Placeholder: next step adds named-pipe control + visible/hidden state.
  AppendLog(logPath, L"exiting");
  HostLogAppendMilestone(L"end");
  WriteStatusAtomic(statusPath, L"exiting", targetPid, L"none");
  return OK;
}
