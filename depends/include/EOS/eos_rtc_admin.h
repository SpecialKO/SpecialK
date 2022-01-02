// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_rtc_admin_types.h"

/**
* The RTC Admin interface.
*
* This is used to manage admin-specific RTC features, such as requesting join tokens, kick users, etc.
*
* @see EOS_Platform_GetRTCAdminInterface
*/

/**
 * Query for a list of user tokens for joining a room.
 *
 * Each query generates a query id ( see EOS_RTCAdmin_QueryJoinRoomTokenCompleteCallbackInfo ) which should be used
 * to retrieve the tokens from inside the callback.
 *
 *This query id and query result itself are only valid for the duration of the callback.
 *
 * @param Options Structure containing information about the application whose user tokens we're retrieving.
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate This function is called when the query join room token operation completes.
 *
 * @return EOS_Success if the operation completes successfully
 *         EOS_InvalidParameters if any of the options are incorrect
 */
EOS_DECLARE_FUNC(void) EOS_RTCAdmin_QueryJoinRoomToken(EOS_HRTCAdmin Handle, const EOS_RTCAdmin_QueryJoinRoomTokenOptions* Options, void* ClientData, const EOS_RTCAdmin_OnQueryJoinRoomTokenCompleteCallback CompletionDelegate);

/**
 * Fetches a user token when called inside of the OnQueryJoinRoomTokenComplete callback.
 *
 * @param Options Structure containing the index being accessed
 * @param OutUserToken The user token for the given index, if it exists and is valid. Use EOS_RTCAdmin_UserToken_Release when finished
 *
 * @note The order of the tokens doesn't necessarily match the order of the EOS_ProductUserId array specified in the EOS_RTCAdmin_QueryJoinRoomTokenOptions when
 * initiating the query.
 *
 * @see EOS_RTCAdmin_UserToken_Release
 *
 * @return EOS_Success if the information is available and passed out in OutUserToken
 *         EOS_InvalidParameters if you pass a null pointer for the out parameter
 *         EOS_NotFound if the user token is not found
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_RTCAdmin_CopyUserTokenByIndex(EOS_HRTCAdmin Handle, const EOS_RTCAdmin_CopyUserTokenByIndexOptions* Options, EOS_RTCAdmin_UserToken ** OutUserToken);

/**
 * Fetches a user token for a given user ID when called inside of the OnQueryJoinRoomTokenComplete callback.
 *
 * @param Options Structure containing the user ID being accessed
 * @param OutUserToken The user token for the given user ID, if it exists and is valid. Use EOS_RTCAdmin_UserToken_Release when finished
 *
 * @see EOS_RTCAdmin_UserToken_Release
 *
 * @return EOS_Success if the information is available and passed out in OutUserToken
 *         EOS_InvalidParameters if you pass a null pointer for the out parameter
 *         EOS_NotFound if the user token is not found
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_RTCAdmin_CopyUserTokenByUserId(EOS_HRTCAdmin Handle, const EOS_RTCAdmin_CopyUserTokenByUserIdOptions* Options, EOS_RTCAdmin_UserToken ** OutUserToken);

/**
 * Starts an asynchronous task that removes a participant from a room and revokes their token.
 *
 * @param Options structure containing the room and user to revoke the token from.
 * @param ClientData arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate a callback that is fired when the async operation completes, either successfully or in error
 *
 */
EOS_DECLARE_FUNC(void) EOS_RTCAdmin_Kick(EOS_HRTCAdmin Handle, const EOS_RTCAdmin_KickOptions* Options, void* ClientData, const EOS_RTCAdmin_OnKickCompleteCallback CompletionDelegate);

/**
 * Starts an asynchronous task remotely mutes/unmutes a room participant.
 *
 * This remotely mutes the specified participant, so no audio is sent from that participant to any other participant in the room.
 *
 * @param Options structure containing the room and user to mute.
 * @param ClientData arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate a callback that is fired when the async operation completes, either successfully or in error
 */
EOS_DECLARE_FUNC(void) EOS_RTCAdmin_SetParticipantHardMute(EOS_HRTCAdmin Handle, const EOS_RTCAdmin_SetParticipantHardMuteOptions* Options, void* ClientData, const EOS_RTCAdmin_OnSetParticipantHardMuteCompleteCallback CompletionDelegate);
