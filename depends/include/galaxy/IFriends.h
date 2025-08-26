#ifndef GALAXY_I_FRIENDS_H
#define GALAXY_I_FRIENDS_H

/**
 * @file
 * Contains data structures and interfaces related to social activities.
 */

#include "GalaxyID.h"
#include "IListenerRegistrar.h"

namespace galaxy
{
	namespace api
	{
		/**
		 * @addtogroup api
		 * @{
		 */

		/**
		 * The type of avatar.
		 */
		enum AvatarType
		{
			AVATAR_TYPE_NONE = 0x0000, ///< No avatar type specified.
			AVATAR_TYPE_SMALL = 0x0001, ///< Avatar resolution size: 32x32.
			AVATAR_TYPE_MEDIUM = 0x0002, ///< Avatar resolution size: 64x64.
			AVATAR_TYPE_LARGE = 0x0004 ///< Avatar resolution size: 184x184.
		};

		/**
		 * The state of the user
		 */
		enum PersonaState
		{
			PERSONA_STATE_OFFLINE = 0, ///< User is not currently logged on.
			PERSONA_STATE_ONLINE = 1 ///< User is logged on.
		};

		/**
		 * Listener for the event of changing persona data.
		 */
		class IPersonaDataChangedListener : public GalaxyTypeAwareListener<PERSONA_DATA_CHANGED>
		{
		public:

			/**
			 * Describes what has changed about a user.
			 */
			enum PersonaStateChange
			{
				PERSONA_CHANGE_NONE = 0x0000, ///< No information has changed.
				PERSONA_CHANGE_NAME = 0x0001, ///< Persona name has changed.
				PERSONA_CHANGE_AVATAR = 0x0002, ///< Avatar, i.e. its URL, has changed.
				PERSONA_CHANGE_AVATAR_DOWNLOADED_IMAGE_SMALL = 0x0004, ///< Small avatar image has been downloaded.
				PERSONA_CHANGE_AVATAR_DOWNLOADED_IMAGE_MEDIUM = 0x0008, ///< Medium avatar image has been downloaded.
				PERSONA_CHANGE_AVATAR_DOWNLOADED_IMAGE_LARGE = 0x0010, ///< Large avatar image has been downloaded.
				PERSONA_CHANGE_AVATAR_DOWNLOADED_IMAGE_ANY = PERSONA_CHANGE_AVATAR_DOWNLOADED_IMAGE_SMALL | PERSONA_CHANGE_AVATAR_DOWNLOADED_IMAGE_MEDIUM | PERSONA_CHANGE_AVATAR_DOWNLOADED_IMAGE_LARGE ///< Any avatar images have been downloaded.
			};

			/**
			 * Notification for the event of changing persona data.
			 *
			 * @param userID The ID of the user.
			 * @param personaStateChange The bit sum of the PersonaStateChange.
			 */
			virtual void OnPersonaDataChanged(GalaxyID userID, uint32_t personaStateChange) = 0;
		};

		/**
		 * Globally self-registering version of IPersonaDataChangedListener.
		 */
		typedef SelfRegisteringListener<IPersonaDataChangedListener> GlobalPersonaDataChangedListener;

		/**
		 * Listener for the event of retrieving requested list of friends.
		 */
		class IFriendListListener : public GalaxyTypeAwareListener<FRIEND_LIST_RETRIEVE>
		{
		public:

			/**
			 * Notification for the event of a success in retrieving the user's list of friends.
			 *
			 * In order to read subsequent friend IDs, call GetFriendCount() and GetFriendByIndex().
			 */
			virtual void OnFriendListRetrieveSuccess() = 0;

			/**
			 * The reason of a failure in retrieving the user's list of friends.
			 */
			enum FailureReason
			{
				FAILURE_REASON_UNDEFINED ///< Unspecified error.
			};

