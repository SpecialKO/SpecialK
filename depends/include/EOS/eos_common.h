// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "eos_base.h"

#pragma pack(push, 8)

#undef EOS_RESULT_VALUE
#undef EOS_RESULT_VALUE_LAST
#define EOS_RESULT_VALUE(Name, Value) Name = Value,
#define EOS_RESULT_VALUE_LAST(Name, Value) Name = Value

EOS_ENUM_START(EOS_EResult)
#include "eos_result.h"
EOS_ENUM_END(EOS_EResult);

#undef EOS_RESULT_VALUE
#undef EOS_RESULT_VALUE_LAST

/**
 * Returns a string representation of an EOS_EResult. 
 * The return value is never null.
 * The return value must not be freed.
 *
 * Example: EOS_EResult_ToString(EOS_Success) returns "EOS_Success"
 */
EOS_DECLARE_FUNC(const char*) EOS_EResult_ToString(EOS_EResult Result);

/**
 * Returns whether a result is to be considered the final result, or false if the callback that returned this result
 * will be called again either after some time or from another action.
 *
 * @param Result The result to check against being a final result for an operation
 * @return True if this result means the operation is complete, false otherwise
 */
EOS_DECLARE_FUNC(EOS_Bool) EOS_EResult_IsOperationComplete(EOS_EResult Result);

/**
 * Encode a byte array into hex encoded string
 *
 * @return An EOS_EResult that indicates whether the byte array was converted and copied into the OutBuffer.
 *         EOS_Success if the encoding was successful and passed out in OutBuffer
 *         EOS_InvalidParameters if you pass a null pointer on invalid length for any of the parameters
 *         EOS_LimitExceeded - The OutBuffer is not large enough to receive the encoding. InOutBufferLength contains the required minimum length to perform the operation successfully.
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_ByteArray_ToString(const uint8_t* ByteArray, const uint32_t Length, char* OutBuffer, uint32_t* InOutBufferLength);

/**
 * A handle to a user's Epic Account ID
 * This ID is associated with a specific login associated with Epic Account Services
 *
 * @see EOS_Auth_Login
 */
typedef struct EOS_EpicAccountIdDetails* EOS_EpicAccountId;

/** 
 * Check whether or not the given Epic Account ID is considered valid
 * NOTE: This will return true for any EOS_EpicAccountId created with EOS_EpicAccountId_FromString as there is no validation
 * 
 * @param AccountId The Epic Account ID to check for validity
 * @return EOS_TRUE if the EOS_EpicAccountId is valid, otherwise EOS_FALSE
 */
EOS_DECLARE_FUNC(EOS_Bool) EOS_EpicAccountId_IsValid(EOS_EpicAccountId AccountId);

/**
 * Retrieve a null-terminated stringified Epic Account ID from an EOS_EpicAccountId. This is useful for replication of Epic Account IDs in multiplayer games.
 * This string will be no larger than EOS_EPICACCOUNTID_MAX_LENGTH + 1 and will only contain UTF8-encoded printable characters (excluding the null-terminator).
 *
 * @param AccountId The Epic Account ID for which to retrieve the stringified version.
 * @param OutBuffer The buffer into which the character data should be written
 * @param InOutBufferLength The size of the OutBuffer in characters.
 *                          The input buffer should include enough space to be null-terminated.
 *                          When the function returns, this parameter will be filled with the length of the string copied into OutBuffer including the null termination character.
 *
 * @return An EOS_EResult that indicates whether the Epic Account ID string was copied into the OutBuffer.
 *         EOS_Success - The OutBuffer was filled, and InOutBufferLength contains the number of characters copied into OutBuffer including the null terminator.
 *         EOS_InvalidParameters - Either OutBuffer or InOutBufferLength were passed as NULL parameters.
 *         EOS_InvalidUser - The AccountId is invalid and cannot be stringified.
 *         EOS_LimitExceeded - The OutBuffer is not large enough to receive the Epic Account ID string. InOutBufferLength contains the required minimum length to perform the operation successfully.
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_EpicAccountId_ToString(EOS_EpicAccountId AccountId, char* OutBuffer, int32_t* InOutBufferLength);

/**
 * Retrieve an EOS_EpicAccountId from a raw string representing an Epic Account ID. The input string must be null-terminated.
 * NOTE: There is no validation on the string format, this should only be used with values serialized from legitimate sources such as EOS_EpicAccountId_ToString
 *
 * @param AccountIdString The stringified account ID for which to retrieve the Epic Account ID
 * @return The EOS_EpicAccountId that corresponds to the AccountIdString
 */
