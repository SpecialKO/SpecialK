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

  if (! wszUnlockSound) // Try again, stupid
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
      iSK_INISection& sec =
        platform_ini->get_section (L"Platform.Achievements");

      if (sec.contains_key (L"SoundFile"))
      {
        wcsncpy_s ( wszFileName,   MAX_PATH,
                    sec.get_value (L"SoundFile").c_str (),
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
      std::filesystem::path (SK_GetDocumentsDir ()) /
           LR"(My Mods\SpecialK\Assets\Shared\Sounds\)";

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
        std::move (req) );
  }

  else
  {
    download_queueX.push (
      std::move (req));
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
            DWORD dwSizeAvailable;

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
       std::move (entry)
     );
   }

   return
     std::move (bundle);
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
         std::filesystem::last_write_time (friend_stats, ec) > 8h )
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

            if (config.system.log_level > 0)
            { steam_log->Log ( L"%hs",
                 std::string ( data.data (),
                               data.data () + data.size ()
                             ).c_str () );
            }

            if (! data.empty ())
            {
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
      sk_download_request_s (schema, url.c_str (),
      []( const std::vector <uint8_t>&& data,
          const std::wstring_view       file )
      {
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

                    steam_log->Log (L"Friend: %hs owns this game...",
                            steam_ctx.Friends ()->
                                   GetFriendPersonaName (CSteamID (_friend_id))
                    );
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
      const auto metadata =
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

  if (! stats)
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
////if (! ( config.cegui.enable &&
////        config.cegui.frames_drawn > 0))
////{
////  return 0;
////}


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

  static int take_screenshot = 0;

  while (it != popups.cend ())
  {
    if (SK_timeGetTime () < (*it).time + POPUP_DURATION_MS)
    {
      float percent_of_lifetime =
        ( static_cast <float> ((*it).time + POPUP_DURATION_MS - SK_timeGetTime ()) /
          static_cast <float> (             POPUP_DURATION_MS)                     );

      //if (SK_PopupManager::getInstance ()->isPopup ((*it).window)) {
      CEGUI::Window* win = (*it).window;

      if (win == nullptr)
      {
        win     = createPopupWindow (&*it);
        created = true;
      }

      const float win_ht =
        256.0f;//win->getPixelSize ().d_height;

      const float win_wd =
        512.0f;//win->getPixelSize ().d_width;

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

      float right_x = win_pos.x * d_scale * full_wd +
                      win_pos.x * d_offset          +
                      win_wd;

      // Window is off-screen horizontally AND vertically
      if ( inset != 0.0f && (right_x > full_wd || right_x < win_wd0)  )
      {
        // Since we're not going to draw this, treat it as a new
        //   popup until it first becomes visible.
        (*it).time =
          SK_timeGetTime ();
      //win->hide        ();
      }

      else
      {
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
            take_screenshot = it->achievement->unlocked_ ? 2 : take_screenshot;
            it->final_pos   = true;
          }
        }

        else if (! it->final_pos)
        {
          take_screenshot = it->achievement->unlocked_ ? 2 : take_screenshot;
          it->final_pos   = true;
        }

///////win->show        ();
///////win->setPosition (win_pos);
      }

      y_pos += y_dir * 256.0f/*win->getSize().d_height*/;

      ++it;
      //} else {
      //it = popups.erase (it);
      //}
    }

    else
    {
      //if (SK_PopupManager::getInstance ()->isPopup ((*it).window)) {
////////////////////////////////////////////CEGUI::Window* win = (*it).window;
////////////////////////////////////////////
////////////////////////////////////////////CEGUI::WindowManager::getDllSingleton ().destroyWindow (win);

      removed = true;
      //SK_PopupManager::getInstance ()->destroyPopup ((*it).window);
      //}

      it = popups.erase (it);
    }
  }

  // Invalidate text overlays any time a window is removed,
  //   this prevents flicker.
  if (removed || created || take_screenshot > 0)
  {
    SK_TextOverlayManager::getInstance ()->drawAllOverlays     (0.0f, 0.0f, true);
    //CEGUI::System::getDllSingleton   ().renderAllGUIContexts ();
  }

  // Popup is in the final location, so now is when screenshots
  //   need to be taken.
  if (config.platform.achievements.take_screenshot && take_screenshot > 0)
  {
    // Delay the screenshot so it doesn't show up twice
    --take_screenshot;

    if (! take_screenshot)
    {
      SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage::EndOfFrame);
      take_screenshot = -1;
    }
  }

  ++drawn;

  if (platform_popup_cs != nullptr)
      platform_popup_cs->unlock ();

  return drawn;
}


