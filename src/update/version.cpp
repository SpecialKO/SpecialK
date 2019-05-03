d/**
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

DWORD dwInetCtx;

volatile DWORD               __SK_LastVersionCheckTime = 0UL;
SK_LazyGlobal <std::wstring> __SK_LastProductTested;

std::wstring
SK_SYS_GetVersionPath (void)
{
  if (! SK_IsInjected ())
  {
    if (! config.system.central_repository)
      return std::wstring (SK_GetRootPath ())   + std::wstring (L"Version\\");
    else
      return std::wstring (SK_GetConfigPath ()) + std::wstring (L"Version\\");
  }

  else
  {
    return SK_GetDocumentsDir () + std::wstring (L"\\My Mods\\SpecialK\\Version\\");
  }
}

std::wstring
SK_SYS_GetInstallPath (void)
{
  if (! SK_IsInjected ())
  {
    return SK_GetRootPath ();
  }

  else
  {
    return SK_GetDocumentsDir () + std::wstring (L"\\My Mods\\SpecialK\\");
  }
}

std::wstring
SK_Version_GetInstallIniPath (void)
{
  return SK_SYS_GetVersionPath () + std::wstring (L"installed.ini");
}

std::wstring
SK_Version_GetRepoIniPath (void)
{
  return SK_SYS_GetVersionPath () + std::wstring (L"repository.ini");
}




bool
__stdcall
SK_FetchVersionInfo1 (const wchar_t* wszProduct, bool force)
{
  //
  // Cache the last tested product
  //
  struct last_result_s {
    bool         ret     = false;
    std::wstring product = L"";
  } static last_result;


  static volatile LONG lock = 0;

  while (InterlockedCompareExchange (&lock, 1, 0) != 0)
  {
    MsgWaitForMultipleObjectsEx (0, nullptr, 33, QS_ALLEVENTS, MWMO_INPUTAVAILABLE);
  }


  // If no log is initialized yet, it's because SKIM is doing
  //   something with this DLL. Init. the log so we can trace
  //     install issues.
  if (! dll_log->initialized)
  {
    dll_log->init (L"logs/installer.log", L"wt+,ccs=UTF-8");
  }

  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested->empty () ?
                   L"SpecialK" : __SK_LastProductTested->c_str ();

  __SK_LastProductTested.get () = wszProduct;

  if (! force)
  {
    if ((! last_result.product.empty ()) &&
           last_result.product.find (wszProduct) != std::wstring::npos)
    {
      InterlockedExchange (&lock, 0);
      return last_result.ret;
    }
  }

  last_result.product = wszProduct;
  last_result.ret     = false;


  if (! wcslen (wszProduct))
  {
    InterlockedExchange (&lock, 0);
    return false;
  }


#define INJECTOR
#ifndef INJECTOR
  if (! SK_IsInjected ())
  {
    if (! lstrcmpW (wszProduct, L"SpecialK"))
    {
      InterlockedExchange (&lock, 0);
      return false;
    }
  }
#endif

  FILETIME                  ftNow;
  GetSystemTimeAsFileTime (&ftNow);

  ULARGE_INTEGER uliNow;

  uliNow.LowPart  = ftNow.dwLowDateTime;
  uliNow.HighPart = ftNow.dwHighDateTime;


  wchar_t wszInstallFile [MAX_PATH] = { };

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


  lstrcatW           (wszInstallFile, SK_Version_GetInstallIniPath ().c_str ());
  PathRemoveFileSpec (wszInstallFile);

  SK_CreateDirectories (wszInstallFile);

  bool need_remind  = false,
       has_remind   = false;

  bool should_fetch       = true;
  bool create_version_ini = false;

  // Update frequency (measured in 100ns)
  ULONGLONG update_freq = 0ULL;

  if (GetFileAttributes (SK_Version_GetInstallIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());

    if ((! install_ini.get_sections ().empty ())         &&
           install_ini.contains_section (L"Update.User") &&
           install_ini.contains_section (L"Version.Local") )
    {
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
      if (! has_freq)
      {
        wcscpy (h, L"6");

        install_ini.import (L"[Update.User]\nFrequency=6h\n\n");
        install_ini.write  (SK_Version_GetInstallIniPath ().c_str ());
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

        auto* remind_time =
          dynamic_cast <sk::ParameterInt64 *> (
            g_ParameterFactory->create_parameter <int64_t> (L"Reminder")
          );

        remind_time->register_to_ini (
          &install_ini,
            L"Update.User",
              L"Reminder"
        );

        int64_t remind = 0LL;

        if (           remind_time->load (remind) &&
             (LONGLONG)uliNow.QuadPart >= remind )
        {
          need_remind = true;
        }

        user_prefs.remove_key (L"Reminder");
        install_ini.write (install_ini.get_filename ());

        delete remind_time;
      }
    }

    else
      create_version_ini = true;
  }

  else
    create_version_ini = true;

  // Create version.ini if it's missing
  if (create_version_ini)
  {
    iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());

    install_ini.import ( L"[Version.Local]\nInstallPackage= \nBranch=Latest\n\n"
                         L"[Update.User]\nFrequency=6h\nReminder=0\n\n"          );
    install_ini.write  (SK_Version_GetInstallIniPath ().c_str ());
  }



  if (GetFileAttributes (SK_Version_GetRepoIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    SK_AutoHandle hVersionConfig (
      CreateFile ( SK_Version_GetRepoIniPath ().c_str (),
                     GENERIC_READ,
                       FILE_SHARE_READ  |
                       FILE_SHARE_WRITE |
                       FILE_SHARE_DELETE,
                         nullptr,
                           OPEN_EXISTING,
                             GetFileAttributes (SK_Version_GetRepoIniPath ().c_str ()) |
                             FILE_FLAG_SEQUENTIAL_SCAN,
                               nullptr )
    );

    if (hVersionConfig != INVALID_HANDLE_VALUE)
    {
      FILETIME ftModify = { };

      if (GetFileTime (hVersionConfig, nullptr, nullptr, &ftModify))
      {
        ULARGE_INTEGER uliModify;

        uliModify.LowPart  = ftModify.dwLowDateTime;
        uliModify.HighPart = ftModify.dwHighDateTime;

        LONGLONG age =
          ( uliNow.QuadPart - uliModify.QuadPart );

        // Check Version:  User Preference (default=6h)
        if ((int64_t)age < (int64_t)update_freq)
        {
          if (! need_remind)
            should_fetch = false;

          need_remind  = false;
          has_remind   = false;
        }

        else
        {
          SK_LOG0 ( (L"repository.ini file is older (%lli seconds) than update frequency (%lli seconds)",
                     age / 10000000LL, update_freq / 10000000), L"AutoUpdate" );
          should_fetch = true;
        }
      }
    }
  }

  //
  // If a reminder is setup, allow it to override the user's
  //   normal check frequency.
  //
  //   So that, for example, a 6h frequency can be delayed up to 1 week.
  //
  if (! force)
  {
    if (! (( should_fetch && (! has_remind)) || need_remind))
    {
      InterlockedExchange (&lock, 0);
      return false;
    }
  }

  HINTERNET hInetRoot =
    InternetOpen (
      L"Special K Auto-Update",
        INTERNET_OPEN_TYPE_DIRECT,
          nullptr, nullptr,
            0x00
    );

  if (! hInetRoot)
  {
    InterlockedExchange (&lock, 0);
    return false;
  }

  HINTERNET hInetGitHub =
    InternetConnect ( hInetRoot,
                        L"raw.githubusercontent.com",
                          INTERNET_DEFAULT_HTTP_PORT,
                            nullptr, nullptr,
                              INTERNET_SERVICE_HTTP,
                                0x00,
                                  reinterpret_cast <DWORD_PTR> (&dwInetCtx) );

  if (! hInetGitHub)
  {
    InternetCloseHandle (hInetRoot);
    InterlockedExchange (&lock, 0);
    return false;
  }

  wchar_t wszRemoteRepoURL [MAX_PATH] = { };

  if (! lstrcmpW (wszProduct, L"SpecialK"))
    swprintf ( wszRemoteRepoURL,
                 L"/Kaldaien/%s/0.10.x/version.ini",
                   wszProduct );
  else if (wcschr (wszProduct, L'/'))
    swprintf ( wszRemoteRepoURL,
                 L"/Kaldaien/%s/version.ini",
                   wszProduct );
  else
    swprintf ( wszRemoteRepoURL,
                 L"/Kaldaien/%s/master/version.ini",
                   wcslen (wszProduct) ? wszProduct : L"SpecialK" );

  ULONG ulTimeout = 5000UL;
  bool  bRet      = FALSE;

  LARGE_INTEGER liStartTime = //-V821
    SK_CurrentPerf ();

  InternetSetOptionW ( hInetGitHub, INTERNET_OPTION_RECEIVE_TIMEOUT, &ulTimeout, sizeof ULONG );

  SK_LOG0 ( (L"Fetching GitHub repository INI file from %s... ", wszRemoteRepoURL ),
             L"AutoUpdate" );

  PCWSTR  rgpszAcceptTypes []         = { L"*/*", nullptr };

  DWORD dwFlags =   INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_IGNORE_CERT_CN_INVALID
                  | INTERNET_FLAG_NEED_FILE                | INTERNET_FLAG_PRAGMA_NOCACHE
                  | INTERNET_FLAG_RESYNCHRONIZE;

  extern bool SK_IsSuperSpecialK (void);

  if (SK_IsSuperSpecialK () || force)
    dwFlags |= INTERNET_FLAG_RELOAD;

  HINTERNET hInetGitHubOpen =
    HttpOpenRequest ( hInetGitHub,
                        nullptr,
                          wszRemoteRepoURL,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                  dwFlags,
                                    reinterpret_cast <DWORD_PTR> (&dwInetCtx) );

  if (! hInetGitHubOpen)
  {
    InternetCloseHandle (hInetGitHub);
    InternetCloseHandle (hInetRoot);
    InterlockedExchange (&lock, 0);
    return false;
  }

  InternetSetOptionW ( hInetGitHubOpen, INTERNET_OPTION_RECEIVE_TIMEOUT, &ulTimeout, sizeof ULONG );

  //////////////////////////////////////////////////////////////////////////
  //
  // Like all other aspects of the WinINet API, this function cannot be
  //   safely called from within DllMain or the constructors and destructors
  //     of global objects.
  //
  //////////////////////////////////////////////////////////////////////////
  if ( HttpSendRequestW ( hInetGitHubOpen,
                            nullptr,
                              0,
                                nullptr,
                                  0 ) )
  {
    DWORD dwSize  = 0;
    DWORD dwTotal = 0;

    if ( InternetQueryDataAvailable ( hInetGitHubOpen,
                                        &dwSize,
                                          0x00, NULL )
      )
    {
      DWORD dwAttribs = GetFileAttributes (SK_Version_GetRepoIniPath ().c_str ());

      if (dwAttribs == INVALID_FILE_ATTRIBUTES)
          dwAttribs = FILE_ATTRIBUTE_NORMAL;

      SK_AutoHandle hVersionFile (
        CreateFileW ( SK_Version_GetRepoIniPath ().c_str (),
                        GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                            nullptr,
                              CREATE_ALWAYS,
                                dwAttribs |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr )
      );

      while (hVersionFile != INVALID_HANDLE_VALUE && dwSize > 0)
      {
        DWORD              dwRead = 0;
        CHeapPtr <uint8_t> pData;
                           pData.Allocate (dwSize);

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

          dwTotal += dwWritten;
        }

        if (! InternetQueryDataAvailable ( hInetGitHubOpen,
                                             &dwSize,
                                               0x00, NULL
                                         )
           ) break;
      }
    }

    bRet = ( dwTotal > 15 );

    if (! bRet)
    {
      // GitHub may return 404: Not Found  (15 bytes)
      DeleteFileW (SK_Version_GetRepoIniPath ().c_str ());
    }

    else
    {
      double seconds =
        SK_DeltaPerfMS (liStartTime.QuadPart, 1) / 1000.0;

      SK_LOG0 ( (L"Fetched %ws from [%ws] in %4.1f seconds (%ws/sec)",
                 SK_File_SizeToString (
                   static_cast <uint64_t> (dwTotal)
                 ).c_str (), wszRemoteRepoURL, seconds,
                 SK_File_SizeToString (
                   static_cast <uint64_t> (
                     static_cast < double>  (dwTotal) /
                                            seconds
                                          )
                 ).c_str ()
                ), L"AutoUpdate"
              );
    }
  }

  if (! bRet)
  {
    SK_LOG0 ( (L"Failed to retrieve version info from %s", wszRemoteRepoURL ),
               L"AutoUpdate" );
  }

  InternetCloseHandle (hInetGitHubOpen);
  InternetCloseHandle (hInetGitHub);
  InternetCloseHandle (hInetRoot) ;

  last_result.product = wszProduct;
  last_result.ret     = bRet;

  InterlockedExchange (&lock, 0);

  if (bRet)
    InterlockedExchange (&__SK_LastVersionCheckTime, timeGetTime ());

  return bRet;
}

