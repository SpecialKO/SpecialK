// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_common.h"

#pragma pack(push, 8)

EXTERN_C typedef struct EOS_KWSHandle* EOS_HKWS;

/** Maximum number of permissions that may be requested */
#define EOS_KWS_MAX_PERMISSIONS 16

/** Maximum size of the name for the permission */
#define EOS_KWS_MAX_PERMISSION_LENGTH 32

/**
 * An enumeration of the different permission statuses.
 */
EOS_ENUM(EOS_EKWSPermissionStatus,
	/** Permission has been granted */
	EOS_KPS_GRANTED = 0,
	/** Permission has been rejected */
	EOS_KPS_REJECTED = 1,
	/** Permission is still pending approval */
	EOS_KPS_PENDING = 2
);

/** The most recent version of the EOS_KWS_PermissionStatus API. */
#define EOS_KWS_PERMISSIONSTATUS_API_LATEST 1

EOS_STRUCT(EOS_KWS_PermissionStatus, (
	/** API Version: Set this to EOS_KWS_PERMISSIONSTATUS_API_LATEST. */
	int32_t ApiVersion;
	/** Name of the permission */
	const char* Name;
	/** Status of the permission */
	EOS_EKWSPermissionStatus Status;
));

EOS_DECLARE_FUNC(void) EOS_KWS_PermissionStatus_Release(EOS_KWS_PermissionStatus* PermissionStatus);

 /** The most recent version of the EOS_KWS_QueryAgeGate API. */
#define EOS_KWS_QUERYAGEGATE_API_LATEST 1

/**
 * Input parameters for the EOS_KWS_QueryAgeGate function.
 */
EOS_STRUCT(EOS_KWS_QueryAgeGateOptions, (
	/** API Version: Set this to EOS_KWS_QUERYAGEGATE_API_LATEST. */
	int32_t ApiVersion;
));

/**
 * Output parameters for the EOS_KWS_QueryAgeGate Function. These parameters are received through the callback provided to EOS_KWS_QueryAgeGate
 */
EOS_STRUCT(EOS_KWS_QueryAgeGateCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_KWS_QueryAgeGate */
	void* ClientData;
	/** Country code determined for this request based on the local client's ip address that the backend resolves */
	const char* CountryCode;
	/** Age of consent in the given country */
	uint32_t AgeOfConsent;
));

/**
 * Function prototype definition for callbacks passed to EOS_KWS_QueryAgeGate
 * @param Data A EOS_KWS_QueryAgeGateCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_KWS_OnQueryAgeGateCallback, const EOS_KWS_QueryAgeGateCallbackInfo* Data);

/** The most recent version of the EOS_KWS_CreateUser API. */
#define EOS_KWS_CREATEUSER_API_LATEST 1

/**
 * Input parameters for the EOS_KWS_CreateUser function.
 */
EOS_STRUCT(EOS_KWS_CreateUserOptions, (
	/** API Version: Set this to EOS_KWS_CREATEUSER_API_LATEST. */
	int32_t ApiVersion;
	/** Local user creating a KWS entry */
	EOS_ProductUserId LocalUserId;
	/** Date of birth in ISO8601 form (YYYY-MM-DD) */
	const char* DateOfBirth;
	/** Parent email */
	const char* ParentEmail;
));

/**
 * Output parameters for the EOS_KWS_CreateUserOptions Function. These parameters are received through the callback provided to EOS_KWS_CreateUser
 */
EOS_STRUCT(EOS_KWS_CreateUserCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_KWS_CreateUser */
	void* ClientData;
	/** Local user that created a KWS entry */
	EOS_ProductUserId LocalUserId;
	/** KWS UserId created */
	const char* KWSUserId;
	/** Is this user a minor */
	EOS_Bool bIsMinor;
));


/**
 * Function prototype definition for callbacks passed to EOS_KWS_CreateUser
 * @param Data A EOS_KWS_CreateUserCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_KWS_OnCreateUserCallback, const EOS_KWS_CreateUserCallbackInfo* Data);

 /** The most recent version of the EOS_KWS_QueryPermissions API. */
#define EOS_KWS_QUERYPERMISSIONS_API_LATEST 1

/**
 * Input parameters for the EOS_KWS_QueryPermissions function.
 */
EOS_STRUCT(EOS_KWS_QueryPermissionsOptions, (
	/** API Version: Set this to EOS_KWS_QUERYPERMISSIONS_API_LATEST. */
	int32_t ApiVersion;
	/** Local user querying their permisssions */
	EOS_ProductUserId LocalUserId;
));

/**
 * Output parameters for the EOS_KWS_QueryPermissionsOptions Function. These parameters are received through the callback provided to EOS_KWS_QueryPermissions
 */
EOS_STRUCT(EOS_KWS_QueryPermissionsCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_KWS_QueryPermissions */
	void* ClientData;
	/** Local user querying their permisssions */
	EOS_ProductUserId LocalUserId;
	/** KWS UserId created */
	const char* KWSUserId;
	/** Date of birth in ISO8601 form (YYYY-MM-DD) */
	const char* DateOfBirth;
	/** Is this user a minor */
	EOS_Bool bIsMinor;
));

/**
 * Function prototype definition for callbacks passed to EOS_KWS_QueryPermissions
 * @param Data A EOS_KWS_QueryPermissionsCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_KWS_OnQueryPermissionsCallback, const EOS_KWS_QueryPermissionsCallbackInfo* Data);

 /** The most recent version of the EOS_KWS_UpdateParentEmail API. */
#define EOS_KWS_UPDATEPARENTEMAIL_API_LATEST 1