			/**
			 * Notification for the event of a failure in retrieving the user's list of friends.
			 *
			 * @param failureReason The cause of the failure.
			 */
			virtual void OnFriendListRetrieveFailure(FailureReason failureReason) = 0;
		};

		/**
		 * Globally self-registering version of IFriendListListener.
		 */
		typedef SelfRegisteringListener<IFriendListListener> GlobalFriendListListener;

		/**
		 * Listener for the event of rich presence modification.
		 */
		class IRichPresenceChangeListener : public GalaxyTypeAwareListener<RICH_PRESENCE_CHANGE_LISTENER>
		{
		public:

			/**
			 * Notification for the event of successful rich presence change.
			 */
			virtual void OnRichPresenceChangeSuccess() = 0;

			/**
			 * The reason of a failure in rich presence modification.
			 */
			enum FailureReason
			{
				FAILURE_REASON_UNDEFINED ///< Unspecified error.
			};

			/**
			 * Notification for the event of failure to modify rich presence.
			 *
			 * @param failureReason The cause of the failure.
			 */
			virtual void OnRichPresenceChangeFailure(FailureReason failureReason) = 0;
		};

		/**
		 * Globally self-registering version of IRichPresenceChangeListener.
		 */
		typedef SelfRegisteringListener<IRichPresenceChangeListener> GlobalRichPresenceChangeListener;

		/**
		 * Listener for the event of any user rich presence update.
		 */
		class IRichPresenceListener : public GalaxyTypeAwareListener<RICH_PRESENCE_LISTENER>
		{
		public:

			/**
			 * Notification for the event of successful rich presence update.
			 *
			 * @param userID The ID of the user.
			 */
			virtual void OnRichPresenceUpdated(GalaxyID userID) = 0;
		};

		/**
		 * Globally self-registering version of IRichPresenceListener.
		 */
		typedef SelfRegisteringListener<IRichPresenceListener> GlobalRichPresenceListener;

		/**
		 * Event of requesting a game join by user.
		 *
		 * This can be triggered by accepting a game invitation
		 * or by user's request to join a friend's game.
		 */
		class IGameJoinRequestedListener : public GalaxyTypeAwareListener<GAME_JOIN_REQUESTED_LISTENER>
		{
		public:

			/**
			 * Notification for the event of accepting a game invitation.
			 *
			 * @param userID The ID of the user who sent invitation.
			 * @param connectionString The string which contains connection info.
			 */
			virtual void OnGameJoinRequested(GalaxyID userID, const char* connectionString) = 0;
		};

		/**
		 * Globally self-registering version of IGameJoinRequestedListener.
		 */
		typedef SelfRegisteringListener<IGameJoinRequestedListener> GlobalGameJoinRequestedListener;

		/**
		 * Event of receiving a game invitation.
		 *
		 * This can be triggered by receiving connection string.
		 */
		class IGameInvitationReceivedListener : public GalaxyTypeAwareListener<GAME_INVITATION_RECEIVED_LISTENER>
		{
		public:

			/**
			 * Notification for the event of receiving a game invitation.
			 *
			 * @param userID The ID of the user who sent invitation.
			 * @param connectionString The string which contains connection info.
			 */
			virtual void OnGameInvitationReceived(GalaxyID userID, const char* connectionString) = 0;
		};

		/**
		 * Globally self-registering version of IGameInvitationReceivedListener.
		 */
		typedef SelfRegisteringListener<IGameInvitationReceivedListener> GlobalGameInvitationReceivedListener;

		/**
		 * Listener for the event of sending an invitation without using the overlay.
		 */
		class ISendInvitationListener : public GalaxyTypeAwareListener<INVITATION_SEND>
		{
		public:

			/**
			 * Notification for the event of success in sending an invitation.
			 *
			 * @param userID The ID of the user to whom the invitation was being sent.
			 * @param connectionString The string which contains connection info.
			 */
			virtual void OnInvitationSendSuccess(GalaxyID userID, const char* connectionString) = 0;

