// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_anticheatserver_types.h"

/**
 * Add a callback issued when a new message must be dispatched to a connected client. The bound function
 * will only be called between a successful call to EOS_AntiCheatServer_BeginSession and the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data
 * @param ClientData This value is returned to the caller when NotificationFn is invoked
 * @param NotificationFn The callback to be fired
 * @return A valid notification ID if successfully bound, or EOS_INVALID_NOTIFICATIONID otherwise
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_AntiCheatServer_AddNotifyMessageToClient(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_AddNotifyMessageToClientOptions* Options, void* ClientData, EOS_AntiCheatServer_OnMessageToClientCallback NotificationFn);

/**
 * Remove a previously bound EOS_AntiCheatServer_AddNotifyMessageToClient handler.
 *
 * @param NotificationId The previously bound notification ID
 */
EOS_DECLARE_FUNC(void) EOS_AntiCheatServer_RemoveNotifyMessageToClient(EOS_HAntiCheatServer Handle, EOS_NotificationId NotificationId);

/**
 * Add a callback issued when an action must be applied to a connected client. The bound function
 * will only be called between a successful call to EOS_AntiCheatServer_BeginSession and the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data
 * @param ClientData This value is returned to the caller when NotificationFn is invoked
 * @param NotificationFn The callback to be fired
 * @return A valid notification ID if successfully bound, or EOS_INVALID_NOTIFICATIONID otherwise
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_AntiCheatServer_AddNotifyClientActionRequired(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_AddNotifyClientActionRequiredOptions* Options, void* ClientData, EOS_AntiCheatServer_OnClientActionRequiredCallback NotificationFn);

/**
 * Remove a previously bound EOS_AntiCheatServer_AddNotifyClientActionRequired handler.
 *
 * @param NotificationId The previously bound notification ID
 */
EOS_DECLARE_FUNC(void) EOS_AntiCheatServer_RemoveNotifyClientActionRequired(EOS_HAntiCheatServer Handle, EOS_NotificationId NotificationId);

/**
 * Add an optional callback issued when a connected client's authentication status changes. The bound function
 * will only be called between a successful call to EOS_AntiCheatServer_BeginSession and the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data
 * @param ClientData This value is returned to the caller when NotificationFn is invoked
 * @param NotificationFn The callback to be fired
 * @return A valid notification ID if successfully bound, or EOS_INVALID_NOTIFICATIONID otherwise
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_AntiCheatServer_AddNotifyClientAuthStatusChanged(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_AddNotifyClientAuthStatusChangedOptions* Options, void* ClientData, EOS_AntiCheatServer_OnClientAuthStatusChangedCallback NotificationFn);

/**
 * Remove a previously bound EOS_AntiCheatServer_AddNotifyClientAuthStatusChanged handler.
 *
 * @param NotificationId The previously bound notification ID
 */
EOS_DECLARE_FUNC(void) EOS_AntiCheatServer_RemoveNotifyClientAuthStatusChanged(EOS_HAntiCheatServer Handle, EOS_NotificationId NotificationId);

