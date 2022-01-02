// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_common.h"
#include "eos_anticheatcommon_types.h"

#pragma pack(push, 8)

EXTERN_C typedef struct EOS_AntiCheatClientHandle* EOS_HAntiCheatClient;

/** Operating modes */
EOS_ENUM(EOS_EAntiCheatClientMode,
	/** Not used */
	EOS_ACCM_Invalid = 0,
	/** Dedicated or listen server mode */
	EOS_ACCM_ClientServer = 1,
	/** Full mesh peer-to-peer mode */
	EOS_ACCM_PeerToPeer = 2
);

/** Anti-cheat integrity violation types */
EOS_ENUM(EOS_EAntiCheatClientViolationType,
	/** Not used */
	EOS_ACCVT_Invalid = 0,
	/** An anti-cheat asset integrity catalog file could not be found */
	EOS_ACCVT_IntegrityCatalogNotFound = 1,
	/** An anti-cheat asset integrity catalog file is corrupt or invalid */
	EOS_ACCVT_IntegrityCatalogError = 2,
	/** An anti-cheat asset integrity catalog file's certificate has been revoked. */
	EOS_ACCVT_IntegrityCatalogCertificateRevoked = 3,
	/**
	 * The primary anti-cheat asset integrity catalog does not include an entry for the game's
	 * main executable, which is required.
	 */
	EOS_ACCVT_IntegrityCatalogMissingMainExecutable = 4,
	/** A disallowed game file modification was detected */
	EOS_ACCVT_GameFileMismatch = 5,
	/** A disallowed game file removal was detected */
	EOS_ACCVT_RequiredGameFileNotFound = 6,
	/** A disallowed game file addition was detected */
	EOS_ACCVT_UnknownGameFileForbidden = 7,
	/** A system file failed an integrity check */
	EOS_ACCVT_SystemFileUntrusted = 8,
	/** A disallowed code module was loaded into the game process */
	EOS_ACCVT_ForbiddenModuleLoaded = 9,
	/** A disallowed game process memory modification was detected */
	EOS_ACCVT_CorruptedMemory = 10,
	/** A disallowed tool was detected running in the system */
	EOS_ACCVT_ForbiddenToolDetected = 11,
	/** An internal anti-cheat integrity check failed */
	EOS_ACCVT_InternalAntiCheatViolation = 12,
	/** Integrity checks on messages between the game client and game server failed */
	EOS_ACCVT_CorruptedNetworkMessageFlow = 13,
	/** The game is running inside a disallowed virtual machine */
	EOS_ACCVT_VirtualMachineNotAllowed = 14,
	/** A forbidden operating system configuration was detected */
	EOS_ACCVT_ForbiddenSystemConfiguration = 15
);

/**
 * Structure containing details about a new message that must be dispatched to the game server.
 */
EOS_STRUCT(EOS_AntiCheatClient_OnMessageToServerCallbackInfo, (
	/** Caller-specified context data */
	void* ClientData;
	/** The message data that must be sent to the server */
	const void* MessageData;
	/** The size in bytes of MessageData */
	uint32_t MessageDataSizeBytes;
));

/**
 * Callback issued when a new message must be dispatched to the game server.
 *
 * Messages contain opaque binary data of up to 256 bytes and must be transmitted
 * to the game server using the game's own networking layer, then delivered
 * to the server anti-cheat instance using the EOS_AntiCheatServer_ReceiveMessageFromClient function.
 *
 * This callback is always issued from within EOS_Platform_Tick on its calling thread.
 */
EOS_DECLARE_CALLBACK(EOS_AntiCheatClient_OnMessageToServerCallback, const EOS_AntiCheatClient_OnMessageToServerCallbackInfo* Data);

/**
 * Callback issued when a new message must be dispatched to a connected peer.
 *
 * Messages contain opaque binary data of up to 256 bytes and must be transmitted
 * to the correct peer using the game's own networking layer, then delivered
 * to the client anti-cheat instance using the EOS_AntiCheatClient_ReceiveMessageFromPeer function.
 *
 * This callback is always issued from within EOS_Platform_Tick on its calling thread.
 */
EOS_DECLARE_CALLBACK(EOS_AntiCheatClient_OnMessageToPeerCallback, const EOS_AntiCheatCommon_OnMessageToClientCallbackInfo* Data);

/**
 * Callback issued when an action must be applied to a connected peer.
 * This callback is always issued from within EOS_Platform_Tick on its calling thread.
 */
EOS_DECLARE_CALLBACK(EOS_AntiCheatClient_OnPeerActionRequiredCallback, const EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo* Data);

/**
 * Optional callback issued when a connected peer's authentication status has changed.
 * This callback is always issued from within EOS_Platform_Tick on its calling thread.
 */
EOS_DECLARE_CALLBACK(EOS_AntiCheatClient_OnPeerAuthStatusChangedCallback, const EOS_AntiCheatCommon_OnClientAuthStatusChangedCallbackInfo* Data);

#define EOS_ANTICHEATCLIENT_ADDNOTIFYMESSAGETOSERVER_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_AddNotifyMessageToServerOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_ADDNOTIFYMESSAGETOSERVER_API_LATEST. */
	int32_t ApiVersion;
));

