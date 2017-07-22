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
#ifndef __SK__STEAM_API_H__
#define __SK__STEAM_API_H__

#define _CRT_SECURE_NO_WARNINGS
#include <steamapi/steam_api.h>

#include <stdint.h>
#include <string>
#include <SpecialK/utility.h>

#define STEAM_CALLRESULT( thisclass, func, param, var ) CCallResult< thisclass, param > var; void func( param *pParam, bool )

namespace SK
{
  namespace SteamAPI
  {
    void Init     (bool preload);
    void Shutdown (void);
    void Pump     (void);

    void __stdcall SetOverlayState (bool active);
    bool __stdcall GetOverlayState (bool real);

    void __stdcall UpdateNumPlayers (void);
    int  __stdcall GetNumPlayers    (void);

    float __stdcall PercentOfAchievementsUnlocked (void);

    bool __stdcall TakeScreenshot  (void);

    uint32_t    AppID   (void);
    std::string AppName (void);

    // The state that we are explicitly telling the game
    //   about, not the state of the actual overlay...
    extern bool overlay_state;

    extern uint64_t    steam_size;
    // Must be global for x86 ABI problems
    extern CSteamID    player;
  }
}

extern volatile ULONG __SK_Steam_init;
extern volatile ULONG __SteamAPI_hook;

// Tests the Import Table of hMod for anything Steam-Related
//
//   If found, and this test is performed after the pre-init
//     DLL phase, SteamAPI in one of its many forms will be
//       hooked.
void
SK_TestSteamImports (HMODULE hMod);

void
SK_Steam_InitCommandConsoleVariables (void);

//
// Internal data stored in the Achievement Manager, this is
//   the publicly visible data...
//
//   I do not want to expose the poorly designed interface
//     of the full achievement manager outside the DLL,
//       so there exist a few flattened API functions
//         that can communicate with it and these are
//           the data they provide.
//
struct SK_SteamAchievement
{
  // If we were to call ISteamStats::GetAchievementName (...),
  //   this is the index we could use.
  int         idx_;

  const char* name_;          // UTF-8 (I think?)
  const char* human_name_;    // UTF-8
  const char* desc_;          // UTF-8
  
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
  
    __forceinline float getPercent (void)
    {
      return 100.0f * (float)current / (float)max;
    }
  } progress_;
  
  bool        unlocked_;
  __time32_t  time_;
};

#include <SpecialK/log.h>
extern          iSK_Logger       steam_log;

#include <vector>

size_t SK_SteamAPI_GetNumPossibleAchievements (void);

std::vector <SK_SteamAchievement *>& SK_SteamAPI_GetUnlockedAchievements (void);
std::vector <SK_SteamAchievement *>& SK_SteamAPI_GetLockedAchievements   (void);
std::vector <SK_SteamAchievement *>& SK_SteamAPI_GetAllAchievements      (void);

float  SK_SteamAPI_GetUnlockedPercentForFriend      (uint32_t friend_idx);
size_t SK_SteamAPI_GetUnlockedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats);
size_t SK_SteamAPI_GetLockedAchievementsForFriend   (uint32_t friend_idx, BOOL* pStats);
size_t SK_SteamAPI_GetSharedAchievementsForFriend   (uint32_t friend_idx, BOOL* pStats);


// Returns true if all friend stats have been pulled from the server
      bool  __stdcall SK_SteamAPI_FriendStatsFinished  (void);

// Percent (0.0 - 1.0) of friend achievement info fetched
     float  __stdcall SK_SteamAPI_FriendStatPercentage (void);

       int  __stdcall SK_SteamAPI_GetNumFriends        (void);
const char* __stdcall SK_SteamAPI_GetFriendName        (uint32_t friend_idx, size_t* pLen = nullptr);


bool __stdcall SK_SteamAPI_TakeScreenshot           (void);
bool __stdcall SK_IsSteamOverlayActive              (void);
bool __stdcall SK_SteamOverlay_GoToURL              (const char* szURL, bool bUseWindowsShellIfOverlayFails = false);

void    __stdcall SK_SteamAPI_UpdateNumPlayers      (void);
int32_t __stdcall SK_SteamAPI_GetNumPlayers         (void);

float __stdcall SK_SteamAPI_PercentOfAchievementsUnlocked (void);
                                                    
void           SK_SteamAPI_LogAllAchievements       (void);
void           SK_UnlockSteamAchievement            (uint32_t idx);
                                                    
bool           SK_SteamImported                     (void);
void           SK_TestSteamImports                  (HMODULE hMod);
                                                    
void           SK_HookCSteamworks                   (void);
void           SK_HookSteamAPI                      (void);
                                                    
void           SK_Steam_ClearPopups                 (void);
void           SK_Steam_DrawOSD                     (void);

bool           SK_Steam_LoadOverlayEarly            (void);

void           SK_Steam_InitCommandConsoleVariables (void);

ISteamUtils*   SK_SteamAPI_Utils                    (void);

uint32_t __stdcall SK_Steam_PiratesAhoy             (void);




#include <SpecialK/hooks.h>


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