			/**
			 * The reason of a failure in sending an invitation.
			 */
			enum FailureReason
			{
				FAILURE_REASON_UNDEFINED, ///< Unspecified error.
				FAILURE_REASON_USER_DOES_NOT_EXIST, ///< Receiver does not exist.
				FAILURE_REASON_RECEIVER_DOES_NOT_ALLOW_INVITING, ///< Receiver does not allow inviting
				FAILURE_REASON_SENDER_DOES_NOT_ALLOW_INVITING, ///< Sender does not allow inviting
				FAILURE_REASON_RECEIVER_BLOCKED, ///< Receiver blocked by sender.
				FAILURE_REASON_SENDER_BLOCKED ///< Sender blocked by receiver. Will also occur if both users blocked each other.
			};

			/**
			 * Notification for the event of a failure in sending an invitation.
			 *
			 * @param userID The ID of the user to whom the invitation was being sent.
			 * @param connectionString The string which contains connection info.
			 * @param failureReason The cause of the failure.
			 */
			virtual void OnInvitationSendFailure(GalaxyID userID, const char* connectionString, FailureReason failureReason) = 0;
		};

		/**
		 * Globally self-registering version of ISendInvitationListener.
		 */
		typedef SelfRegisteringListener<ISendInvitationListener> GlobalSendInvitationListener;

		/**
		 * The interface for managing social info and activities.
		 */
		class IFriends
		{
		public:

			virtual ~IFriends()
			{
			}

			/**
			 * Performs a request for information about specified user.
			 *
			 * This call is asynchronous. Responses come to the IPersonaDataChangedListener.
			 *
			 * @remark This call is performed automatically for friends (after requesting the list
			 * of friends) and fellow lobby members (after entering a lobby or getting a notification
			 * about some other user joining it), therefore in many cases there is no need for you
			 * to call it manually and all you should do is wait for the appropriate callback
			 * to come to the IPersonaDataChangedListener.
			 *
			 * @param userID The ID of the user.
			 * @param avatarCriteria The bit sum of the AvatarType.
			 */
			virtual void RequestUserInformation(GalaxyID userID, uint32_t avatarCriteria = AVATAR_TYPE_NONE) = 0;

			/**
			 * Returns the user's nickname.
			 *
			 * @remark This call is not thread-safe as opposed to GetPersonaNameCopy().
			 *
			 * @return The nickname of the user.
			 */
			virtual const char* GetPersonaName() = 0;

			/**
			 * Copies the user's nickname to a buffer.
			 *
			 * @param buffer The output buffer.
			 * @param bufferLength The size of the output buffer.
			 */
			virtual void GetPersonaNameCopy(char* buffer, uint32_t bufferLength) = 0;

			/**
			 * Returns the user's state.
			 *
			 * @return The state of the user.
			 */
			virtual PersonaState GetPersonaState() = 0;

			/**
			 * Returns the nickname of a specified user.
			 *
			 * @remark You might need to retrieve the data first by calling RequestUserInformation().
			 * @remark This call is not thread-safe as opposed to GetFriendPersonaNameCopy().
			 *
			 * @param userID The ID of the user.
			 * @return The nickname of the user.
			 */
			virtual const char* GetFriendPersonaName(GalaxyID userID) = 0;

			/**
			 * Copies the nickname of a specified user.
			 *
			 * @remark You might need to retrieve the data first by calling RequestUserInformation().
			 *
			 * @param userID The ID of the user.
			 * @param buffer The output buffer.
			 * @param bufferLength The size of the output buffer.
			 */
			virtual void GetFriendPersonaNameCopy(GalaxyID userID, char* buffer, uint32_t bufferLength) = 0;

			/**
			 * Returns the state of a specified user.
			 *
			 * @remark You might need to retrieve the data first by calling RequestUserInformation().
			 *
			 * @param userID The ID of the user.
			 * @return The state of the user.
			 */
			virtual PersonaState GetFriendPersonaState(GalaxyID userID) = 0;

