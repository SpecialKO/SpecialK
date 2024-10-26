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
#include <SpecialK/storefront/achievements.h>
#include <SpecialK/storefront/epic.h>
#include <SpecialK/resource.h>
#include <imgui/backends/imgui_d3d12.h> // For D3D12 Texture Mgmt
#include <imgui/backends/imgui_d3d11.h> // For D3D11 Texture Mgmt

#include <imgui/font_awesome.h>

//#define _HAS_CEGUI_REPLACEMENT
#ifdef __CPP20
#define __cpp_lib_format
#include <format>
#include <source_location>
#endif

#include <ctime>
#include <chrono>
#include <filesystem>
#include <concurrent_queue.h>
#include <concurrent_unordered_set.h>

using namespace std::chrono_literals;

void SK_AchievementManager::loadSound (const wchar_t *wszUnlockSound)
{
  auto log =
    ( SK::EOS::UserID () != nullptr ) ?
                   epic_log.getPtr () : steam_log.getPtr ();

  if (log == nullptr || (! wszUnlockSound)) // Try again, stupid
    return;

  bool xbox = false,
       psn  = false,
       dt   = false;

  wchar_t wszFileName [MAX_PATH + 2] = { };

  extern iSK_INI* platform_ini;

  if (*wszUnlockSound == L'\0' && platform_ini != nullptr)
  {
    // If the config file is empty, establish defaults and then write it.
    if (platform_ini->get_sections ().empty ())
    {
      platform_ini->import ( L"[Platform.Achievements]\n"
                             L"SoundFile=psn\n"
                             L"PlaySound=true\n"
                             L"TakeScreenshot=false\n"
                             L"AnimatePopup=true\n"
                             L"NotifyCorner=0\n" );

      platform_ini->write ();
    }

    if (platform_ini->contains_section (L"Platform.Achievements"))
    {
      const iSK_INISection& sec =
        platform_ini->get_section (L"Platform.Achievements");

      if (sec.contains_key (L"SoundFile"))
      {
        wcsncpy_s ( wszFileName,    MAX_PATH,
                    sec.get_cvalue (L"SoundFile").c_str (),
                                    _TRUNCATE );
      }
    }
  }

  else
  {
    wcsncpy_s ( wszFileName,     MAX_PATH,
                wszUnlockSound, _TRUNCATE );
  }

  if ((!      _wcsnicmp (wszFileName, L"psn",           MAX_PATH)))
    psn  = true;                                        
  else if  (! _wcsnicmp (wszFileName, L"xbox",          MAX_PATH))
    xbox = true;
  else if ((! _wcsnicmp (wszFileName, L"dream_theater", MAX_PATH)))
    dt   = true;

  FILE *fWAV  = nullptr;
  size_t size = 0;

  if ( (!  psn) &&
       (! xbox) &&
       (!   dt) && (fWAV = _wfopen (wszFileName, L"rb")) != nullptr )
  {
    SK_ConcealUserDir (wszFileName);

    log->LogEx ( true,
                  L"  >> Loading Achievement Unlock Sound: '%s'...",
                    wszFileName );

    try
    {
      size =
        sk::narrow_cast <size_t> (
          std::filesystem::file_size (wszFileName) );

      if (size > unlock_sound.size ())
                 unlock_sound.resize (size);

      if (     unlock_sound.size () >= size)
        fread (unlock_sound.data (),   size, 1, fWAV);

      default_loaded = false;
    }

    catch (const std::exception& e)
    {
      size        = 0;
      std::ignore = e;
      // ...
    }

    fclose (fWAV);

    log->LogEx (false, L" %d bytes\n", size);
  }

  else
  {
    // Default to PSN if not specified
    if ((! psn) && (! xbox) && (! dt))
      psn = true;

    log->Log ( L"  * Loading Built-In Achievement Unlock Sound: '%s'",
                 wszFileName );

    auto sound_file =
      std::filesystem::path (SK_GetInstallPath ()) /
                        LR"(Assets\Shared\Sounds\)";

    if (psn)
      sound_file /= L"psn_trophy.wav";
    else if (xbox)
      sound_file /= L"xbox.wav";
    else
      sound_file /= L"dream_theater.wav";

    if (! std::filesystem::exists (sound_file))
    {
      // We're missing something, queue up a replacement download...
      //
      //   It won't finish in time for the load below, but it'll be
      //     there for the next game launch at least.
      SK_FetchBuiltinSounds ();
    }

    if ((fWAV = _wfopen (sound_file.c_str (), L"rb")) != nullptr)
    {
      try
      {
        size =
          sk::narrow_cast <size_t> (
            std::filesystem::file_size (sound_file) );

        if (size > unlock_sound.size ())
                   unlock_sound.resize (size);

        if (     unlock_sound.size () >= size)
          fread (unlock_sound.data (),   size, 1, fWAV);

        default_loaded = true;
      }

      catch (const std::exception& e)
      {
        size        = 0;
        std::ignore = e;
        // ...
      }

      fclose (fWAV);
    }
  }
};

//static constexpr auto SK_BACKEND_URL = "https://backend.special-k.info/";
#define                 SK_BACKEND_URL   "https://backend.special-k.info/"

