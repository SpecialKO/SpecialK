// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_common.h"

#pragma pack(push, 8)

EXTERN_C typedef struct EOS_RTCHandle* EOS_HRTC;

/** Participant RTC's status change */
EOS_ENUM(EOS_ERTCParticipantStatus,
	/** Participant joined the room */
	EOS_RTCPS_Joined = 0,
	/** Participant left the room */
	EOS_RTCPS_Left = 1
);

/** The most recent version of the EOS_RTC_JoinRoom API. */
#define EOS_RTC_JOINROOM_API_LATEST 1

/**
 * Enables echo mode.
 * This can be used during development to have the server send your voice back to you so you don't need 2 clients to test
 * if voice is being sent and received.
 *
 * @see EOS_RTC_JoinRoomOptions::Flags
 */
#define EOS_RTC_JOINROOMFLAGS_ENABLE_ECHO 0x01

/**
 * This struct is used to call EOS_RTC_JoinRoom.
 */
EOS_STRUCT(EOS_RTC_JoinRoomOptions, (
	/** API Version: Set this to EOS_RTC_JOINROOM_API_LATEST. */
	int32_t ApiVersion;
	/** The product user id of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The room the user would like to join. */
	const char* RoomName;
	/** The room the user would like to join. */
	const char* ClientBaseUrl;
	/** Authorization credential token to join the room. */
	const char* ParticipantToken;
	/** The participant id used to join the room. If set to NULL the LocalUserId will be used instead. */
	EOS_ProductUserId ParticipantId;

	/** Join room flags, e.g. EOS_RTC_JOINROOMFLAGS_ENABLE_ECHO. This is a bitwise-or union of the defined flags. */
	uint32_t Flags;
	/** Enable or disable Manual Audio Input. If manual audio input is enabled audio recording is not started and the audio
	 * buffers must be passed manually using EOS_RTCAudio_SendAudio.
	 */
	EOS_Bool bManualAudioInputEnabled;
	/** Enable or disable Manual Audio Output. If manual audio output is enabled audio rendering is not started and the audio
	 * buffers must be received with EOS_RTCAudio_AddNotifyAudioBeforeRender and rendered manually.
	 */
	EOS_Bool bManualAudioOutputEnabled;
));

/**
 * This struct is passed in with a call to EOS_RTC_OnJoinRoomCallback.
 */
EOS_STRUCT(EOS_RTC_JoinRoomCallbackInfo, (
	/** This returns:
	 * EOS_Success if the channel was successfully joined.
	 * EOS_NoConnection: unable to connect to RTC servers (retryable).
	 * EOS_InvalidAuth: if the token is invalid (not retryable).
	 * EOS_RTC_TooManyParticipants: if the room cannot accept more participants (not retryable).
	 * EOS_AccessDenied: if the room name belongs to the Lobby voice system (not retryable).
	 * EOS_UnexpectedError otherwise (retryable).
	 */
	EOS_EResult ResultCode;
	/** Client-specified data passed into EOS_RTC_JoinRoom. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room the user was trying to join. */
	const char* RoomName;
));

/**
 * Callback for completion of room join request.
 */
EOS_DECLARE_CALLBACK(EOS_RTC_OnJoinRoomCallback, const EOS_RTC_JoinRoomCallbackInfo* Data);

/** The most recent version of the EOS_RTC_LeaveRoom API. */
#define EOS_RTC_LEAVEROOM_API_LATEST 1

/**
 * This struct is used to call EOS_RTC_LeaveRoom.
 */
EOS_STRUCT(EOS_RTC_LeaveRoomOptions, (
	/** API Version: Set this to EOS_RTC_LEAVEROOM_API_LATEST. */
	int32_t ApiVersion;
	/** Product User ID of the user requesting to leave the room */
	EOS_ProductUserId LocalUserId;
	/** The room to leave. */
	const char* RoomName;
));

/**
 * This struct is passed in with a call to EOS_RTC_OnLeaveRoomCallback.
 */
