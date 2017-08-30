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

#define NOMINMAX
#define PSAPI_VERSION           1

#include <Windows.h>
#include <process.h>
#include <Shlwapi.h>

#include <psapi.h>
#pragma comment (lib, "psapi.lib")

#include <SpecialK/resource.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>

#include <SpecialK/config.h>
#include <SpecialK/ini.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>
#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/compatibility.h>

#include <SpecialK/osd/popup.h>
#include <SpecialK/osd/text.h>
#include <SpecialK/render_backend.h>

#include <memory>
#include <atlbase.h>

// We're not going to use DLL Import - we will load these function pointers
//  by hand.
#define STEAM_API_NODLL
#include <SpecialK/steam_api.h>

// PlaySound
#pragma comment (lib, "winmm.lib")

#pragma warning (disable: 4706)

         iSK_Logger       steam_log;

volatile ULONG            __SK_Steam_init = FALSE;
volatile ULONG            __SteamAPI_hook = FALSE;

         CRITICAL_SECTION callback_cs     = { };
         CRITICAL_SECTION init_cs         = { };
         CRITICAL_SECTION popup_cs        = { };

#include <time.h>

// To spoof Overlay Activation (pause the game)
#include <set>
#include <unordered_map>

std::multiset <class CCallbackBase *> overlay_activation_callbacks;

void SK_HookSteamAPI                      (void);
void SK_SteamAPI_UpdateGlobalAchievements (void);

#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>

class SK_Steam_OverlayManager
{
public:
  SK_Steam_OverlayManager (void) :
       activation ( this, &SK_Steam_OverlayManager::OnActivate      )
  {
    cursor_visible_ = false;
    active_         = false;
  }

  ~SK_Steam_OverlayManager (void)
  {
    cursor_visible_ = false;
    active_         = false;
  }

  STEAM_CALLBACK ( SK_Steam_OverlayManager,
                   OnActivate,
                   GameOverlayActivated_t,
                   activation )
  {
#if 0
    if (active_ != (pParam->m_bActive != 0))
    {
      if (pParam->m_bActive)
      {
        static bool first = true;

        if (first)
        {
          cursor_visible_ = ShowCursor (FALSE) >= -1;
                            ShowCursor (TRUE);
          first = false;
        }
      }

      // Restore the original cursor state.
      else
      {
        if (! cursor_visible_)
        {
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
#endif

    active_ = (pParam->m_bActive != 0);
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


#include <SpecialK/utility.h>

ISteamUserStats* SK_SteamAPI_UserStats   (void);
void             SK_SteamAPI_ContextInit (HMODULE hSteamAPI);


#define STEAM_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {   \
  void** _vftable = *(void***)*(_Base);                                      \
                                                                             \
  if ((_Original) == nullptr) {                                              \
    SK_CreateVFTableHook ( L##_Name,                                         \
                             _vftable,                                       \
                               (_Index),                                     \
                                 (_Override),                                \
                                   (LPVOID *)&(_Original));                  \
  }                                                                          \
}

using callback_func_t      =
  void (__fastcall *)( CCallbackBase*, LPVOID );
using callback_func_fail_t =
  void (__fastcall *)( CCallbackBase*, LPVOID, bool bIOFailure, SteamAPICall_t hSteamAPICall );

callback_func_t SteamAPI_UserStatsReceived_Original = nullptr;

extern "C" void __fastcall SteamAPI_UserStatsReceived_Detour       (CCallbackBase* This, UserStatsReceived_t* pParam);
//extern "C" void __cdecl SteamAPI_UserStatsReceivedIOFail_Detour (CCallbackBase* This, UserStatsReceived_t* pParam, bool bIOFailure, SteamAPICall_t hSteamAPICall);


const char*
__stdcall
SK_Steam_ClassifyCallback (int iCallback)
{
  switch ((iCallback / 100) * 100)
  {
    case k_iSteamUserCallbacks:                  return "k_iSteamUserCallbacks";
    case k_iSteamGameServerCallbacks:            return "k_iSteamGameServerCallbacks";
    case k_iSteamFriendsCallbacks:               return "k_iSteamFriendsCallbacks";
    case k_iSteamBillingCallbacks:               return "k_iSteamBillingCallbacks";
    case k_iSteamMatchmakingCallbacks:           return "k_iSteamMatchmakingCallbacks";
    case k_iSteamContentServerCallbacks:         return "k_iSteamContentServerCallbacks";
    case k_iSteamUtilsCallbacks:                 return "k_iSteamUtilsCallbacks";
    case k_iClientFriendsCallbacks:              return "k_iClientFriendsCallbacks";
    case k_iClientUserCallbacks:                 return "k_iClientUserCallbacks";
    case k_iSteamAppsCallbacks:                  return "k_iSteamAppsCallbacks";
    case k_iSteamUserStatsCallbacks:             return "k_iSteamUserStatsCallbacks";
    case k_iSteamNetworkingCallbacks:            return "k_iSteamNetworkingCallbacks";
    case k_iClientRemoteStorageCallbacks:        return "k_iClientRemoteStorageCallbacks";
    case k_iClientDepotBuilderCallbacks:         return "k_iClientDepotBuilderCallbacks";
    case k_iSteamGameServerItemsCallbacks:       return "k_iSteamGameServerItemsCallbacks";
    case k_iClientUtilsCallbacks:                return "k_iClientUtilsCallbacks";
    case k_iSteamGameCoordinatorCallbacks:       return "k_iSteamGameCoordinatorCallbacks";
    case k_iSteamGameServerStatsCallbacks:       return "k_iSteamGameServerStatsCallbacks";
    case k_iSteam2AsyncCallbacks:                return "k_iSteam2AsyncCallbacks";
    case k_iSteamGameStatsCallbacks:             return "k_iSteamGameStatsCallbacks";
    case k_iClientHTTPCallbacks:                 return "k_iClientHTTPCallbacks";
    case k_iClientScreenshotsCallbacks:          return "k_iClientScreenshotsCallbacks";
    case k_iSteamScreenshotsCallbacks:           return "k_iSteamScreenshotsCallbacks";
    case k_iClientAudioCallbacks:                return "k_iClientAudioCallbacks";
    case k_iClientUnifiedMessagesCallbacks:      return "k_iClientUnifiedMessagesCallbacks";
    case k_iSteamStreamLauncherCallbacks:        return "k_iSteamStreamLauncherCallbacks";
    case k_iClientControllerCallbacks:           return "k_iClientControllerCallbacks";
    case k_iSteamControllerCallbacks:            return "k_iSteamControllerCallbacks";
    case k_iClientParentalSettingsCallbacks:     return "k_iClientParentalSettingsCallbacks";
    case k_iClientDeviceAuthCallbacks:           return "k_iClientDeviceAuthCallbacks";
    case k_iClientNetworkDeviceManagerCallbacks: return "k_iClientNetworkDeviceManagerCallbacks";
    case k_iClientMusicCallbacks:                return "k_iClientMusicCallbacks";
    case k_iClientRemoteClientManagerCallbacks:  return "k_iClientRemoteClientManagerCallbacks";
    case k_iClientUGCCallbacks:                  return "k_iClientUGCCallbacks";
    case k_iSteamStreamClientCallbacks:          return "k_iSteamStreamClientCallbacks";
    case k_IClientProductBuilderCallbacks:       return "k_IClientProductBuilderCallbacks";
    case k_iClientShortcutsCallbacks:            return "k_iClientShortcutsCallbacks";
    case k_iClientRemoteControlManagerCallbacks: return "k_iClientRemoteControlManagerCallbacks";
    case k_iSteamAppListCallbacks:               return "k_iSteamAppListCallbacks";
    case k_iSteamMusicCallbacks:                 return "k_iSteamMusicCallbacks";
    case k_iSteamMusicRemoteCallbacks:           return "k_iSteamMusicRemoteCallbacks";
    case k_iClientVRCallbacks:                   return "k_iClientVRCallbacks";
    case k_iClientReservedCallbacks:             return "k_iClientReservedCallbacks";
    case k_iSteamReservedCallbacks:              return "k_iSteamReservedCallbacks";
    case k_iSteamHTMLSurfaceCallbacks:           return "k_iSteamHTMLSurfaceCallbacks";
    case k_iClientVideoCallbacks:                return "k_iClientVideoCallbacks";
    case k_iClientInventoryCallbacks:            return "k_iClientInventoryCallbacks";
    default:                                     return "UNKNOWN";
  }
}

S_API
void
S_CALLTYPE
SteamAPI_RegisterCallback_Detour (class CCallbackBase *pCallback, int iCallback)
{
  // Don't care about OUR OWN callbacks ;)
  if (SK_GetCallingDLL () == SK_GetDLL ())
  {
    SteamAPI_RegisterCallback_Original (pCallback, iCallback);
    return;
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

      if (config.steam.block_stat_callback)
      {
        steam_log.Log (L" ### Callback Blacklisted ###");
        LeaveCriticalSection (&callback_cs);
        return;
      }

      //
      // Many games shutdown SteamAPI on an async operation I/O failure;
      //   reading friend achievement stats will fail-fast thanks to Valve
      //     not implementing a query for friends who have a specific AppID
      //       in their library.
      //
      //   Most friends will have no achievements for the current game,
      //     so hook the game's callback procedure and filter out failure
      //       events that we generated much to the game's surprise :)
      //
      else if (config.steam.filter_stat_callback)
      {
#ifdef _WIN64
        void** vftable = *reinterpret_cast <void ***> (&pCallback);

        SK_CreateFuncHook (      L"Callback Redirect",
                                   vftable [3],
                                   SteamAPI_UserStatsReceived_Detour, 
          static_cast_p2p <void> (&SteamAPI_UserStatsReceived_Original) );
        SK_EnableHook (            vftable [3]);

        steam_log.Log ( L" ### Callback Redirected (APPLYING FILTER:  Remvove "
                             L"Third-Party CSteamID / AppID Achievements) ###" );
#else
        /*
        SK_CreateFuncHook ( L"Callback Redirect",
                              vftable [4],
                              SteamAPI_UserStatsReceived_Detour, 
     static_cast_p2p <void> (&SteamAPI_UserStatsReceived_Original) );
        SK_EnableHook (vftable [4]);

        steam_log.Log ( L" ### Callback Redirected (APPLYING FILTER:  Remvove "
                             L"Third-Party CSteamID / AppID Achievements) ###" );
        */
#endif
      }
      break;
    case UserStatsStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Stats Storage Callback",
                        caller.c_str () );
      break;
    case UserAchievementStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Achievements Storage Callback",
                        caller.c_str () );
      break;
    case UserStatsUnloaded_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed User Stats Unloaded Callback",
                        caller.c_str () );
      break;
    case SteamShutdown_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed SteamAPI Shutdown Callback",
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
    case SteamServersConnected_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Steam Server Connected Callback",
                        caller.c_str () );
      break;
    case SteamServerConnectFailure_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Steam Server Connection Failure Callback",
                        caller.c_str () );
      break;
    case SteamServersDisconnected_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Steam Server Disconnect Callback",
                        caller.c_str () );
      break;
    case LobbyEnter_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Lobby Enter Callback",
                        caller.c_str () );
      break;
    case LobbyDataUpdate_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Lobby Data Update Callback",
                        caller.c_str () );
      break;
    case LobbyChatUpdate_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Lobby Chat Update Callback",
                        caller.c_str () );
      break;
    case GameLobbyJoinRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Game Lobby Join Request Callback",
                        caller.c_str () );
      break;
    case GameRichPresenceJoinRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Game Rich Presence Join Request Callback",
                        caller.c_str () );
      break;
    case P2PSessionConnectFail_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Peer to Peer Session Connect Failure Callback",
                        caller.c_str () );
      break;
    case P2PSessionRequest_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Installed Peer to Peer Session Request Callback",
                        caller.c_str () );
      break;
    default:
    {
      steam_log.Log ( L" * (%-28s) Installed Unknown Callback (Class=%hs, Id=%li)",
                        caller.c_str (), SK_Steam_ClassifyCallback (iCallback), iCallback % 100 );
    } break;
  }

  SteamAPI_RegisterCallback_Original (pCallback, iCallback);

  LeaveCriticalSection (&callback_cs);
}

void
SK_SteamAPI_EraseActivationCallback (class CCallbackBase *pCallback)
{
  __try
  {
    overlay_activation_callbacks.erase (pCallback);
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }
}