void
SK_Network_EnqueueDownload (sk_download_request_s&& req, bool high_prio)
{
  static SK_AutoHandle hFetchEvent (
    SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr )
  );

  static
    concurrency::concurrent_queue <sk_download_request_s>
                   download_queue;

  static
    concurrency::concurrent_queue <sk_download_request_s>
                   download_queueX;

  if (! high_prio)
  {
    download_queue.push (
                    req );
  }

  else
  {
    download_queueX.push (
                     req );
  }

  SetEvent (hFetchEvent.m_h);

  SK_RunOnce
  ( SK_Thread_CreateEx ([](LPVOID pUser)
 -> DWORD
    {
      HANDLE hWaitEvents [] = {
        __SK_DLL_TeardownEvent, *(HANDLE *)pUser
      };

      while ( WAIT_OBJECT_0 !=
                WaitForMultipleObjects ( 2, hWaitEvents, FALSE, INFINITE ) )
      {
        sk_download_request_s              download;
        while (!   (download_queue.empty  (        ) &&
                    download_queueX.empty (        )))
        {
          while (! (download_queueX.try_pop (download) ||
                    download_queue.try_pop  (download)))
            SK_SleepEx (1UL, FALSE);

          ULONG ulTimeout = 5000UL;

          PCWSTR rgpszAcceptTypes [] = { L"*/*", nullptr };
          HINTERNET hInetHTTPGetReq  = nullptr,
                    hInetHost        = nullptr,
          hInetRoot                  =
            InternetOpen (
              L"Special K - Asset Crawler",
                INTERNET_OPEN_TYPE_DIRECT,
                  nullptr, nullptr,
                    0x00
            );

          if (config.system.log_level > 0)
          {
            steam_log->Log (L"Trying URL: %ws:%d/%ws ==> %ws", download.wszHostName, download.port,
                                                               download.wszHostPath, download.path.c_str ());
          }

          // (Cleanup On Error)
          auto CLEANUP = [&](bool clean = false) ->
          DWORD
          {
            if (! clean)
            {
              DWORD dwLastError =
                   GetLastError ();

              OutputDebugStringW (
                ( std::wstring (L"WinInet Failure (") +
                      std::to_wstring (dwLastError)   +
                  std::wstring (L"): ")               +
                         _com_error   (dwLastError).ErrorMessage ()
                ).c_str ()
              );
            }

            if (hInetHTTPGetReq != nullptr) InternetCloseHandle (hInetHTTPGetReq);
            if (hInetHost       != nullptr) InternetCloseHandle (hInetHost);
            if (hInetRoot       != nullptr) InternetCloseHandle (hInetRoot);

            if ((! clean) && download.finish_routine != nullptr)
            {
              std::vector <uint8_t>               empty {};
              download.finish_routine (std::move (empty), download.path);
            }

            return 0;
          };

          if (hInetRoot == nullptr)
            return CLEANUP ();

          DWORD_PTR dwInetCtx = 0;

          hInetHost =
            InternetConnect ( hInetRoot,
                                download.wszHostName,
                                  download.port,
                                    nullptr, nullptr,
                                      INTERNET_SERVICE_HTTP,
                                        0x00,
                                          (DWORD_PTR)&dwInetCtx );

          if (hInetHost == nullptr)
          {
            return CLEANUP ();
          }

          hInetHTTPGetReq =
            HttpOpenRequest ( hInetHost,
                                nullptr,
                                  download.wszHostPath,
                                    L"HTTP/1.1",
                                      nullptr,
                                        rgpszAcceptTypes,
                                                                            INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                          INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_IGNORE_CERT_CN_INVALID   |
                                          INTERNET_FLAG_RESYNCHRONIZE     | INTERNET_FLAG_CACHE_ASYNC |
                                          ( download.port == INTERNET_DEFAULT_HTTPS_PORT ?
                                                             INTERNET_FLAG_SECURE        : 0x0 ),
                                            (DWORD_PTR)&dwInetCtx );


          // Wait 2500 msecs for a dead connection, then give up
          //
          InternetSetOptionW ( hInetHTTPGetReq, INTERNET_OPTION_RECEIVE_TIMEOUT,
                                 &ulTimeout,    sizeof (ULONG) );


          if (hInetHTTPGetReq == nullptr)
          {
            return CLEANUP ();
          }

          if ( HttpSendRequestW ( hInetHTTPGetReq,
                                    nullptr,
                                      0,
                                        nullptr,
                                          0 ) )
          {
            DWORD dwStatusCode        = 0;
            DWORD dwStatusCode_Len    = sizeof (DWORD);

            DWORD dwContentLength     = 0;
            DWORD dwContentLength_Len = sizeof (DWORD);

            DWORD dwSizeAvailable     = 0;

            HttpQueryInfo ( hInetHTTPGetReq,
                             HTTP_QUERY_STATUS_CODE |
                             HTTP_QUERY_FLAG_NUMBER,
                               &dwStatusCode,
                                 &dwStatusCode_Len,
                                   nullptr );

            if (dwStatusCode == 200 || dwStatusCode == 403 /* json errors */)
            {                       // ^^ If Content Type == json
              HttpQueryInfo ( hInetHTTPGetReq,
                                HTTP_QUERY_CONTENT_LENGTH |
                                HTTP_QUERY_FLAG_NUMBER,
                                  &dwContentLength,
                                    &dwContentLength_Len,
                                      nullptr );

              std::vector <uint8_t> http_chunk;
              std::vector <uint8_t> concat_buffer;
                                    concat_buffer.reserve (dwContentLength);

              while ( InternetQueryDataAvailable ( hInetHTTPGetReq,
                                                     &dwSizeAvailable,
                                                       0x00, NULL )
                )
              {
                if (dwSizeAvailable > 0)
                {
                  DWORD dwSizeRead = 0;

                  if (http_chunk.size () < dwSizeAvailable)
                      http_chunk.resize   (dwSizeAvailable);

                  if ( InternetReadFile ( hInetHTTPGetReq,
                                            http_chunk.data (),
                                              dwSizeAvailable,
                                                &dwSizeRead )
                     )
                  {
                    if (dwSizeRead == 0)
                      break;

                    concat_buffer.insert ( concat_buffer.cend   (),
                                            http_chunk.cbegin   (),
                                              http_chunk.cbegin () + dwSizeRead );

                    if (dwSizeRead < dwSizeAvailable)
                      break;
                  }
                }

                else
                  break;
              }

              // Write the original file to disk if no finish routine exists,
              //   or if it returns false
              if (    download.finish_routine == nullptr ||
                   (! download.finish_routine (std::move (concat_buffer), download.path)) )
              {
                auto directory =
                  std::filesystem::path (download.path).parent_path ();

                std::error_code                                       ec = { };
                if (! std::filesystem::exists             (directory, ec))
                      std::filesystem::create_directories (directory, ec);

                if ( FILE *fOut = _wfopen ( download.path.c_str (), L"wb+" ) ;
                           fOut != nullptr )
                {
                  fwrite ( concat_buffer.data (),
                           concat_buffer.size (), 1, fOut );
                  fclose (                           fOut );
                }
              }
            }

            else
            {
              SK_LOG0 ( ( L"Received unexpected HTTP Response Code: %d for %ws:%d%ws",
                                                      dwStatusCode,
                         download.wszHostName, download.port, download.wszHostPath ),
                           L"   HTTP   " );
            }
          }

          CLEANUP (true);
        }
      }

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] Achievement NetWorker", &hFetchEvent.m_h)
  );
}

#undef snprintf
#include <json/json.hpp>
#include <SpecialK/steam_api.h>

using kvpair_s =
   std::pair <std::string,
              std::string>;

static auto const
SK_HTTP_BundleArgs =
 [] ( std::initializer_list
            <kvpair_s> list_ )
-> std::vector <kvpair_s>
 {
   std::vector <kvpair_s> bundle;

   for ( auto& entry : list_ )
   {
     bundle.push_back (
            entry
     );
   }

   return bundle;
 };

kvpair_s
SK_HTTP_MakeKVPair ( const char*         key,
                     const std::string&& str_val )
{
  return
    kvpair_s { key,
           str_val
    };
};

kvpair_s
SK_HTTP_MakeKVPair ( const char*    key,
                     const uint64_t val )
{
  return
    SK_HTTP_MakeKVPair ( key,
      std::move (
         std::to_string (val) )
    );
};

std::string
SK_Steam_FormatApiRequest ( const char *apiClass,
                            const char *apiFunc,
                                   int  apiVer,
   const std::vector <kvpair_s>&&       vals )
{
  std::string output =
    SK_FormatString (
      SK_BACKEND_URL "%s/%s%d.php?",
        apiClass, apiFunc, apiVer
    );

  int count = 0;

  for ( const auto& it : vals )
  {
    if (count > 0) output += "&";

    output += SK_FormatString ( "%s=%s",
          it.first.c_str (), it.second.c_str () );

    ++count;
  }

  return output;
}

