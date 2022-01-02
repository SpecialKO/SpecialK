// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_kws_types.h"

/**
 * This interface is not available for general access at this time.
 *
 * The KWS Interface exists as an integration for age gating functionality provided by Kids Web Services from SuperAwesome, using the EOS Connect interface.
 * 
 * All KWS Interface calls take a handle of type EOS_HKWS as the first parameter.
 * This handle can be retrieved from a EOS_HPlatform handle by using the EOS_Platform_GetKWSInterface function.
 *
 * @see EOS_Platform_GetKWSInterface
 */

/**
 * This interface is not available for general access at this time.
 *
 * Query the client's country and age permissions for client side reasoning about the possible need enforce age based restrictions
 *
 * @param Options options required for interacting with the age gate system
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate A callback that is fired when the operation completes, either successfully or in error
 *
 * @return EOS_Success if the query completes successfully
 *         EOS_InvalidParameters if any of the options are incorrect
 *         EOS_TooManyRequests if the number of allowed queries is exceeded
 */
EOS_DECLARE_FUNC(void) EOS_KWS_QueryAgeGate(EOS_HKWS Handle, const EOS_KWS_QueryAgeGateOptions* Options, void* ClientData, const EOS_KWS_OnQueryAgeGateCallback CompletionDelegate);

/**
 * This interface is not available for general access at this time.
 *
 * Create an account with Kids Web Services and associate it with the local Product User ID
 *
 * @param Options options required for creating an account such as the local users Product User ID, their data of birth, and parental contact information
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate A callback that is fired when the operation completes, either successfully or in error
 *
 * @return EOS_Success if account creation completes successfully
 *         EOS_InvalidParameters if any of the options are incorrect
 *         EOS_TooManyRequests if the number of allowed requests is exceeded
 */
EOS_DECLARE_FUNC(void) EOS_KWS_CreateUser(EOS_HKWS Handle, const EOS_KWS_CreateUserOptions* Options, void* ClientData, const EOS_KWS_OnCreateUserCallback CompletionDelegate);

/**
 * This interface is not available for general access at this time.
 *
 * Query the current state of permissions for a given local Product User ID
 *
 * @param Options options required for querying permissions such as the local users Product User ID
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate A callback that is fired when the operation completes, either successfully or in error
 *
 * @return EOS_Success if the account query completes successfully
 *         EOS_InvalidParameters if any of the options are incorrect
 *         EOS_TooManyRequests if the number of allowed requests is exceeded
 */
EOS_DECLARE_FUNC(void) EOS_KWS_QueryPermissions(EOS_HKWS Handle, const EOS_KWS_QueryPermissionsOptions* Options, void* ClientData, const EOS_KWS_OnQueryPermissionsCallback CompletionDelegate);

/**
 * This interface is not available for general access at this time.
 *
 * Update the parent contact information for a given local Product User ID
 *
 * @param Options options required for updating the contact information such as the new email address
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate A callback that is fired when the operation completes, either successfully or in error
 *
 * @return EOS_Success if contact information update completes successfully
 *         EOS_InvalidParameters if any of the options are incorrect
 *         EOS_TooManyRequests if the number of allowed requests is exceeded
 */
EOS_DECLARE_FUNC(void) EOS_KWS_UpdateParentEmail(EOS_HKWS Handle, const EOS_KWS_UpdateParentEmailOptions* Options, void* ClientData, const EOS_KWS_OnUpdateParentEmailCallback CompletionDelegate);

/**
 * This interface is not available for general access at this time.
 *
 * Request new permissions for a given local Product User ID
 *
 * @param Options options required for updating permissions such as the new list of permissions
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate A callback that is fired when the operation completes, either successfully or in error
 *
 * @return EOS_Success if contact information update completes successfully
 *         EOS_InvalidParameters if any of the options are incorrect
 *         EOS_TooManyRequests if the number of allowed requests is exceeded
 *         EOS_KWS_ParentEmailMissing if the account requesting permissions has no parent email associated with it
 *         EOS_LimitExceeded if the number of permissions exceeds EOS_KWS_MAX_PERMISSIONS, or if any permission name exceeds EOS_KWS_MAX_PERMISSION_LENGTH
 */
EOS_DECLARE_FUNC(void) EOS_KWS_RequestPermissions(EOS_HKWS Handle, const EOS_KWS_RequestPermissionsOptions* Options, void* ClientData, const EOS_KWS_OnRequestPermissionsCallback CompletionDelegate);

/**
 * This interface is not available for general access at this time.
 *
 * Fetch the number of permissions found for a given local user
 *
 * @param Options Structure containing the input parameters
 *
 * @return the number of permissions associated with the given user
 */
EOS_DECLARE_FUNC(int32_t) EOS_KWS_GetPermissionsCount(EOS_HKWS Handle, const EOS_KWS_GetPermissionsCountOptions* Options);

/**
 * This interface is not available for general access at this time.
 *
 * Fetch a permission for a given by index for a given local user
 *
 * @param Options Structure containing the input parameters
 * @param OutPermission the permission for the given index, if it exists and is valid, use EOS_KWS_PermissionStatus_Release when finished
 *
 * @see EOS_KWS_CreateUser
 * @see EOS_KWS_QueryPermissions
 * @see EOS_KWS_RequestPermissions
 * @see EOS_KWS_PermissionStatus_Release
 *
 * @return EOS_Success if the permission state is known for the given user and index
 *         EOS_NotFound if the user is not found or the index is invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_KWS_CopyPermissionByIndex(EOS_HKWS Handle, const EOS_KWS_CopyPermissionByIndexOptions* Options, EOS_KWS_PermissionStatus ** OutPermission);

/**
 * This interface is not available for general access at this time.
 *
 * Fetch the state of a given permission that are cached for a given local user.
 *
 * @param Options Structure containing the input parameters
 * @param OutPermission the permission for the given key, if it exists and is valid
 *
 * @see EOS_KWS_CreateUser
 * @see EOS_KWS_QueryPermissions
 * @see EOS_KWS_RequestPermissions
 *
 * @return EOS_Success if the permission state is known for the given user and key
 *         EOS_NotFound if the user or the permission is not found
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_KWS_GetPermissionByKey(EOS_HKWS Handle, const EOS_KWS_GetPermissionByKeyOptions* Options, EOS_EKWSPermissionStatus* OutPermission);

/**
 * This interface is not available for general access at this time.
 *
 * Register to receive notifications about KWS permissions changes for any logged in local users
 * @note must call EOS_KWS_RemoveNotifyPermissionsUpdateReceived to remove the notification
 *
 * @param Options Structure containing information about the request.
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate.
 * @param NotificationFn A callback that is fired when a notification is received.
 *
 * @return handle representing the registered callback
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_KWS_AddNotifyPermissionsUpdateReceived(EOS_HKWS Handle, const EOS_KWS_AddNotifyPermissionsUpdateReceivedOptions* Options, void* ClientData, const EOS_KWS_OnPermissionsUpdateReceivedCallback NotificationFn);

/**
 * This interface is not available for general access at this time.
 *
 * Unregister from receiving notifications about KWS permissions related to logged in users
 *
 * @param InId Handle representing the registered callback
 */
EOS_DECLARE_FUNC(void) EOS_KWS_RemoveNotifyPermissionsUpdateReceived(EOS_HKWS Handle, EOS_NotificationId InId);
