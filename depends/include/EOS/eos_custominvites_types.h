// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_common.h"

#pragma pack(push, 8)

/** Handle to the custom invites interface */
EXTERN_C typedef struct EOS_CustomInvitesHandle* EOS_HCustomInvites;

/** Maximum size of the custom invite payload string */
#define EOS_CUSTOMINVITES_MAX_PAYLOAD_LENGTH 500

/** The most recent version of the EOS_CustomInvites_SetCustomInvite API. */
#define EOS_CUSTOMINVITES_SETCUSTOMINVITE_API_LATEST 1

EOS_STRUCT(EOS_CustomInvites_SetCustomInviteOptions, (
	/** API Version: Set this to EOS_CUSTOMINVITES_SETCUSTOMINVITE_API_LATEST. */
	int32_t ApiVersion;
	/** Local user creating / sending a Custom Invite */
	EOS_ProductUserId LocalUserId;
	/** String payload for the Custom Invite (must be less than EOS_CUSTOMINVITES_MAX_PAYLOAD_LENGTH) */
	const char* Payload;
));

 /** The most recent version of the EOS_CustomInvites_SendCustomInvite API. */
#define EOS_CUSTOMINVITES_SENDCUSTOMINVITE_API_LATEST 1

/**
 * Input parameters for the EOS_CustomInvites_SendCustomInvite function.
 */
EOS_STRUCT(EOS_CustomInvites_SendCustomInviteOptions, (
	/** API Version: Set this to EOS_CUSTOMINVITES_SENDCUSTOMINVITE_API_LATEST. */
	int32_t ApiVersion;
	/** Local user sending a CustomInvite */
	EOS_ProductUserId LocalUserId;
	/** Users to whom the invites should be sent */
	EOS_ProductUserId* TargetUserIds;
	/** The number of users we are sending to */
	uint32_t TargetUserIdsCount;
));

/**
 * Output parameters for the EOS_CustomInvites_SendCustomInvite Function. These parameters are received through the callback provided to EOS_CustomInvites_SendCustomInvite
 */
EOS_STRUCT(EOS_CustomInvites_SendCustomInviteCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_CustomInvites_SendCustomInvite */
	void* ClientData;
	/** Local user sending a CustomInvite */
	EOS_ProductUserId LocalUserId;
	/** Users to whom the invites were successfully sent (can be different than original call if an invite for same Payload was previously sent) */
	EOS_ProductUserId* TargetUserIds;
	/** The number of users we are sending to */
	uint32_t TargetUserIdsCount;
));

/**
 * Function prototype definition for callbacks passed to EOS_CustomInvites_SendCustomInvite
 * @param Data A EOS_CustomInvites_SendCustomInviteCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_CustomInvites_OnSendCustomInviteCallback, const EOS_CustomInvites_SendCustomInviteCallbackInfo* Data);


/** The most recent version of the EOS_CustomInvites_AddNotifyCustomInviteReceived API. */
#define EOS_CUSTOMINVITES_ADDNOTIFYCUSTOMINVITERECEIVED_API_LATEST 1

EOS_STRUCT(EOS_CustomInvites_AddNotifyCustomInviteReceivedOptions, (
	/** API Version: Set this to EOS_CUSTOMINVITES_ADDNOTIFYCUSTOMINVITERECEIVED_API_LATEST. */
	int32_t ApiVersion;
));

/**
 * Output parameters for the EOS_CustomInvites_OnCustomInviteReceivedCallback Function.
 */
EOS_STRUCT(EOS_CustomInvites_OnCustomInviteReceivedCallbackInfo, (
	/** Context that was passed into EOS_CustomInvites_AddNotifyCustomInviteReceived */
	void* ClientData;
	/** User that sent this custom invite */
	EOS_ProductUserId TargetUserId;
	/** Recipient Local user id */
	EOS_ProductUserId LocalUserId;
	/** Id of the received Custom Invite*/
	const char* CustomInviteId;
	/** Payload of the received Custom Invite */
	const char* Payload;
));

/**
 * Function prototype definition for notifications that comes from EOS_CustomInvites_AddNotifyCustomInviteReceived
 *
 * @param Data A EOS_CustomInvites_OnCustomInviteReceivedCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_CustomInvites_OnCustomInviteReceivedCallback, const EOS_CustomInvites_OnCustomInviteReceivedCallbackInfo* Data);


/** The most recent version of the EOS_CustomInvites_AddNotifyCustomInviteAccepted API. */
#define EOS_CUSTOMINVITES_ADDNOTIFYCUSTOMINVITEACCEPTED_API_LATEST 1

EOS_STRUCT(EOS_CustomInvites_AddNotifyCustomInviteAcceptedOptions, (
	/** API Version: Set this to EOS_CUSTOMINVITES_ADDNOTIFYCUSTOMINVITEACCEPTED_API_LATEST. */
	int32_t ApiVersion;
));

/**
 * Output parameters for the EOS_CustomInvites_OnCustomInviteAcceptedCallback Function.
 */
EOS_STRUCT(EOS_CustomInvites_OnCustomInviteAcceptedCallbackInfo, (
	/** Context that was passed into EOS_CustomInvites_AddNotifyCustomInviteAccepted */
	void* ClientData;
	/** User that sent the custom invite */
	EOS_ProductUserId TargetUserId;
	/** Recipient Local user id */
	EOS_ProductUserId LocalUserId;
	/** Id of the accepted Custom Invite */
	const char* CustomInviteId;
	/** Payload of the accepted Custom Invite */
	const char* Payload;
));

/**
 * Function prototype definition for notifications that comes from EOS_CustomInvites_AddNotifyCustomInviteAccepted
 *
 * @param Data A EOS_CustomInvites_OnCustomInviteAcceptedCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_CustomInvites_OnCustomInviteAcceptedCallback, const EOS_CustomInvites_OnCustomInviteAcceptedCallbackInfo* Data);

/** The most recent version of the EOS_CustomInvites_FinalizeInvite API. */
#define EOS_CUSTOMINVITES_FINALIZEINVITE_API_LATEST 1

EOS_STRUCT(EOS_CustomInvites_FinalizeInviteOptions, (
	/** API Version: Set this to EOS_CUSTOMINVITES_FINALIZEINVITE_API_LATEST. */
	int32_t ApiVersion;
	/** User that sent the custom invite */
	EOS_ProductUserId TargetUserId;
	/** Recipient Local user id */
	EOS_ProductUserId LocalUserId;
	/** Id of the Custom Invite accepted */
	const char* CustomInviteId;
	/** Result of the Processing operation, transmitted to Social Overlay if applicable */
	EOS_EResult ProcessingResult;
));

#pragma pack(pop)