SK_AchievementManager::Achievement::Achievement (int idx, const char* szName, ISteamUserStats* stats)
{
  static const auto
    achievements_path =
      std::filesystem::path (
           SK_GetConfigPath () )
     / LR"(SK_Res/Achievements)";

  static const auto
    schema = ( achievements_path /
             L"SchemaForGame.json" );

  static const auto
    friend_stats = ( achievements_path /
                   L"FriendStatsForGame.json" );

  static const uint64_t ui64_sid =
    SK::SteamAPI::UserSteamID ().ConvertToUint64 ();

  static const std::string url =
    SK_Steam_FormatApiRequest
    ( "ISteamUser", "GetFriendList", 1,
         SK_HTTP_BundleArgs
      ( {SK_HTTP_MakeKVPair ("steamid",
            std::to_string (ui64_sid))} )
    );

  static bool          once = false;
  if (! std::exchange (once, true))
  {
    static concurrency::concurrent_unordered_set <uint64_t> friends;
    static concurrency::concurrent_unordered_set <uint64_t> friends_who_own;
    static concurrency::concurrent_unordered_set <uint64_t> friends_processed;

    std::error_code                                      ec = { };
    if ( std::filesystem::exists          (friend_stats, ec) &&
         std::filesystem::file_time_type::clock::now ( ) -
         std::filesystem::last_write_time (friend_stats, ec) < 8h )
    {
      try {
        nlohmann::json jsonFriends =
          std::move (
            nlohmann::json::parse (
              std::ifstream (friend_stats), nullptr, true
            )
          );

        for ( const auto& friend_ : jsonFriends ["friends"] )
        {
          friends_who_own.insert (
            (uint64_t)std::atoll (
              friend_ ["steamid"].get <std::string_view> ().data ()
            )
          );
        }
      }

      catch (const std::exception& e)
      {
        if (! std::filesystem::remove (friend_stats, ec))
        {
          steam_log->Log (L"Error while managing friend ownership file, %hs", ec.message ().c_str ());
        }

#ifdef __CPP20
        const auto&& src_loc =
          std::source_location::current ();

        steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                     src_loc.file_name     (),
                                     src_loc.line          (),
                                     src_loc.column        (), e.what ());
        steam_log->Log (L"%hs",      src_loc.function_name ());
        steam_log->Log (L"%hs",
          std::format ( std::string ("{:*>") +
                     std::to_string (src_loc.column        ()), 'x').c_str ());
#else
        std::ignore = e;
#endif
      }

      for ( const auto& friend_id : friends_who_own )
      {
        SK_Network_EnqueueDownload (
          sk_download_request_s ( std::to_wstring (friend_id),
            SK_Steam_FormatApiRequest (
              "ISteamUserStats", "GetPlayerAchievements", 1,
               SK_HTTP_BundleArgs (
              {SK_HTTP_MakeKVPair ("steamid", friend_id         ),
               SK_HTTP_MakeKVPair ("appid",   config.steam.appid)})
            ),
          []( const std::vector <uint8_t>&& data,
              const std::wstring_view       file )
          {
            std::ignore = file;

            if (! data.empty ())
            {
              if (config.system.log_level > 0)
              { steam_log->Log ( L"%hs",
                   std::string ( data.data (),
                                 data.data () + data.size ()
                               ).c_str () );
              }

              try {
                nlohmann::json jsonAchieved =
                  std::move (
                    nlohmann::json::parse ( data.cbegin (),
                                            data.cend   (), nullptr, true )
                  );
              }

              catch (const std::exception& e)
              {
                if (config.system.log_level > 0)
                {
#ifdef __CPP20
                  const auto&& src_loc =
                    std::source_location::current ();

                  steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                               src_loc.file_name     (),
                                               src_loc.line          (),
                                               src_loc.column        (), e.what ());
                  steam_log->Log (L"%hs",      src_loc.function_name ());
                  steam_log->Log (L"%hs",
                    std::format ( std::string ("{:*>") +
                               std::to_string (src_loc.column        ()), 'x').c_str ());
#else
                  std::ignore = e;
#endif
                }
              }

              SK_SleepEx (1UL, FALSE);
            }

            else
            {
              steam_log->Log (
                L"Unknown Achievement Status for %ws", file.data ()
              );
              SK_SleepEx (2UL, FALSE);
            }

            return true;
          })
        );
      }
    }


    else
    {
      SK_Network_EnqueueDownload (
      sk_download_request_s (friend_stats, url.c_str (),
      []( const std::vector <uint8_t>&& data,
          const std::wstring_view       file )
      {
        if (data.empty ())
          return true;

        std::ignore = file;

        try {
          nlohmann::json jsonFriends =
            std::move (
              nlohmann::json::parse ( data.cbegin (),
                                      data.cend   (), nullptr, true )
            );

          for ( const auto& friend_ : jsonFriends ["friendslist"]["friends"] )
          {
                   const uint64_t
                 friend_id =  sk::
            narrow_cast <uint64_t> (
              std::atoll (
                &friend_ ["steamid"].get
                < std::string_view >( )[0]
                         )         );

            friends.insert (friend_id);

            SK_Network_EnqueueDownload (
              sk_download_request_s ( std::to_wstring (friend_id),
                SK_Steam_FormatApiRequest (
                  "IPlayerService", "GetOwnedGames", 1,
                   SK_HTTP_BundleArgs (
                  {SK_HTTP_MakeKVPair ("steamid", friend_id         ),
                   SK_HTTP_MakeKVPair ("appid",   config.steam.appid)}) ),
              []( const std::vector <uint8_t>&& data,
                  const std::wstring_view       file )
              {
                if (data.empty ())
                  return true;

                auto steam_friends =
                 steam_ctx.Friends ();

                try
                {
                  nlohmann::json jsonOwned =
                    std::move (
                      nlohmann::json::parse ( data.cbegin (),
                                              data.cend   (), nullptr, true )
                    );

                  uint64_t _friend_id =
                      (uint64_t)std::wcstoll (file.data (), nullptr, 10);

                  if (jsonOwned.count ("response")                      &&
                      jsonOwned       ["response"].count ("game_count") &&
                      jsonOwned       ["response"]       ["game_count"].get <int> () != 0)
                  {
                    friends_who_own.insert (_friend_id);

                    if (steam_friends != nullptr)
                    {
                      steam_log->Log (L"Friend: %hs owns this game...",
                        steam_friends == nullptr ? "Unknown (SteamAPI Malfunction)" :
                        steam_friends->GetFriendPersonaName (CSteamID (_friend_id))
                      );
                    }
                  }

                  friends_processed.insert (_friend_id);

                  SK_SleepEx (3UL, FALSE);
                }

                catch (const std::exception& e)
                {
#ifdef __CPP20
                  const auto&& src_loc =
                    std::source_location::current ();

                  steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                               src_loc.file_name     (),
                                               src_loc.line          (),
                                               src_loc.column        (), e.what ());
                  steam_log->Log (L"%hs",      src_loc.function_name ());
                  steam_log->Log (L"%hs",
                    std::format ( std::string ("{:*>") +
                               std::to_string (src_loc.column        ()), 'x').c_str ());
#else
                  std::ignore = e;
#endif
                }

                if (friends_processed.size () == friends.size ())
                {
                  static std::atomic <size_t> idx = 0;
                  static nlohmann::json root_owner;

                  for ( const auto& friend_id : friends_who_own )
                  {
                    const std::wstring wszFriendId =
                        std::to_wstring (friend_id);

                    SK_Network_EnqueueDownload (
                      sk_download_request_s (wszFriendId,
                        SK_Steam_FormatApiRequest
                        ( "ISteamUserStats", "GetPlayerAchievements", 1,
                           SK_HTTP_BundleArgs (
                          {SK_HTTP_MakeKVPair ("steamid", friend_id         ),
                           SK_HTTP_MakeKVPair ("appid",   config.steam.appid)})
                        ),
                      []( const std::vector <uint8_t>&& data,
                          const std::wstring_view       file )
                      {
                        //steam_log->Log (L"%hs", std::string (data.data (), data.data () + data.size ()).c_str ());

                        try
                        {
                          const size_t idx_ =
                            idx.fetch_add (1);

                          root_owner ["friends"][idx_]["steamid"] =
                            SK_WideCharToUTF8 (file.data ()).c_str ();

                          if (! data.empty ())
                          {
                            nlohmann::json jsonAchieved =
                              std::move (
                                nlohmann::json::parse ( data.cbegin (),
                                                        data.cend   (), nullptr, true )
                              );

                            if ( jsonAchieved.contains ("playerstats") &&
                                 jsonAchieved          ["playerstats"].contains ("achievements") )
                            {
                              const auto& achievements_ =
                                jsonAchieved ["playerstats"]["achievements"];

                              for ( const auto& achievement : achievements_ )
                              {
                                auto& cached_achievement =
                                  root_owner ["friends"][idx_]["achievements"][achievement ["apiname"].get <std::string_view> ().data ()];

                                if ( ( achievement.contains ("unlocktime") && achievement ["unlocktime"].get <int> () != 0 ) ||
                                     ( achievement.contains ("achieved" )  && achievement ["achieved"  ].get <int> () != 0 )  )
                                {
                                  cached_achievement ["achieved"]   = true;
                                  cached_achievement ["unlocktime"] = achievement ["unlocktime"].get <int> ();
                                }

                                else
                                {
                                  cached_achievement ["achieved"]   = false;
                                  cached_achievement ["unlocktime"] = 0;
                                }
                              }
                            }

                            // (i.e. {"playerstats":{"error":"Profile is not public","success":false}})
                            else
                            {
                              if (jsonAchieved.contains ("playerstats") && jsonAchieved ["playerstats"].contains ("error"))
                                root_owner ["friends"][idx_]["error"] = jsonAchieved ["playerstats"]["error"].get <std::string_view> ().data ();
                              else
                                root_owner ["friends"][idx_]["state"] = std::string ((char *)data.data ());
                            }
                          }

                          else
                          {
                            root_owner ["friends"][idx_]["error"] = "Null Steam WebAPI Response";
                            steam_log->Log (L"Unknown Achievement Status for %ws", file.data ());
                          }
                        }

                        catch (const std::exception& e)
                        {
#ifdef __CPP20
                          const auto&& src_loc =
                            std::source_location::current ();

                          steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                                       src_loc.file_name     (),
                                                       src_loc.line          (),
                                                       src_loc.column        (), e.what ());
                          steam_log->Log (L"%hs",      src_loc.function_name ());
                          steam_log->Log (L"%hs",
                            std::format ( std::string ("{:*>") +
                                       std::to_string (src_loc.column        ()), 'x').c_str ());
#else
                          std::ignore = e;
#endif
                        }

                        if (idx == friends_who_own.size ())
                        {
                          std::ofstream (friend_stats) << root_owner;
                        }

                        SK_SleepEx (2UL, FALSE);

                        return true;
                      })
                    );
                  }
                }

                return true;
              })
            );
          }
        }

        catch (const std::exception& e)
        {
#ifdef __CPP20
          const auto&& src_loc =
            std::source_location::current ();

          steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                       src_loc.file_name     (),
                                       src_loc.line          (),
                                       src_loc.column        (), e.what ());
          steam_log->Log (L"%hs",      src_loc.function_name ());
          steam_log->Log (L"%hs",
            std::format ( std::string ("{:*>") +
                       std::to_string (src_loc.column        ()), 'x').c_str ());
#else
          std::ignore = e;
#endif
        }

        return true;
      }));
    }
  }


  idx_  = idx;
  name_ = szName;

  if (stats != nullptr)
  {
    const char* human =
      stats->GetAchievementDisplayAttribute (szName, "name");

    const char* desc =
      stats->GetAchievementDisplayAttribute (szName, "desc");

    text_.locked.human_name =
      (human != nullptr ? SK_UTF8ToWideChar (human) : L"<INVALID>");
    text_.locked.desc =
      ( desc != nullptr ? SK_UTF8ToWideChar (desc)  : L"<INVALID>");

    text_.unlocked.human_name =
      (human != nullptr ? SK_UTF8ToWideChar (human) : L"<INVALID>");
    text_.unlocked.desc =
      ( desc != nullptr ? SK_UTF8ToWideChar (desc)  : L"<INVALID>");
  }

  static SK_LazyGlobal <nlohmann::json> json;
  static std::error_code                ec = { };

  static const auto _DownloadIcons = [](const nlohmann::basic_json <>& achievement)
  {
    const auto szName =
      achievement ["name"].get <std::string_view> ().data ();

    std::filesystem::path unlock_img (achievements_path /
        SK_FormatString ("Unlocked_%s.jpg", szName));
    std::filesystem::path   lock_img (achievements_path /
        SK_FormatString   ("Locked_%s.jpg", szName));

    if (! std::filesystem::exists (unlock_img, ec)) SK_Network_EnqueueDownload (
            sk_download_request_s (unlock_img, achievement ["icon"    ].get <std::string_view> ().data ()), true);
    if (! std::filesystem::exists (lock_img,   ec)) SK_Network_EnqueueDownload (
            sk_download_request_s (lock_img,   achievement ["icongray"].get <std::string_view> ().data ()), true);
  };

  if ( static bool        checked_schema = false ;
      (! std::exchange   (checked_schema,  true))
  && ((! std::filesystem::exists (schema, ec)) || std::filesystem::file_time_type::clock::now ( ) -
                                                  std::filesystem::last_write_time ( schema, ec ) > 8h) )
  {
    std::filesystem::create_directories (
                  achievements_path, ec );

    SK_Network_EnqueueDownload (
      sk_download_request_s (schema,
        SK_Steam_FormatApiRequest
        ( "ISteamUserStats", "GetSchemaForGame", 2,
           SK_HTTP_BundleArgs (
           { SK_HTTP_MakeKVPair ( "appid",
                      config.steam.appid ) }
                              )
        ),
        []( const std::vector <uint8_t>&& data,
            const std::wstring_view       file )
        {
          if (data.empty ())
            return true;

          std::ignore = file;

          try
          {
            json.get () = std::move
            ( nlohmann::json::parse
              ( data.cbegin (),
                  data.cend (),
                       nullptr,
                       true   )  );

            const auto& game_ =
              json.get ()["game"];

            if (game_.
                contains ("availableGameStats"))
            { const auto&  availableGameStats_ =
                game_.at ("availableGameStats");

              if (availableGameStats_.
                  contains (              "achievements"))
              { const auto&                achievements_ =
                  availableGameStats_.at ("achievements");
                for ( auto & achievement : achievements_ )
                { _DownloadIcons (
                             achievement
                  );
                }
              }
            }
          }

          catch (const std::exception& e)
          {
#ifdef __CPP20
            const auto&& src_loc =
              std::source_location::current ();

            steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                         src_loc.file_name     (),
                                         src_loc.line          (),
                                         src_loc.column        (), e.what ());
            steam_log->Log (L"%hs",      src_loc.function_name ());
            steam_log->Log (L"%hs",
              std::format ( std::string ("{:*>") +
                         std::to_string (src_loc.column        ()), 'x').c_str ());
#else
            std::ignore = e;
#endif
          }

          return false;
        }
      ), true
    );
  }

  else if (std::filesystem::exists (schema, ec))
  {
    std::filesystem::path unlock_img (achievements_path / SK_FormatString ("Unlocked_%s.jpg", szName));
    std::filesystem::path   lock_img (achievements_path / SK_FormatString ("Locked_%s.jpg",   szName));

    if (! ( std::filesystem::exists (unlock_img, ec) &&
            std::filesystem::exists (  lock_img, ec) ) )
    {
      try {
        // Lazy-init
        if (! json.isAllocated ())
        {
          json.get () =
            std::move (
              nlohmann::json::parse (std::ifstream (schema), nullptr, true)
            );
        }

        if (json.get ()["game"]["availableGameStats"]["achievements"].size () > sk::narrow_cast <unsigned int> (idx))
        {
          const auto& achievement =
            json.get ()["game"]["availableGameStats"]["achievements"][idx];

          if (config.system.log_level > 1)
            SK_ReleaseAssert (achievement ["name"].get <std::string_view> ()._Equal (szName));

          _DownloadIcons (achievement);
        }
      }

      catch (const std::exception& e)
      {
#ifdef __CPP20
        const auto&& src_loc =
          std::source_location::current ();

        steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                     src_loc.file_name     (),
                                     src_loc.line          (),
                                     src_loc.column        (), e.what ());
        steam_log->Log (L"%hs",      src_loc.function_name ());
        steam_log->Log (L"%hs",
          std::format ( std::string ("{:*>") +
                     std::to_string (src_loc.column        ()), 'x').c_str ());
#else
        std::ignore = e;
#endif
      }
    }
  }
}

