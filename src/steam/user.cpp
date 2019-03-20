#include <SpecialK/steam_api.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>

#include <Windows.h>
#include <cstdint>

#include <unordered_map>

class ISteamClient;
class IWrapSteamClient;

class ISteamUser;
class IWrapSteamUser;

#include <concurrent_unordered_map.h>
concurrency::concurrent_unordered_map <ISteamUser*, IWrapSteamUser*>   SK_SteamWrapper_remap_user;

int __SK_SteamUser_BLoggedOn =
  static_cast <int> (SK_SteamUser_LoggedOn_e::Unknown);

class IWrapSteamUser : public ISteamUser
{
public:
  IWrapSteamUser (ISteamUser* pUser) :
                   pRealUser (pUser) {
  };

  HSteamUser GetHSteamUser          ( void ) override { return pRealUser->GetHSteamUser (); };


  bool       BLoggedOn              ( void ) override
  {
    __SK_SteamUser_BLoggedOn =
      static_cast <int> ( pRealUser->BLoggedOn () ? SK_SteamUser_LoggedOn_e::Online :
                                                    SK_SteamUser_LoggedOn_e::Offline );

    if (config.steam.spoof_BLoggedOn)
    {
      if (__SK_SteamUser_BLoggedOn != static_cast <int> (SK_SteamUser_LoggedOn_e::Online))
        __SK_SteamUser_BLoggedOn   |= static_cast <int> (SK_SteamUser_LoggedOn_e::Spoofing);

      return true;
    }

    return (__SK_SteamUser_BLoggedOn & static_cast <int> (SK_SteamUser_LoggedOn_e::Online)) != 0;
  };


  CSteamID   GetSteamID             ( void ) override { return pRealUser->GetSteamID    (); };

  int        InitiateGameConnection ( void     *pAuthBlob,
                                      int       cbMaxAuthBlob,
                                      CSteamID  steamIDGameServer,
                                      uint32    unIPServer,
                                      uint16    usPortServer,
                                      bool      bSecure )                        override
  {
    return pRealUser->InitiateGameConnection ( pAuthBlob,
                                                 cbMaxAuthBlob,
                                                   steamIDGameServer,
                                                     unIPServer,
                                                       usPortServer,
                                                         bSecure );
  };
  void         TerminateGameConnection( uint32 unIPServer, uint16 usPortServer ) override
  {
    return pRealUser->TerminateGameConnection ( unIPServer,
                                                  usPortServer );
  };
  void         TrackAppUsageEvent    (       CGameID  gameID,
                                             int      eAppUsageEvent,
                                       const char    *pchExtraInfo = "" )        override
  {
    return pRealUser->TrackAppUsageEvent    ( gameID,
                                                eAppUsageEvent,
                                                  pchExtraInfo );
  };
  bool         GetUserDataFolder     ( char *pchBuffer,
                                       int   cubBuffer ) override
  {
    return pRealUser->GetUserDataFolder     ( pchBuffer,
                                                cubBuffer );
  };

  void         StartVoiceRecording   (void) override { return pRealUser->StartVoiceRecording (); };
  void         StopVoiceRecording    (void) override { return pRealUser->StopVoiceRecording  (); };

  EVoiceResult GetAvailableVoice     ( uint32 *pcbCompressed,
                                       uint32 *pcbUncompressed,
                                       uint32  nUncompressedVoiceDesiredSampleRate ) override
  {
    return pRealUser->GetAvailableVoice     ( pcbCompressed,
                                                pcbUncompressed,
                                                  nUncompressedVoiceDesiredSampleRate );
  }
  EVoiceResult GetVoice              ( bool    bWantCompressed,
                                       void   *pDestBuffer,
                                       uint32  cbDestBufferSize,
                                       uint32 *nBytesWritten,
                                       bool    bWantUncompressed,
                                       void   *pUncompressedDestBuffer,
                                       uint32  cbUncompressedDestBufferSize,
                                       uint32 *nUncompressBytesWritten,
                                       uint32  nUncompressedVoiceDesiredSampleRate ) override
  {
    return pRealUser->GetVoice              ( bWantCompressed,
                                                pDestBuffer,
                                                  cbDestBufferSize,
                                                    nBytesWritten,
                                                      bWantUncompressed,
                                                        pUncompressedDestBuffer,
                                                          cbUncompressedDestBufferSize,
                                                            nUncompressBytesWritten,
                                                              nUncompressedVoiceDesiredSampleRate );
  }
  EVoiceResult DecompressVoice       ( const void    *pCompressed,
                                             uint32   cbCompressed,
                                             void    *pDestBuffer,
                                             uint32   cbDestBufferSize,
                                             uint32  *nBytesWritten,
                                             uint32   nDesiredSampleRate ) override
  {
    return pRealUser->DecompressVoice       ( pCompressed,
                                                cbCompressed,
                                                  pDestBuffer,
                                                    cbDestBufferSize,
                                                      nBytesWritten,
                                                        nDesiredSampleRate );
  }
  uint32       GetVoiceOptimalSampleRate (void) override { return pRealUser->GetVoiceOptimalSampleRate (); }

