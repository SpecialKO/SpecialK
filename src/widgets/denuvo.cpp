/**
 * This file is part of Special K.
 *
 * Special K is free software: you can redistribute it
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
#include <concurrent_vector.h>
#include <ppltasks.h>    // for concurrency::concurrent_vector

struct denuvo_file_s
{
  AppId64_t    app;
  CSteamID     user;
  uint64       hash;
  std::wstring path;
  FILETIME     ft_key;
  SYSTEMTIME   st_local;
  uint32       epoch;    // scan epoch marker
};

// lock‑free, thread‑safe storage
SK_LazyGlobal<concurrency::concurrent_vector<denuvo_file_s>> denuvo_files;

// simple epoch counter — bump on each retest
static std::atomic<uint32> g_denuvo_epoch{ 0 };

extern const wchar_t* SK_GetSteamDir();

// RAII wrapper for FindFirstFileW handle
class FindHandle
{
  HANDLE hFind;
public:
  explicit FindHandle(HANDLE h) : hFind(h) {}
  ~FindHandle() { if (hFind != INVALID_HANDLE_VALUE) FindClose(hFind); }
  operator HANDLE() const { return hFind; }
  HANDLE get() const { return hFind; }

  FindHandle(const FindHandle&) = delete;
  FindHandle& operator=(const FindHandle&) = delete;

  FindHandle(FindHandle&& o) noexcept : hFind(o.hFind) { o.hFind = INVALID_HANDLE_VALUE; }
  FindHandle& operator=(FindHandle&& o) noexcept
  {
    if (this != &o)
    {
      if (hFind != INVALID_HANDLE_VALUE) FindClose(hFind);
      hFind = o.hFind;
      o.hFind = INVALID_HANDLE_VALUE;
    }
    return *this;
  }
};

// Detects Denuvo files for the current user/app. Thread‑safe, lock‑free.
// On retest==true, clears the “tested” state and bumps the epoch.
bool SK_Denuvo_UsedByGame(bool retest)
{
  static std::atomic<bool> tested{ false };
  static std::atomic<bool> result{ false };

  if (retest)
  {
    tested = false;
    result = false;
    ++g_denuvo_epoch;
  }

  if (tested.load(std::memory_order_acquire))
    return result.load(std::memory_order_relaxed);

  // Steam IDs and base path
  CSteamID   usr_id = SK::SteamAPI::UserSteamID();
  AppId64_t  app_id = SK::SteamAPI::AppID();
  std::wstring basePath = SK_GetSteamDir();

  // Build "<Steam>\userdata\<user>\<app>\*"
  wchar_t searchPath[MAX_PATH + 2] = {};
  swprintf_s(searchPath, MAX_PATH,
    LR"(%ws\userdata\%u\%llu\*)",
    basePath.c_str(),
    usr_id.GetAccountID(),
    app_id);

  // Begin enumeration
  WIN32_FIND_DATAW fd = {};
  FindHandle hFind(FindFirstFileW(searchPath, &fd));
  if (hFind == INVALID_HANDLE_VALUE)
  {
    tested = true;
    result = false;
    return false;
  }

  uint32 thisEpoch = g_denuvo_epoch.load();
  int    foundCount = 0;

  do
  {
    // Cache filename length once and include attribute guard
    size_t len = wcslen(fd.cFileName);
    if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES
      && len >= 8
      && len <= 11
      && wcschr(fd.cFileName, L'.') == nullptr
      && fd.nFileSizeLow > 1024UL
      && fd.nFileSizeLow <= 8192UL)
    {
      denuvo_file_s entry{};
      entry.app = app_id;
      entry.user = usr_id;
      entry.hash = _wtoll(fd.cFileName);
      entry.ft_key = fd.ftLastWriteTime;
      entry.epoch = thisEpoch;

      // Full path: "<base>\userdata\<user>\<app>\filename"
      entry.path = basePath + L"\\userdata\\" +
        std::to_wstring(usr_id.GetAccountID()) + L"\\" +
        std::to_wstring(app_id) + L"\\" +
        fd.cFileName;

      // Convert timestamp to local SYSTEMTIME
      SYSTEMTIME stUTC = {};
      FileTimeToSystemTime(&entry.ft_key, &stUTC);
      SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &entry.st_local);

      // Push into lock‑free container
      denuvo_files->push_back(std::move(entry));
      ++foundCount;
    }
  } while (FindNextFileW(hFind.get(), &fd));

  if (foundCount > 0)
    result = true;

  tested = true;
  return result;
}
