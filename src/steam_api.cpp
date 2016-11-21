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

#define _CRT_SECURE_NO_WARNINGS

#include "resource.h"

#include "core.h"

#include "config.h"
#include "ini.h"
#include "log.h"

// PlaySound
#pragma comment (lib, "winmm.lib")

iSK_Logger steam_log;

static volatile ULONG __SK_Steam_init = FALSE;

// We're not going to use DLL Import - we will load these function pointers
//  by hand.
#define STEAM_API_NODLL
#include "steam_api.h"


#include <time.h>

// To spoof Overlay Activation (pause the game)
#include <set>
#include <unordered_map>

std::multiset <class CCallbackBase *> overlay_activation_callbacks;

CRITICAL_SECTION callback_cs = { 0 };
CRITICAL_SECTION init_cs     = { 0 };
CRITICAL_SECTION popup_cs    = { 0 };

#undef min
#undef max

#include "CEGUI/CEGUI.h"
#include "CEGUI/System.h"


bool S_CALLTYPE SteamAPI_InitSafe_Detour (void);
bool S_CALLTYPE SteamAPI_Init_Detour     (void);

void S_CALLTYPE SteamAPI_Shutdown_Detour (void);

S_API typedef bool (S_CALLTYPE *SteamAPI_Init_t    )(void);
S_API typedef bool (S_CALLTYPE *SteamAPI_InitSafe_t)(void);

S_API typedef bool (S_CALLTYPE *SteamAPI_RestartAppIfNecessary_t)( uint32 unOwnAppID );
S_API typedef bool (S_CALLTYPE *SteamAPI_IsSteamRunning_t)(void);

S_API typedef void (S_CALLTYPE *SteamAPI_Shutdown_t)(void);

S_API typedef void (S_CALLTYPE *SteamAPI_RegisterCallback_t)
          (class CCallbackBase *pCallback, int iCallback);
S_API typedef void (S_CALLTYPE *SteamAPI_UnregisterCallback_t)
          (class CCallbackBase *pCallback);

S_API typedef void (S_CALLTYPE *SteamAPI_RegisterCallResult_t)
          (class CCallbackBase *pCallback, SteamAPICall_t hAPICall );
S_API typedef void (S_CALLTYPE *SteamAPI_UnregisterCallResult_t)
          (class CCallbackBase *pCallback, SteamAPICall_t hAPICall );

S_API typedef void (S_CALLTYPE *SteamAPI_RunCallbacks_t)(void);

S_API typedef HSteamUser (*SteamAPI_GetHSteamUser_t)(void);
S_API typedef HSteamPipe (*SteamAPI_GetHSteamPipe_t)(void);

S_API typedef ISteamClient*    (S_CALLTYPE *SteamClient_t   )(void);

S_API SteamAPI_RunCallbacks_t         SteamAPI_RunCallbacks         = nullptr;
S_API SteamAPI_RegisterCallback_t     SteamAPI_RegisterCallback     = nullptr;
S_API SteamAPI_UnregisterCallback_t   SteamAPI_UnregisterCallback   = nullptr;
S_API SteamAPI_RegisterCallResult_t   SteamAPI_RegisterCallResult   = nullptr;
S_API SteamAPI_UnregisterCallResult_t SteamAPI_UnregisterCallResult = nullptr;

S_API SteamAPI_Init_t               SteamAPI_Init               = nullptr;
S_API SteamAPI_InitSafe_t           SteamAPI_InitSafe           = nullptr;
S_API SteamAPI_Shutdown_t           SteamAPI_Shutdown           = nullptr;

S_API SteamAPI_RestartAppIfNecessary_t SteamAPI_RestartAppIfNecessary=nullptr;
S_API SteamAPI_IsSteamRunning_t        SteamAPI_IsSteamRunning       =nullptr;

S_API SteamAPI_GetHSteamUser_t      SteamAPI_GetHSteamUser      = nullptr;
S_API SteamAPI_GetHSteamPipe_t      SteamAPI_GetHSteamPipe      = nullptr;

S_API SteamClient_t                 SteamClient                 = nullptr;

S_API SteamAPI_RegisterCallback_t   SteamAPI_RegisterCallback_Original   = nullptr;
S_API SteamAPI_UnregisterCallback_t SteamAPI_UnregisterCallback_Original = nullptr;

S_API SteamAPI_Shutdown_t           SteamAPI_Shutdown_Original = nullptr;

class SK_Steam_OverlayManager {
public:
  SK_Steam_OverlayManager (void) :
       activation ( this, &SK_Steam_OverlayManager::OnActivate      ) {
    cursor_visible_ = false;
    active_         = false;
  }

  STEAM_CALLBACK ( SK_Steam_OverlayManager,
                   OnActivate,
                   GameOverlayActivated_t,
                   activation )
  {
    if (active_ != pParam->m_bActive) {
      if (pParam->m_bActive) {
        static bool first = true;

        if (first) {
          cursor_visible_ = ShowCursor (FALSE) >= -1;
                            ShowCursor (TRUE);
          first = false;
        }
      }

      // Restore the original cursor state.
      else {
        if (! cursor_visible_) {
          ULONG calls = 0;

          steam_log.Log (L"  (Steam Overlay Deactivation:  Hiding Mouse Cursor... )");
          while (ShowCursor (FALSE) >= 0)
            ++calls;

          if (calls == 0)
            steam_log.Log (L"  (Steam Overlay Deactivation:  Overlay is functioning correctly => NOP)");
          else
            steam_log.Log (L"  (Steam Overlay Deactivation:  Overlay leaked state => Took %lu tries)", calls);
        }
      }
    }

    active_ = pParam->m_bActive;
  }

  bool isActive (void) { return active_; }

private:
  bool cursor_visible_; // Cursor visible prior to activation?
  bool active_;
} *overlay_manager = nullptr;

bool
WINAPI
SK_IsSteamOverlayActive (void)
{
  if (overlay_manager)
    return overlay_manager->isActive ();

  return false;
}


#include "utility.h"

ISteamUserStats* SK_SteamAPI_UserStats   (void);
void             SK_SteamAPI_ContextInit (HMODULE hSteamAPI);

S_API
void
S_CALLTYPE
SteamAPI_RegisterCallback_Detour (class CCallbackBase *pCallback, int iCallback)
{
  // Don't care about OUR OWN callbacks ;)
  if (SK_GetCallingDLL () == SK_GetDLL ()) {
    SteamAPI_RegisterCallback_Original (pCallback, iCallback);
    return;
  }

  // Sometimes a game will have initialized SteamAPI before this DLL was loaded,
  //   we will usually catch wind of this because callbacks will be registered
  //     seemingly before initialization.
  //
  //  Take this opportunity to finish SteamAPI initialization for SpecialK.
  if (! InterlockedExchangeAdd (&__SK_Steam_init, 0)) {
    CreateThread ( nullptr, 0,
         [](LPVOID user) ->
    DWORD {
      static bool spawned_init = false;

      // Already started a late init. thread
      if (spawned_init) {
        CloseHandle (GetCurrentThread ());
        return 0;
      }

      spawned_init = true;

      Sleep (5000UL);

      if (! InterlockedExchangeAdd (&__SK_Steam_init, 0)) {
        steam_log.Log ( L"Initializing SteamWorks Backend  << %s >>",
                          SK_GetCallerName ().c_str () );
        steam_log.Log (L"----------(Implicit)-----------\n");

        // TODO: Scoped UnHook / ReHook
        SK_DisableHook (SteamAPI_RegisterCallback);

        InterlockedExchange (&__SK_Steam_init, TRUE);

#ifdef _WIN64
        const wchar_t* steam_dll_str = L"steam_api64.dll";
#else
        const wchar_t* steam_dll_str = L"steam_api.dll";
#endif

        HMODULE hSteamAPI;
        GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, steam_dll_str, &hSteamAPI);

        SK_SteamAPI_ContextInit (hSteamAPI);

        steam_log.Log ( L"--- Initialization Finished ([AppId: %lu]) ---\n\n",
                          SK::SteamAPI::AppID () );

        extern void StartSteamPump (void);

        StartSteamPump ();

        SK_EnableHook (SteamAPI_RegisterCallback);
      }

      CloseHandle (GetCurrentThread ());
      return 0;
    }, nullptr, 0x00, nullptr );
  }

  std::wstring caller = 
    SK_GetCallerName ();

  EnterCriticalSection (&callback_cs);

  switch (iCallback)
  {
    case GameOverlayActivated_t::k_iCallback:
    {
      steam_log.Log ( L" * (%-28s) Installed Overlay Activation Callback",
                        caller.c_str () );
      overlay_activation_callbacks.insert (pCallback);
    } break;
    case ScreenshotRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Screenshot Callback",
                        caller.c_str () );
      break;
    case UserStatsReceived_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Stats Receipt Callback",
                        caller.c_str () );
      break;
    case UserStatsStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Stats Storage Callback",
                        caller.c_str () );
      break;
    case UserAchievementStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Achievements Storage Callback",
                        caller.c_str () );
      break;
    case DlcInstalled_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed DLC Installation Callback",
                        caller.c_str () );
      break;
    case SteamAPICallCompleted_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Async. SteamAPI Completion Callback",
                        caller.c_str () );
      break;
    default:
      steam_log.Log ( L" * (%-28s) Installed Unknown Callback (Class=%lu00, Id=%lu)",
                        caller.c_str (), iCallback / 100, iCallback % 100 );
      break;
  }

  SteamAPI_RegisterCallback_Original (pCallback, iCallback);

  LeaveCriticalSection (&callback_cs);
}