#define EOS_ANTICHEATCLIENT_ADDNOTIFYMESSAGETOPEER_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_AddNotifyMessageToPeerOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_ADDNOTIFYMESSAGETOPEER_API_LATEST. */
	int32_t ApiVersion;
));

#define EOS_ANTICHEATCLIENT_ADDNOTIFYPEERACTIONREQUIRED_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_AddNotifyPeerActionRequiredOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_ADDNOTIFYPEERACTIONREQUIRED_API_LATEST. */
	int32_t ApiVersion;
));

#define EOS_ANTICHEATCLIENT_ADDNOTIFYPEERAUTHSTATUSCHANGED_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_AddNotifyPeerAuthStatusChangedOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_ADDNOTIFYPEERAUTHSTATUSCHANGED_API_LATEST. */
	int32_t ApiVersion;
));

#define EOS_ANTICHEATCLIENT_BEGINSESSION_API_LATEST 3
EOS_STRUCT(EOS_AntiCheatClient_BeginSessionOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_BEGINSESSION_API_LATEST. */
	int32_t ApiVersion;
	/** Logged in user identifier from earlier call to EOS_Connect_Login family of functions */
	EOS_ProductUserId LocalUserId;
	/** Operating mode */
	EOS_EAntiCheatClientMode Mode;
));

#define EOS_ANTICHEATCLIENT_ENDSESSION_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_EndSessionOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_ENDSESSION_API_LATEST. */
	int32_t ApiVersion;
));

#define EOS_ANTICHEATCLIENT_POLLSTATUS_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_PollStatusOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_POLLSTATUS_API_LATEST. */
	int32_t ApiVersion;
	/** The size of OutMessage in bytes. Recommended size is 256 bytes. */
	uint32_t OutMessageLength;
));

#define EOS_ANTICHEATCLIENT_ADDEXTERNALINTEGRITYCATALOG_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_AddExternalIntegrityCatalogOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_ADDEXTERNALINTEGRITYCATALOG_API_LATEST. */
	int32_t ApiVersion;
	/** UTF-8 path to the .bin catalog file to add */
	const char* PathToBinFile;
));

#define EOS_ANTICHEATCLIENT_RECEIVEMESSAGEFROMSERVER_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_ReceiveMessageFromServerOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_RECEIVEMESSAGEFROMSERVER_API_LATEST. */
	int32_t ApiVersion;
	/** The size of the data received */
	uint32_t DataLengthBytes;
	/** The data received */
	const void* Data;
));

#define EOS_ANTICHEATCLIENT_GETPROTECTMESSAGEOUTPUTLENGTH_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_GetProtectMessageOutputLengthOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_GETPROTECTMESSAGEOUTPUTLENGTH_API_LATEST. */
	int32_t ApiVersion;
	/** Length in bytes of input */
	uint32_t DataLengthBytes;
));

#define EOS_ANTICHEATCLIENT_PROTECTMESSAGE_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_ProtectMessageOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_PROTECTMESSAGE_API_LATEST. */
	int32_t ApiVersion;
	/** Length in bytes of input */
	uint32_t DataLengthBytes;
	/** The data to encrypt */
	const void* Data;
	/** The size in bytes of OutBuffer */
	uint32_t OutBufferSizeBytes;
));

#define EOS_ANTICHEATCLIENT_UNPROTECTMESSAGE_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_UnprotectMessageOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_UNPROTECTMESSAGE_API_LATEST. */
	int32_t ApiVersion;
	/** Length in bytes of input */
	uint32_t DataLengthBytes;
	/** The data to decrypt */
	const void* Data;
	/** The size in bytes of OutBuffer */
	uint32_t OutBufferSizeBytes;
));

/**
 * A special peer handle that represents the client itself.
 * It does not need to be registered or unregistered and is
 * used in OnPeerActionRequiredCallback to quickly signal to the user
 * that they will not be able to join online play.
 */
#define EOS_ANTICHEATCLIENT_PEER_SELF (EOS_AntiCheatCommon_ClientHandle)(-1)

#define EOS_ANTICHEATCLIENT_REGISTERPEER_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_RegisterPeerOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_REGISTERPEER_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value describing the remote user (e.g. a player object pointer) */
	EOS_AntiCheatCommon_ClientHandle PeerHandle;
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

#define EOS_ANTICHEATCLIENT_UNREGISTERPEER_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_UnregisterPeerOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_UNREGISTERPEER_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value describing the remote user, as previously passed to EOS_AntiCheatClient_RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle PeerHandle;
));

#define EOS_ANTICHEATCLIENT_RECEIVEMESSAGEFROMPEER_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatClient_ReceiveMessageFromPeerOptions, (
	/** API Version: Set this to EOS_ANTICHEATCLIENT_RECEIVEMESSAGEFROMPEER_API_LATEST. */
	int32_t ApiVersion;
	/** The handle describing the sender of this message */
	EOS_AntiCheatCommon_ClientHandle PeerHandle;
	/** The size of the data received */
	uint32_t DataLengthBytes;
	/** The data received */
	const void* Data;
));

#pragma pack(pop)