bool
__stdcall
SK_FetchVersionInfo (const wchar_t* wszProduct)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested->c_str ();

  return SK_FetchVersionInfo1 (wszProduct);
}


SK_VersionInfo
SK_Version_GetLatestInfo_V1 (const wchar_t* wszProduct)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested->c_str ();

  SK_FetchVersionInfo (wszProduct);

  SK_VersionInfo_V1 ver_info;

  if ( GetFileAttributes (SK_Version_GetInstallIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES &&
       GetFileAttributes (SK_Version_GetRepoIniPath    ().c_str ()) != INVALID_FILE_ATTRIBUTES )
  {
    iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());
    iSK_INI repo_ini    (SK_Version_GetRepoIniPath    ().c_str ());

    ver_info.branch   = install_ini.get_section (L"Version.Local").get_value (L"Branch");

    wchar_t wszBranchSection [128] = { };
    _swprintf (wszBranchSection, L"Version.%s", ver_info.branch.c_str ());

    if (repo_ini.contains_section (wszBranchSection))
    {
      ver_info.package = repo_ini.get_section (wszBranchSection).get_value (L"InstallPackage");

      wchar_t wszPackage [128] = { };
      swscanf (ver_info.package.c_str (), L"%127[^,],%i", wszPackage, &ver_info.build);

      ver_info.package = wszPackage;
    }

    return ver_info;
  }

  ver_info.package = L"Unknown";
  ver_info.branch  = L"Unknown";
  ver_info.build   = -1;

  return ver_info;
}