void
SK_SteamAPI_EraseActivationCallback (class CCallbackBase *pCallback)
{
  __try {
    if (overlay_activation_callbacks.find (pCallback) != overlay_activation_callbacks.end ())
      overlay_activation_callbacks.erase (pCallback);
  } __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ? 
           EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {
  }
}

S_API
void
S_CALLTYPE
SteamAPI_UnregisterCallback_Detour (class CCallbackBase *pCallback)
{
  // Skip this if we're uninstalling our own callback
  if (SK_GetCallingDLL () == SK_GetDLL ()) {
    SteamAPI_UnregisterCallback_Original (pCallback);
    return;
  }

  std::wstring caller = 
    SK_GetCallerName ();

  EnterCriticalSection (&callback_cs);

  int iCallback = pCallback->GetICallback ();

  switch (iCallback)
  {
    case GameOverlayActivated_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Overlay Activation Callback",
                        caller.c_str () );

      SK_SteamAPI_EraseActivationCallback (pCallback);

      break;
    case ScreenshotRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Screenshot Callback",
                        caller.c_str () );
      break;
    case UserStatsReceived_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Stats Receipt Callback",
                        caller.c_str () );
      break;
    case UserStatsStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Stats Storage Callback",
                        caller.c_str () );
      break;
    case UserAchievementStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Achievements Storage Callback",
                        caller.c_str () );
      break;
    case DlcInstalled_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled DLC Installation Callback",
                        caller.c_str () );
      break;
    case SteamAPICallCompleted_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Async. SteamAPI Completion Callback",
                        caller.c_str () );
      break;
    default:
      steam_log.Log ( L" * (%-28s) Uninstalled Unknown Callback (Class=%lu00, Id=%lu)",
                        caller.c_str (), iCallback / 100, iCallback % 100 );
      break;
  }

  SteamAPI_UnregisterCallback_Original (pCallback);

  LeaveCriticalSection (&callback_cs);
}

typedef bool (S_CALLTYPE* SteamAPI_InitSafe_pfn)(void);
SteamAPI_InitSafe_pfn SteamAPI_InitSafe_Original = nullptr;

bool S_CALLTYPE SteamAPI_InitSafe_Detour (void);

typedef bool (S_CALLTYPE* SteamAPI_Init_pfn)(void);
SteamAPI_Init_pfn SteamAPI_Init_Original = nullptr;

bool S_CALLTYPE SteamAPI_Init_Detour (void);

extern "C" void __cdecl SteamAPIDebugTextHook (int nSeverity, const char *pchDebugText);

class SK_SteamAPIContext {
public:
  bool InitCSteamworks (HMODULE hSteamDLL)
  {
    if (config.steam.silent)
      return false;

    if (SteamAPI_InitSafe == nullptr) {
      SteamAPI_InitSafe =
        (SteamAPI_InitSafe_t)GetProcAddress (
          hSteamDLL,
            "InitSafe"
        );
    }

    if (SteamAPI_GetHSteamUser == nullptr) {
      SteamAPI_GetHSteamUser =
        (SteamAPI_GetHSteamUser_t)GetProcAddress (
           hSteamDLL,
             "GetHSteamUser_"
        );
    }

    if (SteamAPI_GetHSteamPipe == nullptr) {
      SteamAPI_GetHSteamPipe =
        (SteamAPI_GetHSteamPipe_t)GetProcAddress (
           hSteamDLL,
             "GetHSteamPipe_"
        );
    }

    SteamAPI_IsSteamRunning =
      (SteamAPI_IsSteamRunning_t)GetProcAddress (
         hSteamDLL,
           "IsSteamRunning"
      );

    if (SteamClient == nullptr) {
      SteamClient =
        (SteamClient_t)GetProcAddress (
           hSteamDLL,
             "SteamClient_"
        );
    }

    if (SteamAPI_Shutdown == nullptr) {
      SteamAPI_Shutdown =
        (SteamAPI_Shutdown_t)GetProcAddress (
          hSteamDLL,
            "Shutdown"
        );
    }

    bool success = true;

    if (SteamAPI_GetHSteamUser == nullptr) {
      steam_log.Log (L"Could not load GetHSteamUser (...)");
      success = false;
    }

    if (SteamAPI_GetHSteamPipe == nullptr) {
      steam_log.Log (L"Could not load GetHSteamPipe (...)");
      success = false;
    }

    if (SteamClient == nullptr) {
      steam_log.Log (L"Could not load SteamClient (...)");
      success = false;
    }

    if (SteamAPI_RegisterCallback == nullptr) {
      steam_log.Log (L"Could not load RegisterCallback (...)");
      success = false;
    }

    if (SteamAPI_UnregisterCallback == nullptr) {
      steam_log.Log (L"Could not load UnregisterCallback (...)");
      success = false;
    }

    if (SteamAPI_RunCallbacks == nullptr) {
      steam_log.Log (L"Could not load RunCallbacks (...)");
      success = false;
    }

    if (! success)
      return false;

    client_ = SteamClient ();

    HSteamUser hSteamUser = SteamAPI_GetHSteamUser ();
    HSteamPipe hSteamPipe = SteamAPI_GetHSteamPipe ();

    if (! client_)
      return false;

    user_ =
      client_->GetISteamUser (
        hSteamUser,
          hSteamPipe,
            STEAMUSER_INTERFACE_VERSION
      );

    if (user_ == nullptr) {
      steam_log.Log ( L" >> ISteamUser NOT FOUND for version %hs <<",
                        STEAMUSER_INTERFACE_VERSION );
      return false;
    }

    friends_ =
      client_->GetISteamFriends (
        hSteamUser,
          hSteamPipe,
            STEAMFRIENDS_INTERFACE_VERSION
      );

    if (friends_ == nullptr) {
      steam_log.Log ( L" >> ISteamFriends NOT FOUND for version %hs <<",
                        STEAMFRIENDS_INTERFACE_VERSION );
      return false;
    }

    user_stats_ =
      client_->GetISteamUserStats (
        hSteamUser,
          hSteamPipe,
            STEAMUSERSTATS_INTERFACE_VERSION
      );

    if (user_stats_ == nullptr) {
      steam_log.Log ( L" >> ISteamUserStats NOT FOUND for version %hs <<",
                        STEAMUSERSTATS_INTERFACE_VERSION );
      return false;
    }

    apps_ =
      client_->GetISteamApps (
        hSteamUser,
          hSteamPipe,
            STEAMAPPS_INTERFACE_VERSION
      );

    if (apps_ == nullptr) {
      steam_log.Log ( L" >> ISteamApps NOT FOUND for version %hs <<",
                        STEAMAPPS_INTERFACE_VERSION );
      return false;
    }

    utils_ =
      client_->GetISteamUtils ( hSteamPipe,
                                  STEAMUTILS_INTERFACE_VERSION );

    if (utils_ == nullptr) {
      steam_log.Log ( L" >> ISteamUtils NOT FOUND for version %hs <<",
                        STEAMUTILS_INTERFACE_VERSION );
      return false;
    }

    screenshots_ =
      client_->GetISteamScreenshots ( hSteamUser,
                                        hSteamPipe,
                                          STEAMSCREENSHOTS_INTERFACE_VERSION );

    if (screenshots_ == nullptr) {
      steam_log.Log ( L" >> ISteamScreenshots NOT FOUND for version %hs <<",
                        STEAMSCREENSHOTS_INTERFACE_VERSION );
      return false;
    }

    steam_log.LogEx (true, L" # Installing Steam API Debug Text Callback... ");
    SteamClient ()->SetWarningMessageHook (&SteamAPIDebugTextHook);
    steam_log.LogEx (false, L"SteamAPIDebugTextHook\n\n");

    // 4 == Don't Care
    if (config.steam.notify_corner != 4)
      utils_->SetOverlayNotificationPosition (
        (ENotificationPosition)config.steam.notify_corner
      );

    if (config.steam.inset_x != 0 ||
        config.steam.inset_y != 0) {
      utils_->SetOverlayNotificationInset (config.steam.inset_x,
                                           config.steam.inset_y);
    }

    return true;
  }

