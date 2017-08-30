#include <SpecialK/steam_api.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>
#include <SpecialK/diagnostics/compatibility.h>

#define SMOKE_VERSION_NUM L"0.0.2"
#define SMOKE_VERSION_STR L"SMOKE v " SMOKE_VERSION_NUM


using SteamInternal_CreateInterface_pfn = void * (S_CALLTYPE *)(
        const char *ver
      );
      SteamInternal_CreateInterface_pfn         SteamInternal_CreateInterface_Original = nullptr;

using SteamAPI_ISteamClient_GetISteamUser_pfn = ISteamUser* (S_CALLTYPE *)(
              ISteamClient *This,
              HSteamUser    hSteamUser,
              HSteamPipe    hSteamPipe,
        const char         *pchVersion
      );
      SteamAPI_ISteamClient_GetISteamUser_pfn   SteamAPI_ISteamClient_GetISteamUser_Original = nullptr;


class ISteamUser;
class ISteamUserFake;

class ISteamClient;
class ISteamClientFake;

static std::unordered_map <ISteamUser*,   ISteamUserFake*>   remap_user;
static std::unordered_map <ISteamClient*, ISteamClientFake*> remap_client;


class ISteamUserFake : public ISteamUser
{
public:
  ISteamUserFake (ISteamUser* pUser) : pRealUser (pUser)
  { };

  virtual HSteamUser GetHSteamUser ( )
    override
  {
    return pRealUser->GetHSteamUser ( );
  };
  virtual bool BLoggedOn (void)
    override
  {
    return true;
  };
  virtual CSteamID GetSteamID ( )
    override
  {
    return pRealUser->GetSteamID ( );
  };
  virtual int InitiateGameConnection (void *pAuthBlob, int cbMaxAuthBlob, CSteamID steamIDGameServer, uint32 unIPServer, uint16 usPortServer, bool bSecure)
    override
  {
    return pRealUser->InitiateGameConnection (pAuthBlob, cbMaxAuthBlob, steamIDGameServer, unIPServer, usPortServer, bSecure);
  };
  virtual void TerminateGameConnection (uint32 unIPServer, uint16 usPortServer)
    override
  {
    return pRealUser->TerminateGameConnection (unIPServer, usPortServer);
  };
  virtual void TrackAppUsageEvent (CGameID gameID, int eAppUsageEvent, const char *pchExtraInfo = "")
    override
  {
    return pRealUser->TrackAppUsageEvent (gameID, eAppUsageEvent, pchExtraInfo);
  };
  virtual bool GetUserDataFolder (char *pchBuffer, int cubBuffer)
    override
  {
    return pRealUser->GetUserDataFolder (pchBuffer, cubBuffer);
  };
  virtual void StartVoiceRecording ( )
    override
  {
    return pRealUser->StartVoiceRecording ( );
  };
  virtual void StopVoiceRecording ( )
    override
  {
    return pRealUser->StopVoiceRecording ( );
  };
  virtual EVoiceResult GetAvailableVoice (uint32 *pcbCompressed, uint32 *pcbUncompressed, uint32 nUncompressedVoiceDesiredSampleRate)
    override
  {
    return pRealUser->GetAvailableVoice (pcbCompressed, pcbUncompressed, nUncompressedVoiceDesiredSampleRate);
  }
  virtual EVoiceResult GetVoice (bool bWantCompressed, void *pDestBuffer, uint32 cbDestBufferSize, uint32 *nBytesWritten, bool bWantUncompressed, void *pUncompressedDestBuffer, uint32 cbUncompressedDestBufferSize, uint32 *nUncompressBytesWritten, uint32 nUncompressedVoiceDesiredSampleRate)
    override
  {
    return pRealUser->GetVoice (bWantCompressed, pDestBuffer, cbDestBufferSize, nBytesWritten, bWantUncompressed, pUncompressedDestBuffer, cbUncompressedDestBufferSize, nUncompressBytesWritten, nUncompressedVoiceDesiredSampleRate);
  }
  virtual EVoiceResult DecompressVoice (const void *pCompressed, uint32 cbCompressed, void *pDestBuffer, uint32 cbDestBufferSize, uint32 *nBytesWritten, uint32 nDesiredSampleRate)
    override
  {
    return pRealUser->DecompressVoice (pCompressed, cbCompressed, pDestBuffer, cbDestBufferSize, nBytesWritten, nDesiredSampleRate);
  }
  virtual uint32 GetVoiceOptimalSampleRate ( )
    override
  {
    return pRealUser->GetVoiceOptimalSampleRate ( );
  }
  virtual HAuthTicket GetAuthSessionTicket (void *pTicket, int cbMaxTicket, uint32 *pcbTicket)
    override
  {
    return pRealUser->GetAuthSessionTicket (pTicket, cbMaxTicket, pcbTicket);
  }
  virtual EBeginAuthSessionResult BeginAuthSession (const void *pAuthTicket, int cbAuthTicket, CSteamID steamID)
    override
  {
    return pRealUser->BeginAuthSession (pAuthTicket, cbAuthTicket, steamID);
  }
  virtual void EndAuthSession (CSteamID steamID)
    override
  {
    return pRealUser->EndAuthSession (steamID);
  }
  virtual void CancelAuthTicket (HAuthTicket hAuthTicket)
    override
  {
    return pRealUser->CancelAuthTicket (hAuthTicket);
  }
  virtual EUserHasLicenseForAppResult UserHasLicenseForApp (CSteamID steamID, AppId_t appID)
    override
  {
    return pRealUser->UserHasLicenseForApp (steamID, appID);
  }
  virtual bool BIsBehindNAT ( )
    override
  {
    return pRealUser->BIsBehindNAT ( );
  }
  virtual void AdvertiseGame (CSteamID steamIDGameServer, uint32 unIPServer, uint16 usPortServer)
    override
  {
    return pRealUser->AdvertiseGame (steamIDGameServer, unIPServer, usPortServer);
  }
  virtual SteamAPICall_t RequestEncryptedAppTicket (void *pDataToInclude, int cbDataToInclude)
    override
  {
    return pRealUser->RequestEncryptedAppTicket (pDataToInclude, cbDataToInclude);
  }
  virtual bool GetEncryptedAppTicket (void *pTicket, int cbMaxTicket, uint32 *pcbTicket)
    override
  {
    return pRealUser->GetEncryptedAppTicket (pTicket, cbMaxTicket, pcbTicket);
  }
  virtual int GetGameBadgeLevel (int nSeries, bool bFoil)
    override
  {
    return pRealUser->GetGameBadgeLevel (nSeries, bFoil);
  }
  virtual int GetPlayerSteamLevel ( )
    override
  {
    return pRealUser->GetPlayerSteamLevel ( );
  };
  virtual SteamAPICall_t RequestStoreAuthURL (const char *pchRedirectURL)
    override
  {
    return pRealUser->RequestStoreAuthURL (pchRedirectURL);
  };

private:
  ISteamUser* pRealUser;
};