/**
 * Begin the gameplay session. Event callbacks must be configured with EOS_AntiCheatServer_AddNotifyMessageToClient
 * and EOS_AntiCheatServer_AddNotifyClientActionRequired before calling this function.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the initialization succeeded
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_BeginSession(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_BeginSessionOptions* Options);

/**
 * End the gameplay session. Should be called when the server is shutting down or entering an idle state.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the initialization succeeded
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_EndSession(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_EndSessionOptions* Options);

/**
 * Register a connected client. Must be paired with a call to UnregisterClient.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the player was registered successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_RegisterClient(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_RegisterClientOptions* Options);

/**
 * Unregister a disconnected client.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the player was unregistered successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_UnregisterClient(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_UnregisterClientOptions* Options);

/**
 * Call when an anti-cheat message is received from a client.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the message was processed successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_ReceiveMessageFromClient(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_ReceiveMessageFromClientOptions* Options);

/**
 * Optional. Sets or updates client details including input device and admin status.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the flags were updated successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_SetClientDetails(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_SetClientDetailsOptions* Options);

/**
 * Optional. Sets or updates a game session identifier which can be attached to other data for reference.
 * The identifier can be updated at any time for currently and subsequently registered clients.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the game session identifier was set successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_SetGameSessionId(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_SetGameSessionIdOptions* Options);

/**
 * Optional. Can be used to indicate that a client is legitimately known to be
 * temporarily unable to communicate, for example as a result of loading a new level.
 *
 * The bIsNetworkActive flag must be set back to true when users enter normal
 * gameplay, otherwise anti-cheat enforcement will not work correctly.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the network state was updated successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_SetClientNetworkState(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_SetClientNetworkStateOptions* Options);

/**
 * Optional NetProtect feature for game message encryption.
 * Calculates the required decrypted buffer size for a given input data length.
 * This will not change for a given SDK version, and allows one time allocation of reusable buffers.
 *
 * @param Options Structure containing input data.
 * @param OutBufferLengthBytes On success, the OutBuffer length in bytes that is required to call ProtectMessage on the given input size.
 *
 * @return EOS_Success - If the output length was calculated successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_GetProtectMessageOutputLength(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_GetProtectMessageOutputLengthOptions* Options, uint32_t* OutBufferSizeBytes);

/**
 * Optional NetProtect feature for game message encryption.
 * Encrypts an arbitrary message that will be sent to a game client and decrypted on the other side.
 *
 * Options.Data and OutBuffer may refer to the same buffer to encrypt in place.
 *
 * @param Options Structure containing input data.
 * @param OutBuffer On success, buffer where encrypted message data will be written.
 * @param OutBytesWritten On success, the number of bytes that were written to OutBuffer.
 *
 * @return EOS_Success - If the message was protected successfully
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_InvalidUser - If the specified ClientHandle was invalid or not currently registered. See RegisterClient.
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_ProtectMessage(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_ProtectMessageOptions* Options, void* OutBuffer, uint32_t* OutBytesWritten);

/**
 * Optional NetProtect feature for game message encryption.
 * Decrypts an encrypted message received from a game client.
 *
 * Options.Data and OutBuffer may refer to the same buffer to decrypt in place.
 *
 * @param Options Structure containing input data.
 * @param OutBuffer On success, buffer where encrypted message data will be written.
 * @param OutBytesWritten On success, the number of bytes that were written to OutBuffer.
 *
 * @return EOS_Success - If the message was unprotected successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_UnprotectMessage(EOS_HAntiCheatServer Handle, const EOS_AntiCheatServer_UnprotectMessageOptions* Options, void* OutBuffer, uint32_t* OutBytesWritten);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Registers a custom gameplay event.
 *
 * All custom game events must be registered before EOS_AntiCheatServer_BeginSession is called for the first time.
 * After the first call to EOS_AntiCheatServer_BeginSession, this function cannot be called any longer.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was registered successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_RegisterEvent(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_RegisterEventOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs a custom gameplay event.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogEvent(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogEventOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs a new game round start.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogGameRoundStart(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogGameRoundStartOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs a game round's end and outcome.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogGameRoundEnd(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogGameRoundEndOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs a player spawning into the game.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogPlayerSpawn(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogPlayerSpawnOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs a player despawning in the game, for example as a result of the character's death,
 * switching to spectator mode, etc.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogPlayerDespawn(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogPlayerDespawnOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs a player being revived after being downed (see EOS_AntiCheatServer_LogPlayerTakeDamage options).
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogPlayerRevive(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogPlayerReviveOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs a player's general state including position and view direction.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogPlayerTick(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogPlayerTickOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs that a player has used a weapon, for example firing one bullet or making one melee attack.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogPlayerUseWeapon(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogPlayerUseWeaponOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs that a player has used a special ability or item which affects their character's capabilities,
 * for example temporarily increasing their speed or allowing them to see nearby players behind walls.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogPlayerUseAbility(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogPlayerUseAbilityOptions* Options);

/**
 * Optional Cerberus feature for gameplay data collection.
 * Logs that a player has taken damage.
 *
 * This function may only be called between a successful call to EOS_AntiCheatServer_BeginSession and
 * the matching EOS_AntiCheatServer_EndSession call.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the event was logged successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatServer_LogPlayerTakeDamage(EOS_HAntiCheatServer Handle, const EOS_AntiCheatCommon_LogPlayerTakeDamageOptions* Options);