S_API
void
S_CALLTYPE
SteamAPI_UnregisterCallback_Detour (class CCallbackBase *pCallback)
{
  // Skip this if we're uninstalling our own callback
  if (SK_GetCallingDLL () == SK_GetDLL ())
  {
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

      // May need to block this callback to prevent some games from spuriously shutting
      //   SteamAPI down if the server's not cooperating.
      //
      //  * Fixes issues with missed/backlogged achievements in games like The Witcher 3
      if (config.steam.block_stat_callback)
      {
        steam_log.Log (L" ### Callback Blacklisted ###");
        LeaveCriticalSection (&callback_cs);
        return;
      }
      break;
    case UserStatsStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Stats Storage Callback",
                        caller.c_str () );
      break;
    case UserAchievementStored_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Achievements Storage Callback",
                        caller.c_str () );
      break;
    case UserStatsUnloaded_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled User Stats Unloaded Callback",
                        caller.c_str () );
      break;
    case SteamShutdown_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled SteamAPI Shutdown Callback",
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
    case SteamServersConnected_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Steam Server Connected Callback",
                        caller.c_str () );
      break;
    case SteamServerConnectFailure_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Steam Server Connection Failure Callback",
                        caller.c_str () );
      break;
    case SteamServersDisconnected_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Steam Server Disconnect Callback",
                        caller.c_str () );
      break;
    case LobbyEnter_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Lobby Enter Callback",
                        caller.c_str () );
      break;
    case LobbyDataUpdate_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Lobby Data Update Callback",
                        caller.c_str () );
      break;
    case LobbyChatUpdate_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Lobby Chat Update Callback",
                        caller.c_str () );
      break;
    case GameLobbyJoinRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Game Lobby Join Request Callback",
                        caller.c_str () );
      break;
    case GameRichPresenceJoinRequested_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Game Rich Presence Join Request Callback",
                        caller.c_str () );
      break;
    case P2PSessionConnectFail_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Peer to Peer Session Connect Failure Callback",
                        caller.c_str () );
      break;
    case P2PSessionRequest_t::k_iCallback:
      steam_log.Log ( L" * (%-28s) Uninstalled Peer to Peer Session Request Callback",
                        caller.c_str () );
      break;
    default:
    {
      steam_log.Log ( L" * (%-28s) Uninstalled Unknown Callback (Class=%hs, Id=%li)",
                        caller.c_str (), SK_Steam_ClassifyCallback (iCallback), iCallback % 100 );
    } break;
  }

  SteamAPI_UnregisterCallback_Original (pCallback);

  LeaveCriticalSection (&callback_cs);
}

SteamAPI_InitSafe_pfn SteamAPI_InitSafe_Original = nullptr;
SteamAPI_Init_pfn     SteamAPI_Init_Original     = nullptr;

extern "C" void __cdecl SteamAPIDebugTextHook (int nSeverity, const char *pchDebugText);


const char*
SK_Steam_PopupOriginToStr (int origin)
{
  switch (origin)
  {
    case 0:
      return "TopLeft";
    case 1:
      return "TopRight";
    case 2:
      return "BottomLeft";
    case 3:
    default:
      return "BottomRight";
    case 4:
      return "DontCare";
  }
}

const wchar_t*
SK_Steam_PopupOriginToWStr (int origin)
{
  switch (origin)
  {
    case 0:
      return L"TopLeft";
    case 1:
      return L"TopRight";
    case 2:
      return L"BottomLeft";
    case 3:
    default:
      return L"BottomRight";
    case 4:
      return L"DontCare";
  }
}

int
SK_Steam_PopupOriginWStrToEnum (const wchar_t* str)
{
  std::wstring name (str);

  if (name == L"TopLeft")
    return 0;
  else if (name == L"TopRight")
    return 1;
  else if (name == L"BottomLeft")
    return 2;
  else if (name == L"DontCare")
    return 4;
  else /*if (name == L"TopLeft")*/
    return 3;

  // TODO: TopCenter, BottomCenter, CenterLeft, CenterRight
}

int
SK_Steam_PopupOriginStrToEnum (const char* str)
{
  std::string name (str);

  if (name == "TopLeft")
    return 0;
  else if (name == "TopRight")
    return 1;
  else if (name == "BottomLeft")
    return 2;
  else if (name == "DontCare")
    return 4;
  else /*if (name == L"TopLeft")*/
    return 3;
}

void SK_Steam_SetNotifyCorner (void);


extern bool
ISteamController_GetControllerState_Detour (
  ISteamController*       This,
  uint32                  unControllerIndex,
  SteamControllerState_t *pState );

SK_SteamAPIContext steam_ctx;


void
SK_Steam_SetNotifyCorner (void)
{
  // 4 == Don't Care
  if (config.steam.notify_corner != 4)
  {
    if (steam_ctx.Utils ())
    {
      steam_ctx.Utils ()->SetOverlayNotificationPosition (
        static_cast <ENotificationPosition> (config.steam.notify_corner)
      );
    }
  }
}

bool
SK_SteamAPIContext::OnVarChange (SK_IVariable* var, void* val)
{
  SK_ICommandProcessor*
    pCommandProc = SK_GetCommandProcessor ();

  UNREFERENCED_PARAMETER (pCommandProc);

 
  bool known = false;

  // ( Dead man's switch for silly pirates that attempt to bypass framerate limit
  //    with a hex edit rather than a proper forked project - "crack" this the right way please )
  if (var == tbf_pirate_fun)
  {
    known = true;

    static float history [4] = { 45.0f, 45.0f, 45.0f, 45.0f };
    static int   idx         = 0;

    history [idx] = *static_cast <float *> (val);

    if (*static_cast <float *> (val) != 45.0f)
    {
      if (idx > 0 && history [idx-1] != 45.0f)
      {
        const SK_IVariable* _var =
          SK_GetCommandProcessor ()->ProcessCommandLine (
            "Textures.LODBias"
          ).getVariable ();
        
        if (_var != nullptr)
        {
          *static_cast <float *> (_var->getValuePointer ()) =
            *static_cast <float *> (val);
        }
      }
    }

    idx++;

    if (idx > 3)
      idx = 0;

    tbf_float = 0.0001f;

    return true;
  }


  if (var == notify_corner)
  {
    known = true;

    config.steam.notify_corner =
      SK_Steam_PopupOriginStrToEnum (*reinterpret_cast <char **> (val));

    strcpy (var_strings.notify_corner,
      SK_Steam_PopupOriginToStr (config.steam.notify_corner)
    );

    SK_Steam_SetNotifyCorner ();

    return true;
  }

  else if (var == popup_origin)
  {
    known = true;

    config.steam.achievements.popup.origin =
      SK_Steam_PopupOriginStrToEnum (*reinterpret_cast <char **> (val));

    strcpy (var_strings.popup_origin,
      SK_Steam_PopupOriginToStr (config.steam.achievements.popup.origin)
    );

    return true;
  }

  if (! known)
  {
    dll_log.Log ( L"[SteammAPI] UNKNOWN Variable Changed (%p --> %p)",
                     var,
                       val );
  }

  return false;
}

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

bool           has_global_data  = false;
ULONG          next_friend      = 0UL;
ULONG          friend_count     = 0UL;
SteamAPICall_t friend_query     = 0;

class SK_Steam_AchievementManager
{
public:
  class Achievement : public SK_SteamAchievement
  {
  public:
    Achievement (int idx, const char* szName, ISteamUserStats* stats)
    {
      idx_            = idx;

      global_percent_ = 0.0f;
      unlocked_       = false;
      time_           = 0;

      friends_.possible = 0;
      friends_.unlocked = 0;

      icons_.achieved   = nullptr;
      icons_.unachieved = nullptr;

      progress_.max     = 0;
      progress_.current = 0;

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
      if (name_ != nullptr)
      {
        free ((void *)name_);
                      name_ = nullptr;
      }

      if (human_name_ != nullptr)
      {
        free ((void *)human_name_);
                      human_name_ = nullptr;
      }

      if (desc_ != nullptr)
      {
        free ((void *)desc_);
                      desc_ = nullptr;
      }

      if (icons_.unachieved != nullptr)
      {
        free (icons_.unachieved);
              icons_.unachieved = nullptr;
      }

      if (icons_.achieved != nullptr)
      {
        free (icons_.achieved);
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
                &global_percent_ ) )
      {
        steam_log.Log (L" Global Achievement Read Failure For '%hs'", name_);
        global_percent_ = 0.0f;
      }
    }
  };

private:
  struct SK_AchievementPopup
  {
    CEGUI::Window* window;
    DWORD          time;
    bool           final_pos; // When the animation is finished, this will be set.
    Achievement*   achievement;
  };