EOS_DECLARE_FUNC(EOS_EpicAccountId) EOS_EpicAccountId_FromString(const char* AccountIdString);

/** 
 * A character buffer of this size is large enough to fit a successful output of EOS_EpicAccountId_ToString. This length does not include the null-terminator.
 * The EpicAccountId data structure is opaque in nature and no assumptions of its structure should be inferred
 */
#define EOS_EPICACCOUNTID_MAX_LENGTH 32

/** 
 * A handle to a user's Product User ID (game services related ecosystem)
 * This ID is associated with any of the external account providers (of which Epic Account Services is one)
 * 
 * @see EOS_Connect_Login
 * @see EOS_EExternalCredentialType 
 */
typedef struct EOS_ProductUserIdDetails* EOS_ProductUserId;

/**
 * Check whether or not the given account unique ID is considered valid
 * NOTE: This will return true for any EOS_ProductUserId created with EOS_ProductUserId_FromString as there is no validation
 *
 * @param AccountId The Product User ID to check for validity
 * @return EOS_TRUE if the EOS_ProductUserId is valid, otherwise EOS_FALSE
 */
EOS_DECLARE_FUNC(EOS_Bool) EOS_ProductUserId_IsValid(EOS_ProductUserId AccountId);

/**
 * Retrieve a null-terminated stringified Product User ID from an EOS_ProductUserId. This is useful for replication of Product User IDs in multiplayer games.
 * This string will be no larger than EOS_PRODUCTUSERID_MAX_LENGTH + 1 and will only contain UTF8-encoded printable characters (excluding the null-terminator).
 *
 * @param AccountId The Product User ID for which to retrieve the stringified version.
 * @param OutBuffer The buffer into which the character data should be written
 * @param InOutBufferLength The size of the OutBuffer in characters.
 *                          The input buffer should include enough space to be null-terminated.
 *                          When the function returns, this parameter will be filled with the length of the string copied into OutBuffer including the null termination character.
 *
 * @return An EOS_EResult that indicates whether the Product User ID string was copied into the OutBuffer.
 *         EOS_Success - The OutBuffer was filled, and InOutBufferLength contains the number of characters copied into OutBuffer including the null terminator.
 *         EOS_InvalidParameters - Either OutBuffer or InOutBufferLength were passed as NULL parameters.
 *         EOS_InvalidUser - The AccountId is invalid and cannot be stringified.
 *         EOS_LimitExceeded - The OutBuffer is not large enough to receive the Product User ID string. InOutBufferLength contains the required minimum length to perform the operation successfully.
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_ProductUserId_ToString(EOS_ProductUserId AccountId, char* OutBuffer, int32_t* InOutBufferLength);

/**
 * Retrieve an EOS_ProductUserId from a raw string representing an Epic Online Services Product User ID. The input string must be null-terminated.
 * NOTE: There is no validation on the string format, this should only be used with values serialized from legitimate sources such as EOS_ProductUserId_ToString
 *
 * @param ProductUserIdString The stringified product user ID for which to retrieve the Epic Online Services Product User ID
 * @return The EOS_ProductUserId that corresponds to the ProductUserIdString
 */
EOS_DECLARE_FUNC(EOS_ProductUserId) EOS_ProductUserId_FromString(const char* ProductUserIdString);

/** A character buffer of this size is large enough to fit a successful output of EOS_ProductUserId_ToString. This length does not include the null-terminator. */
#define EOS_PRODUCTUSERID_MAX_LENGTH 32

