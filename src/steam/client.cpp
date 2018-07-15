#include <Windows.h>
#include <SpecialK/steam_api.h>
#include <SpecialK/utility.h>
#include <cstdint>

#include <unordered_map>

class ISteamUser;
class IWrapSteamUser;

class ISteamUtils;
class IWrapSteamUtils;

class ISteamController;
class IWrapSteamController;

class ISteamRemoteStorage;
class IWrapSteamRemoteStorage;

class ISteamClient;
class IWrapSteamClient;

#include <concurrent_unordered_map.h>

extern concurrency::concurrent_unordered_map <ISteamUser*,          IWrapSteamUser*>          SK_SteamWrapper_remap_user;
extern concurrency::concurrent_unordered_map <ISteamUtils*,         IWrapSteamUtils*>         SK_SteamWrapper_remap_utils;
extern concurrency::concurrent_unordered_map <ISteamController*,    IWrapSteamController*>    SK_SteamWrapper_remap_controller;
extern concurrency::concurrent_unordered_map <ISteamRemoteStorage*, IWrapSteamRemoteStorage*> SK_SteamWrapper_remap_remotestorage;
       concurrency::concurrent_unordered_map <ISteamClient*,        IWrapSteamClient*>        SK_SteamWrapper_remap_client;


extern
ISteamUser*
SK_SteamWrapper_WrappedClient_GetISteamUser ( ISteamClient *This,
                                              HSteamUser    hSteamUser,
                                              HSteamPipe    hSteamPipe,
                                              const char   *pchVersion );

extern
ISteamController*
SK_SteamWrapper_WrappedClient_GetISteamController ( ISteamClient *This,
                                                    HSteamUser    hSteamUser,
                                                    HSteamPipe    hSteamPipe,
                                                    const char   *pchVersion );

extern
ISteamUtils*
SK_SteamWrapper_WrappedClient_GetISteamUtils ( ISteamClient *This,
                                               HSteamPipe    hSteamPipe,
                                               const char   *pchVersion );

extern
ISteamRemoteStorage*
SK_SteamWrapper_WrappedClient_GetISteamRemoteStorage ( ISteamClient *This,
                                                       HSteamUser    hSteamuser,
                                                       HSteamPipe    hSteamPipe,
                                                       const char   *pchVersion );

extern
ISteamUGC*
SK_SteamWrapper_WrappedClient_GetISteamUGC ( ISteamClient *This,
                                             HSteamUser    hSteamuser,
                                             HSteamPipe    hSteamPipe,
                                             const char   *pchVersion );

#define WRAP_CONTROLLER
#define WRAP_STORAGE
#define WRAP_USER
#define WRAP_UTILS
//#define WRAP_UGC

class IWrapSteamClient : public ISteamClient
{
public:
  IWrapSteamClient (ISteamClient* pSteamClient) :
                     pRealClient (pSteamClient) {
  };

  virtual HSteamPipe CreateSteamPipe     (void)                                               override { return pRealClient->CreateSteamPipe     (          );                } // 0
  virtual bool       BReleaseSteamPipe   (HSteamPipe   hSteamPipe)                            override { return pRealClient->BReleaseSteamPipe   (hSteamPipe);                } // 1
  virtual HSteamUser ConnectToGlobalUser (HSteamPipe   hSteamPipe)                            override { return pRealClient->ConnectToGlobalUser (hSteamPipe);                } // 2
  virtual HSteamUser CreateLocalUser     (HSteamPipe *phSteamPipe, EAccountType eAccountType) override { return pRealClient->CreateLocalUser     (phSteamPipe, eAccountType); } // 3
  virtual void       ReleaseUser         (HSteamPipe   hSteamPipe, HSteamUser hUser)          override { return pRealClient->ReleaseUser         (hSteamPipe,  hUser);        } // 4



  virtual ISteamUser *GetISteamUser      (       HSteamUser  hSteamUser,
                                                 HSteamPipe  hSteamPipe,
                                           const char       *pchVersion ) override
  {
#ifdef WRAP_USER
    return SK_SteamWrapper_WrappedClient_GetISteamUser ( pRealClient,
                                                           hSteamUser,
                                                             hSteamPipe,
                                                               pchVersion );
#else
    return pRealClient->GetISteamUser ( hSteamUser,
                                          hSteamPipe,
                                            pchVersion );
#endif
  } // 5