#include <EOS/eos_achievements_types.h>

SK_AchievementManager::Achievement::Achievement ( int                            idx,
                                                  EOS_Achievements_DefinitionV2* def )
{
  idx_                      = idx;
  name_                     = def->AchievementId;

  text_.locked.human_name   = SK_UTF8ToWideChar (def->LockedDisplayName);
  text_.locked.desc         = SK_UTF8ToWideChar (def->LockedDescription);

  text_.unlocked.human_name = SK_UTF8ToWideChar (def->UnlockedDisplayName);
  text_.unlocked.desc       = SK_UTF8ToWideChar (def->UnlockedDescription);


  static const auto
    achievements_path =
      std::filesystem::path (
           SK_GetConfigPath () )
     / LR"(SK_Res/Achievements)";


  // Epic allows up to 1024x1024 achievement icons, and also allows PNG
  //
  //   Full detail icons waste a lot of disk space, so we may resize and re-encode
  //
  static const auto _IconTranscoder =
  [ ]( const std::vector <uint8_t> &&buffer,
       const std::wstring_view       file )
  {
    DirectX::ScratchImage img;

    if ( buffer.size () > (1024 * 100) /* 100 KiB or Greater */ &&
          SUCCEEDED ( DirectX::LoadFromWICMemory (
                       buffer.data (),
                       buffer.size (), DirectX::WIC_FLAGS_NONE, nullptr, img ) )
       )
    {
      const auto& metadata =
        img.GetMetadata ();

      // > 256x256 should be converted to JPEG for space savings
      if (metadata.width > 256 || metadata.height > 256)
      {
        // > 512x512 should be resized for even more space savings
        if (metadata.width > 512 || metadata.height > 512)
        {
          DirectX::ScratchImage                                                    resized;
          if ( SUCCEEDED (DirectX::Resize (*img.GetImage (0, 0, 0), 512, 512, 0x0, resized)) )
              img.InitializeFromImage (*resized.GetImage (0, 0, 0));
        }

        return SUCCEEDED (
          DirectX::SaveToWICFile (
            *img.GetImage (0, 0, 0), 0x0,
              DirectX::GetWICCodec (DirectX::WICCodecs::WIC_CODEC_JPEG),
                file.data ()     )
                         );
      }
    }

    return false;
  };

  try {
    SK_RunOnce (
      std::filesystem::create_directories (
                        achievements_path )
    );

    std::filesystem::path unlock_img (achievements_path /
        SK_FormatString ("Unlocked_%s.jpg", def->AchievementId));
    std::filesystem::path lock_img   (achievements_path /
        SK_FormatString ("Locked_%s.jpg",   def->AchievementId));

    if (! std::filesystem::exists (unlock_img)) SK_Network_EnqueueDownload (
            sk_download_request_s (unlock_img, def->UnlockedIconURL, _IconTranscoder) );
    if (! std::filesystem::exists (lock_img))   SK_Network_EnqueueDownload (
            sk_download_request_s (lock_img,   def->LockedIconURL,   _IconTranscoder) );
  }

  catch (const std::exception& e)
  {
    epic_log->Log (L"Achievement Icon Fetch Failure: %hs", e.what ());
  }
}

