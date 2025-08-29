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
#include <storefront/gog.h>
#include <storefront/achievements.h>

#include <json/json.hpp>

#include <galaxy/IListenerRegistrar.h>
#include <galaxy/IUtils.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"GOG Galaxy"

class SK_Galaxy_OverlayManager : public galaxy::api::IOverlayStateChangeListener
{
public:
  bool isActive (void) const {
    return active_;
  }

  void OnOverlayStateChanged (bool overlayIsActive)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    // If the game has an activation callback installed, then
    //   it's also going to see this event... make a note of that when
    //     tracking its believed overlay state.
    if (SK::Galaxy::IsOverlayAware ())
        SK::Galaxy::overlay_state = overlayIsActive;


    // If we want to use this as our own, then don't let the Epic overlay
    //   unpause the game on deactivation unless the control panel is closed.
    if (config.platform.reuse_overlay_pause && SK_Platform_IsOverlayAware ())
    {
      // Deactivating, but we might want to hide this event from the game...
      if (overlayIsActive == false)
      {
        // Undo the event the game is about to receive.
        if (SK_ImGui_Visible) SK_Platform_SetOverlayState (true);
      }
    }

    const bool wasActive =
                  active_;

    active_ = overlayIsActive;

    if (wasActive != active_)
    {
      auto& io =
        ImGui::GetIO ();

      static bool capture_keys  = io.WantCaptureKeyboard;
      static bool capture_text  = io.WantTextInput;
      static bool capture_mouse = io.WantCaptureMouse;
      static bool nav_active    = io.NavActive;

      // When the overlay activates, stop blocking
      //   input !!
      if (! wasActive)
      {
        capture_keys  =
          io.WantCaptureKeyboard;
          io.WantCaptureKeyboard = false;

        capture_text  =
          io.WantTextInput;
          io.WantTextInput       = false;

        capture_mouse =
          io.WantCaptureMouse;
          io.WantCaptureMouse    = false;

        nav_active    =
          io.NavActive;
          io.NavActive           = false;

         ImGui::SetWindowFocus (nullptr);
      }

      else
      {
        io.WantCaptureKeyboard = SK_ImGui_Visible ? capture_keys  : false;
        io.WantCaptureMouse    = SK_ImGui_Visible ? capture_mouse : false;
        io.NavActive           = SK_ImGui_Visible ? nav_active    : false;

        ImGui::SetWindowFocus (nullptr);
        io.WantTextInput       = false;//capture_text;
      }
    }
  }

  void invokeCallbacks (bool active)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    for ( const auto &[listener,callback_data] : callbacks )
    {
      if (listener == nullptr)
        continue;

      auto callback =
        (galaxy::api::IOverlayStateChangeListener *)listener;

      callback->OnOverlayStateChanged (active);
    }
  }

  void
  Register (galaxy::api::IGalaxyListener* listener)
  {
    SK_LOG_FIRST_CALL

    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    if (listener != nullptr)
    {
      callbacks [listener] = { };
    }
  }

  void
  Unregister (galaxy::api::IGalaxyListener* listener)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    if ( const auto cb  = callbacks.find (listener);
                    cb != callbacks.end  (  ) )
    {
      callbacks.erase (cb);
    }
  }

  bool
  isOverlayAware (void) const
  {
    return
      (! callbacks.empty ());
  }

private:
  bool cursor_visible_ = false; // Cursor visible prior to activation?
  bool active_         = false;

  struct callback_s
  {
    // TODO
  };

public:
  std::unordered_map <galaxy::api::IGalaxyListener*, callback_s> callbacks;
  SK_Thread_HybridSpinlock                                       callback_cs;
};

SK_LazyGlobal <SK_Galaxy_OverlayManager>     galaxy_overlay;
//SK_LazyGlobal <SK_Galaxy_AchievementManager> galaxy_achievements;

bool
WINAPI
SK_IsGalaxyOverlayActive (void)
{
  return galaxy_overlay->isActive ();
}

void
__stdcall
SK::Galaxy::SetOverlayState (bool active)
{
  if (config.platform.silent)
    return;

  galaxy_overlay->invokeCallbacks (active);

  overlay_state = active;
}

bool
__stdcall
SK::Galaxy::GetOverlayState (bool real)
{
  return real ? SK_IsGalaxyOverlayActive () :
    overlay_state;
}

bool
__stdcall
SK::Galaxy::IsOverlayAware (void)
{
  if (! galaxy_overlay.getPtr ())
    return false;

  return
    galaxy_overlay->isOverlayAware ();
}