  virtual ISteamGameServer         *GetISteamGameServer          (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamGameServer         (hSteamUser, hSteamPipe, pchVersion);                                                        } // 6
  virtual void                      SetLocalIPBinding            (uint32 unIP, uint16 usPort)                                           override {
    return pRealClient->SetLocalIPBinding           (unIP, usPort);                                                                              } // 7
  virtual ISteamFriends            *GetISteamFriends             (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamFriends            (hSteamUser, hSteamPipe, pchVersion);                                                        } // 8
  virtual ISteamUtils              *GetISteamUtils               (HSteamPipe hSteamPipe, const char *pchVersion)                        override
  {
#ifdef WRAP_UTILS
    return SK_SteamWrapper_WrappedClient_GetISteamUtils ( pRealClient,
                                                            hSteamPipe,
                                                              pchVersion );                                                                      
#else
    return pRealClient->GetISteamUtils ( hSteamPipe,
                                           pchVersion );
#endif
  } // 9
  virtual ISteamMatchmaking        *GetISteamMatchmaking         (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamMatchmaking        (hSteamUser, hSteamPipe, pchVersion);                                                        } // 10
  virtual ISteamMatchmakingServers *GetISteamMatchmakingServers  (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamMatchmakingServers (hSteamUser, hSteamPipe, pchVersion);                                                        } // 11
  virtual void                     *GetISteamGenericInterface    (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamGenericInterface   (hSteamUser, hSteamPipe, pchVersion);                                                        } // 12
  virtual ISteamUserStats          *GetISteamUserStats           (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamUserStats          (hSteamUser, hSteamPipe, pchVersion);                                                        } // 13
  virtual ISteamGameServerStats    *GetISteamGameServerStats     (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamGameServerStats    (hSteamuser, hSteamPipe, pchVersion);                                                        } // 14
  virtual ISteamApps               *GetISteamApps                (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamApps               (hSteamUser, hSteamPipe, pchVersion);                                                        } // 15
  virtual ISteamNetworking         *GetISteamNetworking          (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamNetworking         (hSteamUser, hSteamPipe, pchVersion);                                                        } // 16
  virtual ISteamRemoteStorage      *GetISteamRemoteStorage       (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
#ifdef WRAP_STORAGE
    return SK_SteamWrapper_WrappedClient_GetISteamRemoteStorage ( pRealClient,
                                                                    hSteamuser,
                                                                      hSteamPipe,
                                                                        pchVersion );
#else
    return pRealClient->GetISteamRemoteStorage ( hSteamuser,
                                                   hSteamPipe,
                                                     pchVersion );
#endif
  } // 17
  virtual ISteamScreenshots        *GetISteamScreenshots         (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamScreenshots        (hSteamuser, hSteamPipe, pchVersion);                                                        } // 18
  virtual void                      RunFrame                     (void)                                                                 override {
    return pRealClient->RunFrame                    ();                                                                                          } // 19
  virtual uint32                    GetIPCCallCount              (void)                                                                 override {
    return pRealClient->GetIPCCallCount             ();                                                                                          } // 20
  virtual void                      SetWarningMessageHook        (SteamAPIWarningMessageHook_t pFunction)                               override {
    return pRealClient->SetWarningMessageHook       (pFunction);                                                                                 } // 21
  virtual bool                      BShutdownIfAllPipesClosed    (void)                                                override { return false;  } // 22
  virtual ISteamHTTP               *GetISteamHTTP                (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamHTTP               (hSteamuser, hSteamPipe, pchVersion);                                                        } // 23
  virtual ISteamUnifiedMessages    *GetISteamUnifiedMessages     (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamUnifiedMessages    (hSteamuser, hSteamPipe, pchVersion);                                                        } // 24




  virtual ISteamController         *GetISteamController          (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
#ifdef WRAP_CONTROLLER
    return SK_SteamWrapper_WrappedClient_GetISteamController (pRealClient, hSteamUser, hSteamPipe, pchVersion);
#else
    return pRealClient->GetISteamController (hSteamUser, hSteamPipe, pchVersion);
#endif
                                                                                                                                                 } // 25






