// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_common.h"

#pragma pack(push, 8)

/** 
  * Arbitrary data that is a unique local identifier for
  * a single remote client or peer.
  *
  * Typically this is a pointer to an object describing the
  * player, but it can be anything that is locally unique.
  */
EXTERN_C typedef void* EOS_AntiCheatCommon_ClientHandle;

/** Flags describing the type of a remote client */
EOS_ENUM(EOS_EAntiCheatCommonClientType,
	/** An ordinary player that requires anti-cheat client protection to play */
	EOS_ACCCT_ProtectedClient = 0,
	/** The player does not need the anti-cheat client to play because of their platform or other factors */
	EOS_ACCCT_UnprotectedClient = 1,
	/** The client is an AI bot, not an actual human */
	EOS_ACCCT_AIBot = 2
);

/** Flags describing the platform of a remote client, if known */
EOS_ENUM(EOS_EAntiCheatCommonClientPlatform,
	/** Unknown platform */
	EOS_ACCCP_Unknown = 0,
	/** The client is playing on Windows */
	EOS_ACCCP_Windows = 1,
	/** The client is playing on Mac */
	EOS_ACCCP_Mac = 2,
	/** The client is playing on Linux */
	EOS_ACCCP_Linux = 3,
	/** The client is playing on an Xbox device */
	EOS_ACCCP_Xbox = 4,
	/** The client is playing on a PlayStation device */
	EOS_ACCCP_PlayStation = 5,
	/** The client is playing on a Nintendo device */
	EOS_ACCCP_Nintendo = 6,
	/** The client is playing on iOS */
	EOS_ACCCP_iOS = 7,
	/** The client is playing on Android */
	EOS_ACCCP_Android = 8
);

/** Anti-cheat action values. Applicable to both clients and remote peers. */
EOS_ENUM(EOS_EAntiCheatCommonClientAction,
	/** Not used */
	EOS_ACCCA_Invalid = 0,
	/** The client/peer must be removed from the current game session */
	EOS_ACCCA_RemovePlayer = 1
);

/** Anti-cheat action reasons. Applicable to both clients and remote peers. */
EOS_ENUM(EOS_EAntiCheatCommonClientActionReason,
	/** Not used */
	EOS_ACCCAR_Invalid = 0,
	/** An internal error occurred */
	EOS_ACCCAR_InternalError = 1,
	/** An anti-cheat message received from the client/peer was corrupt or invalid */
	EOS_ACCCAR_InvalidMessage = 2,
	/** The client/peer's anti-cheat authentication failed */
	EOS_ACCCAR_AuthenticationFailed = 3,
	/** The client/peer failed to load the anti-cheat module at startup */
	EOS_ACCCAR_NullClient = 4,
	/** The client/peer's anti-cheat heartbeat was not received */
	EOS_ACCCAR_HeartbeatTimeout = 5,
	/** The client/peer failed an anti-cheat client runtime check */
	EOS_ACCCAR_ClientViolation = 6,
	/** The client/peer failed an anti-cheat backend runtime check */
	EOS_ACCCAR_BackendViolation = 7,
	/** The client/peer is temporarily blocked from playing on this game server */
	EOS_ACCCAR_TemporaryCooldown = 8,
	/** The client/peer is temporarily banned */
	EOS_ACCCAR_TemporaryBanned = 9,
	/** The client/peer is permanently banned */
	EOS_ACCCAR_PermanentBanned = 10
);

/** The client/peer's anti-cheat authentication status */
EOS_ENUM(EOS_EAntiCheatCommonClientAuthStatus,
	/** Not used */
	EOS_ACCCAS_Invalid = 0,
	/** The client/peer's anti-cheat functionality has been validated by this game server */
	EOS_ACCCAS_LocalAuthComplete = 1,
	/** The client/peer's anti-cheat functionality has been validated by the anti-cheat backend service */
	EOS_ACCCAS_RemoteAuthComplete = 2
);

/** Flags describing a remote client. These can be updated during a play session */
EOS_ENUM(EOS_EAntiCheatCommonClientFlags,
	/** No particular flags relevant for this client */
	EOS_ACCCF_None = 0,
	/** The client has admin privileges on the game server */
	EOS_ACCCF_Admin = (1 << 0)
);
EOS_ENUM_BOOLEAN_OPERATORS(EOS_EAntiCheatCommonClientFlags);