void
SK_AchievementManager::addAchievement (Achievement* achievement)
{
  auto achv =
    achievements.string_map.find (achievement->name_);

  if (achv == achievements.string_map.cend ())
  {
    achievements.list.push_back (achievement);
    achievements.string_map     [achievement->name_] =
      achievements.list.back ();
  }

  else
  {
    SK_ReleaseAssert (achv == achievements.string_map.cend ());

    // Already have an entry for this
    delete achievement;
  }
}

float
SK_AchievementManager::getPercentOfAchievementsUnlocked (void) const
{
  return percent_unlocked;
}

int
SK_AchievementManager::getNumberOfAchievementsUnlocked (void) const
{
  return total_unlocked;
}

SK_AchievementManager::Achievement*
SK_AchievementManager::getAchievement (const char* szName) const
{
  if (achievements.string_map.count (szName))
    return achievements.string_map.at (szName);

  return nullptr;
}

SK_Achievement**
SK_AchievementManager::getAchievements (size_t* pnAchievements)
{
  ISteamUserStats* stats =
     steam_ctx.UserStats ();

  if ((! stats) || config.platform.steam_is_b0rked)
    return nullptr;

  size_t           count =
    stats->GetNumAchievements ();

  if (pnAchievements != nullptr)
     *pnAchievements = count;

  static std::set    <SK_Achievement *> achievement_set;
  static std::vector <SK_Achievement *> achievement_data;

  if (achievement_set.size () != count)
  {
    for (size_t i = 0; i < count; i++)
    {
      if (! achievement_set.count (achievements.list [i]))
      {
        achievement_set.emplace    (achievements.list [i]);
        achievement_data.push_back (achievements.list [i]);
      }
    }
  }

  return achievement_data.data ();
}

void
SK_AchievementManager::clearPopups (void)
{
  if (platform_popup_cs != nullptr)
      platform_popup_cs->lock ();

  if (popups.empty ())
  {
    if (platform_popup_cs != nullptr)
        platform_popup_cs->unlock ();

    return;
  }

  popups.clear ();

  if (platform_popup_cs != nullptr)
      platform_popup_cs->unlock ();
}

