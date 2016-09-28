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

#include <Windows.h>
#include <Wininet.h>
#pragma comment (lib, "wininet.lib")

extern const wchar_t*
__stdcall
SK_GetRootPath (void);

#include <cstdint>

DWORD dwInetCtx;

void
__stdcall
SK_FetchVersionInfo (const wchar_t* wszProduct = L"SpecialK")
{
  wchar_t   wszVersionFile [MAX_PATH] = { L'\0' };

  lstrcatW (wszVersionFile, SK_GetRootPath ());
  lstrcatW (wszVersionFile, wszProduct);
  lstrcatW (wszVersionFile, L"_");
  lstrcatW (wszVersionFile, L"version.ini");

  bool should_fetch = true;

  if (GetFileAttributes (wszVersionFile) != INVALID_FILE_ATTRIBUTES) {
    HANDLE hVersionConfig =
      CreateFile ( wszVersionFile,
                     GENERIC_READ,
                       FILE_SHARE_READ,
                         nullptr,
                           OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL |
                             FILE_FLAG_SEQUENTIAL_SCAN,
                               nullptr );

    FILETIME ftCreation;
    FILETIME ftNow;

    if (GetFileTime ( hVersionConfig, &ftCreation, nullptr, nullptr )) {
      GetSystemTimeAsFileTime (&ftNow);

      LARGE_INTEGER liCreation {
        ftCreation.dwLowDateTime,
        ftCreation.dwHighDateTime
      };

      LARGE_INTEGER liNow {
        ftCreation.dwLowDateTime,
        ftCreation.dwHighDateTime
      };

      // Check Version:  Once every 6 hours
      if ((liNow.QuadPart - liCreation.QuadPart) < 216000000000ULL) {
        should_fetch = false;
      }
    }

    CloseHandle (hVersionConfig);
  }

  if (! should_fetch)
    return;

  HINTERNET hInetRoot =
    InternetOpen (
      L"Special K Auto-Update",
        INTERNET_OPEN_TYPE_DIRECT,
          nullptr, nullptr,
            0x00
    );

  HINTERNET hInetGitHub =
    InternetConnect ( hInetRoot,
                        L"raw.githubusercontent.com",
                          INTERNET_DEFAULT_HTTP_PORT,
                            nullptr, nullptr,
                              INTERNET_SERVICE_HTTP,
                                0x00,
                                  (DWORD_PTR)&dwInetCtx );

  HINTERNET hInetGitHubOpen =
    HttpOpenRequest ( hInetGitHub,
                        nullptr,
                          L"https://raw.githubusercontent.com/Kaldaien/SpecialK/master/version.ini",
                            L"HTTP/1.1",
                              nullptr,
                                nullptr,
                                  0x00,
                                    (DWORD_PTR)&dwInetCtx );

  if ( HttpSendRequestW ( hInetGitHubOpen,
                            nullptr,
                              0,
                                nullptr,
                                  0 ) ) {
    DWORD dwSize;

    HttpEndRequest ( hInetGitHubOpen, nullptr, 0x00, 0 );

    if ( InternetQueryDataAvailable ( hInetGitHubOpen,
                                        &dwSize,
                                          0x00, NULL )
       )
    {
      HANDLE hVersionFile =
        CreateFileW ( wszVersionFile,
                        GENERIC_WRITE,
                          FILE_SHARE_READ,
                            nullptr,
                              CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr );

      while (dwSize > 0) {
        DWORD    dwRead = 0;
        uint8_t *pData  = new uint8_t [dwSize];

        if (! pData)
          break;

        if ( InternetReadFile ( hInetGitHubOpen,
                                  pData,
                                    dwSize,
                                      &dwRead )
          )
        {
          if (hVersionFile != INVALID_HANDLE_VALUE) {
            DWORD dwWritten;

            WriteFile ( hVersionFile,
                          pData,
                            dwRead,
                              &dwWritten,
                                nullptr );
          }
        }

        delete [] pData;

        if (! InternetQueryDataAvailable ( hInetGitHubOpen,
                                             &dwSize,
                                               0x00, NULL
                                         )
           ) break;
      }

      CloseHandle (hVersionFile);
    }
  }

  InternetCloseHandle (hInetGitHubOpen);
  InternetCloseHandle (hInetGitHub);
  InternetCloseHandle (hInetRoot);
}