  HAuthTicket  GetAuthSessionTicket      ( void   *pTicket,
                                           int     cbMaxTicket,
                                           uint32 *pcbTicket )                          override
  {
    return pRealUser->GetAuthSessionTicket  ( pTicket,
                                                cbMaxTicket,
                                                  pcbTicket );
  }
  EBeginAuthSessionResult     BeginAuthSession     ( const void     *pAuthTicket,
                                                           int       cbAuthTicket,
                                                           CSteamID  steamID )          override
  {
    return pRealUser->BeginAuthSession      ( pAuthTicket,
                                                cbAuthTicket,
                                                  steamID );
  }
  void                        EndAuthSession       (CSteamID steamID)                   override
  {
    return pRealUser->EndAuthSession        (steamID);
  }
  void                        CancelAuthTicket     (HAuthTicket hAuthTicket)            override
  {
    return pRealUser->CancelAuthTicket      (hAuthTicket);
  }
  EUserHasLicenseForAppResult UserHasLicenseForApp ( CSteamID steamID,
                                                     AppId_t  appID )                   override
  {
    return pRealUser->UserHasLicenseForApp  ( steamID,
                                                appID );
  }
  bool                        BIsBehindNAT         (void) override { return pRealUser->BIsBehindNAT (); }

  void                        AdvertiseGame        ( CSteamID steamIDGameServer,
                                                     uint32   unIPServer,
                                                     uint16   usPortServer )            override
  {
    return pRealUser->AdvertiseGame         ( steamIDGameServer,
                                                unIPServer,
                                                  usPortServer );
  }
  SteamAPICall_t              RequestEncryptedAppTicket ( void *pDataToInclude,
                                                          int   cbDataToInclude )       override
  {
    return pRealUser->RequestEncryptedAppTicket ( pDataToInclude,
                                                    cbDataToInclude );
  }
  bool                        GetEncryptedAppTicket     ( void   *pTicket,
                                                          int     cbMaxTicket,
                                                          uint32 *pcbTicket )           override
  {
    return pRealUser->GetEncryptedAppTicket ( pTicket,
                                                cbMaxTicket,
                                                  pcbTicket );
  }
  int                         GetGameBadgeLevel         ( int  nSeries,
                                                          bool bFoil )                  override
  {
    return pRealUser->GetGameBadgeLevel     ( nSeries,
                                                bFoil );
  }
  int                         GetPlayerSteamLevel       (void)                          override
  {
    return pRealUser->GetPlayerSteamLevel   ();
  };
  SteamAPICall_t              RequestStoreAuthURL       ( const char *pchRedirectURL )  override
  {
    return pRealUser->RequestStoreAuthURL   (pchRedirectURL);
  };


  // 019
  //
  bool                        BIsPhoneVerified          (void)                          override
  {
    return pRealUser->BIsPhoneVerified              ();
  }
  bool                        BIsTwoFactorEnabled       (void)                          override
  {
    return pRealUser->BIsTwoFactorEnabled           ();
  }
  bool                        BIsPhoneIdentifying       (void)                          override
  {
    return pRealUser->BIsPhoneIdentifying           ();
  }
  bool                        BIsPhoneRequiringVerification (void)                      override
  {
    return pRealUser->BIsPhoneRequiringVerification ();
  }

private:
  ISteamUser* pRealUser;
};



