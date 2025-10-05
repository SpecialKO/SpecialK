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
     * The bit sum of the AvatarType.
     */
    typedef uint32_t AvatarCriteria;

    /**
     * The state of the user
     */
    enum PersonaState
    {
      PERSONA_STATE_OFFLINE, ///< User is not currently logged on.
      PERSONA_STATE_ONLINE ///< User is logged on.
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
       * @param [in] userID The ID of the user.
       * @param [in] personaStateChange The bit sum of the PersonaStateChange.
       */
      virtual void OnPersonaDataChanged(GalaxyID userID, uint32_t personaStateChange) = 0;
    };

    /**
     * Globally self-registering version of IPersonaDataChangedListener.
     */
    typedef SelfRegisteringListener<IPersonaDataChangedListener> GlobalPersonaDataChangedListener;

    /**
     * Listener for the event of retrieving requested user's information.
     */
    class IUserInformationRetrieveListener : public GalaxyTypeAwareListener<USER_INFORMATION_RETRIEVE_LISTENER>
    {
    public:

      /**
       * Notification for the event of a success in retrieving the user's information.
       *
       * @param [in] userID The ID of the user.
       */
      virtual void OnUserInformationRetrieveSuccess(GalaxyID userID) = 0;

      /**
       * The reason of a failure in retrieving the user's information.
       */
      enum FailureReason
      {
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in retrieving the user's information.
       *
       * @param [in] userID The ID of the user.
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnUserInformationRetrieveFailure(GalaxyID userID, FailureReason failureReason) = 0;
    };

    /**
     * Globally self-registering version of IUserInformationRetrieveListener.
     */
    typedef SelfRegisteringListener<IUserInformationRetrieveListener> GlobalUserInformationRetrieveListener;

    /**
     * Listener for the event of retrieving requested list of friends.
     */
    class IFriendListListener : public GalaxyTypeAwareListener<FRIEND_LIST_RETRIEVE>
    {
    public:

      /**
       * Notification for the event of a success in retrieving the user's list of friends.
       *
       * In order to read subsequent friend IDs, call IFriends::GetFriendCount() and IFriends::GetFriendByIndex().
       */
      virtual void OnFriendListRetrieveSuccess() = 0;

      /**
       * The reason of a failure in retrieving the user's list of friends.
       */
      enum FailureReason
      {
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in retrieving the user's list of friends.
       *
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnFriendListRetrieveFailure(FailureReason failureReason) = 0;
    };

    /**
     * Globally self-registering version of IFriendListListener.
     */
    typedef SelfRegisteringListener<IFriendListListener> GlobalFriendListListener;

    /**
     * Listener for the event of sending a friend invitation.
     */
    class IFriendInvitationSendListener : public GalaxyTypeAwareListener<FRIEND_INVITATION_SEND_LISTENER>
    {
    public:

      /**
       * Notification for the event of a success in sending a friend invitation.
       *
       * @param [in] userID The ID of the user to whom the invitation was being sent.
       */
      virtual void OnFriendInvitationSendSuccess(GalaxyID userID) = 0;

      /**
       * The reason of a failure in sending a friend invitation.
       */
      enum FailureReason
      {
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_USER_DOES_NOT_EXIST, ///< User does not exist.
        FAILURE_REASON_USER_ALREADY_INVITED, ///< Friend invitation already sent to the user.
        FAILURE_REASON_USER_ALREADY_FRIEND, ///< User already on the friend list.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in sending a friend invitation.
       *
       * @param [in] userID The ID of the user to whom the invitation was being sent.
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnFriendInvitationSendFailure(GalaxyID userID, FailureReason failureReason) = 0;
    };

    /**
     * Globally self-registering version of IFriendInvitationSendListener.
     */
    typedef SelfRegisteringListener<IFriendInvitationSendListener> GlobalFriendInvitationSendListener;

    /**
     * Listener for the event of retrieving requested list of incoming friend invitations.
     */
    class IFriendInvitationListRetrieveListener : public GalaxyTypeAwareListener<FRIEND_INVITATION_LIST_RETRIEVE_LISTENER>
    {
    public:

      /**
       * Notification for the event of a success in retrieving the user's list of incoming friend invitations.
       *
       * In order to read subsequent invitation IDs, call and IFriends::GetFriendInvitationByIndex().
       */
      virtual void OnFriendInvitationListRetrieveSuccess() = 0;

      /**
       * The reason of a failure in retrieving the user's list of incoming friend invitations.
       */
      enum FailureReason
      {
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in retrieving the user's list of incoming friend invitations.
       *
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnFriendInvitationListRetrieveFailure(FailureReason failureReason) = 0;

    };

    /**
     * Globally self-registering version of IFriendInvitationListRetrieveListener.
     */
    typedef SelfRegisteringListener<IFriendInvitationListRetrieveListener> GlobalFriendInvitationListRetrieveListener;

    /**
     * Listener for the event of retrieving requested list of outgoing friend invitations.
     */
    class ISentFriendInvitationListRetrieveListener : public GalaxyTypeAwareListener<SENT_FRIEND_INVITATION_LIST_RETRIEVE_LISTENER>
    {
    public:

      /**
       * Notification for the event of a success in retrieving the user's list of outgoing friend invitations.
       *
       * In order to read subsequent invitation IDs, call and IFriends::GetFriendInvitationByIndex().
       */
      virtual void OnSentFriendInvitationListRetrieveSuccess() = 0;

      /**
       * The reason of a failure in retrieving the user's list of outgoing friend invitations.
       */
      enum FailureReason
      {
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in retrieving the user's list of outgoing friend invitations.
       *
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnSentFriendInvitationListRetrieveFailure(FailureReason failureReason) = 0;
    };

    /**
     * Globally self-registering version of ISentFriendInvitationListRetrieveListener.
     */
    typedef SelfRegisteringListener<ISentFriendInvitationListRetrieveListener> GlobalSentFriendInvitationListRetrieveListener;

    /**
     * Listener for the event of receiving a friend invitation.
     */
    class IFriendInvitationListener : public GalaxyTypeAwareListener<FRIEND_INVITATION_LISTENER>
    {
    public:

      /**
       * Notification for the event of receiving a friend invitation.
       *
       * @param [in] userID The ID of the user who sent the friend invitation.
       * @param [in] sendTime The time at which the friend invitation was sent.
       */
      virtual void OnFriendInvitationReceived(GalaxyID userID, uint32_t sendTime) = 0;
    };

    /**
     * Globally self-registering version of IFriendInvitationListener.
     */
    typedef SelfRegisteringListener<IFriendInvitationListener> GlobalFriendInvitationListener;

    /**
     * Listener for the event of responding to a friend invitation.
     */
    class IFriendInvitationRespondToListener : public GalaxyTypeAwareListener<FRIEND_INVITATION_RESPOND_TO_LISTENER>
    {
    public:

      /**
       * Notification for the event of a success in responding to a friend invitation.
       *
       * @param [in] userID The ID of the user who sent the invitation.
       * @param [in] accept True when accepting the invitation, false when declining.
       */
      virtual void OnFriendInvitationRespondToSuccess(GalaxyID userID, bool accept) = 0;

      /**
       * The reason of a failure in responding to a friend invitation.
       */
      enum FailureReason
      {
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_USER_DOES_NOT_EXIST, ///< User does not exist.
        FAILURE_REASON_FRIEND_INVITATION_DOES_NOT_EXIST, ///< Friend invitation does not exist.
        FAILURE_REASON_USER_ALREADY_FRIEND, ///< User already on the friend list.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in responding to a friend invitation.
       *
       * @param [in] userID The ID of the user who sent the invitation.
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnFriendInvitationRespondToFailure(GalaxyID userID, FailureReason failureReason) = 0;
    };

    /**
     * Globally self-registering version of IFriendInvitationRespondToListener.
     */
    typedef SelfRegisteringListener<IFriendInvitationRespondToListener> GlobalFriendInvitationRespondToListener;

    /**
     * Listener for the event of a user being added to the friend list.
     */
    class IFriendAddListener : public GalaxyTypeAwareListener<FRIEND_ADD_LISTENER>
    {
    public:

      /**
       * The direction of the invitation that initiated the change in the friend list.
       */
      enum InvitationDirection
      {
        INVITATION_DIRECTION_INCOMING, ///< The user indicated in the notification was the inviter.
        INVITATION_DIRECTION_OUTGOING ///< The user indicated in the notification was the invitee.
      };

      /**
       * Notification for the event of a user being added to the friend list.
       *
       * @param [in] userID The ID of the user who has just been added to the friend list.
       * @param [in] invitationDirection The direction of the invitation that determines the inviter and the invitee.
       */
      virtual void OnFriendAdded(GalaxyID userID, InvitationDirection invitationDirection) = 0;
    };

    /**
     * Globally self-registering version of IFriendAddListener.
     */
    typedef SelfRegisteringListener<IFriendAddListener> GlobalFriendAddListener;

    /**
     * Listener for the event of removing a user from the friend list.
     */
    class IFriendDeleteListener : public GalaxyTypeAwareListener<FRIEND_DELETE_LISTENER>
    {
    public:

      /**
       * Notification for the event of a success in removing a user from the friend list.
       *
       * @param [in] userID The ID of the user requested to be removed from the friend list.
       */
      virtual void OnFriendDeleteSuccess(GalaxyID userID) = 0;

      /**
       * The reason of a failure in removing a user from the friend list.
       */
      enum FailureReason
      {
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in removing a user from the friend list.
       *
       * @param [in] userID The ID of the user requested to be removed from the friend list.
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnFriendDeleteFailure(GalaxyID userID, FailureReason failureReason) = 0;
    };

    /**
     * Globally self-registering version of IFriendDeleteListener.
     */
    typedef SelfRegisteringListener<IFriendDeleteListener> GlobalFriendDeleteListener;

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
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of failure to modify rich presence.
       *
       * @param [in] failureReason The cause of the failure.
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
       * @param [in] userID The ID of the user.
       */
      virtual void OnRichPresenceUpdated(GalaxyID userID) = 0;
    };

    /**
     * Globally self-registering version of IRichPresenceListener.
     */
    typedef SelfRegisteringListener<IRichPresenceListener> GlobalRichPresenceListener;

    /**
     * Listener for the event of retrieving requested user's rich presence.
     */
    class IRichPresenceRetrieveListener : public GalaxyTypeAwareListener<RICH_PRESENCE_RETRIEVE_LISTENER>
    {
    public:

      /**
       * Notification for the event of a success in retrieving the user's rich presence.
       *
       * @param [in] userID The ID of the user.
       */
      virtual void OnRichPresenceRetrieveSuccess(GalaxyID userID) = 0;

      /**
       * The reason of a failure in retrieving the user's rich presence.
       */
      enum FailureReason
      {
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in retrieving the user's rich presence.
       *
       * @param [in] userID The ID of the user.
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnRichPresenceRetrieveFailure(GalaxyID userID, FailureReason failureReason) = 0;
    };

    /**
     * Globally self-registering version of IRichPresenceRetrieveListener.
     */
    typedef SelfRegisteringListener<IRichPresenceRetrieveListener> GlobalRichPresenceRetrieveListener;

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
       * @param [in] userID The ID of the user who sent invitation.
       * @param [in] connectionString The string which contains connection info.
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
       * @param [in] userID The ID of the user who sent invitation.
       * @param [in] connectionString The string which contains connection info.
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
       * @param [in] userID The ID of the user to whom the invitation was being sent.
       * @param [in] connectionString The string which contains connection info.
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
        FAILURE_REASON_SENDER_BLOCKED, ///< Sender blocked by receiver. Will also occur if both users blocked each other.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in sending an invitation.
       *
       * @param [in] userID The ID of the user to whom the invitation was being sent.
       * @param [in] connectionString The string which contains connection info.
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnInvitationSendFailure(GalaxyID userID, const char* connectionString, FailureReason failureReason) = 0;
    };

    /**
     * Globally self-registering version of ISendInvitationListener.
     */
    typedef SelfRegisteringListener<ISendInvitationListener> GlobalSendInvitationListener;

    /**
     * Listener for the event of searching a user.
     */
    class IUserFindListener : public GalaxyTypeAwareListener<USER_FIND_LISTENER>
    {
    public:

      /**
       * Notification for the event of success in finding a user.
       *
       * @param [in] userSpecifier The user specifier by which user search was performed.
       * @param [in] userID The ID of the found user.
       */
      virtual void OnUserFindSuccess(const char* userSpecifier, GalaxyID userID) = 0;

      /**
       * The reason of a failure in searching a user.
       */
      enum FailureReason
      {
        FAILURE_REASON_UNDEFINED, ///< Unspecified error.
        FAILURE_REASON_USER_NOT_FOUND, ///< Specified user was not found.
        FAILURE_REASON_CONNECTION_FAILURE ///< Unable to communicate with backend services.
      };

      /**
       * Notification for the event of a failure in finding a user.
       *
       * @param [in] userSpecifier The user specifier by which user search was performed.
       * @param [in] failureReason The cause of the failure.
       */
      virtual void OnUserFindFailure(const char* userSpecifier, FailureReason failureReason) = 0;
    };

    /**
     * Globally self-registering version of IUserFindListener.
     */
    typedef SelfRegisteringListener<IUserFindListener> GlobalUserFindListener;

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
       * Returns the default avatar criteria.
       *
       * @return The bit sum of default AvatarType.
       */
      virtual AvatarCriteria GetDefaultAvatarCriteria() = 0;

      /**
       * Sets the default avatar criteria.
       *
       * @remark The avatar criteria will be used for automated downloads of user information,
       * as well as additional criteria in case of calling RequestUserInformation().
       *
       * @param [in] defaultAvatarCriteria The bit sum of default AvatarType.
       */
      virtual void SetDefaultAvatarCriteria(AvatarCriteria defaultAvatarCriteria) = 0;

      /**
       * Performs a request for information about specified user.
       *
       * This call is asynchronous. Responses come both to the IPersonaDataChangedListener
       * and to the IUserInformationRetrieveListener.
       *
       * @remark This call is performed automatically for friends (after requesting the list
       * of friends) and fellow lobby members (after entering a lobby or getting a notification
       * about some other user joining it), therefore in many cases there is no need for you
       * to call it manually and all you should do is wait for the appropriate callback
       * to come to the IPersonaDataChangedListener.
       *
       * @remark User avatar will be downloaded according to bit sum of avatarCriteria and
       * defaultAvatarCriteria set by calling SetDefaultAvatarCriteria().
       *
       * @param [in] userID The ID of the user.
       * @param [in] avatarCriteria The bit sum of the AvatarType.
       * @param [in] listener The listener for specific operation.
       */
      virtual void RequestUserInformation(
        GalaxyID userID,
        AvatarCriteria avatarCriteria = AVATAR_TYPE_NONE,
        IUserInformationRetrieveListener* const listener = NULL) = 0;

      /**
       * Checks if the information of specified user is available.
       *
       * The information can be retrieved by calling RequestUserInformation().
       *
       * @param [in] userID The ID of the user.
       * @return true if the information of the user is available, false otherwise.
       */
      virtual bool IsUserInformationAvailable(GalaxyID userID) = 0;

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
       * @param [in, out] buffer The output buffer.
       * @param [in] bufferLength The size of the output buffer.
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
       * @remark This call is not thread-safe as opposed to GetFriendPersonaNameCopy().
       *
       * @pre You might need to retrieve the data first by calling RequestUserInformation().
       *
       * @param [in] userID The ID of the user.
       * @return The nickname of the user.
       */
      virtual const char* GetFriendPersonaName(GalaxyID userID) = 0;

      /**
       * Copies the nickname of a specified user.
       *
       * @pre You might need to retrieve the data first by calling RequestUserInformation().
       *
       * @param [in] userID The ID of the user.
       * @param [in, out] buffer The output buffer.
       * @param [in] bufferLength The size of the output buffer.
       */
      virtual void GetFriendPersonaNameCopy(GalaxyID userID, char* buffer, uint32_t bufferLength) = 0;

      /**
       * Returns the state of a specified user.
       *
       * @pre You might need to retrieve the data first by calling RequestUserInformation().
       *
       * @param [in] userID The ID of the user.
       * @return The state of the user.
       */
      virtual PersonaState GetFriendPersonaState(GalaxyID userID) = 0;

      /**
       * Returns the URL of the avatar of a specified user.
       *
       * @remark This call is not thread-safe as opposed to GetFriendAvatarUrlCopy().
       *
       * @pre You might need to retrieve the data first by calling RequestUserInformation().
       *
       * @param [in] userID The ID of the user.
       * @param [in] avatarType The type of avatar.
       * @return The URL of the avatar.
       */
      virtual const char* GetFriendAvatarUrl(GalaxyID userID, AvatarType avatarType) = 0;

      /**
       * Copies URL of the avatar of a specified user.
       *
       * @pre You might need to retrieve the data first by calling RequestUserInformation().
       *
       * @param [in] userID The ID of the user.
       * @param [in] avatarType The type of avatar.
       * @param [in, out] buffer The output buffer.
       * @param [in] bufferLength The size of the output buffer.
       */
      virtual void GetFriendAvatarUrlCopy(GalaxyID userID, AvatarType avatarType, char* buffer, uint32_t bufferLength) = 0;

      /**
       * Returns the ID of the avatar of a specified user.
       *
       * @pre Retrieve the avatar image first by calling RequestUserInformation()
       * with appropriate avatar criteria.
       *
       * @param [in] userID The ID of the user.
       * @param [in] avatarType The type of avatar.
       * @return The ID of the avatar image.
       */
      virtual uint32_t GetFriendAvatarImageID(GalaxyID userID, AvatarType avatarType) = 0;

      /**
       * Copies the avatar of a specified user.
       *
       * @pre Retrieve the avatar image first by calling RequestUserInformation()
       * with appropriate avatar criteria.
       *
       * @pre The size of the output buffer should be 4 * height * width * sizeof(char).
       *
       * @param [in] userID The ID of the user.
       * @param [in] avatarType The type of avatar.
       * @param [in, out] buffer The output buffer.
       * @param [in] bufferLength The size of the output buffer.
       */
      virtual void GetFriendAvatarImageRGBA(GalaxyID userID, AvatarType avatarType, void* buffer, uint32_t bufferLength) = 0;

      /**
       * Checks if a specified avatar image is available.
       *
       * @param [in] userID The ID of the user.
       * @param [in] avatarType The type of avatar.
       * @return true if the specified avatar image is available, false otherwise.
       */
      virtual bool IsFriendAvatarImageRGBAAvailable(GalaxyID userID, AvatarType avatarType) = 0;

      /**
       * Performs a request for the user's list of friends.
       *
       * This call is asynchronous. Responses come to the IFriendListListener.
       *
       * @param [in] listener The listener for specific operation.
       */
      virtual void RequestFriendList(IFriendListListener* const listener = NULL) = 0;

      /**
       * Checks if a specified user is a friend.
       *
       * @pre Retrieve the list of friends first by calling RequestFriendList().
       *
       * @param [in] userID The ID of the user.
       * @return true if the specified user is a friend, false otherwise.
       */
      virtual bool IsFriend(GalaxyID userID) = 0;

      /**
       * Returns the number of retrieved friends in the user's list of friends.
       *
       * @pre Retrieve the list of friends first by calling RequestFriendList().
       *
       * @return The number of retrieved friends, or 0 if failed.
       */
      virtual uint32_t GetFriendCount() = 0;

      /**
       * Returns the GalaxyID for a friend.
       *
       * @pre Retrieve the list of friends first by calling RequestFriendList().
       *
       * @param [in] index Index as an integer in the range of [0, number of friends).
       * @return The GalaxyID of the friend.
       */
      virtual GalaxyID GetFriendByIndex(uint32_t index) = 0;

      /**
       * Sends a friend invitation.
       *
       * This call is asynchronous. Responses come to the IFriendInvitationSendListener.
       *
       * @param [in] userID The ID of the user.
       * @param [in] listener The listener for specific operation.
       */
      virtual void SendFriendInvitation(GalaxyID userID, IFriendInvitationSendListener* const listener = NULL) = 0;

      /**
       * Performs a request for the user's list of incoming friend invitations.
       *
       * This call is asynchronous. Responses come to the IFriendInvitationListRetrieveListener.
       *
       * @param [in] listener The listener for specific operation.
       */
      virtual void RequestFriendInvitationList(IFriendInvitationListRetrieveListener* const listener = NULL) = 0;

      /**
       * Performs a request for the user's list of outgoing friend invitations.
       *
       * This call is asynchronous. Responses come to the ISentFriendInvitationListRetrieveListener.
       *
       * @param [in] listener The listener for specific operation.
       */
      virtual void RequestSentFriendInvitationList(ISentFriendInvitationListRetrieveListener* const listener = NULL) = 0;

      /**
       * Returns the number of retrieved friend invitations.
       *
       * @remark This function can be used only in IFriendInvitationListRetrieveListener callback.
       *
       * @return The number of retrieved friend invitations, or 0 if failed.
       */
      virtual uint32_t GetFriendInvitationCount() = 0;

      /**
       * Reads the details of the friend invitation.
       *
       * @remark This function can be used only in IFriendInvitationListRetrieveListener callback.
       *
       * @param [in] index Index as an integer in the range of [0, number of friend invitations).
       * @param [out] userID The ID of the user who sent the invitation.
       * @param [out] sendTime The time at which the friend invitation was sent.
       */
      virtual void GetFriendInvitationByIndex(uint32_t index, GalaxyID& userID, uint32_t& sendTime) = 0;

      /**
       * Responds to the friend invitation.
       *
       * This call is asynchronous. Responses come to the IFriendInvitationRespondToListener.
       *
       * @param [in] userID The ID of the user who sent the friend invitation.
       * @param [in] accept True when accepting the invitation, false when declining.
       * @param [in] listener The listener for specific operation.
       */
      virtual void RespondToFriendInvitation(GalaxyID userID, bool accept, IFriendInvitationRespondToListener* const listener = NULL) = 0;

      /**
       * Removes a user from the friend list.
       *
       * This call in asynchronous. Responses come to the IFriendDeleteListener.
       *
       * @param [in] userID The ID of the user to be removed from the friend list.
       * @param [in] listener The listener for specific operation.
       */
      virtual void DeleteFriend(GalaxyID userID, IFriendDeleteListener* const listener = NULL) = 0;

      /**
       * Sets the variable value under a specified name.
       *
       * There are three keys that can be used:
       * - "status" - The description visible in Galaxy Client with the limit of 3000 bytes.
       * - "metadata" - The metadata that describes the status to other instances of the game with the limit of 2048 bytes.
       * - "connect" - The string which contains connection info with the limit of 4095 bytes.
       *   It can be regarded as a passive version of IFriends::SendInvitation() because
       *   it allows friends that notice the rich presence to join a multiplayer game.
       *
       * User must be signed in through Galaxy.
       *
       * Passing NULL value removes the entry.
       *
       * This call in asynchronous. Responses come to the IRichPresenceChangeListener.
       *
       * @param [in] key The name of the property of the user's rich presence.
       * @param [in] value The value of the property to set.
       * @param [in] listener The listener for specific operation.
       */
      virtual void SetRichPresence(const char* key, const char* value, IRichPresenceChangeListener* const listener = NULL) = 0;

      /**
       * Removes the variable value under a specified name.
       *
       * If the variable doesn't exist method call has no effect.
       *
       * This call in asynchronous. Responses come to the IRichPresenceChangeListener.
       *
       * @param [in] key The name of the variable to be removed.
       * @param [in] listener The listener for specific operation.
       */
      virtual void DeleteRichPresence(const char* key, IRichPresenceChangeListener* const listener = NULL) = 0;

      /**
       * Removes all rich presence data for the user.
       *
       * This call in asynchronous. Responses come to the IRichPresenceChangeListener.
       *
       * @param [in] listener The listener for specific operation.
       */
      virtual void ClearRichPresence(IRichPresenceChangeListener* const listener = NULL) = 0;

      /**
       * Performs a request for the user's rich presence.
       *
       * This call is asynchronous. Responses come both to the IRichPresenceListener
       * and IRichPresenceRetrieveListener.
       *
       * @param [in] userID The ID of the user.
       * @param [in] listener The listener for specific operation.
       */
      virtual void RequestRichPresence(GalaxyID userID = GalaxyID(), IRichPresenceRetrieveListener* const listener = NULL) = 0;

      /**
       * Returns the rich presence of a specified user.
       *
       * @remark This call is not thread-safe as opposed to GetRichPresenceCopy().
       *
       * @pre Retrieve the rich presence first by calling RequestRichPresence().
       *
       * @param [in] userID The ID of the user.
       * @param [in] key The name of the property of the user's rich presence.
       * @return The rich presence of the user.
       */
      virtual const char* GetRichPresence(const char* key, GalaxyID userID = GalaxyID()) = 0;

      /**
       * Copies the rich presence of a specified user to a buffer.
       *
       * @pre Retrieve the rich presence first by calling RequestRichPresence().
       *
       * @param [in] key The name of the property of the user's rich presence.
       * @param [in, out] buffer The output buffer.
       * @param [in] bufferLength The size of the output buffer.
       * @param [in] userID The ID of the user.
       */
      virtual void GetRichPresenceCopy(const char* key, char* buffer, uint32_t bufferLength, GalaxyID userID = GalaxyID()) = 0;

      /**
       * Returns the number of retrieved properties in user's rich presence.
       *
       * @param [in] userID The ID of the user.
       * @return The number of retrieved keys, or 0 if failed.
       */
      virtual uint32_t GetRichPresenceCount(GalaxyID userID = GalaxyID()) = 0;

      /**
       * Returns a property from the rich presence storage by index.
       *
       * @pre Retrieve the rich presence first by calling RequestRichPresence().
       *
       * @param [in] index Index as an integer in the range of [0, number of entries).
       * @param [in, out] key The name of the property of the rich presence storage.
       * @param [in] keyLength The length of the name of the property of the rich presence storage.
       * @param [in, out] value The value of the property of the rich presence storage.
       * @param [in] valueLength The length of the value of the property of the rich presence storage.
       * @param [in] userID The ID of the user.
       */
      virtual void GetRichPresenceByIndex(uint32_t index, char* key, uint32_t keyLength, char* value, uint32_t valueLength, GalaxyID userID = GalaxyID()) = 0;

      /**
       * Returns a key from the rich presence storage by index.
       *
       * @remark This call is not thread-safe as opposed to GetRichPresenceKeyByIndexCopy().
       *
       * @pre Retrieve the rich presence first by calling RequestRichPresence().
       *
       * @param [in] index Index as an integer in the range of [0, number of entries).
       * @param [in] userID The ID of the user.
       * @return The rich presence key under the index of the user.
       */
      virtual const char* GetRichPresenceKeyByIndex(uint32_t index, GalaxyID userID = GalaxyID()) = 0;

      /**
       * Copies a key from the rich presence storage by index to a buffer.
       *
       * @pre Retrieve the rich presence first by calling RequestRichPresence().
       *
       * @param [in] index Index as an integer in the range of [0, number of entries).
       * @param [in, out] buffer The output buffer.
       * @param [in] bufferLength The size of the output buffer.
       * @param [in] userID The ID of the user.
       */
      virtual void GetRichPresenceKeyByIndexCopy(uint32_t index, char* buffer, uint32_t bufferLength, GalaxyID userID = GalaxyID()) = 0;

      /**
       * Shows game invitation dialog that allows to invite users to game.
       *
       * If invited user accepts the invitation, the connection string
       * gets added to the command-line parameters for launching the game.
       * If the game is already running, the connection string comes to the IGameInvitationReceivedListener,
       * or to the IGameJoinRequestedListener if accepted by the user on the overlay.
       *
       * @pre For this call to work, the overlay needs to be initialized first.
       * To check whether the overlay is initialized, call IUtils::GetOverlayState().
       *
       * @param [in] connectionString The string which contains connection info with the limit of 4095 bytes.
       */
      virtual void ShowOverlayInviteDialog(const char* connectionString) = 0;

      /**
       * Sends a game invitation without using the overlay.
       *
       * This call is asynchronous. Responses come to the ISendInvitationListener.
       *
       * If invited user accepts the invitation, the connection string
       * gets added to the command-line parameters for launching the game.
       * If the game is already running, the connection string comes to the IGameInvitationReceivedListener,
       * or to the IGameJoinRequestedListener if accepted by the user on the overlay.
       *
       * @param [in] userID The ID of the user.
       * @param [in] connectionString The string which contains connection info with the limit of 4095 bytes.
       * @param [in] listener The listener for specific operation.
       */
      virtual void SendInvitation(GalaxyID userID, const char* connectionString, ISendInvitationListener* const listener = NULL) = 0;

      /**
       * Finds a specified user.
       *
       * This call is asynchronous. Responses come to the IUserFindListener.
       *
       * Searches for the user given either a username or an email address.
       * Only exact match will be returned.
       *
       * @param [in] userSpecifier The specifier of the user.
       * @param [in] listener The listener for specific operation.
       */
      virtual void FindUser(const char* userSpecifier, IUserFindListener* const listener = NULL) = 0;

      /**
       * Checks if a specified user is playing the same game.
       *
       * @pre Retrieve the rich presence first by calling RequestRichPresence().
       *
       * @param [in] userID The ID of the user.
       * @return true if the specified user is playing the same game, false otherwise.
       */
      virtual bool IsUserInTheSameGame(GalaxyID userID) const = 0;
    };

    /** @} */
  }
}

#endif