#define FREEBIE     96.0f
#define COMMON      75.0f
#define UNCOMMON    50.0f
#define RARE        25.0f
#define VERY_RARE   15.0f
#define ONE_PERCENT  1.0f


std::string
SK_Achievement_RarityToColor (float percent)
{
#ifdef SK_USE_OLD_ACHIEVEMENT_COLORS
  if (percent <= ONE_PERCENT)
    return "FFFF1111";

  if (percent <= VERY_RARE)
    return "FFFFC711";

  if (percent <= RARE)
    return "FFFFFACD";

  if (percent <= UNCOMMON)
    return "FF0084FF";

  if (percent <= COMMON)
    return "FFFFFFFF";

  if (percent >= FREEBIE)
    return "FFBBBBBB";

  return "FFFFFFFF";
#else
  ImVec4 color =
    ImColor::HSV (0.4f * (percent / 100.0f), 1.0f, 1.0f);

  return
    SK_FormatString ("FF%02X%02X%02X", (int)(color.x * 255.0f),
                                       (int)(color.y * 255.0f),
                                       (int)(color.z * 255.0f) );
#endif
}

const char*
SK_Achievement_RarityToName (float percent)
{
  if (percent <= ONE_PERCENT)
    return "The Other 1%";

  if (percent <= VERY_RARE)
    return "Very Rare";

  if (percent <= RARE)
    return "Rare";

  if (percent <= UNCOMMON)
    return "Uncommon";

  if (percent <= COMMON)
    return "Common";

  if (percent >= FREEBIE)
    return "Freebie";

  return "Common";
}

