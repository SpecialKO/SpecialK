#include <SpecialK/steam_api.h>

class ISteamFriendsFake : public ISteamFriends
{
public:
  virtual const char          *GetPersonaName                  ( void ) = 0;
  virtual SteamAPICall_t       SetPersonaName                  ( const char *pchPersonaName ) = 0;
  virtual EPersonaState        GetPersonaState                 ( void ) = 0;
  virtual int                  GetFriendCount                  ( int iFriendFlags ) = 0;
  virtual CSteamID             GetFriendByIndex                ( int iFriend, int iFriendFlags ) = 0;
  virtual EFriendRelationship  GetFriendRelationship           ( CSteamID steamIDFriend ) = 0;
  virtual EPersonaState        GetFriendPersonaState           ( CSteamID steamIDFriend ) = 0;
  virtual const char          *GetFriendPersonaName            ( CSteamID steamIDFriend ) = 0;
  virtual bool                 GetFriendGamePlayed             ( CSteamID steamIDFriend, OUT_STRUCT() FriendGameInfo_t *pFriendGameInfo ) = 0;
  virtual const char          *GetFriendPersonaNameHistory     ( CSteamID steamIDFriend, int iPersonaName ) = 0;
  virtual int                  GetFriendSteamLevel             ( CSteamID steamIDFriend ) = 0;
  virtual const char          *GetPlayerNickname               ( CSteamID steamIDPlayer ) = 0;
  virtual int                  GetFriendsGroupCount            ( void ) = 0;
  virtual FriendsGroupID_t     GetFriendsGroupIDByIndex        ( int iFG ) = 0;
  virtual const char          *GetFriendsGroupName             ( FriendsGroupID_t friendsGroupID ) = 0;
  virtual int                  GetFriendsGroupMembersCount     ( FriendsGroupID_t friendsGroupID ) = 0;
  virtual void                 GetFriendsGroupMembersList      ( FriendsGroupID_t friendsGroupID, OUT_ARRAY_CALL(nMembersCount, GetFriendsGroupMembersCount, friendsGroupID ) CSteamID *pOutSteamIDMembers, int nMembersCount ) = 0;
  virtual bool                 HasFriend                       ( CSteamID steamIDFriend, int iFriendFlags ) = 0;
  virtual int                  GetClanCount                    ( void ) = 0;
  virtual CSteamID             GetClanByIndex                  ( int iClan ) = 0;
  virtual const char          *GetClanName                     ( CSteamID steamIDClan ) = 0;
  virtual const char          *GetClanTag                      ( CSteamID steamIDClan ) = 0;
  virtual bool                 GetClanActivityCounts           ( CSteamID steamIDClan, int *pnOnline, int *pnInGame, int *pnChatting ) = 0;
  virtual SteamAPICall_t       DownloadClanActivityCounts      ( ARRAY_COUNT(cClansToRequest) CSteamID *psteamIDClans, int cClansToRequest ) = 0;
  virtual int                  GetFriendCountFromSource        ( CSteamID steamIDSource ) = 0;
  virtual CSteamID             GetFriendFromSourceByIndex      ( CSteamID steamIDSource, int iFriend ) = 0;
  virtual bool                 IsUserInSource                  ( CSteamID steamIDUser, CSteamID steamIDSource ) = 0;
  virtual void                 SetInGameVoiceSpeaking          ( CSteamID steamIDUser, bool bSpeaking) = 0;
  virtual void                 ActivateGameOverlay             ( const char *pchDialog ) = 0;
  virtual void                 ActivateGameOverlayToUser       ( const char *pchDialog, CSteamID steamID) = 0;
  virtual void                 ActivateGameOverlayToWebPage    ( const char *pchURL ) = 0;
  virtual void                 ActivateGameOverlayToStore      ( AppId_t nAppID, EOverlayToStoreFlag eFlag ) = 0;
  virtual void                 SetPlayedWith                   ( CSteamID steamIDUserPlayedWith ) = 0;
  virtual void                 ActivateGameOverlayInviteDialog ( CSteamID steamIDLobby ) = 0;
  virtual int                  GetSmallFriendAvatar            ( CSteamID steamIDFriend ) = 0;
  virtual int                  GetMediumFriendAvatar           ( CSteamID steamIDFriend ) = 0;
  virtual int                  GetLargeFriendAvatar            ( CSteamID steamIDFriend ) = 0;
  virtual bool                 RequestUserInformation          ( CSteamID steamIDUser, bool bRequireNameOnly ) = 0;
  virtual SteamAPICall_t       RequestClanOfficerList          ( CSteamID steamIDClan ) = 0;
  virtual CSteamID             GetClanOwner                    ( CSteamID steamIDClan ) = 0;
  virtual int                  GetClanOfficerCount             ( CSteamID steamIDClan ) = 0;
  virtual CSteamID             GetClanOfficerByIndex           ( CSteamID steamIDClan, int iOfficer ) = 0;
  virtual uint32               GetUserRestrictions             ( void ) = 0;
  virtual bool                 SetRichPresence                 ( const char *pchKey, const char *pchValue ) = 0;
  virtual void                 ClearRichPresence               ( void ) = 0;
  virtual const char          *GetFriendRichPresence           ( CSteamID steamIDFriend, const char *pchKey ) = 0;
  virtual int                  GetFriendRichPresenceKeyCount   ( CSteamID steamIDFriend ) = 0;
  virtual const char          *GetFriendRichPresenceKeyByIndex ( CSteamID steamIDFriend, int iKey ) = 0;
  virtual void                 RequestFriendRichPresence       ( CSteamID steamIDFriend ) = 0;
  virtual bool                 InviteUserToGame                ( CSteamID steamIDFriend, const char *pchConnectString ) = 0;
  virtual int                  GetCoplayFriendCount            ( void ) = 0;
  virtual CSteamID             GetCoplayFriend                 ( int iCoplayFriend ) = 0;
  virtual int                  GetFriendCoplayTime             ( CSteamID steamIDFriend ) = 0;
  virtual AppId_t              GetFriendCoplayGame             ( CSteamID steamIDFriend ) = 0;
  virtual SteamAPICall_t       JoinClanChatRoom                ( CSteamID steamIDClan ) = 0;
  virtual bool                 LeaveClanChatRoom               ( CSteamID steamIDClan ) = 0;
  virtual int                  GetClanChatMemberCount          ( CSteamID steamIDClan ) = 0;
  virtual CSteamID             GetChatMemberByIndex            ( CSteamID steamIDClan, int iUser ) = 0;
  virtual bool                 SendClanChatMessage             ( CSteamID steamIDClanChat, const char *pchText ) = 0;
  virtual int                  GetClanChatMessage              ( CSteamID steamIDClanChat, int iMessage, void *prgchText, int cchTextMax, EChatEntryType *peChatEntryType, OUT_STRUCT() CSteamID *psteamidChatter ) = 0;
  virtual bool                 IsClanChatAdmin                 ( CSteamID steamIDClanChat, CSteamID steamIDUser ) = 0;

