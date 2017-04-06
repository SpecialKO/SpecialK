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
//#define ISOLATION_AWARE_ENABLED 1
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
#include <SpecialK/diagnostics/compatibility.h>

#include <SpecialK/osd/popup.h>
#include <SpecialK/osd/text.h>
#include <SpecialK/render_backend.h>

#include <SpecialK/command.h>

// PlaySound
#pragma comment (lib, "winmm.lib")

iSK_Logger steam_log;

volatile ULONG __SK_Steam_init = FALSE;

uint64_t steam_size;

// We're not going to use DLL Import - we will load these function pointers
//  by hand.
#define STEAM_API_NODLL
#include <SpecialK/steam_api.h>


#include <time.h>

// To spoof Overlay Activation (pause the game)
#include <set>
#include <unordered_map>

std::multiset <class CCallbackBase *> overlay_activation_callbacks;

void SK_HookSteamAPI                      (void);
void SK_SteamAPI_InitManagers             (void);
void SK_SteamAPI_DestroyManagers          (void);
void SK_SteamAPI_UpdateGlobalAchievements (void);

void StartSteamPump (bool force = false);

volatile ULONG __CSteamworks_hook = FALSE;
volatile ULONG __SteamAPI_hook    = FALSE;


CRITICAL_SECTION callback_cs = { 0 };
CRITICAL_SECTION init_cs     = { 0 };
CRITICAL_SECTION popup_cs    = { 0 };


#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>


bool S_CALLTYPE SteamAPI_InitSafe_Detour (void);
bool S_CALLTYPE SteamAPI_Init_Detour     (void);
void S_CALLTYPE SteamAPI_Shutdown_Detour (void);

typedef bool (S_CALLTYPE *SteamAPI_Init_pfn    )(void);
typedef bool (S_CALLTYPE *SteamAPI_InitSafe_pfn)(void);

typedef bool (S_CALLTYPE *SteamAPI_RestartAppIfNecessary_pfn)
    (uint32 unOwnAppID);
typedef bool (S_CALLTYPE *SteamAPI_IsSteamRunning_pfn)(void);

typedef void (S_CALLTYPE *SteamAPI_Shutdown_pfn)(void);

typedef void (S_CALLTYPE *SteamAPI_RegisterCallback_pfn)
    (class CCallbackBase *pCallback, int iCallback);
typedef void (S_CALLTYPE *SteamAPI_UnregisterCallback_pfn)
    (class CCallbackBase *pCallback);

typedef void (S_CALLTYPE *SteamAPI_RegisterCallResult_pfn)
    (class CCallbackBase *pCallback, SteamAPICall_t hAPICall );
typedef void (S_CALLTYPE *SteamAPI_UnregisterCallResult_pfn)
    (class CCallbackBase *pCallback, SteamAPICall_t hAPICall );

typedef void (S_CALLTYPE *SteamAPI_RunCallbacks_pfn)(void);

typedef HSteamUser (*SteamAPI_GetHSteamUser_pfn)(void);
typedef HSteamPipe (*SteamAPI_GetHSteamPipe_pfn)(void);

typedef ISteamClient* (S_CALLTYPE *SteamClient_pfn)(void);

SteamAPI_RunCallbacks_pfn          SteamAPI_RunCallbacks                = nullptr;
SteamAPI_RunCallbacks_pfn          SteamAPI_RunCallbacks_Original       = nullptr;

void
S_CALLTYPE
SteamAPI_RunCallbacks_Detour (void);

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

class SK_Steam_OverlayManager
{
public:
  SK_Steam_OverlayManager (void) :
       activation ( this, &SK_Steam_OverlayManager::OnActivate      )
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

        if (first) {
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
  }\
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
  try
  {
    if (overlay_activation_callbacks.find (pCallback) != overlay_activation_callbacks.end ())
      overlay_activation_callbacks.erase (pCallback);
  }

  catch (...)
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
typedef bool (S_CALLTYPE* SteamAPI_Init_pfn)    (void);

SteamAPI_InitSafe_pfn SteamAPI_InitSafe_Original = nullptr;
SteamAPI_Init_pfn     SteamAPI_Init_Original     = nullptr;

bool S_CALLTYPE SteamAPI_InitSafe_Detour (void);
bool S_CALLTYPE SteamAPI_Init_Detour     (void);

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


class SK_SteamAPIContext : public SK_IVariableListener
{
public:
  virtual bool OnVarChange (SK_IVariable* var, void* val = NULL);

