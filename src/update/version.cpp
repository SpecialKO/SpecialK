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
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS

#include <SpecialK/ini.h>
#include <SpecialK/parameter.h>

#include <SpecialK/utility.h>
#include <SpecialK/log.h>

#include <SpecialK/update/version.h>

#include <Windows.h>
#include <Wininet.h>
#pragma comment (lib, "wininet.lib")

#include <cstdint>

DWORD dwInetCtx;

std::wstring __SK_LastProductTested = L"";

bool
__stdcall
SK_FetchVersionInfo1 (const wchar_t* wszProduct, bool force)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested.c_str ();

  __SK_LastProductTested = wszProduct;

  FILETIME                  ftNow;
  GetSystemTimeAsFileTime (&ftNow);

  ULARGE_INTEGER uliNow;

  uliNow.LowPart  = ftNow.dwLowDateTime;
  uliNow.HighPart = ftNow.dwHighDateTime;


  wchar_t wszInstallFile [MAX_PATH] = { L'\0' };
  wchar_t wszRepoFile    [MAX_PATH] = { L'\0' };

  extern const wchar_t* __stdcall SK_GetRootPath (void);

  //Profiles/
  //PlugIns/

  //Version/...
  //Version/<OLD>/...

  //installed.ini
  //repository.ini

  //UPDATE_<tmp>.ini
  //UPDATE_<tmp>.7z


  /// LocalName=UnX  GlobalName=PlugIns/UnX
  //UnX_Ver/... (PlugIns/UnX/Version/...)
  // installed.ini
  // <OLD>/...

  //UnX_Res/... (PlugIns/UnX/Resources/...)


  //// LocalName=TSFix  GlobalName=PlugIns/TSFix

  //TSFix_Ver/... (PlugIns/TSFix/Version/...)
  // installed.ini
  // <OLD>/...

  //TSFix_Res/... (PlugIns/TSFix/Resources/...)


  lstrcatW (wszInstallFile, SK_GetRootPath ());
  lstrcatW (wszInstallFile, L"Version\\");

  lstrcatW (wszRepoFile, SK_GetRootPath ());
  lstrcatW (wszRepoFile, L"Version\\");

  SK_CreateDirectories (wszRepoFile);

  lstrcatW (wszInstallFile, L"installed.ini");
  lstrcatW (wszRepoFile,    L"repository.ini");

  bool need_remind  = false,
       has_remind   = false;

  bool should_fetch = true;

  // Update frequency (measured in 100ns)
  ULONGLONG update_freq = 0ULL;

  if (GetFileAttributes (wszInstallFile) != INVALID_FILE_ATTRIBUTES) {
    iSK_INI install_ini (wszInstallFile);

    install_ini.parse ();

    if (! install_ini.get_sections ().empty ()) {
      iSK_INISection& user_prefs =
        install_ini.get_section (L"Update.User");

      bool has_freq =
        user_prefs.contains_key (L"Frequency");

      std::wstring freq =
        user_prefs.get_value (L"Frequency");

      wchar_t h [3] = { L"0" },
              d [3] = { L"0" },
              w [3] = { L"0" };

      swscanf ( freq.c_str (), L"%3[^h]h", h );

      // Default to 6h if unspecified
      if (! has_freq) {
        wcscpy (h, L"6");

        install_ini.import (L"[Update.User]\nFrequency=6h\n\n");
        install_ini.write  (wszInstallFile);
      }

      const ULONGLONG _Hour = 36000000000ULL;

      update_freq += (   1ULL * _Hour * _wtoi (h) );
      update_freq += (  24ULL * _Hour * _wtoi (d) );
      update_freq += ( 168ULL * _Hour * _wtoi (w) );

      //dll_log.Log ( L"Update Frequency: %lu hours, %lu days, %lu weeks (%s)",
                      //_wtoi (h), _wtoi (d), _wtoi (w), freq.c_str () );

      if (freq == L"never")
        update_freq = MAXLONGLONG;

      else if (user_prefs.contains_key (L"Reminder"))
      {
        has_remind = true;

        sk::ParameterFactory ParameterFactory;

        sk::ParameterInt64* remind_time =
          (sk::ParameterInt64 *)
            ParameterFactory.create_parameter <int64_t> (L"Reminder");

        remind_time->register_to_ini (
          &install_ini,
            L"Update.User",
              L"Reminder"
        );

        remind_time->load ();

        if (uliNow.QuadPart >= (uint64_t)remind_time->get_value ()) {
          need_remind = true;

          user_prefs.remove_key (L"Reminder");
          install_ini.write (wszInstallFile);
        }

        delete remind_time;
      }
    }
  }

  if (GetFileAttributes (wszRepoFile) != INVALID_FILE_ATTRIBUTES)
  {
    HANDLE hVersionConfig =
      CreateFile ( wszRepoFile,
                     GENERIC_READ,
                       FILE_SHARE_READ  |
                       FILE_SHARE_WRITE |
                       FILE_SHARE_DELETE,
                         nullptr,
                           OPEN_EXISTING,
                             GetFileAttributes (wszRepoFile) |
                             FILE_FLAG_SEQUENTIAL_SCAN,
                               nullptr );

    if (hVersionConfig != INVALID_HANDLE_VALUE) {
      FILETIME ftModify;

      if (GetFileTime (hVersionConfig, nullptr, nullptr, &ftModify))
      {
        ULARGE_INTEGER uliModify;

        uliModify.LowPart  = ftModify.dwLowDateTime;
        uliModify.HighPart = ftModify.dwHighDateTime;

        // Check Version:  User Preference (default=6h)
        if ((uliNow.QuadPart - uliModify.QuadPart) < update_freq) {
          should_fetch = false;
        }
      }

      CloseHandle (hVersionConfig);
    }
  }

  //
  // If a reminder is setup, allow it to override the user's
  //   normal check frequency.
  //
  //   So that, for example, a 6h frequency can be delayed up to 1 week.
  //
  if (! force) {
    if (! (( should_fetch && (! has_remind)) || need_remind))
      return false;
  }

  HINTERNET hInetRoot =
    InternetOpen (
      L"Special K Auto-Update",
        INTERNET_OPEN_TYPE_DIRECT,
          nullptr, nullptr,
            0x00
    );

  if (! hInetRoot)
    return false;

  HINTERNET hInetGitHub =
    InternetConnect ( hInetRoot,
                        L"raw.githubusercontent.com",
                          INTERNET_DEFAULT_HTTP_PORT,
                            nullptr, nullptr,
                              INTERNET_SERVICE_HTTP,
                                0x00,
                                  (DWORD_PTR)&dwInetCtx );

  if (! hInetGitHub) {
    InternetCloseHandle (hInetRoot);
    return false;
  }

  wchar_t wszRemoteRepoURL [MAX_PATH] = { L'\0' };

  // TEMPORARILY REBASE TO 0.8.X
  if (! lstrcmpW (wszProduct, L"SpecialK"))
    swprintf ( wszRemoteRepoURL,
                 L"/Kaldaien/%s/0.8.x/version.ini",
                   wszProduct );
  else
    swprintf ( wszRemoteRepoURL,
                 L"/Kaldaien/%s/master/version.ini",
                   wszProduct );

  PCWSTR  rgpszAcceptTypes []         = { L"*/*", nullptr };

  DWORD dwFlags = INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_CACHE_IF_NET_FAIL |
                  INTERNET_FLAG_IGNORE_CERT_CN_INVALID   | INTERNET_FLAG_NEED_FILE         |
                  INTERNET_FLAG_PRAGMA_NOCACHE           | INTERNET_FLAG_RESYNCHRONIZE;

  extern bool SK_IsSuperSpecialK (void);

  if (SK_IsSuperSpecialK ())
    dwFlags |= INTERNET_FLAG_RELOAD;

  HINTERNET hInetGitHubOpen =
    HttpOpenRequest ( hInetGitHub,
                        nullptr,
                          wszRemoteRepoURL,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                  dwFlags,
                                    (DWORD_PTR)&dwInetCtx );

  if (! hInetGitHubOpen) {
    InternetCloseHandle (hInetGitHub);
    InternetCloseHandle (hInetRoot);
    return false;
  }

  if ( HttpSendRequestW ( hInetGitHubOpen,
                            nullptr,
                              0,
                                nullptr,
                                  0 ) ) {
    DWORD dwSize;

    if ( InternetQueryDataAvailable ( hInetGitHubOpen,
                                        &dwSize,
                                          0x00, NULL )
      )
    {
      DWORD dwAttribs = GetFileAttributes (wszRepoFile);

      if (dwAttribs == INVALID_FILE_ATTRIBUTES)
        dwAttribs = FILE_ATTRIBUTE_NORMAL;

      HANDLE hVersionFile =
        CreateFileW ( wszRepoFile,
                        GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                            nullptr,
                              CREATE_ALWAYS,
                                dwAttribs |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr );

      while (hVersionFile != INVALID_HANDLE_VALUE && dwSize > 0) {
        DWORD    dwRead = 0;
        uint8_t *pData  = (uint8_t *)malloc (dwSize);

        if (! pData)
          break;

        if ( InternetReadFile ( hInetGitHubOpen,
                                  pData,
                                    dwSize,
                                      &dwRead )
          )
        {
          DWORD dwWritten;

          WriteFile ( hVersionFile,
                        pData,
                          dwRead,
                            &dwWritten,
                              nullptr );
        }

        free (pData);
        pData = nullptr;

        if (! InternetQueryDataAvailable ( hInetGitHubOpen,
                                             &dwSize,
                                               0x00, NULL
                                         )
           ) break;
      }

      if (hVersionFile != INVALID_HANDLE_VALUE)
        CloseHandle (hVersionFile);
    }
  }

  InternetCloseHandle (hInetGitHubOpen);
  InternetCloseHandle (hInetGitHub);
  InternetCloseHandle (hInetRoot);

  return true;
}