SK_VersionInfo
SK_Version_GetLocalInfo_V1 (const wchar_t* wszProduct)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested->c_str ();

  SK_FetchVersionInfo (wszProduct);

  SK_VersionInfo_V1 ver_info;

  if (GetFileAttributes (SK_Version_GetInstallIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());

    ver_info.branch   = install_ini.get_section (L"Version.Local").get_value (L"Branch");
    ver_info.package  = install_ini.get_section (L"Version.Local").get_value (L"InstallPackage");

    wchar_t wszPackage [128] = { };
    swscanf (ver_info.package.c_str (), L"%128[^,],%i", wszPackage, &ver_info.build);

    ver_info.package = wszPackage;
  }

  return ver_info;
}

static SK_LazyGlobal <std::wstring> cached_time;

std::wstring&
SK_Version_GetLastCheckTime_WStr (void)
{
  static DWORD last_check = 1;

  if (! cached_time->empty ())
  {
    if ( ReadULongAcquire (&__SK_LastVersionCheckTime) ==
           last_check )
    {
      return
        cached_time.get ();
    }
  }

  WIN32_FIND_DATA FindFileData;
  HANDLE            hFileBackup =
    FindFirstFile (SK_Version_GetRepoIniPath ().c_str (), &FindFileData);

  FILETIME   ftModified = { };
  SYSTEMTIME stModified = { };

  FileTimeToLocalFileTime (&FindFileData.ftLastWriteTime, &ftModified);
  FileTimeToSystemTime    (&ftModified,                   &stModified);

  FindClose (hFileBackup);

  wchar_t wszFileTime [48] = { };
  wchar_t wszDateTime [48] = { };

  GetDateFormat (LOCALE_USER_DEFAULT, DATE_AUTOLAYOUT, &stModified, nullptr, wszFileTime, 47);
  GetTimeFormat (LOCALE_USER_DEFAULT, TIME_NOSECONDS, &stModified, nullptr, wszDateTime, 47);

  *cached_time =
    SK_FormatStringW (L"%ws %ws", wszFileTime, wszDateTime);

  last_check =
    ReadULongAcquire (&__SK_LastVersionCheckTime);

  return
    cached_time.get ();
}