/** Flags describing the input device used by a remote client, if known. These can be updated during a play session. */
EOS_ENUM(EOS_EAntiCheatCommonClientInput,
	/** Unknown input device */
	EOS_ACCCI_Unknown = 0,
	/** The client is using mouse and keyboard */
	EOS_ACCCI_MouseKeyboard = 1,
	/** The client is using a gamepad or game controller */
	EOS_ACCCI_Gamepad = 2,
	/** The client is using a touch input device (e.g. phone/tablet screen) */
	EOS_ACCCI_TouchInput = 3
);

/**
 * Types supported for custom gameplay behavior events.
 * These have a considerable impact on performance
 */
EOS_ENUM(EOS_EAntiCheatCommonEventType,
	/** Not used */
	EOS_ACCET_Invalid = 0,
	/**
	 * A general game event that is not specific to any individual player.
	 * Low memory use which is constant with respect to the number of players.
	 */
	EOS_ACCET_GameEvent = 1,
	/**
	 * An event that is logically associated with a specific player. Events logged in
	 * this category require a specific ClientHandle to which they will be attached.
	 * Higher memory use which scales according to the number of players.
	 */
	EOS_ACCET_PlayerEvent = 2
);

/** Types supported for custom gameplay behavior event parameters */
EOS_ENUM(EOS_EAntiCheatCommonEventParamType,
	/** Not used */
	EOS_ACCEPT_Invalid = 0,
	/** EOS_AntiCheatCommon_ClientHandle */
	EOS_ACCEPT_ClientHandle = 1,
	/** const char* */
	EOS_ACCEPT_String = 2,
	/** uint32_t */
	EOS_ACCEPT_UInt32 = 3,
	/** int32_t */
	EOS_ACCEPT_Int32 = 4,
	/** uint64_t */
	EOS_ACCEPT_UInt64 = 5,
	/** int64_t */
	EOS_ACCEPT_Int64 = 6,
	/** EOS_AntiCheatCommon_Vec3f */
	EOS_ACCEPT_Vector3f = 7,
	/** EOS_AntiCheatCommon_Quat */
	EOS_ACCEPT_Quat = 8
);

/** Details of a player's movement state */
EOS_ENUM(EOS_EAntiCheatCommonPlayerMovementState,
	/** No particular state applies */
	EOS_ACCPMS_None = 0,
	/** Player is crouching */
	EOS_ACCPMS_Crouching = 1,
	/** Player is prone */
	EOS_ACCPMS_Prone = 2,
	/** Player is mounted in a vehicle or similar */
	EOS_ACCPMS_Mounted = 3,
	/** Player is swimming in a fluid volume */
	EOS_ACCPMS_Swimming = 4,
	/** Player is falling under the effects of gravity, such as when jumping or walking off the edge of a surface */
	EOS_ACCPMS_Falling = 5,
	/** Player is flying, ignoring the effects of gravity */
	EOS_ACCPMS_Flying = 6,
	/** Player is on a ladder */
	EOS_ACCPMS_OnLadder = 7
);

/** The source of a damage event */
EOS_ENUM(EOS_EAntiCheatCommonPlayerTakeDamageSource,
	/** No particular source relevant */
	EOS_ACCPTDS_None = 0,
	/** Damage caused by a player controlled character */
	EOS_ACCPTDS_Player = 1,
	/** Damage caused by a non-player character such as an AI enemy */
	EOS_ACCPTDS_NonPlayerCharacter = 2,
	/** Damage caused by the world (falling off level, into lava, etc) */
	EOS_ACCPTDS_World = 3
);

/** Type of damage applied in a damage event */
EOS_ENUM(EOS_EAntiCheatCommonPlayerTakeDamageType,
	/** No particular type relevant */
	EOS_ACCPTDT_None = 0,
	/** Damage caused by a point source such as a bullet or melee attack */
	EOS_ACCPTDT_PointDamage = 1,
	/** Damage caused by a radial source such as an explosion */
	EOS_ACCPTDT_RadialDamage = 2,
	/** Damage over time such as bleeding, poison, etc */
	EOS_ACCPTDT_DamageOverTime = 3
);

/** The result of a damage event, if any */
EOS_ENUM(EOS_EAntiCheatCommonPlayerTakeDamageResult,
	/** No direct state change consequence for the victim */
	EOS_ACCPTDR_None = 0,
	/** Player character is temporarily incapacitated and requires assistance to recover */
	EOS_ACCPTDR_Downed = 1,
	/** Player character is permanently incapacitated and cannot recover (e.g. dead) */
	EOS_ACCPTDR_Eliminated = 2
);

