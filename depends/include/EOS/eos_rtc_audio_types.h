// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_common.h"

#pragma pack(push, 8)

EXTERN_C typedef struct EOS_RTCAudioHandle* EOS_HRTCAudio;

/**
 * An enumeration of the different audio channel statuses.
 */
EOS_ENUM(EOS_ERTCAudioStatus,
	/** Audio unsupported by the source (no devices) */
	EOS_RTCAS_Unsupported = 0,
	/** Audio enabled */
	EOS_RTCAS_Enabled = 1,
	/** Audio disabled */
	EOS_RTCAS_Disabled = 2,
	/** Audio disabled by the administrator */
	EOS_RTCAS_AdminDisabled = 3,
	/** Audio channel is disabled temporarily for both sending and receiving */
	EOS_RTCAS_NotListeningDisabled = 4
);

/** The most recent version of the EOS_RTCAudio_RegisterPlatformAudioUser API. */
#define EOS_RTCAUDIO_REGISTERPLATFORMAUDIOUSER_API_LATEST 1

/**
 * This struct is used to inform the audio system of a user.
 */
EOS_STRUCT(EOS_RTCAudio_RegisterPlatformAudioUserOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_REGISTERPLATFORMAUDIOUSER_API_LATEST. */
	int32_t ApiVersion;
	/** Platform dependent user id. */
	const char* UserId;
));

/** The most recent version of the EOS_RTCAudio_UnregisterPlatformAudioUser API. */
#define EOS_RTCAUDIO_UNREGISTERPLATFORMAUDIOUSER_API_LATEST 1

/**
 * This struct is used to remove a user from the audio system.
 */
EOS_STRUCT(EOS_RTCAudio_UnregisterPlatformAudioUserOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_UNREGISTERPLATFORMAUDIOUSER_API_LATEST. */
	int32_t ApiVersion;
	/** The account of a user associated with this event. */
	const char* UserId;
));

/** The most recent version of the EOS_RTCAudio_AddNotifyParticipantUpdated API. */
#define EOS_RTCAUDIO_ADDNOTIFYPARTICIPANTUPDATED_API_LATEST 1

/**
 * This struct is used to call EOS_RTCAudio_AddNotifyParticipantUpdated.
 */
EOS_STRUCT(EOS_RTCAudio_AddNotifyParticipantUpdatedOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_ADDNOTIFYPARTICIPANTUPDATED_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The  room this event is registered on. */
	const char* RoomName;
));

/**
 * This struct is passed in with a call to EOS_RTCAudio_AddNotifyParticipantUpdated registered event.
 */
EOS_STRUCT(EOS_RTCAudio_ParticipantUpdatedCallbackInfo, (
	/** Client-specified data passed into EOS_RTCAudio_AddNotifyParticipantUpdated. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room associated with this event. */
	const char* RoomName;
	/** The participant updated. */
	EOS_ProductUserId ParticipantId;
	/** The participant speaking / non-speaking status. */
	EOS_Bool bSpeaking;
	/** The participant audio status (enabled, disabled). */
	EOS_ERTCAudioStatus AudioStatus;
));

EOS_DECLARE_CALLBACK(EOS_RTCAudio_OnParticipantUpdatedCallback, const EOS_RTCAudio_ParticipantUpdatedCallbackInfo* Data);

/** The most recent version of the EOS_RTCAudio_AddNotifyAudioDevicesChanged API. */
#define EOS_RTCAUDIO_ADDNOTIFYAUDIODEVICESCHANGED_API_LATEST 1

/**
 * This struct is used to call EOS_RTCAudio_AddNotifyAudioDevicesChanged.
 */
EOS_STRUCT(EOS_RTCAudio_AddNotifyAudioDevicesChangedOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_ADDNOTIFYAUDIODEVICESCHANGED_API_LATEST. */
	int32_t ApiVersion;
));

/**
 * This struct is passed in with a call to EOS_RTCAudio_AddNotifyAudioDevicesChanged registered event.
*/
EOS_STRUCT(EOS_RTCAudio_AudioDevicesChangedCallbackInfo, (
	/** Client-specified data passed into EOS_RTCAudio_AddNotifyAudioDevicesChanged. */
	void* ClientData;
));