EOS_STRUCT(EOS_RTC_LeaveRoomCallbackInfo, (
	/** This returns:
	 * EOS_Success if the channel was successfully left.
	 * EOS_AccessDenied if the room name belongs to the Lobby voice system.
	 * EOS_UnexpectedError otherwise.
	 */
	EOS_EResult ResultCode;
	/** Client-specified data passed into EOS_RTC_LeaveRoomOptions. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room the user was trying to leave. */
	const char* RoomName;
));

/**
 * Callback for completion of room leave request.
 */
EOS_DECLARE_CALLBACK(EOS_RTC_OnLeaveRoomCallback, const EOS_RTC_LeaveRoomCallbackInfo* Data);

/** The most recent version of the EOS_RTC_BlockParticipant API. */
#define EOS_RTC_BLOCKPARTICIPANT_API_LATEST 1

/**
 * This struct is passed in with a call to EOS_RTC_BlockParticipant.
 */
EOS_STRUCT(EOS_RTC_BlockParticipantOptions, (
	/** API Version: Set this to EOS_RTC_BLOCKPARTICIPANT_API_LATEST. */
	int32_t ApiVersion;
	/** Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The room the users should be blocked on. */
	const char* RoomName;
	/** Product User ID of the participant to block */
	EOS_ProductUserId ParticipantId;
	/** Block or unblock the participant */
	EOS_Bool bBlocked;
));

/**
 * This struct is passed in with a call to EOS_RTC_OnBlockParticipantCallback.
 */
EOS_STRUCT(EOS_RTC_BlockParticipantCallbackInfo, (
	/** This returns:
	 * EOS_Success if the channel was successfully blocked.
	 * EOS_UnexpectedError otherwise.
	 */
	EOS_EResult ResultCode;
	/** Client-specified data passed into EOS_RTC_BlockParticipant. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room the users should be blocked on. */
	const char* RoomName;
	/** The Product User ID of the participant being blocked */
	EOS_ProductUserId ParticipantId;
	/** The block state that should have been set */
	EOS_Bool bBlocked;
));

/**
 * Callback for completion of block participants request.
 */
EOS_DECLARE_CALLBACK(EOS_RTC_OnBlockParticipantCallback, const EOS_RTC_BlockParticipantCallbackInfo* Data);

/** The most recent version of the EOS_RTC_AddNotifyDisconnected API. */
#define EOS_RTC_ADDNOTIFYDISCONNECTED_API_LATEST 1

/**
 * This struct is used to call EOS_RTC_AddNotifyDisconnected.
 */
EOS_STRUCT(EOS_RTC_AddNotifyDisconnectedOptions, (
	/** API Version: Set this to EOS_RTC_ADDNOTIFYDISCONNECTED_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The room this event is registered on. */
	const char* RoomName;
));

/**
 * This struct is passed in with a call to EOS_RTC_AddNotifyDisconnected registered event.
*/
EOS_STRUCT(EOS_RTC_DisconnectedCallbackInfo, (
	/** This returns:
	 * EOS_Success The room was left cleanly.
	 * EOS_NoConnection: There was a network issue connecting to the server (retryable).
	 * EOS_RTC_UserKicked: The user has been kicked by the server (retryable).
	 * EOS_ServiceFailure: A known error occurred during interaction with the server (retryable).
	 * EOS_UnexpectedError Unexpected error (retryable).
	 */
	EOS_EResult ResultCode;
	/** Client-specified data passed into EOS_RTC_AddNotifyDisconnected. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room associated with this event. */
	const char* RoomName;
));

EOS_DECLARE_CALLBACK(EOS_RTC_OnDisconnectedCallback, const EOS_RTC_DisconnectedCallbackInfo* Data);

#define EOS_RTC_PARTICIPANTMETADATA_KEY_MAXCHARCOUNT 256
#define EOS_RTC_PARTICIPANTMETADATA_VALUE_MAXCHARCOUNT 256

/** The most recent version of the EOS_RTC_ParticipantMetadata struct. */
#define EOS_RTC_PARTICIPANTMETADATA_API_LATEST 1

/**
 * This struct is used to get information about a specific participant metadata item.
 */