int
SK_AchievementManager::drawPopups (void)
{
  int drawn = 0;

  if (platform_popup_cs != nullptr)
      platform_popup_cs->lock ();

  if (popups.empty ())
  {
    if (platform_popup_cs != nullptr)
        platform_popup_cs->unlock ();

    return drawn;
  }

  //
  // We don't need this, we always do this from the render thread.
  //

  auto& io =
    ImGui::GetIO ();

  // If true, we need to redraw all text overlays to prevent flickering
  bool removed = false;
  bool created = false;

#define POPUP_DURATION_MS config.platform.achievements.popup.duration

  auto it =
    popups.begin ();

  float inset =
    config.platform.achievements.popup.inset;

  if (inset < 0.0001f)  inset = 0.0f;

  const float full_ht =
    io.DisplaySize.y * (1.0f - inset);

  const float full_wd = io.DisplaySize.x * (1.0f - inset);

  float x_origin, y_origin,
        x_dir,    y_dir;

  ////////////CEGUI::Window* first = it->window;

  const float win_ht0 = 256.0f;// (first != nullptr ?
    //(it->window->getPixelSize ().d_height) : 0.0f );
  const float win_wd0 = 512.0f;// (first != nullptr ?
     //(it->window->getPixelSize ().d_width) : 0.0f );

  const float title_wd =
    io.DisplaySize.x * (1.0f - 2.0f * inset);

  const float title_ht =
    io.DisplaySize.y * (1.0f - 2.0f * inset);

  float fract_x = 0.0f,
        fract_y = 0.0f;

  modf (title_wd / win_wd0, &fract_x);
  modf (title_ht / win_ht0, &fract_y);

  float x_off = full_wd / (4.0f * fract_x);
  float y_off = full_ht / (4.0f * fract_y);

  // 0.0 == Special Case: Flush With Anchor Point
  if (inset == 0.0f)
  {
    x_off = 0.000001f;
    y_off = 0.000001f;
  }

  switch (config.platform.achievements.popup.origin)
  {
    default:
    case 0:
      x_origin =        inset;                          x_dir = 1.0f;
      y_origin =        inset;                          y_dir = 1.0f;
      break;
    case 1:
      x_origin = 1.0f - inset - (win_wd0 / full_wd);    x_dir = -1.0f;
      y_origin =        inset;                          y_dir =  1.0f;
      break;
    case 2:
      x_origin =        inset;                          x_dir =  1.0f;
      y_origin = 1.0f - inset - (win_ht0 / full_ht);    y_dir = -1.0f;
      break;
    case 3:
      x_origin = 1.0f - inset - (win_wd0 / full_wd);    x_dir = -1.0f;
      y_origin = 1.0f - inset - (win_ht0 / full_ht);    y_dir = -1.0f;
      break;
  }

  float x_pos (x_origin + x_off * x_dir);
  float y_pos (y_origin + y_off * y_dir);

  float x_offset = 0.0f;
  float y_offset = 0.0f;

  static int take_screenshot = 0;

  std::unordered_set <std::string> displayed;

  bool any_eligible_for_screenshots = false;

  while (it != popups.cend ())
  {
    if (displayed.contains (it->achievement->name_))
    {
      ++it; continue;
    }

    if (it->time == 0)
        it->time = SK_timeGetTime ();

    bool eligible_for_screenshots =
      (config.platform.achievements.take_screenshot && it->achievement->unlocked_);

    any_eligible_for_screenshots |= eligible_for_screenshots;

    if (SK_timeGetTime () < (it->time + POPUP_DURATION_MS))
    {
#if 0 // Only used for animated popups
      float percent_of_lifetime =
        ( static_cast <float> (it->time + POPUP_DURATION_MS - SK_timeGetTime ()) /
          static_cast <float> (           POPUP_DURATION_MS)                     );
#endif

      ImGuiWindow* win = it->window;

      if (win == nullptr)
      {
        win     = createPopupWindow (&*it);
        created = true;

        ImGui::OpenPopup (it->achievement->name_.c_str ());
      }

      const float win_ht =
        256.0f;//win->getPixelSize ().d_height;

      //const float win_wd =
      //  512.0f;//win->getPixelSize ().d_width;

      ImVec2 win_pos (x_pos, y_pos);

      float d_scale  = 0.05f;
      float d_offset = 0.05f;

      float bottom_y = win_pos.y * d_scale * full_ht +
                       win_pos.y * d_offset          +
                       win_ht;

      // The bottom of the window would be off-screen, so
      //   move it to the top and offset by the width of
      //     each popup.
      if ( bottom_y > full_ht || bottom_y < win_ht0 )
      {
        y_pos  =    (y_origin, y_off * y_dir);
        x_pos += x_dir * 1024;//win->getSize ().d_width;

        win_pos   = ImVec2 (x_pos, y_pos);
      }

      //float right_x = win_pos.x * d_scale * full_wd +
      //                win_pos.x * d_offset          +
      //                win_wd;

      // Window is off-screen horizontally AND vertically
      //if ( inset != 0.0f && (right_x > full_wd || right_x < win_wd0)  )
      //{
      //  // Since we're not going to draw this, treat it as a new
      //  //   popup until it first becomes visible.
      //  it->time =
      //    SK_timeGetTime ();
      ////win->hide        ();
      //}
      //
      //else
      {
#if 0
        if (config.platform.achievements.popup.animate)
        {
          float percent = y_origin * 0.01f;// +y_off)
          //CEGUI::UDim percent (
          //  CEGUI::UDim (y_origin, y_off).percent ()
          //);

          if (percent_of_lifetime <= 0.1f)
          {
            win_pos.y /= (percent * 100.0f *
                          percent_of_lifetime / 0.1f);
          }

          else if (percent_of_lifetime >= 0.9f)
          {
            win_pos.y /= (percent * 100.0f *
                                     (1.0f -
                         (percent_of_lifetime - 0.9f) / 0.1f));
          }

          else if (! it->final_pos)
          {
            take_screenshot = it->achievement->unlocked_ ? 5 : take_screenshot;
            it->final_pos   = true;
          }
        }

        else
#endif
        if (! it->final_pos)
        {
          if (eligible_for_screenshots)
          {
            take_screenshot = it->achievement->unlocked_ ? 5 : take_screenshot;
          }
          it->final_pos   = true;
        }

        const char *szPopupName =
          it->achievement->name_.c_str ();

        if (ImGui::IsPopupOpen (szPopupName))
        {
          DWORD dwNow = SK_GetCurrentMS ();

          const ImVec4 rare_border_color =
            ImColor::HSV ( 0.133333f,
                             std::min ( static_cast <float>(0.161616f +  (dwNow % 250) / 250.0f -
                                                                 floor ( (dwNow % 250) / 250.0f) ), 1.0f),
                                 std::min ( static_cast <float>(0.333 +  (dwNow % 500) / 500.0f -
                                                                 floor ( (dwNow % 500) / 500.0f) ), 1.0f) );

          const float fGlobalPercent =
            it->achievement->global_percent_;

          // Only Steam has rarity information; Epic has XP, but SK is unable to use that info.
          if (fGlobalPercent < 10.0f && SK::SteamAPI::AppID () != 0)
            ImGui::PushStyleColor (ImGuiCol_Border, rare_border_color);

          auto text =
            it->achievement->unlocked_ ? &it->achievement->text_.unlocked
                                       : &it->achievement->text_.locked;
          ImGui::BeginPopup (szPopupName, ImGuiWindowFlags_AlwaysAutoResize      |
                                          ImGuiWindowFlags_NoDecoration          |
                                          ImGuiWindowFlags_NoNav                 |
                                          ImGuiWindowFlags_NoBringToFrontOnFocus |
                                          ImGuiWindowFlags_NoFocusOnAppearing    |
                                          ImGuiWindowFlags_NoInputs              |
                                          ImGuiWindowFlags_NoCollapse);

          //char         window_id [64] = { };
          //_snprintf_s (window_id, 63, "##TOAST%d", (int)i++);

          //ImGui::BeginGroup ();
          ImGui::BringWindowToDisplayFront (ImGui::GetCurrentWindow ());

          float fTopY = ImGui::GetCursorPosY ();

          if (it->icon_texture != nullptr)
          {
            ImGui::Image (it->icon_texture, ImVec2 (128, 128));
          }

          else
          {
            ImGui::PushID      (it->window);
            ImGui::BeginChildFrame
              (ImGui::GetID    ("###SpecialK_AchievementFrame"), ImVec2 (128, 128), 0x0);
            ImGui::EndChildFrame( );
            ImGui::PopID       (  );
          }

          std::string
            text_desc = SK_WideCharToUTF8 (text->desc);

          ImGui::SameLine      (  );
          ImGui::PushID        (it->window);
          ImGui::BeginChild    ("###SpecialK_AchievementText", ImVec2 (313.0f, 128.0f));
          float fLeftX =
            ImGui::GetCursorPosX( );
          ImGui::BeginGroup    (  );
          ImGui::TextColored   (ImColor (1.0f, 1.0f, 1.0f, 1.0f),
                 "%hs", SK_WideCharToUTF8 (text->human_name).c_str ());
          ImGui::PushStyleColor(ImGuiCol_Text, ImColor (0.7f, 0.7f, 0.7f, 1.0f).Value);
          auto size =
            ImGui::CalcTextSize(text_desc.c_str (), nullptr, false, 313.0f);
          ImGui::SetCursorPosY ( fTopY  + (128.0f - size.y) / 2.0f );
          ImGui::SetCursorPosX ( fLeftX + (313.0f - size.x) / 2.0f );
          ImGui::TextWrapped   ("%hs", text_desc.c_str ());
          ImGui::PopStyleColor (  );
          ImGui::EndGroup      (  );
          ImGui::EndChild      (  );
          ImGui::PopID         (  );

          const bool bSteam =
            (SK::SteamAPI::AppID () != 0);

          ImGui::BeginGroup    (  );
          if (bSteam) // Only Steam has unlock percentage stats
          ImGui::TextUnformatted ("Global: ");
          if (it->achievement->friends_.possible > 0)
          ImGui::TextUnformatted ("Friends: ");
          ImGui::EndGroup      (  );
          ImGui::SameLine      (  );
          ImGui::BeginGroup    (  );
          if (bSteam) // Only Steam has unlock percentage stats
          ImGui::Text          ("%6.2f%%", fGlobalPercent);
          if (it->achievement->friends_.possible > 0)
          ImGui::Text          ("%6.2f%%",
               100.0 * ( (double)          it->achievement->friends_.unlocked /
                         (double)std::max (it->achievement->friends_.possible, 1) ));
          ImGui::EndGroup      (  );
          
          float height =
            ImGui::GetFontSize () * ( it->achievement->friends_.possible > 0 ? 2.0f
                                                                             : 1.0f );

          ImGui::SameLine      (  );
          ImGui::SetCursorPosY (ImGui::GetCursorPosY () + height / 2.0f - ImGui::GetFontSize () / 2.0f);
          ImGui::TextColored   (ImColor (1.0f, 1.0f, 1.0f, 1.0f),
                                  it->achievement->unlocked_ ?
                                ICON_FA_UNLOCK : ICON_FA_LOCK);
          ImGui::SameLine      (  );
          if (it->achievement->unlocked_ && it->achievement->time_ > 0)
            ImGui::TextColored (ImColor (0.85f, 0.85f, 0.85f, 1.0f),
                                ":  %s", _ctime64 (&it->achievement->time_));
          else if (it->achievement->progress_.getPercent () > 0.0f)
            ImGui::ProgressBar (it->achievement->progress_.getPercent () / 100.0f);
          else
            ImGui::Spacing     ();

          float x_loc = 0.0f;
          float y_loc = 0.0f;

          switch (config.platform.achievements.popup.origin)
          {
            default:
            case 0: // Top-Left
              x_loc =        inset;                                x_dir = 1.0f;
              y_loc =        inset;                                y_dir = 1.0f;
              break;
            case 1: // Top-Right
              x_loc = io.DisplaySize.x - inset - ImGui::GetWindowSize ().x;    x_dir = -1.0f;
              y_loc =                    inset;                                y_dir =  1.0f;
              break;
            case 2: // Bottom-Left
              x_loc =                    inset;                                x_dir =  1.0f;
              y_loc = io.DisplaySize.y - inset - ImGui::GetWindowSize ().y;    y_dir = -1.0f;
              break;
            case 3: // Bottom-Right
              x_loc = io.DisplaySize.x - inset - ImGui::GetWindowSize ().x;    x_dir = -1.0f;
              y_loc = io.DisplaySize.y - inset - ImGui::GetWindowSize ().y;    y_dir = -1.0f;
              break;
          }

          ImGui::SetWindowPos  (
            ImVec2 (x_loc + x_offset + io.DisplaySize.x * 0.025f * x_dir,
                    y_loc + y_offset + io.DisplaySize.y * 0.025f * y_dir),
                      ImGuiCond_Always
          );
          //ImGui::EndGroup      (  );
          ImGui::EndPopup      (  );

          if (fGlobalPercent < 10.0f && SK::SteamAPI::AppID () != 0)
            ImGui::PopStyleColor( );
        }
      }

      y_offset += y_dir * ImGui::GetWindowSize ().y;

      if (fabs (y_offset) > io.DisplaySize.y)
      {
        y_offset  = 0.0f;
        x_offset += x_dir * ImGui::GetWindowSize ().x;
      }

      y_pos += y_dir * 256.0f/*win->getSize().d_height*/;

#if 0
      displayed.emplace        (it->achievement->name_);
#else
      break;
#endif
                              ++it;
    }

    else
    {
      if (it->achievement->unlocked_)
        displayed.emplace      (it->achievement->name_);
      ImGui::BeginPopup        (it->achievement->name_.c_str ());
      ImGui::CloseCurrentPopup ();
      ImGui::EndPopup          ();

      if (it->icon_texture != nullptr)
      {
        if (SK_GetCurrentRenderBackend ().api != SK_RenderAPI::D3D12)
        {
          if ( _d3d11_rbk.getPtr ()    != nullptr &&
               _d3d11_rbk->_pDeviceCtx != nullptr )
          {
            _d3d11_rbk->_pDeviceCtx->Flush ();
          }

          std::exchange (
              it->icon_texture, nullptr)->Release ();
        }

        else
        {
          if (it->d3d12_tex != nullptr)
          {
            if (_d3d12_rbk->drain_queue ())
            {
              std::exchange (
                it->d3d12_tex, nullptr)->Release ();
            }
          }
        }
      }

      removed = true;
      it      = popups.erase (it);
    }
  }

  // Invalidate text overlays any time a window is removed,
  //   this prevents flicker.
  if (removed || created || (any_eligible_for_screenshots && take_screenshot > 0))
  {
    SK_TextOverlayManager::getInstance ()->drawAllOverlays  (0.0f, 0.0f, true);
  }

  // Popup is in the final location, so now is when screenshots
  //   need to be taken.
  if (any_eligible_for_screenshots && take_screenshot > 0)
  {
    // Delay the screenshot so it doesn't show up twice
    --take_screenshot;

    if (! take_screenshot)
    {
      auto text =
        it->achievement->unlocked_ ? &it->achievement->text_.unlocked
                                   : &it->achievement->text_.locked;

      SK::SteamAPI::TakeScreenshot (
        SK_ScreenshotStage::PrePresent, false,
          SK_FormatString ("Achievements\\%ws", text->human_name.c_str ())
                                   );

      take_screenshot = -1;
    }
  }

  ++drawn;

  if (platform_popup_cs != nullptr)
      platform_popup_cs->unlock ();

  return drawn;
}