public:
  SK_Steam_AchievementManager (const wchar_t* wszUnlockSound) :
       unlock_listener ( this, &SK_Steam_AchievementManager::OnUnlock                ),
       stat_receipt    ( this, &SK_Steam_AchievementManager::AckStoreStats           ),
       icon_listener   ( this, &SK_Steam_AchievementManager::OnRecvIcon              ),
       async_complete  ( this, &SK_Steam_AchievementManager::OnAsyncComplete         )
  {
    percent_unlocked = 0.0f;
    lifetime_popups  = 0;

    ISteamUserStats* user_stats = nullptr;
    ISteamFriends*   friends    = nullptr;

    if (! (user_stats = steam_ctx.UserStats ()))
    {
      steam_log.Log ( L" *** Cannot get ISteamUserStats interface, bailing-out of"
                      L" Achievement Manager Init. ***" );
      return;
    }

    if (! (friends = steam_ctx.Friends ()))
    {
      steam_log.Log ( L" *** Cannot get ISteamFriends interface, bailing-out of"
                      L" Achievement Manager Init. ***" );
      return;
    }

    loadSound (wszUnlockSound);

    uint32_t achv_reserve =
      std::max (user_stats->GetNumAchievements (), (uint32_t)128UL);
    friend_count =
      friends->GetFriendCount (k_EFriendFlagImmediate);

    // OFFLINE MODE
    if (friend_count == UINT32_MAX || friend_count > 8192)
      friend_count = 0;

    else {
      friend_sid_to_idx.reserve (friend_count);
      friend_stats.resize       (friend_count);
    }

    achievements.list.resize        (achv_reserve);
    achievements.string_map.reserve (achv_reserve);

    for (uint32_t i = 0; i < achv_reserve; i++)
      achievements.list [i] = nullptr;
  }

  ~SK_Steam_AchievementManager (void)
  {
    if ((! default_loaded) && (unlock_sound != nullptr))
    {
      delete [] unlock_sound;
                unlock_sound = nullptr;
    }

    unlock_listener.Unregister ();
    stat_receipt.Unregister    ();
    icon_listener.Unregister   ();
    async_complete.Unregister  ();
  }

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
                           100.0 * (static_cast <double> (achievement.friends_.unlocked) /
                                    static_cast <double> (achievement.friends_.possible)) );

      if (achievement.unlocked_)
      {
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

    uint32 app_id =
      CGameID (pParam->m_nGameID).AppID ();

    if (app_id != SK::SteamAPI::AppID ())
    {
      steam_log.Log ( L" Got User Achievement Stats for Wrong Game (%lu)",
                        app_id );
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

        if (config.steam.achievements.pull_global_stats && (! has_global_data))
        {
          has_global_data = true;

          global_request =
            stats->RequestGlobalAchievementPercentages ();
        }

        // On first stat reception, trigger an update for all friends so that
        //   we can display friend achievement stats.
        if ( next_friend == 0 && config.steam.achievements.pull_friend_stats )
        {
          friend_count =
            friends->GetFriendCount (k_EFriendFlagImmediate);

          friend_stats.resize       (friend_count);
          friend_sid_to_idx.reserve (friend_count);

          if (friend_count > 0 && friend_count != MAXUINT32)
          {
            // Enumerate all known friends immediately
            for (unsigned int i = 0 ; i < friend_count; i++)
            {
              CSteamID sid ( friends->GetFriendByIndex ( i,
                                                           k_EFriendFlagImmediate
                                                       )
                           );

              friend_sid_to_idx [sid.ConvertToUint64 ()] = i;
              friend_stats      [i].name       =
                friends->GetFriendPersonaName (sid);
              friend_stats      [i].account_id = sid.ConvertToUint64 ();
              friend_stats      [i].unlocked.resize (achievements.list.capacity ());
            }

            SteamAPICall_t hCall =
              stats->RequestUserStats (CSteamID (friend_stats [next_friend++].account_id));

            friend_query = hCall;

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

    uint32 app_id =
      CGameID (pParam->m_nGameID).AppID ();

    if (app_id != SK::SteamAPI::AppID ())
    {
      steam_log.Log ( L" Got User Achievement Stats for Wrong Game (%lu)",
                        app_id );
      return;
    }

    ISteamUser*      user    = steam_ctx.User      ();
    ISteamUserStats* stats   = steam_ctx.UserStats ();
    ISteamFriends*   friends = steam_ctx.Friends   ();

    if (! (user && stats && friends && pParam))
      return;

    // A user may return kEResultFail if they don't own this game, so
    //   do this anyway and continue enumerating remaining friends.
    if (pParam->m_eResult == k_EResultOK)
    {
      // If the stats are not for the player, they are probably for one of
      //   the player's friends.
      if ( k_EFriendRelationshipFriend ==
             friends->GetFriendRelationship (pParam->m_steamIDUser) )
      {
        bool     can_unlock  = false;
        int      unlocked    = 0;

        uint32_t friend_idx  =
          friend_sid_to_idx [pParam->m_steamIDUser.ConvertToUint64 ()];

        for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
        {
          char* szName = _strdup (stats->GetAchievementName (i));

          stats->GetUserAchievement (
            pParam->m_steamIDUser,
              szName,
                reinterpret_cast <bool *> (
                  &friend_stats [friend_idx].unlocked [i] )
          );

          if (friend_stats [friend_idx].unlocked [i])
          {
            unlocked++;

            // On the first unlocked achievement, make a note...
            if (! can_unlock)
            {
              ///steam_log.Log ( L"Received Achievement Stats for Friend: '%hs'",
                                ///friends->GetFriendPersonaName (pParam->m_steamIDUser) );
            }

            can_unlock = true;

            ++achievements.list [i]->friends_.unlocked;

            ///steam_log.Log (L" >> Has unlocked '%24hs'", szName);
          }

          free (const_cast <char *> (szName));
        }

        friend_stats [friend_idx].percent_unlocked =
          static_cast <float> (unlocked) /
          static_cast <float> (stats->GetNumAchievements ());

        // Second pass over all achievements, incrementing the number of friends who
        //   can potentially unlock them.
        if (can_unlock)
        {
          for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
            ++achievements.list [i]->friends_.possible;
        }
      }
    }

    // TODO: The function that walks friends should be a lambda
    else
    {
      // Friend doesn't own the game, so just skip them...
      if (pParam->m_eResult == k_EResultFail)
      {
      }

      else
      {
        steam_log.Log ( L" >>> User stat receipt unsuccessful! (m_eResult = 0x%04X)",
                        pParam->m_eResult );
      }
    }

    if (friend_count > 0 && next_friend < friend_count)
    {
      CSteamID sid (friend_stats [next_friend++].account_id);

      SteamAPICall_t hCall =
        stats->RequestUserStats (sid);

      friend_query = hCall;

      friend_listener.Set (
        hCall,
          this,
            &SK_Steam_AchievementManager::OnRecvFriendStats );
    }
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   OnAsyncComplete,
                   SteamAPICallCompleted_t,
                   async_complete )
  {
    bool                 failed    = true;
    ESteamAPICallFailure eCallFail = k_ESteamAPICallFailureNone;

    if (steam_ctx.Utils ())
      steam_ctx.Utils ()->IsAPICallCompleted (pParam->m_hAsyncCall, &failed);

    if (failed)
      eCallFail = steam_ctx.Utils ()->GetAPICallFailureReason (pParam->m_hAsyncCall);

    if (pParam->m_hAsyncCall == global_request)
    {
      if (! failed)
      {
        has_global_data = true;

        SK_SteamAPI_UpdateGlobalAchievements ();
      }

      else
      {
        steam_log.Log ( L" Non-Zero Result (%li) for GlobalAchievementPercentagesReady_t",
                          eCallFail );

        has_global_data = false;
        return;
      }
    }

    else {
      if (failed)
      {
        //steam_log.Log ( L" Asynchronous SteamAPI Call _NOT ISSUED BY Special K_ Failed: "
                        //L"(reason=%lu)",
                          //eCallFail );
      }
    }
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   AckStoreStats,
                   UserStatsStored_t,
                   stat_receipt )
  {
    uint32 app_id =
      CGameID (pParam->m_nGameID).AppID ();

    // Sometimes we receive event callbacks for games that aren't this one...
    //   ignore those!
    if (app_id != SK::SteamAPI::AppID ())
      return;

    steam_log.Log ( L" >> Stats Stored for AppID: %lu",
                      app_id );
  }

  STEAM_CALLBACK ( SK_Steam_AchievementManager,
                   OnRecvIcon,
                   UserAchievementIconFetched_t,
                   icon_listener )
  {
    if (pParam->m_nGameID.AppID () != SK::SteamAPI::AppID ())
    {
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
    uint32 app_id =
      CGameID (pParam->m_nGameID).AppID ();

    // Sometimes we receive event callbacks for games that aren't this one...
    //   ignore those!
    if (app_id != SK::SteamAPI::AppID ())
    {
      steam_log.Log ( L" >>> Received achievement unlock for unrelated game (AppId=%lu) <<<",
                        app_id );
      return;
    }

    Achievement* achievement =
      getAchievement (
        pParam->m_rgchAchievementName
      );

    if (achievement != nullptr)
    {
      if (steam_ctx.UserStats ())
        achievement->update(steam_ctx.UserStats());

      achievement->progress_.current = pParam->m_nCurProgress;
      achievement->progress_.max     = pParam->m_nMaxProgress;

      // Pre-Fetch
      if (steam_ctx.UserStats ())
        steam_ctx.UserStats()->GetAchievementIcon (pParam->m_rgchAchievementName);

      if ( pParam->m_nMaxProgress == pParam->m_nCurProgress )
      {
        // Do this so that 0/0 does not come back as NaN
        achievement->progress_.current = 1;
        achievement->progress_.max     = 1;

        if (config.steam.achievements.play_sound)
          PlaySound ( (LPCWSTR)unlock_sound, nullptr, SND_ASYNC | SND_MEMORY );

        if (achievement != nullptr)
        {
          steam_log.Log (L" Achievement: '%hs' (%hs) - Unlocked!",
            achievement->human_name_, achievement->desc_);
        }

        // If the user wants a screenshot, but no popups (why?!), this is when
        //   the screenshot needs to be taken.
        if ( config.steam.achievements.take_screenshot &&
          (! config.steam.achievements.popup.show) )
        {
          SK::SteamAPI::TakeScreenshot ();
        }

        // Re-calculate percentage unlocked
        pullStats ();
      }

      else
      {
        float progress = 
          static_cast <float> (pParam->m_nCurProgress) /
          static_cast <float> (pParam->m_nMaxProgress);

        steam_log.Log (L" Achievement: '%hs' (%hs) - "
                       L"Progress %lu / %lu (%04.01f%%)",
                  achievement->human_name_,
                  achievement->desc_,
                        pParam->m_nCurProgress,
                        pParam->m_nMaxProgress,
                          100.0f * progress );
      }
    }

    else
    {
      steam_log.Log ( L" Got Rogue Achievement Storage: '%hs'",
                        pParam->m_rgchAchievementName );
    }

    if ( config.cegui.enable && config.steam.achievements.popup.show &&
         achievement != nullptr )
    {
      CEGUI::System* pSys = CEGUI::System::getDllSingletonPtr ();

      if ( pSys                 != nullptr &&
           pSys->getRenderer () != nullptr )
      {
        EnterCriticalSection (&popup_cs);

        try
        {
          SK_AchievementPopup popup;

          popup.window      = nullptr;
          popup.final_pos   = false;
          popup.time        = timeGetTime ();
          popup.achievement = achievement;

          popups.push_back (popup);
        } 

        catch (...)
        {
        }

        LeaveCriticalSection (&popup_cs);
      }
    }
  }

  void clearPopups (void)
  {
    EnterCriticalSection (&popup_cs);

    if (popups.empty ())
    {
      LeaveCriticalSection (&popup_cs);
      return;
    }

    popups.clear ();

    LeaveCriticalSection (&popup_cs);
  }

  void drawPopups (void)
  {
    EnterCriticalSection (&popup_cs);

    if (popups.empty ())
    {
      LeaveCriticalSection (&popup_cs);
      return;
    }

    //
    // We don't need this, we always do this from the render thread.
    //
    //if (SK_PopupManager::getInstance ()->tryLockPopups ())
    {
      try
      {
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
          CEGUI::System::getDllSingleton ().getRenderer ()->
            getDisplaySize ().d_height * (1.0f - inset);

        const float full_wd =
          CEGUI::System::getDllSingleton ().getRenderer ()->
            getDisplaySize ().d_width * (1.0f - inset);

        float x_origin, y_origin,
              x_dir,    y_dir;

        CEGUI::Window* first = popups.begin ()->window;

        const float win_ht0 = first != nullptr ?
                                (popups.begin ()->window->getPixelSize ().d_height) :
                                  0.0f;
        const float win_wd0 = first != nullptr ?
                                (popups.begin ()->window->getPixelSize ().d_width) :
                                  0.0f;

        const float title_wd =
          CEGUI::System::getDllSingleton ().getRenderer ()->
            getDisplaySize ().d_width * (1.0f - 2.0f * inset);

        const float title_ht =
          CEGUI::System::getDllSingleton ().getRenderer ()->
            getDisplaySize ().d_height * (1.0f - 2.0f * inset);

        float fract_x, fract_y;

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

        while (it != popups.end ())
        {
          if (timeGetTime () < (*it).time + POPUP_DURATION_MS)
          {
            float percent_of_lifetime = (static_cast <float> ((*it).time + POPUP_DURATION_MS - timeGetTime ()) / 
                                         static_cast <float> (POPUP_DURATION_MS));

            //if (SK_PopupManager::getInstance ()->isPopup ((*it).window)) {
              CEGUI::Window* win = (*it).window;

              if (win == nullptr)
              {
                win = createPopupWindow (&*it);
                created = true;
              }

              const float win_ht =
                win->getPixelSize ().d_height;

              const float win_wd =
                win->getPixelSize ().d_width;

              CEGUI::UVector2 win_pos (x_pos, y_pos);

              float bottom_y = win_pos.d_y.d_scale * full_ht + 
                                 win_pos.d_y.d_offset        +
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
                                 win_pos.d_x.d_offset       +
                                   win_wd;

              // Window is off-screen horizontally AND vertically
              if ( inset != 0.0f && (right_x > full_wd || right_x < win_wd0)  )
              {
                // Since we're not going to draw this, treat it as a new
                //   popup until it first becomes visible.
                (*it).time =
                     timeGetTime ();
                win->hide        ();
              }

              else
              {
                bool take_screenshot = false;

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
                    it->final_pos   = true;
                    take_screenshot = true;
                  }
                }

                else if (! it->final_pos)
                {
                  take_screenshot = true;
                  it->final_pos   = true;
                }

                // Popup is in the final location, so now is when screenshots
                //   need to be taken.
                if (config.steam.achievements.take_screenshot && take_screenshot && it->achievement->unlocked_)
                  SK::SteamAPI::TakeScreenshot ();

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
        if (removed || created)
        {
          SK_TextOverlayManager::getInstance ()->drawAllOverlays     (0.0f, 0.0f, true);

          if (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D9)
          {
            CEGUI::System::getDllSingleton   ().renderAllGUIContexts ();
          }
        }
      }

      catch (...) {}
    }
    //SK_PopupManager::getInstance ()->unlockPopups ();
    LeaveCriticalSection (&popup_cs);
  }

  float getPercentOfAchievementsUnlocked (void)
  {
    return percent_unlocked;
  }

  Achievement* getAchievement (const char* szName)
  {
    if (achievements.string_map.count (szName))
      return achievements.string_map [szName];

    return nullptr;
  }

  SK_SteamAchievement** getAchievements (size_t* pnAchievements = nullptr)
  {
    ISteamUserStats* stats = steam_ctx.UserStats       ();
    size_t           count = stats->GetNumAchievements ();

    if (pnAchievements != nullptr)
      *pnAchievements = count;

    static std::set    <SK_SteamAchievement *> achievement_set;
    static std::vector <SK_SteamAchievement *> achievement_data;

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

  void requestStats (void)
  {
    if (steam_ctx.UserStats ())
    {
      if (steam_ctx.UserStats ()->RequestCurrentStats ())
      {
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
  }

  // FIXME: Poorly named given the existence of requestStats
  //
  //   (nb: triggered when the async Steam call in requestStats returns)
  void pullStats (void)
  {
    ISteamUserStats* stats = steam_ctx.UserStats ();

    if (stats)
    {
      int unlocked = 0;

      for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
      {
        //steam_log.Log (L"  >> %hs", stats->GetAchievementName (i));

        if (achievements.list [i] == nullptr)
        {
          char* szName = _strdup (stats->GetAchievementName (i));

          achievements.list [i] =
            new Achievement (
                  i,
                  szName,
                  stats
                );

          achievements.string_map [achievements.list [i]->name_] =
            achievements.list [i];

          achievements.list [i]->update (stats);

          //steam_log.Log (L"  >> (%lu) %hs", i, achievements.list [i]->name_);

          // Start pre-loading images so we do not hitch on achievement unlock...

          //if (! achievements.list [i]->unlocked_)
          //stats->SetAchievement   (szName);

          // After setting the achievement, fetch the icon -- this is
          //   necessary so that the unlock dialog does not display
          //     the locked icon.
          if ( steam_ctx.UserStats () )
            steam_ctx.UserStats ()->GetAchievementIcon (szName);

          //if (! achievements.list [i]->unlocked_)
            //stats->ClearAchievement (szName);

          free (const_cast <char *> (szName));
        }

        else
        {
          achievements.list [i]->update (stats);
        }

        if (achievements.list [i]->unlocked_)
          unlocked++;
      }

      percent_unlocked =
        static_cast <float> (unlocked) /
        static_cast <float> (stats->GetNumAchievements ());
    }
  }

  void
  loadSound (const wchar_t* wszUnlockSound)
  {
    bool xbox = false,
         psn  = false;

    wchar_t wszFileName [MAX_PATH + 2] = { };

    if (! wcslen (wszUnlockSound))
    {
      iSK_INI achievement_ini (
        std::wstring (
          SK_GetDocumentsDir () + L"\\My Mods\\SpecialK\\Global\\achievements.ini"
        ).c_str () );

      // If the config file is empty, establish defaults and then write it.
      if (achievement_ini.get_sections ().empty ())
      {
        achievement_ini.import ( L"[Steam.Achievements]\n"
                                 L"SoundFile=psn\n"
                                 L"PlaySound=true\n"
                                 L"TakeScreenshot=false\n"
                                 L"AnimatePopup=true\n"
                                 L"NotifyCorner=0\n" );
        achievement_ini.write (
          std::wstring (
            SK_GetDocumentsDir () + L"\\My Mods\\SpecialK\\Global\\achievements.ini"
          ).c_str () );
      }

      if (achievement_ini.contains_section (L"Steam.Achievements"))
      {
        iSK_INISection& sec = achievement_ini.get_section (L"Steam.Achievements");
        if (sec.contains_key (L"SoundFile"))
          wcscpy (wszFileName, sec.get_value (L"SoundFile").c_str ());
      }
    }

    else
    {
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

      unlock_sound =
        new uint8_t [size];

      if (unlock_sound != nullptr)
        fread  (unlock_sound, size, 1, fWAV);

      fclose (fWAV);

      steam_log.LogEx (false, L" %d bytes\n", size);

      default_loaded = false;
    }

    else
    {
      // Default to PSN if not specified
      if ((! psn) && (! xbox))
        psn = true;

      steam_log.Log (L"  * Loading Built-In Achievement Unlock Sound: '%s'",
                       wszFileName);

      HRSRC   default_sound;

      if (psn)
        default_sound =
          FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_TROPHY), L"WAVE");
      else
        default_sound =
          FindResource (SK_GetDLL (), MAKEINTRESOURCE (IDR_XBOX), L"WAVE");

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
  }

  const std::string& getFriendName (uint32_t friend_idx)
  {
    return friend_stats [friend_idx].name;
  }

  float getFriendUnlockPercentage (uint32_t friend_idx)
  {
    return friend_stats [friend_idx].percent_unlocked;
  }

  bool hasFriendUnlockedAchievement (uint32_t friend_idx, uint32_t achv_idx)
  {
    return friend_stats [friend_idx].unlocked [achv_idx];
  }

protected:
  CEGUI::Window* createPopupWindow (SK_AchievementPopup* popup)
  {
    if (popup->achievement == nullptr)
      return nullptr;

    CEGUI::System* pSys = CEGUI::System::getDllSingletonPtr ();

    extern CEGUI::Window* SK_achv_popup;

    char szPopupName [16];
    sprintf (szPopupName, "Achievement_%li", lifetime_popups++);

    popup->window              = SK_achv_popup->clone (true);
    Achievement*   achievement = popup->achievement;
    CEGUI::Window* achv_popup  = popup->window;

    achv_popup->setName (szPopupName);

    CEGUI::Window* achv_title  = achv_popup->getChild ("Title");
    achv_title->setText ((const CEGUI::utf8 *)achievement->human_name_);

    CEGUI::Window* achv_desc = achv_popup->getChild ("Description");
    achv_desc->setText ((const CEGUI::utf8 *)achievement->desc_);

    CEGUI::Window* achv_rank = achv_popup->getChild ("Rank");
    achv_rank->setProperty ( "NormalTextColour",
                               SK_Steam_RarityToColor (achievement->global_percent_)
                           );
    achv_rank->setText ( SK_Steam_RarityToName (achievement->global_percent_) );

    CEGUI::Window* achv_global = achv_popup->getChild ("GlobalRarity");
    char szGlobal [32] = { };
    snprintf ( szGlobal, 32,
                 "Global: %6.2f%%",
                   achievement->global_percent_ );
    achv_global->setText (szGlobal);
    
    CEGUI::Window* achv_friend = achv_popup->getChild ("FriendRarity");
    char szFriend [32] = { };
    snprintf ( szFriend, 32,
                 "Friends: %6.2f%%",
        100.0 * ((double)          achievement->friends_.unlocked /
                 (double)std::max (achievement->friends_.possible, 1)) );
    achv_friend->setText (szFriend);


    // If the server hasn't updated the unlock time, just use the current time
    if (achievement != nullptr && achievement->time_ == 0)
      _time32 (&achievement->time_);

    CEGUI::Window* achv_unlock  = achv_popup->getChild ("UnlockTime");
    CEGUI::Window* achv_percent = achv_popup->getChild ("ProgressBar");

    float progress = achievement->progress_.getPercent ();
    
    char szUnlockTime [128] = { };
    if (progress == 100.0f)
    {
      snprintf ( szUnlockTime, 128,
                   "Unlocked: %s", _ctime32 (&achievement->time_)
               );

      achv_percent->setProperty ( "CurrentProgress", "1.0" );
    }
    
    else
    {
      snprintf ( szUnlockTime, 16,
                   "%5.4f",
                     progress / 100.0f
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

    int icon_idx =
      steam_ctx.UserStats () ? 
        steam_ctx.UserStats ()->GetAchievementIcon (achievement->name_) :
        0;

    // Icon width and height
    uint32 w = 0,
           h = 0;

    if (icon_idx != 0)
    {
      if (steam_ctx.Utils ())
        steam_ctx.Utils ()->GetImageSize (icon_idx, &w, &h);
    
      int tries = 1;

      while (achievement->icons_.achieved == nullptr && tries < 8)
      {
        achievement->icons_.achieved =
          (uint8_t *)malloc (4 * w * h);

        if (steam_ctx.Utils ())
        {
          if ( ! steam_ctx.Utils ()->GetImageRGBA (
                   icon_idx,
                     achievement->icons_.achieved,
                       4 * w * h 
                 )
             )
          {
            free (achievement->icons_.achieved);
            achievement->icons_.achieved = nullptr;
            ++tries;
          }

          else
          {
            steam_log.Log ( L" * Fetched RGBA Icon (idx=%li) for Achievement: '%hs'  (%lux%lu) "
                              L"{ Took %li try(s) }",
                              icon_idx, achievement->name_, w, h, tries );
          }
        }
      }
    }

    if (achievement->icons_.achieved != nullptr)
    {
      bool exists = CEGUI::ImageManager::getDllSingleton ().isDefined (achievement->name_);

      CEGUI::Image& img =
         exists ?
          CEGUI::ImageManager::getDllSingleton ().get    (              achievement->name_) :
          CEGUI::ImageManager::getDllSingleton ().create ("BasicImage", achievement->name_);

      if (! exists) try
      {
        /* StaticImage */
        CEGUI::Texture& Tex =
          pSys->getRenderer ()->createTexture (achievement->name_);

        Tex.loadFromMemory ( achievement->icons_.achieved,
                               CEGUI::Sizef ( static_cast <float> (w),
                                              static_cast <float> (h) ),
                                 CEGUI::Texture::PF_RGBA );

        ((CEGUI::BasicImage &)img).setTexture (&Tex);

        const CEGUI::Rectf rect (CEGUI::Vector2f (0.0f, 0.0f), Tex.getOriginalDataSize ());
        ((CEGUI::BasicImage &)img).setArea       (rect);
        ((CEGUI::BasicImage &)img).setAutoScaled (CEGUI::ASM_Both);
      }

      catch (...)
      {
      }

      try
      {
        CEGUI::Window* staticImage = achv_popup->getChild ("Icon");
        staticImage->setProperty ( "Image",
                                     achievement->name_ );
      }

      catch (...)
      {
      }
    }

    if (config.steam.achievements.popup.show_title)
    {
      std::string app_name = SK::SteamAPI::AppName ();
    
      if (strlen (app_name.c_str ()))
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

private:
  struct SK_AchievementStorage
  {
    std::vector        <             Achievement*> list;
    std::unordered_map <std::string, Achievement*> string_map;
  } achievements; // SELF

  struct friend_s {
    std::string        name;
    float              percent_unlocked;
    std::vector <BOOL> unlocked;
    uint64_t           account_id;
  };

  std::vector <friend_s>
     friend_stats;

  std::unordered_map <uint64_t, uint32_t>
     friend_sid_to_idx;

  float                             percent_unlocked;

  std::vector <SK_AchievementPopup> popups;
  int                               lifetime_popups;

  SteamAPICall_t global_request;
  SteamAPICall_t stats_request;

  bool           default_loaded = false;
  uint8_t*       unlock_sound   = nullptr;   // A .WAV (PCM) file
} static *steam_achievements = nullptr;

void
SK_SteamAPI_LoadUnlockSound (const wchar_t* wszUnlockSound)
{
  if (steam_achievements != nullptr)
    steam_achievements->loadSound (wszUnlockSound);
}

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
  if (steam_achievements != nullptr)
    steam_achievements->log_all_achievements ();
}

void
SK_UnlockSteamAchievement (uint32_t idx)
{
  if (config.steam.silent)
    return;

  if (! steam_achievements)
  {
    steam_log.Log ( L" >> Steam Achievement Manager Not Yet Initialized ... "
                    L"Skipping Unlock <<" );
    return;
  }

  steam_log.LogEx (true, L" >> Attempting to Unlock Achievement: %lu... ",
    idx );

  ISteamUserStats* stats = steam_ctx.UserStats ();

  if ( stats                        && idx <
       stats->GetNumAchievements () )
  {
    // I am dubious about querying these things by name, so duplicate this
    //   string immediately.
    const char* szName = _strdup (stats->GetAchievementName (idx));

    if (szName != nullptr)
    {
      steam_log.LogEx (false, L" (%hs - Found)\n", szName);

      steam_achievements->pullStats ();

      UserAchievementStored_t store;
      store.m_nCurProgress = 0;
      store.m_nMaxProgress = 0;
      store.m_nGameID      = CGameID (SK::SteamAPI::AppID ()).ToUint64 ();
      strncpy (store.m_rgchAchievementName, szName, 128);

      SK_Steam_AchievementManager::Achievement* achievement =
        steam_achievements->getAchievement (
          store.m_rgchAchievementName
        );

      UNREFERENCED_PARAMETER (achievement);

      steam_achievements->OnUnlock (&store);

//      stats->ClearAchievement            (szName);
//      stats->IndicateAchievementProgress (szName, 0, 1);
//      stats->StoreStats                  ();
#if 0
      bool achieved;
      if (stats->GetAchievement (szName, &achieved))
      {
        if (achieved)
        {
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
      free (const_cast <char *> (szName));
    }

    else
      steam_log.LogEx (false, L" (None Found)\n");
  }

  else
    steam_log.LogEx   (false, L" (ISteamUserStats is NULL?!)\n");

  SK_SteamAPI_LogAllAchievements ();
}

void
SK_SteamAPI_UpdateGlobalAchievements (void)
{
  ISteamUserStats* stats =
    steam_ctx.UserStats ();

  if (stats != nullptr && steam_achievements != nullptr)
  {
    const int num_achievements = stats->GetNumAchievements ();

    for (int i = 0; i < num_achievements; i++)
    {
      const char* szName = _strdup (stats->GetAchievementName (i));

      SK_Steam_AchievementManager::Achievement* achievement =
        steam_achievements->getAchievement (szName);

      if (achievement != nullptr)
        achievement->update_global (stats);

      else
      {
        dll_log.Log ( L" Got Global Data For Unknown Achievement ('%hs')",
                        szName );
      }

      free (const_cast <char *> (szName));
    }
  }

  else
  {
    steam_log.Log ( L"Got Global Stats from SteamAPI, but no "
                    L"ISteamUserStats interface exists?!" );
  }

  if (steam_achievements)
    SK_SteamAPI_LogAllAchievements ();
}

void
SK_Steam_ClearPopups (void)
{
  if (steam_achievements != nullptr)
  {
    steam_achievements->clearPopups ();
    SteamAPI_RunCallbacks_Original  ();
  }
}

class SK_Steam_UserManager
{
public:
   SK_Steam_UserManager (void) = default;
  ~SK_Steam_UserManager (void) = default;

  int32_t  GetNumPlayers    (void) { return InterlockedAdd (&num_players_, 0L); }
  void     UpdateNumPlayers (void)
  {
    // Service Only One Update
    if (! InterlockedCompareExchange (&num_players_call_, 1, 0))
    {
      ISteamUserStats* user_stats =
        steam_ctx.UserStats ();

      if (user_stats != nullptr)
      {
        InterlockedExchange (&num_players_call_,
          user_stats->GetNumberOfCurrentPlayers ());

        current_players_call.Set (
          InterlockedAdd64 ((LONG64 *)&num_players_call_, 0LL),
            this,
              &SK_Steam_UserManager::OnRecvNumCurrentPlayers );
      }
    }
  }

protected:
  STEAM_CALLRESULT ( SK_Steam_UserManager,
                     OnRecvNumCurrentPlayers,
                     NumberOfCurrentPlayers_t,
                     current_players_call )
  {
    current_players_call.Cancel ();

    if (pParam->m_bSuccess)
      InterlockedExchange (&num_players_, pParam->m_cPlayers);

    InterlockedExchange (&num_players_call_, 0);
  }

private:
  // Can be updated on-request
  volatile LONG           num_players_      = 1;
  volatile SteamAPICall_t num_players_call_ = 0;
} static *user_manager = nullptr;


void
SK_Steam_DrawOSD (void)
{
  if (steam_achievements != nullptr)
  {
    //SteamAPI_RunCallbacks_Original ();
    steam_achievements->drawPopups ();
  }
}

volatile LONGLONG SK_SteamAPI_CallbackRunCount    = 0LL;
volatile LONG     SK_SteamAPI_ManagersInitialized = 0L;

void
S_CALLTYPE
SteamAPI_RunCallbacks_Detour (void)
{
  static bool failure = false;

  // Throttle to 1000 Hz for STUPID games like Akiba's Trip
  static DWORD dwLastTick = 0;
         DWORD dwNow      = timeGetTime ();

  if (dwLastTick < dwNow)
    dwLastTick = dwNow;

  else
  {
    if (InterlockedCompareExchange (&__SK_Steam_init, 0, 0))
    {
      InterlockedIncrement64 (&SK_SteamAPI_CallbackRunCount);
      // Skip the callbacks, one call every 1 ms is enough!!

      SleepEx (0, TRUE);
      return;
    }
  }

  if ((! failure) && (( InterlockedAdd64 (&SK_SteamAPI_CallbackRunCount, 0) == 0 || steam_achievements == nullptr )))
  {
    // Handle situations where Steam was initialized earlier than
    //   expected...
    if (InterlockedCompareExchange (&__SK_Steam_init, 0, 0))
    {
      // Init the managers after the first callback run
      void SK_SteamAPI_InitManagers (void);
           SK_SteamAPI_InitManagers ();
    }

    static volatile HANDLE hThread = nullptr;

    if (InterlockedCompareExchangePointer (&hThread, nullptr, nullptr) == nullptr)
    {
      InterlockedExchangePointer ((void **)&hThread,
        CreateThread ( nullptr, 0,
        [](LPVOID user) ->
          DWORD
            {
              strcpy ( steam_ctx.var_strings.popup_origin,
                         SK_Steam_PopupOriginToStr (
                           config.steam.achievements.popup.origin
                         )
                     );

              strcpy ( steam_ctx.var_strings.notify_corner,
                         SK_Steam_PopupOriginToStr (
                           config.steam.notify_corner
                         )
                     );

              if (! steam_ctx.UserStats ())
              {
                if (SteamAPI_InitSafe_Original != nullptr)
                  SteamAPI_InitSafe_Detour ();

                if (! steam_ctx.UserStats ())
                {
                  InterlockedExchangePointer (static_cast <void **> (user), nullptr);

                  CloseHandle (GetCurrentThread ());

                  return (DWORD)-1;
                }
              }

              __try
              {
                SK_Steam_SetNotifyCorner ();

                SteamAPI_RunCallbacks_Original ();

                steam_ctx.UserStats ()->RequestGlobalAchievementPercentages ();

                SteamAPI_RunCallbacks_Original ();
              }

              __except (EXCEPTION_EXECUTE_HANDLER)
              {
                failure = true;
                steam_log.Log (L" Caught a Structured Exception while running Steam Callbacks!");
                InterlockedIncrement64 (&SK_SteamAPI_CallbackRunCount);
              }

              InterlockedExchangePointer (static_cast <void **> (user), nullptr);

              CloseHandle (GetCurrentThread ());

              return 0;
            }, (LPVOID)&hThread,
          0x00,
        nullptr)
      );
    }
  }

  __try
  {
    extern volatile ULONG          __SK_DLL_Ending;
    if (! InterlockedExchangeAdd (&__SK_DLL_Ending, 0))
    {
      InterlockedIncrement64         (&SK_SteamAPI_CallbackRunCount);
      SteamAPI_RunCallbacks_Original ();
    }

    else
    {
      // Do not keep restarting SteamAPI :)
      //if (SteamAPI_Shutdown != nullptr)
      //  TerminateThread (GetCurrentThread (), MAXDWORD);
    }
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    if (! failure)
    {
      steam_log.Log (L" Caught a Structured Exception while running Steam Callbacks!");
      failure = true;
    }
  }
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

// TODO: Remove
void
SK::SteamAPI::Init (bool pre_load)
{
  UNREFERENCED_PARAMETER (pre_load);

  if (config.steam.silent)
    return;
}

void
SK::SteamAPI::Shutdown (void)
{
  // Since we might restart SteamAPI, don't do this.
  //SK_AutoClose_Log (steam_log);

  steam_ctx.Shutdown ();
}

void SK::SteamAPI::Pump (void)
{
  if (steam_ctx.UserStats ())
  {
    if (SteamAPI_RunCallbacks_Original != nullptr)
      SteamAPI_RunCallbacks_Detour ();
  }
}

volatile HANDLE hSteamPump = nullptr;

DWORD
WINAPI
SteamAPI_PumpThread (_Unreferenced_parameter_ LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  // Wait 5 seconds, then begin a timing investigation
  SleepEx (5000, FALSE);

  // First, begin a timing probe.
  //
  //   If after 15 seconds the game has not called SteamAPI_RunCallbacks
  //     frequently enough, switch the thread to auto-pump mode.

  LONGLONG callback_count0 = SK_SteamAPI_CallbackRunCount;

  const UINT TEST_PERIOD = 15;

  SleepEx (TEST_PERIOD * 1000UL, FALSE);

  LONGLONG callback_count1 = SK_SteamAPI_CallbackRunCount;

  double callback_freq = (double)(callback_count1 - callback_count0) / (double)TEST_PERIOD;

  steam_log.Log ( L"Game runs its callbacks at ~%5.2f Hz ... this is %s the "
                  L"minimum threshold for achievement unlock sound.\n",
                    callback_freq, callback_freq > 3.0 ? L"above" : L"below" );

  // If the timing period failed, then start auto-pumping.
  //
  if (callback_freq < 3.0)
  {
    if (SteamAPI_InitSafe_Original ())
    {
      steam_log.Log ( L" >> Installing a callback auto-pump at 4 Hz.\n\n");

      while (true)
      {
        SK::SteamAPI::Pump ();

        SleepEx (250, FALSE);
      }
    }

    else
    {
      steam_log.Log ( L" >> Tried to install a callback auto-pump, but "
                      L"could not initialize SteamAPI.\n\n" );
    }
  }

  CloseHandle (InterlockedExchangePointer ((void **)&hSteamPump, nullptr));

  return 0;
}

void
SK_Steam_StartPump (bool force)
{
  if (InterlockedCompareExchangePointer (&hSteamPump, nullptr, nullptr) != nullptr)
    return;

  if (config.steam.auto_pump_callbacks || force)
  {
    InterlockedExchangePointer ( (void **)&hSteamPump,
                                   CreateThread ( nullptr,
                                                    0,
                                                      SteamAPI_PumpThread,
                                                        nullptr,
                                                          0x00,
                                                            nullptr )
                               );
  }
}

void
SK_Steam_KillPump (void)
{
  CHandle hOriginal (
    InterlockedExchangePointer ((void **)&hSteamPump, nullptr)
  );

  if (hOriginal != nullptr)
  {
    TerminateThread (hOriginal, 0x00);
  }
}


extern const wchar_t*
SK_GetFullyQualifiedApp (void);

std::string
SK_UseManifestToGetAppName (uint32_t appid);

uint32_t
SK::SteamAPI::AppID (void)
{
  ISteamUtils* utils = steam_ctx.Utils ();

  if (utils != nullptr)
  {
    static uint32_t id    = 0;
    static bool     first = true;

    if (first)
    {
      id = utils->GetAppID ();

      if (config.system.central_repository)
      {
        if (id != 0)
        {
          first = false;

          app_cache_mgr.addAppToCache      (SK_GetFullyQualifiedApp (), SK_GetHostApp (), SK_UTF8ToWideChar (SK_UseManifestToGetAppName (id)).c_str (), id);
          app_cache_mgr.saveAppCache       (true);

          app_cache_mgr.loadAppCacheForExe (SK_GetFullyQualifiedApp ());

          // Trigger profile migration if necessary
          app_cache_mgr.getConfigPathForAppID (id);
        }
      }
    }

    // If no AppID was manually set, let's assign one now.
    if (config.steam.appid == 0)
    {
      config.steam.appid = id;
    }

    return id;
  }

  return 0;
}

// Easier to DLL export a flat interface
uint32_t
__stdcall
SK_SteamAPI_AppID (void)
{
  return SK::SteamAPI::AppID ();
}

const wchar_t*
SK_GetSteamDir (void)
{
         DWORD   len         = MAX_PATH;
  static wchar_t wszSteamPath [MAX_PATH];

  LSTATUS status =
    RegGetValueW ( HKEY_CURRENT_USER,
                     L"SOFTWARE\\Valve\\Steam\\",
                       L"SteamPath",
                         RRF_RT_REG_SZ,
                           nullptr,
                             wszSteamPath,
                               (LPDWORD)&len );

  if (status == ERROR_SUCCESS)
    return wszSteamPath;
  else
    return nullptr;
}

std::string
SK_UseManifestToGetAppName (uint32_t appid)
{
  using steam_library_t =
    char* [MAX_PATH * 2];

#define MAX_STEAM_LIBRARIES 16
  static bool            scanned_libs = false;
  static int             steam_libs   = 0;
  static steam_library_t steam_lib_paths [MAX_STEAM_LIBRARIES] = { };

  static const wchar_t* wszSteamPath;

  if (! scanned_libs)
  {
    wszSteamPath =
      SK_GetSteamDir ();

    if (wszSteamPath != nullptr)
    {
      wchar_t wszLibraryFolders [MAX_PATH * 2 + 1] = { };

      lstrcpyW (wszLibraryFolders, wszSteamPath);
      lstrcatW (wszLibraryFolders, L"\\steamapps\\libraryfolders.vdf");

      CHandle hLibFolders (
        CreateFileW ( wszLibraryFolders,
                        GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                            nullptr,
                              OPEN_EXISTING,
                                GetFileAttributesW (wszLibraryFolders),
                                  nullptr )
      );

      if (hLibFolders != INVALID_HANDLE_VALUE)
      {
        DWORD  dwSize     = 0,
               dwSizeHigh = 0,
               dwRead     = 0;

        dwSize =
          GetFileSize (hLibFolders, &dwSizeHigh);

        void* data =
          new uint8_t [dwSize + 1] { };

        if (data == nullptr)
          return nullptr;

        std::unique_ptr <uint8_t> _data ((uint8_t *)data);

        dwRead = dwSize;

        if (ReadFile (hLibFolders, data, dwSize, &dwRead, nullptr))
        {
          for (DWORD i = 0; i < dwSize; i++)
          {
            if (((const char *)data) [i] == '"' && i < dwSize - 3)
            {
              if (((const char *)data) [i + 2] == '"')
                i += 2;
              else if (((const char *)data) [i + 3] == '"')
                i += 3;
              else
                continue;

              char* lib_start = nullptr;

              for (DWORD j = i; j < dwSize; j++,i++)
              {
                if (((char *)data) [j] == '"' && lib_start == nullptr && j < dwSize - 1)
                {
                  lib_start = &((char *)data) [j+1];
                }

                else if (((char *)data) [j] == '"')
                {
                  ((char *)data) [j] = '\0';
                  lstrcpyA ((char *)steam_lib_paths [steam_libs++], lib_start);
                  lib_start = nullptr;
                }
              }
            }
          }
        }
      }
    }

    scanned_libs = true;
  }


  // Search custom library paths first
  if (steam_libs != 0)
  {
    for (int i = 0; i < steam_libs; i++)
    {
      char szManifest [MAX_PATH * 2 + 1] = { };

      sprintf ( szManifest,
                  R"(%s\steamapps\appmanifest_%lu.acf)",
                    (char *)steam_lib_paths [i],
                      appid );

      CHandle hManifest (
        CreateFileA ( szManifest,
                      GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr,
                            OPEN_EXISTING,
                              GetFileAttributesA (szManifest),
                                nullptr )
      );

      if (hManifest != INVALID_HANDLE_VALUE)
      {
        DWORD  dwSize     = 0,
               dwSizeHigh = 0,
               dwRead     = 0;

        dwSize =
          GetFileSize (hManifest, &dwSizeHigh);

        auto* szManifestData =
          new char [dwSize + 1] { };

        if (! szManifestData)
          continue;

        std::unique_ptr <char> manifest_data (szManifestData);

        bool bRead =
          ReadFile ( hManifest,
                       szManifestData,
                         dwSize,
                           &dwRead,
                             nullptr );

        if (! (bRead && dwRead))
        {
          continue;
        }

        char* szAppName =
          StrStrIA (szManifestData, R"("name")");

        char szGameName [513] = { };

        if (szAppName != nullptr)
        {
          // Make sure everything is lowercase
          memcpy (szAppName, R"("name")", 6);

          sscanf ( szAppName,
                     R"("name" "%512[^"]")",
                       szGameName );

          return szGameName;
        }
      }
    }
  }


  char szManifest [MAX_PATH * 2 + 1] = { };

  sprintf ( szManifest,
              R"(%ls\steamapps\appmanifest_%lu.acf)",
                wszSteamPath,
                  appid );

  CHandle hManifest (
    CreateFileA ( szManifest,
                  GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                      nullptr,
                        OPEN_EXISTING,
                          GetFileAttributesA (szManifest),
                            nullptr )
  );

  if (hManifest != INVALID_HANDLE_VALUE)
  {
    DWORD dwSize     = 0,
          dwSizeHigh = 0,
          dwRead     = 0;

    dwSize =
      GetFileSize (hManifest, &dwSizeHigh);

    auto* szManifestData =
      new char [dwSize + 1] { };

    if (szManifestData == nullptr)
    {
      return "";
    }

    std::unique_ptr <char> manifest_data (szManifestData);

    bool bRead =
      ReadFile ( hManifest,
                   szManifestData,
                     dwSize,
                       &dwRead,
                         nullptr );

    if (! (bRead && dwRead))
    {
      return "";
    }

    char* szAppName =
      StrStrIA (szManifestData, R"("name")");

    char szGameName [513] = { };

    if (szAppName != nullptr)
    {
      *szAppName = '\0';

      // Make sure everything is lowercase
      memcpy (szAppName, R"("name")", 6);

      sscanf ( szAppName,
                 R"("name" "%512[^"]")",
                   szGameName );

      return szGameName;
    }
  }

  return "";
}

std::string
SK::SteamAPI::AppName (void)
{
  if (AppID () != 0x00)
  {
    // Only do this once, the AppID never changes =P
    static std::string app_name =
      SK_UseManifestToGetAppName (AppID ());

    return app_name;
  }

  return "";
}

void
__stdcall
SK::SteamAPI::SetOverlayState (bool active)
{
  if (config.steam.silent)
    return;

  EnterCriticalSection (&callback_cs);

  __try
  {
    GameOverlayActivated_t state;
    state.m_bActive = active;
    overlay_state   = active;

    auto it =
      overlay_activation_callbacks.begin ();

    while (it != overlay_activation_callbacks.end ())
    {
      (*it++)->Run (&state);
    }
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }

  LeaveCriticalSection (&callback_cs);
}

bool
__stdcall
SK::SteamAPI::GetOverlayState (bool real)
{
  return real ? SK_IsSteamOverlayActive () :
                  overlay_state;
}

bool
__stdcall
SK::SteamAPI::TakeScreenshot (void)
{
  ISteamScreenshots* pScreenshots =
    steam_ctx.Screenshots ();

  if (pScreenshots != nullptr)
  {
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
  // In case we already initialized stuff...
  if (InterlockedExchangeAdd (&__SK_Steam_init, 0))
    return SteamAPI_InitSafe_Original ();

  EnterCriticalSection (&init_cs);

  static int init_tries = -1;

  if (++init_tries == 0)
  {
    steam_log.Log ( L"Initializing SteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"----------(InitSafe)-----------\n");
  }

  if ( SteamAPI_InitSafe_Original () )
  {
    InterlockedIncrement (&__SK_Steam_init);

#ifdef _WIN64
  const wchar_t* steam_dll_str    = L"steam_api64.dll";
#else
  const wchar_t* steam_dll_str = L"steam_api.dll";
#endif

    HMODULE hSteamAPI = LoadLibraryW_Original (steam_dll_str);

    if (! steam_ctx.UserStats ())
      steam_ctx.InitSteamAPI (hSteamAPI);

    steam_log.Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                      init_tries + 1,
                        SK::SteamAPI::AppID () );

    SK_Steam_StartPump ();

    LeaveCriticalSection (&init_cs);
    return true;
  }

  LeaveCriticalSection (&init_cs);
  return false;
}

void
S_CALLTYPE
SteamAPI_Shutdown_Detour (void)
{
  steam_log.Log (L" *** Game called SteamAPI_Shutdown (...)");

  EnterCriticalSection         (&init_cs);
  SK_Steam_KillPump            ();
  LeaveCriticalSection         (&init_cs);

  steam_ctx.Shutdown           ();

  CreateThread (nullptr, 0,
    [](LPVOID user) ->
    DWORD
    {
      UNREFERENCED_PARAMETER (user);

      for (int i = 0; i < 100; i++)
      {
        SleepEx (100UL, TRUE);

        extern volatile ULONG        __SK_DLL_Ending;
        if (InterlockedExchangeAdd (&__SK_DLL_Ending, 0))
        {
          CloseHandle (GetCurrentThread ());

          return 0;
        }
      }

      // Start back up again :)
      //
      //  >> Stupid hack for The Witcher 3
      //
      SteamAPI_RunCallbacks_Detour ();

      CloseHandle (GetCurrentThread ());

      return 0;
    }, nullptr, 0x00, nullptr);
}

bool
S_CALLTYPE
SteamAPI_Init_Detour (void)
{
  // In case we already initialized stuff...
  if (InterlockedCompareExchange (&__SK_Steam_init, FALSE, FALSE))
    return true;

  EnterCriticalSection (&init_cs);

  static int init_tries = -1;

  if (++init_tries == 0)
  {
    steam_log.Log ( L"Initializing SteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"-------------------------------\n");
  }

  if (SteamAPI_Init_Original ())
  {
    InterlockedIncrement (&__SK_Steam_init);

#ifdef _WIN64
  const wchar_t* steam_dll_str    = L"steam_api64.dll";
#else
  const wchar_t* steam_dll_str = L"steam_api.dll";
#endif

    HMODULE hSteamAPI = GetModuleHandleW (steam_dll_str);

    steam_ctx.InitSteamAPI (hSteamAPI);

    steam_log.Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                      init_tries + 1,
                        SK::SteamAPI::AppID () );

    SK_Steam_StartPump ();

    LeaveCriticalSection (&init_cs);
    return true;
  }

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
SK_SteamAPI_GetOverlayState (bool real)
{
  return SK::SteamAPI::GetOverlayState (real);
}

bool
__stdcall
SK_SteamAPI_TakeScreenshot (void)
{
  return SK::SteamAPI::TakeScreenshot ();
}

void
SK_Steam_InitCommandConsoleVariables (void)
{
  steam_log.init (L"logs/steam_api.log", L"w");
  steam_log.silent = config.steam.silent;

  InitializeCriticalSectionAndSpinCount (&callback_cs, 1024UL);
  InitializeCriticalSectionAndSpinCount (&popup_cs,    4096UL);
  InitializeCriticalSectionAndSpinCount (&init_cs,     1024UL);

  SK_ICommandProcessor* cmd =
    SK_GetCommandProcessor ();

  cmd->AddVariable ("Steam.TakeScreenshot", SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.take_screenshot));
  cmd->AddVariable ("Steam.ShowPopup",      SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.popup.show));
  cmd->AddVariable ("Steam.PopupDuration",  SK_CreateVar (SK_IVariable::Int,     (int  *)&config.steam.achievements.popup.duration));
  cmd->AddVariable ("Steam.PopupInset",     SK_CreateVar (SK_IVariable::Float,   (float*)&config.steam.achievements.popup.inset));
  cmd->AddVariable ("Steam.ShowPopupTitle", SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.popup.show_title));
  cmd->AddVariable ("Steam.PopupAnimate",   SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.popup.animate));
  cmd->AddVariable ("Steam.PlaySound",      SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.steam.achievements.play_sound));

  steam_ctx.popup_origin = SK_CreateVar (SK_IVariable::String, steam_ctx.var_strings.popup_origin, &steam_ctx);
  cmd->AddVariable ("Steam.PopupOrigin",    steam_ctx.popup_origin);

  steam_ctx.notify_corner = SK_CreateVar (SK_IVariable::String, steam_ctx.var_strings.notify_corner, &steam_ctx);
  cmd->AddVariable ("Steam.NotifyCorner",   steam_ctx.notify_corner);

  steam_ctx.tbf_pirate_fun = SK_CreateVar (SK_IVariable::Float, &steam_ctx.tbf_float, &steam_ctx);
}

DWORD
WINAPI
SteamAPI_Delay_Init (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  if (! SK_IsInjected ())
  {
    CloseHandle (GetCurrentThread ());

    return 0;
  }

  int tries = 0;

  while ( (! InterlockedExchangeAddAcquire (&__SK_Steam_init, 0)) &&
            tries < 120 )
  {
    SleepEx (config.steam.init_delay, FALSE);

    if (InterlockedExchangeAddRelease (&__SK_Steam_init, 0))
      break;

    if (SteamAPI_InitSafe_Original != nullptr)
      SteamAPI_InitSafe_Detour ();

    ++tries;
  }

  CloseHandle (GetCurrentThread ());

  return 0;
}

void
SK_HookSteamAPI (void)
{
#ifdef _WIN64
    const wchar_t* wszSteamAPI = L"steam_api64.dll";
#else
    const wchar_t* wszSteamAPI = L"steam_api.dll";
#endif
  SK::SteamAPI::steam_size = SK_GetFileSize (wszSteamAPI);

  if (config.steam.silent)
    return;

  if (! GetModuleHandle (wszSteamAPI))
    return;

  EnterCriticalSection (&init_cs);
  if (InterlockedCompareExchange (&__SteamAPI_hook, TRUE, FALSE))
  {
    LeaveCriticalSection (&init_cs);
    return;
  }

  steam_log.Log (L"%s was loaded, hooking...", wszSteamAPI);

    SK_CreateDLLHook2 ( wszSteamAPI,
                        "SteamAPI_InitSafe",
                         SteamAPI_InitSafe_Detour,
static_cast_p2p <void> (&SteamAPI_InitSafe_Original),
static_cast_p2p <void> (&SteamAPI_InitSafe) );

    SK_CreateDLLHook2 ( wszSteamAPI,
                        "SteamAPI_Init",
                         SteamAPI_Init_Detour,
static_cast_p2p <void> (&SteamAPI_Init_Original),
static_cast_p2p <void> (&SteamAPI_Init) );

    SK_CreateDLLHook2 ( wszSteamAPI,
                        "SteamAPI_RegisterCallback",
                         SteamAPI_RegisterCallback_Detour,
static_cast_p2p <void> (&SteamAPI_RegisterCallback_Original),
static_cast_p2p <void> (&SteamAPI_RegisterCallback) );

    SK_CreateDLLHook2 ( wszSteamAPI,
                        "SteamAPI_UnregisterCallback",
                         SteamAPI_UnregisterCallback_Detour,
static_cast_p2p <void> (&SteamAPI_UnregisterCallback_Original),
static_cast_p2p <void> (&SteamAPI_UnregisterCallback) );

    SK_CreateDLLHook2 ( wszSteamAPI,
                        "SteamAPI_RunCallbacks",
                         SteamAPI_RunCallbacks_Detour,
static_cast_p2p <void> (&SteamAPI_RunCallbacks_Original),
static_cast_p2p <void> (&SteamAPI_RunCallbacks) );

    //
    // Do not queue these up (by calling CreateDLLHook2),
    //   they will be installed only upon the game successfully
    //     calling one of the SteamAPI initialization functions.
    //
    SK_CreateDLLHook  ( wszSteamAPI,
                        "SteamAPI_Shutdown",
                         SteamAPI_Shutdown_Detour,
static_cast_p2p <void> (&SteamAPI_Shutdown_Original),
static_cast_p2p <void> (&SteamAPI_Shutdown) );

  SK_ApplyQueuedHooks ();

  if (! config.steam.force_load_steamapi)
  {
    if (config.steam.init_delay > 0)
    {
      CreateThread (nullptr, 0, SteamAPI_Delay_Init, nullptr, 0x00, nullptr);
    }
  }

  else
  {
    CreateThread (nullptr, 0, [](LPVOID user) ->
      DWORD
      {
        UNREFERENCED_PARAMETER (user);

        SteamAPI_InitSafe_Detour ();

        CloseHandle (GetCurrentThread ());

        return 0;
      }, nullptr, 0x00, nullptr);
  }

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

void
SK_SteamAPI_InitManagers (void)
{
  if (! InterlockedCompareExchange (&SK_SteamAPI_ManagersInitialized, 1, 0))
  {
    has_global_data = false;
    next_friend     = 0;

    ISteamUserStats* stats =
      steam_ctx.UserStats ();

    if (stats != nullptr)
    {
      if (stats->GetNumAchievements ())
      {
        steam_log.Log (L" Creating Achievement Manager...");

        steam_achievements = new SK_Steam_AchievementManager (
          config.steam.achievements.sound_file.c_str     ()
        );

        steam_achievements->requestStats ();
      }

      steam_log.LogEx (false, L"\n");

      user_manager         = new SK_Steam_UserManager    ();
    }

    if (steam_ctx.Utils ())
      overlay_manager      = new SK_Steam_OverlayManager ();

    // Failed, try again later...
    if (overlay_manager == nullptr && steam_achievements == nullptr)
      InterlockedExchange (&SK_SteamAPI_ManagersInitialized, 0);
  }
}

void
SK_SteamAPI_DestroyManagers (void)
{
  if (InterlockedCompareExchange (&SK_SteamAPI_ManagersInitialized, 0, 1))
  {
    if (steam_achievements != nullptr)
    {
      delete steam_achievements;
             steam_achievements = nullptr;
    }

    if (overlay_manager != nullptr)
    {
      delete overlay_manager;
             overlay_manager = nullptr;
    }

    if (user_manager != nullptr)
    {
      delete user_manager;
             user_manager = nullptr;
    }
  }
}


size_t
SK_SteamAPI_GetNumPossibleAchievements (void)
{
  size_t possible = 0;

  if (steam_achievements != nullptr)
    steam_achievements->getAchievements (&possible);

  return possible;
}

std::vector <SK_SteamAchievement *>&
SK_SteamAPI_GetAllAchievements (void)
{
  static std::vector <SK_SteamAchievement *> achievements;

  size_t num;

  SK_SteamAchievement** ppAchv =
    steam_achievements->getAchievements (&num);

  if (achievements.size () != num)
  {
    achievements.clear ();

    for (size_t i = 0; i < num; i++)
      achievements.push_back (ppAchv [i]);
  }

  return achievements;
}

std::vector <SK_SteamAchievement *>&
SK_SteamAPI_GetUnlockedAchievements (void)
{
  static std::vector <SK_SteamAchievement *> unlocked_achievements;

  unlocked_achievements.clear ();

  std::vector <SK_SteamAchievement *>& achievements =
    SK_SteamAPI_GetAllAchievements ();

  for ( auto it : achievements )
  {
    if (it->unlocked_)
      unlocked_achievements.push_back (it);
  }

  return unlocked_achievements;
}

std::vector <SK_SteamAchievement *>&
SK_SteamAPI_GetLockedAchievements (void)
{
  static std::vector <SK_SteamAchievement *> locked_achievements;

  locked_achievements.clear ();

  std::vector <SK_SteamAchievement *>& achievements =
    SK_SteamAPI_GetAllAchievements ();

  for ( auto it : achievements )
  {
    if (! it->unlocked_)
      locked_achievements.push_back (it);
  }

  return locked_achievements;
}

float
SK_SteamAPI_GetUnlockedPercentForFriend (uint32_t friend_idx)
{
  return steam_achievements->getFriendUnlockPercentage (friend_idx);
}

size_t
SK_SteamAPI_GetUnlockedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats)
{
  size_t num_achvs = SK_SteamAPI_GetNumPossibleAchievements ();
  size_t unlocked  = 0;

  for (size_t i = 0; i < num_achvs; i++)
  {
    if (steam_achievements->hasFriendUnlockedAchievement (friend_idx, (uint32_t)i))
    {
      if (pStats != nullptr)
        pStats [i] = TRUE;

      unlocked++;
    }

    else if (pStats != nullptr)
      pStats [i] = FALSE;
  }

  return unlocked;
}

size_t
SK_SteamAPI_GetLockedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats)
{
  size_t num_achvs = SK_SteamAPI_GetNumPossibleAchievements ();
  size_t locked  = 0;

  for (size_t i = 0; i < num_achvs; i++)
  {
    if (! steam_achievements->hasFriendUnlockedAchievement (friend_idx, (uint32_t)i))
    {
      if (pStats != nullptr)
        pStats [i] = TRUE;

      locked++;
    }

    else if (pStats != nullptr)
      pStats [i] = FALSE;
  }

  return locked;
}

size_t
SK_SteamAPI_GetSharedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats)
{
  std::vector <BOOL> friend_stats;
  friend_stats.resize (SK_SteamAPI_GetNumPossibleAchievements ());

//  size_t unlocked =
    SK_SteamAPI_GetUnlockedAchievementsForFriend (friend_idx, friend_stats.data ());

  size_t shared = 0;

  std::vector <SK_SteamAchievement *>&
    unlocked_achvs = SK_SteamAPI_GetUnlockedAchievements ();

  if (pStats != nullptr)
    ZeroMemory (pStats, SK_SteamAPI_GetNumPossibleAchievements ());

  for ( auto it : unlocked_achvs )
  {
    if (friend_stats [it->idx_])
    {
      if (pStats != nullptr)
        pStats [it->idx_] = TRUE;

      shared++;
    }
  }

  return shared;
}


// Returns true if all friend stats have been pulled from the server
bool
__stdcall
SK_SteamAPI_FriendStatsFinished (void)
{
  return (next_friend >= friend_count);
}

// Percent (0.0 - 1.0) of friend achievement info fetched
float
__stdcall
SK_SteamAPI_FriendStatPercentage (void)
{
  if (SK_SteamAPI_FriendStatsFinished ())
    return 1.0f;

  return static_cast <float> (next_friend) /
         static_cast <float> (friend_count);
}

const char*
__stdcall
SK_SteamAPI_GetFriendName (uint32_t idx, size_t* pLen)
{
  const std::string& name =
    steam_achievements->getFriendName (idx);

  if (pLen != nullptr)
    *pLen = name.length ();

  // Not the most sensible place to implement this, but ... whatever
  return name.c_str ();
}

int
__stdcall
SK_SteamAPI_GetNumFriends (void)
{
  return friend_count;
}


bool steam_imported = false;

bool
SK_SteamImported (void)
{
#ifdef _WIN64
  const wchar_t* steam_dll_str = L"steam_api64.dll";
#else
  const wchar_t* steam_dll_str = L"steam_api.dll";
#endif

  return steam_imported || GetModuleHandle (steam_dll_str)      ||
                           GetModuleHandle (L"CSteamworks.dll") ||
                           GetModuleHandle (L"SteamNative.dll");
}

void
SK_TestSteamImports (HMODULE hMod)
{
#ifdef _WIN64
  static const wchar_t* steam_dll_str = L"steam_api64.dll";
#else
  static const wchar_t* steam_dll_str = L"steam_api.dll";
#endif

  sk_import_test_s steam_api [] = { 
#ifdef _WIN64
                                    { "steam_api64.dll", false } };
#else
                                    { "steam_api.dll",   false } };
#endif

  SK_TestImports (hMod, steam_api, sizeof (steam_api) / sizeof sk_import_test_s);

  if (steam_api->used)
  {
    SK_HookSteamAPI ();
    steam_imported = true;
  }

  else
  {
    if (GetModuleHandle (L"CSteamworks.dll"))
    {
      SK_HookCSteamworks ();
      steam_imported = true;
    }

    else if (! SK_IsInjected ())
    {
      if ( LoadLibraryW_Original (steam_dll_str) ||
           GetModuleHandle       (L"SteamNative.dll") )
      {
         SK_HookSteamAPI ();
         steam_imported = true;
      }
    }
  }

  if ( LoadLibraryW_Original (steam_dll_str) ||
       GetModuleHandle       (L"SteamNative.dll") )
  {
     SK_HookSteamAPI ();
     steam_imported = true;
  }
}

void
WINAPI
SK_SteamAPI_AddScreenshotToLibrary (const char *pchFilename, const char *pchThumbnailFilename, int nWidth, int nHeight)
{
  if (steam_ctx.Screenshots ())
    steam_ctx.Screenshots ()->AddScreenshotToLibrary (pchFilename, pchThumbnailFilename, nWidth, nHeight);
}

void
WINAPI
SK_SteamAPI_WriteScreenshot (void *pubRGB, uint32 cubRGB, int nWidth, int nHeight)
{
  if (steam_ctx.Screenshots ())
    steam_ctx.Screenshots ()->WriteScreenshot (pubRGB, cubRGB, nWidth, nHeight);
}

HMODULE hModOverlay;

bool
SK_Steam_LoadOverlayEarly (void)
{
  const wchar_t* wszSteamPath =
      SK_GetSteamDir ();

  wchar_t wszOverlayDLL [MAX_PATH + 2] = { };

  lstrcatW (wszOverlayDLL, wszSteamPath);

#ifdef _WIN64
  lstrcatW (wszOverlayDLL, L"\\GameOverlayRenderer64.dll");
#else
  lstrcatW (wszOverlayDLL, L"\\GameOverlayRenderer.dll");
#endif

  hModOverlay =
    LoadLibraryW (wszOverlayDLL);

  return hModOverlay != nullptr;
}


bool SK::SteamAPI::overlay_state = false;


void
__stdcall
SK_SteamAPI_UpdateNumPlayers (void)
{
  if (user_manager != nullptr)
    user_manager->UpdateNumPlayers ();
}

int32_t
__stdcall
SK_SteamAPI_GetNumPlayers (void)
{
  if (user_manager != nullptr)
  {
    int32 num = user_manager->GetNumPlayers ();

    if (num <= 1)
      user_manager->UpdateNumPlayers ();

    return std::max (1, user_manager->GetNumPlayers ());
  }

  return 1; // You, and only you, apparently ;)
}

void
__stdcall
SK::SteamAPI::UpdateNumPlayers (void)
{
  SK_SteamAPI_UpdateNumPlayers ();
}

int32_t
__stdcall
SK::SteamAPI::GetNumPlayers (void)
{
  return SK_SteamAPI_GetNumPlayers ();
}

float
__stdcall
SK_SteamAPI_PercentOfAchievementsUnlocked (void)
{
  if (steam_achievements != nullptr)
    return steam_achievements->getPercentOfAchievementsUnlocked ();

  return 0.0f;
}

float
__stdcall
SK::SteamAPI::PercentOfAchievementsUnlocked (void)
{
  return SK_SteamAPI_PercentOfAchievementsUnlocked ();
}

ISteamUtils*
SK_SteamAPI_Utils (void)
{
  return steam_ctx.Utils ();
}

ISteamMusic*
SK_SteamAPI_Music (void)
{
  return steam_ctx.Music ();
}


//
// ATTENTION PIRATES:  Forking Special K and circumventing this is 99% of the work you need
// ------------------    to do to "crack" my mods. The remaining 1% involves removing my name.
//
//
//  You will find various measures scattered throughout the code that took me all of five
//    minutes to devise whose intention is to make any attempt to do something other than
//      modify the source code fail.
//
//  >> TBFix, for example, will throw up a HUGE positive LOD bias causing blurry textures if
//       the framerate limit restriction is bypassed with a simple hex edit.
//
//  Please refrain from simply binary-editing one of my distributions, if you have the skill to
//    circumvent DRM it should be obvious that I intend every one of my measures to be plainly
//      visible and should take you thirty-seconds to figure out and remove the correct way.
//
//
//
//  "Proper" (lol) Redistribution for Illegal Purposes (which I do not condone) follows:
//  ------------------------------------------------------------------------------------
//
//    1.  Fork the C++ code, make your changes public for the decency of your community
//    2.  Remove my name if you are going to distribute your modification in anything illegal
//    3.  Stop accusing me of distributing malware, you rarely even honor bullet-point #1
//
//
//
//  Common Sense Practices it Frequently Astounds me that you Ignore:
//  -----------------------------------------------------------------
//
//   (*) Disable SteamAPI enhancement, most of the stuff you complain about isn't even a problem
//        if you would simply turn SteamAPI features off.
//
//    >> Your fake SteamAPI implementations are fragile, and I sure as hell have no intention to
//         test against them or fix them.
//
//    >> Opt-out of Steam-related enhancement if you care about stability.
//
//
//   (*) Do not grab the latest version of my mod and shoe-horn it into your cracked binary
//
//    >> I maintain forward-compatibility for the benefit of the paying Steam community,
//         you need the opposite since your executable never receives updates.
//
//    >> I remove fixes for stuff that the publisher has already fixed, you get nothing from
//         following my update schedule when your executable has its own set of problems that
//           are not addressed by new versions of the software.
//
//
//  The feature that deletes files is for the benefit of people who "upgrade" from pirated to
//  legit and swear up and down they are continuing to be detected as a pirate. Don't click that
//  button unless you are a legitimate user who upgraded and has remnant illegal files lingering.
//
//
//-----------------------------------------------------------------------------------------------
//    Remove my name from any forked work, and stop making your community look like a bunch of
//      children; it reflects poorly on the whole of PC gaming.
//-----------------------------------------------------------------------------------------------
//
static uint32_t        verdict        = 0x00;
static bool            decided        = false;

static SteamAPICall_t  hAsyncSigCheck = 0; // File Signature Validation

enum class SK_Steam_FileSigPass_e
{
  Executable = 0,
  SteamAPI   = 1,
  DLC        = 2,

  Done       = MAXINT
};

static SK_Steam_FileSigPass_e
                       validation_pass (SK_Steam_FileSigPass_e::Executable);

uint32_t
__stdcall
SK_Steam_PiratesAhoy (void)
{
#if 0
  if (validation_pass != SK_Steam_FileSigPass_e::Done)
  {
    if ( InterlockedCompareExchange (&hAsyncSigCheck, 1, 0) == 0 && steam_ctx.Utils () != nullptr && (! config.steam.silent) )
    {
      switch (validation_pass)
      {
        case SK_Steam_FileSigPass_e::Executable:
        {
          InterlockedExchange (&hAsyncSigCheck,
            steam_ctx.Utils ()->CheckFileSignature (SK_WideCharToUTF8 (SK_GetHostApp ()).c_str ()));
        } break;

        case SK_Steam_FileSigPass_e::SteamAPI:
        {
          char szRelSteamAPI [MAX_PATH * 2] = { };

          // Generally, though not always (i.e. CSteamWorks) we will link against one of these two DLLs
          //
          //   The actual DLL used is pulled from the IAT during init, but I am too lazy to bother doing
          //     this the right way ;)
#ifdef _WIN64
          snprintf ( szRelSteamAPI, MAX_PATH * 2 - 1, "%ws\\steam_api64.dll",
                       SK_GetHostPath () );
#else
          snprintf ( szRelSteamAPI, MAX_PATH * 2 - 1, "%ws\\steam_api.dll",
                       SK_GetHostPath () );
#endif
          InterlockedExchange (&hAsyncSigCheck,
            steam_ctx.Utils ()->CheckFileSignature (szRelSteamAPI));
        } break;
      }

      steam_ctx.chk_file_sig.Set (
        hAsyncSigCheck,
          &steam_ctx,
            &SK_SteamAPIContext::OnFileSigDone );
    }
  }
#endif

  if (decided)
  {
    return verdict;
  }

#ifdef _WIN64
  static uint32_t crc32_steamapi =
    SK_GetFileCRC32C (L"steam_api64.dll");
#else
  static uint32_t crc32_steamapi =
    SK_GetFileCRC32C (L"steam_api.dll");
#endif

  if (SK::SteamAPI::steam_size > 0 && SK::SteamAPI::steam_size < (1024 * 92))
  {
    verdict = 0x68992;
  }

  // CPY
  if (steam_ctx.User () != nullptr)
  {
    if ( steam_ctx.User ()->GetSteamID ().ConvertToUint64 () == 18295873490452480 << 4 ||
         steam_ctx.User ()->GetSteamID ().ConvertToUint64 () == 25520399320093309  * 3 )
      verdict = 0x1;
  }

  if (crc32_steamapi == 0x28140083 << 1)
    verdict = crc32_steamapi;

  decided = true;

  // User opted out of Steam enhancement, no further action necessary
  if (config.steam.silent && verdict)
  {
    validation_pass = SK_Steam_FileSigPass_e::Done;
    verdict         = 0x00;
  }

  return verdict;
}

uint32_t
__stdcall
SK_Steam_PiratesAhoy2 (void)
{
  return SK_Steam_PiratesAhoy ();
}

void
SK_SteamAPIContext::OnFileSigDone ( CheckFileSignature_t* pParam,
                                    bool                  bFailed )
{
  ECheckFileSignature result =
    bFailed ? k_ECheckFileSignatureNoSignaturesFoundForThisApp :
              pParam->m_eCheckFileSignature;


  auto HandleResult =
    [&](const wchar_t* wszFileName)
    {
      switch (result)
      {
        case k_ECheckFileSignatureNoSignaturesFoundForThisApp:
        {
          steam_log.Log ( L"> SteamAPI Application Signature Verification:  "
                          L" No Signatures For This App" );
        } break;

        case k_ECheckFileSignatureNoSignaturesFoundForThisFile:
        {
          steam_log.Log ( L"> SteamAPI Application Signature Verification:  "
                          L" No Signature For This App File :: '%ws'",
                            wszFileName );
        } break;

        case k_ECheckFileSignatureFileNotFound:
        {
          steam_log.Log ( L"> SteamAPI Application Signature Verification:  "
                          L" Signature File Not Found :: '%ws'",
                            wszFileName );
        } break;

        case k_ECheckFileSignatureInvalidSignature:
        {
          steam_log.Log ( L"> SteamAPI Application Signature Verification:  "
                          L" INVALID File Signature :: '%ws'",
                            wszFileName );
          if ( validation_pass == SK_Steam_FileSigPass_e::SteamAPI ||
               validation_pass == SK_Steam_FileSigPass_e::DLC )
          {
            verdict |= ~( k_ECheckFileSignatureInvalidSignature );
            decided  =    true;
          }
        } break;

        case k_ECheckFileSignatureValidSignature:
        {
          steam_log.Log ( L"> SteamAPI Application Signature Verification:  "
                          L" VALID File Signature :: '%ws'",
                            wszFileName );
        } break;

        default:
        {
          steam_log.Log ( L"> SteamAPI Application Signature Verification:  "
                          L" UNKNOWN STATUS (%lu) :: '%ws'",
                            result, wszFileName );
        } break;
      }
    };


  switch (validation_pass)
  {
    case SK_Steam_FileSigPass_e::Executable:
    {
      HandleResult (SK_GetHostApp ());

      validation_pass = SK_Steam_FileSigPass_e::SteamAPI;
      InterlockedExchange (&hAsyncSigCheck, 0);
    } break;

    case SK_Steam_FileSigPass_e::SteamAPI:
    {
      HandleResult (L"SteamAPI");

      validation_pass = SK_Steam_FileSigPass_e::Done;
      InterlockedExchange (&hAsyncSigCheck, 0);
    } break;
  }


  chk_file_sig.Cancel ();
}


//
// Detoured to filter out async call failure events that the game isn't expecting and
//   may respond by shutting SteamAPI down if received.
//
extern "C"
void
__fastcall
SteamAPI_UserStatsReceived_Detour ( CCallbackBase* This, UserStatsReceived_t* pParam )
{
  if (config.system.log_level > 2)
  {
    dll_log.Log (L"Result: %li - User: %llx, Game: %llu  [Callback: %lu]", pParam->m_eResult,
      pParam->m_steamIDUser.ConvertToUint64 (), pParam->m_nGameID, pParam->k_iCallback );
  }

#ifdef _WIN64
  if (          pParam->m_steamIDUser       != steam_ctx.User  ()->GetSteamID () ||
       CGameID (pParam->m_nGameID).AppID () != steam_ctx.Utils ()->GetAppID   () )
  {
    // Default Filter:  Remove Everything not This Game or This User
    //
    //                    If a game ever queries data for other users, this policy must change
    return;
  }
#else
  static CSteamID last_user;

  // This message only means that data has been received, not
  //   what that data is... so we can spoof the receipt of the player's
  //     data over and over if need be.
  if (pParam->m_steamIDUser != SK::SteamAPI::player)
  {
    pParam->m_nGameID     = 0;
    pParam->m_steamIDUser = last_user;
  }
  else
    last_user = pParam->m_steamIDUser;
#endif

  SteamAPI_UserStatsReceived_Original (This, pParam);
}



// Fallback to Win32 API if the Steam overlay is not functioning.
#include <SpecialK/window.h>

bool
__stdcall
SK_SteamOverlay_GoToURL (const char* szURL, bool bUseWindowsShellIfOverlayFails)
{
  if (steam_ctx.Utils () != nullptr && steam_ctx.Utils ()->IsOverlayEnabled ())
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToWebPage (szURL);
      return true;
    }
  }

  if (bUseWindowsShellIfOverlayFails)
  {
    ShellExecuteA (game_window.hWnd, "open", szURL, nullptr, nullptr, SW_NORMAL);
    return true;
  }

  return false;
}

bool
__stdcall
SK_SteamOverlay_GoToFriendProfile (CSteamID friend_sid)
{
  if (steam_ctx.Utils () != nullptr && steam_ctx.Utils ()->IsOverlayEnabled ())
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToUser ("steamid", friend_sid);
      return true;
    }
  }

  return false;
}

bool
__stdcall
SK_SteamOverlay_GoToFriendAchievements (CSteamID friend_sid)
{
  if (steam_ctx.Utils () != nullptr && steam_ctx.Utils ()->IsOverlayEnabled ())
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToUser ("achievements", friend_sid);
      return true;
    }
  }

  return false;
}