typedef bool (*GetControllerState_pfn)
    (ISteamController* This, uint32 unControllerIndex, SteamControllerState_t *pState);

typedef bool (S_CALLTYPE* SteamAPI_InitSafe_pfn)(void);
typedef bool (S_CALLTYPE* SteamAPI_Init_pfn)    (void);



extern "C" SteamAPI_InitSafe_pfn              SteamAPI_InitSafe_Original;
extern "C" SteamAPI_Init_pfn                  SteamAPI_Init_Original;

extern "C" SteamAPI_RunCallbacks_pfn          SteamAPI_RunCallbacks;
extern "C" SteamAPI_RunCallbacks_pfn          SteamAPI_RunCallbacks_Original;

extern "C" SteamAPI_RegisterCallback_pfn      SteamAPI_RegisterCallback;
extern "C" SteamAPI_RegisterCallback_pfn      SteamAPI_RegisterCallback_Original;

extern "C" SteamAPI_UnregisterCallback_pfn    SteamAPI_UnregisterCallback;
extern "C" SteamAPI_UnregisterCallback_pfn    SteamAPI_UnregisterCallback_Original;

extern "C" SteamAPI_RegisterCallResult_pfn    SteamAPI_RegisterCallResult;
extern "C" SteamAPI_UnregisterCallResult_pfn  SteamAPI_UnregisterCallResult;

extern "C" SteamAPI_Init_pfn                  SteamAPI_Init;
extern "C" SteamAPI_InitSafe_pfn              SteamAPI_InitSafe;

extern "C" SteamAPI_RestartAppIfNecessary_pfn SteamAPI_RestartAppIfNecessary;
extern "C" SteamAPI_IsSteamRunning_pfn        SteamAPI_IsSteamRunning;

extern "C" SteamAPI_GetHSteamUser_pfn         SteamAPI_GetHSteamUser;
extern "C" SteamAPI_GetHSteamPipe_pfn         SteamAPI_GetHSteamPipe;

extern "C" SteamClient_pfn                    SteamClient;

extern "C" SteamAPI_Shutdown_pfn              SteamAPI_Shutdown;
extern "C" SteamAPI_Shutdown_pfn              SteamAPI_Shutdown_Original;

extern "C" GetControllerState_pfn             GetControllerState_Original;


extern "C" bool S_CALLTYPE SteamAPI_InitSafe_Detour     (void);
extern "C" bool S_CALLTYPE SteamAPI_Init_Detour         (void);
extern "C" void S_CALLTYPE SteamAPI_RunCallbacks_Detour (void);

extern "C" void S_CALLTYPE SteamAPI_RegisterCallback_Detour   (class CCallbackBase *pCallback, int iCallback);
extern "C" void S_CALLTYPE SteamAPI_UnregisterCallback_Detour (class CCallbackBase *pCallback);


void SK_SteamAPI_InitManagers            (void);
void SK_SteamAPI_DestroyManagers         (void);

extern "C" bool S_CALLTYPE SteamAPI_InitSafe_Detour (void);
extern "C" bool S_CALLTYPE SteamAPI_Init_Detour     (void);
extern "C" void S_CALLTYPE SteamAPI_Shutdown_Detour (void);
void                       SK_Steam_StartPump       (bool force = false);



#include <SpecialK/command.h>

class SK_SteamAPIContext : public SK_IVariableListener
{
public:
  virtual bool OnVarChange (SK_IVariable* var, void* val = NULL);

  bool InitCSteamworks (HMODULE hSteamDLL);

  bool InitSteamAPI (HMODULE hSteamDLL)
  {
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
    //uint64_t s64 = user_->GetSteamID ().ConvertToUint64 ();

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

    // We can live without this...
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

    SK::SteamAPI::player = user_->GetSteamID ();

#if 0
    controller_ =
      client_->GetISteamController (
        hSteamUser,
          hSteamPipe,
            STEAMCONTROLLER_INTERFACE_VERSION );

    void** vftable = *(void***)*(&controller_);
    SK_CreateVFTableHook ( L"ISteamController::GetControllerState",
                             vftable, 3,
              ISteamController_GetControllerState_Detour,
                    (LPVOID *)&GetControllerState_Original );
#endif

    return true;
  }

  STEAM_CALLRESULT ( SK_SteamAPIContext,
                     OnFileSigDone,
                     CheckFileSignature_t,
                     chk_file_sig );

  void Shutdown (void)
  {
    if (InterlockedDecrement (&__SK_Steam_init) <= 0)
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
      controller_  = nullptr;
      
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
  ISteamController*  Controller  (void) { return controller_;  }

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
  ISteamController*  controller_    = nullptr;
} extern steam_ctx;


#include <SpecialK/log.h>

extern volatile ULONG            __SK_Steam_init;
extern volatile ULONG            __SteamAPI_hook;

extern          iSK_Logger       steam_log;

extern          CRITICAL_SECTION callback_cs;
extern          CRITICAL_SECTION init_cs;
extern          CRITICAL_SECTION popup_cs;


#endif /* __SK__STEAM_API_H__ */