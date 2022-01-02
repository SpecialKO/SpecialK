// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_progressionsnapshot_types.h"

/**
 * Progression Snapshots allow you to store player specific game state.
 *
 * BeginSnapshot must be called to obtain a handle for a given player.
 * AddProgression allows you to add Key/Value pairs that represent some state for that player in the game.
 * SubmitSnapshot sends all the progression data you have added (via AddProgression) to the service.
 * EndSnapshot cleans up internal resources allocated for that snapshot.
 * DeleteSnapshot wipes out all data associated with a particular player.
 *
 * @see EOS_Platform_GetProgressionSnapshotInterface
 */

/**
 * Creates a new progression-snapshot resource for a given user.
 *
 * @param Options Object containing properties that identifies the PUID this Snapshot will belong to.
 * @param OutSnapshotId A progression-snapshot identifier output parameter. Use that identifier to reference the snapshot in the other APIs.
 *
 * @return EOS_Success when successful.
 *          EOS_ProgressionSnapshot_SnapshotIdUnavailable when no IDs are available. This is irrecoverable state.
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_ProgressionSnapshot_BeginSnapshot(EOS_HProgressionSnapshot Handle, const EOS_ProgressionSnapshot_BeginSnapshotOptions* Options, uint32_t* OutSnapshotId);

/**
 * Stores a Key/Value pair in memory for a given snapshot.
 * If multiple calls happen with the same key, the last invocation wins, overwriting the previous value for that
 * given key.
 *
 * The order in which the Key/Value pairs are added is stored as is for later retrieval/display.
 * Ideally, you would make multiple calls to AddProgression() followed by a single call to SubmitSnapshot().
 *
 * @return EOS_Success when successful; otherwise, EOS_NotFound
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_ProgressionSnapshot_AddProgression(EOS_HProgressionSnapshot Handle, const EOS_ProgressionSnapshot_AddProgressionOptions* Options);

/**
 * Saves the previously added Key/Value pairs of a given Snapshot to the service.
 *
 * Note: This will overwrite any prior progression data stored with the service that's associated with the user.
 **/
EOS_DECLARE_FUNC(void) EOS_ProgressionSnapshot_SubmitSnapshot(EOS_HProgressionSnapshot Handle, const EOS_ProgressionSnapshot_SubmitSnapshotOptions* Options, void* ClientData, const EOS_ProgressionSnapshot_OnSubmitSnapshotCallback CompletionDelegate);

/**
 * Cleans up and releases resources associated with the given progression snapshot identifier.
 *
 * @return EOS_Success when successful; otherwise, EOS_NotFound
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_ProgressionSnapshot_EndSnapshot(EOS_HProgressionSnapshot Handle, const EOS_ProgressionSnapshot_EndSnapshotOptions* Options);

/**
 * Wipes out all progression data for the given user from the service. However, any previous progression data that haven't
 * been submitted yet are retained.
 */
EOS_DECLARE_FUNC(void) EOS_ProgressionSnapshot_DeleteSnapshot(EOS_HProgressionSnapshot Handle, const EOS_ProgressionSnapshot_DeleteSnapshotOptions* Options, void* ClientData, const EOS_ProgressionSnapshot_OnDeleteSnapshotCallback CompletionDelegate);
