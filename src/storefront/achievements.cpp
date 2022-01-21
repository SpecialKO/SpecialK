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

  extern iSK_INI* steam_ini;

  if (*wszUnlockSound == L'\0' && steam_ini != nullptr)
  {
    // If the config file is empty, establish defaults and then write it.
    if (steam_ini->get_sections ().empty ())
    {
      steam_ini->import ( L"[Steam.Achievements]\n"
                          L"SoundFile=psn\n"
                          L"PlaySound=true\n"
                          L"TakeScreenshot=false\n"
                          L"AnimatePopup=true\n"
                          L"NotifyCorner=0\n" );

      steam_ini->write ();
    }

    if (steam_ini->contains_section (L"Steam.Achievements"))
    {
      iSK_INISection& sec =
        steam_ini->get_section (L"Steam.Achievements");

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
    wcsncpy_s ( wszFileName,    MAX_PATH,
                wszUnlockSound, _TRUNCATE );
  }

  if ((!      _wcsnicmp (wszFileName, L"psn", MAX_PATH)))
    psn  = true;
  else if  (! _wcsnicmp (wszFileName, L"xbox", MAX_PATH))
    xbox = true;
  else if ((! _wcsnicmp (wszFileName, L"dream_theater", MAX_PATH)))
    dt   = true;

  FILE *fWAV = nullptr;

  if ( (!  psn) &&
       (! xbox) &&
       (!   dt) && (fWAV = _wfopen (wszFileName, L"rb")) != nullptr )
  {
    SK_ConcealUserDir (wszFileName);

    log->LogEx ( true,
                  L"  >> Loading Achievement Unlock Sound: '%s'...",
                    wszFileName );

                fseek (fWAV, 0, SEEK_END);
    long size = ftell (fWAV);
               rewind (fWAV);

    unlock_sound =
      static_cast <uint8_t *> (
        malloc (size)
      );

    if (unlock_sound != nullptr)
      fread  (unlock_sound, size, 1, fWAV);

    fclose (fWAV);

    log->LogEx (false, L" %d bytes\n", size);

    default_loaded = false;
  }

  else
  {
    // Default to PSN if not specified
    if ((! psn) && (! xbox) && (! dt))
      psn = true;

    log->Log ( L"  * Loading Built-In Achievement Unlock Sound: '%s'",
                 wszFileName );

    HRSRC default_sound = nullptr;

    if (psn)
      default_sound =
      FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_TROPHY),        L"WAVE");
    else if (xbox)
      default_sound =
      FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_XBOX),          L"WAVE");
    else
      default_sound =
      FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_DREAM_THEATER), L"WAVE");

    if (default_sound != nullptr)
    {
      HGLOBAL sound_ref     =
        LoadResource (SK_GetDLL (), default_sound);

      if (sound_ref != nullptr)
      {
        unlock_sound        =
          static_cast <uint8_t *> (LockResource (sound_ref));

        default_loaded = true;
      }
    }
  }
};

SK_AchievementManager::Achievement::Achievement (int idx, const char* szName, ISteamUserStats* stats)
{
  idx_  = idx;
  name_ = szName;

  if (stats != nullptr)
  {
    const char* human =
      stats->GetAchievementDisplayAttribute (szName, "name");

    const char* desc =
      stats->GetAchievementDisplayAttribute (szName, "desc");

    text_.locked.human_name =
      (human != nullptr ? human : "<INVALID>");
    text_.locked.desc =
      ( desc != nullptr ? desc  : "<INVALID>");

    text_.unlocked.human_name =
      (human != nullptr ? human : "<INVALID>");
    text_.unlocked.desc =
      ( desc != nullptr ? desc  : "<INVALID>");
  }
}

#include <filesystem>
#include <concurrent_queue.h>
#include <EOS/eos_achievements_types.h>