  virtual bool                 IsClanChatWindowOpenInSteam     ( CSteamID steamIDClanChat ) override { return pRealFriends->IsClanChatWindowOpenInSteam (steamIDClanChat); }
  virtual bool                 OpenClanChatWindowInSteam       ( CSteamID steamIDClanChat ) override { return pRealFriends->OpenClanChatWindowInSteam   (steamIDClanChat); }
  virtual bool                 CloseClanChatWindowInSteam      ( CSteamID steamIDClanChat ) override { return pRealFriends->CloseClanChatWindowInSteam  (steamIDClanChat); }

  virtual bool                 SetListenForFriendsMessages     ( bool     bInterceptEnabled ) override { return pRealFriends->SetListenForFriendsMessages (bInterceptEnabled); }

  virtual bool                 ReplyToFriendMessage            ( CSteamID        steamIDFriend, const char *pchMsgToSend) = 0;
  virtual int                  GetFriendMessage                ( CSteamID        steamIDFriend,       int    iMessageID,
                                                                 void           *pvData,              int    cubData,
                                                                 EChatEntryType *peChatEntryType) override
  {
    return pRealFriends->GetFriendMessage (steamIDFriend, iMessageID, pvData, cubData, peChatEntryType);
  }

  virtual SteamAPICall_t       GetFollowerCount                ( CSteamID steamID      ) override { return pRealFriends->GetFollowerCount       (steamID);      };
  virtual SteamAPICall_t       IsFollowing                     ( CSteamID steamID      ) override { return pRealFriends->IsFollowing            (steamID);      };
  virtual SteamAPICall_t       EnumerateFollowingList          ( uint32   unStartIndex ) override { return pRealFriends->EnumerateFollowingList (unStartIndex); };


private:
  ISteamFriends* pRealFriends;
};