			/**
			 * Returns the URL of the avatar of a specified user.
			 *
			 * @remark You might need to retrieve the data first by calling RequestUserInformation().
			 * @remark This call is not thread-safe as opposed to GetFriendAvatarUrlCopy().
			 *
			 * @param userID The ID of the user.
			 * @param avatarType The type of avatar.
			 * @return The URL of the avatar.
			 */
			virtual const char* GetFriendAvatarUrl(GalaxyID userID, AvatarType avatarType) = 0;

			/**
			 * Copies URL of the avatar of a specified user.
			 *
			 * @remark You might need to retrieve the data first by calling RequestUserInformation().
			 *
			 * @param userID The ID of the user.
			 * @param avatarType The type of avatar.
			 * @param buffer The output buffer.
			 * @param bufferLength The size of the output buffer.
			 */
			virtual void GetFriendAvatarUrlCopy(GalaxyID userID, AvatarType avatarType, char* buffer, uint32_t bufferLength) = 0;

			/**
			 * Returns the ID of the avatar of a specified user.
			 *
			 * @remark Retrieve the avatar image first by calling RequestUserInformation()
			 * with appropriate avatar criteria.
			 *
			 * @param userID The ID of the user.
			 * @param avatarType The type of avatar.
			 * @return The ID of the avatar image.
			 */
			virtual uint32_t GetFriendAvatarImageID(GalaxyID userID, AvatarType avatarType) = 0;

			/**
			 * Copies the avatar of a specified user.
			 *
			 * @remark Retrieve the avatar image first by calling RequestUserInformation()
			 * with appropriate avatar criteria.
			 * @remark The size of the output buffer should be 4 * height * width * sizeof(char).
			 *
			 * @param userID The ID of the user.
			 * @param avatarType The type of avatar.
			 * @param buffer The output buffer.
			 * @param bufferLength The size of the output buffer.
			 */
			virtual void GetFriendAvatarImageRGBA(GalaxyID userID, AvatarType avatarType, unsigned char* buffer, uint32_t bufferLength) = 0;

			/**
			 * Checks if a specified avatar image is available.
			 *
			 * @param userID The ID of the user.
			 * @param avatarType The type of avatar.
			 * @return true if the specified avatar image is available, false otherwise.
			 */
			virtual bool IsFriendAvatarImageRGBAAvailable(GalaxyID userID, AvatarType avatarType) = 0;

			/**
			 * Performs a request for the user's list of friends.
			 *
			 * This call is asynchronous. Responses come to the IFriendListListener.
			 */
			virtual void RequestFriendList() = 0;

			/**
			 * Checks if a specified user is a friend.
			 *
			 * @remark Retrieve the list of friends first by calling RequestFriendList().
			 *
			 * @param userID The ID of the user.
			 * @return true if the specified user is a friend, false otherwise.
			 */
			virtual bool IsFriend(GalaxyID userID) = 0;

			/**
			 * Returns the number of retrieved friends in the user's list of friends.
			 *
			 * @remark Retrieve the list of friends first by calling RequestFriendList().
			 *
			 * @return The number of retrieved friends, or 0 if failed.
			 */
			virtual uint32_t GetFriendCount() = 0;

			/**
			 * Returns the GalaxyID for a friend.
			 *
			 * @remark Retrieve the list of friends first by calling RequestFriendList().
			 *
			 * @param index Index as an integer in the range of [0, number of friends).
			 * @return The GalaxyID of the friend.
			 */
			virtual GalaxyID GetFriendByIndex(uint32_t index) = 0;

			/**
			 * Sets the variable value under a specified name.
			 *
			 * There are two keys that can be used:
			 * - "status" - will be visible in Galaxy Client with the limit of 3000 bytes.
			 * - "connect" - should contain connection string that allows to join a multiplayer
			 * game when passed to game as a parameter with the limit of 4095 bytes.
			 *
			 * Passing NULL value removes the entry.
			 *
			 * This call in asynchronous. Responses come to the IRichPresenceChangeListener.
			 *
			 * @param key The name of the property of the user's rich presence.
			 * @param value The value of the property to set.
			 */
			virtual void SetRichPresence(const char* key, const char* value) = 0;