  bool InitSteamAPI (HMODULE hSteamDLL)
  {
    if (config.steam.silent)
      return false;

    if (SteamAPI_InitSafe == nullptr) {
      SteamAPI_InitSafe =
        (SteamAPI_InitSafe_t)GetProcAddress (
          hSteamDLL,
            "SteamAPI_InitSafe"
        );
    }

    if (SteamAPI_GetHSteamUser == nullptr) {
      SteamAPI_GetHSteamUser =
        (SteamAPI_GetHSteamUser_t)GetProcAddress (
           hSteamDLL,
             "SteamAPI_GetHSteamUser"
        );
    }

    if (SteamAPI_GetHSteamPipe == nullptr) {
      SteamAPI_GetHSteamPipe =
        (SteamAPI_GetHSteamPipe_t)GetProcAddress (
           hSteamDLL,
             "SteamAPI_GetHSteamPipe"
        );
    }

    SteamAPI_IsSteamRunning =
      (SteamAPI_IsSteamRunning_t)GetProcAddress (
         hSteamDLL,
           "SteamAPI_IsSteamRunning"
      );

    if (SteamClient == nullptr) {
      SteamClient =
        (SteamClient_t)GetProcAddress (
           hSteamDLL,
             "SteamClient"
        );
    }

    if (SteamAPI_RegisterCallResult == nullptr) {
      SteamAPI_RegisterCallResult =
        (SteamAPI_RegisterCallResult_t)GetProcAddress (
          hSteamDLL,
            "SteamAPI_RegisterCallResult"
        );
    }

    if (SteamAPI_UnregisterCallResult == nullptr) {
      SteamAPI_UnregisterCallResult =
        (SteamAPI_UnregisterCallResult_t)GetProcAddress (
          hSteamDLL,
            "SteamAPI_UnregisterCallResult"
        );
    }

    if (SteamAPI_Shutdown == nullptr) {
      SteamAPI_Shutdown =
        (SteamAPI_Shutdown_t)GetProcAddress (
          hSteamDLL,
            "SteamAPI_Shutdown"
        );
    }

    bool success = true;

    if (SteamAPI_GetHSteamUser == nullptr) {
      steam_log.Log (L"Could not load SteamAPI_GetHSteamUser (...)");
      success = false;
    }

    if (SteamAPI_GetHSteamPipe == nullptr) {
      steam_log.Log (L"Could not load SteamAPI_GetHSteamPipe (...)");
      success = false;
    }

    if (SteamClient == nullptr) {
      steam_log.Log (L"Could not load SteamClient (...)");
      success = false;
    }

    if (SteamAPI_RegisterCallback == nullptr) {
      steam_log.Log (L"Could not load SteamAPI_RegisterCallback (...)");
      success = false;
    }

    if (SteamAPI_UnregisterCallback == nullptr) {
      steam_log.Log (L"Could not load SteamAPI_UnregisterCallback (...)");
      success = false;
    }

    if (SteamAPI_RunCallbacks == nullptr) {
      steam_log.Log (L"Could not load SteamAPI_RunCallbacks (...)");
      success = false;
    }

    if (! success)
      return false;

    SteamAPI_InitSafe_Original ();

    client_ = SteamClient ();

    HSteamUser hSteamUser = SteamAPI_GetHSteamUser ();
    HSteamPipe hSteamPipe = SteamAPI_GetHSteamPipe ();

    if (! client_)
      return false;

    user_ =
      client_->GetISteamUser (
        hSteamUser,
          hSteamPipe,
            STEAMUSER_INTERFACE_VERSION
      );

    if (user_ == nullptr) {
      steam_log.Log ( L" >> ISteamUser NOT FOUND for version %hs <<",
                        STEAMUSER_INTERFACE_VERSION );
      return false;
    }

    // Blacklist of people not allowed to use my software
    uint32_t aid = user_->GetSteamID ().GetAccountID ();

    if (aid == 64655118 || aid == 183437803) {
      SK_MessageBox ( L"You are not authorized to use this software",
                        L"Unauthorized User", MB_ICONWARNING | MB_OK );
      ExitProcess (0x00);
    }

    friends_ =
      client_->GetISteamFriends (
        hSteamUser,
          hSteamPipe,
            STEAMFRIENDS_INTERFACE_VERSION
      );

    if (friends_ == nullptr) {
      steam_log.Log ( L" >> ISteamFriends NOT FOUND for version %hs <<",
                        STEAMFRIENDS_INTERFACE_VERSION );
      return false;
    }

    user_stats_ =
      client_->GetISteamUserStats (
        hSteamUser,
          hSteamPipe,
            STEAMUSERSTATS_INTERFACE_VERSION
      );

    if (user_stats_ == nullptr) {
      steam_log.Log ( L" >> ISteamUserStats NOT FOUND for version %hs <<",
                        STEAMUSERSTATS_INTERFACE_VERSION );
      return false;
    }

    apps_ =
      client_->GetISteamApps (
        hSteamUser,
          hSteamPipe,
            STEAMAPPS_INTERFACE_VERSION
      );

    if (apps_ == nullptr) {
      steam_log.Log ( L" >> ISteamApps NOT FOUND for version %hs <<",
                        STEAMAPPS_INTERFACE_VERSION );
      return false;
    }

    utils_ =
      client_->GetISteamUtils ( hSteamPipe,
                                  STEAMUTILS_INTERFACE_VERSION );

    if (utils_ == nullptr) {
      steam_log.Log ( L" >> ISteamUtils NOT FOUND for version %hs <<",
                        STEAMUTILS_INTERFACE_VERSION );
      return false;
    }

    screenshots_ =
      client_->GetISteamScreenshots ( hSteamUser,
                                        hSteamPipe,
                                          STEAMSCREENSHOTS_INTERFACE_VERSION );

    if (screenshots_ == nullptr) {
      steam_log.Log ( L" >> ISteamScreenshots NOT FOUND for version %hs <<",
                        STEAMSCREENSHOTS_INTERFACE_VERSION );
      return false;
    }

    //steam_log.LogEx (true, L" # Installing Steam API Debug Text Callback... ");
    //SteamClient ()->SetWarningMessageHook (&SteamAPIDebugTextHook);
    //steam_log.LogEx (false, L"SteamAPIDebugTextHook\n\n");

    // 4 == Don't Care
    if (config.steam.notify_corner != 4)
      utils_->SetOverlayNotificationPosition (
        (ENotificationPosition)config.steam.notify_corner
      );

    if (config.steam.inset_x != 0 ||
        config.steam.inset_y != 0) {
      utils_->SetOverlayNotificationInset (config.steam.inset_x,
                                           config.steam.inset_y);
    }

    return true;
  }

  void Shutdown (void) {
    client_      = nullptr;
    user_        = nullptr;
    user_stats_  = nullptr;
    apps_        = nullptr;
    friends_     = nullptr;
    utils_       = nullptr;
    screenshots_ = nullptr;

#if 0
    if (SteamAPI_Shutdown != nullptr) {
      // We probably should not shutdown Steam API; the underlying
      //  game will do this at a more opportune time for sure.
      SteamAPI_Shutdown ();
    }
#endif
  }

  ISteamUser*        User        (void) { return user_;        }
  ISteamUserStats*   UserStats   (void) { return user_stats_;  }
  ISteamApps*        Apps        (void) { return apps_;        }
  ISteamFriends*     Friends     (void) { return friends_;     }
  ISteamUtils*       Utils       (void) { return utils_;       }
  ISteamScreenshots* Screenshots (void) { return screenshots_; }

protected:
private:
  // TODO: We have an obvious lack of thread-safety here...

  ISteamClient*      client_      = nullptr;
  ISteamUser*        user_        = nullptr;
  ISteamUserStats*   user_stats_  = nullptr;
  ISteamApps*        apps_        = nullptr;
  ISteamFriends*     friends_     = nullptr;
  ISteamUtils*       utils_       = nullptr;
  ISteamScreenshots* screenshots_ = nullptr;
} steam_ctx;

#if 0
struct BaseStats_t
{
  uint64  m_nGameID;
  EResult status;
  uint64  test;
};

S_API typedef void (S_CALLTYPE *steam_callback_run_t)
                       (CCallbackBase *pThis, void *pvParam);

S_API typedef void (S_CALLTYPE *steam_callback_run_ex_t)
                       (CCallbackBase *pThis, void *pvParam, bool, SteamAPICall_t);

steam_callback_run_t    Steam_Callback_RunStat_Orig   = nullptr;
steam_callback_run_ex_t Steam_Callback_RunStatEx_Orig = nullptr;

S_API
void
S_CALLTYPE
Steam_Callback_RunStat (CCallbackBase *pThis, void *pvParam)
{
  steam_log.Log ( L"CCallback::Run (%04Xh, %04Xh)  <Stat %s>;",
                    pThis, pvParam, pThis->GetICallback () == 1002L ? L"Store" : L"Receive" );

  BaseStats_t* stats = (BaseStats_t *)pvParam;

  steam_log.Log ( L" >> Size: %04i, Event: %04i - %04lu, %04llu, %04llu\n", pThis->GetCallbackSizeBytes (), pThis->GetICallback (), stats->status, stats->m_nGameID, stats->test);

  Steam_Callback_RunStat_Orig (pThis, pvParam);
}

S_API
void
S_CALLTYPE
Steam_Callback_RunStatEx (CCallbackBase *pThis, void           *pvParam,
                          bool           tf,    SteamAPICall_t  call)
{
  steam_log.Log ( L"CCallback::Run (%04Xh, %04Xh, %01i, %04llu)  "
                  L"<Stat %s>;",
                    pThis, pvParam, tf, call, pThis->GetICallback () == 1002L ? L"Store" : L"Receive" );

  BaseStats_t* stats = (BaseStats_t *)pvParam;

  steam_log.Log ( L" >> Size: %04i, Event: %04i - %04lu, %04llu, %04llu\n", pThis->GetCallbackSizeBytes (), pThis->GetICallback (), stats->status, stats->m_nGameID, stats->test);

  Steam_Callback_RunStatEx_Orig (pThis, pvParam, tf, call);
}
#endif