/**
 * Input parameters for the EOS_KWS_UpdateParentEmail function.
 */
EOS_STRUCT(EOS_KWS_UpdateParentEmailOptions, (
	/** API Version: Set this to EOS_KWS_UPDATEPARENTEMAIL_API_LATEST. */
	int32_t ApiVersion;
	/** Local user updating parental information */
	EOS_ProductUserId LocalUserId;
	/** New parent email */
	const char* ParentEmail;
));

/**
 * Output parameters for the EOS_KWS_UpdateParentEmailOptions Function. These parameters are received through the callback provided to EOS_KWS_UpdateParentEmail
 */
EOS_STRUCT(EOS_KWS_UpdateParentEmailCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_KWS_UpdateParentEmail */
	void* ClientData;
	/** Local user updating their parental email */
	EOS_ProductUserId LocalUserId;
));

/**
 * Function prototype definition for callbacks passed to EOS_KWS_UpdateParentEmail
 * @param Data A EOS_KWS_UpdateParentEmailCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_KWS_OnUpdateParentEmailCallback, const EOS_KWS_UpdateParentEmailCallbackInfo* Data);

 /** The most recent version of the EOS_KWS_RequestPermissions API. */
#define EOS_KWS_REQUESTPERMISSIONS_API_LATEST 1

/**
 * Input parameters for the EOS_KWS_RequestPermissions function.
 */
EOS_STRUCT(EOS_KWS_RequestPermissionsOptions, (
	/** API Version: Set this to EOS_KWS_REQUESTPERMISSIONS_API_LATEST. */
	int32_t ApiVersion;
	/** Local user requesting new permisssions */
	EOS_ProductUserId LocalUserId;
	/** The number of permissions to request, may not exceed EOS_KWS_MAX_PERMISSIONS. Only new permissions need be included. */
	uint32_t PermissionKeyCount;
	/** Names of the permissions to request (Setup with KWS) */
	const char** PermissionKeys;
));

/**
 * Output parameters for the EOS_KWS_RequestPermissionsOptions Function. These parameters are received through the callback provided to EOS_KWS_RequestPermissions
 */
EOS_STRUCT(EOS_KWS_RequestPermissionsCallbackInfo, (
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_KWS_RequestPermissions */
	void* ClientData;
	/** Local user requesting new permisssions */
	EOS_ProductUserId LocalUserId;
));

/**
 * Function prototype definition for callbacks passed to EOS_KWS_RequestPermissions
 * @param Data A EOS_KWS_RequestPermissionsCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_KWS_OnRequestPermissionsCallback, const EOS_KWS_RequestPermissionsCallbackInfo* Data);


#define EOS_KWS_GETPERMISSIONSCOUNT_API_LATEST 1

/**
 * Input parameters for the EOS_KWS_GetPermissionsCount function.
 */
EOS_STRUCT(EOS_KWS_GetPermissionsCountOptions, (
	/** API Version: Set this to EOS_KWS_GETPERMISSIONSCOUNT_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the local user whose permissions are being accessed */
	EOS_ProductUserId LocalUserId;
));

/** The most recent version of the EOS_KWS_CopyPermissionByIndex API. */
#define EOS_KWS_COPYPERMISSIONBYINDEX_API_LATEST 1

/**
 * Input parameters for the EOS_KWS_CopyPermissionByIndex function.
 */
EOS_STRUCT(EOS_KWS_CopyPermissionByIndexOptions, (
	/** API Version: Set this to EOS_KWS_COPYPERMISSIONBYINDEX_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the local user whose permissions are being accessed */
	EOS_ProductUserId LocalUserId;
	/** The index of the permission to get. */
	uint32_t Index;
));

/** The most recent version of the EOS_KWS_CreateUser API. */
#define EOS_KWS_GETPERMISSIONBYKEY_API_LATEST 1

/**
 * Input parameters for the EOS_KWS_GetPermissionByKey function.
 */
EOS_STRUCT(EOS_KWS_GetPermissionByKeyOptions, (
	/** API Version: Set this to EOS_KWS_GETPERMISSIONBYKEY_API_LATEST. */
	int32_t ApiVersion;
	/** The Product User ID of the local user getting permissions */
	EOS_ProductUserId LocalUserId;
	/** Permission name to query */
	const char* Key;
));

/** The most recent version of the EOS_KWS_AddNotifyPermissionsUpdateReceived API. */
#define EOS_KWS_ADDNOTIFYPERMISSIONSUPDATERECEIVED_API_LATEST 1

EOS_STRUCT(EOS_KWS_AddNotifyPermissionsUpdateReceivedOptions, (
	/** API Version: Set this to EOS_KWS_ADDNOTIFYPERMISSIONSUPDATERECEIVED_API_LATEST. */
	int32_t ApiVersion;
));

/**
 * Output parameters for the EOS_KWS_OnPermissionsUpdateReceivedCallback Function.
 */
EOS_STRUCT(EOS_KWS_PermissionsUpdateReceivedCallbackInfo, (
	/** Context that was passed into EOS_KWS_AddNotifyPermissionsUpdateReceived */
	void* ClientData;
	/** Recipient Local user id */
	EOS_ProductUserId LocalUserId;
));

/**
 * Function prototype definition for notifications that comes from EOS_KWS_AddNotifyPermissionsUpdateReceived
 *
 * @param Data A EOS_KWS_PermissionsUpdateReceivedCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_KWS_OnPermissionsUpdateReceivedCallback, const EOS_KWS_PermissionsUpdateReceivedCallbackInfo* Data);

#pragma pack(pop)