using SteamAPI_ISteamClient_GetISteamUser_pfn = ISteamUser* (S_CALLTYPE *)(
  ISteamClient *This,
  HSteamUser    hSteamUser,
  HSteamPipe    hSteamPipe,
  const char   *pchVersion
  );
SteamAPI_ISteamClient_GetISteamUser_pfn   SteamAPI_ISteamClient_GetISteamUser_Original = nullptr;

ISteamUser*
S_CALLTYPE
SteamAPI_ISteamClient_GetISteamUser_Detour (ISteamClient *This,
                                            HSteamUser    hSteamUser,
                                            HSteamPipe    hSteamPipe,
                                            const char   *pchVersion)
{
  SK_RunOnce (
    steam_log->Log ( L"[!] %hs (..., %hs)",
                       __FUNCTION__, pchVersion )
  );

  ISteamUser* pUser =
    SteamAPI_ISteamClient_GetISteamUser_Original ( This,
                                                     hSteamUser,
                                                       hSteamPipe,
                                                         pchVersion );

  if (pUser != nullptr)
  {
    if ((! lstrcmpA (pchVersion, STEAMUSER_INTERFACE_VERSION_018)) ||
        (! lstrcmpA (pchVersion, STEAMUSER_INTERFACE_VERSION_019)) ||
        (! lstrcmpA (pchVersion, STEAMUSER_INTERFACE_VERSION_017)))
    {
      if (SK_SteamWrapper_remap_user.count (pUser))
         return SK_SteamWrapper_remap_user [pUser];

      else
      {
        SK_SteamWrapper_remap_user [pUser] =
                new IWrapSteamUser (pUser);

        return SK_SteamWrapper_remap_user [pUser];
      }
    }

    else
    {
      SK_RunOnce (
        steam_log->Log ( L"Game requested unexpected interface version (%hs)!",
                           pchVersion )
      );

      return pUser;
    }
  }

  return nullptr;
}


ISteamUser*
SK_SteamWrapper_WrappedClient_GetISteamUser ( ISteamClient *This,
                                              HSteamUser    hSteamUser,
                                              HSteamPipe    hSteamPipe,
                                              const char   *pchVersion )
{
  SK_RunOnce (
    steam_log->Log ( L"[!] %hs (..., %hs)",
                       __FUNCTION__, pchVersion )
  );

  ISteamUser* pUser =
    This->GetISteamUser ( hSteamUser,
                            hSteamPipe,
                              pchVersion );

  if (pUser != nullptr)
  {
    if ((! lstrcmpA (pchVersion, STEAMUSER_INTERFACE_VERSION_018)) ||
        (! lstrcmpA (pchVersion, STEAMUSER_INTERFACE_VERSION_019)) ||
        (! lstrcmpA (pchVersion, STEAMUSER_INTERFACE_VERSION_017)))
    {
      if (SK_SteamWrapper_remap_user.count (pUser))
         return SK_SteamWrapper_remap_user [pUser];

      else
      {
        SK_SteamWrapper_remap_user [pUser] =
                new IWrapSteamUser (pUser);

        return SK_SteamWrapper_remap_user [pUser];
      }
    }

    else
    {
      SK_RunOnce (
        steam_log->Log ( L"Game requested unexpected interface version (%hs)!",
                           pchVersion )
      );

      return pUser;
    }
  }

  return nullptr;
}

using SteamUser_pfn = ISteamUser* (S_CALLTYPE *)(
        void
      );
SteamUser_pfn SteamUser_Original = nullptr;

ISteamUser*
S_CALLTYPE
SteamUser_Detour (void)
{
  SK_RunOnce (
    steam_log->Log ( L"[!] %hs ()",
                       __FUNCTION__ )
  );

#ifndef DANGEROUS_INTERFACE_ALIASING
  return SteamUser_Original ( );
#else
  ISteamUser* pUser =
    static_cast <ISteamUser *> ( SteamUser_Original () );

  if (pUser != nullptr)
  {
    if (SK_SteamWrapper_remap_user.count (pUser))
       return SK_SteamWrapper_remap_user [pUser];

    else
    {
      SK_SteamWrapper_remap_user [pUser] =
              new IWrapSteamUser (pUser);

      return SK_SteamWrapper_remap_user [pUser];
    }
  }

  return nullptr;
#endif
}