#define FREEBIE     96.0f
#define COMMON      75.0f
#define UNCOMMON    50.0f
#define RARE        25.0f
#define VERY_RARE   15.0f
#define ONE_PERCENT  1.0f

const char*
SK_Steam_RarityToColor (float percent)
{
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
}

const char*
SK_Steam_RarityToName (float percent)
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

bool has_global_data  = false;
int  next_friend      = 0;

#define STEAM_CALLRESULT( thisclass, func, param, var ) CCallResult< thisclass, param > var; void func( param *pParam, bool )

class SK_Steam_AchievementManager {
public:
  SK_Steam_AchievementManager (const wchar_t* wszUnlockSound) :
       unlock_listener ( this, &SK_Steam_AchievementManager::OnUnlock                ),
       stat_receipt    ( this, &SK_Steam_AchievementManager::AckStoreStats           ),
       icon_listener   ( this, &SK_Steam_AchievementManager::OnRecvIcon              ),
       async_complete  ( this, &SK_Steam_AchievementManager::OnAsyncComplete         )
  {
    bool xbox = false,
         psn  = false;

    wchar_t wszFileName [MAX_PATH + 2] = { L'\0' };

    if (! wcslen (wszUnlockSound)) {
      iSK_INI achievement_ini (
        SK_EvalEnvironmentVars (
          L"%USERPROFILE%\\Documents\\My Mods\\SpecialK\\Global\\achievements.ini"
        ).c_str ());

      achievement_ini.parse ();

      // If the config file is empty, establish defaults and then write it.
      if (achievement_ini.get_sections ().size () == 0) {
        achievement_ini.import ( L"[Steam.Achievements]\n"
                                 L"SoundFile=psn\n"
                                 L"NoSound=false\n"
                                 L"TakeScreenshot=false\n" );
        achievement_ini.write (        SK_EvalEnvironmentVars (
          L"%USERPROFILE%\\Documents\\My Mods\\SpecialK\\Global\\achievements.ini"
        ).c_str ());
      }

      if (achievement_ini.contains_section (L"Steam.Achievements")) {
        iSK_INISection& sec = achievement_ini.get_section (L"Steam.Achievements");
        if (sec.contains_key (L"SoundFile"))
          wcscpy (wszFileName, sec.get_value (L"SoundFile").c_str ());
      }
    } else {
      wcscpy (wszFileName, wszUnlockSound);
    }

    if ((! _wcsicmp (wszFileName, L"psn")))
      psn = true;
    else if (! _wcsicmp (wszFileName, L"xbox"))
      xbox = true;

    FILE* fWAV = _wfopen (wszFileName, L"rb");

    if ((! psn) && (! xbox) && fWAV != nullptr)
    {
      steam_log.LogEx (true, L"  >> Loading Achievement Unlock Sound: '%s'...",
                       wszFileName);

                  fseek  (fWAV, 0, SEEK_END);
      long size = ftell  (fWAV);
                  rewind (fWAV);

      unlock_sound = (uint8_t *)new uint8_t [size];

      if (unlock_sound != nullptr)
        fread  (unlock_sound, size, 1, fWAV);

      fclose (fWAV);

      steam_log.LogEx (false, L" %d bytes\n", size);

      default_loaded = false;
    } else {
      // Default to PSN if not specified
      if ((! psn) && (! xbox))
        psn = true;

      steam_log.Log (L"  * Loading Built-In Achievement Unlock Sound: '%s'",
                       wszFileName);

      extern HMODULE __stdcall SK_GetDLL (void);
      HRSRC   default_sound;

      if (psn)
        default_sound =
          FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_TROPHY), L"WAVE");
      else
        default_sound =
          FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_XBOX), L"WAVE");

      if (default_sound != nullptr) {
        HGLOBAL sound_ref     =
          LoadResource (SK_GetDLL (), default_sound);
        if (sound_ref != 0) {
          unlock_sound        = (uint8_t *)LockResource (sound_ref);

          default_loaded = true;
        }
      }
    }

    uint32_t reserve =
      std::max (steam_ctx.UserStats ()->GetNumAchievements (), (uint32_t)128UL);

    achievements.list.resize (reserve);

    for (uint32_t i = 0; i < reserve; i++) {
      achievements.list [i] = nullptr;
    }
  }

  ~SK_Steam_AchievementManager (void) {
    if ((! default_loaded) && (unlock_sound != nullptr)) {
      delete [] unlock_sound;
      unlock_sound = nullptr;
    }
  }

  class Achievement {
  public:
    Achievement (const char* szName, ISteamUserStats* stats) {
      global_percent_ = 0.0f;
      unlocked_       = false;
      time_           = 0;

      friends_.possible = 0;
      friends_.unlocked = 0;

      icons_.achieved   = nullptr;
      icons_.unachieved = nullptr;

      name_ =
        _strdup (szName);

      const char* human =
        stats->GetAchievementDisplayAttribute (szName, "name");

      const char* desc =
        stats->GetAchievementDisplayAttribute (szName, "desc");

      human_name_ =
        _strdup (human != nullptr ? human : "<INVALID>");
      desc_ =
        _strdup (desc != nullptr ? desc : "<INVALID>");
    }

    Achievement (Achievement& copy) = delete;

    ~Achievement (void)
    {
      if (name_ != nullptr) {
        free ((void *)name_);
        name_ = nullptr;
      }

      if (human_name_ != nullptr) {
        free ((void *)human_name_);
        human_name_ = nullptr;
      }

      if (desc_ != nullptr) {
        free ((void *)desc_);
        desc_ = nullptr;
      }

      if (icons_.unachieved != nullptr) {
        free ((void *)icons_.unachieved);
        icons_.unachieved = nullptr;
      }

      if (icons_.achieved != nullptr) {
        free ((void *)icons_.achieved);
        icons_.achieved = nullptr;
      }
    }

    void update (ISteamUserStats* stats)
    {
      stats->GetAchievementAndUnlockTime ( name_,
                                             &unlocked_,
                                               (uint32_t *)&time_ );
    }

    void update_global (ISteamUserStats* stats)
    {
      // Reset to 0.0 on read failure
      if (! stats->GetAchievementAchievedPercent (
              name_,
                &global_percent_ ) ) {
        steam_log.Log (L" Global Achievement Read Failure For '%hs'", name_);
        global_percent_ = 0.0f;
      }
    }

    const char* name_;
    const char* human_name_;
    const char* desc_;

    float       global_percent_;

    struct {
      int unlocked; // Number of friends who have unlocked
      int possible; // Number of friends who may be able to unlock
    } friends_;

    struct {
      uint8_t*  achieved;
      uint8_t*  unachieved;
    } icons_;

    bool        unlocked_;
    __time32_t  time_;
  };

  void log_all_achievements (void)
  {
    ISteamUserStats* stats = steam_ctx.UserStats ();

    if (stats == nullptr)
      return;

    for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
    {
      Achievement* pAchievement = achievements.list [i];

      if (! pAchievement)
        continue;

      Achievement& achievement = *pAchievement;

      steam_log.LogEx (false, L"\n [%c] Achievement %03lu......: '%hs'\n",
                         achievement.unlocked_ ? L'X' : L' ',
                           i, achievement.name_
                      );
      steam_log.LogEx (false,
                              L"  + Human Readable Name...: %hs\n",
                         achievement.human_name_);
      if (achievement.desc_ != nullptr && strlen (achievement.desc_))
        steam_log.LogEx (false,
                                L"  *- Detailed Description.: %hs\n",
                          achievement.desc_);

      if (achievement.global_percent_ > 0.0f)
        steam_log.LogEx (false,
                                L"  #-- Rarity (Global).....: %6.2f%%\n",
                          achievement.global_percent_);

      if (achievement.friends_.possible > 0)
        steam_log.LogEx ( false,
                                 L"  #-- Rarity (Friend).....: %6.2f%%\n",
                           100.0 * ((double)achievement.friends_.unlocked /
                                    (double)achievement.friends_.possible) );

      if (achievement.unlocked_) {
        steam_log.LogEx (false,
                                L"  @--- Player Unlocked At.: %s",
                                  _wctime32 (&achievement.time_));
      }
    }

    steam_log.LogEx (false, L"\n");
  }

  STEAM_CALLRESULT ( SK_Steam_AchievementManager,
                     OnRecvSelfStats,
                     UserStatsReceived_t,
                     self_listener )
  {
    self_listener.Cancel ();

    if (pParam->m_nGameID != SK::SteamAPI::AppID ()) {
      steam_log.Log ( L" Got User Achievement Stats for Wrong Game (%lu)",
                        pParam->m_nGameID );
      return;
    }

    if (pParam->m_eResult == k_EResultOK)
    {
      ISteamUser*      user    = steam_ctx.User      ();
      ISteamUserStats* stats   = steam_ctx.UserStats ();
      ISteamFriends*   friends = steam_ctx.Friends   ();

      if (! (user && stats && friends && pParam))
        return;

      if ( pParam->m_steamIDUser.GetAccountID () ==
           user->GetSteamID ().GetAccountID   () )
      {
        pullStats ();

        if (config.steam.achievements.pull_global_stats && (! has_global_data)) {
          has_global_data = true;

          global_request =
            stats->RequestGlobalAchievementPercentages ();
        }

        // On first stat reception, trigger an update for all friends so that
        //   we can display friend achievement stats.
        if ( next_friend == 0 && config.steam.achievements.pull_friend_stats )
        {
          int friend_count = friends->GetFriendCount (k_EFriendFlagImmediate);

          if (friend_count > 0) {
            SteamAPICall_t hCall =
              stats->RequestUserStats (
                friends->GetFriendByIndex ( next_friend++,
                                              k_EFriendFlagImmediate
                                          )
                                      );

            friend_listener.Set (
              hCall,
                this,
                  &SK_Steam_AchievementManager::OnRecvFriendStats );
          }
        }
      }
    }
  }

  STEAM_CALLRESULT ( SK_Steam_AchievementManager,
                     OnRecvFriendStats,
                     UserStatsReceived_t,
                     friend_listener )
  {
    friend_listener.Cancel ();

    if (pParam->m_nGameID != SK::SteamAPI::AppID ()) {
      steam_log.Log ( L" Got User Achievement Stats for Wrong Game (%lu)",
                        pParam->m_nGameID );
      return;
    }

    ISteamUser*      user    = steam_ctx.User      ();
    ISteamUserStats* stats   = steam_ctx.UserStats ();
    ISteamFriends*   friends = steam_ctx.Friends   ();

    // A user may return kEResultFail if they don't own this game, so
    //   do this anyway so we can continue enumerating all friends.
    if (pParam->m_eResult == k_EResultOK)
    {
      if (! (user && stats && friends && pParam))
        return;

      // If the stats are not for the player, they are probably for one of
      //   the player's friends.
      if ( k_EFriendRelationshipFriend ==
             friends->GetFriendRelationship (pParam->m_steamIDUser) )
      {
        bool can_unlock = false;

        for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
        {
          char* szName = _strdup (stats->GetAchievementName (i));

          stats->GetUserAchievement (
            pParam->m_steamIDUser,
              szName,
                &friend_stats [pParam->m_steamIDUser.GetAccountID ()][i]
          );

          if (friend_stats [pParam->m_steamIDUser.GetAccountID ()][i]) {
            // On the first unlocked achievement, make a note...
            if (! can_unlock) {
              steam_log.Log ( L"Received Achievement Stats for Friend: '%hs'",
                                friends->GetFriendPersonaName (pParam->m_steamIDUser) );
            }

            can_unlock = true;

            ++achievements.list [i]->friends_.unlocked;

            steam_log.Log (L" >> Has unlocked '%24hs'", szName);
          }

          free ((void *)szName);
        }

        // Second pass over all achievements, incrementing the number of friends who
        //   can potentially unlock them.
        if (can_unlock) {
          for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
            ++achievements.list [i]->friends_.possible;
        }

        int friend_count = friends->GetFriendCount (k_EFriendFlagImmediate);

        if (friend_count > 0 && next_friend < friend_count) {
          SteamAPICall_t hCall =
            stats->RequestUserStats (
              friends->GetFriendByIndex ( next_friend++,
                                            k_EFriendFlagImmediate
                                        )
                                    );

          friend_listener.Set (
            hCall,
              this,
                &SK_Steam_AchievementManager::OnRecvFriendStats );
        }
      }
    }

    // TODO: The function that walks friends should be a lambda
    else {
      // Friend doesn't own the game, so just skip them...
      if (pParam->m_eResult == k_EResultFail) {
        int friend_count = friends->GetFriendCount (k_EFriendFlagImmediate);

        if (friend_count > 0 && next_friend < friend_count) {
          SteamAPICall_t hCall =
            stats->RequestUserStats (
              friends->GetFriendByIndex ( next_friend++,
                                            k_EFriendFlagImmediate
                                        )
                                    );

          friend_listener.Set (
            hCall,
              this,
                &SK_Steam_AchievementManager::OnRecvFriendStats );
        }
      }

      else {
        steam_log.Log ( L" >>> User stat receipt unsuccessful! (m_eResult = 0x%04X)",
                        pParam->m_eResult );
      }
    }
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   OnAsyncComplete,
                   SteamAPICallCompleted_t,
                   async_complete )
  {
    bool failed = true;

    steam_ctx.Utils ()->IsAPICallCompleted (pParam->m_hAsyncCall, &failed);

    if (pParam->m_hAsyncCall == global_request)
    {
      if (! failed) {
        has_global_data = true;

        extern void SK_SteamAPI_UpdateGlobalAchievements (void);
        SK_SteamAPI_UpdateGlobalAchievements ();
      }

      else {
        steam_log.Log ( L" Non-Zero Result for GlobalAchievementPercentagesReady_t"
                           );

        has_global_data = false;
        return;
      }
    }
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   AckStoreStats,
                   UserStatsStored_t,
                   stat_receipt )
  {
    // Sometimes we receive event callbacks for games that aren't this one...
    //   ignore those!
    if (pParam->m_nGameID != SK::SteamAPI::AppID ())
      return;

    steam_log.Log ( L" >> Stats Stored for AppID: %llu",
                      pParam->m_nGameID );
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   OnRecvIcon,
                   UserAchievementIconFetched_t,
                   icon_listener )
  {
    if (pParam->m_nGameID.AppID () != SK::SteamAPI::AppID ()) {
      steam_log.Log ( L" >>> Received achievement icon for unrelated game (AppId=%lu) <<<",
                        pParam->m_nGameID.AppID () );
      return;
    }

    if (pParam->m_nIconHandle == 0)
      return;
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   OnUnlock,
                   UserAchievementStored_t,
                   unlock_listener )
  {
    // Sometimes we receive event callbacks for games that aren't this one...
    //   ignore those!
    if (pParam->m_nGameID != SK::SteamAPI::AppID ()) {
      steam_log.Log ( L" >>> Received achievement unlock for unrelated game (AppId=%lu) <<<",
                        pParam->m_nGameID );
      return;
    }

    Achievement* achievement =
      getAchievement (
        pParam->m_rgchAchievementName
      );

    if (achievement != nullptr)
      achievement->update (steam_ctx.UserStats ());

    int idx =
      steam_ctx.UserStats ()->GetAchievementIcon (pParam->m_rgchAchievementName);

    // If this is != 1.0, the achievement has not been unlocked, the update
    //   is for progress reporting purposes only.
    float progress = 0.0f;

    if ( pParam->m_nMaxProgress == pParam->m_nCurProgress )
    {
            //->m_nMaxProgress == 0 &&
      //pParam->m_nCurProgress == 0) {
      progress = 1.0f;

      if (! config.steam.nosound)
        PlaySound ( (LPCWSTR)unlock_sound, NULL, SND_ASYNC | SND_MEMORY );

      if (achievement != nullptr) {
        steam_log.Log (L" Achievement: '%hs' (%hs) - Unlocked!",
          achievement->human_name_, achievement->desc_);
      }

      if (config.steam.achievement_sshot) {
        SK::SteamAPI::TakeScreenshot ();
      }
    }

    else if (achievement != nullptr)
    {
      progress = 
        (float)pParam->m_nCurProgress /
        (float)pParam->m_nMaxProgress;

      steam_log.Log (L" Achievement: '%hs' (%hs) - "
                     L"Progress %lu / %lu (%04.01f%%)",
                achievement->human_name_,
                achievement->desc_,
                      pParam->m_nCurProgress,
                      pParam->m_nMaxProgress,
                        100.0f * progress );
    }

    else
    {
      steam_log.Log ( L" Got Rogue Achievement Storage: '%hs'",
                        pParam->m_rgchAchievementName );
    }

    if (config.cegui.enable && config.steam.achievements.show_popup && achievement != nullptr) {
      CEGUI::System* pSys = CEGUI::System::getDllSingletonPtr ();

    if ( pSys                 != nullptr &&
         pSys->getRenderer () != nullptr )
    {
      EnterCriticalSection (&popup_cs);

      try {

      CEGUI::WindowManager& window_mgr = CEGUI::WindowManager::getDllSingleton ();
      CEGUI::Window*        achv_popup = window_mgr.loadLayoutFromFile      ("Achievements.layout");

      CEGUI::Window* achv_title = achv_popup->getChild ("Achievement/Title");
      achv_title->setText (achievement->human_name_);

      CEGUI::Window* achv_desc = achv_popup->getChild ("Achievement/Description");
      achv_desc->setText (achievement->desc_);

      CEGUI::Window* achv_rank = achv_popup->getChild ("Achievement/Rank");
      achv_rank->setProperty ( "NormalTextColour",
                                 SK_Steam_RarityToColor (achievement->global_percent_)
                             );
      achv_rank->setText ( SK_Steam_RarityToName (achievement->global_percent_) );

      CEGUI::Window* achv_global = achv_popup->getChild ("Achievement/GlobalRarity");
      char szGlobal [32] = { '\0' };
      snprintf ( szGlobal, 32,
                   "Global: %6.2f%%",
                     achievement->global_percent_ );
      achv_global->setText (szGlobal);

      CEGUI::Window* achv_friend = achv_popup->getChild ("Achievement/FriendRarity");
      char szFriend [32] = { '\0' };
      snprintf ( szFriend, 32,
                   "Friends: %6.2f%%",
          100.0 * ((double)          achievement->friends_.unlocked /
                   (double)std::max (achievement->friends_.possible, 1)) );
      achv_friend->setText (szFriend);


      // If the server hasn't updated the unlock time, just use the curren time
      if (achievement != nullptr && achievement->time_ == 0)
        _time32 (&achievement->time_);


      CEGUI::Window* achv_unlock  = achv_popup->getChild ("Achievement/UnlockTime");
      CEGUI::Window* achv_percent = achv_popup->getChild ("Achievement/ProgressBar");

      char szUnlockTime [128] = { '\0' };
      if (progress == 1.0f) {
        snprintf ( szUnlockTime, 128,
                     "Unlocked: %s", _ctime32 (&achievement->time_)
                 );

        achv_percent->setProperty ( "CurrentProgress", "1.0" );
      }

      else {
        snprintf ( szUnlockTime, 16,
                     "%5.4f",
                       progress
                 );

        achv_percent->setProperty ( "CurrentProgress", szUnlockTime );

        snprintf ( szUnlockTime, 128,
                     "Current Progress: %lu/%lu (%6.2f%%)",
                     pParam->m_nCurProgress, pParam->m_nMaxProgress,
                     100.0f * progress
                 );
      }
      achv_unlock->setText (szUnlockTime);


      // Icon width and height
      uint32 w, h;

      if (idx != 0) {
        steam_ctx.Utils ()->GetImageSize (idx, &w, &h);

        if (achievement->icons_.achieved == nullptr) {
          achievement->icons_.achieved =
            (uint8_t *)malloc (4 * w * h);

          steam_ctx.Utils ()->GetImageRGBA (
            idx,
              achievement->icons_.achieved,
                4 * w * h
          );
        }
      }

      steam_log.Log ( L" * Fetched RGBA Icon (idx=%lu) for Achievement: '%hs'  (%lux%lu)",
                        idx, achievement->name_, w, h );

      if (achievement->icons_.achieved != nullptr) {
        bool exists = CEGUI::ImageManager::getDllSingleton ().isDefined (achievement->name_);

        const CEGUI::Image& img =
           exists ?
            CEGUI::ImageManager::getDllSingleton ().get    (              achievement->name_) :
            CEGUI::ImageManager::getDllSingleton ().create ("BasicImage", achievement->name_);

        if (! exists) {
          /* StaticImage */
          CEGUI::Texture& Tex =
            pSys->getRenderer ()->createTexture (achievement->name_);

          Tex.loadFromMemory ( achievement->icons_.achieved,
                                 CEGUI::Sizef ((float)w, (float)h),
                                   CEGUI::Texture::PF_RGBA );

          ((CEGUI::BasicImage &)img).setTexture (&Tex);

          const CEGUI::Rectf rect (CEGUI::Vector2f (0.0f, 0.0f), Tex.getOriginalDataSize ());
          ((CEGUI::BasicImage &)img).setArea       (rect);
          ((CEGUI::BasicImage &)img).setAutoScaled (CEGUI::ASM_Both);
        }

        CEGUI::Window* staticImage = achv_popup->getChild ("Achievement/Icon");
        staticImage->setProperty ( "Image",
                                     achievement->name_ );
      }

      SK_AchievementPopup popup;

      popup.window      = achv_popup;
      popup.time        = timeGetTime ();
      popup.achievement = achievement;

      pSys->getDefaultGUIContext ().setRootWindow (popup.window);

      char    szAppName [256] = { '\0' };
      bool    avail           = false;
      AppId_t app             = 0;

      bool has_app =
        steam_ctx.Apps ()->BGetDLCDataByIndex
        (
          0, &app,
            &avail,
              szAppName, 255
        );

      popup.window->getChild ("Achievement")->setText (
        has_app ? szAppName :
                  ""
      );
      popup.window->show    ();

      popups.push_back (popup);

      } catch (...) {
      }

      LeaveCriticalSection (&popup_cs);
    }
    }
  }

  void DrawPopups (void)
  {
    if (! popups.size ())
      return;

    EnterCriticalSection (&popup_cs);

    try {
      #define POPUP_DURATION_MS config.steam.achievements.popup_duration

      std::vector <SK_AchievementPopup>::iterator it = popups.begin ();

      while (it != popups.end ()) {
        if (timeGetTime () < (*it).time + POPUP_DURATION_MS) {
          (*it++).window->show ();
        } else {
          (*it).window->hide ();

          it = popups.erase (it);
        }
      }
    } catch (...) {}

    LeaveCriticalSection (&popup_cs);
  }

  Achievement* getAchievement (const char* szName)
  {
    if (achievements.string_map.count (szName))
      return achievements.string_map [szName];

    return nullptr;
  }

  void requestStats (void) {
    if (steam_ctx.UserStats ()->RequestCurrentStats ()) {
      SteamAPICall_t hCall =
        steam_ctx.UserStats ()->RequestUserStats (
          steam_ctx.User ()->GetSteamID ()
        );

      self_listener.Set (
        hCall,
          this,
            &SK_Steam_AchievementManager::OnRecvSelfStats );
    }
  }

  void pullStats (void)
  {
    ISteamUserStats* stats = steam_ctx.UserStats ();

    for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
    {
      //steam_log.Log (L"  >> %hs", stats->GetAchievementName (i));

      if (achievements.list [i] == nullptr) {
        char* szName = _strdup (stats->GetAchievementName (i));

        achievements.list [i] =
          new Achievement (
                szName,
                stats
              );

        achievements.string_map [achievements.list [i]->name_] =
          achievements.list [i];

        achievements.list [i]->update (stats);

        //steam_log.Log (L"  >> (%lu) %hs", i, achievements.list [i]->name_);

        // Start pre-loading images so we do not hitch on achievement unlock...

        if (! achievements.list [i]->unlocked_)
          stats->SetAchievement   (szName);

        // After setting the achievement, fetch the icon -- this is
        //   necessary so that the unlock dialog does not display
        //     the locked icon.
        stats->GetAchievementIcon (szName);

        if (! achievements.list [i]->unlocked_)
          stats->ClearAchievement (szName);

        free ((void *)szName);
      } else {
        achievements.list [i]->update (stats);
      }
    }
  }

