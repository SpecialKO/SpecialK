// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_rtc_types.h"
#include "eos_rtc_audio_types.h"

/**
 * The RTC Interface is used to manage joining and leaving rooms.
 *
 * @see EOS_Platform_GetRTCInterface
 */

/**
 * Get a handle to the Audio interface
 * @return EOS_HRTCAudio handle
 *
 * @see eos_rtc_audio.h
 * @see eos_rtc_audio_types.h
 */
EOS_DECLARE_FUNC(EOS_HRTCAudio) EOS_RTC_GetAudioInterface(EOS_HRTC Handle);

/**
 * Use this function to join a room.
 *
 * This function does not need to called for the Lobby RTC Room system; doing so will return EOS_AccessDenied. The lobby system will
 * automatically join and leave RTC Rooms for all lobbies that have RTC rooms enabled.
 *
 * @param Options structure containing the parameters for the operation.
 * @param ClientData Arbitrary data that is passed back in the CompletionDelegate
 * @param CompletionDelegate a callback that is fired when the async operation completes, either successfully or in error
 */
EOS_DECLARE_FUNC(void) EOS_RTC_JoinRoom(EOS_HRTC Handle, const EOS_RTC_JoinRoomOptions* Options, void* ClientData, const EOS_RTC_OnJoinRoomCallback CompletionDelegate);

/**
 * Use this function to leave a room and clean up all the resources associated with it. This function has to always be called when the
 * room is abandoned even if the user is already disconnected for other reasons.
 *
 * This function does not need to called for the Lobby RTC Room system; doing so will return EOS_AccessDenied. The lobby system will
 * automatically join and leave RTC Rooms for all lobbies that have RTC rooms enabled.
 *
 * @param Options structure containing the parameters for the operation.
 * @param ClientData Arbitrary data that is passed back in the CompletionDelegate
 * @param CompletionDelegate a callback that is fired when the async operation completes, either successfully or in error
 * @return EOS_Success if the operation succeeded
 *         EOS_InvalidParameters if any of the parameters are incorrect
 *         EOS_NotFound if not in the specified room
 */
EOS_DECLARE_FUNC(void) EOS_RTC_LeaveRoom(EOS_HRTC Handle, const EOS_RTC_LeaveRoomOptions* Options, void* ClientData, const EOS_RTC_OnLeaveRoomCallback CompletionDelegate);

/**
 * Use this function to block a participant already connected to the room. After blocking them no media will be sent or received between
 * that user and the local user. This method can be used after receiving the OnParticipantStatusChanged notification.
 *
 * @param Options structure containing the parameters for the operation.
 * @param ClientData Arbitrary data that is passed back in the CompletionDelegate
 * @param CompletionDelegate a callback that is fired when the async operation completes, either successfully or in error
 * @return EOS_Success if the operation succeeded
 *         EOS_InvalidParameters if any of the parameters are incorrect
 *         EOS_NotFound if either the local user or specified participant are not in the specified room
 */
EOS_DECLARE_FUNC(void) EOS_RTC_BlockParticipant(EOS_HRTC Handle, EOS_RTC_BlockParticipantOptions* Options, void* ClientData, const EOS_RTC_OnBlockParticipantCallback CompletionDelegate);

/**
 * Register to receive notifications when disconnected from the room. If the returned NotificationId is valid, you must call
 * EOS_RTC_RemoveNotifyDisconnected when you no longer wish to have your CompletionDelegate called.
 *
 * This function will always return EOS_INVALID_NOTIFICATIONID when used with lobby RTC room. To be notified of the connection
 * status of a Lobby-managed RTC room, use the EOS_Lobby_AddNotifyRTCRoomConnectionChanged function instead.
 *
 * @param ClientData Arbitrary data that is passed back in the CompletionDelegate
 * @param CompletionDelegate The callback to be fired when a presence change occurs
 * @return Notification ID representing the registered callback if successful, an invalid NotificationId if not
 *
 * @see EOS_INVALID_NOTIFICATIONID
 * @see EOS_RTC_RemoveNotifyDisconnected
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_RTC_AddNotifyDisconnected(EOS_HRTC Handle, EOS_RTC_AddNotifyDisconnectedOptions* Options, void* ClientData, const EOS_RTC_OnDisconnectedCallback CompletionDelegate);

/**
 * Unregister a previously bound notification handler from receiving room disconnection notifications
 *
 * @param NotificationId The Notification ID representing the registered callback
 */
EOS_DECLARE_FUNC(void) EOS_RTC_RemoveNotifyDisconnected(EOS_HRTC Handle, EOS_NotificationId NotificationId);

/**
 * Register to receive notifications when a participant's status changes (e.g: join or leave the room). If the returned NotificationId is valid, you must call
 * EOS_RTC_RemoveNotifyParticipantStatusChanged when you no longer wish to have your CompletionDelegate called.
 *
 * If you register to this notification before joining a room, you will receive a notification for every member already in the room when you join said room.
 * This allows you to know who is already in the room when you join.
 *
 * To be used effectively with a Lobby-managed RTC room, this should be registered during the EOS_Lobby_CreateLobby or EOS_Lobby_JoinLobby completion
 * callbacks when the ResultCode is EOS_Success. If this notification is registered after that point, it is possible to miss notifications for
 * already-existing room participants.
 *
 * @param ClientData Arbitrary data that is passed back in the CompletionDelegate
 * @param CompletionDelegate The callback to be fired when a presence change occurs
 * @return Notification ID representing the registered callback if successful, an invalid NotificationId if not
 *
 * @note This notification is also raised when the local user joins the room, but NOT when the local user leaves the room.
 *
 * @see EOS_INVALID_NOTIFICATIONID
 * @see EOS_RTC_RemoveNotifyParticipantStatusChanged
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_RTC_AddNotifyParticipantStatusChanged(EOS_HRTC Handle, EOS_RTC_AddNotifyParticipantStatusChangedOptions* Options, void* ClientData, const EOS_RTC_OnParticipantStatusChangedCallback CompletionDelegate);

/**
 * Unregister a previously bound notification handler from receiving participant status change notifications
 *
 * @param NotificationId The Notification ID representing the registered callback
 */
EOS_DECLARE_FUNC(void) EOS_RTC_RemoveNotifyParticipantStatusChanged(EOS_HRTC Handle, EOS_NotificationId NotificationId);

/**
 * Use this function to control settings.
 *
 * The available settings are documented as part of EOS_RTC_SetSettingOptions.
 *
 * @param Options structure containing the parameters for the operation
 * @return EOS_Success when the setting is successfully set, EOS_NotFound when the setting is unknown, EOS_InvalidParameters when the value is invalid.
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_RTC_SetSetting(EOS_HRTC Handle, const EOS_RTC_SetSettingOptions* Options);

/**
 * Use this function to control settings for the specific room.
 *
 * The available settings are documented as part of EOS_RTC_SetRoomSettingOptions.
 *
 * @param Options structure containing the parameters for the operation
 * @return EOS_Success when the setting is successfully set, EOS_NotFound when the setting is unknown, EOS_InvalidParameters when the value is invalid.
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_RTC_SetRoomSetting(EOS_HRTC Handle, const EOS_RTC_SetRoomSettingOptions* Options);
