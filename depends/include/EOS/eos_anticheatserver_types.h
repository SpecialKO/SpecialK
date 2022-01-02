// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_common.h"
#include "eos_anticheatcommon_types.h"

#pragma pack(push, 8)

EXTERN_C typedef struct EOS_AntiCheatServerHandle* EOS_HAntiCheatServer;

/**
 * Callback issued when a new message must be dispatched to a connected client.
 *
 * Messages contain opaque binary data of up to 256 bytes and must be transmitted
 * to the correct client using the game's own networking layer, then delivered
 * to the client anti-cheat instance using the EOS_AntiCheatClient_ReceiveMessageFromServer function.
 *
 * This callback is always issued from within EOS_Platform_Tick on its calling thread.
 */
EOS_DECLARE_CALLBACK(EOS_AntiCheatServer_OnMessageToClientCallback, const EOS_AntiCheatCommon_OnMessageToClientCallbackInfo* Data);

/**
 * Callback issued when an action must be applied to a connected client.
 * This callback is always issued from within EOS_Platform_Tick on its calling thread.
 */
EOS_DECLARE_CALLBACK(EOS_AntiCheatServer_OnClientActionRequiredCallback, const EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo* Data);

/**
 * Optional callback issued when a connected client's authentication status has changed.
 * This callback is always issued from within EOS_Platform_Tick on its calling thread.
 */
EOS_DECLARE_CALLBACK(EOS_AntiCheatServer_OnClientAuthStatusChangedCallback, const EOS_AntiCheatCommon_OnClientAuthStatusChangedCallbackInfo* Data);

#define EOS_ANTICHEATSERVER_ADDNOTIFYMESSAGETOCLIENT_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_AddNotifyMessageToClientOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_ADDNOTIFYMESSAGETOCLIENT_API_LATEST. */
	int32_t ApiVersion;
));

#define EOS_ANTICHEATSERVER_ADDNOTIFYCLIENTACTIONREQUIRED_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_AddNotifyClientActionRequiredOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_ADDNOTIFYCLIENTACTIONREQUIRED_API_LATEST. */
	int32_t ApiVersion;
));

#define EOS_ANTICHEATSERVER_ADDNOTIFYCLIENTAUTHSTATUSCHANGED_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_AddNotifyClientAuthStatusChangedOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_ADDNOTIFYCLIENTAUTHSTATUSCHANGED_API_LATEST. */
	int32_t ApiVersion;
));

/** Limits on RegisterTimeoutSeconds parameter */
#define EOS_ANTICHEATSERVER_BEGINSESSION_MIN_REGISTERTIMEOUT 10
#define EOS_ANTICHEATSERVER_BEGINSESSION_MAX_REGISTERTIMEOUT 120
#define EOS_ANTICHEATSERVER_BEGINSESSION_API_LATEST 3
EOS_STRUCT(EOS_AntiCheatServer_BeginSessionOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_BEGINSESSION_API_LATEST. */
	int32_t ApiVersion;
	/** 
	 * Time in seconds to allow newly registered clients to complete anti-cheat authentication.
	 * Recommended value: 60
	 */
	uint32_t RegisterTimeoutSeconds;
	/** Optional name of this game server */
	const char* ServerName;
	/** 
	 * Gameplay data collection APIs such as LogPlayerTick will be enabled if set to true.
	 * If you do not use these APIs, it is more efficient to set this value to false.
	 */
	EOS_Bool bEnableGameplayData;
	/** The Product User ID of the local user who is associated with this session. Dedicated servers should set this to null. */
	EOS_ProductUserId LocalUserId;
));

#define EOS_ANTICHEATSERVER_ENDSESSION_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_EndSessionOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_ENDSESSION_API_LATEST. */
	int32_t ApiVersion;
));

#define EOS_ANTICHEATSERVER_REGISTERCLIENT_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_RegisterClientOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_REGISTERCLIENT_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value describing the remote user (e.g. a player object pointer) */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** Type of remote user being registered */
	EOS_EAntiCheatCommonClientType ClientType;
	/** Remote user's platform, if known */
	EOS_EAntiCheatCommonClientPlatform ClientPlatform;
	/** 
	 * Identifier for the remote user. This is typically a string representation of an
	 * account ID, but it can be any string which is both unique (two different users will never
	 * have the same string) and consistent (if the same user connects to this game session
	 * twice, the same string will be used) in the scope of a single protected game session.
	 */
	const char* AccountId;
	/** 
	 * Optional IP address for the remote user. May be null if not available.
	 * IPv4 format: "0.0.0.0"
	 * IPv6 format: "0:0:0:0:0:0:0:0"
	 */
	const char* IpAddress;
));

#define EOS_ANTICHEATSERVER_UNREGISTERCLIENT_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_UnregisterClientOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_UNREGISTERCLIENT_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value describing the remote user, as previously passed to RegisterClient */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
));

#define EOS_ANTICHEATSERVER_RECEIVEMESSAGEFROMCLIENT_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_ReceiveMessageFromClientOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_RECEIVEMESSAGEFROMCLIENT_API_LATEST. */
	int32_t ApiVersion;
	/** Optional value, if non-null then only messages addressed to this specific client will be returned */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** The size of the data received */
	uint32_t DataLengthBytes;
	/** The data received */
	const void* Data;
));

#define EOS_ANTICHEATSERVER_SETCLIENTNETWORKSTATE_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_SetClientNetworkStateOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_SETCLIENTNETWORKSTATE_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value describing the remote user (e.g. a player object pointer) */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** True if the network is functioning normally, false if temporarily interrupted */
	EOS_Bool bIsNetworkActive;
));

#define EOS_ANTICHEATSERVER_GETPROTECTMESSAGEOUTPUTLENGTH_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_GetProtectMessageOutputLengthOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_GETPROTECTMESSAGEOUTPUTLENGTH_API_LATEST. */
	int32_t ApiVersion;
	/** Length in bytes of input */
	uint32_t DataLengthBytes;
));

#define EOS_ANTICHEATSERVER_PROTECTMESSAGE_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_ProtectMessageOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_PROTECTMESSAGE_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value describing the remote user to whom the message will be sent */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** Length in bytes of input */
	uint32_t DataLengthBytes;
	/** The data to encrypt */
	const void* Data;
	/** The size in bytes of OutBuffer */
	uint32_t OutBufferSizeBytes;
));

#define EOS_ANTICHEATSERVER_UNPROTECTMESSAGE_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatServer_UnprotectMessageOptions, (
	/** API Version: Set this to EOS_ANTICHEATSERVER_UNPROTECTMESSAGE_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value describing the remote user from whom the message was received */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** Length in bytes of input */
	uint32_t DataLengthBytes;
	/** The data to decrypt */
	const void* Data;
	/** The size in bytes of OutBuffer */
	uint32_t OutBufferSizeBytes;
));

#pragma pack(pop)
