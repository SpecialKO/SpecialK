// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_common.h"

#pragma pack(push, 8)

EXTERN_C typedef struct EOS_ReportsHandle* EOS_HReports;

/**
 * An enumeration of the different player behavior categories that can be reported.
 */
EOS_ENUM(EOS_EPlayerReportsCategory,
	/** Not used */
	EOS_PRC_Invalid = 0,
	/** The reported player is cheating */
	EOS_PRC_Cheating = 1,
	/** The reported player is exploiting the game */
	EOS_PRC_Exploiting = 2,
	/** The reported player has an offensive profile, name, etc */
	EOS_PRC_OffensiveProfile = 3,
	/** The reported player is being abusive in chat */
	EOS_PRC_VerbalAbuse = 4,
	/** The reported player is scamming other players */
	EOS_PRC_Scamming = 5,
	/** The reported player is spamming chat */
	EOS_PRC_Spamming = 6,
	/** The player is being reported for something else */
	EOS_PRC_Other = 7
);

/** Max length of a report message text, not including the null terminator. */
#define EOS_REPORTS_REPORTMESSAGE_MAX_LENGTH 512

/** Max length of a report context JSON payload, not including the null terminator. */
#define EOS_REPORTS_REPORTCONTEXT_MAX_LENGTH 4096

/** The most recent version of the EOS_Reports_SendPlayerBehaviorReport API. */
#define EOS_REPORTS_SENDPLAYERBEHAVIORREPORT_API_LATEST 2

/**
 * Input parameters for the EOS_Reports_SendPlayerBehaviorReport function.
 */
EOS_STRUCT(EOS_Reports_SendPlayerBehaviorReportOptions, (
	/** API Version: Set this to EOS_REPORTS_SENDPLAYERBEHAVIORREPORT_API_LATEST. */
	int32_t ApiVersion;
	/** Product User ID of the reporting player */
	EOS_ProductUserId ReporterUserId;
	/** Product User ID of the reported player. */
	EOS_ProductUserId ReportedUserId;
	/** Category for the player report. */
	EOS_EPlayerReportsCategory Category;
	/**
	 * Optional plain text string associated with the report as UTF-8 encoded null-terminated string.
	 *
	 * The length of the message can be at maximum up to EOS_REPORTS_REPORTMESSAGE_MAX_LENGTH bytes
	 * and any excess characters will be truncated upon sending the report.
	 */
	const char* Message;
	/**
	 * Optional JSON string associated with the report as UTF-8 encoded null-terminated string.
	 * This is intended as a way to associate arbitrary structured context information with a report.
	 *
	 * This string needs to be valid JSON, report will fail otherwise.
	 * The length of the context can be at maximum up to EOS_REPORTS_REPORTCONTEXT_MAX_LENGTH bytes, not including the null terminator, report will fail otherwise.
	 */
	const char* Context;
));

/**
 * Output parameters for the EOS_Reports_SendPlayerBehaviorReport function.
 */
EOS_STRUCT(EOS_Reports_SendPlayerBehaviorReportCompleteCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_Reports_SendPlayerBehaviorReport. */
	void* ClientData;
));

/**
 * Function prototype definition for callbacks passed to EOS_Reports_SendPlayerBehaviorReport.
 * @param Data A EOS_Reports_SendPlayerBehaviorReportCompleteCallbackInfo containing the output information and result.
 */
EOS_DECLARE_CALLBACK(EOS_Reports_OnSendPlayerBehaviorReportCompleteCallback, const EOS_Reports_SendPlayerBehaviorReportCompleteCallbackInfo* Data);

#pragma pack(pop)