class ISteamClientFake : public ISteamClient
{
public:
  ISteamClientFake (ISteamClient* pSteamClient) : pRealClient (pSteamClient)
  { };

  virtual HSteamPipe CreateSteamPipe ( ) override
  {
    return pRealClient->CreateSteamPipe ( );
  }
  virtual bool BReleaseSteamPipe (HSteamPipe hSteamPipe) override
  {
    return pRealClient->BReleaseSteamPipe (hSteamPipe);
  }
  virtual HSteamUser ConnectToGlobalUser (HSteamPipe hSteamPipe) override
  {
    return pRealClient->ConnectToGlobalUser (hSteamPipe);
  }
  virtual HSteamUser CreateLocalUser (HSteamPipe *phSteamPipe, EAccountType eAccountType)
    override
  {
    return pRealClient->CreateLocalUser (phSteamPipe, eAccountType);
  }
  virtual void ReleaseUser (HSteamPipe hSteamPipe, HSteamUser hUser)
    override
  {
    return pRealClient->ReleaseUser (hSteamPipe, hUser);
  }



  virtual ISteamUser *GetISteamUser (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override
  {
    ISteamUser* pUser =
      pRealClient->GetISteamUser (hSteamUser, hSteamPipe, pchVersion);

    if (pUser != nullptr)
    {
      if (remap_user.count (pUser))
        return remap_user [pUser];

      else
      {
        remap_user [pUser] =
          new ISteamUserFake (pUser);

        return remap_user [pUser];
      }
    }

    return nullptr;
    return pRealClient->GetISteamUser (hSteamUser, hSteamPipe, pchVersion);
  }



  virtual ISteamGameServer *GetISteamGameServer (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamGameServer (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual void SetLocalIPBinding (uint32 unIP, uint16 usPort)
    override
  {
    return pRealClient->SetLocalIPBinding (unIP, usPort);
  }
  virtual ISteamFriends *GetISteamFriends (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamFriends (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual ISteamUtils *GetISteamUtils (HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamUtils (hSteamPipe, pchVersion);
  }
  virtual ISteamMatchmaking *GetISteamMatchmaking (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamMatchmaking (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual ISteamMatchmakingServers *GetISteamMatchmakingServers (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamMatchmakingServers (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual void *GetISteamGenericInterface (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamGenericInterface (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual ISteamUserStats *GetISteamUserStats (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamUserStats (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual ISteamGameServerStats *GetISteamGameServerStats (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamGameServerStats (hSteamuser, hSteamPipe, pchVersion);
  }
  virtual ISteamApps *GetISteamApps (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamApps (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual ISteamNetworking *GetISteamNetworking (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamNetworking (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual ISteamRemoteStorage *GetISteamRemoteStorage (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamRemoteStorage (hSteamuser, hSteamPipe, pchVersion);
  }
  virtual ISteamScreenshots *GetISteamScreenshots (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamScreenshots (hSteamuser, hSteamPipe, pchVersion);
  }
  virtual void RunFrame ( )
    override
  {
    return pRealClient->RunFrame ( );
  }
  virtual uint32 GetIPCCallCount ( )
    override
  {
    return pRealClient->GetIPCCallCount ( );
  }
  virtual void SetWarningMessageHook (SteamAPIWarningMessageHook_t pFunction)
    override
  {
    return pRealClient->SetWarningMessageHook (pFunction);
  }
  virtual bool BShutdownIfAllPipesClosed ( )
    override
  {
    return false;
    //return pRealClient->BShutdownIfAllPipesClosed ( );
  }
  virtual ISteamHTTP *GetISteamHTTP (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamHTTP (hSteamuser, hSteamPipe, pchVersion);
  }
  virtual ISteamUnifiedMessages *GetISteamUnifiedMessages (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamUnifiedMessages (hSteamuser, hSteamPipe, pchVersion);
  }
  virtual ISteamController *GetISteamController (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual ISteamUGC *GetISteamUGC (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamUGC (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual ISteamAppList *GetISteamAppList (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamAppList (hSteamUser, hSteamPipe, pchVersion);
  }
  virtual ISteamMusic *GetISteamMusic (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamMusic (hSteamuser, hSteamPipe, pchVersion);
  }
  virtual ISteamMusicRemote *GetISteamMusicRemote (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamMusicRemote (hSteamuser, hSteamPipe, pchVersion);
  }
  virtual ISteamHTMLSurface *GetISteamHTMLSurface (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamHTMLSurface (hSteamuser, hSteamPipe, pchVersion);
  }
  virtual void Set_SteamAPI_CPostAPIResultInProcess (SteamAPI_PostAPIResultInProcess_t func)
    override
  {
    return pRealClient->Set_SteamAPI_CPostAPIResultInProcess (func);
  }
  virtual void Remove_SteamAPI_CPostAPIResultInProcess (SteamAPI_PostAPIResultInProcess_t func)
    override
  {
    return pRealClient->Remove_SteamAPI_CPostAPIResultInProcess (func);
  }
  virtual void Set_SteamAPI_CCheckCallbackRegisteredInProcess (SteamAPI_CheckCallbackRegistered_t func)
    override
  {
    return pRealClient->Set_SteamAPI_CCheckCallbackRegisteredInProcess (func);
  }
  virtual ISteamInventory *GetISteamInventory (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamInventory (hSteamuser, hSteamPipe, pchVersion);
  }
  virtual ISteamVideo *GetISteamVideo (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion)
    override
  {
    return pRealClient->GetISteamVideo (hSteamuser, hSteamPipe, pchVersion);
  }

private:
  ISteamClient* pRealClient;
};


void*
S_CALLTYPE
SteamInternal_CreateInterface_Detour (const char *ver)
{
  if (! lstrcmpA (ver, STEAMCLIENT_INTERFACE_VERSION))
  {
    ISteamClient* pClient =
      static_cast <ISteamClient *> (SteamInternal_CreateInterface_Original (ver));

    if (pClient != nullptr)
    {
      if (remap_client.count (pClient))
        return remap_client [pClient];

      else
      {
        remap_client [pClient] =
          new ISteamClientFake (pClient);

        return remap_client [pClient];
      }
    }
  }
  //dll_log.Log ("%s", ver);

  return SteamInternal_CreateInterface_Original (ver);
}

ISteamUser*
S_CALLTYPE
SteamAPI_ISteamClient_GetISteamUser_Detour (ISteamClient* This, HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion)
{
  ISteamUser* pUser =
    SteamAPI_ISteamClient_GetISteamUser_Original (This, hSteamUser, hSteamPipe, pchVersion);

  if (pUser != nullptr)
  {
    if (remap_user.count (pUser))
      return remap_user [pUser];

    else
    {
             remap_user [pUser] =
        new ISteamUserFake (pUser);

      return remap_user [pUser];
    }
  }

  return nullptr;
}



BOOL
SK_SteamHack_PatchSonicManiaDRM (void)
{
  SK_LoadLibrary_PinModule    (L"steam_api.dll");

  SK_CreateDLLHook2 (          L"steam_api.dll",
                                "SteamAPI_ISteamClient_GetISteamUser",
                                 SteamAPI_ISteamClient_GetISteamUser_Detour,
        static_cast_p2p <void> (&SteamAPI_ISteamClient_GetISteamUser_Original) );

  SK_CreateDLLHook2 (          L"steam_api.dll",
                                "SteamInternal_CreateInterface",
                                 SteamInternal_CreateInterface_Detour,
        static_cast_p2p <void> (&SteamInternal_CreateInterface_Original) );

  SK_ApplyQueuedHooks ();

  return TRUE;
}



extern void
__stdcall
SK_SetPluginName (std::wstring name);

#include <SpecialK/parameter.h>
#include <SpecialK/core.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

struct {
  struct {
    sk::ParameterBool* enable;
  } offline;
} SMOKE_drm;


using  SK_PlugIn_ControlPanelWidget_pfn = void (__stdcall         *)(void);
static SK_PlugIn_ControlPanelWidget_pfn SK_PlugIn_ControlPanelWidget_Original = nullptr;


void
__stdcall
SK_SMOKE_ControlPanel (void)
{
  if (ImGui::CollapsingHeader ("Sonic Mania", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("DRM Nonsense", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool val =
        SMOKE_drm.offline.enable->get_value ();

      ImGui::TreePush ("");

      if (ImGui::Checkbox ("Enable Offline Play", &val))
      {
        SMOKE_drm.offline.enable->set_value (val);
        SMOKE_drm.offline.enable->store     (   );
        SK_GetDLLConfig ()->write           ( SK_GetDLLConfig ()->get_filename () );
      }

      ImGui::TreePop     ( );
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }
}

void
SK_SMOKE_InitPlugin (void)
{
  static sk::ParameterFactory smoke_factory;

  SMOKE_drm.offline.enable = 
    dynamic_cast <sk::ParameterBool *> (
      smoke_factory.create_parameter <bool> (L"Enable Offline Play")
  );

  SMOKE_drm.offline.enable->register_to_ini ( SK_GetDLLConfig (),
                                                L"SMOKE.PlugIn",
                                                  L"EnableOfflinePlay" );

  if (! SMOKE_drm.offline.enable->load ())
  {
    SMOKE_drm.offline.enable->set_value (true);
    SMOKE_drm.offline.enable->store     (    );
  }


  if (SMOKE_drm.offline.enable->get_value () && SK_SteamHack_PatchSonicManiaDRM ())


  // SMOKE only works when locally wrapped
  if (! SK_IsInjected ())
    SK_SetPluginName (SMOKE_VERSION_STR);


  SK_CreateFuncHook (      L"SK_PlugIn_ControlPanelWidget",
                             SK_PlugIn_ControlPanelWidget,
                             SK_SMOKE_ControlPanel,
    static_cast_p2p <void> (&SK_PlugIn_ControlPanelWidget_Original) );
  MH_QueueEnableHook (       SK_PlugIn_ControlPanelWidget);
};