#include <Shlwapi.h>

std::vector <std::string>
SK_Version_GetAvailableBranches (const wchar_t* wszProduct)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested->c_str ();

  std::vector <std::string> branches;

  SK_FetchVersionInfo (wszProduct);

  if (GetFileAttributes (SK_Version_GetRepoIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    iSK_INI repo_ini (SK_Version_GetRepoIniPath ().c_str ());

    iSK_INI::_TSectionMap& sections =
      repo_ini.get_sections ();

    for ( auto& it : sections )
    {
      if (StrStrIW (it.first.c_str (), L"Version.") == it.first.c_str ())
      {
        wchar_t wszBranchName [128] = { };
        swscanf (it.first.c_str (), L"Version.%127s", wszBranchName);

        branches.push_back (SK_WideCharToUTF8 (wszBranchName));
      }
    }
  }

  return branches;
}


uint64_t
SK_Version_GetUpdateFrequency (const wchar_t* wszProduct)
{
  uint64_t update_freq = MAXULONGLONG;

  UNREFERENCED_PARAMETER (wszProduct);
  //if (wszProduct == nullptr)
  //  wszProduct = __SK_LastProductTested.c_str ();

  if (GetFileAttributes (SK_Version_GetInstallIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());

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
    if (! has_freq)
    {
      wcscpy (h, L"6");

      install_ini.import (L"[Update.User]\nFrequency=6h\n\n");
      install_ini.write  (SK_Version_GetInstallIniPath ().c_str ());
    }

    const ULONGLONG _Hour = 36000000000ULL;

    update_freq += (   1ULL * _Hour * _wtoi (h) );
    update_freq += (  24ULL * _Hour * _wtoi (d) );
    update_freq += ( 168ULL * _Hour * _wtoi (w) );

    //dll_log.Log ( L"Update Frequency: %lu hours, %lu days, %lu weeks (%s)",
                    //_wtoi (h), _wtoi (d), _wtoi (w), freq.c_str () );

    if (freq == L"never")
      update_freq = MAXULONGLONG;
  }

  return update_freq;
}