bool
__stdcall
SK_FetchVersionInfo (const wchar_t* wszProduct)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested.c_str ();

  return SK_FetchVersionInfo1 (wszProduct);
}


#include <SpecialK/core.h>

std::wstring
SK_SYS_GetInstallPath (void)
{
  if (SK_IsInjected ())
  {
    std::wstring path  = SK_GetDocumentsDir ();
                 path += L"\\My Mods\\SpecialK";
          return path;
  }

  return SK_GetHostPath ();
}

std::wstring
SK_Version_GetInstallIniPath (void)
{
  return SK_SYS_GetInstallPath () + std::wstring (L"\\Version\\installed.ini");
}

std::wstring
SK_Version_GetRepoIniPath (void)
{
  return SK_SYS_GetInstallPath () + std::wstring (L"\\Version\\repository.ini");
}

SK_VersionInfo
SK_Version_GetLatestInfo_V1 (const wchar_t* wszProduct)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested.c_str ();

  SK_FetchVersionInfo (wszProduct);

  iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());
  iSK_INI repo_ini    (SK_Version_GetRepoIniPath    ().c_str ());

  install_ini.parse ();
  repo_ini.parse    ();

  SK_VersionInfo_V1 ver_info;
  ver_info.branch   = install_ini.get_section (L"Version.Local").get_value (L"Branch");

  wchar_t wszBranchSection [128] = { L'\0' };
  _swprintf (wszBranchSection, L"Version.%s", ver_info.branch.c_str ());

  if (repo_ini.contains_section (wszBranchSection)) {
    ver_info.package = repo_ini.get_section (wszBranchSection).get_value (L"InstallPackage");

    wchar_t wszPackage [128] = { L'\0' };
    swscanf (ver_info.package.c_str (), L"%128[^,],%li", wszPackage, &ver_info.build);

    ver_info.package = wszPackage;
  }

  else {
    ver_info.package = L"Invalid";
    ver_info.branch  = L"Invalid";
    ver_info.build   = -1;
  }

  return ver_info;
}

