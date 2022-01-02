// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_reports_types.h"

/**
 * The following EOS_Reports_* functions allow you to send reports directly from the players
 * to the back-end services without having to route these through a game server.
 */

/**
 * Sends the provided report directly to the Epic Online Services back-end.
 *
 * @param Options Structure containing the player report information.
 * @param ClientData Optional client data provided by the user of the SDK.
 * @param CompletionDelegate This function is called when the send operation completes.
 */
EOS_DECLARE_FUNC(void) EOS_Reports_SendPlayerBehaviorReport(EOS_HReports Handle, const EOS_Reports_SendPlayerBehaviorReportOptions* Options, void* ClientData, const EOS_Reports_OnSendPlayerBehaviorReportCompleteCallback CompletionDelegate);
