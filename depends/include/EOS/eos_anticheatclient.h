// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_anticheatclient_types.h"

/**
 * Add a callback issued when a new message must be dispatched to the game server. The bound function will only be called
 * between a successful call to EOS_AntiCheatClient_BeginSession and the matching EOS_AntiCheatClient_EndSession call in mode EOS_ACCM_ClientServer.
 * Mode: EOS_ACCM_ClientServer.
 *
 * @param Options Structure containing input data
 * @param ClientData This value is returned to the caller when NotificationFn is invoked
 * @param NotificationFn The callback to be fired
 * @return A valid notification ID if successfully bound, or EOS_INVALID_NOTIFICATIONID otherwise
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_AntiCheatClient_AddNotifyMessageToServer(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_AddNotifyMessageToServerOptions* Options, void* ClientData, EOS_AntiCheatClient_OnMessageToServerCallback NotificationFn);

/**
 * Remove a previously bound EOS_AntiCheatClient_AddNotifyMessageToServer handler.
 * Mode: Any.
 *
 * @param NotificationId The previously bound notification ID
 */
EOS_DECLARE_FUNC(void) EOS_AntiCheatClient_RemoveNotifyMessageToServer(EOS_HAntiCheatClient Handle, EOS_NotificationId NotificationId);

/**
 * Add a callback issued when a new message must be dispatched to a connected peer. The bound function will only be called
 * between a successful call to EOS_AntiCheatClient_BeginSession and the matching EOS_AntiCheatClient_EndSession call in mode EOS_ACCM_PeerToPeer.
 * Mode: EOS_ACCM_PeerToPeer.
 *
 * @param Options Structure containing input data
 * @param ClientData This value is returned to the caller when NotificationFn is invoked
 * @param NotificationFn The callback to be fired
 * @return A valid notification ID if successfully bound, or EOS_INVALID_NOTIFICATIONID otherwise
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_AntiCheatClient_AddNotifyMessageToPeer(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_AddNotifyMessageToPeerOptions* Options, void* ClientData, EOS_AntiCheatClient_OnMessageToPeerCallback NotificationFn);

/**
 * Remove a previously bound EOS_AntiCheatClient_AddNotifyMessageToPeer handler.
 * Mode: Any.
 *
 * @param NotificationId The previously bound notification ID
 */
EOS_DECLARE_FUNC(void) EOS_AntiCheatClient_RemoveNotifyMessageToPeer(EOS_HAntiCheatClient Handle, EOS_NotificationId NotificationId);

/**
 * Add a callback issued when an action must be applied to a connected client. The bound function will only be called
 * between a successful call to EOS_AntiCheatClient_BeginSession and the matching EOS_AntiCheatClient_EndSession call in mode EOS_ACCM_PeerToPeer.
 * Mode: EOS_ACCM_PeerToPeer.
 *
 * @param Options Structure containing input data
 * @param ClientData This value is returned to the caller when NotificationFn is invoked
 * @param NotificationFn The callback to be fired
 * @return A valid notification ID if successfully bound, or EOS_INVALID_NOTIFICATIONID otherwise
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_AntiCheatClient_AddNotifyPeerActionRequired(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_AddNotifyPeerActionRequiredOptions* Options, void* ClientData, EOS_AntiCheatClient_OnPeerActionRequiredCallback NotificationFn);

/**
 * Remove a previously bound EOS_AntiCheatClient_AddNotifyPeerActionRequired handler.
 * Mode: Any.
 *
 * @param NotificationId The previously bound notification ID
 */
EOS_DECLARE_FUNC(void) EOS_AntiCheatClient_RemoveNotifyPeerActionRequired(EOS_HAntiCheatClient Handle, EOS_NotificationId NotificationId);

/**
 * Add an optional callback issued when a connected peer's authentication status changes. The bound function will only be called
 * between a successful call to EOS_AntiCheatClient_BeginSession and the matching EOS_AntiCheatClient_EndSession call in mode EOS_ACCM_PeerToPeer.
 * Mode: EOS_ACCM_PeerToPeer.
 *
 * @param Options Structure containing input data
 * @param ClientData This value is returned to the caller when NotificationFn is invoked
 * @param NotificationFn The callback to be fired
 * @return A valid notification ID if successfully bound, or EOS_INVALID_NOTIFICATIONID otherwise
 */
EOS_DECLARE_FUNC(EOS_NotificationId) EOS_AntiCheatClient_AddNotifyPeerAuthStatusChanged(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_AddNotifyPeerAuthStatusChangedOptions* Options, void* ClientData, EOS_AntiCheatClient_OnPeerAuthStatusChangedCallback NotificationFn);

/**
 * Remove a previously bound EOS_AntiCheatClient_AddNotifyPeerAuthStatusChanged handler.
 * Mode: Any.
 *
 * @param NotificationId The previously bound notification ID
 */
EOS_DECLARE_FUNC(void) EOS_AntiCheatClient_RemoveNotifyPeerAuthStatusChanged(EOS_HAntiCheatClient Handle, EOS_NotificationId NotificationId);