void
SK_Version_SetUpdateFrequency (const wchar_t* wszProduct, uint64_t freq)
{
  UNREFERENCED_PARAMETER (wszProduct);
  //if (wszProduct == nullptr)
    //wszProduct = __SK_LastProductTested.c_str ();

  if (GetFileAttributes (SK_Version_GetInstallIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());

    iSK_INISection& user_prefs =
      install_ini.get_section (L"Update.User");

    wchar_t wszFormatted [128] = { };

    if (freq > 0 && freq < MAXULONGLONG)
      _swprintf (wszFormatted, L"%lluh", freq / 36000000000ULL);
    else
      _swprintf (wszFormatted, L"never");

    if (! user_prefs.contains_key (L"Frequency"))
      user_prefs.add_key_value (L"Frequency", wszFormatted);
    else
      user_prefs.get_value (L"Frequency") = wszFormatted;

    install_ini.write (SK_Version_GetInstallIniPath ().c_str ());
  }
}


void
SK_Version_ForceUpdateNextLaunch (const wchar_t* wszProduct)
{
  UNREFERENCED_PARAMETER (wszProduct);
  //if (wszProduct == nullptr)
  //  wszProduct = __SK_LastProductTested.c_str ();

  if (GetFileAttributes (SK_Version_GetInstallIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());

    if (install_ini.contains_section (L"Update.User"))
    {
      if (install_ini.get_section (L"Update.User").contains_key (L"Reminder"))
          install_ini.get_section (L"Update.User").remove_key   (L"Reminder");
    }

    iSK_INISection& user_prefs =
      install_ini.get_section (L"Update.User");

    user_prefs.add_key_value (L"Reminder", L"0");

    install_ini.write (SK_Version_GetInstallIniPath ().c_str ());
  }
}