SK_AchievementManager::Achievement::Achievement ( int                            idx,
                                                  EOS_Achievements_DefinitionV2* def )
{
  idx_                      = idx;
  name_                     = def->AchievementId;

  text_.locked.human_name   = def->LockedDisplayName;
  text_.locked.desc         = def->LockedDescription;

  text_.unlocked.human_name = def->UnlockedDisplayName;
  text_.unlocked.desc       = def->UnlockedDescription;

  struct download_request_s
  {
    std::wstring  path;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTP_PORT;
    wchar_t       wszHostName [INTERNET_MAX_HOST_NAME_LENGTH] = { };
    wchar_t       wszHostPath [INTERNET_MAX_PATH_LENGTH]      = { };

    download_request_s (void) { };
    download_request_s (const std::wstring& local_path,
                        const char*         url)
    {
      path = local_path;

      std::wstring wide_url =
        SK_UTF8ToWideChar (url);

      URL_COMPONENTSW
        urlcomps                  = {                      };
        urlcomps.dwStructSize     = sizeof (URL_COMPONENTSW);

        urlcomps.lpszHostName     = wszHostName;
        urlcomps.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

        urlcomps.lpszUrlPath      = wszHostPath;
        urlcomps.dwUrlPathLength  = INTERNET_MAX_PATH_LENGTH;

      InternetCrackUrl (          wide_url.c_str  (),
         sk::narrow_cast <DWORD> (wide_url.length ()),
                           0x00,
                             &urlcomps );

      if (wide_url.find (L"https") != std::wstring::npos)
        port = INTERNET_DEFAULT_HTTPS_PORT;
    }
  };

  static SK_AutoHandle hFetchEvent (
      SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr )
    );

  static concurrency::concurrent_queue <download_request_s *>
                                        download_queue;

  try {
    std::filesystem::path achievements_path (
      std::filesystem::path (SK_GetConfigPath ()) / LR"(SK_Res/Achievements)"
    );

    std::filesystem::path unlock_img (achievements_path / L"Unlocked_");
                          unlock_img += def->AchievementId;
                          unlock_img += L".jpg";

    std::filesystem::path lock_img (achievements_path / L"Locked_");
                          lock_img += def->AchievementId;
                          lock_img += L".jpg";

    std::filesystem::create_directories (achievements_path);

    if (! std::filesystem::exists (unlock_img)) { download_queue.push (new
               download_request_s (unlock_img, def->UnlockedIconURL)); SetEvent (hFetchEvent.m_h); }
    if (! std::filesystem::exists (lock_img))   { download_queue.push (new
               download_request_s (lock_img,   def->LockedIconURL  )); SetEvent (hFetchEvent.m_h); }
  }

  catch (...) {};

  if (! download_queue.empty ())
  {
    SK_RunOnce (
      SK_Thread_CreateEx ([](LPVOID pUser) -> DWORD
      {
        HANDLE hWaitEvents [] = {
          __SK_DLL_TeardownEvent, *(HANDLE *)pUser
        };

        while ( WaitForMultipleObjects ( 2, hWaitEvents, FALSE, INFINITE ) != WAIT_OBJECT_0 )
        {
          download_request_s*                download_ptr;
          while (!   download_queue.empty   (            ))
          { while (! download_queue.try_pop (download_ptr))
              SK_SleepEx (1, FALSE);

            std::unique_ptr <download_request_s>
                             download       (download_ptr);

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

              return 0;
            };

            if (hInetRoot == nullptr)
              return CLEANUP ();

            DWORD_PTR dwInetCtx = 0;

            hInetHost =
              InternetConnect ( hInetRoot,
                                  download->wszHostName,
                                    download->port,
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
                                    download->wszHostPath,
                                      L"HTTP/1.1",
                                        nullptr,
                                          rgpszAcceptTypes,
                                                                              INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                            INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_IGNORE_CERT_CN_INVALID   |
                                            INTERNET_FLAG_RESYNCHRONIZE     | INTERNET_FLAG_CACHE_ASYNC |
                                            ( download->port == INTERNET_DEFAULT_HTTPS_PORT ?
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

              if (dwStatusCode == 200)
              {
                HttpQueryInfo ( hInetHTTPGetReq,
                                  HTTP_QUERY_CONTENT_LENGTH |
                                  HTTP_QUERY_FLAG_NUMBER,
                                    &dwContentLength,
                                      &dwContentLength_Len,
                                        nullptr );

                std::vector <char> http_chunk;
                std::vector <char> concat_buffer;

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

                // Epic allows up to 1024x1024 achievement icons, and also allows PNG
                //
                //   Full detail icons waste a lot of disk space, so we may resize and re-encode
                //
                bool optimized = false;

                DirectX::ScratchImage img;

                if ( concat_buffer.size () > (1024 * 100) /* 100 KiB or Greater */ &&
                      SUCCEEDED ( DirectX::LoadFromWICMemory (
                                   concat_buffer.data (),
                                   concat_buffer.size (), DirectX::WIC_FLAGS_NONE, nullptr, img ) )
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

                    optimized = SUCCEEDED (
                      DirectX::SaveToWICFile (
                        *img.GetImage (0, 0, 0), 0x0,
                          DirectX::GetWICCodec (DirectX::WICCodecs::WIC_CODEC_JPEG),
                            download->path.c_str ()
                      )
                    );
                  }
                }

                // Write the original file to disk
                if (! optimized)
                {
                  FILE *fOut =
                    _wfopen ( download->path.c_str (), L"wb+" );

                  if (fOut != nullptr)
                  {
                    fwrite (concat_buffer.data (), concat_buffer.size (), 1, fOut);
                    fclose (fOut);
                  }
                }
              }
            }

            CLEANUP (true);
          }
        }

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] EOS Achievement Worker", &hFetchEvent.m_h)
    );
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
  if (steam_popup_cs != nullptr)
      steam_popup_cs->lock ();

  if (popups.empty ())
  {
    if (steam_popup_cs != nullptr)
        steam_popup_cs->unlock ();
    return;
  }

  popups.clear ();

  if (steam_popup_cs != nullptr)
      steam_popup_cs->unlock ();
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
  if (! ( config.cegui.enable &&
          config.cegui.frames_drawn > 0))
  {
    return 0;
  }


  int drawn = 0;

  if (steam_popup_cs != nullptr)
      steam_popup_cs->lock ();

  if (popups.empty ())
  {
    if (steam_popup_cs != nullptr)
        steam_popup_cs->unlock ();
    return drawn;
  }

  //
  // We don't need this, we always do this from the render thread.
  //
  {
    try
    {
      auto& DisplaySize =
        CEGUI::System::getDllSingleton ().getRenderer ()->
                        getDisplaySize ();

      // If true, we need to redraw all text overlays to prevent flickering
      bool removed = false;
      bool created = false;

#define POPUP_DURATION_MS config.steam.achievements.popup.duration

      auto it =
        popups.begin ();

      float inset =
        config.steam.achievements.popup.inset;

      if (inset < 0.0001f)  inset = 0.0f;

      const float full_ht =
        DisplaySize.d_height * (1.0f - inset);

      const float full_wd = DisplaySize.d_width * (1.0f - inset);

      float x_origin, y_origin,
            x_dir,    y_dir;

      CEGUI::Window* first = it->window;

      const float win_ht0 = ( first != nullptr ?
        (it->window->getPixelSize ().d_height) : 0.0f );
      const float win_wd0 = ( first != nullptr ?
         (it->window->getPixelSize ().d_width) : 0.0f );

      const float title_wd =
        DisplaySize.d_width * (1.0f - 2.0f * inset);

      const float title_ht =
        DisplaySize.d_height * (1.0f - 2.0f * inset);

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

      switch (config.steam.achievements.popup.origin)
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

      CEGUI::UDim x_pos (x_origin, x_off * x_dir);
      CEGUI::UDim y_pos (y_origin, y_off * y_dir);

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
            win->getPixelSize ().d_height;

          const float win_wd =
            win->getPixelSize ().d_width;

          CEGUI::UVector2 win_pos (x_pos, y_pos);

          float bottom_y = win_pos.d_y.d_scale * full_ht +
                           win_pos.d_y.d_offset          +
                           win_ht;

          // The bottom of the window would be off-screen, so
          //   move it to the top and offset by the width of
          //     each popup.
          if ( bottom_y > full_ht || bottom_y < win_ht0 )
          {
            y_pos  = CEGUI::UDim (y_origin, y_off * y_dir);
            x_pos += x_dir * win->getSize ().d_width;

            win_pos   = (CEGUI::UVector2 (x_pos, y_pos));
          }

          float right_x = win_pos.d_x.d_scale * full_wd +
                          win_pos.d_x.d_offset          +
                          win_wd;

          // Window is off-screen horizontally AND vertically
          if ( inset != 0.0f && (right_x > full_wd || right_x < win_wd0)  )
          {
            // Since we're not going to draw this, treat it as a new
            //   popup until it first becomes visible.
            (*it).time =
              SK_timeGetTime ();
            win->hide        ();
          }

          else
          {
            if (config.steam.achievements.popup.animate)
            {
              CEGUI::UDim percent (
                CEGUI::UDim (y_origin, y_off).percent ()
              );

              if (percent_of_lifetime <= 0.1f)
              {
                win_pos.d_y /= (percent * 100.0f *
                                CEGUI::UDim (percent_of_lifetime / 0.1f, 0.0f));
              }

              else if (percent_of_lifetime >= 0.9f)
              {
                win_pos.d_y /= (percent * 100.0f *
                                CEGUI::UDim ( (1.0f -
                                (percent_of_lifetime - 0.9f) / 0.1f),
                                0.0f ));
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

            win->show        ();
            win->setPosition (win_pos);
          }

          y_pos += y_dir * win->getSize ().d_height;

          ++it;
          //} else {
          //it = popups.erase (it);
          //}
        }

        else
        {
          //if (SK_PopupManager::getInstance ()->isPopup ((*it).window)) {
          CEGUI::Window* win = (*it).window;

          CEGUI::WindowManager::getDllSingleton ().destroyWindow (win);

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
          CEGUI::System::getDllSingleton   ().renderAllGUIContexts ();
      }

      // Popup is in the final location, so now is when screenshots
      //   need to be taken.
      if (config.steam.achievements.take_screenshot && take_screenshot > 0)
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
    }

    catch (const CEGUI::GenericException&) {}
  }

  if (steam_popup_cs != nullptr)
      steam_popup_cs->unlock ();

  return drawn;
}


CEGUI::Window*
SK_AchievementManager::createPopupWindow (SK_AchievementPopup* popup)
{
  if (! (config.cegui.enable && config.cegui.frames_drawn > 0)) return nullptr;

  if (popup->achievement == nullptr)
    return nullptr;

  auto log =
    ( SK::EOS::UserID () != nullptr ) ?
                   epic_log.getPtr () : steam_log.getPtr ();

  CEGUI::System* pSys =
    CEGUI::System::getDllSingletonPtr ();

  extern CEGUI::Window* SK_achv_popup;

  char     szPopupName [32] = { };
  sprintf (szPopupName, "Achievement_%i", lifetime_popups++);

  popup->window              = SK_achv_popup->clone (true);
  Achievement*   achievement = popup->achievement;
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
  }

  if (config.steam.achievements.popup.show_title)
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
}