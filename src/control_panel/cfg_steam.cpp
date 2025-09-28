// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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


#include <SpecialK/control_panel/steam.h>
#include <SpecialK/control_panel/platform.h>

extern volatile LONG SK_SteamAPI_CallbackRateLimit;
extern volatile LONG __SK_Steam_Downloading;

using namespace SK::ControlPanel;

extern bool
SK_Denuvo_UsedByGame (bool retest = false);

struct denuvo_file_s
{
  AppId_t      app;
  CSteamID     user;
  uint64       hash;
  std::wstring path;
  FILETIME     ft_key;
  SYSTEMTIME   st_local;
};

extern SK_LazyGlobal <std::vector <denuvo_file_s>> denuvo_files;

bool
SK::ControlPanel::Steam::Draw (void)
{
  if (SK::SteamAPI::AppID () != 0)
  {
    auto* pRemote = (! config.platform.steam_is_b0rked) ?
                             steam_ctx.RemoteStorage () : nullptr;

    static BOOL app_has_cloud_storage = -1 /* Unknown */;

    if (app_has_cloud_storage == -1 && pRemote != nullptr)
        app_has_cloud_storage = ReadAcquire64 (&SK_SteamAPI_CallbackRunCount) > 0 &&
                                             pRemote->IsCloudEnabledForAccount () &&
                                             pRemote->IsCloudEnabledForApp     ();

    struct sk_steam_cloud_entry_s {
      std::string filename;
      std::string detail;   // Temporary, formatted string.
      int32_t     size;     // Apparently 2 GiB is a hard limit, but we can
                            //   always use negative storage if we run out ;)
      int64_t     timestamp;
      bool        blacklisted;
    };

    static std::vector <sk_steam_cloud_entry_s> files;

    // Empty
    if ( app_has_cloud_storage != -1 &&
                  files.size () == 1 &&
                  files[0].size == 0 )
      app_has_cloud_storage = 0 /* No */;

    else if (app_has_cloud_storage == 1 /* Yes */ && files.empty () && pRemote != nullptr)
    {
      const int32_t num_files =
        pRemote->GetFileCount ();

      for (int i = 0; i < num_files; i++)
      {
        sk_steam_cloud_entry_s file = { };

        file.filename  =
          pRemote->GetFileNameAndSize (i, &file.size);
        file.timestamp =
          pRemote->GetFileTimestamp     (file.filename.c_str ());

        file.blacklisted =
          (config.steam.cloud.blacklist.count (file.filename) != 0);

        files.emplace_back (file);
      }

      if (files.empty ())
          files.emplace_back (sk_steam_cloud_entry_s { "No Files", "", 0L, 0LL, false });
    }



    if (app_has_cloud_storage != FALSE && ImGui::CollapsingHeader ("Cloud Storage"))
    {
      bool dirty = false;

      const  DWORD cycle_freq = 2250UL;
      static DWORD last_cycle = current_time;
      static int   detail_idx = 0;

      if (last_cycle < current_time - cycle_freq)
      {
        detail_idx = ~detail_idx;
        last_cycle =  current_time;
        dirty      =  true;
      }

      ImGui::BeginChild ( "CloudStorageList",
                            ImVec2 ( -1.0f, font.size_multiline *
                      static_cast <float> (files.size ())       +
                                     0.1f * font.size_multiline ),
                              true,
                                ImGuiWindowFlags_AlwaysAutoResize );
      for ( auto& it : files )
      {
        ImGui::PushID   (it.filename.c_str ());

        bool allow_sync = false;

        if (it.size != 0)
        {
          allow_sync =
            (! it.blacklisted);
        }

        if (dirty)
        {
          switch (detail_idx)
          {
            case 0:
              it.detail = SK_FormatString ("%i Bytes", it.size);
              break;
            default:
            {
              const struct tm *t = _localtime64 (&it.timestamp);
                    it.detail    =      asctime (  t);
            } break;
          }
        }

        if ( ImGui::MenuItem ( it.filename.c_str (),
                               it.detail.c_str   (), &allow_sync ) )
        {
          if (        allow_sync  && config.steam.cloud.blacklist.count (it.filename) != 0)
          {
            config.steam.cloud.blacklist.erase (it.filename);
            it.blacklisted = false;
          }

          else if ((! allow_sync) && config.steam.cloud.blacklist.count (it.filename) == 0)
          {
            config.steam.cloud.blacklist.emplace (it.filename);
            it.blacklisted = true;
          }

          SK_SaveConfig ();
        }

        ImGui::PopID    ();
      }
      ImGui::EndChild   ();
    }

    if (SK_Denuvo_UsedByGame ())
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.00f, 0.00f, 0.00f, 1.00f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.00f, 0.00f, 0.00f, 1.00f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.00f, 0.00f, 0.00f, 1.00f));
      ImGui::PushStyleColor (ImGuiCol_Text,          (ImVec4&&)ImColor::HSV (0.15f, 1.0f, 1.0f));

      if (ImGui::CollapsingHeader ("Denuvo"))
      {
        ImGui::PopStyleColor (4);

        ImGui::TreePush ("");

        size_t idx = 0;

        ImGui::BeginGroup ();
        for ( const auto& it : denuvo_files.get () )
        {
          const size_t found =
            it.path.find_last_of (L'\\');

          ImGui::BeginGroup      ();
          ImGui::Text            ( "Key %zi:",
                                         idx++ );
          ImGui::TextUnformatted ( "First Activated:" );
          ImGui::EndGroup        ();

          ImGui::SameLine        ();

          ImGui::BeginGroup      ();
          ImGui::Text            ( "%hs", SK_WideCharToUTF8 (&it.path.c_str ()[found + 1]).c_str () );

          if (denuvo_files->size () > idx)
          {
            ImGui::SameLine      ();
            ImGui::TextColored   (ImColor::HSV (0.08f, 1.f, 1.f), " [ Expired ]");
          }

          ImGui::Text            ( "%02d/%02d/%d  %02d:%02d",
                                     it.st_local.wMonth, it.st_local.wDay, it.st_local.wYear,
                                     it.st_local.wHour,  it.st_local.wMinute );

          ImGui::EndGroup        ();
        }
        ImGui::EndGroup          ();

        ImGui::SameLine          ();

        bool retest = false;

        ImGui::BeginGroup        ();
        for ( const auto& it : denuvo_files.get () )
        {
          if (ImGui::Button      (SK_FormatString ("  Delete Me  ###%ws", it.path.c_str ()).c_str ()))
          {
            if (DeleteFileW (it.path.c_str ()))
              retest = true;

            else
            {
              SK_ImGui_Warning (
                SK_FormatStringW (
                  L"Could not delete file: '%ws'", it.path.c_str ()
                ).c_str ()
              );
            }
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip  ();
            ImGui::Text          ("Force Denuvo to Re-Activate the next time the Game Starts");
            ImGui::Separator     ();
            ImGui::BulletText    ("Useful if you plan to go offline for an extended period.");
            ImGui::BulletText    (" >> RESTART the game immediately after doing this to re-activate <<");
            ImGui::EndTooltip    ();
          }

          ImGui::TextUnformatted ( "" );
        }
        ImGui::EndGroup          ();

        ImGui::TreePop  ();

        if (retest)
          SK_Denuvo_UsedByGame (true); // Re-Test
      }

      else
      {
        ImGui::PopStyleColor   (4);
      }
    }

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    bool bDefaultOpen = false;

    // Uncollapse this header by default if the user is forcing the overlay off; make it obvious!
    if ( config.steam.disable_overlay ||
         config.platform.silent ) bDefaultOpen = true;

    if (ImGui::CollapsingHeader ("Compatibility", bDefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen
                                                               : 0x0))
    {
      ImGui::TreePush   ("");
      ImGui::BeginGroup (  );

      if (ImGui::Checkbox (" Disable SteamAPI Integration", &config.platform.silent))
      {
        SK_SaveConfig ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    ();
        ImGui::TextUnformatted ("Turns off almost all Steam-related features");
        ImGui::Separator       ();
        ImGui::BulletText      ("Steam Input problems will not be fixed, and background input will be unsupported in Steam Input games.");
        ImGui::BulletText      ("Optimized Steam screenshots and achievement popups will be disabled.");
        ImGui::BulletText      ("The Steam Overlay can still be disabled");
        ImGui::Separator       ();
        ImGui::TextUnformatted ("If compatibility is not a problem, consider disabling individual Steam features, rather than all of them.");
        ImGui::EndTooltip      ();        
      }

      if (ImGui::Checkbox (" Prevent Overlay From Drawing  ",    &config.steam.disable_overlay))
      {
        SetEnvironmentVariable (
          L"SteamNoOverlayUIDrawing", config.steam.disable_overlay ?
                                                              L"1" : L"0"
        );
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Game Restart Required");

      if (! config.platform.silent)
      {
        ImGui::Checkbox (" Disable User Stats Receipt Callback",   &config.steam.block_stat_callback);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Fix for Games that Panic when Flooded with Achievement Data");
          ImGui::Separator    ();
          ImGui::BulletText   ("These Games may shutdown SteamAPI when Special K fetches Friend Achievements");
          ImGui::BulletText   ("If SteamAPI Frame Counter is STUCK, turn this option ON and restart the Game");
          ImGui::EndTooltip   ();
        }
      }

      if (! config.platform.silent)
      {
        ImGui::EndGroup   (  );
        ImGui::SameLine   (  );
        ImGui::BeginGroup (  );

        ImGui::Checkbox   (" Bypass Online \"DRM\" Checks  ",      &config.steam.spoof_BLoggedOn);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::TextColored  (ImColor::HSV (0.159f, 1.0f, 1.0f), "Fixes pesky games that use SteamAPI to deny Offline mode");
          ImGui::Separator    ();
          ImGui::BulletText   ("This is a much larger problem than you would believe.");
          ImGui::BulletText   ("This also fixes some games that crash when Steam disconnects (unrelated to DRM).");
          ImGui::EndTooltip   ();
        }

        ImGui::Checkbox   (" Load Steam Overlay Early  ",          &config.steam.preload_overlay);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Can make the Steam Overlay work in situations it otherwise would not.");

        ImGui::Checkbox   (" Load Steam Client DLL Early  ",       &config.steam.preload_client);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("May prevent some Steam DRM-based games from hanging at startup.");
      }

      ImGui::EndGroup ();
      ImGui::TreePop  ();
    }

    ImGui::PopStyleColor (3);

    bool valid =
      (! config.platform.silent) &&
      (! SK_Steam_PiratesAhoy ());

    if (valid)
      ImGui::MenuItem ("SteamAPI Valid",   "", &valid, false);
    else
      ImGui::MenuItem ("SteamAPI Invalid", "", &valid, false);

    return true;
  }

  return false;
}