void
SK::Galaxy::Init (void)
{
  if (config.platform.silent)
    return;

  int             gog_gameId =  0;
  WIN32_FIND_DATA fd         = {};

  auto _FindGameInfo = [&](const wchar_t* wszPattern)
  {
    HANDLE hFind =
      FindFirstFileW (wszPattern, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          if (1 == swscanf (fd.cFileName, L"goggame-%d.info", &gog_gameId))
          {
            gog_log->Log (
              L"GOG gameId: %d", gog_gameId
            );

            if (gog_gameId != 0)
            {
              config.platform.type = SK_Platform_GOG;
            }

            if (config.platform.equivalent_steam_app == -1)
            {
              std::wstring url =
                SK_FormatStringW (
                  LR"(https://www.pcgamingwiki.com/w/api.php?action=cargoquery&format=json&tables=Infobox_game&fields=Steam_AppID&where=GOGcom_ID+HOLDS+%%27%d%%27)",
                                                                                                                                       gog_gameId
                );

              SK_Network_EnqueueDownload (
                sk_download_request_s (L"pcgw_appid_map.json", url.data (),
                  []( const std::vector <uint8_t>&& data,
                      const std::wstring_view       file )
                  {
                    if (data.empty ())
                      return true;

                    std::ignore = file;

                    try {
                      gog_log->Log (L"PCGW Queried Data: %hs", data.data ());

                      nlohmann::json json =
                        std::move (
                          nlohmann::json::parse ( data.cbegin (),
                                                  data.cend   (), nullptr, true )
                        );

                      for ( auto& query : json ["cargoquery"] )
                      {
                        if ( query.contains ("title") &&
                             query.at       ("title").contains ("Steam AppID") )
                        {
                          config.platform.equivalent_steam_app =
                            atoi (
                              query.at ("title").
                                    at ("Steam AppID").
                                    get <std::string_view> ().
                                                      data () );
                          break;
                        }
                      }

                      gog_log->Log (L"Steam AppID: %d", config.platform.equivalent_steam_app);

                      if (config.platform.equivalent_steam_app != -1)
                      {   config.utility.save_async ();
                        void SK_Platform_PingBackendForNonSteamGame (void);
                             SK_Platform_PingBackendForNonSteamGame ();
                      }

                      else
                      {
                        config.platform.equivalent_steam_app = 0;
                      }
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

                    return true;
                  }
                )
              );
            }

            break;
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }
  };

  if (config.platform.equivalent_steam_app == -1)
  {
    // Stupid way of dealing with Unreal Engine games
                                   _FindGameInfo (               L"goggame-*.info");
    if         (gog_gameId == 0) { _FindGameInfo (            L"../goggame-*.info");
      if       (gog_gameId == 0) { _FindGameInfo (         L"../../goggame-*.info");
        if     (gog_gameId == 0) { _FindGameInfo (      L"../../../goggame-*.info");
          if   (gog_gameId == 0) { _FindGameInfo (   L"../../../../goggame-*.info");
            if (gog_gameId == 0) { _FindGameInfo (L"../../../../../goggame-*.info"); } } } } }

    if (gog_gameId != 0)
    {
      config.platform.type = SK_Platform_GOG;
    }
  }

  const wchar_t*
    wszGalaxyDLLName =
      SK_RunLHIfBitness ( 64, L"Galaxy64.dll",
                                L"Galaxy.dll" );

  static HMODULE     hModGalaxy = nullptr;
  if (std::exchange (hModGalaxy, SK_LoadLibraryW (wszGalaxyDLLName)) == nullptr && hModGalaxy != nullptr)
  {
    gog_log->init (L"logs/galaxy.log", L"wt+,ccs=UTF-8");
    gog_log->silent = config.platform.silent;

    gog_log->Log (L"Galaxy DLL: %p", SK_LoadLibraryW (wszGalaxyDLLName));

    SK_ICommandProcessor* cmd = nullptr;

    #define cmdAddAliasedVar(name,pVar)                 \
      for ( const char* alias : { "GOG."      #name,    \
                                  "Platform." #name } ) \
        cmd->AddVariable (alias, pVar);

    SK_RunOnce (
      cmd =
        SK_Render_InitializeSharedCVars ()
    );

    if (cmd != nullptr)
    {
#if 0
      cmdAddAliasedVar (TakeScreenshot,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.take_screenshot));
      cmdAddAliasedVar (ShowPopup,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.popup.show));
      cmdAddAliasedVar (PopupDuration,
          SK_CreateVar (SK_IVariable::Int,
                          (int  *)&config.platform.achievements.popup.duration));
      cmdAddAliasedVar (PopupInset,
          SK_CreateVar (SK_IVariable::Float,
                          (float*)&config.platform.achievements.popup.inset));
      cmdAddAliasedVar (ShowPopupTitle,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.popup.show_title));
      cmdAddAliasedVar (PopupAnimate,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.popup.animate));
      cmdAddAliasedVar (PlaySound,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.play_sound));
#endif

      gog->PreInit (hModGalaxy);
    }
  }

  auto _SetupGalaxy =
  [&](void)
  {
#if 0
    // Hook code here (i.e. Listener registrar Register/Unregister and IGalaxy::ProcessData (...))

    bool bEnable = SK_EnableApplyQueuedHooks ();
                         SK_ApplyQueuedHooks ();
    if (!bEnable) SK_DisableApplyQueuedHooks ();
#endif
  };

  if (hModGalaxy != nullptr)
  {
    SK_RunOnce (
      if (SK::Galaxy::GetTicksRetired () == 0)
        _SetupGalaxy ();
    );
  }
}

void
SK::Galaxy::Shutdown (void)
{
}

void
SK_GalaxyContext::PreInit (HMODULE hGalaxyDLL)
{
  sdk_dll_ = hGalaxyDLL;
}

bool
__stdcall
SK_Galaxy_GetOverlayState (bool real)
{
  return
    SK::Galaxy::GetOverlayState (real);
}


SK_LazyGlobal <SK_GalaxyContext> gog;
bool SK::Galaxy::overlay_state = false;

volatile LONGLONG __SK_Galaxy_Ticks = 0;

LONGLONG
SK::Galaxy::GetTicksRetired (void)
{
  return
    ReadAcquire64 (&__SK_Galaxy_Ticks);
}

// TODO: Add hook on IGalaxy::ProcessData (...) here that increments __SK_Galaxy_Ticks