EOS_DECLARE_CALLBACK(EOS_RTCAudio_OnAudioDevicesChangedCallback, const EOS_RTCAudio_AudioDevicesChangedCallbackInfo* Data);

/** The most recent version of the EOS_RTCAudio_GetAudioInputDevicesCount API. */
#define EOS_RTCAUDIO_GETAUDIOINPUTDEVICESCOUNT_API_LATEST 1

/**
 * Input parameters for the EOS_RTCAudio_GetAudioInputDevicesCount function.
 */
EOS_STRUCT(EOS_RTCAudio_GetAudioInputDevicesCountOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_GETAUDIOINPUTDEVICESCOUNT_API_LATEST. */
	int32_t ApiVersion;
));

/** The most recent version of the EOS_RTCAudio_GetAudioInputDeviceByIndex API. */
#define EOS_RTCAUDIO_GETAUDIOINPUTDEVICEBYINDEX_API_LATEST 1

/**
 * Input parameters for the EOS_RTCAudio_GetAudioInputDeviceByIndex function.
 */
EOS_STRUCT(EOS_RTCAudio_GetAudioInputDeviceByIndexOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_GETAUDIOINPUTDEVICEBYINDEX_API_LATEST. */
	int32_t ApiVersion;
	/** Index of the device info to retrieve. */
	uint32_t DeviceInfoIndex;
));

/** The most recent version of the EOS_RTCAudio_AudioInputDeviceInfo struct. */
#define EOS_RTCAUDIO_AUDIOINPUTDEVICEINFO_API_LATEST 1

/**
 * This struct is used to get information about a specific input device.
 */
EOS_STRUCT(EOS_RTCAudio_AudioInputDeviceInfo, (
	/** API Version: Set this to EOS_RTCAUDIO_AUDIOINPUTDEVICEINFO_API_LATEST. */
	int32_t ApiVersion;
	/** True if this is the default audio input device in the system. */
	EOS_Bool bDefaultDevice;
	/** 
	 *  The persistent unique id of the device.
	 */
	const char* DeviceId;
	/**
	 * The name of the device
	 */
	const char* DeviceName;
));

/** The most recent version of the EOS_RTCAudio_GetAudioOutputDevicesCount API. */
#define EOS_RTCAUDIO_GETAUDIOOUTPUTDEVICESCOUNT_API_LATEST 1

/**
 * Input parameters for the EOS_RTCAudio_GetAudioOutputDevicesCount function.
 */
EOS_STRUCT(EOS_RTCAudio_GetAudioOutputDevicesCountOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_GETAUDIOOUTPUTDEVICESCOUNT_API_LATEST. */
	int32_t ApiVersion;
));

/** The most recent version of the EOS_RTCAudio_GetAudioOutputDeviceByIndex API. */
#define EOS_RTCAUDIO_GETAUDIOOUTPUTDEVICEBYINDEX_API_LATEST 1

/**
 * Input parameters for the EOS_RTCAudio_GetAudioOutputDeviceByIndex function.
 */
EOS_STRUCT(EOS_RTCAudio_GetAudioOutputDeviceByIndexOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_GETAUDIOOUTPUTDEVICEBYINDEX_API_LATEST. */
	int32_t ApiVersion;
	/** Index of the device info to retrieve. */
	uint32_t DeviceInfoIndex;
));

/** The most recent version of the EOS_RTCAudio_AudioOutputDeviceInfo struct. */
#define EOS_RTCAUDIO_AUDIOOUTPUTDEVICEINFO_API_LATEST 1

/**
 * This struct is used to get information about a specific output device.
 */
EOS_STRUCT(EOS_RTCAudio_AudioOutputDeviceInfo, (
	/** API Version: Set this to EOS_RTCAUDIO_AUDIOOUTPUTDEVICEINFO_API_LATEST. */
	int32_t ApiVersion;
	/** True if this is the default audio output device in the system. */
	EOS_Bool bDefaultDevice;
	/** 
	 *  The persistent unique id of the device.
	 */
	const char* DeviceId;
	/**
	 * The name of the device
	 */
	const char* DeviceName;
));

/** The most recent version of the EOS_RTCAudio_SetAudioInputSettings API. */
#define EOS_RTCAUDIO_SETAUDIOINPUTSETTINGS_API_LATEST 1

