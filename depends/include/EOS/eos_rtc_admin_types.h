// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_common.h"

#pragma pack(push, 8)

EXTERN_C typedef struct EOS_RTCAdminHandle* EOS_HRTCAdmin;

/** The most recent version of the EOS_RTCAdmin_QueryJoinRoomToken API */
#define EOS_RTCADMIN_QUERYJOINROOMTOKEN_API_LATEST 2

/**
 * Input parameters for the EOS_RTCAdmin_QueryJoinRoomToken function.
 */
EOS_STRUCT(EOS_RTCAdmin_QueryJoinRoomTokenOptions, (
	/** API Version: Set this to EOS_RTCADMIN_QUERYJOINROOMTOKEN_API_LATEST. */
	int32_t ApiVersion;
	/** Product User ID for local user who is querying join room tokens. */
	EOS_ProductUserId LocalUserId;
	/** Room name to request a token for. */
	const char* RoomName;
	/** An array of Product User IDs indicating the users to retrieve a token for. */
	EOS_ProductUserId* TargetUserIds;
	/** The number of users included in the query. */
	uint32_t TargetUserIdsCount;
	/**
	 * Array of IP Addresses, one for each of the users we're querying tokens for.
	 * There should be TargetUserIdsCount Ip Addresses, you can set an entry to NULL if not known.
	 * If TargetUserIpAddresses is set to NULL IP Addresses will be ignored.
	 * IPv4 format: "0.0.0.0"
	 * IPv6 format: "0:0:0:0:0:0:0:0"
	 */
	const char** TargetUserIpAddresses;
));

/**
 * Data containing the result information for a query join room token request.
 */
EOS_STRUCT(EOS_RTCAdmin_QueryJoinRoomTokenCompleteCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_RTCAdmin_QueryJoinRoomToken. */
	void* ClientData;
	/** Room the request was made for. */
	const char* RoomName;
	/** URL passed to backend to join room. */
	const char* ClientBaseUrl;
	/** If the query completed successfully, this contains an identifier that should be used to retrieve the tokens.
	 * This identifier is only valid for the duration of the callback. 
	 *
	 * @see EOS_RTCAdmin_CopyUserTokenByIndex
	 * @see EOS_RTCAdmin_CopyUserTokenByUserId
	 */
	uint32_t QueryId;
	/** How many token received as result of the query */
	uint32_t TokenCount;
));

/**
 * Function prototype definition for callbacks passed to EOS_RTCAdmin_QueryJoinRoomToken
 * @param Data An EOS_RTCAdmin_QueryJoinRoomTokenCompleteCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_RTCAdmin_OnQueryJoinRoomTokenCompleteCallback, const EOS_RTCAdmin_QueryJoinRoomTokenCompleteCallbackInfo* Data);

/** The most recent version of the EOS_RTCAdmin_UserToken struct. */
#define EOS_RTCADMIN_USERTOKEN_API_LATEST 1

/**
 * Contains information about a collection of user tokens for joining a room.
 */
EOS_STRUCT(EOS_RTCAdmin_UserToken, (
	/** API Version: Set this to EOS_RTCADMIN_USERTOKEN_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID for the user who owns this user token. */
	EOS_ProductUserId ProductUserId;
	/** Access token to enable a user to join a room */
	const char* Token;
));

/** The most recent version of the EOS_RTCAdmin_CopyUserTokenByIndexOptions struct. */
#define EOS_RTCADMIN_COPYUSERTOKENBYINDEX_API_LATEST 2

/**
 * Input parameters for the EOS_RTCAdmin_CopyUserTokenByIndex function.
 */
EOS_STRUCT(EOS_RTCAdmin_CopyUserTokenByIndexOptions, (
	/** API Version: Set this to EOS_RTCADMIN_COPYUSERTOKENBYINDEX_API_LATEST. */
	int32_t ApiVersion;
	/** Index of the user token to retrieve from the cache. */
	uint32_t UserTokenIndex;
	/** Query identifier received as part of a previous query.
	 * @see EOS_RTCAdmin_QueryJoinRoomTokenCompleteCallbackInfo
	 */
	uint32_t QueryId;
));