bool
__stdcall
SK_SteamOverlay_GoToFriendStats (CSteamID friend_sid)
{
  if (steam_ctx.Utils () != nullptr && steam_ctx.Utils ()->IsOverlayEnabled ())
  {
    if (steam_ctx.Friends () != nullptr)
    {
      steam_ctx.Friends ()->ActivateGameOverlayToUser ("stats", friend_sid);
      return true;
    }
  }

  return false;
}



ISteamMusic*
SAFE_GetISteamMusic (ISteamClient* pClient, HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
{
  __try {
    return pClient->GetISteamMusic (hSteamuser, hSteamPipe, pchVersion);
  }
  __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}


SteamAPI_RunCallbacks_pfn          SteamAPI_RunCallbacks                = nullptr;
SteamAPI_RunCallbacks_pfn          SteamAPI_RunCallbacks_Original       = nullptr;

SteamAPI_RegisterCallback_pfn      SteamAPI_RegisterCallback            = nullptr;
SteamAPI_RegisterCallback_pfn      SteamAPI_RegisterCallback_Original   = nullptr;

SteamAPI_UnregisterCallback_pfn    SteamAPI_UnregisterCallback          = nullptr;
SteamAPI_UnregisterCallback_pfn    SteamAPI_UnregisterCallback_Original = nullptr;

SteamAPI_RegisterCallResult_pfn    SteamAPI_RegisterCallResult          = nullptr;
SteamAPI_UnregisterCallResult_pfn  SteamAPI_UnregisterCallResult        = nullptr;

SteamAPI_Init_pfn                  SteamAPI_Init                        = nullptr;
SteamAPI_InitSafe_pfn              SteamAPI_InitSafe                    = nullptr;

SteamAPI_RestartAppIfNecessary_pfn SteamAPI_RestartAppIfNecessary       = nullptr;
SteamAPI_IsSteamRunning_pfn        SteamAPI_IsSteamRunning              = nullptr;

SteamAPI_GetHSteamUser_pfn         SteamAPI_GetHSteamUser               = nullptr;
SteamAPI_GetHSteamPipe_pfn         SteamAPI_GetHSteamPipe               = nullptr;

SteamClient_pfn                    SteamClient                          = nullptr;

SteamAPI_Shutdown_pfn              SteamAPI_Shutdown                    = nullptr;
SteamAPI_Shutdown_pfn              SteamAPI_Shutdown_Original           = nullptr;

GetControllerState_pfn             GetControllerState_Original          = nullptr;


uint64_t    SK::SteamAPI::steam_size                                    = 0ULL;

// Must be global for x86 ABI problems
CSteamID    SK::SteamAPI::player (0ULL);