/** Vector using left-handed coordinate system (as in Unreal Engine) */
EOS_STRUCT(EOS_AntiCheatCommon_Vec3f, (
	/** X axis coordinate - forward direction */
	float x;
	/** Y axis coordinate - right direction */
	float y;
	/** Z axis coordinate - up direction */
	float z;
));

/** Quaternion using left-handed coordinate system (as in Unreal Engine) */
EOS_STRUCT(EOS_AntiCheatCommon_Quat, (
	/** W component - scalar part */
	float w;
	/** X component - forward direction */
	float x;
	/** Y component - right direction */
	float y;
	/** Z component - up direction */
	float z;
));

/**
 * Structure containing details about a new message that must be dispatched to a connected client/peer.
 */
EOS_STRUCT(EOS_AntiCheatCommon_OnMessageToClientCallbackInfo, (
	/** Caller-specified context data */
	void* ClientData;
	/** The identifier of the client/peer that this message must be delivered to. See the RegisterClient and RegisterPeer functions. */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** The message data that must be sent to the client */
	const void* MessageData;
	/** The size in bytes of MessageData */
	uint32_t MessageDataSizeBytes;
));

/** Structure containing details about a required client/peer action */
EOS_STRUCT(EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo, (
	/** Caller-specified context data */
	void* ClientData;
	/** The identifier of the client/peer that this action applies to. See the RegisterClient and RegisterPeer functions. */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** The action that must be applied to the specified client/peer */
	EOS_EAntiCheatCommonClientAction ClientAction;
	/** Code indicating the reason for the action. This can be displayed to the affected player. */
	EOS_EAntiCheatCommonClientActionReason ActionReasonCode;
	/** String containing details about the action reason. This can be displayed to the affected player. */
	const char* ActionReasonDetailsString;
));

/** Structure containing details about a client/peer authentication status change */
EOS_STRUCT(EOS_AntiCheatCommon_OnClientAuthStatusChangedCallbackInfo, (
	/** Caller-specified context data */
	void* ClientData;
	/** The identifier of the client/peer that this status change applies to. See the RegisterClient and RegisterPeer functions. */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** The client/peer's new authentication status */
	EOS_EAntiCheatCommonClientAuthStatus ClientAuthStatus;
));

#define EOS_ANTICHEATCOMMON_SETCLIENTDETAILS_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatCommon_SetClientDetailsOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_SETCLIENTDETAILS_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** General flags associated with this client, if any */
	EOS_EAntiCheatCommonClientFlags ClientFlags;
	/** Input device being used by this client, if known */
	EOS_EAntiCheatCommonClientInput ClientInputMethod;
));

#define EOS_ANTICHEATCOMMON_SETGAMESESSIONID_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatCommon_SetGameSessionIdOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_SETGAMESESSIONID_API_LATEST. */
	int32_t ApiVersion;
	/** Game session identifier */
	const char* GameSessionId;
));

#define EOS_ANTICHEATCOMMON_REGISTEREVENT_API_LATEST 1
#define EOS_ANTICHEATCOMMON_REGISTEREVENT_CUSTOMEVENTBASE 0x10000000
#define EOS_ANTICHEATCOMMON_REGISTEREVENT_MAX_PARAMDEFSCOUNT 12
EOS_STRUCT(EOS_AntiCheatCommon_RegisterEventParamDef, (
	/** Parameter name. Allowed characters are 0-9, A-Z, a-z, '_', '-' */
	const char* ParamName;
	/** Parameter type */
	EOS_EAntiCheatCommonEventParamType ParamType;
));
EOS_STRUCT(EOS_AntiCheatCommon_RegisterEventOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_REGISTEREVENT_API_LATEST. */
	int32_t ApiVersion;
	/** Unique event identifier. Must be >= EOS_ANTICHEATCOMMON_REGISTEREVENT_CUSTOMEVENTBASE. */
	uint32_t EventId;
	/** Name of the custom event. Allowed characters are 0-9, A-Z, a-z, '_', '-' */
	const char* EventName;
	/** Type of the custom event */
	EOS_EAntiCheatCommonEventType EventType;
	/** Number of parameters described in ParamDefs. Must be <= EOS_ANTICHEATCOMMON_REGISTEREVENT_MAX_PARAMDEFSCOUNT. */
	uint32_t ParamDefsCount;
	/** Pointer to an array of EOS_AntiCheatCommon_RegisterEventParamDef with ParamDefsCount elements */
	const EOS_AntiCheatCommon_RegisterEventParamDef* ParamDefs;
));