/** The most recent version of the EOS_RTCAdmin_CopyUserTokenByUserIdOptions struct. */
#define EOS_RTCADMIN_COPYUSERTOKENBYUSERID_API_LATEST 2

/**
 * Input parameters for the EOS_RTCAdmin_CopyUserTokenByUserId function.
 */
EOS_STRUCT(EOS_RTCAdmin_CopyUserTokenByUserIdOptions, (
	/** API Version: Set this to EOS_RTCADMIN_COPYUSERTOKENBYUSERID_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID for the user whose user token we're copying. */
	EOS_ProductUserId TargetUserId;
	/** Query identifier received as part of a previous query.
	 * @see EOS_RTCAdmin_QueryJoinRoomTokenCompleteCallbackInfo
	 */
	uint32_t QueryId;
));

/**
 * Release the memory associated with EOS_RTCAdmin_UserToken. This must be called on data retrieved from
 * EOS_RTCAdmin_CopyUserTokenByIndex or EOS_RTCAdmin_CopyUserTokenByUserId.
 *
 * @param UserToken - The user token to release.
 *
 * @see EOS_RTCAdmin_UserToken
 * @see EOS_RTCAdmin_CopyUserTokenByIndex
 * @see EOS_RTCAdmin_CopyUserTokenByUserId
 */
EOS_DECLARE_FUNC(void) EOS_RTCAdmin_UserToken_Release(EOS_RTCAdmin_UserToken* UserToken);

/** The most recent version of the EOS_RTCAdmin_Kick API */
#define EOS_RTCADMIN_KICK_API_LATEST 1

/**
 * Input parameters for the EOS_RTCAdmin_Kick function.
 */
EOS_STRUCT(EOS_RTCAdmin_KickOptions, (
	/** API Version: Set this to EOS_RTCADMIN_KICK_API_LATEST. */
	int32_t ApiVersion;
	/** Room name to kick the participant from */
	const char* RoomName;
	/** Product User ID of the participant to kick from the room */
	EOS_ProductUserId TargetUserId;
));

/**
 * Data containing the result information for a kick participant request.
 */
EOS_STRUCT(EOS_RTCAdmin_KickCompleteCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Client-specified data passed into the kick request */
	void* ClientData;
));

/**
 * Function prototype definition for callbacks passed to EOS_RTCAdmin_Kick
 * @param Data An EOS_RTCAdmin_KickCompleteCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_RTCAdmin_OnKickCompleteCallback, const EOS_RTCAdmin_KickCompleteCallbackInfo* Data);

/** The most recent version of the EOS_RTCAdmin_SetParticipantHardMuteOptions struct. */
#define EOS_RTCADMIN_SETPARTICIPANTHARDMUTE_API_LATEST 1

/**
 * Input parameters for the EOS_RTCAdmin_SetParticipantHardMute function.
 */
EOS_STRUCT(EOS_RTCAdmin_SetParticipantHardMuteOptions, (
	/** API Version: Set this to EOS_RTCADMIN_SETPARTICIPANTHARDMUTE_API_LATEST. */
	int32_t ApiVersion;
	/** Room to kick the participant from */
	const char* RoomName;
	/** Product User ID of the participant to hard mute for every participant in the room. */
	EOS_ProductUserId TargetUserId;
	/** Hard mute status (Mute on or off) */
	EOS_Bool bMute;
));

/**
 * Data containing the result information for a hard mute request.
 */
EOS_STRUCT(EOS_RTCAdmin_SetParticipantHardMuteCompleteCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Client-specified data passed into the hard mute request */
	void* ClientData;
));

EOS_DECLARE_CALLBACK(EOS_RTCAdmin_OnSetParticipantHardMuteCompleteCallback, const EOS_RTCAdmin_SetParticipantHardMuteCompleteCallbackInfo* Data);

#pragma pack(pop)