bool
SK::ControlPanel::Steam::DrawFooter (void)
{
  if (SK::SteamAPI::AppID () != 0)
  {
    ImGui::Columns    ( 1 );
    ImGui::Separator  (   );

    if (SK::SteamAPI::GetNumPlayers () > 1)
    {
      ImGui::Columns    ( 2, "SteamSep", true );

      static char szNumber       [16] = { };
      static char szPrettyNumber [32] = { };

      const NUMBERFMTA fmt = { 0, 0, 3, (char *)".", (char *)",", 0 };

      snprintf (szNumber, 15, "%i", SK::SteamAPI::GetNumPlayers ());

      GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
                           0x00,
                             szNumber, &fmt,
                               szPrettyNumber, 32 );

      ImGui::Text       (" %s Players in-Game on Steam  ", szPrettyNumber);
      ImGui::NextColumn (   );
    }

    ImGui::Bullet     ();   ImGui::SameLine ();

    bool pause =
      SK_Platform_GetOverlayState (false);

    // New version of ImGui doesn't allow right-clicking disabled items, so we have to break this to fix it.
    if (ImGui::Selectable ( "SteamAPI Frame", &pause) &&
                          SK_Platform_IsOverlayAware ())
    {
      SK_Platform_SetOverlayState (pause);
    }

    const auto SteamCallbackThrottleSubMenu =
    [&](void) ->
    void
    {
      int rate = ReadAcquire (&SK_SteamAPI_CallbackRateLimit);

      if (ImGui::SliderInt ("Limit SteamAPI Rate", &rate, -1, 240))
          InterlockedExchange (&SK_SteamAPI_CallbackRateLimit, rate);
    };


    const bool right_clicked =
      SK_ImGui_IsItemRightClicked ();

    if (SK_Platform_IsOverlayAware ())
    {
      if (right_clicked)
      {
        ImGui::OpenPopup         ("SteamOverlayPauseMenu");
        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiCond_Always);
      }

      else if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip   (       );
        ImGui::Text           ( "In"  );                 ImGui::SameLine ();
        ImGui::PushStyleColor ( ImGuiCol_Text, ImVec4 (0.95f, 0.75f, 0.25f, 1.0f) );
        ImGui::Text           ( "Steam Overlay Aware");  ImGui::SameLine ();
        ImGui::PopStyleColor  (       );
        ImGui::Text           ( "software, click to toggle the game's overlay pause mode." );
        ImGui::EndTooltip     (       );
      }

      if (ImGui::BeginPopup ("SteamOverlayPauseMenu"))
      {
        if (ImGui::Checkbox ("Pause Game while Control Panel is Visible",
            &config.platform.reuse_overlay_pause))
        {
          SK_Platform_SetOverlayState (config.platform.reuse_overlay_pause);
        }

        SteamCallbackThrottleSubMenu ();

        ImGui::EndPopup     ();
      }
    }

    else if (right_clicked)
    {
      ImGui::OpenPopup         ("SteamCallbackRateMenu");
      ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiCond_Always);
    }

    if (ImGui::BeginPopup ("SteamCallbackRateMenu"))
    {
      SteamCallbackThrottleSubMenu ();

      ImGui::EndPopup     ();
    }

    ImGui::SameLine ();
    ImGui::Text     ( ": %10llu  ", ReadAcquire64 (&SK_SteamAPI_CallbackRunCount) );
    ImGui::Columns(1, nullptr, false);

    // If fetching stats and online...
    if ( (! SK_SteamAPI_FriendStatsFinished  ())        &&
            SK_SteamAPI_GetNumPlayers        ()  > 1    &&
            SK_SteamAPI_FriendStatPercentage () != 0.0f &&
            config.platform.achievements.pull_friend_stats )
    {
      const float ratio   = SK_SteamAPI_FriendStatPercentage ();
      const int   friends = SK_SteamAPI_GetNumFriends        ();

      static char szLabel [512] = { };

      snprintf ( szLabel, 511,
                   "Fetching Achievements... %.2f%% (%u/%u) : %s",
                     100.0 * ratio,
static_cast <uint32_t> (
                    ratio * static_cast <float>    (friends)
                  ),
                            static_cast <uint32_t> (friends),
                  SK_SteamAPI_GetFriendName (
static_cast <uint32_t> (
                      ratio * static_cast <float>    (friends))
                  ).c_str ()
               );

      ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f) );
      ImGui::PushStyleColor ( ImGuiCol_Text,          ImVec4 (1.0f,   1.0f,  1.0f,  1.0f) );
      ImGui::ProgressBar (
        SK_SteamAPI_FriendStatPercentage (),
          ImVec2 (-1, 0), szLabel );
      ImGui::PopStyleColor  (2);
    }

    return true;
  }

  return false;
}