#define EOS_ANTICHEATCOMMON_LOGEVENT_API_LATEST 1
#define EOS_ANTICHEATCOMMON_LOGEVENT_STRING_MAX_LENGTH 39
EOS_STRUCT(EOS_AntiCheatCommon_LogEventParamPair, (
	/** Parameter type */
	EOS_EAntiCheatCommonEventParamType ParamValueType;
	/** Parameter value */
	union
	{
		EOS_AntiCheatCommon_ClientHandle ClientHandle;
		const char* String; // Will be truncated if longer than EOS_ANTICHEATCOMMON_LOGEVENT_STRING_MAX_LENGTH bytes.
		uint32_t UInt32;
		int32_t Int32;
		uint64_t UInt64;
		int64_t Int64;
		EOS_AntiCheatCommon_Vec3f Vec3f;
		EOS_AntiCheatCommon_Quat Quat;
	} ParamValue;
));
EOS_STRUCT(EOS_AntiCheatCommon_LogEventOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGEVENT_API_LATEST. */
	int32_t ApiVersion;
	/** Optional client who this event is primarily associated with. If not applicable, use 0. */
	EOS_AntiCheatCommon_ClientHandle ClientHandle;
	/** Unique event identifier previously configured in RegisterEvent */
	uint32_t EventId;
	/** Number of parameters described in Params */
	uint32_t ParamsCount;
	/** Set of parameter types previously configured in RegisterEvent, and their values */
	const EOS_AntiCheatCommon_LogEventParamPair* Params;
));

#define EOS_ANTICHEATCOMMON_LOGGAMEROUNDSTART_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatCommon_LogGameRoundStartOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGGAMEROUNDSTART_API_LATEST. */
	int32_t ApiVersion;
	/** Optional game session or match identifier useful for some backend API integrations */
	const char* SessionIdentifier;
	/** Optional name of the map being played */
	const char* LevelName;
	/** Optional name of the game mode being played */
	const char* ModeName;
	/** Optional length of the game round to be played, in seconds. If none, use 0. */
	uint32_t RoundTimeSeconds;
));

#define EOS_ANTICHEATCOMMON_LOGGAMEROUNDEND_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatCommon_LogGameRoundEndOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGGAMEROUNDEND_API_LATEST. */
	int32_t ApiVersion;
	/** Optional identifier for the winning team */
	uint32_t WinningTeamId;
));

#define EOS_ANTICHEATCOMMON_LOGPLAYERSPAWN_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatCommon_LogPlayerSpawnOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGPLAYERSPAWN_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle SpawnedPlayerHandle;
	/** Optional identifier for the player's team. If none, use 0. */
	uint32_t TeamId;
	/** Optional identifier for the player's character. If none, use 0. */
	uint32_t CharacterId;
));

#define EOS_ANTICHEATCOMMON_LOGPLAYERDESPAWN_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatCommon_LogPlayerDespawnOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGPLAYERDESPAWN_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle DespawnedPlayerHandle;
));

#define EOS_ANTICHEATCOMMON_LOGPLAYERREVIVE_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatCommon_LogPlayerReviveOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGPLAYERREVIVE_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle RevivedPlayerHandle;
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle ReviverPlayerHandle;
));

#define EOS_ANTICHEATCOMMON_LOGPLAYERTICK_API_LATEST 2
EOS_STRUCT(EOS_AntiCheatCommon_LogPlayerTickOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGPLAYERTICK_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle PlayerHandle;
	/** Player's current world position as a 3D vector */
	EOS_AntiCheatCommon_Vec3f* PlayerPosition;
	/** Player's view rotation as a quaternion */
	EOS_AntiCheatCommon_Quat* PlayerViewRotation;
	/** True if the player's view is zoomed (e.g. using a sniper rifle), otherwise false */
	EOS_Bool bIsPlayerViewZoomed;
	/** Player's current health value */
	float PlayerHealth;
	/** Any movement state applicable */
	EOS_EAntiCheatCommonPlayerMovementState PlayerMovementState;
));