			/**
			 * Removes the variable value under a specified name.
			 *
			 * If the variable doesn't exist method call has no effect.
			 *
			 * This call in asynchronous. Responses come to the IRichPresenceChangeListener.
			 *
			 * @param key The name of the variable to be removed.
			 */
			virtual void DeleteRichPresence(const char* key) = 0;

			/**
			 * Removes all rich presence data for the user.
			 *
			 * This call in asynchronous. Responses come to the IRichPresenceChangeListener.
			 */
			virtual void ClearRichPresence() = 0;

			/**
			 * Performs a request for the user's rich presence.
			 *
			 * This call is asynchronous. Responses come to the IRichPresenceListener.
			 *
			 * @param userID The ID of the user.
			 */
			virtual void RequestRichPresence(GalaxyID userID = GalaxyID()) = 0;

			/**
			 * Returns the rich presence of a specified user.
			 *
			 * @remark Retrieve the rich presence first by calling RequestRichPresence().
			 * @remark This call is not thread-safe as opposed to GetRichPresenceCopy().
			 *
			 * @param userID The ID of the user.
			 * @param key The name of the property of the user's rich presence.
			 * @return The rich presence of the user.
			 */
			virtual const char* GetRichPresence(const char* key, GalaxyID userID = GalaxyID()) = 0;

			/**
			 * Copies the rich presence of a specified user to a buffer.
			 *
			 * @remark Retrieve the rich presence first by calling RequestRichPresence().
			 *
			 * @param userID The ID of the user.
			 * @param key The name of the property of the user's rich presence.
			 * @param buffer The output buffer.
			 * @param bufferLength The size of the output buffer.
			 */
			virtual void GetRichPresenceCopy(const char* key, char* buffer, uint32_t bufferLength, GalaxyID userID = GalaxyID()) = 0;

			/**
			* Returns the number of retrieved properties in user's rich presence.
			*
			* @param userID The ID of the user.
			* @return The number of retrieved keys, or 0 if failed.
			*/
			virtual uint32_t GetRichPresenceCount(GalaxyID userID = GalaxyID()) = 0;

			/**
			 * Returns a property from the rich presence storage by index.
			 *
			 * @remark Retrieve the rich presence first by calling RequestRichPresence().
			 *
			 * @param index Index as an integer in the range of [0, number of entries).
			 * @param key The name of the property of the rich presence storage.
			 * @param keyLength The length of the name of the property of the rich presence storage.
			 * @param value The value of the property of the rich presence storage.
			 * @param valueLength The length of the value of the property of the rich presence storage.
			 * @param userID The ID of the user.
			 * @return true if succeeded, false when there is no such property.
			 */
			virtual bool GetRichPresenceByIndex(uint32_t index, char* key, uint32_t keyLength, char* value, uint32_t valueLength, GalaxyID userID = GalaxyID()) = 0;

			/**
			 * Shows game invitation dialog that allows to invite users to game.
			 *
			 * If invited user accepts the invitation, the connection string
			 * gets added to the command-line parameters for launching the game.
			 * If the game is already running, the connection string comes
			 * to the IGameJoinRequestedListener instead.
			 *
			 * @param connectionString The string which contains connection info with the limit of 4095 bytes.
			 **/
			virtual void ShowOverlayInviteDialog(const char* connectionString) = 0;

			/**
			 * Sends invitation without using the overlay.
			 *
			 * @param userID The ID of the user.
			 * @param connectionString The string which contains connection info with the limit of 4095 bytes.
			 */
			virtual void SendInvitation(GalaxyID userID, const char* connectionString) = 0;
		};

		/** @} */
	}
}

#endif