protected:
private:
  struct SK_AchievementData {
    std::vector        <             Achievement*> list;
    std::unordered_map <std::string, Achievement*> string_map;
  } achievements; // SELF

  std::unordered_map <
    uint32_t,
      std::unordered_map < uint32_t, bool >
  > friend_stats;

  struct SK_AchievementPopup {
    CEGUI::Window* window;
    DWORD          time;
    Achievement*   achievement;
  };

  std::vector <SK_AchievementPopup> popups;

  SteamAPICall_t global_request;
  SteamAPICall_t stats_request;

  bool           default_loaded;
  uint8_t*       unlock_sound;   // A .WAV (PCM) file
} *steam_achievements = nullptr;

#if 0
S_API typedef void (S_CALLTYPE *steam_unregister_callback_t)
                       (class CCallbackBase *pCallback);
S_API typedef void (S_CALLTYPE *steam_register_callback_t)
                       (class CCallbackBase *pCallback, int iCallback);

steam_register_callback_t SteamAPI_RegisterCallbackOrig = nullptr;


S_API bool S_CALLTYPE SteamAPI_Init_Detour (void);
#endif

void
SK_SteamAPI_LogAllAchievements (void)
{
  if (steam_achievements != nullptr) {
    steam_achievements->log_all_achievements ();
  }
}

