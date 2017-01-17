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

bool
__stdcall
SK_FetchVersionInfo1 (const wchar_t* wszProduct, bool force)
{
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

  swprintf ( wszRemoteRepoURL,
               L"/Kaldaien/%s/master/version.ini",
                 wszProduct );

  PCWSTR  rgpszAcceptTypes []         = { L"*/*", nullptr };

  HINTERNET hInetGitHubOpen =
    HttpOpenRequest ( hInetGitHub,
                        nullptr,
                          wszRemoteRepoURL,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                  INTERNET_FLAG_MAKE_PERSISTENT   | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                  INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_IGNORE_CERT_CN_INVALID   |
                                  INTERNET_FLAG_RESYNCHRONIZE     | INTERNET_FLAG_CACHE_ASYNC              |
                                  INTERNET_FLAG_NEED_FILE,
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
  return SK_FetchVersionInfo1 (wszProduct);
}