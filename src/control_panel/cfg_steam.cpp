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

#if 0
class SK_Steam_CloudManager
{
public:
   SK_Steam_CloudManager (void)
   {
     pCloudControl = nullptr;
   };

  ~SK_Steam_CloudManager (void)
  {
    if (query.IsActive ())
    {
      query.Cancel ();
    }

    //pCloudControl->ReleaseQueryUGCRequest (query_handle);

    pCloudControl = nullptr;
  }

  SteamAPICall_t
  initialQuery (void)
  {
    if (! pCloudControl)
      pCloudControl = steam_ctx.UGC ();

    if (! (pCloudControl && (! query.IsActive ())))
      return k_uAPICallInvalid;

    static bool run_once = false;

    if (run_once) return 0;

    run_once = true;

    ISteamUtils* pUtils =
      steam_ctx.Utils ();

    if (! pUtils)
      return k_uAPICallInvalid;

    const AppId_t app_id =
      pUtils->GetAppID ();

    UGCQueryHandle_t query_handle =
      pCloudControl->CreateQueryAllUGCRequest (
        k_EUGCQuery_RankedByPublicationDate,
        /*k_EUGCMatchingUGCType_All=*/(EUGCMatchingUGCType)~0,
        app_id, app_id,
        1
      );

    if (query_handle != k_UGCQueryHandleInvalid)
    {
      const SteamAPICall_t api_call =
        pCloudControl->SendQueryUGCRequest (query_handle);

      if (api_call != k_uAPICallInvalid)
      {
        query.Set (api_call, this, &SK_Steam_CloudManager::OnQueryCompleted);
      }

      else
      {
        pCloudControl->ReleaseQueryUGCRequest (query_handle);
      }
    }
  }

  void
  finishQuery (UGCQueryHandle_t query_handle)
  {
    if (! pCloudControl)
      return;

    int idx = 0;

    SteamUGCDetails_t details = { };

    while (pCloudControl->GetQueryUGCResult (query_handle, idx++, &details))
    {
      steam_log->Log ( L"SteamUGC [%lu] :: AppID = %lu, Name = %hs", idx - 1,
                         details.m_nCreatorAppID, details.m_pchFileName );
    }
  }

  void
  OnQueryCompleted (SteamUGCQueryCompleted_t* pCompleted, bool bIOFail)
  {
    if (bIOFail) return;

    if (pCompleted->m_unNumResultsReturned > 0)
    {
      UGCQueryHandle_t query_handle =
        pCompleted->m_handle;

      finishQuery (query_handle);
    }

    pCloudControl->ReleaseQueryUGCRequest (pCompleted->m_handle);
  };

protected:
private:
  ISteamUGC* pCloudControl;

  CCallResult <SK_Steam_CloudManager, SteamUGCQueryCompleted_t> query;
};

void
SK_Steam_CloudQuery (void)
{
  static SK_Steam_CloudManager cloud_mgr;

  cloud_mgr.initialQuery ();
}
#endif

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

    else if (app_has_cloud_storage == 1 /* Yes */ && files.empty ())
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


#ifdef _ENABLE_OBSOLETE_REDISTRIBUTABLE_CLEANUP
    static uint64_t redist_size  = UINT64_MAX;
    static int      redist_count = -1;

    if (redist_count == -1 || redist_size == UINT64_MAX)
    {
      const uint64_t finished_size =
        SK_Steam_ScrubRedistributables (redist_count, false);

      if (redist_count != -1)
        redist_size = finished_size;
    }

    if (redist_count > 0)
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

      const bool summarize =
        ImGui::CollapsingHeader ("Wasted Disk Space", ImGuiTreeNodeFlags_DefaultOpen);

      ImGui::PopStyleColor (3);

      if (summarize)
      {
        ImGui::TreePush ("");

        if (
          ImGui::MenuItem ( SK_FormatString (
                              "Steam is currently wasting %ws on %i redistributable files!",
                                SK_File_SizeToStringF (redist_size, 5, 2).data,
                                  redist_count
                            ).c_str (), "" )
           )
        {
          redist_size =
            SK_Steam_ScrubRedistributables (redist_count, true);
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Click here to free the wasted space.");

        ImGui::TreePop  ();
      }
    }