/**
 * Begins a multiplayer game session. After this call returns successfully, the client is ready to exchange
 * anti-cheat messages with a game server or peer(s). When leaving one game session and connecting to a
 * different one, a new anti-cheat session must be created by calling EOS_AntiCheatClient_EndSession and EOS_AntiCheatClient_BeginSession again.
 * Mode: All
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the session was started successfully
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_AntiCheat_InvalidMode - If the current mode does not support this function
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_BeginSession(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_BeginSessionOptions* Options);

/**
 * Ends a multiplayer game session, either by leaving an ongoing session or shutting it down entirely.
 * Mode: All
 *
 * Must be called when the multiplayer session ends, or when the local user leaves a session in progress.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the session was ended normally
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_AntiCheat_InvalidMode - If the current mode does not support this function
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_EndSession(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_EndSessionOptions* Options);

/**
 * Polls for changes in client integrity status.
 * Mode: All
 *
 * The purpose of this function is to allow the game to display information
 * about anti-cheat integrity problems to the user. These are often the result of a
 * corrupt game installation rather than cheating attempts. This function does not
 * check for violations, it only provides information about violations which have
 * automatically been discovered by the anti-cheat client. Such a violation may occur
 * at any time and afterwards the user will be unable to join any protected multiplayer
 * session until after restarting the game. Note that this function returns EOS_NotFound
 * when everything is normal and there is no violation to display.
 *
 * @param Options Structure containing input data.
 * @param OutViolationType On success, receives a code describing the violation that occurred.
 * @param OutMessage On success, receives a string describing the violation which should be displayed to the user.
 *
 * @return EOS_Success - If violation information was returned successfully
 *		   EOS_LimitExceeded - If OutMessage is too small to receive the message string. Call again with a larger OutMessage.
 *         EOS_NotFound - If no violation has occurred since the last call
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_PollStatus(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_PollStatusOptions* Options, EOS_EAntiCheatClientViolationType* OutViolationType, char* OutMessage);

/**
 * Optional. Adds an integrity catalog and certificate pair from outside the game directory,
 * for example to support mods that load from elsewhere.
 * Mode: All
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the integrity catalog was added successfully
 *         EOS_InvalidParameters - If input data was invalid
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_AddExternalIntegrityCatalog(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_AddExternalIntegrityCatalogOptions* Options);

/**
 * Call when an anti-cheat message is received from the game server.
 * Mode: EOS_ACCM_ClientServer.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the message was processed successfully
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_AntiCheat_InvalidMode - If the current mode does not support this function
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_ReceiveMessageFromServer(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_ReceiveMessageFromServerOptions* Options);

/**
 * Optional NetProtect feature for game message encryption.
 * Calculates the required decrypted buffer size for a given input data length.
 * This will not change for a given SDK version, and allows one time allocation of reusable buffers.
 * Mode: EOS_ACCM_ClientServer.
 *
 * @param Options Structure containing input data.
 * @param OutBufferLengthBytes On success, the OutBuffer length in bytes that is required to call ProtectMessage on the given input size.
 *
 * @return EOS_Success - If the output length was calculated successfully
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_AntiCheat_InvalidMode - If the current mode does not support this function
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_GetProtectMessageOutputLength(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_GetProtectMessageOutputLengthOptions* Options, uint32_t* OutBufferSizeBytes);

/**
 * Optional NetProtect feature for game message encryption.
 * Encrypts an arbitrary message that will be sent to the game server and decrypted on the other side.
 * Mode: EOS_ACCM_ClientServer.
 *
 * Options.Data and OutBuffer may refer to the same buffer to encrypt in place.
 *
 * @param Options Structure containing input data.
 * @param OutBuffer On success, buffer where encrypted message data will be written.
 * @param OutBytesWritten On success, the number of bytes that were written to OutBuffer.
 *
 * @return EOS_Success - If the message was protected successfully
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_AntiCheat_InvalidMode - If the current mode does not support this function
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_ProtectMessage(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_ProtectMessageOptions* Options, void* OutBuffer, uint32_t* OutBytesWritten);

/**
 * Optional NetProtect feature for game message encryption.
 * Decrypts an encrypted message received from the game server.
 * Mode: EOS_ACCM_ClientServer.
 *
 * Options.Data and OutBuffer may refer to the same buffer to decrypt in place.
 *
 * @param Options Structure containing input data.
 * @param OutBuffer On success, buffer where encrypted message data will be written.
 * @param OutBytesWritten On success, the number of bytes that were written to OutBuffer.
 *
 * @return EOS_Success - If the message was unprotected successfully
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_AntiCheat_InvalidMode - If the current mode does not support this function
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_UnprotectMessage(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_UnprotectMessageOptions* Options, void* OutBuffer, uint32_t* OutBytesWritten);

/**
 * Registers a connected peer-to-peer client.
 * Mode: EOS_ACCM_PeerToPeer.
 *
 * Must be paired with a call to EOS_AntiCheatClient_UnregisterPeer if this user leaves the session
 * in progress, or EOS_AntiCheatClient_EndSession if the entire session is ending.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the player was registered successfully
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_AntiCheat_InvalidMode - If the current mode does not support this function
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_RegisterPeer(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_RegisterPeerOptions* Options);

/**
 * Unregisters a disconnected peer-to-peer client.
 * Mode: EOS_ACCM_PeerToPeer.
 *
 * Must be called when a user leaves a session in progress.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the player was unregistered successfully
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_AntiCheat_InvalidMode - If the current mode does not support this function
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_UnregisterPeer(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_UnregisterPeerOptions* Options);

/**
 * Call when an anti-cheat message is received from a peer.
 * Mode: EOS_ACCM_PeerToPeer.
 *
 * @param Options Structure containing input data.
 *
 * @return EOS_Success - If the message was processed successfully
 *         EOS_InvalidParameters - If input data was invalid
 *         EOS_AntiCheat_InvalidMode - If the current mode does not support this function
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_AntiCheatClient_ReceiveMessageFromPeer(EOS_HAntiCheatClient Handle, const EOS_AntiCheatClient_ReceiveMessageFromPeerOptions* Options);