/** Handle to an existing registered notification (0 is an invalid handle) */
EXTERN_C typedef uint64_t EOS_NotificationId;

/** An invalid notification ID */
#define EOS_INVALID_NOTIFICATIONID ((EOS_NotificationId)0)

/** A handle to a continuance token @see eos_connect.h */
typedef struct EOS_ContinuanceTokenDetails* EOS_ContinuanceToken;

/**
 * Retrieve a null-terminated stringified continuance token from an EOS_ContinuanceToken.
 *
 * To get the required buffer size, call once with OutBuffer set to NULL, InOutBufferLength will contain the buffer size needed.
 * Call again with valid params to get the stringified continuance token which will only contain UTF8-encoded printable characters (excluding the null-terminator).
 *
 * @param ContinuanceToken The continuance token for which to retrieve the stringified version.
 * @param OutBuffer The buffer into which the character data should be written
 * @param InOutBufferLength The size of the OutBuffer in characters.
 *                          The input buffer should include enough space to be null-terminated.
 *                          When the function returns, this parameter will be filled with the length of the string copied into OutBuffer including the null termination character.
 *
 * @return An EOS_EResult that indicates whether the continuance token string was copied into the OutBuffer.
 *         EOS_Success - The OutBuffer was filled, and InOutBufferLength contains the number of characters copied into OutBuffer including the null terminator.
 *         EOS_InvalidParameters - Either OutBuffer or InOutBufferLength were passed as NULL parameters.
 *         EOS_InvalidUser - The AccountId is invalid and cannot be stringified.
 *         EOS_LimitExceeded - The OutBuffer is not large enough to receive the continuance token string. InOutBufferLength contains the required minimum length to perform the operation successfully.
 */
EOS_DECLARE_FUNC(EOS_EResult) EOS_ContinuanceToken_ToString(EOS_ContinuanceToken ContinuanceToken, char* OutBuffer, int32_t* InOutBufferLength);

/** The most recent version of the EOS_PageQuery structs. */
#define EOS_PAGEQUERY_API_LATEST 1

/** DEPRECATED! Use EOS_PAGEQUERY_API_LATEST instead. */
#define EOS_PAGINATION_API_LATEST EOS_PAGEQUERY_API_LATEST

/** The default MaxCount used for a EOS_PageQuery when the API allows the EOS_PageQuery to be omitted. */
#define EOS_PAGEQUERY_MAXCOUNT_DEFAULT 10

/** The maximum MaxCount used for a EOS_PageQuery. */
#define EOS_PAGEQUERY_MAXCOUNT_MAXIMUM 100

/**
 * A page query is part of query options. It is used to allow pagination of query results.
 */
EOS_STRUCT(EOS_PageQuery, (
	/** API Version: Set this to EOS_PAGEQUERY_API_LATEST. */
	int32_t ApiVersion;
	/** The index into the ordered query results to start the page at. */
	int32_t StartIndex;
	/** The maximum number of results to have in the page. */
	int32_t MaxCount;
));

/**
 * A page result is part of query callback info. It is used to provide pagination details of query results.
 */
EOS_STRUCT(EOS_PageResult, (
	/** The index into the ordered query results to start the page at. */
	int32_t StartIndex;
	/** The number of results in the current page. */
	int32_t Count;
	/** The number of results associated with they original query options. */
	int32_t TotalCount;
));

/**
 * All possible states of a local user
 *
 * @see EOS_Auth_AddNotifyLoginStatusChanged
 * @see EOS_Auth_GetLoginStatus
 * @see EOS_Auth_Login
 * @see EOS_Connect_AddNotifyLoginStatusChanged
 * @see EOS_Connect_GetLoginStatus
 * @see EOS_Connect_Login
 */
EOS_ENUM(EOS_ELoginStatus,
	/** Player has not logged in or chosen a local profile */
	EOS_LS_NotLoggedIn = 0,
	/** Player is using a local profile but is not logged in */
	EOS_LS_UsingLocalProfile = 1,
	/** Player has been validated by the platform specific authentication service */
	EOS_LS_LoggedIn = 2
);