  virtual ISteamUGC                *GetISteamUGC                 (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
#ifdef WRAP_UGC
    return SK_SteamWrapper_WrappedClient_GetISteamUGC ( pRealClient,
                                                          hSteamUser,
                                                            hSteamPipe,
                                                              pchVersion );
#else
    return pRealClient->GetISteamUGC                (hSteamUser, hSteamPipe, pchVersion);
#endif
  } // 26
  virtual ISteamAppList            *GetISteamAppList             (HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamAppList            (hSteamUser, hSteamPipe, pchVersion);                                                        } // 27
  virtual ISteamMusic              *GetISteamMusic               (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamMusic              (hSteamuser, hSteamPipe, pchVersion);                                                        } // 28
  virtual ISteamMusicRemote        *GetISteamMusicRemote         (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamMusicRemote        (hSteamuser, hSteamPipe, pchVersion);                                                        } // 29
  virtual ISteamHTMLSurface        *GetISteamHTMLSurface         (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamHTMLSurface        (hSteamuser, hSteamPipe, pchVersion);                                                        } // 30

  virtual void                      Set_SteamAPI_CPostAPIResultInProcess           (SteamAPI_PostAPIResultInProcess_t  func) override {
    return pRealClient->Set_SteamAPI_CPostAPIResultInProcess           (func);                                                        } // 31
  virtual void                      Remove_SteamAPI_CPostAPIResultInProcess        (SteamAPI_PostAPIResultInProcess_t  func) override {
    return pRealClient->Remove_SteamAPI_CPostAPIResultInProcess        (func);                                                        } // 32
  virtual void                      Set_SteamAPI_CCheckCallbackRegisteredInProcess (SteamAPI_CheckCallbackRegistered_t func) override {
    return pRealClient->Set_SteamAPI_CCheckCallbackRegisteredInProcess (func);                                                        } // 33

  virtual ISteamInventory          *GetISteamInventory           (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamInventory          (hSteamuser, hSteamPipe, pchVersion);                                                        } // 34
  virtual ISteamVideo              *GetISteamVideo               (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamVideo              (hSteamuser, hSteamPipe, pchVersion);                                                        } // 35
	virtual ISteamParentalSettings   *GetISteamParentalSettings    (HSteamUser hSteamuser, HSteamPipe hSteamPipe, const char *pchVersion) override {
    return pRealClient->GetISteamParentalSettings   (hSteamuser, hSteamPipe, pchVersion);                                                        } // 36


  ISteamClient* pRealClient;
};



using SteamClient_pfn = ISteamClient* (S_CALLTYPE *)(
        void
      );
SteamClient_pfn SteamClient_Original = nullptr;

ISteamClient*
S_CALLTYPE
SteamClient_Detour (void)
{
  SK_RunOnce (
    steam_log.Log ( L"[!] %hs ()",
                      __FUNCTION__ )
  );

  ISteamClient* pClient =
    static_cast <ISteamClient *> ( SteamClient_Original () );

  if (pClient != nullptr)
  {
    if (SK_SteamWrapper_remap_client.count (pClient))
       return SK_SteamWrapper_remap_client [pClient];

    else
    {
      SK_SteamWrapper_remap_client [pClient] =
              new IWrapSteamClient (pClient);

      return SK_SteamWrapper_remap_client [pClient];
    }
  }

  return nullptr;
}


using SteamInternal_CreateInterface_pfn = void*       (S_CALLTYPE *)(
  const char *ver
  );
SteamInternal_CreateInterface_pfn         SteamInternal_CreateInterface_Original = nullptr;


void*
S_CALLTYPE
SteamInternal_CreateInterface_Detour (const char *ver)
{
  steam_log.Log (L"[Steam Wrap] [!] SteamInternal_CreateInterface (%hs)", ver);

  if (! lstrcmpA (ver, STEAMCLIENT_INTERFACE_VERSION))
  {
    ISteamClient* pClient =
      static_cast <ISteamClient *> ( SteamInternal_CreateInterface_Original (ver) );

    if (pClient != nullptr)
    {
      if (SK_SteamWrapper_remap_client.count (pClient))
         return SK_SteamWrapper_remap_client [pClient];

      else
      {
        SK_SteamWrapper_remap_client [pClient] =
                new IWrapSteamClient (pClient);

        return SK_SteamWrapper_remap_client [pClient];
      }
    }
  }

  else
  {
    SK_RunOnce (
      steam_log.Log ( L"Game requested unexpected interface version (%s)!",
                        ver )
    );
  }

  return
    SteamInternal_CreateInterface_Original (ver);
}