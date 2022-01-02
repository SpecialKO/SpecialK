// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_custominvites_types.h"

/**
 * The Custom Invites Interface is designed to allow developers to have custom game Invite and Join operations driven by the Notification Service and supported by the Overlay (if desired).
 * All Custom Invites Interface calls take a handle of type EOS_HCustomInvites as the first parameter.
 * This handle can be retrieved from a EOS_HPlatform handle by using the EOS_Platform_GetCustomInvitesInterface function.
 *
 * @see EOS_Platform_GetCustomInvitesInterface
 */

/**
 * Initializes a Custom Invite with a specified payload in preparation for it to be sent to another user or users.
 *
 * @param Options Structure containing information about the request.
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate A callback that is fired when the operation completes, either successfully or in error
 *
 * @return EOS_Success if the operation completes successfully
 *         EOS_InvalidParameters if any of the options values are incorrect
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_CustomInvites_SetCustomInvite(EOS_HCustomInvites Handle, const EOS_CustomInvites_SetCustomInviteOptions* Options);

/**
 * Sends a Custom Invite that has previously been initialized via SetCustomInvite to a group of users.
 *
 * @param Options Structure containing information about the request.
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate A callback that is fired when the operation completes, either successfully or in error
 *
 * @return EOS_Success if the query completes successfully
 *         EOS_InvalidParameters if any of the options values are incorrect
 *         EOS_TooManyRequests if the number of allowed queries is exceeded
 *         EOS_NotFound if SetCustomInvite has not been previously successfully called for this user
 */
EOS_DECLARE_FUNC(void) EOS_CustomInvites_SendCustomInvite(EOS_HCustomInvites Handle, const EOS_CustomInvites_SendCustomInviteOptions* Options, void* ClientData, const EOS_CustomInvites_OnSendCustomInviteCallback CompletionDelegate);

/**
 * Register to receive notifications when a Custom Invite for any logged in local users is received
 * @note must call EOS_CustomInvites_RemoveNotifyCustomInviteReceived to remove the notification
 *
 * @param Options Structure containing information about the request.
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate.
 * @param NotificationFn A callback that is fired when a Custom Invite is received.
 *
 * @return handle representing the registered callback
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_CustomInvites_AddNotifyCustomInviteReceived(EOS_HCustomInvites Handle, const EOS_CustomInvites_AddNotifyCustomInviteReceivedOptions* Options, void* ClientData, const EOS_CustomInvites_OnCustomInviteReceivedCallback NotificationFn);

/**
 * Unregister from receiving notifications when a Custom Invite for any logged in local users is received
 *
 * @param InId Handle representing the registered callback
 */
EOS_DECLARE_FUNC(void) EOS_CustomInvites_RemoveNotifyCustomInviteReceived(EOS_HCustomInvites Handle, EOS_NotificationId InId);

/**
 * Register to receive notifications when a Custom Invite for any logged in local users is accepted via the Social Overlay
 * @note must call EOS_CustomInvites_RemoveNotifyCustomInviteAccepted to remove the notification
 *
 * @param Options Structure containing information about the request.
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate.
 * @param NotificationFn A callback that is fired when a Custom Invite is accepted via the Social Overlay.
 *
 * @return handle representing the registered callback
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_CustomInvites_AddNotifyCustomInviteAccepted(EOS_HCustomInvites Handle, const EOS_CustomInvites_AddNotifyCustomInviteAcceptedOptions* Options, void* ClientData, const EOS_CustomInvites_OnCustomInviteAcceptedCallback NotificationFn);

/**
 * Unregister from receiving notifications when a Custom Invite for any logged in local users is accepted via the Social Overlay
 *
 * @param InId Handle representing the registered callback
 */
EOS_DECLARE_FUNC(void) EOS_CustomInvites_RemoveNotifyCustomInviteAccepted(EOS_HCustomInvites Handle, EOS_NotificationId InId);

/**
 * Signal that the title has completed processing a received Custom Invite, and that it should be cleaned up internally and in the Overlay
 *
 * @param Options Structure containing information about the request.
 *
 * @return EOS_Success if the operation completes successfully
 *         EOS_InvalidParameters if any of the option values are incorrect
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_CustomInvites_FinalizeInvite(EOS_HCustomInvites Handle, const EOS_CustomInvites_FinalizeInviteOptions* Options);