#endif

    if (app_has_cloud_storage == 1 && ImGui::CollapsingHeader ("Cloud Storage"))
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
      SK_SteamAPI_GetOverlayState (false);

    // New version of ImGui doesn't allow right-clicking disabled items, so we have to break this to fix it.
    if (ImGui::Selectable ( "SteamAPI Frame", &pause/*, SK::SteamAPI::IsOverlayAware () ? 0 : ImGuiSelectableFlags_Disabled*/) &&
                                                      SK::SteamAPI::IsOverlayAware ())
    {
      SK_SteamAPI_SetOverlayState (pause);
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

    if (SK::SteamAPI::IsOverlayAware ())
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
          SK::SteamAPI::SetOverlayState (config.platform.reuse_overlay_pause);
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

struct sk_depot_get_t {
  wchar_t   wszHostName [INTERNET_MAX_HOST_NAME_LENGTH] = { };
  wchar_t   wszHostPath [INTERNET_MAX_PATH_LENGTH]      = { };
  DepotId_t depot_id                                    =  0;
};

const wchar_t*
SK_SteamDB_BuildRandomAgentString (void)
{
  const wchar_t* wszAgents0 [] = {
    L"Mozilla/5.0 (Windows; U; Windows NT 10.0; ",
    L"Mozilla/5.0 (Windows; U; Windows NT 10.0; ",
    L"Mozilla/5.0 (Windows; U; Windows NT 6.2; ",
    L"Mozilla/5.0 (Windows; U; Windows NT 6.1; ",
    L"Mozilla/5.0 (Windows; U; Windows NT 6.1; ",
    L"Mozilla/5.0 (X11; Linux x86_64; "
  };
  const wchar_t* wszAgents1 [] = {
    L"en-US; ",
    L"ja-JP; ",
    L"sv-SE; ",
    L"pt-BR; ",
    L"en-US; ",
    L"fr-FR; "
  };
  const wchar_t* wszAgents2 [] = {
    L"Valve Steam GameOverlay/1551832902; ) ",
    L"Valve Steam GameOverlay/1258164314; ) ",
    L"Valve Steam GameOverlay/1035679203; ) ",
    L"Valve Steam GameOverlay/1327041285; ) "
  };
  const wchar_t* wszAgents3 [] = {
    L"AppleWebKit/537.36 (KHTML, like Gecko) Chrome/68.0.3440.106 Safari/537.36",
    L"AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.110 Safari/537.36",
    L"AppleWebKit/537.36 (KHTML, like Gecko) Chrome/64.0.3282.140 Safari/537.36",
    L"AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36",
    L"AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.90 Safari/537.36"
  };

  SK_RunOnce (srand (SK_timeGetTime ()));

  // Use one of these fake agent strings for the entire runtime of the application.
  static unsigned int agent_idx0 =
    ( rand () % (sizeof (wszAgents0) / sizeof (wchar_t*)) );
  static unsigned int agent_idx1 =
    ( rand () % (sizeof (wszAgents1) / sizeof (wchar_t*)) );
  static unsigned int agent_idx2 =
    ( rand () % (sizeof (wszAgents2) / sizeof (wchar_t*)) );
  static unsigned int agent_idx3 =
    ( rand () % (sizeof (wszAgents3) / sizeof (wchar_t*)) );

  static const std::wstring random_agent =
    SK_FormatStringW (L"%s%s%s%s", wszAgents0 [agent_idx0],
                                   wszAgents1 [agent_idx1],
                                   wszAgents2 [agent_idx2],
                                   wszAgents3 [agent_idx3]);


  static bool first = true;

  if (first)
  {
    first = false;

    SK_LOG0 ( ( L"Randomized User Agent: %s", random_agent.c_str () ),
                L" Steam DB ");
  }

  return
    random_agent.c_str ();
}

DWORD
WINAPI
SK_SteamDB_ManifestFetch (sk_depot_get_t* get)
{
  SetCurrentThreadDescription (L"[SK] Patch Steam Roller(backer)");

  ULONG ulTimeout = 5000UL;

  PCWSTR rgpszAcceptTypes [] = { L"*/*", nullptr };
  HINTERNET hInetHTTPGetReq  = nullptr,
            hInetHost        = nullptr,
  hInetRoot                  =
    InternetOpen (
      SK_SteamDB_BuildRandomAgentString (),
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


      SK_LOG0 ( ( L"WinInet Failure (%x): %ws",
                    dwLastError,
        _com_error (dwLastError).ErrorMessage   ()
                ), L" Steam DB " );
    }

    if (hInetHTTPGetReq != nullptr) InternetCloseHandle (hInetHTTPGetReq);
    if (hInetHost       != nullptr) InternetCloseHandle (hInetHost);
    if (hInetRoot       != nullptr) InternetCloseHandle (hInetRoot);

    sk_depot_get_t* to_delete = nullptr;
    std::swap (get, to_delete);
    delete          to_delete;

    return 0;
  };

  if (hInetRoot == nullptr)
    return CLEANUP ();

  DWORD_PTR dwInetCtx = 0;

  hInetHost =
    InternetConnect ( hInetRoot,
                        get->wszHostName,
                          INTERNET_DEFAULT_HTTP_PORT,
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
                          get->wszHostPath,
                            L"HTTP/1.1",
                              nullptr,
                                rgpszAcceptTypes,
                                                                    INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                  INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_IGNORE_CERT_CN_INVALID   |
                                  INTERNET_FLAG_RESYNCHRONIZE     | INTERNET_FLAG_CACHE_ASYNC,
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

    DWORD dwContentLength     = 0;
    DWORD dwContentLength_Len = sizeof (DWORD);
    DWORD dwSizeAvailable;

    HttpQueryInfo ( hInetHTTPGetReq,
                      HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
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

          concat_buffer.insert ( concat_buffer.cend (),
                                  http_chunk.cbegin (),
                                  http_chunk.cbegin () + dwSizeRead );

          if (dwSizeRead < dwSizeAvailable)
            break;
        }
      }

      else
        break;
    }

    concat_buffer.insert (concat_buffer.cend (), '\0');
    concat_buffer.insert (concat_buffer.cend (), '\0');

    char *szDepotName =
      StrStrIA (concat_buffer.data (),"<td>Name</td>\n<td>");

    std::string name;

    if (szDepotName != nullptr)
    {   szDepotName += 18;

      const char* szDepotEndTag =
        StrStrIA (szDepotName, "</td>");

      name                       = szDepotName;
      name.resize (szDepotEndTag - szDepotName);
    }

    char *szManifestHeading =
      StrStrIA (concat_buffer.data (), "<h2>Previous manifests</h2>");
    char *szTableBegin = nullptr,
         *szTableEnd   = nullptr;

    if (szManifestHeading != nullptr)
    {
      szTableBegin =
        StrStrIA (szManifestHeading, "<tbody>");

      if (szTableBegin != nullptr)
      {
        szTableEnd =
          StrStrIA (szTableBegin, "</tbody>");
      }
    }


    if (szTableEnd != nullptr)
    {
      char *pos         = szTableBegin;
      char *closing_tag = nullptr;

      while ( pos != nullptr &&
              pos <  szTableEnd )
      {
        SK_Steam_DepotManifest package;

        pos =
          StrStrIA (pos, "<td class=\"text-right\">") + 23;


        if (reinterpret_cast <char *> (23) == pos)
          break;

        closing_tag =
          StrStrIA (pos, "</td>");

        package.depot.name    =
          SK_WideCharToUTF8 (SK_UTF8ToWideChar (name));

        package.manifest.date =
          SK_WideCharToUTF8 (
            SK_UTF8ToWideChar (
              std::string ( pos, closing_tag - pos )
                              )
                            );

        pos =
          StrStrIA (closing_tag, "<td>") + 4;

        if (reinterpret_cast <char *> (4) == pos)
          break;

        closing_tag =
          StrStrIA (pos, "</td>");

        package.manifest.id =
          strtoll ( std::string (pos, closing_tag - pos).c_str (), nullptr, 0 );

        SK_Steam_DepotManifestRegistry [get->depot_id].emplace_back (
          package
        );

        pos = closing_tag;
      }
    }
  }

  CLEANUP (true);

  return 1;
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
#if 0
  if (SK::SteamAPI::AppID () != 0)
  {
    if (ImGui::BeginMenu ("Steam"))
    {
      auto* user_ex =
        steam_ctx.UserEx ();

      if (user_ex != nullptr)
      {
        if (! user_ex->BConnected ())
        {
          if (ImGui::MenuItem ("Connect to Steam"))
          {
            SK_Steam_ConnectUserIfNeeded (user_ex->GetSteamID ());
          }
        }

        else
        {
          const auto logon_state =
            user_ex->GetLogonState ();

          static DWORD dwStartLogOffTime = SK::ControlPanel::current_time;
          static DWORD dwStartLogOnTime  = SK::ControlPanel::current_time;

          switch (logon_state)
          {
            case SK::SteamAPI::k_ELogonStateNotLoggedOn:
            case SK::SteamAPI::k_ELogonStateLoggingOff:
            {
              if (ImGui::MenuItem ("Log On"))
              {
                dwStartLogOnTime = SK::ControlPanel::current_time;
                user_ex->LogOn (user_ex->GetSteamID ());
              }

              if (logon_state == SK::SteamAPI::k_ELogonStateLoggingOff)
              {
                ImGui::Separator  (  );
                ImGui::TreePush   ("");
                ImGui::BulletText ("Logging off... (%3.1f seconds)",
                  double (SK::ControlPanel::current_time - dwStartLogOffTime)
                         / 1000.0
                );
                ImGui::TreePop    (  );
              }
            } break;

            case SK::SteamAPI::k_ELogonStateLoggedOn:
            case SK::SteamAPI::k_ELogonStateLoggingOn:
            {
              if (ImGui::MenuItem ("Log Off"))
              {
                dwStartLogOffTime = SK::ControlPanel::current_time;
                user_ex->LogOff ();
              }

              if (logon_state == SK::SteamAPI::k_ELogonStateLoggingOn)
              {
                ImGui::Separator  (  );
                ImGui::TreePush   ("");
                ImGui::BulletText ("Logging on... (%3.1f seconds)",
                  double (SK::ControlPanel::current_time - dwStartLogOnTime)
                         / 1000.0
                );
                ImGui::TreePop    (  );
              }

              //
              // This broke when Valve introduced the overhauled chat system,
              //   it's more or less obsolete now thanks to the Invisible mode.
              //
              ////else
              ////{
              ////  ImGui::Separator ();
              ////
              ////  int state =
              ////    SK::SteamAPI::GetPersonaState ();
              ////
              ////  bool always_anti_social =
              ////    config.steam.online_status != -1;
              ////
              ////  if (ImGui::Checkbox ("Always Appear", &always_anti_social))
              ////  {
              ////    if (always_anti_social) config.steam.online_status = state;
              ////    else                    config.steam.online_status =    -1;
              ////  }
              ////
              ////  ImGui::SameLine ();
              ////
              ////  // Range restrict -- there are online states that are meaningless while in-game,
              ////  //                     we want to hide these, but not force them on/off.
              ////  state = std::min (2, std::max (0, state));
              ////
              ////  if ( ImGui::Combo ( "###Steam_Social_Status",
              ////                        &state,
              ////                          "Offline\0"
              ////                          "Online\0"
              ////                          "Busy\0\0",
              ////                            3 ) )
              ////  {
              ////    if (always_anti_social)
              ////      config.steam.online_status = state;
              ////
              ////    SK::SteamAPI::SetPersonaState ((EPersonaState)state);
              ////  }
              ////}
            } break;
          }
        }
      }


      if (SK::SteamAPI::AppID () != 0)
      {
        ImGui::Separator ();

        if (ImGui::MenuItem ("Edit Steam Application Manifest"))
        {
          SK_ShellExecuteW ( HWND_DESKTOP,   L"OPEN",
                             L"notepad.exe", SK_Steam_GetApplicationManifestPath ().c_str (),
                             nullptr,        SW_SHOWNORMAL );

          SK_ImGui_Warning (L"Remember to close notepad, Steam will count the game as running until you do.");
        }
      }


      auto InitializeSteamDepots = [&](void) ->
      void
      {
        for ( auto cache_record : SK_Steam_DepotManifestRegistry.get () )
          cache_record.second.clear ();

        SK_Steam_DepotManifestRegistry->clear ();

        SK_Thread_Create ([](LPVOID)->
        DWORD
        {
          std::vector <SK_Steam_Depot> depots =
            SK_UseManifestToGetDepots (SK::SteamAPI::AppID ());

          for ( const auto& it : depots )
          {
            auto* get =
              new sk_depot_get_t { };

            URL_COMPONENTSW urlcomps = { };

            urlcomps.dwStructSize     = sizeof (URL_COMPONENTSW);

            urlcomps.lpszHostName     = get->wszHostName;
            urlcomps.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

            urlcomps.lpszUrlPath      = get->wszHostPath;
            urlcomps.dwUrlPathLength  = INTERNET_MAX_PATH_LENGTH;

            if (SK_Steam_InstalledManifest->count (it.depot) == 0)
            {
              SK_Steam_InstalledManifest [it.depot].manifest.id =
                SK_UseManifestToGetDepotManifest (SK::SteamAPI::AppID (), it.depot);
            }

            get->depot_id = it.depot;

            std::wstring url  = L"http://steamdb.info/depot/";
                         url += std::to_wstring (it.depot);
                         url += L"/manifests/";

            if ( InternetCrackUrl (          url.c_str  (),
                    sk::narrow_cast <DWORD> (url.length ()),
                                      0x00,
                                        &urlcomps
                                  )
               )
            {
              if (! SK_SteamDB_ManifestFetch (get))
              {
                SK_LOG0 ( ( L"Failed to fetch depot manifest info for (appid=%lu, depot=%lu)",
                            SK::SteamAPI::AppID (), it.depot ),
                            L" Steam DB " );
              }
            }
          }

          // Stash and cache
          app_cache_mgr->saveAppCache ();

          SK_Thread_CloseSelf ();

          return 0;
        });
      };

      if (SK_Steam_DepotManifestRegistry->empty ())
        SK_RunOnce (InitializeSteamDepots ());

      ImGui::Separator ();

      bool unroll =
        ImGui::BeginMenu (  SK_FormatString ( "Steam Unroller (%d Depots)###SK_STEAMROLL_PATCH_FIX",
                                                SK_Steam_DepotManifestRegistry.get ().size ()
                                            ).c_str ()
                         );

      if (unroll)
      {
        if (! SK_Steam_DepotManifestRegistry->empty ())
        {
          for ( const auto& it : SK_Steam_DepotManifestRegistry.get () )
          {
            if (ImGui::BeginMenu (it.second.front ().depot.name.c_str ()))
            {
              // 2/18/20 -- Replaced repeated statement w/ this reference
              auto& test_manifest =
                SK_Steam_InstalledManifest [it.first].manifest;

              for ( const auto& it2 : it.second )
              {
                bool selected =
                  ( test_manifest.id == it2.manifest.id );

                if (ImGui::MenuItem ( SK_FormatString ( "%s##%llu",
                                                          it2.manifest.date.c_str (),
                                                            it2.manifest.id
                                                      ).c_str (), "",  &selected ) &&
                                             test_manifest.id != it2.manifest.id
                   )
                {
                  if (OpenClipboard (nullptr))
                  {
                    std::wstring command =
                      SK_FormatStringW ( L"download_depot %u %u %llu %llu",
                                         SK::SteamAPI::AppID (),
                                                       it.first,
                                                             it2.manifest.id,
                                                            test_manifest.id
                                       );

                    const SIZE_T len =
                      command.length () + 1;

                    HGLOBAL hGlobal =
                      GlobalAlloc (GHND, len * sizeof (wchar_t));

                    if (hGlobal != nullptr)
                    {
                      wchar_t *wszCommand =
                        sk::narrow_cast <wchar_t *> (
                          GlobalLock (hGlobal)
                        );

                      wcscpy (wszCommand, command.c_str ());

                      GlobalUnlock     (hGlobal);
                      EmptyClipboard   (       );
                      SetClipboardData (CF_UNICODETEXT,
                                        hGlobal);
                      CloseClipboard   (       );
                      GlobalFree       (hGlobal);

                      SK_ShellExecute (L"OPEN", L"steam://nav/console");

                      SK_ImGui_Warning ( L"An unpatch command has been added to the clipboard; paste it to Steam's console and press Enter.\n\n\t"
                                         L"Once the Steam console indicates completion, copy files:"
                                         L"\n\n\t\tfrom 'content\\app_...\\depot_...\\' to your game's install directory." );

                      ///SK_Steam_InstalledManifest [it.first].manifest.id =
                      ///  atoll (SK_WideCharToUTF8 (sec.get_value (key_to_use.c_str ())).c_str ());

                      InterlockedExchange (&__SK_Steam_Downloading, it.first);

                      test_manifest.id = it2.manifest.id;

                      SK_Thread_Create ([](LPVOID) ->
                        DWORD
                        {
                          extern const wchar_t*
                            SK_GetSteamDir (void);

                          std::wstring content_dir =
                            SK_GetSteamDir ();

                          content_dir +=
                            SK_FormatStringW ( LR"(\steamapps\content\app_%lu\depot_%lu\)",
                                                 SK::SteamAPI::AppID (),
                                                   ReadAcquire (&__SK_Steam_Downloading) );

                          // TODO: Add Directory Watch
                          int tries = 0;
                          do {
                            SK_Sleep (100UL);
                          } while ((! ::PathIsDirectory (content_dir.c_str ())) && tries++ < 1200);

                          SK_ShellExecute (L"EXPLORE", content_dir.c_str ());

                          app_cache_mgr->saveAppCache ();

                          SK_Thread_CloseSelf ();

                          return 0;
                        }
                      );
                    }
                  }
                }
              }

              ImGui::EndMenu ();
            }
          }

          ImGui::Separator ();
        }

        bool selected = false;

        if (ImGui::MenuItem ("Refresh Patch List", "", &selected))
          InitializeSteamDepots ();

        ImGui::EndMenu (); // Steam Unroll
      } ImGui::EndMenu (); // Steam

      return  true;
    }
  }
#endif

  return false;
}


bool
SK::ControlPanel::Steam::WarnIfUnsupported (void)
{
  static DWORD dwLastTime = 0x00;

  bool pirate = ( SK::SteamAPI::AppID  () != 0 &&
                  SK_Steam_PiratesAhoy () != 0x0 );
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

  return false;
}