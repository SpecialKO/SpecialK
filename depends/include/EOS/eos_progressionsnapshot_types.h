// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_common.h"

#pragma pack(push, 8)

/** Handle to the ProgressionSnapshot interface */
EXTERN_C typedef struct EOS_ProgressionSnapshotHandle* EOS_HProgressionSnapshot;

#define EOS_INVALID_PROGRESSIONSNAPSHOTID  0

#define EOS_PROGRESSIONSNAPSHOT_BEGINSNAPSHOT_API_LATEST 1
EOS_STRUCT(EOS_ProgressionSnapshot_BeginSnapshotOptions, (
	/** API Version: Set this to EOS_PROGRESSIONSNAPSHOT_BEGINSNAPSHOT_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the local user to whom the key/value pair belong */
	EOS_ProductUserId LocalUserId;
));

#define EOS_PROGRESSIONSNAPSHOT_ADDPROGRESSION_API_LATEST  1
EOS_STRUCT(EOS_ProgressionSnapshot_AddProgressionOptions, (
	/** API Version: Set this to EOS_PROGRESSIONSNAPSHOT_ADDPROGRESSION_API_LATEST. */
	int32_t ApiVersion;
	/** The Snapshot Id received via a EOS_ProgressionSnapshot_BeginSnapshot function. */
	uint32_t SnapshotId;
	/** The key in a key/value pair of progression entry */
	const char* Key;
	/** The value in a key/value pair of progression entry */
	const char* Value;
));

#define EOS_PROGRESSIONSNAPSHOT_SUBMITSNAPSHOT_API_LATEST   1
EOS_STRUCT(EOS_ProgressionSnapshot_SubmitSnapshotOptions, (
	/** API Version: Set this to EOS_PROGRESSIONSNAPSHOT_SUBMITSNAPSHOT_API_LATEST. */
	int32_t ApiVersion;
	/** The Snapshot Id received via a EOS_ProgressionSnapshot_BeginSnapshot function. */
	uint32_t SnapshotId;
));

#define EOS_PROGRESSIONSNAPSHOT_ENDSNAPSHOT_API_LATEST    1
EOS_STRUCT(EOS_ProgressionSnapshot_EndSnapshotOptions, (
	/** API Version: Set this to EOS_PROGRESSIONSNAPSHOT_ENDSNAPSHOT_API_LATEST. */
	int32_t ApiVersion;
	/** The Snapshot Id received via a EOS_ProgressionSnapshot_BeginSnapshot function. */
	uint32_t SnapshotId;
));

EOS_STRUCT(EOS_ProgressionSnapshot_SubmitSnapshotCallbackInfo, (
	/* The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** The Snapshot Id used in the Submit function. */
	uint32_t SnapshotId;
	/** Context that was passed into EOS_ProgressionSnapshot_SubmitSnapshot. */
	void* ClientData;
));

EOS_DECLARE_CALLBACK(EOS_ProgressionSnapshot_OnSubmitSnapshotCallback, const EOS_ProgressionSnapshot_SubmitSnapshotCallbackInfo* Data);

#define EOS_PROGRESSIONSNAPSHOT_DELETESNAPSHOT_API_LATEST   1
EOS_STRUCT(EOS_ProgressionSnapshot_DeleteSnapshotOptions, (
	/** API Version: Set this to EOS_PROGRESSIONSNAPSHOT_DELETESNAPSHOT_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the local user to whom the key/value pair belong */
	EOS_ProductUserId LocalUserId;
));

EOS_STRUCT(EOS_ProgressionSnapshot_DeleteSnapshotCallbackInfo, (
	/* The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** The Product User ID of the local user to whom the key/value pair belong */
	EOS_ProductUserId LocalUserId;
	/** Context that was passed into EOS_ProgressionSnapshot_SubmitSnapshot. */
	void* ClientData;
));
EOS_DECLARE_CALLBACK(EOS_ProgressionSnapshot_OnDeleteSnapshotCallback, const EOS_ProgressionSnapshot_DeleteSnapshotCallbackInfo* Data);

#pragma pack(pop)