/**
 * Supported types of data that can be stored with inside an attribute (used by sessions/lobbies/etc)
 *
 * @see EOS_LobbySearch_SetParameter
 * @see EOS_SessionSearch_SetParameter
 */
EOS_ENUM(EOS_EAttributeType,
	/** Boolean value (true/false) */
	EOS_AT_BOOLEAN = 0,
	/** 64 bit integers */
	EOS_AT_INT64 = 1,
	/** Double/floating point precision */
	EOS_AT_DOUBLE = 2,
	/** UTF8 Strings */
	EOS_AT_STRING = 3
);

typedef EOS_EAttributeType EOS_ESessionAttributeType;
typedef EOS_EAttributeType EOS_ELobbyAttributeType;

/**
 * All comparison operators associated with parameters in a search query
 *
 * @see EOS_LobbySearch_SetParameter
 * @see EOS_SessionSearch_SetParameter
 */
EOS_ENUM(EOS_EComparisonOp,
	/** Value must equal the one stored on the lobby/session */
	EOS_CO_EQUAL = 0,
	/** Value must not equal the one stored on the lobby/session */
	EOS_CO_NOTEQUAL = 1,
	/** Value must be strictly greater than the one stored on the lobby/session */
	EOS_CO_GREATERTHAN = 2,
	/** Value must be greater than or equal to the one stored on the lobby/session */
	EOS_CO_GREATERTHANOREQUAL = 3,
	/** Value must be strictly less than the one stored on the lobby/session */
	EOS_CO_LESSTHAN = 4,
	/** Value must be less than or equal to the one stored on the lobby/session */
	EOS_CO_LESSTHANOREQUAL = 5,
	/** Prefer values nearest the one specified ie. abs(SearchValue-SessionValue) closest to 0 */
	EOS_CO_DISTANCE = 6,
	/** Value stored on the lobby/session may be any from a specified list */
	EOS_CO_ANYOF = 7,
	/** Value stored on the lobby/session may NOT be any from a specified list */
	EOS_CO_NOTANYOF = 8,
	/** This one value is a part of a collection */
	EOS_CO_ONEOF = 9,
	/** This one value is NOT part of a collection */
	EOS_CO_NOTONEOF = 10,
	/** This value is a CASE SENSITIVE substring of an attribute stored on the lobby/session */
	EOS_CO_CONTAINS = 11
);

typedef EOS_EComparisonOp EOS_EOnlineComparisonOp;

/**
 * All supported external account providers
 *
 * @see EOS_Connect_QueryExternalAccountMappings
 */
EOS_ENUM(EOS_EExternalAccountType,
	/** External account is associated with Epic Games */
	EOS_EAT_EPIC = 0,
	/** External account is associated with Steam */
	EOS_EAT_STEAM = 1,
	/** External account is associated with PlayStation(TM)Network */
	EOS_EAT_PSN = 2,
	/** External account is associated with Xbox Live */
	EOS_EAT_XBL = 3,
	/** External account is associated with Discord */
	EOS_EAT_DISCORD = 4,
	/** External account is associated with GOG */
	EOS_EAT_GOG = 5,
	/**
	 * External account is associated with Nintendo
	 *
	 * With both EOS Connect and EOS UserInfo APIs, the associated account type is Nintendo Service Account ID.
	 * Local user authentication is possible using Nintendo Account ID, while the account type does not get exposed to the SDK in queries related to linked accounts information.
	 */
	EOS_EAT_NINTENDO = 6,
	/** External account is associated with Uplay */
	EOS_EAT_UPLAY = 7,
	/** External account is associated with an OpenID Provider */
	EOS_EAT_OPENID = 8,
	/** External account is associated with Apple */
	EOS_EAT_APPLE = 9,
	/** External account is associated with Google */
	EOS_EAT_GOOGLE = 10,
	/** External account is associated with Oculus */
	EOS_EAT_OCULUS = 11,
	/** External account is associated with itch.io */
	EOS_EAT_ITCHIO = 12,
	/** External account is associated with Amazon */
	EOS_EAT_AMAZON = 13
);

