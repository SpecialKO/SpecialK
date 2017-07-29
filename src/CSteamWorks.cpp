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

#include <SpecialK/steam_api.h>
#include <SpecialK/resource.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>

#include <SpecialK/config.h>
#include <SpecialK/ini.h>
#include <SpecialK/log.h>
#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/compatibility.h>

#include <SpecialK/osd/popup.h>
#include <SpecialK/osd/text.h>
#include <SpecialK/render_backend.h>

#include <SpecialK/command.h>
#include <SpecialK/utility.h>

extern CRITICAL_SECTION callback_cs;
extern CRITICAL_SECTION init_cs;

volatile ULONG __CSteamworks_hook = FALSE;

bool
SK_SteamAPIContext::InitCSteamworks (HMODULE hSteamDLL)
{
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

  if (SteamClient == nullptr)
  {
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

    screenshots_ =
      client_->GetISteamScreenshots ( hSteamUser,
                                        hSteamPipe,
                                          "STEAMSCREENSHOTS_INTERFACE_VERSION001" );

    steam_log.Log ( L" >> ISteamScreenshots NOT FOUND for version %hs <<",
                      "STEAMSCREENSHOTS_INTERFACE_VERSION001" );


    //return false;
  }

  controller_ =
    client_->GetISteamController (
      hSteamUser,
        hSteamPipe,
          STEAMCONTROLLER_INTERFACE_VERSION );

  //void** vftable = *(void***)*&controller_;
  //SK_CreateVFTableHook ( L"ISteamController::GetControllerState",
                           //vftable, 3,
            //ISteamController_GetControllerState_Detour,
                  //(LPVOID *)&GetControllerState_Original );

  return true;
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

    if (hSteamAPI)
    {
      if (! steam_ctx.UserStats ())
        steam_ctx.InitCSteamworks (hSteamAPI);

      steam_log.Log ( L"--- Initialization Finished (%d tries [AppId: %lu]) ---\n\n",
                        init_tries + 1,
                          SK::SteamAPI::AppID () );

      SK_Steam_StartPump ();

      LeaveCriticalSection (&init_cs);
      return true;
    }
  }

  LeaveCriticalSection (&init_cs);
  return false;
}

DWORD
WINAPI
CSteamworks_Delay_Init (LPVOID user)
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

    if (InitSafe_Detour != nullptr)
      InitSafe_Detour ();

    ++tries;
  }

  CloseHandle (GetCurrentThread ());

  return 0;
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
  HMODULE hMod = GetModuleHandle (L"CSteamworks.dll");

  if (! hMod) hMod  = GetModuleHandle (L"steamworks.net.dll"); // Mafia 3 nonsense

  wchar_t wszModName [MAX_PATH] = { };

  if ( GetModuleFileNameW ( hMod,
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