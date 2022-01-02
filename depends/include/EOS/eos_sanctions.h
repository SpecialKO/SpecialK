// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_sanctions_types.h"

/**
 * Start an asynchronous query to retrieve any active sanctions for a specified user.
 * Call EOS_Sanctions_GetPlayerSanctionCount and EOS_Sanctions_CopyPlayerSanctionByIndex to retrieve the data.
 *
 * @param Options Structure containing the input parameters
 * @param ClientData Arbitrary data that is passed back to you in the CompletionDelegate
 * @param CompletionDelegate A callback that is fired when the async operation completes, either successfully or in error
 *
 * @see EOS_Sanctions_GetPlayerSanctionCount
 * @see EOS_Sanctions_CopyPlayerSanctionByIndex
 */
EOS_DECLARE_FUNC(void) EOS_Sanctions_QueryActivePlayerSanctions(EOS_HSanctions Handle, const EOS_Sanctions_QueryActivePlayerSanctionsOptions* Options, void* ClientData, const EOS_Sanctions_OnQueryActivePlayerSanctionsCallback CompletionDelegate);

/**
 * Fetch the number of player sanctions that have been retrieved for a given player.
 * You must call QueryActivePlayerSanctions first to retrieve the data from the service backend.
 *
 * @param Options Structure containing the input parameters
 *
 * @see EOS_Sanctions_QueryActivePlayerSanctions
 * @see EOS_Sanctions_CopyPlayerSanctionByIndex
 *
 * @return Number of available sanctions for this player.
 */
EOS_DECLARE_FUNC(uint32_t) EOS_Sanctions_GetPlayerSanctionCount(EOS_HSanctions Handle, const EOS_Sanctions_GetPlayerSanctionCountOptions* Options);

/**
 * Copies an active player sanction.
 * You must call QueryActivePlayerSanctions first to retrieve the data from the service backend.
 * On success, EOS_Sanctions_PlayerSanction_Release must be called on OutSanction to free memory.
 *
 * @param Options Structure containing the input parameters
 * @param OutSanction The player sanction data for the given index, if it exists and is valid
 *
 * @see EOS_Sanctions_QueryActivePlayerSanctions
 * @see EOS_Sanctions_PlayerSanction_Release
 *
 * @return EOS_Success if the information is available and passed out in OutSanction
 *         EOS_InvalidParameters if you pass a null pointer for the out parameter
 *         EOS_NotFound if the player achievement is not found
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_Sanctions_CopyPlayerSanctionByIndex(EOS_HSanctions Handle, const EOS_Sanctions_CopyPlayerSanctionByIndexOptions* Options, EOS_Sanctions_PlayerSanction ** OutSanction);