/**
 * List of the supported identity providers to authenticate a user.
 *
 * The type of authentication token is specific to each provider.
 * Tokens in string format should be passed as-is to the function.
 * Tokens retrieved as raw byte arrays should be converted into a hex-encoded UTF-8 string (e.g. "FA87097A..") before being passed to the function.
 * EOS_ByteArray_ToString can be used for this conversion.
 *
 * @see EOS_Auth_Login
 * @see EOS_Connect_Login
 */
EOS_ENUM(EOS_EExternalCredentialType,
	/**
	 * Epic Account Services Token
	 *
	 * Using ID Token is preferred, retrieved with EOS_Auth_CopyIdToken that returns EOS_Auth_IdToken::JsonWebToken.
	 * Using Auth Token is supported for backwards compatibility, retrieved with EOS_Auth_CopyUserAuthToken that returns EOS_Auth_Token::AccessToken.
	 *
	 * Supported with EOS_Connect_Login.
	 *
	 * @see EOS_Auth_CopyIdToken
	 * @see EOS_Auth_CopyUserAuthToken
	 */
	EOS_ECT_EPIC = 0,
	/**
	 * Steam Encrypted App Ticket
	 *
	 * Generated using the ISteamUser::RequestEncryptedAppTicket API of Steamworks SDK.
	 * For ticket generation parameters, use pDataToInclude(NULL) and cbDataToInclude(0).
	 *
	 * The retrieved App Ticket byte buffer needs to be converted into a hex-encoded UTF-8 string (e.g. "FA87097A..") before passing it to the EOS_Auth_Login or EOS_Connect_Login APIs.
	 * EOS_ByteArray_ToString can be used for this conversion.
	 *
	 * Supported with EOS_Auth_Login, EOS_Connect_Login.
	 */
	EOS_ECT_STEAM_APP_TICKET = 1,
	/**
	 * PlayStation(TM)Network ID Token
	 *
	 * Retrieved from the PlayStation(R) SDK. Please see first-party documentation for additional information.
	 *
	 * Supported with EOS_Auth_Login, EOS_Connect_Login.
	 */
	EOS_ECT_PSN_ID_TOKEN = 2,
	/**
	 * Xbox Live XSTS Token
	 *
	 * Retrieved from the GDK and XDK. Please see first-party documentation for additional information.
	 *
	 * Supported with EOS_Auth_Login, EOS_Connect_Login.
	 */
	EOS_ECT_XBL_XSTS_TOKEN = 3,
	/**
	 * Discord Access Token
	 *
	 * Retrieved using the ApplicationManager::GetOAuth2Token API of Discord SDK.
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_DISCORD_ACCESS_TOKEN = 4,
	/**
	 * GOG Galaxy Encrypted App Ticket
	 *
	 * Generated using the IUser::RequestEncryptedAppTicket API of GOG Galaxy SDK.
	 * For ticket generation parameters, use data(NULL) and dataSize(0).
	 *
	 * The retrieved App Ticket byte buffer needs to be converted into a hex-encoded UTF-8 string (e.g. "FA87097A..") before passing it to the EOS_Connect_Login API.
	 * For C/C++ API integration, use the EOS_ByteArray_ToString API for the conversion.
	 * For C# integration, you can use <see cref="Helper.ToHexString" /> for the conversion.
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_GOG_SESSION_TICKET = 5,
	/**
	 * Nintendo Account ID Token
	 *
	 * Identifies a Nintendo user account and is acquired through web flow authentication where the local user logs in using their email address/sign-in ID and password.
	 * This is the common Nintendo account that users login with outside the Nintendo Switch device.
	 *
	 * Supported with EOS_Auth_Login, EOS_Connect_Login.
	 */
	EOS_ECT_NINTENDO_ID_TOKEN = 6,
	/**
	 * Nintendo Service Account ID Token (NSA ID)
	 *
	 * The NSA ID identifies uniquely the local Nintendo Switch device. The authentication token is acquired locally without explicit user credentials.
	 * As such, it is the primary authentication method for seamless login on Nintendo Switch.
	 *
	 * The NSA ID is not exposed directly to the user and does not provide any means for login outside the local device.
	 * Because of this, Nintendo Switch users will need to link their Nintendo Account or another external user account
	 * to their Product User ID in order to share their game progression across other platforms. Otherwise, the user will
	 * not be able to login to their existing Product User ID on another platform due to missing login credentials to use.
	 * It is recommended that the game explicitly communicates this restriction to the user so that they will know to add
	 * the first linked external account on the Nintendo Switch device and then proceed with login on another platform.
	 *
	 * In addition to sharing cross-platform game progression, linking the Nintendo Account or another external account
	 * will allow preserving the game progression permanently. Otherwise, the game progression will be tied only to the
	 * local device. In case the user loses access to their local device, they will not be able to recover the game
	 * progression if it is only associated with this account type.
	 *
	 * Supported with EOS_Auth_Login, EOS_Connect_Login.
	 */
	EOS_ECT_NINTENDO_NSA_ID_TOKEN = 7,
	/**
	 * Uplay Access Token
	 */
	EOS_ECT_UPLAY_ACCESS_TOKEN = 8,
	/**
	 * OpenID Provider Access Token
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_OPENID_ACCESS_TOKEN = 9,
	/**
	 * Device ID access token that identifies the current locally logged in user profile on the local device.
	 * The local user profile here refers to the operating system user login, for example the user's Windows Account
	 * or on a mobile device the default active user profile.
	 *
	 * This credential type is used to automatically login the local user using the EOS Connect Device ID feature.
	 *
	 * The intended use of the Device ID feature is to allow automatically logging in the user on a mobile device
	 * and to allow playing the game without requiring the user to necessarily login using a real user account at all.
	 * This makes a seamless first-time experience possible and allows linking the local device with a real external
	 * user account at a later time, sharing the same EOS_ProductUserId that is being used with the Device ID feature.
	 *
	 * Supported with EOS_Connect_Login.
	 *
	 * @see EOS_Connect_CreateDeviceId
	 */
	EOS_ECT_DEVICEID_ACCESS_TOKEN = 10,
	/**
	 * Apple ID Token
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_APPLE_ID_TOKEN = 11,
	/**
	 * Google ID Token
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_GOOGLE_ID_TOKEN = 12,
	/**
	 * Oculus User ID and Nonce
	 *
	 * Call ovr_User_GetUserProof(), or Platform.User.GetUserProof() if you are using Unity, to retrieve the nonce.
	 * Then pass the local User ID and the Nonce as a "{UserID}|{Nonce}" formatted string for the EOS_Connect_Login Token parameter.
	 *
	 * Note that in order to successfully retrieve a valid non-zero id for the local user using ovr_User_GetUser(),
	 * your Oculus App needs to be configured in the Oculus Developer Dashboard to have the User ID feature enabled.
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_OCULUS_USERID_NONCE = 13,
	/**
	 * itch.io JWT Access Token
	 *
	 * Use the itch.io app manifest to receive a JWT access token for the local user via the ITCHIO_API_KEY process environment variable.
	 * The itch.io access token is valid for 7 days after which the game needs to be restarted by the user as otherwise EOS Connect
	 * authentication session can no longer be refreshed.
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_ITCHIO_JWT = 14,
	/**
	 * itch.io Key Access Token
	 *
	 * This access token type is retrieved through the OAuth 2.0 authentication flow for the itch.io application.
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_ITCHIO_KEY = 15,
	/**
	 * Epic Games ID Token
	 * 
	 * Acquired using EOS_Auth_CopyIdToken that returns EOS_Auth_IdToken::JsonWebToken.
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_EPIC_ID_TOKEN = 16,
	/**
	 * Amazon Access Token
	 *
	 * Supported with EOS_Connect_Login.
	 */
	EOS_ECT_AMAZON_ACCESS_TOKEN = 17
);

#pragma pack(pop)