void
SK_UnlockSteamAchievement (uint32_t idx)
{
  if (config.steam.silent)
    return;

  if (! steam_achievements) {
    steam_log.Log ( L" >> Steam Achievement Manager Not Yet Initialized ... "
                    L"Skipping Unlock <<" );
    return;
  }

  steam_log.LogEx (true, L" >> Attempting to Unlock Achievement: %i... ",
    idx );

  ISteamUserStats* stats = steam_ctx.UserStats ();

  if ( stats                        && idx <
       stats->GetNumAchievements () )
  {
    // I am dubious about querying these things by name, so duplicate this
    //   string immediately.
    const char* szName = _strdup (stats->GetAchievementName (idx));

    if (szName != nullptr) {
      steam_log.LogEx (false, L" (%hs - Found)\n", szName);

      steam_achievements->pullStats ();

      UserAchievementStored_t store;
      store.m_nCurProgress = 0;
      store.m_nMaxProgress = 0;
      store.m_nGameID = SK::SteamAPI::AppID ();
      strncpy (store.m_rgchAchievementName, szName, 128);

      SK_Steam_AchievementManager::Achievement* achievement =
        steam_achievements->getAchievement (
          store.m_rgchAchievementName
        );

      steam_achievements->OnUnlock (&store);

//      stats->ClearAchievement            (szName);
//      stats->IndicateAchievementProgress (szName, 0, 1);
//      stats->StoreStats                  ();
#if 0
      bool achieved;
      if (stats->GetAchievement (szName, &achieved)) {
        if (achieved) {
          steam_log.LogEx (true, L"Clearing first\n");
          stats->ClearAchievement            (szName);
          stats->StoreStats                  ();

          SteamAPI_RunCallbacks              ();
        } else {
          steam_log.LogEx (true, L"Truly unlocking\n");
          stats->SetAchievement              (szName);

          stats->StoreStats                  ();

          // Dispatch these ASAP, there's a lot of latency apparently...
          SteamAPI_RunCallbacks ();
        }
      }
      else {
        steam_log.LogEx (true, L" >> GetAchievement (...) Failed\n");
      }
#endif
      free ((void *)szName);
    }
    else {
      steam_log.LogEx (false, L" (None Found)\n");
    }
  } else {
    steam_log.LogEx (false, L" (ISteamUserStats is NULL?!)\n");
  }

  SK_SteamAPI_LogAllAchievements ();
}