EOS_STRUCT(EOS_RTC_ParticipantMetadata, (
	/** API Version: Set this to EOS_RTC_PARTICIPANTMETADATA_API_LATEST. */
	int32_t ApiVersion;
	/** The unique key of this metadata item. The max size of the string is EOS_RTC_PARTICIPANTMETADATA_KEY_MAXCHARCOUNT. */
	const char* Key;
	/** The value of this metadata item. The max size of the string is EOS_RTC_PARTICIPANTMETADATA_VALUE_MAXCHARCOUNT. */
	const char* Value;
));

/** The most recent version of the EOS_RTC_AddNotifyParticipantStatusChanged API. */
#define EOS_RTC_ADDNOTIFYPARTICIPANTSTATUSCHANGED_API_LATEST 1

/**
 * This struct is used to call EOS_RTC_AddNotifyParticipantStatusChanged.
 */
EOS_STRUCT(EOS_RTC_AddNotifyParticipantStatusChangedOptions, (
	/** API Version: Set this to EOS_RTC_ADDNOTIFYPARTICIPANTSTATUSCHANGED_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The room this event is registered on. */
	const char* RoomName;
));

/**
 * This struct is passed in with a call to EOS_RTC_AddNotifyParticipantStatusChanged registered event.
 */
EOS_STRUCT(EOS_RTC_ParticipantStatusChangedCallbackInfo, (
	/** Client-specified data passed into EOS_RTC_AddNotifyParticipantStatusChanged. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room associated with this event. */
	const char* RoomName;
	/** The participant whose status changed. */
	EOS_ProductUserId ParticipantId;
	/** What status change occurred */
	EOS_ERTCParticipantStatus ParticipantStatus;
	/** The participant metadata items count.
	 * This is only set if ParticipantStatus is EOS_RTCPS_Joined
	 */
	uint32_t ParticipantMetadataCount;
	/** The participant metadata items.
	 * This is only set if ParticipantStatus is EOS_RTCPS_Joined
	 */
	const EOS_RTC_ParticipantMetadata* ParticipantMetadata;
));

EOS_DECLARE_CALLBACK(EOS_RTC_OnParticipantStatusChangedCallback, const EOS_RTC_ParticipantStatusChangedCallbackInfo* Data);



/**
* RTC SetSettings API
*/

/** The most recent version of the EOS_RTC_SetSetting API. */
#define EOS_RTC_SETSETTING_API_LATEST 1

/**
 * This struct is used to call EOS_RTC_SetSetting
 *
 * Available values of SettingName:
 * - DisableEchoCancelation: Disables the use of echo cancellation for the audio channel. Default "False".
 * - DisableNoiseSupression: Disables the use of noise suppression for the audio channel. Default "False".
 * - DisableAutoGainControl: Disables the use of auto gain control for the audio channel. Default "False".
 * - DisableDtx: Allows to disable the use of DTX.  Default "False".
 */
EOS_STRUCT(EOS_RTC_SetSettingOptions, (
	/** API Version: Set this to EOS_RTC_SETSETTING_API_LATEST. */
	int32_t ApiVersion;
	/** Setting that should be set. */
	const char* SettingName;
	/** Value to set the setting to. */
	const char* SettingValue;
));

/** The most recent version of the EOS_RTC_SetRoomSetting API. */
#define EOS_RTC_SETROOMSETTING_API_LATEST 1

/**
 * This struct is used to call EOS_RTC_SetRoomSetting
 *
 * Available values of SettingName:
 * - DisableEchoCancelation: Disables the use of echo cancellation for the audio channel. Default "False".
 * - DisableNoiseSupression: Disables the use of noise suppression for the audio channel. Default "False".
 * - DisableAutoGainControl: Disables the use of auto gain control for the audio channel. Default "False".
 * - DisableDtx: Allows to disable the use of DTX.  Default "False".
 */
EOS_STRUCT(EOS_RTC_SetRoomSettingOptions, (
	/** API Version: Set this to EOS_RTC_SETROOMSETTING_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The room the setting will be applied to. */
	const char* RoomName;
	/** Setting that should be set. */
	const char* SettingName;
	/** Value to set the setting to. */
	const char* SettingValue;
));

#pragma pack(pop)