SK_VersionInfo
SK_Version_GetLocalInfo_V1 (const wchar_t* wszProduct)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested.c_str ();

  SK_FetchVersionInfo (wszProduct);

  iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());

  install_ini.parse ();

  SK_VersionInfo_V1 ver_info;
  ver_info.branch   = install_ini.get_section (L"Version.Local").get_value (L"Branch");
  ver_info.package  = install_ini.get_section (L"Version.Local").get_value (L"InstallPackage");

  wchar_t wszPackage [128] = { L'\0' };
  swscanf (ver_info.package.c_str (), L"%128[^,],%li", wszPackage, &ver_info.build);

  ver_info.package = wszPackage;

  return ver_info;
}


std::wstring
SK_Version_GetLastCheckTime_WStr (void)
{
  WIN32_FIND_DATA FindFileData;

  HANDLE hFileBackup = FindFirstFile (std::wstring (SK_Version_GetRepoIniPath ()).c_str (), &FindFileData);

  FILETIME   ftModified;
  FileTimeToLocalFileTime (&FindFileData.ftLastWriteTime, &ftModified);

  SYSTEMTIME stModified;
  FileTimeToSystemTime (&ftModified, &stModified);

  FindClose (hFileBackup);

  wchar_t wszFileTime [512];

  GetDateFormat (LOCALE_CUSTOM_UI_DEFAULT, DATE_AUTOLAYOUT, &stModified, NULL, wszFileTime, 512);

  std::wstring date_time = wszFileTime;

  GetTimeFormat (LOCALE_CUSTOM_UI_DEFAULT, TIME_NOSECONDS, &stModified, NULL, wszFileTime, 512);

  date_time += L" ";
  date_time += wszFileTime;

  return date_time;
}


#include <Shlwapi.h>

std::vector <std::string>
SK_Version_GetAvailableBranches (const wchar_t* wszProduct)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested.c_str ();

  std::vector <std::string> branches;

  SK_FetchVersionInfo (wszProduct);

  iSK_INI repo_ini (SK_Version_GetRepoIniPath    ().c_str ());
  repo_ini.parse   ();

  iSK_INI::_TSectionMap& sections =
    repo_ini.get_sections ();

  for ( auto it : sections )
  {
    if (StrStrIW (it.first.c_str (), L"Version.") == it.first.c_str ())
    {
      wchar_t wszBranchName [128] = { L'\0' };
      swscanf (it.first.c_str (), L"Version.%s", wszBranchName);

      branches.push_back (SK_WideCharToUTF8 (wszBranchName));
    }
  }

  return branches;
}

bool
SK_Version_SwitchBranches (const wchar_t* wszProduct, const char* szBranch)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested.c_str ();

  iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());
  install_ini.parse   ();

  // TODO: Validate selected branch against those in the repository

  wchar_t wszBranch [128] = { L'\0' };
  swprintf (wszBranch, L"%hs", szBranch);

  install_ini.get_section (L"Version.Local").get_value (L"Branch")         = wszBranch;
  install_ini.get_section (L"Version.Local").get_value (L"InstallPackage") = L"PendingBranchMigration,0";

  install_ini.write (SK_Version_GetInstallIniPath ().c_str ());

  return true;
}