CEGUI::Window*
SK_AchievementManager::createPopupWindow (SK_AchievementPopup* popup)
{
////if (! (config.cegui.enable && config.cegui.frames_drawn > 0)) return nullptr;

  if (popup->achievement == nullptr)
    return nullptr;

  auto log =
    ( SK::EOS::UserID () != nullptr ) ?
                   epic_log.getPtr () : steam_log.getPtr ();

  char     szPopupName [32] = { };
  sprintf (szPopupName, "Achievement_%i", lifetime_popups++);

  Achievement*   achievement = popup->achievement;

#ifdef _HAS_CEGUI_REPLACEMENT
  CEGUI::System* pSys =
    CEGUI::System::getDllSingletonPtr ();

  extern CEGUI::Window* SK_achv_popup;

  popup->window              = SK_achv_popup->clone (true);
  CEGUI::Window* achv_popup  = popup->window;

  assert (achievement != nullptr);

  achv_popup->setName (szPopupName);

  CEGUI::Window* achv_title  = achv_popup->getChild ("Title");
  achv_title->setText (achievement->unlocked_ ?
                         (const CEGUI::utf8 *)achievement->text_.unlocked.human_name.c_str ()
                                              :
                         (const CEGUI::utf8 *)achievement->text_.locked.human_name.c_str   ());

  CEGUI::Window* achv_desc = achv_popup->getChild ("Description");
  achv_desc->setText (achievement->unlocked_ ?
                         (const CEGUI::utf8 *)achievement->text_.unlocked.desc.c_str ()
                                             :
                         (const CEGUI::utf8 *)achievement->text_.locked.desc.c_str   ());

  CEGUI::Window* achv_rank = achv_popup->getChild ("Rank");
  achv_rank->setProperty ("NormalTextColour",
                           SK_Achievement_RarityToColor (achievement->global_percent_).c_str ());
  achv_rank->setText     ( SK_Achievement_RarityToName  (achievement->global_percent_) );

  CEGUI::Window* achv_global = achv_popup->getChild ("GlobalRarity");

  char       szGlobal [32] = { };
  snprintf ( szGlobal, 32,
               "Global: %6.2f%%",
                 achievement->global_percent_ );
  achv_global->setText (szGlobal);

  CEGUI::Window* achv_friend =
    achv_popup->getChild ("FriendRarity");

  char       szFriend [32] = { };
  snprintf ( szFriend, 32,
               "Friends: %6.2f%%",
                 100.0 * ( (double)          achievement->friends_.unlocked /
                           (double)std::max (achievement->friends_.possible, 1) )
           );
  achv_friend->setText (szFriend);


  // If the server hasn't updated the unlock time, just use the current time
  if (achievement->time_ == 0)
    _time64 (&achievement->time_);

  CEGUI::Window* achv_unlock  = achv_popup->getChild ("UnlockTime");
  CEGUI::Window* achv_percent = achv_popup->getChild ("ProgressBar");

  float progress =
    achievement->progress_.getPercent ();

  char szUnlockTime [128] = { };
  if (progress == 100.0f)
  {
    snprintf ( szUnlockTime, 128,
                 "Unlocked: %s", _ctime64 (&achievement->time_) );

    achv_percent->setProperty ( "CurrentProgress", "1.0" );
  }

  else
  {
    snprintf ( szUnlockTime, 16,
                 "%5.4f",
                   progress / 100.0
    );

    achv_percent->setProperty ( "CurrentProgress", szUnlockTime );

    snprintf ( szUnlockTime, 128,
                 "Current Progress: %li/%li (%6.2f%%)",
                   achievement->progress_.current,
                   achievement->progress_.max,
                                progress
    );
  }
  achv_unlock->setText (szUnlockTime);
#endif

  auto pUserStats = steam_ctx.UserStats ();
  auto pUtils     = steam_ctx.Utils     ();

  int icon_idx =
    ( pUserStats != nullptr ?
      pUserStats->GetAchievementIcon (achievement->name_.c_str ()) :
                            0 );

  // Icon width and height
  uint32 w = 0,
    h = 0;

  if (icon_idx != 0)
  {
    if (pUtils != nullptr)
        pUtils->GetImageSize (icon_idx, &w, &h);

    int tries = 1;

    while (achievement->icons_.achieved == nullptr && tries < 8)
    {
      achievement->icons_.achieved =
        (uint8_t *)_aligned_malloc (static_cast <size_t> (4) * w * h, 16);

      if (pUtils != nullptr)
      {
        if ( ! pUtils->GetImageRGBA (
                 icon_idx,
                   achievement->icons_.achieved,
                     ( 4 * w * h )
               )
           )
        {
          _aligned_free (achievement->icons_.achieved);
                         achievement->icons_.achieved = nullptr;

          ++tries;
        }

        else
        {
          log->Log ( L" * Fetched RGBA Icon (idx=%li) for Achievement: '%hs'  (%lux%lu) "
                     L"{ Took %li try(s) }",
                       icon_idx, achievement->name_.c_str (), w, h, tries
          );
        }
      }
    }
  }

  if (achievement->icons_.achieved != nullptr)
  {
#ifdef _HAS_CEGUI_REPLACEMENT
    bool exists =
      CEGUI::ImageManager::getDllSingleton ().isDefined (achievement->name_.c_str ());

    CEGUI::Image& img =
      ( exists ?
          CEGUI::ImageManager::getDllSingleton ().get    (              achievement->name_.c_str ()) :
          CEGUI::ImageManager::getDllSingleton ().create ("BasicImage", achievement->name_.c_str ()) );

    if (! exists) try
    {
      /* StaticImage */
      CEGUI::Texture& Tex =
        pSys->getRenderer ()->createTexture (achievement->name_.c_str ());

      Tex.loadFromMemory ( achievement->icons_.achieved,
                             CEGUI::Sizef ( static_cast <float> (w),
                                            static_cast <float> (h) ),
                             CEGUI::Texture::PF_RGBA );

      ((CEGUI::BasicImage &)img).setTexture (&Tex);

      const CEGUI::Rectf rect (CEGUI::Vector2f (0.0f, 0.0f), Tex.getOriginalDataSize ());

      ((CEGUI::BasicImage &)img).setArea       (rect);
      ((CEGUI::BasicImage &)img).setAutoScaled (CEGUI::ASM_Both);
    }

    catch (const CEGUI::GenericException&)
    {
    }

    try
    {
      CEGUI::Window* staticImage =
        achv_popup->getChild ("Icon");

      staticImage->setProperty ( "Image",
                                   achievement->name_.c_str () );
    }

    catch (const CEGUI::GenericException&)
    {
    }
#endif
  }

#ifdef _HAS_CEGUI_REPLACEMENT
  if (config.platform.achievements.popup.show_title)
  {
    std::string app_name = SK::SteamAPI::AppName ();

    if (! app_name.empty ())
    {
      achv_popup->setText (
        (const CEGUI::utf8 *)app_name.c_str ()
      );
    }
  }

  CEGUI::System::getDllSingleton ().
    getDefaultGUIContext ().
           getRootWindow ()->
                addChild (popup->window);

  return achv_popup;
#else
  return nullptr;
#endif
}