  bool InitCSteamworks (HMODULE hSteamDLL)
  {
    return false;

    if (config.steam.silent)
      return false;

    if (SteamAPI_InitSafe == nullptr)
    {
      SteamAPI_InitSafe =
        (SteamAPI_InitSafe_pfn)GetProcAddress (
          hSteamDLL,
            "InitSafe"
        );
    }

    if (SteamAPI_GetHSteamUser == nullptr)
    {
      SteamAPI_GetHSteamUser =
        (SteamAPI_GetHSteamUser_pfn)GetProcAddress (
           hSteamDLL,
             "GetHSteamUser_"
        );
    }

    if (SteamAPI_GetHSteamPipe == nullptr)
    {
      SteamAPI_GetHSteamPipe =
        (SteamAPI_GetHSteamPipe_pfn)GetProcAddress (
           hSteamDLL,
             "GetHSteamPipe_"
        );
    }

    SteamAPI_IsSteamRunning =
      (SteamAPI_IsSteamRunning_pfn)GetProcAddress (
         hSteamDLL,
           "IsSteamRunning"
      );

    if (SteamClient == nullptr)
    {
      SteamClient =
        (SteamClient_pfn)GetProcAddress (
           hSteamDLL,
             "SteamClient_"
        );
    }

    bool success = true;

    if (SteamAPI_GetHSteamUser == nullptr)
    {
      steam_log.Log (L"Could not load GetHSteamUser (...)");
      success = false;
    }

    if (SteamAPI_GetHSteamPipe == nullptr)
    {
      steam_log.Log (L"Could not load GetHSteamPipe (...)");
      success = false;
    }

    if (SteamClient == nullptr) {
      steam_log.Log (L"Could not load SteamClient (...)");
      success = false;
    }

    if (SteamAPI_RegisterCallback == nullptr)
    {
      steam_log.Log (L"Could not load RegisterCallback (...)");
      success = false;
    }

    if (SteamAPI_UnregisterCallback == nullptr)
    {
      steam_log.Log (L"Could not load UnregisterCallback (...)");
      success = false;
    }

    if (SteamAPI_RunCallbacks == nullptr)
    {
      steam_log.Log (L"Could not load RunCallbacks (...)");
      success = false;
    }

    if (! success)
      return false;

    client_ = SteamClient ();

    hSteamUser = SteamAPI_GetHSteamUser ();
    hSteamPipe = SteamAPI_GetHSteamPipe ();

    if (! client_)
      return false;

    user_ =
      client_->GetISteamUser (
        hSteamUser,
          hSteamPipe,
            STEAMUSER_INTERFACE_VERSION
      );

    if (user_ == nullptr)
    {
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

    if (user_stats_ == nullptr)
    {
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

    if (apps_ == nullptr)
    {
      steam_log.Log ( L" >> ISteamApps NOT FOUND for version %hs <<",
                        STEAMAPPS_INTERFACE_VERSION );
      return false;
    }

    utils_ =
      client_->GetISteamUtils ( hSteamPipe,
                                  STEAMUTILS_INTERFACE_VERSION );

    if (utils_ == nullptr)
    {
      steam_log.Log ( L" >> ISteamUtils NOT FOUND for version %hs <<",
                        STEAMUTILS_INTERFACE_VERSION );
      return false;
    }

    screenshots_ =
      client_->GetISteamScreenshots ( hSteamUser,
                                        hSteamPipe,
                                          STEAMSCREENSHOTS_INTERFACE_VERSION );

    if (screenshots_ == nullptr)
    {
      steam_log.Log ( L" >> ISteamScreenshots NOT FOUND for version %hs <<",
                        STEAMSCREENSHOTS_INTERFACE_VERSION );
      return false;
    }

    steam_log.LogEx (true, L" # Installing Steam API Debug Text Callback... ");
    SteamClient ()->SetWarningMessageHook (&SteamAPIDebugTextHook);
    steam_log.LogEx (false, L"SteamAPIDebugTextHook\n\n");

    return true;
  }

  bool InitSteamAPI (HMODULE hSteamDLL)
  {
    if (config.steam.silent)
      return false;

    if (SteamAPI_InitSafe == nullptr)
    {
      SteamAPI_InitSafe =
        (SteamAPI_InitSafe_pfn)GetProcAddress (
          hSteamDLL,
            "SteamAPI_InitSafe"
        );
    }

    if (SteamAPI_GetHSteamUser == nullptr)
    {
      SteamAPI_GetHSteamUser =
        (SteamAPI_GetHSteamUser_pfn)GetProcAddress (
           hSteamDLL,
             "SteamAPI_GetHSteamUser"
        );
    }

    if (SteamAPI_GetHSteamPipe == nullptr)
    {
      SteamAPI_GetHSteamPipe =
        (SteamAPI_GetHSteamPipe_pfn)GetProcAddress (
           hSteamDLL,
             "SteamAPI_GetHSteamPipe"
        );
    }

    SteamAPI_IsSteamRunning =
      (SteamAPI_IsSteamRunning_pfn)GetProcAddress (
         hSteamDLL,
           "SteamAPI_IsSteamRunning"
      );

    if (SteamClient == nullptr)
    {
      SteamClient =
        (SteamClient_pfn)GetProcAddress (
           hSteamDLL,
             "SteamClient"
        );
    }

    if (SteamAPI_RegisterCallResult == nullptr)
    {
      SteamAPI_RegisterCallResult =
        (SteamAPI_RegisterCallResult_pfn)GetProcAddress (
          hSteamDLL,
            "SteamAPI_RegisterCallResult"
        );
    }

    if (SteamAPI_UnregisterCallResult == nullptr)
    {
      SteamAPI_UnregisterCallResult =
        (SteamAPI_UnregisterCallResult_pfn)GetProcAddress (
          hSteamDLL,
            "SteamAPI_UnregisterCallResult"
        );
    }


    bool success = true;

    if (SteamAPI_GetHSteamUser == nullptr)
    {
      steam_log.Log (L"Could not load SteamAPI_GetHSteamUser (...)");
      success = false;
    }

    if (SteamAPI_GetHSteamPipe == nullptr)
    {
      steam_log.Log (L"Could not load SteamAPI_GetHSteamPipe (...)");
      success = false;
    }

    if (SteamClient == nullptr)
    {
      steam_log.Log (L"Could not load SteamClient (...)");
      success = false;
    }


    if (SteamAPI_RunCallbacks == nullptr)
    {
      steam_log.Log (L"Could not load SteamAPI_RunCallbacks (...)");
      success = false;
    }

    if (SteamAPI_Shutdown == nullptr)
    {
      steam_log.Log (L"Could not load SteamAPI_Shutdown (...)");
      success = false;
    }

    if (! success)
      return false;

    if (! SteamAPI_InitSafe ())
    {
      steam_log.Log (L" SteamAPI_InitSafe (...) Failed?!");
      return false;
    }

    client_ = SteamClient ();

    if (! client_)
    {
      steam_log.Log (L" SteamClient (...) Failed?!");
      return false;
    }

    hSteamPipe = SteamAPI_GetHSteamPipe ();
    hSteamUser = SteamAPI_GetHSteamUser ();

    MH_QueueEnableHook (SteamAPI_Shutdown);
    MH_QueueEnableHook (SteamAPI_RunCallbacks);
    MH_ApplyQueued     ();

    user_ =
      client_->GetISteamUser (
        hSteamUser,
          hSteamPipe,
            STEAMUSER_INTERFACE_VERSION
      );

    if (user_ == nullptr)
    {
      steam_log.Log ( L" >> ISteamUser NOT FOUND for version %hs <<",
                        STEAMUSER_INTERFACE_VERSION );
      return false;
    }

    // Blacklist of people not allowed to use my software (for being disruptive to other users)
    uint32_t aid = user_->GetSteamID ().GetAccountID    ();
    uint64_t s64 = user_->GetSteamID ().ConvertToUint64 ();

    if ( aid ==  64655118 || aid == 183437803 )
    {
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

    if (friends_ == nullptr)
    {
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

    if (user_stats_ == nullptr)
    {
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

    if (apps_ == nullptr)
    {
      steam_log.Log ( L" >> ISteamApps NOT FOUND for version %hs <<",
                        STEAMAPPS_INTERFACE_VERSION );
      return false;
    }

    utils_ =
      client_->GetISteamUtils ( hSteamPipe,
                                  STEAMUTILS_INTERFACE_VERSION );

    if (utils_ == nullptr)
    {
      steam_log.Log ( L" >> ISteamUtils NOT FOUND for version %hs <<",
                        STEAMUTILS_INTERFACE_VERSION );
      return false;
    }

    screenshots_ =
      client_->GetISteamScreenshots ( hSteamUser,
                                        hSteamPipe,
                                          STEAMSCREENSHOTS_INTERFACE_VERSION );

    if (screenshots_ == nullptr)
    {
      steam_log.Log ( L" >> ISteamScreenshots NOT FOUND for version %hs <<",
                        STEAMSCREENSHOTS_INTERFACE_VERSION );
      return false;
    }

    //steam_log.LogEx (true, L" # Installing Steam API Debug Text Callback... ");
    //SteamClient ()->SetWarningMessageHook (&SteamAPIDebugTextHook);
    //steam_log.LogEx (false, L"SteamAPIDebugTextHook\n\n");

    return true;
  }

  void Shutdown (void)
  {
    if (InterlockedExchange         (&__SK_Steam_init, 0))
    { 
      if (client_)
      {
#if 0
        if (hSteamUser != 0)
          client_->ReleaseUser       (hSteamPipe, hSteamUser);
      
        if (hSteamPipe != 0)
          client_->BReleaseSteamPipe (hSteamPipe);
#endif
      
        SK_SteamAPI_DestroyManagers  ();
      }
      
      hSteamPipe   = 0;
      hSteamUser   = 0;
      
      client_      = nullptr;
      user_        = nullptr;
      user_stats_  = nullptr;
      apps_        = nullptr;
      friends_     = nullptr;
      utils_       = nullptr;
      screenshots_ = nullptr;
      
      if (SteamAPI_Shutdown_Original != nullptr)
      {
        SteamAPI_Shutdown_Original = nullptr;

        SK_DisableHook (SteamAPI_RunCallbacks);
        SK_DisableHook (SteamAPI_Shutdown);

        SteamAPI_Shutdown ();
      }
    }
  }

  ISteamUser*        User        (void) { return user_;        }
  ISteamUserStats*   UserStats   (void) { return user_stats_;  }
  ISteamApps*        Apps        (void) { return apps_;        }
  ISteamFriends*     Friends     (void) { return friends_;     }
  ISteamUtils*       Utils       (void) { return utils_;       }
  ISteamScreenshots* Screenshots (void) { return screenshots_; }

  SK_IVariable*      popup_origin   = nullptr;
  SK_IVariable*      notify_corner  = nullptr;

  SK_IVariable*      tbf_pirate_fun = nullptr;
  float              tbf_float      = 45.0f;

  // Backing storage for the human readable variable names,
  //   the actual system uses an integer value but we need
  //     storage for the cvars.
  struct {
    char popup_origin  [16] = { "DontCare" };
    char notify_corner [16] = { "DontCare" };
  } var_strings;

protected:
private:
  HSteamPipe         hSteamPipe     = 0;
  HSteamUser         hSteamUser     = 0;

  ISteamClient*      client_        = nullptr;
  ISteamUser*        user_          = nullptr;
  ISteamUserStats*   user_stats_    = nullptr;
  ISteamApps*        apps_          = nullptr;
  ISteamFriends*     friends_       = nullptr;
  ISteamUtils*       utils_         = nullptr;
  ISteamScreenshots* screenshots_   = nullptr;
} steam_ctx;


void
SK_Steam_SetNotifyCorner (void)
{
  // 4 == Don't Care
  if (config.steam.notify_corner != 4)
  {
    if (steam_ctx.Utils ())
    {
      steam_ctx.Utils ()->SetOverlayNotificationPosition (
        (ENotificationPosition)config.steam.notify_corner
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

  if (var == tbf_pirate_fun)
  {
    known = true;

    static float history [4] = { 45.0f, 45.0f, 45.0f, 45.0f };
    static int   idx         = 0;

    history [idx] = *(float *)val;

    if (*(float *)val != 45.0f)
    {
      if (idx > 0 && history [idx-1] != 45.0f)
      {
        const SK_IVariable* var =
          SK_GetCommandProcessor ()->ProcessCommandLine (
            "Textures.LODBias"
          ).getVariable ();
        
        if (var != nullptr)
        {
          *(float *)var->getValuePointer () = *(float *)val;
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
      SK_Steam_PopupOriginStrToEnum (*(char **)val);

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
      SK_Steam_PopupOriginStrToEnum (*(char **)val);

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

bool has_global_data  = false;
int  next_friend      = 0;

#define STEAM_CALLRESULT( thisclass, func, param, var ) CCallResult< thisclass, param > var; void func( param *pParam, bool )

class SK_Steam_AchievementManager
{
public:
  class Achievement
  {
  public:
    Achievement (const char* szName, ISteamUserStats* stats)
    {
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
        free ((void *)icons_.unachieved);
                      icons_.unachieved = nullptr;
      }

      if (icons_.achieved != nullptr)
      {
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
                &global_percent_ ) )
      {
        steam_log.Log (L" Global Achievement Read Failure For '%hs'", name_);
        global_percent_ = 0.0f;
      }
    }

    const char* name_;
    const char* human_name_;
    const char* desc_;

    float       global_percent_;

    struct
    {
      int unlocked; // Number of friends who have unlocked
      int possible; // Number of friends who may be able to unlock
    } friends_;

    struct
    {
      uint8_t*  achieved;
      uint8_t*  unachieved;
    } icons_;

    struct
    {
      int current;
      int max;

      float getPercent (void)
      {
        return 100.0f * (float)current / (float)max;
      }
    } progress_;

    bool        unlocked_;
    __time32_t  time_;
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
    lifetime_popups = 0;

    loadSound (wszUnlockSound);

    if (! steam_ctx.UserStats ())
      return;

    uint32_t reserve =
      std::max (steam_ctx.UserStats ()->GetNumAchievements (), (uint32_t)128UL);

    achievements.list.resize        (reserve);
    achievements.string_map.reserve (reserve);
    friend_stats.reserve            (128);

    for (uint32_t i = 0; i < reserve; i++)
    {
      achievements.list [i] = nullptr;
    }
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
                           100.0 * ((double)achievement.friends_.unlocked /
                                    (double)achievement.friends_.possible) );

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
          int friend_count =
            friends->GetFriendCount (k_EFriendFlagImmediate);

          if (friend_count > 0)
          {
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

          if (friend_stats [pParam->m_steamIDUser.GetAccountID ()][i])
          {
            // On the first unlocked achievement, make a note...
            if (! can_unlock)
            {
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
        if (can_unlock)
        {
          for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
            ++achievements.list [i]->friends_.possible;
        }

        int friend_count = friends->GetFriendCount (k_EFriendFlagImmediate);
        if (friend_count > 0 && next_friend < friend_count)
        {
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
    else
    {
      // Friend doesn't own the game, so just skip them...
      if (pParam->m_eResult == k_EResultFail)
      {
        int friend_count = friends->GetFriendCount (k_EFriendFlagImmediate);

        if (friend_count > 0 && next_friend < friend_count)
        {
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

      else
      {
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
        steam_log.Log ( L" Non-Zero Result (%lu) for GlobalAchievementPercentagesReady_t",
                          eCallFail );

        has_global_data = false;
        return;
      }
    }

    else {
      if (failed) {
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
          PlaySound ( (LPCWSTR)unlock_sound, NULL, SND_ASYNC | SND_MEMORY );

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
      }

      else
      {
        float progress = 
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

    if (! popups.size ())
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

    if (! popups.size ())
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

        std::vector <SK_AchievementPopup>::iterator it = popups.begin ();

        float inset = config.steam.achievements.popup.inset;

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
          if (timeGetTime () < (*it).time + POPUP_DURATION_MS) {
            float percent_of_lifetime = ((float)((*it).time + POPUP_DURATION_MS - timeGetTime ()) / 
                                                (float)POPUP_DURATION_MS);

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
                if (config.steam.achievements.take_screenshot && take_screenshot)
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

  Achievement* getAchievement (const char* szName)
  {
    if (achievements.string_map.count (szName))
      return achievements.string_map [szName];

    return nullptr;
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

  void pullStats (void)
  {
    ISteamUserStats* stats = steam_ctx.UserStats ();

    if (stats)
    {
      for (uint32 i = 0; i < stats->GetNumAchievements (); i++)
      {
        //steam_log.Log (L"  >> %hs", stats->GetAchievementName (i));

        if (achievements.list [i] == nullptr)
        {
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

          //if (! achievements.list [i]->unlocked_)
          //stats->SetAchievement   (szName);

          // After setting the achievement, fetch the icon -- this is
          //   necessary so that the unlock dialog does not display
          //     the locked icon.
          if ( steam_ctx.UserStats () )
            steam_ctx.UserStats ()->GetAchievementIcon (szName);

          //if (! achievements.list [i]->unlocked_)
            //stats->ClearAchievement (szName);

          free ((void *)szName);
        }

        else
        {
          achievements.list [i]->update (stats);
        }
      }
    }
  }

  void
  loadSound (const wchar_t* wszUnlockSound)
  {
    bool xbox = false,
         psn  = false;

    wchar_t wszFileName [MAX_PATH + 2] = { L'\0' };

    if (! wcslen (wszUnlockSound))
    {
      iSK_INI achievement_ini (
        SK_EvalEnvironmentVars (
          L"%USERPROFILE%\\Documents\\My Mods\\SpecialK\\Global\\achievements.ini"
        ).c_str ());

      achievement_ini.parse ();

      // If the config file is empty, establish defaults and then write it.
      if (achievement_ini.get_sections ().size () == 0)
      {
        achievement_ini.import ( L"[Steam.Achievements]\n"
                                 L"SoundFile=psn\n"
                                 L"PlaySound=true\n"
                                 L"TakeScreenshot=false\n"
                                 L"AnimatePopup=true\n"
                                 L"NotifyCorner=0\n" );
        achievement_ini.write (        SK_EvalEnvironmentVars (
          L"%USERPROFILE%\\Documents\\My Mods\\SpecialK\\Global\\achievements.ini"
        ).c_str ());
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

      unlock_sound = (uint8_t *)new uint8_t [size];

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
        if (sound_ref != 0) {
          unlock_sound        = (uint8_t *)LockResource (sound_ref);

          default_loaded = true;
        }
      }
    }
  }

protected:
  CEGUI::Window* createPopupWindow (SK_AchievementPopup* popup)
  {
    if (popup->achievement == nullptr)
      return nullptr;

    CEGUI::System* pSys = CEGUI::System::getDllSingletonPtr ();

    extern CEGUI::Window* SK_achv_popup;

    char szPopupName [16];
    sprintf (szPopupName, "Achievement_%lu", lifetime_popups++);

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
    char szGlobal [32] = { '\0' };
    snprintf ( szGlobal, 32,
                 "Global: %6.2f%%",
                   achievement->global_percent_ );
    achv_global->setText (szGlobal);
    
    CEGUI::Window* achv_friend = achv_popup->getChild ("FriendRarity");
    char szFriend [32] = { '\0' };
    snprintf ( szFriend, 32,
                 "Friends: %6.2f%%",
        100.0 * ((double)          achievement->friends_.unlocked /
                 (double)std::max (achievement->friends_.possible, 1)) );
    achv_friend->setText (szFriend);


    // If the server hasn't updated the unlock time, just use the curren time
    if (achievement != nullptr && achievement->time_ == 0)
      _time32 (&achievement->time_);

    CEGUI::Window* achv_unlock  = achv_popup->getChild ("UnlockTime");
    CEGUI::Window* achv_percent = achv_popup->getChild ("ProgressBar");

    float progress = achievement->progress_.getPercent ();
    
    char szUnlockTime [128] = { '\0' };
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

      //achv_percent->setProperty ( "CurrentProgress", szUnlockTime );
    
      snprintf ( szUnlockTime, 128,
                   "Current Progress: %lu/%lu (%6.2f%%)",
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
            steam_log.Log ( L" * Fetched RGBA Icon (idx=%lu) for Achievement: '%hs'  (%lux%lu) "
                              L"{ Took %lu try(s) }",
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
                               CEGUI::Sizef ((float)w, (float)h),
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
  struct SK_AchievementData
  {
    std::vector        <             Achievement*> list;
    std::unordered_map <std::string, Achievement*> string_map;
  } achievements; // SELF

  std::unordered_map <
    uint32_t,
      std::unordered_map < uint32_t, bool >
  > friend_stats;

  std::vector <SK_AchievementPopup> popups;
  int                               lifetime_popups;

  SteamAPICall_t global_request;
  SteamAPICall_t stats_request;

  bool           default_loaded;
  uint8_t*       unlock_sound;   // A .WAV (PCM) file
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

  steam_log.LogEx (true, L" >> Attempting to Unlock Achievement: %i... ",
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

      free ((void *)szName);
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

void
SK_Steam_DrawOSD (void)
{
  if (steam_achievements != nullptr)
  {
    SteamAPI_RunCallbacks_Original ();
    steam_achievements->drawPopups ();
  }
}

volatile LONGLONG SK_SteamAPI_CallbackRunCount    = 0LL;
volatile LONG     SK_SteamAPI_ManagersInitialized = 0L;

void
S_CALLTYPE
SteamAPI_RunCallbacks_Detour (void)
{
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
      return;
    }
  }

  if (InterlockedAdd64 (&SK_SteamAPI_CallbackRunCount, 0) == 0)
  {
    // Handle situations where Steam was initialized earlier than
    //   expected...
    if (InterlockedCompareExchange (&__SK_Steam_init, 0, 0))
    {
      // Init the managers after the first callback run
      void SK_SteamAPI_InitManagers (void);
           SK_SteamAPI_InitManagers ();
    }

    steam_achievements->requestStats ();

    __try
    {
      SteamAPI_RunCallbacks_Original ();
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
      steam_log.Log (L" Caught a Structured Exception while running Steam Callbacks!");
    }

    if (! steam_ctx.UserStats ()) {
      SteamAPI_InitSafe ();
      return;
    }

    SteamAPICall_t call =
      steam_ctx.UserStats ()->RequestGlobalAchievementPercentages ();

    __try
    {
      SteamAPI_RunCallbacks_Original ();
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
      steam_log.Log (L" Caught a Structured Exception while running Steam Callbacks!");
    }

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

            Sleep (1500UL);

            SK_Steam_SetNotifyCorner ();

            CloseHandle (GetCurrentThread ());

            return 0;
          }, nullptr,
        0x00,
      nullptr
    );
  }
  InterlockedIncrement64 (&SK_SteamAPI_CallbackRunCount);

  __try
  {
    SteamAPI_RunCallbacks_Original ();
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    steam_log.Log (L" Caught a Structured Exception while running Steam Callbacks!");
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

void
SK::SteamAPI::Init (bool pre_load)
{
  if (config.steam.silent)
    return;
}

void
SK::SteamAPI::Shutdown (void)
{
  //SK_AutoClose_Log (steam_log);

  steam_ctx.Shutdown ();
}

void SK::SteamAPI::Pump (void)
{
  if (steam_ctx.UserStats ())
  {
    if (SteamAPI_RunCallbacks != nullptr)
      SteamAPI_RunCallbacks ();
  }
}

volatile HANDLE hSteamPump = 0;

DWORD
WINAPI
SteamAPI_PumpThread (_Unreferenced_parameter_ LPVOID user)
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
  if (callback_freq < 3.0)
  {
    if (SteamAPI_InitSafe_Original ())
    {
      steam_log.Log ( L" >> Installing a callback auto-pump at 4 Hz.\n\n");

      while (true)
      {
        Sleep (250);

        SK::SteamAPI::Pump ();
      }
    }

    else
    {
      steam_log.Log ( L" >> Tried to install a callback auto-pump, but "
                      L"could not initialize SteamAPI.\n\n" );
    }
  }

  CloseHandle (InterlockedExchangePointer (&hSteamPump, 0));

  return 0;
}

void
StartSteamPump (bool force)
{
  if (InterlockedCompareExchangePointer (&hSteamPump, 0, 0) != 0)
    return;

  if (config.steam.auto_pump_callbacks || force)
  {
    InterlockedExchangePointer ( &hSteamPump,
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
KillSteamPump (void)
{
  HANDLE hOriginal =
    InterlockedExchangePointer (&hSteamPump, 0);

  if (hOriginal != 0)
  {
    TerminateThread (hOriginal, 0x00);
    CloseHandle     (hOriginal);
  }
}


uint32_t
SK::SteamAPI::AppID (void)
{
  ISteamUtils* utils = steam_ctx.Utils ();

  if (utils != nullptr)
  {
    static uint32_t id = utils->GetAppID ();

    // If no AppID was manually set, let's assign one now.
    if (config.steam.appid == 0)
      config.steam.appid = id;

    return id;
  }

  return 0;
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
                           NULL,
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
  typedef char* steam_library_t [MAX_PATH];
  static bool   scanned_libs = false;

#define MAX_STEAM_LIBRARIES 16
  static int             steam_libs = 0;
  static steam_library_t steam_lib_paths [MAX_STEAM_LIBRARIES] = { 0 };

  static const wchar_t* wszSteamPath;

  if (! scanned_libs)
  {
    wszSteamPath =
      SK_GetSteamDir ();

    if (wszSteamPath != nullptr)
    {
      wchar_t wszLibraryFolders [MAX_PATH];

      lstrcpyW (wszLibraryFolders, wszSteamPath);
      lstrcatW (wszLibraryFolders, L"\\steamapps\\libraryfolders.vdf");

      if (GetFileAttributesW (wszLibraryFolders) != INVALID_FILE_ATTRIBUTES)
      {
        HANDLE hLibFolders =
          CreateFileW ( wszLibraryFolders,
                          GENERIC_READ,
                            FILE_SHARE_READ,
                              nullptr,
                                OPEN_EXISTING,
                                  GetFileAttributesW (wszLibraryFolders),
                                    nullptr );

        if (hLibFolders != INVALID_HANDLE_VALUE)
        {
          DWORD  dwSize,
                 dwSizeHigh,
                 dwRead;

          // This isn't a 4+ GiB file... so who the heck cares about the high-bits?
          dwSize = GetFileSize (hLibFolders, &dwSizeHigh);

          void* data =
            new uint8_t [dwSize];

          if (data == nullptr)
          {
            CloseHandle (hLibFolders);
            return nullptr;
          }

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

          delete [] data;

          CloseHandle (hLibFolders);
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
      char szManifest [MAX_PATH] = { '\0' };

      sprintf ( szManifest,
                  "%s\\steamapps\\appmanifest_%d.acf",
                    (char *)steam_lib_paths [i],
                      appid );

      if (GetFileAttributesA (szManifest) != INVALID_FILE_ATTRIBUTES)
      {
        HANDLE hManifest =
          CreateFileA ( szManifest,
                        GENERIC_READ,
                          FILE_SHARE_READ,
                            nullptr,
                              OPEN_EXISTING,
                                GetFileAttributesA (szManifest),
                                  nullptr );

        if (hManifest != INVALID_HANDLE_VALUE)
        {
          DWORD  dwSize,
                 dwSizeHigh,
                 dwRead;

          dwSize = GetFileSize (hManifest, &dwSizeHigh);

          char* szManifestData =
            new char [dwSize + 1];

          szManifestData [dwSize] = '\0';

          ReadFile ( hManifest,
                       szManifestData,
                         dwSize,
                           &dwRead,
                             nullptr );

          CloseHandle (hManifest);

          if (! dwRead)
          {
            delete [] szManifestData;
            continue;
          }

          char* szAppName =
            StrStrIA (szManifestData, "\"name\"");

          char szGameName [MAX_PATH] = { '\0' };

          if (szAppName != nullptr)
          {
            // Make sure everything is lowercase
            strncpy (szAppName, "\"name\"", strlen ("\"name\""));

            sscanf ( szAppName,
                       "\"name\" \"%259[^\"]\"",
                         szGameName );

            return szGameName;
          }

          delete [] szManifestData;
        }
      }
    }
  }

  char szManifest [MAX_PATH] = { '\0' };

  sprintf ( szManifest,
              "%ls\\steamapps\\appmanifest_%d.acf",
                wszSteamPath,
                  appid );

  if (GetFileAttributesA (szManifest) != INVALID_FILE_ATTRIBUTES)
  {
    HANDLE hManifest =
      CreateFileA ( szManifest,
                    GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr,
                          OPEN_EXISTING,
                            GetFileAttributesA (szManifest),
                              nullptr );

    if (hManifest != INVALID_HANDLE_VALUE)
    {
      DWORD  dwSize,
             dwSizeHigh,
             dwRead;

      dwSize = GetFileSize (hManifest, &dwSizeHigh);

      char* szManifestData =
        new char [dwSize + 1];

      szManifestData [dwSize] = '\0';

      if (szManifestData == nullptr)
      {
        CloseHandle (hManifest);
        return nullptr;
      }

      ReadFile ( hManifest,
                   szManifestData,
                     dwSize,
                       &dwRead,
                         nullptr );

      CloseHandle (hManifest);

      if (! dwRead)
      {
        delete [] szManifestData;
        return nullptr;
      }

      char* szAppName =
        StrStrIA (szManifestData, "\"name\"");

      char szGameName [MAX_PATH] = { '\0' };

      if (szAppName != nullptr)
      {
        // Make sure everything is lowercase
        strncpy (szAppName, "\"name\"", strlen ("\"name\""));

        sscanf ( szAppName,
                   "\"name\" \"%259[^\"]\"",
                     szGameName );

        return szGameName;
      }

      delete [] szManifestData;
    }
  }

  return "";
}

std::string
SK::SteamAPI::AppName (void)
{
  // Only do this once, the AppID never changes =P
  static std::string app_name =
    SK_UseManifestToGetAppName (AppID ());

  return app_name;
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
  overlay_state   = active;

  std::set <CCallbackBase *>::iterator it =
    overlay_activation_callbacks.begin ();

  try
  {
    while (it != overlay_activation_callbacks.end ())
    {
      (*it++)->Run (&state);
    }
  }

  catch (...)
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
  EnterCriticalSection (&init_cs);

  // In case we already initialized stuff...
  if (InterlockedCompareExchange (&__SK_Steam_init, FALSE, FALSE))
  {
    LeaveCriticalSection (&init_cs);
    return true;
  }

  static int init_tries = -1;

  if (++init_tries == 0)
  {
    steam_log.Log ( L"Initializing SteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"----------(InitSafe)-----------\n");
  }

  if ( SteamAPI_InitSafe_Original != nullptr &&
       SteamAPI_InitSafe_Original () )
  {
    InterlockedExchange (&__SK_Steam_init, TRUE);

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

    StartSteamPump ();

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

#if 1
  EnterCriticalSection         (&init_cs);
  KillSteamPump                ();
  LeaveCriticalSection         (&init_cs);

  steam_ctx.Shutdown           ();
  return;
#endif

#if 0
  CreateThread ( nullptr, 0,
       [](LPVOID user) ->
  DWORD {
    Sleep (20000UL);

    while (SteamAPI_InitSafe_Original == nullptr)
      Sleep (10000UL);

    if (SteamAPI_InitSafe ())
      StartSteamPump (true);

    CloseHandle (GetCurrentThread ());
    return 0;
  }, nullptr, 0x00, nullptr );
#endif
}

bool
S_CALLTYPE
SteamAPI_Init_Detour (void)
{
  EnterCriticalSection (&init_cs);

  // In case we already initialized stuff...
  if (InterlockedCompareExchange (&__SK_Steam_init, FALSE, FALSE))
  {
    LeaveCriticalSection (&init_cs);
    return true;
  }

  static int init_tries = -1;

  if (++init_tries == 0)
  {
    steam_log.Log ( L"Initializing SteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"-------------------------------\n");
  }

  if (SteamAPI_Init_Original ())
  {
    InterlockedExchange (&__SK_Steam_init, TRUE);

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

    StartSteamPump ();

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


typedef bool (S_CALLTYPE *InitSafe_pfn)(void);
InitSafe_pfn InitSafe_Original = nullptr;

bool
S_CALLTYPE
InitSafe_Detour (void)
{
  EnterCriticalSection (&init_cs);

  // In case we already initialized stuff...
  if (InterlockedCompareExchange (&__SK_Steam_init, FALSE, FALSE))
  {
    LeaveCriticalSection (&init_cs);
    return true;
  }

  static int init_tries = -1;

  if (++init_tries == 0)
  {
    steam_log.Log ( L"Initializing CSteamWorks Backend  << %s >>",
                      SK_GetCallerName ().c_str () );
    steam_log.Log (L"-----------(InitSafe)-----------\n");
  }

  if (InitSafe_Original ())
  {
    InterlockedExchange (&__SK_Steam_init, TRUE);

    HMODULE hSteamAPI =
      LoadLibraryW_Original (L"CSteamworks.dll");

    if (! steam_ctx.UserStats ())
      steam_ctx.InitCSteamworks (hSteamAPI);

    steam_log.Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                      init_tries + 1,
                        SK::SteamAPI::AppID () );

    StartSteamPump ();

    LeaveCriticalSection (&init_cs);
    return true;
  }

  LeaveCriticalSection (&init_cs);
  return false;
}

DWORD
WINAPI
CSteamworks_Delay_Init (LPVOID user)
{
#if 0
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
#endif

  CloseHandle (GetCurrentThread ());

  return 0;
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

void
SK_HookCSteamworks (void)
{
  if (config.steam.silent)
    return;

  bool has_steamapi = false;

  EnterCriticalSection (&init_cs);
  if ( InterlockedCompareExchange (&__CSteamworks_hook, TRUE, FALSE) ||
       InterlockedCompareExchange (&__SteamAPI_hook,    FALSE, FALSE) )
  {
    LeaveCriticalSection (&init_cs);
    return;
  }

  steam_log.Log (L"CSteamworks.dll was loaded, hooking...");

  // Get the full path to the module's file.
  HANDLE  hProc = GetCurrentProcess ();
  HMODULE hMod  = GetModuleHandle (L"CSteamworks.dll");

  wchar_t wszModName [MAX_PATH] = { L'\0' };

  if ( GetModuleFileNameExW ( hProc,
                                hMod,
                                  wszModName,
                                    sizeof (wszModName) /
                                      sizeof (wchar_t) ) )
  {
    wchar_t* dll_path =
      StrStrIW (wszModName, L"CSteamworks.dll");

    if (dll_path != nullptr)
      *dll_path = L'\0';

#ifdef _WIN64
    lstrcatW (wszModName, L"steam_api64.dll");
#else
    lstrcatW (wszModName, L"steam_api.dll");
#endif

    if (LoadLibraryW_Original (wszModName))
    {
      steam_log.Log ( L" >>> Located a real steam_api DLL: '%s'...",
                      wszModName );

      has_steamapi = true;

      SK_HookSteamAPI ();

      SteamAPI_InitSafe_Detour ();

      //SK_ApplyQueuedHooks ();
    }

    else
    {
      SK_CreateDLLHook3 ( wszModName,
                          "SteamAPI_InitSafe",
                         SteamAPI_InitSafe_Detour,
              (LPVOID *)&SteamAPI_InitSafe_Original,
              (LPVOID *)&SteamAPI_InitSafe );
     
      SK_CreateDLLHook3 ( wszModName,
                          "SteamAPI_Init",
                         SteamAPI_Init_Detour,
              (LPVOID *)&SteamAPI_Init_Original,
              (LPVOID *)&SteamAPI_Init );
     
      SK_CreateDLLHook3 ( wszModName,
                          "SteamAPI_RegisterCallback",
                          SteamAPI_RegisterCallback_Detour,
               (LPVOID *)&SteamAPI_RegisterCallback_Original,
               (LPVOID *)&SteamAPI_RegisterCallback );
     
      SK_CreateDLLHook3 ( wszModName,
                          "SteamAPI_UnregisterCallback",
                          SteamAPI_UnregisterCallback_Detour,
                (LPVOID *)&SteamAPI_UnregisterCallback_Original,
                (LPVOID *)&SteamAPI_UnregisterCallback );

      SK_CreateDLLHook3 ( L"CSteamworks.dll",
                         "InitSafe",
                         InitSafe_Detour,
              (LPVOID *)&InitSafe_Original );
      
      SK_CreateDLLHook3 ( L"CSteamworks.dll",
                         "Init",
                        SteamAPI_Init_Detour,
              (LPVOID *)&SteamAPI_Init_Original );
      
      SK_CreateDLLHook3 ( L"CSteamworks.dll",
                          "Shutdown",
                          SteamAPI_Shutdown_Detour,
               (LPVOID *)&SteamAPI_Shutdown_Original );
      
      SK_CreateDLLHook3 ( L"CSteamworks.dll",
                         "RegisterCallback",
                         SteamAPI_RegisterCallback_Detour,
              (LPVOID *)&SteamAPI_RegisterCallback_Original,
              (LPVOID *)&SteamAPI_RegisterCallback );
      
      SK_CreateDLLHook3 ( L"CSteamworks.dll",
                         "UnregisterCallback",
                         SteamAPI_UnregisterCallback_Detour,
              (LPVOID *)&SteamAPI_UnregisterCallback_Original,
              (LPVOID *)&SteamAPI_UnregisterCallback );
      
      SK_CreateDLLHook3 ( L"CSteamworks.dll",
                         "RunCallbacks",
                         SteamAPI_RunCallbacks_Detour,
              (LPVOID *)&SteamAPI_RunCallbacks_Original,
              (LPVOID *)&SteamAPI_RunCallbacks );

      SK_ApplyQueuedHooks ();

      SteamAPI_InitSafe ();
    }
  }

  LeaveCriticalSection (&init_cs);
}

DWORD
WINAPI
SteamAPI_Delay_Init (LPVOID user)
{
#if 0
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
#endif

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
  steam_size = SK_GetFileSize (wszSteamAPI);

  if (config.steam.silent)
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
          (LPVOID *)&SteamAPI_InitSafe_Original,
          (LPVOID *)&SteamAPI_InitSafe );

  SK_CreateDLLHook2 ( wszSteamAPI,
                      "SteamAPI_Init",
                     SteamAPI_Init_Detour,
          (LPVOID *)&SteamAPI_Init_Original,
          (LPVOID *)&SteamAPI_Init );

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

  //
  // Do not queue these up (by calling CreateDLLHook2),
  //   they will be installed only upon the game successfully
  //     calling one of the SteamAPI initialization functions.
  //
  SK_CreateDLLHook ( wszSteamAPI,
                     "SteamAPI_RunCallbacks",
                      SteamAPI_RunCallbacks_Detour,
           (LPVOID *)&SteamAPI_RunCallbacks_Original,
           (LPVOID *)&SteamAPI_RunCallbacks );

  SK_CreateDLLHook ( wszSteamAPI,
                     "SteamAPI_Shutdown",
                      SteamAPI_Shutdown_Detour,
           (LPVOID *)&SteamAPI_Shutdown_Original,
           (LPVOID *)&SteamAPI_Shutdown );

  SK_ApplyQueuedHooks ();

  if (config.steam.init_delay > 0) {
    CreateThread (nullptr, 0, SteamAPI_Delay_Init, nullptr, 0x00, nullptr);
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
      steam_log.Log (L" Creating Achievement Manager...");

      steam_achievements = new SK_Steam_AchievementManager (
        config.steam.achievements.sound_file.c_str ()
      );
      overlay_manager    = new SK_Steam_OverlayManager ();

      steam_log.LogEx (false, L"\n");
    }
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
  }
}




bool steam_imported = false;

bool
SK_SteamImported (void)
{
  static int tries = 0;

  return steam_imported || GetModuleHandle (L"steam_api64.dll") ||
                           GetModuleHandle (L"steam_api.dll")   ||
                           GetModuleHandle (L"CSteamworks.dll") ||
                           GetModuleHandle (L"SteamNative.dll");
}

void
SK_TestSteamImports (HMODULE hMod)
{
  sk_import_test_s steamworks [] = { { "CSteamworks.dll", false } };

  SK_TestImports (hMod, steamworks, sizeof (steamworks) / sizeof sk_import_test_s);

  if (steamworks [0].used)
  {
    SK_HookCSteamworks ();
    steam_imported = true;
  }

  else
  {
    sk_import_test_s steam_api [] = { { "steam_api.dll",   false },
                                      { "steam_api64.dll", false } };

    SK_TestImports (hMod, steam_api, sizeof (steam_api) / sizeof sk_import_test_s);

    if (steam_api [0].used | steam_api [1].used)
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
        if ( LoadLibraryW_Original (L"steam_api.dll")   ||
             LoadLibraryW_Original (L"steam_api64.dll") ||
             GetModuleHandle       (L"SteamNative.dll") )
        {
           SK_HookSteamAPI ();
           steam_imported = true;
        }
      }
    }
  }

  if ( LoadLibraryW_Original (L"steam_api.dll")   ||
       LoadLibraryW_Original (L"steam_api64.dll") ||
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


bool SK::SteamAPI::overlay_state = false;


uint32_t
__stdcall
SK_Steam_PiratesAhoy (void)
{
  static uint32_t verdict = 0x00;
  static bool     decided = false;

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

  if (steam_size > 0 && steam_size < (1024 * 92))
  {
    verdict = 0x68992;
  }

  // CPY
  if (steam_ctx.User () != nullptr)
  {
    if (steam_ctx.User ()->GetSteamID ().ConvertToUint64 () == 18295873490452480 << 4)
      verdict = 0x1;
  }

  if (crc32_steamapi == 0x28140083 << 1)
    verdict = crc32_steamapi;

  decided = true;

  // User opted out of Steam enhancement, no further action necessary
  if (config.steam.silent)
    verdict = 0x00;

  return verdict;
}

uint32_t
__stdcall
SK_Steam_PiratesAhoy2 (void)
{
  return SK_Steam_PiratesAhoy ();
}