void
SK_SteamAPI_UpdateGlobalAchievements (void)
{
  ISteamUserStats* stats =
    steam_ctx.UserStats ();

  if (stats != nullptr) {
    const int num_achievements = stats->GetNumAchievements ();

    for (int i = 0; i < num_achievements; i++) {
      const char* szName = _strdup (stats->GetAchievementName (i));

      SK_Steam_AchievementManager::Achievement* achievement =
        steam_achievements->getAchievement (szName);

      if (achievement != nullptr) {
        achievement->update_global (stats);
      } else {
        dll_log.Log ( L" Got Global Data For Unknown Achievement ('%hs')",
                        szName );
      }

      free ((void *)szName);
    }
  } else {
    steam_log.Log ( L"Got Global Stats from SteamAuPI, but no "
                    L"ISteamUserStats interface exists?!" );
  }

  SK_SteamAPI_LogAllAchievements ();
}

volatile LONGLONG SK_SteamAPI_CallbackRunCount = 0LL;

SteamAPI_RunCallbacks_t SteamAPI_RunCallbacks_Original = nullptr;

void
S_CALLTYPE
SteamAPI_RunCallbacks_Detour (void)
{
  // Throttle to 1000 Hz for STUPID games like Akiba's Trip
  static DWORD dwLastTick = 0;
         DWORD dwNow      = timeGetTime ();

  if (dwLastTick < dwNow) {
    dwLastTick = dwNow;
  } else {
    // Skip the callbacks, one call every 1 ms is enough!!
    return;
  }

  InterlockedIncrement64 (&SK_SteamAPI_CallbackRunCount);

  void SK_SteamAPI_InitManagers (void);
  SK_SteamAPI_InitManagers ();

  if (steam_achievements != nullptr)
    steam_achievements->DrawPopups ();

  SteamAPI_RunCallbacks_Original ();
}

#define STEAMAPI_CALL1(x,y,z) ((x) = SteamAPI_##y z)
#define STEAMAPI_CALL0(y,z)   (SteamAPI_##y z)

#include <Windows.h>

extern "C"
void
__cdecl
SteamAPIDebugTextHook (int nSeverity, const char *pchDebugText)
{
  steam_log.Log (" [SteamAPI] Severity: %d - '%hs'",
    nSeverity, pchDebugText);
}

#include <ctime>

void
SK::SteamAPI::Init (bool pre_load)
{
  if (config.steam.silent)
    return;
}

void
SK::SteamAPI::Shutdown (void)
{
  SK_AutoClose_Log (steam_log);

  steam_ctx.Shutdown ();
}

void SK::SteamAPI::Pump (void)
{
  if (steam_ctx.UserStats ()) {
    if (SteamAPI_RunCallbacks != nullptr)
      SteamAPI_RunCallbacks ();
  }
}

DWORD
WINAPI
SteamAPI_PumpThread (LPVOID user)
{
  // Wait 5 seconds, then begin a timing investigation
  Sleep (5000);

  // First, begin a timing probe.
  //
  //   If after 15 seconds the game has not called SteamAPI_RunCallbacks
  //     frequently enough, switch the thread to auto-pump mode.

  LONGLONG callback_count0 = SK_SteamAPI_CallbackRunCount;

  const UINT TEST_PERIOD = 15;

  Sleep (TEST_PERIOD * 1000UL);

  LONGLONG callback_count1 = SK_SteamAPI_CallbackRunCount;

  double callback_freq = (double)(callback_count1 - callback_count0) / (double)TEST_PERIOD;

  steam_log.Log ( L"Game runs its callbacks at ~%5.2f Hz ... this is %s the "
                  L"minimum threshold for achievement unlock sound.\n",
                    callback_freq, callback_freq > 3.0 ? L"above" : L"below" );

  // If the timing period failed, then start auto-pumping.
  //
  if (callback_freq < 3.0) {
    steam_log.Log ( L" >> Installing a callback auto-pump at 4 Hz.\n\n");

    while (true) {
      Sleep (250);

      SK::SteamAPI::Pump ();
    }
  }

  CloseHandle (GetCurrentThread ());

  return 0;
}

void
StartSteamPump (void)
{
  static bool pump_started = false;

  if (pump_started)
    return;

  if (config.steam.auto_pump_callbacks)
    CreateThread (nullptr, 0, SteamAPI_PumpThread, nullptr, 0x00, nullptr);

  pump_started = true;
}


uint32_t
SK::SteamAPI::AppID (void)
{
  ISteamUtils* utils = steam_ctx.Utils ();

  if (utils != nullptr) {
    static uint32_t id = utils->GetAppID ();

    // If no AppID was manually set, let's assign one now.
    if (config.steam.appid == 0)
      config.steam.appid = id;

    return id;
  }

  return 0;
}

void
__stdcall
SK::SteamAPI::SetOverlayState (bool active)
{
  if (config.steam.silent)
    return;

  EnterCriticalSection (&callback_cs);

  GameOverlayActivated_t state;
  state.m_bActive = active;

  std::set <CCallbackBase *>::iterator it =
    overlay_activation_callbacks.begin ();

  __try {
    while (it != overlay_activation_callbacks.end ()) {
      (*it++)->Run (&state);
    }
  } __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ? 
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {
  }

  LeaveCriticalSection (&callback_cs);
}

bool
__stdcall
SK::SteamAPI::TakeScreenshot (void)
{
  ISteamScreenshots* pScreenshots =
    steam_ctx.Screenshots ();

  if (pScreenshots != nullptr) {
    steam_log.LogEx (true, L"  >> Triggering Screenshot: ");
    pScreenshots->TriggerScreenshot ();
    steam_log.LogEx (false, L"done!\n");
    return true;
  }

  return false;
}




bool
S_CALLTYPE
SteamAPI_InitSafe_Detour (void)
{
  EnterCriticalSection (&init_cs);

  // In case we already initialized stuff...
  if (InterlockedCompareExchange (&__SK_Steam_init, FALSE, FALSE)) {
    LeaveCriticalSection (&init_cs);
    return true;
  }

  static int init_tries = -1;

  if (++init_tries == 0) {
    steam_log.Log ( L"Initializing SteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"----------(InitSafe)-----------\n");
  }

  // TODO: Scoped UnHook / ReHook
  SK_DisableHook (SteamAPI_RegisterCallback);

  if (SteamAPI_InitSafe_Original ()) {
    InterlockedExchange (&__SK_Steam_init, TRUE);

#ifdef _WIN64
  const wchar_t* steam_dll_str    = L"steam_api64.dll";
#else
  const wchar_t* steam_dll_str = L"steam_api.dll";
#endif

    HMODULE hSteamAPI;
    GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, steam_dll_str, &hSteamAPI);

    if (! steam_ctx.UserStats ())
      steam_ctx.InitSteamAPI (hSteamAPI);

    steam_log.Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                      init_tries + 1,
                        SK::SteamAPI::AppID () );

    StartSteamPump ();

    SK_EnableHook (SteamAPI_RegisterCallback);

    LeaveCriticalSection (&init_cs);
    return true;
  }

  SK_EnableHook (SteamAPI_RegisterCallback);

  LeaveCriticalSection (&init_cs);
  return false;
}

void
S_CALLTYPE
SteamAPI_Shutdown_Detour (void)
{
  steam_log.Log (L" *** Game called SteamAPI_Shutdown (...)");

  //steam_ctx.Shutdown         ();
  //SteamAPI_Shutdown_Original ();

  InterlockedExchange (&__SK_Steam_init, 0);

  CreateThread ( nullptr, 0,
       [](LPVOID user) ->
  DWORD {
    Sleep (1000UL);

    if (SteamAPI_InitSafe ()) {
      StartSteamPump ();
    }

    CloseHandle (GetCurrentThread ());
    return 0;
  }, nullptr, 0x00, nullptr );
}

bool
S_CALLTYPE
SteamAPI_Init_Detour (void)
{
  EnterCriticalSection (&init_cs);

  // In case we already initialized stuff...
  if (InterlockedCompareExchange (&__SK_Steam_init, FALSE, FALSE)) {
    LeaveCriticalSection (&init_cs);
    return true;
  }

  static int init_tries = -1;

  if (++init_tries == 0) {
    steam_log.Log ( L"Initializing SteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"-------------------------------\n");
  }

  SK_DisableHook (SteamAPI_RegisterCallback);

  if (SteamAPI_Init_Original ()) {
    InterlockedExchange (&__SK_Steam_init, TRUE);

#ifdef _WIN64
  const wchar_t* steam_dll_str    = L"steam_api64.dll";
#else
  const wchar_t* steam_dll_str = L"steam_api.dll";
#endif

    HMODULE hSteamAPI;
    GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, steam_dll_str, &hSteamAPI);

    steam_ctx.InitSteamAPI (hSteamAPI);

    steam_log.Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                      init_tries + 1,
                        SK::SteamAPI::AppID () );

    StartSteamPump ();

    SK_EnableHook (SteamAPI_RegisterCallback);

    LeaveCriticalSection (&init_cs);
    return true;
  }

  SK_EnableHook (SteamAPI_RegisterCallback);

  LeaveCriticalSection (&init_cs);
  return false;
}