/**
 * This struct is used to call EOS_RTCAudio_SetAudioInputSettings.
 */
EOS_STRUCT(EOS_RTCAudio_SetAudioInputSettingsOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_SETAUDIOINPUTSETTINGS_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The device Id to be used for this user. Pass NULL or empty string to use default input device. */
	const char* DeviceId;
	/** The volume to be configured for this device (range 0.0 to 100.0).
	 * At the moment, the only value that produce any effect is 0.0 (silence). Any other value is ignored and causes no change to the volume.
	 */
	float Volume;
	/** Enable or disable Platform AEC (Acoustic Echo Cancellation) if available. */
	EOS_Bool bPlatformAEC;
));

/** The most recent version of the EOS_RTCAudio_SetAudioOutputSettings API. */
#define EOS_RTCAUDIO_SETAUDIOOUTPUTSETTINGS_API_LATEST 1

/**
 * This struct is used to call EOS_RTCAudio_SetAudioOutputSettings.
 */
EOS_STRUCT(EOS_RTCAudio_SetAudioOutputSettingsOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_SETAUDIOOUTPUTSETTINGS_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The device Id to be used for this user. Pass NULL or empty string to use default output device. */
	const char* DeviceId;
	/** The volume to be configured for this device (range 0.0 to 100.0). Volume 50 means that the audio volume is not modified
	 * and stays in its source value.
	 */
	float Volume;
));

/**
 * An enumeration of the different audio input device statuses.
 */
EOS_ENUM(EOS_ERTCAudioInputStatus,
	/** The device is not in used right now (e.g: you are alone in the room). In such cases, the hardware resources are not allocated. */
	EOS_RTCAIS_Idle = 0,
	/** The device is being used and capturing audio */
	EOS_RTCAIS_Recording = 1,
	/**
	 * The SDK is in a recording state, but actually capturing silence because the device is exclusively being used by the platform at the moment.
	 * This only applies to certain platforms.
	 */
	EOS_RTCAIS_RecordingSilent = 2,
	/**
	 * The SDK is in a recording state, but actually capturing silence because the device is disconnected (e.g: the microphone is not plugged in).
	 * This only applies to certain platforms.
	 */
	EOS_RTCAIS_RecordingDisconnected = 3,
	/** Something failed while trying to use the device */
	EOS_RTCAIS_Failed = 4
);

/** The most recent version of the EOS_RTCAudio_AddNotifyAudioInputState API. */
#define EOS_RTCAUDIO_ADDNOTIFYAUDIOINPUTSTATE_API_LATEST 1

/**
 * This struct is used to call EOS_RTCAudio_AddNotifyAudioInputState.
 */
EOS_STRUCT(EOS_RTCAudio_AddNotifyAudioInputStateOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_ADDNOTIFYAUDIOINPUTSTATE_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The room this event is registered on. */
	const char* RoomName;
));

/**
 * This struct is passed in with a call to EOS_RTCAudio_AddNotifyAudioInputState registered event.
 */
EOS_STRUCT(EOS_RTCAudio_AudioInputStateCallbackInfo, (
	/** Client-specified data passed into EOS_RTCAudio_AddNotifyAudioInputState. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room associated with this event. */
	const char* RoomName;
	/** The status of the audio input. */
	EOS_ERTCAudioInputStatus Status;
));

EOS_DECLARE_CALLBACK(EOS_RTCAudio_OnAudioInputStateCallback, const EOS_RTCAudio_AudioInputStateCallbackInfo* Data);

/**
 * An enumeration of the different audio output device statuses.
 */
EOS_ENUM(EOS_ERTCAudioOutputStatus,
	/** The device is not in used right now (e.g: you are alone in the room). In such cases, the hardware resources are not allocated. */
	EOS_RTCAOS_Idle = 0,
	/** Device is in use */
	EOS_RTCAOS_Playing = 1,
	/** Something failed while trying to use the device */
	EOS_RTCAOS_Failed = 2
);

/** The most recent version of the EOS_RTCAudio_AddNotifyAudioOutputState API. */
#define EOS_RTCAUDIO_ADDNOTIFYAUDIOOUTPUTSTATE_API_LATEST 1

/**
 * This struct is used to call EOS_RTCAudio_AddNotifyAudioOutputState.
 */