ImGuiWindow*
SK_AchievementManager::createPopupWindow (SK_AchievementPopup* popup)
{
  if (popup->achievement == nullptr)
    return nullptr;

  Achievement* achievement =
        popup->achievement;

  static const auto
    achievements_path =
      std::filesystem::path (
           SK_GetConfigPath () )
     / LR"(SK_Res/Achievements)";

  auto icon_filename =
    achievements_path /
      SK_FormatStringW ( L"%ws_%hs.jpg",
            achievement->unlocked_ ? L"Unlocked"
                                   : L"Locked",
            achievement->name_.c_str () );

  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  const bool bIsNativeOrLayeredD3D11 =
    ( static_cast <int> (rb.api) &
      static_cast <int> (SK_RenderAPI::D3D11) ) != 0x0;

  const bool bIsNativeOrLayeredD3D12 =
    ( static_cast <int> (rb.api) &
      static_cast <int> (SK_RenderAPI::D3D12) ) != 0x0;

  if ( bIsNativeOrLayeredD3D11 ||
       bIsNativeOrLayeredD3D12 )
  {
    DirectX::TexMetadata  metadata = {};
    DirectX::ScratchImage image    = {};

    if ( SUCCEEDED (
           DirectX::LoadFromWICFile (
             icon_filename.c_str (),
               0x0, &metadata, image )
         )
       )
    {
      if (bIsNativeOrLayeredD3D11)
      {
        SK_ComPtr <ID3D11Resource> pIconTex;

        auto pDev =
          rb.getDevice <ID3D11Device> ();

        if ( SUCCEEDED (
               DirectX::CreateTexture (
                 pDev, image.GetImages     (),
                       image.GetImageCount (),
                         metadata, &pIconTex.p ) )
           )
        {
          SK_ComPtr <ID3D11ShaderResourceView> pSRV;

          pDev->CreateShaderResourceView (
            pIconTex.p, nullptr, &pSRV.p );

          if (pSRV != nullptr)
          {
            popup->icon_texture =
              pSRV;
              pSRV.p->AddRef ();
          }
        }
      }

      else
      {
        auto texture =
          SK_D3D12_CreateDXTex (metadata, image);

        if (texture.pTexture != nullptr)
        {
          popup->icon_texture =
            (IUnknown *)(texture.hTextureSrvGpuDescHandle.ptr);
          popup->d3d12_tex =
            texture.pTexture;
        }
      }
    }
  }

  else if ( ( static_cast <int> (rb.api) &
              static_cast <int> (SK_RenderAPI::D3D9) ) != 0x0 )
  {
    auto pDev =
      rb.getDevice <IDirect3DDevice9> ();

    using D3DXCreateTextureFromFileW_pfn =
      HRESULT (WINAPI *)(LPDIRECT3DDEVICE9,LPCWSTR,LPDIRECT3DTEXTURE9*);

     static
     D3DXCreateTextureFromFileW_pfn
    _D3DXCreateTextureFromFileW =
    (D3DXCreateTextureFromFileW_pfn)SK_GetProcAddress (L"D3DX9_43.DLL",
    "D3DXCreateTextureFromFileW");

     if (_D3DXCreateTextureFromFileW != nullptr)
         _D3DXCreateTextureFromFileW ( pDev, icon_filename.c_str (),
                                         (IDirect3DTexture9 **)&popup->icon_texture );
  }

  popup->window =
    (ImGuiWindow *)1;

  return popup->window;
}

bool
SK_AchievementManager::OnVarChange (SK_IVariable *var, void *val)
{
  if (var == achievement_test)
  {
    auto iAchievement = *(int *)val;

    static bool
        bSteam = (SK::SteamAPI::AppID () != 0);
    if (bSteam)
      SK_Steam_UnlockAchievement (iAchievement);

    else if (SK::EOS::GetTicksRetired ( ) > 0)
             SK_EOS_UnlockAchievement (iAchievement);

    return true;
  }

  return false;
}

SK_AchievementManager::SK_AchievementManager (void)
{
  SK_RunOnce ({
    static int achievement_id = 0;

    achievement_test =
      SK_CreateVar (SK_IVariable::Int, &achievement_id, this);

    SK_GetCommandProcessor ()->AddVariable ("Platform.AchievementTest", achievement_test);
              });
}