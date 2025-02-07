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
#ifndef __SK__Update__Network_H__
#define __SK__Update__Network_H__

#include <winnt.h>
#include <WinInet.h>

extern HRESULT
  __stdcall
    SK_UpdateSoftware1 (const wchar_t* wszProduct, bool force = false);

extern HRESULT
  __stdcall
    SK_UpdateSoftware (const wchar_t* wszProduct);


struct sk_internet_get_t {
  enum {
    STATUS_INVALID   = 0,
    STATUS_UPDATED   = 1,
    STATUS_REMINDER  = 2,
    STATUS_CANCELLED = 4,
    STATUS_FAILED    = 8
  };

  wchar_t wszHostName  [INTERNET_MAX_HOST_NAME_LENGTH] = { };
  wchar_t wszHostPath  [INTERNET_MAX_PATH_LENGTH]      = { };
  wchar_t wszLocalPath [MAX_PATH + 2]                  = { };
  HWND    hTaskDlg                                     = HWND_DESKTOP;
  int     status                                       = STATUS_INVALID;
};

struct sk_download_request_s
{
  std::wstring  path;
  INTERNET_PORT port =       INTERNET_DEFAULT_HTTP_PORT;
  wchar_t       wszHostName [INTERNET_MAX_HOST_NAME_LENGTH] = { };
  wchar_t       wszHostPath [INTERNET_MAX_PATH_LENGTH]      = { };

  uint64_t      user = 0;

  bool (*finish_routine)( const std::vector <uint8_t>&&,
                          const std::wstring_view ) = nullptr;

  sk_download_request_s (void) = default;
  sk_download_request_s (const std::wstring&    local_path,
                         const std::string_view url,
                         bool (*finisher)(const std::vector <uint8_t>&&,
                                          const std::wstring_view) = nullptr)
  {
    finish_routine = finisher;
    path           = local_path;

    std::wstring wide_url =
      SK_UTF8ToWideChar (url.data ());

    URL_COMPONENTSW
      urlcomps                  = {                      };
      urlcomps.dwStructSize     = sizeof (URL_COMPONENTSW);

      urlcomps.lpszHostName     = wszHostName;
      urlcomps.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

      urlcomps.lpszUrlPath      = wszHostPath;
      urlcomps.dwUrlPathLength  = INTERNET_MAX_PATH_LENGTH;

    InternetCrackUrlW (         wide_url.c_str  (),
       sk::narrow_cast <DWORD> (wide_url.length ()),
                         0x00,
                           &urlcomps );

    if (StrStrIW (wide_url.c_str (), L"https") == wide_url.c_str ())
      port = INTERNET_DEFAULT_HTTPS_PORT;
  }

  sk_download_request_s (const std::wstring&     local_path,
                         const std::wstring_view url,
                         bool (*finisher)(const std::vector <uint8_t>&&,
                                          const std::wstring_view) = nullptr)
  {
    finish_routine = finisher;
    path           = local_path;

    URL_COMPONENTSW
      urlcomps                  = {                      };
      urlcomps.dwStructSize     = sizeof (URL_COMPONENTSW);

      urlcomps.lpszHostName     = wszHostName;
      urlcomps.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

      urlcomps.lpszUrlPath      = wszHostPath;
      urlcomps.dwUrlPathLength  = INTERNET_MAX_PATH_LENGTH;

    InternetCrackUrlW (         url.data   (),
       sk::narrow_cast <DWORD> (url.length ()),
                         0x00,
                           &urlcomps );

    if (StrStrIW (url.data (), L"https") == url.data ())
      port = INTERNET_DEFAULT_HTTPS_PORT;
  }
};

void SK_Network_EnqueueDownload (sk_download_request_s&& req, bool high_prio = false);


#endif /* __SK_Update__Network_H__ */