EOS_STRUCT(EOS_RTCAudio_AddNotifyAudioOutputStateOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_ADDNOTIFYAUDIOOUTPUTSTATE_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The  room this event is registered on. */
	const char* RoomName;
));

/**
 * This struct is passed in with a call to EOS_RTCAudio_AddNotifyAudioOutputState registered event.
 */
EOS_STRUCT(EOS_RTCAudio_AudioOutputStateCallbackInfo, (
	/** Client-specified data passed into EOS_RTCAudio_AddNotifyAudioOutputState. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room associated with this event. */
	const char* RoomName;
	/** The status of the audio output. */
	EOS_ERTCAudioOutputStatus Status;
));

EOS_DECLARE_CALLBACK(EOS_RTCAudio_OnAudioOutputStateCallback, const EOS_RTCAudio_AudioOutputStateCallbackInfo* Data);

/** The most recent version of the EOS_RTCAudio_AddNotifyAudioBeforeSend API. */
#define EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORESEND_API_LATEST 1

/**
 * This struct is used to call EOS_RTCAudio_AddNotifyAudioBeforeSend.
 */
EOS_STRUCT(EOS_RTCAudio_AddNotifyAudioBeforeSendOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORESEND_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The  room this event is registered on. */
	const char* RoomName;
));


/** The most recent version of the EOS_RTCAudio_AudioBuffer API */
#define EOS_RTCAUDIO_AUDIOBUFFER_API_LATEST 1

/**
 * This struct is used to represent an audio buffer received in callbacks from EOS_RTCAudio_AddNotifyAudioBeforeSend and EOS_RTCAudio_AddNotifyAudioBeforeRender.
 */
EOS_STRUCT(EOS_RTCAudio_AudioBuffer, (
	/** API Version: Set this to EOS_RTCAUDIO_AUDIOBUFFER_API_LATEST. */
	int32_t ApiVersion;
	/** Pointer to the data with the interleaved audio frames in signed 16 bits format. */
	int16_t* Frames;
	/**
	 * Number of frames available in the Frames buffer.
	 * @note This is the number of frames in a channel, not the total number of frames in the buffer.
	 */
	uint32_t FramesCount;
	/** Sample rate for the samples in the Frames buffer. */
	uint32_t SampleRate;
	/** Number of channels for the samples in the Frames buffer. */
	uint32_t Channels;
));

/**
 * This struct is passed in with a call to EOS_RTCAudio_AddNotifyAudioBeforeSend registered event.
 */
EOS_STRUCT(EOS_RTCAudio_AudioBeforeSendCallbackInfo, (
	/** Client-specified data passed into EOS_RTCAudio_AddNotifyAudioBeforeSend. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room associated with this event. */
	const char* RoomName;
	/** Audio buffer. */
	EOS_RTCAudio_AudioBuffer* Buffer;
));

EOS_DECLARE_CALLBACK(EOS_RTCAudio_OnAudioBeforeSendCallback, const EOS_RTCAudio_AudioBeforeSendCallbackInfo* Data);

/** The most recent version of the EOS_RTCAudio_AddNotifyAudioBeforeRender API. */
#define EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORERENDER_API_LATEST 1

/**
* This struct is used to call EOS_RTCAudio_AddNotifyAudioBeforeRender.
*/
EOS_STRUCT(EOS_RTCAudio_AddNotifyAudioBeforeRenderOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_ADDNOTIFYAUDIOBEFORERENDER_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The  room this event is registered on. */
	const char* RoomName;
	/**
	 * Mixed audio or unmixed audio.
	 */
	EOS_Bool bUnmixedAudio;
));


/**
 * This struct is passed in with a call to EOS_RTCAudio_AddNotifyAudioBeforeRender registered event.
 */
EOS_STRUCT(EOS_RTCAudio_AudioBeforeRenderCallbackInfo, (
	/** Client-specified data passed into EOS_RTCAudio_AddNotifyAudioBeforeRender. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room associated with this event. */
	const char* RoomName;
	/**
	 * Audio buffer.
	 */
	EOS_RTCAudio_AudioBuffer* Buffer;
	/**
	 * The Product User ID of the participant if bUnmixedAudio was set to true when setting the notifications, or empty if
	 * bUnmixedAudio was set to false and thus the buffer is the mixed audio of all participants
	 */
	EOS_ProductUserId ParticipantId;
));