#define EOS_ANTICHEATCOMMON_LOGPLAYERUSEWEAPON_API_LATEST 2
#define EOS_ANTICHEATCOMMON_LOGPLAYERUSEWEAPON_WEAPONNAME_MAX_LENGTH 16
EOS_STRUCT(EOS_AntiCheatCommon_LogPlayerUseWeaponData, (
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle PlayerHandle;
	/** Attack origin world position as a 3D vector */
	EOS_AntiCheatCommon_Vec3f* PlayerPosition;
	/** Attack direction as a quaternion */
	EOS_AntiCheatCommon_Quat* PlayerViewRotation;
	/** True if the player's view is zoomed (e.g. using a sniper rifle), otherwise false */
	EOS_Bool bIsPlayerViewZoomed;
	/** Set to true if the player is using a melee attack, otherwise false */
	EOS_Bool bIsMeleeAttack;
	/** Name of the weapon used. Will be truncated to EOS_ANTICHEATCOMMON_LOGPLAYERUSEWEAPON_WEAPONNAME_MAX_LENGTH bytes if longer. */
	const char* WeaponName;
));
EOS_STRUCT(EOS_AntiCheatCommon_LogPlayerUseWeaponOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGPLAYERUSEWEAPON_API_LATEST. */
	int32_t ApiVersion;
	/** Struct containing detailed information about a weapon use event */
	EOS_AntiCheatCommon_LogPlayerUseWeaponData* UseWeaponData;
));

#define EOS_ANTICHEATCOMMON_LOGPLAYERUSEABILITY_API_LATEST 1
EOS_STRUCT(EOS_AntiCheatCommon_LogPlayerUseAbilityOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGPLAYERUSEABILITY_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle PlayerHandle;
	/** Game defined unique identifier for the ability being used */
	uint32_t AbilityId;
	/** Duration of the ability effect in milliseconds. If not applicable, use 0. */
	uint32_t AbilityDurationMs;
	/** Cooldown until the ability can be used again in milliseconds. If not applicable, use 0. */
	uint32_t AbilityCooldownMs;
));

#define EOS_ANTICHEATCOMMON_LOGPLAYERTAKEDAMAGE_API_LATEST 3
EOS_STRUCT(EOS_AntiCheatCommon_LogPlayerTakeDamageOptions, (
	/** API Version: Set this to EOS_ANTICHEATCOMMON_LOGPLAYERTAKEDAMAGE_API_LATEST. */
	int32_t ApiVersion;
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle VictimPlayerHandle;
	/** Victim player's current world position as a 3D vector */
	EOS_AntiCheatCommon_Vec3f* VictimPlayerPosition;
	/** Victim player's view rotation as a quaternion */
	EOS_AntiCheatCommon_Quat* VictimPlayerViewRotation;
	/** Locally unique value used in RegisterClient/RegisterPeer */
	EOS_AntiCheatCommon_ClientHandle AttackerPlayerHandle;
	/** Attacker player's current world position as a 3D vector */
	EOS_AntiCheatCommon_Vec3f* AttackerPlayerPosition;
	/** Attacker player's view rotation as a quaternion */
	EOS_AntiCheatCommon_Quat* AttackerPlayerViewRotation;
	/**
	 * True if the damage was applied instantly at the time of attack from the game
	 * simulation's perspective, otherwise false (simulated ballistics, arrow, etc).
	 */
	EOS_Bool bIsHitscanAttack;
	/**
	 * True if there is a visible line of sight between the attacker and the victim at the time
	 * that damage is being applied, false if there is an obstacle like a wall or terrain in
	 * the way. For some situations like melee or hitscan weapons this is trivially
	 * true, for others like projectiles with simulated physics it may not be e.g. a player
	 * could fire a slow moving projectile and then move behind cover before it strikes.
	 */
	EOS_Bool bHasLineOfSight;
	/** True if this was a critical hit that causes extra damage (e.g. headshot) */
	EOS_Bool bIsCriticalHit;
	/** Deprecated - use DamagePosition instead */
	uint32_t HitBoneId_DEPRECATED;
	/** Number of health points that the victim lost due to this damage event */
	float DamageTaken;
	/** Number of health points that the victim has remaining after this damage event */
	float HealthRemaining;
	/** Source of the damage event */
	EOS_EAntiCheatCommonPlayerTakeDamageSource DamageSource;
	/** Type of the damage being applied */
	EOS_EAntiCheatCommonPlayerTakeDamageType DamageType;
	/** Result of the damage for the victim, if any */
	EOS_EAntiCheatCommonPlayerTakeDamageResult DamageResult;
	/** PlayerUseWeaponData associated with this damage event if available, otherwise NULL */
	EOS_AntiCheatCommon_LogPlayerUseWeaponData* PlayerUseWeaponData;
	/** Time in milliseconds since the PlayerUseWeaponData event occurred if available, otherwise 0 */
	uint32_t TimeSincePlayerUseWeaponMs;
	/** World position where damage hit the victim as a 3D vector if available, otherwise NULL */
	EOS_AntiCheatCommon_Vec3f* DamagePosition;
));

#pragma pack(pop)