SK_LazyGlobal <Concurrency::concurrent_unordered_map <DepotId_t,           SK_DepotList> > SK_Steam_DepotManifestRegistry;
SK_LazyGlobal <Concurrency::concurrent_unordered_map <DepotId_t, SK_Steam_DepotManifest> > SK_Steam_InstalledManifest;

SK_DepotList&
SK_AppCache_Manager::getAvailableManifests (DepotId_t steam_depot)
{
  return
    (*SK_Steam_DepotManifestRegistry)[steam_depot];
}

ManifestId_t
SK_AppCache_Manager::getInstalledManifest (DepotId_t steam_depot)
{
  if (! app_cache_db)
    return 0ULL;

  auto& sec =
    app_cache_db->get_section_f (L"Depot.%lu", steam_depot);

  if (! sec.contains_key (L"Installed"))
    return 0ULL;

  return
    static_cast <uint64_t> (
      strtoll (SK_WideCharToUTF8 (sec.get_value (L"Installed")).c_str (), nullptr, 0)
    );
}

int
SK_AppCache_Manager::loadDepotCache (DepotId_t steam_depot)
{
  if (! app_cache_db)
    return 0;

  for ( auto& cache_record : SK_Steam_DepotManifestRegistry.get () )
    cache_record.second.clear ();


  auto& sections =
    app_cache_db->get_sections ();

  for ( const auto& sec : sections )
  {
    if (StrStrIW (sec.first.c_str (), L"Depot."))
    {
      std::string   depot_name;
      unsigned long depot_id   = 0;

      if (! std::swscanf (sec.first.c_str (), L"Depot.%lu", &depot_id))
        continue;

      if ( steam_depot != 0 &&
           steam_depot != depot_id )
      {
        continue;
      }

      for ( const auto& key : sec.second.keys )
      {
        if ( key.first._Equal (L"Installed") )
        {
          SK_Steam_InstalledManifest [depot_id].depot.id    = depot_id;
          SK_Steam_InstalledManifest [depot_id].manifest.id =
            wcstoll (key.second.c_str (), nullptr, 10);
        }

        else if ( key.first._Equal (L"Name") )
        {
          depot_name =
            SK_WideCharToUTF8 (key.second);

          SK_Steam_InstalledManifest [depot_id].depot.name =
            depot_name;
        }

        else
        {
          unsigned long long manifest_id =
            wcstoll (key.first.c_str (), nullptr, 10);

          SK_Steam_DepotManifest depot_record;

          depot_record.manifest.id   = manifest_id;
          depot_record.manifest.date = SK_WideCharToUTF8 (key.second);

          SK_Steam_DepotManifestRegistry [depot_id].emplace_back (depot_record);
        }
      }

      for ( auto& cache_entry : SK_Steam_DepotManifestRegistry [depot_id] )
      {
        cache_entry.depot.name = depot_name;
        cache_entry.depot.id   = depot_id;
      }
    }
  }

  return 0;
}