EOS_DECLARE_CALLBACK(EOS_RTCAudio_OnAudioBeforeRenderCallback, const EOS_RTCAudio_AudioBeforeRenderCallbackInfo* Data);


/** The most recent version of the EOS_RTCAudio_SendAudio API. */
#define EOS_RTCAUDIO_SENDAUDIO_API_LATEST 1

/**
 * This struct is used to call EOS_RTCAudio_SendAudio.
 */
EOS_STRUCT(EOS_RTCAudio_SendAudioOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_SENDAUDIO_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The  room this event is registered on. */
	const char* RoomName;
	/**
	 * Audio buffer, which must have a duration of 10 ms.
	 * @note The SDK makes a copy of buffer. There is no need to keep the buffer around after calling EOS_RTCAudio_SendAudio
	 */
	EOS_RTCAudio_AudioBuffer* Buffer;
));

/** The most recent version of the EOS_RTCAudio_UpdateSending API. */
#define EOS_RTCAUDIO_UPDATESENDING_API_LATEST 1

/**
 * This struct is passed in with a call to EOS_RTCAudio_UpdateSending
 */
EOS_STRUCT(EOS_RTCAudio_UpdateSendingOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_UPDATESENDING_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The room this settings should be applied on. */
	const char* RoomName;
	/** Muted or unmuted audio track status */
	EOS_ERTCAudioStatus AudioStatus;
));

/**
 * This struct is passed in with a call to EOS_RTCAudio_OnUpdateSendingCallback.
 */
EOS_STRUCT(EOS_RTCAudio_UpdateSendingCallbackInfo, (
	/** This returns:
	 * EOS_Success if the channel was successfully blocked.
	 * EOS_UnexpectedError otherwise.
	 */
	EOS_EResult ResultCode;
	/** Client-specified data passed into EOS_RTCAudio_UpdateSending. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room this settings should be applied on. */
	const char* RoomName;
	/** Muted or unmuted audio track status */
	EOS_ERTCAudioStatus AudioStatus;
));

/**
 * Callback for completion of update sending request.
 */
EOS_DECLARE_CALLBACK(EOS_RTCAudio_OnUpdateSendingCallback, const EOS_RTCAudio_UpdateSendingCallbackInfo* Data);

/** The most recent version of the EOS_RTCAudio_UpdateReceiving API. */
#define EOS_RTCAUDIO_UPDATERECEIVING_API_LATEST 1

/**
 * This struct is passed in with a call to EOS_RTCAudio_UpdateReceiving.
 */
EOS_STRUCT(EOS_RTCAudio_UpdateReceivingOptions, (
	/** API Version: Set this to EOS_RTCAUDIO_UPDATERECEIVING_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the user trying to request this operation. */
	EOS_ProductUserId LocalUserId;
	/** The room this settings should be applied on. */
	const char* RoomName;
	/** The participant to modify or null to update the global configuration */
	EOS_ProductUserId ParticipantId;
	/** Mute or unmute audio track */
	EOS_Bool bAudioEnabled;
));

/**
 * This struct is passed in with a call to EOS_RTCAudio_OnUpdateReceivingCallback.
 */
EOS_STRUCT(EOS_RTCAudio_UpdateReceivingCallbackInfo, (
	/** This returns:
	 * EOS_Success if the users were successfully unblocked.
	 * EOS_UnexpectedError otherwise.
	 */
	EOS_EResult ResultCode;
	/** Client-specified data passed into EOS_RTCAudio_UpdateReceiving. */
	void* ClientData;
	/** The Product User ID of the user who initiated this request. */
	EOS_ProductUserId LocalUserId;
	/** The room this settings should be applied on. */
	const char* RoomName;
	/** The participant to modify or null to update the global configuration */
	EOS_ProductUserId ParticipantId;
	/** Muted or unmuted audio track */
	EOS_Bool bAudioEnabled;
));

/**
 * Callback for completion of update receiving request
 */
EOS_DECLARE_CALLBACK(EOS_RTCAudio_OnUpdateReceivingCallback, const EOS_RTCAudio_UpdateReceivingCallbackInfo* Data);

#pragma pack(pop)