void
__stdcall
SK_SteamAPI_SetOverlayState (bool active)
{
  SK::SteamAPI::SetOverlayState (active);
}

bool
__stdcall
SK_SteamAPI_TakeScreenshot (void)
{
  return SK::SteamAPI::TakeScreenshot ();
}


typedef bool (S_CALLTYPE *InitSafe_pfn)(void);
InitSafe_pfn InitSafe_Original = nullptr;

bool
S_CALLTYPE
InitSafe_Detour (void)
{
  EnterCriticalSection (&init_cs);

  // In case we already initialized stuff...
  if (InterlockedCompareExchange (&__SK_Steam_init, FALSE, FALSE)) {
    LeaveCriticalSection (&init_cs);
    return true;
  }

  static int init_tries = -1;

  if (++init_tries == 0) {
    steam_log.Log ( L"Initializing CSteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"-----------(InitSafe)-----------\n");
  }

  SK_DisableHook (SteamAPI_RegisterCallback);

  if (InitSafe_Original ()) {
    InterlockedExchange (&__SK_Steam_init, TRUE);

    HMODULE hSteamAPI;
    GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN, L"CSteamworks.dll", &hSteamAPI);

    if (! steam_ctx.UserStats ())
      steam_ctx.InitCSteamworks (hSteamAPI);

    steam_log.Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                      init_tries + 1,
                        SK::SteamAPI::AppID () );

    StartSteamPump ();

    SK_EnableHook (SteamAPI_RegisterCallback);

    LeaveCriticalSection (&init_cs);
    return true;
  }

  SK_EnableHook (SteamAPI_RegisterCallback);

  LeaveCriticalSection (&init_cs);
  return false;
}

DWORD
WINAPI
CSteamworks_Delay_Init (LPVOID user)
{
  int tries = 0;

  while ( (! InterlockedExchangeAddAcquire (&__SK_Steam_init, 0)) &&
            tries < 120 )
  {
    Sleep (config.steam.init_delay);

    if (InterlockedExchangeAddRelease (&__SK_Steam_init, 0))
      break;

    if (InitSafe_Detour != nullptr)
      InitSafe_Detour ();

    ++tries;
  }

  CloseHandle (GetCurrentThread ());

  return 0;
}

volatile ULONG __hooked           = FALSE;
volatile ULONG __CSteamworks_hook = FALSE;

void
SK_HookCSteamworks (void)
{
  if (! InterlockedCompareExchange (&__hooked, TRUE, FALSE)) {
    steam_log.init (L"logs/steam_api.log", L"w");
    steam_log.silent = config.steam.silent;
    InitializeCriticalSectionAndSpinCount (&callback_cs, 1024UL);
    InitializeCriticalSectionAndSpinCount (&popup_cs,    16384UL);
    InitializeCriticalSectionAndSpinCount (&init_cs,     1048576UL);
  }

  EnterCriticalSection (&init_cs);
  if (InterlockedCompareExchange (&__CSteamworks_hook, TRUE, FALSE)) {
    LeaveCriticalSection (&init_cs);
    return;
  }

  steam_log.Log (L"CSteamworks.dll was loaded, hooking...");

  SK_CreateDLLHook ( L"CSteamworks.dll",
                      "InitSafe",
                      InitSafe_Detour,
           (LPVOID *)&InitSafe_Original );

#if 0
  SK_CreateDLLHook ( L"CSteamworks.dll", "RegisterCallback",
                     SteamAPI_RegisterCallback_Detour,
                   (LPVOID *)&SteamAPI_RegisterCallback_Original,
                   (LPVOID *)&SteamAPI_RegisterCallback);
  SK_EnableHook (SteamAPI_RegisterCallback);

  SK_CreateDLLHook (L"CSteamworks.dll", "UnregisterCallback",
                     SteamAPI_UnregisterCallback_Detour,
                     (LPVOID *)&SteamAPI_UnregisterCallback_Original,
                     (LPVOID *)&SteamAPI_UnregisterCallback);
  SK_EnableHook (SteamAPI_UnregisterCallback);
#endif

  if (config.steam.init_delay > 0)
    CreateThread (nullptr, 0, CSteamworks_Delay_Init, nullptr, 0x00, nullptr);

  LeaveCriticalSection (&init_cs);
}

DWORD
WINAPI
SteamAPI_Delay_Init (LPVOID user)
{
  int tries = 0;

  while ( (! InterlockedExchangeAddAcquire (&__SK_Steam_init, 0)) &&
            tries < 120 )
  {
    Sleep (config.steam.init_delay);

    if (InterlockedExchangeAddRelease (&__SK_Steam_init, 0))
      break;

    if (SteamAPI_InitSafe_Detour != nullptr)
      SteamAPI_InitSafe_Detour ();

    ++tries;
  }

  CloseHandle (GetCurrentThread ());

  return 0;
}

volatile ULONG __SteamAPI_hook = FALSE;

void
SK_HookSteamAPI (void)
{
  if (! InterlockedCompareExchange (&__hooked, TRUE, FALSE)) {
    steam_log.init (L"logs/steam_api.log", L"w");
    steam_log.silent = config.steam.silent;
    InitializeCriticalSectionAndSpinCount (&callback_cs, 1024UL);
    InitializeCriticalSectionAndSpinCount (&popup_cs,    16384UL);
    InitializeCriticalSectionAndSpinCount (&init_cs,     1048576UL);
  }

  EnterCriticalSection (&init_cs);
  if (InterlockedCompareExchange (&__SteamAPI_hook, TRUE, FALSE)) {
    LeaveCriticalSection (&init_cs);
    return;
  }

#ifdef _WIN64
    const wchar_t* wszSteamAPI = L"steam_api64.dll";
#else
    const wchar_t* wszSteamAPI = L"steam_api.dll";
#endif

  steam_log.Log (L"%s was loaded, hooking...", wszSteamAPI);

  SK_CreateDLLHook2 ( wszSteamAPI,
                     "SteamAPI_InitSafe",
                     SteamAPI_InitSafe_Detour,
          (LPVOID *)&SteamAPI_InitSafe_Original );

  SK_CreateDLLHook2 ( wszSteamAPI,
                     "SteamAPI_Init",
                     SteamAPI_Init_Detour,
          (LPVOID *)&SteamAPI_Init_Original );

  SK_CreateDLLHook ( wszSteamAPI,
                      "SteamAPI_Shutdown",
                      SteamAPI_Shutdown_Detour,
           (LPVOID *)&SteamAPI_Shutdown_Original );

  SK_CreateDLLHook2 ( wszSteamAPI,
                     "SteamAPI_RegisterCallback",
                     SteamAPI_RegisterCallback_Detour,
          (LPVOID *)&SteamAPI_RegisterCallback_Original,
          (LPVOID *)&SteamAPI_RegisterCallback );

  SK_CreateDLLHook2 ( wszSteamAPI,
                     "SteamAPI_UnregisterCallback",
                     SteamAPI_UnregisterCallback_Detour,
          (LPVOID *)&SteamAPI_UnregisterCallback_Original,
          (LPVOID *)&SteamAPI_UnregisterCallback );

  SK_CreateDLLHook2 ( wszSteamAPI,
                     "SteamAPI_RunCallbacks",
                     SteamAPI_RunCallbacks_Detour,
          (LPVOID *)&SteamAPI_RunCallbacks_Original,
          (LPVOID *)&SteamAPI_RunCallbacks );

  SK_ApplyQueuedHooks ();

  if (config.steam.init_delay > 0)
    CreateThread (nullptr, 0, SteamAPI_Delay_Init, nullptr, 0x00, nullptr);

  LeaveCriticalSection (&init_cs);
}

ISteamUserStats*
SK_SteamAPI_UserStats (void)
{
  return steam_ctx.UserStats ();
}

void
SK_SteamAPI_ContextInit (HMODULE hSteamAPI)
{
  if (! steam_ctx.UserStats ())
    steam_ctx.InitSteamAPI (hSteamAPI);
}

//
// Not thread safe
//
void
SK_SteamAPI_InitManagers (void)
{
  static bool init = false;

  if (! init)
    init = true;
  else
    return;

  ISteamUserStats* stats =
    steam_ctx.UserStats ();

  steam_log.Log (L" Creating Achievement Manager...");

  steam_achievements = new SK_Steam_AchievementManager (
      config.steam.achievement_sound.c_str ()
    );
  overlay_manager    = new SK_Steam_OverlayManager ();

  steam_log.LogEx (false, L"\n");

  steam_achievements->requestStats ();

  SteamAPICall_t call =
    stats->RequestGlobalAchievementPercentages ();
}