int
SK_AppCache_Manager::storeDepotCache (DepotId_t steam_depot)
{
  for ( const auto& depot : SK_Steam_DepotManifestRegistry.get () )
  {
    if ( steam_depot != 0 &&
         steam_depot != depot.first )
    {
      continue;
    }

    auto& sec =
      app_cache_db->get_section_f (L"Depot.%lu", depot.first);

    for ( const auto& cache_entry : depot.second )
    {
      if (! cache_entry.depot.name.empty ())
      {
        sec.add_key_value ( L"Name",
          SK_UTF8ToWideChar (cache_entry.depot.name).c_str ()
        );
      }

      sec.add_key_value ( std::to_wstring   (cache_entry.manifest.id  ).c_str (),
                          SK_UTF8ToWideChar (cache_entry.manifest.date).c_str () );
    }

    sec.add_key_value ( L"Installed",
                          std::to_wstring (
                            SK_Steam_InstalledManifest [depot.first].manifest.id
                          ).c_str ()
                      );
  }

  return 0;
}

void
SK_ShellExecute (const wchar_t* verb, const wchar_t* file)
{
  SK_AutoCOMInit auto_com (
    COINIT_MULTITHREADED//COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE
  );

  SHELLEXECUTEINFO
  sei        = { };
  sei.cbSize = sizeof (sei);
  sei.nShow  = SW_SHOWNORMAL;

  sei.fMask  = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC |
               SEE_MASK_WAITFORINPUTIDLE;
  sei.lpVerb = verb;
  sei.lpFile = file;

  if (ShellExecuteEx (&sei))
  {
    DWORD                     shell_pid = GetProcessId (sei.hProcess);
    AllowSetForegroundWindow (shell_pid);

    window_t win =
      SK_FindRootWindow (shell_pid);

    if (win.root != nullptr)
    {
      SetForegroundWindow (win.root);
      SetActiveWindow     (win.root);
    }
    SK_CloseHandle (sei.hProcess);
  }
};

bool
SK::ControlPanel::Steam::DrawMenu (void)
{
  return false;
}


bool
SK::ControlPanel::Steam::WarnIfUnsupported (void)
{
  bool pirate = ( SK::SteamAPI::AppID  () != 0 &&
                  SK_Steam_PiratesAhoy () != 0x0 );

#if 0
  static DWORD dwLastTime = 0x00;

  if (pirate)
  {
    if (dwLastTime < current_time - 1000)
    {
      dwLastTime             = current_time;

      eula.show              = true;
      eula.never_show_again  = false;

      return true;
    }
  }
#else
  std::ignore = pirate;
#endif

  return false;
}