bool
SK_Version_SwitchBranches (const wchar_t* wszProduct, const char* szBranch)
{
  UNREFERENCED_PARAMETER (wszProduct);
  //if (wszProduct == nullptr)
  //  wszProduct = __SK_LastProductTested.c_str ();

  if (GetFileAttributes (SK_Version_GetInstallIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    iSK_INI install_ini (SK_Version_GetInstallIniPath ().c_str ());

    // TODO: Validate selected branch against those in the repository

    wchar_t wszBranch [128] = { };
    swprintf (wszBranch, L"%hs", szBranch);

    install_ini.get_section (L"Version.Local").get_value (L"Branch")         = wszBranch;
    install_ini.get_section (L"Version.Local").get_value (L"InstallPackage") = L"PendingBranchMigration,0";

    install_ini.write (SK_Version_GetInstallIniPath ().c_str ());
  }

  return true;
}



SK_BranchInfo_V1
SK_Version_GetLatestBranchInfo_V1 (const wchar_t* wszProduct, const char* szBranch)
{
  if (wszProduct == nullptr)
    wszProduct = __SK_LastProductTested->c_str ();


  static SK_BranchInfo_V1 __INVAID_BRANCH
  {
    0x1, SK_GetVersionStrW (), L"N/A", L"No Description Available",

        SK_VersionInfo_V1 { L"Unknown (or Custom Install)", L"N/A", -1 },

    0x1, L"NotUsed", L"Invalid"
  };


  if (GetFileAttributes (SK_Version_GetRepoIniPath ().c_str ()) != INVALID_FILE_ATTRIBUTES)
  {
    iSK_INI repo_ini (SK_Version_GetRepoIniPath ().c_str ());

    wchar_t wszFormatted [256] = { };
    _swprintf (wszFormatted, L"Version.%hs", szBranch);

    if (repo_ini.contains_section (wszFormatted))
    {
      iSK_INISection& branch_sec =
        repo_ini.get_section (wszFormatted);

      auto ParseInstallPackage = [](const char* szBranch, const wchar_t*) ->
        SK_VersionInfo_V1
          {
            SK_VersionInfo_V1 vinfo1;

            vinfo1.branch  = SK_UTF8ToWideChar (szBranch);

            wchar_t wszPackage_ [128] = { };
            swscanf (vinfo1.package.c_str (), L"%127[^,],%i", wszPackage_, &vinfo1.build);

            vinfo1.package = wszPackage_;

            return vinfo1;
          };

      SK_BranchInfo_V1 branch {
        0x1,
        branch_sec.get_value (L"Title"),
        branch_sec.get_value (L"ReleaseNotes"),
        branch_sec.get_value (L"Description"),

        ParseInstallPackage (szBranch, branch_sec.get_value (L"InstallPackage").c_str ()),

        0x1,
        L"Not Implemented",
        branch_sec.get_value (L"BranchDescription")
      };

      return branch;
    }
  }

  return __INVAID_BRANCH;
}