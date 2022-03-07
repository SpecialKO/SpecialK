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

SK_LazyGlobal <std::vector <denuvo_file_s>> denuvo_files;

extern const wchar_t*
SK_GetSteamDir (void);

// NOT THREAD SAFE
bool
SK_Denuvo_UsedByGame (bool retest)
{
  static bool result = false;
  static bool tested = false;

  if (retest)
  {
    tested = false;
    denuvo_files->clear ();
  }


  if (tested)
    return result;


  CSteamID     usr_id =
    SK::SteamAPI::UserSteamID ();

  AppId64_t    app_id =
    SK::SteamAPI::AppID ();

  std::wstring path   =
      SK_GetSteamDir ();


  WIN32_FIND_DATA fd     = {   };
  int             files  =   0;
  LARGE_INTEGER   liSize = {   };

  wchar_t wszInstallPath [MAX_PATH + 2] = { };
  wchar_t wszSearchPath  [MAX_PATH + 2] = { };

  swprintf_s ( wszInstallPath, MAX_PATH,
                 LR"(%ws\userdata\%u\%llu\)",
                               path.c_str (),
                                 usr_id.GetAccountID (),
                                   app_id );

  swprintf_s ( wszSearchPath,  MAX_PATH,
                 L"%ws*", wszInstallPath );

  HANDLE hFind =
    FindFirstFileW (wszSearchPath, &fd);

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
      {
        size_t len =
          wcslen (fd.cFileName);

        if ( len >= 8  &&
             len <= 11 &&
               (! wcschr (fd.cFileName, L'.')) && fd.nFileSizeLow >  1024UL &&
                                                  fd.nFileSizeLow <= 8192UL )
        {
          denuvo_file_s file = { };

          file.app    = app_id;
          file.path   = wszInstallPath;
          file.path  += fd.cFileName;
          file.user   = usr_id;
          file.ft_key = fd.ftLastWriteTime;

          HANDLE hFile =
            CreateFileW ( file.path.c_str (), GENERIC_READ,
                                                FILE_SHARE_READ,
                                                  nullptr,
                                                    OPEN_EXISTING,
                                                      0,
                                                        nullptr );

          if (hFile != INVALID_HANDLE_VALUE)
          {
            SYSTEMTIME stUTC    = { };

            FileTimeToSystemTime            (&file.ft_key, &stUTC);
            SystemTimeToTzSpecificLocalTime (nullptr,      &stUTC, &file.st_local);

            file.hash =
              _wtoll (fd.cFileName);

            denuvo_files->emplace_back (file);

            ++files;

            SK_CloseHandle (hFile);
          }

          LARGE_INTEGER fsize;

          fsize.HighPart = fd.nFileSizeHigh;
          fsize.LowPart  = fd.nFileSizeLow;

          liSize.QuadPart += fsize.QuadPart;
        }
      }
    } while (FindNextFileW (hFind, &fd) != 0);

    FindClose (hFind);
  }

  if (files > 0)
  {
    std::sort ( denuvo_files->begin (),
                denuvo_files->end   (),
      []( denuvo_file_s& a,
          denuvo_file_s& b )
      {
        return ( CompareFileTime ( &a.ft_key,
                                     &b.ft_key ) < 0 );
      }
    );

    result = true;
  }

  tested = true;

  return result;
}