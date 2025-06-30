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

struct denuvo_file_s
{
  AppId64_t    app;
  CSteamID     user;
  uint64       hash;
  std::wstring path;
  FILETIME     ft_key;
  SYSTEMTIME   st_local;
};

SK_LazyGlobal<std::vector<denuvo_file_s>> denuvo_files;

extern const wchar_t* SK_GetSteamDir();

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

  FindHandle(FindHandle&& other) noexcept : hFind(other.hFind) { other.hFind = INVALID_HANDLE_VALUE; }
  FindHandle& operator=(FindHandle&& other) noexcept
  {
    if (this != &other)
    {
      if (hFind != INVALID_HANDLE_VALUE) FindClose(hFind);
      hFind = other.hFind;
      other.hFind = INVALID_HANDLE_VALUE;
    }
    return *this;
  }
};

bool SK_Denuvo_UsedByGame(bool retest)
{
  static bool result = false;
  static bool tested = false;

  if (retest)
  {
    tested = false;
    denuvo_files->clear();
  }

  if (tested)
    return result;

  CSteamID usr_id = SK::SteamAPI::UserSteamID();
  AppId64_t app_id = SK::SteamAPI::AppID();
  std::wstring basePath = SK_GetSteamDir();

  wchar_t searchPath[MAX_PATH + 2] = {};
  swprintf_s(searchPath, MAX_PATH, LR"(%ws\userdata\%u\%llu\*)", basePath.c_str(), usr_id.GetAccountID(), app_id);

  WIN32_FIND_DATAW fd = {};

  FindHandle hFind(FindFirstFileW(searchPath, &fd));
  if (hFind == INVALID_HANDLE_VALUE)
  {
    tested = true;
    return false;
  }

  int fileCount = 0;
  LARGE_INTEGER totalSize = { 0 };

  do
  {
    if ((fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES) &&
      (wcslen(fd.cFileName) >= 8 && wcslen(fd.cFileName) <= 11) &&
      (wcschr(fd.cFileName, L'.') == nullptr) &&
      (fd.nFileSizeLow > 1024UL && fd.nFileSizeLow <= 8192UL))
    {
      denuvo_file_s file{};
      file.app = app_id;
      file.user = usr_id;
      file.hash = _wtoll(fd.cFileName);
      file.path = std::wstring(LR"()") + basePath + L"\\userdata\\" + std::to_wstring(usr_id.GetAccountID()) + L"\\" + std::to_wstring(app_id) + L"\\" + fd.cFileName;
      file.ft_key = fd.ftLastWriteTime;

      SK_AutoHandle hFile(CreateFileW(file.path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
      if (hFile != INVALID_HANDLE_VALUE)
      {
        SYSTEMTIME stUTC = {};
        FileTimeToSystemTime(&file.ft_key, &stUTC);
        SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &file.st_local);

        denuvo_files->emplace_back(std::move(file));
        ++fileCount;

        LARGE_INTEGER fileSize = {};
        fileSize.HighPart = fd.nFileSizeHigh;
        fileSize.LowPart = fd.nFileSizeLow;
        totalSize.QuadPart += fileSize.QuadPart;
      }
    }
  } while (FindNextFileW(hFind.get(), &fd));

  if (fileCount > 0)
  {
    std::sort(denuvo_files->begin(), denuvo_files->end(),
      [](const denuvo_file_s& a, const denuvo_file_s& b)
      {
        return CompareFileTime(&a.ft_key, &b.ft_key) < 0;
      });
    result = true;
  }

  tested = true;
  return result;
}
